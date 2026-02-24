#include "ModelBinWriter.h"

#include "REON/AssetManagement/ModelBinFormat.h"

#include <type_traits>

namespace REON::EDITOR
{
static uint64_t AlignUp(uint64_t v, uint64_t a)
{
    return (v + (a - 1)) & ~(a - 1);
}

static void WriteZeros(std::ofstream& out, uint64_t count)
{
    static const char zeros[256] = {};
    while (count)
    {
        const uint64_t n = std::min<uint64_t>(count, sizeof(zeros));
        out.write(zeros, (std::streamsize)n);
        count -= n;
    }
}

template <class T> static void WritePOD(std::ofstream& out, const T& pod)
{
    static_assert(std::is_trivially_copyable_v<T>);
    out.write(reinterpret_cast<const char*>(&pod), sizeof(T));
}

template <class T> static void WriteSpan(std::ofstream& out, const T* data, size_t count)
{
    out.write(reinterpret_cast<const char*>(data), (std::streamsize)(sizeof(T) * count));
}

CookOutput ModelBinWriter::WriteModelBin(const ImportedModel& model, const std::filesystem::path& outFile)
{
    std::filesystem::create_directories(outFile.parent_path());

    std::ofstream out(outFile, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
        throw std::runtime_error("ModelBinWriter: failed to open output");

    FileHeader header{};
    std::vector<ChunkEntry> chunks;
    chunks.reserve(3);

    const uint32_t chunkCount = 3;
    header.chunkCount = chunkCount;

    const uint64_t headerOffset = 0;
    const uint64_t chunkTableOffset = sizeof(FileHeader);
    const uint64_t chunkTableBytes = sizeof(ChunkEntry) * uint64_t(chunkCount);
    uint64_t cursor = chunkTableOffset + chunkTableBytes;
    cursor = AlignUp(cursor, 16);

    WritePOD(out, header);
    for (uint32_t i = 0; i < chunkCount; ++i)
    {
        ChunkEntry ce{};
        ce.type = ChunkType::SCENE;
        ce.flags = 0;
        ce.offset = 0;
        ce.size = 0;
        WritePOD(out, ce);
    }

    {
        const uint64_t pos = (uint64_t)out.tellp();
        if (pos < cursor)
            WriteZeros(out, cursor - pos);
    }

    ChunkEntry sceneChunk{};
    sceneChunk.type = ChunkType::SCENE;
    sceneChunk.flags = 0;
    sceneChunk.offset = cursor;

    const uint32_t nodeCount = (uint32_t)model.nodes.size();
    WritePOD(out, nodeCount);

    std::vector<SceneNode> sceneNodes;
    sceneNodes.resize(model.nodes.size());

    for (int i = 0; i < model.nodes.size(); ++i)
    {
        const auto& n = model.nodes[i];

        auto& sn = sceneNodes[i];
        sn.parent = n.parent;
        if (n.children.empty())
            sn.firstChild = UINT32_MAX;
        else
        {
            sn.firstChild = n.children[0];

            for (size_t c = 0; c < n.children.size(); ++c)
            {
                const uint32_t childIndex = n.children[c];

                if (c + 1 == n.children.size())
                {
                    sceneNodes[childIndex].nextSibling = UINT32_MAX;
                }
                else
                {
                    sceneNodes[childIndex].nextSibling = n.children[c + 1];
                }
            }
        }

        sn.t[0] = n.t.x;
        sn.t[1] = n.t.y;
        sn.t[2] = n.t.z;
        sn.r[0] = n.r.x;
        sn.r[1] = n.r.y;
        sn.r[2] = n.r.z;
        sn.r[3] = n.r.w;
        sn.s[0] = n.s.x;
        sn.s[1] = n.s.y;
        sn.s[2] = n.s.z;

        if (!IsNull(n.meshId))
            std::copy(n.meshId.begin(), n.meshId.end(), sn.meshId);
        else
            sn.meshId[0] = sn.meshId[1] = sn.meshId[2] = sn.meshId[3] = 0;
        if (n.materialId.size() <= 10)
        {
            std::memcpy(sn.materialId, n.materialId.data(), n.materialId.size() * sizeof(AssetId));
        }
        else
        {
            REON_ERROR("Max materials per node exceeded limit: 10, found: {}", n.materialId.size());
        }
    }

    for (auto& sn : sceneNodes)
        WritePOD(out, sn);

    sceneChunk.size = uint64_t(out.tellp()) - sceneChunk.offset;
    cursor = AlignUp((uint64_t)out.tellp(), 16);
    WriteZeros(out, cursor - (uint64_t)out.tellp());

    std::vector<MeshIndexEntry> meshIndex;
    meshIndex.reserve(model.meshes.size());

    ChunkEntry meshDataChunk{};
    meshDataChunk.type = ChunkType::MESH_DATA;
    meshDataChunk.flags = 0;
    meshDataChunk.offset = cursor;

    for (const auto& m : model.meshes)
    {
        const uint64_t meshPayloadOffset = (uint64_t)out.tellp();

        MeshHeader mh{};
        mh.vertexCount = (uint32_t)m.positions.size();
        mh.indexCount = (uint32_t)m.indices.size();
        mh.flags = 0;

        uint32_t off = (uint32_t)sizeof(MeshHeader);

        mh.posOffset = off;
        off += uint32_t(sizeof(glm::vec3) * m.positions.size());

        mh.nrmOffset = off;
        off += uint32_t(sizeof(glm::vec3) * m.normals.size());

        mh.tanOffset = off;
        off += uint32_t(sizeof(glm::vec4) * m.tangents.size());

        mh.uv0Offset = off;
        off += uint32_t(sizeof(glm::vec2) * m.uv0.size());

        mh.idxOffset = off;
        off += uint32_t(sizeof(uint32_t) * m.indices.size());

        mh.colorOffset = off;
        off += uint32_t(sizeof(glm::vec4) * m.colors.size());

        mh.subMeshOffset = off;
        mh.subMeshCount = (uint32_t)m.subMeshes.size();
        off += uint32_t(sizeof(SubMeshEntry) * m.subMeshes.size());

        WritePOD(out, mh);
        WriteSpan(out, m.positions.data(), m.positions.size());
        WriteSpan(out, m.normals.data(), m.normals.size());
        WriteSpan(out, m.tangents.data(), m.tangents.size());
        WriteSpan(out, m.uv0.data(), m.uv0.size());
        WriteSpan(out, m.indices.data(), m.indices.size());
        WriteSpan(out, m.colors.data(), m.colors.size());

        for (const auto& sm : m.subMeshes)
        {
            SubMeshEntry e{};
            e.indexOffset = sm.indexOffset;
            e.indexCount = sm.indexCount;
            e.materialId = sm.materialId;
            e.reserved = 0;
            WritePOD(out, e);
        }

        const uint64_t meshPayloadEnd = (uint64_t)out.tellp();
        const uint64_t meshPayloadSize = meshPayloadEnd - meshPayloadOffset;

        const uint64_t alignedEnd = AlignUp(meshPayloadEnd, 16);
        WriteZeros(out, alignedEnd - meshPayloadEnd);

        MeshIndexEntry mie{};
        std::copy(m.id.begin(), m.id.end(), mie.id);
        mie.dataOffset = meshPayloadOffset;
        mie.dataSize = alignedEnd - meshPayloadOffset;
        mie.vertexCount = mh.vertexCount;
        mie.indexCount = mh.indexCount;
        mie.flags = mh.flags;
        mie.reserved = 0;
        meshIndex.push_back(mie);
    }

    meshDataChunk.size = uint64_t(out.tellp()) - meshDataChunk.offset;
    cursor = AlignUp((uint64_t)out.tellp(), 16);
    WriteZeros(out, cursor - (uint64_t)out.tellp());

    ChunkEntry meshIndexChunk{};
    meshIndexChunk.type = ChunkType::MESH_INDEX;
    meshIndexChunk.flags = 0;
    meshIndexChunk.offset = cursor;

    const uint32_t meshCount = (uint32_t)meshIndex.size();
    WritePOD(out, meshCount);
    if (!meshIndex.empty())
        WriteSpan(out, meshIndex.data(), meshIndex.size());

    meshIndexChunk.size = uint64_t(out.tellp()) - meshIndexChunk.offset;
    cursor = AlignUp((uint64_t)out.tellp(), 16);
    WriteZeros(out, cursor - (uint64_t)out.tellp());

    chunks = {sceneChunk, meshIndexChunk, meshDataChunk};

    header.fileBytes = (uint64_t)out.tellp();

    out.seekp((std::streamoff)headerOffset, std::ios::beg);
    WritePOD(out, header);

    out.seekp((std::streamoff)chunkTableOffset, std::ios::beg);
    for (const auto& ce : chunks)
        WritePOD(out, ce);

    out.flush();
    if (!out.good())
        throw std::runtime_error("ModelBinWriter: write failed");

    std::unordered_map<AssetKey, ArtifactRef, AssetKeyHash> assetMap;

    for (const auto& mie : meshIndex)
    {
        ArtifactRef ref{};
        ref.uri = outFile.filename().generic_string();
        ref.offset = mie.dataOffset;
        ref.size = mie.dataSize;
        ref.format = /*MESH_V1*/ 0x1001;
        AssetKey key;
        key.type = ASSET_MESH;
        std::copy(std::begin(mie.id), std::end(mie.id), key.id.begin());
        assetMap[key] = ref;
    }

    assetMap[AssetKey{ASSET_MODEL, model.modelId}] = {outFile.generic_string(), 0, std::filesystem::file_size(outFile),
                                                      0x2001};

    return {0, {}, {}, assetMap};
}
} // namespace REON::EDITOR