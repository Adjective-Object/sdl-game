#ifndef __ENGINE_LOADEDMESH_H
#define __ENGINE_LOADEDMESH_H

#include <engine/model/modelmesh.hpp>
#include <mesh.h>

/**
 * Helper class for modelloader to hold an assimp mesh & the associated
 * internal mesh
 */
class LoadedMesh {
   public:
    LoadedMesh(ModelMesh, const aiMesh*);
    ModelMesh loadedMesh;
    const aiMesh* sourceMesh;
};

#endif