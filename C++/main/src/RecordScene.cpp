#include "RecordScene.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "FileDialog.h"
#include <csv.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <iomanip>

RecordScene::RecordScene(COM::Port* comPort) : Scene(comPort) {
    vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "out vec3 ourColor;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "   ourColor = aColor;\n"
        "}\0";

    fragmentShaderSource = "#version 330 core\n"
        "in vec3 ourColor;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(ourColor, 1.0f);\n"
        "}\0";

    // Параметры платы
    BOARD_WIDTH = 0.6f;
    BOARD_HEIGHT = 1.0f;
    BOARD_THICKNESS = 0.1f;
    AXIS_LENGTH = 1.5f;

    // Инициализация кватерниона (w, x, y, z)
    q = { 1.0f, 0.0f, 0.0f, 0.0f };
    a = { 0.0f, 0.0f, 0.0f };

    if (p_comPort && p_comPort->IsOpen()) {  // Проверка указателя и состояния порта
        // Пор уже открыт, ничего не делаем
    }
    else if (p_comPort) {
        p_comPort->Open();  // Открываем порт, если он не открыт
    };
}

RecordScene::~RecordScene() {
    // Удаляем VAO и VBO для платы
    glDeleteVertexArrays(1, &boardVAO);
    glDeleteBuffers(1, &boardVBO);

    // Удаляем VAO и VBO для осей
    glDeleteVertexArrays(1, &axesVAO);
    glDeleteBuffers(1, &axesVBO);

    // Удаляем шейдерную программу
    glDeleteProgram(shaderProgram);

    if (csvFile.is_open()) {
        csvFile.close();
    }
}

void RecordScene::InitRender() {
    InitBoard();
    InitAxes();
    SetupCamera();
}
static inline void trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());
}

std::string RecordScene::GenerateCSVFIlePath() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return savePath + "/recording_" + oss.str() + ".csv";
}

void RecordScene::StartNewRecording() {
    // Закрываем предыдущий файл, если он был открыт
    if (csvFile.is_open()) {
        csvFile.close();
    }

    // Генерируем новое имя файла
    csvFilePath = GenerateCSVFIlePath();

    // Открываем файл для записи
    csvFile.open(csvFilePath);
    if (csvFile.is_open()) {
        // Записываем заголовок
        csvFile << "t,w,x,y,z,ax,ay,az\n";
    }
    else {
        std::cout << "error due file creating " << csvFilePath << std::endl;
    }
}

void RecordScene::WriteQToCSV() {
    if (csvFile.is_open()) {
        // Получаем текущее время с микросекундами
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);
        auto now_tm = *std::localtime(&now_time);

        // Получаем микросекунды (остаток от деления на 1 000 000)
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()) % 1000000;

        // Форматируем время: ЧАСЫ:МИНУТЫ:СЕКУНДЫ.МИКРОСЕКУНДЫ (6 знаков)
        csvFile << std::put_time(&now_tm, "%H:%M:%S") << "."
            << std::setfill('0') << std::setw(6) << us.count() << ","
            << q[0] << "," << q[1] << "," << q[2] << "," << q[3] << "\n";
    }
    else {
        std::cout << "error due file writing " << csvFilePath << std::endl;
    }
}

void RecordScene::StopRecording() {
    if (csvFile.is_open()) {
        csvFile.close();
    }
}
void RecordScene::Update() {
    if (!p_comPort || !p_comPort->IsOpen()) return;

    std::string data = p_comPort->Read();
    if (data.empty()) return;

    // Проверка команд управления записью
    if (data.find("Start recording") != std::string::npos) {
        isRecording = true;
        return;
    }
    else if (data.find("Stop recording") != std::string::npos) {
        isRecording = false;
        return;
    }

    if (!isRecording) return;

    // Быстрый парсинг данных (формат: w,x,y,z,ax,ay,az)
    size_t pos = 0;
    size_t comma_pos;
    float values[7];
    int i = 0;

    try {
        while ((comma_pos = data.find(',', pos)) != std::string::npos && i < 7) {
            values[i++] = std::stof(data.substr(pos, comma_pos - pos));
            pos = comma_pos + 1;
        }
        // Последнее значение (az) после последней запятой до конца строки
        if (i < 7 && pos < data.size()) {
            values[i++] = std::stof(data.substr(pos));
        }

        if (i == 7) { // Все 7 значений успешно прочитаны
            // Копируем в массивы
            std::copy(values, values + 4, q.begin());
            std::copy(values + 4, values + 7, a.begin());
        }
    }
    catch (...) {
        // Ошибка парсинга - пропускаем этот пакет
    }
}


void RecordScene::Render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Активируем шейдер
    glUseProgram(shaderProgram);

    // Передаем матрицы в шейдер
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Создаем матрицу модели на основе кватерниона
    glm::mat4 model = glm::mat4(1.0f);
    float angle = 2.0f * acos(q[0]);
    float norm = sqrt(1.0f - q[0] * q[0]);
    if (norm > 0.001f) {
        norm = 1.0f / norm;
        glm::vec3 axis(q[1] * norm, q[2] * norm, q[3] * norm);
        model = glm::rotate(model, angle, axis);
    }
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // Рисуем плату
    glBindVertexArray(boardVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // Рисуем оси
    glBindVertexArray(axesVAO);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
}

void RecordScene::RenderUI() {
    ImGui::Begin("3D Board Orientation");

    ImGui::BeginDisabled();
    ImGui::Checkbox("Record status: ", &isRecording);
    ImGui::EndDisabled();

    ImGui::Separator();
    if (ImGui::Button("Choose save path")) {
        savePath = OpenFolderDialog();
    }
    ImGui::Text("Selected path:");
    ImGui::SameLine();
    ImGui::Text("%s", savePath.c_str());
    ImGui::Separator();

    ImGui::Text("Quaternion values:");
    ImGui::Text("w: %.3f, x: %.3f, y: %.3f, z: %.3f", q[0], q[1], q[2], q[3]);
    ImGui::End();
}

