#pragma once

#include "REON/Rendering/Material.h"
#include "REON/Rendering/Structs/LightData.h"
#include "REON/Rendering/Structs/Vertex.h"
#include <filesystem>

namespace REON {

    struct SubMesh {
        int indexCount;
        int indexOffset;
        int materialIndex;
    };

    class Mesh : public Resource {
    public:
        Mesh() {}
        ~Mesh() override { Unload(); }

        void Draw(Material& material, std::vector<LightData> lightData) const;

        virtual void Load(const std::string& name, std::any metadata = {}) override;

        //virtual void Load() override;

        virtual void Unload() override;

        void Serialize(const std::string& path);
        void DeSerialize(const std::string& path);

        std::vector<glm::vec3> vertices;
        std::vector<glm::vec4> colors;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> tangents;
        std::vector<uint> indices;

        std::vector<SubMesh> subMeshes;

    private:
        // initializes all the buffer objects/arrays
        void setupMesh();

    private:
        //  render data
        unsigned int m_VBO, m_EBO;
        unsigned int m_DepthMap;
        std::vector<Vertex> m_Vertices;
        // mesh data
        unsigned int m_VAO, m_SSBO;
    };
}