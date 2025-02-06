# Resonance
Resonance is an engine focussed towards real-time audiovisualisation written in C++ 20 and GLSL, using premake as its buildsystem

## Features
### Rendering
- Full PRB renderer with IBL
- Model loading through Assimp
- Basic support for post processing (preset post processing effects coming later)

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
To build this project, make sure you have llvm downloaded on your machine, if you chose a non-standard install path change the llvm path in the Directory.build.props. Clone the project onto your machine, run the GenerateReflectionData.bat and Setup-Windows.bat scripts in the scripts folder to generate all the project files and link the dependencies, then just open the solution and build.

