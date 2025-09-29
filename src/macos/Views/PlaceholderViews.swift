// Placeholder views for remaining features

import SwiftUI

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
    StashesView(gitManager: GitManagerWrapper())
}