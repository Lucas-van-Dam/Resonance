#include "reonpch.h"
#include "ProjectManager.h"

#include "REON/Events/EventBus.h"
#include "REON/GameHierarchy/Components/Transform.h"

namespace REON::EDITOR {

	ProjectManager::ProjectManager() : isProjectOpen(false) {
		serializers["int"] = [](const void* data) {
			return std::to_string(*static_cast<const int*>(data));
			};

		serializers["float"] = [](const void* data) {
			return std::to_string(*static_cast<const float*>(data));
			};

		serializers["glm::vec3"] = [](const void* data) {
			const auto& vec = *static_cast<const glm::vec3*>(data);
			return "(" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + ")";
			};

		serializers["glm::uvec2"] = [](const void* data) {
			const auto& vec = *static_cast<const glm::uvec2*>(data);
			return "(" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ")";
			};

		serializers["Quaternion"] = [](const void* data) {
			const auto& quat = *static_cast<const Quaternion*>(data);
			return "(" + std::to_string(quat.x) + ", " + std::to_string(quat.y) + ", " + std::to_string(quat.z) + ", " + std::to_string(quat.w) + ")";
			};

		serializers["std::string"] = [](const void* data) {
			return fmt::format("\"{}\"", *static_cast<const std::string*>(data));
			};

		serializers["LightType"] = [](const void* data) {
			const auto& lightType = *static_cast<const LightType*>(data);
			return std::to_string((int)lightType);
			};

		RegisterDeserializer<int>("int", [](const std::string& str) {return std::stoi(str); });
		RegisterDeserializer<float>("float", [](const std::string& str) {return std::stof(str); });
		RegisterDeserializer<glm::vec3>("glm::vec3", [](const std::string& str) {
			glm::vec3 result;

			// Find the positions of the first and last parentheses
			size_t first = str.find('(');
			size_t last = str.find(')');

			if (first == std::string::npos || last == std::string::npos || first >= last) {
				throw std::runtime_error("Invalid glm::vec3 format: " + str);
			}

			// Extract the inner string between the parentheses
			std::string inner = str.substr(first + 1, last - first - 1);

			// Split the inner string by commas
			size_t pos1 = inner.find(',');
			size_t pos2 = inner.find(',', pos1 + 1);

			if (pos1 == std::string::npos || pos2 == std::string::npos) {
				throw std::runtime_error("Invalid glm::vec3 format: " + str);
			}

			// Extract x, y, z substrings and convert to floats
			result.x = std::stof(inner.substr(0, pos1));
			result.y = std::stof(inner.substr(pos1 + 1, pos2 - pos1 - 1));
			result.z = std::stof(inner.substr(pos2 + 1));

			return result;
			});

		RegisterDeserializer<glm::uvec2>("glm::uvec2", [](const std::string& str) {
			glm::uvec2 result;

			// Find the positions of the first and last parentheses
			size_t first = str.find('(');
			size_t last = str.find(')');

			if (first == std::string::npos || last == std::string::npos || first >= last) {
				throw std::runtime_error("Invalid glm::uvec2 format: " + str);
			}

			// Extract the inner string between the parentheses
			std::string inner = str.substr(first + 1, last - first - 1);

			// Split the inner string by commas
			size_t pos1 = inner.find(',');

			if (pos1 == std::string::npos) {
				throw std::runtime_error("Invalid glm::uvec2 format: " + str);
			}

			// Extract x, y substrings and convert to floats
			result.x = std::stof(inner.substr(0, pos1));
			result.y = std::stof(inner.substr(pos1 + 1));

			return result;
			});

		RegisterDeserializer<Quaternion>("Quaternion", [](const std::string& str) {
			Quaternion result;

			// Find the positions of the first and last parentheses
			size_t first = str.find('(');
			size_t last = str.find(')');

			if (first == std::string::npos || last == std::string::npos || first >= last) {
				throw std::runtime_error("Invalid Quaternion format: " + str);
			}

			// Extract the inner string between the parentheses
			std::string inner = str.substr(first + 1, last - first - 1);

			// Split the inner string by commas
			size_t pos1 = inner.find(',');
			size_t pos2 = inner.find(',', pos1 + 1);
			size_t pos3 = inner.find(',', pos2 + 1);

			if (pos1 == std::string::npos || pos2 == std::string::npos || pos3 == std::string::npos) {
				throw std::runtime_error("Invalid Quaternion format: " + str);
			}

			// Extract x, y, z, w substrings and convert to floats
			result.x = std::stof(inner.substr(0, pos1));
			result.y = std::stof(inner.substr(pos1 + 1, pos2 - pos1 - 1));
			result.z = std::stof(inner.substr(pos2 + 1, pos3 - pos2 - 1));
			result.w = std::stof(inner.substr(pos3 + 1));

			return result;
			});

		RegisterDeserializer<std::string>("std::string", [](const std::string& str) {
			// Check if the string starts and ends with double quotes
			if (str.size() < 2 || str.front() != '"' || str.back() != '"') {
				throw std::runtime_error("Invalid std::string format: " + str);
			}

			// Extract the inner content without the quotes
			return str.substr(1, str.size() - 2);
			});

		RegisterDeserializer<LightType>("LightType", [](const std::string& str) {
			int value = std::stoi(str); // Convert string to int
			return static_cast<LightType>(value); // Convert int to LightType
			});
	}

