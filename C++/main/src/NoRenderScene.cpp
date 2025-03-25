#include "NoRenderScene.h"
#include "imgui.h"


void NoRenderScene::InitRender() {

}


void NoRenderScene::Render() {

}

void NoRenderScene::Update() {

}

void NoRenderScene::RenderUI() {
    ImGui::Begin("No Render Scene");
    ImGui::Text("This scene does not render anything. ");
    ImGui::Text("It is needed so that you can make sure that the controller works correctly before starting other scenes");
    ImGui::Text("And also select the desired port and the desired method of operation.");
    ImGui::End();
}