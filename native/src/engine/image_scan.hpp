#pragma once
// Leitura do cabecalho PE do jogo, impressao digital da versao e busca de assinaturas de bytes em todo o codigo.
//
// Por que existe: o app nunca deve depender de enderecos fixos de UM executavel. Todo gancho e todo ponteiro do
// jogo e achado por uma assinatura de bytes (com curingas) que precisa ser UNICA no codigo do processo. O mesmo
// motor funciona em qualquer executavel baseado na engine do PES 2021, e quando uma assinatura falha o app sabe
// dizer qual, em vez de gravar em um endereco errado.

#include "win32_mem.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace pes {

struct ImageSection {
    std::string name;
    uint32_t rva = 0;
    uint32_t vsize = 0;
    uint32_t characteristics = 0;
    // Secao com codigo: IMAGE_SCN_CNT_CODE (0x20) ou IMAGE_SCN_MEM_EXECUTE (0x20000000)
    bool is_code() const { return (characteristics & 0x20u) != 0 || (characteristics & 0x20000000u) != 0; }
    // Secao executavel de fato (IMAGE_SCN_MEM_EXECUTE): so nelas uma assinatura pode ser codigo. Secoes marcadas apenas
    // como "contem codigo" (CNT_CODE) costumam guardar dados.
    bool is_exec() const { return (characteristics & 0x20000000u) != 0; }
};

struct ImageInfo {
    bool valid = false;
    uint64_t base = 0;
    uint32_t size_of_image = 0;
    uint32_t timestamp = 0;      // carimbo de data do link (TimeDateStamp)
    uint32_t checksum = 0;
    uint32_t entry_rva = 0;
    uint16_t machine = 0;
    std::vector<ImageSection> sections;
};

// Le o cabecalho PE do modulo principal direto da memoria do processo
bool read_image_info(const Win32Memory& mem, ImageInfo& out);

// Versoes do jogo conferidas a mao. Fora da lista o app continua funcionando pelas assinaturas, mas avisa.
struct KnownBuild {
    const char* name;
    uint32_t timestamp;
    uint32_t size_of_image;
    bool reference_rvas;   // true so no executavel de referencia: a tabela antiga de RVAs fixos vale apenas nele
};
const KnownBuild* identify_build(const ImageInfo& img);
const KnownBuild* known_builds(size_t& count);

// ---- Padroes de bytes com curinga ("48 8B ?? 2C")
struct Pattern {
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> mask;   // 0xFF = byte fixo, 0x00 = curinga
    bool empty() const { return bytes.empty(); }
};
bool parse_pattern(std::string_view text, Pattern& out);

// Procura `p` em `data` e grava ate `max_out` posicoes. Retorna quantas achou (conta ate `count_cap`).
size_t find_pattern(const uint8_t* data, size_t len, const Pattern& p, size_t* out_pos, size_t max_out, size_t count_cap = 16);

// ---- Assinaturas
struct SigDef {
    const char* id;        // nome curto, usado no relatorio de diagnostico
    const char* pattern;   // bytes com curingas
    uint32_t hint_rva;     // onde a assinatura cai na versao de referencia (so informativo)
};

struct SigResult {
    std::string id;
    uint32_t hint_rva = 0;
    int matches = 0;       // quantas ocorrencias (0 = nao achou, >1 = ambigua)
    uint64_t addr = 0;     // endereco da ocorrencia unica (0 se nao ha uma)
    bool unique() const { return matches == 1 && addr != 0; }
};

// Procura assinaturas em todas as secoes de codigo do processo, le o codigo uma vez e guarda o resultado.
class SigResolver {
public:
    explicit SigResolver(Win32Memory& m) : mem_(m) {}

    // Esquece tudo (novo processo)
    void reset();
    // Endereco da ocorrencia UNICA da assinatura, ou 0 (nao achou ou ambigua). Sucesso fica em cache. Falha e
    // tentada de novo depois de alguns segundos (o jogo pode ainda estar descompactando o codigo).
    uint64_t find(const SigDef& def);
    const ImageInfo& image();
    const std::map<std::string, SigResult>& results() const { return results_; }
    // Texto para o relatorio: versao do jogo, secoes e o resultado de cada assinatura
    std::string describe();
    // Libera o codigo lido (so as assinaturas ja achadas continuam em cache)
    void release_code();
    // Plano B, no maximo uma vez por processo: se NENHUMA assinatura foi achada nas secoes executaveis normais, o codigo
    // pode estar numa secao enorme (empacotador). Le essas secoes uma unica vez e procura todas as pendentes juntas.
    void resolve_bulk(const SigDef* const* defs, size_t n);
    // Quanto das secoes executaveis foi lido na ultima carga: o relatorio mostra, para nao fingir cobertura total
    struct Coverage {
        uint64_t scanned = 0;      // bytes lidos
        uint64_t unreadable = 0;   // bytes de paginas que nao puderam ser lidas
        uint64_t skipped = 0;      // bytes de secoes grandes demais, fora da busca normal
    };
    const Coverage& coverage() const { return cov_; }

private:
    bool ensure_loaded();
    size_t scan_code(const Pattern& p, uint64_t* out_addr, size_t max_out);
    void schedule_retry(const std::string& id);

    struct Chunk {
        uint32_t rva = 0;
        std::vector<uint8_t> bytes;
    };
    Win32Memory& mem_;
    ImageInfo img_;
    bool img_read_ = false;
    bool loaded_ = false;
    std::vector<Chunk> code_;
    std::map<std::string, SigResult> results_;
    std::map<std::string, uint64_t> retry_after_;   // GetTickCount64 da proxima tentativa de uma assinatura que falhou
    std::map<std::string, int> fail_count_;         // falhas seguidas: o intervalo entre tentativas cresce de 3 s ate 60 s
    Coverage cov_;
    bool bulk_done_ = false;
};

} // namespace pes
