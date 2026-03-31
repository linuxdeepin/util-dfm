# dfm-mount 库开发规范

> dfm-mount 是设备挂载库，使用 UDisks2 和 GIO/GDBus 管理设备。

## 技术栈

- **C++11+**
- **Qt5/Qt6** - QObject、QSharedPointer、QDBusServiceWatcher
- **UDisks2** - libudisks2 设备管理系统
- **GLib/GIO** - GVariant、GError、GDBus、GSignal
- **libmount** - 解析挂载点

## 规范索引

| 文档 | 描述 | 优先级 |
|------|------|--------|
| [error-handling.md](error-handling.md) | UDisks2、GIO、GDBus 错误处理 | P0 |
| [naming-conventions.md](naming-conventions.md) | 类名、方法名、变量名约定 | P0 |
| [memory-management.md](memory-management.md) | GLib autoptr、UDisksX_autoptr | P0 |
| [code-patterns.md](code-patterns.md) | DBus 集成、GIO 信号桥接 | P1 |
| [dbus-integration.md](dbus-integration.md) | UDisks2 调用模式 | P1 |

---

## 开发前检查清单

- [ ] 阅读 [error-handling.md](error-handling.md) 了解 GError 转换和 DeviceError 枚举
- [ ] 阅读 [memory-management.md](memory-management.md) 了解 UDisksX_autoptr 使用
- [ ] 阅读 [dbus-integration.md](dbus-integration.md) 了解同步/异步 DBus 调用
- [ ] 确认禁用 signals/slots 关键字，使用 Q_SIGNALS/Q_SLOTS 宏
- [ ] 同步方法会调用 `warningIfNotInMain()` 检查线程安全

---

## 核心规则摘要

| 规则 | 要求 |
|------|------|
| 信号声明 | 必须使用 `Q_SIGNALS` 宏 |
| GIO 回调 | 使用静态回调函数 + userData 传递 this |
| GLib 对象 | 必须使用 `UDisksX_autoptr` 自动管理 |
| 线程安全 | 同步方法仅主线程，否则用 Async 版本 |
| 错误处理 | `lastError()` 返回 OperationErrorInfo |
| 函数注册 | 使用 std::bind 注册虚函数到基类 |
