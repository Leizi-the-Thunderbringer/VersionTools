import SwiftUI

struct CommitDialogView: View {
    @ObservedObject var gitManager: GitManagerWrapper
    @Binding var message: String
    @Environment(\.dismiss) private var dismiss
    @State private var isCommitting = false
    @State private var showingError = false
    @State private var errorMessage = ""
    
    var body: some View {
        NavigationStack {
            VStack(spacing: 20) {
                // Header
                VStack(spacing: 8) {
                    Text("Commit Changes")
                        .font(.largeTitle)
                        .fontWeight(.semibold)
                    
                    Text("\(gitManager.stagedChanges.count) files staged for commit")
                        .font(.body)
                        .foregroundColor(.secondary)
                }
                
                // Commit message input
                VStack(alignment: .leading, spacing: 8) {
                    Text("Commit Message")
                        .font(.headline)
                    
                    ZStack(alignment: .topLeading) {
                        if message.isEmpty {
                            Text("Enter commit message...")
                                .foregroundColor(.secondary)
                                .padding(.top, 8)
                                .padding(.leading, 4)
                        }
                        
                        TextEditor(text: $message)
                            .font(.body)
                            .scrollContentBackground(.hidden)
                            .background(Color.clear)
                    }
                    .frame(height: 120)
                    .padding(8)
                    .background(Color.gray.opacity(0.1), in: RoundedRectangle(cornerRadius: 8))
                    
                    // Character count
                    HStack {
                        Spacer()
                        Text("\(message.count) characters")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                }
                
                // Staged files preview
                VStack(alignment: .leading, spacing: 8) {
                    Text("Files to Commit")
                        .font(.headline)
                    
                    ScrollView {
                        LazyVStack(alignment: .leading, spacing: 4) {
                            ForEach(gitManager.stagedChanges.prefix(5), id: \.filePath) { change in
                                HStack {
                                    StatusIndicatorView(status: change.status)
                                    Text(change.fileName)
                                        .font(.body)
                                    Spacer()
                                }
                            }
                            
                            if gitManager.stagedChanges.count > 5 {
                                Text("... and \(gitManager.stagedChanges.count - 5) more files")
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                            }
                        }
                    }
                    .frame(maxHeight: 120)
                    .padding(8)
                    .background(Color.gray.opacity(0.05), in: RoundedRectangle(cornerRadius: 8))
                }
                
                Spacer()
            }
            .padding()
            .frame(width: 500, height: 500)
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") {
                        dismiss()
                    }
                    .disabled(isCommitting)
                }
                
                ToolbarItem(placement: .confirmationAction) {
                    Button("Commit") {
                        performCommit()
                    }
                    .buttonStyle(.glassProminent)
                    .disabled(message.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty || isCommitting)
                }
            }
        }
        .alert("Commit Failed", isPresented: $showingError) {
            Button("OK") { }
        } message: {
            Text(errorMessage)
        }
    }
    
    private func performCommit() {
        isCommitting = true
        
        gitManager.commit(message: message) { success in
            isCommitting = false
            
            if success {
                message = ""
                dismiss()
            } else {
                errorMessage = "Failed to create commit. Please check your staged changes and try again."
                showingError = true
            }
        }
    }
}

struct NewBranchDialogView: View {
    @ObservedObject var gitManager: GitManagerWrapper
    @Environment(\.dismiss) private var dismiss
    @State private var branchName = ""
    @State private var startPoint = "HEAD"
    @State private var checkoutAfterCreate = true
    @State private var isCreating = false
    @State private var showingError = false
    @State private var errorMessage = ""
    
