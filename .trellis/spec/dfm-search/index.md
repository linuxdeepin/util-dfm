# dfm-search 库开发规范

> dfm-search 是文件搜索库，采用现代 C++17 + Qt，使用 std::error_code 进行错误处理。

## 技术栈

- **C++17**
- **Qt5/Qt6** - 信号槽、线程、事件循环
- **Lucene++** - 全文搜索引擎
- **std::error_code** - 错误处理

## 规范索引

| 文档 | 描述 | 优先级 |
|------|------|--------|
| [error-handling.md](error-handling.md) | std::error_code + ErrorCategory模式 | P0 |
| [naming-conventions.md](naming-conventions.md) | 类名、方法名、m_ 前缀成员变量 | P0 |
| [memory-management.md](memory-management.md) | std::unique_ptr、PIMPL | P0 |
| [code-patterns.md](code-patterns.md) | 策略模式、线程模型 | P1 |
| [signal-threading.md](signal-threading.md) | 线程间信号通信 | P1 |

---

## 开发前检查清单

- [ ] 阅读 [error-handling.md](error-handling.md) 了解 std::error_code 和 ErrorCategory
- [ ] 阅读 [memory-management.md](memory-management.md) 了解 std::unique_ptr 使用规则
- [ ] 阅读 [signal-threading.md](signal-threading.md) 了解工作线程与主线程通信
- [ ] 信号声明使用 `Q_SIGNALS`，槽使用 `Q_SLOTS`

---

## 核心规则摘要

| 规则 | 要求 |
|------|------|
| 成员变量 | `m_` 前缀 + camelCase |
| 智能指针 | 仅用 `std::unique_ptr`，不用 `std::shared_ptr` |
| 错误处理 | `std::error_code` + 自定义 ErrorCategory |
| PIMPL | SearchResult、SearchOptions 使用 PIMPL |
| 线程模型 | SearchWorker 在独立线程，使用信号通信 |
| 策略模式 | BaseSearchStrategy + SearchStrategyFactory |
