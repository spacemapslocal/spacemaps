# Sprint 2 — Dossiê Scrum do Projeto Veri

## Visão do Produto e Contexto
- **Nome do produto:** Veri – Inteligência que enxerga com você.
- **Proposta de valor:** óculos inteligentes com IA que descrevem ambientes, objetos, rostos e obstáculos para ampliar a autonomia de pessoas com deficiência visual.
- **Foco desta sprint:** otimizar o fluxo ESP32-CAM + Bluetooth SPP, manter latência ≤ 2 s e permitir controle remoto de captura via aplicativo móvel.
- **Incremento esperado:** entregar um conjunto integrado (firmware + Android + documentação + branding) pronto para validação de campo com usuários, preservando a arquitetura atual e apresentando o rebranding “Veri”.

## Time Scrum e Colaborações
- **Product Owner (PO):** Thádio — prioriza o backlog, garante alinhamento com a visão de produto e valida o incremento.
- **Scrum Master:** Suel Marques Albuquerque — facilita cerimônias, remove impedimentos e mantém o acompanhamento do BurnDown/Dashboard.
- **Desenvolvedores:**  
  - Tiago Damião dos Santos — integração Android (Bluetooth SPP, inferência offline).  
  - Thiago Roberto Soares Martins — integração iOS (BLE, Core ML).  
  - Pedro Silva Carvalho — site de apresentação/venda alinhado ao branding Veri.  
  - Michael Douglas Figueiroa dos Santos — naming definitivo e desenvolvimento da logomarca.  
  - Suel Marques Albuquerque — dashboard com BurnDown Chart e métricas da sprint.  
  - Demais membros de firmware/mobile colaboram conforme necessidade da Frente 1.
- **Stakeholders principais:** equipe de teste de campo, comunidade de usuários com deficiência visual e corpo docente SENAC ADS.

## Timebox e Agenda
- **Janela da sprint:** 24/10/2025 a 06/11/2025 (10 dias úteis).  
  - Dia 0: Sprint Planning (definição de meta e estimativas).  
  - Dias 1–9: Dailies às 09:30 (15 min) com registro de impedimentos.  
  - Dia 9 fim do expediente: Sprint Review com demonstração do streaming otimizado e site.  
  - Dia 10 manhã: Sprint Retrospective (60 min) focada em fluxo entre firmware e mobile.  
  - Refinement leve nas terças e quintas (30 min) para ajustar histórias futuras.
- **Timeboxes adicionais:**  
  - **Daily Scrum:** 15 minutos.  
  - **Planning Poker:** 60 minutos na abertura da sprint.  
  - **Review:** 90 minutos (inclui feedback de stakeholders).  
  - **Retrospective:** 60 minutos com ações priorizadas para a Sprint 3.

## Artefatos Scrum da Sprint 2

### 1. Backlog da Sprint + MoSCoW
| ID | História | Tarefa | Pontos (Fib.) | MoSCoW | Status inicial |
| --- | --- | --- | --- | --- | --- |
| S1-T1 | Operador | Implementar handshake e conexão SPP no ESP32-CAM com autenticação básica | 5 | Must | A fazer |
| S1-T2 | Operador | Ajustar captura QQVGA/alta compressão e pipeline de envio para manter ≤2 s | 8 | Must | A fazer |
| S1-T3 | Operador | Implementar reconexão automática com backoff exponencial | 3 | Should | A fazer |
| S1-T4 | Operador | Medir latência ponta-a-ponta e documentar tuning | 3 | Must | A fazer |
| S2-T1 | Dev Mobile | Implementar parser de header + CRC16 no app e firmware | 5 | Must | A fazer |
| S2-T2 | Dev Mobile | Tratar frames corrompidos com reenvio e proteção de UI | 3 | Must | A fazer |
| S2-T3 | Dev Mobile | Criar teste de estresse de 5 min monitorando perdas | 2 | Should | A fazer |
| S2-T4 | Dev Mobile | Documentar protocolo de transmissão para integração futura | 2 | Could | A fazer |
| S3-T1 | Analista | Desenvolver UI com sliders de ROI (≥25%) e intervalo (0,2–2 s) | 5 | Must | A fazer |
| S3-T2 | Analista | Implementar comandos de ajuste no ESP32 e confirmação de recebimento | 3 | Must | A fazer |
| S3-T3 | Analista | Registrar métricas de throughput e configurações em log estruturado | 3 | Should | A fazer |
| S3-T4 | Analista | Criar painel no app exibindo últimas métricas e estado | 2 | Could | A fazer |

- **Capacidade total:** 44 pontos Fibonacci.  
- **Distribuição MoSCoW:** Must (31 pts), Should (11 pts), Could (4 pts). Nenhum item Won’t nesta sprint.

