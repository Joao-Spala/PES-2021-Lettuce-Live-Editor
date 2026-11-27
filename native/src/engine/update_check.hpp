#pragma once
// Aviso de nova versao (opt-in). Uma unica consulta HTTPS a API publica de releases do GitHub, em
// segundo plano, sem enviar nenhum dado do usuario. Nunca baixa nem executa nada: so mostra o link.

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

enum class State { Idle, Checking, UpToDate, Available, Failed };

struct Snapshot {
    State state = State::Idle;
    Release release;
};

class Checker {
public:
    Checker();
    ~Checker();
    Checker(const Checker&) = delete;
    Checker& operator=(const Checker&) = delete;

    // Inicia a consulta se nao houver outra em andamento. on_done roda na thread de rede (so deve postar
    // uma mensagem de janela). Depois do destrutor o callback nunca e chamado.
    void start(const std::string& current_version, std::function<void()> on_done);
    Snapshot snapshot() const;
#ifdef PES_QA_DUMP
    // So no harness de QA: forca um estado para capturar a tela sem rede
    void qa_force(State s, const Release& r);
#endif

private:
    struct Shared;
    std::shared_ptr<Shared> sh_;
};

} // namespace pes::update
