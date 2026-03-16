#pragma once

#include "glm/glm.hpp"

namespace REON
{

struct alignas(16) LightData
{
    LightData(float intensity, glm::vec3 lightColor, glm::vec3 position, glm::vec3 direction, float innerCutOff,
              float outerCutOff, int type, glm::mat4 viewProj, float range)
        : position(position.x, position.y, position.z, type),
          direction(direction.x, direction.y, direction.z, glm::radians(innerCutOff)),
          lightColor(lightColor.x, lightColor.y, lightColor.z, intensity), viewProj(viewProj), range(range),
          outerCutoff(outerCutOff), type(type)
    {
    }
    LightData() {};
    glm::vec4 position;
    glm::vec4 direction;
    glm::vec4 lightColor;
    glm::mat4 viewProj;
    float range;
    float outerCutoff;
    int type;
    float _padding;
};

} // namespace REON