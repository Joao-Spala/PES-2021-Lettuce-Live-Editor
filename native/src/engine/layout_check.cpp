#include "layout_check.hpp"

#include "ovr_engine.hpp"
#include "../data/game_data.hpp"

namespace pes::layout {

namespace {

// Campo de nome: texto (ASCII imprimivel ou UTF-8 valido) terminado em NUL dentro do campo, com ao menos duas letras.
// UTF-8 valido e o que separa nome de dado desalinhado: bytes soltos acima de 0x80 quase nunca formam sequencias validas.
bool name_like(const uint8_t* p, size_t cap) {
    size_t n = 0;
    while (n < cap && p[n] != 0) ++n;
    if (n < 2 || n >= cap) return false;   // vazio, ou sem terminador dentro do campo
    int letters = 0;
    size_t i = 0;
    while (i < n) {
        const uint8_t c = p[i];
        if (c < 0x80) {
            if (c < 0x20 || c == 0x7F) return false;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) ++letters;
            ++i;
            continue;
        }
        size_t extra = 0;
        if (c >= 0xC2 && c <= 0xDF) extra = 1;
        else if (c >= 0xE0 && c <= 0xEF) extra = 2;
        else if (c >= 0xF0 && c <= 0xF4) extra = 3;
        else return false;                       // continuacao solta ou byte invalido
        if (i + extra >= n) return false;        // sequencia cortada pelo fim do texto
        for (size_t k = 1; k <= extra; ++k) {
            if ((p[i + k] & 0xC0) != 0x80) return false;
        }
        ++letters;
        i += extra + 1;
    }
    return letters >= 2;
}

// Todos os atributos do registro (linha e goleiro) entre 0 e 99. Dado desalinhado estoura isso: o campo guarda 7 bits e
// valores acima de 99 aparecem em 22% dos casos, em cada um dos ~25 atributos.
bool stats_plausible(const uint8_t* rec) {
    for (int f : data::OUTFIELD_FIDS) {
        if (pes::get_fid_from_buf(rec, f) > 99) return false;
    }
    for (int f : data::GK_FIDS) {
        if (pes::get_fid_from_buf(rec, f) > 99) return false;
    }
    return true;
}

} // namespace

RecordPlausibility record_plausibility(const uint8_t* rec) {
    RecordPlausibility r;
    if (!rec) return r;
    r.named = name_like(rec + 0x38, 0x2E);
    const int age = rec[0x1C] & 0x3F;
    r.age_ok = age >= 15 && age <= 50;
    r.stats_ok = stats_plausible(rec);
    return r;
}

Result evaluate(const Input& in) {
    Result r;
    if (in.club_handle == 0 || in.club_handle == 0xFFFFFFFFu) {
        r.verdict = Verdict::Unknown;
        r.reason = "no_club";
        return r;
    }
    r.verdict = Verdict::Mismatch;
    if (in.year < 2019 || in.year > 2150) { r.reason = "year"; return r; }
    if (in.count_field == 0 || in.count_field > in.max_records) { r.reason = "count"; return r; }
    if (in.squad < kMinSquad) { r.reason = "squad_size"; return r; }
    const double need = kMinFraction * static_cast<double>(in.squad);
    if (static_cast<double>(in.named) < need) { r.reason = "names"; return r; }
    if (static_cast<double>(in.age_ok) < need) { r.reason = "ages"; return r; }
    if (static_cast<double>(in.stats_ok) < need) { r.reason = "stats"; return r; }
    r.verdict = Verdict::Ok;
    r.reason = "";
    return r;
}

} // namespace pes::layout