	bool ProjectManager::CreateNewProject(const std::string& projectName, const std::string& targetDirectory) {
		m_CurrentProjectPath = targetDirectory + "/" + projectName;

		if (!std::filesystem::exists(m_CurrentProjectPath)) {
			std::filesystem::create_directories(m_CurrentProjectPath);
			InitializeFolders();
			InitializeDefaultFiles(projectName);
			isProjectOpen = true;
			ProjectOpenedEvent event(m_CurrentProjectPath);
			EventBus::Get().publish(event);
			return true;
		}
		else {
			return false;
		}
	}

	bool ProjectManager::OpenProject(const std::string& projectPath) {
		std::error_code ec;
		if (!std::filesystem::exists(projectPath, ec)) {
			REON_ERROR("Project path does not exist: {0}.\n", ec.message());
			return false;
		}

		m_CurrentProjectPath = projectPath;

		if (!std::filesystem::exists(m_CurrentProjectPath + "/Settings/ProjectSettings.json", ec)) {
			REON_ERROR("Error: Missing ProjectSettings.json in the project.\n");
			return false;
		}

		SettingsManager::GetInstance().LoadSettings(m_CurrentProjectPath);

		auto scene = std::make_shared<Scene>("TestScene");
		scene->InitializeSceneWithObjects();

		SceneManager::Get()->SetActiveScene(scene);

		ProjectOpenedEvent event(projectPath);
		EventBus::Get().publish(event);

		return true;
	}

	bool ProjectManager::SaveProject()
	{
		if (m_CurrentProjectPath.empty()) {
			REON_ERROR("No project loaded");
			return false;
		}

		if (!std::filesystem::exists(m_CurrentProjectPath)) {
			REON_ERROR("Project does not exist at: {}", m_CurrentProjectPath);
			return false;
		}

		SettingsManager::GetInstance().SaveSettings(m_CurrentProjectPath);

		if (!SaveScenes()) {
			REON_ERROR("Failed to save scenes");
			return false;
		}

		return true;
	}

	void ProjectManager::InitializeFolders() {
		std::filesystem::create_directories(m_CurrentProjectPath + "/Assets/Materials");
		std::filesystem::create_directories(m_CurrentProjectPath + "/Assets/Models");
		std::filesystem::create_directories(m_CurrentProjectPath + "/Assets/Shaders");
		std::filesystem::create_directories(m_CurrentProjectPath + "/Assets/Textures");
		std::filesystem::create_directories(m_CurrentProjectPath + "/Assets/Videos");
		std::filesystem::create_directories(m_CurrentProjectPath + "/Assets/Scenes");
		std::filesystem::create_directories(m_CurrentProjectPath + "/Settings");
		std::filesystem::create_directories(m_CurrentProjectPath + "/Outputs/Renders");
		std::filesystem::create_directories(m_CurrentProjectPath + "/Outputs/Exports");
		std::filesystem::create_directories(m_CurrentProjectPath + "/Logs");
		std::filesystem::create_directories(m_CurrentProjectPath + "/EngineCache");
	}

