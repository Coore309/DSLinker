#include <dslinker.h>
#include <thread>

namespace dslinker {
	DSLinker::DSLinker(std::string apikey) : apikey_(apikey), client_(base_url_), alive_(false), stop_(false), threadRequest(nullptr){
		client_.set_bearer_token_auth(apikey_);
		client_.set_read_timeout(660);
	}

	DSLinker::~DSLinker() {
		stopRequest();
	}

	// 以非流式传入请求体json字符串，返回回应
	bool DSLinker::requestChatNoStream(nlohmann::json jsonRequest) {
		if (hasProccess()) return false;

		openAlive();
		resetStop();
		threadRequest = new std::thread([this](nlohmann::json jsonRequest) {
			auto res = client_.Post(chat_path_, jsonRequest.dump(), "application/json");
			if (orderStop()) {
				pushWords(-3, 0, "", "");
				closeAlive();
				return;
			}

			if (!res) {
				pushWords(-2, 0, "", "");
				closeAlive();
				return;
			}

			if (res->status != 200) {
				std::string error_message;
				switch (res->status) {
				case 400:
					error_message = "400：格式错误。原因：请求体格式错误。解决方法：请根据错误信息提示修改请求体。";
					break;
				case 401:
					error_message = "401：认证失败。原因：API key 错误，认证失败。解决方法：请检查您的 API key 是否正确，如没有 API key，请先创建 API key。";
					break;
				case 402:
					error_message = "402：余额不足。原因：账号余额不足。解决方法：请确认账户余额，并前往充值页面进行充值。";
					break;
				case 422:
					error_message = "422：参数错误。原因：请求体参数错误。解决方法：请根据错误信息提示修改相关参数。";
					break;
				case 429:
					error_message = "429：请求速率达到上限。原因：请求速率（TPM 或 RPM）达到上限。解决方法：请合理规划您的请求速率。";
					break;
				case 500:
					error_message = "500：服务器故障。原因：服务器内部故障。解决方法：请等待后重试。若问题一直存在，请联系我们解决。";
					break;
				case 503:
					error_message = "503：服务器繁忙。原因：服务器负载过高。解决方法：请稍后重试您的请求。";
					break;
				default:
					error_message = "其它错误。";
					break;
				}

				pushWords(-1, res->status, error_message, res->body);
				closeAlive();
				return;
			}

			try {
				nlohmann::json jsonResponse = nlohmann::json::parse(res->body);
				pushJson(jsonResponse);
				auto& message = jsonResponse["choices"][0]["message"];
				pushWords(0, 0, message["reasoning_content"].is_null() ? "" : message["reasoning_content"], message["content"].is_null() ? "" : message["content"]);
			}
			catch (nlohmann::json::parse_error& e) {
				pushWords(-4, 0, std::string("Json parse 发生错误：") + e.what(), res->body);
			}

			closeAlive();
			return;
		}, jsonRequest);

		return true;
	}

	bool DSLinker::requestChatStream(nlohmann::json jsonRequest) {
		if (hasProccess()) return false;

		openAlive();
		resetStop();

		threadRequest = new std::thread([this](nlohmann::json jsonRequest) {
			httplib::Headers headers = {
				{"Accept", "text/event-stream"},
				{"Cache-Control", "no-cache"}
			};

			std::string bufRsp;
			const std::string prefix = "data: ";

			auto res = client_.Post(chat_path_, headers, jsonRequest.dump(), "application/json", [this, &bufRsp, &prefix](const char* data, size_t len) {
				bufRsp.append(data, len);

				size_t pos = 0;
				while ((pos = bufRsp.find('\n')) != std::string::npos) {
					std::string line = bufRsp.substr(0, pos);
					bufRsp.erase(0, pos + 1);

					if (!line.empty() && (line.back() == '\r')) line.pop_back();

					if (line.empty() || line[0] == ':') continue;

					if (line.compare(0, prefix.length(), prefix) != 0) continue;

					std::string dataContent = line.substr(prefix.length());

					if (dataContent == "[DONE]") {
						pushWords(0, 0, "", "[DONE]");
						return false;
					}

					try {
						nlohmann::json jsonResponse = nlohmann::json::parse(dataContent);
						pushJson(jsonResponse);
						if (jsonResponse["choices"][0]["finish_reason"].is_null()) {
							auto& delta = jsonResponse["choices"][0]["delta"];
							pushWords(1, 0, delta["reasoning_content"].is_null() ? "" : delta["reasoning_content"], delta["content"].is_null() ? "" : delta["content"]);
						}
					}
					catch (nlohmann::json::parse_error& e) {
						pushWords(-4, 0, std::string("Json parse 发生错误：") + e.what(), dataContent);
						return false;
					}
				}

				return true;
			});

			if (!res) {
				pushWords(-2, 0, "", "");
				closeAlive();
				return;
			}

			if (res->status != 200) {
				std::string error_message;
				switch (res->status) {
				case 400:
					error_message = "400：格式错误。原因：请求体格式错误。解决方法：请根据错误信息提示修改请求体。";
					break;
				case 401:
					error_message = "401：认证失败。原因：API key 错误，认证失败。解决方法：请检查您的 API key 是否正确，如没有 API key，请先创建 API key。";
					break;
				case 402:
					error_message = "402：余额不足。原因：账号余额不足。解决方法：请确认账户余额，并前往充值页面进行充值。";
					break;
				case 422:
					error_message = "422：参数错误。原因：请求体参数错误。解决方法：请根据错误信息提示修改相关参数。";
					break;
				case 429:
					error_message = "429：请求速率达到上限。原因：请求速率（TPM 或 RPM）达到上限。解决方法：请合理规划您的请求速率。";
					break;
				case 500:
					error_message = "500：服务器故障。原因：服务器内部故障。解决方法：请等待后重试。若问题一直存在，请联系我们解决。";
					break;
				case 503:
					error_message = "503：服务器繁忙。原因：服务器负载过高。解决方法：请稍后重试您的请求。";
					break;
				default:
					error_message = "其它错误。";
					break;
				}

				pushWords(-1, res->status, error_message, res->body);
				closeAlive();
				return;
			}

			if (orderStop()) {
				pushWords(-3, 0, "", "");
				closeAlive();
				return;
			}

			closeAlive();
		}, jsonRequest);


		return true;
	}

