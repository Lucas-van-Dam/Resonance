#pragma once

#include "REON/Rendering/Material.h"
#include "REON/Rendering/Structs/LightData.h"
#include "REON/Rendering/Structs/Vertex.h"
#include <filesystem>

namespace REON {

    class Mesh : public Resource {
    public:
        Mesh() {}
        ~Mesh() override { Unload(); }

        void Draw(Material& material, std::vector<LightData> lightData) const;

        virtual void Load(const std::string& name, std::any metadata = {}) override;

        virtual void Unload() override;

        void Serialize(const std::string& path);
        void DeSerialize(const std::string& path);
    private:
        // initializes all the buffer objects/arrays
        void setupMesh();

    private:
        //  render data
        unsigned int m_VBO, m_EBO;
        unsigned int m_DepthMap;
        // mesh data
        std::vector<Vertex> m_Vertices;
        std::vector<unsigned int> m_Indices;
        unsigned int m_VAO, m_SSBO;
    };
}