#pragma once

#include <jngl.hpp>

class ShaderCache : public jngl::Singleton<ShaderCache> {
public:
    jngl::ShaderProgram& get(std::string_view name);

private:
    struct StringHash {
        using is_transparent = void;
        size_t operator()(std::string_view s) const noexcept {
            return std::hash<std::string_view>{}(s);
        }
    };

    struct Shader {
        jngl::Shader fragment;
        jngl::ShaderProgram program;
    };
    std::unordered_map<std::string, Shader, StringHash, std::equal_to<>> cache;
};
