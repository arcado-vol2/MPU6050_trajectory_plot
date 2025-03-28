#pragma once
#ifndef RECORDSCENE_H
#define RECORDCENE_H

#include "Scenes.h"
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>

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
    void ShowPopUp(const std::string& label, const std::string& message);


    std::ofstream csvFile;
    std::string csvFilePath;
    std::string savePath = "";
    bool isRecording = false;

    bool isPopUpShowing = false;
    std::string popUpMessage;
    std::string popUpLabel;

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


};

#endif // RECORDSCENE_H
