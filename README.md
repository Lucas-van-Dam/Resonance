# Resonance
Resonance is an engine focussed towards creating real-time visuals based on audio written in C++ 20 and HLSL, using premake as its buildsystem

## Features
### Rendering
- Full PRB renderer with OIT
- Model loading through TinyGLTF
- Vulkan renderer (Render graph coming soon)

### Gameobject Component Patern
- Parent child structure
- Custom component support using templates

## Dependencies
- Clang
- GLFW
- OpenGL
- GLM
- GLAD
- Assimp (being depricated)
- tinyGLTF
- ImGui
- ImGuizmo
- stb_image

# Building
Currently this project only supports MSVC, Clang will be supported in the near future as well, to build this project, clone it onto a location of your choosing, make sure to do a recursive clone to get all the submodules. Then run the .bat scripts in the scripts folder to generate the needed reflection and project files. Now just run the Editor project and youre done!

