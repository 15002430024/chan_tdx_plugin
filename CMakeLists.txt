# ============================================================================
# 缠论通达信DLL插件 - CMake构建配置
# ============================================================================
cmake_minimum_required(VERSION 3.15)
project(chan_tdx_plugin VERSION 1.0.0 LANGUAGES CXX)

# ----------------------------------------------------------------------------
# 编译器设置
# ----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 强制32位编译 (通达信只支持32位DLL)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(WARNING "检测到64位编译器，请使用32位工具链!")
    message(WARNING "CMake命令: cmake -A Win32 ..")
endif()

# MSVC特定设置
if(MSVC)
    # 使用静态运行时 /MT (Release) 或 /MTd (Debug)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    # 编译选项
    add_compile_options(
        /W4                 # 警告级别4
        /WX-                # 警告不视为错误
        /utf-8              # 源文件UTF-8编码
        /Zc:__cplusplus     # 正确报告C++标准版本
    )
    
    # 禁用安全警告
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

# ----------------------------------------------------------------------------
# 源文件
# [v7.5] chan_core.cpp 为唯一算法真源，tdx_standard.cpp 为纯导出映射层
# ----------------------------------------------------------------------------
set(HEADER_FILES
    include/tdx_interface.h
    include/chan_types.h
    include/chan_core.h
    include/logger.h
    include/config_reader.h
)

# ----------------------------------------------------------------------------
# 主 DLL 目标（链接 chan_core.cpp 统一算法）
# ----------------------------------------------------------------------------
add_library(chan SHARED
    src/tdx_standard.cpp
    src/chan_core.cpp
    src/logger.cpp
    chan.def
)

target_include_directories(chan PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# MSVC优化选项
if(MSVC)
    target_compile_options(chan PRIVATE /W3 /O2)
endif()

# 输出目录
set_target_properties(chan PROPERTIES
    OUTPUT_NAME "chan"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# ----------------------------------------------------------------------------
# 安装配置
# ----------------------------------------------------------------------------
install(TARGETS chan
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

# ----------------------------------------------------------------------------
# 测试目标
# ----------------------------------------------------------------------------
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    # 测试可执行文件
    add_executable(test_chan_core
        test/test_chan_core.cpp
        src/chan_core.cpp
        src/logger.cpp
    )
    
    target_include_directories(test_chan_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    set_target_properties(test_chan_core PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    
    # 添加测试命令
    enable_testing()
    add_test(NAME ChanCoreTests COMMAND test_chan_core)
endif()

# ----------------------------------------------------------------------------
# 最小版DLL（用于调试）
# ----------------------------------------------------------------------------
add_library(chan_min SHARED
    src/tdx_minimal.cpp
    chan_min.def
)

set_target_properties(chan_min PROPERTIES
    OUTPUT_NAME "chan_min"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

if(MSVC)
    target_compile_options(chan_min PRIVATE /W3 /O2)
endif()

# chan_std 目标已合并到 chan 主目标（tdx_standard.cpp），不再单独构建

# ----------------------------------------------------------------------------
# 构建后复制到通达信目录（可选）
# ----------------------------------------------------------------------------
# 取消注释以下行并修改路径以自动复制DLL
# set(TDX_PLUGIN_DIR "C:/tdx/T0002/dlls")
# add_custom_command(TARGET chan POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:chan> ${TDX_PLUGIN_DIR}/
#     COMMENT "复制 chan.dll 到通达信插件目录"
# )

# ----------------------------------------------------------------------------
# 打印构建信息
# ----------------------------------------------------------------------------
message(STATUS "========================================")
message(STATUS "项目: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "编译器: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "目标架构: ${CMAKE_GENERATOR_PLATFORM}")
message(STATUS "========================================")
