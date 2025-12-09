#按c++11标准编译
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
#把所有源文件放到变量SRC_LIST中
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)
#生成可执行文件的依赖关系
add_executable(chatserver ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})
#可执行文件指定连接的库
target_link_libraries(chatserver muduo_net muduo_base mysqlclient hiredis pthread jsoncpp)