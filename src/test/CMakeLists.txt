# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)

# 项目信息
project (test_prj CXX)


# include directories
INCLUDE_DIRECTORIES(system ../include/)
INCLUDE_DIRECTORIES(after  ${PROJECT_SOURCE_DIR})



# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量
file(GLOB_RECURSE SRCS_ALL  ${PROJECT_SOURCE_DIR} "*.cpp")

# 指定生成目标
ADD_EXECUTABLE(test ${SRCS_ALL})


#设置依赖
ADD_DEPENDENCIES(test  libws)
# 设置链接库
target_link_libraries(test libws)