	void ProjectManager::InitializeDefaultFiles(const std::string& projectName) {
		nlohmann::json projectSettings = {
			{"projectName", projectName},
			{"version", 1}
		};
		std::ofstream settingsFile(m_CurrentProjectPath + "/Settings/ProjectSettings.json");
		settingsFile << projectSettings.dump(4);

		nlohmann::json audioSettings = {
			{"device", "Default Microphone"},
			{"sampleRate", 48000},
			{"bufferSize", 512}
		};
		std::ofstream audioSettingsFile(m_CurrentProjectPath + "/Settings/AudioInputSettings.json");
		audioSettingsFile << audioSettings.dump(4);

		nlohmann::json visualSettings = {
			{"resolution", {1920, 1080}},
			{"frameRate", 60}
		};
		std::ofstream visualSettingsFile(m_CurrentProjectPath + "/Settings/VisualSettings.json");
		visualSettingsFile << visualSettings.dump(4);
	}

	bool ProjectManager::SaveSettings()
	{
		return false;
	}

	bool ProjectManager::SaveScene(const REON::Scene& scene)
	{
		return false;
	}

	bool ProjectManager::SaveScenes()
	{
		auto scene = REON::SceneManager::Get()->GetCurrentScene();

		if (!scene) {
			REON_ERROR("No current scene");
			return false;
		}

		nlohmann::json jsonScene;
		jsonScene["Id"] = scene->GetID();
		jsonScene["SceneName"] = scene->GetName();

		for (const auto& gameobject : scene->GetRootObjects()) {
			jsonScene["RootObjects"].push_back(gameobject->GetID());
			SerializeGameObjectForScene(jsonScene, gameobject);
		}



		std::ofstream file(m_CurrentProjectPath + "/Assets/Scenes/Scene1.scene");
		if (file.is_open()) {
			file << jsonScene.dump(4);
			file.close();
		}
		else {
			REON_ERROR("Failed to open file for writing: {0}", m_CurrentProjectPath + "/Assets/Scenes/Scene1.scene");
			return false;
		}

		return true;
	}

	void ProjectManager::LoadScene(const std::filesystem::path& path)
	{
		std::shared_ptr<Scene> scene = std::make_shared<Scene>(path.filename().string());

		nlohmann::json j;

		std::ifstream file(path);
		if (file.is_open()) {
			file >> j;
			file.close();
		}

		scene->SetName(j["SceneName"]);
		for (const auto& object : j["RootObjects"]) {
			nlohmann::json objectJson;
			std::shared_ptr<GameObject> rootObject = std::make_shared<GameObject>(object);
			for (const auto& gameObject : j["GameObjects"]) {
				if (gameObject.contains("Id") && gameObject["Id"] == object) {
					objectJson = gameObject;
				}
			}
			if (objectJson.empty()) {
				REON_ERROR("ROOT OBJECT NOT FOUND IN SCENE");
				continue;
			}
			scene->AddGameObject(rootObject);
			DeSerializeGameObjectForScene(objectJson, rootObject, j);
		}

		SceneManager::Get()->SetActiveScene(scene);
		scene->selectedObject = nullptr;
	}

