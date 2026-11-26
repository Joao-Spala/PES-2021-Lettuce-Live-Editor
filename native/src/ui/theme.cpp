#include "theme.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace pes::ui {

namespace {

std::filesystem::path get_settings_path() {
    wchar_t appdata[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appdata))) {
        std::filesystem::path dir = std::filesystem::path(appdata) / L"PES2021Workbench";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        return dir / L"settings.ini";
    }
    return L"settings.ini";
}

// Geometria salva a mao ou corrompida nao pode estourar a aritmetica de RECT
int clamp_geom(const std::string& v) {
    long x = std::strtol(v.c_str(), nullptr, 10);
    return static_cast<int>(std::clamp(x, -100000L, 100000L));
}

bool file_exists(const char* path) {
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

} // namespace

namespace {
struct Palette {
    ImVec4 bg_title, bg_canvas, bg_sidebar, bg_surface, bg_card, bg_card_top, bg_card_bot, bg_card2, bg_card_hov, bg_input;
    ImVec4 border, border_lit, text, subtext, muted;
    ImVec4 popup, scroll_grab, scroll_grab_hov, table_head, table_line, hero_bg;
    ImVec4 glass_top, glass_bot, glass_hero_top, glass_hero_bot, glow_a, glow_b, dim;
    int default_accent;
    const char* name_pt;
    const char* name_en;
};

const Palette kPalettes[kThemeCount] = {
    // 0: Estudio (azul-marinho, o visual original)
    {ImVec4(0.055f, 0.082f, 0.137f, 1), ImVec4(0.031f, 0.047f, 0.078f, 1), ImVec4(0.059f, 0.086f, 0.141f, 1), ImVec4(0.059f, 0.086f, 0.141f, 1),
     ImVec4(0.075f, 0.110f, 0.180f, 1), ImVec4(0.090f, 0.129f, 0.212f, 1), ImVec4(0.063f, 0.094f, 0.157f, 1), ImVec4(0.082f, 0.122f, 0.196f, 1),
     ImVec4(0.102f, 0.149f, 0.239f, 1), ImVec4(0.043f, 0.067f, 0.118f, 1),
     ImVec4(0.133f, 0.188f, 0.290f, 1), ImVec4(0.200f, 0.278f, 0.420f, 1), ImVec4(0.945f, 0.961f, 0.976f, 1), ImVec4(0.580f, 0.639f, 0.722f, 1), ImVec4(0.392f, 0.455f, 0.545f, 1),
     ImVec4(0.059f, 0.086f, 0.141f, 0.98f), ImVec4(0.149f, 0.212f, 0.322f, 0.90f), ImVec4(0.220f, 0.310f, 0.471f, 1.0f), ImVec4(0.055f, 0.086f, 0.153f, 1.0f),
     ImVec4(0.118f, 0.176f, 0.282f, 0.55f), ImVec4(0.075f, 0.114f, 0.192f, 0.98f),
     ImVec4(0.133f, 0.196f, 0.322f, 1), ImVec4(0.055f, 0.082f, 0.141f, 1), ImVec4(0.118f, 0.188f, 0.322f, 1), ImVec4(0.059f, 0.090f, 0.153f, 1),
     ImVec4(0.145f, 0.388f, 0.922f, 1), ImVec4(0.063f, 0.725f, 0.506f, 1), ImVec4(0.016f, 0.027f, 0.055f, 0.80f),
     0, "Azul", "Blue"},
    // 1: Alface (verde-floresta com folhas de alface, o visual Lettuce Live Editor)
    {ImVec4(0.039f, 0.078f, 0.047f, 1), ImVec4(0.024f, 0.051f, 0.031f, 1), ImVec4(0.043f, 0.086f, 0.055f, 1), ImVec4(0.043f, 0.086f, 0.055f, 1),
     ImVec4(0.059f, 0.114f, 0.075f, 1), ImVec4(0.078f, 0.157f, 0.102f, 1), ImVec4(0.047f, 0.094f, 0.063f, 1), ImVec4(0.071f, 0.145f, 0.102f, 1),
     ImVec4(0.094f, 0.192f, 0.137f, 1), ImVec4(0.031f, 0.071f, 0.043f, 1),
     ImVec4(0.118f, 0.227f, 0.153f, 1), ImVec4(0.176f, 0.353f, 0.235f, 1), ImVec4(0.933f, 0.969f, 0.910f, 1), ImVec4(0.643f, 0.733f, 0.627f, 1), ImVec4(0.500f, 0.610f, 0.510f, 1),
     ImVec4(0.043f, 0.090f, 0.055f, 0.98f), ImVec4(0.160f, 0.300f, 0.190f, 0.90f), ImVec4(0.230f, 0.420f, 0.270f, 1.0f), ImVec4(0.078f, 0.145f, 0.098f, 1.0f),
     ImVec4(0.118f, 0.227f, 0.153f, 0.55f), ImVec4(0.075f, 0.145f, 0.094f, 0.98f),
     ImVec4(0.133f, 0.275f, 0.169f, 1), ImVec4(0.047f, 0.102f, 0.063f, 1), ImVec4(0.118f, 0.267f, 0.157f, 1), ImVec4(0.043f, 0.098f, 0.059f, 1),
     ImVec4(0.520f, 0.800f, 0.090f, 1), ImVec4(0.120f, 0.720f, 0.450f, 1), ImVec4(0.012f, 0.035f, 0.016f, 0.80f),
     5, "Alface", "Lettuce"},
};
int g_visual_theme = 1;
} // namespace

