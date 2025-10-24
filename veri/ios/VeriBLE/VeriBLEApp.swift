import SwiftUI

@main
struct VeriBLEApp: App {
    @StateObject private var viewModel = BLEViewModel()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(viewModel)
        }
    }
}
