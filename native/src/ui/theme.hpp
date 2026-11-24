#pragma once
// Studio Pro Dark Theme, multi-scale native Windows typography, and user settings persistence.

#include "i18n.hpp"
#include "../../third_party/imgui/imgui.h"

#include <string>

namespace pes::ui {

struct AppSettings {
    Lang lang = Lang::PT;
    int currency_idx = 0;   // 0=EUR, 1=BRL, 2=USD, 3=GBP
    int font_idx = 0;       // 0=Segoe UI, 1=Bahnschrift, 2=Calibri, 3=Consolas
    int econ_sync_mode = 1; // 0=Off, 1=All, 2=Market+Release, 3=SalaryOnly
    bool sidebar_collapsed = false;
};

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
    inline constexpr ImVec4 BgTitlebar = ImVec4(0.055f, 0.082f, 0.137f, 1.0f); // #0e1523
    inline constexpr ImVec4 BgCanvas   = ImVec4(0.031f, 0.047f, 0.078f, 1.0f); // #080c14 (--bg)
    inline constexpr ImVec4 BgSidebar  = ImVec4(0.059f, 0.086f, 0.141f, 1.0f); // #0f1624 (--surface)
    inline constexpr ImVec4 BgSurface  = ImVec4(0.059f, 0.086f, 0.141f, 1.0f); // #0f1624 (--surface)
    inline constexpr ImVec4 BgCard     = ImVec4(0.075f, 0.110f, 0.180f, 1.0f); // #131c2e (--card)
    inline constexpr ImVec4 BgCardTop  = ImVec4(0.090f, 0.129f, 0.212f, 1.0f); // #172136 (.card top)
    inline constexpr ImVec4 BgCardBot  = ImVec4(0.063f, 0.094f, 0.157f, 1.0f); // #101828 (.card bottom)
    inline constexpr ImVec4 BgCard2    = ImVec4(0.082f, 0.122f, 0.196f, 1.0f); // #151f32 (--surface-2)
    inline constexpr ImVec4 BgCardHov  = ImVec4(0.102f, 0.149f, 0.239f, 1.0f); // #1a263d (--card-hover)
    inline constexpr ImVec4 BgInput    = ImVec4(0.043f, 0.067f, 0.118f, 1.0f); // #0b111e (.input)
    inline constexpr ImVec4 Border     = ImVec4(0.133f, 0.188f, 0.290f, 1.0f); // #22304a (--border)
    inline constexpr ImVec4 BorderLit  = ImVec4(0.200f, 0.278f, 0.420f, 1.0f); // #33476b (--border-light)
    inline constexpr ImVec4 Text       = ImVec4(0.945f, 0.961f, 0.976f, 1.0f); // #f1f5f9 (--text)
    inline constexpr ImVec4 Subtext    = ImVec4(0.580f, 0.639f, 0.722f, 1.0f); // #94a3b8 (--subtext)
    inline constexpr ImVec4 Muted      = ImVec4(0.392f, 0.455f, 0.545f, 1.0f); // #64748b (--muted)
    inline constexpr ImVec4 Accent     = ImVec4(0.231f, 0.510f, 0.965f, 1.0f); // #3b82f6 (--accent)
    inline constexpr ImVec4 AccentSoft = ImVec4(0.576f, 0.773f, 0.992f, 1.0f); // #93c5fd (active nav/soft blue)
    inline constexpr ImVec4 Green      = ImVec4(0.063f, 0.725f, 0.506f, 1.0f); // #10b981 (--green)
    inline constexpr ImVec4 GreenSoft  = ImVec4(0.204f, 0.827f, 0.600f, 1.0f); // #34d399
    inline constexpr ImVec4 Gold       = ImVec4(0.961f, 0.620f, 0.043f, 1.0f); // #f59e0b (--gold)
    inline constexpr ImVec4 GoldSoft   = ImVec4(0.984f, 0.749f, 0.141f, 1.0f); // #fbbf24
    inline constexpr ImVec4 Red        = ImVec4(0.937f, 0.267f, 0.267f, 1.0f); // #ef4444 (--red)
    inline constexpr ImVec4 RedSoft    = ImVec4(0.973f, 0.443f, 0.443f, 1.0f); // #f87171
    inline constexpr ImVec4 Cyan       = ImVec4(0.024f, 0.714f, 0.831f, 1.0f); // #06b6d4 (--cyan)
    inline constexpr ImVec4 Orange     = ImVec4(0.984f, 0.573f, 0.235f, 1.0f); // #fb923c
    inline constexpr ImVec4 Yellow     = ImVec4(0.980f, 0.800f, 0.082f, 1.0f); // #facc15
}

void apply_studio_theme(float dpi_scale = 1.0f);
void build_font_bank(FontBank& bank, float dpi_scale = 1.0f);

AppSettings load_settings();
void save_settings(const AppSettings& s);

} // namespace pes::ui
