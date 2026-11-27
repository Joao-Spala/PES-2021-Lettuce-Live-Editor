#include "widgets.hpp"
#include "icons.hpp"
#include "arrow_text.hpp"
#include "../engine/action_log.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <unordered_map>

namespace pes::ui {

namespace {

std::unordered_map<ImGuiID, float> g_anim_map;
bool g_anim_active = false;
int  g_anim_frame  = -1;

float anim_toward(ImGuiID id, float target, float speed = 12.0f) {
    int fc = ImGui::GetFrameCount();
    if (fc != g_anim_frame) {
        g_anim_frame = fc;
        g_anim_active = false;
    }
    auto it = g_anim_map.find(id);
    if (it == g_anim_map.end()) {
        if (g_anim_map.size() > 512) g_anim_map.clear();
        g_anim_map[id] = target;
        return target;
    }
    float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.001f, 0.05f);
    float cur = it->second;
    float diff = target - cur;
    if (std::fabs(diff) < 0.005f) {
        it->second = target;
        return target;
    }
    float next = cur + diff * std::min(1.0f, dt * speed);
    it->second = next;
    g_anim_active = true;
    return next;
}

// Sinaliza animacao ativa neste quadro (sincroniza o contador de quadros antes de marcar)
void mark_animating() {
    int fc = ImGui::GetFrameCount();
    if (fc != g_anim_frame) { g_anim_frame = fc; g_anim_active = false; }
    g_anim_active = true;
}

ImVec4 lerp_col(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

ImU32 to_u32(const ImVec4& c, float alpha_mul = 1.0f) {
    return ImGui::GetColorU32(ImVec4(c.x, c.y, c.z, c.w * alpha_mul));
}

// Draw a multi-layer soft rounded drop shadow into `dl`
void draw_soft_shadow(ImDrawList* dl, ImVec2 rmin, ImVec2 rmax, float rounding, float alpha_scale = 1.0f) {
    const float off[3] = {2.0f, 5.0f, 9.0f};
    const int alp[3]   = {44, 28, 14};
    for (int i = 2; i >= 0; --i) {
        float o = off[i];
        int a = static_cast<int>(alp[i] * alpha_scale);
        dl->AddRectFilled(
            ImVec2(rmin.x - o * 0.45f, rmin.y + o * 0.35f),
            ImVec2(rmax.x + o * 0.45f, rmax.y + o),
            IM_COL32(0, 0, 0, a),
            rounding + o * 0.4f
        );
    }
}

// Draw a silky-smooth radial glow (zero visible ring banding via 24 quadratic-falloff passes)
void draw_radial_glow(ImDrawList* dl, ImVec2 center, float radius, const ImVec4& col, float peak_alpha) {
    constexpr int kRings = 24;
    for (int i = kRings; i >= 1; --i) {
        float frac = static_cast<float>(i) / static_cast<float>(kRings);
        float t = 1.0f - frac;
        float a = peak_alpha * (0.15f + 0.85f * t * t) / static_cast<float>(kRings) * 2.15f;
        if (a > 0.001f) {
            dl->AddCircleFilled(center, radius * frac, to_u32(col, a), 64);
        }
    }
}

// Draw a crisp directional arrow vector inside a bounding center/radius
void draw_arrow_vector(ImDrawList* dl, ImVec2 c, float r, int arrow, ImU32 col) {
    // 4=Up, 3=DiagUpRight, 2=Right, 1=DiagDownRight, 0=Down
    static constexpr float kAngles[5] = {
        3.14159265f * 0.5f,   // 0: Down
        3.14159265f * 0.25f,  // 1: Diag Down-Right
        0.0f,                 // 2: Right
       -3.14159265f * 0.25f,  // 3: Diag Up-Right
       -3.14159265f * 0.5f,   // 4: Up
    };
    int idx = std::clamp(arrow, 0, 4);
    float ang = kAngles[idx];
    float ca = std::cos(ang), sa = std::sin(ang);
    float px = -sa, py = ca; // perpendicular

    ImVec2 tip(c.x + ca * r, c.y + sa * r);
    ImVec2 tail(c.x - ca * r * 0.85f, c.y - sa * r * 0.85f);
    ImVec2 wing1(tip.x - ca * r * 0.72f + px * r * 0.58f, tip.y - sa * r * 0.72f + py * r * 0.58f);
    ImVec2 wing2(tip.x - ca * r * 0.72f - px * r * 0.58f, tip.y - sa * r * 0.72f - py * r * 0.58f);
    ImVec2 neck(tip.x - ca * r * 0.45f, tip.y - sa * r * 0.45f);

    float sw = std::max(1.8f, r * 0.32f);
    dl->AddLine(tail, neck, col, sw);
    dl->AddTriangleFilled(tip, wing1, wing2, col);
}

} // namespace

bool widgets_is_animating() {
    return g_anim_active && (g_anim_frame == ImGui::GetFrameCount());
}

ImVec4 ovr_color(int ovr) {
    if (ovr >= 90) return col::GreenSoft;
    if (ovr >= 80) return col::Sky;
    if (ovr >= 70) return col::GoldSoft;
    return ImVec4(col::Subtext.x * 0.55f + col::Text.x * 0.45f, col::Subtext.y * 0.55f + col::Text.y * 0.45f, col::Subtext.z * 0.55f + col::Text.z * 0.45f, 1.0f);
}

ImVec4 pos_color(int pos_id) {
    if (pos_id == 0) return col::GoldSoft;
    if (pos_id >= 1 && pos_id <= 3) return col::Sky;
    if (pos_id >= 4 && pos_id <= 8) return col::GreenSoft;
    return col::RedSoft;
}

ImVec4 stamina_color(int stam) {
    if (stam >= 75) return col::Green;
    if (stam >= 45) return col::Gold;
    return col::Red;
}

ImVec4 arrow_color(int arrow) {
    switch (arrow) {
        case 4: return col::Cyan;
        case 3: return col::GreenSoft;
        case 2: return col::Yellow;
        case 1: return col::Orange;
        default: return col::RedSoft;
    }
}

const char* arrow_glyph(int arrow) {
    switch (arrow) {
        case 4: return "↑";
        case 3: return "↗";
        case 2: return "→";
        case 1: return "↘";
        default: return "↓";
    }
}

// Folha de alface: contorno ondulado simetrico, nervura central e nervuras laterais.
// base = ponto de fixacao, angle em radianos (0 = para cima), fill 0 = so contorno.
void draw_leaf(ImDrawList* dl, ImVec2 base, float angle, float len, float wid, ImU32 fill, ImU32 line, float thick) {
    constexpr int kN = 14;
    const float ca = std::cos(angle), sa = std::sin(angle);
    auto place = [&](float lx, float ly) { // lx lateral, ly ao longo da folha (positivo = em direcao a ponta)
        float x = lx, y = -ly;
        return ImVec2(base.x + x * ca - y * sa, base.y + x * sa + y * ca);
    };
    auto half_w = [&](float t) {
        float shape = std::sin(3.14159265f * std::pow(t, 0.9f));
        float ruffle = 1.0f + 0.10f * std::sin(t * 19.0f) * (0.4f + 0.6f * t);
        return wid * std::pow(std::max(shape, 0.0f), 0.6f) * ruffle;
    };
    ImVec2 pts[2 * kN + 2];
    int n = 0;
    for (int i = 0; i <= kN; ++i) { float t = static_cast<float>(i) / kN; pts[n++] = place(-half_w(t), len * t); }
    for (int i = kN; i >= 0; --i) { float t = static_cast<float>(i) / kN; pts[n++] = place(half_w(t), len * t); }
    if (fill) dl->AddConvexPolyFilled(pts, n, fill);
    dl->AddPolyline(pts, n, line, ImDrawFlags_Closed, thick);
    dl->AddLine(place(0, len * 0.04f), place(0, len * 0.92f), line, thick);
    for (int i = 0; i < 3; ++i) {
        float t = 0.28f + 0.2f * i;
        float w = half_w(std::min(t + 0.10f, 0.98f)) * 0.78f;
        dl->AddLine(place(0, len * t), place(-w, len * (t + 0.13f)), line, thick * 0.85f);
        dl->AddLine(place(0, len * t), place(w, len * (t + 0.13f)), line, thick * 0.85f);
    }
}

// Mistura linear de duas cores ImU32 (t = 0 -> a, t = 1 -> b)
ImU32 lerp_u32(ImU32 a, ImU32 b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [&](int shift) {
        float va = static_cast<float>((a >> shift) & 0xFF), vb = static_cast<float>((b >> shift) & 0xFF);
        return static_cast<ImU32>(va + (vb - va) * t + 0.5f) & 0xFF;
    };
    return (ch(IM_COL32_R_SHIFT) << IM_COL32_R_SHIFT) | (ch(IM_COL32_G_SHIFT) << IM_COL32_G_SHIFT) |
           (ch(IM_COL32_B_SHIFT) << IM_COL32_B_SHIFT) | (ch(IM_COL32_A_SHIFT) << IM_COL32_A_SHIFT);
}

// Folha do logo: degrade vertical (c_top em cima, c_bot embaixo), sombra projetada, brilho fino de borda e nervuras sutis.
void draw_leaf_lit(ImDrawList* dl, ImVec2 base, float angle, float len, float wid, ImU32 c_bot, ImU32 c_top, float size, bool veins) {
    constexpr int kN = 16;
    const float ca = std::cos(angle), sa = std::sin(angle);
    auto place = [&](float lx, float ly) {
        float x = lx, y = -ly;
        return ImVec2(base.x + x * ca - y * sa, base.y + x * sa + y * ca);
    };
    auto half_w = [&](float t) {
        float shape = std::sin(3.14159265f * std::pow(t, 0.9f));
        float ruffle = 1.0f + 0.10f * std::sin(t * 19.0f) * (0.4f + 0.6f * t);
        return wid * std::pow(std::max(shape, 0.0f), 0.6f) * ruffle;
    };
    ImVec2 pts[2 * kN + 2];
    int n = 0;
    for (int i = 0; i <= kN; ++i) { float t = static_cast<float>(i) / kN; pts[n++] = place(-half_w(t), len * t); }
    for (int i = kN; i >= 0; --i) { float t = static_cast<float>(i) / kN; pts[n++] = place(half_w(t), len * t); }

    // sombra suave sobre as folhas de tras
    ImVec2 sh[2 * kN + 2];
    const float off = size * 0.016f;
    for (int i = 0; i < n; ++i) sh[i] = ImVec2(pts[i].x, pts[i].y + off);
    dl->AddConvexPolyFilled(sh, n, IM_COL32(0, 20, 8, 80));

    // corpo: leque de triangulos a partir do centroide, cor por altura
    float min_y = pts[0].y, max_y = pts[0].y;
    ImVec2 cen(0, 0);
    for (int i = 0; i < n; ++i) { min_y = std::min(min_y, pts[i].y); max_y = std::max(max_y, pts[i].y); cen.x += pts[i].x; cen.y += pts[i].y; }
    cen.x /= n; cen.y /= n;
    auto col_at = [&](float y) { return lerp_u32(c_bot, c_top, (max_y > min_y) ? (max_y - y) / (max_y - min_y) : 0.0f); };
    const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
    dl->PrimReserve(n * 3, n * 3);
    for (int i = 0; i < n; ++i) {
        const ImVec2 a = pts[i], b = pts[(i + 1) % n];
        const ImDrawIdx vi = static_cast<ImDrawIdx>(dl->_VtxCurrentIdx);
        dl->PrimWriteVtx(cen, uv, col_at(cen.y));
        dl->PrimWriteVtx(a, uv, col_at(a.y));
        dl->PrimWriteVtx(b, uv, col_at(b.y));
        dl->PrimWriteIdx(vi); dl->PrimWriteIdx(static_cast<ImDrawIdx>(vi + 1)); dl->PrimWriteIdx(static_cast<ImDrawIdx>(vi + 2));
    }
    if (veins) {
        const float vw = std::max(1.0f, size * 0.013f);
        dl->AddLine(place(0, len * 0.05f), place(0, len * 0.90f), IM_COL32(8, 56, 26, 95), vw);
        for (int i = 0; i < 3; ++i) {
            float t = 0.26f + 0.19f * i;
            float w = half_w(std::min(t + 0.10f, 0.98f)) * 0.74f;
            dl->AddLine(place(0, len * t), place(-w, len * (t + 0.12f)), IM_COL32(8, 56, 26, 70), vw);
            dl->AddLine(place(0, len * t), place(w, len * (t + 0.12f)), IM_COL32(8, 56, 26, 70), vw);
        }
    }
    dl->AddPolyline(pts, n, IM_COL32(255, 255, 255, 70), ImDrawFlags_Closed, std::max(1.0f, size * 0.012f));
}

void draw_workspace_ambient_glow(ImDrawList* dl, ImVec2 origin, ImVec2 size) {
    // Match Python .app-shell ambient radial stadium lights:
    // radial-gradient(ellipse 80% 50% at 65% -10%, rgba(37, 99, 235, 0.14))
    // radial-gradient(ellipse 50% 40% at 15% 110%, rgba(16, 185, 129, 0.08))
    float r_blue = std::max(size.x, size.y) * 0.56f;
    draw_radial_glow(dl, ImVec2(origin.x + size.x * 0.68f, origin.y - size.y * 0.06f),
                     r_blue, col::GlowA, 0.16f);

    float r_green = std::max(size.x, size.y) * 0.42f;
    draw_radial_glow(dl, ImVec2(origin.x + size.x * 0.14f, origin.y + size.y * 1.04f),
                     r_green, col::GlowB, 0.095f);

    // Ultra-subtle broadcast tactical pitch watermark arc on far-right canvas
    if (size.x > 640.0f && size.y > 420.0f) {
        float cx = origin.x + size.x * 0.92f;
        float cy = origin.y + size.y * 0.50f;
        float cr = std::min(size.x, size.y) * 0.24f;
            // Marca d'agua: cabeca de alface (tres folhas com nervuras) bem discreta
            ImU32 wm = to_u32(col::AccentSoft, 0.045f);
            float base_y = cy + cr * 1.05f;
            draw_leaf(dl, ImVec2(cx, base_y), -0.66f, cr * 1.55f, cr * 0.62f, 0, wm, 1.3f);
            draw_leaf(dl, ImVec2(cx, base_y),  0.66f, cr * 1.55f, cr * 0.62f, 0, wm, 1.3f);
            draw_leaf(dl, ImVec2(cx, base_y),  0.00f, cr * 1.95f, cr * 0.74f, 0, wm, 1.3f);
    }
}

void draw_led_dot(ImDrawList* dl, ImVec2 center, float radius, const ImVec4& col, bool glow) {
    if (glow) {
        dl->AddCircleFilled(center, radius * 2.55f, to_u32(col, 0.14f), 20);
        dl->AddCircleFilled(center, radius * 1.70f, to_u32(col, 0.30f), 20);
    }
    dl->AddCircleFilled(center, radius, to_u32(col, 1.0f), 16);
    if (glow) {
        dl->AddCircleFilled(ImVec2(center.x - radius * 0.25f, center.y - radius * 0.25f),
                            radius * 0.36f, IM_COL32(255, 255, 255, 170), 10);
    }
}

void draw_nav_icon(ImDrawList* dl, ImVec2 c, float s, int view_idx, ImU32 col) {
    static const Icon kNav[8] = {Icon::Dashboard, Icon::Squad, Icon::Wallet, Icon::Jersey, Icon::Search, Icon::Eye, Icon::Stopwatch, Icon::Dumbbell};
    draw_icon(dl, kNav[std::clamp(view_idx, 0, 7)], c, s, col);
}

void draw_kpi_medallion(int kind, float size, const ImVec4& accent) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(size, size));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p2(p.x + size, p.y + size);
    float rnd = size * 0.28f;
    ImVec2 c(p.x + size * 0.5f, p.y + size * 0.5f);

    // Subtle colored aura + tinted medallion body + specular top highlight
    dl->AddRectFilled(p, p2, to_u32(accent, 0.15f), rnd);
    dl->AddRectFilled(ImVec2(p.x + 1, p.y + 1), ImVec2(p2.x - 1, p.y + size * 0.48f),
                      IM_COL32(255, 255, 255, 14), rnd, ImDrawFlags_RoundCornersTop);
    dl->AddRect(p, p2, to_u32(accent, 0.42f), rnd, 0, 1.1f);

    // 0 clube, 1 orcamento, 2 salarios, 3 atleta, 4 recursos do motor, 5 comandante, 6 teto salarial, 7 folha ativa
    static const Icon kMed[8] = {Icon::Crest, Icon::Coins, Icon::Banknote, Icon::Jersey, Icon::Pulse, Icon::Bolt, Icon::Lock, Icon::Contract};
    draw_icon(dl, kMed[std::clamp(kind, 0, 7)], c, size * 0.60f, to_u32(accent));
}

