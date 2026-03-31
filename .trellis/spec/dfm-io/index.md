# dfm-io 库开发规范

> dfm-io 是 Deepin 文件管理器的文件 I/O 操作核心库，基于 GIO/GLib 提供异步文件操作。

## 技术栈

- **C++11+**
- **Qt5/Qt6** - QUrl、QObject、智能指针
- **GLib/GIO** - 底层文件系统操作
- **g_autoptr** - GLib 自动指针

## 规范索引

| 文档 | 描述 | 优先级 |
|------|------|--------|
| [error-handling.md](error-handling.md) | GError + DFMIOErrorCode 错误处理 | P0 |
| [naming-conventions.md](naming-conventions.md) | 类名、方法名、变量名约定 | P0 |
| [memory-management.md](memory-management.md) | P-impl 模式、智能指针、GLib 对象管理 | P0 |
| [code-patterns.md](code-patterns.md) | 信号槽、回调、文件组织 | P1 |
| [signals-and-callbacks.md](signals-and-callbacks.md) | Q_SIGNALS、GIO 信号桥接 | P1 |

---

## 开发前检查清单

- [ ] 阅读 [error-handling.md](error-handling.md) 了解 GError 转换和 DFMIOErrorCode 使用
- [ ] 阅读 [memory-management.md](memory-management.md) 了解 g_autoptr 和 GLib 对象释放
- [ ] 确认禁用 signals/slots 关键字，使用 Q_SIGNALS/Q_SLOTS 宏

---

## 核心规则摘要

| 规则 | 要求 |
|------|------|
| 信号声明 | 必须使用 `Q_SIGNALS` 宏 |
| 槽声明 | 必须使用 `Q_SLOTS` 宏 |
| GLib 对象 | 优先使用 `g_autoptr`，手动释放用 `g_object_unref` |
| 错误处理 | 检查 `lastError()` 获取错误信息 |
| 命名空间 | 使用 `BEGIN_IO_NAMESPACE` / `END_IO_NAMESPACE` 宏 |
| 私有实现 | 派生 QSharedData |
