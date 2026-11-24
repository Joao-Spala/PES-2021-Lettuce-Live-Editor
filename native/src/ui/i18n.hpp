#pragma once
// 100% PT-BR / EN localization & currency formatting for PES 2021 Workbench Native.

#include "../data/game_data.hpp"
#include "../engine/pes_engine.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

namespace pes::ui {

enum class Lang : int { PT = 0, EN = 1 };

struct CurrencyInfo {
    const char* code;
    const char* sym;
    char sep;
};

inline constexpr CurrencyInfo kCurrencies[4] = {
    {"EUR", "€", '.'}, // €
    {"BRL", "R$", '.'},
    {"USD", "$", ','},
    {"GBP", "£", ','},     // £
};

inline const char* T(Lang lang, const char* pt, const char* en) {
    return (lang == Lang::EN) ? en : pt;
}

inline const char* pos_short(Lang lang, int pos_id) {
    if (pos_id < 0 || pos_id >= 13) return "---";
    return (lang == Lang::EN) ? data::POSITIONS[pos_id].en : data::POSITIONS[pos_id].pt;
}

inline const char* pos_full(Lang lang, int pos_id) {
    if (pos_id < 0 || pos_id >= 13) return "---";
    return (lang == Lang::EN) ? data::POSITIONS[pos_id].name_en : data::POSITIONS[pos_id].name_pt;
}

inline std::string fmt_money(int32_t raw, int cur_idx, Lang lang) {
    const auto& cur = kCurrencies[(cur_idx >= 0 && cur_idx < 4) ? cur_idx : 0];
    int64_t euros = static_cast<int64_t>(raw) * 100LL;
    bool neg = euros < 0;
    uint64_t abs_v = static_cast<uint64_t>(neg ? -euros : euros);
    std::string digits = std::to_string(abs_v);
    char sep = (lang == Lang::EN && (cur_idx == 2 || cur_idx == 3)) ? ',' : '.';
    std::string grouped;
    grouped.reserve(digits.size() + digits.size() / 3);
    int cnt = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
        if (cnt > 0 && cnt % 3 == 0) grouped.push_back(sep);
        grouped.push_back(digits[i]);
        ++cnt;
    }
    std::reverse(grouped.begin(), grouped.end());
    std::string out;
    if (neg) out.push_back('-');
    out += cur.sym;
    out.push_back(' ');
    out += grouped;
    return out;
}

inline std::string fmt_club(Lang lang, const std::string& club) {
    return club.empty() ? std::string(T(lang, "Sem Clube / Livre", "Free Agent")) : club;
}

inline const char* fmt_mode(Lang lang, const std::string& code) {
    if (code == "ml") return "Master League";
    if (code == "bal") return T(lang, "Rumo ao Estrelato (BAL)", "Become a Legend (BAL)");
    if (code == "menu") return T(lang, "Menu / Exibição", "Menu / Exhibition");
    return T(lang, "Aguardando PES 2021...", "Waiting for PES 2021...");
}

inline std::string fmt_code_msg(Lang lang, const std::string& code, int orig = 0, int fin = 0, int pri = 0, int pos = -1, int reg_pos = -1, bool needs_grade = false) {
    if (code == "not_connected") return T(lang, "Jogo não conectado.", "Game not connected.");
    if (code == "no_career") return T(lang, "Abra uma Master League ou BAL primeiro.", "Open a Master League or BAL save first.");
    if (code == "no_player") return T(lang, "Nenhum atleta selecionado.", "No player selected.");
    if (code == "nothing_to_undo") return T(lang, "Nenhuma alteração para desfazer.", "Nothing to undo.");
    if (code == "invalid_money") return T(lang, "Valor monetário inválido (use ex: 25M, 500k ou 1.5B).", "Invalid money amount (use e.g. 25M, 500k or 1.5B).");
    if (code == "not_in_match") return T(lang, "Entre em uma partida para congelar o relógio.", "Start a match first to freeze the clock.");
    if (code == "hook_failed") return T(lang, "Não foi possível ativar o hook x64 neste executável.", "Could not activate x64 hook on this executable.");
    if (code == "already_at_ovr") {
        char b[96];
        std::snprintf(b, sizeof(b), T(lang, "Já está em %d OVR.", "Already at %d OVR."), orig ? orig : fin);
        return b;
    }
    if (code == "pri_updated") {
        char b[96];
        std::snprintf(b, sizeof(b), T(lang, "OVR Principal: %d -> %d", "Primary OVR: %d -> %d"), orig, fin);
        return b;
    }
    if (code == "pos_updated") {
        char b[160];
        std::snprintf(b, sizeof(b),
            T(lang, "%s: %d -> %d OVR (%s mantido em %d)", "%s: %d -> %d OVR (%s kept at %d)"),
            pos_short(lang, pos), orig, fin, pos_short(lang, reg_pos), pri);
        return b;
    }
    if (code == "pos_partial") {
        char b[220];
        std::snprintf(b, sizeof(b),
            T(lang, "%s: %d -> %d OVR (máximo sem alterar %s %d).%s", "%s: %d -> %d OVR (max without changing %s %d).%s"),
            pos_short(lang, pos), orig, fin, pos_short(lang, reg_pos), pri,
            needs_grade ? T(lang, " Suba o Grau para A para liberar mais OVR.", " Raise Grade to A to unlock more OVR.") : "");
        return b;
    }
    if (code == "iso_limit") {
        char b[240];
        if (needs_grade) {
            std::snprintf(b, sizeof(b),
                T(lang, "Não dá para subir mais sem alterar %s (%d OVR). Suba o Grau para B ou A primeiro.",
                        "Cannot raise further without changing %s (%d OVR). Raise Grade to B or A first."),
                pos_short(lang, reg_pos), pri);
        } else {
            std::snprintf(b, sizeof(b),
                T(lang, "Teto máximo atingido (%d OVR) mantendo %s em %d OVR.",
                        "Maximum possible reached (%d OVR) while keeping %s at %d OVR."),
                orig, pos_short(lang, reg_pos), pri);
        }
        return b;
    }
    if (code == "scout_added") return T(lang, "Adicionado na lista nativa!", "Added to native list!");
    if (code == "scout_dup") return T(lang, "Jogador já está nesta lista.", "Player is already in this list.");
    if (code == "scout_full") return T(lang, "Esta lista está cheia.", "This list is full.");
    if (code == "invalid_json") return T(lang, "Backup JSON inválido.", "Invalid JSON backup.");
    return T(lang, "Operação não pôde ser concluída.", "Operation could not be completed.");
}

} // namespace pes::ui
