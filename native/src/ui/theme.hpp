#pragma once
// Studio Pro Dark Theme, multi-scale native Windows typography, and user settings persistence.

#include "i18n.hpp"
#include "../../third_party/imgui/imgui.h"

#include <iosfwd>
#include <string>

namespace pes::ui {

struct AppSettings {
    Lang lang = Lang::PT;
    int currency_idx = 0;   // 0=EUR, 1=BRL, 2=USD, 3=GBP
    int font_idx = 0;       // 0=Segoe UI, 1=Bahnschrift, 2=Calibri, 3=Consolas
    int econ_sync_mode = 1; // 0=Off, 1=All, 2=Market+Release, 3=SalaryOnly
    bool sidebar_collapsed = false;
    int theme_idx = 0;      // 0 = Azul (padrao), 1 = Alface (Lettuce)
    int ui_scale_idx = 1;   // indice em kUiScales (90%, 100%, 115%, 130%, 150%)
    int accent_idx = 5;     // 0=Cobalto, 1=Rubro, 2=Esmeralda, 3=Ouro, 4=Ciano, 5=Alface
    bool topmost = false;   // Always-on-Top
    int win_x = 0, win_y = 0, win_w = 0, win_h = 0; // geometria restaurada (0 = padrao)
    bool win_max = false;
    // Servicos opcionais (v14.1)
    bool check_updates = false;      // opt-in: consulta o GitHub uma vez por abertura
    std::string update_dismissed;    // tag da versao cujo aviso foi dispensado (ex.: "v14.2.0")
    int backup_keep_idx = 0;         // indice em backups::kKeepOptions (0 = guardar todos)
    bool action_log = true;          // log local das acoes (Documentos\Lettuce Live Editor\Logs)
    bool first_run_done = false;     // guia de boas-vindas ja exibido
    std::string game_process;        // executavel escolhido a mao no seletor de processo (vazio = detectar sozinho)
};

inline constexpr float kUiScales[5] = {0.90f, 1.00f, 1.15f, 1.30f, 1.50f};
inline constexpr int kAccentCount = 6;
inline constexpr int kThemeCount = 2;

struct FontBank {
    ImFont* regular[4]{};
    ImFont* bold[4]{};
    ImFont* small_reg = nullptr;
    ImFont* small_bold = nullptr;
    ImFont* title_bold = nullptr;
    ImFont* large_num = nullptr;
    int count = 4;
    static constexpr const char* kNames[4] = {"Segoe UI", "Bahnschrift", "Calibri", "Consolas"};
};

namespace col {
    // Exact Python Studio Pro CSS palette (--bg / --surface / --surface-2 / --card / --border)
    inline ImVec4 BgTitlebar = ImVec4(0.055f, 0.082f, 0.137f, 1.0f); // #0e1523
    inline ImVec4 BgCanvas   = ImVec4(0.031f, 0.047f, 0.078f, 1.0f); // #080c14 (--bg)
    inline ImVec4 BgSidebar  = ImVec4(0.059f, 0.086f, 0.141f, 1.0f); // #0f1624 (--surface)
    inline ImVec4 BgSurface  = ImVec4(0.059f, 0.086f, 0.141f, 1.0f); // #0f1624 (--surface)
    inline ImVec4 BgCard     = ImVec4(0.075f, 0.110f, 0.180f, 1.0f); // #131c2e (--card)
    inline ImVec4 BgCardTop  = ImVec4(0.090f, 0.129f, 0.212f, 1.0f); // #172136 (.card top)
    inline ImVec4 BgCardBot  = ImVec4(0.063f, 0.094f, 0.157f, 1.0f); // #101828 (.card bottom)
    inline ImVec4 BgCard2    = ImVec4(0.082f, 0.122f, 0.196f, 1.0f); // #151f32 (--surface-2)
    inline ImVec4 BgCardHov  = ImVec4(0.102f, 0.149f, 0.239f, 1.0f); // #1a263d (--card-hover)
    inline ImVec4 BgInput    = ImVec4(0.043f, 0.067f, 0.118f, 1.0f); // #0b111e (.input)
    inline ImVec4 Border     = ImVec4(0.133f, 0.188f, 0.290f, 1.0f); // #22304a (--border)
    inline ImVec4 BorderLit  = ImVec4(0.200f, 0.278f, 0.420f, 1.0f); // #33476b (--border-light)
    inline ImVec4 Text       = ImVec4(0.945f, 0.961f, 0.976f, 1.0f); // #f1f5f9 (--text)
    inline ImVec4 Subtext    = ImVec4(0.580f, 0.639f, 0.722f, 1.0f); // #94a3b8 (--subtext)
    inline ImVec4 Muted      = ImVec4(0.392f, 0.455f, 0.545f, 1.0f); // #64748b (--muted)
    inline ImVec4 Accent     = ImVec4(0.231f, 0.510f, 0.965f, 1.0f); // #3b82f6 (--accent), mutavel pelo tema de destaque
    inline ImVec4 AccentSoft = ImVec4(0.576f, 0.773f, 0.992f, 1.0f); // #93c5fd (active nav/soft blue)
    inline constexpr ImVec4 Green      = ImVec4(0.063f, 0.725f, 0.506f, 1.0f); // #10b981 (--green)
    inline constexpr ImVec4 GreenSoft  = ImVec4(0.204f, 0.827f, 0.600f, 1.0f); // #34d399
    inline constexpr ImVec4 Gold       = ImVec4(0.961f, 0.620f, 0.043f, 1.0f); // #f59e0b (--gold)
    inline constexpr ImVec4 GoldSoft   = ImVec4(0.984f, 0.749f, 0.141f, 1.0f); // #fbbf24
    inline constexpr ImVec4 Red        = ImVec4(0.937f, 0.267f, 0.267f, 1.0f); // #ef4444 (--red)
    inline constexpr ImVec4 RedSoft    = ImVec4(0.973f, 0.443f, 0.443f, 1.0f); // #f87171
    inline constexpr ImVec4 Cyan       = ImVec4(0.024f, 0.714f, 0.831f, 1.0f); // #06b6d4 (--cyan)
    inline constexpr ImVec4 Orange     = ImVec4(0.984f, 0.573f, 0.235f, 1.0f); // #fb923c
    inline constexpr ImVec4 Yellow     = ImVec4(0.980f, 0.800f, 0.082f, 1.0f); // #facc15
    inline constexpr ImVec4 Sky        = ImVec4(0.490f, 0.780f, 0.980f, 1.0f); // azul-celeste fixo (OVR 80+, defensores, grau C)

