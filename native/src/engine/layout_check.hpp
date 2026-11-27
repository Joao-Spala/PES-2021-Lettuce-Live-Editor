#pragma once
// Checagem da estrutura dos dados da carreira em execucao.
//
// O app acha os ganchos e ponteiros por assinatura de bytes, mas os DADOS ficam em offsets fixos da estrutura da
// carreira (ano, contador de atletas, clube do usuario, registros de 0x17C bytes...). Se uma versao do jogo ou de um
// patch reorganizar esses dados, as assinaturas ainda podem casar e o app leria e gravaria no campo errado.
//
// Esta checagem cruza campos independentes entre si: o clube do usuario (um offset da carreira) tem que apontar para
// um elenco de verdade dentro da tabela de atletas (outro offset, tamanho de registro, base e contador), e cada
// registro desse elenco precisa ter nome de texto, idade e atributos plausiveis (offsets de dentro do registro). Um layout
// diferente quebra pelo menos um desses cruzamentos. Quando algo nao bate, as edicoes que gravam na carreira ficam
// desligadas, com o motivo no relatorio, em vez de gravar no lugar errado.

#include <cstddef>
#include <cstdint>

namespace pes::layout {

enum class Verdict : uint8_t {
    Unknown,    // nao ha carreira carregada (ou clube do usuario ainda indefinido): nada a julgar
    Ok,
    Mismatch,
};

// Um registro de atleta compativel com o layout esperado
struct RecordPlausibility {
    bool named = false;    // nome de texto (ASCII ou UTF-8 valido) no campo de nome
    bool age_ok = false;   // idade entre 15 e 50
    bool stats_ok = false; // todos os atributos de linha e de goleiro entre 0 e 99
};
RecordPlausibility record_plausibility(const uint8_t* rec);

struct Input {
    int year = 0;                  // u16 em career + 0x1642A24
    uint32_t count_field = 0;      // u32 em career + 0xD0BCE8
    uint32_t max_records = 0;      // limite estrutural da tabela
    uint32_t club_handle = 0;      // u32 em career + 0x1642A2C (ou o derivado do BAL)
    int squad = 0;                 // registros validos que apontam para esse clube
    int named = 0;                 // quantos desses tem nome plausivel
    int age_ok = 0;
    int stats_ok = 0;
};

// Minimo de atletas no elenco do usuario e fracao minima que precisa passar em cada teste de registro
inline constexpr int kMinSquad = 11;
inline constexpr double kMinFraction = 0.8;

struct Result {
    Verdict verdict = Verdict::Unknown;
    const char* reason = "";   // codigo curto: no_club, year, count, squad_size, names, ages, stats
};
Result evaluate(const Input& in);

} // namespace pes::layout
