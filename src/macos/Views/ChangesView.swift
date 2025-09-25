import SwiftUI

struct ChangesView: View {
    @ObservedObject var gitManager: GitManagerWrapper
    @State private var selectedFiles: Set<GitFileChangeWrapper> = []
    @State private var commitMessage = ""
    @State private var showingCommitDialog = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Button("Stage All") {
                    gitManager.stageAllChanges()
                }
                .buttonStyle(.glass)
                .disabled(gitManager.unstagedChanges.isEmpty)
                
                Button("Unstage All") {
                    gitManager.unstageAllChanges()
                }
                .buttonStyle(.glass)
                .disabled(gitManager.stagedChanges.isEmpty)
                
                Spacer()
                
                Button("Commit") {
                    showingCommitDialog = true
                }
                .buttonStyle(.glassProminent)
                .disabled(gitManager.stagedChanges.isEmpty)
            }
            .padding()
            .background(Color.clear)
            
            // Changes List
            List {
                if !gitManager.stagedChanges.isEmpty {
                    Section {
                        ForEach(gitManager.stagedChanges, id: \.filePath) { change in
                            FileChangeRow(
                                change: change,
                                isSelected: selectedFiles.contains(change),
                                onToggle: { toggleFileSelection(change) },
                                onStageToggle: { gitManager.unstageFile(change.filePath) }
                            )
                        }
                    } header: {
                        SectionHeaderView(
                            title: "Staged Changes",
                            count: gitManager.stagedChanges.count,
                            systemImage: "plus.circle.fill",
                            color: .blue
                        )
                    }
                }
                
                if !gitManager.unstagedChanges.isEmpty {
                    Section {
                        ForEach(gitManager.unstagedChanges, id: \.filePath) { change in
                            FileChangeRow(
                                change: change,
                                isSelected: selectedFiles.contains(change),
                                onToggle: { toggleFileSelection(change) },
                                onStageToggle: { gitManager.stageFile(change.filePath) }
                            )
                        }
                    } header: {
                        SectionHeaderView(
                            title: "Unstaged Changes",
                            count: gitManager.unstagedChanges.count,
                            systemImage: "minus.circle.fill",
                            color: .orange
                        )
                    }
                }
                
                if gitManager.stagedChanges.isEmpty && gitManager.unstagedChanges.isEmpty {
                    EmptyStateView(
                        systemImage: "checkmark.circle.fill",
                        title: "No Changes",
                        message: "Your working directory is clean"
                    )
                }
            }
            .listStyle(.sidebar)
        }
        .navigationTitle("Changes")
        .sheet(isPresented: $showingCommitDialog) {
            CommitDialogView(
                gitManager: gitManager,
                message: $commitMessage
            )
        }
        .refreshable {
            await gitManager.refreshStatus()
        }
    }
    
    private func toggleFileSelection(_ change: GitFileChangeWrapper) {
        if selectedFiles.contains(change) {
            selectedFiles.remove(change)
        } else {
            selectedFiles.insert(change)
        }
    }
}

struct FileChangeRow: View {
    let change: GitFileChangeWrapper
    let isSelected: Bool
    let onToggle: () -> Void
    let onStageToggle: () -> Void
    
    var body: some View {
        HStack {
            // Status indicator
            StatusIndicatorView(status: change.status)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(change.fileName)
                    .font(.body)
                    .fontWeight(.medium)
                    .lineLimit(1)
                
                if !change.directoryPath.isEmpty {
                    Text(change.directoryPath)
                        .font(.caption)
                        .foregroundColor(.secondary)
                        .lineLimit(1)
                }
                
                if change.linesAdded > 0 || change.linesDeleted > 0 {
                    HStack(spacing: 8) {
                        if change.linesAdded > 0 {
                            Label("+\(change.linesAdded)", systemImage: "plus")
                                .font(.caption2)
                                .foregroundColor(.green)
                        }
                        
                        if change.linesDeleted > 0 {
                            Label("-\(change.linesDeleted)", systemImage: "minus")
                                .font(.caption2)
                                .foregroundColor(.red)
                        }
                    }
                }
            }
            
            Spacer()
            
            // Stage/Unstage button
            Button(action: onStageToggle) {
                Image(systemName: change.isStaged ? "minus.circle" : "plus.circle")
                    .foregroundColor(change.isStaged ? .red : .blue)
            }
            .buttonStyle(.plain)
        }
        .padding(.vertical, 4)
        .background(isSelected ? Color.blue.opacity(0.1) : Color.clear)
        .cornerRadius(8)
        .contentShape(Rectangle())
        .onTapGesture {
            onToggle()
        }
        .contextMenu {
            ContextMenuView(change: change, onStageToggle: onStageToggle)
        }
    }
}

struct StatusIndicatorView: View {
    let status: GitFileStatus
    
    var body: some View {
        Circle()
            .fill(statusColor)
            .frame(width: 8, height: 8)
            .overlay(
                Text(statusText)
                    .font(.system(size: 6, weight: .bold))
                    .foregroundColor(.white)
            )
    }
    
    private var statusColor: Color {
        switch status {
        case .added:
            return .green
        case .modified:
            return .blue
        case .deleted:
            return .red
        case .renamed:
            return .orange
        case .copied:
            return .purple
        case .untracked:
            return .gray
        case .conflicted:
            return .red
        case .ignored:
            return .gray
        }
    }
    
    private var statusText: String {
        switch status {
        case .added:
            return "A"
        case .modified:
            return "M"
        case .deleted:
            return "D"
        case .renamed:
            return "R"
        case .copied:
            return "C"
        case .untracked:
            return "?"
        case .conflicted:
            return "!"
        case .ignored:
            return "I"
        }
    }
}

struct SectionHeaderView: View {
    let title: String
    let count: Int
    let systemImage: String
    let color: Color
    
    var body: some View {
        HStack {
            Image(systemName: systemImage)
                .foregroundColor(color)
            
            Text(title)
                .font(.headline)
            
            Text("(\(count))")
                .font(.subheadline)
                .foregroundColor(.secondary)
            
            Spacer()
        }
    }
}

struct ContextMenuView: View {
    let change: GitFileChangeWrapper
    let onStageToggle: () -> Void
    
    var body: some View {
        Group {
            Button(change.isStaged ? "Unstage" : "Stage") {
                onStageToggle()
            }
            
            Button("View Diff") {
                // TODO: Show diff view
            }
            
            if change.status != .added {
                Button("Discard Changes") {
                    // TODO: Implement discard changes
                }
            }
            
            Divider()
            
            Button("Open in Finder") {
                // TODO: Open file in Finder
            }
            
            Button("Copy Path") {
                NSPasteboard.general.clearContents()
                NSPasteboard.general.setString(change.filePath, forType: .string)
            }
        }
    }
}

struct EmptyStateView: View {
    let systemImage: String
    let title: String
    let message: String
    
    var body: some View {
        VStack(spacing: 16) {
            Image(systemName: systemImage)
                .font(.system(size: 48))
                .foregroundColor(.secondary)
            
            VStack(spacing: 4) {
                Text(title)
                    .font(.headline)
                
                Text(message)
                    .font(.body)
                    .foregroundColor(.secondary)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color.clear)
    }
}

#Preview {
    ChangesView(gitManager: GitManagerWrapper())
}