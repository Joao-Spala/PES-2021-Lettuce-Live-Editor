// Testes do leitor de PE, da impressao digital de versao e do buscador de assinaturas (image_scan + signatures).
// Roda sobre o proprio processo de teste: o cabecalho PE e o codigo dele sao lidos pelo mesmo caminho que le o jogo.

#include "../src/engine/image_scan.hpp"
#include "../src/engine/signatures.hpp"

#include <cstdio>
#include <cstring>
#include <set>
#include <string>

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

// Marcadores de bytes colocados no CODIGO do executavel de teste. Duas copias de um, uma de outro.
__attribute__((noinline)) static void marker_unique() {
    asm volatile(".byte 0xDE,0xAD,0xBE,0xEF,0x13,0x37,0xC0,0xDE,0x42,0x01");
}
__attribute__((noinline)) static void marker_twice_a() {
    asm volatile(".byte 0xCA,0xFE,0xBA,0xBE,0x77,0x66,0x55,0x44,0x33");
}
__attribute__((noinline)) static void marker_twice_b() {
    asm volatile(".byte 0xCA,0xFE,0xBA,0xBE,0x77,0x66,0x55,0x44,0x33");
}

int main() {
    // ---- parse_pattern
    {
        Pattern p;
        CHECK(parse_pattern("48 8B ?? 2C 89 01", p));
        CHECK(p.bytes.size() == 6);
        CHECK(p.mask[2] == 0x00 && p.mask[0] == 0xFF);
        CHECK(p.bytes[0] == 0x48 && p.bytes[5] == 0x01);
        CHECK(parse_pattern("  e8   ??  ?? ?? ?? 48  8b 48 48 ", p));   // minusculas e espacos extras
        CHECK(p.bytes.size() == 9 && p.mask[1] == 0);
        CHECK(!parse_pattern("48 8B ?? ??", p) && p.empty());           // so 2 bytes fixos
    }
    {
        Pattern p;
        CHECK(!parse_pattern("48 8B ?? 2C", p));   // 3 bytes fixos: abaixo do minimo de 4
        CHECK(parse_pattern("48 8B 05 2C", p));
        CHECK(!parse_pattern("48 GG 05 2C 11", p) && p.empty());
        CHECK(!parse_pattern("", p) && p.empty());
        CHECK(!parse_pattern("4 8B 05 2C 11", p));   // byte com um digito so
        CHECK(!parse_pattern("488B05 2C 11 22", p)); // bytes colados
    }

    // ---- find_pattern
    {
        uint8_t buf[64]{};
        for (size_t i = 0; i < sizeof(buf); ++i) buf[i] = static_cast<uint8_t>(i * 7 + 3);
        const uint8_t sig[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11};
        std::memcpy(buf + 20, sig, 6);

        Pattern p;
        size_t pos[8]{};
        CHECK(parse_pattern("AA BB CC DD EE 11", p));
        CHECK(find_pattern(buf, sizeof(buf), p, pos, 8) == 1 && pos[0] == 20);

        CHECK(parse_pattern("AA BB ?? DD EE 11", p));   // curinga no meio
        CHECK(find_pattern(buf, sizeof(buf), p, pos, 8) == 1 && pos[0] == 20);

        CHECK(parse_pattern("AA BB CC DD EE 12", p));   // ultimo byte diferente
        CHECK(find_pattern(buf, sizeof(buf), p, pos, 8) == 0);

        // Na borda: comeco e fim do bloco
        uint8_t edge[16]{};
        std::memcpy(edge, sig, 6);
        std::memcpy(edge + 10, sig, 6);
        CHECK(parse_pattern("AA BB CC DD EE 11", p));
        const size_t n = find_pattern(edge, sizeof(edge), p, pos, 8);
        CHECK(n == 2 && pos[0] == 0 && pos[1] == 10);

        // Bloco menor que o padrao e ponteiro nulo nao quebram
        CHECK(find_pattern(edge, 3, p, pos, 8) == 0);
        CHECK(find_pattern(nullptr, 0, p, pos, 8) == 0);

        // Curinga no inicio: a ancora precisa ser outro trecho
        uint8_t w[12] = {0x01, 0x02, 0xAA, 0xBB, 0xCC, 0xDD, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04};
        CHECK(parse_pattern("?? ?? AA BB CC DD", p));
        CHECK(find_pattern(w, sizeof(w), p, pos, 8) == 1 && pos[0] == 0);
        CHECK(parse_pattern("?? AA BB CC DD", p));
        CHECK(find_pattern(w, sizeof(w), p, pos, 8) == 1 && pos[0] == 1);
    }

    // ---- read_image_info no proprio processo
    Win32Memory self;
    self.handle = GetCurrentProcess();   // pseudo-handle: ReadProcessMemory aceita
    self.module_base = reinterpret_cast<uint64_t>(GetModuleHandleW(nullptr));
    {
        ImageInfo info;
        CHECK(read_image_info(self, info));
        CHECK(info.valid && info.base == self.module_base);
        CHECK(info.machine == 0x8664);
        CHECK(info.size_of_image > 0x1000 && !info.sections.empty());
        bool has_code = false, has_exec = false;
        for (const auto& s : info.sections) {
            has_code = has_code || s.is_code();
            has_exec = has_exec || s.is_exec();
            if (s.is_exec()) CHECK(s.is_code());   // executavel sempre conta como codigo, o contrario nao vale
        }
        CHECK(has_code && has_exec);
        ImageSection data_only;
        data_only.characteristics = 0xC0000020u;   // CNT_CODE sem MEM_EXECUTE (como a .text1 do PES 2021)
        CHECK(data_only.is_code() && !data_only.is_exec());
        CHECK(identify_build(info) == nullptr);   // o binario de teste nao e um jogo conhecido

        // Base errada: nao e PE
        Win32Memory bad;
        bad.handle = GetCurrentProcess();
        bad.module_base = self.module_base + 0x10;
        ImageInfo none;
        CHECK(!read_image_info(bad, none) && !none.valid);
        bad.handle = nullptr;
    }

    // ---- impressao digital de versao
    {
        ImageInfo img;
        img.valid = true;
        img.timestamp = 0x62787C6Au;
        img.size_of_image = 0x1B48A000u;
        const KnownBuild* ref = identify_build(img);
        CHECK(ref && ref->reference_rvas);

        img.timestamp = 0x5F5ED4BFu;
        img.size_of_image = 0x1BBA6000u;
        const KnownBuild* fl = identify_build(img);
        CHECK(fl && !fl->reference_rvas);   // conhecido, mas sem a tabela de RVAs fixos

        img.size_of_image = 0x1BBA7000u;     // mesmo carimbo, tamanho diferente
        CHECK(identify_build(img) == nullptr);
        img.valid = false;
        CHECK(identify_build(img) == nullptr);

        size_t cnt = 0;
        known_builds(cnt);
        CHECK(cnt >= 2);
    }

    // ---- SigResolver sobre o codigo do proprio processo
    {
        void (*volatile keep)() = marker_unique;   // impede o compilador de remover as funcoes
        void (*volatile keep_a)() = marker_twice_a;
        void (*volatile keep_b)() = marker_twice_b;
        (void)keep; (void)keep_a; (void)keep_b;

        SigResolver res(self);
        const SigDef uniq{"t_unique", "DE AD BE EF 13 37 C0 DE 42 01", 0};
        const SigDef wild{"t_wild", "DE AD ?? EF 13 37 C0 DE 42", 0};
        const SigDef dup{"t_dup", "CA FE BA BE 77 66 55 44 33", 0};
        const SigDef none{"t_none", "12 34 56 78 9A BC DE F0 11 22", 0};

        const uint64_t a = res.find(uniq);
        CHECK(a != 0);
        if (a) {
            uint8_t got[10]{};
            CHECK(self.read_into(a, got, sizeof(got)));
            const uint8_t want[10] = {0xDE, 0xAD, 0xBE, 0xEF, 0x13, 0x37, 0xC0, 0xDE, 0x42, 0x01};
            CHECK(std::memcmp(got, want, sizeof(want)) == 0);
        }
        CHECK(res.find(wild) == a);     // o curinga acha o mesmo lugar
        CHECK(res.find(uniq) == a);     // segunda chamada vem do cache
        CHECK(res.find(dup) == 0);      // duas ocorrencias: ambigua, nao devolve endereco
        CHECK(res.results().count("t_dup") && res.results().at("t_dup").matches == 2);
        CHECK(res.find(none) == 0);
        CHECK(res.results().count("t_none") && res.results().at("t_none").matches == 0);

        const std::string rep = res.describe();
        CHECK(rep.find("t_unique") != std::string::npos);
        CHECK(rep.find("AMBIGUOUS") != std::string::npos);
        CHECK(rep.find("NOT FOUND") != std::string::npos);
        CHECK(rep.find("UNKNOWN") != std::string::npos);   // build desconhecido avisa

        // Cobertura: o relatorio nao finge que leu o que nao leu
        CHECK(res.coverage().scanned > 0);
        CHECK(rep.find("scan coverage") != std::string::npos);

        // Plano B: sem secao enorme para varrer, nao muda nada e nao quebra ao ser repetido
        res.resolve_bulk(sig::kAll, sizeof(sig::kAll) / sizeof(sig::kAll[0]));
        res.resolve_bulk(sig::kAll, sizeof(sig::kAll) / sizeof(sig::kAll[0]));
        CHECK(res.results().count("career_owner") == 0);
        CHECK(res.find(uniq) == a);

        res.release_code();
        CHECK(res.find(uniq) == a);     // o que ja foi achado continua em cache depois de liberar o codigo

        res.reset();
        CHECK(res.results().empty());
        CHECK(res.find(uniq) == a);     // e achado de novo depois do reset
    }

    // ---- tabela de assinaturas do app
    {
        std::set<std::string> ids;
        for (const SigDef* d : sig::kAll) {
            Pattern p;
            CHECK(parse_pattern(d->pattern, p));     // todas validas e com bytes fixos suficientes
            CHECK(ids.insert(d->id).second);         // ids unicos
            CHECK(d->hint_rva != 0);
        }
        CHECK(ids.size() == sizeof(sig::kAll) / sizeof(sig::kAll[0]));
        // O gancho do folego refaz o epilogo da funcao na mao: a assinatura precisa cobrir ate o RET
        Pattern st;
        CHECK(parse_pattern(sig::kStamina.pattern, st));
        CHECK(st.bytes.size() == 24 && st.bytes.back() == 0xC3 && st.bytes[0] == 0x6B);
        // As duas de ponteiro carregam o deslocamento do campo da estrutura da carreira (guarda de layout)
        Pattern p;
        parse_pattern(sig::kCareerGetter.pattern, p);
        const uint8_t guard[4] = {0xE0, 0xCB, 0x6E, 0x01};
        size_t pos[2]{};
        CHECK(find_pattern(p.bytes.data(), p.bytes.size(), [&] { Pattern g; g.bytes.assign(guard, guard + 4); g.mask.assign(4, 0xFF); return g; }(), pos, 2) == 1);
    }

    self.handle = nullptr;   // pseudo-handle: nao deve ser fechado pelo destrutor
    std::printf("image_scan: %d checagens, %d falhas\n", g_checks, g_fail);
    return g_fail == 0 ? 0 : 1;
}