### 2. Histórias de Usuário e Requisitos de Aceitação
**HU1 — Operador:** iniciar streaming Bluetooth e receber descrição ≤ 2 s.  
Requisitos de aceitação:  
1. Conexão Bluetooth SPP estabelecida em < 5 s.  
2. Latência entre captura e descrição ≤ 2 s em QQVGA.  
3. Reconexão automática em ≤ 10 s após quedas.

**HU2 — Desenvolvedor Mobile:** receber frames comprimidos confiáveis para TFLite.  
Requisitos de aceitação:  
1. Header com tamanho (4 bytes little-endian) + CRC16 por frame.  
2. Perda de frames < 2% em teste de 5 minutos.  
3. App rejeita frames corrompidos, solicita reenvio e mantém UI responsiva.

**HU3 — Analista de Produto:** ajustar ROI e frequência de captura no campo.  
Requisitos de aceitação:  
1. UI com sliders para ROI (≥ 25% do frame) e intervalo (0,2–2 s).  
2. Alterações ativas até o próximo frame gerado.  
3. Logs exibem configuração vigente e métricas de throughput.

### 3. Kanban da Sprint
| Coluna | Itens | Observações |
| --- | --- | --- |
| To Do | S1-T1, S1-T2, S1-T3, S1-T4, S2-T1, S2-T2, S2-T3, S2-T4, S3-T1, S3-T2, S3-T3, S3-T4 | Prioridade executada conforme MoSCoW e dependências entre firmware ↔ mobile. |
| In Progress | Definido dinamicamente nas dailies | Limite de WIP: 4 itens para evitar gargalos em testes de campo. |
| Review | Itens aguardando validação do PO e verificação cruzada (QA/manual). | Checklist inclui evidências de testes e logs anexados. |
| Done | Itens aceitos pelo PO e integrados ao repositório principal. | Atualização imediata do BurnDown por Suel após cada conclusão. |

- **Ferramenta sugerida:** Trello ou Jira, com labels `Must`, `Should`, `Could` e campos customizados para latência medida.

### 4. BurnDown Chart
| Dia útil | Ideal restante (pts) | Atual restante (pts) | Comentários |
| --- | --- | --- | --- |
| 0 | 44 | 44 | Linha base pós-planning. |
| 1 | 40 | 42 | Ajustes iniciais no firmware ainda em progresso. |
| 2 | 36 | 39 | Dependências entre parser CRC e firmware mapeadas. |
| 3 | 31 | 34 | Primeiros testes de latência mostram necessidade de tuning. |
| 4 | 27 | 29 | Reconexão automatizada pronta para validação. |
| 5 | 22 | 26 | Pico de esforço em testes de estresse mobile. |
| 6 | 18 | 20 | UI de ROI disponível em build interno. |
| 7 | 13 | 14 | Ajustes de logs estruturados e painel parcial. |
| 8 | 9 | 10 | Site Veri e branding integrados ao material de review. |
| 9 | 4 | 5 | Preparação para Review, finalização de documentação. |
| 10 | 0 | 0 | Incremento aprovado na Review. |

- **Métricas acompanhadas:** pontos restantes, variância diária, tarefas bloqueadas, lead time por item.

### 5. Definition of Done (DoD)
- Código revisado por pelo menos um par (PR aprovada) e cobertura mínima dos cenários críticos.  
- Build do firmware (binário) e APK de teste publicados com instruções de instalação/flash.  
- Testes manuais documentados (latência, reconexão, ajuste ROI, perdas).  
- Logs de execução e relatórios anexados às PRs.  
- Documentação atualizada (`docs/operator_story_setup.md`, protocolo de transmissão, dashboards).  
- Dashboard BurnDown atualizado e compartilhado no canal da equipe.  
- Incremento demonstrado para stakeholders com feedback registrado.

### 6. Definition of Ready (Suporte aos items Must)
- História priorizada pelo PO com descrição clara e critérios de aceitação.  
- Dependências externas mapeadas (hardware, dispositivos móveis, assets).  
- Estimativa definida via Planning Poker e capacidade validada no planning.  
- Protótipo/tarefa possui dados de entrada necessários (modelo TFLite, scripts, logotipos).

### 7. Planning Poker, Sequência Fibonacci e Estimativas
- **Sequência adotada:** 1, 2, 3, 5, 8, 13, 21 (com uso dos valores 2, 3, 5, 8 nesta sprint).  
- **Ritual:** conduzido no Sprint Planning; o Scrum Master coleta votos simultâneos no board digital. Divergências > 1 passo discutidas rapidamente, com o PO tirando dúvidas.  
- **Distribuição de pontos:**  
  - 8 pontos: tarefas com dependências cruzadas firmware↔app (ex.: S1-T2).  
  - 5 pontos: itens com média complexidade ou integração (S1-T1, S2-T1, S3-T1).  
  - 3 pontos: melhorias incrementais e reconexão (S1-T3, S1-T4, S2-T2, S3-T2, S3-T3).  
  - 2 pontos: documentação, scripts e UI auxiliar (S2-T3, S2-T4, S3-T4).  
