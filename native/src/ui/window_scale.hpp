#pragma once
// Piso de tamanho da janela. A escala da interface (Windows x opcao do app) NAO redimensiona a janela: ela cresce sozinha
// e o layout se reorganiza para a area que sobrar (colunas opcionais somem, cartoes empilham, listas rolam). O unico
// limite e o piso abaixo, em pixels logicos (a 100%), multiplicado pela escala efetiva, para o usuario nao encolher a
// janela alem do que os layouts comportam. Funcao pura: o main.cpp so aplica o resultado.

#include <algorithm>
#include <cmath>

namespace pes::ui {

// Tamanho LOGICO (em pixels a 100%) abaixo do qual a janela nao encolhe ao arrastar
inline constexpr float kMinLogicalW = 640.0f;
inline constexpr float kMinLogicalH = 480.0f;

struct WinSize {
    int w = 0;
    int h = 0;
};

inline int round_px(float v) { return static_cast<int>(std::lround(v)); }

// Menor tamanho permitido na escala efetiva `eff`. Nunca passa da area de trabalho (em um monitor pequeno com escala
// alta o piso encosta no limite da tela em vez de ficar impossivel).
inline WinSize min_window_size(float eff, int work_w, int work_h) {
    WinSize m;
    m.w = std::min(round_px(kMinLogicalW * eff), std::max(work_w, 1));
    m.h = std::min(round_px(kMinLogicalH * eff), std::max(work_h, 1));
    return m;
}

} // namespace pes::ui
