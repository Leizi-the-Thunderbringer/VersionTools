import SwiftUI

struct BranchesView: View {
    @ObservedObject var gitManager: GitManagerWrapper
    @State private var selectedBranch: GitBranchWrapper?
    @State private var showingNewBranchDialog = false
    @State private var showingMergeDialog = false
    @State private var selectedBranchType: BranchType = .local
    @State private var searchText = ""
    
    enum BranchType: String, CaseIterable {
        case local = "Local"
        case remote = "Remote"
        case all = "All"
        
        var systemImage: String {
            switch self {
            case .local:
                return "laptopcomputer"
            case .remote:
                return "globe"
            case .all:
                return "list.bullet"
            }
        }
    }
    
    var filteredBranches: [GitBranchWrapper] {
        let branches: [GitBranchWrapper]
        
        switch selectedBranchType {
        case .local:
            branches = gitManager.localBranches
        case .remote:
            branches = gitManager.remoteBranches
        case .all:
            branches = gitManager.allBranches
        }
        
        if searchText.isEmpty {
            return branches
        }
        
        return branches.filter { branch in
            branch.name.localizedCaseInsensitiveContains(searchText)
        }
    }
    
    var body: some View {
        HSplitView {
            // Branch List
            VStack(spacing: 0) {
                // Search and Filter
                VStack(spacing: 12) {
                    // Search bar
                    HStack {
                        Image(systemName: "magnifyingglass")
                            .foregroundColor(.secondary)
                        
                        TextField("Search branches...", text: $searchText)
                            .textFieldStyle(.plain)
                    }
                    .padding(.horizontal, 8)
                    .padding(.vertical, 6)
                    .background(Color.gray.opacity(0.1), in: RoundedRectangle(cornerRadius: 6))
                    
                    // Type filter
                    Picker("Branch Type", selection: $selectedBranchType) {
                        ForEach(BranchType.allCases, id: \.self) { type in
                            Label(type.rawValue, systemImage: type.systemImage)
                                .tag(type)
                        }
                    }
                    .pickerStyle(.segmented)
                }
                .padding()
                
                // Branch List
                List(filteredBranches, id: \.fullName, selection: $selectedBranch) { branch in
                    BranchRowView(
                        branch: branch,
                        isSelected: selectedBranch?.fullName == branch.fullName,
                        onSelect: { selectedBranch = branch },
                        onCheckout: { checkoutBranch(branch) },
                        onDelete: { deleteBranch(branch) }
                    )
                    .tag(branch)
                }
                .listStyle(.sidebar)
                
                // Action Buttons
                HStack {
                    Button("New Branch") {
                        showingNewBranchDialog = true
                    }
                    .buttonStyle(.glass)
                    
                    Button("Merge") {
                        showingMergeDialog = true
                    }
                    .buttonStyle(.glass)
                    .disabled(selectedBranch == nil || selectedBranch?.isCurrent == true)
                    
                    Spacer()
                    
                    Button("Refresh") {
                        gitManager.refreshBranches()
                    }
                    .buttonStyle(.glass)
                }
                .padding()
            }
            .frame(minWidth: 300)
            
            // Branch Detail
            if let selectedBranch = selectedBranch {
                BranchDetailView(branch: selectedBranch, gitManager: gitManager)
                    .frame(minWidth: 400)
            } else {
                EmptyBranchDetailView()
                    .frame(minWidth: 400)
            }
        }
        .navigationTitle("Branches")
        .sheet(isPresented: $showingNewBranchDialog) {
            NewBranchDialogView(gitManager: gitManager)
        }
        .sheet(isPresented: $showingMergeDialog) {
            if let branch = selectedBranch {
                MergeBranchDialogView(branch: branch, gitManager: gitManager)
            }
        }
        .onAppear {
            gitManager.loadBranches()
            if let currentBranch = gitManager.currentBranchInfo {
                selectedBranch = currentBranch
            }
        }
        .refreshable {
            await gitManager.refreshBranches()
        }
    }
    
    private func checkoutBranch(_ branch: GitBranchWrapper) {
        gitManager.checkoutBranch(branch.name) { success in
            if success {
                gitManager.refreshStatus()
            }
        }
    }
    
    private func deleteBranch(_ branch: GitBranchWrapper) {
        gitManager.deleteBranch(branch.name) { success in
            if success {
                gitManager.refreshBranches()
                if selectedBranch?.fullName == branch.fullName {
                    selectedBranch = nil
                }
            }
        }
    }
}

struct BranchRowView: View {
    let branch: GitBranchWrapper
    let isSelected: Bool
    let onSelect: () -> Void
    let onCheckout: () -> Void
    let onDelete: () -> Void
    
