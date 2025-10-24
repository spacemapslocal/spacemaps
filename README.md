# 🛰️ SpaceMaps - Banco de Dados

Este repositório contém o **módulo de banco de dados** do projeto **SpaceMaps**, um sistema baseado em ESP32-CAM e Inteligência Artificial que auxilia pessoas com deficiência visual na identificação de objetos e ambientes.

---

## 📚 Sobre o Módulo
O módulo de banco de dados é responsável por armazenar as **imagens capturadas**, seus **metadados** e **descrições** geradas pela IA.

### Estrutura da Tabela `imagens`
| Campo        | Tipo     | Descrição                                 |
|---------------|-----------|-------------------------------------------|
| id            | INTEGER   | Identificador único                      |
| nome_arquivo  | TEXT      | Nome do arquivo de imagem                |
| caminho       | TEXT      | Caminho onde a imagem foi salva          |
| data_hora     | TEXT      | Data e hora do registro                  |
| descricao     | TEXT      | Descrição gerada ou anotada da imagem    |

---

