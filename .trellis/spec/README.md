# util-dfm 开发规范

> util-dfm 是 Deepin 文件管理器的工具包，包含四个独立的库模块。

**项目**: util-dfm (libdfm-*)
**版本**: Qt5+Qt6 双版本支持
**语言**: C++17

---

## 库模块

util-dfm 包含以下四个独立库，每个库有不同的技术栈和编码规范：

| 库 | 技术栈 | 错误处理 | 目录 |
|-----|-------|---------|------|
| **dfm-io** | GIO/GLib + Qt | GError + DFMIOErrorCode | [dfm-io/](dfm-io/) |
| **dfm-mount** | UDisks2 + GDBus + GIO | DeviceError (多来源转换) | [dfm-mount/](dfm-mount/) |
| **dfm-search** | Lucene++ + Qt | std::error_code + ErrorCategory | [dfm-search/](dfm-search/) |
| **dfm-burn** | xorriso + Qt | lastError() + errorMsg 字符串 | [dfm-burn/](dfm-burn/) |

---

## 规范索引

### 库特定规范

| 层级 | 文档 | 描述 |
|------|------|------|
| **dfm-io** | [error-handling.md](dfm-io/error-handling.md) | GError 转换、DFMIOErrorCode |
| **dfm-io** | [naming-conventions.md](dfm-io/naming-conventions.md) | 命名约定 |
| **dfm-io** | [memory-management.md](dfm-io/memory-management.md) | P-impl、智能指针、g_autoptr |
| **dfm-mount** | [error-handling.md](dfm-mount/error-handling.md) | UDisks2/ GIO/GDBus 错误转换 |
| **dfm-mount** | [naming-conventions.md](dfm-mount/naming-conventions.md) | 命名约定 |
| **dfm-mount** | [memory-management.md](dfm-mount/memory-management.md) | GLib autoptr 管理模式 |
| **dfm-search** | [error-handling.md](dfm-search/error-handling.md) | std::error_code 使用指南 |
| **dfm-search** | [naming-conventions.md](dfm-search/naming-conventions.md) | m_ 前缀命名规范 |
| **dfm-search** | [memory-management.md](dfm-search/memory-management.md) | std::unique_ptr、PIMPL |
| **dfm-burn** | [error-handling.md](dfm-burn/error-handling.md) | lastError() 模式 |
| **dfm-burn** | [naming-conventions.md](dfm-burn/naming-conventions.md) | 命名约定 |
| **dfm-burn** | [memory-management.md](dfm-burn/memory-management.md) | QScopedPointer、QSharedData |

### 通用共享规范

| 层级 | 文档 | 描述 |
|------|------|------|
| **shared** | [cpp-conventions.md](shared/cpp-conventions.md) | C++17 编码约定、RAII |
| **shared** | [git-conventions.md](shared/git-conventions.md) | Git 提交约定 |
| **shared** | [internationalization.md](shared/internationalization.md) | 国际化规范 |
| **shared** | [deepin-terminology.md](shared/deepin-terminology.md) | 品牌术语规范 |
| **shared** | [dbus-conventions.md](shared/dbus-conventions.md) | DBus 接口规范 |

### 思考指南

| 层级 | 文档 | 描述 |
|------|------|------|
| **guides** | [root-cause-analysis.md](guides/root-cause-analysis.md) | 5-Why 根因分析 |

### 常见问题

| 层级 | 文档 | 描述 |
|------|------|------|
| **big-question** | [gthread-ui-thread-safety.md](big-question/gthread-ui-thread-safety.md) | 跨线程更新 UI |
| **big-question** | [gvfs-mount-path-issues.md](big-question/gvfs-mount-path-issues.md) | GVfs 挂载路径问题 |
| **big-question** | [dbus-async-vs-sync.md](big-question/dbus-async-vs-sync.md) | DBus 异步/同步选择 |

### 代码审查

| 层级 | 文档 | 描述 |
|------|------|------|
| **review** | [code-review-standards.md](review/code-review-standards.md) | 代码审查标准 |
| **review** | [reference/](review/reference/) | 架构、安全、性能审查 |

---

## 快速开始

### 选择库开发

**开发前必须确认**：你在为哪个库开发？

| 如果你在开发... | 请阅读... |
|----------------|----------|
| dfm-io (文件 I/O) | [dfm-io/index.md](dfm-io/index.md) |
| dfm-mount (设备挂载) | [dfm-mount/index.md](dfm-mount/index.md) |
| dfm-search (文件搜索) | [dfm-search/index.md](dfm-search/index.md) |
| dfm-burn (光盘刻录) | [dfm-burn/index.md](dfm-burn/index.md) |

### 重要注意事项

1. **每个库的规范不同**：不要假设所有库使用相同的模式
   - dfm-io 使用 `GError` 和 `g_autoptr`
   - dfm-search 使用 `std::error_code`
   - dfm-burn 使用简单的 `lastError()` 模式

2. **禁用 signals/slots 关键字**：所有库使用 `Q_SIGNALS`/`Q_SLOTS` 宏

3. **Qt5/Qt6 双版本**：代码必须同时支持

---

## 核心规则摘要

| 规则 | 说明 |
|------|------|
| **库特定规范优先** | 遵循具体库的开发规范，而非通用规范 |
| **禁用 signals/slots** | 使用 `Q_SIGNALS`/`Q_SLOTS` 宏 |
| **错误处理** | 按库约定：GError / std::error_code / lastError() |
| **内存管理** | 按库约定：g_autoptr / std::unique_ptr / QScopedPointer |
| **国际化** | 用户文本使用 `tr()` |
| **线程安全** | 跨线程用 `QueuedConnection`，同步方法检查主线程 |