bool styled_button(const char* label, const ImVec4& bg, const ImVec4& border, const ImVec4& text, ImVec2 size) {
    ImVec4 hov(
        std::min(1.0f, bg.x + 0.055f),
        std::min(1.0f, bg.y + 0.065f),
        std::min(1.0f, bg.z + 0.085f),
        bg.w > 0.01f ? std::min(1.0f, bg.w + 0.14f) : 0.18f
    );
    ImVec4 act(
        std::min(1.0f, bg.x + 0.09f),
        std::min(1.0f, bg.y + 0.11f),
        std::min(1.0f, bg.z + 0.14f),
        bg.w > 0.01f ? std::min(1.0f, bg.w + 0.24f) : 0.28f
    );
    ImGui::PushStyleColor(ImGuiCol_Button, bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hov);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, act);
    ImGui::PushStyleColor(ImGuiCol_Border, border);
    ImGui::PushStyleColor(ImGuiCol_Text, text);
    bool clicked = ImGui::Button(label, size);
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    ImGui::PopStyleColor(5);
    return clicked;
}

bool btn_primary(const char* label, ImVec2 size) {
    return styled_button(label, col::Accent, ImVec4(col::AccentSoft.x, col::AccentSoft.y, col::AccentSoft.z, 0.72f), ImVec4(1, 1, 1, 1), size);
}

bool btn_green(const char* label, ImVec2 size) {
    return styled_button(label, ImVec4(0.063f, 0.725f, 0.506f, 0.16f), ImVec4(0.063f, 0.725f, 0.506f, 0.48f), col::GreenSoft, size);
}

bool btn_gold(const char* label, ImVec2 size) {
    return styled_button(label, ImVec4(0.961f, 0.620f, 0.043f, 0.16f), ImVec4(0.961f, 0.620f, 0.043f, 0.48f), col::GoldSoft, size);
}

bool btn_red(const char* label, ImVec2 size) {
    return styled_button(label, ImVec4(0.937f, 0.267f, 0.267f, 0.18f), ImVec4(0.937f, 0.267f, 0.267f, 0.52f), col::RedSoft, size);
}

bool btn_ghost(const char* label, ImVec2 size) {
    return styled_button(label, ImVec4(0, 0, 0, 0), ImVec4(0, 0, 0, 0), col::Subtext, size);
}

bool toggle_switch(const char* id, const char* label, bool* v) {
    ImGui::PushID(id);
    const float h = std::round(std::max(18.0f, ImGui::GetFontSize() * 1.24f));
    const float w = std::round(h * 1.84f);
    const float radius = h * 0.5f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 txt_sz = (label && label[0]) ? ImGui::CalcTextSize(label) : ImVec2(0, 0);
    float total_w = w + (txt_sz.x > 0.0f ? (8.0f + txt_sz.x) : 0.0f);
    float total_h = std::max(h, txt_sz.y + 4.0f);

    bool clicked = ImGui::InvisibleButton("##sw", ImVec2(total_w, total_h));
    if (clicked && v) *v = !(*v);
    bool hov = ImGui::IsItemHovered();
    if (hov) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    bool on = v ? *v : false;
    ImGuiID sw_id = ImGui::GetItemID();
    float t_on = anim_toward(sw_id, on ? 1.0f : 0.0f, 16.0f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float ty = p.y + (total_h - h) * 0.5f;
    ImVec4 off_trk = hov ? col::BgCard2 : col::BgInput;
    ImVec4 trk_col = lerp_col(off_trk, col::Accent, t_on);
    ImVec4 brd_col = lerp_col(hov ? col::BorderLit : col::Border, col::AccentSoft, t_on);
    if (t_on > 0.05f) {
        dl->AddRectFilled(ImVec2(p.x - 1.5f, ty - 1.5f), ImVec2(p.x + w + 1.5f, ty + h + 1.5f),
                          to_u32(col::Accent, 0.24f * t_on), radius + 1.5f);
    }
    dl->AddRectFilled(ImVec2(p.x, ty), ImVec2(p.x + w, ty + h), to_u32(trk_col), radius);
    dl->AddRect(ImVec2(p.x, ty), ImVec2(p.x + w, ty + h), to_u32(brd_col), radius, 0, 1.0f);

    float kx = (p.x + radius) + t_on * (w - radius * 2.0f);
    float ky = ty + radius;
    float kr = radius - 3.0f + t_on * 1.0f;
    ImVec4 kcol = lerp_col(hov ? col::Text : col::Subtext, ImVec4(1, 1, 1, 1), t_on);
    dl->AddCircleFilled(ImVec2(kx, ky), kr, to_u32(kcol), 16);

    if (label && label[0]) {
        dl->AddText(ImVec2(p.x + w + 8.0f, p.y + (total_h - txt_sz.y) * 0.5f),
                    to_u32(on ? col::Text : col::Subtext), label);
    }
    ImGui::PopID();
    return clicked;
}

bool draw_titlebar_win_btn(const char* id, int kind, bool is_maximized, float w, float h) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton(id, ImVec2(w, h));
    bool hov = ImGui::IsItemHovered();
    bool held = ImGui::IsItemActive();
    if (hov) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (hov || held) {
        ImU32 bg = (kind == 2)
            ? (held ? IM_COL32(185, 28, 28, 255) : IM_COL32(220, 38, 38, 235))
            : to_u32(col::BgCardHov, held ? 1.0f : 0.88f);
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), bg, 0.0f);
    }

    ImU32 fg = (kind == 2 && hov) ? IM_COL32(255, 255, 255, 255)
                                  : (hov ? to_u32(col::Text) : to_u32(col::Subtext));
    float cx = p.x + w * 0.5f;
    float cy = p.y + h * 0.5f;
    if (kind == 0) {
        dl->AddLine(ImVec2(cx - 5.5f, cy + 1.0f), ImVec2(cx + 5.5f, cy + 1.0f), fg, 1.4f);
    } else if (kind == 1) {
        if (is_maximized) {
            dl->AddRect(ImVec2(cx - 4.5f, cy - 2.5f), ImVec2(cx + 3.0f, cy + 5.0f), fg, 1.0f, 0, 1.3f);
            dl->AddLine(ImVec2(cx - 2.5f, cy - 4.5f), ImVec2(cx + 5.0f, cy - 4.5f), fg, 1.3f);
            dl->AddLine(ImVec2(cx + 5.0f, cy - 4.5f), ImVec2(cx + 5.0f, cy + 3.0f), fg, 1.3f);
        } else {
            dl->AddRect(ImVec2(cx - 5.0f, cy - 4.5f), ImVec2(cx + 5.0f, cy + 4.5f), fg, 1.2f, 0, 1.3f);
        }
    } else {
        dl->AddLine(ImVec2(cx - 4.5f, cy - 4.5f), ImVec2(cx + 4.5f, cy + 4.5f), fg, 1.5f);
        dl->AddLine(ImVec2(cx + 4.5f, cy - 4.5f), ImVec2(cx - 4.5f, cy + 4.5f), fg, 1.5f);
    }
    return clicked;
}

