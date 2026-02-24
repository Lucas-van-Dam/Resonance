#include "TangentCalculator.h"

namespace REON::EDITOR
{
void TangentCalculator::CalculateTangents(std::vector<glm::vec3>& positions,
                                                        std::vector<glm::vec2>& texCoords,
                                                        std::vector<glm::vec3>& normals,
                                                        std::vector<glm::vec4>& tangents,
                                                        const std::vector<uint32_t>& indices)
{
    tangents.resize(indices.size());
    SMikkTSpaceMeshContext userContext{positions, texCoords, normals, tangents, indices};

    SMikkTSpaceInterface mikkInterface{};
    mikkInterface.m_getNumFaces = getNumFaces;
    mikkInterface.m_getNumVerticesOfFace = getNumVerticesOfFace;
    mikkInterface.m_getPosition = getPosition;
    mikkInterface.m_getNormal = getNormal;
    mikkInterface.m_getTexCoord = getTexCoord;
    mikkInterface.m_setTSpaceBasic = setTSpaceBasic;

    SMikkTSpaceContext context{};
    context.m_pUserData = &userContext;
    context.m_pInterface = &mikkInterface;

    genTangSpaceDefault(&context);
}
} // namespace REON::EDITOR