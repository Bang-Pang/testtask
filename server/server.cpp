#include "server.hpp"

#include <common/protocol.hpp>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <stdexcept>

namespace integration {

namespace {

// ── low-level helpers ─────────────────────────────────────────────────────────

/// Send a string terminated by '\n' over a socket.
void send_line(int fd, const std::string& line) {
    std::string msg = line + "\n";
    std::size_t sent = 0;
    while (sent < msg.size()) {
        ssize_t n = ::send(fd, msg.data() + sent, msg.size() - sent, 0);
        if (n <= 0) throw std::runtime_error("send failed");
        sent += static_cast<std::size_t>(n);
    }
}

/// Read bytes from fd until '\n'; returns the line (without the newline).
std::string recv_line(int fd) {
    std::string line;
    char ch;
    while (true) {
        ssize_t n = ::recv(fd, &ch, 1, 0);
        if (n <= 0) throw std::runtime_error("recv failed or connection closed");
        if (ch == '\n') break;
        line += ch;
    }
    return line;
}

} // anonymous namespace

// ── Server implementation ─────────────────────────────────────────────────────

Server::Server(int port, int wait_seconds,
               std::string function, double a, double b,
               int total_n, int method)
    : port_(port), wait_seconds_(wait_seconds),
      function_(std::move(function)), a_(a), b_(b),
      total_n_(total_n), method_(method)
{}

void Server::run() {
    // ── create listening socket ────────────────────────────────────────────
    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) throw std::runtime_error("socket() failed");

    int opt = 1;
    ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<uint16_t>(port_));

    if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");

    if (::listen(listen_fd, 16) < 0)
        throw std::runtime_error("listen() failed");

    std::cout << "[server] Listening on port " << port_
              << ", waiting " << wait_seconds_ << "s for clients...\n";

    // ── accept loop (non-blocking style with timeout) ──────────────────────
    struct ClientInfo {
        int fd;
        int threads;
    };
    std::vector<ClientInfo> clients;
    std::mutex              clients_mutex;

    // Set accept timeout
    struct timeval tv{static_cast<time_t>(wait_seconds_), 0};
    ::setsockopt(listen_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (true) {
        sockaddr_in client_addr{};
        socklen_t   client_len = sizeof(client_addr);
        int cfd = ::accept(listen_fd,
                           reinterpret_cast<sockaddr*>(&client_addr),
                           &client_len);
        if (cfd < 0) {
            // timeout or no more clients
            break;
        }

        char ip_buf[INET_ADDRSTRLEN];
        ::inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));
        std::cout << "[server] Client connected from " << ip_buf << "\n";

        // receive RegisterMsg
        try {
            std::string line = recv_line(cfd);
            auto j = nlohmann::json::parse(line);
            RegisterMsg reg = j.get<RegisterMsg>();
            std::cout << "[server] Client registered with " << reg.threads << " thread(s)\n";
            std::lock_guard<std::mutex> lk(clients_mutex);
            clients.push_back({cfd, reg.threads});
        } catch (const std::exception& e) {
            std::cerr << "[server] Error reading register: " << e.what() << "\n";
            ::close(cfd);
        }
    }

    ::close(listen_fd);

    if (clients.empty()) {
        std::cout << "[server] No clients connected. Exiting.\n";
        return;
    }

    // ── distribute tasks ───────────────────────────────────────────────────
    // Divide total_n proportionally to each client's thread count.
    int total_threads = 0;
    for (auto& c : clients) total_threads += c.threads;

    // For Simpson's rule every subtask must have an even number of intervals.
    // Round total_n up to the nearest multiple of (2 * total_threads) so that
    // the even constraint is satisfied without changing the integration bounds.
    int effective_n = total_n_;
    if (method_ == 2) {
        int granularity = 2 * total_threads;
        effective_n = ((total_n_ + granularity - 1) / granularity) * granularity;
    }

    int task_id    = 1;
    int assigned_n = 0;
    double range   = b_ - a_;

    for (std::size_t i = 0; i < clients.size(); ++i) {
        auto& c = clients[i];

        // How many steps for this client (proportional to its thread count)?
        int client_n = (i + 1 == clients.size())
                       ? (effective_n - assigned_n)
                       : (effective_n * c.threads / total_threads);
        if (client_n <= 0) client_n = (method_ == 2 ? 2 : 1);

        double client_a = a_ + range * assigned_n / effective_n;
        double client_b = a_ + range * (assigned_n + client_n) / effective_n;
        assigned_n += client_n;

        // Further split client's work into subtasks (one per thread).
        TaskMsg task;
        task.task_id  = task_id++;
        task.function = function_;

        // Each subtask gets an equal share; keep counts even for Simpson.
        int sub_n_each = client_n / c.threads;
        if (method_ == 2 && sub_n_each % 2 != 0) sub_n_each += 1;
        if (sub_n_each < (method_ == 2 ? 2 : 1)) sub_n_each = (method_ == 2 ? 2 : 1);
        double sub_range = client_b - client_a;
        for (int t = 0; t < c.threads; ++t) {
            SubTask st;
            st.a      = client_a + sub_range * t / c.threads;
            st.b      = client_a + sub_range * (t + 1) / c.threads;
            st.n      = (t + 1 == c.threads)
                        ? (client_n - sub_n_each * t)
                        : sub_n_each;
            if (st.n < 1) st.n = 1;
            if (method_ == 2 && st.n % 2 != 0) st.n += 1;
            st.method = method_;
            task.subtasks.push_back(st);
        }

        nlohmann::json jt = task;
        send_line(c.fd, jt.dump());
        std::cout << "[server] Sent task " << task.task_id
                  << " to client (" << c.threads << " subtasks)\n";
    }

    // ── collect results ────────────────────────────────────────────────────
    double total = 0.0;
    for (auto& c : clients) {
        try {
            std::string line = recv_line(c.fd);
            auto j = nlohmann::json::parse(line);
            ResultMsg res = j.get<ResultMsg>();
            std::cout << "[server] Received result for task " << res.task_id
                      << ": " << res.value << "\n";
            total += res.value;
        } catch (const std::exception& e) {
            std::cerr << "[server] Error reading result: " << e.what() << "\n";
        }
        ::close(c.fd);
    }

    std::cout << "[server] Final integral of " << function_
              << " over [" << a_ << ", " << b_ << "] = " << total << "\n";
}

} // namespace integration