    // Derivadas do tema visual (preenchidas por apply_visual_theme)
    inline ImVec4 PopupBg       = ImVec4(0.059f, 0.086f, 0.141f, 0.98f);
    inline ImVec4 ScrollGrab    = ImVec4(0.149f, 0.212f, 0.322f, 0.90f);
    inline ImVec4 ScrollGrabHov = ImVec4(0.220f, 0.310f, 0.471f, 1.0f);
    inline ImVec4 TableHead     = ImVec4(0.055f, 0.086f, 0.153f, 1.0f);
    inline ImVec4 TableLine     = ImVec4(0.118f, 0.176f, 0.282f, 0.55f);
    inline ImVec4 HeroBg        = ImVec4(0.075f, 0.114f, 0.192f, 0.98f);
    inline ImVec4 GlassTop      = ImVec4(0.133f, 0.196f, 0.322f, 1.0f);
    inline ImVec4 GlassBot      = ImVec4(0.055f, 0.082f, 0.141f, 1.0f);
    inline ImVec4 GlassHeroTop  = ImVec4(0.118f, 0.188f, 0.322f, 1.0f);
    inline ImVec4 GlassHeroBot  = ImVec4(0.059f, 0.090f, 0.153f, 1.0f);
    inline ImVec4 GlowA         = ImVec4(0.145f, 0.388f, 0.922f, 1.0f);
    inline ImVec4 GlowB         = ImVec4(0.063f, 0.725f, 0.506f, 1.0f);
    inline ImVec4 Dim           = ImVec4(0.016f, 0.027f, 0.055f, 0.80f);
}

void apply_studio_theme(float dpi_scale = 1.0f);

// Tema visual (paleta + marca). Chame antes de criar a janela e sempre que o usuario trocar o tema.
void apply_visual_theme(int idx);
int current_visual_theme();
const char* brand_title();            // "Lettuce Live Editor" ou "PES 2021 Workbench"
const char* brand_tag();              // etiqueta ao lado do titulo
const char* theme_name(int idx, Lang lang);
int theme_default_accent(int idx);
void theme_preview(int idx, ImVec4 out[4]); // canvas, cartao, destaque, destaque suave
// Define a cor de destaque (chame apply_studio_theme em seguida para atualizar o ImGuiStyle)
void set_accent_theme(int idx);
const char* accent_name(int idx, Lang lang);
ImVec4 accent_swatch(int idx);
void build_font_bank(FontBank& bank, float dpi_scale = 1.0f);

AppSettings load_settings();
void save_settings(const AppSettings& s);
// Versoes sobre streams: usadas pelas duas acima e pelos testes (sem tocar no settings.ini real)
AppSettings parse_settings(std::istream& in);
void write_settings(std::ostream& out, const AppSettings& s);

} // namespace pes::ui