namespace {
ImTextureID g_logo_tex = static_cast<ImTextureID>(0);
}
void set_logo_texture(ImTextureID id) { g_logo_tex = id; }

void draw_brand_emblem(ImVec2 pos, float size, ImFont* /*bold_font*/) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p2(pos.x + size, pos.y + size);
    if (g_logo_tex != static_cast<ImTextureID>(0)) {
        // O logo vem da mesma imagem do icone do executavel (render em alta qualidade, com mipmaps)
        dl->AddImage(g_logo_tex, pos, p2);
        return;
    }
    // Logo Lettuce Live Editor: medalhao verde-floresta, cabeca de alface em camadas e indicador "ao vivo"
    const float sr = size * 0.26f;
    dl->AddRectFilled(ImVec2(pos.x, pos.y + size * 0.014f), ImVec2(p2.x, p2.y + size * 0.014f), IM_COL32(0, 0, 0, 70), sr + 1.0f);
    dl->AddRectFilled(pos, p2, IM_COL32(14, 66, 34, 255), sr);
    {   // degrade diagonal dentro do medalhao (o retangulo fica recuado para nao vazar pelos cantos arredondados)
        const float d = sr * 0.30f;
        dl->AddRectFilledMultiColor(ImVec2(pos.x + d, pos.y + d), ImVec2(p2.x - d, p2.y - d),
                                    IM_COL32(36, 124, 62, 150), IM_COL32(20, 90, 44, 40), IM_COL32(0, 16, 8, 150), IM_COL32(8, 50, 26, 40));
    }
    dl->PushClipRect(pos, p2, true);
    draw_radial_glow(dl, ImVec2(pos.x + size * 0.5f, pos.y + size * 0.52f), size * 0.46f, ImVec4(0.64f, 0.90f, 0.21f, 1.0f), 0.32f);
    dl->PopClipRect();
    dl->AddRectFilled(ImVec2(pos.x + 1.0f, pos.y + 1.0f), ImVec2(p2.x - 1.0f, pos.y + size * 0.42f), IM_COL32(255, 255, 255, 22), sr, ImDrawFlags_RoundCornersTop);
    dl->AddRect(pos, p2, IM_COL32(190, 245, 100, 120), sr, 0, std::max(1.0f, size * 0.022f));
    const float cx = pos.x + size * 0.5f;
    const float by = pos.y + size * 0.88f;
    draw_leaf_lit(dl, ImVec2(cx, by), -1.05f, size * 0.50f, size * 0.17f, IM_COL32(16, 70, 36, 255), IM_COL32(40, 116, 56, 255), size, size >= 36.0f);
    draw_leaf_lit(dl, ImVec2(cx, by),  1.05f, size * 0.50f, size * 0.17f, IM_COL32(16, 70, 36, 255), IM_COL32(40, 116, 56, 255), size, size >= 36.0f);
    draw_leaf_lit(dl, ImVec2(cx, by), -0.62f, size * 0.66f, size * 0.21f, IM_COL32(30, 104, 46, 255), IM_COL32(86, 170, 58, 255), size, size >= 36.0f);
    draw_leaf_lit(dl, ImVec2(cx, by),  0.62f, size * 0.66f, size * 0.21f, IM_COL32(30, 104, 46, 255), IM_COL32(86, 170, 58, 255), size, size >= 36.0f);
    draw_leaf_lit(dl, ImVec2(cx, by),  0.00f, size * 0.76f, size * 0.27f, IM_COL32(56, 136, 52, 255), IM_COL32(132, 206, 72, 255), size, size >= 28.0f);
    draw_leaf_lit(dl, ImVec2(cx, by),  0.00f, size * 0.62f, size * 0.22f, IM_COL32(120, 200, 62, 255), IM_COL32(210, 250, 136, 255), size, size >= 22.0f);
    // base cortada da alface
    dl->AddEllipseFilled(ImVec2(cx, by), ImVec2(size * 0.12f, size * 0.052f), IM_COL32(232, 248, 196, 255), 0.0f, 20);
    if (size >= 20.0f) {
        const ImVec2 dot(pos.x + size * 0.80f, pos.y + size * 0.21f);
        dl->AddCircleFilled(dot, size * 0.105f, IM_COL32(163, 230, 53, 70), 20);
        dl->AddCircle(dot, size * 0.075f, IM_COL32(190, 245, 100, 200), 20, std::max(1.0f, size * 0.014f));
        dl->AddCircleFilled(dot, size * 0.046f, IM_COL32(214, 255, 110, 255), 16);
    }
}

void begin_studio_card(const char* id, float min_height, bool hero_gradient, const ImVec4* accent_top) {
    float sc = std::clamp(ImGui::GetFontSize() / 15.0f, 0.85f, 2.5f);
    float rnd = std::round((hero_gradient ? 14.0f : 12.0f) * sc);
    ImVec2 pad(std::round(18.0f * sc), std::round(15.0f * sc));

    // Paint subtle soft drop shadow into PARENT drawlist right behind this card
    ImDrawList* parent_dl = ImGui::GetWindowDrawList();
    ImVec2 card_pos = ImGui::GetCursorScreenPos();
    float est_w = ImGui::GetContentRegionAvail().x;
    if (min_height > 0.0f && est_w > 10.0f) {
        draw_soft_shadow(parent_dl, card_pos, ImVec2(card_pos.x + est_w, card_pos.y + min_height), rnd, hero_gradient ? 1.15f : 0.85f);
    }

    ImVec4 bg_base = hero_gradient ? col::HeroBg : col::BgCard;
    ImVec4 brd_col = hero_gradient ? col::BorderLit : (accent_top ? ImVec4(accent_top->x, accent_top->y, accent_top->z, 0.40f) : col::Border);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_base);
    ImGui::PushStyleColor(ImGuiCol_Border, brd_col);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, pad);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, rnd);
    ImGuiChildFlags flags = ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding;
    if (min_height <= 0.0f) flags |= ImGuiChildFlags_AutoResizeY;
    ImGui::BeginChild(id, ImVec2(0.0f, min_height), flags);

    // Rich internal gradient sheen + specular glass top reflection (matches Python .card / .player-hero)
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wmin = ImGui::GetWindowPos();
    float ww = ImGui::GetWindowWidth();
    float wh = ImGui::GetWindowHeight();
    ImVec2 wmax(wmin.x + ww, wmin.y + wh);

    // Paint shadow behind AutoResizeY cards too (using actual resolved frame height in parent_dl!)
    if (min_height <= 0.0f && wh > 12.0f && ww > 12.0f) {
        draw_soft_shadow(parent_dl, wmin, wmax, rnd, hero_gradient ? 1.15f : 0.80f);
    }

    // Silky-smooth rounded vertical card gradient with ZERO mid-card seam and ZERO corner bleed
    if (wh > rnd * 2.0f + 4.0f) {
        if (hero_gradient) {
            ImU32 top_c = to_u32(col::GlassHeroTop, 0.51f);
            ImU32 bot_c = to_u32(col::GlassHeroBot, 0.0f);
            dl->AddRectFilled(ImVec2(wmin.x + 1.0f, wmin.y + 1.0f), ImVec2(wmax.x - 1.0f, wmin.y + rnd),
                              top_c, rnd, ImDrawFlags_RoundCornersTop);
            dl->AddRectFilledMultiColor(
                ImVec2(wmin.x + 1.0f, wmin.y + rnd), ImVec2(wmax.x - 1.0f, wmax.y - rnd),
                top_c, top_c, bot_c, bot_c);
            dl->PushClipRect(ImVec2(wmin.x + rnd * 0.5f, wmin.y + rnd * 0.5f),
                             ImVec2(wmax.x - rnd * 0.5f, wmax.y - rnd * 0.5f), true);
            draw_radial_glow(dl, ImVec2(wmin.x + ww * 0.14f, wmin.y + wh * 0.42f),
                             std::min(ww * 0.36f, 260.0f * sc), col::Accent, 0.22f);
            dl->PopClipRect();
        } else {
            ImU32 top_c = to_u32(col::GlassTop, 0.21f);
            ImU32 bot_c = to_u32(col::GlassBot, 0.0f);
            dl->AddRectFilled(ImVec2(wmin.x + 1.0f, wmin.y + 1.0f), ImVec2(wmax.x - 1.0f, wmin.y + rnd),
                              top_c, rnd, ImDrawFlags_RoundCornersTop);
            dl->AddRectFilledMultiColor(
                ImVec2(wmin.x + 1.0f, wmin.y + rnd), ImVec2(wmax.x - 1.0f, wmax.y - rnd),
                top_c, top_c, bot_c, bot_c);
        }
    }

    if (accent_top) {
        // Colored top accent bar + silky-smooth fade downward (zero seam, zero corner bleed)
        // A barra acompanha o raio da borda: borda superior reta + um quarto de arco em cada canto,
        // com a ponta esmaecendo suavemente para dentro da lateral (sem corte seco)
        if (ww > rnd * 2.0f + 6.0f) {
            constexpr float kPiF = 3.14159265f;
            constexpr int kSeg = 14;
            const float arc_r = std::max(2.0f, rnd - 1.6f);
            const float bar_t = std::max(2.0f, std::round(2.4f * sc));
            const float top_y = wmin.y + rnd - arc_r;
            dl->AddLine(ImVec2(wmin.x + rnd, top_y), ImVec2(wmax.x - rnd, top_y), to_u32(*accent_top, 0.92f), bar_t);
            const float step = (kPiF * 0.5f) / static_cast<float>(kSeg);
            for (int side = 0; side < 2; ++side) {
                const ImVec2 ctr(side == 0 ? wmin.x + rnd : wmax.x - rnd, wmin.y + rnd);
                const float base = (side == 0) ? kPiF : kPiF * 1.5f;
                for (int i = 0; i < kSeg; ++i) {
                    float t = (static_cast<float>(i) + 0.5f) / static_cast<float>(kSeg); // 0 = lateral, 1 = topo (canto esquerdo)
                    if (side == 1) t = 1.0f - t;
                    const float e = t * t * (3.0f - 2.0f * t);
                    const float a0 = base + static_cast<float>(i) * step;
                    dl->PathClear();
                    dl->PathArcTo(ctr, arc_r, a0, a0 + step + 0.03f, 3);
                    dl->PathStroke(to_u32(*accent_top, 0.92f * e), ImDrawFlags_None, bar_t);
                }
            }
        }
        if (wh > rnd * 2.0f + 4.0f) {
            float glow_end = std::min(wmax.y - rnd, wmin.y + std::max(rnd + 8.0f, 76.0f * sc));
            ImU32 top_c = to_u32(*accent_top, 0.085f);
            ImU32 bot_c = to_u32(*accent_top, 0.0f);
            dl->AddRectFilled(ImVec2(wmin.x + 1.0f, wmin.y + 2.0f), ImVec2(wmax.x - 1.0f, wmin.y + rnd),
                              top_c, rnd, ImDrawFlags_RoundCornersTop);
            dl->AddRectFilledMultiColor(
                ImVec2(wmin.x + 1.0f, wmin.y + rnd), ImVec2(wmax.x - 1.0f, glow_end),
                top_c, top_c, bot_c, bot_c);
        }
    } else {
        // Specular 1px glass inset highlight (--highlight: inset 0 1px 0 rgba(255,255,255,0.06))
        dl->AddLine(ImVec2(wmin.x + rnd, wmin.y + 1.0f), ImVec2(wmax.x - rnd, wmin.y + 1.0f),
                    IM_COL32(255, 255, 255, hero_gradient ? 34 : 20), 1.0f);
    }
}

