#include "PlayScene.h"
#include "imgui.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <thread>
#include <future>

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <glad/glad.h>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // !M_PI



PlayScene::PlayScene(COM::Port* comPort) : Scene(comPort), isCalculating(false), calculationProgress(0) {
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
}

PlayScene::~PlayScene() {
    if (calculationFuture.valid()) {
        calculationFuture.wait();
    }
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &axesVAO);
    glDeleteBuffers(1, &axesVBO);
    glDeleteVertexArrays(1, &pointsVAO);
    glDeleteBuffers(1, &pointsVBO);
    glDeleteProgram(shaderProgram);
}

void PlayScene::InitRender() {
    CompileShaders();
    InitCube();
    InitAxes();
    SetupCamera();
    
}

void PlayScene::Render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    // Update camera view matrix based on rotation and zoom
    glm::vec3 cameraPos = glm::vec3(3.0f, 2.0f, 3.0f) * cameraDistance;
    glm::mat4 view = glm::lookAt(
        cameraPos,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    view = glm::rotate(view, glm::radians(cameraAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, glm::radians(cameraAngleY), glm::vec3(0.0f, 1.0f, 0.0f));
    view = glm::rotate(view, glm::radians(cameraAngleZ), glm::vec3(0.0f, 0.0f, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Draw coordinate axes (white)
    glm::mat4 axisModel = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(axisModel));
    glBindVertexArray(axesVAO);
    glDrawArrays(GL_LINES, 0, 6);

    // Draw cube with rotation axes (if rotation data available)
    if (!Rs.empty()) {
        glm::mat4 cubeModel = glm::mat4(1.0f);
        // Convert 3x3 matrix to glm::mat4
        glm::mat3 rotationMat(
            Rs[0], Rs[1], Rs[2],
            Rs[3], Rs[4], Rs[5],
            Rs[6], Rs[7], Rs[8]
        );
        cubeModel = glm::mat4(rotationMat);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Draw cube's local axes (red, green, blue)
        glBindVertexArray(axesVAO);
        glDrawArrays(GL_LINES, 0, 6);
    }

    // Draw points from pos array
    if (!pos.empty() && isPlaying) {
        glm::mat4 pointsModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(pointsModel));
        glBindVertexArray(pointsVAO);
        glDrawArrays(GL_POINTS, 0, pos.size() / 3);
    }

    glBindVertexArray(0);
}

void PlayScene::Update() {
    // Handle camera rotation with WASD
    const float rotationSpeed = 2.0f;
    const float zoomSpeed = 0.1f;

    if (GetAsyncKeyState('A') & 0x8000) {
        cameraAngleY -= rotationSpeed;
    }
    if (GetAsyncKeyState('D') & 0x8000) {
        cameraAngleY += rotationSpeed;
    }
    if (GetAsyncKeyState('W') & 0x8000) {
        cameraAngleX -= rotationSpeed;
    }
    if (GetAsyncKeyState('S') & 0x8000) {
        cameraAngleX += rotationSpeed;
    }
    if (GetAsyncKeyState('Q') & 0x8000) {
        cameraAngleZ -= rotationSpeed;
    }
    if (GetAsyncKeyState('E') & 0x8000) {
        cameraAngleZ += rotationSpeed;
    }
    // Zoom in/out with X/Z
    if (GetAsyncKeyState('X') & 0x8000) {
        cameraDistance = std::max(0.1f, cameraDistance - zoomSpeed);
    }
    if (GetAsyncKeyState('Z') & 0x8000) {
        cameraDistance += zoomSpeed;
    }

    // Keep angles in 0-360 range
    cameraAngleX = fmod(cameraAngleX, 360.0f);
    cameraAngleY = fmod(cameraAngleY, 360.0f);
    cameraAngleZ = fmod(cameraAngleZ, 360.0f);
}

void PlayScene::LoadData() {
    ts.clear();
    qs.clear();
    as.clear();

    std::ifstream file(csvFilePath);
    if (!file.is_open()) {
        std::cerr << "Error due file opening " << csvFilePath << std::endl;
        return;
    }

    std::string line;
    bool firstLine = true;


    dataSize.store(0);

    while (std::getline(file, line)) {
        if (firstLine) {
            firstLine = false;
            continue;
        }

        std::istringstream iss(line);
        std::string token;
        int column = 0;
        static bool isFirstTime = true;    

        while (std::getline(iss, token, ',')) {
            // 0 is delta time
            if (column == 0) {
                try {
                    ts.push_back(std::stoull(token) / 1000.0f);
                    isFirstTime = false;
                }
                catch (...) {
                    std::cerr << "Error parsing time (microseconds) in file: " << csvFilePath << std::endl;
                    ts.push_back(0.0f);
                }
            }
            else if (column >= T_SIZE && column <= Q_SIZE) {
                // Columns 1-4 go to qs
                try {
                    float value = std::stof(token);
                    qs.push_back(value);

                }
                catch (...) {
                    std::cerr << "Quarantion conversion error " << token << std::endl;
                    qs.push_back(0.0f);
                }
            }
            else {
                // Remaining columns go to as
                try {
                    float value = std::stof(token);
                    as.push_back(value);
                }
                catch (...) {
                    std::cerr << "Acceliration conversion error " << token << std::endl;
                    as.push_back(0.0f);
                }
            }

            column++;
            
        }
        dataSize.store(dataSize + 1);
    }

    calculationProgress.store(dataSize);

    file.close();
   
    std::cout << "Data loaded: q count: " << qs.size() / Q_SIZE << " a count: " << as.size() / A_SIZE << " t count: " << ts.size() << std::endl;

}

void PlayScene::AddRotationMatrix(float w, float x, float y, float z) {
    //quaternions to rotation matrix 
    float R[9] =
    {
        1.0f - 2.0f * pow(y, 2) - 2.0f * pow(z, 2),   2.0f * x * y - 2.0f * z * w,                  2.0f * x * z + 2.0f * y * w,
        2.0f * x * y + 2.0f * z * w,                  1.0f - 2.0f * pow(x, 2) - 2.0f * pow(z, 2),   2.0f * y * z - 2.0f * x * w,
        2.0f * x * z - 2.0f * y * w,                  2.0f * y * z + 2.0f * x * w,                  1.0f - 2.0f * pow(x, 2) - 2.0f * pow(y, 2)

    };
    Rs.insert(Rs.end(), R, R + R_SIZE);
}

void PlayScene::TiltCompensateA(int i) {
    const float* R = &Rs[i * R_SIZE];
    float* a = &as[i * A_SIZE];

    const float ax = a[0], ay = a[1], az = a[2];

    // R x a 
    a[0] = R[0] * ax + R[1] * ay + R[2] * az;  
    a[1] = R[3] * ax + R[4] * ay + R[5] * az;  
    a[2] = R[6] * ax + R[7] * ay + R[8] * az; 
}

void PlayScene::CompensateGravity(int i) {
    //basicly here first two row: value - 0, but what if gravity direction will change?
    float* a = &as[i * A_SIZE];
    a[0] -= gravityVector[0];
    a[1] -= gravityVector[1];
    a[2] -= gravityVector[2];

    //convert to metrs per second
    a[0] *= g;
    a[1] *= g;
    a[2] *= g;
}

void PlayScene::ConvertAtoV(int i) {
    Integrate(i, as, vs);
}

void PlayScene::Integrate(int i, const std::vector<float>& input, std::vector<float>& output) {
    if (i == 0) return;
    switch (integrationMethodIndex)
    {
    case 0:
        SquaresIntegration(i, input, output);
        break;
    default:
        SquaresIntegration(i, input, output);
        break;
    }
}

void PlayScene::HighPass3DFilter(std::vector<float>& data, float sample_rate, float cutoff) {


    // Buttower coef 1 order
    double tan_wc = std::tan(M_PI * cutoff);
    double b0 = 1.0 / (1.0 + tan_wc);
    double b1 = -b0;
    double a1 = (tan_wc - 1.0) / (tan_wc + 1.0);

    int num_columns = 3;
    size_t num_rows = data.size() / num_columns;
    for (size_t col = 0; col < num_columns; ++col) {
        std::vector<double> column(num_rows);
        for (size_t i = 0; i < num_rows; ++i) {
            column[i] = data[i * num_columns + col];
        }

        // Straight pass
        std::vector<double> forward(num_rows);
        double prev_input = 0.0, prev_output = 0.0;
        for (size_t i = 0; i < num_rows; ++i) {
            forward[i] = b0 * column[i] + b1 * prev_input - a1 * prev_output;
            prev_input = column[i];
            prev_output = forward[i];
        }

        // Return pass (like filtfilt)
        std::vector<double> backward(num_rows);
        prev_input = 0.0;
        prev_output = 0.0;
        for (int i = num_rows - 1; i >= 0; --i) {
            backward[i] = b0 * forward[i] + b1 * prev_input - a1 * prev_output;
            prev_input = forward[i];
            prev_output = backward[i];
        }

        // Saving
        for (size_t i = 0; i < num_rows; ++i) {
            data[i * num_columns + col] = backward[i];
        }
    }

}

void PlayScene::ConvertVtoPos(int i) {
    Integrate(i, vs, pos);
}

void PlayScene::StartCalculation() {
    std::cout << "Calc start\n";
    calculationFuture = std::async(std::launch::async, &PlayScene::Calculate, this);

}


void PlayScene::Calculate() {

    calculationProgress = 0;
    isCalculating = true;

    //1. Load of quaternions and accelerometer data
    LoadData();
    if (qs.size() < 4) {
        std::cerr << "Not enough data!\n";
        return;
    }

    //2. Calculate rotation matrices
    for (int i = 0; i < qs.size(); i += Q_SIZE) {
        float w = qs[i];
        float x = qs[i + 1];
        float y = qs[i + 2];
        float z = qs[i + 3];

        AddRotationMatrix(w, x, y, z);
        calculationProgress.store(calculationProgress + 1);
    }

    //3. Tilt compensation
    for (int i = 0; i < ts.size(); i++) {
        TiltCompensateA(i);
        calculationProgress.store(calculationProgress + 1);
    }

    //4. Convert to linear velocity
    for (int i = 0; i < ts.size(); i++) {
        CompensateGravity(i);
        calculationProgress.store(calculationProgress + 1);
    }

    //5. Velocity calculation
    double sampleRate = 0.0; //for 6.
    vs.resize(as.size());
    vs[0] = vs[1] = vs[2] = 0.0f;
    for (int i = 0; i < ts.size(); i++) {
        ConvertAtoV(i);
        calculationProgress.store(calculationProgress + 1);
        sampleRate += ts[i];

    }
    sampleRate = 1.0 / (sampleRate / ts.size());

    //6. Drift compensation
    double nyQuist = 0.5f * sampleRate;
    double normalCutoff = filterCutoff / nyQuist;
    HighPass3DFilter(vs, sampleRate, normalCutoff);
    calculationProgress.store(calculationProgress + dataSize.load());

    //7. Position calculation
    pos.resize(vs.size());
    for (int i = 0; i < ts.size(); i++) {
        ConvertVtoPos(i);
        calculationProgress.store(calculationProgress + 1);
    }
    

    //8. Position filtration
    HighPass3DFilter(pos, sampleRate, normalCutoff);
    calculationProgress.store(calculationProgress + dataSize.load());
    std::cout << "Calc end\n";
    isCalculating = false;
}

#pragma region Integration_methods
void PlayScene::SquaresIntegration(int i, const std::vector<float>& input, std::vector<float>& output) {
    if (i == 0) {
        return;
    }
    output[i * 3] = output[(i - 1) * 3] + input[i * 3] * ts[i];
    output[i * 3 + 1] = output[(i - 1) * 3 + 1] + input[i * 3 + 1] * ts[i];
    output[i * 3 + 2] = output[(i - 1) * 3 + 2] + input[i * 3 + 2] * ts[i];
}

#pragma endregion


void PlayScene::RenderUI() {
    int calcProgress = calculationProgress.load();
    bool isCalc = isCalculating.load();

    ImGui::Begin("Play Scene");
    if (isCalc) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Choose data file")) {

        csvFilePath = UIStuff::OpenFileDialog(L"*.txt;*.csv");
        calculationProgress.store(0);
    }
    ImGui::Text("Selected path:");
    ImGui::SameLine();
    ImGui::Text("%s", csvFilePath.c_str());

    ImGui::Separator();

    ImGui::Text("Select integration method");
    if (ImGui::BeginCombo("Select method", integrationMethods[integrationMethodIndex]) ){
        for (int i = 0; i < integrationMethodsCount; i++) {
            bool isSelected = (integrationMethodIndex == i);
            if (ImGui::Selectable(integrationMethods[i], isSelected)) {
                integrationMethodIndex = i;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (isCalc) {
        ImGui::EndDisabled();
    }

    if (csvFilePath == "" || isPlaying) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Start calculation") && !isCalc) {
        std::cout << "test\n";
        StartCalculation();
        
    }
    if (csvFilePath == "" || isPlaying) {
        ImGui::EndDisabled();
    }

    

    ImGui::Text("Calculation progress:");
    ImGui::SameLine();
    ImGui::Text("%.2f",((float)calcProgress / (float)dataSize.load()) / ETAPS_COUNT * 100.0f);

    if (isCalc || calcProgress == 0) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(isPlaying ? "Stop animation": "Show animation")) {
        if (!isPlaying)
            InitPoints();
        isPlaying = !isPlaying;
    }
    if (isCalc || calcProgress == 0) {
        ImGui::EndDisabled();
    }
    ImGui::End();
    



}


void PlayScene::CompileShaders() {
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
}

void PlayScene::SetupCamera() {
    view = glm::lookAt(
        glm::vec3(3.0f, 2.0f, 3.0f), // camera position
        glm::vec3(0.0f, 0.0f, 0.0f), // look at origin
        glm::vec3(0.0f, 0.0f, 1.0f)  // up vector
    );

    projection = glm::perspective(
        glm::radians(45.0f), // FOV
        800.0f / 600.0f,    // aspect ratio
        0.1f,               // near plane
        100.0f              // far plane
    );

    glEnable(GL_DEPTH_TEST);
    glPointSize(5.0f); // Set point size for position markers
}

void PlayScene::InitPoints() {
    if (pos.empty()) return;

    // Create vector with positions and colors (yellow)
    std::vector<float> vertices;
    for (size_t i = 0; i < pos.size(); i += 3) {
        std::cout << pos[i] << " " << pos[i+1] << " " << pos[i+2] << "\n ";
        vertices.push_back(pos[i] * 10.0f);
        vertices.push_back(pos[i + 1] * 10.0f);
        vertices.push_back(pos[i + 2] * 10.0f);
        vertices.push_back(1.0f); // R
        vertices.push_back(1.0f); // G
        vertices.push_back(0.0f); // B
    }

    glGenVertexArrays(1, &pointsVAO);
    glGenBuffers(1, &pointsVBO);

    glBindVertexArray(pointsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PlayScene::InitCube() {
    float halfSize = CUBE_SIZE / 32.0f;

    std::vector<float> vertices = {
        // Front face (red)
        -halfSize, -halfSize,  halfSize,  0.8f, 0.2f, 0.2f,
         halfSize, -halfSize,  halfSize,  0.8f, 0.2f, 0.2f,
         halfSize,  halfSize,  halfSize,  0.8f, 0.2f, 0.2f,
         halfSize,  halfSize,  halfSize,  0.8f, 0.2f, 0.2f,
        -halfSize,  halfSize,  halfSize,  0.8f, 0.2f, 0.2f,
        -halfSize, -halfSize,  halfSize,  0.8f, 0.2f, 0.2f,

        // Back face (red)
        -halfSize, -halfSize, -halfSize,  0.8f, 0.2f, 0.2f,
         halfSize, -halfSize, -halfSize,  0.8f, 0.2f, 0.2f,
         halfSize,  halfSize, -halfSize,  0.8f, 0.2f, 0.2f,
         halfSize,  halfSize, -halfSize,  0.8f, 0.2f, 0.2f,
        -halfSize,  halfSize, -halfSize,  0.8f, 0.2f, 0.2f,
        -halfSize, -halfSize, -halfSize,  0.8f, 0.2f, 0.2f,

        // Left face (darker red)
        -halfSize,  halfSize,  halfSize,  0.6f, 0.1f, 0.1f,
        -halfSize,  halfSize, -halfSize,  0.6f, 0.1f, 0.1f,
        -halfSize, -halfSize, -halfSize,  0.6f, 0.1f, 0.1f,
        -halfSize, -halfSize, -halfSize,  0.6f, 0.1f, 0.1f,
        -halfSize, -halfSize,  halfSize,  0.6f, 0.1f, 0.1f,
        -halfSize,  halfSize,  halfSize,  0.6f, 0.1f, 0.1f,

        // Right face (darker red)
         halfSize,  halfSize,  halfSize,  0.6f, 0.1f, 0.1f,
         halfSize,  halfSize, -halfSize,  0.6f, 0.1f, 0.1f,
         halfSize, -halfSize, -halfSize,  0.6f, 0.1f, 0.1f,
         halfSize, -halfSize, -halfSize,  0.6f, 0.1f, 0.1f,
         halfSize, -halfSize,  halfSize,  0.6f, 0.1f, 0.1f,
         halfSize,  halfSize,  halfSize,  0.6f, 0.1f, 0.1f,

         // Bottom face (darkest red)
         -halfSize, -halfSize, -halfSize,  0.5f, 0.1f, 0.1f,
          halfSize, -halfSize, -halfSize,  0.5f, 0.1f, 0.1f,
          halfSize, -halfSize,  halfSize,  0.5f, 0.1f, 0.1f,
          halfSize, -halfSize,  halfSize,  0.5f, 0.1f, 0.1f,
         -halfSize, -halfSize,  halfSize,  0.5f, 0.1f, 0.1f,
         -halfSize, -halfSize, -halfSize,  0.5f, 0.1f, 0.1f,

         // Top face (bright red)
         -halfSize,  halfSize, -halfSize,  0.7f, 0.2f, 0.2f,
          halfSize,  halfSize, -halfSize,  0.7f, 0.2f, 0.2f,
          halfSize,  halfSize,  halfSize,  0.7f, 0.2f, 0.2f,
          halfSize,  halfSize,  halfSize,  0.7f, 0.2f, 0.2f,
         -halfSize,  halfSize,  halfSize,  0.7f, 0.2f, 0.2f,
         -halfSize,  halfSize, -halfSize,  0.7f, 0.2f, 0.2f
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PlayScene::InitAxes() {
    std::vector<float> vertices = {
        // X axis (red)
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        AXIS_LENGTH, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

        // Y axis (green)
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, AXIS_LENGTH, 0.0f, 0.0f, 1.0f, 0.0f,

        // Z axis (blue)
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, AXIS_LENGTH, 0.0f, 0.0f, 1.0f
    };

    glGenVertexArrays(1, &axesVAO);
    glGenBuffers(1, &axesVBO);

    glBindVertexArray(axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
