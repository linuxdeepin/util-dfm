# dfm-mount 错误处理规范

## 概述

dfm-mount 处理来自多个来源的错误：UDisks2、GIO、GDBus，通过 `DeviceError` 枚举统一管理。

## 错误类型

### 1. DeviceError 枚举

**位置**: `include/dfm-mount/dfm-mount/base/dmount_global.h:158-343`

```cpp
enum class DeviceError : int16_t {
    kNoError = 0,
    kUnhandledError = 10001,

    // UDisks2 错误 (10000-10099)
    kUDisksErrorFailed = UDISKS_ERR_START,       // 10000
    kUDisksErrorAlreadyMounted,
    kUDisksErrorNotMounted,
    // ...

    // GIO 错误 (10100-10199)
    kGIOError = GIO_ERR_START,                    // 10100
    kGIOErrorFailed,
    kGIOErrorNotFound,
    // ...

    // GDBus 错误 (10200-10299)
    kGDBusError = GDBUS_ERR_START,                // 10200

    // 自定义错误 (20000-29999)
    kUserError = USER_ERR_START,                  // 20000
    kUserErrorNotMountable,
    kUserErrorAlreadyMounted,
    // ...
};
```

### 2. OperationErrorInfo 结构体

```cpp
struct OperationErrorInfo {
    DeviceError code;      // 错误码
    QString message;       // 错误消息

    OperationErrorInfo()
        : code(DeviceError::kNoError) {}

    OperationErrorInfo(DeviceError c, const QString &msg = "")
        : code(c), message(msg) {}

    bool hasError() const { return code != DeviceError::kNoError; }
};

// 私有实现中存储最后一次错误
mutable OperationErrorInfo lastError { DeviceError::kNoError, "" };
```

## 错误转换

### 1. GError 到 DeviceError 转换

**位置**: `src/dfm-mount/lib/base/dmountutils.cpp:503-519`

```cpp
DeviceError Utils::castFromGError(const GError *const err)
{
    if (!err)
        return DeviceError::kNoError;

    // 根据 GError domain 映射到不同的 DeviceError 范围
    const char *errDomain = g_quark_to_string(err->domain);
    if (strcmp(errDomain, UDISKS_ERR_DOMAIN) == 0) {
        return static_cast<DeviceError>(err->code + UDISKS_ERR_START);
    } else if (strcmp(errDomain, GIO_ERR_DOMAIN) == 0) {
        return static_cast<DeviceError>(err->code + GIO_ERR_START);
    } else if (strcmp(errDomain, GDBUS_ERR_DOMAIN) == 0) {
        return static_cast<DeviceError>(err->code + GDBUS_ERR_START);
    }
    return DeviceError::kUnhandledError;
}
```

### 2. 错误生成工具

```cpp
OperationErrorInfo Utils::genOperateErrorInfo(DeviceError errCode, const QString &errMsg = "")
{
    return { errCode, errMsg };
}
```

## 同步操作错误处理

**位置**: `src/dfm-mount/lib/dblockdevice.cpp:581-612`

```cpp
QString DBlockDevicePrivate::mount(const QVariantMap &opts)
{
    warningIfNotInMain();  // 线程安全检查

    // 检查是否有正在进行的作业
    if (findJob(kBlockJob))
        return "";

    UDisksFilesystem_autoptr fs = getFilesystemHandler();
    if (!fs) {
        lastError = Utils::genOperateErrorInfo(DeviceError::kUserErrorNotMountable);
        return "";
    }

    // 准备参数
    GError *err = nullptr;
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    char *mountPoint = nullptr;

    // 同步调用 DBus 方法
    bool mounted = udisks_filesystem_call_mount_sync(fs, gopts, &mountPoint, nullptr, &err);

    // 处理错误
    handleErrorAndRelease(err);

    返回挂载点或空字符串
    QString ret;
    if (mounted && mountPoint) {
        ret = mountPoint;
        g_free(mountPoint);
    }
    return ret;
}
```

## 异步操作错误处理

**位置**: `src/dfm-mount/lib/dblockdevice.cpp:23-55`

```cpp
// 错误处理回调
void DBlockDevicePrivate::handleErrorAndRelease(CallbackProxy *proxy,
                                               bool result,
                                               GError *gerr,
                                               QString info)
{
    OperationErrorInfo err;
    if (!result && gerr) {
        // 转换 GError
        err.code = Utils::castFromGError(gerr);
        err.message = gerr->message;
        qInfo() << "error occured while operating device" << err.message;

        // 释放 GError
        g_error_free(gerr);
    }

    // 调用用户回调
    if (proxy) {
        if (proxy->cb) {
            proxy->cb(result, err);
        } else if (proxy->cbWithInfo) {
            proxy->cbWithInfo(result, err, info);
        }
        delete proxy;  // 清理代理对象
    }
}

// 异步挂载回调
void DBlockDevicePrivate::mountAsyncCallback(GObject *sourceObj,
                                             GAsyncResult *res,
                                             gpointer userData)
{
    UDisksFilesystem *fs = UDISKS_FILESYSTEM(sourceObj);
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    g_autofree char *mountPoint = nullptr;

    // 完成 DBus 异步调用
    bool result = udisks_filesystem_call_mount_finish(fs, &mountPoint, res, &err);
    if (mountPoint)
        result = true;

    // 处理结果和错误
    QString info(mountPoint ? mountPoint : "");
    handleErrorAndRelease(proxy, result, err, info);
}
```

## 错误处理最佳实践

### DO - 应该做

1. **使用 GError 自动指针**
```cpp
g_autoptr(GError) gerror = nullptr;
// GError 自动释放
```

2. **回调中正确释放 GError**
```cpp
if (gerr) {
    // 处理错误
    g_error_free(gerr);
}
```

3. **使用 warningIfNotInMain() 检查线程安全**
```cpp
QString DBlockDevicePrivate::mount(const QVariantMap &opts) {
    warningIfNotInMain();  // 同步方法必须在主线程
}
```

### DON'T - 不应该做

1. **不要忽略 GError 释放**
```cpp
GError *err = nullptr;
udisks_call(&err);
// 忘记 g_error_free(err)
```

2. **不要在非主线程调用同步方法**
```cpp
// 错误: 在工作线程中调用 mount()
QThread::create([]() {
    device->mount();  // 会触发警告
})->start();

// 正确: 使用异步方法
QThread::create([]() {
    device->mountAsync({}, [](bool ok, OperationErrorInfo err) {
        // 处理结果
    });
})->start();
```

3. **不要忘记释放 g_autofree 和 g_free**
```cpp
// g_autofree 自动释放
g_autofree char *mountPoint = nullptr;
udisks_filesystem_call_mount_sync(fs, opts, &mountPoint, nullptr, &err);
// 不需要 g_free(mountPoint)

// 手动分配需要释放
char *data = g_strdup("hello");
// 使用后
g_free(data);
```

## 示例代码引用

| 文件路径 | 描述 |
|---------|------|
| `include/dfm-mount/dfm-mount/base/dmount_global.h:158-343` | DeviceError 枚举 |
| `src/dfm-mount/lib/base/dmountutils.cpp:503-519` | GError 转换函数 |
| `src/dfm-mount/lib/dblockdevice.cpp:581-612` | 同步挂载错误处理 |
| `src/dfm-mount/lib/dblockdevice.cpp:23-55` | 异步回调错误处理 |
