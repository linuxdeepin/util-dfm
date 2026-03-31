# dfm-burn 命名约定

## 类命名

### 公共接口类

**规则**: `D` + 功能名词，大驼峰 (PascalCase)

```cpp
class DOpticalDiscManager;       // 光盘管理器
class DOpticalDiscInfo;          // 光盘信息
class DPacketWritingController;  // 包写入控制器
```

### 引擎类

**规则**: `D` + 引擎名称 + `Engine`

```cpp
class DXorrisoEngine;            // XORRISO 刻录引擎
class DUDFBurnEngine;            // UDF 刻录引擎
```

## 方法命名

### 公共方法

**规则**: 小驼峰 (camelCase)

```cpp
// 操作方法
bool setStageFile(const QString &diskPath, const QString &isoPath);
bool commit(const BurnOptions &opts, int speed = 0, const QString &volId = "ISOIMAGE");
bool eraseDisc(EraseType type);

// 错误获取
QString lastError() const;

// 光盘信息查询
bool hasMedium() const;
const QString &volumeId() const;
quint64 usedSize() const;
quint64 totalSize() const;
```

## 成员变量命名

### 普通成员

**规则**: 小驼峰命名

```cpp
class DOpticalDiscManagerPrivate {
public:
    QString errorMsg;
    QString curDev;
    QPair<QString, QString> files;   // first: local disk, second: optical disk
};

class DOpticalDiscInfoPrivate {
public:
    bool formatted {};
    MediaType media;
    quint64 data {};
    QString devid {};
    QString volid {};
};
```

### 布尔成员

**规则**: `is` 或 `has` 前缀

```cpp
class DOpticalDiscInfoPrivate {
public:
    bool hasMedium() const;
    bool isBlank() const;
    bool isRewritable() const;
    bool isFormatted() const;
};
```

## 枚举命名

### 枚举类型

**规则**: 大驼峰 (PascalCase)

```cpp
enum JobStatus { ... };
enum MediaType { ... };
enum EraseType { ... };
enum BurnType { ... };
```

### 枚举值

**规则**: 大驼峰 (PascalCase)

```cpp
enum JobStatus {
    kIdle,
    kRunning,
    kFinished,
    kFailed
};

enum MediaType {
    kUnknown,
    kCD,
    kDVD,
    kBD,
    kHDDVD
};

enum EraseType {
    kFast,
    kComplete
};

enum BurnType {
    kISO9660,
    kUDF
};
```

## 文件命名

| 类型 | 规则 | 示例 |
|------|------|------|
| 公共头文件 | 类名小写 + `.h` | `dopticaldiscmanager.h` |
| 私有实现头文件 | 类名小写 + `_p.h` | `dopticaldiscmanager_p.h` |
| 实现文件 | 类名小写 + `.cpp` | `dopticaldiscmanager.cpp` |

## 信号命名

```cpp
class DOpticalDiscManager : public QObject {
Q_SIGNALS:
    void jobStatusChanged(JobStatus status, int progress, QString speed, QStringList message);
};

class DXorrisoEngine : public QObject {
Q_SIGNALS:
    void jobStatusChanged(JobStatus status, int progress, QString speed);
    void errorOccurred(const QString &error);
};
```

## 命名空间

```cpp
// 全局头文件定义宏
#define DFM_BURN_BEGIN_NS namespace DFMBURN {
#define DFM_BURN_END_NS }
#define DFM_BURN_USE_NS using namespace DFMBURN;

// 使用
DFM_BURN_BEGIN_NS
class DOpticalDiscManager { /* ... */ };
DFM_BURN_END_NS
```

## 示例代码引用

| 文件路径 | 描述 |
|---------|------|
| `src/dfm-burn/dfm-burn-lib/private/dpacketwritingcontroller_p.h:12-28` | 成员变量命名 |
| `src/dfm-burn/dfm-burn-lib/private/dopticaldiscinfo_p.h:44-61` | 布尔成员命名 |
| `include/dfm-burn/dfm-burn/dopticaldiscinfo.h:26-42` | 枚举命名 |
