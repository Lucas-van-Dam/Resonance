#pragma once
#include <string>
#include <unordered_map>
#include <Reon.h>
#include <filesystem>
#include "ShaderNode.h"

namespace REON::SG {
	class ShaderNodeLibrary
	{
	public:
		static ShaderNodeLibrary& GetInstance()
		{
			static ShaderNodeLibrary instance;
			return instance;
		}

		void RegisterTemplate(const ShaderNodeTemplate& nodeTemplate);
		const ShaderNodeTemplate* GetTemplate(const std::string& type) const;
		const std::unordered_map<std::string, ShaderNodeTemplate>& GetAllTemplates() const { return m_Templates; }
		void Initialize(std::filesystem::path templateFile);

	private:
		std::unordered_map<std::string, ShaderNodeTemplate> m_Templates;

		inline ShaderValueType ShaderValueTypeFromString(const std::string& str);
	};

}