    var body: some View {
        NavigationStack {
            VStack(spacing: 20) {
                Text("Create New Branch")
                    .font(.largeTitle)
                    .fontWeight(.semibold)
                
                VStack(alignment: .leading, spacing: 16) {
                    // Branch name
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Branch Name")
                            .font(.headline)
                        
                        TextField("feature/new-feature", text: $branchName)
                            .textFieldStyle(.roundedBorder)
                    }
                    
                    // Start point
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Start Point")
                            .font(.headline)
                        
                        HStack {
                            TextField("HEAD", text: $startPoint)
                                .textFieldStyle(.roundedBorder)
                            
                            Button("Browse") {
                                // TODO: Implement branch/commit browser
                            }
                            .buttonStyle(.glass)
                        }
                    }
                    
                    // Options
                    Toggle("Checkout after creation", isOn: $checkoutAfterCreate)
                }
                
                Spacer()
            }
            .padding()
            .frame(width: 400, height: 300)
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") {
                        dismiss()
                    }
                    .disabled(isCreating)
                }
                
                ToolbarItem(placement: .confirmationAction) {
                    Button("Create") {
                        createBranch()
                    }
                    .buttonStyle(.glassProminent)
                    .disabled(branchName.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty || isCreating)
                }
            }
        }
        .alert("Branch Creation Failed", isPresented: $showingError) {
            Button("OK") { }
        } message: {
            Text(errorMessage)
        }
    }
    
    private func createBranch() {
        isCreating = true
        
        gitManager.createBranch(branchName) { success in
            isCreating = false
            
            if success {
                if checkoutAfterCreate {
                    gitManager.checkoutBranch(branchName) { _ in
                        dismiss()
                    }
                } else {
                    dismiss()
                }
            } else {
                errorMessage = "Failed to create branch '\(branchName)'. Please check the name and try again."
                showingError = true
            }
        }
    }
}

struct MergeBranchDialogView: View {
    let branch: GitBranchWrapper
    @ObservedObject var gitManager: GitManagerWrapper
    @Environment(\.dismiss) private var dismiss
    @State private var mergeStrategy: MergeStrategy = .createMergeCommit
    @State private var commitMessage = ""
    @State private var isMerging = false
    @State private var showingError = false
    @State private var errorMessage = ""
    
    enum MergeStrategy: String, CaseIterable {
        case createMergeCommit = "Create Merge Commit"
        case fastForward = "Fast Forward"
        case squashAndMerge = "Squash and Merge"
        
        var description: String {
            switch self {
            case .createMergeCommit:
                return "Create a merge commit even if fast-forward is possible"
            case .fastForward:
                return "Fast-forward if possible, otherwise create merge commit"
            case .squashAndMerge:
                return "Squash all commits into a single commit"
            }
        }
    }
    
    var body: some View {
        NavigationStack {
            VStack(spacing: 20) {
                VStack(spacing: 8) {
                    Text("Merge Branch")
                        .font(.largeTitle)
                        .fontWeight(.semibold)
                    
                    Text("Merge '\(branch.displayName)' into '\(gitManager.currentBranch ?? "current branch")'")
                        .font(.body)
                        .foregroundColor(.secondary)
                        .multilineTextAlignment(.center)
                }
                
                VStack(alignment: .leading, spacing: 16) {
                    // Merge strategy
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Merge Strategy")
                            .font(.headline)
                        
                        Picker("Strategy", selection: $mergeStrategy) {
                            ForEach(MergeStrategy.allCases, id: \.self) { strategy in
                                Text(strategy.rawValue)
                                    .tag(strategy)
                            }
                        }
                        .pickerStyle(.menu)
                        
                        Text(mergeStrategy.description)
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                    
                    // Commit message (for merge commits)
                    if mergeStrategy != .fastForward {
                        VStack(alignment: .leading, spacing: 8) {
                            Text("Commit Message")
                                .font(.headline)
                            
                            TextField("Merge \(branch.displayName)", text: $commitMessage)
                                .textFieldStyle(.roundedBorder)
                        }
                    }
                }
                
                Spacer()
            }
            .padding()
            .frame(width: 450, height: 350)
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") {
                        dismiss()
                    }
                    .disabled(isMerging)
                }
                
                ToolbarItem(placement: .confirmationAction) {
                    Button("Merge") {
                        performMerge()
                    }
                    .buttonStyle(.glassProminent)
                    .disabled(isMerging)
                }
            }
        }
        .onAppear {
            commitMessage = "Merge \(branch.displayName)"
        }
        .alert("Merge Failed", isPresented: $showingError) {
            Button("OK") { }
        } message: {
            Text(errorMessage)
        }
    }
    
    private func performMerge() {
        isMerging = true
        
        // TODO: Implement actual merge logic with strategy
        // For now, just simulate
        DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
            isMerging = false
            errorMessage = "Merge functionality not yet implemented"
            showingError = true
        }
    }
}

#Preview {
    CommitDialogView(
        gitManager: GitManagerWrapper(),
        message: .constant("")
    )
}