#include <iostream>
#include <algorithm>

#include <SDL.h>
#include "scene.hpp"
#include "entity.hpp"

Scene::Scene() {
    entities = std::vector<Entity *>();
}

Scene::~Scene() {
    for(Entity * e: entities) {
        delete e;
    }
}

void Scene::init() {
    // init all the entities
    for (Entity * e : this->entities) {
        e->init();
    }
}

void Scene::start() {
}

void Scene::update() {
    // update all the entities
    for (Entity * e : this->entities) {
        e->update();
    }
}

void Scene::render(SDL_Renderer * r) {
    for (Entity * e: this->entities) {
        e->render(r);
    }

}