	bool ProjectManager::BuildProject(const std::filesystem::path& buildDirectory)
	{
		auto projectSettings = SettingsManager::GetInstance().GetProjectSettings();
		//Create build directory
		std::filesystem::path buildPath = buildDirectory / projectSettings.projectName; // Replace with project name later

		if (!std::filesystem::exists(buildPath)) {
			std::filesystem::create_directories(buildPath);
			std::filesystem::create_directory(buildPath / "Assets");
		}

		//copy assets to build directory now
		std::unordered_set<std::string> usedAssets = getUsedAssetsFromScene(SceneManager::Get()->GetCurrentScene());

		for (const auto& asset : usedAssets) {
			std::filesystem::path sourcePath(asset);
			std::filesystem::path newPath = buildPath / "Assets" / sourcePath.filename();
			
			try {
				std::filesystem::create_directories(newPath.parent_path());
				std::filesystem::copy_file(m_CurrentProjectPath / sourcePath, newPath, std::filesystem::copy_options::overwrite_existing);
				if (std::filesystem::exists((m_CurrentProjectPath / sourcePath).string() + ".meta")) {
					std::filesystem::copy_file((m_CurrentProjectPath / sourcePath).string() + ".meta",
						newPath.string() + ".meta",
						std::filesystem::copy_options::overwrite_existing);
				}
			}
			catch (const std::filesystem::filesystem_error& e) {
				std::cerr << "Failed to copy: " << sourcePath << " -> " << newPath << "\n"
					<< "Reason: " << e.what() << "\n";
			}
		}

		//copy scenes to build directory

		if(!std::filesystem::exists(buildPath / "Assets/Scenes")) {
			std::filesystem::create_directories(buildPath / "Assets/Scenes");
		}
		std::filesystem::copy_file(m_CurrentProjectPath + "/Assets/Scenes/Scene1.scene",
			buildPath / "Assets/Scenes/Scene1.scene",
			std::filesystem::copy_options::overwrite_existing);

		//copy executable to build directory


		//create runtime configuration file
		std::ofstream runtimeConfig(buildPath / "runtime_config.json");
		if (runtimeConfig.is_open()) {
			nlohmann::json configJson;
			configJson["projectName"] = projectSettings.projectName;
			configJson["startScene"] = "Assets/Scenes/Scene1.scene";
			configJson["WindowProperties"] = {
				{"width", 1200}, // Example width
				{"height", 800}, // Example height
				{"title", projectSettings.projectName},
				{"fullscreen", false},
				{"vsync", false},
				{"resizable", true}
			};

			configJson["AssetsPath"] = "Assets";
			configJson["ShaderPath"] = "Assets/Shaders";

			runtimeConfig << configJson.dump(4);
			runtimeConfig.close();
		}
		else {
			REON_ERROR("Failed to create runtime configuration file in build directory");
			return false;
		}

		REON_ERROR("BuildProject is not implemented yet");

		return false;
	}

	std::unordered_set<std::string> ProjectManager::getUsedAssetsFromScene(const std::shared_ptr<Scene>& scene)
	{
		AssetRegistry& assetRegistry = AssetRegistry::Instance();
		std::unordered_set<std::string> usedAssets;

		for (auto gameObject : scene->GetRootObjects()) {
			for (auto& component : gameObject->GetComponents()) {
				if (auto renderer = std::dynamic_pointer_cast<Renderer>(component)) {
					if (auto assetInfo = AssetRegistry::Instance().GetAssetById(renderer->mesh.Get<Mesh>()->GetID())) {
						usedAssets.insert(assetInfo->path.string());
					}

					for (auto material : renderer->materials) {
						auto materialResource = material.Get<Material>();
						if (auto assetInfo = AssetRegistry::Instance().GetAssetById(materialResource->GetID())) {
							usedAssets.insert(assetInfo->path.string());
						}
						usedAssets.insert(materialResource->albedoTexture.Get<Texture>()->GetPath());
						usedAssets.insert(materialResource->normalTexture.Get<Texture>()->GetPath());
						usedAssets.insert(materialResource->metallicRoughnessTexture.Get<Texture>()->GetPath());
						usedAssets.insert(materialResource->emissiveTexture.Get<Texture>()->GetPath());
					}
				}

			}
		}

		return usedAssets;
	}