    var body: some View {
        HStack {
            // Branch icon and current indicator
            HStack(spacing: 8) {
                Image(systemName: branch.isCurrent ? "checkmark.circle.fill" : branchIcon)
                    .foregroundColor(branch.isCurrent ? .green : (branch.isRemote ? .blue : .primary))
                    .frame(width: 16)
                
                VStack(alignment: .leading, spacing: 2) {
                    Text(branch.displayName)
                        .font(.body)
                        .fontWeight(branch.isCurrent ? .semibold : .regular)
                        .lineLimit(1)
                    
                    if let upstream = branch.upstreamBranch, !upstream.isEmpty {
                        Text("↗ \(upstream)")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                }
            }
            
            Spacer()
            
            // Tracking status
            HStack(spacing: 4) {
                if branch.aheadCount > 0 {
                    Label("\(branch.aheadCount)", systemImage: "arrow.up")
                        .font(.caption2)
                        .foregroundColor(.green)
                }
                
                if branch.behindCount > 0 {
                    Label("\(branch.behindCount)", systemImage: "arrow.down")
                        .font(.caption2)
                        .foregroundColor(.red)
                }
            }
            
            // Last commit info
            if let lastCommit = branch.lastCommit {
                VStack(alignment: .trailing, spacing: 2) {
                    Text(lastCommit.relativeTime)
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    Text(lastCommit.shortHash)
                        .font(.caption2)
                        .fontFamily(.monospaced)
                        .foregroundColor(.blue)
                }
            }
        }
        .padding(.vertical, 4)
        .background(isSelected ? Color.blue.opacity(0.1) : Color.clear)
        .cornerRadius(6)
        .contentShape(Rectangle())
        .onTapGesture {
            onSelect()
        }
        .contextMenu {
            BranchContextMenu(
                branch: branch,
                onCheckout: onCheckout,
                onDelete: onDelete
            )
        }
    }
    
    private var branchIcon: String {
        if branch.isRemote {
            return "globe"
        } else {
            return "arrow.triangle.branch"
        }
    }
}

struct BranchDetailView: View {
    let branch: GitBranchWrapper
    @ObservedObject var gitManager: GitManagerWrapper
    @State private var commits: [GitCommitWrapper] = []
    @State private var selectedTab: BranchDetailTab = .commits
    
    enum BranchDetailTab: String, CaseIterable {
        case commits = "Commits"
        case info = "Info"
        
        var systemImage: String {
            switch self {
            case .commits:
                return "clock"
            case .info:
                return "info.circle"
            }
        }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Image(systemName: branch.isCurrent ? "checkmark.circle.fill" : "arrow.triangle.branch")
                        .foregroundColor(branch.isCurrent ? .green : .blue)
                        .font(.title2)
                    
                    VStack(alignment: .leading, spacing: 2) {
                        Text(branch.displayName)
                            .font(.title2)
                            .fontWeight(.semibold)
                        
                        if branch.isCurrent {
                            Text("Current Branch")
                                .font(.caption)
                                .foregroundColor(.green)
                        }
                    }
                    
                    Spacer()
                    
                    if !branch.isCurrent {
                        Button("Checkout") {
                            gitManager.checkoutBranch(branch.name) { _ in }
                        }
                        .buttonStyle(.glassProminent)
                    }
                }
                
                if let upstream = branch.upstreamBranch, !upstream.isEmpty {
                    Text("Tracking: \(upstream)")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }
            .padding()
            .background(Color(NSColor.controlBackgroundColor).opacity(0.5))
            
            // Tab Bar
            Picker("Detail Tab", selection: $selectedTab) {
                ForEach(BranchDetailTab.allCases, id: \.self) { tab in
                    Label(tab.rawValue, systemImage: tab.systemImage)
                        .tag(tab)
                }
            }
            .pickerStyle(.segmented)
            .padding()
            
            // Content
            switch selectedTab {
            case .commits:
                BranchCommitsView(branch: branch, commits: $commits, gitManager: gitManager)
            case .info:
                BranchInfoView(branch: branch)
            }
        }
        .onAppear {
            loadBranchCommits()
        }
        .onChange(of: branch.fullName) { _ in
            loadBranchCommits()
        }
    }
    
    private func loadBranchCommits() {
        gitManager.loadBranchCommits(branchName: branch.name) { branchCommits in
            commits = branchCommits
        }
    }
}

struct BranchCommitsView: View {
    let branch: GitBranchWrapper
    @Binding var commits: [GitCommitWrapper]
    @ObservedObject var gitManager: GitManagerWrapper
    
