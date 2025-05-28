require("Vendor/ecc/ecc")

workspace "Resonance"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "Editor"

OutputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

   -- Workspace-wide build options
   filter "system:windows"
      toolset "clang"
      buildoptions
      {
         "-m64",
         "-Wno-pragma-pack"
      }
      defines
      {
         "_WIN32",
         "_CRT_SECURE_NO_WARNINGS",
         "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS",
         "REON_EDITOR",
         "GLM_ENABLE_EXPERIMENTAL",
         "GLM_FORCE_DEPTH_ZERO_TO_ONE",
         "VK_USE_PLATFORM_WIN32_KHR",
      }
      disablewarnings
      {
         "/Wno-pragma-pack",
         "/Wno-pragma-once-outside-header"
      }

group "Core"
	include "Resonance-Core/Build-Core.lua"
group ""

include "Resonance-Editor/Build-Editor.lua"
--include "Resonance-Runtime/Build-Runtime.lua"

IncludeDir = {}
IncludeDir["GLFW"] = "vendor/GLFW/include"
IncludeDir["GLAD"] = "vendor/GLAD/include"
IncludeDir["ImGui"] = "vendor/imgui"
IncludeDir["glm"] = "vendor/glm"
IncludeDir["stb_image"] = "vendor/stb_image"
IncludeDir["assimp"] = "vendor/assimp/include"
IncludeDir["ImGuizmo"] = "vendor/ImGuizmo"
IncludeDir["json"] = "vendor/json/include"
IncludeDir["Tracy"] = "vendor/Tracy"
IncludeDir["Vulkan"] = "vendor/Vulkan/include"
IncludeDir["MikkTSpace"] = "vendor/MikkTSpace"
IncludeDir["cppcodec"] = "vendor/cppcodec"

include "Resonance-Core/vendor/GLFW/premake5.lua"
include "Resonance-Core/vendor/GLAD/premake5.lua"
--include "Resonance-Core/vendor/imgui/premake5.lua"
include "Resonance-Core/vendor/assimp/premake5.lua"