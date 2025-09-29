import SwiftUI

struct RemotesView: View {
    @EnvironmentObject var gitManager: GitManagerWrapper
    @State private var remotes: [(name: String, url: String)] = []
    @State private var showAddRemote = false
    @State private var newRemoteName = ""
    @State private var newRemoteURL = ""
    @State private var selectedRemote: String?
    @State private var showError = false
    @State private var errorMessage = ""
    @State private var isLoading = false

    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Label("Remote Repositories", systemImage: "network")
                    .font(.headline)

                Spacer()

                Button(action: { showAddRemote.toggle() }) {
                    Image(systemName: "plus.circle")
                }
                .buttonStyle(.plain)

                Button(action: loadRemotes) {
                    Image(systemName: "arrow.clockwise")
                }
                .buttonStyle(.plain)
            }
            .padding()
            .background(Color(NSColor.controlBackgroundColor))

            Divider()

            // Remotes list
            if isLoading {
                Spacer()
                ProgressView("Loading remotes...")
                    .progressViewStyle(CircularProgressViewStyle())
                Spacer()
            } else if remotes.isEmpty {
                Spacer()
                VStack(spacing: 12) {
                    Image(systemName: "network.slash")
                        .font(.system(size: 48))
                        .foregroundColor(.secondary)

                    Text("No Remote Repositories")
                        .font(.title2)

                    Text("Add a remote repository to push and pull changes")
                        .foregroundColor(.secondary)

                    Button("Add Remote") {
                        showAddRemote = true
                    }
                    .buttonStyle(.borderedProminent)
                }
                Spacer()
            } else {
                List(selection: $selectedRemote) {
                    ForEach(remotes, id: \.name) { remote in
                        RemoteItemView(
                            name: remote.name,
                            url: remote.url,
                            isSelected: selectedRemote == remote.name,
                            onFetch: { fetchFromRemote(remote.name) },
                            onPush: { pushToRemote(remote.name) },
                            onRemove: { removeRemote(remote.name) }
                        )
                        .tag(remote.name)
                    }
                }
                .listStyle(.inset)
            }

            // Bottom toolbar
            if !remotes.isEmpty && selectedRemote != nil {
                Divider()

                HStack {
                    Button(action: {
                        if let remote = selectedRemote {
                            fetchFromRemote(remote)
                        }
                    }) {
                        Label("Fetch", systemImage: "arrow.down.circle")
                    }
                    .buttonStyle(.borderless)

                    Button(action: {
                        if let remote = selectedRemote {
                            pushToRemote(remote)
                        }
                    }) {
                        Label("Push", systemImage: "arrow.up.circle")
                    }
                    .buttonStyle(.borderless)

                    Spacer()

                    Button(action: {
                        if let remote = selectedRemote {
                            removeRemote(remote)
                        }
                    }) {
                        Label("Remove", systemImage: "trash")
                    }
                    .buttonStyle(.borderless)
                    .foregroundColor(.red)
                }
                .padding()
                .background(Color(NSColor.controlBackgroundColor))
            }
        }
        .sheet(isPresented: $showAddRemote) {
            AddRemoteSheet(
                remoteName: $newRemoteName,
                remoteURL: $newRemoteURL,
                onAdd: addRemote,
                onCancel: {
                    showAddRemote = false
                    newRemoteName = ""
                    newRemoteURL = ""
                }
            )
        }
        .alert("Error", isPresented: $showError) {
            Button("OK") {
                showError = false
            }
        } message: {
            Text(errorMessage)
        }
        .onAppear {
            loadRemotes()
        }
    }

    private func loadRemotes() {
        isLoading = true

        Task {
            do {
                // Execute git remote -v command
                let output = try await gitManager.executeCommand("git remote -v")

                await MainActor.run {
                    // Parse the output
                    var remotesDict: [String: String] = [:]
                    let lines = output.components(separatedBy: .newlines)

                    for line in lines {
                        let components = line.components(separatedBy: .whitespaces)
                            .filter { !$0.isEmpty }

                        if components.count >= 2 {
                            let name = components[0]
                            let url = components[1]
                            // Only add fetch URLs (ignore push URLs to avoid duplicates)
                            if components.count < 3 || components[2] == "(fetch)" {
                                remotesDict[name] = url
                            }
                        }
                    }

                    remotes = remotesDict.map { (name: $0.key, url: $0.value) }
                        .sorted { $0.name < $1.name }

                    isLoading = false
                }
            } catch {
                await MainActor.run {
                    errorMessage = "Failed to load remotes: \(error.localizedDescription)"
                    showError = true
                    isLoading = false
                }
            }
        }
    }

    private func addRemote() {
        guard !newRemoteName.isEmpty && !newRemoteURL.isEmpty else {
            errorMessage = "Please provide both name and URL"
            showError = true
            return
        }

        Task {
            do {
                _ = try await gitManager.executeCommand("git remote add \(newRemoteName) \(newRemoteURL)")

                await MainActor.run {
                    showAddRemote = false
                    newRemoteName = ""
                    newRemoteURL = ""
                    loadRemotes()
                }
            } catch {
                await MainActor.run {
                    errorMessage = "Failed to add remote: \(error.localizedDescription)"
                    showError = true
                }
            }
        }
    }

    private func removeRemote(_ name: String) {
        Task {
            do {
                _ = try await gitManager.executeCommand("git remote remove \(name)")

                await MainActor.run {
                    if selectedRemote == name {
                        selectedRemote = nil
                    }
                    loadRemotes()
                }
            } catch {
                await MainActor.run {
                    errorMessage = "Failed to remove remote: \(error.localizedDescription)"
                    showError = true
                }
            }
        }
    }

    private func fetchFromRemote(_ name: String) {
        Task {
            do {
                _ = try await gitManager.executeCommand("git fetch \(name)")

                await MainActor.run {
                    // Optionally show success message
                }
            } catch {
                await MainActor.run {
                    errorMessage = "Failed to fetch from \(name): \(error.localizedDescription)"
                    showError = true
                }
            }
        }
    }

    private func pushToRemote(_ name: String) {
        Task {
            do {
                let currentBranch = try await gitManager.executeCommand("git branch --show-current")
                    .trimmingCharacters(in: .whitespacesAndNewlines)

                _ = try await gitManager.executeCommand("git push \(name) \(currentBranch)")

                await MainActor.run {
                    // Optionally show success message
                }
            } catch {
                await MainActor.run {
                    errorMessage = "Failed to push to \(name): \(error.localizedDescription)"
                    showError = true
                }
            }
        }
    }
}

