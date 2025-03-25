#pragma once
#ifndef SCENES_H
#define SCENES_H

#include "scene.h"
#include <memory>

class NoRenderScene;
class RecordScene;
class PlayScene;
class RealTimeScene;

std::unique_ptr<Scene> CreateScene(const std::string& sceneName, COM::Port& comPort);

enum class SceneType {
    NORENDER,
    RECORD,
    PLAY,
    REALTIME,
};

#endif // SCENES_H