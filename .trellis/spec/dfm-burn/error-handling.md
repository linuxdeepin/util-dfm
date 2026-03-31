# dfm-burn 错误处理规范

## 概述

dfm-burn 使用简单直接的错误处理模式：操作返回 `bool`，错误信息通过 `lastError()` 获取。

## 错误处理模式

### 1. errorMsg 成员变量

**私有实现类**:
```cpp
class DOpticalDiscManagerPrivate
{
public:
    QString errorMsg;  // 存储最后一次操作错误信息
};
```

### 2. 公共接口返回值

```cpp
class DOpticalDiscManager : public QObject
{
    Q_OBJECT

public:
    // 所有操作返回 bool
    bool setStageFile(const QString &diskPath, const QString &isoPath);
    bool commit(const BurnOptions &opts, int speed = 0, const QString &volId = "ISOIMAGE");
    bool eraseDisc(EraseType type);

    // 获取错误信息
    QString lastError() const;
};
```

### 3. 使用示例

```cpp
DOpticalDiscManager manager(devicePath);

if (!manager.commit(options)) {
    QString err = manager.lastError();
    qWarning() << "Burn failed:" << err;
} else {
    qDebug() << "Burn successful";
}
```

## 错误设置模式

**位置**: `src/dfm-burn/dfm-burn-lib/dopticaldiscmanager.cpp:34-47`

```cpp
bool DOpticalDiscManager::setStageFile(const QString &diskPath, const QString &isoPath)
{
    dptr->errorMsg.clear();

    QUrl diskUrl = QUrl::fromLocalFile(diskPath);
    if (diskUrl.isEmpty() || !diskUrl.isValid()) {
        dptr->errorMsg = "Invalid disk path";
        return false;
    }

    QUrl isoUrl = QUrl::fromLocalFile(isoPath);
    if (isoUrl.isEmpty() || !isoUrl.isValid()) {
        dptr->errorMsg = "Invalid ISO path";
        return false;
    }

    // ... 执行操作
    return true;
}
```

## 引擎错误传递

**位置**: `src/dfm-burn/dfm-burn-lib/dopticaldiscmanager.cpp:61-101`

```cpp
bool DOpticalDiscManager::commit(const BurnOptions &opts, int speed, const QString &volId)
{
    QScopedPointer<DXorrisoEngine> xorrisoEngine { new DXorrisoEngine };

    connect(xorrisoEngine.data(), &DXorrisoEngine::jobStatusChanged, this,
            [this, ptr = QPointer(xorrisoEngine.data())](JobStatus status, int progress, QString speed) {
                if (ptr)
                    Q_EMIT jobStatusChanged(status, progress, speed, ptr->takeInfoMessages());
            },
            Qt::DirectConnection);

    connect(xorrisoEngine.data(), &DXorrisoEngine::errorOccurred, this,
            [this, ptr = QPointer(xorrisoEngine.data())](const QString &err) {
                if (ptr)
                    dptr->errorMsg = err;
            },
            Qt::DirectConnection);

    ret = xorrisoEngine->doBurn(...);

    return ret;
}
```

## 错误处理最佳实践

### DO - 应该做

1. **在操作开始时清空错误**
```cpp
bool MyClass::doSomething() {
    d->errorMsg.clear();  // 清空之前的错误
    // ... 执行操作
}
```

2. **返回 bool 表示操作结果**
```cpp
bool MyClass::doSomething() {
    if (somethingWrong) {
        d->errorMsg = "Something went wrong";
        return false;
    }
    return true;
}
```

3. **使用 lastError() 获取错误**
```cpp
if (!obj.doOperation()) {
    qWarning() << obj.lastError();
}
```

### DON'T - 不应该做

1. **不要忽略错误检查**
```cpp
// 错误:
obj.doOperation();  // 不检查返回值

// 正确:
if (!obj.doOperation()) {
    // 处理错误
}
```

2. **不要使用异常**
```cpp
// 错误:
if (invalid) {
    throw std::runtime_error("Invalid");
}

// 正确:
if (invalid) {
    d->errorMsg = "Invalid";
    return false;
}
```

3. **不要忘记设置错误信息**
```cpp
// 错误:
if (invalid) {
    return false;  // 没有解释为什么失败
}

// 正确:
if (invalid) {
    d->errorMsg = "Invalid parameter: expected X got Y";
    return false;
}
```

## 信号报告进度

除了 `lastError()`，dfm-burn 使用信号报告进度和状态：

```cpp
class DOpticalDiscManager : public QObject
{
Q_SIGNALS:
    void jobStatusChanged(JobStatus status, int progress, QString speed, QStringList message);
};

// 连接信号
connect(&manager, &DOpticalDiscManager::jobStatusChanged,
        [](JobStatus status, int progress, QString speed, QStringList messages) {
    if (status == JobStatus::kFailed) {
        qWarning() << "Burn failed:" << messages.join(", ");
    }
});
```

## 示例代码引用

| 文件路径 | 描述 |
|---------|------|
| `src/dfm-burn/dfm-burn-lib/dopticaldiscmanager.cpp:34-47` | 错误设置示例 |
| `src/dfm-burn/dfm-burn-lib/dopticaldiscmanager.cpp:61-101` | 引擎错误传递 |
| `src/dfm-burn/dfm-burn-lib/dpacketwritingcontroller.cpp:162-163` | lastError() 实现 |