void apply_visual_theme(int idx) {
    g_visual_theme = std::clamp(idx, 0, kThemeCount - 1);
    const Palette& p = kPalettes[g_visual_theme];
    col::BgTitlebar = p.bg_title; col::BgCanvas = p.bg_canvas; col::BgSidebar = p.bg_sidebar; col::BgSurface = p.bg_surface;
    col::BgCard = p.bg_card; col::BgCardTop = p.bg_card_top; col::BgCardBot = p.bg_card_bot; col::BgCard2 = p.bg_card2;
    col::BgCardHov = p.bg_card_hov; col::BgInput = p.bg_input;
    col::Border = p.border; col::BorderLit = p.border_lit; col::Text = p.text; col::Subtext = p.subtext; col::Muted = p.muted;
    col::PopupBg = p.popup; col::ScrollGrab = p.scroll_grab; col::ScrollGrabHov = p.scroll_grab_hov;
    col::TableHead = p.table_head; col::TableLine = p.table_line; col::HeroBg = p.hero_bg;
    col::GlassTop = p.glass_top; col::GlassBot = p.glass_bot; col::GlassHeroTop = p.glass_hero_top; col::GlassHeroBot = p.glass_hero_bot;
    col::GlowA = p.glow_a; col::GlowB = p.glow_b; col::Dim = p.dim;
}
int current_visual_theme() { return g_visual_theme; }
const char* brand_title() { return "Lettuce Live Editor"; }
const char* brand_tag() { return "PES 2021"; }
const char* theme_name(int idx, bool en) {
    const Palette& p = kPalettes[std::clamp(idx, 0, kThemeCount - 1)];
    return en ? p.name_en : p.name_pt;
}
int theme_default_accent(int idx) { return kPalettes[std::clamp(idx, 0, kThemeCount - 1)].default_accent; }

namespace {
struct AccentDef { const char* pt; const char* en; ImVec4 main; ImVec4 soft; };
const AccentDef kAccents[kAccentCount] = {
    {"Cobalto",   "Cobalt",   ImVec4(0.170f, 0.400f, 0.900f, 1.0f), ImVec4(0.576f, 0.773f, 0.992f, 1.0f)},
    {"Rubro",     "Crimson",  ImVec4(0.780f, 0.170f, 0.220f, 1.0f), ImVec4(0.988f, 0.647f, 0.667f, 1.0f)},
    {"Esmeralda", "Emerald",  ImVec4(0.030f, 0.480f, 0.330f, 1.0f), ImVec4(0.431f, 0.906f, 0.718f, 1.0f)},
    {"Ouro",      "Gold",     ImVec4(0.620f, 0.400f, 0.020f, 1.0f), ImVec4(0.992f, 0.878f, 0.455f, 1.0f)},
    {"Ciano",     "Cyan",     ImVec4(0.030f, 0.480f, 0.620f, 1.0f), ImVec4(0.500f, 0.898f, 0.965f, 1.0f)},
    {"Alface",    "Lettuce",  ImVec4(0.250f, 0.460f, 0.060f, 1.0f), ImVec4(0.639f, 0.902f, 0.208f, 1.0f)},
};
} // namespace

