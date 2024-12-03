project "Core"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "on"

   pchheader "reonpch.h"
   pchsource "Source/reonpch.cpp"

   files 
   { 
    "Source/**.h", 
    "Source/**.cpp",
    "Reflection/**.cpp",
    "Reflection/**.h",
    "vendor/glm/glm/**.hpp",
    "vendor/glm/glm/**.inl",
    "vendor/stb_image/**.cpp",
    "vendor/stb_image/**.h",
    "vendor/ImGuizmo/**.h",
    "vendor/ImGuizmo/**.cpp"
   }

   includedirs
   {
      "Source",
      "vendor/spdlog/include",
      "%{IncludeDir.GLFW}",
      "%{IncludeDir.GLAD}",
      "%{IncludeDir.ImGui}",
      "%{IncludeDir.glm}",
      "%{IncludeDir.stb_image}",
      "%{IncludeDir.assimp}",
      "%{IncludeDir.ImGuizmo}",
   }

   links{
      "GLFW",
      "GLAD",
      "ImGui",
      "assimp",
      "opengl32.lib"
   }

   local function generateRegistrationHeader()
    local output_file = path.join("Reflection", "ReflectionRegistry_Registration.h")
    local reflections = os.matchfiles("Reflection/*.cpp")

    local file = io.open(output_file, "w")
    file:write("#pragma once\n\n")

    -- Write includes for each generated reflection file
    for _, reflection_file in ipairs(reflections) do
        file:write('#include "' .. path.getname(reflection_file) .. '"\n')
    end

    file:close()
   end

-- Call the function to generate the header
    generateRegistrationHeader()

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines 
        { 
            "REON_PLATFORM_WINDOWS" ,
            "GLFW_INCLUDE_NONE"
        }

   filter "configurations:Debug"
       defines { "REON_DEBUG", "REON_ENABLE_ASSERTS" }
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