	// 获取回复增量
	ModelAnswer DSLinker::popWords(void) {
		std::lock_guard<std::mutex> lockStream(mtxStream_);
		if (wordStream_.empty()) return { 2,0,"","" };
		
		ModelAnswer deltaMsg = wordStream_.front();
		wordStream_.pop();
		
		return deltaMsg;
	}

	// 获取当前模型工作状态，若工作正在进行，则返回 true
	bool DSLinker::hasProccess(void) {
		std::lock_guard<std::mutex> lockAlive(mtxAlive_);
		return alive_;
	}

	// 获取增量组是否有未处理的新增量，若还有新增量，则返回 true
	bool DSLinker::hasWords(void) {
		std::lock_guard<std::mutex> lockStream(mtxStream_);
		return !wordStream_.empty();
	}

	bool DSLinker::hasJsons(void) {
		std::lock_guard<std::mutex> lockStream(mtxStream_);
		return !Jsons_.empty();
	}

	// 获取最后一次请求的原始 json。若是非流式则只有一个；若是流式则是若干个，需要多次调用该函数获取。当返回空字符串时代表本轮 json 已输出完毕。
	nlohmann::json DSLinker::popResponseJsons(void) {
		std::lock_guard<std::mutex> lockStream(mtxStream_);
		if (Jsons_.empty()) return {};

		nlohmann::json jsn = Jsons_.front();
		Jsons_.pop();

		return jsn;
	}

	void DSLinker::clearQueues(void) {
		std::lock_guard<std::mutex> lockStream(mtxStream_);
		Jsons_ = {};
		wordStream_ = {};
	}

	void DSLinker::pushWords(int statu, int error, std::string thought, std::string answer) {
		std::lock_guard<std::mutex> lockStream(mtxStream_);
		wordStream_.push({ statu, error, thought, answer });
	}

	void DSLinker::pushJson(nlohmann::json jR) {
		std::lock_guard<std::mutex> lockStream(mtxStream_);
		Jsons_.push(jR);
	}

	void DSLinker::openAlive(void) {
		std::lock_guard<std::mutex> lockAlive(mtxAlive_);
		alive_ = true;
	}

	void DSLinker::closeAlive(void) {
		std::lock_guard<std::mutex> lockAlive(mtxAlive_);
		alive_ = false;
	}

	void DSLinker::setStop(void) {
		std::lock_guard<std::mutex> lockAlive(mtxStop_);
		stop_ = true;
	}

	void DSLinker::resetStop(void) {
		std::lock_guard<std::mutex> lockAlive(mtxStop_);
		stop_ = false;
	}

	void DSLinker::stopRequest(void) {
		setStop();
		if(hasProccess()) client_.stop();
		if (threadRequest && threadRequest->joinable()) {
			threadRequest->join();
		}
		delete threadRequest;
		threadRequest = nullptr;
		closeAlive();
	}

	bool DSLinker::orderStop(void) {
		std::lock_guard<std::mutex> lockAlive(mtxStop_);
		return stop_;
	}

	bool DSLinker::waitRequest(void) {
		if (threadRequest && threadRequest->joinable()) {
			threadRequest->join();
			return true;
		}
		return false;
	}
}