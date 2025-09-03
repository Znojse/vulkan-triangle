#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "hello_triangle_application.hpp"

int main() {
    vt::triangle::HelloTriangleApplication app;

    std::cout << "Hello Vulkan Triangle!" << std::endl;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
