# TODO.md - VersionTools 待实现功能列表

## 编译和构建修复记录
- ✅ 修复GitBridge桥接配置问题
- ✅ 修复SwiftUI兼容性问题（insetGrouped样式）
- ✅ 更新弃用的onChange API
- ✅ 修复Swift和Objective-C类型转换问题
- ✅ 添加缺失方法的存根实现以解决链接错误

## 核心功能待实现（src/core/GitManager.cpp）

### 分支操作
- [x] `getBranches()` - 获取所有分支列表 ✅ 2025-09-26
  - 实现本地和远程分支的获取
  - 解析分支的上游信息、ahead/behind计数
  - 获取每个分支的最后提交信息

- [x] `createBranch()` - 创建新分支 ✅ 2025-09-26
  - 实现基于指定起始点创建分支
  - 支持从当前HEAD或指定commit创建

- [x] `deleteBranch()` - 删除分支 ✅ 2025-09-26
  - 实现安全删除和强制删除选项

- [x] `checkoutBranch()` - 切换分支 ✅ 2025-09-26
  - 实现分支切换

### Stash操作
- [x] `getStashes()` - 获取储藏列表 ✅ 2025-09-26
  - 实现stash列表获取
  - 解析stash消息和时间戳

- [ ] `stash()` - 创建储藏
- [ ] `stashPop()` - 弹出储藏
- [ ] `stashApply()` - 应用储藏
- [ ] `stashDrop()` - 删除储藏
- [ ] `stashClear()` - 清空所有储藏

### Diff操作
- [x] `getCommitDiff()` - 获取单个提交的差异 ✅ 2025-09-26
  - 解析commit的文件变更
  - 生成diff hunks和lines

- [x] `getCommitDiffAll()` - 获取提交的所有文件差异 ✅ 2025-09-26
  - 批量处理commit中的所有文件变更

- [ ] `getDiff()` - 获取单个文件的差异
- [ ] `getDiffAll()` - 获取所有文件的差异
- [ ] `getDiffBetweenCommits()` - 获取两个提交之间的差异

### 远程操作
- [ ] `getRemotes()` - 获取远程仓库列表
- [ ] `addRemote()` - 添加远程仓库
- [ ] `removeRemote()` - 删除远程仓库
- [ ] `fetch()` - 拉取远程更新
- [ ] `pull()` - 拉取并合并
- [ ] `push()` - 推送到远程

### 标签操作
- [ ] `getTags()` - 获取标签列表
- [ ] `createTag()` - 创建标签
- [ ] `deleteTag()` - 删除标签
- [ ] `pushTags()` - 推送标签

### 合并和变基
- [ ] `mergeBranch()` - 合并分支
- [ ] `rebaseBranch()` - 变基操作
- [ ] `abortMerge()` - 中止合并
- [ ] `continueMerge()` - 继续合并

## UI功能待完善

### macOS SwiftUI界面
- [ ] 实现分支视图的实际操作
  - 创建分支对话框
  - 删除分支确认
  - 分支切换功能

- [ ] 实现Stash视图
  - Stash列表显示
  - 创建/应用/删除stash

- [ ] 实现远程操作界面
  - 远程仓库管理
  - Push/Pull/Fetch操作界面

- [ ] 实现合并冲突解决界面
  - 冲突文件列表
  - 冲突解决编辑器

- [ ] 实现设置界面
  - 用户信息配置
  - 仓库设置
  - 应用偏好设置

### Qt界面（Linux/Windows）
- [ ] 实现Qt版本的所有界面功能
  - 参照macOS版本实现对应功能
  - 确保跨平台一致性

## 性能优化
- [ ] 实现异步操作的进度反馈
- [ ] 优化大型仓库的性能
- [ ] 实现操作缓存机制
- [ ] 添加取消操作的支持

## 测试
- [ ] 添加单元测试
  - GitManager核心功能测试
  - 工具函数测试

- [ ] 添加集成测试
  - 完整Git操作流程测试
  - UI交互测试

## 文档
- [ ] 完善API文档
- [ ] 添加用户使用文档
- [ ] 创建开发者指南

## 已知问题
1. GitBridge返回的NSDate在某些情况下可能需要格式转换
2. 某些Git操作的错误处理还不完善
3. 异步操作的取消机制未实现

## 注意事项
- 所有Git操作应该在后台线程执行
- UI更新必须在主线程
- 需要正确处理Git命令的错误输出
- 考虑添加libgit2支持以提升性能