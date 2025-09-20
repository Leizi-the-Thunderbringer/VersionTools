import SwiftUI

@main
struct VersionToolsApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
                .preferredColorScheme(.none) // Support system theme
        }
        .windowStyle(.titleBar)
        .windowToolbarStyle(.unified)
    }
}