- **Objetivo do método:** medir esforço relativo, promover alinhamento técnico e ajustar capacidade total (44 pts) para caber nos 10 dias úteis.

### 8. MoSCoW Detalhado
- **Must (31 pts):** funcionalidades essenciais para validação do fluxo SPP, ROI e estabilidade (S1-T1, S1-T2, S1-T4, S2-T1, S2-T2, S3-T1, S3-T2).  
- **Should (11 pts):** itens que melhoram resiliência e métricas (S1-T3, S2-T3, S3-T3).  
- **Could (4 pts):** documentação e painel complementar (S2-T4, S3-T4).  
- **Won’t:** backlog futuro (ex.: integração avançada com sensores adicionais) — mantido fora do escopo desta sprint.

### 9. Incremento da Sprint
- **Firmware ESP32-CAM:** handshake SPP seguro, captura QQVGA otimizada, compressão ajustável, reconexão automática e aplicação imediata de comandos de ROI/intervalo.  
- **Aplicativo Android:** parser robusto de headers + CRC16, tratamento de frames corrompidos, sliders de ROI/intervalo, painel de métricas e logging estruturado.  
- **Aplicativo iOS (pré-incremento):** plano de integração BLE documentado com passos de build (responsável Thiago Roberto).  
- **Dashboard:** BurnDown Chart atualizado diariamente com variação ideal vs real, pronto para compartilhamento em Review.  
- **Branding e Site:** nome Veri validado, logomarca inicial e site de apresentação/venda alinhados ao novo branding (responsáveis Michael e Pedro).  
- **Documentação:** protocolos de transmissão, tuning de latência, guias Android/iOS e resumo da sprint atualizados em `docs/`.

### 10. Checklist de Requisitos de Aceitação (Consolidação)
- **Latência:** medições ≤ 2 s registradas em relatório; qualquer exceção documentada com ação corretiva.  
- **Conectividade:** logs demonstrando conexão < 5 s e reconexão < 10 s.  
- **Integridade de dados:** testes unitários/com instrumentação confirmando CRC16, e script de estresse com perdas < 2%.  
- **Configurações remotas:** mensagens de comando confirmadas no serial e na UI, com logs JSON centralizados.  
- **Experiência do usuário final:** descrições de objetos entregues consistentemente durante demonstração da Review.

### 11. Ferramentas e Logs de Suporte
- **Repositório:** `github.com/spacemapslocal/spacemaps` (pasta `veri`).  
- **Firmware:** `firmware/esp32cam_bt_stream.ino` (Bluetooth SPP) e `firmware/esp32cam_ble_stream.ino` (BLE para iOS).  
- **Mobile Android:** `mobile/app/src/...` com `BluetoothStreamClient` e assets (`ssd_mobilenet_v1.tflite`, `labelmap.txt`).  
- **Mobile iOS:** `ios/VeriBLE/` (SwiftUI, integração BLE).  
- **Documentação complementar:** `docs/operator_story_setup.md`, `docs/operator_story_ios.md`, `delivery_plan.md`, `sprint2.md`.  
- **Scripts de suporte:** conversão TFLite → Core ML (`ios/VeriBLE/scripts/convert_tflite_to_coreml.py`).  
- **Dashboard BurnDown:** mantido por Suel com base na tabela desta sprint.

### 12. Comunicação e Transparência
- **Canal primário:** grupo WhatsApp/SENAC citado nas mensagens de 23/10/25.  
- **Atualizações pós-daily:** quadro Kanban e BurnDown sincronizados antes das 11h30.  
- **Compartilhamento de incrementos:** vídeos curtos da inferência e capturas de tela do app/site anexados às PRs e apresentados na Review.  
- **Feedback loop:** comentários de stakeholders registrados em ata e inseridos no backlog como novas histórias ou melhorias.

### 13. Próximos Passos Pós-Sprint
- Mapear integrações BLE avançadas após validação do incremento Android.  
- Priorizar suporte a novos sensores e integração com serviços de voz para Sprint 3.  
- Evoluir o site Veri com materiais de marketing e depoimentos após obtidas métricas de campo.  
- Planejar testes com usuários finais utilizando os óculos inteligentes em ambientes reais.

---
**Resumo:** Este arquivo consolida todos os artefatos da metodologia Scrum para a Sprint 2 do projeto Veri, garantindo visão compartilhada do backlog, critérios de aceite, DoD, métricas (BurnDown), governança de timebox, estimativas via Fibonacci/Planning Poker, quadro Kanban de execução e descrição do incremento esperado. Thádio, como PO, usa este dossiê para acompanhar progresso e validar o incremento entregue ao final da sprint.
