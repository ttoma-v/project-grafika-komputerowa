#pragma once

#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    unsigned int id = 0;

    Shader() = default;
    bool loadFromFiles(const std::string& vertPath, const std::string& fragPath);
    void use() const;
    void setBool(const char* name, bool value) const;
    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setVec3(const char* name, const glm::vec3& v) const;
    void setMat4(const char* name, const glm::mat4& m) const;
};
