#pragma once
#ifndef PLAYSCENE_H
#define PLAYSCENE_H

#include "Scenes.h"
#include <atomic>
#include "UIStuff.h"

class PlayScene : public Scene {
public:
	PlayScene(COM::Port* comPort) : Scene(comPort), isCalculating(false), calculationProgress(0) {}
	void Render() override;
	void Update() override;
	void RenderUI() override;
	void InitRender() override;
private:
	


	void LoadData();
	void AddRotationMatrix(float w, float x, float y, float z);
	void Calculate();

	std::atomic<int> calculationProgress;
	std::atomic<bool> isCalculating;

	UIStuff::PopUp popUp;
	std::vector<float> qs; 
	std::vector<float> as;
	std::vector<float> Rs;


	std::string csvFilePath = "";

};

#endif // PLAYSCENE_H
