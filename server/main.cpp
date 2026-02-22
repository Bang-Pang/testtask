#include "server.hpp"

#include <iostream>
#include <cstdlib>
#include <string>
#include <cmath>

int main(int argc, char* argv[]) {
    // Defaults
    int         port         = 9000;
    int         wait_seconds = 10;
    std::string function     = "sin";
    double      a            = 0.0;
    double      b            = M_PI;
    int         total_n      = 1'000'000;
    int         method       = 0; // RECTANGLE

    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        std::cout << "Usage: server [port] [wait_sec] [function] [a] [b] [n] [method]\n"
                  << "  function: sin, cos, x2, x3\n"
                  << "  method:   0=rectangle  1=trapezoid  2=simpson\n";
        return EXIT_SUCCESS;
    }

    if (argc > 1) port         = std::stoi(argv[1]);
    if (argc > 2) wait_seconds = std::stoi(argv[2]);
    if (argc > 3) function     = argv[3];
    if (argc > 4) a            = std::stod(argv[4]);
    if (argc > 5) b            = std::stod(argv[5]);
    if (argc > 6) total_n      = std::stoi(argv[6]);
    if (argc > 7) method       = std::stoi(argv[7]);

    try {
        integration::Server srv(port, wait_seconds, function, a, b, total_n, method);
        srv.run();
    } catch (const std::exception& e) {
        std::cerr << "[server] Fatal: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
