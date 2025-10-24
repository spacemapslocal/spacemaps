# Fluxo Operador — Versão iOS (BLE)

## Restrições de Plataforma
- iOS não expõe Bluetooth Clássico (SPP) para aplicativos comuns. Para contornar, o ESP32 precisa anunciar um serviço BLE personalizado e fragmentar as imagens em notificações.
- Latência alvo continua ≤ 2 s, compensada por:
  - Resolução QQVGA.
  - Chunking em blocos de 244 bytes úteis por notificação.
  - Bufferização mínima no app iOS antes da inferência.

## Componentes
- Firmware BLE: `firmware/esp32cam_ble_stream.ino`.
- App iOS (SwiftUI): `ios/VeriBLE/`.
- Modelo Core ML (`SSDModel.mlmodelc`) convertido a partir do MobileNet quantizado (ver instruções abaixo).

## Preparação do Modelo
1. Converta o TFLite usado no Android para Core ML:
   ```bash
   python -m pip install coremltools==7.0
   python ios/VeriBLE/scripts/convert_tflite_to_coreml.py \
       --tflite path/para/ssd_mobilenet_v1.tflite \
       --labels path/para/labelmap.txt \
       --output ios/VeriBLE/Model/SSDModel.mlpackage
   ```
2. Arraste `SSDModel.mlpackage` para o projeto Xcode (Build Phases ➜ Copy Bundle Resources).
3. Atualize `Info.plist` com a chave `Privacy - Bluetooth Always Usage Description`.

## Fluxo
1. ESP32 inicializa NimBLE, expõe serviço `0xFFE0` com características:
   - `0xFFE1` (Notify) para envio de frames fragmentados.
   - `0xFFE2` (Write Without Response) para comandos (ex.: ACK, ajuste de ROI).
2. App iOS descobre o serviço, envia um comando de handshake (`0xCA FE BA BE`) e começa a montar os fragmentos.
3. Quando um frame completo chega, o app reconstrói o JPEG, roda inferência local e exibe descrição em tela.
4. Caso a conexão caia, o ESP32 volta ao modo advertising e o app reinicia o scan automaticamente.

## Build do App iOS
1. Abra `ios/VeriBLE/VeriBLE.xcodeproj` (crie o projeto pelo Xcode e adicione os arquivos Swift fornecidos).
2. Adicione o package `TensorFlowLiteTaskVision` ou use o modelo Core ML convertido.
3. Conecte o iPhone via cabo e selecione como destino.
4. Compile e instale (⌘+R).

## Cuidados
- Definir MTU máxima (até 517 bytes no ESP32); o firmware usa blocos de 244 bytes para compatibilidade ampla.
- Se a latência exceder 2 s, reduza intervalo de captura ou aumente compressão.
- BLE consome menos energia, mas throughput cai se o celular estiver distante (> 3 m).
- A inferência Core ML roda na CPU; habilite `MLComputeUnits.all` para usar Neural Engine em iPhones suportados.
