# dfm-burn 库开发规范

> dfm-burn 是光盘刻录库，使用 xorriso/udfburn 后端。

## 技术栈

- **C++11/14**
- **Qt5/Qt6** - QObject、信号槽
- **libburnia xorriso** - 光盘刻录后端
- **UDF 客户端** - 自定义 UDF 文件系统实现

## 规范索引

| 文档 | 描述 | 优先级 |
|------|------|--------|
| [error-handling.md](error-handling.md) | errorMsg + lastError() 模式 | P0 |
| [naming-conventions.md](naming-conventions.md) | 类名、方法名约定 | P0 |
| [memory-management.md](memory-management.md) | QScopedPointer、QSharedData | P0 |
| [code-patterns.md](code-patterns.md) | 信号连接、DirectConnection | P1 |

---

## 开发前检查清单

- [ ] 阅读 [error-handling.md](error-handling.md) 了解 errorMsg 模式
- [ ] 阅读 [memory-management.md](memory-management.md) 了解 QScopedPointer 和 QSharedData
- [ ] 信号声明使用 `Q_SIGNALS`，槽使用 `Q_SLOTS`

---

## 核心规则摘要

| 规则 | 要求 |
|------|------|
| 信号声明 | 必须使用 `Q_SIGNALS` 宏 |
| 成员变量 | 小驼峰命名，bool 用 is/has 前缀 |
| 错误处理 | 返回 bool + lastError() 获取错误 |
| QObject 子类 | 使用 QScopedPointer 管理私有实现 |
| 数据类 | 使用 QSharedData（隐式共享） |
| 异步操作 | 使用 DirectConnection + QPointer 防止野指针 |
