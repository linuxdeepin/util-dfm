# dfm-io 错误处理规范

## 概述

dfm-io 使用双重错误处理系统：`GError`（来自 GLib/GIO）和 `DFMIOErrorCode`（项目自定义）。

## 错误类型

### 1. DFMIOErrorCode 枚举

**位置**: `include/dfm-io/dfm-io/error/en.h`

```cpp
enum class DFMIOErrorCode : int {
    DFM_IO_ERROR_NONE = -1,          // 无错误
    DFM_IO_ERROR_FAILED,             // 通用错误
    DFM_IO_ERROR_NOT_FOUND,          // 文件未找到
    DFM_IO_ERROR_EXISTS,             // 文件已存在
    DFM_IO_ERROR_PERMISSION_DENIED,  // 权限拒绝
    DFM_IO_ERROR_NO_SPACE,           // 磁盘空间不足
    DFM_IO_ERROR_NOT_DIRECTORY,      // 不是目录
    // ... 约 70+ 个错误码
};
```

### 2. DFMIOError 结构体

**位置**: `include/dfm-io/dfm-io/error/error.h`

```cpp
struct DFMIOError {
    DFMIOErrorCode errorCode;
    QString errMsg;

    // 获取错误码对应的默认消息
    QString code() const { return GetError_En(errorCode); }

    // 优先返回自定义消息，否则返回默认消息
    QString errorMsg() const { returnerrMsg.isEmpty() ? code() : errMsg; }

    // 判断是否有错误
    bool isError() const { return errorCode != DFM_IO_ERROR_NONE; }

    // 设置错误码
    void setCode(DFMIOErrorCode code) { errorCode = code; }

    // 设置自定义消息
    void setMessage(const QString &msg) { errMsg = msg; }

    // 隐式转换为 bool
    explicit operator bool() const { return isError(); }
};
```

## 错误转换模式

### 1. 从 GError 转换

**位置**: `src/dfm-io/dfm-io/doperator.cpp:31-42`

```cpp
void DOperatorPrivate::setErrorFromGError(GError *gerror)
{
    if (!gerror)
        return;

    // 将 GError code 映射到 DFMIOErrorCode
    error.setCode(DFMIOErrorCode(gerror->code));

    // 通用错误需要特殊处理消息
    if (error.code() == DFMIOErrorCode::DFM_IO_ERROR_FAILED) {
        QString strErr(gerror->message);
        // GIO 错误消息格式通常是 "domain: message: other: info"
        // 提取核心信息
        if (strErr.contains(':'))
            strErr = strErr.left(strErr.indexOf(":")) + strErr.mid(strErr.lastIndexOf(":"));
        error.setMessage(strErr);
    }
}
```

### 2. 从 errno 转换

**位置**: `src/dfm-io/dfm-io/doperator.cpp:44-90`

```cpp
void DOperatorPrivate::setErrorFromErrno(int errnoValue)
{
    DFMIOErrorCode errorCode;
    switch (errnoValue) {
    case EACCES:
    case EPERM:
        errorCode = DFM_IO_ERROR_PERMISSION_DENIED;
        break;
    case ENOENT:
        errorCode = DFM_IO_ERROR_NOT_FOUND;
        break;
    case EEXIST:
        errorCode = DFM_IO_ERROR_EXISTS;
        break;
    case ENOSPC:
        errorCode = DFM_IO_ERROR_NO_SPACE;
        break;
    default:
        errorCode = DFM_IO_ERROR_FAILED;
        break;
    }
    error.setCode(errorCode);
}
```

## 公共接口错误处理模式

### 每个操作类都有 lastError() 方法

```cpp
// DOperator
DFMIOError lastError() const;

// DFileInfo
DFMIOError lastError() const;

// DWatcher
DFMIOError lastError() const;

// DEnumerator
DFMIOError lastError() const;
```

### 使用示例

```cpp
// 操作失败时，返回 false 或 nullptr
DOperator op(filePath);
if (!op.renameFile(newPath)) {
    DFMIOError err = op.lastError();
    qWarning() << "Rename failed:" << err.errorMsg();
    return;
}
```

## 错误处理最佳实践

### DO - 应该做

1. **每次操作后检查错误**
```cpp
DOperator op(filePath);
if (!op.copyFile(destPath)) {
    handle_error(op.lastError());
}
```

2. **使用 lastError() 获取完整错误信息**
```cpp
DFMIOError err = op.lastError();
if (err.isError()) {
    errCode = err.errorCode();  // 获取错误码
    errMsg = err.errorMsg();    // 获取错误消息
}
```

3. **使用 g_autoptr 自动管理 GError**
```cpp
g_autoptr(GError) gerror = nullptr;
// GLib 函数调用会填充 gerror
if (opFailed) {
    setErrorFromGError(gerror);
}
// g_error_free 不需要，g_autoptr 自动处理
```

### DON'T - 不应该做

1. **不要忽略错误**
```cpp
// 错误:
op.renameFile(newPath);  // 忽略返回值

// 正确:
if (!op.renameFile(newPath)) {
    // 处理错误
}
```

2. **不要在转换后丢失原始错误码**
```cpp
// 错误:
if (gerror) {
    QString msg(gerror->message);
    setError(msg);  // 丢失了 gerror->code
}

// 正确:
if (gerror) {
    error.setCode(DFMIOErrorCode(gerror->code));
    error.setMessage(gerror->message);
}
```

3. **不要忘记释放手动分配的 GError**
```cpp
// 错误:
GError *err = nullptr;
call_func(&err);
// 忘记 g_error_free(err);

// 正确:
GError *err = nullptr;
call_func(&err);
if (err) {
    // 处理错误
    g_error_free(err);
}

// 更好: 使用 g_autoptr
g_autoptr(GError) err = nullptr;
call_func(&err);
```

## 示例代码引用

| 文件路径 | 描述 |
|---------|------|
| `include/dfm-io/dfm-io/error/en.h` | 错误码枚举定义 |
| `include/dfm-io/dfm-io/error/error.h` | DFMIOError 结构体 |
| `src/dfm-io/dfm-io/doperator.cpp:31-90` | 错误转换实现 |
| `src/dfm-io/dfm-io/dfileinfo.cpp` | DFileInfo 错误处理 |
