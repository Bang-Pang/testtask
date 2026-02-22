#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>

namespace integration {

/**
 * TCP server that:
 *  1. Listens for client connections.
 *  2. Receives a RegisterMsg from each client (to learn its thread count).
 *  3. Splits the integration task into subtasks proportional to each client's
 *     thread count and sends TaskMsg to every connected client.
 *  4. Collects ResultMsg replies and prints the aggregated result.
 *
 * The server accepts clients for `wait_seconds` seconds after start, then
 * distributes tasks and waits for all results.
 */
class Server {
public:
    /**
     * @param port          TCP port to listen on.
     * @param wait_seconds  How long to wait for clients before distributing.
     * @param function      Named function to integrate ("sin", "cos", "x2").
     * @param a             Lower integration bound.
     * @param b             Upper integration bound.
     * @param total_n       Total number of sub-intervals (split across clients).
     * @param method        Integration method (0=rectangle,1=trapezoid,2=simpson).
     */
    Server(int port, int wait_seconds,
           std::string function, double a, double b,
           int total_n, int method);

    /// Start listening, distribute tasks, collect results, print answer.
    void run();

private:
    int         port_;
    int         wait_seconds_;
    std::string function_;
    double      a_, b_;
    int         total_n_;
    int         method_;
};

} // namespace integration
