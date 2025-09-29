import SwiftUI

struct TagsView: View {
    @EnvironmentObject var gitManager: GitManagerWrapper
    @State private var tags: [TagInfo] = []
    @State private var showCreateTag = false
    @State private var newTagName = ""
    @State private var newTagMessage = ""
    @State private var selectedTag: String?
    @State private var showError = false
    @State private var errorMessage = ""
    @State private var isLoading = false
    @State private var tagType: TagType = .lightweight

    enum TagType: String, CaseIterable {
        case lightweight = "Lightweight"
        case annotated = "Annotated"
    }

    struct TagInfo: Identifiable {
        let id = UUID()
        let name: String
        let commit: String
        let date: String?
        let message: String?
    }

    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Label("Tags", systemImage: "tag")
                    .font(.headline)

                Spacer()

                Button(action: { showCreateTag.toggle() }) {
                    Image(systemName: "plus.circle")
                }
                .buttonStyle(.plain)

                Button(action: loadTags) {
                    Image(systemName: "arrow.clockwise")
                }
                .buttonStyle(.plain)
            }
            .padding()
            .background(Color(NSColor.controlBackgroundColor))

            Divider()

            // Tags list
            if isLoading {
                Spacer()
                ProgressView("Loading tags...")
                    .progressViewStyle(CircularProgressViewStyle())
                Spacer()
            } else if tags.isEmpty {
                Spacer()
                VStack(spacing: 12) {
                    Image(systemName: "tag.slash")
                        .font(.system(size: 48))
                        .foregroundColor(.secondary)

                    Text("No Tags")
                        .font(.title2)

                    Text("Create tags to mark important commits")
                        .foregroundColor(.secondary)

                    Button("Create Tag") {
                        showCreateTag = true
                    }
                    .buttonStyle(.borderedProminent)
                }
                Spacer()
            } else {
                List(selection: $selectedTag) {
                    ForEach(tags) { tag in
                        TagItemView(
                            tag: tag,
                            isSelected: selectedTag == tag.name,
                            onCheckout: { checkoutTag(tag.name) },
                            onDelete: { deleteTag(tag.name) },
                            onPush: { pushTag(tag.name) }
                        )
                        .tag(tag.name)
                    }
                }
                .listStyle(.inset)
            }

            // Bottom toolbar
            if !tags.isEmpty && selectedTag != nil {
                Divider()

                HStack {
                    Button(action: {
                        if let tag = selectedTag {
                            checkoutTag(tag)
                        }
                    }) {
                        Label("Checkout", systemImage: "arrow.right.circle")
                    }
                    .buttonStyle(.borderless)

                    Button(action: {
                        if let tag = selectedTag {
                            pushTag(tag)
                        }
                    }) {
                        Label("Push", systemImage: "arrow.up.circle")
                    }
                    .buttonStyle(.borderless)

                    Spacer()

                    Button(action: {
                        if let tag = selectedTag {
                            deleteTag(tag)
                        }
                    }) {
                        Label("Delete", systemImage: "trash")
                    }
                    .buttonStyle(.borderless)
                    .foregroundColor(.red)
                }
                .padding()
                .background(Color(NSColor.controlBackgroundColor))
            }
        }
        .sheet(isPresented: $showCreateTag) {
            CreateTagSheet(
                tagName: $newTagName,
                tagMessage: $newTagMessage,
                tagType: $tagType,
                onCreate: createTag,
                onCancel: {
                    showCreateTag = false
                    newTagName = ""
                    newTagMessage = ""
                    tagType = .lightweight
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
            loadTags()
        }
    }

    private func loadTags() {
        isLoading = true

        Task {
            do {
                // Get all tags with their details
                let output = try await gitManager.executeCommand("git tag -l --format='%(refname:short)|%(objectname:short)|%(creatordate:short)|%(contents:subject)'")

                await MainActor.run {
                    var tagsList: [TagInfo] = []
                    let lines = output.components(separatedBy: .newlines)
                        .filter { !$0.isEmpty }

                    for line in lines {
                        let components = line.components(separatedBy: "|")
                        if components.count >= 1 {
                            let name = components[0]
                            let commit = components.count > 1 ? String(components[1].prefix(7)) : ""
                            let date = components.count > 2 ? components[2] : nil
                            let message = components.count > 3 ? components[3] : nil

                            tagsList.append(TagInfo(
                                name: name,
                                commit: commit,
                                date: date,
                                message: message
                            ))
                        }
                    }

                    tags = tagsList.sorted { $0.name > $1.name }
                    isLoading = false
                }
            } catch {
                await MainActor.run {
                    errorMessage = "Failed to load tags: \(error.localizedDescription)"
                    showError = true
                    isLoading = false
                }
            }
        }
    }

    private func createTag() {
        guard !newTagName.isEmpty else {
            errorMessage = "Please provide a tag name"
            showError = true
            return
        }

        Task {
            do {
                let command: String
                if tagType == .annotated && !newTagMessage.isEmpty {
                    command = "git tag -a \(newTagName) -m '\(newTagMessage)'"
                } else {
                    command = "git tag \(newTagName)"
                }

                _ = try await gitManager.executeCommand(command)

                await MainActor.run {
                    showCreateTag = false
                    newTagName = ""
                    newTagMessage = ""
                    tagType = .lightweight
                    loadTags()
                }
            } catch {
                await MainActor.run {
                    errorMessage = "Failed to create tag: \(error.localizedDescription)"
                    showError = true
                }
            }
        }
    }

    private func deleteTag(_ name: String) {
        Task {
            do {
                _ = try await gitManager.executeCommand("git tag -d \(name)")

                await MainActor.run {
                    if selectedTag == name {
                        selectedTag = nil
                    }
                    loadTags()
                }
            } catch {
                await MainActor.run {
                    errorMessage = "Failed to delete tag: \(error.localizedDescription)"
                    showError = true
                }
            }
        }
    }

    private func checkoutTag(_ name: String) {
        Task {
            do {
                _ = try await gitManager.executeCommand("git checkout \(name)")

                await MainActor.run {
                    // Refresh the UI or show success
                }
            } catch {
                await MainActor.run {
                    errorMessage = "Failed to checkout tag: \(error.localizedDescription)"
                    showError = true
                }
            }
        }
    }

    private func pushTag(_ name: String) {
        Task {
            do {
                // First, get the default remote
                let remoteOutput = try await gitManager.executeCommand("git remote")
                let remotes = remoteOutput.components(separatedBy: .newlines)
                    .filter { !$0.isEmpty }

                let remote = remotes.first ?? "origin"

                _ = try await gitManager.executeCommand("git push \(remote) \(name)")

                await MainActor.run {
                    // Optionally show success message
                }
            } catch {
                await MainActor.run {
                    errorMessage = "Failed to push tag: \(error.localizedDescription)"
                    showError = true
                }
            }
        }
    }
}

