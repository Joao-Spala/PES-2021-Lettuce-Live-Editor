#pragma once
// Verificacao e instalacao de nova versao. Uma consulta HTTPS a API publica de releases do GitHub, em segundo
// plano, sem enviar nenhum dado do usuario. Com a atualizacao automatica ligada (padrao) o app tambem baixa a
// release, confere a assinatura do mantenedor e troca o proprio executavel (veja self_update.hpp). O novo
// executavel passa a valer na proxima abertura: o app so avisa e oferece reiniciar.

#include "self_update.hpp"

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace pes::update {

struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;
    bool valid = false;
};

// Aceita "v14.1.0", "14.1", "V14.1.0-beta": usa so o prefixo numerico. Invalida se nao comecar com digito.
Version parse_version(std::string_view text);
// <0 se a < b, 0 se iguais, >0 se a > b
int compare_versions(const Version& a, const Version& b);

struct Release {
    std::string tag;
    std::string url;
};

// Le o JSON de GET /repos/{repo}/releases/latest. Rejeita rascunho e pre-release, tag fora do formato
// de versao e URL fora do repositorio do projeto.
bool parse_release_json(std::string_view body, Release& out);
// Somente a pagina de releases do proprio projeto, em HTTPS, sem espacos nem caracteres de controle
bool is_trusted_release_url(std::string_view url);

enum class State {
    Idle,
    Checking,
    UpToDate,
    Available,    // ha versao nova que NAO foi instalada (automatico desligado, release sem assinatura ou falha: veja note)
    Installing,   // baixando, conferindo e trocando o executavel
    Installed,    // executavel novo no lugar: vale na proxima abertura
    Failed,       // nao foi possivel consultar o GitHub
};

// Por que uma versao nova nao foi instalada (note em Snapshot)
inline constexpr const char* kNoteNone = "";
inline constexpr const char* kNoteUnsigned = "unsigned";            // a release nao traz os arquivos assinados
inline constexpr const char* kNoteDownload = "download_failed";     // rede ou download incompleto
inline constexpr const char* kNoteRefused = "refused";              // assinatura, versao ou hash nao conferem
inline constexpr const char* kNoteWrite = "write_failed";           // sem permissao para trocar o executavel

struct Snapshot {
    State state = State::Idle;
    Release release;
    std::string note;     // kNote*, so quando state == Available
    std::string detail;   // texto curto em ingles para o log
};

struct Config {
    selfupdate::Source source = selfupdate::github_source();
    std::array<uint8_t, 64> public_key = selfupdate::kUpdatePublicKey;
    std::wstring exe_path;   // vazio = o executavel em execucao
};

class Checker {
public:
    explicit Checker(Config cfg = Config{});
    ~Checker();
    Checker(const Checker&) = delete;
    Checker& operator=(const Checker&) = delete;

    // Inicia a consulta se nao houver outra em andamento. Com auto_install, uma versao nova e instalada na
    // sequencia. on_done roda na thread de rede em cada mudanca de estado (so deve postar uma mensagem de
    // janela). Depois do destrutor o callback nunca e chamado.
    void start(const std::string& current_version, bool auto_install, std::function<void()> on_done);
    // Instala a versao que esta em Available (botao "Atualizar agora"). Nao faz nada em outro estado.
    void install(const std::string& current_version, std::function<void()> on_done);
    Snapshot snapshot() const;
#ifdef PES_QA_DUMP
    // So no harness de QA: forca um estado para capturar a tela sem rede
    void qa_force(State s, const Release& r, const std::string& note = std::string());
#endif

    struct Shared;   // estado compartilhado com as threads de rede (tipo opaco para quem usa a classe)

private:
    std::shared_ptr<Shared> sh_;
};

} // namespace pes::update
