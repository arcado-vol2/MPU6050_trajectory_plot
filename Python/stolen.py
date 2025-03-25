filename = 'moving_zyx.csv'
filename = 'rotation_zyx.csv'

import numpy as np
import matplotlib.pyplot as plt
from pyquaternion import Quaternion
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
import matplotlib.animation as animation
from datetime import datetime

def parse_time(time_str):
    """Парсит строку времени в объект datetime"""
    return datetime.strptime(time_str, "%H:%M:%S.%f")

def load_quaternion_data(filename):
    """Загружает данные кватернионов из файла с временными метками"""
    data = np.genfromtxt(filename, delimiter=',', skip_header=1, dtype=str)
    
    # Разделяем временные метки и кватернионы
    timestamps = np.array([parse_time(row[0]) for row in data])
    quaternions = np.array([row[1:5] for row in data], dtype=float)
    
    # Нормализуем кватернионы
    norms = np.linalg.norm(quaternions, axis=1)
    quaternions = quaternions / norms[:, np.newaxis]
    
    return timestamps, quaternions

def quaternion_to_rotation_matrix(q):
    """Преобразует кватернион в матрицу поворота"""
    q_obj = Quaternion(q)
    return q_obj.rotation_matrix

def create_cube(center=(0, 0, 0), size=0.05):
    """Создаёт вершины куба"""
    half = size / 2
    vertices = np.array([
        [-half, -half, -half],  # 0
        [half, -half, -half],   # 1
        [half, half, -half],    # 2
        [-half, half, -half],   # 3
        [-half, -half, half],   # 4
        [half, -half, half],   # 5
        [half, half, half],     # 6
        [-half, half, half]    # 7
    ])
    vertices += np.array(center)
    
    # Грани куба (индексы вершин)
    faces = [
        [vertices[0], vertices[1], vertices[2], vertices[3]],  # низ
        [vertices[4], vertices[5], vertices[6], vertices[7]],  # верх
        [vertices[0], vertices[1], vertices[5], vertices[4]],  # перед
        [vertices[2], vertices[3], vertices[7], vertices[6]],  # зад
        [vertices[1], vertices[2], vertices[6], vertices[5]],  # право
        [vertices[0], vertices[3], vertices[7], vertices[4]]   # лево
    ]
    
    return vertices, faces

# Загрузка данных
timestamps, quat_data = load_quaternion_data(filename)

# Вычисление матриц поворота
R = np.zeros((quat_data.shape[0], 3, 3))
for i in range(quat_data.shape[0]):
    R[i] = quaternion_to_rotation_matrix(quat_data[i])

# Создаём куб для визуализации
cube_vertices, cube_faces = create_cube(size=0.1)

# Визуализация с кубом
def animate_orientation(R, timestamps, cube_vertices, cube_faces, sample_plot_freq=8):
    fig = plt.figure(figsize=(12, 8))
    ax = fig.add_subplot(111, projection='3d')
    
    # Настройка осей
    ax.set_xlim(-0.2, 0.2)
    ax.set_ylim(-0.2, 0.2)
    ax.set_zlim(-0.2, 0.2)
    ax.set_xlabel('X (m)')
    ax.set_ylabel('Y (m)')
    ax.set_zlabel('Z (m)')
    ax.set_title('Ориентация объекта по кватернионам')
    
    # Создание осей координат
    axis_length = 0.15
    x_axis = ax.quiver(0, 0, 0, axis_length, 0, 0, color='r', arrow_length_ratio=0.1)
    y_axis = ax.quiver(0, 0, 0, 0, axis_length, 0, color='g', arrow_length_ratio=0.1)
    z_axis = ax.quiver(0, 0, 0, 0, 0, axis_length, color='b', arrow_length_ratio=0.1)
    
    # Создание куба
    cube_poly = Poly3DCollection(cube_faces, alpha=0.5, linewidths=1, edgecolor='k')
    cube_poly.set_facecolor('cyan')
    ax.add_collection3d(cube_poly)
    
    # Текст для времени
    time_text = ax.text2D(0.02, 0.95, '', transform=ax.transAxes)
    
    def update(frame):
        idx = frame * sample_plot_freq
        if idx >= R.shape[0]:
            idx = R.shape[0] - 1
        
        # Применяем поворот к вершинам куба
        rotated_vertices = (R[idx] @ cube_vertices.T).T
        
        # Обновляем грани куба
        new_faces = [
            [rotated_vertices[0], rotated_vertices[1], rotated_vertices[2], rotated_vertices[3]],
            [rotated_vertices[4], rotated_vertices[5], rotated_vertices[6], rotated_vertices[7]],
            [rotated_vertices[0], rotated_vertices[1], rotated_vertices[5], rotated_vertices[4]],
            [rotated_vertices[2], rotated_vertices[3], rotated_vertices[7], rotated_vertices[6]],
            [rotated_vertices[1], rotated_vertices[2], rotated_vertices[6], rotated_vertices[5]],
            [rotated_vertices[0], rotated_vertices[3], rotated_vertices[7], rotated_vertices[4]]
        ]
        cube_poly.set_verts(new_faces)
        
        # Обновляем оси координат
        rot = R[idx]
        x_dir = rot[:, 0] * axis_length
        y_dir = rot[:, 1] * axis_length
        z_dir = rot[:, 2] * axis_length
        
        x_axis.set_segments([[[0, 0, 0], x_dir]])
        y_axis.set_segments([[[0, 0, 0], y_dir]])
        z_axis.set_segments([[[0, 0, 0], z_dir]])
        
        # Обновляем время
        time_text.set_text(f'Time: {timestamps[idx].strftime("%H:%M:%S.%f")[:-3]}')
        
        return [cube_poly, x_axis, y_axis, z_axis, time_text]
    
    frames = range(0, R.shape[0] // sample_plot_freq)
    ani = animation.FuncAnimation(
        fig, update, frames=frames, interval=50, blit=False
    )
    plt.show()

# Запуск анимации
animate_orientation(R, timestamps, cube_vertices, cube_faces)
