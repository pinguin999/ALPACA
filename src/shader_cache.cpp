#include "shader_cache.hpp"

namespace {
void replaceAll(std::string& subject, const std::string& search, const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

std::stringstream loadAndReplace(std::string_view name, int width, int height) {
    std::stringstream buffer = jngl::readAsset(std::string(name) + ".frag");
    std::string tmp = buffer.str();
    replaceAll(tmp, "FBO_WIDTH", std::to_string(width) + ".f");
    replaceAll(tmp, "FBO_HEIGHT", std::to_string(height) + ".f");
    return std::stringstream(tmp);
}
} // namespace

jngl::ShaderProgram& ShaderCache::get(std::string_view name) {
    auto it = cache.find(name);
    if (it != cache.end()) {
        return it->second.program;
    }

    jngl::Shader fragment(loadAndReplace(name, jngl::getWindowWidth(),
                                         jngl::getWindowHeight()),
                          jngl::Shader::Type::FRAGMENT);
    jngl::ShaderProgram program(jngl::Sprite::vertexShader(), fragment);
    auto [inserted, _] = cache.emplace(std::string(name),
                                       Shader{ .fragment = std::move(fragment), .program = std::move(program) });
    return inserted->second.program;
}
