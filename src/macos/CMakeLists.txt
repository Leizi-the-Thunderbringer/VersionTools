cmake_minimum_required(VERSION 3.20)

if(NOT GUI_BACKEND STREQUAL "SwiftUI")
    return()
endif()

# 启用Swift支持
enable_language(Swift)
set(CMAKE_Swift_LANGUAGE_VERSION 5.0)

# SwiftUI应用程序源文件
set(SWIFT_SOURCES
    VersionToolsApp.swift
    ContentView.swift
    Views/SidebarView.swift
    Views/ChangesView.swift
    Views/HistoryView.swift
    Views/BranchesView.swift
    Views/DiffView.swift
    Views/PlaceholderViews.swift
    Views/Dialogs.swift
    Views/StashView.swift
    Views/RemotesView.swift
    Views/TagsView.swift
    Models/GitManagerWrapper.swift
)

# Objective-C++ Bridge files
set(OBJC_SOURCES
    Utils/GitBridge.mm
)

# 检查哪些文件实际存在
set(EXISTING_SOURCES)
foreach(source ${SWIFT_SOURCES})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${source})
        list(APPEND EXISTING_SOURCES ${source})
    endif()
endforeach()

# Add Objective-C++ sources if they exist
foreach(source ${OBJC_SOURCES})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${source})
        list(APPEND EXISTING_SOURCES ${source})
    endif()
endforeach()

# 如果没有main.swift，创建一个简单的
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/main.swift)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/main.swift
        "import SwiftUI\n\n"
        "@main\n"
        "struct VersionToolsAppMain: App {\n"
        "    var body: some Scene {\n"
        "        WindowGroup {\n"
        "            ContentView()\n"
        "        }\n"
        "    }\n"
        "}\n"
    )
    list(APPEND EXISTING_SOURCES main.swift)
endif()

add_executable(VersionTools MACOSX_BUNDLE ${EXISTING_SOURCES})

# 设置应用图标
set(APP_ICON "${CMAKE_SOURCE_DIR}/icons/AppIcon.icns")
if(EXISTS ${APP_ICON})
    set_source_files_properties(${APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(VersionTools PRIVATE ${APP_ICON})
    set_target_properties(VersionTools PROPERTIES
        MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
    )
endif()

# 链接框架
target_link_libraries(VersionTools
    GitCore
    "-framework SwiftUI"
    "-framework Combine"
    "-framework Foundation"
    "-framework AppKit"
)

# libgit2必须直接链接到macOS应用程序
if(LIBGIT2_FOUND)
    # 直接使用完整路径的库文件
    target_link_libraries(VersionTools ${LIBGIT2_LIBRARIES})
    message(STATUS "Linking libgit2: ${LIBGIT2_LIBRARIES}")
else()
    # 如果没找到libgit2，尝试直接链接
    find_library(GIT2_LIBRARY git2 PATHS /usr/local/lib /opt/homebrew/lib)
    if(GIT2_LIBRARY)
        target_link_libraries(VersionTools ${GIT2_LIBRARY})
        message(STATUS "Linking libgit2 (found): ${GIT2_LIBRARY}")
    else()
        # 最后的尝试：使用-lgit2并添加搜索路径
        target_link_libraries(VersionTools "-L/opt/homebrew/lib" "-lgit2")
        message(STATUS "Linking libgit2 with -lgit2")
    endif()
endif()

# Swift-C++互操作
set_target_properties(VersionTools PROPERTIES
    Swift_MODULE_NAME "VersionTools"
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Utils/GitBridge-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "gnu++17"
)

# 如果有Info.plist文件
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Info.plist)
    set_target_properties(VersionTools PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Info.plist
    )
endif()

# 如果有Assets.xcassets文件
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Assets.xcassets)
    set_target_properties(VersionTools PROPERTIES
        RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Assets.xcassets"
    )
endif()

# 设置包含目录
target_include_directories(VersionTools PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)