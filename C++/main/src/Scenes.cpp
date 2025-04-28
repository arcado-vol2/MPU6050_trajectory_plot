#include "Scenes.h"
#include "NoRenderScene.h"
#include "RecordScene.h"
#include "PlayScene.h"

std::unique_ptr<Scene> CreateScene(const std::string& sceneName, COM::Port* comPort) {
    if (sceneName == "NoRender") return std::make_unique<NoRenderScene>(comPort);
    if (sceneName == "Record") return std::make_unique<RecordScene>(comPort);
    if (sceneName == "Play") return std::make_unique<PlayScene>(comPort);
    return nullptr;
}

std::unique_ptr<Scene> CreateScene(SceneType type, COM::Port* comPort) {
    switch (type) {
    case SceneType::NORENDER: return std::make_unique<NoRenderScene>(comPort);
    case SceneType::RECORD: return std::make_unique<RecordScene>(comPort);
    case SceneType::PLAY: return std::make_unique<PlayScene>(comPort);
    default: return nullptr;
    }
}