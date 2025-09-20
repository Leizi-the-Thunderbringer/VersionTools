cmake_minimum_required(VERSION 3.20)

# 核心Git管理器库
add_library(GitCore STATIC
    GitManager.cpp
    GitManager.h
    GitTypes.h
    GitUtils.cpp
    GitUtils.h
    SystemCommand.cpp
    SystemCommand.h
)

# 链接库
target_link_libraries(GitCore 
    Threads::Threads
)

if(LIBGIT2_FOUND)
    target_link_libraries(GitCore ${LIBGIT2_LIBRARIES})
    target_compile_definitions(GitCore PRIVATE USE_LIBGIT2)
endif()

# 平台特定代码（如果需要的话）
if(WIN32)
    # Windows特定源文件可以在这里添加
    target_link_libraries(GitCore ws2_32)
elseif(APPLE)
    # macOS特定链接
    target_link_libraries(GitCore "-framework Foundation")
elseif(UNIX)
    # Linux特定链接
    target_link_libraries(GitCore dl)
endif()

# 导出头文件
target_include_directories(GitCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)