# dfm-mount 命名约定

## 类命名

### 公共接口类

**规则**: `D` + 功能名词，大驼峰 (PascalCase)

```cpp
class DDevice;               // 设备基类
class DBlockDevice;          // 块设备
class DProtocolDevice;       // 协议设备
class DBlockMonitor;         // 块设备监视器
class DProtocolMonitor;      // 协议设备监视器
class DDeviceManager;        // 设备管理器
```

### 私有实现类

**规则**: 公共类名 + `Private`

```cpp
class DDevicePrivate;
class DBlockDevicePrivate;
class DProtocolDevicePrivate;
class DBlockMonitorPrivate;
class DProtocolMonitorPrivate;
```

## 方法命名

### 公共方法

**规则**: 小驼峰 (camelCase)

```cpp
// 同步/异步方法配对
QString mount(const QVariantMap &opts = {});
void mountAsync(const QVariantMap &opts = {}, DeviceOperateCallbackWithMessage cb = nullptr);

bool unmount(const QVariantMap &opts = {});
void unmountAsync(const QVariantMap &opts = {}, DeviceOperateCallback cb = nullptr);

QString path() const;
bool hasBlock() const;
QVariant queryProperty(Property property) const;
```

### 静态回调函数

**规则**: `on` + 事件描述，静态成员

```cpp
class DBlockMonitorPrivate final {
private:
    // GIO 信号使用静态回调函数
    static void onObjectAdded(GDBusObjectManager *mng,
                             GDBusObject *obj,
                             gpointer userData);
    static void onObjectRemoved(GDBusObjectManager *mng,
                                GDBusObject *obj,
                                gpointer userData);
    static void onPropertyChanged(GDBusObjectManagerClient *mngClient,
                                 GDBusObjectProxy *objProxy,
                                 GDBusProxy *dbusProxy,
                                 GVariant *property,
                                 const gchar *const invalidProperty,
                                 gpointer userData);
};
```

## 成员变量命名

### d 和 q 指针

```cpp
class DDevice {
protected:
    QScopedPointer<DDevicePrivate> d;  // d 指向私有实现
};

class DDevicePrivate {
public:
    DDevice *q { nullptr };  // q 指向公共类
};
```

### 普通成员变量

**规则**: 小驼峰命名

```cpp
class DBlockDevicePrivate {
    QString blkObjPath;
    UDisksClient *client { nullptr };
    bool deviceOpended { false };
    OperationErrorInfo lastError;
    QHash<QString, ulong> connections;
};
```

### GLib/UDisks 对象

**规则**: 全小写描述性名称

```cpp
UDisksClient *client { nullptr };
UDisksObject_autoptr udisksObj;
UDisksBlock_autoptr block;
UDisksDrive_autoptr drive;
```

## 枚举命名

### 枚举类型

**规则**: 大驼峰 (PascalCase)

```cpp
enum class DeviceType : uint16_t { ... };
enum class Property : uint16_t { ... };
enum class DeviceError : int16_t { ... };
```

### 枚举值

**规则**: `k` + 大驼峰 (PascalCase)

```cpp
enum class DeviceType : uint16_t {
    kAllDevice = 0,
    kBlockDevice = 1,
    kProtocolDevice = 2,
    kNetDevice = 3,
};

enum class Property : uint16_t {
    kNotInit = 0,
    kBlockProperty = 1,
    kDriveProperty = 30,
    kFileSystemProperty = 31,
};

enum class DeviceError : int16_t {
    kNoError = 0,
    kUnhandledError = 10001,
    kUDisksErrorFailed = 10000,
    kUserErrorNotMountable = 20000,
};
```

## 文件命名

| 类型 | 规则 | 示例 |
|------|------|------|
| 公共头文件 | 类名小写 + `.h` | `ddevice.h`, `dblockdevice.h` |
| 私有实现头文件 | 类名小写 + `_p.h` | `ddevice_p.h`, `dblockdevice_p.h` |
| 实现文件 | 类名小写 + `.cpp` | `ddevice.cpp`, `dblockdevice.cpp` |

## 类型别名

```cpp
using DeviceOperateCallback = std::function<void(bool, OperationErrorInfo)>;
using DeviceOperateCallbackWithMessage = std::function<void(bool, OperationErrorInfo, QString)>;
```

## 宏定义

```cpp
// 虚函数占位符
#define DMNT_VIRTUAL virtual

// 线程安全警告
#define warningIfNotInMain()    \
    {                          \
        if (qApp->thread() != QThread::currentThread()) \
            qWarning() << "<" << __PRETTY_FUNCTION__ << ">"; \
    }

// 错误域常量
#define UDISKS_ERR_DOMAIN "org.freedesktop.UDisks2.Error"
#define GIO_ERR_DOMAIN "g-io-error-quark"
#define GDBUS_ERR_DOMAIN "g-dbus-error-quark"
```

## 示例代码引用

| 文件路径 | 描述 |
|---------|------|
| `include/dfm-mount/dfm-mount/base/dmount_global.h:36-57` | 枚举命名示例 |
| `src/dfm-mount/private/dblockmonitor_p.h:48-53` | 静态回调函数命名 |
| `src/dfm-mount/private/dblockdevice_p.h:95-96` | 成员变量命名 |
