import SwiftUI

struct DiffView: View {
    let file: GitFileChangeWrapper
    let commitHash: String
    @ObservedObject var gitManager: GitManagerWrapper
    @State private var diffContent: GitDiffWrapper?
    @State private var showingLineNumbers = true
    @State private var wrapLines = false
    @State private var selectedHunk: Int?
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                // File info
                HStack {
                    StatusIndicatorView(status: file.status)
                    
                    VStack(alignment: .leading, spacing: 2) {
                        Text(file.fileName)
                            .font(.headline)
                            .lineLimit(1)
                        
                        if !file.directoryPath.isEmpty {
                            Text(file.directoryPath)
                                .font(.caption)
                                .foregroundColor(.secondary)
                        }
                    }
                }
                
                Spacer()
                
                // View options
                HStack {
                    Toggle("Line Numbers", isOn: $showingLineNumbers)
                        .toggleStyle(.button)
                        .controlSize(.small)
                    
                    Toggle("Wrap Lines", isOn: $wrapLines)
                        .toggleStyle(.button)
                        .controlSize(.small)
                    
                    Button("Copy Diff") {
                        if let diff = diffContent {
                            NSPasteboard.general.clearContents()
                            NSPasteboard.general.setString(diff.rawContent, forType: .string)
                        }
                    }
                    .buttonStyle(.glass)
                    .controlSize(.small)
                }
            }
            .padding()
            .background(Color(NSColor.controlBackgroundColor).opacity(0.5))
            
            Divider()
            
            // Diff Content
            if let diff = diffContent {
                if diff.isBinary {
                    BinaryFileView(fileName: file.fileName)
                } else {
                    DiffContentView(
                        diff: diff,
                        showLineNumbers: showingLineNumbers,
                        wrapLines: wrapLines,
                        selectedHunk: $selectedHunk
                    )
                }
            } else {
                LoadingDiffView()
            }
        }
        .background(Color.clear)
        .onAppear {
            loadDiff()
        }
        .onChange(of: file.filePath) {
            loadDiff()
        }
        .onChange(of: commitHash) {
            loadDiff()
        }
    }
    
    private func loadDiff() {
        gitManager.loadFileDiff(filePath: file.filePath, commitHash: commitHash) { diff in
            diffContent = diff
        }
    }
}

struct DiffContentView: View {
    let diff: GitDiffWrapper
    let showLineNumbers: Bool
    let wrapLines: Bool
    @Binding var selectedHunk: Int?
    
    var body: some View {
        ScrollView([.horizontal, .vertical]) {
            LazyVStack(alignment: .leading, spacing: 0) {
                ForEach(Array(diff.hunks.enumerated()), id: \.offset) { hunkIndex, hunk in
                    DiffHunkView(
                        hunk: hunk,
                        hunkIndex: hunkIndex,
                        showLineNumbers: showLineNumbers,
                        wrapLines: wrapLines,
                        isSelected: selectedHunk == hunkIndex,
                        onSelect: { selectedHunk = hunkIndex }
                    )
                }
            }
            .padding()
        }
        .background(Color(NSColor.textBackgroundColor))
        .font(.system(.body, design: .monospaced))
    }
}

struct DiffHunkView: View {
    let hunk: GitDiffHunkWrapper
    let hunkIndex: Int
    let showLineNumbers: Bool
    let wrapLines: Bool
    let isSelected: Bool
    let onSelect: () -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Hunk header
            HStack {
                Text(hunk.header)
                    .font(.caption)
                    .fontWeight(.semibold)
                    .foregroundColor(.white)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(Color.blue, in: RoundedRectangle(cornerRadius: 4))
                
                Spacer()
                
                Text("@@ -\(hunk.oldStart),\(hunk.oldCount) +\(hunk.newStart),\(hunk.newCount) @@")
                    .font(.caption)
                    .font(.system(.body, design: .monospaced))
                    .foregroundColor(.secondary)
            }
            .padding(.vertical, 8)
            .background(isSelected ? Color.blue.opacity(0.1) : Color.clear)
            .contentShape(Rectangle())
            .onTapGesture {
                onSelect()
            }
            
            // Hunk lines
            ForEach(Array(hunk.lines.enumerated()), id: \.offset) { lineIndex, line in
                DiffLineView(
                    line: line,
                    showLineNumbers: showLineNumbers,
                    wrapLines: wrapLines
                )
            }
        }
        .padding(.bottom, 16)
    }
}

struct DiffLineView: View {
    let line: GitDiffLineWrapper
    let showLineNumbers: Bool
    let wrapLines: Bool
    
    var body: some View {
        HStack(alignment: .top, spacing: 0) {
            // Line numbers
            if showLineNumbers {
                HStack(spacing: 4) {
                    Text(line.oldLineNumber >= 0 ? "\(line.oldLineNumber)" : "")
                        .frame(width: 40, alignment: .trailing)
                        .foregroundColor(.secondary)
                    
                    Text(line.newLineNumber >= 0 ? "\(line.newLineNumber)" : "")
                        .frame(width: 40, alignment: .trailing)
                        .foregroundColor(.secondary)
                }
                .font(.caption)
                .font(.system(.body, design: .monospaced))
                .padding(.trailing, 8)
            }
            
            // Line content
            HStack(alignment: .top, spacing: 4) {
                // Change indicator
                Text(linePrefix)
                    .foregroundColor(lineColor)
                    .frame(width: 12, alignment: .leading)
                
                // Content
                Text(line.content)
                    .foregroundColor(lineColor)
                    .frame(maxWidth: wrapLines ? .infinity : nil, alignment: .leading)
                    .lineLimit(wrapLines ? nil : 1)
                    .textSelection(.enabled)
            }
        }
        .padding(.vertical, 1)
        .background(lineBackgroundColor)
        .frame(maxWidth: .infinity, alignment: .leading)
    }
    
    private var linePrefix: String {
        switch line.type {
        case .addition:
            return "+"
        case .deletion:
            return "-"
        case .context:
            return " "
        case .header:
            return "@"
        }
    }
    
    private var lineColor: Color {
        switch line.type {
        case .addition:
            return .green
        case .deletion:
            return .red
        case .context:
            return .primary
        case .header:
            return .blue
        }
    }
    
    private var lineBackgroundColor: Color {
        switch line.type {
        case .addition:
            return Color.green.opacity(0.1)
        case .deletion:
            return Color.red.opacity(0.1)
        case .context:
            return Color.clear
        case .header:
            return Color.blue.opacity(0.1)
        }
    }
}

struct BinaryFileView: View {
    let fileName: String
    
    var body: some View {
        VStack(spacing: 16) {
            Image(systemName: "doc.fill")
                .font(.system(size: 64))
                .foregroundColor(.secondary)
            
            VStack(spacing: 4) {
                Text("Binary File")
                    .font(.headline)
                
                Text(fileName)
                    .font(.body)
                    .foregroundColor(.secondary)
                
                Text("Binary files cannot be displayed as text")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}

struct LoadingDiffView: View {
    var body: some View {
        VStack(spacing: 16) {
            ProgressView()
                .scaleEffect(1.2)
            
            Text("Loading diff...")
                .font(.body)
                .foregroundColor(.secondary)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}

#Preview {
    DiffView(
        file: GitFileChangeWrapper.preview,
        commitHash: "abc123",
        gitManager: GitManagerWrapper()
    )
}