# 测试规范

> DDE 应用的 Qt Test 测试框架规范。

---

## 概览

本指南涵盖使用 Qt Test 框架编写 DDE 应用测试的模式和最佳实践。

---

## Qt Test 框架

### 基本测试结构

```cpp
#include <QtTest>
#include <DWidget>

class TestMyWidget : public QObject
{
    Q_OBJECT

private slots:
    // 设置/清理
    void initTestCase();      // 所有测试前调用一次
    void cleanupTestCase();   // 所有测试后调用一次
    void init();              // 每个测试前调用
    void cleanup();            // 每个测试后调用

    // 测试用例
    void testConstructor();
    void testBasicFunctionality();
    void testEdgeCases();
    void testSignals();
};

void TestMyWidget::initTestCase()
{
    // 一次性初始化
    qRegisterMetaType<MyCustomType>();
}

void TestMyWidget::cleanupTestCase()
{
    // 一次性清理
}

void TestMyWidget::init()
{
    // 每个测试的设置
    m_widget = new MyWidget();
}

void TestMyWidget::cleanup()
{
    // 每个测试的清理
    delete m_widget;
    m_widget = nullptr;
}

void TestMyWidget::testConstructor()
{
    QVERIFY(m_widget != nullptr);
    QCOMPARE(m_widget->isEnabled(), true);
}

QTEST_MAIN(TestMyWidget)
#include "test_mywidget.moc"
```

### 常见断言

```cpp
// 布尔检查
QVERIFY(condition);              // 条件为真
QVERIFY2(condition, "message");  // 带自定义消息

// 相等性检查
QCOMPARE(actual, expected);      // 使用 operator==

// 类型检查
QCOMPARE(actual.type(), ExpectedType);

// 空指针检查
QVERIFY(ptr != nullptr);
QVERIFY(ptr == nullptr);

// 字符串比较
QCOMPARE(actualString, expectedString);
QVERIFY(actualString.contains("substring"));
QVERIFY(actualString.startsWith("prefix"));
QVERIFY(actualString.isEmpty());

// 数值比较
QCOMPARE(actual, expected);
QVERIFY(actual > expected);
QVERIFY(qFuzzyCompare(actual, expected));  // 浮点数
```

### 数据驱动测试

```cpp
private slots:
    void testValidation_data();
    void testValidation();

void TestMyWidget::testValidation_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("expectedValid");
    QTest::addColumn<QString>("expectedError");

    QTest::newRow("valid email") << "test@example.com" << true << "";
    QTest::newRow("invalid email") << "invalid" << false << "Invalid format";
    QTest::newRow("empty") << "" << false << "Required field";
    QTest::newRow("null chars") << QString() << false << "Required field";
}

void TestMyWidget::testValidation()
{
    QFETCH(QString, input);
    QFETCH(bool, expectedValid);
    QFETCH(QString, expectedError);

    bool isValid = m_widget->validate(input);
    QCOMPARE(isValid, expectedValid);

    if (!expectedValid) {
        QCOMPARE(m_widget->errorString(), expectedError);
    }
}
```

### 信号测试

```cpp
#include <QSignalSpy>

void TestMyWidget::testSignals()
{
    QSignalSpy spy(m_widget, &MyWidget::valueChanged);

    // 触发信号
    m_widget->setValue(42);

    // 验证信号已发出
    QCOMPARE(spy.count(), 1);

    // 验证信号参数
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 42);
}

void TestMyWidget::testMultipleSignals()
{
    QSignalSpy valueSpy(m_widget, &MyWidget::valueChanged);
    QSignalSpy errorSpy(m_widget, &MyWidget::errorOccurred);

    m_widget->setValue(100);

    QCOMPARE(valueSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 0);

    m_widget->setInvalidValue();

    QCOMPARE(valueSpy.count(), 1);  // 没有新信号
    QCOMPARE(errorSpy.count(), 1);
}
```

### 使用 QSignalSpy 进行 Mock

