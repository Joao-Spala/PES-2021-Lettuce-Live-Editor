// Testes da checagem de estrutura dos dados da carreira (avaliador puro e plausibilidade de registro).

#include "../src/engine/layout_check.hpp"
#include "../src/engine/ovr_engine.hpp"
#include "../src/data/game_data.hpp"

#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

using namespace pes;
using namespace pes::layout;

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

// Registro de um atleta de linha como o jogo guarda: nome, idade e atributos
static std::vector<uint8_t> make_record(const char* name, int age, int stat) {
    std::vector<uint8_t> b(data::PLAYER_RECORD_SIZE, 0);
    set_bits_buf(b.data(), b.size(), 0x04, 28, 4, 12);   // posicao registrada
    set_apt_in_buf(b.data(), 12, 2);
    for (int f : data::OUTFIELD_FIDS) set_fid_in_buf(b.data(), f, stat);
    for (int f : data::GK_FIDS) set_fid_in_buf(b.data(), f, 40);
    b[0x1C] = static_cast<uint8_t>((b[0x1C] & ~0x3F) | (age & 0x3F));
    std::strncpy(reinterpret_cast<char*>(&b[0x38]), name, 0x2D);
    return b;
}

static Input good_input() {
    Input in;
    in.year = 2026;
    in.count_field = 24434;
    in.max_records = 61425;   // 0x1642A24 / 0x17C: a tabela nao pode passar do campo do ano
    in.club_handle = 0x138007A;
    in.squad = 31;
    in.named = in.age_ok = in.stats_ok = 31;
    return in;
}