void RecordScene::InitBoard() {
    // Вершины платы (кубоид)
    float halfW = BOARD_WIDTH / 2.0f;
    float halfH = BOARD_HEIGHT / 2.0f;
    float halfT = BOARD_THICKNESS / 2.0f;

    std::vector<float> vertices = {
        // Передняя грань (красная)
        -halfW, -halfH,  halfT,  0.8f, 0.2f, 0.2f,
         halfW, -halfH,  halfT,  0.8f, 0.2f, 0.2f,
         halfW,  halfH,  halfT,  0.8f, 0.2f, 0.2f,
         halfW,  halfH,  halfT,  0.8f, 0.2f, 0.2f,
        -halfW,  halfH,  halfT,  0.8f, 0.2f, 0.2f,
        -halfW, -halfH,  halfT,  0.8f, 0.2f, 0.2f,

        // Задняя грань
        -halfW, -halfH, -halfT,  0.8f, 0.2f, 0.2f,
         halfW, -halfH, -halfT,  0.8f, 0.2f, 0.2f,
         halfW,  halfH, -halfT,  0.8f, 0.2f, 0.2f,
         halfW,  halfH, -halfT,  0.8f, 0.2f, 0.2f,
        -halfW,  halfH, -halfT,  0.8f, 0.2f, 0.2f,
        -halfW, -halfH, -halfT,  0.8f, 0.2f, 0.2f,

        // Левая грань
        -halfW,  halfH,  halfT,  0.6f, 0.1f, 0.1f,
        -halfW,  halfH, -halfT,  0.6f, 0.1f, 0.1f,
        -halfW, -halfH, -halfT,  0.6f, 0.1f, 0.1f,
        -halfW, -halfH, -halfT,  0.6f, 0.1f, 0.1f,
        -halfW, -halfH,  halfT,  0.6f, 0.1f, 0.1f,
        -halfW,  halfH,  halfT,  0.6f, 0.1f, 0.1f,

        // Правая грань
         halfW,  halfH,  halfT,  0.6f, 0.1f, 0.1f,
         halfW,  halfH, -halfT,  0.6f, 0.1f, 0.1f,
         halfW, -halfH, -halfT,  0.6f, 0.1f, 0.1f,
         halfW, -halfH, -halfT,  0.6f, 0.1f, 0.1f,
         halfW, -halfH,  halfT,  0.6f, 0.1f, 0.1f,
         halfW,  halfH,  halfT,  0.6f, 0.1f, 0.1f,

         // Нижняя грань
         -halfW, -halfH, -halfT,  0.5f, 0.1f, 0.1f,
          halfW, -halfH, -halfT,  0.5f, 0.1f, 0.1f,
          halfW, -halfH,  halfT,  0.5f, 0.1f, 0.1f,
          halfW, -halfH,  halfT,  0.5f, 0.1f, 0.1f,
         -halfW, -halfH,  halfT,  0.5f, 0.1f, 0.1f,
         -halfW, -halfH, -halfT,  0.5f, 0.1f, 0.1f,

         // Верхняя грань
         -halfW,  halfH, -halfT,  0.7f, 0.2f, 0.2f,
          halfW,  halfH, -halfT,  0.7f, 0.2f, 0.2f,
          halfW,  halfH,  halfT,  0.7f, 0.2f, 0.2f,
          halfW,  halfH,  halfT,  0.7f, 0.2f, 0.2f,
         -halfW,  halfH,  halfT,  0.7f, 0.2f, 0.2f,
         -halfW,  halfH, -halfT,  0.7f, 0.2f, 0.2f
    };

    // Создаем VAO и VBO для платы
    glGenVertexArrays(1, &boardVAO);
    glGenBuffers(1, &boardVBO);

    glBindVertexArray(boardVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boardVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Позиции
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Цвета
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void RecordScene::InitAxes() {
    // Вершины осей координат
    std::vector<float> vertices = {
        // Ось X (красная)
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        AXIS_LENGTH, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

        // Ось Y (зеленая)
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, AXIS_LENGTH, 0.0f, 0.0f, 1.0f, 0.0f,

        // Ось Z (синяя)
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, AXIS_LENGTH, 0.0f, 0.0f, 1.0f
    };

    // Создаем VAO и VBO для осей
    glGenVertexArrays(1, &axesVAO);
    glGenBuffers(1, &axesVBO);

    glBindVertexArray(axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Позиции
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Цвета
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void RecordScene::SetupCamera() {
    // Компиляция шейдеров
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Проверка ошибок компиляции вершинного шейдера
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Проверка ошибок компиляции фрагментного шейдера
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Создание шейдерной программы
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Проверка ошибок линковки
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Настройка матриц вида и проекции
    view = glm::lookAt(
        glm::vec3(3.0f, 2.0f, 3.0f), // Позиция камеры
        glm::vec3(0.0f, 0.0f, 0.0f), // Точка, на которую смотрим
        glm::vec3(0.0f, 0.0f, 1.0f)  // Вектор "вверх"
    );

    projection = glm::perspective(
        glm::radians(45.0f), // Угол обзора
        800.0f / 600.0f,     // Соотношение сторон
        0.1f,                // Ближняя плоскость отсечения
        50.0f                // Дальняя плоскость отсечения
    );

    // Включаем тест глубины
    glEnable(GL_DEPTH_TEST);
}