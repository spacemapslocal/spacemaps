# Estratégia de Projeto para Reconhecimento de Objetos

## Frente 1 — Otimização com ESP32-CAM + Bluetooth SPP

- **Objetivo**: manter a arquitetura atual usando ESP32-CAM conectado ao smartphone via Bluetooth Classic SPP, garantindo latência mínima aceitável sem alterar o modelo pré-treinado no celular.
- **Escopo técnico**:
  - Configurar câmera OV2640 para `FRAMESIZE_QQVGA` (160×120) e `jpeg_quality` alto (valor numérico menor).
  - Implementar envio periódico ou sob demanda de JPEG por SPP com controle de fluxo e checksum.
  - Permitir seleção de recorte (ROI) e ajuste da taxa de quadros a partir do app móvel.
  - Aplicativo recebe, roda inferência off-line (TensorFlow Lite Task Library) e devolve descrição textual opcionalmente.

### Histórias de Usuário

1. **Como operador**, quero iniciar o streaming Bluetooth e ver respostas descritivas em até 2 segundos para saber o que o dispositivo está vendo.
   - Critérios de aceitação:
     - Conexão Bluetooth SPP estabelecida em menos de 5 segundos.
     - Tempo entre captura e descrição no celular ≤ 2 segundos para resolução QQVGA.
     - Em caso de perda de conexão, o sistema reconecta automaticamente em até 10 segundos.

2. **Como desenvolvedor mobile**, quero receber frames comprimidos de forma confiável para processar com TFLite sem precisar decodificar formatos proprietários.
   - Critérios de aceitação:
     - Cada frame chega precedido por header com tamanho (4 bytes little-endian) e checksum CRC16.
     - Taxa de perdas menor que 2% em um teste de 5 minutos contínuos.
     - O app rejeita frames corrompidos e solicita reenvio sem travar a UI.

3. **Como analista de produto**, quero poder ajustar ROI e frequência de captura para equilibrar qualidade e velocidade durante testes de campo.
   - Critérios de aceitação:
     - Interface no app com sliders para ROI (mínimo 25% do frame) e intervalo (0,2 a 2 s).
     - Alterações entram em vigor no máximo após o próximo frame.
     - Logs exibem configuração ativa e métricas de throughput.

## Frente 2 — Evolução de Hardware/IA Híbrida

- **Objetivo**: melhorar desempenho explorando hardware mais capaz (ESP32-S3 com USB/PSRAM ampliada) ou distribuir inferência entre MCU e celular para reduzir carga do link.
- **Escopo técnico**:
  - Avaliar ESP32-S3 com suporte USB CDC ou Wi-Fi rápido (OTA local) para streaming de vídeo.
  - Implementar pipeline local de pré-filtragem no MCU (por exemplo, classificador binário para eventos de interesse).
  - Transferir ao celular apenas frames relevantes ou vetores de características compatíveis com modelo pré-treinado selecionado.
  - Suporte para integração futura com display ou feedback háptico.

### Histórias de Usuário

1. **Como engenheiro de hardware**, quero coletar métricas comparativas entre ESP32-CAM e ESP32-S3 para justificar a migração.
   - Critérios de aceitação:
     - Relatório apresenta throughput médio (fps) e latência ponta-a-ponta em três cenários (USB serial, Wi-Fi AP direto, Bluetooth).
     - Medições repetidas 3 vezes com desvio padrão < 10%.
     - Conclusão recomenda opção baseada em dados e custo estimado.

2. **Como desenvolvedor embarcado**, quero executar um classificador binário local que sinalize o celular apenas quando detectar evento relevante.
   - Critérios de aceitação:
     - Modelo TinyML (≤ 250 KB) roda no ESP32 em ≤ 120 ms por frame.
     - Evento positivo gera notificação com timestamp enviada ao celular (USB ou Wi-Fi) e frame JPEG correspondente.
     - Com taxa de eventos baixa (< 10%), uso médio de banda cai pelo menos 60% frente ao streaming contínuo.

3. **Como product owner**, quero garantir que o aplicativo móvel possa alternar entre modo streaming completo e modo notificação otimizada.
   - Critérios de aceitação:
     - App oferece menu para escolher modo "Streaming" ou "Evento".
     - Em modo "Evento", a interface mostra o último frame recebido e descrição do objeto.
     - Mudança de modo ocorre sem reconectar hardware; atraso máximo de 3 segundos.

## Considerações Gerais

- Definir cronograma paralelo: sprints alternadas validando entregas incrementais nas duas frentes.
- Estabelecer suite de testes automatizados no app móvel para verificar integridade de frames e latência.
- Documentar protocolos (headers, comandos, formatos) em anexo técnico para evitar divergências entre frentes.
- Avaliar dependências de hardware (baterias, invólucro, antena) antes de consolidar a solução final.
