import Foundation
import SwiftUI

// Swift wrapper for C++ Git functionality
class GitManagerWrapper: ObservableObject {
    @Published var isLoading = false
    @Published var repositoryPath: String = ""
    @Published var repositoryName: String = "No Repository"
    @Published var currentBranch: String?
    @Published var repositoryStatus: GitRepositoryStatus?
    @Published var commitHistory: [GitCommitWrapper] = []
    @Published var stagedChanges: [GitFileChangeWrapper] = []
    @Published var unstagedChanges: [GitFileChangeWrapper] = []
    @Published var localBranches: [GitBranchWrapper] = []
    @Published var remoteBranches: [GitBranchWrapper] = []
    @Published var allBranches: [GitBranchWrapper] = []
    @Published var currentBranchInfo: GitBranchWrapper?
    
    private var gitBridge: GitBridge
    
    init() {
        self.gitBridge = GitBridge()
    }
    
    // MARK: - Repository Operations
    
    func openRepository(path: String) {
        DispatchQueue.global(qos: .userInitiated).async {
            let success = self.gitBridge.openRepository(path)
            
            DispatchQueue.main.async {
                if success {
                    self.repositoryPath = path
                    self.repositoryName = URL(fileURLWithPath: path).lastPathComponent
                    self.refreshAll()
                } else {
                    self.showError("Failed to open repository at \(path)")
                }
            }
        }
    }
    
    func refreshAll() {
        refreshStatus()
        refreshBranches()
        loadCommitHistory()
    }
    
    @MainActor
    func refreshStatus() async {
        isLoading = true
        
        await withTaskGroup(of: Void.self) { group in
            group.addTask {
                await self.loadStatus()
            }
            group.addTask {
                await self.loadChanges()
            }
        }
        
        isLoading = false
    }
    
    @MainActor
    func refreshCommitHistory() async {
        isLoading = true
        await loadCommitHistoryAsync()
        isLoading = false
    }
    
    @MainActor
    func refreshBranches() async {
        isLoading = true
        await loadBranchesAsync()
        isLoading = false
    }
    
    // MARK: - Synchronous Wrappers
    
    func refreshStatus() {
        Task {
            await refreshStatus()
        }
    }
    
    func refreshBranches() {
        Task {
            await refreshBranches()
        }
    }
    
    func loadCommitHistory() {
        Task {
            await loadCommitHistoryAsync()
        }
    }
    
    func loadBranches() {
        Task {
            await loadBranchesAsync()
        }
    }
    
    // MARK: - Private Loading Methods
    
    @MainActor
    private func loadStatus() async {
        let status = await Task.detached {
            self.gitBridge.getRepositoryStatus()
        }.value
        
        repositoryStatus = status
        currentBranch = status?.currentBranch
    }
    
    @MainActor
    private func loadChanges() async {
        let changes = await Task.detached {
            self.gitBridge.getFileChanges()
        }.value
        
        stagedChanges = changes.filter { $0.isStaged }
        unstagedChanges = changes.filter { !$0.isStaged }
    }
    
    @MainActor
    private func loadCommitHistoryAsync() async {
        let commits = await Task.detached {
            self.gitBridge.getCommitHistory(maxCount: 100)
        }.value
        
        commitHistory = commits
    }
    
    @MainActor
    private func loadBranchesAsync() async {
        let branches = await Task.detached {
            self.gitBridge.getBranches()
        }.value
        
        localBranches = branches.filter { !$0.isRemote }
        remoteBranches = branches.filter { $0.isRemote }
        allBranches = branches
        currentBranchInfo = branches.first { $0.isCurrent }
    }
    
    // MARK: - File Operations
    
    func stageFile(_ filePath: String) {
        DispatchQueue.global(qos: .userInitiated).async {
            let success = self.gitBridge.stageFile(filePath)
            if success {
                DispatchQueue.main.async {
                    self.refreshStatus()
                }
            }
        }
    }
    
    func unstageFile(_ filePath: String) {
        DispatchQueue.global(qos: .userInitiated).async {
            let success = self.gitBridge.unstageFile(filePath)
            if success {
                DispatchQueue.main.async {
                    self.refreshStatus()
                }
            }
        }
    }
    
    func stageAllChanges() {
        DispatchQueue.global(qos: .userInitiated).async {
            let success = self.gitBridge.stageAllFiles()
            if success {
                DispatchQueue.main.async {
                    self.refreshStatus()
                }
            }
        }
    }
    
    func unstageAllChanges() {
        DispatchQueue.global(qos: .userInitiated).async {
            let success = self.gitBridge.unstageAllFiles()
            if success {
                DispatchQueue.main.async {
                    self.refreshStatus()
                }
            }
        }
    }
    
    // MARK: - Commit Operations
    
