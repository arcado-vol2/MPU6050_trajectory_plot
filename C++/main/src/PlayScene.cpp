#include "PlayScene.h"
#include "imgui.h"
#include <iostream>



void PlayScene::InitRender() {

}

void PlayScene::Render() {

    
}

void PlayScene::Update() {

}

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

    if (csvFilePath == "") {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Start calculation") && !isCalc) {
        std::cout << "test\n";
        calculationProgress.store(100);
        
    }
    if (csvFilePath == "") {
        ImGui::EndDisabled();
    }

    

    ImGui::Text("Calculation progress:");
    ImGui::SameLine();
    ImGui::Text("%d", calcProgress);

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