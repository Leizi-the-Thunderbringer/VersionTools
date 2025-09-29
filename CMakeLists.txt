# CMakeLists.txt - 根目录
cmake_minimum_required(VERSION 3.20)

project(VersionTools 
    VERSION 1.0.0
    DESCRIPTION "Modern Version Control GUI Tool"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 平台检测
if(APPLE)
    set(PLATFORM_MACOS TRUE)
    set(GUI_BACKEND "SwiftUI")
    message(STATUS "Building for macOS with SwiftUI")
elseif(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    set(GUI_BACKEND "Qt")
    message(STATUS "Building for Windows with Qt")
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
    set(GUI_BACKEND "Qt")
    message(STATUS "Building for Linux with Qt")
endif()

# 编译选项 - 仅应用于C++代码
if(MSVC)
    add_compile_options(/W4)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
else()
    # 为C++设置警告选项，但不为Swift设置
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
    # 移除 -Werror 以避免构建问题
endif()

# Debug/Release配置
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    if(NOT MSVC)
        add_compile_options(-g -O0)
    endif()
else()
    add_definitions(-DNDEBUG)
    if(NOT MSVC)
        add_compile_options(-O3)
    endif()
endif()

# 查找依赖
if(GUI_BACKEND STREQUAL "Qt")
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
    if(Qt6_FOUND)
        message(STATUS "Found Qt6: ${Qt6_VERSION}")
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTOUIC ON)
        set(CMAKE_AUTORCC ON)
    else()
        message(FATAL_ERROR "Qt6 not found!")
    endif()
endif()

# Git依赖
find_package(PkgConfig)

# macOS上的特殊处理 - 如果在标准位置找不到，检查Homebrew位置
if(APPLE)
    # 首先尝试用pkg-config
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBGIT2 libgit2)
    endif()

    # 如果pkg-config没找到，手动查找
    if(NOT LIBGIT2_FOUND)
        # 查找Homebrew路径
        execute_process(
            COMMAND brew --prefix libgit2
            OUTPUT_VARIABLE BREW_LIBGIT2_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        if(BREW_LIBGIT2_PREFIX)
            set(LIBGIT2_INCLUDE_DIR "${BREW_LIBGIT2_PREFIX}/include")
            set(LIBGIT2_LIBRARY "${BREW_LIBGIT2_PREFIX}/lib/libgit2.dylib")

            if(EXISTS ${LIBGIT2_INCLUDE_DIR} AND EXISTS ${LIBGIT2_LIBRARY})
                set(LIBGIT2_FOUND TRUE)
                set(LIBGIT2_INCLUDE_DIRS ${LIBGIT2_INCLUDE_DIR})
                set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARY})
                message(STATUS "Found libgit2 via Homebrew at ${BREW_LIBGIT2_PREFIX}")
            endif()
        endif()
    endif()

    # 如果还没找到，尝试标准位置
    if(NOT LIBGIT2_FOUND)
        find_path(LIBGIT2_INCLUDE_DIR git2.h
            PATHS /usr/local/include /opt/homebrew/include /opt/local/include
        )
        find_library(LIBGIT2_LIBRARY NAMES git2
            PATHS /usr/local/lib /opt/homebrew/lib /opt/local/lib
        )
        if(LIBGIT2_INCLUDE_DIR AND LIBGIT2_LIBRARY)
            set(LIBGIT2_FOUND TRUE)
            set(LIBGIT2_INCLUDE_DIRS ${LIBGIT2_INCLUDE_DIR})
            set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARY})
            message(STATUS "Found libgit2 manually")
        endif()
    endif()
else()
    # Linux/Windows的原始代码
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBGIT2 libgit2)
        if(LIBGIT2_FOUND)
            message(STATUS "Found libgit2: ${LIBGIT2_VERSION}")
        endif()
    else()
        # 手动查找libgit2
        find_path(LIBGIT2_INCLUDE_DIR git2.h)
        find_library(LIBGIT2_LIBRARY NAMES git2)
        if(LIBGIT2_INCLUDE_DIR AND LIBGIT2_LIBRARY)
            set(LIBGIT2_FOUND TRUE)
            set(LIBGIT2_INCLUDE_DIRS ${LIBGIT2_INCLUDE_DIR})
            set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARY})
            message(STATUS "Found libgit2 manually")
        endif()
    endif()
endif()

if(NOT LIBGIT2_FOUND)
    message(WARNING "libgit2 not found, will use system git commands")
    add_definitions(-DUSE_SYSTEM_GIT)
endif()

# 线程支持
find_package(Threads REQUIRED)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/src)
if(LIBGIT2_FOUND)
    include_directories(${LIBGIT2_INCLUDE_DIRS})
endif()

# 添加子目录
add_subdirectory(src/core)

if(GUI_BACKEND STREQUAL "Qt")
    add_subdirectory(src/qt)
elseif(GUI_BACKEND STREQUAL "SwiftUI")
    add_subdirectory(src/macos)
endif()

# 安装规则
if(PLATFORM_LINUX)
    # Linux安装
    install(TARGETS VersionTools DESTINATION bin)
    if(EXISTS ${CMAKE_SOURCE_DIR}/resources/versiontools.desktop)
        install(FILES resources/versiontools.desktop DESTINATION share/applications)
    endif()
    if(EXISTS ${CMAKE_SOURCE_DIR}/resources/icons/versiontools.png)
        install(FILES resources/icons/versiontools.png DESTINATION share/icons/hicolor/256x256/apps)
    endif()
elseif(PLATFORM_WINDOWS)
    # Windows安装
    install(TARGETS VersionTools DESTINATION .)
    if(Qt6_FOUND)
        # 复制Qt依赖
        install(CODE "
            include(BundleUtilities)
            fixup_bundle(\${CMAKE_INSTALL_PREFIX}/VersionTools.exe \"\" \"\")
        ")
    endif()
elseif(PLATFORM_MACOS)
    # macOS应用包
    set_target_properties(VersionTools PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Version Tools"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_IDENTIFIER "com.versiontools.app"
    )
    install(TARGETS VersionTools BUNDLE DESTINATION .)
endif()

# 测试 (可选)
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests)
        add_subdirectory(tests)
    endif()
endif()

# CPack配置
include(CPack)
set(CPACK_PACKAGE_NAME "VersionTools")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern Version Control GUI Tool")
set(CPACK_PACKAGE_VENDOR "VersionTools Project")

if(PLATFORM_LINUX)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6widgets6, libqt6gui6, libgit2-dev")
    set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase, libgit2-devel")
elseif(PLATFORM_WINDOWS)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "Version Tools")
    set(CPACK_NSIS_PACKAGE_NAME "Version Tools")
elseif(PLATFORM_MACOS)
    set(CPACK_GENERATOR "DragNDrop")
endif()