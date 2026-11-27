#include "update_check.hpp"

#include "../app_version.hpp"

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>

#include "../../third_party/json/json.hpp"

namespace pes::update {

namespace {

constexpr size_t kMaxBodyBytes = selfupdate::kMaxApiBytes;
constexpr size_t kMaxTagLen = 40;
constexpr size_t kMaxUrlLen = 300;

bool has_bad_chars(std::string_view s) {
    for (unsigned char c : s) {
        if (c <= 0x20 || c == 0x7F) return true;
    }
    return false;
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
    Config cfg;
};

namespace {

struct InstallOutcome {
    bool installed = false;
    std::string note;
    std::string detail;
};

// Baixa o manifesto, a assinatura e o exe da release `tag`, confere tudo e troca o executavel.
InstallOutcome run_install(const Config& cfg, const std::string& tag, const std::string& current_version, const std::string& ua) {
    InstallOutcome o;
    const std::wstring exe = cfg.exe_path.empty() ? selfupdate::current_exe_path() : cfg.exe_path;
    if (exe.empty()) {
        o.note = kNoteWrite;
        o.detail = "executable path unknown";
        return o;
    }
    // O host e o caminho vem da configuracao fixa. Da API entra so a tag, que parse_release_json restringe a [0-9A-Za-z.+-].
    const std::wstring base = cfg.source.dl_prefix + std::wstring(tag.begin(), tag.end()) + L"/";
    std::string manifest, sig, bytes;
    unsigned long st = 0;
    if (!selfupdate::http_get(cfg.source, false, base + selfupdate::kAssetManifest, ua, selfupdate::kMaxManifestBytes, manifest, &st) ||
        !selfupdate::http_get(cfg.source, false, base + selfupdate::kAssetSignature, ua, selfupdate::kMaxSignatureBytes, sig, &st)) {
        o.note = (st == 404) ? kNoteUnsigned : kNoteDownload;
        o.detail = "manifest or signature not available (HTTP " + std::to_string(st) + ")";
        return o;
    }
    if (!selfupdate::http_get(cfg.source, false, base + selfupdate::kAssetExe, ua, selfupdate::kMaxExeBytes, bytes, &st)) {
        o.note = (st == 404) ? kNoteUnsigned : kNoteDownload;
        o.detail = "executable download failed (HTTP " + std::to_string(st) + ")";
        return o;
    }
    selfupdate::Manifest m;
    const char* why = "";
    if (!selfupdate::validate_update(manifest, sig, tag, bytes, current_version, cfg.public_key.data(), m, &why)) {
        o.note = kNoteRefused;
        o.detail = why;
        return o;
    }
    unsigned long err = 0;
    if (!selfupdate::apply_update(exe, bytes, &err)) {
        o.note = kNoteWrite;
        o.detail = "could not replace the executable (Windows error " + std::to_string(err) + ")";
        return o;
    }
    o.installed = true;
    return o;
}

// O estado ja esta em Installing (quem chamou trocou dentro do lock)
void finish_install(const std::shared_ptr<Checker::Shared>& sh, const Release& rel, const std::string& current_version,
                    const std::string& ua, const std::function<void()>& notify);

} // namespace

Checker::Checker(Config cfg) : sh_(std::make_shared<Shared>()) {
    sh_->cfg = std::move(cfg);
}

Checker::~Checker() {
    // Nao espera a rede: a thread guarda sua propria referencia ao estado e termina sozinha
    if (sh_) sh_->cancelled.store(true);
}

namespace {

void finish_install(const std::shared_ptr<Checker::Shared>& sh, const Release& rel, const std::string& current_version,
                    const std::string& ua, const std::function<void()>& notify) {
    notify();   // o banner mostra "baixando"
    InstallOutcome o;
    try {
        o = run_install(sh->cfg, rel.tag, current_version, ua);
    } catch (...) {
        o.note = kNoteDownload;
        o.detail = "unexpected error";
    }
    {
        std::lock_guard<std::mutex> g(sh->mtx);
        if (o.installed) {
            sh->snap.state = State::Installed;
            sh->snap.note.clear();
            sh->snap.detail.clear();
        } else {
            sh->snap.state = State::Available;
            sh->snap.note = o.note;
            sh->snap.detail = o.detail;
        }
    }
    notify();
}

} // namespace

void Checker::start(const std::string& current_version, bool auto_install, std::function<void()> on_done) {
    {
        std::lock_guard<std::mutex> g(sh_->mtx);
        // Com a atualizacao ja no lugar nao ha o que verificar ate reiniciar. Instalar de novo tentaria apagar o .old,
        // que e a imagem do processo em execucao, e acabaria em um "sem permissao" enganoso.
        if (sh_->snap.state == State::Checking || sh_->snap.state == State::Installing || sh_->snap.state == State::Installed) return;
        sh_->snap = Snapshot{};
        sh_->snap.state = State::Checking;
    }
    std::shared_ptr<Shared> sh = sh_;
    const std::string ua = std::string("LettuceLiveEditor-Update/") + current_version;
    const Version cur = parse_version(current_version);

    try {
        std::thread([sh, ua, cur, current_version, auto_install, on_done = std::move(on_done)]() {
            auto notify = [&sh, &on_done]() {
                if (!sh->cancelled.load() && on_done) {
                    try { on_done(); } catch (...) {}
                }
            };
            std::string body;
            Release rel;
            State result = State::Failed;
            try {
                if (cur.valid &&
                    selfupdate::http_get(sh->cfg.source, true, sh->cfg.source.api_path, ua, selfupdate::kMaxApiBytes, body, nullptr) &&
                    parse_release_json(body, rel)) {
                    result = (compare_versions(parse_version(rel.tag), cur) > 0) ? State::Available : State::UpToDate;
                }
            } catch (...) {
                result = State::Failed;   // nenhuma excecao pode derrubar o app a partir desta thread
            }
            bool install_now = false;
            {
                std::lock_guard<std::mutex> g(sh->mtx);
                sh->snap.state = result;
                sh->snap.release = (result == State::Available) ? rel : Release{};
                if (result == State::Available && auto_install) {
                    sh->snap.state = State::Installing;
                    install_now = true;
                }
            }
            if (install_now) {
                finish_install(sh, rel, current_version, ua, notify);
            } else {
                notify();
            }
        }).detach();
    } catch (...) {
        // Sem thread nova: volta ao estado de falha para o botao "Verificar agora" nao ficar travado
        std::lock_guard<std::mutex> g(sh_->mtx);
        sh_->snap.state = State::Failed;
    }
}

void Checker::install(const std::string& current_version, std::function<void()> on_done) {
    Release rel;
    {
        std::lock_guard<std::mutex> g(sh_->mtx);
        if (sh_->snap.state != State::Available) return;
        rel = sh_->snap.release;
        sh_->snap.state = State::Installing;
        sh_->snap.note.clear();
        sh_->snap.detail.clear();
    }
    std::shared_ptr<Shared> sh = sh_;
    const std::string ua = std::string("LettuceLiveEditor-Update/") + current_version;
    try {
        std::thread([sh, rel, ua, current_version, on_done = std::move(on_done)]() {
            auto notify = [&sh, &on_done]() {
                if (!sh->cancelled.load() && on_done) {
                    try { on_done(); } catch (...) {}
                }
            };
            finish_install(sh, rel, current_version, ua, notify);
        }).detach();
    } catch (...) {
        std::lock_guard<std::mutex> g(sh_->mtx);
        sh_->snap.state = State::Available;
        sh_->snap.note = kNoteDownload;
    }
}

Snapshot Checker::snapshot() const {
    std::lock_guard<std::mutex> g(sh_->mtx);
    return sh_->snap;
}

#ifdef PES_QA_DUMP
void Checker::qa_force(State s, const Release& r, const std::string& note) {
    std::lock_guard<std::mutex> g(sh_->mtx);
    sh_->snap.state = s;
    sh_->snap.release = r;
    sh_->snap.note = note;
}
#endif

} // namespace pes::update
