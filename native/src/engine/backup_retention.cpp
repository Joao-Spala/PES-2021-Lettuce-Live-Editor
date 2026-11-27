#include "backup_retention.hpp"

#include <algorithm>
#include <cwctype>

namespace pes::backups {

namespace fs = std::filesystem;

bool is_backup_name(const fs::path& file_name) {
    std::wstring n = file_name.filename().wstring();
    constexpr wchar_t kPrefix[] = L"Backup_";
    constexpr wchar_t kExt[] = L".json";
    const size_t pl = sizeof(kPrefix) / sizeof(wchar_t) - 1;
    const size_t el = sizeof(kExt) / sizeof(wchar_t) - 1;
    if (n.size() <= pl + el) return false;
    if (n.compare(0, pl, kPrefix) != 0) return false;
    for (size_t i = 0; i < el; ++i) {
        if (std::towlower(n[n.size() - el + i]) != kExt[i]) return false;
    }
    return true;
}

std::vector<Entry> list(const fs::path& dir) {
    std::vector<Entry> out;
    std::error_code ec;
    if (dir.empty() || !fs::is_directory(dir, ec)) return out;
    for (fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end; it.increment(ec)) {
        std::error_code e1, e2, e3;
        if (!it->is_regular_file(e1) || e1) continue;
        if (!is_backup_name(it->path())) continue;
        Entry e;
        e.path = it->path();
        e.size = static_cast<uint64_t>(it->file_size(e2));
        if (e2) e.size = 0;
        e.mtime = it->last_write_time(e3);
        if (e3) e.mtime = fs::file_time_type::min();
        out.push_back(std::move(e));
    }
    // Mais novo primeiro. Em empate de data, o nome decide (a data faz parte do nome)
    std::sort(out.begin(), out.end(), [](const Entry& a, const Entry& b) {
        if (a.mtime != b.mtime) return a.mtime > b.mtime;
        return a.path.filename().wstring() > b.path.filename().wstring();
    });
    return out;
}

uint64_t total_bytes(const std::vector<Entry>& entries) {
    uint64_t t = 0;
    for (const auto& e : entries) t += e.size;
    return t;
}

int prune(const fs::path& dir, int keep, const fs::path& protect) {
    if (keep <= 0) return 0;
    auto all = list(dir);
    if (static_cast<int>(all.size()) <= keep) return 0;

    std::error_code ec;
    int removed = 0;
    int kept = 0;
    for (const auto& e : all) {
        const bool is_protected = !protect.empty() && fs::equivalent(e.path, protect, ec);
        ec.clear();
        if (is_protected) { ++kept; continue; }
        if (kept < keep) { ++kept; continue; }
        std::error_code rm_ec;
        if (fs::remove(e.path, rm_ec) && !rm_ec) ++removed;
    }
    return removed;
}

} // namespace pes::backups