    var body: some View {
        ScrollView {
            LazyVStack(spacing: 8) {
                ForEach(commits, id: \.hash) { commit in
                    CommitRowCompactView(commit: commit)
                        .padding(.horizontal)
                }
            }
            .padding(.vertical)
        }
        .background(Color.clear)
    }
}

struct CommitRowCompactView: View {
    let commit: GitCommitWrapper
    
    var body: some View {
        HStack {
            Circle()
                .fill(Color.blue)
                .frame(width: 6, height: 6)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(commit.shortMessage)
                    .font(.body)
                    .lineLimit(1)
                
                HStack {
                    Text(commit.author)
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    Spacer()
                    
                    Text(commit.relativeTime)
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }
            
            Text(commit.shortHash)
                .font(.caption)
                .fontFamily(.monospaced)
                .foregroundColor(.blue)
                .padding(.horizontal, 4)
                .padding(.vertical, 1)
                .background(Color.blue.opacity(0.1), in: RoundedRectangle(cornerRadius: 3))
        }
        .padding(.vertical, 4)
    }
}

struct BranchInfoView: View {
    let branch: GitBranchWrapper
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                InfoSection(title: "Basic Information") {
                    InfoRow(label: "Name", value: branch.name)
                    InfoRow(label: "Full Name", value: branch.fullName)
                    InfoRow(label: "Type", value: branch.isRemote ? "Remote" : "Local")
                    InfoRow(label: "Current", value: branch.isCurrent ? "Yes" : "No")
                }
                
                if let upstream = branch.upstreamBranch, !upstream.isEmpty {
                    InfoSection(title: "Tracking") {
                        InfoRow(label: "Upstream", value: upstream)
                        InfoRow(label: "Ahead", value: "\(branch.aheadCount)")
                        InfoRow(label: "Behind", value: "\(branch.behindCount)")
                    }
                }
                
                if let lastCommit = branch.lastCommit {
                    InfoSection(title: "Last Commit") {
                        InfoRow(label: "Hash", value: lastCommit.hash, isMonospaced: true)
                        InfoRow(label: "Author", value: lastCommit.author)
                        InfoRow(label: "Date", value: lastCommit.formattedDate)
                        InfoRow(label: "Message", value: lastCommit.shortMessage)
                    }
                }
                
                Spacer()
            }
            .padding()
        }
    }
}

struct InfoSection<Content: View>: View {
    let title: String
    let content: Content
    
    init(title: String, @ViewBuilder content: () -> Content) {
        self.title = title
        self.content = content()
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text(title)
                .font(.headline)
                .foregroundColor(.primary)
            
            VStack(alignment: .leading, spacing: 4) {
                content
            }
            .padding(.leading, 8)
        }
    }
}

struct InfoRow: View {
    let label: String
    let value: String
    let isMonospaced: Bool
    
    init(label: String, value: String, isMonospaced: Bool = false) {
        self.label = label
        self.value = value
        self.isMonospaced = isMonospaced
    }
    
    var body: some View {
        HStack {
            Text(label)
                .fontWeight(.medium)
                .frame(width: 80, alignment: .leading)
            
            Text(value)
                .font(isMonospaced ? .body.monospaced() : .body)
                .foregroundColor(.primary)
                .textSelection(.enabled)
            
            Spacer()
        }
    }
}

struct EmptyBranchDetailView: View {
    var body: some View {
        VStack(spacing: 16) {
            Image(systemName: "arrow.triangle.branch")
                .font(.system(size: 48))
                .foregroundColor(.secondary)
            
            Text("Select a Branch")
                .font(.headline)
            
            Text("Choose a branch to view its details and commits")
                .font(.body)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}

struct BranchContextMenu: View {
    let branch: GitBranchWrapper
    let onCheckout: () -> Void
    let onDelete: () -> Void
    
    var body: some View {
        Group {
            if !branch.isCurrent {
                Button("Checkout") {
                    onCheckout()
                }
            }
            
            Button("Copy Name") {
                NSPasteboard.general.clearContents()
                NSPasteboard.general.setString(branch.name, forType: .string)
            }
            
            Divider()
            
            Button("Merge into Current") {
                // TODO: Implement merge
            }
            
            Button("Rebase onto Current") {
                // TODO: Implement rebase
            }
            
            Divider()
            
            if !branch.isCurrent && !branch.isRemote {
                Button("Delete") {
                    onDelete()
                }
                .foregroundColor(.red)
            }
        }
    }
}

#Preview {
    BranchesView(gitManager: GitManagerWrapper())
}