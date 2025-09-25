# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Current Status (2024-09-25)

### ✅ Build Status: WORKING
The project now successfully compiles and builds on macOS with the following fixes applied:
- GitBridge Objective-C++ bridging properly configured
- SwiftUI compatibility issues resolved
- Type conversion between Objective-C and Swift fixed
- All linking errors resolved with stub implementations

### ⚠️ Important Notes
Many Git operations in `src/core/GitManager.cpp` are currently **stub implementations** that return empty results or error status. See `TODO.md` for the complete list of unimplemented features.

## Project Overview

VersionTools is a modern cross-platform Git GUI tool with:
- C++ core library for Git operations (`src/core/`)
- macOS SwiftUI interface using Apple's Liquid Glass design (`src/macos/`)
- Qt6 interface for Linux/Windows (`src/qt/`)

## Build Commands

### macOS (SwiftUI)
```bash
# Configure and build
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)

# Build with Xcode
cmake -GXcode ..
cmake --build . --config Release

# Run the application
./bin/VersionTools.app/Contents/MacOS/VersionTools
```

### Linux/Windows (Qt)
```bash
# Configure and build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)  # Linux
cmake --build . --config Release  # Windows

# Run the application
./bin/VersionTools
```

### Clean build
```bash
rm -rf build
mkdir build && cd build
cmake ..
make
```

## Development Workflow

### Testing
```bash
# Enable tests during configuration
cmake -DBUILD_TESTS=ON ..
make
ctest --output-on-failure
```

Note: Test directory is not yet created. When implemented, tests will be in `tests/`.

### Packaging
```bash
# macOS: Create .app bundle and DMG
cpack -G DragNDrop

# Linux: Create DEB/RPM packages
cpack -G DEB
cpack -G RPM

# Windows: Create NSIS installer
cpack -G NSIS
```

## Architecture

### Core Library (`src/core/`)
The C++ core provides all Git functionality through the `GitManager` class:
- **GitManager**: Main interface for Git operations (src/core/GitManager.h:35)
- **GitTypes**: Data structures for commits, branches, etc. (src/core/GitTypes.h)
- **SystemCommand**: Cross-platform command execution (src/core/SystemCommand.h)
- **GitUtils**: Helper functions (src/core/GitUtils.h)

The core uses the PImpl idiom for ABI stability and can work with either libgit2 or system git commands.

### Platform Integration

#### macOS (`src/macos/`)
- **Entry point**: VersionToolsApp.swift - SwiftUI app structure
- **Main view**: ContentView.swift - Navigation split view layout
- **C++ Bridge**: GitBridge (src/macos/Utils/) - Swift/C++ interop layer
- **Design**: Implements Apple's Liquid Glass design with materials and glass effects

#### Qt (`src/qt/`)
- **Main window**: VersionToolsMainWindow - Central Qt application window
- **Widgets**: Custom widgets in src/qt/widgets/ for different views
- **Worker threads**: GitWorker for async operations
- **Theme**: ThemeManager for dark/light mode support

### Build System
The project uses CMake with platform detection:
- Automatically selects SwiftUI for macOS, Qt for Linux/Windows
- Supports both libgit2 and system git commands
- Configurable through CMake options (BUILD_TESTS, Qt6_DIR, etc.)

## Key Design Decisions

1. **Dual GUI Strategy**: Native SwiftUI for macOS to leverage Liquid Glass design, Qt6 for cross-platform consistency on Linux/Windows

2. **C++ Core**: Shared core library ensures consistent Git operations across platforms while allowing native UI implementations

3. **Flexible Git Backend**: Supports both libgit2 (for performance) and system git commands (for compatibility)

4. **Async Operations**: All network operations (clone, fetch, pull, push) support async execution with progress callbacks

5. **Modern C++17**: Uses smart pointers, optional, string_view, and other modern features for safety and performance

## Common Tasks

### Adding a new Git operation
1. Add method declaration to GitManager.h
2. Implement in GitManager.cpp (replace stub if exists)
3. Update GitBridge for macOS if needed
4. Add UI elements in respective platform code

### Implementing stub functions
Look for "TODO: Implement" comments in `src/core/GitManager.cpp`. Current stubs:
- Branch operations (getBranches, createBranch, deleteBranch, checkoutBranch)
- Stash operations (getStashes)
- Diff operations (getCommitDiff, getCommitDiffAll)

### Modifying the UI
- **macOS**: Edit SwiftUI views in src/macos/Views/
- **Qt**: Modify widgets in src/qt/widgets/

### Debugging
- macOS: Open build/VersionTools.xcodeproj in Xcode
- Linux: Use gdb or lldb with debug build
- Windows: Open in Visual Studio with CMake support