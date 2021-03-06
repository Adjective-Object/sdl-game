#include <SDL.h>
#include "abstractrenderer.hpp"
#include "screenrenderer.hpp"
#include "engine/shader/screenspaceshader.hpp"
#include "engine/game.hpp"
#include "engine/gl.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

ScreenRenderer::ScreenRenderer(SDL_Texture* texture, ScreenSpaceQuad* mesh)
    : mesh(mesh) {
    material.setAmbientTexture(texture);
}

void ScreenRenderer::render(glm::mat4& baseTransform) {
    ScreenShader* shader = &screenShader;
    glUseProgram(shader->programId);

    // pass screen dimensions to the shader
    SDL_Rect screenBounds = EnG->getScreenBounds();
    glUniform2f(shader->uniforms.screenDimensions, screenBounds.w,
                screenBounds.h);

    // pass texture to the shader
    glActiveTexture(GL_TEXTURE0);
    SDL_GL_BindTexture(material.ambientTexture, NULL, NULL);
    glUniform1i(shader->uniforms.ambientTexture, 0);

    // pass vertex data to the shader
    glEnableVertexAttribArray(shader->attributes.pixelPosition);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBuffer);
    glVertexAttribPointer(shader->attributes.pixelPosition,
                          2,         // size
                          GL_FLOAT,  // type
                          GL_FALSE,  // normalized?
                          0,         // stride
                          (void*)0   // array buffer offset
                          );

    // pass uv data to she shader
    glEnableVertexAttribArray(shader->attributes.uvs);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->uvBuffer);
    glVertexAttribPointer(shader->attributes.uvs,
                          2,         // size
                          GL_FLOAT,  // type
                          GL_FALSE,  // normalized?
                          0,         // stride
                          (void*)0   // array buffer offset
                          );

    glDrawArrays(GL_TRIANGLES, 0, ScreenSpaceQuad::num_points);
    glDisableVertexAttribArray(shader->attributes.uvs);
    glDisableVertexAttribArray(shader->attributes.pixelPosition);
    glDisableVertexAttribArray(shader->uniforms.ambientTexture);
    glDisableVertexAttribArray(shader->uniforms.screenDimensions);
}
