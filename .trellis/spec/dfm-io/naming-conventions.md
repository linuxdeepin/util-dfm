# dfm-io 命名约定

## 类命名

### 公共接口类

**规则**: `D` + 功能名词，大驼峰 (PascalCase)

```cpp
class DFile;
class DFileInfo;
class DOperator;
class DWatcher;
class DEnumerator;
class DFileFuture;
class DFileInfoFuture;
class DTrashHelper;
class DLocalHelper;
class DMediaInfo;
```

### 私有实现类

**规则**: 公共类名 + `Private`

```cpp
class DFilePrivate;
class DFileInfoPrivate;
class DOperatorPrivate;
class DWatcherPrivate;
class DEnumeratorPrivate;
class DFileFuturePrivate;
```

## 方法命名

### 公共方法

**规则**: 小驼峰 (camelCase)，动词开头

```cpp
// 文件操作
bool renameFile(const QString &newName);
bool copyFile(const QString &dest);
bool moveFile(const QString &dest);
bool deleteFile();

// 属性访问
QUrl uri() const;
bool exists() const;
void setUri(const QUrl &uri);

// 异步方法
void renameFileAsync(const QString &newName, RenameCallbackFunc cb, void *userData);
void copyFileAsync(const QString &dest, CopyCallbackFunc cb, void *userData);
```

### 私有方法

**规则**: 小驼峰

```cpp
void setErrorFromGError(GError *gerror);
void setErrorFromErrno(int errnoValue);
GFile *makeGFile(const QUrl &url);
void checkAndResetCancel();
```

## 成员变量命名

### d 和 q 指针

**规则**:
- `d`: 指向私有实现（公共类中）
- `q`: 指向公共类（私有实现类中）

```cpp
// 公共类
class DFileInfo {
private:
    mutable QSharedDataPointer<DFileInfoPrivate> d;  // d 指向私有实现
};

// 私有实现类
class DFileInfoPrivate {
public:
    DFileInfo *q { nullptr };  // q 指向公共类
};
```

### 普通成员变量

**规则**: 小驼峰命名

```cpp
class DWatcherPrivate {
    QUrl uri;
    QList<QUrl> children;
    GFileMonitor *monitor { nullptr };
    GCancellable *gcancellable { nullptr };
    DFMIOError error;
};
```

### GLib 对象

**规则**: 加 `g` 前缀或直接使用类型名

```cpp
GFile *gfile { nullptr };
GFileInfo *gfileinfo { nullptr };
GFileMonitor *monitor { nullptr };
GCancellable *gcancellable { nullptr };
```

## 枚举命名

### 枚举类型

**规则**: 大驼峰 (PascalCase)

```cpp
enum class DFileType : uint16_t { ... };
enum class DFileAttributeType : uint8_t { ... };
enum class FileQueryInfoFlags : uint8_t { ... };
```

### 枚举值

**规则**: `k` + 大驼峰 (PascalCase)

```cpp
enum class DFileType : uint16_t {
    kUnknown = 0,
    kRegular = 1,
    kDirectory = 2,
    kSymbolicLink = 3,
    kSpecial = 4,
    kShortcut = 5,
    kMountable = 6,
    kUserType = 0x100
};

enum class DFileAttributeType : uint8_t {
    kTypeInvalid = 0,
    kTypeString = 1,
    kTypeByteString = 2,
    kTypeBool = 3,
    kTypeUint32 = 4,
    kTypeInt32 = 5,
    kTypeUint64 = 6,
    kTypeInt64 = 7
};
```

## 文件命名

| 类型 | 规则 | 示例 |
|------|------|------|
| 公共头文件 | 类名小写 + `.h` | `dfile.h`, `dfileinfo.h` |
| 私有实现头文件 | 类名小写 + `_p.h` | `dfile_p.h`, `dfileinfo_p.h` |
| 实现文件 | 类名小写 + `.cpp` | `dfile.cpp`, `dfileinfo.cpp` |
| 工具类 | 小驼峰 + `.h` | `dlocalhelper.h`, `dmediainfo.h` |

## 命名空间

```cpp
// 全局头文件定义宏
#define DFMIO dfmio
#define BEGIN_IO_NAMESPACE namespace DFMIO {
#define USING_IO_NAMESPACE using namespace DFMIO;
#define END_IO_NAMESPACE }

// 使用
BEGIN_IO_NAMESPACE
class DFileInfo { /* ... */ };
END_IO_NAMESPACE
```

## 类型别名

```cpp
// 回调函数类型
using ProgressCallbackFunc = void (*)(int64_t, int64_t, void *);
using FileOperateCallbackFunc = void (*)(bool, void *);

// std::function 类型
using InitQuerierAsyncCallback = std::function<void(bool, void *)>;
using AttributeAsyncCallback = std::function<void(bool, void *, QVariant)>;
```

## 示例代码引用

| 文件路径 | 描述 |
|---------|------|
| `include/dfm-io/dfm-io/dfileinfo.h:27-191` | 枚举命名示例 |
| `src/dfm-io/dfm-io/private/dfileinfo_p.h` | 私有类命名示例 |
| `include/dfm-io/dfm-io/doperator.h:25-27` | 回调函数类型定义 |