void set_accent_theme(int idx) {
    const auto& a = kAccents[std::clamp(idx, 0, kAccentCount - 1)];
    col::Accent = a.main;
    col::AccentSoft = a.soft;
}
const char* accent_name(int idx, bool en) {
    const auto& a = kAccents[std::clamp(idx, 0, kAccentCount - 1)];
    return en ? a.en : a.pt;
}
void theme_preview(int idx, ImVec4 out[4]) {
    const Palette& p = kPalettes[std::clamp(idx, 0, kThemeCount - 1)];
    out[0] = p.bg_canvas;
    out[1] = p.bg_card;
    const auto& a = kAccents[std::clamp(p.default_accent, 0, kAccentCount - 1)];
    out[2] = a.main;
    out[3] = a.soft;
}

ImVec4 accent_swatch(int idx) { return kAccents[std::clamp(idx, 0, kAccentCount - 1)].main; }

void apply_studio_theme(float dpi_scale) {
    float s = std::clamp(dpi_scale, 0.85f, 2.5f);
    ImGuiStyle& st = ImGui::GetStyle();
    st = ImGuiStyle();

    st.WindowPadding = ImVec2(std::round(16.0f * s), std::round(14.0f * s));
    st.FramePadding = ImVec2(std::round(10.0f * s), std::round(6.0f * s));
    st.CellPadding = ImVec2(std::round(10.0f * s), std::round(7.0f * s));
    st.ItemSpacing = ImVec2(std::round(9.0f * s), std::round(8.0f * s));
    st.ItemInnerSpacing = ImVec2(std::round(7.0f * s), std::round(5.0f * s));
    st.ScrollbarSize = std::round(9.0f * s);
    st.GrabMinSize = std::round(12.0f * s);

    st.WindowBorderSize = 0.0f;
    st.ChildBorderSize = 1.0f;
    st.PopupBorderSize = 1.0f;
    st.FrameBorderSize = 1.0f;
    st.TabBorderSize = 0.0f;

    st.WindowRounding = 0.0f;
    st.ChildRounding = std::round(12.0f * s);
    st.FrameRounding = std::round(8.0f * s);
    st.PopupRounding = std::round(10.0f * s);
    st.ScrollbarRounding = std::round(9.0f * s);
    st.GrabRounding = std::round(6.0f * s);
    st.TabRounding = std::round(8.0f * s);
    st.SelectableTextAlign = ImVec2(0.0f, 0.5f);
    st.DisabledAlpha = 0.58f;

    ImVec4* c = st.Colors;
    c[ImGuiCol_Text]                 = col::Text;
    c[ImGuiCol_TextDisabled]         = col::Muted;
    c[ImGuiCol_WindowBg]             = col::BgCanvas;
    c[ImGuiCol_ChildBg]              = col::BgCard;
    c[ImGuiCol_PopupBg]              = col::PopupBg;
    c[ImGuiCol_Border]               = col::Border;
    c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg]              = col::BgInput;
    c[ImGuiCol_FrameBgHovered]       = col::BgCard2;
    c[ImGuiCol_FrameBgActive]        = col::BgCardHov;
    c[ImGuiCol_TitleBg]              = col::BgTitlebar;
    c[ImGuiCol_TitleBgActive]        = col::BgTitlebar;
    c[ImGuiCol_TitleBgCollapsed]     = col::BgTitlebar;
    c[ImGuiCol_MenuBarBg]            = col::BgSidebar;
    c[ImGuiCol_ScrollbarBg]          = ImVec4(col::BgCanvas.x, col::BgCanvas.y, col::BgCanvas.z, 0.35f);
    c[ImGuiCol_ScrollbarGrab]        = col::ScrollGrab;
    c[ImGuiCol_ScrollbarGrabHovered] = col::ScrollGrabHov;
    c[ImGuiCol_ScrollbarGrabActive]  = col::Accent;
    c[ImGuiCol_CheckMark]            = col::AccentSoft;
    c[ImGuiCol_SliderGrab]           = col::AccentSoft;
    c[ImGuiCol_SliderGrabActive]     = col::Accent;
    c[ImGuiCol_Button]               = col::BgCard2;
    c[ImGuiCol_ButtonHovered]        = col::Border;     // #22304a (.btn:hover)
    c[ImGuiCol_ButtonActive]         = col::BorderLit;
    c[ImGuiCol_Header]               = ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.20f);
    c[ImGuiCol_HeaderHovered]        = ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.32f);
    c[ImGuiCol_HeaderActive]         = ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.44f);
    c[ImGuiCol_Separator]            = ImVec4(col::Border.x, col::Border.y, col::Border.z, 0.80f);
    c[ImGuiCol_SeparatorHovered]     = col::BorderLit;
    c[ImGuiCol_SeparatorActive]      = col::Accent;
    c[ImGuiCol_TableBorderStrong]    = col::Border;
    c[ImGuiCol_Tab]                  = col::BgSidebar;
    c[ImGuiCol_TabHovered]           = col::BgCard2;
    c[ImGuiCol_TabSelected]          = col::Accent;
    c[ImGuiCol_TableHeaderBg]        = col::TableHead;
    c[ImGuiCol_TableBorderStrong]    = col::Border;
    c[ImGuiCol_TableBorderLight]     = col::TableLine;
    c[ImGuiCol_TableRowBg]           = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TableRowBgAlt]        = ImVec4(1.0f, 1.0f, 1.0f, 0.018f);
    c[ImGuiCol_TextSelectedBg]       = ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.36f);
    c[ImGuiCol_ModalWindowDimBg]     = col::Dim;
}

