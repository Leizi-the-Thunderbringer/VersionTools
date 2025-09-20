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
    Models/GitManagerWrapper.swift
    Utils/GitBridge.cpp
    Utils/GitBridge.h
    Utils/GitBridge-Bridging-Header.h
)

# 检查哪些文件实际存在
set(EXISTING_SOURCES)
foreach(source ${SWIFT_SOURCES})
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

# 链接框架
target_link_libraries(VersionTools
    GitCore
    "-framework SwiftUI"
    "-framework Combine"
    "-framework Foundation"
    "-framework AppKit"
)

# Swift-C++互操作
set_target_properties(VersionTools PROPERTIES
    Swift_MODULE_NAME "VersionTools"
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