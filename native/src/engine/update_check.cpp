#include "update_check.hpp"

#include "../app_version.hpp"

#include <windows.h>
#include <winhttp.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>

#include "../../third_party/json/json.hpp"

namespace pes::update {

namespace {

constexpr size_t kMaxBodyBytes = 256 * 1024;
constexpr size_t kMaxTagLen = 40;
constexpr size_t kMaxUrlLen = 300;

bool has_bad_chars(std::string_view s) {
    for (unsigned char c : s) {
        if (c <= 0x20 || c == 0x7F) return true;
    }
    return false;
}

// GET https://api.github.com/repos/<repo>/releases/latest. Retorna false em qualquer falha.
bool fetch_latest_release_body(const std::string& user_agent, std::string& out) {
    out.clear();
    std::wstring ua(user_agent.begin(), user_agent.end());
    HINTERNET ses = WinHttpOpen(ua.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                                WINHTTP_NO_PROXY_BYPASS, 0);
    if (!ses) return false;
    // resolve, connect, send, receive (ms)
    WinHttpSetTimeouts(ses, 4000, 4000, 6000, 8000);

    bool ok = false;
    HINTERNET con = WinHttpConnect(ses, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET req = nullptr;
    if (con) {
        std::string path_a = std::string("/repos/") + kUpdateRepo + "/releases/latest";
        std::wstring path(path_a.begin(), path_a.end());
        req = WinHttpOpenRequest(con, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    }
    if (req) {
        const wchar_t* hdr = L"Accept: application/vnd.github+json\r\n";
        if (WinHttpSendRequest(req, hdr, static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(req, nullptr)) {
            DWORD status = 0, sz = sizeof(status);
            if (WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                    WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz, WINHTTP_NO_HEADER_INDEX) &&
                status == 200) {
                ok = true;
                for (;;) {
                    DWORD avail = 0;
                    if (!WinHttpQueryDataAvailable(req, &avail)) { ok = false; break; }
                    if (avail == 0) break;
                    if (out.size() + avail > kMaxBodyBytes) { ok = false; break; }
                    const size_t old = out.size();
                    out.resize(old + avail);
                    DWORD got = 0;
                    if (!WinHttpReadData(req, &out[old], avail, &got)) { ok = false; break; }
                    out.resize(old + got);
                    if (got == 0) break;
                }
            }
        }
    }
    if (req) WinHttpCloseHandle(req);
    if (con) WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
    if (!ok) out.clear();
    return ok;
}

} // namespace

Version parse_version(std::string_view text) {
    Version v;
    size_t i = 0;
    if (i < text.size() && (text[i] == 'v' || text[i] == 'V')) ++i;
    if (i >= text.size() || text[i] < '0' || text[i] > '9') return v;

    int parts[3] = {0, 0, 0};
    int idx = 0;
    while (idx < 3 && i < text.size()) {
        if (text[i] < '0' || text[i] > '9') break;
        long long n = 0;
        int digits = 0;
        while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
            n = n * 10 + (text[i] - '0');
            if (++digits > 6) return Version{};
            ++i;
        }
        parts[idx++] = static_cast<int>(n);
        if (i < text.size() && text[i] == '.' && idx < 3) {
            // Ponto so vale se seguido de digito ("14." nao e versao valida alem do major)
            if (i + 1 < text.size() && text[i + 1] >= '0' && text[i + 1] <= '9') { ++i; continue; }
        }
        break;
    }
    // Depois do numero so vale fim do texto ou sufixo "-beta" / "+build"
    if (i < text.size() && text[i] != '-' && text[i] != '+') return Version{};
    v.major = parts[0];
    v.minor = parts[1];
    v.patch = parts[2];
    v.valid = true;
    return v;
}

int compare_versions(const Version& a, const Version& b) {
    if (a.major != b.major) return a.major < b.major ? -1 : 1;
    if (a.minor != b.minor) return a.minor < b.minor ? -1 : 1;
    if (a.patch != b.patch) return a.patch < b.patch ? -1 : 1;
    return 0;
}

bool is_trusted_release_url(std::string_view url) {
    if (url.size() > kMaxUrlLen || has_bad_chars(url)) return false;
    if (url.find("..") != std::string_view::npos) return false;
    const std::string_view prefix = kReleasesPagePrefix;
    return url.size() > prefix.size() && url.substr(0, prefix.size()) == prefix;
}

bool parse_release_json(std::string_view body, Release& out) {
    out = Release{};
    if (body.empty() || body.size() > kMaxBodyBytes) return false;
    nlohmann::json j = nlohmann::json::parse(body.begin(), body.end(), nullptr, false);
    if (j.is_discarded() || !j.is_object()) return false;
    // draft/prerelease de tipo errado (null, texto) nao pode derrubar o app: so aceita booleano
    for (const char* flag : {"draft", "prerelease"}) {
        auto it = j.find(flag);
        if (it == j.end()) continue;
        if (!it->is_boolean()) return false;
        if (it->get<bool>()) return false;
    }

    auto tag_it = j.find("tag_name");
    auto url_it = j.find("html_url");
    if (tag_it == j.end() || !tag_it->is_string() || url_it == j.end() || !url_it->is_string()) return false;

    std::string tag = tag_it->get<std::string>();
    std::string url = url_it->get<std::string>();
    if (tag.empty() || tag.size() > kMaxTagLen || has_bad_chars(tag)) return false;
    // So [0-9A-Za-z.+-]: e o mesmo conjunto que o settings.ini guarda, para o aviso dispensado casar apos reiniciar
    for (unsigned char c : tag) {
        const bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || c == '-' || c == '+';
        if (!ok) return false;
    }
    if (!parse_version(tag).valid) return false;
    if (!is_trusted_release_url(url)) return false;

    out.tag = std::move(tag);
    out.url = std::move(url);
    return true;
}

struct Checker::Shared {
    mutable std::mutex mtx;
    Snapshot snap;
    std::atomic<bool> cancelled{false};
};

Checker::Checker() : sh_(std::make_shared<Shared>()) {}

Checker::~Checker() {
    // Nao espera a rede: a thread guarda sua propria referencia ao estado e termina sozinha
    if (sh_) sh_->cancelled.store(true);
}

void Checker::start(const std::string& current_version, std::function<void()> on_done) {
    {
        std::lock_guard<std::mutex> g(sh_->mtx);
        if (sh_->snap.state == State::Checking) return;
        sh_->snap = Snapshot{};
        sh_->snap.state = State::Checking;
    }
    std::shared_ptr<Shared> sh = sh_;
    const std::string ua = std::string("LettuceLiveEditor-UpdateCheck/") + current_version;
    const Version cur = parse_version(current_version);

    try {
        std::thread([sh, ua, cur, on_done = std::move(on_done)]() {
            std::string body;
            Release rel;
            State result = State::Failed;
            try {
                if (cur.valid && fetch_latest_release_body(ua, body) && parse_release_json(body, rel)) {
                    result = (compare_versions(parse_version(rel.tag), cur) > 0) ? State::Available : State::UpToDate;
                }
            } catch (...) {
                result = State::Failed;   // nenhuma excecao pode derrubar o app a partir desta thread
            }
            {
                std::lock_guard<std::mutex> g(sh->mtx);
                sh->snap.state = result;
                sh->snap.release = (result == State::Available) ? std::move(rel) : Release{};
            }
            if (!sh->cancelled.load() && on_done) {
                try { on_done(); } catch (...) {}
            }
        }).detach();
    } catch (...) {
        // Sem thread nova: volta ao estado de falha para o botao "Verificar agora" nao ficar travado
        std::lock_guard<std::mutex> g(sh_->mtx);
        sh_->snap.state = State::Failed;
    }
}

Snapshot Checker::snapshot() const {
    std::lock_guard<std::mutex> g(sh_->mtx);
    return sh_->snap;
}

#ifdef PES_QA_DUMP
void Checker::qa_force(State s, const Release& r) {
    std::lock_guard<std::mutex> g(sh_->mtx);
    sh_->snap.state = s;
    sh_->snap.release = r;
}
#endif

} // namespace pes::update
