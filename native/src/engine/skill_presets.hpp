// Presets de habilidades por funcao em campo (curadoria propria, baseada no que cada habilidade faz no jogo).
// Os nomes (campo `name` de data::PLAYER_SKILLS) sao resolvidos em tempo de execucao.
#pragma once

#include <array>
#include <string_view>

namespace pes::presets {

struct SkillPreset {
    const char* pt;
    const char* en;
    const char* desc_pt;
    const char* desc_en;
    std::array<std::string_view, 6> skills; // vazio = sem uso
};

inline constexpr std::array<SkillPreset, 10> kSkillPresets = {{
    {"Finalizador", "Poacher",
     "Centroavante de área: finaliza de primeira, de cabeça e em bolas difíceis, bate pênalti e cobre o goleiro.",
     "Box striker: finishes first time, in the air and on awkward balls, takes penalties and chips the keeper.",
     {"First-time Shot", "Acrobatic Finishing", "Heading", "Penalty Specialist", "Chip shot control", ""}},
    {"Ponta driblador", "Dribbling winger",
     "Ponta ou segundo atacante que vence no 1 contra 1 e cruza ou finaliza em curva.",
     "Winger or second striker who wins 1v1 duels and crosses or shoots with curl.",
     {"Scissors Feint", "Double Touch", "Cross Over Turn", "Pinpoint Crossing", "Outside Curler", ""}},
    {"Meia criativo", "Playmaker",
     "Armador de ataque que acelera o jogo com passes de primeira, enfiadas e jogadas de efeito.",
     "Attacking creator who speeds play up with one-touch passes, through balls and flair.",
     {"One-touch Pass", "Through Passing", "Weighted Pass", "No Look Pass", "Outside Curler", ""}},
    {"Regista", "Deep-lying playmaker",
     "Volante ou meia recuado que distribui jogo com lançamentos e passes na medida e ainda rouba bolas.",
     "Deep midfielder who distributes with lofted and weighted passes and still wins the ball back.",
     {"Low Lofted Pass", "Weighted Pass", "Through Passing", "One-touch Pass", "Interception", ""}},
    {"Volante destruidor", "Ball winner",
     "Meio-campista de combate: antecipa passes, marca de perto e não desiste da dividida.",
     "Combative midfielder: reads passes, marks tightly and never gives up a tackle.",
     {"Interception", "Man Marking", "Fighting Spirit", "One-touch Pass", "", ""}},
    {"Zagueiro", "Centre-back",
     "Defensor que acompanha o atacante, corta passes, afasta bolas perigosas e vai bem no jogo aéreo.",
     "Defender who tracks the striker, cuts passes, clears danger and wins aerial duels.",
     {"Man Marking", "Interception", "Acrobatic Clear", "Heading", "Fighting Spirit", ""}},
    {"Lateral ofensivo", "Attacking full-back",
     "Lateral que apoia pelo corredor, cruza com precisão, volta para marcar e faz laterais longos.",
     "Full-back who overlaps, crosses accurately, tracks back and throws long.",
     {"Pinpoint Crossing", "Track Back", "Low Lofted Pass", "Outside Curler", "Long Throw", ""}},
    {"Goleiro", "Goalkeeper",
     "Goleiro completo: reposições longas com os pés e com as mãos e especialista em pegar pênaltis.",
     "Complete keeper: long distribution with feet and hands and a penalty-saving specialist.",
     {"GK Low Punt", "GK High Punt", "GK Long Throw", "GK Penalty Saver", "", ""}},
    {"Líder", "Leader",
     "Capitão em campo que mantém o nível sob pressão e ainda tira o adversário do sério.",
     "On-pitch captain who holds his level under pressure and gets under opponents' skin.",
     {"Captaincy", "Fighting Spirit", "Gamesmanship", "", "", ""}},
    {"Craque de bola parada", "Set-piece specialist",
     "Cobrador de falta, pênalti e escanteio: chute de longe, folha seca e passe curvo.",
     "Free-kick, penalty and corner taker: long-range shooting, dipping shots and curled passes.",
     {"Long Range Shooting", "Dipping Shot", "Knuckle Shot", "Penalty Specialist", "Outside Curler", ""}},
}};

} // namespace pes::presets
