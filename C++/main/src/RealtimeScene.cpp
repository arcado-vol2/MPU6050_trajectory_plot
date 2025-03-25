#include "RealtimeScene.h"
#include "imgui.h"

void RealtimeScene::InitRender() {

}

void RealtimeScene::Render() {

}

void RealtimeScene::Update() {

}

void RealtimeScene::RenderUI() {
    ImGui::Begin("Blue Square Scene");
    ImGui::Text("This scene shows a blue square.");
    ImGui::Text("Square position: (0, 0)");
    ImGui::Text("Square size: 100x100");
    ImGui::End();
}