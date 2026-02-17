import sys
import os
import glob

def dump_context():
    # Capturamos todos los argumentos pasados (excepto el nombre del script)
    raw_args = sys.argv[1:]

    if not raw_args:
        print("Uso: python contextDumper.py [archivo] [carpeta/*] [*.py] ...")
        return

    # Expandimos los patrones (wildcards) para obtener la lista real de archivos
    filenames = []
    for arg in raw_args:
        # glob.glob expande "ruta/*" en una lista de rutas reales
        expanded = glob.glob(arg, recursive=True)
        if expanded:
            filenames.extend(expanded)
        else:
            # Si glob no encuentra nada, lo dejamos pasar para que el error se maneje después
            filenames.append(arg)

    separator = "=" * 60
    
    for filename in filenames:
        # Solo procesamos si es un archivo (ignoramos carpetas que coincidan con el patrón)
        if os.path.isfile(filename):
            try:
                with open(filename, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                print(f"\n{separator}")
                print(f"FICHERO: {filename}")
                print(f"RUTA ABSOLUTA: {os.path.abspath(filename)}")
                print(separator)
                print(content)
                print(f"{separator}\n")
                
            except Exception as e:
                print(f">> [ERROR] No se pudo leer {filename}: {e}")
        elif os.path.isdir(filename):
            # Opcional: Avisar si el patrón incluyó una carpeta
            continue 
        else:
            print(f">> [ADVERTENCIA] No se encontró o no es archivo: {filename}")

if __name__ == "__main__":
    dump_context()