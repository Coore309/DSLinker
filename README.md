# DSLinker
一个轻量级的DeepSeek API通信层封装库

_基础通信模块_ 已开发完成，支持流式/非流式请求、请求体构造、响应状态管理。

## 特性
- **请求体打包** 通过 packChatRequest() 将结构化数据转换为 JSON，覆盖全部 DeepSeek API 参数
- **双模式请求** 支持 stream 与非流式两种调用方式，通过统一接口 popWords() 消费增量或完整回答
- **状态管理** 内置 alive_ 与 stop_ 标志管理异步请求，支持 hasProccess() 轮询与 stopRequest() 强制中断
- **原始 JSON 透传** 通过 popResponseJsons() 获取完整响应体，便于上层自行处理复杂数据（如工具调用）

## 安装与构建

### 手动链接

1. 将 `include/` 加入 C/C++ → 常规 → 附加包含目录
2. 将 `DSLinker.lib` 加入链接器 → 输入 → 附加依赖项

``` cpp
#pragma comment(lib, "DSLinker.lib")
```

### 从源码构建
``` bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```
## API 概览

### Chat 请求

| 作用类别 | 接口 |
|:------:|:----|
| 非流式请求 | `requestChatNoStream(jsonRequest)` |
| 流式请求 | `requestChatStream(jsonRequest)` |
| 消费增量/回答 | `popWords()` → `ModelAnswer` |
| 获取原始 JSON | `popResponseJsons()` |
| 状态轮询 | `hasProccess()` / `hasWords()` / `hasJsons()` |
| 阻塞等待 | `waitRequest()` |
| 强制停止 | `stopRequest()` |

### ModelAnswer 字段说明

| 字段 | 说明 |
|:---:|:----|
| `statu` | 状态码：`-4` JSON 解析错误；`-3` 手动中断；`-2` 连接失败；`-1` HTTP 错误；`1` 流式增量；`0` 正常结束 |
| `id` | 错误状态码 或 tool_call_id（预留） |
| `thought` | 思维链 / 错误原因 |
| `answer` | 回答内容 / 错误原始体 |

### 请求体构造

```cpp
dslinker::ChatRequestBody rb;
rb.model = dslinker::ChatModel::deepseek_v4_flash;
rb.messages.push_back({dslinker::Role::user, "你好"});
rb.stream = true;

nlohmann::json req = dslinker::packChatRequest(rb);
dsl.requestChatStream(req);
```

## 注意事项

- 库与调用方需使用相同的运行库设置（静态链接时尤为重要）
- 接口统一接受、输出 UTF-8 编码的字符串
- **工具调用**：本库不内置工具执行回调，需通过 `popResponseJsons()` 获取原始 `tool_calls` JSON 后自行处理。更多封装将在未来推出。
- `[DONE]` 标记不在 `wordStream_` 中推送，流结束时请通过 `hasProccess()` 判断

## 许可证

本项目基于 **MIT 许可证** 开源发布，详细许可证文本见 [LICENSE](LICENSE) 文件