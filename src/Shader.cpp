#include "Shader.h"

#include "GlLoader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Failed to open shader: " << path << '\n';
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static unsigned int compileStage(unsigned int type, const std::string& src) {
    const char* csrc = src.c_str();
    const unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &csrc, nullptr);
    glCompileShader(shader);
    int ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << '\n';
    }
    return shader;
}

bool Shader::loadFromFiles(const std::string& vertPath, const std::string& fragPath) {
    const std::string vsrc = readFile(vertPath);
    const std::string fsrc = readFile(fragPath);
    if (vsrc.empty() || fsrc.empty()) return false;

    const unsigned int vs = compileStage(GL_VERTEX_SHADER, vsrc);
    const unsigned int fs = compileStage(GL_FRAGMENT_SHADER, fsrc);
    id = glCreateProgram();
    glAttachShader(id, vs);
    glAttachShader(id, fs);
    glLinkProgram(id);
    glDeleteShader(vs);
    glDeleteShader(fs);

    int linked = 0;
    glGetProgramiv(id, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024];
        glGetProgramInfoLog(id, 1024, nullptr, log);
        std::cerr << "Program link error:\n" << log << '\n';
        return false;
    }
    return true;
}

void Shader::use() const { glUseProgram(id); }

void Shader::setBool(const char* name, bool value) const {
    glUniform1i(glGetUniformLocation(id, name), static_cast<int>(value));
}

void Shader::setInt(const char* name, int value) const {
    glUniform1i(glGetUniformLocation(id, name), value);
}

void Shader::setFloat(const char* name, float value) const {
    glUniform1f(glGetUniformLocation(id, name), value);
}

void Shader::setVec3(const char* name, const glm::vec3& v) const {
    glUniform3fv(glGetUniformLocation(id, name), 1, &v[0]);
}

void Shader::setMat4(const char* name, const glm::mat4& m) const {
    glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, &m[0][0]);
}
