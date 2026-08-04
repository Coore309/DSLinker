#include <dslinker.h>

namespace dslinker {
	// 添加工具到工具集，并且绑定对应的回调函数，请调用addTool(funcDef, std::make_unique<IToolCallBack子类>(构造参数))
	void ToolSet::addTool(Function fcTool, ToolCallBack callback) {
		functions.push_back(fcTool);
		toolset.emplace(fcTool.name, callback);
	}

	// 从工具集删除工具，并且解绑对应的回调函数
	void ToolSet::removeTool(std::string name) {
		bool flagDelete = false;
		for (auto i = functions.begin(); i != functions.end(); ++i)
			if (i->name == name) {
				functions.erase(i);
				flagDelete = true;
				break;
			}

		if (flagDelete) toolset.erase(name);
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

	// 从工具集中以 name 获取工具回调 Functor
	ToolCallBack ToolSet::getResponse(std::string name) {
		auto ptr = toolset.find(name);

		if (ptr != toolset.end()) return ptr->second;
		else return nullptr;
	}

}
