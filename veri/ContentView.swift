import SwiftUI

struct ContentView: View {
    @EnvironmentObject private var viewModel: BLEViewModel

    var body: some View {
        VStack(spacing: 16) {
            Text("Status:")
                .font(.headline)
            Text(viewModel.statusText)
                .multilineTextAlignment(.center)
                .padding()

            if !viewModel.lastDescription.isEmpty {
                Text("Última detecção:")
                    .font(.subheadline)
                Text(viewModel.lastDescription)
                    .bold()
                    .multilineTextAlignment(.center)
                Text(String(format: "Latência: %.0f ms", viewModel.latencyMs))
                    .font(.caption)
            }

            Button("Conectar") {
                viewModel.connect()
            }
            .buttonStyle(.borderedProminent)
            .padding(.top, 24)
        }
        .padding()
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView().environmentObject(BLEViewModel())
    }
}
