import mysql.connector

# 🔹 Conectar ao banco de dados
try:
    conexao = mysql.connector.connect(
        host="localhost",
        user="root",
        password="1234",  # troque se necessário
    )
    cursor = conexao.cursor()
    print("✅ Conectado ao MySQL!")

    # 🔹 Criar banco e tabela, se não existirem
    cursor.execute("CREATE DATABASE IF NOT EXISTS fotosdb")
    cursor.execute("USE fotosdb")
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS imagens (
            id INT AUTO_INCREMENT PRIMARY KEY,
            nome VARCHAR(100),
            foto LONGBLOB
        )
    """)
    conexao.commit()
    print("📦 Banco e tabela prontos.")

    # 🔹 Inserir uma imagem de teste (simulação)
    imagem_simulada = b"exemplo_de_bytes_de_imagem"
    cursor.execute("INSERT INTO imagens (nome, foto) VALUES (%s, %s)", ("teste_inicial", imagem_simulada))
    conexao.commit()
    print("🖼️ Imagem de teste salva no banco.")

    # 🔹 Buscar e exibir os registros
    cursor.execute("SELECT id, nome FROM imagens")
    for (id, nome) in cursor.fetchall():
        print(f"📸 ID: {id}, Nome: {nome}")

except mysql.connector.Error as erro:
    print("❌ Erro ao conectar ou operar no MySQL:", erro)

finally:
    if 'cursor' in locals():
        cursor.close()
    if 'conexao' in locals():
        conexao.close()