void end_studio_card() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

float calc_combo_width(const char* const* items, int count, float min_w, float max_w) {
    float widest = 0.0f;
    for (int i = 0; i < count; ++i) {
        if (items[i]) widest = std::max(widest, ImGui::CalcTextSize(items[i]).x);
    }
    float pad = ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetFrameHeight() + 18.0f;
    return std::clamp(widest + pad, min_w, max_w);
}

void draw_ovr_badge(int ovr, float min_w, float h, ImFont* bold_font) {
    char txt[16];
    if (ovr <= 0) std::snprintf(txt, sizeof(txt), "--");
    else std::snprintf(txt, sizeof(txt), "%d", ovr);

    ImVec4 fg = (ovr <= 0) ? col::Subtext : ovr_color(ovr);
    ImVec4 bg(fg.x, fg.y, fg.z, 0.22f);
    ImVec4 brd(fg.x, fg.y, fg.z, 0.58f);

    if (bold_font) ImGui::PushFont(bold_font);
    ImVec2 ts = ImGui::CalcTextSize(txt);
    float w = std::max(min_w, ts.x + 14.0f);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, h));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), to_u32(bg), 7.0f);
    dl->AddRectFilled(ImVec2(p.x + 1, p.y + 1), ImVec2(p.x + w - 1, p.y + h * 0.48f),
                      IM_COL32(255, 255, 255, 16), 7.0f, ImDrawFlags_RoundCornersTop);
    dl->AddRect(p, ImVec2(p.x + w, p.y + h), to_u32(brd), 7.0f, 0, 1.1f);
    dl->AddText(ImVec2(p.x + (w - ts.x) * 0.5f, p.y + (h - ts.y) * 0.5f), to_u32(fg), txt);
    if (bold_font) ImGui::PopFont();
}

void draw_ovr_shield(int ovr, float size, ImFont* big_font, ImFont* small_bold) {
    char txt[16];
    if (ovr <= 0) std::snprintf(txt, sizeof(txt), "--");
    else std::snprintf(txt, sizeof(txt), "%d", ovr);

    ImVec4 fg = (ovr <= 0) ? col::Subtext : ovr_color(ovr);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(size, size));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float rnd = size * 0.22f;
    ImVec2 p2(p.x + size, p.y + size);

    // Outer tier glow + soft drop shadow
    draw_soft_shadow(dl, p, p2, rnd, 1.1f);
    dl->AddRectFilled(ImVec2(p.x - 2.5f, p.y - 2.5f), ImVec2(p2.x + 2.5f, p2.y + 2.5f),
                      to_u32(fg, 0.16f), rnd + 2.5f);

    // Illuminated shield body + glossy top glass reflection
    dl->AddRectFilled(p, p2, to_u32(col::BgInput), rnd);
    dl->AddRectFilled(p, p2, to_u32(fg, 0.24f), rnd);
    dl->AddRectFilled(ImVec2(p.x + 1.5f, p.y + 1.5f), ImVec2(p2.x - 1.5f, p.y + size * 0.48f),
                      IM_COL32(255, 255, 255, 26), rnd, ImDrawFlags_RoundCornersTop);
    dl->AddRect(p, p2, to_u32(fg, 0.78f), rnd, 0, 1.6f);

    ImFont* nf = big_font ? big_font : ImGui::GetFont();
    ImVec2 ns = nf->CalcTextSizeA(nf->FontSize, FLT_MAX, 0.0f, txt);
    dl->AddText(nf, nf->FontSize, ImVec2(p.x + (size - ns.x) * 0.5f, p.y + size * 0.13f), to_u32(fg), txt);

    ImFont* sf = small_bold ? small_bold : ImGui::GetFont();
    float sfs = std::min(11.0f, sf->FontSize * 0.86f);
    ImVec2 ss = sf->CalcTextSizeA(sfs, FLT_MAX, 0.0f, "OVR");
    dl->AddText(sf, sfs, ImVec2(p.x + (size - ss.x) * 0.5f, p.y + size * 0.69f), to_u32(fg, 0.88f), "OVR");
}

void draw_pos_chip(Lang lang, int pos_id, float min_w, float h, ImFont* bold_font) {
    const char* txt = (pos_id < 0 || pos_id >= 13) ? "---" : pos_short(lang, pos_id);
    ImVec4 fg = (pos_id < 0 || pos_id >= 13) ? col::GreenSoft : pos_color(pos_id);
    ImVec4 bg(fg.x, fg.y, fg.z, 0.20f);
    ImVec4 brd(fg.x, fg.y, fg.z, 0.52f);

    if (bold_font) ImGui::PushFont(bold_font);
    ImVec2 ts = ImGui::CalcTextSize(txt);
    float w = std::max(min_w, ts.x + 14.0f);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, h));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), to_u32(bg), 6.0f);
    dl->AddRect(p, ImVec2(p.x + w, p.y + h), to_u32(brd), 6.0f, 0, 1.0f);
    dl->AddText(ImVec2(p.x + (w - ts.x) * 0.5f, p.y + (h - ts.y) * 0.5f), to_u32(fg), txt);
    if (bold_font) ImGui::PopFont();
}

void draw_arrow_badge(int arrow, float h, bool blink, Lang lang) {
    int a = std::clamp(arrow, 0, 4);
    ImVec4 fg = arrow_color(a);
    float w = h * 1.18f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, h));
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
        const std::string tip = arrow_effect_text(lang, a);
        show_rich_tooltip(arrow_title(lang, a), tip.c_str());
    }
    if (blink && !ImGui::IsItemVisible()) blink = false; // fora da tela nao precisa manter 60 fps
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float pulse = 1.0f;
    if (blink) {
        // Pisca em onda suave (~1,25 Hz), mantendo o frame loop ativo apenas enquanto houver seta piscando visivel
        pulse = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(static_cast<float>(ImGui::GetTime()) * 7.85f));
        mark_animating();
    }
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), to_u32(fg, 0.18f + (blink ? 0.22f * pulse : 0.0f)), 6.0f);
    dl->AddRect(p, ImVec2(p.x + w, p.y + h), to_u32(fg, blink ? 0.40f + 0.60f * pulse : 0.55f), 6.0f, 0, blink ? 1.6f : 1.1f);
    draw_arrow_vector(dl, ImVec2(p.x + w * 0.5f, p.y + h * 0.5f), h * 0.30f, a, to_u32(fg, pulse));
}

void draw_pill(const char* text, const ImVec4& fg, const ImVec4& bg, const ImVec4& border, bool compact) {
    ImVec2 ts = ImGui::CalcTextSize(text);
    float px = compact ? 7.0f : 9.0f;
    float py = compact ? 2.5f : 4.0f;
    float w = ts.x + px * 2.0f;
    float h = ts.y + py * 2.0f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, h));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), to_u32(bg), h * 0.5f);
    dl->AddRect(p, ImVec2(p.x + w, p.y + h), to_u32(border), h * 0.5f, 0, 1.0f);
    dl->AddText(ImVec2(p.x + px, p.y + py), to_u32(fg), text);
}

void draw_stamina_bar(int stam, float bar_w) {
    int cl = std::clamp(stam, 0, 100);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float sc = std::clamp(ImGui::GetFontSize() / 15.0f, 0.85f, 2.5f);
    float bw = bar_w * sc;
    float h = std::round(8.0f * sc);
    float lh = ImGui::GetTextLineHeight();
    float y_mid = p.y + (lh - h) * 0.5f;
    ImGui::Dummy(ImVec2(bw + 40.0f * sc, lh));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(ImVec2(p.x, y_mid), ImVec2(p.x + bw, y_mid + h), to_u32(col::BgInput), h * 0.5f);
    dl->AddRect(ImVec2(p.x, y_mid), ImVec2(p.x + bw, y_mid + h), to_u32(col::Border), h * 0.5f);
    float fill_w = bw * (static_cast<float>(cl) / 100.0f);
    if (fill_w > 2.0f) {
        ImVec4 sc_col = stamina_color(cl);
        dl->AddRectFilled(ImVec2(p.x + 1.0f, y_mid + 1.0f), ImVec2(p.x + fill_w - 1.0f, y_mid + h - 1.0f),
                          to_u32(sc_col), (h - 2.0f) * 0.5f);
    }
    char pct[12];
    std::snprintf(pct, sizeof(pct), "%d%%", cl);
    dl->AddText(ImVec2(p.x + bw + 7.0f * sc, p.y), to_u32(col::Subtext), pct);
}

void draw_rating_segments(int val, int max_val, const ImVec4& active_col, float width, float h) {
    int n = std::clamp(max_val, 2, 10);
    int v = std::clamp(val, 0, n);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(width, h + 2.0f));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float gap = 3.0f;
    float seg_w = (width - gap * static_cast<float>(n - 1)) / static_cast<float>(n);
    for (int i = 0; i < n; ++i) {
        float x0 = p.x + static_cast<float>(i) * (seg_w + gap);
        bool lit = (i < v);
        dl->AddRectFilled(ImVec2(x0, p.y + 1.0f), ImVec2(x0 + seg_w, p.y + 1.0f + h),
                          lit ? to_u32(active_col, 0.92f) : to_u32(col::BgInput), 2.5f);
        dl->AddRect(ImVec2(x0, p.y + 1.0f), ImVec2(x0 + seg_w, p.y + 1.0f + h),
                    lit ? to_u32(active_col, 0.55f) : to_u32(col::Border), 2.5f, 0, 1.0f);
    }
}