	void ProjectManager::DeSerializeGameObjectForScene(const nlohmann::json& objectJson, std::shared_ptr<GameObject> object, const nlohmann::json& sceneJson)
	{
		object->SetName(objectJson["Name"]);

		if (objectJson.contains("Transform")) {
			auto transformJson = objectJson["Transform"];
			auto transform = object->GetTransform();
			transform->localPosition = Deserialize<glm::vec3>("glm::vec3", transformJson["Position"]);
			transform->localRotation = Deserialize<Quaternion>("Quaternion", transformJson["Rotation"]);
			transform->localScale = Deserialize<glm::vec3>("glm::vec3", transformJson["Scale"]);
		}
		else {
			REON_ERROR("No transform found in gameobject");
		}

		if (objectJson.contains("Components")) {
			for (const auto& componentJson : objectJson["Components"]) {

				if(componentJson["Type"] == "Transform") {
					// Transform is already handled above, skip it
					continue;
				}
				else if(componentJson["Type"] == "Light") {
					auto light = std::make_shared<Light>();
					light->Deserialize(componentJson, GetCurrentProjectPath());
					object->AddComponent(light);
					continue;
				}
				else if(componentJson["Type"] == "Renderer") {
					auto renderer = std::make_shared<Renderer>();
					renderer->Deserialize(componentJson, GetCurrentProjectPath());
					object->AddComponent(renderer);
					continue;
				}
				else if(componentJson["Type"] == "Camera"){
					auto camera = std::make_shared<Camera>();
					camera->Deserialize(componentJson, GetCurrentProjectPath());
					object->AddComponent(camera);
					continue;
				}
				else {
					//REON_CORE_WARN("Unsupported component type: {0}", componentJson["Type"]);
					continue; // Skip unsupported component types
				}
				auto baseComponent = ReflectionRegistry::Instance().Create(componentJson["Name"]);
				const ReflectionClass* componentStructure = ReflectionRegistry::Instance().GetClass(componentJson["Name"]);
				for (auto it = componentJson.begin(); it != componentJson.end(); ++it) {
					const std::string& key = it.key();
					if (key == "Name") continue;
					auto fieldIter = std::find_if(componentStructure->fields, componentStructure->fields + componentStructure->field_count, [&](const FieldInfo& field) {
						return field.name == key;
						});

					if (fieldIter != componentStructure->fields + componentStructure->field_count) {
						std::string stringType = std::string(fieldIter->type);
						void* data;
						if (stringType == "ResourceHandle") {
							std::shared_ptr<ResourceBase> resource;
							if (!(resource = ResourceManager::GetInstance().GetResource<ResourceBase>(it.value()))) {
								std::string value = it.value();
								auto assetInfo = AssetRegistry::Instance().GetAssetById(value);
								REON_CORE_ASSERT(assetInfo);
								resource = ResourceManager::GetInstance().GetResourceFromAsset(assetInfo, GetCurrentProjectPath());
								ResourceManager::GetInstance().AddResource(resource);
							}
							ResourceHandle handle(std::move(resource));
							data = static_cast<void*>(&handle);
							if (data) {
								std::cout << "Setting field: " << fieldIter->name << " with handle address: " << &handle << std::endl;
								fieldIter->setter(baseComponent.get(), &handle);
								auto checkHandle = fieldIter->getter(baseComponent.get());
								std::cout << "Stored handle address: " << checkHandle << std::endl;
							}
						}
						else if (stringType == "std::vector<ResourceHandle>") {
							std::string value = it.value();
							std::vector<ResourceHandle> handles;
							if (value.front() == '[' && value.back() == ']') {
								std::string inner = value.substr(1, value.size() - 2);
								std::stringstream ss(inner);
								std::string handleId;
								while (std::getline(ss, handleId, ',')) {
									handleId.erase(remove(handleId.begin(), handleId.end(), ' '), handleId.end());
									std::shared_ptr<ResourceBase> resource;
									if (!handleId.empty() && !(resource = ResourceManager::GetInstance().GetResource<ResourceBase>(it.value()))) {
										auto assetInfo = AssetRegistry::Instance().GetAssetById(handleId);
										if (assetInfo) {
											resource = ResourceManager::GetInstance().GetResourceFromAsset(assetInfo, GetCurrentProjectPath());
											ResourceManager::GetInstance().AddResource(resource);
											ResourceHandle handle(std::move(resource));
											handles.push_back(handle);
										}
									}
								}
							}
							if (!handles.empty()) {
								data = static_cast<void*>(&handles);
								fieldIter->setter(baseComponent.get(), data);
							}
							else {
								REON_ERROR("No valid ResourceHandles found in vector for field: {0}", fieldIter->name);
							}
						}
						else {
							data = DeserializeToVoid(stringType, it.value());
							if (data)
								fieldIter->setter(baseComponent.get(), data);
						}
					}
				}
				auto component = std::dynamic_pointer_cast<Component>(baseComponent);
				object->AddComponent(component);
			}
		}

		if (objectJson.contains("Children")) {
			nlohmann::json j;
			for (const auto& child : objectJson["Children"]) {
				std::shared_ptr<GameObject> childObject = std::make_shared<GameObject>(child);
				for (const auto& gameObject : sceneJson["GameObjects"]) {
					if (gameObject.contains("Id") && gameObject["Id"] == child) {
						j = gameObject;
					}
				}
				if (j.empty()) {
					REON_ERROR("CHILD NOT FOUND IN SCENE");
					continue;
				}

				object->AddChild(childObject);

				DeSerializeGameObjectForScene(j, childObject, sceneJson);
			}
		}
	}

