import sqlite3
from datetime import datetime

def inserir_imagem(nome_arquivo, caminho, descricao=None):
    conn = sqlite3.connect('spacemaps.db')
    cursor = conn.cursor()

    data_hora = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    cursor.execute("""
        INSERT INTO imagens (nome_arquivo, caminho, data_hora, descricao)
        VALUES (?, ?, ?, ?)
    """, (nome_arquivo, caminho, data_hora, descricao))

    conn.commit()
    conn.close()
    print(f"Imagem '{nome_arquivo}' registrada com sucesso no banco.")

# Exemplo de uso
if __name__ == "__main__":
    inserir_imagem("image_20251008.jpg", "server/images/image_20251008.jpg", "Foto de ambiente capturada pelo ESP32")