void build_font_bank(FontBank& bank, float dpi_scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    static const ImWchar kLatinRanges[] = {
        0x0020, 0x00FF,
        0x0100, 0x017F,
        0x2018, 0x2026,
        0x20AC, 0x20AC,
        0,
    };
    static const ImWchar kSymbolRanges[] = {
        0x2190, 0x2199,
        0x25B2, 0x25BC,
        0x25CF, 0x25CF,
        0x2605, 0x2605,
        0x2713, 0x2714,
        0x2B06, 0x2B07,
        0x27A1, 0x27A1,
        0,
    };

    const char* reg_paths[4] = {
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\bahnschrift.ttf",
        "C:\\Windows\\Fonts\\calibri.ttf",
        "C:\\Windows\\Fonts\\consola.ttf",
    };
    const char* bold_paths[4] = {
        "C:\\Windows\\Fonts\\segoeuib.ttf",
        "C:\\Windows\\Fonts\\bahnschrift.ttf",
        "C:\\Windows\\Fonts\\calibrib.ttf",
        "C:\\Windows\\Fonts\\consolab.ttf",
    };
    const char* sym_path = "C:\\Windows\\Fonts\\seguisym.ttf";
    bool has_sym = file_exists(sym_path);

    const float base_px  = std::round(15.0f * dpi_scale);
    const float small_px = std::round(12.5f * dpi_scale);
    const float title_px = std::round(20.5f * dpi_scale);
    const float big_px   = std::round(25.0f * dpi_scale);

    auto load_pair = [&](const char* path, float sz, bool merge_sym = true) -> ImFont* {
        ImFontConfig cfg;
        cfg.OversampleH = 3;
        cfg.OversampleV = 2;
        cfg.PixelSnapH = true;
        ImFont* f = nullptr;
        if (file_exists(path)) {
            f = io.Fonts->AddFontFromFileTTF(path, sz, &cfg, kLatinRanges);
        }
        if (!f) f = io.Fonts->AddFontDefault();
        if (merge_sym && has_sym) {
            ImFontConfig scfg;
            scfg.MergeMode = true;
            scfg.OversampleH = 2;
            scfg.OversampleV = 2;
            scfg.PixelSnapH = true;
            io.Fonts->AddFontFromFileTTF(sym_path, sz * 0.92f, &scfg, kSymbolRanges);
        }
        return f;
    };

    for (int i = 0; i < 4; ++i) {
        bank.regular[i] = load_pair(reg_paths[i], base_px, true);
        bank.bold[i] = load_pair(bold_paths[i], base_px, true);
    }
    bank.small_reg  = load_pair(reg_paths[0], small_px, true);
    bank.small_bold = load_pair(bold_paths[0], small_px, true);
    bank.title_bold = load_pair(bold_paths[0], title_px, false);
    bank.large_num  = load_pair(bold_paths[0], big_px, false);
    io.Fonts->Build();
}

