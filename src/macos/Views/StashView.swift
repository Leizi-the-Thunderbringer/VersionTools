import SwiftUI

struct StashView: View {
    @ObservedObject var gitManager: GitManagerWrapper
    @State private var selectedStash: GitStashWrapper?
    @State private var showingNewStashDialog = false
    @State private var searchText = ""

    var filteredStashes: [GitStashWrapper] {
        if searchText.isEmpty {
            return gitManager.stashes
        }
        return gitManager.stashes.filter { stash in
            stash.message.localizedCaseInsensitiveContains(searchText) ||
            (stash.branch?.localizedCaseInsensitiveContains(searchText) ?? false)
        }
    }

    var body: some View {
        VStack(spacing: 0) {
            // 头部工具栏
            VStack(spacing: 12) {
                // 搜索栏
                HStack {
                    Image(systemName: "magnifyingglass")
                        .foregroundColor(.secondary)

                    TextField("搜索储藏...", text: $searchText)
                        .textFieldStyle(.plain)
                }
                .padding(.horizontal, 8)
                .padding(.vertical, 6)
                .background(Color.gray.opacity(0.1), in: RoundedRectangle(cornerRadius: 6))

                // 操作按钮
                HStack {
                    Button("新建储藏") {
                        showingNewStashDialog = true
                    }
                    .buttonStyle(.glass)
                    .disabled(!(gitManager.repositoryStatus?.hasUncommittedChanges ?? false))

                    Spacer()

                    Text("\(gitManager.stashes.count) 个储藏")
                        .font(.caption)
                        .foregroundColor(.secondary)

                    Button("刷新") {
                        gitManager.loadStashes()
                    }
                    .buttonStyle(.glass)
                }
            }
            .padding()

            Divider()

            // Stash 列表
            if filteredStashes.isEmpty {
                EmptyStashView()
                    .frame(maxHeight: .infinity)
            } else {
                List(filteredStashes, id: \.id, selection: $selectedStash) { stash in
                    StashRowView(
                        stash: stash,
                        isSelected: selectedStash?.id == stash.id,
                        onApply: { applyStash(stash) },
                        onPop: { popStash(stash) },
                        onDrop: { dropStash(stash) }
                    )
                    .tag(stash)
                }
                .listStyle(.sidebar)
            }
        }
        .navigationTitle("储藏")
        .sheet(isPresented: $showingNewStashDialog) {
            NewStashDialogView(gitManager: gitManager)
        }
        .onAppear {
            gitManager.loadStashes()
        }
        .refreshable {
            gitManager.loadStashes()
        }
    }

    private func applyStash(_ stash: GitStashWrapper) {
        gitManager.applyStash(index: stash.index) { success in
            if success {
                // 可以显示成功提示
            }
        }
    }

    private func popStash(_ stash: GitStashWrapper) {
        gitManager.popStash(index: stash.index) { success in
            if success {
                if selectedStash?.id == stash.id {
                    selectedStash = nil
                }
            }
        }
    }

    private func dropStash(_ stash: GitStashWrapper) {
        gitManager.dropStash(index: stash.index) { success in
            if success {
                if selectedStash?.id == stash.id {
                    selectedStash = nil
                }
            }
        }
    }
}

struct StashRowView: View {
    let stash: GitStashWrapper
    let isSelected: Bool
    let onApply: () -> Void
    let onPop: () -> Void
    let onDrop: () -> Void

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // 主要信息
            HStack {
                Image(systemName: "archivebox")
                    .foregroundColor(.orange)

                VStack(alignment: .leading, spacing: 4) {
                    Text(stash.message)
                        .font(.body)
                        .lineLimit(2)

                    HStack {
                        if let branch = stash.branch {
                            Label(branch, systemImage: "arrow.triangle.branch")
                                .font(.caption)
                                .foregroundColor(.secondary)
                        }

                        Text(stash.relativeTime)
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                }

                Spacer()

                Text("stash@{\(stash.index)}")
                    .font(.caption)
                    .font(.system(.body, design: .monospaced))
                    .foregroundColor(.blue)
            }
        }
        .padding(.vertical, 8)
        .padding(.horizontal, 4)
        .background(isSelected ? Color.blue.opacity(0.1) : Color.clear)
        .cornerRadius(6)
        .contextMenu {
            StashContextMenu(
                stash: stash,
                onApply: onApply,
                onPop: onPop,
                onDrop: onDrop
            )
        }
    }
}

struct StashContextMenu: View {
    let stash: GitStashWrapper
    let onApply: () -> Void
    let onPop: () -> Void
    let onDrop: () -> Void

    var body: some View {
        Group {
            Button("应用储藏") {
                onApply()
            }

            Button("弹出储藏（应用并删除）") {
                onPop()
            }

            Divider()

            Button("复制消息") {
                NSPasteboard.general.clearContents()
                NSPasteboard.general.setString(stash.message, forType: .string)
            }

            Divider()

            Button("删除储藏") {
                onDrop()
            }
            .foregroundColor(.red)
        }
    }
}

struct EmptyStashView: View {
    var body: some View {
        VStack(spacing: 16) {
            Image(systemName: "archivebox")
                .font(.system(size: 48))
                .foregroundColor(.secondary)

            Text("没有储藏")
                .font(.headline)

            Text("当你有未提交的更改时，可以创建储藏来暂时保存这些更改")
                .font(.body)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
                .frame(maxWidth: 300)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}

struct NewStashDialogView: View {
    @ObservedObject var gitManager: GitManagerWrapper
    @State private var message: String = ""
    @State private var includeUntracked = false
    @Environment(\.dismiss) private var dismiss

    var body: some View {
        VStack(spacing: 20) {
            Text("创建储藏")
                .font(.headline)

            VStack(alignment: .leading, spacing: 8) {
                Text("储藏消息：")
                    .font(.caption)
                    .foregroundColor(.secondary)

                TextField("输入储藏描述...", text: $message)
                    .textFieldStyle(.roundedBorder)
            }

            Toggle("包含未跟踪的文件", isOn: $includeUntracked)

            HStack {
                Button("取消") {
                    dismiss()
                }
                .buttonStyle(.glass)

                Spacer()

                Button("创建") {
                    createStash()
                }
                .buttonStyle(.glassProminent)
                .disabled(message.isEmpty)
            }
        }
        .padding()
        .frame(width: 400)
    }

    private func createStash() {
        let finalMessage = message.isEmpty ? "WIP on \(gitManager.currentBranch ?? "unknown")" : message

        gitManager.createStash(message: finalMessage) { success in
            if success {
                dismiss()
            }
        }
    }
}

#Preview {
    StashView(gitManager: GitManagerWrapper())
}