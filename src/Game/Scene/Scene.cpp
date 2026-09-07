#include "Game/Scene/Scene.hpp"

#include "Game/Scene/SceneObjHolder.hpp"

Scene::Scene(const char* pName) : NerveExecutor(pName), mListExecutor(nullptr), _C(0), mSceneObjHolder(nullptr) {
}

Scene::~Scene() {
    delete mSceneObjHolder;
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
    mListExecutor = nullptr;
}

void Scene::initSceneObjHolder() {
    mSceneObjHolder = new SceneObjHolder();
}
