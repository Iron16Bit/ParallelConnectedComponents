from os import walk
import os

BASE_PATH = f"{os.path.dirname(os.path.realpath(__file__))}/.."
def ordina_file(file_path):
    data = []
    has_met_first_line = False
    with open(f"{BASE_PATH}/data/"+file_path, 'r') as f:
        for line in f:
            if(line[0] == '%'):
                continue
            if not has_met_first_line:
                has_met_first_line = True
                with open(f"{BASE_PATH}/data/sorted_"+file_path, 'w') as f:
                    f.write(line)
                continue
            elements = line.strip().split()
            if len(elements) == 2:
                row = int(elements[0])
                col = int(elements[1])
                val = None
            elif len(elements) == 3:
                row = int(elements[0])
                col = int(elements[1])
                val = None
            else:
                continue
            data.append((row, col, val))

    sorted_data = sorted(data, key=lambda x: (x[0], x[1]))

    with open(f"{BASE_PATH}/data/sorted_"+file_path, 'a') as f:
        for row, col, val in sorted_data:
            f.write(f"{row} {col}\n")

filenames = next(walk(f"{BASE_PATH}/data"), (None, None, []))[2]
for file in filenames:
    if(file.endswith(".mtx")):
        ordina_file(file)
        os.remove(f"{BASE_PATH}/data/{file}")
