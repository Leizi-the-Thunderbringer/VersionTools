# Version Tools - 项目结构和构建说明

## 项目概述

这是一个现代化的跨平台版本控制GUI工具，采用C++实现核心功能，并根据不同平台使用相应的GUI框架：
- **macOS**: SwiftUI界面，采用Apple Liquid Glass设计语言
- **Linux**: Qt6界面，现代化Material Design风格  
- **Windows**: Qt6界面，符合Windows 11设计规范

## 项目结构

```
VersionTools/
├── CMakeLists.txt                 # 主CMake构建文件
├── README.md                      # 项目说明
├── LICENSE                        # 开源协议
├── .gitignor
e                     # Git忽略文件
├── .github/                       # GitHub Actions CI/CD配置
│   └── workflows/
│       ├── build-macos.yml
│       ├── build-linux.yml
│       └── build-windows.yml
├── docs/                          # 文档目录
│   ├── API.md                     # API文档
│   ├── CONTRIBUTING.md            # 贡献指南
│   └── BUILDING.md                # 构建说明
├── src/                           # 源代码目录
│   ├── core/                      # C++核心功能
│   │   ├── CMakeLists.txt
│   │   ├── GitManager.h           # Git管理器头文件
│   │   ├── GitManager.cpp         # Git管理器实现
│   │   ├── GitTypes.h             # Git数据类型定义
│   │   ├── GitUtils.cpp           # Git工具函数
│   │   ├── GitUtils.h
│   │   ├── SystemCommand.cpp      # 系统命令执行
│   │   ├── SystemCommand.h
│   │   └── platform/              # 平台特定代码
│   │       ├── Windows/
│   │       │   ├── WindowsUtils.cpp
│   │       │   └── WindowsUtils.h
│   │       ├── macOS/
│   │       │   ├── macOSUtils.cpp
│   │       │   └── macOSUtils.h
│   │       └── Linux/
│   │           ├── LinuxUtils.cpp
│   │           └── LinuxUtils.h
│   ├── qt/                        # Qt GUI实现(Linux/Windows)
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp               # Qt程序入口
│   │   ├── VersionToolsMainWindow.h # 主窗口头文件
│   │   ├── VersionToolsMainWindow.cpp # 主窗口实现
│   │   ├── widgets/               # 自定义窗口部件
│   │   │   ├── SidebarWidget.h
│   │   │   ├── SidebarWidget.cpp
│   │   │   ├── ChangesWidget.h
│   │   │   ├── ChangesWidget.cpp
│   │   │   ├── HistoryWidget.h
│   │   │   ├── HistoryWidget.cpp
│   │   │   ├── BranchesWidget.h
│   │   │   ├── BranchesWidget.cpp
│   │   │   ├── DiffViewerWidget.h
│   │   │   ├── DiffViewerWidget.cpp
│   │   │   ├── FileStatusItem.h
│   │   │   └── FileStatusItem.cpp
│   │   ├── dialogs/               # 对话框
│   │   │   ├── SettingsDialog.h
│   │   │   ├── SettingsDialog.cpp
│   │   │   ├── CommitDialog.h
│   │   │   ├── CommitDialog.cpp
│   │   │   ├── BranchDialog.h
│   │   │   └── BranchDialog.cpp
│   │   ├── utils/                 # Qt工具类
│   │   │   ├── GitWorker.h
│   │   │   ├── GitWorker.cpp
│   │   │   ├── ThemeManager.h
│   │   │   └── ThemeManager.cpp
│   │   └── resources/             # Qt资源
│   │       ├── resources.qrc
│   │       ├── icons/
│   │       ├── styles/
│   │       └── translations/
│   └── macos/                     # macOS SwiftUI实现
│       ├── CMakeLists.txt
│       ├── main.swift             # Swift程序入口
│       ├── VersionToolsApp.swift  # SwiftUI应用主类
│       ├── ContentView.swift      # 主内容视图
│       ├── Views/                 # SwiftUI视图
│       │   ├── SidebarView.swift
│       │   ├── ChangesView.swift
│       │   ├── HistoryView.swift
│       │   ├── BranchesView.swift
│       │   ├── DiffView.swift
│       │   └── SettingsView.swift
│       ├── Models/                # 数据模型
│       │   └── GitManagerWrapper.swift
│       ├── Utils/                 # Swift-C++桥接
│       │   ├── GitBridge.cpp
│       │   ├── GitBridge.h
│       │   └── GitBridge-Bridging-Header.h
│       └── Resources/             # 资源文件
│           ├── Info.plist
│           └── Assets.xcassets
├── tests/                         # 测试代码
│   ├── CMakeLists.txt
│   ├── test_git_manager.cpp       # Git管理器测试
│   ├── test_git_utils.cpp         # Git工具测试
│   ├── test_system_command.cpp    # 系统命令测试
│   ├── integration_tests.cpp      # 集成测试
│   └── mocks/                     # 测试模拟对象
│       └── MockGitManager.h
├── resources/                     # 全局资源
│   ├── icons/
│   │   ├── versiontools.png
│   │   ├── versiontools.ico
│   │   └── versiontools.icns
│   ├── versiontools.desktop       # Linux桌面文件
│   └── versiontools.rc            # Windows资源文件
└── scripts/                       # 构建和部署脚本
    ├── build.sh                   # Linux/macOS构建脚本
    ├── build.bat                  # Windows构建脚本
    ├── deploy-macos.sh            # macOS部署脚本
    ├── deploy-linux.sh            # Linux部署脚本
    └── deploy-windows.bat         # Windows部署脚本
```

