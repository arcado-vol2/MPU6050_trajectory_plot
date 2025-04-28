#pragma once
#ifndef PLAYSCENE_H
#define PLAYSCENE_H

#include "Scenes.h"
#include <atomic>
#include "UIStuff.h"
#include <future>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define Q_SIZE 4
#define A_SIZE 3
#define R_SIZE 9
#define T_SIZE 1
#define INTEGRATION_SIZE 3
#define ETAPS_COUNT 8.0f



class PlayScene : public Scene {
public:
	PlayScene(COM::Port* comPort);
	~PlayScene();
	void Render() override;
	void Update() override;
	void RenderUI() override;
	void InitRender() override;
private:
	
	const float g = 9.81f;
	const float gravityVector[3] = { 0.0f, 0.0f, 1.0f };
	const double filterCutoff = 0.1;

	void LoadData();
	void AddRotationMatrix(float w, float x, float y, float z);
	void TiltCompensateA(int i);
	void CompensateGravity(int i);
	void ConvertAtoV(int i);
	void Integrate(int i, const std::vector<float>& input, std::vector<float>& output);
	void HighPass3DFilter(std::vector<float>& data, float sample_rate, float cutoff);
	void ConvertVtoPos(int i);
	void SaveToSCV(const std::vector<float>& data, int s, const char* header, const char* name);
	
	//integration methods
	void SquaresIntegration(int i, const std::vector<float>& input, std::vector<float>& output);
	void TrapezoidIntegration(int i, const std::vector<float>& input, std::vector<float>& output);
	void RungeKuttaIntegration(int i, const std::vector<float>& input, std::vector<float>& output);

	void StartCalculation();
	void Calculate();

	
	void InitCube();
	void InitAxes();
	void InitPoints();
	void SetupCamera();
	void CompileShaders();
	


	std::atomic<int> dataSize = 1;
	std::atomic<int> calculationProgress;
	std::atomic<bool> isCalculating;
	std::future<void> calculationFuture;

	UIStuff::PopUp popUp;
	std::vector<float> qs;
	std::vector<float> as;
	std::vector<float> Rs;
	std::vector<float> ts;
	std::vector<float> vs;
	std::vector<float> pos;

	std::string csvFilePath = "";
	std::string outputPath = "";
	bool saveCalculations = false;
	
	const char* integrationMethods[3] = {"Method of Squares", "Trapezoidal Rule", "Runge-Kutta Method"};
	const int integrationMethodsCount = sizeof(integrationMethods) / sizeof(integrationMethods[0]);
	int integrationMethodIndex = 0;

	bool isPlaying = false;


	// Graphics objects
	unsigned int cubeVAO, cubeVBO;
	unsigned int cubeAxesVAO, cubeAxesVBO;
	unsigned int axesVAO, axesVBO;
	unsigned int pointsVAO, pointsVBO;
	unsigned int shaderProgram;

	// Camera
	glm::mat4 view;
	glm::mat4 projection;
	float cameraAngleX = 0.0f;
	float cameraAngleY = 0.0f;
	float cameraAngleZ = 0.0f;
	float cameraDistance = 1.0f;

	// Dimensions
	const float CUBE_SIZE = 1.0f;
	const float AXIS_LENGTH = 3.5f;
	const float CUBE_AXIS_LENGTH = 0.2f;

	// Shader sources
	const char* vertexShaderSource;
	const char* fragmentShaderSource;

	glm::vec3 cubePosition;
	glm::mat3x3 cubeRotation;
	
	int currentFrame = 0;
};

#endif // PLAYSCENE_H
