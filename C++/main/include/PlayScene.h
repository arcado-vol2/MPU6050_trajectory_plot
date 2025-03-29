#pragma once
#ifndef PLAYSCENE_H
#define PLAYSCENE_H

#include "Scenes.h"
#include <atomic>
#include "UIStuff.h"
#include <future>

#define Q_SIZE 4
#define A_SIZE 3
#define R_SIZE 9
#define T_SIZE 1
#define INTEGRATION_SIZE 3
#define ETAPS_COUNT 8.0f



class PlayScene : public Scene {
public:
	PlayScene(COM::Port* comPort) : Scene(comPort), isCalculating(false), calculationProgress(0) {}
	~PlayScene();
	void Render() override;
	void Update() override;
	void RenderUI() override;
	void InitRender() override;
private:
	
	const float g = 9.81f;
	const float gravityVector[3] = { 0.0f, 0.0f, 1.0f };
	const float filterCutoff = 0.1f;

	void LoadData();
	void AddRotationMatrix(float w, float x, float y, float z);
	void TiltCompensateA(int i);
	void CompensateGravity(int i);
	void ConvertAtoV(int i);
	void Integrate(int i, const std::vector<float>& input, std::vector<float>& output);
	void HighPassFilter(std::vector<float>& data, int i, float a0, float a1, float b0, float b1);
	void ConvertVtoPos(int i);
	
	//integration methods
	void SquaresIntegration(int i, const std::vector<float>& input, std::vector<float>& output);

	void StartCalculation();
	void Calculate();

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

	float filterXPrev[3] = { 0 };
	float filterYPrev[3] = { 0 };

	std::string csvFilePath = "";

	
	const char* integrationMethods[5] = {"Method of Squares", "Trapezoidal Rule", "Simpson's Rule", "Runge-Kutta Method", "Gaussian Quadrature"};
	const int integrationMethodsCount = sizeof(integrationMethods) / sizeof(integrationMethods[0]);
	int integrationMethodIndex = 0;
};

#endif // PLAYSCENE_H
