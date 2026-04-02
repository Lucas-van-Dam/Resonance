#include "ModelBinWriter.h"

#include "REON/AssetManagement/ModelBinFormat.h"

#include <type_traits>

namespace REON_EDITOR
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

    const uint32_t chunkCount = 4;
    header.chunkCount = chunkCount;

    if (model.rig.has_value())
    {
        header.flags |= HAS_RIG;
        std::memcpy(header.rigId, model.rig.value().rigId.bytes.data(), 16);
    }

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

    std::vector<SceneNode> sceneNodes(nodeCount, SceneNode{});

    for (uint32_t i = 0; i < nodeCount; ++i)
    {
        const auto& n = model.nodes[i];
        auto& sn = sceneNodes[i];

        sn.parent = n.parent;
        sn.firstChild = UINT32_MAX;
        sn.nextSibling = UINT32_MAX;

        sn.version = 1;

        std::memcpy(sn.nodeId, n.NodeId.bytes.data(), 16);

        sn.t[0] = n.t.x;
        sn.t[1] = n.t.y;
        sn.t[2] = n.t.z;
        sn.r[0] = n.r.w;
        sn.r[1] = n.r.x;
        sn.r[2] = n.r.y;
        sn.r[3] = n.r.z;
        sn.s[0] = n.s.x;
        sn.s[1] = n.s.y;
        sn.s[2] = n.s.z;

        sn.flags = 0;
        sn.reserved0 = 0;

        sn.skinIndex = n.skinIndex; // UINT32_MAX if none

        if (!IsNull(n.meshId))
            std::memcpy(sn.meshId, n.meshId.bytes.data(), 16);
        else
            std::memset(sn.meshId, 0, 16);

        sn.materialCount = (uint32_t)std::min<size_t>(n.materialId.size(), 10);

        std::memset(sn.materialId, 0, sizeof(sn.materialId));
        if (sn.materialCount > 0)
            std::memcpy(sn.materialId, n.materialId.data(), sn.materialCount * 16);

        if (!n.debugName.empty())
        {
            std::strncpy(sn.debugName, n.debugName.c_str(), sizeof(sn.debugName) - 1);
        }
        else
        {
            sn.debugName[0] = '\0'; // mark as null
        }
    }

    for (uint32_t i = 0; i < nodeCount; ++i)
    {
        const auto& n = model.nodes[i];
        auto& sn = sceneNodes[i];

        if (!n.children.empty())
        {
            sn.firstChild = n.children[0];

            for (size_t c = 0; c + 1 < n.children.size(); ++c)
            {
                const uint32_t a = n.children[c];
                const uint32_t b = n.children[c + 1];
                sceneNodes[a].nextSibling = b;
            }

            sceneNodes[n.children.back()].nextSibling = UINT32_MAX;
        }
    }

    for (const auto& sn : sceneNodes)
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

        if (!m.joints_0.empty())
        {
            mh.joint0offset = off;
            off += uint32_t(sizeof(glm::u16vec4) * m.joints_0.size());
        }

        if (!m.joints_1.empty())
        {
            mh.joint1offset = off;
            off += uint32_t(sizeof(glm::u16vec4) * m.joints_1.size());
        }

        if (!m.weights_0.empty())
        {
            mh.weight0offset = off;
            off += uint32_t(sizeof(glm::vec4) * m.weights_0.size());
        }

        if (!m.weights_1.empty())
        {
            mh.weight1offset = off;
            off += uint32_t(sizeof(glm::vec4) * m.weights_1.size());
        }

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

        if (!m.joints_0.empty())
        {
            WriteSpan(out, m.joints_0.data(), m.joints_0.size());
        }

        if (!m.joints_1.empty())
        {
            WriteSpan(out, m.joints_1.data(), m.joints_1.size());
        }

        if (!m.weights_0.empty())
        {
            WriteSpan(out, m.weights_0.data(), m.weights_0.size());
        }

        if (!m.weights_1.empty())
        {
            WriteSpan(out, m.weights_1.data(), m.weights_1.size());
        }

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

    if (model.rig.has_value())
    {
        ChunkEntry rigChunk{};
        rigChunk.type = ChunkType::RIG;
        rigChunk.flags = 0;
        rigChunk.offset = cursor;

        const uint64_t chunkStart = rigChunk.offset;

        const uint32_t skinCount = (uint32_t)model.skins.size();          // embed all skins
        const uint32_t jointIdCount = (uint32_t)model.rig->joints.size(); // optional metadata list

        RigChunkHeader rh{};
        rh.version = 1;
        rh.skinCount = skinCount;
        rh.jointCount = jointIdCount;
        std::memcpy(rh.rigId, model.rig->rigId.bytes.data(), 16);

        const uint64_t headerPos = (uint64_t)out.tellp();
        WritePOD(out, rh);

        // Skin record table immediately after header
        rh.skinOffset = (uint32_t)((uint64_t)out.tellp() - chunkStart);

        std::vector<SkinRecord> records(skinCount);
        const uint64_t recordsPos = (uint64_t)out.tellp();
        if (!records.empty())
            WriteSpan(out, records.data(), records.size());

        uint64_t cur = AlignUp((uint64_t)out.tellp(), 16);
        WriteZeros(out, cur - (uint64_t)out.tellp());

        // Optional: write rig joint nodeIds (16 bytes each)
        if (jointIdCount > 0)
        {
            rh.jointNodeIdsOffset = (uint32_t)((uint64_t)out.tellp() - chunkStart);
            for (const REON::AssetId& id : model.rig->joints)
                out.write((const char*)id.bytes.data(), 16);

            cur = AlignUp((uint64_t)out.tellp(), 16);
            WriteZeros(out, cur - (uint64_t)out.tellp());
        }
        else
        {
            rh.jointNodeIdsOffset = 0;
        }

        // Write embedded skins payload blocks, fill records
        for (uint32_t i = 0; i < skinCount; ++i)
        {
            const auto& s = model.skins[i];

            if (s.jointIndices.size() != s.inverseBindMatrices.size())
            {
                REON_WARN("Size mismatch in IBM and joints for skin {}", i);

                SkinRecord r{};
                r.skeletonNode = UINT32_MAX;
                r.jointCount = 0;
                r.jointsOffset = 0;
                r.ibmOffset = 0;
                records[i] = r;
                continue;
            }

            SkinRecord r{};
            r.skeletonNode = s.skeleton;
            r.jointCount = (uint32_t)s.jointIndices.size();

            // joints
            r.jointsOffset = (uint32_t)((uint64_t)out.tellp() - chunkStart);
            if (!s.jointIndices.empty())
                WriteSpan(out, s.jointIndices.data(), s.jointIndices.size());

            cur = AlignUp((uint64_t)out.tellp(), 16);
            WriteZeros(out, cur - (uint64_t)out.tellp());

            // IBMs
            r.ibmOffset = (uint32_t)((uint64_t)out.tellp() - chunkStart);
            if (!s.inverseBindMatrices.empty())
                WriteSpan(out, s.inverseBindMatrices.data(), s.inverseBindMatrices.size());

            cur = AlignUp((uint64_t)out.tellp(), 16);
            WriteZeros(out, cur - (uint64_t)out.tellp());

            records[i] = r;
        }

        // Patch records table
        const uint64_t endPos = (uint64_t)out.tellp();
        out.seekp((std::streamoff)recordsPos, std::ios::beg);
        if (!records.empty())
            WriteSpan(out, records.data(), records.size());
        out.seekp((std::streamoff)endPos, std::ios::beg);

        // Patch header (skinsOffset/jointNodeIdsOffset are now known)
        out.seekp((std::streamoff)headerPos, std::ios::beg);
        WritePOD(out, rh);
        out.seekp((std::streamoff)endPos, std::ios::beg);

        rigChunk.size = endPos - rigChunk.offset;
        cursor = AlignUp(endPos, 16);
        WriteZeros(out, cursor - endPos);

        chunks.push_back(rigChunk);
    }

    header.fileBytes = (uint64_t)out.tellp();

    out.seekp((std::streamoff)headerOffset, std::ios::beg);
    WritePOD(out, header);

    out.seekp((std::streamoff)chunkTableOffset, std::ios::beg);
    for (const auto& ce : chunks)
        WritePOD(out, ce);

    out.flush();
    if (!out.good())
        throw std::runtime_error("ModelBinWriter: write failed");

    std::unordered_map<REON::AssetKey, REON::ArtifactRef, REON::AssetKeyHash> assetMap;

    for (const auto& mie : meshIndex)
    {
        REON::ArtifactRef ref{};
        ref.uri = outFile.filename().generic_string();
        ref.revision = 0;
        ref.offset = mie.dataOffset;
        ref.size = mie.dataSize;
        ref.format = /*MESH_V1*/ 0x1001;
        REON::AssetKey key;
        key.type = REON::ASSET_MESH;
        std::copy(std::begin(mie.id), std::end(mie.id), key.id.begin());
        assetMap[key] = ref;
    }

    assetMap[REON::AssetKey{REON::ASSET_MODEL, model.modelId}] = {outFile.generic_string(), 0, 0,
                                                            std::filesystem::file_size(outFile),
                                                      0x2001};
    if (model.rig.has_value())
        assetMap[REON::AssetKey{REON::ASSET_RIG, model.rig.value().rigId}] = {outFile.generic_string(), 0,
                                                                              chunks[3].offset,
                                                                        chunks[3].size,
                                                                  0x3001};

    return {0, {}, {}, assetMap};
}
} // namespace REON::EDITOR