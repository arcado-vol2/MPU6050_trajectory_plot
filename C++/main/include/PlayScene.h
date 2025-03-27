#pragma once
#ifndef PLAYSCENE_H
#define PLAYSCENE_H

#include "Scenes.h"

class PlayScene : public Scene {
public:
	PlayScene(COM::Port* comPort) : Scene(comPort) {}
	void Render() override;
	void Update() override;
	void RenderUI() override;
	void InitRender() override;
private:
	//scene methods
};

#endif // PLAYSCENE_H
