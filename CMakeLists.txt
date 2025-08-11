cmake_minimum_required(VERSION 3.10)

# 指定编译器（必须在project之前设置）
set(CMAKE_C_COMPILER "D:/software/x86_64-13.1.0-release-win32-seh-msvcrt-rt_v11-rev1/mingw64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "D:/software/x86_64-13.1.0-release-win32-seh-msvcrt-rt_v11-rev1/mingw64/bin/g++.exe")

project("ModelLifeManager")


#头文件路径 
include_directories(
    # "C:/Program Files/MySQL/MySQL Server 8.0/include"
    "D:/software/mysql-8.0.42-winx64/include"
    "include"
    "include/entity"
    "include/util"
    "include/dao"
    "include/service"
    "include/controller"
    "${PROJECT_SOURCE_DIR}/extern/"
    "${PROJECT_SOURCE_DIR}/extern/Crow-master/include"
    "${PROJECT_SOURCE_DIR}/extern/asio/asio-1.34.2/include"
    "${PROJECT_SOURCE_DIR}/extern/json/json-develop/include"
    "${PROJECT_SOURCE_DIR}/extern/jwt-cpp-master/include"
)

#指定链接库的路径
# link_directories("C:/Program Files/MySQL/MySQL Server 8.0/lib")
link_directories(
    "D:/software/mysql-8.0.42-winx64/lib"
    "${PROJECT_SOURCE_DIR}/extern/openssl/lib"
)

#查找所有源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

#生成可执行文件
add_executable(ModelLifeManager ${SOURCES})
#设置输出文件路径   
set_target_properties(ModelLifeManager PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

<<<<<<< HEAD
#指定需要链接的具体的库
target_link_libraries(ModelLifeManager PRIVATE libmysql)    # MySQL数据库驱动库
target_link_libraries(ModelLifeManager PRIVATE ws2_32 mswsock) # Windows网络相关库（socket支持）
target_link_libraries(ModelLifeManager PRIVATE ssl crypto)  # OpenSSL加密库（jwt-cpp等依赖）

target_compile_definitions(ModelLifeManager PRIVATE LOG_PATH="${CMAKE_BINARY_DIR}/logs/log.txt")
=======
# 测试程序1也链接它
# add_executable(TicketTest test/ticketTest.cpp)
# target_link_libraries(TicketTest PRIVATE ModelLifeCore Catch2::Catch2WithMain)
# set_target_properties(TicketTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
# target_include_directories(TicketTest PRIVATE ${MY_INCLUDES})

# 测试程序2也链接它
# add_executable(UserTest test/userTest.cpp)
# target_link_libraries(UserTest PRIVATE ModelLifeCore Catch2::Catch2WithMain)
# set_target_properties(UserTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
# target_include_directories(UserTest PRIVATE ${MY_INCLUDES})

# 测试程序3也链接它
add_executable(ModelTest test/modelTest.cpp)
target_link_libraries(ModelTest PRIVATE ModelLifeCore Catch2::Catch2WithMain)
set_target_properties(ModelTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
target_include_directories(ModelTest PRIVATE ${MY_INCLUDES})

#catch_discover_tests(TicketTest)

# # 主程序链接这个库
# add_executable(ModelLifeManager src/main.cpp)
# #主程序链接的核心库
# target_link_libraries(ModelLifeManager PRIVATE ModelLifeCore)
# #指定需要链接的具体的库 (web,加密相关的库)
# target_link_libraries(ModelLifeManager PRIVATE  ws2_32 mswsock "${CMAKE_SOURCE_DIR}/extern/openssl/lib/libcrypto.lib")
# target_include_directories(ModelLifeManager PRIVATE ${MY_INCLUDES})

# #设置可执行文件输出的位置
# set_target_properties(ModelLifeManager PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

# #宏定义 给出日志文件输出的位置
# target_compile_definitions(ModelLifeManager PRIVATE LOG_PATH="${CMAKE_BINARY_DIR}/logs/log.txt")
>>>>>>> 088b7a22b195f8695000948e362f1f54cbf2dc2e

# 自动复制所需的DLL文件到输出目录
add_custom_command(TARGET ModelLifeManager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "D:/software/mysql-8.0.42-winx64/lib/libmysql.dll"
        $<TARGET_FILE_DIR:ModelLifeManager>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "D:/software/x86_64-13.1.0-release-win32-seh-msvcrt-rt_v11-rev1/mingw64/bin/libgcc_s_seh-1.dll"
        $<TARGET_FILE_DIR:ModelLifeManager>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "D:/software/x86_64-13.1.0-release-win32-seh-msvcrt-rt_v11-rev1/mingw64/bin/libstdc++-6.dll"
        $<TARGET_FILE_DIR:ModelLifeManager>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "D:/software/x86_64-13.1.0-release-win32-seh-msvcrt-rt_v11-rev1/mingw64/bin/libwinpthread-1.dll"
        $<TARGET_FILE_DIR:ModelLifeManager>
)