    func commit(message: String, completion: @escaping (Bool) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            let success = self.gitBridge.commit(message)
            DispatchQueue.main.async {
                if success {
                    self.refreshAll()
                }
                completion(success)
            }
        }
    }
    
    // MARK: - Branch Operations
    
    func checkoutBranch(_ branchName: String, completion: @escaping (Bool) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            let success = self.gitBridge.checkoutBranch(branchName)
            DispatchQueue.main.async {
                if success {
                    self.refreshAll()
                }
                completion(success)
            }
        }
    }
    
    func createBranch(_ branchName: String, completion: @escaping (Bool) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            let success = self.gitBridge.createBranch(branchName)
            DispatchQueue.main.async {
                if success {
                    self.refreshBranches()
                }
                completion(success)
            }
        }
    }
    
    func deleteBranch(_ branchName: String, completion: @escaping (Bool) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            let success = self.gitBridge.deleteBranch(branchName)
            DispatchQueue.main.async {
                if success {
                    self.refreshBranches()
                }
                completion(success)
            }
        }
    }
    
    // MARK: - Diff and Detail Operations
    
    func loadCommitChanges(commitHash: String, completion: @escaping ([GitFileChangeWrapper]) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            let changes = self.gitBridge.getCommitChanges(commitHash)
            DispatchQueue.main.async {
                completion(changes)
            }
        }
    }
    
    func loadFileDiff(filePath: String, commitHash: String, completion: @escaping (GitDiffWrapper?) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            let diff = self.gitBridge.getFileDiff(filePath, commitHash: commitHash)
            DispatchQueue.main.async {
                completion(diff)
            }
        }
    }
    
    func loadBranchCommits(branchName: String, completion: @escaping ([GitCommitWrapper]) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            let commits = self.gitBridge.getBranchCommits(branchName, maxCount: 50)
            DispatchQueue.main.async {
                completion(commits)
            }
        }
    }
    
    // MARK: - Error Handling
    
    private func showError(_ message: String) {
        // TODO: Implement proper error handling
        print("Git Error: \(message)")
    }
}

// MARK: - Data Models

struct GitRepositoryStatus {
    let currentBranch: String
    let upstreamBranch: String?
    let aheadCount: Int
    let behindCount: Int
    let stagedChangesCount: Int
    let unstagedChangesCount: Int
    let stashCount: Int
    let hasUncommittedChanges: Bool
}

struct GitFileChangeWrapper: Hashable, Identifiable {
    let id = UUID()
    let filePath: String
    let fileName: String
    let directoryPath: String
    let status: GitFileStatus
    let isStaged: Bool
    let linesAdded: Int
    let linesDeleted: Int
    
    static func == (lhs: GitFileChangeWrapper, rhs: GitFileChangeWrapper) -> Bool {
        lhs.filePath == rhs.filePath && lhs.isStaged == rhs.isStaged
    }
    
    func hash(into hasher: inout Hasher) {
        hasher.combine(filePath)
        hasher.combine(isStaged)
    }
    
    static let preview = GitFileChangeWrapper(
        filePath: "src/main.cpp",
        fileName: "main.cpp",
        directoryPath: "src",
        status: .modified,
        isStaged: false,
        linesAdded: 10,
        linesDeleted: 5
    )
}

enum GitFileStatus {
    case added
    case modified
    case deleted
    case renamed
    case copied
    case untracked
    case conflicted
    case ignored
}

struct GitCommitWrapper: Identifiable {
    let id = UUID()
    let hash: String
    let shortHash: String
    let author: String
    let email: String
    let message: String
    let shortMessage: String
    let fullMessage: String
    let timestamp: Date
    let parentHashes: [String]
    let isMerge: Bool
    
    var formattedDate: String {
        let formatter = DateFormatter()
        formatter.dateStyle = .medium
        formatter.timeStyle = .short
        return formatter.string(from: timestamp)
    }
    
    var relativeTime: String {
        let formatter = RelativeDateTimeFormatter()
        formatter.unitsStyle = .short
        return formatter.localizedString(for: timestamp, relativeTo: Date())
    }
}

struct GitBranchWrapper: Identifiable, Hashable {
    let id = UUID()
    let name: String
    let fullName: String
    let displayName: String
    let isRemote: Bool
    let isCurrent: Bool
    let upstreamBranch: String?
    let aheadCount: Int
    let behindCount: Int
    let lastCommit: GitCommitWrapper?
    
    static func == (lhs: GitBranchWrapper, rhs: GitBranchWrapper) -> Bool {
        lhs.fullName == rhs.fullName
    }
    
    func hash(into hasher: inout Hasher) {
        hasher.combine(fullName)
    }
}

struct GitDiffWrapper {
    let filePath: String
    let isBinary: Bool
    let isNewFile: Bool
    let isDeletedFile: Bool
    let hunks: [GitDiffHunkWrapper]
    let rawContent: String
}

struct GitDiffHunkWrapper {
    let header: String
    let oldStart: Int
    let oldCount: Int
    let newStart: Int
    let newCount: Int
    let lines: [GitDiffLineWrapper]
}

struct GitDiffLineWrapper {
    let type: GitDiffLineType
    let content: String
    let oldLineNumber: Int
    let newLineNumber: Int
}

enum GitDiffLineType {
    case context
    case addition
    case deletion
    case header
}