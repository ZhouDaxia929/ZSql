cmake_minimum_required(VERSION 3.13)
project(ZSql)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXE_LINKER_FLAGS -static)

#声明头文件路径
set(INC_DIR D:\\MySoftware\\MySQL\\Connector-C\\include)

#声明链接库路径
set(LINK_DIR D:\\MySoftware\\MySQL\\Connector-C\\lib)

#引入头文件
include_directories(${INC_DIR})

#引入库文件
link_directories(${LINK_DIR})
link_libraries(libmysql)

add_executable(ZSql main.cpp ZSql.cpp ZSql.h ResultData.h ResultData.cpp)

target_link_libraries(ZSql libmysql)