#pragma once
#ifndef SCENE_H
#define SCENE_H


#include "ComPort.h"
class Scene {
public:
	Scene(COM::Port& comPort) : r_comPort(comPort) {}

	virtual ~Scene() = default;
	virtual void InitRender() = 0;
	virtual void Render() = 0;
	virtual void Update() = 0;
	virtual void RenderUI() = 0;
	
public:
	COM::Port& r_comPort;
	
};



#endif // SCENE_H