struct TagItemView: View {
    let tag: TagsView.TagInfo
    let isSelected: Bool
    let onCheckout: () -> Void
    let onDelete: () -> Void
    let onPush: () -> Void

    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                HStack {
                    Image(systemName: "tag.fill")
                        .foregroundColor(.orange)
                        .font(.system(size: 12))

                    Text(tag.name)
                        .font(.system(.body, design: .monospaced))
                        .fontWeight(isSelected ? .semibold : .regular)
                }

                HStack {
                    Text(tag.commit)
                        .font(.caption)
                        .foregroundColor(.secondary)

                    if let date = tag.date {
                        Text("•")
                            .font(.caption)
                            .foregroundColor(.secondary)

                        Text(date)
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                }

                if let message = tag.message, !message.isEmpty {
                    Text(message)
                        .font(.caption)
                        .foregroundColor(.secondary)
                        .lineLimit(1)
                        .truncationMode(.tail)
                }
            }

            Spacer()

            HStack(spacing: 8) {
                Button(action: onCheckout) {
                    Image(systemName: "arrow.right.circle")
                        .foregroundColor(.blue)
                }
                .buttonStyle(.plain)
                .help("Checkout tag")

                Button(action: onPush) {
                    Image(systemName: "arrow.up.circle")
                        .foregroundColor(.green)
                }
                .buttonStyle(.plain)
                .help("Push tag to remote")

                Button(action: onDelete) {
                    Image(systemName: "xmark.circle")
                        .foregroundColor(.red)
                }
                .buttonStyle(.plain)
                .help("Delete tag")
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

struct CreateTagSheet: View {
    @Binding var tagName: String
    @Binding var tagMessage: String
    @Binding var tagType: TagsView.TagType
    let onCreate: () -> Void
    let onCancel: () -> Void

    var body: some View {
        VStack(spacing: 20) {
            Text("Create Tag")
                .font(.title2)
                .fontWeight(.semibold)

            VStack(alignment: .leading, spacing: 12) {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Tag Name")
                        .font(.caption)
                        .foregroundColor(.secondary)

                    TextField("v1.0.0", text: $tagName)
                        .textFieldStyle(.roundedBorder)
                }

                VStack(alignment: .leading, spacing: 4) {
                    Text("Tag Type")
                        .font(.caption)
                        .foregroundColor(.secondary)

                    Picker("", selection: $tagType) {
                        ForEach(TagsView.TagType.allCases, id: \.self) { type in
                            Text(type.rawValue).tag(type)
                        }
                    }
                    .pickerStyle(.segmented)
                }

                if tagType == .annotated {
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Tag Message")
                            .font(.caption)
                            .foregroundColor(.secondary)

                        TextEditor(text: $tagMessage)
                            .font(.system(.body, design: .monospaced))
                            .frame(minHeight: 60)
                            .overlay(
                                RoundedRectangle(cornerRadius: 6)
                                    .stroke(Color.secondary.opacity(0.2), lineWidth: 1)
                            )
                    }
                }
            }

            HStack(spacing: 12) {
                Button("Cancel") {
                    onCancel()
                }
                .keyboardShortcut(.escape)

                Button("Create Tag") {
                    onCreate()
                }
                .keyboardShortcut(.defaultAction)
                .disabled(tagName.isEmpty)
            }
        }
        .padding(24)
        .frame(width: 400)
    }
}

#Preview {
    TagsView()
        .environmentObject(GitManagerWrapper())
        .frame(width: 600, height: 400)
}