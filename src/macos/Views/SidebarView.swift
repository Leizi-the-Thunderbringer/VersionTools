import SwiftUI

struct SidebarView: View {
    @Binding var selection: SidebarItem?
    @ObservedObject var gitManager: GitManagerWrapper
    @State private var showingRepositoryPicker = false
    
    var body: some View {
        List(selection: $selection) {
            // Repository Section
            Section {
                HStack {
                    Image(systemName: "folder.fill")
                        .foregroundColor(.blue)
                    
                    VStack(alignment: .leading, spacing: 2) {
                        Text(gitManager.repositoryName)
                            .font(.headline)
                            .lineLimit(1)
                        
                        if let currentBranch = gitManager.currentBranch {
                            Text("on \(currentBranch)")
                                .font(.caption)
                                .foregroundColor(.secondary)
                        }
                    }
                    
                    Spacer()
                    
                    Button(action: {
                        showingRepositoryPicker = true
                    }) {
                        Image(systemName: "ellipsis.circle")
                            .foregroundColor(.secondary)
                    }
                    .buttonStyle(.plain)
                }
                .padding(.vertical, 4)
            } header: {
                HStack {
                    Text("Repository")
                    Spacer()
                    if gitManager.isLoading {
                        ProgressView()
                            .scaleEffect(0.6)
                    }
                }
            }
            
            // Navigation Section
            Section("Navigation") {
                ForEach(SidebarItem.allCases) { item in
                    SidebarItemView(
                        item: item,
                        isSelected: selection == item,
                        badge: badgeCount(for: item)
                    )
                    .tag(item)
                }
            }
            
            // Status Section
            if let status = gitManager.repositoryStatus {
                Section("Status") {
                    StatusItemView(
                        title: "Ahead",
                        count: status.aheadCount,
                        systemImage: "arrow.up.circle.fill",
                        color: .green
                    )
                    
                    StatusItemView(
                        title: "Behind", 
                        count: status.behindCount,
                        systemImage: "arrow.down.circle.fill",
                        color: .orange
                    )
                    
                    StatusItemView(
                        title: "Staged",
                        count: status.stagedChangesCount,
                        systemImage: "plus.circle.fill",
                        color: .blue
                    )
                    
                    StatusItemView(
                        title: "Unstaged",
                        count: status.unstagedChangesCount,
                        systemImage: "minus.circle.fill",
                        color: .red
                    )
                }
            }
        }
        .listStyle(.sidebar)
        .navigationTitle("Version Tools")
        .toolbar {
            ToolbarItem(placement: .primaryAction) {
                Button("Open Repository") {
                    showingRepositoryPicker = true
                }
                .buttonStyle(.glass)
            }
        }
        .sheet(isPresented: $showingRepositoryPicker) {
            RepositoryPickerView(gitManager: gitManager)
        }
    }
    
    private func badgeCount(for item: SidebarItem) -> Int? {
        guard let status = gitManager.repositoryStatus else { return nil }
        
        switch item {
        case .changes:
            let total = status.stagedChangesCount + status.unstagedChangesCount
            return total > 0 ? total : nil
        case .stashes:
            return status.stashCount > 0 ? status.stashCount : nil
        default:
            return nil
        }
    }
}

struct SidebarItemView: View {
    let item: SidebarItem
    let isSelected: Bool
    let badge: Int?
    
    var body: some View {
        HStack {
            Image(systemName: item.systemImage)
                .foregroundColor(isSelected ? .white : .primary)
                .frame(width: 16)
            
            Text(item.title)
                .foregroundColor(isSelected ? .white : .primary)
            
            Spacer()
            
            if let badge = badge {
                Text("\(badge)")
                    .font(.caption2)
                    .fontWeight(.semibold)
                    .foregroundColor(.white)
                    .padding(.horizontal, 6)
                    .padding(.vertical, 2)
                    .background(Color.red, in: Capsule())
            }
        }
        .padding(.vertical, 2)
        .contentShape(Rectangle())
    }
}

struct StatusItemView: View {
    let title: String
    let count: Int
    let systemImage: String
    let color: Color
    
    var body: some View {
        if count > 0 {
            HStack {
                Image(systemName: systemImage)
                    .foregroundColor(color)
                    .frame(width: 16)
                
                Text(title)
                    .foregroundColor(.primary)
                
                Spacer()
                
                Text("\(count)")
                    .font(.caption)
                    .fontWeight(.medium)
                    .foregroundColor(.secondary)
            }
        }
    }
}

struct RepositoryPickerView: View {
    @ObservedObject var gitManager: GitManagerWrapper
    @Environment(\.dismiss) private var dismiss
    @State private var selectedURL: URL?
    
    var body: some View {
        NavigationStack {
            VStack(spacing: 20) {
                Text("Select Repository")
                    .font(.largeTitle)
                    .fontWeight(.semibold)
                
                Button("Choose Folder") {
                    let panel = NSOpenPanel()
                    panel.allowsMultipleSelection = false
                    panel.canChooseDirectories = true
                    panel.canChooseFiles = false
                    panel.title = "Select Git Repository"
                    
                    if panel.runModal() == .OK {
                        if let url = panel.url {
                            selectedURL = url
                            gitManager.openRepository(path: url.path)
                            UserDefaults.standard.set(url.path, forKey: "lastRepositoryPath")
                            dismiss()
                        }
                    }
                }
                .buttonStyle(.glassProminent)
                .controlSize(.large)
                
                Text("Or clone a repository")
                    .foregroundColor(.secondary)
                
                Button("Clone Repository") {
                    // TODO: Implement clone dialog
                }
                .buttonStyle(.glass)
            }
            .padding()
            .frame(width: 400, height: 300)
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") {
                        dismiss()
                    }
                }
            }
        }
    }
}

#Preview {
    NavigationSplitView {
        SidebarView(selection: .constant(.changes), gitManager: GitManagerWrapper())
    } detail: {
        Text("Detail View")
    }
}