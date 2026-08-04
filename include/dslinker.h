#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <memory>

namespace dslinker {
	/*-----------------------------------------------------------------
	* 
	* 工具函数封装
	* 
	-----------------------------------------------------------------*/
	// 工具函数条类
	struct Function {
		// function 的功能描述，供模型理解何时以及如何调用该 function。
		std::optional<std::string> description = std::nullopt;
		// （必要）要调用的 function 名称。必须由 a-z、A-Z、0-9 字符组成，或包含下划线和连字符，最大长度为 64 个字符。
		std::string name;
		// function 的输入参数
		std::optional<nlohmann::json> param;

		// 严格输出模式。如果设置为 true，API 将在函数调用中使用 strict 模式，以确保输出始终符合函数的 JSON schema 定义。该功能为 Beta 功能，详细使用方式请参阅 Tool Calls 指南
		std::optional<bool> strict = std::nullopt;
	};

	/*
	回调函数原型：
	void* (*callback)(const std::string& jrpParam, std::string& outResult);
	jrpParam 为json格式字符串，请自行按照自己在 param 约定的参数处理。
	outResult 为输出结果，用于接受回调函数运算结果
	返回值 void* 作为调用者可获取并且完全自定义的上下文，用于保存中间结果、记录错误信息。请自行在回调函数内与调用逻辑内对返回值进行解析、内存管理，toolset 以及其他更高层的库都只负责传递该返回值。
	*/
	using ToolCallBack = void* (*)(const std::string& jrpParam, std::string& outResult);

	// 工具集
	class ToolSet {
	public:
		// 添加工具到工具集，并且绑定对应的回调函数。请调用addTool(funcDef, std::make_unique<IToolCallBack子类>(构造参数))，将指针所有权交给 ToolSet 管理
		void addTool(Function fcTool, ToolCallBack);
		// 从工具集删除工具，并且解绑对应的回调函数, name 为 Function 中的 name 成员变量
		void removeTool(std::string name);
		// 获取 json 格式的工具列表
		nlohmann::json getToolsList(void);
		// 从工具集中以 name 获取工具回调 Functor
		ToolCallBack getResponse(std::string name);
	private:
		std::vector<Function> functions;
		std::unordered_map<std::string, ToolCallBack> toolset;
	};

	/*-----------------------------------------------------------------
	* 
	* Chat Completions API 请求
	* 
	-----------------------------------------------------------------*/

	// 消息的发起角色枚举
	enum class Role{
		system,
		user,
		assistant,
		tool
	};

	// 消息结构体
	struct Message {
		// （必要）该消息的发起角色
		Role role;
		// （必要）消息的内容
		std::string content;

		// 可以选填的参与者的名称，为模型提供信息以区分相同角色的参与者。适用于 system、user、assistant
		std::optional<std::string> name = std::nullopt;

		// (Beta) 设置此参数为 true，来强制模型在其回答中以此 assistant 消息中提供的前缀内容开始。
		// 您必须设置 base_url = "https://api.deepseek.com/beta" 来使用此功能。
		std::optional<bool> prefix = std::nullopt;
		// (Beta) 用于思考模式下在对话前缀续写功能下，作为最后一条 assistant 思维链内容的输入。使用此功能时，prefix 参数必须设置为 true
		std::optional<std::string> reasoning_content = std::nullopt;

		std::optional<std::string> tool_call_id = std::nullopt;
	};

	enum class ChatModel {
		deepseek_v4_flash,
		deepseek_v4_pro
	};

	enum class OptionsThinkingType {
		enabled,
		disabled
	};

	enum class OptionsReasoningEffort {
		low,
		high,
		max
	};

	enum class OptionsResponseFormatType {
		text,
		json_object
	};

	enum class OptionsToolChoice {
		None,
		Auto,
		Required,
		Function
	};

	// Chat Completions API 请求结构体
	struct ChatRequestBody {
		// （必要）对话的消息列表
		std::vector<Message> messages;
		// （必要）使用的模型的 ID
		ChatModel model;
		
