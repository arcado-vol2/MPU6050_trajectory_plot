import serial
import numpy as np
from OpenGL.GL import *
from OpenGL.GLUT import *
from OpenGL.GLU import *
import time
import math
import csv
from datetime import datetime
import os

ser = serial.Serial('COM5', 230400, timeout=0.001)
ser.flushInput()

# Параметры платы
BOARD_WIDTH, BOARD_HEIGHT, BOARD_THICKNESS = 0.6, 1.0, 0.1
AXIS_LENGTH = 1.5  # Длина осей

# Кватернион для вращения
q = np.array([1.0, 0.0, 0.0, 0.0])  # w, x, y, z

# Переменные для работы с CSV
csv_file = None
csv_writer = None
is_recording = False
file_counter = 1
output_dir = "sensor_data"

def init_csv():
    global csv_file, csv_writer, file_counter, output_dir
    
    # Создаем папку для данных, если ее нет
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # Закрываем предыдущий файл, если он был открыт
    if csv_file is not None:
        csv_file.close()
        csv_file = None
        csv_writer = None
    
    # Создаем новый файл с уникальным именем
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    filename = f"{output_dir}/sensor_data_{timestamp}_{file_counter}.csv"
    file_counter += 1
    
    csv_file = open(filename, mode='w', newline='')
    csv_writer = csv.writer(csv_file)
    csv_writer.writerow(['t', 'w', 'x', 'y', 'z', 'ax', 'ay', 'az'])
    print(f"Создан новый файл для записи: {filename}")

def init_gl():
    glClearColor(0.1, 0.1, 0.1, 1.0)
    glEnable(GL_DEPTH_TEST)
    glMatrixMode(GL_PROJECTION)
    gluPerspective(45, (800/600), 0.1, 50.0)
    glMatrixMode(GL_MODELVIEW)

def draw_axes():
    """Отрисовка осей координат"""
    glBegin(GL_LINES)
    glColor3f(1, 0, 0)
    glVertex3f(0, 0, 0)
    glVertex3f(AXIS_LENGTH, 0, 0)
    glColor3f(0, 1, 0)
    glVertex3f(0, 0, 0)
    glVertex3f(0, AXIS_LENGTH, 0)
    glColor3f(0, 0, 1)
    glVertex3f(0, 0, 0)
    glVertex3f(0, 0, AXIS_LENGTH)
    glEnd()

def draw_board():
    half_w = BOARD_WIDTH / 2
    half_h = BOARD_HEIGHT / 2
    
    glBegin(GL_QUADS)
    glColor3f(0.8, 0.2, 0.2)
    glVertex3f(-half_w, -half_h, BOARD_THICKNESS)
    glVertex3f(half_w, -half_h, BOARD_THICKNESS)
    glVertex3f(half_w, half_h, BOARD_THICKNESS)
    glVertex3f(-half_w, half_h, BOARD_THICKNESS)
    glEnd()
    
    draw_axes()

def read_serial():
    global q, is_recording, csv_file, csv_writer
    
    while ser.in_waiting > 0:
        try:
            line = ser.readline().decode('utf-8').strip()
            if not line:
                continue
                
            # Проверяем, не сигнал ли это о начале/остановке записи
            if line == "Start recording":
                is_recording = True
                init_csv()
                print("Начало записи данных")
                continue
            elif line == "Stop recording":
                is_recording = False
                if csv_file is not None:
                    csv_file.close()
                    csv_file = None
                    csv_writer = None
                print("Остановка записи данных")
                continue
                
            # Если идет запись и строка содержит данные кватерниона
            print(line)
            if is_recording and line.count(',') == 6:
                try:
                    data = list(map(float, line.split(',')))
                    if len(data) == 7:
                        # Сохраняем все данные, но для вращения используем только кватернион
                        q = np.array(data[:4])  # Только кватернион
                        
                        if csv_writer is not None:
                            timestamp = datetime.now().strftime('%H:%M:%S.%f')
                            csv_writer.writerow([timestamp] + data)  # Записываем только кватернион
                            csv_file.flush()
                except ValueError as e:
                    print(f"Ошибка преобразования данных: {e}")
                    
        except UnicodeDecodeError:
            print("Ошибка декодирования строки", line)
        except Exception as e:
            print(f"Ошибка при обработке данных: {e}")

def display():
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    glLoadIdentity()
    gluLookAt(3, 2, 3, 0, 0, 0, 0, 0, 1)
    
    # Применение кватерниона
    try:
        # Берем только первые 4 элемента (кватернион)
        quat = q[:4]
        
        # Нормализуем кватернион
        norm = np.linalg.norm(quat)
        if norm < 0.001:
            # Нулевой кватернион - не применяем вращение
            pass
        else:
            quat = quat / norm
            
            # Убедимся, что w компонента в допустимом диапазоне
            w = max(-1.0, min(1.0, quat[0]))
            angle = 2 * math.acos(w)
            
            # Вычисляем ось вращения
            axis_norm = math.sqrt(1 - w**2)
            if axis_norm > 0.001:
                x, y, z = quat[1]/axis_norm, quat[2]/axis_norm, quat[3]/axis_norm
                glRotatef(math.degrees(angle), x, y, z)
        
        draw_board()
        glutSwapBuffers()
    except Exception as e:
        print(f"Ошибка в display: {e}, q={q}")

def idle():
    read_serial()
    glutPostRedisplay()
    time.sleep(0.001)

def cleanup():
    """Функция для закрытия файла при завершении программы"""
    if csv_file is not None:
        csv_file.close()
    ser.close()

def main():
    glutInit()
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH)
    glutInitWindowSize(800, 600)
    glutCreateWindow(b"MPU6050 3D Orientation with Axes")
    init_gl()
    glutDisplayFunc(display)
    glutIdleFunc(idle)
    try:
        glutMainLoop()
    finally:
        cleanup()

if __name__ == "__main__":
    main()
