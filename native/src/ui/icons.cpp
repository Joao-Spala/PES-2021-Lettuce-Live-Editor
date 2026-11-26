#include "icons.hpp"
#include "theme.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <string>

namespace pes::ui {

namespace {

constexpr float kPi = 3.14159265f;

// Pintor em grade 24x24: converte coordenadas da grade para pixels e garante pontas e juncoes arredondadas.
struct Pen {
    ImDrawList* dl;
    ImVec2 o;      // canto superior esquerdo da grade
    float k;       // pixels por unidade da grade
    ImU32 col;
    float t;       // espessura em pixels

    ImVec2 p(float x, float y) const { return ImVec2(o.x + x * k, o.y + y * k); }

    void cap(ImVec2 a) const { if (t > 1.6f) dl->AddCircleFilled(a, t * 0.5f, col, 8); }

    void line(float x1, float y1, float x2, float y2) const {
        dl->AddLine(p(x1, y1), p(x2, y2), col, t);
        cap(p(x1, y1)); cap(p(x2, y2));
    }
    void poly(std::initializer_list<ImVec2> pts, bool closed = false) const {
        ImVec2 prev{}, first{};
        int i = 0;
        for (const ImVec2& q : pts) {
            ImVec2 cur = p(q.x, q.y);
            if (i == 0) first = cur; else { dl->AddLine(prev, cur, col, t); cap(prev); }
            prev = cur; ++i;
        }
        if (closed && i > 2) dl->AddLine(prev, first, col, t);
        cap(prev);
        if (closed) cap(first);
    }
    void circle(float x, float y, float r, bool filled = false) const {
        if (filled) dl->AddCircleFilled(p(x, y), r * k, col, 20);
        else dl->AddCircle(p(x, y), r * k, col, 28, t);
    }
    void rect(float x1, float y1, float x2, float y2, float rnd = 0.0f, bool filled = false) const {
        if (filled) dl->AddRectFilled(p(x1, y1), p(x2, y2), col, rnd * k);
        else dl->AddRect(p(x1, y1), p(x2, y2), col, rnd * k, 0, t);
    }
    void arc(float cx, float cy, float r, float a0, float a1) const {
        dl->PathArcTo(p(cx, cy), r * k, a0, a1, 24);
        dl->PathStroke(col, 0, t);
        cap(p(cx + std::cos(a0) * r, cy + std::sin(a0) * r));
        cap(p(cx + std::cos(a1) * r, cy + std::sin(a1) * r));
    }
    void bez(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) const {
        dl->AddBezierCubic(p(x1, y1), p(x2, y2), p(x3, y3), p(x4, y4), col, t, 20);
        cap(p(x1, y1)); cap(p(x4, y4));
    }
};

} // namespace

void draw_icon(ImDrawList* dl, Icon ic, ImVec2 c, float size, ImU32 col, float stroke_px) {
    Pen g;
    g.dl = dl;
    g.k = size / 24.0f;
    g.o = ImVec2(c.x - size * 0.5f, c.y - size * 0.5f);
    g.col = col;
    g.t = stroke_px > 0.0f ? stroke_px : std::max(1.35f, size * 0.082f);

    switch (ic) {
        case Icon::Dashboard:
            g.rect(3.5f, 3.5f, 10.5f, 10.5f, 2.2f);
            g.rect(13.5f, 3.5f, 20.5f, 10.5f, 2.2f);
            g.rect(3.5f, 13.5f, 10.5f, 20.5f, 2.2f);
            g.rect(13.5f, 13.5f, 20.5f, 20.5f, 2.2f, true);
            break;
        case Icon::Squad:
            g.circle(9.0f, 8.2f, 3.3f);
            g.bez(2.8f, 20.0f, 2.8f, 13.6f, 15.2f, 13.6f, 15.2f, 20.0f);
            g.circle(17.2f, 9.0f, 2.6f);
            g.bez(17.0f, 13.9f, 20.6f, 13.8f, 21.4f, 17.2f, 21.4f, 19.8f);
            break;
        case Icon::Wallet:
            g.rect(3.0f, 7.0f, 21.0f, 20.0f, 3.0f);
            g.poly({ImVec2(5.5f, 7.0f), ImVec2(15.5f, 4.0f), ImVec2(17.5f, 7.0f)});
            g.circle(16.6f, 13.5f, 1.3f, true);
            break;
        case Icon::Jersey:
            g.poly({ImVec2(8.0f, 3.5f), ImVec2(2.8f, 6.8f), ImVec2(5.2f, 11.0f), ImVec2(8.0f, 9.4f),
                    ImVec2(8.0f, 20.5f), ImVec2(16.0f, 20.5f), ImVec2(16.0f, 9.4f), ImVec2(18.8f, 11.0f),
                    ImVec2(21.2f, 6.8f), ImVec2(16.0f, 3.5f)});
            g.bez(8.0f, 3.5f, 9.4f, 6.4f, 14.6f, 6.4f, 16.0f, 3.5f);
            break;
        case Icon::Search:
            g.circle(10.5f, 10.5f, 6.6f);
            g.line(15.4f, 15.4f, 20.8f, 20.8f);
            break;
        case Icon::Eye:
            g.bez(2.2f, 12.0f, 6.0f, 5.6f, 18.0f, 5.6f, 21.8f, 12.0f);
            g.bez(2.2f, 12.0f, 6.0f, 18.4f, 18.0f, 18.4f, 21.8f, 12.0f);
            g.circle(12.0f, 12.0f, 3.1f);
            g.circle(12.0f, 12.0f, 0.9f, true);
            break;
        case Icon::Stopwatch:
            g.circle(12.0f, 13.6f, 7.6f);
            g.line(12.0f, 13.6f, 12.0f, 9.2f);
            g.line(12.0f, 13.6f, 15.0f, 15.4f);
            g.line(9.6f, 3.2f, 14.4f, 3.2f);
            g.line(12.0f, 3.2f, 12.0f, 6.0f);
            g.line(18.6f, 6.4f, 19.9f, 5.1f);
            break;
        case Icon::Dumbbell:
            g.line(7.0f, 12.0f, 17.0f, 12.0f);
            g.rect(4.4f, 7.2f, 7.2f, 16.8f, 1.2f);
            g.rect(16.8f, 7.2f, 19.6f, 16.8f, 1.2f);
            g.line(2.2f, 10.0f, 2.2f, 14.0f);
            g.line(21.8f, 10.0f, 21.8f, 14.0f);
            break;
        case Icon::Crest:
            g.poly({ImVec2(12.0f, 2.8f), ImVec2(19.6f, 5.8f), ImVec2(19.6f, 12.4f), ImVec2(12.0f, 21.2f),
                    ImVec2(4.4f, 12.4f), ImVec2(4.4f, 5.8f)}, true);
            g.poly({ImVec2(8.4f, 10.4f), ImVec2(12.0f, 14.0f), ImVec2(15.6f, 10.4f)});
            break;
        case Icon::Coins:
            g.circle(12.0f, 12.0f, 9.0f);
            g.circle(12.0f, 12.0f, 5.4f);
            g.line(12.0f, 9.6f, 12.0f, 14.4f);
            break;
        case Icon::Banknote:
            g.rect(2.4f, 6.4f, 21.6f, 17.6f, 2.2f);
            g.circle(12.0f, 12.0f, 2.9f);
            g.circle(6.0f, 12.0f, 0.9f, true);
            g.circle(18.0f, 12.0f, 0.9f, true);
            break;
        case Icon::Lock:
            g.rect(5.0f, 11.0f, 19.0f, 21.0f, 2.6f);
            g.arc(12.0f, 11.0f, 4.6f, kPi, 2.0f * kPi);
            g.circle(12.0f, 15.8f, 1.3f, true);
            break;
        case Icon::Undo:
            g.poly({ImVec2(8.8f, 4.4f), ImVec2(4.2f, 9.0f), ImVec2(8.8f, 13.6f)});
            g.bez(4.6f, 9.0f, 14.5f, 6.8f, 20.5f, 10.2f, 19.0f, 18.0f);
            break;
        case Icon::History:
            g.circle(12.0f, 12.0f, 8.6f);
            g.line(12.0f, 12.0f, 12.0f, 7.0f);
            g.line(12.0f, 12.0f, 15.6f, 14.2f);
            break;
        case Icon::Refresh:
            g.arc(12.0f, 12.0f, 7.8f, -0.55f, 4.15f);
            g.poly({ImVec2(14.2f, 2.8f), ImVec2(9.6f, 4.6f), ImVec2(11.9f, 8.7f)});
            break;
        case Icon::Gear: {
            g.circle(12.0f, 12.0f, 3.2f);
            g.circle(12.0f, 12.0f, 7.2f);
            for (int i = 0; i < 8; ++i) {
                float a = i * kPi / 4.0f;
                g.line(12.0f + std::cos(a) * 7.2f, 12.0f + std::sin(a) * 7.2f,
                       12.0f + std::cos(a) * 9.6f, 12.0f + std::sin(a) * 9.6f);
            }
            break;
        }
        case Icon::Export:
            g.line(12.0f, 15.0f, 12.0f, 3.8f);
            g.poly({ImVec2(7.4f, 8.4f), ImVec2(12.0f, 3.8f), ImVec2(16.6f, 8.4f)});
            g.poly({ImVec2(4.0f, 14.0f), ImVec2(4.0f, 20.0f), ImVec2(20.0f, 20.0f), ImVec2(20.0f, 14.0f)});
            break;
        case Icon::Import:
            g.line(12.0f, 3.8f, 12.0f, 15.0f);
            g.poly({ImVec2(7.4f, 10.4f), ImVec2(12.0f, 15.0f), ImVec2(16.6f, 10.4f)});
            g.poly({ImVec2(4.0f, 14.0f), ImVec2(4.0f, 20.0f), ImVec2(20.0f, 20.0f), ImVec2(20.0f, 14.0f)});
            break;
        case Icon::Plus:
            g.line(12.0f, 5.0f, 12.0f, 19.0f);
            g.line(5.0f, 12.0f, 19.0f, 12.0f);
            break;
        case Icon::Minus:
            g.line(5.0f, 12.0f, 19.0f, 12.0f);
            break;
        case Icon::Trash:
            g.line(4.0f, 6.6f, 20.0f, 6.6f);
            g.poly({ImVec2(9.0f, 6.6f), ImVec2(9.0f, 4.0f), ImVec2(15.0f, 4.0f), ImVec2(15.0f, 6.6f)});
            g.poly({ImVec2(6.0f, 6.6f), ImVec2(7.0f, 20.0f), ImVec2(17.0f, 20.0f), ImVec2(18.0f, 6.6f)});
            g.line(10.2f, 10.4f, 10.2f, 16.4f);
            g.line(13.8f, 10.4f, 13.8f, 16.4f);
            break;
        case Icon::Check:
            g.poly({ImVec2(4.6f, 12.6f), ImVec2(10.0f, 18.0f), ImVec2(19.6f, 6.6f)});
            break;
        case Icon::Close:
            g.line(5.6f, 5.6f, 18.4f, 18.4f);
            g.line(18.4f, 5.6f, 5.6f, 18.4f);
            break;
        case Icon::Star: {
            ImVec2 s[10];
            for (int i = 0; i < 10; ++i) {
                float a = -kPi * 0.5f + i * kPi / 5.0f;
                float r = (i % 2 == 0) ? 9.4f : 4.1f;
                s[i] = ImVec2(12.0f + std::cos(a) * r, 12.6f + std::sin(a) * r);
            }
            g.poly({s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9]}, true);
            break;
        }
        case Icon::Bolt:
            g.poly({ImVec2(13.4f, 2.6f), ImVec2(5.4f, 13.2f), ImVec2(11.6f, 13.2f), ImVec2(10.4f, 21.4f),
                    ImVec2(18.6f, 10.4f), ImVec2(12.4f, 10.4f)}, true);
            break;
        case Icon::Cross:
            g.poly({ImVec2(9.0f, 3.4f), ImVec2(15.0f, 3.4f), ImVec2(15.0f, 9.0f), ImVec2(20.6f, 9.0f),
                    ImVec2(20.6f, 15.0f), ImVec2(15.0f, 15.0f), ImVec2(15.0f, 20.6f), ImVec2(9.0f, 20.6f),
                    ImVec2(9.0f, 15.0f), ImVec2(3.4f, 15.0f), ImVec2(3.4f, 9.0f), ImVec2(9.0f, 9.0f)}, true);
            break;
        case Icon::Battery:
            g.rect(2.8f, 7.6f, 19.0f, 16.4f, 2.4f);
            g.line(21.4f, 10.6f, 21.4f, 13.4f);
            g.rect(5.2f, 10.0f, 12.6f, 14.0f, 0.8f, true);
            break;
        case Icon::Contract:
            g.poly({ImVec2(6.0f, 3.2f), ImVec2(14.0f, 3.2f), ImVec2(19.0f, 8.2f), ImVec2(19.0f, 20.8f), ImVec2(6.0f, 20.8f)}, true);
            g.poly({ImVec2(14.0f, 3.2f), ImVec2(14.0f, 8.2f), ImVec2(19.0f, 8.2f)});
            g.line(9.2f, 13.2f, 15.8f, 13.2f);
            g.line(9.2f, 17.0f, 15.8f, 17.0f);
            break;
        case Icon::Copy:
            g.rect(9.0f, 9.0f, 20.0f, 20.0f, 2.2f);
            g.poly({ImVec2(15.0f, 6.0f), ImVec2(15.0f, 5.2f), ImVec2(13.8f, 4.0f), ImVec2(5.6f, 4.0f),
                    ImVec2(4.0f, 5.6f), ImVec2(4.0f, 14.0f), ImVec2(5.2f, 15.0f), ImVec2(6.0f, 15.0f)});
            break;
        case Icon::Move:
            g.line(4.0f, 12.0f, 19.6f, 12.0f);
            g.poly({ImVec2(13.8f, 6.2f), ImVec2(19.6f, 12.0f), ImVec2(13.8f, 17.8f)});
            break;
        case Icon::Filter:
            g.poly({ImVec2(3.4f, 5.0f), ImVec2(20.6f, 5.0f), ImVec2(14.0f, 12.8f), ImVec2(14.0f, 19.6f),
                    ImVec2(10.0f, 17.6f), ImVec2(10.0f, 12.8f)}, true);
            break;
        case Icon::Pulse:
            g.poly({ImVec2(2.0f, 12.0f), ImVec2(6.6f, 12.0f), ImVec2(9.4f, 4.8f), ImVec2(14.2f, 19.4f),
                    ImVec2(16.8f, 12.0f), ImVec2(22.0f, 12.0f)});
            break;
        case Icon::Info:
            g.circle(12.0f, 12.0f, 9.2f);
            g.line(12.0f, 11.0f, 12.0f, 16.8f);
            g.circle(12.0f, 7.8f, 0.95f, true);
            break;
        case Icon::List:
            g.line(8.6f, 6.4f, 20.6f, 6.4f);
            g.line(8.6f, 12.0f, 20.6f, 12.0f);
            g.line(8.6f, 17.6f, 20.6f, 17.6f);
            g.circle(4.4f, 6.4f, 0.9f, true);
            g.circle(4.4f, 12.0f, 0.9f, true);
            g.circle(4.4f, 17.6f, 0.9f, true);
            break;
        case Icon::ArrowUp:
            g.line(12.0f, 20.0f, 12.0f, 5.0f);
            g.poly({ImVec2(6.2f, 10.6f), ImVec2(12.0f, 4.8f), ImVec2(17.8f, 10.6f)});
            break;
        case Icon::ArrowDown:
            g.line(12.0f, 4.0f, 12.0f, 19.0f);
            g.poly({ImVec2(6.2f, 13.4f), ImVec2(12.0f, 19.2f), ImVec2(17.8f, 13.4f)});
            break;
        case Icon::Chip:
            g.rect(6.4f, 6.4f, 17.6f, 17.6f, 2.4f);
            g.rect(9.6f, 9.6f, 14.4f, 14.4f, 1.0f, true);
            for (int i = 0; i < 3; ++i) {
                float v = 9.0f + i * 3.0f;
                g.line(v, 3.0f, v, 6.4f); g.line(v, 17.6f, v, 21.0f);
                g.line(3.0f, v, 6.4f, v); g.line(17.6f, v, 21.0f, v);
            }
            break;
        case Icon::Pin:
            g.poly({ImVec2(12.0f, 21.4f), ImVec2(5.6f, 11.6f), ImVec2(5.6f, 9.0f)});
            g.arc(12.0f, 9.0f, 6.4f, kPi, 2.0f * kPi);
            g.poly({ImVec2(18.4f, 9.0f), ImVec2(18.4f, 11.6f), ImVec2(12.0f, 21.4f)});
            g.circle(12.0f, 9.0f, 2.3f);
            break;
        case Icon::Sliders:
            g.line(4.0f, 7.0f, 20.0f, 7.0f);
            g.line(4.0f, 17.0f, 20.0f, 17.0f);
            g.circle(9.0f, 7.0f, 2.4f, true);
            g.circle(15.0f, 17.0f, 2.4f, true);
            break;
        default: break;
    }
}

