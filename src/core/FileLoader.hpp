#pragma once

#include <string>
#include <vector>

namespace FileLoader {
std::vector<char> readBinaryFile(const std::string& filename);
}
