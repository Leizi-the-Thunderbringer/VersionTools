import SwiftUI

struct HistoryView: View {
    @ObservedObject var gitManager: GitManagerWrapper
    @State private var selectedCommit: GitCommitWrapper?
    @State private var searchText = ""
    @State private var showingBranchFilter = false
    @State private var selectedBranch: String = "All Branches"
    
    var filteredCommits: [GitCommitWrapper] {
        let commits = gitManager.commitHistory
        
        if searchText.isEmpty {
            return commits
        }
        
        return commits.filter { commit in
            commit.message.localizedCaseInsensitiveContains(searchText) ||
            commit.author.localizedCaseInsensitiveContains(searchText) ||
            commit.hash.localizedCaseInsensitiveContains(searchText)
        }
    }
    
    var body: some View {
        HSplitView {
            // Commit List
            VStack(spacing: 0) {
                // Search and Filter Bar
                HStack {
                    HStack {
                        Image(systemName: "magnifyingglass")
                            .foregroundColor(.secondary)
                        
                        TextField("Search commits...", text: $searchText)
                            .textFieldStyle(.plain)
                    }
                    .padding(.horizontal, 8)
                    .padding(.vertical, 6)
                    .background(Color.gray.opacity(0.1), in: RoundedRectangle(cornerRadius: 6))
                    
                    Button(selectedBranch) {
                        showingBranchFilter = true
                    }
                    .buttonStyle(.glass)
                    .menuIndicator(.visible)
                }
                .padding()
                
                // Commit Timeline
                ScrollView {
                    LazyVStack(spacing: 0) {
                        ForEach(filteredCommits, id: \.hash) { commit in
                            CommitRowView(
                                commit: commit,
                                isSelected: selectedCommit?.hash == commit.hash,
                                onSelect: { selectedCommit = commit }
                            )
                            .background(
                                RoundedRectangle(cornerRadius: 8)
                                    .fill(selectedCommit?.hash == commit.hash ? Color.blue.opacity(0.1) : Color.clear)
                            )
                            .padding(.horizontal)
                        }
                    }
                }
                .background(Color.clear)
            }
            .frame(minWidth: 400)
            
            // Commit Detail
            if let selectedCommit = selectedCommit {
                CommitDetailView(commit: selectedCommit, gitManager: gitManager)
                    .frame(minWidth: 500)
            } else {
                EmptyCommitDetailView()
                    .frame(minWidth: 500)
            }
        }
        .navigationTitle("History")
        .onAppear {
            gitManager.loadCommitHistory()
            if let firstCommit = gitManager.commitHistory.first {
                selectedCommit = firstCommit
            }
        }
        .refreshable {
            await gitManager.refreshCommitHistory()
        }
    }
}

struct CommitRowView: View {
    let commit: GitCommitWrapper
    let isSelected: Bool
    let isLast: Bool = false
    let onSelect: () -> Void
    
    var body: some View {
        HStack(spacing: 12) {
            // Timeline indicator
            VStack {
                Circle()
                    .fill(isSelected ? Color.blue : Color.secondary)
                    .frame(width: 8, height: 8)
                
                if !isLast { // Not the last commit
                    Rectangle()
                        .fill(Color.secondary.opacity(0.3))
                        .frame(width: 1)
                        .frame(maxHeight: .infinity)
                }
            }
            .frame(width: 8)
            
            VStack(alignment: .leading, spacing: 4) {
                // Commit message
                Text(commit.shortMessage)
                    .font(.body)
                    .fontWeight(.medium)
                    .lineLimit(2)
                    .multilineTextAlignment(.leading)
                
                // Author and time
                HStack {
                    Text(commit.author)
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    Spacer()
                    
                    Text(commit.relativeTime)
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
                
                // Hash and branch info
                HStack {
                    Text(commit.shortHash)
                        .font(.caption)

                        .font(.system(.body, design: .monospaced))
                        .foregroundColor(.blue)
                        .padding(.horizontal, 4)
                        .padding(.vertical, 1)
                        .background(Color.blue.opacity(0.1), in: RoundedRectangle(cornerRadius: 3))
                    
                    if commit.isMerge {
                        Label("Merge", systemImage: "arrow.triangle.merge")
                            .font(.caption2)
                            .foregroundColor(.purple)
                    }
                    
                    Spacer()
                }
            }
        }
        .padding(.vertical, 8)
        .contentShape(Rectangle())
        .onTapGesture {
            onSelect()
        }
        .contextMenu {
            CommitContextMenu(commit: commit)
        }
    }
}

struct CommitDetailView: View {
    let commit: GitCommitWrapper
    @ObservedObject var gitManager: GitManagerWrapper
    @State private var selectedTab: CommitDetailTab = .overview
    @State private var changedFiles: [GitFileChangeWrapper] = []
    
    enum CommitDetailTab: String, CaseIterable {
        case overview = "Overview"
        case changes = "Changes"
        
        var systemImage: String {
            switch self {
            case .overview:
                return "info.circle"
            case .changes:
                return "doc.text"
            }
        }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Tab Bar
            Picker("Detail Tab", selection: $selectedTab) {
                ForEach(CommitDetailTab.allCases, id: \.self) { tab in
                    Label(tab.rawValue, systemImage: tab.systemImage)
                        .tag(tab)
                }
            }
            .pickerStyle(.segmented)
            .padding()
            
            // Content
            switch selectedTab {
            case .overview:
                CommitOverviewView(commit: commit)
            case .changes:
                CommitChangesView(commit: commit, changedFiles: $changedFiles, gitManager: gitManager)
            }
        }
        .background(Color.clear)
        .onAppear {
            loadCommitDetails()
        }
        .onChange(of: commit.hash) {
            loadCommitDetails()
        }
    }
    