// ---------------- botoes com icone ----------------

namespace {

const char* label_start(const char* label) {
    return (label[0] == '+' && label[1] == ' ') ? label + 2 : label;
}

const char* label_end(const char* label) {
    const char* hh = std::strstr(label, "##");
    return hh ? hh : label + std::strlen(label);
}

struct IconBtnMetrics {
    float icon, gap, pad;
};
IconBtnMetrics btn_metrics() {
    float fh = ImGui::GetFrameHeight();
    return {std::round(fh * 0.56f), std::round(fh * 0.28f), ImGui::GetStyle().FramePadding.x};
}

} // namespace

float icon_button_width(const char* label) {
    auto m = btn_metrics();
    const char* e = label_end(label);
    const char* b = label_start(label);
    float tw = (e > b) ? ImGui::CalcTextSize(b, e).x : 0.0f;
    return m.pad + m.icon + (tw > 0.0f ? m.gap + tw : 0.0f) + m.pad;
}

bool icon_button(Icon ic, const char* label, const ImVec4& bg, const ImVec4& border, const ImVec4& text, ImVec2 size) {
    auto m = btn_metrics();
    const char* e = label_end(label);
    const char* b = label_start(label);
    const bool has_text = e > b;
    float tw = has_text ? ImGui::CalcTextSize(b, e).x : 0.0f;
    float w = (size.x > 0.0f) ? size.x : (m.pad + m.icon + (has_text ? m.gap + tw : 0.0f) + m.pad);
    float h = (size.y > 0.0f) ? size.y : ImGui::GetFrameHeight();

    ImVec4 hov(std::min(1.0f, bg.x + 0.055f), std::min(1.0f, bg.y + 0.065f), std::min(1.0f, bg.z + 0.085f),
               bg.w > 0.01f ? std::min(1.0f, bg.w + 0.14f) : 0.18f);
    ImVec4 act(std::min(1.0f, bg.x + 0.09f), std::min(1.0f, bg.y + 0.11f), std::min(1.0f, bg.z + 0.14f),
               bg.w > 0.01f ? std::min(1.0f, bg.w + 0.24f) : 0.28f);
    std::string id = std::string("##") + label;
    ImGui::PushStyleColor(ImGuiCol_Button, bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hov);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, act);
    ImGui::PushStyleColor(ImGuiCol_Border, border);
    bool clicked = ImGui::Button(id.c_str(), ImVec2(w, h));
    bool hovered = ImGui::IsItemHovered();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    ImVec2 pmin = ImGui::GetItemRectMin();
    ImVec2 pmax = ImGui::GetItemRectMax();
    ImGui::PopStyleColor(4);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 col = ImGui::GetColorU32(text);
    float content_w = m.icon + (has_text ? m.gap + tw : 0.0f);
    float x0 = pmin.x + ((pmax.x - pmin.x) - content_w) * 0.5f;
    float cy = (pmin.y + pmax.y) * 0.5f;
    draw_icon(dl, ic, ImVec2(x0 + m.icon * 0.5f, cy), m.icon, col);
    if (has_text) {
        float ty = cy - ImGui::GetTextLineHeight() * 0.5f;
        dl->AddText(ImVec2(x0 + m.icon + m.gap, ty), col, b, e);
    }
    return clicked;
}