## 核心功能架构

### C++ 核心层 (`src/core/`)
- **GitManager**: 主要的Git操作类，封装所有Git功能
- **GitTypes**: 定义Git相关的数据结构（提交、分支、文件状态等）
- **GitUtils**: Git工具函数，如字符串处理、路径操作等
- **SystemCommand**: 跨平台的系统命令执行封装
- **Platform Utils**: 各平台特定的系统集成功能

### GUI层架构

#### macOS SwiftUI版本 (`src/macos/`)
采用Apple的Liquid Glass设计语言，特点包括：
- **材质效果**: 使用`.regularMaterial`、`.glass`等效果
- **自适应布局**: `NavigationSplitView`实现三栏布局
- **现代控件**: 使用`.glassProminent`按钮样式
- **系统集成**: 原生工具栏、菜单栏和窗口控件
- **动态适应**: 支持明暗模式自动切换

#### Qt版本 (`src/qt/`)
为Linux和Windows提供现代化界面：
- **自定义样式**: 模拟现代设计语言
- **响应式布局**: 使用QSplitter实现可调整布局
- **主题支持**: 支持明暗主题切换
- **跨平台兼容**: 在Windows和Linux上提供一致体验

## 构建要求

### 通用依赖
- CMake 3.20+
- C++17兼容编译器
- Git 2.20+
- libgit2 (可选，提升性能)

### macOS
- Xcode 15.0+
- macOS 14.0+ SDK
- Swift 5.9+

### Linux
- GCC 9+ 或 Clang 10+
- Qt6.2+ (Core, Widgets, Gui)
- 开发工具: `build-essential`, `cmake`, `ninja-build`

### Windows
- Visual Studio 2022或更新版本
- Qt6.2+ for MSVC
- Windows 10 SDK 或更新

## 构建说明

### macOS构建
```bash
# 安装依赖
brew install cmake libgit2 qt6

# 配置和构建
mkdir build && cd build
cmake -GXcode ..
cmake --build . --config Release

# 或使用Unix Makefiles
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
```

### Linux构建
```bash
# Ubuntu/Debian安装依赖
sudo apt update
sudo apt install build-essential cmake ninja-build git
sudo apt install libgit2-dev
sudo apt install qt6-base-dev qt6-tools-dev

# RHEL/CentOS安装依赖
sudo dnf install gcc-c++ cmake ninja-build git
sudo dnf install libgit2-devel
sudo dnf install qt6-qtbase-devel qt6-qttools-devel

# 构建
mkdir build && cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
ninja

# 安装
sudo ninja install
```

### Windows构建
```batch
@REM 确保安装了Qt6和Visual Studio 2022

@REM 设置环境
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set Qt6_DIR=C:\Qt\6.5.0\msvc2022_64\lib\cmake\Qt6

@REM 构建
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 -DQT_VERSION=6 ..
cmake --build . --config Release

@REM 打包
cpack
```

## 设计特色

### macOS版本 - Liquid Glass设计
基于Apple最新的Liquid Glass设计语言：

1. **材质系统**
   - 使用半透明玻璃效果
   - 动态模糊和光影效果
   - 自适应颜色和透明度

2. **交互设计**
   - 流体动画过渡
   - 响应式触觉反馈
   - 上下文感知界面

3. **布局原则**
   - 内容优先的层次结构
   - 圆角和有机形状
   - 集中注意力的视觉引导

### Qt版本 - 现代化界面
提供媲美原生应用的体验：

1. **视觉设计**
   - 扁平化设计语言
   - 一致的配色方案
   - 清晰的图标系统

2. **交互体验**
   - 平滑的动画过渡
   - 直观的操作反馈
   - 键盘快捷键支持

3. **自适应布局**
   - 响应式窗口大小调整
   - 可停靠的面板系统
   - 多显示器支持

## 开发和贡献

### 代码规范
- C++: 遵循Google C++ Style Guide
- Swift: 遵循Swift API Design Guidelines
- Qt: 遵循Qt Coding Conventions

### 测试
```bash
# 运行所有测试
ctest --output-on-failure

# 运行特定测试
./build/bin/GitCoreTests
./build/bin/IntegrationTests
```

### 调试
- macOS: 使用Xcode调试器
- Linux: 使用GDB或LLDB
- Windows: 使用Visual Studio调试器

## 发布打包

### macOS
```bash
# 创建应用包
cmake --build . --target GitGUI
# 创建DMG安装包
cpack -G DragNDrop
```

### Linux
```bash
# 创建DEB包
cpack -G DEB
# 创建RPM包
cpack -G RPM
# 创建AppImage
# (需要额外的linuxdeploy工具)
```

### Windows
```bash
# 创建安装程序
cpack -G NSIS
# 创建便携版
cpack -G ZIP
```

## 路线图

### v1.0 (当前版本)
- [x] 基本Git操作
- [x] 文件状态管理
- [x] 提交历史查看
- [x] 分支管理
- [x] 差异查看

### v1.1 (计划中)
- [ ] 远程仓库管理
- [ ] 标签操作
- [ ] 子模块支持
- [ ] 合并冲突解决

### v1.2 (未来版本)
- [ ] GitLFS支持
- [ ] 性能优化
- [ ] 插件系统
- [ ] 高级搜索功能

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 支持和反馈

- 提交Issue: [GitHub Issues](https://github.com/Zixiao-Tech/VersionTools/issues)
- 功能建议: [GitHub Discussions](https://github.com/Zixiao-Tech/VersionTools/discussions)
- 邮件联系: versiontools@example.com