# Journal - Zhang Sheng (Part 1)

> AI development session journal
> Started: 2026-03-31

---



## Session 1: Bootstrap: Create Library-Specific Guidelines

**Date**: 2026-03-31
**Task**: Bootstrap: Create Library-Specific Guidelines

### Summary

(Add summary)

### Main Changes

# Bootstrap Guidelines - 创建库特定开发规范

## 任务目标
为 util-dfm 项目创建库特定的开发规范，替换通用的 DDE 桌面应用规范。

## 为什么需要库特定规范
util-dfm 包含 4 个独立库，每个库有不同的技术栈和编码约定：
- **dfm-io**: GIO/GLib + Qt
- **dfm-mount**: UDisks2 + GDBus + GIO
- **dfm-search**: Lucene++ + Qt + std::error_code
- **dfm-burn**: xorriso + Qt

## 完成的工作

### 1. 创建库特定规范目录
```
.trellis/spec/
├── dfm-io/         # dfm-io 库规范
├── dfm-mount/      # dfm-mount 库规范
├── dfm-search/     # dfm-search 库规范
└── dfm-burn/       # dfm-burn 库规范
```

### 2. 为每个库创建规范文档

#### dfm-io (GIO/GLib + Qt)
- `index.md` - 规范索引
- `error-handling.md` - GError 转换、DFMIOErrorCode 使用
- `naming-conventions.md` - D 前缀类名、k 前缀枚举、d/q 指针模式

#### dfm-mount (UDisks2 + GDBus + GIO)
- `index.md` - 规范索引
- `error-handling.md` - DeviceError 多来源转换、GError 处理
- `naming-conventions.md` - d/q 指针模式、static 回调函数

#### dfm-search (Lucene++ + Qt + std::error_code)
- `index.md` - 规范索引
- `error-handling.md` - std::error_code + ErrorCategory 模式
- `naming-conventions.md` - m_ 前缀成员变量、策略模式命名

#### dfm-burn (xorriso + Qt)
- `index.md` - 规范索引
- `error-handling.md` - lastError() + errorMsg 字符串模式
- `naming-conventions.md` - is/has 布尔前缀、小驼峰成员变量

### 3. 更新主 README
更新 `.trellis/spec/README.md`，明确说明：
- 四个库的不同技术栈
- 每个库的错误处理方式
- 开发前必须选择正确的库规范

## 关键差异总结

| 方面 | dfm-io | dfm-mount | dfm-search | dfm-burn |
|------|--------|-----------|------------|----------|
| **错误处理** | GError / DFMIOErrorCode | DeviceError 转换 | std::error_code | lastError() 字符串 |
| **智能指针** | QSharedDataPointer | UDisksX_autoptr | std::unique_ptr | QScopedPointer |
| **成员变量** | d/q 指针 | d/q 指针 | m_ 前缀 | 小驼峰 |
| **GLib 集成** | g_autoptr 大量使用 | UDisks2 autoptr | 无 | 无 |

## 共同规则
- 禁用 signals/slots 关键字，使用 Q_SIGNALS/Q_SLOTS 宏
- Qt5/Qt6 双版本支持
- 国际化：用户文本使用 tr()

## 待完成（可选）
- memory-management.md - 详细的内存管理规范
- code-patterns.md - 策略模式、线程模型等代码模式
- dbus-integration.md (dfm-mount) - UDisks2 调用模式


### Git Commits

| Hash | Message |
|------|---------|
| `d846cbc` | (see git log) |

### Testing

- [OK] (Add test results)

### Status

[OK] **Completed**

### Next Steps

- None - task complete
