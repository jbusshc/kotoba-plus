import sys
import os
import glob

def dump_context():
    raw_args = sys.argv[1:]

    if not raw_args:
        print("Uso: python contextDumper.py [archivo] [carpeta/*] [*.py] ...")
        return

    filenames = []
    for arg in raw_args:
        expanded = glob.glob(arg, recursive=True)
        if expanded:
            filenames.extend(expanded)
        else:
            filenames.append(arg)

    separator = "=" * 60
    output_file = "dump.txt"

    try:
        # Abrimos el archivo de salida una sola vez en modo escritura
        with open(output_file, 'w', encoding='utf-8') as dump:
            for filename in filenames:
                if os.path.isfile(filename):
                    try:
                        # Leemos el archivo original
                        with open(filename, 'r', encoding='utf-8') as f:
                            content = f.read()
                        
                        # Escribimos en el dump.txt en lugar de hacer print
                        dump.write(f"\n{separator}\n")
                        dump.write(f"FICHERO: {filename}\n")
                        dump.write(f"RUTA ABSOLUTA: {os.path.abspath(filename)}\n")
                        dump.write(f"{separator}\n")
                        dump.write(content)
                        dump.write(f"\n{separator}\n\n")
                        
                        print(f"[OK] Procesado: {filename}")
                        
                    except Exception as e:
                        error_msg = f">> [ERROR] No se pudo leer {filename}: {e}\n"
                        dump.write(error_msg)
                        print(error_msg.strip())
                
                elif os.path.isdir(filename):
                    continue 
                else:
                    print(f">> [ADVERTENCIA] No se encontró: {filename}")
        
        print(f"\n--- Proceso finalizado. Revisa '{output_file}' ---")

    except Exception as e:
        print(f"Error crítico al crear el archivo de salida: {e}")

if __name__ == "__main__":
    dump_context()