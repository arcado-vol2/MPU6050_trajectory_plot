import os
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path

def plot_csv_columns(csv_file):
    try:
        data = np.genfromtxt(csv_file, delimiter=',', skip_header=1)
        
        if data.size == 0:
            print(f"Файл {csv_file} пустой.")
            return
        if data.ndim == 1:
            data = data.reshape(-1, 1)
        with open(csv_file, 'r') as f:
            headers = f.readline().strip().split(',')
        plt.figure(figsize=(10, 6))
        colors = ['r', 'g', 'b', 'c', 'm', 'y', 'k']
        
        for i in range(data.shape[1]):
            color = colors[i % len(colors)]
            label = headers[i] if i < len(headers) else f'Column {i+1}'
            plt.plot(data[:, i], color, label=label)
        
        plt.xlabel('Номер строки')
        plt.ylabel('Значение')
        plt.title(Path(csv_file).stem)
        plt.legend()
        plt.grid(True)
        
        output_filename = f"plots/{Path(csv_file).stem}_plot.png"
        plt.savefig(output_filename)
        plt.close()
        print(f"График для {csv_file} сохранен как {output_filename}")
    
    except Exception as e:
        print(f"Ошибка при обработке файла {csv_file}: {str(e)}")

for file in os.listdir():
    if file.endswith('.csv'):
        plot_csv_columns(file)