```cpp
// 使用信号的 Mock 对象
class MockService : public QObject {
    Q_OBJECT
public:
    void emitDataReady(const QString &data) {
        emit dataReady(data);
    }
signals:
    void dataReady(const QString &data);
};

void TestIntegration::testServiceIntegration()
{
    MockService mockService;
    QSignalSpy spy(&mockService, &MockService::dataReady);

    m_widget->setService(&mockService);
    mockService.emitDataReady("test data");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(m_widget->currentData(), QString("test data"));
}
```

### 测试异步操作

```cpp
void TestAsync::testAsyncOperation()
{
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);

    connect(m_worker, &Worker::finished, &loop, &QEventLoop::quit);

    m_worker->startAsync();
    loop.exec();  // 等待完成

    QVERIFY(m_worker->isComplete());
    QCOMPARE(m_worker->result(), expectedResult);
}

// 或使用 QTRY_VERIFY
void TestAsync::testWithTryVerify()
{
    m_worker->startAsync();

    // 等待最多 5 秒满足条件
    QTRY_VERIFY_WITH_TIMEOUT(m_worker->isComplete(), 5000);

    QCOMPARE(m_worker->result(), expectedResult);
}
```

### 测试 D-Bus

```cpp
void TestDBus::testDBusCall()
{
    // 注册 mock 服务
    QDBusConnection bus = QDBusConnection::sessionBus();

    MockDBusAdaptor adaptor(m_mockService);
    bus.registerService("org.deepin.Test");
    bus.registerObject("/Test", m_mockService);

    // 测试调用
    QDBusInterface iface("org.deepin.Test", "/Test");
    QDBusReply<QString> reply = iface.call("GetMethod");

    QVERIFY(reply.isValid());
    QCOMPARE(reply.value(), QString("expected"));

    // 清理
    bus.unregisterObject("/Test");
    bus.unregisterService("org.deepin.Test");
}
```

---

## 覆盖率分析

### 启用覆盖率 (CMake)

```cmake
# 启用覆盖率标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

# 查找 gcov/lcov
find_program(GCOV_PATH gcov)
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)
```

### 生成覆盖率报告

```bash
# 运行测试
./test_myapp

# 生成覆盖率数据
lcov --capture --directory . --output-file coverage.info

# 过滤系统头文件
lcov --remove coverage.info '/usr/*' --output-file coverage.info

# 生成 HTML 报告
genhtml coverage.info --output-directory coverage_html
```

### 覆盖率目标

```cmake
# 添加覆盖率目标
add_custom_target(coverage
    COMMAND ${LCOV_PATH} --capture --directory . --output-file coverage.info
    COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' --output-file coverage.info
    COMMAND ${GENHTML_PATH} coverage.info --output-directory coverage_html
    DEPENDS test_myapp
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating coverage report..."
)
```

---

## 测试组织

### 目录结构

```
tests/
├── CMakeLists.txt
├── test_main.cpp          # 公共测试设置
├── unit/
│   ├── test_widget.cpp
│   ├── test_model.cpp
│   └── test_service.cpp
├── integration/
│   ├── test_dbus.cpp
│   └── test_workflow.cpp
└── mocks/
    ├── mock_service.h
    └── mock_dbus.h
```

### 测试 CMakeLists.txt

```cmake
enable_testing()

# 查找 Qt Test
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

# 添加测试可执行文件
add_executable(test_myapp
    test_main.cpp
    unit/test_widget.cpp
    unit/test_model.cpp
)

target_link_libraries(test_myapp
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    myapp_lib
)

# 使用 CTest 注册
add_test(NAME test_myapp COMMAND test_myapp)
```

---

## 最佳实践

1. **每个类一个测试文件**
2. **描述性命名测试**: `test<方法>_<场景>_<期望结果>`
3. **测试边界情况**: null、空、最大值
4. **保持测试独立**: 没有共享状态
5. **正确使用 init/cleanup**
6. **Mock 外部依赖**
7. **目标 >80% 覆盖率**
8. **在 CI 中运行测试**
9. **测试成功和失败路径**

---

## 测试用例模板

```cpp
void TestMyClass::testMethodName_scenario_expectedResult()
{
    // given
    QString input = "test input";
    QString expected = "expected output";

    // when
    QString actual = m_object->methodName(input);

    // then
    QCOMPARE(actual, expected);
}
```