bool icon_btn_primary(Icon ic, const char* label, ImVec2 size) {
    return icon_button(ic, label, col::Accent, ImVec4(0.376f, 0.647f, 0.980f, 0.72f), ImVec4(1, 1, 1, 1), size);
}
bool icon_btn_green(Icon ic, const char* label, ImVec2 size) {
    return icon_button(ic, label, ImVec4(0.063f, 0.725f, 0.506f, 0.16f), ImVec4(0.063f, 0.725f, 0.506f, 0.48f), col::GreenSoft, size);
}
bool icon_btn_gold(Icon ic, const char* label, ImVec2 size) {
    return icon_button(ic, label, ImVec4(0.961f, 0.620f, 0.043f, 0.16f), ImVec4(0.961f, 0.620f, 0.043f, 0.48f), col::GoldSoft, size);
}
bool icon_btn_red(Icon ic, const char* label, ImVec2 size) {
    return icon_button(ic, label, ImVec4(0.937f, 0.267f, 0.267f, 0.18f), ImVec4(0.937f, 0.267f, 0.267f, 0.52f), col::RedSoft, size);
}
bool icon_btn_ghost(Icon ic, const char* label, ImVec2 size) {
    return icon_button(ic, label, ImVec4(0, 0, 0, 0), ImVec4(0, 0, 0, 0), col::Subtext, size);
}
bool icon_btn_plain(Icon ic, const char* label, ImVec2 size) {
    return icon_button(ic, label, col::BgCard, col::Border, col::Text, size);
}

bool icon_only_button(Icon ic, const char* id, const ImVec4& bg, const ImVec4& border, const ImVec4& fg, float size) {
    return icon_button(ic, id, bg, border, fg, ImVec2(size, size));
}

} // namespace pes::ui
