#include "reonpch.h"
#include "Mesh.h"
#include <REON/ResourceManagement/ResourceInfo.h>

namespace REON {

    
    void Mesh::Load(const std::string& name, std::any metadata)
    {
        try {
            Mesh mesh = std::any_cast<Mesh>(metadata);
            return;
        }
        catch (const std::bad_any_cast&) {}
        m_Path = name;
        auto data = std::any_cast<std::tuple<std::vector<Vertex>, std::vector<unsigned int>>>(metadata);
        this->m_Vertices = std::get<0>(data);
        this->indices = std::get<1>(data);

        setupMesh();
    }

    void Mesh::Load()
    {
        setupMesh2();
    }

    void Mesh::Unload()
    {
        m_Vertices.clear();
        indices.clear();

        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
    }

    nlohmann::ordered_json Mesh::Serialize() const
    {
        nlohmann::ordered_json meshJson;
        meshJson["GUID"] = GetID();
        meshJson["vertices"] = ConvertVec3Array(vertices); // Convert glm::vec3 to JSON
        meshJson["normals"] = ConvertVec3Array(normals);
        meshJson["uvs"] = ConvertVec2Array(uvs);
        meshJson["tangents"] = ConvertVec3Array(tangents);
        meshJson["colors"] = ConvertVec4Array(colors);
        meshJson["indices"] = indices;

        // Serialize submeshes
        nlohmann::ordered_json subMeshArray = nlohmann::ordered_json::array();
        for (const auto& subMesh : subMeshes)
        {
            nlohmann::ordered_json subMeshJson;
            subMeshJson["indexCount"] = subMesh.indexCount;
            subMeshJson["indexOffset"] = subMesh.indexOffset;
            subMeshJson["materialIndex"] = subMesh.materialIndex;
            subMeshArray.push_back(subMeshJson);
        }
        meshJson["subMeshes"] = subMeshArray;
        
        return meshJson;
    }

    void Mesh::DeSerialize(const nlohmann::ordered_json& meshJson)
    {
        vertices = ConvertJsonToVec3Array(meshJson["vertices"]);
        normals = ConvertJsonToVec3Array(meshJson["normals"]);
        uvs = ConvertJsonToVec2Array(meshJson["uvs"]);
        tangents = ConvertJsonToVec3Array(meshJson["tangents"]);
        colors = ConvertJsonToVec4Array(meshJson["colors"]);
        indices = meshJson["indices"].get<std::vector<uint32_t>>();

        // Deserialize subMeshes
        for (const auto& subMeshJson : meshJson["subMeshes"]) {
            SubMesh subMesh;
            subMesh.indexCount = subMeshJson["indexCount"].get<int>();
            subMesh.indexOffset = subMeshJson["indexOffset"].get<int>();
            subMesh.materialIndex = subMeshJson["materialIndex"].get<int>();
            subMeshes.push_back(subMesh);
        }
    }

    nlohmann::ordered_json Mesh::ConvertVec3Array(const std::vector<glm::vec3>& vecs) const{
        nlohmann::ordered_json arr = nlohmann::ordered_json::array();
        for (const auto& v : vecs) {
            arr.push_back({ v.x, v.y, v.z });
        }
        return arr;
    }

    nlohmann::ordered_json Mesh::ConvertVec2Array(const std::vector<glm::vec2>& vecs) const {
        nlohmann::ordered_json arr = nlohmann::ordered_json::array();
        for (const auto& v : vecs) {
            arr.push_back({ v.x, v.y });
        }
        return arr;
    }

    nlohmann::ordered_json Mesh::ConvertVec4Array(const std::vector<glm::vec4>& vecs) const {
        nlohmann::ordered_json arr = nlohmann::ordered_json::array();
        for (const auto& v : vecs) {
            arr.push_back({ v.x, v.y, v.z, v.w });
        }
        return arr;
    }

    std::vector<glm::vec3> Mesh::ConvertJsonToVec3Array(const nlohmann::json& jsonArray) {
        std::vector<glm::vec3> vec;
        for (const auto& item : jsonArray) {
            vec.emplace_back(item[0], item[1], item[2]);
        }
        return vec;
    }

    std::vector<glm::vec2> Mesh::ConvertJsonToVec2Array(const nlohmann::json& jsonArray) {
        std::vector<glm::vec2> vec;
        for (const auto& item : jsonArray) {
            vec.emplace_back(item[0], item[1]);
        }
        return vec;
    }

    std::vector<glm::vec4> Mesh::ConvertJsonToVec4Array(const nlohmann::json& jsonArray) {
        std::vector<glm::vec4> vec;
        for (const auto& item : jsonArray) {
            vec.emplace_back(item[0], item[1], item[2], item[3]);
        }
        return vec;
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

    void Mesh::setupMesh2()
    {
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);

        // Compute the total size needed for interleaved buffer
        size_t vertexCount = vertices.size();
        size_t totalSize = (vertices.size() * sizeof(glm::vec3)) +
            (normals.size() * sizeof(glm::vec3)) +
            (uvs.size() * sizeof(glm::vec2)) +
            (tangents.size() * sizeof(glm::vec3)) +
            (colors.size() * sizeof(glm::vec4));

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, totalSize, nullptr, GL_STATIC_DRAW);

        // Offset tracker for interleaved buffer
        size_t offset = 0;

        // Upload positions
        glBufferSubData(GL_ARRAY_BUFFER, offset, vertices.size() * sizeof(glm::vec3), vertices.data());
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)offset);
        offset += vertices.size() * sizeof(glm::vec3);

        // Upload normals (if available)
        if (!normals.empty()) {
            glBufferSubData(GL_ARRAY_BUFFER, offset, normals.size() * sizeof(glm::vec3), normals.data());
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)offset);
            offset += normals.size() * sizeof(glm::vec3);
        }

        // Upload UVs (if available)
        if (!uvs.empty()) {
            glBufferSubData(GL_ARRAY_BUFFER, offset, uvs.size() * sizeof(glm::vec2), uvs.data());
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)offset);
            offset += uvs.size() * sizeof(glm::vec2);
        }

        // Upload tangents (if available)
        if (!tangents.empty()) {
            glBufferSubData(GL_ARRAY_BUFFER, offset, tangents.size() * sizeof(glm::vec3), tangents.data());
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)offset);
            offset += tangents.size() * sizeof(glm::vec3);
        }

        // Upload colors (if available)
        if (!colors.empty()) {
            glBufferSubData(GL_ARRAY_BUFFER, offset, colors.size() * sizeof(glm::vec4), colors.data());
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)offset);
            offset += colors.size() * sizeof(glm::vec4);
        }

        // Upload indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint), indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    Mesh::Mesh(const Mesh& mesh)
    {
        
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