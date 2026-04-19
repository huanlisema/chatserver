#按c++11标准编译
set(CMAKE_CXX_STANDARD 11)
#配置头文件搜索路径
#include_directories()
#配置库文件搜索路径
#link_directories()

#设置要编译的源文件列表
set(SRC_LIST ./muduo_server.cc)

#设置可执行文件最终储存路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#把.指定路径的文件名放到变量SRC_LIST里
#aux_source_directory(. SRC_LIST)

#编译形成可执行文件的依赖关系
add_executable(server ${SRC_LIST})

#目标程序要连接的库名称
target_link_libraries(server muduo_net muduo_base pthread)