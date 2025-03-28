#include "RecordScene.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "UIStuff.h"
#include <csv.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <charconv>
#include <algorithm>

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

    //Board sizes
    BOARD_WIDTH = 0.6f;
    BOARD_HEIGHT = 1.0f;
    BOARD_THICKNESS = 0.1f;
    AXIS_LENGTH = 1.5f;

    
    q = { 1.0f, 0.0f, 0.0f, 0.0f };
    a = { 0.0f, 0.0f, 0.0f };

    //if somehow we forget to open com port
    if (p_comPort) {
        if (!p_comPort->Open()) {
            p_comPort->Open();
        }
    }

    popUp = UIStuff::PopUp();
}

RecordScene::~RecordScene() {
    glDeleteVertexArrays(1, &boardVAO);
    glDeleteBuffers(1, &boardVBO);

    glDeleteVertexArrays(1, &axesVAO);
    glDeleteBuffers(1, &axesVBO);

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

bool RecordScene::StartNewRecording() {
    if (csvFile.is_open()) {
        csvFile.close();
    }

    csvFilePath = GenerateCSVFIlePath();

    csvFile.open(csvFilePath);
    if (csvFile.is_open()) {
        csvFile << "t,w,x,y,z,ax,ay,az\n";
        return true;
    }
    else {
        popUp.ShowPopUp("Error", "Path for recording file not assigned!\n\nDon't forget to stop translation.");
        std::cerr << "error due file creating " << csvFilePath << std::endl;
        return false;
    }
}

void RecordScene::WriteToCSV() {
    if (csvFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);
        auto now_tm = *std::localtime(&now_time);

        //Microseconds
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()) % 1000000;

        //date time format: HOURS:MINUTS:SEC.MICROSE
        csvFile << std::put_time(&now_tm, "%H:%M:%S") << "."
            << std::setfill('0') << std::setw(6) << us.count() << ","
            << q[0] << "," << q[1] << "," << q[2] << "," << q[3] << ","
            << a[0] << "," << a[1] << "," << a[2] << "\n";
    }
    else {
        std::cerr << "error due file writing " << csvFilePath << std::endl;
    }
}

void RecordScene::StopRecording() {
    if (csvFile.is_open()) {
        csvFile.close();
    }
}




