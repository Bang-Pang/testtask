#pragma once

#include <string>

namespace integration {

/**
 * TCP client that:
 *  1. Connects to the server.
 *  2. Sends a RegisterMsg (thread count).
 *  3. Receives a TaskMsg.
 *  4. Executes all subtasks in a thread pool.
 *  5. Aggregates partial results and sends a ResultMsg back.
 */
class Client {
public:
    /**
     * @param host     Server hostname or IP.
     * @param port     Server TCP port.
     * @param threads  Number of worker threads to announce and use.
     */
    Client(std::string host, int port, int threads);

    /// Connect, receive task, compute, send result.
    void run();

private:
    std::string host_;
    int         port_;
    int         threads_;
};

} // namespace integration