		// 思考模式开关（API 默认 enabled）
		std::optional<OptionsThinkingType> thinking = std::nullopt;
		// 控制模型的推理强度。（API 默认为 high）
		std::optional<OptionsReasoningEffort> reasoning_effort = std::nullopt;

		/* 限制一次请求中模型生成 completion 的最大 token 数。
		   输入 token 和输出 token 的总长度受模型的上下文长度的限制。
		   取值范围与默认值详见 DeepSeek API 文档*/
		std::optional<int> max_tokens = std::nullopt;

		/* 指定模型必须输出的格式。
		   设置为 json_object 以启用 JSON 模式，该模式保证模型生成的消息是有效的 JSON。
		   注意 : 使用 JSON 模式时，你还必须通过系统或用户消息指示模型生成 JSON。*/
		std::optional<OptionsResponseFormatType> response_format = std::nullopt;
		// 一个 string 或最多包含 16 个 string 的 list，在遇到这些词时，API 将停止生成更多的 token
		std::optional<std::vector<std::string>> stop = std::nullopt;

		// 流式模式开关。开启后将会以 SSE（server-sent events）的形式以流式发送消息增量。消息流以 data: [DONE] 结尾
		std::optional<bool> stream = std::nullopt;
		/* 属于 stream_options。
		   如果设置为 true，在流式消息最后的 data : [DONE] 之前将会传输一个额外的块。
		   此块上的 usage 字段显示整个请求的 token 使用统计信息，而 choices 字段将始终是一个空数组。
		   所有其他块也将包含一个 usage 字段，但其值为 null。*/
		std::optional<bool> include_usage = std::nullopt;

		/* 采样温度，介于 0 和 2 之间。
		   更高的值，如 0.8，会使输出更随机，而更低的值，如 0.2，会使其更加集中和确定。
		   我们通常建议可以更改这个值或者更改 top_p，但不建议同时对两者进行修改。*/
		std::optional<double> temperature = std::nullopt;
		/* 作为调节采样温度的替代方案，模型会考虑前 top_p 概率的 token 的结果。
		   所以 0.1 就意味着只有包括在最高 10% 概率中的 token 会被考虑。 
		   通常建议修改这个值或者更改 temperature，但不建议同时对两者进行修改。*/
		std::optional<double> top_p = std::nullopt;

		// 模型可能会调用的 tool 的列表。目前，仅支持 function 作为工具
		std::optional<ToolSet> tools = std::nullopt;
		// 控制模型调用 tool 的行为
		std::optional<OptionsToolChoice> tool_choice = std::nullopt;
		// 在 tool_choice 为 Function 时指定特定 tool 的 name，会强制模型调用该 tool。
		std::optional<std::string> target_tool = std::nullopt;

		// 是否返回所输出 token 的对数概率。如果为 true，则在 message 的 content 中返回每个输出 token 的对数概率
		std::optional<bool> logprobs = std::nullopt;
		// 一个介于 0 到 20 之间的整数 N，指定每个输出位置返回输出概率 top N 的 token，且返回这些 token 的对数概率。指定此参数时，logprobs 必须为 true。
		std::optional<int> top_logprobs = std::nullopt;

		/* 用户自定义的 user_id，可选字符集为[a - zA - Z0 - 9\ - _]，最大长度为 512。请不要在 user_id 中包含用户隐私信息。
		   user_id 可用于区分业务侧的用户身份，以帮助 DeepSeek 官方进行内容安全处理。
		   可用于 KVCache 缓存隔离，以进行隐私管理。
		   可用于 DeepSeek 官方对您业务侧用户进行调度隔离。关于 user_id 参数更详细的描述，请参考限速与隔离*/
		std::optional<std::string> user_id = std::nullopt;
	};

	// 转写 ChatRequest 为 JsonString
	std::string packChatRequest(ChatRequestBody request_body);
}