bool draw_rating_segments_interactive(const char* id, int* val, int max_val, const ImVec4& active_col, ImFont* bold_font) {
    int n = std::clamp(max_val, 2, 10);
    int cur = val ? std::clamp(*val, 1, n) : 1;
    float sc = std::clamp(ImGui::GetFontSize() / 15.0f, 0.85f, 2.5f);
    float avail = std::max(100.0f * sc, ImGui::GetContentRegionAvail().x);
    float row_h = ImGui::GetFrameHeight();
    float badge_w = std::round(36.0f * sc);
    float bar_w = std::max(60.0f * sc, avail - badge_w - 8.0f * sc);

    ImGui::PushID(id);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##seg_hit", ImVec2(avail, row_h));
    bool hov = ImGui::IsItemHovered();
    bool clicked = hov && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool dragging = ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (hov) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    int hover_v = cur;
    if (hov || dragging) {
        float rel = (ImGui::GetIO().MousePos.x - p.x) / std::max(1.0f, bar_w);
        hover_v = std::clamp(static_cast<int>(std::floor(rel * n)) + 1, 1, n);
    }
    bool changed = false;
    if ((clicked || dragging) && val && hover_v != cur) {
        *val = hover_v;
        cur = hover_v;
        changed = true;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    int disp_v = hov ? hover_v : cur;
    float gap = std::round(4.0f * sc);
    float seg_w = (bar_w - gap * static_cast<float>(n - 1)) / static_cast<float>(n);
    float seg_h = std::round(14.0f * sc);
    float sy0 = p.y + (row_h - seg_h) * 0.5f;

    for (int i = 0; i < n; ++i) {
        float x0 = p.x + static_cast<float>(i) * (seg_w + gap);
        bool lit = (i < disp_v);
        bool is_preview = hov && (i < hover_v) && (i >= cur);
        ImVec4 c = is_preview ? ImVec4(active_col.x, active_col.y, active_col.z, 0.55f) : active_col;
        if (lit) {
            dl->AddRectFilled(ImVec2(x0, sy0), ImVec2(x0 + seg_w, sy0 + seg_h), to_u32(c, 0.90f), 4.0f);
            dl->AddRectFilled(ImVec2(x0 + 1, sy0 + 1), ImVec2(x0 + seg_w - 1, sy0 + seg_h * 0.45f),
                              IM_COL32(255, 255, 255, 28), 4.0f, ImDrawFlags_RoundCornersTop);
            dl->AddRect(ImVec2(x0, sy0), ImVec2(x0 + seg_w, sy0 + seg_h), to_u32(c, 0.70f), 4.0f, 0, 1.0f);
        } else {
            dl->AddRectFilled(ImVec2(x0, sy0), ImVec2(x0 + seg_w, sy0 + seg_h), to_u32(col::BgInput), 4.0f);
            dl->AddRect(ImVec2(x0, sy0), ImVec2(x0 + seg_w, sy0 + seg_h), to_u32(col::Border), 4.0f, 0, 1.0f);
        }
    }

    // Crisp numeric readout badge on right (e.g. "3/4")
    ImVec2 bmin(p.x + bar_w + 8.0f * sc, p.y + 2.0f);
    ImVec2 bmax(p.x + avail, p.y + row_h - 2.0f);
    dl->AddRectFilled(bmin, bmax, to_u32(active_col, 0.16f), 6.0f);
    dl->AddRect(bmin, bmax, to_u32(active_col, 0.45f), 6.0f, 0, 1.0f);
    char nbuf[16];
    std::snprintf(nbuf, sizeof(nbuf), "%d/%d", disp_v, n);
    if (bold_font) ImGui::PushFont(bold_font);
    ImVec2 ts = ImGui::CalcTextSize(nbuf);
    dl->AddText(ImVec2(bmin.x + (bmax.x - bmin.x - ts.x) * 0.5f, bmin.y + (bmax.y - bmin.y - ts.y) * 0.5f),
                to_u32(active_col), nbuf);
    if (bold_font) ImGui::PopFont();

    ImGui::PopID();
    return changed;
}

bool draw_stamina_slider(const char* id, int* stam, float width, ImFont* bold_font) {
    int cur = stam ? std::clamp(*stam, 0, 100) : 100;
    float sc = std::clamp(ImGui::GetFontSize() / 15.0f, 0.85f, 2.5f);
    float row_h = ImGui::GetFrameHeight();
    float badge_w = std::round(50.0f * sc);
    float track_w = std::max(72.0f * sc, width - badge_w - 10.0f * sc);

    ImGui::PushID(id);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##stam_sl", ImVec2(width, row_h));
    bool hov = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();
    if (hov || active) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    if (active && ImGui::IsMouseDown(ImGuiMouseButton_Left) && stam) {
        float frac = std::clamp((ImGui::GetIO().MousePos.x - (p.x + 8.0f * sc)) / std::max(1.0f, track_w - 16.0f * sc), 0.0f, 1.0f);
        *stam = static_cast<int>(std::lround(frac * 100.0f));
        cur = *stam;
    }
    bool committed = ImGui::IsItemDeactivated();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    float mid_y = p.y + row_h * 0.5f;
    float trk_h = std::round(4.0f * sc);
    float frac = static_cast<float>(cur) / 100.0f;
    float thumb_x = p.x + 8.0f * sc + frac * (track_w - 16.0f * sc);
    ImVec4 sc_col = stamina_color(cur);

    dl->AddRectFilled(ImVec2(p.x, mid_y - trk_h), ImVec2(p.x + track_w, mid_y + trk_h), to_u32(col::BgInput), trk_h);
    dl->AddRectFilled(ImVec2(p.x, mid_y - trk_h), ImVec2(thumb_x, mid_y + trk_h), to_u32(sc_col, 0.90f), trk_h);
    dl->AddRect(ImVec2(p.x, mid_y - trk_h), ImVec2(p.x + track_w, mid_y + trk_h), to_u32(col::Border), trk_h);
    float tr = ((hov || active) ? 7.6f : 6.2f) * sc;
    if (hov || active) dl->AddCircleFilled(ImVec2(thumb_x, mid_y), tr + 4.5f * sc, to_u32(sc_col, 0.28f), 18);
    dl->AddCircleFilled(ImVec2(thumb_x, mid_y), tr, IM_COL32(248, 250, 252, 255), 18);
    dl->AddCircle(ImVec2(thumb_x, mid_y), tr, to_u32(sc_col), 18, 2.0f);

    ImVec2 bmin(p.x + track_w + 10.0f * sc, p.y + 1.0f);
    ImVec2 bmax(p.x + width, p.y + row_h - 1.0f);
    dl->AddRectFilled(bmin, bmax, to_u32(sc_col, 0.16f), 6.0f);
    dl->AddRect(bmin, bmax, to_u32(sc_col, 0.48f), 6.0f, 0, 1.0f);
    char pb[16];
    std::snprintf(pb, sizeof(pb), "%d%%", cur);
    if (bold_font) ImGui::PushFont(bold_font);
    ImVec2 ts = ImGui::CalcTextSize(pb);
    dl->AddText(ImVec2(bmin.x + (bmax.x - bmin.x - ts.x) * 0.5f, bmin.y + (bmax.y - bmin.y - ts.y) * 0.5f),
                to_u32(sc_col), pb);
    if (bold_font) ImGui::PopFont();

    ImGui::PopID();
    return committed;
}

int draw_arrow_picker(const char* id, int cur_arrow, Lang lang, float avail_w, ImFont* bold_font) {
    struct AItem { int a; const char* pt; const char* en; };
    const AItem items[5] = {
        {4, "Azul", "Blue"},
        {3, "Verde", "Green"},
        {2, "Amarela", "Yellow"},
        {1, "Laranja", "Orange"},
        {0, "Vermelha", "Red"},
    };
    float sc = std::clamp(ImGui::GetFontSize() / 15.0f, 0.85f, 2.5f);
    float gap = std::round(6.0f * sc);
    float tile_w = std::max(54.0f * sc, (avail_w - gap * 4.0f) / 5.0f);
    float tile_h = std::round(34.0f * sc);
    bool show_lbl = (tile_w >= 78.0f * sc);
    int picked = -1;

    ImGui::PushID(id);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (int i = 0; i < 5; ++i) {
        if (i > 0) ImGui::SameLine(0.0f, gap);
        const auto& it = items[i];
        bool act = (cur_arrow == it.a);
        ImVec4 fg = arrow_color(it.a);
        ImVec2 p = ImGui::GetCursorScreenPos();
        char bid[16];
        std::snprintf(bid, sizeof(bid), "##ap_%d", it.a);
        if (ImGui::InvisibleButton(bid, ImVec2(tile_w, tile_h))) picked = it.a;
        bool hov = ImGui::IsItemHovered();
        if (hov) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        float t_h = anim_toward(ImGui::GetItemID(), (act || hov) ? 1.0f : 0.0f, 14.0f);
        ImVec2 p2(p.x + tile_w, p.y + tile_h);
        if (act) {
            dl->AddRectFilled(ImVec2(p.x - 1.5f, p.y - 1.5f), ImVec2(p2.x + 1.5f, p2.y + 1.5f),
                              to_u32(fg, 0.24f), 9.5f);
        }
        dl->AddRectFilled(p, p2, act ? to_u32(fg, 0.24f) : to_u32(lerp_col(col::BgInput, col::BgCard2, t_h)), 8.0f);
        dl->AddRect(p, p2, act ? to_u32(fg, 0.88f) : to_u32(lerp_col(col::Border, fg, t_h * 0.65f)), 8.0f, 0, act ? 1.5f : 1.0f);

        const char* lbl = T(lang, it.pt, it.en);
        if (show_lbl) {
            if (bold_font && act) ImGui::PushFont(bold_font);
            ImVec2 ts = ImGui::CalcTextSize(lbl);
            float icon_r = std::round(6.5f * sc);
            float content_w = icon_r * 2.0f + 7.0f * sc + ts.x;
            float sx = p.x + (tile_w - content_w) * 0.5f;
            draw_arrow_vector(dl, ImVec2(sx + icon_r, p.y + tile_h * 0.5f), icon_r, it.a, to_u32(fg));
            dl->AddText(ImVec2(sx + icon_r * 2.0f + 7.0f * sc, p.y + (tile_h - ts.y) * 0.5f),
                        to_u32(act ? col::Text : (hov ? col::Text : fg)), lbl);
            if (bold_font && act) ImGui::PopFont();
        } else {
            draw_arrow_vector(dl, ImVec2(p.x + tile_w * 0.5f, p.y + tile_h * 0.5f), std::round(7.5f * sc), it.a, to_u32(fg));
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
            const std::string tip = arrow_effect_text(lang, it.a);
            show_rich_tooltip(arrow_title(lang, it.a), tip.c_str());
        }
    }
    ImGui::PopID();
    return picked;
}

bool draw_mode_card(const char* id, const char* title, const char* subtitle, bool active, float height, ImFont* bold_font, ImFont* small_bold) {
    float sc = std::clamp(ImGui::GetFontSize() / 15.0f, 0.85f, 2.5f);
    float w = ImGui::GetContentRegionAvail().x;
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton(id, ImVec2(w, height));
    bool hov = ImGui::IsItemHovered();
    if (hov) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    float t_h = anim_toward(ImGui::GetItemID(), (active || hov) ? 1.0f : 0.0f, 14.0f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p2(p.x + w, p.y + height);
    float rnd = std::round(10.0f * sc);

    if (active) {
        dl->AddRectFilled(ImVec2(p.x - 1.0f, p.y - 1.0f), ImVec2(p2.x + 1.0f, p2.y + 1.0f),
                          to_u32(col::Accent, 0.18f), rnd + 1.0f);
    }
    ImVec4 bg = active
        ? [] { ImVec4 c = lerp_col(col::BgCard2, col::Accent, 0.28f); c.w = 0.96f; return c; }()
        : lerp_col(col::BgInput, col::BgCard2, t_h * 0.85f);
    ImVec4 brd = active ? col::Accent : lerp_col(col::Border, col::BorderLit, t_h);
    dl->AddRectFilled(p, p2, to_u32(bg), rnd);
    if (active) {
        dl->AddRectFilled(p, ImVec2(p.x + 3.5f * sc, p2.y), to_u32(col::Accent), rnd, ImDrawFlags_RoundCornersLeft);
    }
    dl->AddRect(p, p2, to_u32(brd), rnd, 0, active ? 1.5f : 1.0f);

    float pad_x = std::round(14.0f * sc);
    float pad_y = std::round(11.0f * sc);
    float max_txt_w = w - pad_x * 2.0f - (active ? 44.0f * sc : 0.0f);

    dl->PushClipRect(ImVec2(p.x + pad_x, p.y + 4.0f), ImVec2(p.x + pad_x + std::max(40.0f, max_txt_w), p2.y - 4.0f), true);
    if (bold_font) ImGui::PushFont(bold_font);
    dl->AddText(ImVec2(p.x + pad_x, p.y + pad_y), to_u32(active ? col::Text : (hov ? col::Text : ImVec4(0.86f, 0.90f, 0.96f, 1.0f))), title);
    float title_h = ImGui::GetTextLineHeight();
    if (bold_font) ImGui::PopFont();

    if (small_bold) ImGui::PushFont(small_bold);
    dl->AddText(ImVec2(p.x + pad_x, p.y + pad_y + title_h + 4.0f * sc),
                to_u32(active ? col::AccentSoft : col::Subtext), subtitle);
    if (small_bold) ImGui::PopFont();
    dl->PopClipRect();

    if (active) {
        if (small_bold) ImGui::PushFont(small_bold);
        ImVec2 ts = ImGui::CalcTextSize("ON");
        float pw = ts.x + 12.0f * sc, ph = ts.y + 4.0f * sc;
        ImVec2 op(p2.x - pw - 10.0f * sc, p.y + 9.0f * sc);
        dl->AddRectFilled(op, ImVec2(op.x + pw, op.y + ph), to_u32(col::Accent), ph * 0.5f);
        dl->AddText(ImVec2(op.x + 6.0f * sc, op.y + 2.0f * sc), IM_COL32(255, 255, 255, 255), "ON");
        if (small_bold) ImGui::PopFont();
    }
    return clicked;
}

bool draw_active_player_pill(int ovr, int pos_id, const char* name, Lang lang, float max_w, float h, ImFont* bold_font) {
    float sc = std::clamp(ImGui::GetFontSize() / 15.0f, 0.85f, 2.5f);
    char ovr_s[12];
    std::snprintf(ovr_s, sizeof(ovr_s), "%d", ovr);
    const char* pos_s = (pos_id >= 0 && pos_id < 13) ? pos_short(lang, pos_id) : "---";

    if (bold_font) ImGui::PushFont(bold_font);
    ImVec2 ns = ImGui::CalcTextSize(name ? name : "---");
    ImVec2 os = ImGui::CalcTextSize(ovr_s);
    ImVec2 ps = ImGui::CalcTextSize(pos_s);
    if (bold_font) ImGui::PopFont();

    float b_h = h - 8.0f * sc;
    float ovr_w = std::max(30.0f * sc, os.x + 12.0f * sc);
    float pos_w = std::max(36.0f * sc, ps.x + 12.0f * sc);
    float pad_l = 5.0f * sc;
    float pad_r = 14.0f * sc;
    float natural_w = pad_l + ovr_w + 6.0f * sc + pos_w + 9.0f * sc + ns.x + pad_r;
    float pill_w = std::min(max_w, std::max(ovr_w + pos_w + 60.0f * sc, natural_w));

    ImVec2 p = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton("##tb_active_pill", ImVec2(pill_w, h));
    bool hov = ImGui::IsItemHovered();
    if (hov) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    float t_h = anim_toward(ImGui::GetItemID(), hov ? 1.0f : 0.0f, 14.0f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p2(p.x + pill_w, p.y + h);
    float rnd = h * 0.5f;

    dl->AddRectFilled(p, p2, to_u32(lerp_col(col::BgCard2, col::BgCardHov, t_h)), rnd);
    dl->AddRect(p, p2, to_u32(lerp_col(col::Border, col::Accent, t_h)), rnd, 0, 1.1f);

    float by = p.y + (h - b_h) * 0.5f;
    // OVR pill inside
    ImVec4 oc = ovr_color(ovr);
    ImVec2 o0(p.x + pad_l, by);
    ImVec2 o1(o0.x + ovr_w, by + b_h);
    dl->AddRectFilled(o0, o1, to_u32(oc, 0.22f), b_h * 0.5f);
    dl->AddRect(o0, o1, to_u32(oc, 0.55f), b_h * 0.5f, 0, 1.0f);
    if (bold_font) ImGui::PushFont(bold_font);
    dl->AddText(ImVec2(o0.x + (ovr_w - os.x) * 0.5f, o0.y + (b_h - os.y) * 0.5f), to_u32(oc), ovr_s);

    // POS chip inside
    ImVec4 pc = pos_color(pos_id);
    ImVec2 c0(o1.x + 6.0f * sc, by);
    ImVec2 c1(c0.x + pos_w, by + b_h);
    dl->AddRectFilled(c0, c1, to_u32(pc, 0.18f), 6.0f);
    dl->AddRect(c0, c1, to_u32(pc, 0.50f), 6.0f, 0, 1.0f);
    dl->AddText(ImVec2(c0.x + (pos_w - ps.x) * 0.5f, c0.y + (b_h - ps.y) * 0.5f), to_u32(pc), pos_s);

    // Player name with clean ellipsis when narrow (zero hard-cut glyphs)
    float tx0 = c1.x + 9.0f * sc;
    float tx1 = p2.x - 10.0f * sc;
    float avail_tw = tx1 - tx0;
    if (avail_tw > 12.0f) {
        std::string disp = (name && name[0]) ? std::string(name) : std::string("---");
        if (ImGui::CalcTextSize(disp.c_str()).x > avail_tw) {
            float ell_w = ImGui::CalcTextSize("...").x;
            while (disp.size() > 1) {
                // Pop one UTF-8 codepoint cleanly
                while (!disp.empty() && (static_cast<unsigned char>(disp.back()) & 0xC0) == 0x80) disp.pop_back();
                if (!disp.empty()) disp.pop_back();
                if (ImGui::CalcTextSize(disp.c_str()).x + ell_w <= avail_tw) break;
            }
            while (!disp.empty() && disp.back() == ' ') disp.pop_back();
            disp += "...";
        }
        dl->PushClipRect(ImVec2(tx0, p.y), ImVec2(tx1, p2.y), true);
        dl->AddText(ImVec2(tx0, p.y + (h - ns.y) * 0.5f), to_u32(col::Text), disp.c_str());
        dl->PopClipRect();
    }
    if (bold_font) ImGui::PopFont();
    return clicked;
}

void show_rich_tooltip(const char* title, const char* body) {
    if ((!title || !title[0]) && (!body || !body[0])) return;
    float sc = std::clamp(ImGui::GetFontSize() / 15.0f, 0.85f, 2.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(std::round(13.0f * sc), std::round(10.0f * sc)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, std::round(10.0f * sc));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, col::PopupBg);
    ImGui::PushStyleColor(ImGuiCol_Border, col::BorderLit);
    if (ImGui::BeginTooltip()) {
        ImGui::PushTextWrapPos(360.0f * sc);
        if (title && title[0]) {
            ImGui::TextColored(col::AccentSoft, "%s", title);
        }
        if (body && body[0]) {
            ImGui::TextColored(ImVec4(0.840f, 0.875f, 0.922f, 1.0f), "%s", body);
        }
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void show_tip_if_hovered(const char* title, const char* body) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_AllowWhenDisabled)) {
        show_rich_tooltip(title, body);
    }
}

void draw_radar_6axis(const std::array<int, 48>& stats, Lang lang, float width, ImFont* bold_font, ImFont* small_bold) {
    auto avg = [&](std::initializer_list<int> fids) {
        int sum = 0;
        for (int f : fids) sum += stats[f];
        return static_cast<int>(std::lround(static_cast<double>(sum) / static_cast<double>(fids.size())));
    };
    struct Axis { const char* lbl; int val; };
    const Axis axes[6] = {
        {T(lang, "ATQ", "ATK"), avg({14, 20, 23, 26, 27})},
        {"DRI",                 avg({17, 18, 19})},
        {"PAS",                 avg({21, 22})},
        {T(lang, "FIS", "PHY"), avg({33, 34, 35, 36, 37, 38, 39})},
        {"DEF",                 avg({15, 24, 25})},
        {T(lang, "GO", "GK"),   avg({16, 28, 29, 30, 31})},
    };

    float sc = std::clamp(ImGui::GetFontSize() / 15.0f, 0.85f, 2.5f);
    float canvas_w = std::max(210.0f * sc, width);
    float canvas_h = std::round(215.0f * sc);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(canvas_w, canvas_h));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImVec2 center(p.x + canvas_w * 0.5f, p.y + canvas_h * 0.5f);
    float max_r = std::min(canvas_w, canvas_h) * 0.36f;

    // Subtle radial spotlight behind the radar web
    draw_radial_glow(dl, center, max_r * 1.15f, col::Accent, 0.16f);

    auto pt = [&](int i, float frac) -> ImVec2 {
        float a = (-3.14159265f * 0.5f) + (static_cast<float>(i) * 3.14159265f * 2.0f / 6.0f);
        return ImVec2(center.x + std::cos(a) * max_r * frac, center.y + std::sin(a) * max_r * frac);
    };

    ImU32 ring_col = to_u32(col::Subtext, 0.25f);
    for (float f : {0.25f, 0.50f, 0.75f, 1.0f}) {
        ImVec2 poly[6];
        for (int i = 0; i < 6; ++i) poly[i] = pt(i, f);
        dl->AddPolyline(poly, 6, ring_col, ImDrawFlags_Closed, f > 0.99f ? 1.3f : 1.0f);
    }
    ImU32 spoke_col = to_u32(col::Subtext, 0.20f);
    for (int i = 0; i < 6; ++i) dl->AddLine(center, pt(i, 1.0f), spoke_col, 1.0f);

    ImVec2 dpoly[6];
    for (int i = 0; i < 6; ++i) {
        float frac = std::clamp(static_cast<float>(axes[i].val - 30) / 70.0f, 0.15f, 1.0f);
        dpoly[i] = pt(i, frac);
    }
    // Star-shaped triangulation from center so concave stat profiles never bleed outside the polyline
    ImU32 fill_col = to_u32(col::Accent, 0.34f);
    for (int i = 0; i < 6; ++i) {
        dl->AddTriangleFilled(center, dpoly[i], dpoly[(i + 1) % 6], fill_col);
    }
    dl->AddPolyline(dpoly, 6, to_u32(col::AccentSoft, 0.96f), ImDrawFlags_Closed, 2.2f);
    for (int i = 0; i < 6; ++i) {
        ImVec4 vc = ovr_color(axes[i].val);
        dl->AddCircleFilled(dpoly[i], 6.5f * sc, to_u32(vc, 0.24f), 14);
        dl->AddCircleFilled(dpoly[i], 3.8f * sc, to_u32(vc), 14);
        dl->AddCircle(dpoly[i], 3.8f * sc, to_u32(col::BgSidebar), 14, 1.4f);
    }

    ImFont* lbl_f = small_bold ? small_bold : (bold_font ? bold_font : ImGui::GetFont());
    ImGui::PushFont(lbl_f);
    for (int i = 0; i < 6; ++i) {
        ImVec2 lp = pt(i, 1.24f);
        ImVec2 ts = ImGui::CalcTextSize(axes[i].lbl);
        dl->AddText(ImVec2(lp.x - ts.x * 0.5f, lp.y - ts.y * 0.5f), to_u32(col::Text, 0.88f), axes[i].lbl);
    }
    ImGui::PopFont();

    if (ImGui::BeginTable("##radar_leg", 3, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoPadOuterX)) {
        for (int i = 0; i < 6; ++i) {
            ImGui::TableNextColumn();
            ImVec2 bp = ImGui::GetCursorScreenPos();
            float bw = ImGui::GetContentRegionAvail().x;
            float bh = std::round(44.0f * sc);
            ImGui::Dummy(ImVec2(bw, bh));
            ImVec4 vc = ovr_color(axes[i].val);
            dl->AddRectFilled(bp, ImVec2(bp.x + bw, bp.y + bh), to_u32(col::BgInput), 7.0f);
            dl->AddRectFilled(bp, ImVec2(bp.x + bw, bp.y + 2.5f), to_u32(vc, 0.55f), 7.0f, ImDrawFlags_RoundCornersTop);
            dl->AddRect(bp, ImVec2(bp.x + bw, bp.y + bh), to_u32(col::Border), 7.0f);

            ImGui::PushFont(lbl_f);
            ImVec2 lt = ImGui::CalcTextSize(axes[i].lbl);
            dl->AddText(ImVec2(bp.x + (bw - lt.x) * 0.5f, bp.y + 5.0f * sc), to_u32(col::Muted), axes[i].lbl);
            ImGui::PopFont();

            char vn[12];
            std::snprintf(vn, sizeof(vn), "%d", axes[i].val);
            if (bold_font) ImGui::PushFont(bold_font);
            ImVec2 vt = ImGui::CalcTextSize(vn);
            dl->AddText(ImVec2(bp.x + (bw - vt.x) * 0.5f, bp.y + 20.0f * sc), to_u32(vc), vn);
            if (bold_font) ImGui::PopFont();
        }
        ImGui::EndTable();
    }
}

int draw_tactical_pitch(const std::array<PositionEntry, 13>& positions, int highlighted_pos, Lang lang, bool horizontal, float avail_w, ImFont* bold_font) {
    float sc = std::clamp(ImGui::GetFontSize() / 15.0f, 0.85f, 2.5f);
    const float base_w = horizontal ? 460.0f : 220.0f;
    const float base_h = horizontal ? 204.0f : 320.0f;
    const float max_w = horizontal ? std::min(620.0f * sc, avail_w) : std::min(280.0f * sc, avail_w);
    const float scale = std::max(0.65f, max_w / base_w);
    const float draw_w = base_w * scale;
    const float draw_h = base_h * scale;

    float offset_x = std::max(0.0f, (avail_w - draw_w) * 0.5f);
    if (offset_x > 1.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset_x);

    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##tactical_pitch_canvas", ImVec2(draw_w, draw_h));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    auto sx = [&](float x) { return origin.x + x * scale; };
    auto sy = [&](float y) { return origin.y + y * scale; };

    // Soft outer stadium drop shadow + broadcast mowed-turf alternating grass stripes
    draw_soft_shadow(dl, origin, ImVec2(origin.x + draw_w, origin.y + draw_h), 12.0f, 0.9f);
    ImU32 turf_dark  = IM_COL32(11, 34, 25, 255);
    ImU32 turf_light = IM_COL32(16, 48, 35, 255);
    dl->AddRectFilled(origin, ImVec2(origin.x + draw_w, origin.y + draw_h), turf_dark, 12.0f);

    dl->PushClipRect(ImVec2(origin.x + 2, origin.y + 2), ImVec2(origin.x + draw_w - 2, origin.y + draw_h - 2), true);
    constexpr int kStripes = 8;
    for (int s = 0; s < kStripes; ++s) {
        if (s % 2 == 1) continue;
        if (horizontal) {
            float x0 = origin.x + (draw_w * s) / kStripes;
            float x1 = origin.x + (draw_w * (s + 1)) / kStripes;
            dl->AddRectFilled(ImVec2(x0, origin.y), ImVec2(x1, origin.y + draw_h), turf_light);
        } else {
            float y0 = origin.y + (draw_h * s) / kStripes;
            float y1 = origin.y + (draw_h * (s + 1)) / kStripes;
            dl->AddRectFilled(ImVec2(origin.x, y0), ImVec2(origin.x + draw_w, y1), turf_light);
        }
    }
    // Stadium center floodlight sheen
    draw_radial_glow(dl, ImVec2(origin.x + draw_w * 0.5f, origin.y + draw_h * 0.5f),
                     std::max(draw_w, draw_h) * 0.42f, col::GreenSoft, 0.14f);
    dl->PopClipRect();

    dl->AddRect(origin, ImVec2(origin.x + draw_w, origin.y + draw_h), IM_COL32(52, 211, 153, 120), 12.0f, 0, 1.4f);

    ImU32 line_c = IM_COL32(167, 243, 208, 95);
    if (horizontal) {
        dl->AddRect(ImVec2(sx(8), sy(8)), ImVec2(sx(452), sy(196)), line_c, 6.0f, 0, 1.3f);
        dl->AddLine(ImVec2(sx(230), sy(8)), ImVec2(sx(230), sy(196)), line_c, 1.2f);
        dl->AddCircle(ImVec2(sx(230), sy(102)), 28.0f * scale, line_c, 36, 1.2f);
        dl->AddCircleFilled(ImVec2(sx(230), sy(102)), 2.5f * scale, line_c, 12);
        // Penalty boxes + 6-yard boxes
        dl->AddRect(ImVec2(sx(8), sy(44)), ImVec2(sx(64), sy(160)), line_c, 0.0f, 0, 1.2f);
        dl->AddRect(ImVec2(sx(8), sy(72)), ImVec2(sx(28), sy(132)), line_c, 0.0f, 0, 1.0f);
        dl->AddRect(ImVec2(sx(396), sy(44)), ImVec2(sx(452), sy(160)), line_c, 0.0f, 0, 1.2f);
        dl->AddRect(ImVec2(sx(432), sy(72)), ImVec2(sx(452), sy(132)), line_c, 0.0f, 0, 1.0f);
    } else {
        dl->AddRect(ImVec2(sx(8), sy(8)), ImVec2(sx(212), sy(312)), line_c, 6.0f, 0, 1.3f);
        dl->AddLine(ImVec2(sx(8), sy(160)), ImVec2(sx(212), sy(160)), line_c, 1.2f);
        dl->AddCircle(ImVec2(sx(110), sy(160)), 28.0f * scale, line_c, 36, 1.2f);
        dl->AddCircleFilled(ImVec2(sx(110), sy(160)), 2.5f * scale, line_c, 12);
        dl->AddRect(ImVec2(sx(52), sy(8)), ImVec2(sx(168), sy(56)), line_c, 0.0f, 0, 1.2f);
        dl->AddRect(ImVec2(sx(80), sy(8)), ImVec2(sx(140), sy(26)), line_c, 0.0f, 0, 1.0f);
        dl->AddRect(ImVec2(sx(52), sy(264)), ImVec2(sx(168), sy(312)), line_c, 0.0f, 0, 1.2f);
        dl->AddRect(ImVec2(sx(80), sy(294)), ImVec2(sx(140), sy(312)), line_c, 0.0f, 0, 1.0f);
    }

    static constexpr float kVert[13][2] = {
        {110, 286}, {110, 240}, {40, 232}, {180, 232},
        {110, 194}, {110, 154}, {40, 146}, {180, 146},
        {110, 114}, {44, 62},   {176, 62}, {110, 74}, {110, 34},
    };
    static constexpr float kHoriz[13][2] = {
        {36, 102},  {98, 102},  {98, 36},  {98, 168},
        {160, 102}, {224, 102}, {224, 34}, {224, 170},
        {288, 102}, {352, 38},  {352, 166},{352, 102}, {416, 102},
    };

    ImVec2 mouse = ImGui::GetIO().MousePos;
    bool hovered_canvas = ImGui::IsItemHovered();
    bool clicked_canvas = hovered_canvas && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    int clicked_pos = -1;

    float nw = 24.0f * scale;
    float nh = 15.0f * scale;

    for (const auto& pos : positions) {
        int id = pos.id;
        float cx = sx(horizontal ? kHoriz[id][0] : kVert[id][0]);
        float cy = sy(horizontal ? kHoriz[id][1] : kVert[id][1]);
        ImVec2 rmin(cx - nw, cy - nh);
        ImVec2 rmax(cx + nw, cy + nh);

        bool hov = hovered_canvas && mouse.x >= rmin.x && mouse.x <= rmax.x && mouse.y >= rmin.y && mouse.y <= rmax.y;
        if (hov) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (clicked_canvas) clicked_pos = id;
            char tip_title[64];
            std::snprintf(tip_title, sizeof(tip_title), "%s - %s", pos_short(lang, id), pos_full(lang, id));
            char tip_body[96];
            const char* gr = pos.apt == 2 ? "A" : (pos.apt == 1 ? "B" : "C");
            std::snprintf(tip_body, sizeof(tip_body), T(lang, "OVR: %d | Grau: %s%s", "OVR: %d | Grade: %s%s"),
                          pos.ovr, gr, pos.is_reg ? T(lang, " (Principal)", " (Primary)") : "");
            show_rich_tooltip(tip_title, tip_body);
        }

        ImU32 fill = pos.is_reg ? to_u32(col::Accent, 0.98f)
                   : (pos.apt == 2 ? IM_COL32(5, 150, 105, 245)
                   : (pos.apt == 1 ? IM_COL32(180, 83, 9, 245)
                   : to_u32(col::BgCard2, 0.94f)));
        ImU32 stroke = (highlighted_pos == id || hov) ? IM_COL32(255, 255, 255, 255)
                     : (pos.is_reg ? to_u32(col::AccentSoft)
                     : (pos.apt == 2 ? IM_COL32(52, 211, 153, 255)
                     : (pos.apt == 1 ? IM_COL32(251, 191, 36, 255)
                     : to_u32(col::Muted))));
        float sw = (highlighted_pos == id || pos.is_reg || hov) ? 2.1f : 1.1f;

        // Glow ring for active / primary / hovered node
        if (highlighted_pos == id || pos.is_reg || hov) {
            ImU32 halo = pos.is_reg ? to_u32(col::Accent, 0.37f) : IM_COL32(255, 255, 255, 65);
            dl->AddRectFilled(ImVec2(rmin.x - 3.0f, rmin.y - 3.0f), ImVec2(rmax.x + 3.0f, rmax.y + 3.0f), halo, 8.5f);
        }
        dl->AddRectFilled(ImVec2(rmin.x + 1, rmin.y + 2), ImVec2(rmax.x + 1, rmax.y + 2), IM_COL32(0, 0, 0, 95), 6.0f);
        dl->AddRectFilled(rmin, rmax, fill, 6.0f);
        dl->AddRectFilled(ImVec2(rmin.x + 1, rmin.y + 1), ImVec2(rmax.x - 1, cy),
                          IM_COL32(255, 255, 255, 22), 6.0f, ImDrawFlags_RoundCornersTop);
        dl->AddRect(rmin, rmax, stroke, 6.0f, 0, sw);

        const char* ptxt = pos_short(lang, id);
        char otxt[8];
        std::snprintf(otxt, sizeof(otxt), "%d", pos.ovr);

        float fs_lbl = std::clamp(11.0f * scale, 10.0f, 14.0f);
        float fs_ovr = std::clamp(12.5f * scale, 11.0f, 15.5f);
        ImFont* fnt = bold_font ? bold_font : ImGui::GetFont();
        ImVec2 ts1 = fnt->CalcTextSizeA(fs_lbl, FLT_MAX, 0.0f, ptxt);
        ImVec2 ts2 = fnt->CalcTextSizeA(fs_ovr, FLT_MAX, 0.0f, otxt);
        dl->AddText(fnt, fs_lbl, ImVec2(cx - ts1.x * 0.5f, cy - nh + 1.5f * scale), IM_COL32(248, 250, 252, 255), ptxt);
        dl->AddText(fnt, fs_ovr, ImVec2(cx - ts2.x * 0.5f, cy - 0.5f * scale), IM_COL32(241, 245, 249, 255), otxt);
    }

    return clicked_pos;
}

