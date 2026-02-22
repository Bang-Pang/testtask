#include "client.hpp"

#include <common/integrator.hpp>
#include <common/protocol.hpp>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <stdexcept>
#include <cmath>
#include <map>

namespace integration {

namespace {

// ── simple thread-pool ────────────────────────────────────────────────────────

class ThreadPool {
public:
    explicit ThreadPool(int n) : stop_(false) {
        for (int i = 0; i < n; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lk(mtx_);
                        cv_.wait(lk, [this] { return stop_ || !queue_.empty(); });
                        if (stop_ && queue_.empty()) return;
                        task = std::move(queue_.front());
                        queue_.pop();
                    }
                    task();
                }
            });
        }
    }

    template<typename F>
    std::future<double> enqueue(F&& f) {
        auto promise = std::make_shared<std::promise<double>>();
        auto future  = promise->get_future();
        {
            std::lock_guard<std::mutex> lk(mtx_);
            queue_.push([p = std::move(promise), f = std::forward<F>(f)]() mutable {
                try   { p->set_value(f()); }
                catch (...) { p->set_exception(std::current_exception()); }
            });
        }
        cv_.notify_one();
        return future;
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& w : workers_) w.join();
    }

private:
    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> queue_;
    std::mutex                        mtx_;
    std::condition_variable           cv_;
    bool                              stop_;
};

// ── function registry ─────────────────────────────────────────────────────────

using Fn = std::function<double(double)>;

static const std::map<std::string, Fn> FUNCTIONS = {
    {"sin",  [](double x) { return std::sin(x); }},
    {"cos",  [](double x) { return std::cos(x); }},
    {"x2",   [](double x) { return x * x; }},
    {"x3",   [](double x) { return x * x * x; }},
};

Fn lookup_function(const std::string& name) {
    auto it = FUNCTIONS.find(name);
    if (it == FUNCTIONS.end())
        throw std::runtime_error("Unknown function: " + name);
    return it->second;
}

// ── low-level helpers ─────────────────────────────────────────────────────────

void send_line(int fd, const std::string& line) {
    std::string msg = line + "\n";
    std::size_t sent = 0;
    while (sent < msg.size()) {
        ssize_t n = ::send(fd, msg.data() + sent, msg.size() - sent, 0);
        if (n <= 0) throw std::runtime_error("send failed");
        sent += static_cast<std::size_t>(n);
    }
}

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

// ── Client implementation ─────────────────────────────────────────────────────

Client::Client(std::string host, int port, int threads)
    : host_(std::move(host)), port_(port), threads_(threads)
{}

void Client::run() {
    // ── resolve host ────────────────────────────────────────────────────────
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (::getaddrinfo(host_.c_str(), std::to_string(port_).c_str(), &hints, &res) != 0)
        throw std::runtime_error("getaddrinfo failed for " + host_);

    int fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) { ::freeaddrinfo(res); throw std::runtime_error("socket() failed"); }

    if (::connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        ::freeaddrinfo(res);
        ::close(fd);
        throw std::runtime_error("connect() failed");
    }
    ::freeaddrinfo(res);
    std::cout << "[client] Connected to " << host_ << ":" << port_ << "\n";

    // ── send RegisterMsg ────────────────────────────────────────────────────
    RegisterMsg reg;
    reg.threads = threads_;
    nlohmann::json jr = reg;
    send_line(fd, jr.dump());
    std::cout << "[client] Registered with " << threads_ << " thread(s)\n";

    // ── receive TaskMsg ─────────────────────────────────────────────────────
    std::string line = recv_line(fd);
    auto jt = nlohmann::json::parse(line);
    TaskMsg task = jt.get<TaskMsg>();
    std::cout << "[client] Received task " << task.task_id
              << " with " << task.subtasks.size() << " subtask(s)\n";

    Fn f = lookup_function(task.function);

    // ── compute subtasks in thread pool ────────────────────────────────────
    ThreadPool pool(threads_);
    std::vector<std::future<double>> futures;
    futures.reserve(task.subtasks.size());

    for (const auto& st : task.subtasks) {
        futures.push_back(pool.enqueue([=, &f]() -> double {
            return integrate(f, st.a, st.b, st.n,
                             static_cast<Method>(st.method));
        }));
    }

    double partial = 0.0;
    for (auto& fut : futures) {
        partial += fut.get();
    }

    std::cout << "[client] Partial result: " << partial << "\n";

    // ── send ResultMsg ──────────────────────────────────────────────────────
    ResultMsg result;
    result.task_id = task.task_id;
    result.value   = partial;
    nlohmann::json jres = result;
    send_line(fd, jres.dump());

    ::close(fd);
    std::cout << "[client] Done.\n";
}

} // namespace integration
