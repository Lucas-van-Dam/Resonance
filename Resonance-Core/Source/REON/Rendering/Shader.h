#pragma once

#include <glad/glad.h>
#include "glm/glm.hpp"
#include "REON/ResourceManagement/Resource.h"

#ifdef REON_PLATFORM_WINDOWS
#include <Windows.h>
#include <objidl.h>
#include "dxc/dxcapi.h"
#endif



namespace REON {

	class [[clang::annotate("serialize")]] Shader : public ResourceBase {
	public:
		// constructor reads and builds the shader
		Shader();

		void ReloadShader();

		void use() {}

		// Inherited via Resource
		void Load(const std::string& filePath, std::any metadata) override;
		virtual void Load() override {};
		void Unload() override;

		static std::vector<char> CompileHLSLToSPIRV(const std::string& source, uint32_t flags = 0);
		std::vector<char> getVertexSPIRV() { if (m_VertexSPIRV.empty()) return CompileHLSLToSPIRV(m_VertexPath); else return m_VertexSPIRV; }
		std::vector<char> getFragmentSPIRV() { if (m_FragmentSPIRV.empty()) return CompileHLSLToSPIRV(m_FragmentPath); else return m_FragmentSPIRV; }
		std::vector<char> getGeometrySPIRV() { if (m_GeometrySPIRV.empty()) return CompileHLSLToSPIRV(m_GeometryPath); else return m_GeometrySPIRV; }

	public:
		// the program ID
		unsigned int ID;

	private:
		std::vector<char> m_VertexSPIRV, m_FragmentSPIRV, m_GeometrySPIRV;
		std::string m_VertexPath, m_FragmentPath, m_GeometryPath;
		GLuint m_FragmentID, m_VertexID, m_GeometryID;
	};
}