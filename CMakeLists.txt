project("ModelLifeManager")

cmake_minimum_required(VERSION 4.0)

#include(${CMAKE_SOURCE_DIR}/extern/Catch2-3.5.3/extras/Catch.cmake)
# 启用测试功能
#enable_testing()
# 👇 这一步非常关键！！自动注册测试用例
include(CTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexec-charset=UTF-8")

set(MY_INCLUDES
    "C:/Program Files/MySQL/MySQL Server 8.0/include"
    "include"
    "include/entity"
    "include/util"
    "include/dao"
    "include/service"
    "include/controller"
    "${PROJECT_SOURCE_DIR}/extern/Crow-master/include"
    "${PROJECT_SOURCE_DIR}/extern/asio/asio-1.34.2/include"
    "${PROJECT_SOURCE_DIR}/extern/json/json-develop/include"
    "extern/Crow-master/include"
    "extern/jwt-cpp-master/include"
    "extern"
)

file(GLOB_RECURSE SOURCES "src/*.cpp")
# 手动移除 main.cpp
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")

#编译成静态库供外部调用(供测试程序或者工程本身)
add_library(ModelLifeCore STATIC ${SOURCES})
target_link_libraries(ModelLifeCore PRIVATE "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.lib")
target_include_directories(ModelLifeCore PRIVATE ${MY_INCLUDES})
add_subdirectory(extern/Catch2-3.5.3)

# 测试程序1
add_executable(TicketTest test/ticketTest.cpp)
target_link_libraries(TicketTest PRIVATE ModelLifeCore Catch2::Catch2WithMain)
set_target_properties(TicketTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
target_include_directories(TicketTest PRIVATE ${MY_INCLUDES})

# 测试程序2
# add_executable(UserTest test/userTest.cpp)
# target_link_libraries(UserTest PRIVATE ModelLifeCore Catch2::Catch2WithMain)
# set_target_properties(UserTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
# target_include_directories(UserTest PRIVATE ${MY_INCLUDES})

# 测试程序3
# add_executable(ModelTest test/modelTest.cpp)
# target_link_libraries(ModelTest PRIVATE ModelLifeCore Catch2::Catch2WithMain)
# set_target_properties(ModelTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
# target_include_directories(ModelTest PRIVATE ${MY_INCLUDES})

# 测试程序4
# add_executable(EncryptionKeyTest test/encryptionKeyTest.cpp)
# target_link_libraries(EncryptionKeyTest PRIVATE ModelLifeCore Catch2::Catch2WithMain)
# set_target_properties(EncryptionKeyTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
# target_include_directories(EncryptionKeyTest PRIVATE ${MY_INCLUDES})


#catch_discover_tests(TicketTest)

#主程序链接这个库
add_executable(ModelLifeManager src/main.cpp)
#主程序链接的核心库
target_link_libraries(ModelLifeManager PRIVATE ModelLifeCore)
#指定需要链接的具体的库 (web,加密相关的库)
target_link_libraries(ModelLifeManager PRIVATE  ws2_32 mswsock "${CMAKE_SOURCE_DIR}/extern/openssl/lib/libcrypto.lib")
set_target_properties(ModelLifeManager PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
target_include_directories(ModelLifeManager PRIVATE ${MY_INCLUDES})

# #设置可执行文件输出的位置
# set_target_properties(ModelLifeManager PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

# #宏定义 给出日志文件输出的位置
# target_compile_definitions(ModelLifeManager PRIVATE LOG_PATH="${CMAKE_BINARY_DIR}/logs/log.txt")
