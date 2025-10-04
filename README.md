# Version Tools - Modern Git GUI

[![Build & Test](https://github.com/Zixiao-Tech/VersionTools/actions/workflows/build.yml/badge.svg)](https://github.com/Zixiao-Tech/VersionTools/actions/workflows/build.yml)
[![Release](https://github.com/Zixiao-Tech/VersionTools/actions/workflows/release.yml/badge.svg)](https://github.com/Zixiao-Tech/VersionTools/actions/workflows/release.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**由于维护难度过大，本项目将停止维护**
一个现代化的Git图形界面工具，使用C++核心和平台特定的GUI实现。

## 特性

- **跨平台支持**: macOS (SwiftUI), Linux/Windows (Qt)
- **现代化设计**: 遵循各平台的设计语言
- **高性能**: C++核心确保快速的Git操作
- **直观的界面**: 易于使用的图形界面

### macOS特性
- 采用Liquid Glass设计语言
- 原生SwiftUI界面
- 支持macOS外观和主题

### Linux/Windows特性
- 现代化Qt界面
- 自适应主题支持
- 跨平台一致性

## 功能

- [x] 文件更改管理 (暂存/取消暂存)
- [x] 提交历史查看
- [x] 分支管理
- [x] 差异查看器
- [ ] 远程仓库管理
- [ ] 标签管理
- [ ] 储藏管理

## 构建要求

### 通用要求
- CMake 3.20+
- C++17兼容编译器
- Git (系统安装)

### macOS
- Xcode 15.0+
- Swift 5.0+
- macOS 13.0+ SDK

### Linux/Windows
- Qt6 (Core, Widgets, Gui)
- 可选: libgit2 (否则使用系统git命令)

## 构建说明

### macOS (SwiftUI)
```bash
mkdir build && cd build
cmake ..
make
```

### Linux/Windows (Qt)
```bash
# 确保安装Qt6
mkdir build && cd build
cmake ..
make
```

### 配置选项
```bash
# 启用测试
cmake -DBUILD_TESTS=ON ..

# 指定Qt安装路径 (如需要)
cmake -DQt6_DIR=/path/to/qt6 ..
```

## 下载

从 [Releases](https://github.com/Zixiao-Tech/VersionTools/releases) 页面下载最新版本：
- **macOS**: VersionTools-x.x.x-macOS.dmg
- **Linux**: VersionTools-x.x.x-Linux.deb 或 .rpm
- **Windows**: VersionTools-x.x.x-Windows.zip

## 安装

### Linux
```bash
sudo make install
```

### Windows
```bash
make install
# 或者使用NSIS安装包
cpack -G NSIS
```

### macOS
```bash
make install
# 或者创建DMG
cpack -G DragNDrop
```

## 架构

```
src/
├── core/           # C++ Git核心库
│   ├── GitManager  # Git操作管理
│   ├── GitTypes    # 数据类型定义
│   └── GitUtils    # 工具函数
├── macos/          # SwiftUI macOS界面
│   ├── Views/      # SwiftUI视图
│   ├── Models/     # 数据模型
│   └── Utils/      # C++桥接
└── qt/             # Qt跨平台界面
    ├── widgets/    # Qt控件
    ├── dialogs/    # 对话框
    └── utils/      # 工具类
```

## 开发

### 添加新功能
1. 在`core/`中实现C++逻辑
2. 在相应的GUI模块中添加界面
3. 更新CMakeLists.txt (如需要)

### 代码风格
- C++: 遵循现代C++17标准
- Swift: 遵循Swift API设计指南
- Qt: 遵循Qt代码约定

## 许可证

MIT License - 详见LICENSE文件

## 贡献

欢迎提交Issue和Pull Request！