    private func loadCommitDetails() {
        gitManager.loadCommitChanges(commitHash: commit.hash) { files in
            changedFiles = files
        }
    }
}

struct CommitOverviewView: View {
    let commit: GitCommitWrapper
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                // Commit Header
                VStack(alignment: .leading, spacing: 8) {
                    Text(commit.shortMessage)
                        .font(.title2)
                        .fontWeight(.semibold)
                    
                    if commit.message != commit.shortMessage {
                        Text(commit.fullMessage)
                            .font(.body)
                            .foregroundColor(.primary)
                    }
                }
                
                Divider()
                
                // Commit Metadata
                VStack(alignment: .leading, spacing: 12) {
                    MetadataRow(label: "Author", value: commit.author, systemImage: "person.fill")
                    MetadataRow(label: "Email", value: commit.email, systemImage: "envelope.fill")
                    MetadataRow(label: "Date", value: commit.formattedDate, systemImage: "calendar")
                    MetadataRow(label: "Hash", value: commit.hash, systemImage: "number", isMonospaced: true)
                    
                    if !commit.parentHashes.isEmpty {
                        VStack(alignment: .leading, spacing: 4) {
                            HStack {
                                Image(systemName: "arrow.up")
                                    .foregroundColor(.secondary)
                                    .frame(width: 16)
                                Text("Parents")
                                    .fontWeight(.medium)
                            }
                            
                            ForEach(commit.parentHashes, id: \.self) { parentHash in
                                Text(parentHash)
                                    .font(.caption)
                                    .font(.system(.body, design: .monospaced))
                                    .foregroundColor(.blue)
                                    .padding(.leading, 20)
                            }
                        }
                    }
                }
                
                Spacer()
            }
            .padding()
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }
}

struct MetadataRow: View {
    let label: String
    let value: String
    let systemImage: String
    let isMonospaced: Bool
    
    init(label: String, value: String, systemImage: String, isMonospaced: Bool = false) {
        self.label = label
        self.value = value
        self.systemImage = systemImage
        self.isMonospaced = isMonospaced
    }
    
    var body: some View {
        HStack(alignment: .top) {
            Image(systemName: systemImage)
                .foregroundColor(.secondary)
                .frame(width: 16)
            
            Text(label)
                .fontWeight(.medium)
                .frame(width: 60, alignment: .leading)
            
            Text(value)
                .font(isMonospaced ? .caption.monospaced() : .body)
                .foregroundColor(.primary)
                .textSelection(.enabled)
            
            Spacer()
        }
    }
}

struct CommitChangesView: View {
    let commit: GitCommitWrapper
    @Binding var changedFiles: [GitFileChangeWrapper]
    @ObservedObject var gitManager: GitManagerWrapper
    @State private var selectedFile: GitFileChangeWrapper?
    
    var body: some View {
        HSplitView {
            // File List
            List(changedFiles, id: \.filePath, selection: $selectedFile) { file in
                FileChangeRowCompact(change: file)
                    .tag(file)
            }
            .listStyle(.sidebar)
            .frame(minWidth: 200)
            
            // Diff View
            if let selectedFile = selectedFile {
                DiffView(
                    file: selectedFile,
                    commitHash: commit.hash,
                    gitManager: gitManager
                )
                .frame(minWidth: 400)
            } else {
                Text("Select a file to view diff")
                    .foregroundColor(.secondary)
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
    }
}

struct FileChangeRowCompact: View {
    let change: GitFileChangeWrapper
    
    var body: some View {
        HStack {
            StatusIndicatorView(status: change.status)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(change.fileName)
                    .font(.body)
                    .lineLimit(1)
                
                Text(change.directoryPath)
                    .font(.caption)
                    .foregroundColor(.secondary)
                    .lineLimit(1)
            }
            
            Spacer()
            
            if change.linesAdded > 0 || change.linesDeleted > 0 {
                HStack(spacing: 4) {
                    if change.linesAdded > 0 {
                        Text("+\(change.linesAdded)")
                            .font(.caption2)
                            .foregroundColor(.green)
                    }
                    
                    if change.linesDeleted > 0 {
                        Text("-\(change.linesDeleted)")
                            .font(.caption2)
                            .foregroundColor(.red)
                    }
                }
            }
        }
        .padding(.vertical, 2)
    }
}

struct EmptyCommitDetailView: View {
    var body: some View {
        VStack(spacing: 16) {
            Image(systemName: "doc.text")
                .font(.system(size: 48))
                .foregroundColor(.secondary)
            
            Text("Select a Commit")
                .font(.headline)
            
            Text("Choose a commit from the timeline to view its details")
                .font(.body)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}

struct CommitContextMenu: View {
    let commit: GitCommitWrapper
    
    var body: some View {
        Group {
            Button("Copy Hash") {
                NSPasteboard.general.clearContents()
                NSPasteboard.general.setString(commit.hash, forType: .string)
            }
            
            Button("Copy Short Hash") {
                NSPasteboard.general.clearContents()
                NSPasteboard.general.setString(commit.shortHash, forType: .string)
            }
            
            Divider()
            
            Button("Create Branch from Here") {
                // TODO: Implement branch creation
            }
            
            Button("Create Tag") {
                // TODO: Implement tag creation
            }
            
            Divider()
            
            Button("Revert This Commit") {
                // TODO: Implement revert
            }
            
            Button("Cherry-pick") {
                // TODO: Implement cherry-pick
            }
        }
    }
}

#Preview {
    HistoryView(gitManager: GitManagerWrapper())
}