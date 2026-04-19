#按c++11标准编译
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
#把所有源文件放到变量SRC_LIST中
aux_source_directory(. SRC_LIST)
#生成可执行文件的依赖关系
add_executable(chatclient ${SRC_LIST})
#指定可执行文件依赖库
target_link_libraries(chatclient pthread)
