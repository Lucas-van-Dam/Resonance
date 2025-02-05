#include "reonpch.h"
#include "Shader.h"

namespace REON {

    Shader::Shader() {
    }

    void Shader::use() const {
        glUseProgram(ID);
    }

    void Shader::setBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void Shader::setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void Shader::setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    // ------------------------------------------------------------------------
    void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void Shader::setVec2(const std::string& name, float x, float y) const {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }

    // ------------------------------------------------------------------------
    void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void Shader::setVec3(const std::string& name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

    // ------------------------------------------------------------------------
    void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void Shader::setVec4(const std::string& name, float x, float y, float z, float w) const {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }

    // ------------------------------------------------------------------------
    void Shader::setMat2(const std::string& name, const glm::mat2& mat) const {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    // ------------------------------------------------------------------------
    void Shader::setMat3(const std::string& name, const glm::mat3& mat) const {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    // ------------------------------------------------------------------------
    void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void Shader::checkCompileErrors(unsigned int shader, const std::string& type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog
                    << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog
                    << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }

    void Shader::Load(const std::string& name, std::any metadata)
    {
        m_Path = name;
        //vertex, fragment, geometry
        auto shaderFiles = std::any_cast<std::tuple<const char*, const char*, std::optional<std::string>>>(metadata);
        std::string vertexFileName = std::get<0>(shaderFiles);
        std::string fragmentFileName = std::get<1>(shaderFiles);
        std::string geometryFileName = std::get<2>(shaderFiles).value_or("");
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::ifstream gShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            // open files
            m_VertexPath = std::string("Assets/Shaders/" + vertexFileName);
            m_FragmentPath = std::string("Assets/Shaders/" + fragmentFileName);
            vShaderFile.open(m_VertexPath);
            if (!vShaderFile.is_open()) {
                std::cerr << "Error opening file: " << strerror(errno) << std::endl;
            }
            fShaderFile.open(m_FragmentPath);
            if (!fShaderFile.is_open()) {
                std::cerr << "Error opening file: " << strerror(errno) << std::endl;
            }
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
            if (!geometryFileName.empty()) {
                m_GeometryPath = std::string("Assets/Shaders/" + geometryFileName);
                gShaderFile.open(m_GeometryPath);
                std::stringstream gShaderStream;
                gShaderStream << gShaderFile.rdbuf();
                gShaderFile.close();
                geometryCode = gShaderStream.str();
            }
        }
        catch (std::ifstream::failure e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        // vertex shader
        m_VertexID = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(m_VertexID, 1, &vShaderCode, nullptr);
        glCompileShader(m_VertexID);
        checkCompileErrors(m_VertexID, "VERTEX");
        // fragment Shader
        m_FragmentID = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(m_FragmentID, 1, &fShaderCode, nullptr);
        glCompileShader(m_FragmentID);
        checkCompileErrors(m_FragmentID, "FRAGMENT");
        // geometry shader if provided
        if (!geometryFileName.empty())
        {
            const char* gShaderCode = geometryCode.c_str();
            m_GeometryID = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(m_GeometryID, 1, &gShaderCode, nullptr);
            glCompileShader(m_GeometryID);
            checkCompileErrors(m_GeometryID, "GEOMETRY");
        }
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, m_VertexID);
        glAttachShader(ID, m_FragmentID);
        if (!geometryFileName.empty())
            glAttachShader(ID, m_GeometryID);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(m_VertexID);
        glDeleteShader(m_FragmentID);
        if (!geometryFileName.empty())
            glDeleteShader(m_GeometryID);
    }

    void Shader::Unload()
    {

    }

    void Shader::ReloadShader() {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::ifstream gShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            vShaderFile.open(m_VertexPath);
            if (!vShaderFile.is_open()) {
                std::cerr << "Error opening file: " << strerror(errno) << std::endl;
            }
            fShaderFile.open(m_FragmentPath);
            if (!fShaderFile.is_open()) {
                std::cerr << "Error opening file: " << strerror(errno) << std::endl;
            }
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
            if (!m_GeometryPath.empty()) {
                gShaderFile.open(m_GeometryPath);
                std::stringstream gShaderStream;
                gShaderStream << gShaderFile.rdbuf();
                gShaderFile.close();
                geometryCode = gShaderStream.str();
            }
        }
        catch (std::ifstream::failure e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        // vertex shader
        m_VertexID = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(m_VertexID, 1, &vShaderCode, nullptr);
        glCompileShader(m_VertexID);
        checkCompileErrors(m_VertexID, "VERTEX");
        // fragment Shader
        m_FragmentID = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(m_FragmentID, 1, &fShaderCode, nullptr);
        glCompileShader(m_FragmentID);
        checkCompileErrors(m_FragmentID, "FRAGMENT");
        // geometry shader if provided
        if (!m_GeometryPath.empty())
        {
            const char* gShaderCode = geometryCode.c_str();
            m_GeometryID = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(m_GeometryID, 1, &gShaderCode, nullptr);
            glCompileShader(m_GeometryID);
            checkCompileErrors(m_GeometryID, "GEOMETRY");
        }
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, m_VertexID);
        glAttachShader(ID, m_FragmentID);
        if (!m_GeometryPath.empty())
            glAttachShader(ID, m_GeometryID);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(m_VertexID);
        glDeleteShader(m_FragmentID);
        if (!m_GeometryPath.empty())
            glDeleteShader(m_GeometryID);
    }

}