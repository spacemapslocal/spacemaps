import sqlite3

conn = sqlite3.connect('spacemaps.db')
cursor = conn.cursor()

cursor.execute("SELECT * FROM imagens")
imagens = cursor.fetchall()

print("=== Imagens Registradas ===")
for img in imagens:
    print(img)

conn.close()
