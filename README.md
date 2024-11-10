# Resonance
Resonance is an audio visual engine written in C++ 20 and GLSL, using premake as its buildsystem

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
- Assimp
- ImGui
- ImGuizmo
- stb_image

# Building
To build this project, make sure you have llvm downloaded on your machine, if you chose a non-standard install path change the llvm path in the Directory.build.props. Clone the project onto your machine, run the Setup-Windows.bat script in the scripts folder to generate all the project files and link the dependencies, then just open the solution and build.

