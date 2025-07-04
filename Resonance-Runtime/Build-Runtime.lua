project "Runtime"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "on"

   files { "Source/**.h", "Source/**.cpp"}

   includedirs
   {
      "Source",
      "Vendor/ImGuiFileDialog",
      "Vendor/libstud-uuid",
      "Vendor/tinygltf",
      
	  -- Include Core
	  "../Resonance-Core/Source",
      "../Resonance-Core/vendor/spdlog/include",
      "../Resonance-Core/vendor",

      "../Resonance-Core/%{IncludeDir.json}",
      "../Resonance-Core/%{IncludeDir.GLFW}",
      "../Resonance-Core/%{IncludeDir.GLAD}",
      "../Resonance-Core/%{IncludeDir.ImGui}",
      "../Resonance-Core/%{IncludeDir.glm}",
      "../Resonance-Core/%{IncludeDir.stb_image}",
      "../Resonance-Core/%{IncludeDir.assimp}",
      "../Resonance-Core/%{IncludeDir.ImGuizmo}",
      "../Resonance-Core/%{IncludeDir.Vulkan}",
      "../Resonance-Core/%{IncludeDir.MikkTSpace}",
      "../Resonance-Core/%{IncludeDir.cppcodec}",
      "../Resonance-Core/%{IncludeDir.stduuid}",
   }

   dependson
   {
    "Core"
   }

   libdirs{ "../Resonance-Core/vendor/Vulkan/Lib" }

   links
   {
      "Core","rpcrt4"
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines 
        { 
            "WINDOWS", 
            "REON_PLATFORM_WINDOWS",
        }

   filter "configurations:Debug"
       defines { "REON_DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "REON_RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "REON_DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"