# Unit Tests for util-dfm

This directory contains unit tests for the util-dfm libraries using the Qt Test framework.

## Directory Structure

```
autotests/
├── CMakeLists.txt           # Main test configuration
├── README.md                # This file
├── dfm-io-tests/            # Tests for dfm-io library
│   ├── CMakeLists.txt
│   └── tst_dfm_io.cpp
├── dfm-mount-tests/         # Tests for dfm-mount library
│   ├── CMakeLists.txt
│   └── tst_dfm_mount.cpp
├── dfm-burn-tests/          # Tests for dfm-burn library
│   ├── CMakeLists.txt
│   └── tst_dfm_burn.cpp
└── dfm-search-tests/        # Tests for dfm-search library
    ├── CMakeLists.txt
    └── tst_dfm_search.cpp
```

## Building with Tests

By default, unit tests are enabled. To build with tests:

```bash
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr -j$(nproc) ..
cmake --build .
```

To disable building of tests:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_UNIT_TESTS=OFF -j$(nproc) ..
```

## Running Tests

### Run all tests

```bash
# From build directory
ctest --output-on-failure

# Or use the custom target
make test-all
```

### Run individual module tests

```bash
# From build directory
./bin/dfm-io-test
./bin/dfm-mount-test
./bin/dfm-burn-test
./bin/dfm-search-test
```

### Run with verbose output

```bash
ctest --verbose

# Or for individual tests
./bin/dfm-io-test -vs2
```

## Writing New Tests

1. Choose the appropriate test subdirectory for the module you're testing
2. Create a new test file following the naming convention `tst_*.cpp`
3. Include `<QTest>` header
4. Create a test class inheriting from `QObject` with `Q_OBJECT` macro
5. Use `QTEST_MAIN()` macro at the end of your test file
6. Update the corresponding CMakeLists.txt to include your new file

Example test:

```cpp
#include <QTest>

class tst_MyTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void myTestFunction();
};

void tst_MyTest::initTestCase()
{
    // Setup that runs once before all tests
}

void tst_MyTest::cleanupTestCase()
{
    // Cleanup that runs once after all tests
}

void tst_MyTest::myTestFunction()
{
    QVERIFY(true);                 // Verifies condition is true
    QCOMPARE(1 + 1, 2);           // Verifies equality
    QVERIFY_EXCEPTION_THROWN(
        throw std::runtime_error("Test"),
        std::runtime_error
    );
}

QTEST_MAIN(tst_MyTest)
#include "tst_mytest.moc"
```

## Qt Version Compatibility

The test system automatically detects Qt versions (Qt5 or Qt6) and links against the appropriate test library:

- Qt6: `Qt6::Test`
- Qt5: `Qt5::Test`

Library names are also adjusted based on Qt version:
- Qt6: `dfm6-io`, `dfm6-mount`, `dfm6-burn`, `dfm6-search`
- Qt5: `dfm-io`, `dfm-mount`, `dfm-burn`, `dfm-search`
