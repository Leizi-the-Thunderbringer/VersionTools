# VersionTools 开发指南

## 项目结构

```
VersionTools/
├── src/
│   ├── core/           # C++ 核心库
│   ├── macos/          # macOS SwiftUI 界面
│   └── qt/             # Qt 跨平台界面
├── docs/               # 文档
├── resources/          # 资源文件
└── tests/              # 测试 (可选)
```

## 开发环境设置

### macOS 开发
```bash
# 安装 Xcode Command Line Tools
xcode-select --install

# 安装 CMake
brew install cmake

# 可选: 安装 libgit2
brew install libgit2
```

### Linux 开发
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-tools-dev libgit2-dev

# Fedora/CentOS
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qttools-devel libgit2-devel
```

### Windows 开发
1. 安装 Visual Studio 2019/2022 (with C++ support)
2. 安装 CMake
3. 安装 Qt6
4. 可选: 安装 vcpkg 并通过它安装 libgit2

## 构建说明

### 基本构建
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 构建选项
```bash
# Debug 构建
cmake -DCMAKE_BUILD_TYPE=Debug ..

# 启用测试
cmake -DBUILD_TESTS=ON ..

# 指定 Qt 路径 (如果需要)
cmake -DQt6_DIR=/path/to/qt6/lib/cmake/Qt6 ..
```

### 平台特定构建

#### macOS
```bash
mkdir build && cd build
cmake -G "Xcode" ..
xcodebuild -configuration Release
```

#### Windows (Visual Studio)
```bash
mkdir build && cd build
cmake -G "Visual Studio 17 2022" ..
cmake --build . --config Release
```

## 代码规范

### C++ 代码风格
- 使用现代 C++17 特性
- 遵循 Google C++ Style Guide
- 使用智能指针管理内存
- 优先使用 STL 容器和算法

### Swift 代码风格
- 遵循 Swift API Design Guidelines
- 使用 SwiftUI 最佳实践
- 保持视图组件的单一职责

### Qt 代码风格
- 遵循 Qt Coding Style
- 使用信号和槽机制
- 正确处理内存管理

## 架构设计

### 核心原则
1. **分离关注点**: GUI 与业务逻辑分离
2. **跨平台兼容**: 核心功能平台无关
3. **可扩展性**: 易于添加新功能
4. **性能优化**: 异步操作，避免阻塞 UI

### 模块依赖
```
GUI层 (SwiftUI/Qt) → 核心库 (C++) → Git操作
```

## 添加新功能

### 1. 核心功能
1. 在 `src/core/` 中实现 C++ 逻辑
2. 更新 `GitManager` 和相关类
3. 添加必要的数据类型到 `GitTypes.h`

### 2. macOS 界面
1. 在 `src/macos/Views/` 中创建 SwiftUI 视图
2. 更新 `GitManagerWrapper` 以桥接 C++ 功能
3. 在 `ContentView.swift` 中集成新视图

### 3. Qt 界面
1. 在 `src/qt/widgets/` 中创建 Qt 控件
2. 更新 `VersionToolsMainWindow` 集成新控件
3. 在 `CMakeLists.txt` 中添加新源文件

## 测试

### 单元测试
```bash
# 启用测试构建
cmake -DBUILD_TESTS=ON ..
make
ctest
```

### 集成测试
```bash
# 测试实际 Git 仓库操作
./bin/IntegrationTests
```

## 调试

### macOS 调试
```bash
# Xcode 调试
open VersionTools.xcodeproj

# 命令行调试
lldb ./bin/VersionTools
```

### Linux 调试
```bash
# GDB 调试
gdb ./bin/VersionTools

# Valgrind 内存检查
valgrind --tool=memcheck ./bin/VersionTools
```

### Windows 调试
- 使用 Visual Studio 调试器
- 或者使用 Qt Creator

## 打包发布

### macOS
```bash
# 创建 DMG
cpack -G DragNDrop

# 签名 (需要开发者证书)
codesign --deep --force --verify --verbose --sign "Developer ID" VersionTools.app
```

### Linux
```bash
# 创建 DEB 包
cpack -G DEB

# 创建 RPM 包
cpack -G RPM
```

### Windows
```bash
# 创建 NSIS 安装包
cpack -G NSIS
```

## 性能优化

### 一般优化
- 使用异步操作避免阻塞 UI
- 实现懒加载减少启动时间
- 缓存频繁访问的数据

### 平台特定优化
- macOS: 利用 GCD 和 Combine 框架
- Qt: 使用 QThread 和 QtConcurrent

## 故障排除

### 常见问题

#### 构建失败
1. 检查依赖是否正确安装
2. 确认 CMake 版本 >= 3.20
3. 检查编译器支持 C++17

#### Qt 相关问题
1. 确认 Qt6 路径正确
2. 检查 MOC 文件生成
3. 验证信号槽连接

#### macOS 特定问题
1. 检查 Xcode 版本
2. 确认 Swift 版本兼容性
3. 验证 Framework 链接

### 日志和调试信息
- 启用详细日志: `cmake -DCMAKE_VERBOSE_MAKEFILE=ON`
- Qt 调试: 设置 `QT_LOGGING_RULES` 环境变量
- macOS 调试: 使用 Console.app 查看系统日志

## 贡献指南

1. Fork 项目
2. 创建功能分支: `git checkout -b feature/new-feature`
3. 提交更改: `git commit -m 'Add new feature'`
4. 推送分支: `git push origin feature/new-feature`
5. 创建 Pull Request

### 提交消息格式
```
type(scope): description

[optional body]

[optional footer]
```

类型: feat, fix, docs, style, refactor, test, chore
范围: core, macos, qt, build, etc.

## 许可证

MIT License - 详见 LICENSE 文件