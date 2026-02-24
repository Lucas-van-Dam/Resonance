#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>
#include <mikktspace.h>

namespace REON::EDITOR
{
class TangentCalculator
{
  public:
    static void CalculateTangents(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& texCoords,
                                  std::vector<glm::vec3>& normals, std::vector<glm::vec4>& tangents,
                                  const std::vector<uint32_t>& indices);

  private:
    struct SMikkTSpaceMeshContext
    {
        std::vector<glm::vec3>& positions;
        std::vector<glm::vec2>& texCoords;
        std::vector<glm::vec3>& normals;
        std::vector<glm::vec4>& tangents;
        const std::vector<uint32_t>& indices;
    };

    static int getNumFaces(const SMikkTSpaceContext* pContext)
    {
        auto* ctx = static_cast<SMikkTSpaceMeshContext*>(pContext->m_pUserData);
        return ctx->indices.size() / 3;
    }

    static int getNumVerticesOfFace(const SMikkTSpaceContext* pContext, int face)
    {
        return 3;
    }

    static void getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], int face, int vert)
    {
        auto* ctx = static_cast<SMikkTSpaceMeshContext*>(pContext->m_pUserData);
        

        int index = ctx->indices[face * 3 + vert];
        fvPosOut[0] = ctx->positions[index].x;
        fvPosOut[1] = ctx->positions[index].y;
        fvPosOut[2] = ctx->positions[index].z;
    }

    static void getNormal(const SMikkTSpaceContext* pContext, float fvNormalOut[], int face, int vert)
    {
        auto* ctx = static_cast<SMikkTSpaceMeshContext*>(pContext->m_pUserData);
        int index = ctx->indices[face * 3 + vert];
        fvNormalOut[0] = ctx->normals[index].x;
        fvNormalOut[1] = ctx->normals[index].y;
        fvNormalOut[2] = ctx->normals[index].z;
    }

    static void getTexCoord(const SMikkTSpaceContext* pContext, float fvTexOut[], int face, int vert)
    {
        auto* ctx = static_cast<SMikkTSpaceMeshContext*>(pContext->m_pUserData);
        int index = ctx->indices[face * 3 + vert];
        fvTexOut[0] = ctx->texCoords[index].x;
        fvTexOut[1] = ctx->texCoords[index].y;
    }

    static void setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, int face,
                               int vert)
    {
        auto* ctx = static_cast<SMikkTSpaceMeshContext*>(pContext->m_pUserData);
        int index = ctx->indices[face * 3 + vert];
        ctx->tangents[index].x = fvTangent[0];
        ctx->tangents[index].y = fvTangent[1];
        ctx->tangents[index].z = fvTangent[2];
        ctx->tangents[index].w = fSign;
    }
};
} // namespace REON::EDITOR