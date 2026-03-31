# Bootstrap: Fill Project Development Guidelines

## 目的

为 util-dfm 项目创建库特定的开发规范。由于项目包含多个独立库（dfm-io、dfm-mount、dfm-search、dfm-burn），每个库有不同的技术栈和编码约定，需要创建特定于每个库的规范文档。

---

## 已完成工作

### 1. 创建库特定规范目录

```
.trellis/spec/
├── dfm-io/         # dfm-io 库规范
├── dfm-mount/      # dfm-mount 库规范
├── dfm-search/     # dfm-search 库规范
└── dfm-burn/       # dfm-burn 库规范
```

### 2. 创建的规范文档

每个库创建了以下关键文档：

#### dfm-io (GIO/GLib + Qt)
- `index.md` - 规范索引
- `error-handling.md` - GError + DFMIOErrorCode 错误处理
- `naming-conventions.md` - 类名、方法名、变量名约定

#### dfm-mount (UDisks2 + GDBus + GIO)
- `index.md` - 规范索引
- `error-handling.md` - UDisks2/GIO/GDBus 错误转换
- `naming-conventions.md` - 类名、方法名、变量名约定

#### dfm-search (Lucene++ + Qt + std::error_code)
- `index.md` - 规范索引
- `error-handling.md` - std::error_code + ErrorCategory 模式
- `naming-conventions.md` - m_ 前缀成员变量命名

#### dfm-burn (xorriso + Qt)
- `index.md` - 规范索引
- `error-handling.md` - lastError() 模式
- `naming-conventions.md` - 类名、方法名约定

### 3. 更新主 README

更新 `.trellis/spec/README.md`，明确说明：
- 四个库的不同技术栈
- 每个库的错误处理方式
- 开发前必须选择正确的库规范

---

## 核心要点

### 差异总结

| 方面 | dfm-io | dfm-mount | dfm-search | dfm-burn |
|------|--------|-----------|------------|----------|
| **错误处理** | GError + DFMIOErrorCode | DeviceError (多来源转换) | std::error_code | lastError() |
| **智能指针** | QSharedDataPointer, QSharedPointer | UDisksX_autoptr | std::unique_ptr | QScopedPointer, QSharedData |
| **成员变量** | d/q 指针模式 | d/q 指针模式 | m_ 前缀 | 小驼峰 |
| **GLib 集成** | 大量使用 g_autoptr | UDisks2 autoptr | 无 | 无 |
| **线程模型** | GIO 异步回调 | GDBus 异步回调 | QThread + 信号槽 | DirectConnection |

### 共同规则

1. **禁用 signals/slots 关键字**：所有库使用 `Q_SIGNALS`/`Q_SLOTS` 宏
2. **Qt5/Qt6 双版本支持**
3. **国际化**：用户文本使用 `tr()`

---

## 待完成

以下文档可以后续根据需要补充：

### dfm-io
- `memory-management.md` - P-impl 模式、智能指针、GLib 对象管理
- `code-patterns.md` - 信号槽、回调、文件组织
- `signals-and-callbacks.md` - Q_SIGNALS、GIO 信号桥接

### dfm-mount
- `memory-management.md` - GLib autoptr、UDisksX_autoptr
- `code-patterns.md` - DBus 集成、GIO 信号桥接
- `dbus-integration.md` - UDisks2 调用模式

### dfm-search
- `memory-management.md` - std::unique_ptr、PIMPL
- `code-patterns.md` - 策略模式、线程模型
- `signal-threading.md` - 线程间信号通信

### dfm-burn
- `memory-management.md` - QScopedPointer、QSharedData
- `code-patterns.md` - 信号连接、DirectConnection

---

## 完成检查清单

- [x] 创建库特定规范目录
- [x] 为每个库创建 index.md
- [x] 为每个库创建 error-handling.md
- [x] 为每个库创建 naming-conventions.md
- [x] 更新主 README 说明库差异
- [ ] 额外的内存管理文档（可选）
- [ ] 额外的代码模式文档（可选）

---

## 使用方式

开发前，根据目标库阅读相应规范：

```bash
# 开发 dfm-io
cat .trellis/spec/dfm-io/index.md

# 开发 dfm-mount
cat .trellis/spec/dfm-mount/index.md

# 开发 dfm-search
cat .trellis/spec/dfm-search/index.md

# 开发 dfm-burn
cat .trellis/spec/dfm-burn/index.md
```
