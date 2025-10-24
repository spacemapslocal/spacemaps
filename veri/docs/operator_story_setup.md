# Configuração do Fluxo Operador (Streaming Bluetooth SPP)

## Visão Geral
- **Objetivo:** garantir resposta descritiva ≤ 2 s usando ESP32-CAM + aplicativo Android com inferência offline.
- **Componentes:**
  - Firmware `firmware/esp32cam_bt_stream.ino`.
  - App Android (`mobile/app/src/...`) com `BluetoothStreamClient`.
  - Modelo TFLite quantizado `ssd_mobilenet_v1.tflite` em `assets`.
- **Limitação para iOS:** Bluetooth clássico (SPP) não é suportado nativamente em iPhone sem programa MFi. Para testes no iPhone, usar a alternativa documentada em `docs/operator_story_ios.md`, que emprega BLE ou Wi-Fi.

## Passos de Build
1. **Firmware**
   - Abra `firmware/esp32cam_bt_stream.ino` na Arduino IDE.
   - Selecione placa `AI Thinker ESP32-CAM`, PSRAM habilitada e baud 115200.
   - Carregue o sketch; reinicie para iniciar o serviço SPP (`ESP32CAM-Detector`).

2. **Aplicativo Android**
   - Copie `ssd_mobilenet_v1.tflite` e `labelmap.txt` para `mobile/app/src/main/assets/`.
   - No `build.gradle`, adicione:
     ```gradle
     implementation "org.tensorflow:tensorflow-lite-task-vision:0.4.4"
     ```
   - Construa e instale o app (`MainActivity`).

## Execução do Cenário
1. Emparelhe o ESP32-CAM com o Android (Bluetooth clássico).
2. Abra o app e toque em “Conectar ESP32-CAM”.
3. O app envia `HANDSHAKE_MAGIC` (0xCAFEBABE) e aguarda ACK (0xABCD4321). O firmware confirma em serial e começa a transmitir frames QQVGA.
4. A cada frame:
   - Firmware envia header + JPEG + CRC16.
   - App valida, roda inferência e envia descrição via SPP (termina com `\n`).
   - Firmware exibe descrição em `Serial`.

## Verificação dos Critérios
- **Conexão ≤ 5 s:** `BluetoothStreamClient` tenta conectar com backoff exponencial até 10 s e `esp32cam_bt_stream.ino` reinicia serviço SPP a cada 10 s sem cliente.
- **Latência ≤ 2 s:** `measureTimeMillis` no app registra e gera log sempre que limite for excedido; durante testes com QQVGA/compressão alta, latência média esperada ≈ 400–700 ms.
- **Reconexão ≤ 10 s:** caso o socket caia, o firmware reseta o serviço; o app cancela `readerJob` e reexecuta `connect()` após 1–10 s (backoff limitado), garantindo nova tentativa automática.

## Dicas de Ajuste
- Ajuste `kCaptureIntervalMs` no firmware caso o celular suporte mais FPS.
- Personalize `descriptionMap` e `setScoreThreshold()` conforme objetos de interesse.
- Utilize monitor serial para confirmar heartbeat (`HB`) e mensagens de descrição.

## Logs Importantes
- Firmware:
  - `[INF] Waiting for Bluetooth client...`
  - `[INF] Handshake completed`
  - `[DBG] Frame XXX bytes sent`
- App:
  - `Detection LABEL % latency=XXms`
  - Alertas quando a latência ultrapassar SLA ou CRC falhar.
