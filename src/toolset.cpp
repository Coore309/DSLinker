#include <dslinker.h>

namespace dslinker {
	// 添加工具到工具集，并且绑定对应的回调函数，请调用addTool(funcDef, std::make_unique<IToolCallBack子类>(构造参数))
	void ToolSet::addTool(Function fcTool) {
		functions.push_back(fcTool);
	}

	// 从工具集删除工具，并且解绑对应的回调函数
	void ToolSet::removeTool(std::string name) {
		for (auto i = functions.begin(); i != functions.end(); ++i)
			if (i->name == name) {
				functions.erase(i);
				break;
			}
	}

	// 获取 json 格式的工具列表
	nlohmann::json ToolSet::getToolsList(void) {
		nlohmann::json listTools;
		for (auto i = functions.begin(); i != functions.end(); ++i) {
			nlohmann::json tool;
			tool["type"] = "function";
			if ((i->description).has_value()) tool["function"]["description"] = (i->description).value();
			tool["function"]["name"] = i->name;
			if ((i->param).has_value()) tool["function"]["parameters"] = (i->param).value();
			if ((i->strict).has_value()) tool["function"]["strict"] = (i->strict).value();

			listTools.push_back(tool);
		}

		return listTools;
	}
}
