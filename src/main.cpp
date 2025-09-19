#include <cstdlib>
#include <exception>
#include <iostream>

#include "hello_triangle_application.hpp"

auto main() -> int {
    vt::triangle::HelloTriangleApplication app;

    std::cout << "Hello Vulkan Triangle!\n";

    try {
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