void RecordScene::Update() {
    if (!p_comPort || !p_comPort->IsOpen()) return;

    while (p_comPort->BytesAvailable() > 0) {
        try {
            std::string line = p_comPort->Read();
            
            if (line.empty()) continue;
            //we get some comand code
            if (line.length() == 2) {
                //Start translation code
                if (line[0] == '1') {
                    isRecording = StartNewRecording();
                    continue;
                }
                //End translation code
                else if (line[0] == '0') {
                    isRecording = false;
                    StopRecording();
                    popUp.ShowPopUp("Success", "File was succesfully saved!");
                    continue;
                }
            }
            
            if (isRecording) {

                float values[7];
                const char* ptr = line.data();
                const char* const end = ptr + line.size();
                int i = 0;

                while (i < 7 && ptr < end) {
                    const char* comma = std::strchr(ptr, ',');
                    if (i == 6) comma = end;
                    if (!comma && i != 6) break;
                    auto result = std::from_chars(ptr, comma, values[i]);
                    if (result.ec != std::errc() || (comma != end && *comma != ',')) {
                        break;
                    }
                    ptr = comma + 1;
                    i++;
                }

                if (i == 7) {
                    std::copy(values, values + 4, q.begin());
                    std::copy(values + 4, values + 7, a.begin());
                    WriteToCSV();
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Data parsing error: " << e.what() << std::endl;
        }
    }
}


void RecordScene::Render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    //3D model based on quarantion
    glm::mat4 model = glm::mat4(1.0f);
    float angle = 2.0f * acos(q[0]);
    float norm = sqrt(1.0f - q[0] * q[0]);
    if (norm > 0.001f) {
        norm = 1.0f / norm;
        glm::vec3 axis(q[1] * norm, q[2] * norm, q[3] * norm);
        model = glm::rotate(model, angle, axis);
    }
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    //draw board
    glBindVertexArray(boardVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    //draw axes
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

        savePath = UIStuff::OpenFolderDialog();
    }
    ImGui::Text("Selected path:");
    ImGui::SameLine();
    ImGui::Text("%s", savePath.c_str());
    ImGui::Separator();

    ImGui::Text("Quaternion values:");
    ImGui::Text("w: %.3f, x: %.3f, y: %.3f, z: %.3f", q[0], q[1], q[2], q[3]);

    ImGui::Text("Accel values:");
    ImGui::Text("x: %.3f, y: %.3f, z: %.3f", a[0], a[1], a[2]);
    ImGui::End();

    popUp.RenderPopUp();


}

void RecordScene::InitBoard() {
    float halfW = BOARD_WIDTH / 2.0f;
    float halfH = BOARD_HEIGHT / 2.0f;
    float halfT = BOARD_THICKNESS / 2.0f;

    std::vector<float> vertices = {
        //front
        -halfW, -halfH,  halfT,  0.8f, 0.2f, 0.2f,
         halfW, -halfH,  halfT,  0.8f, 0.2f, 0.2f,
         halfW,  halfH,  halfT,  0.8f, 0.2f, 0.2f,
         halfW,  halfH,  halfT,  0.8f, 0.2f, 0.2f,
        -halfW,  halfH,  halfT,  0.8f, 0.2f, 0.2f,
        -halfW, -halfH,  halfT,  0.8f, 0.2f, 0.2f,

        //back
        -halfW, -halfH, -halfT,  0.8f, 0.2f, 0.2f,
         halfW, -halfH, -halfT,  0.8f, 0.2f, 0.2f,
         halfW,  halfH, -halfT,  0.8f, 0.2f, 0.2f,
         halfW,  halfH, -halfT,  0.8f, 0.2f, 0.2f,
        -halfW,  halfH, -halfT,  0.8f, 0.2f, 0.2f,
        -halfW, -halfH, -halfT,  0.8f, 0.2f, 0.2f,

        //left
        -halfW,  halfH,  halfT,  0.6f, 0.1f, 0.1f,
        -halfW,  halfH, -halfT,  0.6f, 0.1f, 0.1f,
        -halfW, -halfH, -halfT,  0.6f, 0.1f, 0.1f,
        -halfW, -halfH, -halfT,  0.6f, 0.1f, 0.1f,
        -halfW, -halfH,  halfT,  0.6f, 0.1f, 0.1f,
        -halfW,  halfH,  halfT,  0.6f, 0.1f, 0.1f,

        //right
         halfW,  halfH,  halfT,  0.6f, 0.1f, 0.1f,
         halfW,  halfH, -halfT,  0.6f, 0.1f, 0.1f,
         halfW, -halfH, -halfT,  0.6f, 0.1f, 0.1f,
         halfW, -halfH, -halfT,  0.6f, 0.1f, 0.1f,
         halfW, -halfH,  halfT,  0.6f, 0.1f, 0.1f,
         halfW,  halfH,  halfT,  0.6f, 0.1f, 0.1f,

         //bottom
         -halfW, -halfH, -halfT,  0.5f, 0.1f, 0.1f,
          halfW, -halfH, -halfT,  0.5f, 0.1f, 0.1f,
          halfW, -halfH,  halfT,  0.5f, 0.1f, 0.1f,
          halfW, -halfH,  halfT,  0.5f, 0.1f, 0.1f,
         -halfW, -halfH,  halfT,  0.5f, 0.1f, 0.1f,
         -halfW, -halfH, -halfT,  0.5f, 0.1f, 0.1f,

         //top
         -halfW,  halfH, -halfT,  0.7f, 0.2f, 0.2f,
          halfW,  halfH, -halfT,  0.7f, 0.2f, 0.2f,
          halfW,  halfH,  halfT,  0.7f, 0.2f, 0.2f,
          halfW,  halfH,  halfT,  0.7f, 0.2f, 0.2f,
         -halfW,  halfH,  halfT,  0.7f, 0.2f, 0.2f,
         -halfW,  halfH, -halfT,  0.7f, 0.2f, 0.2f
    };

    glGenVertexArrays(1, &boardVAO);
    glGenBuffers(1, &boardVBO);

    glBindVertexArray(boardVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boardVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    //pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    //colors
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void RecordScene::InitAxes() {
    std::vector<float> vertices = {
        //X red
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        AXIS_LENGTH, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

        //Y green
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, AXIS_LENGTH, 0.0f, 0.0f, 1.0f, 0.0f,

        //Z blue
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, AXIS_LENGTH, 0.0f, 0.0f, 1.0f
    };

    glGenVertexArrays(1, &axesVAO);
    glGenBuffers(1, &axesVBO);

    glBindVertexArray(axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    //pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    //colors
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void RecordScene::SetupCamera() {
    //shader compilation
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


    view = glm::lookAt(
        glm::vec3(3.0f, 2.0f, 3.0f), //camera pos
        glm::vec3(0.0f, 0.0f, 0.0f), //view point
        glm::vec3(0.0f, 0.0f, 1.0f)  //top vector
    );

    projection = glm::perspective(
        glm::radians(45.0f), //FOV
        800.0f / 600.0f,     //aspect ratio
        0.1f,                //near
        50.0f                //far
    );

    glEnable(GL_DEPTH_TEST);
}