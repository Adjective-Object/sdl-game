#ifndef __ENGINE_MESH_RENDERER
#define __ENGINE_MESH_RENDERER
#include "abstractrenderer.hpp"
#include "engine/mesh/worldspacemesh.hpp"
#include "engine/material/material.hpp"
#include "engine/shader/meshshader.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <GL/gl.h>

class MeshRenderer : public AbstractRenderer {
    WorldspaceMesh* mesh;
    MeshShader* shader;
    Material* material;
    glm::mat4 modelTransform;

   public:
    MeshRenderer(MeshShader* shader,
                 WorldspaceMesh* mesh,
                 Material* material = NULL);
    virtual void render(glm::mat4& baseTransform) override;
    void setModelTransform(glm::mat4 newModelTransform);
};

#endif
