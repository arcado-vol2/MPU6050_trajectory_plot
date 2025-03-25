#include "PlayScene.h"
#include "imgui.h"

void PlayScene::InitRender() {

}

void PlayScene::Render() {

    
}

void PlayScene::Update() {

}

void PlayScene::RenderUI() {
    ImGui::Begin("Green Square Scene");
    ImGui::Text("This scene shows a green square.");
    ImGui::Text("Square position: (0, 0)");
    ImGui::Text("Square size: 100x100");
    ImGui::End();
}