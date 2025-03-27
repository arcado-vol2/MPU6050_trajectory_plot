#pragma once
#ifndef REALTIMESCENE_H
#define REALTIMESCENE_H

#include "Scenes.h"

class RealtimeScene : public Scene {
public:
	RealtimeScene(COM::Port* comPort) : Scene(comPort) {}
	void Render() override;
	void Update() override;
	void RenderUI() override;
	void InitRender() override;

private:
	//scene methods
};

#endif // REALTIMESCENE_H