struct RemoteItemView: View {
    let name: String
    let url: String
    let isSelected: Bool
    let onFetch: () -> Void
    let onPush: () -> Void
    let onRemove: () -> Void

    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                Text(name)
                    .font(.system(.body, design: .monospaced))
                    .fontWeight(isSelected ? .semibold : .regular)

                Text(url)
                    .font(.caption)
                    .foregroundColor(.secondary)
                    .lineLimit(1)
                    .truncationMode(.middle)
            }

            Spacer()

            HStack(spacing: 8) {
                Button(action: onFetch) {
                    Image(systemName: "arrow.down.circle")
                        .foregroundColor(.blue)
                }
                .buttonStyle(.plain)
                .help("Fetch from remote")

                Button(action: onPush) {
                    Image(systemName: "arrow.up.circle")
                        .foregroundColor(.green)
                }
                .buttonStyle(.plain)
                .help("Push to remote")

                Button(action: onRemove) {
                    Image(systemName: "xmark.circle")
                        .foregroundColor(.red)
                }
                .buttonStyle(.plain)
                .help("Remove remote")
            }
        }
        .padding(.vertical, 8)
        .padding(.horizontal, 12)
        .background(
            RoundedRectangle(cornerRadius: 6)
                .fill(isSelected ? Color.accentColor.opacity(0.1) : Color.clear)
        )
    }
}

struct AddRemoteSheet: View {
    @Binding var remoteName: String
    @Binding var remoteURL: String
    let onAdd: () -> Void
    let onCancel: () -> Void

    var body: some View {
        VStack(spacing: 20) {
            Text("Add Remote Repository")
                .font(.title2)
                .fontWeight(.semibold)

            VStack(alignment: .leading, spacing: 12) {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Remote Name")
                        .font(.caption)
                        .foregroundColor(.secondary)

                    TextField("origin", text: $remoteName)
                        .textFieldStyle(.roundedBorder)
                }

                VStack(alignment: .leading, spacing: 4) {
                    Text("Repository URL")
                        .font(.caption)
                        .foregroundColor(.secondary)

                    TextField("https://github.com/user/repo.git", text: $remoteURL)
                        .textFieldStyle(.roundedBorder)
                }
            }

            HStack(spacing: 12) {
                Button("Cancel") {
                    onCancel()
                }
                .keyboardShortcut(.escape)

                Button("Add Remote") {
                    onAdd()
                }
                .keyboardShortcut(.defaultAction)
                .disabled(remoteName.isEmpty || remoteURL.isEmpty)
            }
        }
        .padding(24)
        .frame(width: 400)
    }
}

#Preview {
    RemotesView()
        .environmentObject(GitManagerWrapper())
        .frame(width: 600, height: 400)
}