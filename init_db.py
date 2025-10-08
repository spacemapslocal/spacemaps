import sqlite3

# Cria (ou conecta) ao banco local
conn = sqlite3.connect('spacemaps.db')
cursor = conn.cursor()

# Cria a tabela de fotos
cursor.execute("""
CREATE TABLE IF NOT EXISTS imagens (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    nome_arquivo TEXT NOT NULL,
    caminho TEXT NOT NULL,
    data_hora TEXT NOT NULL,
    descricao TEXT
);
""")

conn.commit()
conn.close()

print("Banco de dados e tabela 'imagens' criados com sucesso!")
