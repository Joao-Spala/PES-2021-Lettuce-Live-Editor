#include "action_log.hpp"

#include <atomic>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <mutex>

namespace pes::actionlog {

namespace fs = std::filesystem;

namespace {
std::mutex g_mtx;
fs::path g_dir;
std::atomic<bool> g_enabled{true};
constexpr size_t kMaxMessage = 1500;
}

void set_directory(const fs::path& dir) {
    std::lock_guard<std::mutex> g(g_mtx);
    g_dir = dir;
}

void set_enabled(bool e) { g_enabled.store(e, std::memory_order_relaxed); }
bool enabled() { return g_enabled.load(std::memory_order_relaxed); }

fs::path directory() {
    std::lock_guard<std::mutex> g(g_mtx);
    return g_dir;
}

std::string format_line(std::string_view timestamp, std::string_view level, std::string_view message) {
    std::string line;
    line.reserve(timestamp.size() + level.size() + message.size() + 8);
    line.append(timestamp);
    line += " [";
    line.append(level);
    line += "] ";
    size_t n = 0;
    for (char c : message) {
        // Corta so em limite de caractere UTF-8 (byte de continuacao 10xxxxxx nunca inicia o corte)
        const bool continuation = (static_cast<unsigned char>(c) & 0xC0u) == 0x80u;
        if (n >= kMaxMessage && !continuation) { line += "..."; break; }
        ++n;
        line += (c == '\n' || c == '\r' || c == '\t') ? ' ' : c;
    }
    line += '\n';
    return line;
}

void write(std::string_view level, std::string_view message) {
    if (!enabled()) return;
    try {
        std::lock_guard<std::mutex> g(g_mtx);
        if (g_dir.empty()) return;

        std::error_code ec;
        fs::create_directories(g_dir, ec);

        const fs::path file = g_dir / L"workbench.log";
        if (fs::exists(file, ec)) {
            const auto sz = fs::file_size(file, ec);
            if (!ec && sz >= kRotateBytes) {
                const fs::path old = g_dir / L"workbench.1.log";
                fs::remove(old, ec);
                fs::rename(file, old, ec);
            }
        }

        std::time_t t = std::time(nullptr);
        std::tm lt{};
        localtime_s(&lt, &t);
        char stamp[24];
        std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &lt);

        std::ofstream f(file, std::ios::binary | std::ios::app);
        if (!f) return;
        const std::string line = format_line(stamp, level, message);
        f.write(line.data(), static_cast<std::streamsize>(line.size()));
    } catch (...) {
        // O log nunca pode derrubar o app
    }
}

} // namespace pes::actionlog
