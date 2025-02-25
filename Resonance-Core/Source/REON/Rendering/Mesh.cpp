#include "reonpch.h"
#include "Mesh.h"
#include <REON/ResourceManagement/ResourceInfo.h>

namespace REON {

    
    void Mesh::Load(const std::string& name, std::any metadata)
    {
        m_Path = name;
        auto data = std::any_cast<std::tuple<std::vector<Vertex>, std::vector<unsigned int>>>(metadata);
        this->m_Vertices = std::get<0>(data);
        this->indices = std::get<1>(data);

        setupMesh();
    }

    void Mesh::Unload()
    {
        m_Vertices.clear();
        indices.clear();

        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
    }

    void Mesh::Serialize(const std::string& path)
    {
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
        std::ofstream outFile(path + "\\" + GetID() + ".mesh", std::ios::binary);
        if (!outFile.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + path);
        }

        // Save vertex data
        size_t vertexCount = m_Vertices.size();
        outFile.write(reinterpret_cast<const char*>(&vertexCount), sizeof(size_t));
        outFile.write(reinterpret_cast<const char*>(m_Vertices.data()), vertexCount * sizeof(Vertex));

        // Save index data
        size_t indexCount = indices.size();
        outFile.write(reinterpret_cast<const char*>(&indexCount), sizeof(size_t));
        outFile.write(reinterpret_cast<const char*>(indices.data()), indexCount * sizeof(unsigned int));

        //// Save material ID
        //size_t materialIdLength = m_MaterialId.size();
        //outFile.write(reinterpret_cast<const char*>(&materialIdLength), sizeof(size_t));
        //outFile.write(m_MaterialId.data(), materialIdLength);

        outFile.close();
    }

    void Mesh::DeSerialize(const std::string& path)
    {
        std::ifstream inFile(path, std::ios::binary);
        if (!inFile.is_open()) {
            throw std::runtime_error("Failed to open file for reading: " + path);
        }

        // Load vertex data
        size_t vertexCount;
        inFile.read(reinterpret_cast<char*>(&vertexCount), sizeof(size_t));
        m_Vertices.resize(vertexCount);
        inFile.read(reinterpret_cast<char*>(m_Vertices.data()), vertexCount * sizeof(Vertex));

        // Load index data
        size_t indexCount;
        inFile.read(reinterpret_cast<char*>(&indexCount), sizeof(size_t));
        indices.resize(indexCount);
        inFile.read(reinterpret_cast<char*>(indices.data()), indexCount * sizeof(unsigned int));

        //// Load material ID
        //size_t materialIdLength;
        //inFile.read(reinterpret_cast<char*>(&materialIdLength), sizeof(size_t));
        //m_MaterialId.resize(materialIdLength);
        //inFile.read(&m_MaterialId[0], materialIdLength);

        inFile.close();

        // Recreate OpenGL buffers
        setupMesh();
    }

    // initializes all the buffer objects/arrays
    void Mesh::setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);
        glGenBuffers(1, &m_SSBO);

        glBindVertexArray(m_VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(Vertex), &m_Vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
        // ids
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

        // weights
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
        glBindVertexArray(0);
    }

    // render the mesh
    void Mesh::Draw(Material& material, std::vector<LightData> lightData) const
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        //load textures or flat values
        //diffuse
        if (material.albedoTexture == nullptr) {
            material.shader->setBool("useAlbedoTexture", false);
            material.shader->setVec4("albedo", material.albedoColor);
        }
        else {
            material.shader->setBool("useAlbedoTexture", true);
            glActiveTexture(GL_TEXTURE0);
            glUniform1i(glGetUniformLocation(material.shader->ID, "texture_albedo"), 0);
            glBindTexture(GL_TEXTURE_2D, material.albedoTexture->GetTextureId());
        }
        //normal
        if (material.normalTexture == nullptr) {
            material.shader->setBool("useNormalTexture", false);
        }
        else {
            material.shader->setBool("useNormalTexture", true);
            glActiveTexture(GL_TEXTURE1);
            glUniform1i(glGetUniformLocation(material.shader->ID, "texture_normal"), 1);
            glBindTexture(GL_TEXTURE_2D, material.normalTexture->GetTextureId());
        }
        //roughness
        if (material.roughnessTexture == nullptr) {
            material.shader->setBool("useRoughnessTexture", false);
            material.shader->setFloat("roughness", material.roughness);
        }
        else {
            material.shader->setBool("useRoughnessTexture", true);
            glActiveTexture(GL_TEXTURE2);
            glUniform1i(glGetUniformLocation(material.shader->ID, "texture_roughness"), 2);
            glBindTexture(GL_TEXTURE_2D, material.roughnessTexture->GetTextureId());
        }
        //metallic
        if (material.metallicTexture == nullptr) {
            material.shader->setBool("useMetallicTexture", false);
            material.shader->setFloat("metallic", material.metallic);
        }
        else {
            material.shader->setBool("useMetallicTexture", true);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, material.metallicTexture->GetTextureId());
        }

        if (!lightData.empty()) {
            LightData* lightDataStatic = &lightData.front();

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBO);
            size_t totalSize = sizeof(LightData) * lightData.size();
            glBufferData(GL_SHADER_STORAGE_BUFFER, totalSize, lightDataStatic,
                GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_SSBO);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
        }

        // draw mesh
        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
    }
}