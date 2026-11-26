#pragma once
// Custom Studio Pro native Dear ImGui + ImDrawList GPU vector widgets for PES 2021 Workbench.

#include "theme.hpp"
#include "i18n.hpp"
#include "../engine/pes_engine.hpp"
#include "../../third_party/imgui/imgui.h"

#include <array>
#include <string>
#include <vector>

namespace pes::ui {

ImVec4 ovr_color(int ovr);
ImVec4 pos_color(int pos_id);
ImVec4 stamina_color(int stam);
ImVec4 arrow_color(int arrow);
const char* arrow_glyph(int arrow);

// Lightweight GPU micro-animation state check (true only during brief hover/toggle transitions)
bool widgets_is_animating();

// Ambient broadcast studio radial lighting + subtle pitch watermark on workspace canvas
void draw_workspace_ambient_glow(ImDrawList* dl, ImVec2 origin, ImVec2 size);

// Glowing LED status dot (soft multi-ring bloom when active)
void draw_led_dot(ImDrawList* dl, ImVec2 center, float radius, const ImVec4& col, bool glow = true);

// Crisp GPU vector icons for the 8 sidebar views (0..7)
void draw_nav_icon(ImDrawList* dl, ImVec2 center, float size, int view_idx, ImU32 col);

// Illuminated medallion icon for KPI & section cards (0=Club, 1=Transfer, 2=Salary, 3=Player, 4=Hooks, 5=Bolt)
void draw_kpi_medallion(int kind, float size, const ImVec4& accent);

// Styled button helpers
bool styled_button(const char* label, const ImVec4& bg, const ImVec4& border, const ImVec4& text, ImVec2 size = ImVec2(0, 0));
bool btn_primary(const char* label, ImVec2 size = ImVec2(0, 0));
bool btn_green(const char* label, ImVec2 size = ImVec2(0, 0));
bool btn_gold(const char* label, ImVec2 size = ImVec2(0, 0));
bool btn_red(const char* label, ImVec2 size = ImVec2(0, 0));
bool btn_ghost(const char* label, ImVec2 size = ImVec2(0, 0));

// Modern pill toggle switch (returns true when toggled)
bool toggle_switch(const char* id, const char* label, bool* v);

// Custom frameless window titlebar icon button (0=Min, 1=Max/Restore, 2=Close)
bool draw_titlebar_win_btn(const char* id, int kind, bool is_maximized, float w = 44.0f, float h = 38.0f);

// Vector Brand Emblem (cobalt-to-cyan gradient matching Python .brand-icon)
void draw_brand_emblem(ImVec2 pos, float size, ImFont* bold_font);

// Elevated Studio Card container (multi-layer drop shadow, vertical gradient, specular top highlight)
// accent_top: optional colored top-edge accent bar / glow (nullptr = neutral glass highlight)
void begin_studio_card(const char* id, float min_height = 0.0f, bool hero_gradient = false, const ImVec4* accent_top = nullptr);
void end_studio_card();

// Exact dropdown width calculator so combo labels NEVER truncate
float calc_combo_width(const char* const* items, int count, float min_w = 110.0f, float max_w = 340.0f);

// Inline badge/chip/shield/arrow/segment renderers
void draw_ovr_badge(int ovr, float min_w = 36.0f, float h = 24.0f, ImFont* bold_font = nullptr);
void draw_ovr_shield(int ovr, float size, ImFont* big_font, ImFont* small_bold);
void draw_pos_chip(Lang lang, int pos_id, float min_w = 42.0f, float h = 23.0f, ImFont* bold_font = nullptr);
void draw_arrow_badge(int arrow, float h = 24.0f, bool blink = false);
void draw_pill(const char* text, const ImVec4& fg, const ImVec4& bg, const ImVec4& border, bool compact = false);
void draw_stamina_bar(int stam, float bar_w = 66.0f);
void draw_rating_segments(int val, int max_val, const ImVec4& active_col, float width = 88.0f, float h = 7.0f);

// Interactive GPU controls (smooth micro-animations, zero ugly raw ImGui defaults)
bool draw_rating_segments_interactive(const char* id, int* val, int max_val, const ImVec4& active_col, ImFont* bold_font = nullptr);
bool draw_stamina_slider(const char* id, int* stam, float width, ImFont* bold_font = nullptr);
int  draw_arrow_picker(const char* id, int cur_arrow, Lang lang, float avail_w, ImFont* bold_font = nullptr);
bool draw_mode_card(const char* id, const char* title, const char* subtitle, bool active, float height, ImFont* bold_font, ImFont* small_bold);
bool draw_active_player_pill(int ovr, int pos_id, const char* name, Lang lang, float max_w, float h, ImFont* bold_font);

// Rich dark floating tooltip (360px max wrap width)
void show_rich_tooltip(const char* title, const char* body);
void show_tip_if_hovered(const char* title, const char* body);

// Vector 6-axis hexagonal radar (ATQ/ATK, DRI, PAS, FIS/PHY, DEF, GO/GK)
void draw_radar_6axis(const std::array<int, 48>& stats, Lang lang, float width, ImFont* bold_font, ImFont* small_bold = nullptr);

// Interactive 13-position tactical broadcast mini-pitch (automatic vertical / horizontal layout)
int draw_tactical_pitch(const std::array<PositionEntry, 13>& positions, int highlighted_pos, Lang lang, bool horizontal, float avail_w, ImFont* bold_font);

// High-precision stat row (damped drag slider 9px/step, Shift=18px/step + direct number input + press-and-hold -/+)
struct StatRowState {
    bool dragging = false;
    int drag_fid = -1;
    float drag_start_x = 0.0f;
    int drag_base_val = 40;
    int drag_cur_val = 40;
    int drag_orig_val = 40;

    int hold_fid = -1;
    int hold_delta = 0;
    int hold_cur_val = 40;
    int hold_start_val = 40;
    double hold_next_t = 0.0;
};

bool draw_stat_row(
    const data::StatDef& s,
    int current_val,
    Lang lang,
    StatRowState& st,
    int& out_preview_val,
    int& out_commit_val,
    ImFont* bold_font
);

// Hover-to-pause non-blocking toast queue with animated countdown bar
struct ToastItem {
    std::string msg;
    bool is_err = false;
    double remaining = 6.5;
    double total_dur = 6.5;
    double age = 0.0;
};

class ToastManager {
public:
    void push(const std::string& msg, bool is_err = false, double custom_dur = 0.0);
    bool render(double now_sec, ImFont* bold_font);
    bool has_active() const { return !items_.empty(); }

private:
    std::vector<ToastItem> items_;
    double last_tick_ = 0.0;
};

} // namespace pes::ui
