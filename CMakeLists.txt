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

# ==============================================================================
# 可执行文件生成
# ==============================================================================

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

#测试程序5
add_executable(CustomerInfoTest test/customerInfoTest.cpp)
target_link_libraries(CustomerInfoTest PRIVATE ModelLifeCore Catch2::Catch2WithMain)
set_target_properties(CustomerInfoTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
target_include_directories(CustomerInfoTest PRIVATE ${MY_INCLUDES})

# 设置可执行文件输出路径到 build/bin 目录
set_target_properties(ModelLifeManager PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

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
    
    COMMENT "Copying required runtime DLLs to executable directory"
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