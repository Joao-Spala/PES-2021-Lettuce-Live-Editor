#pragma once
// Assinaturas de bytes que o app usa para achar ganchos e ponteiros da engine do PES 2021.
//
// Regra: nenhum endereco fixo. Cada assinatura precisa ser UNICA em todo o codigo do processo, e `hint_rva` so
// registra onde ela cai no PES 2021 Steam 1.7.2.0 (para o relatorio dizer se o executavel e igual ao de referencia).
// As quatro primeiras e a de treino, partida e fôlego tambem sao as usadas pelas tabelas da comunidade, o que as
// torna conhecidas como unicas em varias versoes. As duas de ponteiro carregam o deslocamento do campo na
// estrutura da carreira (E0 CB 6E 01): se a estrutura mudar, a assinatura nao casa e o app recusa editar.

#include "image_scan.hpp"

namespace pes::sig {

// Captura do atleta destacado (gancho 1): quatro pontos do codigo da interface
inline constexpr SigDef kPlayerA{"player_capture_1", "CB B8 02 00 00 00 0F 1F 40 00 66 0F 1F 84 00 00 00 00 00 0F 10 02 0F 11 01", 0xC6ED2D};
inline constexpr SigDef kPlayerB{"player_capture_2", "48 8B 40 2C 48 89 02", 0x8E24D7};
inline constexpr SigDef kPlayerC{"player_capture_3", "0F B6 80 50 01 00 00 89 87 98 00 00 00", 0xDDCA5C};
inline constexpr SigDef kPlayerD{"player_capture_4", "0F B6 80 50 01 00 00 89 45 67", 0xF27462};
// Multiplicador de XP de treino (gancho 3)
inline constexpr SigDef kTraining{"training_xp", "45 8B C8 83 FA 6C 0F 8D 1E 01 00 00", 0x12E14A0};
// Limite dos Pontos de Foco do BAL (gancho 4)
inline constexpr SigDef kBalCap{"bal_focus_cap", "0F BE 41 0A 2B C2 83 F8 15 77 09 48 63 C3", 0xBBFE71};
// Relogio e HUD da partida (gancho 5)
inline constexpr SigDef kMatchHud{"match_hud", "49 8B F8 44 0F B6 E2 88 55 B7", 0x20F9FD6};
inline constexpr SigDef kMatchClock{"match_clock", "8B 44 24 40 89 44 24 40 F3 0F 10 44 24 40 48 83 C4 30 5B C3", 0xA3C787};
// Folego seletivo em partida (gancho 6)
// Cobre tambem o epilogo da funcao (mov rbx/rdi, add rsp 30h, pop rbp, ret): o gancho refaz esse epilogio na mao
inline constexpr SigDef kStamina{"match_stamina", "6B 45 10 64 48 8B 5C 24 50 48 8B 7C 24 58 99 F7 7D 18 48 83 C4 30 5D C3", 0xA58EAC};
// Ponteiros globais: dono da carreira e gerente de partida
inline constexpr SigDef kCareerGetter{"career_owner", "E8 ?? ?? ?? ?? 48 8B 48 48 48 8D 81 E0 CB 6E 01", 0xBBFEF3};
inline constexpr SigDef kMatchMgr{"match_manager", "48 8B 05 ?? ?? ?? ?? 4C 8B E2 45 33 FF 48 8B B0 70 02 00 00", 0x4242AE};

inline constexpr const SigDef* kAll[] = {
    &kPlayerA, &kPlayerB, &kPlayerC, &kPlayerD, &kTraining, &kBalCap, &kMatchHud, &kMatchClock, &kStamina, &kCareerGetter, &kMatchMgr,
};

} // namespace pes::sig
