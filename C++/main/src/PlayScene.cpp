#include "PlayScene.h"
#include "imgui.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <thread>
#include <chrono>
#include <future>

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


PlayScene::~PlayScene() {
    if (calculationFuture.valid()) {
        calculationFuture.wait(); // Ожидаем завершения при уничтожении сцены
    }
}

void PlayScene::InitRender() {

}

void PlayScene::Render() {

    
}

void PlayScene::Update() {

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
        static double previousTime = 0.0;  // Используем double для большей точности
        static bool isFirstTime = true;    // Флаг для первого значения

        while (std::getline(iss, token, ',')) {
            if (column == 0) {
                std::tm tm = {};
                double microseconds = 0.0;
                char dot;

                std::istringstream timeStream(token);
                timeStream >> std::get_time(&tm, "%H:%M:%S") >> dot >> microseconds;

                if (timeStream.fail()) {
                    std::cerr << "Error parsing time in file: " << csvFilePath << std::endl;
                    continue;
                }

                // Корректное вычисление времени в секундах (с double)
                double currentTime = tm.tm_hour * 3600.0 + tm.tm_min * 60.0 + tm.tm_sec + microseconds / 1000000.0;

                // Обработка перехода через полночь (если currentTime < previousTime)
                if (currentTime < previousTime) {
                    currentTime += 24 * 3600;  // Добавляем 24 часа
                }

                double timeDifference = (isFirstTime) ? 0.0 : (currentTime - previousTime);
                ts.push_back(static_cast<float>(timeDifference));  // Можно оставить float, но вычислять в double

                previousTime = currentTime;
                isFirstTime = false;
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

    const float ax = a[0], ay = a[1], az = a[3];

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

void PlayScene::HighPassFilter(std::vector<float>& data, int i, float a0, float a1, float b0, float b1) {
    if (i <= 0 || i * 3 + 2 >= vs.size()) {
        std::cerr << "High pass filter error i:" << i << std::endl;
        return;
    }

    for (int ch = 0; ch < 3; ch++) {
        int idx = i * 3 + ch;

        float x = data[idx];
        float y = b0 * x + b1 * filterXPrev[ch] - a1 * filterYPrev[ch];

        filterXPrev[ch] = x;
        filterYPrev[ch] = y;
        data[idx] = y;
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

    //for 6.
    float sampleRate = 0.0f;

    //5. Velocity calculation

    vs.resize(as.size());
    for (int i = 0; i < ts.size(); i++) {
        ConvertAtoV(i);
        calculationProgress.store(calculationProgress + 1);
        sampleRate += ts[i];

    }
    sampleRate = 1.0f / (sampleRate / ts.size());

    //6. Drift compensation

    float nyQuist = 0.5f * sampleRate;
    float normalCutoff = std::clamp(filterCutoff / nyQuist, 0.01f, 0.99f);
    
    //High pass filter
    float alpha = tanf(M_PI * normalCutoff / 2.0f);
    float commonTerm = 1.0f / (1.0f + alpha);

    float b0 = 1.0f * commonTerm;
    float b1 = -1.0f * commonTerm;
    float a0 = 1.0f;
    float a1 = (1.0f - alpha) * commonTerm;


    for (int i = 1; i < ts.size(); i++) {
        HighPassFilter(vs, i, a0, a1, b0, b1);
        calculationProgress.store(calculationProgress + 1);
    }
    calculationProgress.store(calculationProgress + 1);
    

    //7. Position calculation
    pos.resize(vs.size());
    for (int i = 0; i < ts.size(); i++) {
        ConvertAtoV(i);
        calculationProgress.store(calculationProgress + 1);
    }

    std::cout << "Calc end\n";
    isCalculating = false;

    //8. Position filtration
    filterXPrev[3] = { 0 };
    filterYPrev[3] = { 0 };
    for (int i = 1; i < ts.size(); i++) {
        HighPassFilter(pos, i, a0, a1, b0, b1);
        calculationProgress.store(calculationProgress + 1);
    }
    calculationProgress.store(calculationProgress + 1);
}

#pragma region Integration_methods
void PlayScene::SquaresIntegration(int i, const std::vector<float>& input, std::vector<float>& output) {

    const float* prev_in = &input[(i - 1) * INTEGRATION_SIZE];
    const float* curr_in = &input[i * INTEGRATION_SIZE];
    float* prev_out = &output[(i - 1) * INTEGRATION_SIZE];
    float* curr_out = &output[i * INTEGRATION_SIZE];

    curr_out[0] = prev_out[0] + curr_in[0] * ts[i]; // X
    curr_out[1] = prev_out[1] + curr_in[1] * ts[i]; // Y
    curr_out[2] = prev_out[2] + curr_in[2] * ts[i]; // Z

}


#pragma endregion




void PlayScene::RenderUI() {
    int calcProgress = calculationProgress.load();
    bool isCalc = isCalculating.load();

    ImGui::Begin("Play Scene");
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


    if (csvFilePath == "") {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Start calculation") && !isCalc) {
        std::cout << "test\n";
        StartCalculation();
        
    }
    if (csvFilePath == "") {
        ImGui::EndDisabled();
    }

    

    ImGui::Text("Calculation progress:");
    ImGui::SameLine();
    ImGui::Text("%.2f",((float)calcProgress / (float)dataSize.load()) / ETAPS_COUNT * 100.0f);

    if (isCalc || calcProgress == 0) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Show animation")) {

    }
    if (isCalc || calcProgress == 0) {
        ImGui::EndDisabled();
    }
    ImGui::End();
    

}