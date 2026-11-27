// Testes da atualizacao automatica assinada: criptografia, manifesto, troca do executavel e o ciclo completo contra
// um servidor HTTP local (sem TLS: so os testes apontam a fonte para o servidor local, o app nunca).

#include "../src/engine/self_update.hpp"
#include "../src/engine/update_check.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace pes;

static int g_fail = 0;
static int g_checks = 0;

#define CHECK(cond)                                                                    \
    do {                                                                               \
        ++g_checks;                                                                    \
        if (!(cond)) {                                                                 \
            ++g_fail;                                                                  \
            std::printf("  FALHA linha %d: %s\n", __LINE__, #cond);                    \
        }                                                                              \
    } while (0)

// ------------------------------------------------------------------------------------------------ utilitarios

static std::string to_hex(const uint8_t* p, size_t n) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; ++i) { s.push_back(d[p[i] >> 4]); s.push_back(d[p[i] & 15]); }
    return s;
}

static std::string sha_hex(const std::string& data) {
    uint8_t h[32];
    selfupdate::sha256(data.data(), data.size(), h);
    return to_hex(h, 32);
}

struct TestKey {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_KEY_HANDLE key = nullptr;
    uint8_t pub[64]{};
    ~TestKey() {
        if (key) BCryptDestroyKey(key);
        if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    }
};

static bool make_key(TestKey& k) {
    if (BCryptOpenAlgorithmProvider(&k.alg, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0) != 0) return false;
    if (BCryptGenerateKeyPair(k.alg, &k.key, 256, 0) != 0) return false;
    if (BCryptFinalizeKeyPair(k.key, 0) != 0) return false;
    ULONG cb = 0;
    if (BCryptExportKey(k.key, nullptr, BCRYPT_ECCPUBLIC_BLOB, nullptr, 0, &cb, 0) != 0) return false;
    std::vector<uint8_t> blob(cb);
    if (BCryptExportKey(k.key, nullptr, BCRYPT_ECCPUBLIC_BLOB, blob.data(), cb, &cb, 0) != 0) return false;
    if (cb < sizeof(BCRYPT_ECCKEY_BLOB) + 64) return false;
    std::memcpy(k.pub, blob.data() + sizeof(BCRYPT_ECCKEY_BLOB), 64);
    return true;
}

static std::string sign_hex(const TestKey& k, const std::string& msg) {
    uint8_t digest[32];
    selfupdate::sha256(msg.data(), msg.size(), digest);
    uint8_t sig[64]{};
    ULONG out = 0;
    if (BCryptSignHash(k.key, nullptr, digest, 32, sig, 64, &out, 0) != 0 || out != 64) return {};
    return to_hex(sig, 64);
}

// "Executavel" minimo que passa na checagem de PE de 64 bits
static std::string make_exe(size_t size, unsigned seed) {
    std::string b(size, '\0');
    unsigned x = seed * 2654435761u + 1;
    for (size_t i = 0; i < size; ++i) { x = x * 1664525u + 1013904223u; b[i] = static_cast<char>(x >> 24); }
    b[0] = 'M'; b[1] = 'Z';
    const uint32_t nt = 0x80;
    std::memcpy(&b[0x3C], &nt, 4);
    std::memcpy(&b[nt], "PE\0\0", 4);
    const uint16_t machine = 0x8664;
    std::memcpy(&b[nt + 4], &machine, 2);
    return b;
}

static std::string make_manifest(const std::string& version, const std::string& exe) {
    return std::string(selfupdate::kManifestHeader) + "\nversion = " + version + "\nexe = " + sha_hex(exe) + " " + std::to_string(exe.size()) + "\n";
}

static void write_file(const fs::path& p, const std::string& data) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f.write(data.data(), static_cast<std::streamsize>(data.size()));
}

static std::string read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// ------------------------------------------------------------------------------------------------ cripto e manifesto