	void ProjectManager::SerializeGameObjectForScene(nlohmann::json& sceneJson, std::shared_ptr<REON::GameObject> object) {
		sceneJson["GameObjects"].push_back(SerializeGameObject(object));

		for (auto& child : object->GetChildren()) {
			SerializeGameObjectForScene(sceneJson, child);
		}
	}

	nlohmann::json ProjectManager::SerializeGameObject(std::shared_ptr<REON::GameObject> object) {
		nlohmann::json jsonObject;
		for (auto& child : object->GetChildren()) {
			jsonObject["Children"].push_back(child->GetID());
		}

		auto transform = object->GetTransform();
		nlohmann::json jsonTransform;
		jsonTransform["Position"] = serializers["glm::vec3"](&transform->localPosition);
		jsonTransform["Rotation"] = serializers["Quaternion"](&transform->localRotation);
		jsonTransform["Scale"] = serializers["glm::vec3"](&transform->localScale);

		jsonObject["Transform"] = jsonTransform;

		for (auto& component : object->GetComponents()) {
			nlohmann::json jsonComponent;

			jsonComponent = component->Serialize();
			jsonObject["Components"].push_back(jsonComponent);
			continue;

			jsonComponent["Name"] = component->GetTypeName();
			auto reflectionClass = ReflectionRegistry::Instance().GetClass(component->GetTypeName());
			for (int i = 0; i < reflectionClass->field_count; i++) {
				auto& field = reflectionClass->fields[i];

				void* baseAddress = reinterpret_cast<std::uint8_t*>(component.get());

				void* fieldPtr = field.getter(baseAddress);

				// Now pass the field pointer to the serializer
				jsonComponent[field.name] = SerializeField(field.type, fieldPtr);
			}
			jsonObject["Components"].push_back(jsonComponent);

		}

		jsonObject["Name"] = object->GetName();

		jsonObject["Id"] = object->GetID();

		return jsonObject;
	}

	std::string ProjectManager::SerializeField(const char* fieldType, const void* data) {
		static auto dummy = std::make_shared<REON::Object>();
		auto it = serializers.find(fieldType);
		if (it != serializers.end()) {
			return it->second(data); // Call the corresponding serializer
		}
		else {
			try {
				std::string stringType = std::string(fieldType);
				if (stringType == "ResourceHandle") {
					auto sharedPtr = reinterpret_cast<const ResourceHandle*>(data);
					return sharedPtr->Get<ResourceBase>()->GetID();
				}
				else if (stringType == "std::vector<ResourceHandle>") {
					const auto& handles = *static_cast<const std::vector<ResourceHandle>*>(data);
					std::string result = "[";
					for (const auto& handle : handles) {
						if (handle.Get<ResourceBase>()) {
							result += handle.Get<ResourceBase>()->GetID() + ", ";
						}
					}
					if (result.size() > 1) {
						result.pop_back(); // Remove last comma
						result.pop_back(); // Remove last space
					}
					result += "]";
					return result;
				}
				REON_WARN("No serializer found for type: {0}", fieldType);
				return "";
			}
			catch (std::exception ex) {
				REON_CORE_ERROR("Exception while trying to serialize field type {0}: {1}.", fieldType, ex.what());
			}
		}
	}

	std::string ProjectManager::ExtractInnerType(const std::string& typeString) {
		// Find the position of '<' and '>'
		size_t start = typeString.find('<');
		size_t end = typeString.find('>');

		// Ensure both characters are found
		if (start == std::string::npos || end == std::string::npos || end <= start) {
			return ""; // Return an empty string if the format is invalid
		}

		// Extract the substring inside the angle brackets
		return typeString.substr(start + 1, end - start - 1);
	}

}