AppSettings load_settings() {
    AppSettings s;
    std::ifstream f(get_settings_path());
    if (!f) return s;
    std::string line;
    bool has_theme = false;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);
        if (k == "lang") s.lang = (v == "en") ? Lang::EN : Lang::PT;
        else if (k == "currency") s.currency_idx = std::clamp(std::atoi(v.c_str()), 0, 3);
        else if (k == "font") s.font_idx = std::clamp(std::atoi(v.c_str()), 0, 3);
        else if (k == "econ_sync") s.econ_sync_mode = std::clamp(std::atoi(v.c_str()), 0, 3);
        else if (k == "sidebar_collapsed") s.sidebar_collapsed = (v == "1");
        else if (k == "theme") { s.theme_idx = std::clamp(std::atoi(v.c_str()), 0, kThemeCount - 1); has_theme = true; }
        else if (k == "ui_scale") s.ui_scale_idx = std::clamp(std::atoi(v.c_str()), 0, 4);
        else if (k == "accent") s.accent_idx = std::clamp(std::atoi(v.c_str()), 0, kAccentCount - 1);
        else if (k == "topmost") s.topmost = (v == "1");
        else if (k == "win_x") s.win_x = clamp_geom(v);
        else if (k == "win_y") s.win_y = clamp_geom(v);
        else if (k == "win_w") s.win_w = clamp_geom(v);
        else if (k == "win_h") s.win_h = clamp_geom(v);
        else if (k == "win_max") s.win_max = (v == "1");
    }
    // Arquivo anterior aos temas: adota tambem a cor de destaque do tema padrao (identidade completa)
    // (so migra quem ainda usava o destaque padrao antigo: uma cor escolhida de proposito e mantida)
    if (!has_theme && s.accent_idx == 0) s.accent_idx = theme_default_accent(s.theme_idx);
    return s;
}

void save_settings(const AppSettings& s) {
    // Grava em arquivo temporario e troca: um crash no meio nao deixa o settings.ini vazio
    const auto path = get_settings_path();
    auto tmp = path;
    tmp += L".tmp";
    {
        std::ofstream f(tmp, std::ios::trunc);
        if (!f) return;
        f << "lang=" << (s.lang == Lang::EN ? "en" : "pt") << "\n";
        f << "currency=" << s.currency_idx << "\n";
        f << "font=" << s.font_idx << "\n";
        f << "econ_sync=" << s.econ_sync_mode << "\n";
        f << "sidebar_collapsed=" << (s.sidebar_collapsed ? 1 : 0) << "\n";
        f << "theme=" << s.theme_idx << "\n";
        f << "ui_scale=" << s.ui_scale_idx << "\n";
        f << "accent=" << s.accent_idx << "\n";
        f << "topmost=" << (s.topmost ? 1 : 0) << "\n";
        f << "win_x=" << s.win_x << "\n" << "win_y=" << s.win_y << "\n";
        f << "win_w=" << s.win_w << "\n" << "win_h=" << s.win_h << "\n";
        f << "win_max=" << (s.win_max ? 1 : 0) << "\n";
        if (!f.good()) return;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::copy_file(tmp, path, std::filesystem::copy_options::overwrite_existing, ec);
        std::filesystem::remove(tmp, ec);
    }
}

} // namespace pes::ui
