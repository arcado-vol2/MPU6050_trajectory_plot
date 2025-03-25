#pragma once
#ifndef NORENDERSCENE_H
#define NORENDERSCENE_H

#include "Scenes.h"

class NoRenderScene : public Scene {
public:
	NoRenderScene(COM::Port& comPort) : Scene(comPort) {}
	void Render() override;
	void Update() override;
	void RenderUI() override;
	void InitRender() override;
private:
	//scene methods
};

#endif // NORENDERSCENE_H
