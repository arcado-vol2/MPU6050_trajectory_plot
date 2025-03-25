#pragma once
#ifndef RECORDSCENE_H
#define RECORDCENE_H

#include "Scenes.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>

class RecordScene : public Scene {
public:
	RecordScene(COM::Port& comPort);
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
    void StartNewRecording();
    void WriteQToCSV();
    void StopRecording();

    std::ofstream csvFile;
    std::string csvFilePath;
    std::string savePath = "";
    bool isRecording = false;

    // Шейдерные программы и буферы
    unsigned int shaderProgram;
    unsigned int boardVAO, boardVBO;
    unsigned int axesVAO, axesVBO;
    const char* vertexShaderSource;
    const char* fragmentShaderSource;

    // Матрицы для преобразований
    glm::mat4 view;
    glm::mat4 projection;

    // Параметры платы и осей
    float BOARD_WIDTH;
    float BOARD_HEIGHT;
    float BOARD_THICKNESS;
    float AXIS_LENGTH;

    // Кватернион для ориентации (w, x, y, z)
    std::vector<float> q;


};

#endif // RECORDSCENE_H
