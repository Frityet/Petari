#include "Game/Scene/Scene.hpp"
#include "Game/Scene/SceneNameObjListExecutor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "scene/SceneLifetimeBinding.hpp"
#include <memory>

Scene::Scene(const char *pName) : NerveExecutor(pName) {
    mListExecutor = nullptr;
    _C = 0;
    mSceneObjHolder = nullptr;
}

Scene::~Scene() {
    smgpc::scene::retire_scene_services(*this);
    if (mSceneObjHolder != nullptr) {
        delete mSceneObjHolder;
    }

    if (mListExecutor != nullptr) {
        delete mListExecutor;
    }
}

void Scene::init() {
}

void Scene::start() {
}

void Scene::update() {
}

void Scene::draw() const {
}

void Scene::calcAnim() {
}

void Scene::initNameObjListExecutor() {
    auto exec = std::make_unique<SceneNameObjListExecutor>();
    exec->init();
    mListExecutor = exec.release();
}

void Scene::initSceneObjHolder() {
    mSceneObjHolder = new SceneObjHolder();
}