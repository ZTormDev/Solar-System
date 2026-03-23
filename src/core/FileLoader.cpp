#include "FileLoader.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace FileLoader {
std::vector<char> readBinaryFile(const std::string& filename) {
    std::vector<std::filesystem::path> candidates = {
        std::filesystem::path(filename),
        std::filesystem::path("..") / filename,
        std::filesystem::path("..") / ".." / filename,
        std::filesystem::path("build") / filename,
        std::filesystem::path("..") / "build" / filename
    };

    std::ifstream file;
    for (const auto& candidate : candidates) {
        file = std::ifstream(candidate, std::ios::ate | std::ios::binary);
        if (file.is_open()) {
            break;
        }
    }

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename + " (tried cwd, ../ and ../../)");
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    file.close();

    return buffer;
}
}
