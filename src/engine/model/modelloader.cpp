// clib
#include <cstring>
#include <iostream>
#include <random>
#include <set>
#include <string>
// sdl
#include <SDL.h>
#include <SDL_image.h>
// assimp
#include <Importer.hpp>
#include <scene.h>
#include <postprocess.h>
#include <quaternion.h>
// locals
#include "engine/mesh/worldspacemesh.hpp"
#include "engine/material/material.hpp"
#include "modelloader.hpp"
#include "engine/meshanim/meshanim.hpp"

using namespace std;
using namespace Assimp;

ModelLoader::ModelLoader(SDL_Renderer* renderCtx) : renderCtx(renderCtx) {}

bool ModelLoader::load(const char* fpath) {
    std::cout << "load(" << fpath << ")" << std::endl;

    scene = importer.ReadFile(
        fpath, aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                   aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    return scene != NULL;
}

LoadedMesh::LoadedMesh(ModelMesh m, const aiMesh* a)
    : loadedMesh(m), sourceMesh(a) {}

#define MAX_BONES_PER_VERT 16
bool ModelLoader::loadMeshBoneWeights(aiMesh* mesh,
                                      uint8_t*& vertBoneCounts,
                                      uint16_t*& vertBoneIndecies,
                                      GLfloat*& vertBoneWeights, ) {
    int numBones = -1;
    if (mesh->HasBones()) {
        numBones = mesh->mNumBones;
        size_t numWeights = mesh->mNumVertices * MAX_BONES_PER_VERT;

        // prepare bone count matrix
        vertBoneCounts = new uint8_t[mesh->mNumVertices];
        memset(vertBoneCounts, 0, sizeof(uint8_t) * mesh->mNumVertices);

        // prepare bone index matrix
        vertBoneIndecies = new uint16_t[numWeights];
        memset(vertBoneIndecies, -1, numWeights);

        // prepare bone weight matrix
        vertBoneWeights = new GLfloat[numWeights];
        memset(vertBoneWeights, 0, numWeights * sizeof(GLfloat));

        uint16_t numBones = mesh->mNumBones;
        if (mesh->mNumBones > 0xFFFF) {
            std::cout << "error: mesh has more than 0xFFFF bones" << std::endl;
            numBones = 0xFFFF;
        }

        for (uint16_t boneIndex = 0; boneIndex < numBones; boneIndex++) {
            aiBone* bone = mesh->mBones[boneIndex];
            for (size_t weightIndex = 0; weightIndex < bone->mNumWeights;
                 weightIndex++) {
                aiVertexWeight weight = bone->mWeights[weightIndex];
                size_t currBones = vertBoneCounts[weight.mVertexId];
                if (currBones == MAX_BONES_PER_VERT) {
                    std::cout << "error: vertex " << weight.mVertexId
                              << "has more than " << MAX_BONES_PER_VERT
                              << " bone weights. "
                              << "Discarding additional bone weights "
                              << "(bone " << boneIndex << ")" << std::endl;
                    continue;
                }

                vertBoneIndecies[currBones] = boneIndex;
                vertBoneCounts[weight.mVertexId]++;
                vertBoneWeights[boneIndex * MAX_BONES_PER_VERT + currBones] =
                    weight.mWeight;
            }
        }
    }
}

LoadedMesh* makeModel(SDL_Renderer* renderCtx,
                      const aiScene* scene,
                      const aiMesh* mesh) {
    GLfloat* verts = new GLfloat[mesh->mNumFaces * 9];
    GLfloat* colors = NULL;
    GLfloat* uvs = NULL;

    if (mesh->HasVertexColors(0)) {
        std::cout << "mesh has vertex colors" << std::endl;
        colors = new GLfloat[mesh->mNumFaces * 9];
    }

    if (mesh->HasTextureCoords(0)) {
        std::cout << "mesh has uvs" << std::endl;
        uvs = new GLfloat[mesh->mNumFaces * 6];
    }

    size_t tri = 0;
    for (size_t f = 0; f < mesh->mNumFaces; f++) {
        aiFace face = mesh->mFaces[f];
        for (size_t ind = 0; ind < face.mNumIndices; ind++) {
            size_t vertexIndex = face.mIndices[ind];

            aiVector3D vert = mesh->mVertices[vertexIndex];
            verts[tri * 3] = vert.x;
            verts[tri * 3 + 1] = vert.y;
            verts[tri * 3 + 2] = vert.z;

            if (colors) {
                aiColor4D color = mesh->mColors[0][vertexIndex];
                colors[tri * 3] = color.r;
                colors[tri * 3 + 1] = color.g;
                colors[tri * 3 + 2] = color.b;
            }

            if (uvs) {
                aiVector3D uv = mesh->mTextureCoords[0][vertexIndex];
                uvs[tri * 2] = uv.x;
                uvs[tri * 2 + 1] = uv.y;
            }

            tri++;
        }
    }

    // build an array of bone weights if the model has bones
    uint8_t* vertBoneCounts = NULL;
    uint16_t* vertBoneIndecies = NULL;
    GLfloat* vertBoneWeights = NULL;
    ModelLoader::loadMeshBoneWeights(mesh, vertBoneCounts, vertBoneIndecies,
                                     vertBoneWeights);

    // assemble the final mesh
    WorldspaceMesh* outMesh = new WorldspaceMesh();
    outMesh->init(verts, colors, uvs, tri);
    if (vertBoneWeights) {
        outMesh->initSkeleton(vertBoneCounts, vertBoneIndecies, vertBoneWeights,
                              MAX_BONES_PER_VERT, numBones);
    }

    // construct a material from the corresponding aiMaterial
    aiMaterial* loadedMaterial = scene->mMaterials[mesh->mMaterialIndex];
    Material* material = new Material();
    if (loadedMaterial->GetTextureCount(aiTextureType_AMBIENT)) {
        std::cout << "material has an ambient texture" << std::endl;
        aiString path;
        if (loadedMaterial->GetTexture(aiTextureType_AMBIENT, 0, &path)) {
            std::cout << "failed to load texture" << std::endl;
        } else {
            std::cout << "ambient texture @ " << path.C_Str() << std::endl;
            SDL_Surface* loadedSurface = IMG_Load(path.C_Str());
            SDL_Texture* loadedTexture =
                SDL_CreateTextureFromSurface(renderCtx, loadedSurface);
            material->setAmbientTexture(loadedTexture);
        }
    }

    LoadedMesh* m = new LoadedMesh(
        ModelMesh{
            .material = material,
            .mesh = outMesh,
            .animations = std::map<std::string, MeshAnim*>(),
        },
        mesh);
    return m;
}

const aiBone* getBoneFromNodeName(const aiMesh* mesh, const aiString name) {
    for (size_t i = 0; i < mesh->mNumBones; i++) {
        if (mesh->mBones[i]->mName == name) {
            return mesh->mBones[i];
        }
    }
    return NULL;
}

glm::mat4 glmMat4FromAiMat4(aiMatrix4x4& mat) {
    return glm::mat4(mat.a1, mat.a2, mat.a3, mat.a4, mat.b1, mat.b2, mat.b3,
                     mat.b4, mat.c1, mat.c2, mat.c3, mat.c4, mat.d1, mat.d2,
                     mat.d3, mat.d4);
}

MeshAnim* ModelLoader::makeModelAnimation(LoadedMesh* mesh,
                                          aiAnimation* animation) {
    // get times of all keyframes in a way we can actually iterate over
    size_t numPosKeys[animation->mNumChannels];
    size_t curPosKeys[animation->mNumChannels] = {0};
    size_t numRotKeys[animation->mNumChannels];
    size_t curRotKeys[animation->mNumChannels] = {0};
    size_t numSclKeys[animation->mNumChannels];
    size_t curSclKeys[animation->mNumChannels] = {0};
    std::set<float> keyframeTimes;
    for (size_t i = 0; i < animation->mNumChannels; i++) {
        aiNodeAnim* nodeAnim = animation->mChannels[i];
        for (size_t y = 0; y < nodeAnim->mNumPositionKeys; y++) {
            float time = (float)(nodeAnim->mPositionKeys[y].mTime);
            keyframeTimes.insert(time);
        }
        for (size_t y = 0; y < nodeAnim->mNumRotationKeys; y++) {
            float time = (float)(nodeAnim->mRotationKeys[y].mTime);
            keyframeTimes.insert(time);
        }
        for (size_t y = 0; y < nodeAnim->mNumScalingKeys; y++) {
            float time = (float)(nodeAnim->mScalingKeys[y].mTime);
            keyframeTimes.insert(time);
        }
    }

    // sort the list of keyframe times
    std::vector<float> keyframeTimesOrdered = std::vector<float>();
    std::copy(keyframeTimes.begin(), keyframeTimes.end(),
              std::back_inserter(keyframeTimesOrdered));
    std::sort(keyframeTimesOrdered.begin(), keyframeTimesOrdered.end());

    std::vector<glm::mat4> frameTransforms[keyframeTimesOrdered.size()];
    for (int t = 0; t < keyframeTimesOrdered.size(); t++) {
        float keyTime = keyframeTimesOrdered[t];
        for (size_t i = 0; i < animation->mNumChannels; i++) {
            aiNodeAnim* nodeAnim = animation->mChannels[i];
            float ratio;

            const aiBone* bone =
                getBoneFromNodeName(mesh->sourceMesh, nodeAnim->mNodeName);
            if (bone == NULL) {
                // we ignore animation keys for bones not inside this mesh
                continue;
            }
            aiMatrix4x4 baseTransformAi = bone->mOffsetMatrix;
            glm::mat4 baseTransform = glmMat4FromAiMat4(baseTransformAi);
            if (numRotKeys[i] == 0) {
                // no keyframes, push the base transform matrix
                frameTransforms[t].push_back(baseTransform);
                continue;
            }

            aiQuatKey curRotKey = nodeAnim->mRotationKeys[curRotKeys[i]];
            aiQuatKey nextRotKey =
                nodeAnim
                    ->mRotationKeys[min(curRotKeys[i] + 1, numRotKeys[i] - 1)];
            ratio = (float)((curRotKey.mTime - keyTime) /
                            (nextRotKey.mTime - curRotKey.mTime));
            aiQuaternion rotation;
            aiQuaternion::Interpolate(rotation, curRotKey.mValue,
                                      nextRotKey.mValue, ratio);

            aiVectorKey curPosKey = nodeAnim->mPositionKeys[curPosKeys[i]];
            aiVectorKey nextPosKey =
                nodeAnim
                    ->mPositionKeys[min(curPosKeys[i] + 1, numPosKeys[i] - 1)];
            ratio = (float)((curPosKey.mTime - keyTime) /
                            (nextPosKey.mTime - curPosKey.mTime));
            aiVector3D position =
                ratio * curPosKey.mValue + (1 - ratio) * nextPosKey.mValue;

            aiVectorKey curSclKey = nodeAnim->mScalingKeys[curSclKeys[i]];
            aiVectorKey nextSclKey =
                nodeAnim
                    ->mScalingKeys[min(curSclKeys[i] + 1, numSclKeys[i] - 1)];
            ratio = (float)((curSclKey.mTime - keyTime) /
                            (nextSclKey.mTime - curSclKey.mTime));
            aiVector3D scale =
                ratio * curSclKey.mValue + (1 - ratio) * nextSclKey.mValue;

            aiMatrix3x3 rotateMatrix3 = rotation.GetMatrix();
            aiMatrix4x4 rotateMatrix = aiMatrix4x4(rotateMatrix3);

            aiMatrix4x4 scaleMatrix, positionMatrix;
            aiMatrix4x4::Scaling(scale, scaleMatrix);
            aiMatrix4x4::Translation(position, positionMatrix);

            aiMatrix4x4 transform = rotateMatrix * scaleMatrix * positionMatrix;
            glm::mat4 animTransform = glmMat4FromAiMat4(transform);

            frameTransforms[t].push_back(animTransform * baseTransform);
        }
    }

    std::vector<Keyframe*> frames(keyframeTimesOrdered.size());
    for (int t = 0; t < keyframeTimesOrdered.size(); t++) {
        frames[t] = new Keyframe;
        frames[t]->time = keyframeTimesOrdered[t];
        frames[t]->transforms = frameTransforms[t];
    }

    return new MeshAnim(frames);
}

Model* ModelLoader::queryScene(const char* scenePath) {
    std::cout << "queryScene(" << scenePath << ")" << std::endl;
    if (scene == NULL) {
        return NULL;
    }

    std::vector<LoadedMesh*> loadedMeshes;
    for (size_t i = 0; i < scene->mNumMeshes; i++) {
        std::cout << "querying mesh" << i << " of " << scene->mNumMeshes
                  << std::endl;
        const aiMesh* mesh = scene->mMeshes[i];
        std::cout << "query " << mesh->mName.C_Str() << "? " << scenePath
                  << std::endl;
        if (0 == strcmp(mesh->mName.C_Str(), scenePath)) {
            std::cout << "loading a mesh with " << mesh->mNumVertices
                      << " verts"
                      << ", " << mesh->mNumFaces << " faces" << std::endl;

            if (mesh->mNumFaces != 0) {
                loadedMeshes.push_back(makeModel(renderCtx, scene, mesh));
            }
        }
    }

    if (loadedMeshes.size() == 0) {
        return NULL;
    }

    // create an ordered set of bone names
    std::vector<ModelMesh> meshes;
    for (LoadedMesh* mesh : loadedMeshes) {
        for (size_t i = 0; i < scene->mNumAnimations; i++) {
            aiAnimation* animation = scene->mAnimations[i];
            MeshAnim* modelAnimation = makeModelAnimation(mesh, animation);
            std::string name = std::string(animation->mName.C_Str());
            mesh->loadedMesh.animations[name] = modelAnimation;
            meshes.push_back(mesh->loadedMesh);
        }
    }

    return new Model(meshes);
}
