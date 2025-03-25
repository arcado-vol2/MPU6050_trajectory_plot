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

ser = serial.Serial('COM9', 115200, timeout=0.001)
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
    
    # Создаем новый файл с уникальным именем
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    filename = f"{output_dir}/sensor_data_{timestamp}_{file_counter}.csv"
    file_counter += 1
    
    csv_file = open(filename, mode='w', newline='')
    csv_writer = csv.writer(csv_file)
    csv_writer.writerow(['Timestamp', 'Q_w', 'Q_x', 'Q_y', 'Q_z'])

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
    global q, is_recording
    
    while ser.in_waiting >= 14:
        try:
            line = ser.readline().decode('utf-8').strip()
            if line:
                # Проверяем, не пустая ли строка (может быть после strip())
                if not line:
                    continue
                    
                # Проверяем, содержит ли строка данные кватерниона
                if line.count(',') == 3:
                    data = list(map(float, line.split(',')))
                    if len(data) == 4:
                        q = np.array(data)
                        
                        # Если началась запись и файл еще не создан
                        if is_recording and csv_writer is None:
                            init_csv()
                        
                        # Записываем данные, если идет запись
                        if is_recording and csv_writer is not None:
                            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
                            csv_writer.writerow([timestamp, q[0], q[1], q[2], q[3]])
                            csv_file.flush()
                else:
                    # Если строка не содержит данных кватерниона, проверяем, не сигнал ли это о начале/остановке записи
                    if "Start" in line or "Stop" in line:
                        is_recording = "Start" in line
                        if is_recording:
                            print("Начало записи данных")
                            init_csv()
                        else:
                            print("Остановка записи данных")
                            if csv_file is not None:
                                csv_file.close()
                                csv_file = None
                                csv_writer = None
        except Exception as e:
            print(f"Ошибка при обработке данных: {e}")

def display():
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    glLoadIdentity()
    gluLookAt(3, 2, 3, 0, 0, 0, 0, 0, 1)
    
    # Применение кватерниона
    angle = 2 * math.acos(q[0])
    norm = max(0.001, math.sqrt(1 - q[0]**2))
    x, y, z = q[1]/norm, q[2]/norm, q[3]/norm
    glRotatef(math.degrees(angle), x, y, z)
    
    draw_board()
    glutSwapBuffers()

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