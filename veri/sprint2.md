# Sprint 2 — Plano de Execução

## Visão Geral
- **Duração:** 2 semanas (10 dias úteis).
- **Escopo da sprint:** entregar as funcionalidades que suportam o streaming via Bluetooth SPP otimizado e controles remotos de captura no aplicativo móvel.
- **Capacidade estimada:** 44 pontos (Fibonacci).
- **Critérios globais de sucesso:** atender aos critérios de aceitação das três histórias priorizadas e manter a latência total ≤ 2 s durante cenários de teste contínuo.

## Backlog da Sprint

| ID | História | Tarefa | Pontos | MoSCoW | DOD |
| --- | --- | --- | --- | --- | --- |
| S1-T1 | Operador | Implementar handshake e conexão SPP no ESP32-CAM com autenticação básica | 5 | Must | Conexão estável em bancada durante 15 min sem queda; logs confirmam handshake concluído |
| S1-T2 | Operador | Ajustar captura QQVGA/alta compressão e pipeline de envio para manter ≤2 s | 8 | Must | Teste com 30 frames consecutivos entrega inferência ≤2 s; consumo de memória monitorado |
| S1-T3 | Operador | Implementar reconexão automática com backoff exponencial | 3 | Should | Reconecta em <10 s após desconexão simulada; sem travar loop principal |
| S1-T4 | Operador | Medir latência ponta-a-ponta e documentar tuning | 3 | Must | Relatório com métricas (média/máx) e parâmetros aplicados armazenado no repositório |
| S2-T1 | Dev Mobile | Implementar parser de header + CRC16 no app e firmware | 5 | Must | Casos de teste unitários validando CRC e tamanhos extremos |
| S2-T2 | Dev Mobile | Tratar frames corrompidos com reenvio e proteção de UI | 3 | Must | Teste manual injeta 5% erros; app recupera sem travar; UI permanece responsiva |
| S2-T3 | Dev Mobile | Criar teste de estresse de 5 min monitorando perdas | 2 | Should | Script automatizado gera relatório com taxa de perda; anexado ao repositório |
| S2-T4 | Dev Mobile | Documentar protocolo de transmissão para integração futura | 2 | Could | Documento em Markdown com campos/fluxos revisado por 2 membros |
| S3-T1 | Analista | Desenvolver UI com sliders de ROI (≥25%) e intervalo (0,2–2 s) | 5 | Must | Controles visíveis/em funcionamento em build interno; valores validados em tempo real |
| S3-T2 | Analista | Implementar comandos de ajuste no ESP32 e confirmação de recebimento | 3 | Must | Ao alterar no app, ESP aplica na próxima captura e confirma via log serial |
| S3-T3 | Analista | Registrar métricas de throughput e configurações em log estruturado | 3 | Should | Log JSON com timestamp, ROI, intervalo e FPS salvo em arquivo/exportável |
| S3-T4 | Analista | Criar painel no app exibindo últimas métricas e estado | 2 | Could | Tela secundária mostra FPS atual e configurações; passa em teste exploratório |

### Resumo por História
- **História Operador:** 19 pontos (Must: 2, Should: 1).
- **História Desenvolvedor Mobile:** 12 pontos (Must: 2, Should: 1, Could: 1).
- **História Analista de Produto:** 13 pontos (Must: 2, Should: 1, Could: 1).

## Definition of Done (Sprint)
- Todos os itens Must concluídos e revisados por pelo menos um par.
- Build do firmware e APK de teste publicados no repositório interno com instruções de flash/instalação.
- Testes manuais documentados cobrindo cenários de latência, perda de conexão e ajuste de ROI.
- Logs de execução e relatórios de métricas anexados nas PRs correspondentes.
- Dashboard de BurnDown atualizado diariamente.

## Dashboard de Progresso

### BurnDown Chart (2 semanas / 10 dias úteis)

| Dia | Ideal restante (pts) | Atual restante (pts) |
| --- | --- | --- |
| 0 | 44 | 44 |
| 1 | 40 | 42 |
| 2 | 36 | 39 |
| 3 | 31 | 34 |
| 4 | 27 | 29 |
| 5 | 22 | 26 |
| 6 | 18 | 20 |
| 7 | 13 | 14 |
| 8 | 9 | 10 |
| 9 | 4 | 5 |
| 10 | 0 | 0 |

- **Observações:** progresso inicial abaixo da linha ideal, compensado na segunda semana com aceleração após integração do pipeline de transmissão.
