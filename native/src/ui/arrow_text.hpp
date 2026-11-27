#pragma once
// Texto do tooltip de cada seta de forma: o que ela faz com os atributos durante a partida.
// Funcao pura (sem ImGui) para ser testada. Os valores vem de engine/form_modifiers.hpp.

#include "i18n.hpp"
#include "../engine/form_modifiers.hpp"

#include <cstdio>
#include <string>

namespace pes::ui {

// Nome da seta (0 = vermelha ... 4 = azul)
inline const char* arrow_name(Lang L, int arrow) {
    switch (arrow) {
        case 4: return T(L, "Azul", "Blue");
        case 3: return T(L, "Verde", "Green");
        case 2: return T(L, "Amarela", "Yellow");
        case 1: return T(L, "Laranja", "Orange");
        default: return T(L, "Vermelha", "Red");
    }
}

// Titulo do tooltip: cor e estado de forma
inline const char* arrow_title(Lang L, int arrow) {
    switch (arrow) {
        case 4: return T(L, "Seta azul: forma excelente", "Blue arrow: excellent form");
        case 3: return T(L, "Seta verde: boa forma", "Green arrow: good form");
        case 2: return T(L, "Seta amarela: forma normal", "Yellow arrow: normal form");
        case 1: return T(L, "Seta laranja: forma ruim", "Orange arrow: poor form");
        default: return T(L, "Seta vermelha: forma péssima", "Red arrow: bad form");
    }
}

// Corpo do tooltip: variacao por grupo de atributos
inline std::string arrow_effect_text(Lang L, int arrow) {
    using namespace form;
    if (arrow < 0) arrow = 0;
    if (arrow > 4) arrow = 4;
    std::string out;
    if (arrow == 2) {
        out = T(L, "Forma neutra: durante a partida nenhum atributo muda.",
                   "Neutral form: no attribute changes during a match.");
    } else {
        out = T(L, "Durante a partida, os atributos do jogador ficam assim:",
                   "During a match the player's attributes become:");
        for (int g = 0; g < kGroups; ++g) {
            std::string names;
            for (const auto& sd : data::STAT_DEFS) {
                if (group_of_fid(sd.fid) != g) continue;
                if (!names.empty()) names += ", ";
                names += T(L, sd.pt, sd.en);
            }
            char pct[16];
            std::snprintf(pct, sizeof(pct), "%+d%%", percent(arrow, g));
            out += "\n";
            out += pct;
            out += "  ";
            out += names;
        }
        std::string none;
        for (const auto& sd : data::STAT_DEFS) {
            if (group_of_fid(sd.fid) != -1) continue;
            if (!none.empty()) none += ", ";
            none += T(L, sd.pt, sd.en);
        }
        out += "\n";
        out += T(L, "Sem alteração: ", "Unchanged: ");
        out += none;
    }
    out += "\n";
    out += T(L, "Valores da tabela interna do PES 2021 v1.7.2.0. Valem só durante a partida, multiplicam junto com o efeito do fôlego e não mudam os atributos gravados no atleta.",
                "Values from PES 2021 v1.7.2.0's internal table. They apply only during a match, multiply together with the stamina effect and do not change the attributes saved on the player.");
    return out;
}

} // namespace pes::ui
