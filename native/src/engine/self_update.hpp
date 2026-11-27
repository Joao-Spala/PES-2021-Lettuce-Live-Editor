#pragma once
// Atualizacao automatica assinada do proprio executavel (mecanica: rede, criptografia e troca de arquivo).
//
// Modelo de confianca: cada release publica, alem do zip, tres arquivos soltos
//   Lettuce_Live_Editor.exe, update_manifest.txt e update_manifest.sig
// O manifesto lista a versao, o SHA-256 e o tamanho do exe e e assinado com a chave privada do mantenedor
// (ECDSA P-256 sobre SHA-256), que nunca sai do PC dele. A chave publica esta compilada aqui. Uma conta do
// GitHub comprometida, sozinha, nao consegue empurrar codigo: sem a assinatura valida nada e instalado.
//
// O app so instala quando TODAS as condicoes valem: assinatura valida, versao do manifesto igual a tag da
// release, versao acima da que esta rodando (nunca regride), hash e tamanho do exe iguais ao manifesto e o
// arquivo e um executavel PE de 64 bits. Qualquer falha deixa o executavel atual intacto.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace pes::selfupdate {

// Metade publica da chave de assinatura das releases (X depois Y). A privada fica fora do repositorio.
inline constexpr std::array<uint8_t, 64> kUpdatePublicKey = {
    0x53, 0x5C, 0xB8, 0x8B, 0xD4, 0x7E, 0x32, 0xC4, 0x55, 0xBA, 0xC8, 0x5B, 0xE0, 0xF9, 0x3F, 0x8A,
    0x93, 0x2D, 0x61, 0x7B, 0xB1, 0xB8, 0x99, 0xE8, 0x6C, 0xAE, 0xE8, 0xE5, 0x5A, 0x45, 0xF1, 0xB2,
    0x81, 0xC4, 0xDB, 0xE3, 0x4B, 0x9D, 0x8B, 0x55, 0xF7, 0xE8, 0x47, 0xCF, 0x16, 0x67, 0xFE, 0x11,
    0x7E, 0xD3, 0x23, 0x60, 0xE9, 0xBD, 0x38, 0xD3, 0x77, 0xB5, 0x4A, 0xC7, 0x95, 0x67, 0x09, 0xC0,
};

// Primeira linha do manifesto. Diferente da de outros produtos que usem a mesma chave: um manifesto de outro
// projeto nao vale aqui.
inline constexpr const char* kManifestHeader = "lettuce-live-editor update 1";

// Nomes fixos dos arquivos soltos na release
inline constexpr const wchar_t* kAssetManifest = L"update_manifest.txt";
inline constexpr const wchar_t* kAssetSignature = L"update_manifest.sig";
inline constexpr const wchar_t* kAssetExe = L"Lettuce_Live_Editor.exe";

inline constexpr size_t kMaxApiBytes = 256 * 1024;
inline constexpr size_t kMaxManifestBytes = 4 * 1024;
inline constexpr size_t kMaxSignatureBytes = 1024;
inline constexpr size_t kMaxExeBytes = 64 * 1024 * 1024;

// De onde vem a release. O padrao e o GitHub em HTTPS. Os testes apontam para um servidor local sem TLS:
// nenhuma configuracao do usuario, arquivo ou variavel de ambiente muda isso.
struct Source {
    std::wstring api_host = L"api.github.com";
    std::wstring api_path;                    // /repos/<dono>/<repo>/releases/latest
    std::wstring dl_host = L"github.com";
    std::wstring dl_prefix;                   // /<dono>/<repo>/releases/download/
    uint16_t port = 443;
    bool secure = true;
};
Source github_source();

struct Manifest {
    std::string version;       // "14.2.0", sem o "v"
    uint8_t exe_hash[32] = {};
    uint64_t exe_size = 0;
};

// ---- Rede
// GET com limite de bytes. `api` escolhe o host da API ou o de downloads. Redirecionamentos para a CDN do
// GitHub sao seguidos, nunca de HTTPS para HTTP. False em qualquer erro de rede ou status diferente de 200.
bool http_get(const Source& src, bool api, const std::wstring& path, const std::string& user_agent, size_t max_bytes,
              std::string& body, unsigned long* http_status);

// ---- Criptografia e manifesto (puras, testadas)
bool sha256(const void* data, size_t len, uint8_t out[32]);
// Texto hexadecimal (espacos nas pontas permitidos) em exatamente out_len bytes
bool parse_hex(std::string_view text, uint8_t* out, size_t out_len);
// ECDSA P-256 sobre SHA-256(msg). Assinatura r||s (64 bytes), chave publica X||Y (64 bytes)
bool verify_signature(const void* msg, size_t len, const uint8_t sig[64], const uint8_t pub_xy[64]);
bool parse_manifest(std::string_view text, Manifest& out);
// Confere o download de ponta a ponta. Em falha, *why diz o motivo (texto curto em ingles para o log).
bool validate_update(const std::string& manifest_text, const std::string& sig_hex, const std::string& tag,
                     const std::string& exe_bytes, const std::string& current_version, const uint8_t pub_xy[64],
                     Manifest& out, const char** why);

// ---- Troca do executavel em execucao
// O Windows deixa RENOMEAR um exe em uso, mas nao sobrescrever. Grava <exe>.new, confere a gravacao, renomeia o
// atual para <exe>.old e o novo para o nome original. Em qualquer falha desfaz tudo e o exe atual continua
// no lugar. *error recebe o codigo do Windows da falha.
bool apply_update(const std::wstring& exe_path, const std::string& new_exe, unsigned long* error);
// No inicio do app: apaga o .old e o .new que sobraram de uma atualizacao anterior (falha em silencio se em uso)
void finish_previous_update(const std::wstring& exe_path);
std::wstring current_exe_path();
// Abre o exe com "--after-update <PID deste processo>" e devolve true se o processo nasceu
bool relaunch(const std::wstring& exe_path);
// PID depois de "--after-update" na linha de comando, 0 se nao houver
unsigned long parse_after_update_pid(const wchar_t* cmdline);
// Espera o processo terminar (ate timeout_ms). Usado pelo app reaberto para nao disputar a instancia unica.
void wait_for_exit(unsigned long pid, unsigned long timeout_ms);

} // namespace pes::selfupdate
