import SwiftUI

struct ContentView: View {
    @StateObject private var gitManager = GitManagerWrapper()
    @State private var selectedSidebarItem: SidebarItem? = .changes
    @State private var showingSettings = false
    
    var body: some View {
        NavigationSplitView {
            SidebarView(selection: $selectedSidebarItem, gitManager: gitManager)
                .navigationSplitViewColumnWidth(min: 200, ideal: 250, max: 300)
        } detail: {
            Group {
                switch selectedSidebarItem {
                case .changes:
                    ChangesView(gitManager: gitManager)
                case .history:
                    HistoryView(gitManager: gitManager)
                case .branches:
                    BranchesView(gitManager: gitManager)
                case .remotes:
                    RemotesView(gitManager: gitManager)
                case .tags:
                    TagsView(gitManager: gitManager)
                case .stashes:
                    StashesView(gitManager: gitManager)
                case .none:
                    WelcomeView()
                }
            }
            .navigationTitle(selectedSidebarItem?.title ?? "Version Tools")
            .toolbar {
                ToolbarItemGroup(placement: .primaryAction) {
                    Button("Settings") {
                        showingSettings = true
                    }
                    .buttonStyle(.glass)
                }
            }
        }
        .sheet(isPresented: $showingSettings) {
            SettingsView()
        }
        .background(Color.clear) // Let Liquid Glass show through
        .onAppear {
            // Load repository if available
            if let repoPath = UserDefaults.standard.string(forKey: "lastRepositoryPath") {
                gitManager.openRepository(path: repoPath)
            }
        }
    }
}

enum SidebarItem: String, CaseIterable, Identifiable {
    case changes = "Changes"
    case history = "History"
    case branches = "Branches"
    case remotes = "Remotes"
    case tags = "Tags"
    case stashes = "Stashes"
    
    var id: String { rawValue }
    
    var title: String {
        return rawValue
    }
    
    var systemImage: String {
        switch self {
        case .changes:
            return "doc.text.fill"
        case .history:
            return "clock.fill"
        case .branches:
            return "arrow.triangle.branch"
        case .remotes:
            return "globe"
        case .tags:
            return "tag.fill"
        case .stashes:
            return "archivebox.fill"
        }
    }
}

struct WelcomeView: View {
    var body: some View {
        VStack(spacing: 20) {
            Image(systemName: "folder.badge.gearshape")
                .font(.system(size: 64))
                .foregroundColor(.secondary)
            
            Text("Welcome to Version Tools")
                .font(.largeTitle)
                .fontWeight(.semibold)
            
            Text("Open a Git repository to get started")
                .font(.body)
                .foregroundColor(.secondary)
            
            Button("Open Repository") {
                // TODO: Implement repository selection
            }
            .buttonStyle(.glassProminent)
            .controlSize(.large)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color.clear)
    }
}

#Preview {
    ContentView()
}