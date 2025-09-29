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

# 如果找到了libgit2，确保它被链接
if(LIBGIT2_FOUND)
    target_link_libraries(VersionTools ${LIBGIT2_LIBRARIES})
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