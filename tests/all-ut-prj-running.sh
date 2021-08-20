#!/bin/bash

export DISPLAY=":0"
export QT_QPA_PLATFORM=

# 定位脚本所在父目录
PROJECT_FOLDER=${1}

TESTS_FOLDER=$PROJECT_FOLDER/tests
SRC_FOLDER=$PROJECT_FOLDER/src

echo $TESTS_FOLDER
echo $SRC_FOLDER

# 定位build_ut
BUILD_DIR=${2}

UT_TESTS_FOLDER=$BUILD_DIR/tests
UT_SRC_FOLDER=$BUILD_DIR/src

echo $UT_SRC_FOLDER

#运行UT类型 all,后续需要补充
UT_PRJ_TYPE=${3}
UT_TYPE_ALL="all"
UT_TYPE_UTIL_DFM_IO="dfm-io"
UT_TYPE_UTIL_DFM_BURN="dfm-burn"

REBUILD_PRJ=${4}
REBUILD_TYPE_YES="yes"

#qmake 参数
QMAKE_ARGS="-spec linux-g++ CONFIG+=debug"
#CPU 个数
CPU_NUMBER=${5}
#是否显示报告
SHOW_REPORT=${6}

check_ut_result()
{
  if [ $1 != 0 ]; then
     echo "Error: UT process is broken by: " $2 ",end with: "$1
     exit $1
  fi
}

# 打印当前目录，当前目录应当是 build-ut
echo `pwd`
echo "start util-dfm all UT cases:" $UT_PRJ_TYPE

# 下面是编译和工程测试
# 1. 编译工程
mkdir -p $UT_TESTS_FOLDER
cd $UT_TESTS_FOLDER
cmake $TESTS_FOLDER
make -j$CPU_NUMBER


# 下面是编译和工程测试
# -------------------------------------------------------------------------------------------

#> 1. dfm-io 单元测试与覆盖率测试
if [ "$UT_PRJ_TYPE" = "$UT_TYPE_ALL" ] || [ "$UT_PRJ_TYPE" = "$UT_TYPE_UTIL_DFM_IO" ] ; then
	echo $UT_TYPE_UTIL_DFM_IO "test case is running"

    DIR_TEST_DFM_IO=$UT_TESTS_FOLDER/dfm-io
	cd $DIR_TEST_DFM_IO


	dfm_io_extract_path="*/src/dfm-io/*"
	dfm_io_remove_path="*/third-party/* *tests* */build-ut/* *moc_* *qrc_*"
	# report的文件夹，报告后缀名，编译路径，可执行程序名，正向解析设置，逆向解析设置
	./../../../../tests/ut-target-running.sh $BUILD_DIR dfm-io $DIR_TEST_DFM_IO test-dfm-io "$dfm_io_extract_path" "$dfm_io_remove_path" $SHOW_REPORT
        check_ut_result $? $UT_TYPE_FILE_MANAGER
fi

# TODO 其它模块的覆盖测试

echo "end util-dfm all UT cases"

exit 0
