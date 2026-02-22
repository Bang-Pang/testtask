#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace integration {

// ── Message type tags ────────────────────────────────────────────────────────

constexpr const char* MSG_REGISTER = "register";
constexpr const char* MSG_TASK     = "task";
constexpr const char* MSG_RESULT   = "result";

// ── Structures ───────────────────────────────────────────────────────────────

/// Sent by a client immediately after connecting to announce its capabilities.
struct RegisterMsg {
    int threads{1}; ///< Number of worker threads the client can use
};

/// One piece of work: integrate over [a, b] with n steps using the given method.
struct SubTask {
    double a{0.0};
    double b{1.0};
    int    n{1000};
    int    method{0}; ///< Corresponds to integration::Method enum value
};

/// Sent by the server to a client; contains one or more subtasks.
struct TaskMsg {
    int                  task_id{0};
    std::string          function;   ///< Named function identifier (e.g. "sin")
    std::vector<SubTask> subtasks;
};

/// Sent by the client back to the server with the aggregated partial result.
struct ResultMsg {
    int    task_id{0};
    double value{0.0};
};

// ── JSON serialisation ────────────────────────────────────────────────────────

inline void to_json(nlohmann::json& j, const RegisterMsg& m) {
    j = {{"type", MSG_REGISTER}, {"threads", m.threads}};
}
inline void from_json(const nlohmann::json& j, RegisterMsg& m) {
    j.at("threads").get_to(m.threads);
}

inline void to_json(nlohmann::json& j, const SubTask& s) {
    j = {{"a", s.a}, {"b", s.b}, {"n", s.n}, {"method", s.method}};
}
inline void from_json(const nlohmann::json& j, SubTask& s) {
    j.at("a").get_to(s.a);
    j.at("b").get_to(s.b);
    j.at("n").get_to(s.n);
    j.at("method").get_to(s.method);
}

inline void to_json(nlohmann::json& j, const TaskMsg& t) {
    j = {{"type", MSG_TASK},
         {"task_id",  t.task_id},
         {"function", t.function},
         {"subtasks", t.subtasks}};
}
inline void from_json(const nlohmann::json& j, TaskMsg& t) {
    j.at("task_id").get_to(t.task_id);
    j.at("function").get_to(t.function);
    j.at("subtasks").get_to(t.subtasks);
}

inline void to_json(nlohmann::json& j, const ResultMsg& r) {
    j = {{"type", MSG_RESULT}, {"task_id", r.task_id}, {"value", r.value}};
}
inline void from_json(const nlohmann::json& j, ResultMsg& r) {
    j.at("task_id").get_to(r.task_id);
    j.at("value").get_to(r.value);
}

} // namespace integration
