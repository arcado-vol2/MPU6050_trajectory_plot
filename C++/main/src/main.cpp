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

#include "ComPort.h"
#include <iostream>
#include <memory>
#include <vector>
#include <chrono>

#include "Scene.h"
#include "NoRenderScene.h"
#include "RecordScene.h"
#include "PlayScene.h"

#define TARGET_FPS 60

const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

std::unique_ptr<COM::Port> currentPort;
std::vector<std::string> comPorts;
int selectedPortIndex = -1;

void SetupCurrentPort(const std::string& name){
    if (currentPort) {
        currentPort->Close();
    }
    currentPort = std::make_unique<COM::Port>(name, 115200);
    
    if (!currentPort->IsAvailable()) {
        currentPort.reset(); 
        selectedPortIndex = -1;
    }
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
    case SceneType::NORENDER: return std::make_unique<NoRenderScene>(currentPort.get());
    case SceneType::RECORD: return std::make_unique<RecordScene>(currentPort.get());
    case SceneType::PLAY: return std::make_unique<PlayScene>(currentPort.get());
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
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);


    GLFWwindow* window = glfwCreateWindow(1280, 960, "Visualizer", NULL, NULL);
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

    io.FontGlobalScale = 1.4f;

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
    const char* sceneItems[] = { "No Render", "Record", "Play"};
    UpdateAvailablePorts();

    while (!glfwWindowShouldClose(window))
    {
        auto frameStart = std::chrono::high_resolution_clock::now();
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
       
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        if (currentScene) {
            currentScene->Update();
            currentScene->Render();
        }

#pragma region imgui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
#pragma endregion

        if (currentScene) {
            currentScene->RenderUI();
        }   

#pragma region imgui_general
        ImGui::Begin("Main control");


        

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
        
        ImGui::Separator();

        ImGui::Text("Select scene to render:");
        int currentSceneIndex = static_cast<int>(currentSceneType);
        if (ImGui::Combo("Scene", &currentSceneIndex, sceneItems, IM_ARRAYSIZE(sceneItems)))
        {
            SceneType newSceneType = static_cast<SceneType>(currentSceneIndex);


            if (currentPort != nullptr || newSceneType == SceneType::NORENDER || newSceneType == SceneType::PLAY) {
                if (currentPort != nullptr && currentPort->IsOpen())
                    currentPort->Close();

                if (newSceneType != currentSceneType) {
                    currentSceneType = newSceneType;
                    currentScene = CreateScene(currentSceneType);
                    if (currentScene) {
                        currentScene->InitRender();
                    }
                }
            }
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


        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> frameDuration = frameEnd - frameStart;
        double elapsedTime = frameDuration.count();

        if (elapsedTime < TARGET_FRAME_TIME) {
            std::this_thread::sleep_for(std::chrono::duration<double>(TARGET_FRAME_TIME - elapsedTime));
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}