bool draw_stat_row(
    const data::StatDef& s,
    int current_val,
    Lang lang,
    StatRowState& st,
    int& out_preview_val,
    int& out_commit_val,
    ImFont* bold_font
) {
    bool committed = false;
    int disp_val = current_val;
    if (st.dragging && st.drag_fid == s.fid) disp_val = st.drag_cur_val;
    else if (st.hold_fid == s.fid) disp_val = st.hold_cur_val;
    out_preview_val = disp_val;

    ImGui::PushID(s.fid);
    const char* lbl = T(lang, s.pt, s.en);
    const char* desc = T(lang, s.desc_pt, s.desc_en);

    float sc = std::clamp(ImGui::GetFontSize() / 15.0f, 0.85f, 2.5f);
    float avail = ImGui::GetContentRegionAvail().x;
    float row_h = ImGui::GetFrameHeight();
    float num_w = std::round(48.0f * sc);
    float btn_w = std::round(26.0f * sc);
    float btns_w = btn_w * 2.0f + 4.0f * sc;
    float name_w = std::clamp(avail * 0.40f, 130.0f * sc, 200.0f * sc);
    float slider_w = std::max(60.0f * sc, avail - name_w - num_w - btns_w - 26.0f * sc);

    // 1. Stat Name clipped cleanly inside exact name_w column with tooltip
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##lbl_hit", ImVec2(name_w, row_h));
    bool row_hov = ImGui::IsItemHovered();
    show_tip_if_hovered(lbl, desc);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(p0, ImVec2(p0.x + name_w - 2.0f, p0.y + row_h), true);
    ImVec2 lsz = ImGui::CalcTextSize(lbl);
    dl->AddText(ImVec2(p0.x, p0.y + (row_h - lsz.y) * 0.5f), to_u32(row_hov ? col::Text : col::Subtext), lbl);
    dl->PopClipRect();

    // 2. High-Precision Damped Drag Slider
    ImGui::SameLine(0.0f, 8.0f * sc);
    ImVec2 sp = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##sl", ImVec2(slider_w, row_h));
    bool sl_hov = ImGui::IsItemHovered();
    if (sl_hov || (st.dragging && st.drag_fid == s.fid)) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    float frac = static_cast<float>(std::clamp(disp_val, 40, 99) - 40) / 59.0f;
    float thumb_x = sp.x + 8.0f * sc + frac * (slider_w - 16.0f * sc);
    float mid_y = sp.y + row_h * 0.5f;

    if (ImGui::IsItemActivated()) {
        st.dragging = true;
        st.drag_fid = s.fid;
        st.drag_orig_val = current_val;
        st.drag_start_x = ImGui::GetIO().MousePos.x;
        if (std::fabs(st.drag_start_x - thumb_x) > 22.0f * sc && slider_w > 16.0f * sc) {
            float cf = std::clamp((st.drag_start_x - (sp.x + 8.0f * sc)) / (slider_w - 16.0f * sc), 0.0f, 1.0f);
            st.drag_base_val = 40 + static_cast<int>(std::lround(cf * 59.0f));
        } else {
            st.drag_base_val = current_val;
        }
        st.drag_cur_val = st.drag_base_val;
        disp_val = st.drag_cur_val;
        out_preview_val = disp_val;
    }

    if (st.dragging && st.drag_fid == s.fid) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            float px_per_unit = (ImGui::GetIO().KeyShift ? 18.0f : 9.0f) * sc;
            int steps = static_cast<int>(std::lround((ImGui::GetIO().MousePos.x - st.drag_start_x) / px_per_unit));
            st.drag_cur_val = std::clamp(st.drag_base_val + steps, 40, 99);
            disp_val = st.drag_cur_val;
            out_preview_val = disp_val;
        } else {
            st.dragging = false;
            st.drag_fid = -1;
            if (st.drag_cur_val != st.drag_orig_val) {
                committed = true;
                out_commit_val = st.drag_cur_val;
            }
        }
    }

    frac = static_cast<float>(std::clamp(disp_val, 40, 99) - 40) / 59.0f;
    thumb_x = sp.x + 8.0f * sc + frac * (slider_w - 16.0f * sc);
    ImVec4 vcol = ovr_color(disp_val);
    float trk_h = std::round(3.5f * sc);
    dl->AddRectFilled(ImVec2(sp.x, mid_y - trk_h), ImVec2(sp.x + slider_w, mid_y + trk_h), to_u32(col::BgInput), trk_h);
    dl->AddRectFilled(ImVec2(sp.x, mid_y - trk_h), ImVec2(thumb_x, mid_y + trk_h), to_u32(vcol, 0.88f), trk_h);
    dl->AddRect(ImVec2(sp.x, mid_y - trk_h), ImVec2(sp.x + slider_w, mid_y + trk_h), to_u32(col::Border), trk_h);
    float tr = ((sl_hov || (st.dragging && st.drag_fid == s.fid)) ? 7.4f : 6.0f) * sc;
    if (sl_hov || (st.dragging && st.drag_fid == s.fid)) {
        dl->AddCircleFilled(ImVec2(thumb_x, mid_y), tr + 4.2f * sc, to_u32(vcol, 0.28f), 18);
    }
    dl->AddCircleFilled(ImVec2(thumb_x, mid_y), tr, IM_COL32(248, 250, 252, 255), 18);
    dl->AddCircle(ImVec2(thumb_x, mid_y), tr, to_u32(vcol), 18, 2.0f);

    // 3. Direct Editable Numeric Badge
    ImGui::SameLine(0.0f, 8.0f * sc);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(vcol.x, vcol.y, vcol.z, 0.18f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(vcol.x, vcol.y, vcol.z, 0.52f));
    ImGui::PushStyleColor(ImGuiCol_Text, vcol);
    if (bold_font) ImGui::PushFont(bold_font);
    ImGui::SetNextItemWidth(num_w);
    int edit_v = disp_val;
    ImGui::InputInt("##num", &edit_v, 0, 0, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        int cl = std::clamp(edit_v, 40, 99);
        if (cl != current_val) {
            committed = true;
            out_commit_val = cl;
            out_preview_val = cl;
        }
    }
    if (bold_font) ImGui::PopFont();
    ImGui::PopStyleColor(3);

    // 4. Press-and-hold - / + buttons
    ImGui::SameLine(0.0f, 6.0f * sc);
    auto step_btn = [&](const char* sym, int delta) {
        ImGui::Button(sym, ImVec2(btn_w, row_h));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsItemActivated()) {
            st.hold_fid = s.fid;
            st.hold_delta = delta;
            st.hold_start_val = current_val;
            st.hold_cur_val = std::clamp(current_val + delta, 40, 99);
            st.hold_next_t = ImGui::GetTime() + 0.34;
            out_preview_val = st.hold_cur_val;
        }
    };
    step_btn("-", -1);
    ImGui::SameLine(0.0f, 4.0f * sc);
    step_btn("+", 1);

    if (st.hold_fid == s.fid) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            double t = ImGui::GetTime();
            if (t >= st.hold_next_t) {
                st.hold_cur_val = std::clamp(st.hold_cur_val + st.hold_delta, 40, 99);
                st.hold_next_t = t + 0.08;
                out_preview_val = st.hold_cur_val;
            }
        } else {
            int fin = st.hold_cur_val;
            int orig = st.hold_start_val;
            st.hold_fid = -1;
            if (fin != orig) {
                committed = true;
                out_commit_val = fin;
            }
        }
    }

    ImGui::PopID();
    return committed;
}

