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
        let statusDict = gitBridge.getRepositoryStatus()

        if let dict = statusDict as? [String: Any] {
            repositoryStatus = GitRepositoryStatus(
                currentBranch: dict["currentBranch"] as? String ?? "",
                upstreamBranch: dict["upstreamBranch"] as? String,
                aheadCount: dict["aheadCount"] as? Int ?? 0,
                behindCount: dict["behindCount"] as? Int ?? 0,
                stagedChangesCount: dict["stagedChangesCount"] as? Int ?? 0,
                unstagedChangesCount: dict["unstagedChangesCount"] as? Int ?? 0,
                stashCount: dict["stashCount"] as? Int ?? 0,
                hasUncommittedChanges: dict["hasUncommittedChanges"] as? Bool ?? false
            )
            currentBranch = repositoryStatus?.currentBranch
        }
    }
    
    @MainActor
    private func loadChanges() async {
        let changesArray = gitBridge.getFileChanges()

        var staged: [GitFileChangeWrapper] = []
        var unstaged: [GitFileChangeWrapper] = []

        if let changes = changesArray as? [[String: Any]] {
            for change in changes {
                let wrapper = GitFileChangeWrapper(
                    filePath: change["filePath"] as? String ?? "",
                    fileName: change["fileName"] as? String ?? "",
                    directoryPath: change["directoryPath"] as? String ?? "",
                    status: GitFileStatus(rawValue: change["status"] as? Int ?? 0) ?? .modified,
                    isStaged: change["isStaged"] as? Bool ?? false,
                    linesAdded: change["linesAdded"] as? Int ?? 0,
                    linesDeleted: change["linesDeleted"] as? Int ?? 0
                )

                if wrapper.isStaged {
                    staged.append(wrapper)
                } else {
                    unstaged.append(wrapper)
                }
            }
        }

        stagedChanges = staged
        unstagedChanges = unstaged
    }
    
    @MainActor
    private func loadCommitHistoryAsync() async {
        let commitsArray = gitBridge.getCommitHistory(100)

        var commits: [GitCommitWrapper] = []
        if let commitsData = commitsArray as? [[String: Any]] {
            for commitData in commitsData {
                let commit = GitCommitWrapper(
                    hash: commitData["hash"] as? String ?? "",
                    shortHash: commitData["shortHash"] as? String ?? "",
                    author: commitData["author"] as? String ?? "",
                    email: commitData["email"] as? String ?? "",
                    message: commitData["message"] as? String ?? "",
                    shortMessage: commitData["shortMessage"] as? String ?? "",
                    fullMessage: commitData["message"] as? String ?? "",
                    timestamp: commitData["timestamp"] as? Date ?? Date(),
                    parentHashes: commitData["parentHashes"] as? [String] ?? [],
                    isMerge: commitData["isMerge"] as? Bool ?? false
                )
                commits.append(commit)
            }
        }

        commitHistory = commits
    }
    
    @MainActor
    private func loadBranchesAsync() async {
        let branchesArray = gitBridge.getBranches()

        var branches: [GitBranchWrapper] = []
        if let branchesData = branchesArray as? [[String: Any]] {
            for branchData in branchesData {
                var lastCommit: GitCommitWrapper?
                if let commitData = branchData["lastCommit"] as? [String: Any] {
                    lastCommit = GitCommitWrapper(
                        hash: commitData["hash"] as? String ?? "",
                        shortHash: commitData["shortHash"] as? String ?? "",
                        author: commitData["author"] as? String ?? "",
                        email: "",
                        message: commitData["message"] as? String ?? "",
                        shortMessage: commitData["message"] as? String ?? "",
                        fullMessage: commitData["message"] as? String ?? "",
                        timestamp: commitData["timestamp"] as? Date ?? Date(),
                        parentHashes: [],
                        isMerge: false
                    )
                }

                let branch = GitBranchWrapper(
                    name: branchData["name"] as? String ?? "",
                    fullName: branchData["fullName"] as? String ?? "",
                    displayName: branchData["displayName"] as? String ?? "",
                    isRemote: branchData["isRemote"] as? Bool ?? false,
                    isCurrent: branchData["isCurrent"] as? Bool ?? false,
                    upstreamBranch: branchData["upstreamBranch"] as? String,
                    aheadCount: branchData["aheadCount"] as? Int ?? 0,
                    behindCount: branchData["behindCount"] as? Int ?? 0,
                    lastCommit: lastCommit
                )
                branches.append(branch)
            }
        }

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
            let changesArray = self.gitBridge.getCommitChanges(commitHash)

            var changes: [GitFileChangeWrapper] = []
            if let changesData = changesArray as? [[String: Any]] {
                for change in changesData {
                    let wrapper = GitFileChangeWrapper(
                        filePath: change["filePath"] as? String ?? "",
                        fileName: change["fileName"] as? String ?? "",
                        directoryPath: change["directoryPath"] as? String ?? "",
                        status: GitFileStatus(rawValue: change["status"] as? Int ?? 0) ?? .modified,
                        isStaged: change["isStaged"] as? Bool ?? false,
                        linesAdded: change["linesAdded"] as? Int ?? 0,
                        linesDeleted: change["linesDeleted"] as? Int ?? 0
                    )
                    changes.append(wrapper)
                }
            }

            DispatchQueue.main.async {
                completion(changes)
            }
        }
    }
    
    func loadFileDiff(filePath: String, commitHash: String, completion: @escaping (GitDiffWrapper?) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            let diffDict = self.gitBridge.getFileDiff(filePath, commitHash: commitHash)

            var diff: GitDiffWrapper?
            if let diffData = diffDict as? [String: Any] {
                var hunks: [GitDiffHunkWrapper] = []
                if let hunksArray = diffData["hunks"] as? [[String: Any]] {
                    for hunkData in hunksArray {
                        var lines: [GitDiffLineWrapper] = []
                        if let linesArray = hunkData["lines"] as? [[String: Any]] {
                            for lineData in linesArray {
                                let lineType = GitDiffLineType(rawValue: lineData["type"] as? Int ?? 0) ?? .context
                                let line = GitDiffLineWrapper(
                                    type: lineType,
                                    content: lineData["content"] as? String ?? "",
                                    oldLineNumber: lineData["oldLineNumber"] as? Int ?? 0,
                                    newLineNumber: lineData["newLineNumber"] as? Int ?? 0
                                )
                                lines.append(line)
                            }
                        }

                        let hunk = GitDiffHunkWrapper(
                            header: hunkData["header"] as? String ?? "",
                            oldStart: hunkData["oldStart"] as? Int ?? 0,
                            oldCount: hunkData["oldCount"] as? Int ?? 0,
                            newStart: hunkData["newStart"] as? Int ?? 0,
                            newCount: hunkData["newCount"] as? Int ?? 0,
                            lines: lines
                        )
                        hunks.append(hunk)
                    }
                }

                diff = GitDiffWrapper(
                    filePath: diffData["filePath"] as? String ?? "",
                    isBinary: diffData["isBinary"] as? Bool ?? false,
                    isNewFile: diffData["isNewFile"] as? Bool ?? false,
                    isDeletedFile: diffData["isDeletedFile"] as? Bool ?? false,
                    hunks: hunks,
                    rawContent: diffData["rawContent"] as? String ?? ""
                )
            }

            DispatchQueue.main.async {
                completion(diff)
            }
        }
    }
    
    func loadBranchCommits(branchName: String, completion: @escaping ([GitCommitWrapper]) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            let commitsArray = self.gitBridge.getBranchCommits(branchName, maxCount: 50)

            var commits: [GitCommitWrapper] = []
            if let commitsData = commitsArray as? [[String: Any]] {
                for commitData in commitsData {
                    let commit = GitCommitWrapper(
                        hash: commitData["hash"] as? String ?? "",
                        shortHash: commitData["shortHash"] as? String ?? "",
                        author: commitData["author"] as? String ?? "",
                        email: commitData["email"] as? String ?? "",
                        message: commitData["message"] as? String ?? "",
                        shortMessage: commitData["shortMessage"] as? String ?? "",
                        fullMessage: commitData["message"] as? String ?? "",
                        timestamp: commitData["timestamp"] as? Date ?? Date(),
                        parentHashes: commitData["parentHashes"] as? [String] ?? [],
                        isMerge: commitData["isMerge"] as? Bool ?? false
                    )
                    commits.append(commit)
                }
            }

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

enum GitFileStatus: Int {
    case added = 0
    case modified = 1
    case deleted = 2
    case renamed = 3
    case copied = 4
    case untracked = 5
    case conflicted = 6
    case ignored = 7
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

enum GitDiffLineType: Int {
    case context = 0
    case addition = 1
    case deletion = 2
    case header = 3
}