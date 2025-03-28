#pragma once
#ifndef RECORDSCENE_H
#define RECORDSCENE_H

#include "Scenes.h"
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include "UIStuff.h"

class RecordScene : public Scene {
public:
    RecordScene(COM::Port* comPort);
	~RecordScene() override;

	void Render() override;
	void Update() override;
	void RenderUI() override;
	void InitRender() override;

private:
	
    void InitBoard();
    void InitAxes();
    void SetupCamera();
    std::string GenerateCSVFIlePath();
    bool StartNewRecording();
    void WriteToCSV();
    void StopRecording();
    


    std::ofstream csvFile;
    std::string csvFilePath;
    std::string savePath = "";
    bool isRecording = false;

    

    //OpenGL stuff
    unsigned int shaderProgram;
    unsigned int boardVAO, boardVBO;
    unsigned int axesVAO, axesVBO;
    const char* vertexShaderSource;
    const char* fragmentShaderSource;

    glm::mat4 view;
    glm::mat4 projection;

    float BOARD_WIDTH;
    float BOARD_HEIGHT;
    float BOARD_THICKNESS;
    float AXIS_LENGTH;

    //data stuff
    std::vector<float> q;
    std::vector<float> a;

    UIStuff::PopUp popUp;
};

#endif // RECORDSCENE_H
