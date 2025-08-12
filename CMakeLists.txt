cmake_minimum_required(VERSION 3.10)

# ==============================================================================
# 项目配置
# ==============================================================================

# 指定编译器（必须在 project() 之前设置）
set(CMAKE_C_COMPILER "D:/software/x86_64-13.1.0-release-win32-seh-msvcrt-rt_v11-rev1/mingw64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "D:/software/x86_64-13.1.0-release-win32-seh-msvcrt-rt_v11-rev1/mingw64/bin/g++.exe")

# 定义项目
project("ModelLifeManager" 
    VERSION 1.0.0
    DESCRIPTION "Model Life Manager Application"
    LANGUAGES CXX C
)

# ==============================================================================
# 编译配置
# ==============================================================================

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 生成 compile_commands.json 用于 IDE 支持（如 VS Code IntelliSense）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 调试配置
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Debug 模式：添加调试信息，关闭优化，启用更多警告
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG -Wall -Wextra -Wpedantic")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG -Wall -Wextra")
    message(STATUS "Building in Debug mode with GDB support")
else()
    # Release 模式：开启优化，但保留调试信息
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -g -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O2 -g -DNDEBUG")
    message(STATUS "Building in Release mode")
endif()

# ==============================================================================
# 路径定义（便于维护）
# ==============================================================================

# MySQL 安装路径
set(MYSQL_ROOT "D:/software/mysql-8.0.42-winx64")
set(MYSQL_INCLUDE_DIR "${MYSQL_ROOT}/include")
set(MYSQL_LIB_DIR "${MYSQL_ROOT}/lib")

# MinGW 路径
set(MINGW_ROOT "D:/software/x86_64-13.1.0-release-win32-seh-msvcrt-rt_v11-rev1/mingw64")
set(MINGW_BIN_DIR "${MINGW_ROOT}/bin")

# 外部库路径
set(EXTERN_DIR "${PROJECT_SOURCE_DIR}/extern")

# ==============================================================================
# 头文件包含路径
# ==============================================================================

# 项目头文件路径
include_directories(
    # 项目内部头文件
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/entity"
    "${PROJECT_SOURCE_DIR}/include/util"
    "${PROJECT_SOURCE_DIR}/include/dao"
    "${PROJECT_SOURCE_DIR}/include/service"
    "${PROJECT_SOURCE_DIR}/include/controller"
    
    # 第三方库头文件
    "${MYSQL_INCLUDE_DIR}"                                      # MySQL 头文件
    "${EXTERN_DIR}"                                            # 外部库根目录
    "${EXTERN_DIR}/Crow-master/include"                        # Crow HTTP 框架
    "${EXTERN_DIR}/asio/asio-1.34.2/include"                  # ASIO 异步 I/O
    "${EXTERN_DIR}/json/json-develop/include"                  # JSON 库
    "${EXTERN_DIR}/jwt-cpp-master/include"                     # JWT 库
)

# ==============================================================================
# 链接库路径
# ==============================================================================

link_directories(
    "${MYSQL_LIB_DIR}"                      # MySQL 库文件
    "${EXTERN_DIR}/openssl/lib"             # OpenSSL 库文件
)

# ==============================================================================
# 源文件收集
# ==============================================================================

# 递归查找所有 .cpp 源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 显示找到的源文件数量（调试信息）
list(LENGTH SOURCES SOURCE_COUNT)
message(STATUS "Found ${SOURCE_COUNT} source files")

# 测试程序1
# add_executable(TicketTest test/ticketTest.cpp)
# target_link_libraries(TicketTest PRIVATE ModelLifeCore Catch2::Catch2WithMain)
# set_target_properties(TicketTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
# target_include_directories(TicketTest PRIVATE ${MY_INCLUDES})

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
add_executable(EncryptionKeyTest test/encryptionKeyTest.cpp)
target_link_libraries(EncryptionKeyTest PRIVATE ModelLifeCore Catch2::Catch2WithMain)
set_target_properties(EncryptionKeyTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
target_include_directories(EncryptionKeyTest PRIVATE ${MY_INCLUDES})


# ==============================================================================
# 链接库配置
# ==============================================================================

# 链接 MySQL 数据库驱动库
target_link_libraries(ModelLifeManager PRIVATE libmysql)

# 链接 Windows 网络相关库（用于 socket 支持）
target_link_libraries(ModelLifeManager PRIVATE ws2_32 mswsock)

# #宏定义 给出日志文件输出的位置
# target_compile_definitions(ModelLifeManager PRIVATE LOG_PATH="${CMAKE_BINARY_DIR}/logs/log.txt")