static void test_crypto_and_manifest() {
    using namespace selfupdate;
    // SHA-256 de "abc" (vetor padrao)
    uint8_t h[32];
    CHECK(sha256("abc", 3, h));
    CHECK(to_hex(h, 32) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(sha256("", 0, h) && to_hex(h, 32) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    uint8_t b[4];
    CHECK(parse_hex("  0A0b ff00\n", b, 4) == false);          // espaco no meio nao vale
    CHECK(parse_hex("  0a0bff00\r\n", b, 4) && b[0] == 0x0A && b[1] == 0x0B && b[2] == 0xFF && b[3] == 0);
    CHECK(!parse_hex("0a0bff0", b, 4));                         // tamanho errado
    CHECK(!parse_hex("0a0bff0g", b, 4));                        // nao hexadecimal
    CHECK(!parse_hex("", b, 4));

    const std::string exe = make_exe(4096, 1);
    const std::string good = make_manifest("14.2.0", exe);
    Manifest m;
    CHECK(parse_manifest(good, m) && m.version == "14.2.0" && m.exe_size == 4096);
    CHECK(sha_hex(exe) == to_hex(m.exe_hash, 32));
    std::string crlf = good;
    for (size_t p = 0; (p = crlf.find('\n', p)) != std::string::npos; p += 2) crlf.insert(p, "\r");
    CHECK(parse_manifest(crlf, m));                              // fim de linha do Windows
    CHECK(!parse_manifest("", m));
    CHECK(!parse_manifest("pes21-var-mod update 1\nversion = 14.2.0\nexe = " + sha_hex(exe) + " 4096\n", m));   // manifesto de outro produto
    CHECK(!parse_manifest(std::string(kManifestHeader) + "\nversion = 14.2.0\n", m));                          // sem exe
    CHECK(!parse_manifest(std::string(kManifestHeader) + "\nexe = " + sha_hex(exe) + " 4096\n", m));          // sem versao
    CHECK(!parse_manifest(good + "version = 14.3.0\n", m));                                                    // chave repetida
    CHECK(!parse_manifest(good + "extra = 1\n", m));                                                           // chave desconhecida
    CHECK(!parse_manifest(std::string(kManifestHeader) + "\nversion = 14.2.0\nexe = abcd 4096\n", m));        // hash curto
    CHECK(!parse_manifest(std::string(kManifestHeader) + "\nversion = 14.2.0\nexe = " + sha_hex(exe) + " 0\n", m));            // tamanho 0
    CHECK(!parse_manifest(std::string(kManifestHeader) + "\nversion = 14.2.0\nexe = " + sha_hex(exe) + " 99999999999\n", m));  // acima do teto
    CHECK(!parse_manifest(std::string(kManifestHeader) + "\nversion = 14.2.x\nexe = " + sha_hex(exe) + " 4096\n", m));         // versao invalida
    CHECK(!parse_manifest(std::string(10000, 'a'), m));                                                        // grande demais

    // A chave publica compilada e um ponto valido (nao zero) e a verificacao rejeita lixo sem travar
    bool nonzero = false;
    for (uint8_t v : kUpdatePublicKey) nonzero = nonzero || v != 0;
    CHECK(nonzero);
    uint8_t junk[64]{};
    CHECK(!verify_signature("x", 1, junk, kUpdatePublicKey.data()));

    // Assinatura de verdade com um par de chaves de teste
    TestKey key, other;
    CHECK(make_key(key) && make_key(other));
    const std::string sig = sign_hex(key, good);
    CHECK(sig.size() == 128);
    uint8_t sb[64];
    CHECK(parse_hex(sig, sb, 64));
    CHECK(verify_signature(good.data(), good.size(), sb, key.pub));
    CHECK(!verify_signature(good.data(), good.size(), sb, other.pub));            // chave errada
    std::string tampered = good;
    tampered[good.find("14.2.0")] = '9';
    CHECK(!verify_signature(tampered.data(), tampered.size(), sb, key.pub));      // manifesto adulterado
    uint8_t flipped[64];
    std::memcpy(flipped, sb, 64);
    flipped[10] ^= 1;
    CHECK(!verify_signature(good.data(), good.size(), flipped, key.pub));          // assinatura adulterada

    // validate_update: o caminho feliz e cada recusa
    const char* why = nullptr;
    Manifest vm;
    CHECK(validate_update(good, sig, "v14.2.0", exe, "14.1.1", key.pub, vm, &why) && std::string(why).empty());
    CHECK(validate_update(good, sig, "14.2.0", exe, "14.1.1", key.pub, vm, &why));    // tag sem "v"
    CHECK(!validate_update(good, "zz", "v14.2.0", exe, "14.1.1", key.pub, vm, &why) && std::string(why) == "malformed signature");
    CHECK(!validate_update(good, sig, "v14.2.0", exe, "14.1.1", other.pub, vm, &why) && std::string(why) == "signature does not verify");
    CHECK(!validate_update(tampered, sig, "v14.2.0", exe, "14.1.1", key.pub, vm, &why) && std::string(why) == "signature does not verify");
    CHECK(!validate_update(good, sig, "v14.3.0", exe, "14.1.1", key.pub, vm, &why) && std::string(why) == "manifest version differs from the release tag");
    CHECK(!validate_update(good, sig, "v14.2.0", exe, "14.2.0", key.pub, vm, &why) && std::string(why) == "not newer than the running version");   // igual
    CHECK(!validate_update(good, sig, "v14.2.0", exe, "15.0.0", key.pub, vm, &why) && std::string(why) == "not newer than the running version");   // regressao
    CHECK(!validate_update(good, sig, "v14.2.0", exe, "lixo", key.pub, vm, &why));                                                                   // versao atual invalida
    std::string exe2 = exe;
    exe2[1000] ^= 0x55;
    CHECK(!validate_update(good, sig, "v14.2.0", exe2, "14.1.1", key.pub, vm, &why) && std::string(why) == "executable differs from the signed manifest");
    CHECK(!validate_update(good, sig, "v14.2.0", exe.substr(0, 4000), "14.1.1", key.pub, vm, &why));     // tamanho
    // Manifesto assinado de um arquivo que nao e executavel de 64 bits
    std::string notpe(4096, 'A');
    const std::string m2 = make_manifest("14.2.0", notpe);
    CHECK(!validate_update(m2, sign_hex(key, m2), "v14.2.0", notpe, "14.1.1", key.pub, vm, &why) && std::string(why) == "not a 64-bit executable");
    std::string pe32 = exe;
    const uint16_t m32 = 0x014C;
    std::memcpy(&pe32[0x80 + 4], &m32, 2);
    const std::string m3 = make_manifest("14.2.0", pe32);
    CHECK(!validate_update(m3, sign_hex(key, m3), "v14.2.0", pe32, "14.1.1", key.pub, vm, &why) && std::string(why) == "not a 64-bit executable");

    std::printf("Atualizacao: criptografia, manifesto e validacao PASS\n");
}

// ------------------------------------------------------------------------------------------------ troca do executavel

static void test_apply(const fs::path& dir) {
    using namespace selfupdate;
    const fs::path exe = dir / "app.exe";
    const std::wstring wexe = exe.wstring();
    const std::string a = make_exe(5000, 10), b = make_exe(6000, 11), c = make_exe(7000, 12);
    unsigned long err = 99;

    write_file(exe, a);
    CHECK(apply_update(wexe, b, &err) && err == 0);
    CHECK(read_file(exe) == b);
    CHECK(read_file(exe.string() + ".old") == a);
    CHECK(!fs::exists(exe.string() + ".new"));

    // Um "executavel em execucao": a imagem mapeada permite renomear (FILE_SHARE_DELETE), nao sobrescrever
    HANDLE running = CreateFileW(wexe.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);
    CHECK(running != INVALID_HANDLE_VALUE);
    finish_previous_update(wexe);                                 // o .old nao esta em uso: some
    CHECK(!fs::exists(exe.string() + ".old"));
    CHECK(apply_update(wexe, c, &err));                           // troca com o exe aberto
    CHECK(read_file(exe) == c && read_file(exe.string() + ".old") == b);
    char first[4] = {};
    DWORD got = 0;
    SetFilePointer(running, 0, nullptr, FILE_BEGIN);
    CHECK(ReadFile(running, first, 2, &got, nullptr) && got == 2 && first[0] == 'M');   // o processo antigo segue lendo a imagem antiga
    CloseHandle(running);

    // O .old de uma atualizacao anterior ainda em uso (versao antiga rodando): nao troca nada e o exe fica intacto
    HANDLE oldrun = CreateFileW((exe.wstring() + L".old").c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    CHECK(oldrun != INVALID_HANDLE_VALUE);
    const std::string d = make_exe(8000, 13);
    CHECK(!apply_update(wexe, d, &err) && err != 0);
    CHECK(read_file(exe) == c);
    CHECK(!fs::exists(exe.string() + ".new"));
    CloseHandle(oldrun);
    CHECK(apply_update(wexe, d, &err) && read_file(exe) == d);    // liberado: volta a funcionar

    // Sem o executavel original nada acontece
    CHECK(!apply_update((dir / "nao_existe.exe").wstring(), d, &err));
    CHECK(!fs::exists(dir / "nao_existe.exe.new"));

    // Pasta sem permissao de escrita: falha limpa. Aqui, um .new que e uma pasta impede a gravacao
    const fs::path exe3 = dir / "app3.exe";
    write_file(exe3, a);
    fs::create_directory(exe3.string() + ".new");
    CHECK(!apply_update(exe3.wstring(), b, &err));
    CHECK(read_file(exe3) == a);                                  // intacto
    fs::remove_all(exe3.string() + ".new");

    // Linha de comando de reabertura
    CHECK(parse_after_update_pid(L"--after-update 4242") == 4242);
    CHECK(parse_after_update_pid(L"\"C:\\a b\\x.exe\" --after-update 17 --outra") == 17);
    CHECK(parse_after_update_pid(L"--after-update=99") == 99);
    CHECK(parse_after_update_pid(L"--after-update") == 0);
    CHECK(parse_after_update_pid(L"--after-update abc") == 0);
    CHECK(parse_after_update_pid(L"") == 0);
    CHECK(parse_after_update_pid(nullptr) == 0);
    CHECK(parse_after_update_pid(L"--after-update 99999999999") == 0);     // nao cabe em 32 bits: recusa em vez de dar a volta
    CHECK(parse_after_update_pid(L"--after-update 4294967295") == 4294967295ul);
    wait_for_exit(0, 10);                                          // PID 0 e o proprio: voltam na hora
    wait_for_exit(GetCurrentProcessId(), 10);

    std::printf("Atualizacao: troca do executavel (inclusive em uso) e rollback PASS\n");
}

// ------------------------------------------------------------------------------------------------ servidor local

struct Resp {
    int status = 200;
    std::string body;
};

class TestServer {
public:
    TestServer() {
        WSADATA wd;
        WSAStartup(MAKEWORD(2, 2), &wd);
        listen_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        bind(listen_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        listen(listen_, 8);
        int len = sizeof(addr);
        getsockname(listen_, reinterpret_cast<sockaddr*>(&addr), &len);
        port_ = ntohs(addr.sin_port);
        th_ = std::thread([this] { loop(); });
    }
    ~TestServer() {
        stop_ = true;
        closesocket(listen_);
        if (th_.joinable()) th_.join();
        WSACleanup();
    }
    uint16_t port() const { return port_; }
    void set(const std::string& path, int status, const std::string& body) {
        std::lock_guard<std::mutex> g(mtx_);
        routes_[path] = Resp{status, body};
    }
    void clear() {
        std::lock_guard<std::mutex> g(mtx_);
        routes_.clear();
        hits_.clear();
    }
    int hits_containing(const std::string& part) {
        std::lock_guard<std::mutex> g(mtx_);
        int n = 0;
        for (const auto& h : hits_) if (h.find(part) != std::string::npos) ++n;
        return n;
    }
    int total_hits() {
        std::lock_guard<std::mutex> g(mtx_);
        return static_cast<int>(hits_.size());
    }

private:
    void loop() {
        while (!stop_) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(listen_, &fds);
            timeval tv{0, 100000};
            if (select(0, &fds, nullptr, nullptr, &tv) <= 0) continue;
            SOCKET c = accept(listen_, nullptr, nullptr);
            if (c == INVALID_SOCKET) continue;
            std::string req;
            char buf[2048];
            while (req.find("\r\n\r\n") == std::string::npos && req.size() < 16384) {
                const int n = recv(c, buf, sizeof(buf), 0);
                if (n <= 0) break;
                req.append(buf, static_cast<size_t>(n));
            }
            std::string path;
            const size_t sp1 = req.find(' '), sp2 = req.find(' ', sp1 + 1);
            if (sp1 != std::string::npos && sp2 != std::string::npos) path = req.substr(sp1 + 1, sp2 - sp1 - 1);
            Resp r{404, "not found"};
            {
                std::lock_guard<std::mutex> g(mtx_);
                hits_.push_back(path);
                auto it = routes_.find(path);
                if (it != routes_.end()) r = it->second;
            }
            const std::string head = "HTTP/1.1 " + std::to_string(r.status) + (r.status == 200 ? " OK" : " ERR") +
                                     "\r\nContent-Length: " + std::to_string(r.body.size()) + "\r\nConnection: close\r\n\r\n";
            send(c, head.data(), static_cast<int>(head.size()), 0);
            size_t off = 0;
            while (off < r.body.size()) {
                const int n = send(c, r.body.data() + off, static_cast<int>(std::min<size_t>(r.body.size() - off, 65536)), 0);
                if (n <= 0) break;
                off += static_cast<size_t>(n);
            }
            shutdown(c, SD_SEND);
            closesocket(c);
        }
    }
    SOCKET listen_ = INVALID_SOCKET;
    uint16_t port_ = 0;
    std::thread th_;
    std::atomic<bool> stop_{false};
    std::mutex mtx_;
    std::map<std::string, Resp> routes_;
    std::vector<std::string> hits_;
};

static bool wait_settled(update::Checker& ck, update::Snapshot& out, int timeout_ms = 20000) {
    const auto t0 = std::chrono::steady_clock::now();
    for (;;) {
        out = ck.snapshot();
        if (out.state != update::State::Checking && out.state != update::State::Installing && out.state != update::State::Idle) return true;
        if (std::chrono::steady_clock::now() - t0 > std::chrono::milliseconds(timeout_ms)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

static const char* kTagUrl = "https://github.com/Joao-Spala/PES-2021-Lettuce-Live-Editor/releases/tag/";

static void publish(TestServer& srv, const std::string& tag, const std::string& exe, const std::string& manifest, const std::string& sig_hex) {
    srv.set("/repos/test/releases/latest", 200,
            "{\"tag_name\":\"" + tag + "\",\"html_url\":\"" + kTagUrl + tag + "\",\"draft\":false,\"prerelease\":false}");
    const std::string base = "/test/releases/download/" + tag + "/";
    srv.set(base + "update_manifest.txt", 200, manifest);
    srv.set(base + "update_manifest.sig", 200, sig_hex + "\n");
    srv.set(base + "Lettuce_Live_Editor.exe", 200, exe);
}

static update::Config make_cfg(const TestServer& srv, const TestKey& key, const fs::path& exe) {
    update::Config cfg;
    cfg.source.api_host = L"127.0.0.1";
    cfg.source.dl_host = L"127.0.0.1";
    cfg.source.port = srv.port();
    cfg.source.secure = false;
    cfg.source.api_path = L"/repos/test/releases/latest";
    cfg.source.dl_prefix = L"/test/releases/download/";
    std::memcpy(cfg.public_key.data(), key.pub, 64);
    cfg.exe_path = exe.wstring();
    return cfg;
}

static void test_end_to_end(const fs::path& dir) {
    using namespace update;
    TestServer srv;
    TestKey key, other;
    CHECK(make_key(key) && make_key(other));
    const fs::path exe = dir / "live.exe";
    const std::string oldbytes = make_exe(5000, 20), newbytes = make_exe(300000, 21);
    const std::string manifest = make_manifest("99.1.0", newbytes);
    const std::string sig = sign_hex(key, manifest);
    Snapshot s;
    std::atomic<int> calls{0};
    auto on_done = [&calls] { ++calls; };

    // 1. Release assinada e valida, automatico ligado: instala sozinho
    {
        write_file(exe, oldbytes);
        srv.clear();
        publish(srv, "v99.1.0", newbytes, manifest, sig);
        Checker ck(make_cfg(srv, key, exe));
        ck.start("14.1.1", true, on_done);
        CHECK(wait_settled(ck, s));
        CHECK(s.state == State::Installed && s.release.tag == "v99.1.0" && s.note.empty());
        CHECK(read_file(exe) == newbytes);
        CHECK(read_file(exe.string() + ".old") == oldbytes);
        CHECK(calls.load() >= 2);                                  // avisou a interface: ao menos "instalando" e "instalada"
        CHECK(srv.hits_containing("Lettuce_Live_Editor.exe") == 1);
    }

    // 2. Automatico desligado: so avisa e nao baixa nada. O botao "Atualizar agora" instala.
    {
        write_file(exe, oldbytes);
        std::error_code ec;
        fs::remove(exe.string() + ".old", ec);
        srv.clear();
        publish(srv, "v99.1.0", newbytes, manifest, sig);
        Checker ck(make_cfg(srv, key, exe));
        ck.start("14.1.1", false, on_done);
        CHECK(wait_settled(ck, s));
        CHECK(s.state == State::Available && s.note.empty() && s.release.tag == "v99.1.0");
        CHECK(read_file(exe) == oldbytes);
        CHECK(srv.hits_containing("download") == 0);               // nada foi baixado
        ck.install("14.1.1", on_done);
        CHECK(wait_settled(ck, s));
        CHECK(s.state == State::Installed && read_file(exe) == newbytes);
        ck.install("14.1.1", on_done);                              // ja instalada: nao faz nada
        CHECK(ck.snapshot().state == State::Installed);
        // Com a atualizacao no lugar, outra verificacao e ignorada (o .old e a imagem em uso: reinstalar daria erro enganoso)
        const int before = srv.total_hits();
        ck.start("14.1.1", true, on_done);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        CHECK(ck.snapshot().state == State::Installed && srv.total_hits() == before);
        CHECK(read_file(exe) == newbytes);
    }

    // 3. Executavel adulterado no servidor (hash diferente do manifesto assinado): recusa e nada muda
    {
        write_file(exe, oldbytes);
        std::error_code ec;
        fs::remove(exe.string() + ".old", ec);
        srv.clear();
        std::string evil = newbytes;
        evil[2000] ^= 0x7F;
        publish(srv, "v99.1.0", evil, manifest, sig);
        Checker ck(make_cfg(srv, key, exe));
        ck.start("14.1.1", true, on_done);
        CHECK(wait_settled(ck, s));
        CHECK(s.state == State::Available && s.note == kNoteRefused);
        CHECK(read_file(exe) == oldbytes && !fs::exists(exe.string() + ".new") && !fs::exists(exe.string() + ".old"));
    }

    // 4. Manifesto e exe trocados juntos mas assinados por OUTRA chave (conta do GitHub comprometida): recusa
    {
        write_file(exe, oldbytes);
        srv.clear();
        const std::string evil = make_exe(9000, 77);
        const std::string evil_manifest = make_manifest("99.1.0", evil);
        publish(srv, "v99.1.0", evil, evil_manifest, sign_hex(other, evil_manifest));
        Checker ck(make_cfg(srv, key, exe));
        ck.start("14.1.1", true, on_done);
        CHECK(wait_settled(ck, s));
        CHECK(s.state == State::Available && s.note == kNoteRefused && s.detail == "signature does not verify");
        CHECK(read_file(exe) == oldbytes);
    }

    // 5. Release sem os arquivos assinados (como as versoes antigas): so o aviso, com o motivo
    {
        write_file(exe, oldbytes);
        srv.clear();
        srv.set("/repos/test/releases/latest", 200,
                std::string("{\"tag_name\":\"v99.1.0\",\"html_url\":\"") + kTagUrl + "v99.1.0\",\"draft\":false,\"prerelease\":false}");
        Checker ck(make_cfg(srv, key, exe));
        ck.start("14.1.1", true, on_done);
        CHECK(wait_settled(ck, s));
        CHECK(s.state == State::Available && s.note == kNoteUnsigned);
        CHECK(read_file(exe) == oldbytes);
    }

    // 6. Versao assinada que nao e mais nova que a atual (regressao): nem consulta os arquivos
    {
        write_file(exe, oldbytes);
        srv.clear();
        publish(srv, "v14.1.1", newbytes, make_manifest("14.1.1", newbytes), sign_hex(key, make_manifest("14.1.1", newbytes)));
        Checker ck(make_cfg(srv, key, exe));
        ck.start("14.1.1", true, on_done);
        CHECK(wait_settled(ck, s));
        CHECK(s.state == State::UpToDate);
        CHECK(srv.hits_containing("download") == 0 && read_file(exe) == oldbytes);
    }

    // 7. Tag da API diz 99.1.0 mas o manifesto assinado e de uma versao mais velha (replay de manifesto antigo)
    {
        write_file(exe, oldbytes);
        srv.clear();
        const std::string old_m = make_manifest("14.0.0", newbytes);
        publish(srv, "v99.1.0", newbytes, old_m, sign_hex(key, old_m));
        Checker ck(make_cfg(srv, key, exe));
        ck.start("14.1.1", true, on_done);
        CHECK(wait_settled(ck, s));
        CHECK(s.state == State::Available && s.note == kNoteRefused && s.detail == "manifest version differs from the release tag");
        CHECK(read_file(exe) == oldbytes);
    }

    // 8. Download do exe cai no meio (servidor responde erro): nota de download, nada muda
    {
        write_file(exe, oldbytes);
        srv.clear();
        publish(srv, "v99.1.0", newbytes, manifest, sig);
        srv.set("/test/releases/download/v99.1.0/Lettuce_Live_Editor.exe", 500, "boom");
        Checker ck(make_cfg(srv, key, exe));
        ck.start("14.1.1", true, on_done);
        CHECK(wait_settled(ck, s));
        CHECK(s.state == State::Available && s.note == kNoteDownload);
        CHECK(read_file(exe) == oldbytes);
    }

    // 9. Sem permissao para trocar (exe do app nao existe no caminho): write_failed, sem derrubar nada
    {
        srv.clear();
        publish(srv, "v99.1.0", newbytes, manifest, sig);
        Checker ck(make_cfg(srv, key, dir / "nao_existe_9.exe"));
        ck.start("14.1.1", true, on_done);
        CHECK(wait_settled(ck, s));
        CHECK(s.state == State::Available && s.note == kNoteWrite);
    }

    // 10. GitHub fora do ar
    {
        write_file(exe, oldbytes);
        srv.clear();
        srv.set("/repos/test/releases/latest", 500, "x");
        Checker ck(make_cfg(srv, key, exe));
        ck.start("14.1.1", true, on_done);
        CHECK(wait_settled(ck, s));
        CHECK(s.state == State::Failed);
        // resposta que nao e JSON de release e pre-release: tambem Failed, nunca instala
        srv.set("/repos/test/releases/latest", 200, "<html>nope</html>");
        ck.start("14.1.1", true, on_done);
        CHECK(wait_settled(ck, s) && s.state == State::Failed);
        srv.set("/repos/test/releases/latest", 200,
                std::string("{\"tag_name\":\"v99.1.0\",\"html_url\":\"") + kTagUrl + "v99.1.0\",\"draft\":false,\"prerelease\":true}");
        ck.start("14.1.1", true, on_done);
        CHECK(wait_settled(ck, s) && s.state == State::Failed);
        CHECK(read_file(exe) == oldbytes);
    }

    // 11. Destruir o Checker com a rede em andamento nao quebra (a thread guarda o proprio estado)
    {
        srv.clear();
        publish(srv, "v99.1.0", newbytes, manifest, sig);
        {
            Checker ck(make_cfg(srv, key, exe));
            ck.start("14.1.1", false, on_done);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        CHECK(true);
    }

    std::printf("Atualizacao: ciclo completo contra servidor local (valida, adulterada, sem assinatura, chave errada, regressao) PASS\n");
}

int main() {
    const fs::path dir = fs::temp_directory_path() / ("lle_selfupdate_test_" + std::to_string(GetCurrentProcessId()));
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir);
    std::printf("== Atualizacao automatica ==\n");
    test_crypto_and_manifest();
    test_apply(dir);
    test_end_to_end(dir);
    fs::remove_all(dir, ec);
    std::printf("%d verificacoes, %d falha(s)\n", g_checks, g_fail);
    return g_fail == 0 ? 0 : 1;
}
