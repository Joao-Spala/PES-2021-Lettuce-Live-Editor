#pragma once
// Efeito da seta de forma (condicao do atleta) nos atributos DURANTE A PARTIDA.
//
// Origem dos numeros: tabela interna do proprio PES2021.exe (Steam 1.7.2.0, secao de dados, RVA 0x263BE20).
// A rotina do jogo que a usa (RVA ~0x92BC09) monta, para cada partida, um multiplicador por atributo e o aplica
// ao valor gravado: atributo_em_jogo = valor * forma * folego * outros efeitos. O indice na tabela e
// (nivel + 5 * grupo), com nivel 0 = melhor forma, 2 = neutro (a rotina pula o calculo) e 4 = pior.
// O id interno do atributo no jogo e o fid do app + 6.
//
// Nada disso altera o valor gravado no atleta: so o que a partida calcula a cada jogo.

#include <cmath>

namespace pes::form {

inline constexpr int kTiers = 5;    // setas 0..4, o mesmo valor gravado no registro (4 = azul, 2 = amarela, 0 = vermelha)
inline constexpr int kGroups = 3;

// Multiplicadores por grupo e nivel (0 = azul ... 4 = vermelha)
inline constexpr float kMult[kGroups][kTiers] = {
    {1.12f, 1.06f, 1.00f, 0.92f, 0.84f},   // grupo 0: consciencia, goleiro, forca de chute
    {1.09f, 1.05f, 1.00f, 0.94f, 0.88f},   // grupo 1: tecnica, finalizacao, passe, velocidade, salto
    {1.06f, 1.03f, 1.00f, 0.94f, 0.88f},   // grupo 2: contato fisico, equilibrio, resistencia
};

// Grupo do atributo (fid do app) ou -1 quando a forma nao o altera (Posse apertada e Agressividade)
constexpr int group_of_fid(int fid) {
    switch (fid) {
        case 14: case 15: case 16:          // Talento ofensivo, Talento defensivo, Talento de goleiro
        case 28: case 29: case 30: case 31: // Goleiro: pegada, rebatida, reflexo, alcance
        case 36:                            // Forca de chute
            return 0;
        case 17: case 18: case 20: case 21: case 22: case 23: case 24:  // Drible .. Roubo de bola
        case 26: case 27:                   // Bola parada, Curva
        case 33: case 37: case 38:          // Velocidade, Aceleracao, Salto
            return 1;
        case 34: case 35: case 39:          // Contato fisico, Equilibrio, Resistencia
            return 2;
        default:
            return -1;
    }
}

// Multiplicador para a seta (0..4) e o grupo (0..2); fora disso, 1.0
inline float multiplier(int arrow, int group) {
    if (arrow < 0 || arrow >= kTiers || group < 0 || group >= kGroups) return 1.0f;
    return kMult[group][kTiers - 1 - arrow];
}

// Variacao em porcentagem inteira (+12, -16, 0)
inline int percent(int arrow, int group) {
    return static_cast<int>(std::lround((multiplier(arrow, group) - 1.0f) * 100.0f));
}

} // namespace pes::form
