#include "renderer/VulkanApp.hpp"

#include <exception>
#include <iostream>

int main() {
    try {
        VulkanApp app;
        app.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << std::endl;
        return 1;
    }
}
