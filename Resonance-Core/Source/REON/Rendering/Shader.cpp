#include "reonpch.h"
#include "Shader.h"

using Microsoft::WRL::ComPtr;

namespace REON {

	Shader::Shader() {
	}

	void Shader::Load(const std::string& name, std::any metadata)
	{
		m_Path = name;
		//vertex, fragment, geometry
		auto shaderFiles = std::any_cast<std::tuple<const char*, const char*, std::optional<std::string>>>(metadata);
		std::string vertexFileName = std::get<0>(shaderFiles);
		std::string fragmentFileName = std::get<1>(shaderFiles);
		std::string geometryFileName = std::get<2>(shaderFiles).value_or("");
		m_VertexPath = std::string("Assets/Shaders/" + vertexFileName);
		m_FragmentPath = std::string("Assets/Shaders/" + fragmentFileName);
		if (!geometryFileName.empty()) {
			m_GeometryPath = std::string("Assets/Shaders/" + geometryFileName);
		}
	}

	void Shader::Unload()
	{

	}

	std::vector<char> Shader::CompileHLSLToSPIRV(const std::string& source)
	{
		std::wstring filename(source.begin(), source.end());

		HRESULT hres;

		ComPtr<IDxcLibrary> library;
		hres = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
		REON_CORE_ASSERT(SUCCEEDED(hres), "Could not init DXC Library");

		ComPtr<IDxcCompiler3> compiler;
		hres = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		REON_CORE_ASSERT(SUCCEEDED(hres), "Could not init DXC Compiler");

		ComPtr<IDxcUtils> utils;
		hres = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		REON_CORE_ASSERT(SUCCEEDED(hres), "Could not init DXC Utility");

		uint32_t codePage = DXC_CP_ACP;
		ComPtr<IDxcBlobEncoding> sourceBlob;
		hres = utils->LoadFile(filename.c_str(), &codePage, &sourceBlob);
		REON_CORE_ASSERT(SUCCEEDED(hres), "Could not load shader file");

		LPCWSTR targetProfile{};
		size_t idx = filename.rfind('.');
		if (idx != std::string::npos) {
			std::wstring extension = filename.substr(idx + 1);
			if (extension == L"vert") {
				targetProfile = L"vs_6_1";
			}
			if (extension == L"frag") {
				targetProfile = L"ps_6_1";
			}
		}

		std::vector<LPCWSTR> arguments = {
			// (Optional) name of the shader file to be displayed e.g. in an error message
			filename.c_str(),
			// Shader main entry point
			L"-E", L"main",
			// Shader target profile
			L"-T", targetProfile,
			// Compile to SPIRV
			L"-spirv",
			//DXC_ARG_PACK_MATRIX_COLUMN_MAJOR,
			L"-fspv-debug=vulkan-with-source"
		};

		DxcBuffer buffer{};
		buffer.Encoding = DXC_CP_ACP;
		buffer.Ptr = sourceBlob->GetBufferPointer();
		buffer.Size = sourceBlob->GetBufferSize();

		ComPtr<IDxcResult> result{ nullptr };
		hres = compiler->Compile(&buffer, arguments.data(), static_cast<uint32_t>(arguments.size()), nullptr, IID_PPV_ARGS(&result));

		if (SUCCEEDED(hres)) {
			result->GetStatus(&hres);
		}

		if (FAILED(hres) && (result)) {
			ComPtr<IDxcBlobEncoding> errorBlob;
			hres = result->GetErrorBuffer(&errorBlob);
			if (SUCCEEDED(hres) && errorBlob) {
				REON_CORE_ERROR("Shader compilation failed: \n\n{}", (const char*)errorBlob->GetBufferPointer());
				throw std::runtime_error("Compilation failed");
			}
		}

		ComPtr<IDxcBlob> code;
		result->GetResult(&code);

		const char* byteCodePtr = reinterpret_cast<const char*>(code->GetBufferPointer());
		std::vector<char> shaderCode(byteCodePtr, byteCodePtr + code->GetBufferSize());

		return shaderCode;
	}

	void Shader::ReloadShader() {
		
	}

}