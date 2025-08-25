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

# ==============================================================================
# 可执行文件生成
# ==============================================================================

# 生成可执行文件
add_executable(ModelLifeManager ${SOURCES})

# 设置可执行文件输出路径到 build/bin 目录
set_target_properties(ModelLifeManager PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

# ==============================================================================
# 链接库配置
# ==============================================================================

# 链接 MySQL 数据库驱动库
target_link_libraries(ModelLifeManager PRIVATE libmysql)

# 链接 Windows 网络相关库（用于 socket 支持）
target_link_libraries(ModelLifeManager PRIVATE ws2_32 mswsock)

# 链接 OpenSSL 加密库（JWT 等依赖）
target_link_libraries(ModelLifeManager PRIVATE ssl crypto)

# ==============================================================================
# 编译定义和包含路径
# ==============================================================================

# 添加私有包含路径（避免污染全局）
target_include_directories(ModelLifeManager PRIVATE 
    "${EXTERN_DIR}/Crow-master/include"
)

# 定义编译时宏
target_compile_definitions(ModelLifeManager PRIVATE 
    LOG_PATH="${CMAKE_BINARY_DIR}/logs/log.txt"     # 日志文件路径
    $<$<CONFIG:Debug>:DEBUG_MODE=1>                # Debug 模式宏
)

# ==============================================================================
# 运行时文件复制
# ==============================================================================

# 创建必要的目录
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/logs")

# 自动复制运行时需要的 DLL 文件
add_custom_command(TARGET ModelLifeManager POST_BUILD
    # MySQL 动态库
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MYSQL_LIB_DIR}/libmysql.dll"
        $<TARGET_FILE_DIR:ModelLifeManager>
    
    # MinGW 运行时库
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
        $<TARGET_FILE_DIR:ModelLifeManager>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MINGW_BIN_DIR}/libstdc++-6.dll"
        $<TARGET_FILE_DIR:ModelLifeManager>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MINGW_BIN_DIR}/libwinpthread-1.dll"
        $<TARGET_FILE_DIR:ModelLifeManager>
        
    # 复制 config.ini 到 bin 目录
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/config/config.ini"
        $<TARGET_FILE_DIR:ModelLifeManager>
    COMMENT "Copying required runtime DLLs and config.ini to executable directory"
)

# ==============================================================================
# 构建信息显示
# ==============================================================================

# 显示重要的构建信息
message(STATUS "=== Build Configuration ===")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "MySQL: ${MYSQL_ROOT}")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "Source files: ${SOURCE_COUNT}")
message(STATUS "=============================")

# ==============================================================================
# 可选：安装配置（如果需要打包分发）
# ==============================================================================

# 设置安装路径
# install(TARGETS ModelLifeManager
#     RUNTIME DESTINATION bin
#     COMPONENT Runtime
# )

# install(FILES config.ini
#     DESTINATION bin
#     COMPONENT Configuration
# )