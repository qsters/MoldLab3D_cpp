#include "ShaderVariable.h"

// Specializations for supported types
template <>
void ShaderVariable<vec2>::upload() const {
    glUniform2f(location, (*value)[0], (*value)[1]);
}

template <>
void ShaderVariable<vec3>::upload() const {
    glUniform3f(location, (*value)[0], (*value)[1], (*value)[2]);
}

template <>
void ShaderVariable<float>::upload() const {
    glUniform1f(location, *value);
}

template <>
void ShaderVariable<int>::upload() const {
    glUniform1i(location, *value);
}
