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
				auto baseComponent = ReflectionRegistry::Instance().Create(componentJson["Name"]);
				const ReflectionClass* componentStructure = ReflectionRegistry::Instance().GetClass(componentJson["Name"]);
				for (auto it = componentJson.begin(); it != componentJson.end(); ++it) {
					const std::string& key = it.key();
					if (key == "Name") continue;
					auto fieldIter = std::find_if(componentStructure->fields, componentStructure->fields + componentStructure->field_count,[&](const FieldInfo& field) {
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
