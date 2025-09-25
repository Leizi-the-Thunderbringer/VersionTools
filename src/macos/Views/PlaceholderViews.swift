// Placeholder views for remaining features

import SwiftUI

struct RemotesView: View {
    @ObservedObject var gitManager: GitManagerWrapper
    
    var body: some View {
        VStack(spacing: 20) {
            Image(systemName: "globe")
                .font(.system(size: 48))
                .foregroundColor(.secondary)
            
            Text("Remotes")
                .font(.headline)
            
            Text("Remote repository management coming soon")
                .font(.body)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .navigationTitle("Remotes")
    }
}

struct TagsView: View {
    @ObservedObject var gitManager: GitManagerWrapper
    
    var body: some View {
        VStack(spacing: 20) {
            Image(systemName: "tag.fill")
                .font(.system(size: 48))
                .foregroundColor(.secondary)
            
            Text("Tags")
                .font(.headline)
            
            Text("Tag management coming soon")
                .font(.body)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .navigationTitle("Tags")
    }
}

struct StashesView: View {
    @ObservedObject var gitManager: GitManagerWrapper
    
    var body: some View {
        VStack(spacing: 20) {
            Image(systemName: "archivebox.fill")
                .font(.system(size: 48))
                .foregroundColor(.secondary)
            
            Text("Stashes")
                .font(.headline)
            
            Text("Stash management coming soon")
                .font(.body)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .navigationTitle("Stashes")
    }
}

struct SettingsView: View {
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        NavigationStack {
            VStack(spacing: 20) {
                Text("Settings")
                    .font(.largeTitle)
                    .fontWeight(.semibold)
                
                Text("Application settings coming soon")
                    .font(.body)
                    .foregroundColor(.secondary)
                
                Spacer()
            }
            .padding()
            .frame(width: 500, height: 400)
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button("Done") {
                        dismiss()
                    }
                    .buttonStyle(.glassProminent)
                }
            }
        }
    }
}

#Preview {
    RemotesView(gitManager: GitManagerWrapper())
}