int main() {
    // ---- plausibilidade de um registro
    {
        auto r = make_record("G. DE ARRASCAETA", 32, 78);
        auto p = record_plausibility(r.data());
        CHECK(p.named && p.age_ok && p.stats_ok);
        CHECK(record_plausibility(make_record("J\xC3\xBA" "nior Alonso", 24, 70).data()).named);    // UTF-8 valido
        CHECK(record_plausibility(make_record("\xE6\x97\xA5\xE6\x9C\xAC", 24, 70).data()).named);  // CJK (3 bytes por letra)
        CHECK(!record_plausibility(nullptr).named);

        CHECK(!record_plausibility(make_record("", 25, 70).data()).named);                    // sem nome
        CHECK(!record_plausibility(make_record("X", 25, 70).data()).named);                   // uma letra so
        CHECK(!record_plausibility(make_record("\x01\x02\x03\x04", 25, 70).data()).named);    // controle, nao texto
        CHECK(!record_plausibility(make_record("12", 25, 70).data()).named);                  // sem letras
        CHECK(!record_plausibility(make_record("Ab\xC3", 25, 70).data()).named);              // UTF-8 cortado no fim
        CHECK(!record_plausibility(make_record("Ab\x80zz", 25, 70).data()).named);            // continuacao solta
        CHECK(!record_plausibility(make_record("Ab\xFF" "zz", 25, 70).data()).named);         // byte invalido em UTF-8
        CHECK(!record_plausibility(make_record("Ab\xC3" "Xzz", 25, 70).data()).named);        // lead sem continuacao
        // nome sem terminador dentro do campo de 0x2E bytes
        auto full = make_record("A", 25, 70);
        std::memset(&full[0x38], 'A', 0x2E);
        CHECK(!record_plausibility(full.data()).named);

        CHECK(!record_plausibility(make_record("Fulano", 9, 70).data()).age_ok);              // idade fora do plausivel
        CHECK(record_plausibility(make_record("Fulano", 15, 70).data()).age_ok);
        CHECK(record_plausibility(make_record("Fulano", 50, 70).data()).age_ok);
        CHECK(!record_plausibility(make_record("Fulano", 51, 70).data()).age_ok);

        CHECK(record_plausibility(make_record("Fulano", 25, 99).data()).stats_ok);            // todos os atributos em 99
        CHECK(record_plausibility(make_record("Fulano", 25, 1).data()).stats_ok);
        // O setter limita a 40..99, entao o valor estourado (7 bits, ate 127) e gravado direto nos bytes do cabecalho
        auto all_bad = make_record("Fulano", 25, 70);
        std::memset(all_bad.data(), 0xFF, 0x28);
        CHECK(!record_plausibility(all_bad.data()).stats_ok);
        auto one_bad = make_record("Fulano", 25, 70);
        CHECK(record_plausibility(one_bad.data()).stats_ok);
        one_bad[3] = 0x7F;                                                                    // o atributo que mora no byte 3 vai a 127
        CHECK(!record_plausibility(one_bad.data()).stats_ok);
    }

    // ---- lixo aleatorio (tabela desalinhada) nao parece registro
    {
        std::mt19937 rng(12345);
        int named = 0, stats = 0, all = 0;
        const int n = 20000;
        for (int i = 0; i < n; ++i) {
            std::vector<uint8_t> rec(data::PLAYER_RECORD_SIZE);
            for (auto& v : rec) v = static_cast<uint8_t>(rng());
            const auto p = record_plausibility(rec.data());
            named += p.named;
            stats += p.stats_ok;
            all += (p.named && p.age_ok && p.stats_ok);
        }
        std::printf("  lixo aleatorio em %d registros: nome %d, atributos %d, os tres %d\n", n, named, stats, all);
        CHECK(named < n / 200);    // menos de 0,5%
        CHECK(stats < n / 50);     // menos de 2%
        CHECK(all == 0);
    }

    // ---- avaliador: o caso bom e cada recusa
    {
        Input in = good_input();
        Result r = evaluate(in);
        CHECK(r.verdict == Verdict::Ok && r.reason[0] == '\0');

        in = good_input(); in.club_handle = 0;
        r = evaluate(in);
        CHECK(r.verdict == Verdict::Unknown && std::string(r.reason) == "no_club");        // sem carreira: nao julga
        in.club_handle = 0xFFFFFFFFu;
        CHECK(evaluate(in).verdict == Verdict::Unknown);

        in = good_input(); in.year = 2018;
        r = evaluate(in);
        CHECK(r.verdict == Verdict::Mismatch && std::string(r.reason) == "year");
        in.year = 2151;
        CHECK(evaluate(in).verdict == Verdict::Mismatch);
        in.year = 2019;
        CHECK(evaluate(in).verdict == Verdict::Ok);
        in.year = 2150;
        CHECK(evaluate(in).verdict == Verdict::Ok);

        in = good_input(); in.count_field = 0;
        CHECK(std::string(evaluate(in).reason) == "count");
        in.count_field = 61426;
        CHECK(evaluate(in).verdict == Verdict::Mismatch && std::string(evaluate(in).reason) == "count");
        in.count_field = 61425;
        CHECK(evaluate(in).verdict == Verdict::Ok);

        in = good_input(); in.squad = 10; in.named = in.age_ok = in.stats_ok = 10;
        CHECK(evaluate(in).verdict == Verdict::Mismatch && std::string(evaluate(in).reason) == "squad_size");
        in.squad = 11; in.named = in.age_ok = in.stats_ok = 11;
        CHECK(evaluate(in).verdict == Verdict::Ok);
        in.squad = 0; in.named = in.age_ok = in.stats_ok = 0;
        CHECK(evaluate(in).verdict == Verdict::Mismatch);   // elenco vazio (o caso do FL que motivou isto)

        // fracoes: 80% de 31 e 24.8, entao 25 passa e 24 nao
        in = good_input(); in.named = 25;
        CHECK(evaluate(in).verdict == Verdict::Ok);
        in.named = 24;
        CHECK(evaluate(in).verdict == Verdict::Mismatch && std::string(evaluate(in).reason) == "names");
        in = good_input(); in.age_ok = 24;
        CHECK(std::string(evaluate(in).reason) == "ages");
        in = good_input(); in.stats_ok = 24;
        CHECK(std::string(evaluate(in).reason) == "stats");

        // Um elenco real mistura registros bons e alguns esquisitos (jogador sem nome de exibicao etc.)
        in = good_input(); in.named = 29; in.age_ok = 30; in.stats_ok = 31;
        CHECK(evaluate(in).verdict == Verdict::Ok);
    }

    // ---- o cenario completo: um elenco de 31 registros bons passa, o mesmo elenco com campos deslocados nao
    {
        std::vector<std::vector<uint8_t>> squad;
        for (int i = 0; i < 31; ++i) squad.push_back(make_record(("Jogador " + std::to_string(i)).c_str(), 20 + i % 15, 60 + i % 25));
        auto tally = [&](auto&& transform) {
            Input in = good_input();
            in.squad = in.named = in.age_ok = in.stats_ok = 0;
            for (const auto& r : squad) {
                const auto p = record_plausibility(transform(r).data());
                ++in.squad; in.named += p.named; in.age_ok += p.age_ok; in.stats_ok += p.stats_ok;
            }
            return in;
        };
        Input intact = tally([](const std::vector<uint8_t>& r) { return r; });
        CHECK(intact.squad == 31 && intact.named == 31 && intact.age_ok == 31 && intact.stats_ok == 31);
        CHECK(evaluate(intact).verdict == Verdict::Ok);

        // layout diferente: o nome foi para outro lugar
        Input moved_name = tally([](std::vector<uint8_t> r) {
            std::vector<uint8_t> s = r;
            std::memset(&s[0x38], 0, 0x2E);
            std::memcpy(&s[0x58], &r[0x38], 0x0E);
            return s;
        });
        CHECK(evaluate(moved_name).verdict == Verdict::Mismatch && std::string(evaluate(moved_name).reason) == "names");

        // o registro inteiro deslocado em 4 bytes (tamanho de registro ou base diferentes)
        Input shifted = tally([](const std::vector<uint8_t>& r) {
            std::vector<uint8_t> s(r.size(), 0);
            std::memcpy(s.data() + 4, r.data(), r.size() - 4);
            return s;
        });
        CHECK(evaluate(shifted).verdict == Verdict::Mismatch);
    }

    std::printf("layout: %d checagens, %d falhas\n", g_checks, g_fail);
    return g_fail == 0 ? 0 : 1;
}
