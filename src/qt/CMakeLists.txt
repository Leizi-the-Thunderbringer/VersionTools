cmake_minimum_required(VERSION 3.20)

if(NOT GUI_BACKEND STREQUAL "Qt")
    return()
endif()

# 创建资源文件
set(RESOURCES_QRC ${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc)
if(NOT EXISTS ${RESOURCES_QRC})
    file(WRITE ${RESOURCES_QRC}
        "<!DOCTYPE RCC>\n"
        "<RCC version=\"1.0\">\n"
        "<qresource>\n"
        "</qresource>\n"
        "</RCC>\n"
    )
endif()

# Qt应用程序
set(QT_SOURCES
    main.cpp
    VersionToolsMainWindow.cpp
    VersionToolsMainWindow.h
    widgets/SidebarWidget.cpp
    widgets/SidebarWidget.h
    utils/GitWorker.cpp
    utils/GitWorker.h
    utils/ThemeManager.cpp
    utils/ThemeManager.h
)

# 添加存在的组件源文件
foreach(component ChangesWidget HistoryWidget BranchesWidget DiffViewerWidget FileStatusItem)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/widgets/${component}.cpp)
        list(APPEND QT_SOURCES widgets/${component}.cpp)
        list(APPEND QT_SOURCES widgets/${component}.h)
    endif()
endforeach()

foreach(dialog SettingsDialog CommitDialog BranchDialog)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/${dialog}.cpp)
        list(APPEND QT_SOURCES dialogs/${dialog}.cpp)
        list(APPEND QT_SOURCES dialogs/${dialog}.h)
    endif()
endforeach()

# 添加资源文件
if(EXISTS ${RESOURCES_QRC})
    list(APPEND QT_SOURCES ${RESOURCES_QRC})
endif()

add_executable(VersionTools ${QT_SOURCES})

# Qt链接
target_link_libraries(VersionTools 
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    GitCore
)

# Windows特定设置
if(WIN32)
    set_target_properties(VersionTools PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    # 如果有应用程序图标资源文件
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/resources/versiontools.rc)
        target_sources(VersionTools PRIVATE resources/versiontools.rc)
    endif()
endif()

# 设置包含目录
target_include_directories(VersionTools PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)