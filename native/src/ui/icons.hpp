// Sistema de icones vetoriais do Workbench: grade 24x24, tracos arredondados, desenhados com ImDrawList
// (sem fontes de icone e sem imagens, entao escalam perfeitamente em qualquer DPI).
#pragma once

#include "../../third_party/imgui/imgui.h"

namespace pes::ui {

enum class Icon {
    Dashboard, Squad, Wallet, Jersey, Search, Eye, Stopwatch, Dumbbell,
    Crest, Coins, Banknote, Lock, Undo, History, Refresh, Gear,
    Export, Import, Plus, Minus, Trash, Check, Close, Star,
    Bolt, Cross, Battery, Contract, Copy, Move, Filter, Pulse,
    Info, List, ArrowUp, ArrowDown, Chip, Pin, Sliders,
    Count_
};

// Desenha o icone centrado em `center` dentro de um quadrado `size` x `size`. stroke_px <= 0 usa o tracado automatico.
void draw_icon(ImDrawList* dl, Icon ic, ImVec2 center, float size, ImU32 col, float stroke_px = 0.0f);

// Botao com icone + texto. O texto exibido vai ate o primeiro "##" (o resto vira ID, como no ImGui).
// Retorna true no clique. size.x == 0 calcula a largura exata.
bool icon_button(Icon ic, const char* label, const ImVec4& bg, const ImVec4& border, const ImVec4& text, ImVec2 size = ImVec2(0, 0));
float icon_button_width(const char* label);

bool icon_btn_primary(Icon ic, const char* label, ImVec2 size = ImVec2(0, 0));
bool icon_btn_green(Icon ic, const char* label, ImVec2 size = ImVec2(0, 0));
bool icon_btn_gold(Icon ic, const char* label, ImVec2 size = ImVec2(0, 0));
bool icon_btn_red(Icon ic, const char* label, ImVec2 size = ImVec2(0, 0));
bool icon_btn_ghost(Icon ic, const char* label, ImVec2 size = ImVec2(0, 0));
bool icon_btn_plain(Icon ic, const char* label, ImVec2 size = ImVec2(0, 0)); // fundo de card, texto padrao

// Botao quadrado so com icone (ex.: engrenagem, fechar). `id` precisa ser unico.
bool icon_only_button(Icon ic, const char* id, const ImVec4& bg, const ImVec4& border, const ImVec4& fg, float size);

} // namespace pes::ui
