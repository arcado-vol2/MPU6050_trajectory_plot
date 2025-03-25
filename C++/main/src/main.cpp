#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <openglErrorReporting.h>
#include <future>


#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imguiThemes.h"

#include "csv.h"
#include "ComPort.h"
#include <iostream>
#include <memory>
#include <vector>

#include "Scene.h"
#include "NoRenderScene.h"
#include "RecordScene.h"
#include "PlayScene.h"
#include "RealtimeScene.h"


COM::Port currentPort;
std::vector<std::string> comPorts;
int selectedPortIndex = -1;

void SetupCurrentPort(const std::string& name){
    currentPort.close();
    currentPort = COM::Port(name, CBR_115200);
}

void UpdateAvailablePorts() {
    comPorts = COM::GetAvailableComPorts();
}

static void error_callback(int error, const char* description)
{
    std::cout << "Error: " << description << "\n";
}


std::unique_ptr<Scene> CreateScene(SceneType type) {
    switch (type) {
    case SceneType::NORENDER: return std::make_unique<NoRenderScene>(currentPort);
    case SceneType::RECORD: return std::make_unique<RecordScene>(currentPort);
    case SceneType::PLAY: return std::make_unique<PlayScene>(currentPort);
    case SceneType::REALTIME: return std::make_unique<RealtimeScene>(currentPort);
    default: return nullptr;
    }
}

int main(void)
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //you might want to do this when testing



    GLFWwindow* window = glfwCreateWindow(640, 480, "Visualizer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }



    enableReportGlErrors();
    glfwSwapInterval(1);
    //glfwSwapInterval(1); //vsync

#pragma region imgui
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    imguiThemes::red();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    io.FontGlobalScale = 1.2f;

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        //style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 0.f;
        style.Colors[ImGuiCol_DockingEmptyBg].w = 0.f;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
#pragma endregion



    SceneType currentSceneType = SceneType::NORENDER;
    auto currentScene = CreateScene(currentSceneType);
    const char* sceneItems[] = { "No Render", "Record", "Play", "Realtime" };
    UpdateAvailablePorts();

    while (!glfwWindowShouldClose(window))
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
       
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        if (currentScene) {
            currentScene->Render();
        }

        auto updateFuture = std::async(std::launch::async, [&]() {
            if (currentScene) {
                currentScene->Update();
            }
        }
        );

#pragma region imgui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
#pragma endregion

        if (currentScene) {
            currentScene->RenderUI();
        }
        updateFuture.wait();

#pragma region imgui_general
        ImGui::Begin("Main control");
        ImGui::Text("Select scene to render:");
        int currentSceneIndex = static_cast<int>(currentSceneType);
        if (ImGui::Combo("Scene", &currentSceneIndex, sceneItems, IM_ARRAYSIZE(sceneItems)))
        {
            currentPort.close();
            SceneType newSceneType = static_cast<SceneType>(currentSceneIndex);
            if (newSceneType != currentSceneType) {
                currentSceneType = newSceneType;
                currentScene = CreateScene(currentSceneType);
                if (currentScene) {
                    currentScene->InitRender();
                }
            }
        }
        ImGui::Separator();


        ImGui::Text("COM port controls");
        if (ImGui::Button("Refresh COM ports")) {
            UpdateAvailablePorts();
        }
        
        if (ImGui::BeginCombo("port", selectedPortIndex == -1 ? "None" : comPorts[selectedPortIndex].c_str())) {
            for (size_t i = 0; i < comPorts.size(); i++) {
                bool isSelected = (selectedPortIndex == i);
                if (ImGui::Selectable(comPorts[i].c_str(), isSelected)) {
                    selectedPortIndex = i;
                    SetupCurrentPort(comPorts[selectedPortIndex]);
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::End();
#pragma endregion

        
        

      

#pragma region imgui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

#pragma endregion

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}