void ToastManager::push(const std::string& msg, bool is_err, double custom_dur) {
    // Generous read time: 5.5s for normal notices, 8.0s for multi-line economy/stat diffs
    pes::actionlog::write(is_err ? "ERROR" : "INFO", msg);
    bool multi = (msg.find('\n') != std::string::npos);
    double dur = (custom_dur > 0.5) ? custom_dur : (multi ? 8.0 : 5.5);
    items_.push_back({msg, is_err, dur, dur, 0.0});
    if (items_.size() > 2) items_.erase(items_.begin());
}

bool ToastManager::render(double now_sec, ImFont* bold_font) {
    double dt = (last_tick_ > 0.0 && now_sec > last_tick_) ? std::min(0.1, now_sec - last_tick_) : 0.016;
    last_tick_ = now_sec;

    if (items_.empty()) return false;

    float sc = std::clamp(ImGui::GetFontSize() / 15.0f, 0.85f, 2.5f);
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    // Anchor at top-center over the chrome/header band so toasts never cover right-edge action buttons
    float y = vp->Pos.y + 10.0f * sc;
    float center_x = vp->Pos.x + vp->Size.x * 0.5f;
    float max_txt_w = std::min(430.0f * sc, std::max(240.0f * sc, vp->Size.x - 160.0f * sc));
    ImVec2 mouse = ImGui::GetIO().MousePos;
    bool clicked_left = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    int dismiss_idx = -1;
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        auto& it = items_[i];
        it.age += dt;

        ImVec2 ts = ImGui::CalcTextSize(it.msg.c_str(), nullptr, false, max_txt_w);
        float bw = std::min(max_txt_w + 64.0f * sc, std::max(220.0f * sc, ts.x + 64.0f * sc));
        float bh = std::max(40.0f * sc, ts.y + 22.0f * sc);
        ImVec2 rmin(std::round(center_x - bw * 0.5f), std::round(y));
        ImVec2 rmax(std::round(center_x + bw * 0.5f), std::round(y + bh));

        bool hovered = (mouse.x >= rmin.x && mouse.x <= rmax.x && mouse.y >= rmin.y && mouse.y <= rmax.y);
        if (!hovered) {
            it.remaining -= dt;
        } else if (clicked_left) {
            dismiss_idx = i;
        }

        float alpha = 1.0f;
        if (it.age < 0.14) alpha = static_cast<float>(it.age / 0.14);
        else if (it.remaining < 0.35) alpha = std::max(0.0f, static_cast<float>(it.remaining / 0.35));

        ImVec4 accent = it.is_err ? col::Red : col::Green;
        float rnd = 10.0f * sc;
        draw_soft_shadow(dl, rmin, rmax, rnd, alpha * 1.15f);
        dl->AddRectFilled(rmin, rmax, to_u32(ImVec4(col::BgCard.x, col::BgCard.y, col::BgCard.z, 0.98f), alpha), rnd);
        dl->AddRect(rmin, rmax, to_u32(hovered ? col::AccentSoft : col::BorderLit, alpha), rnd, 0, hovered ? 1.4f : 1.0f);
        dl->AddRectFilled(rmin, ImVec2(rmin.x + 4.5f * sc, rmax.y), to_u32(accent, alpha), rnd, ImDrawFlags_RoundCornersLeft);

        // Glowing icon dot + text
        draw_led_dot(dl, ImVec2(rmin.x + 19.0f * sc, rmin.y + bh * 0.48f), 4.0f * sc, accent, alpha > 0.5f);
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                    ImVec2(rmin.x + 33.0f * sc, rmin.y + 10.0f * sc),
                    to_u32(col::Text, alpha), it.msg.c_str(), nullptr, max_txt_w);

        // Subtle close 'x' hint on top-right
        if (bold_font) ImGui::PushFont(bold_font);
        dl->AddText(ImVec2(rmax.x - 18.0f * sc, rmin.y + 7.0f * sc),
                    to_u32(hovered ? col::Text : col::Muted, alpha), "x");
        if (bold_font) ImGui::PopFont();

        // Bottom countdown progress bar (pauses smoothly on hover!)
        float rem_frac = std::clamp(static_cast<float>(it.remaining / std::max(0.1, it.total_dur)), 0.0f, 1.0f);
        float bar_y0 = rmax.y - 3.5f * sc;
        float bar_x0 = rmin.x + 5.0f * sc;
        float bar_w = (bw - 9.0f * sc) * rem_frac;
        dl->AddRectFilled(ImVec2(bar_x0, bar_y0), ImVec2(rmax.x - 4.0f * sc, rmax.y - 1.0f),
                          IM_COL32(255, 255, 255, static_cast<int>(18 * alpha)), 2.0f);
        if (bar_w > 1.0f) {
            dl->AddRectFilled(ImVec2(bar_x0, bar_y0), ImVec2(bar_x0 + bar_w, rmax.y - 1.0f),
                              to_u32(hovered ? col::AccentSoft : accent, alpha * 0.90f), 2.0f);
        }

        y += (bh + 8.0f * sc);
    }

    if (dismiss_idx >= 0 && dismiss_idx < static_cast<int>(items_.size())) {
        items_.erase(items_.begin() + dismiss_idx);
    }
    items_.erase(
        std::remove_if(items_.begin(), items_.end(), [](const ToastItem& it) { return it.remaining <= 0.0; }),
        items_.end()
    );
    return !items_.empty();
}

} // namespace pes::ui
