import matplotlib.pyplot as plt
import numpy as np

# Чтение данных из файла
data = []
with open('data.txt', 'r') as file:  # замените 'data.txt' на имя вашего файла
    for line in file:
        if line.strip():  # пропускаем пустые строки
            row = list(map(float, line.strip().split()))
            data.append(row)

# Преобразование в numpy массив
points = np.array(data)
x = points[:, 0]
y = points[:, 1]
z = points[:, 2]

# Создание 3D графика
fig = plt.figure(figsize=(10, 8))
ax = fig.add_subplot(111, projection='3d')

# Отображение линии с точками
# Сначала рисуем линию
ax.plot(x, y, z, c='b', linestyle='-', linewidth=1, alpha=0.5)
# Затем рисуем точки поверх линии
scatter = ax.scatter(x, y, z, c=z, cmap='viridis', marker='o', s=50)

# Настройка осей
ax.set_xlabel('X координата')
ax.set_ylabel('Y координата')
ax.set_zlabel('Z координата')
ax.set_title('3D визуализация точек с соединяющей линией')

# Добавление цветовой шкалы
cbar = fig.colorbar(scatter, ax=ax, shrink=0.5, aspect=5)
cbar.set_label('Значение Z')

# Поворот графика для лучшего обзора
ax.view_init(elev=20, azim=45)

plt.tight_layout()
plt.show()
