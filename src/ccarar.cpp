#include <dslinker.h>
#include <cmath>

namespace dslinker{
	std::string packChatRequest(ChatRequestBody rb) {
		nlohmann::json jRequest;

		// messages 装填
		for (const auto& i : rb.messages) {
			nlohmann::json msg;
			switch (i.role) {
			case Role::system:
				msg["role"] = "system";
				break;
			case Role::user:
				msg["role"] = "user";
				break;
			case Role::assistant:
				msg["role"] = "assistant";
				break;
			case Role::tool:
				msg["role"] = "tool";
			}
			msg["content"] = i.content;

			if (i.name.has_value()) msg["name"] = i.name.value();
			if (i.prefix.has_value()) msg["prefix"] = i.prefix.value();
			if (i.reasoning_content.has_value()) msg["reasoning_content"] = i.reasoning_content.value();
			if (i.tool_call_id.has_value()) msg["tool_call_id"] = i.tool_call_id.value();

			jRequest["messages"].push_back(msg);
		}

		switch (rb.model) {
		case ChatModel::deepseek_v4_flash:
			jRequest["model"] = "deepseek-v4-flash";
			break;
		case ChatModel::deepseek_v4_pro:
			jRequest["model"] = "deepseek-v4-pro";
			break;
		}

		if (rb.thinking.has_value()) {
			switch (rb.thinking.value()) {
			case OptionsThinkingType::enabled:
				jRequest["thinking"]["type"] = "enabled";
				break;
			case OptionsThinkingType::disabled:
				jRequest["thinking"]["type"] = "disabled";
				break;
			}
		}

		if (rb.reasoning_effort.has_value()) {
			switch (rb.reasoning_effort.value()) {
			case OptionsReasoningEffort::high:
				jRequest["reasoning_effort"] = "high";
				break;
			case OptionsReasoningEffort::low:
				jRequest["reasoning_effort"] = "low";
				break;
			case OptionsReasoningEffort::max:
				jRequest["reasoning_effort"] = "max";
				break;
			}
		}

		if (rb.max_tokens.has_value()) jRequest["max_tokens"] = rb.max_tokens.value();

		if (rb.response_format.has_value()) {
			switch (rb.response_format.value()) {
			case OptionsResponseFormatType::text:
				jRequest["response_format"]["type"] = "text";
				break;
			case OptionsResponseFormatType::json_object:
				jRequest["response_format"]["type"] = "json_object";
				break;
			}
		}

		if (rb.stop.has_value())
			for (auto i : rb.stop.value())
				jRequest["stop"].push_back(i);

		if (rb.stream.has_value()) jRequest["stream"] = rb.stream.value();

		if (rb.include_usage.has_value()) jRequest["stream_options"]["include_usage"] = rb.include_usage.value();

		if (rb.temperature.has_value()) jRequest["temperature"] = std::round(rb.temperature.value() * 100.0) / 100.0;

		if (rb.top_p.has_value()) jRequest["top_p"] = std::round(rb.top_p.value() * 100.0) / 100.0;

		if (rb.tools.has_value()) jRequest["tools"] = rb.tools.value().getToolsList();

		if (rb.tool_choice.has_value()) {
			if (rb.tool_choice.value() != OptionsToolChoice::Function) {
				switch (rb.tool_choice.value()) {
				case OptionsToolChoice::None:
					jRequest["tool_choice"] = "none";
					break;
				case OptionsToolChoice::Auto:
					jRequest["tool_choice"] = "auto";
					break;
				case OptionsToolChoice::Required:
					jRequest["tool_choice"] = "required";
					break;
				}
			}
			else {
				jRequest["tool_choice"]["type"] = "function";
				jRequest["tool_choice"]["function"]["name"] = rb.target_tool;
			}
		}

		if (rb.logprobs.has_value()) jRequest["logprobs"] = rb.logprobs.value();

		if (rb.top_logprobs.has_value()) jRequest["top_logprobs"] = rb.top_logprobs.value();

		if (rb.user_id.has_value()) jRequest["user_id"] = rb.user_id.value();

		return jRequest.dump();
	}
}
