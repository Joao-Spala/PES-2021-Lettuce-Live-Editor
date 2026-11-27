#include "workbench_ui.hpp"
#include "../engine/skill_presets.hpp"
#include "../engine/action_log.hpp"
#include "../engine/backup_retention.hpp"
#include "../app_version.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#include "../../third_party/imgui/imgui_stdlib.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>

namespace pes::ui {

namespace {

constexpr UINT kWmStatusUpdated = WM_APP + 1;

// RAII helper for multi-column card grids: aligns outer edges flush with workspace padding
// and maintains a clean, uniform DPI-scaled gutter between cards.
struct ScopedCardGrid {
    bool open = false;
    ScopedCardGrid(const char* id, int cols, float dpi, ImGuiTableFlags extra = ImGuiTableFlags_SizingStretchSame) {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(std::round(8.0f * dpi), std::round(6.0f * dpi)));
        open = ImGui::BeginTable(id, cols, extra | ImGuiTableFlags_NoPadOuterX);
    }
    ~ScopedCardGrid() {
        if (open) ImGui::EndTable();
        ImGui::PopStyleVar();
    }
    explicit operator bool() const { return open; }
};

// RAII helper for stacked label+input table cells: resets PrevLineTextBaseOffset to 0.0f
// with zero added vertical height so Dear ImGui's TableEndCell never shifts columns 1..N down.
struct ScopedFormCell {
    ScopedFormCell() {
        ImGui::TableNextColumn();
    }
    ~ScopedFormCell() {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);
        ImGui::Dummy(ImVec2(0.0f, 0.0f));
    }
};

void copy_str(char* dst, size_t dst_sz, const std::string& src) {
    if (dst_sz == 0) return;
    size_t n = std::min(dst_sz - 1, src.size());
    std::memcpy(dst, src.data(), n);
    dst[n] = '\0';
}

bool status_materially_changed(const StatusSnapshot& a, const StatusSnapshot& b) {
    if (a.attached != b.attached || a.access_denied != b.access_denied ||
        a.conflict_other_wb != b.conflict_other_wb || a.pid != b.pid ||
        a.career != b.career || a.in_match != b.in_match || a.mode_code != b.mode_code ||
        a.club != b.club || a.training_mode != b.training_mode || a.stamina_mode != b.stamina_mode ||
        a.freeze_clock != b.freeze_clock || a.bal_focus != b.bal_focus || a.bal_lock != b.bal_lock ||
        a.hooks != b.hooks || a.has_player != b.has_player || a.has_finances != b.has_finances ||
        a.val_lock_active != b.val_lock_active || !(a.val_lock == b.val_lock) ||
        a.val_lock_addr != b.val_lock_addr || a.val_lock_lost != b.val_lock_lost ||
        a.val_lock_name != b.val_lock_name || a.target_exe != b.target_exe || a.target_unsupported != b.target_unsupported ||
        a.compat_missing != b.compat_missing) {
        return true;
    }
    if (a.has_finances && (a.finances.transfer_raw != b.finances.transfer_raw ||
                           a.finances.salary_avail_raw != b.finances.salary_avail_raw)) {
        return true;
    }
    if (a.has_player && (a.player.addr != b.player.addr || a.player.ovr != b.player.ovr ||
                         a.player.pos != b.player.pos || a.player.stamina != b.player.stamina ||
                         a.player.arrow != b.player.arrow || a.player.blink != b.player.blink || a.player.injury != b.player.injury ||
                         a.player.age != b.player.age || a.player.undo_count != b.player.undo_count ||
                         a.player.name != b.player.name)) {
        return true;
    }
    return false;
}

void card_header(const char* title, ImFont* bold, const char* tip = nullptr) {
    if (bold) ImGui::PushFont(bold);
    ImGui::TextColored(col::Text, "%s", title);
    if (bold) ImGui::PopFont();
    if (tip && tip[0]) show_tip_if_hovered(title, tip);
}

void page_title(const char* title, const FontBank& fonts, ImFont* bold) {
    ImFont* tf = fonts.title_bold ? fonts.title_bold : bold;
    if (tf) ImGui::PushFont(tf);
    ImGui::TextColored(col::Text, "%s", title);
    if (tf) ImGui::PopFont();
}

void section_label(const char* txt, const FontBank& fonts) {
    if (fonts.small_bold) ImGui::PushFont(fonts.small_bold);
    ImGui::TextColored(col::Muted, "%s", txt);
    if (fonts.small_bold) ImGui::PopFont();
}

// Corta em n bytes sem partir um caractere UTF-8 no meio
std::string utf8_prefix(const std::string& s, size_t n) {
    if (s.size() <= n) return s;
    size_t cut = n;
    while (cut > 0 && (static_cast<unsigned char>(s[cut]) & 0xC0) == 0x80) --cut;
    return s.substr(0, cut);
}

// Texto descritivo colorido que quebra linha no espaco disponivel (nunca corta em janelas estreitas)
void wrapped_colored(const ImVec4& c, const char* fmt, const char* txt) {
    (void)fmt;
    ImGui::PushStyleColor(ImGuiCol_Text, c);
    ImGui::TextWrapped("%s", txt);
    ImGui::PopStyleColor();
}

float calc_btn_w(const char* label) {
    const char* hash = std::strstr(label, "##");
    ImVec2 ts = hash ? ImGui::CalcTextSize(label, hash) : ImGui::CalcTextSize(label);
    return ts.x + ImGui::GetStyle().FramePadding.x * 2.0f + 2.0f;
}

// Wrap horizontally across available width before placing next button/chip
void flow_next(float item_w, float gap = 8.0f) {
    float cur_right = ImGui::GetItemRectMax().x;
    float win_right = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
    if (cur_right + gap + item_w <= win_right) {
        ImGui::SameLine(0.0f, gap);
    }
}

// Right-align an action group of known total width if space permits, else wrap cleanly
void align_right_or_wrap(float group_w) {
    float cur_end = ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x;
    float right_edge = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
    if (right_edge - group_w > cur_end + 12.0f) {
        ImGui::SameLine(right_edge - group_w);
    }
}

const data::StatDef* find_stat_def(int fid) {
    for (const auto& s : data::STAT_DEFS) if (s.fid == fid) return &s;
    return nullptr;
}

std::string cur_hms() {
    std::time_t t = std::time(nullptr);
    std::tm tm_buf{};
    localtime_s(&tm_buf, &t);
    char b[16];
    std::snprintf(b, sizeof(b), "%02d:%02d:%02d", tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
    return b;
}

std::wstring utf8_to_wide(std::string_view s) {
    if (s.empty()) return {};
    int need = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (need <= 0) return {};
    std::wstring out(static_cast<size_t>(need), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), need);
    return out;
}

std::string wide_to_utf8(std::wstring_view w) {
    if (w.empty()) return {};
    int need = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (need <= 0) return {};
    std::string out(static_cast<size_t>(need), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), out.data(), need, nullptr, nullptr);
    return out;
}

std::string pick_open_json_dialog(HWND owner, const std::string& init_dir) {
    wchar_t buf[MAX_PATH * 2]{};
    std::wstring wdir = utf8_to_wide(init_dir);
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Lettuce Pack (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = static_cast<DWORD>(std::size(buf));
    ofn.lpstrInitialDir = wdir.empty() ? nullptr : wdir.c_str();
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&ofn)) return wide_to_utf8(buf);
    return {};
}

std::string pick_save_json_dialog(HWND owner, const std::string& init_dir, const std::string& def_name) {
    wchar_t buf[MAX_PATH * 2]{};
    std::wstring wdef = utf8_to_wide(def_name);
    if (!wdef.empty()) {
        size_t n = std::min(wdef.size(), std::size(buf) - 1);
        std::wmemcpy(buf, wdef.data(), n);
        buf[n] = L'\0';
    }
    std::wstring wdir = utf8_to_wide(init_dir);
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Lettuce Pack (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrDefExt = L"json";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = static_cast<DWORD>(std::size(buf));
    ofn.lpstrInitialDir = wdir.empty() ? nullptr : wdir.c_str();
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetSaveFileNameW(&ofn)) return wide_to_utf8(buf);
    return {};
}

} // namespace

WorkbenchApp::WorkbenchApp(PESEngine& engine) : engine_(engine) {
    settings_ = load_settings();
    engine_.set_econ_sync_mode(static_cast<EconSyncMode>(std::clamp(settings_.econ_sync_mode, 0, 3)));
    actionlog::set_directory(std::filesystem::path(utf8_to_wide(get_logs_directory())));
    actionlog::set_enabled(settings_.action_log);
    apply_backup_retention();
    engine_.set_target_process(settings_.game_process, 0);   // processo escolhido a mao em outra sessao (vazio = automatico)
}

WorkbenchApp::~WorkbenchApp() {
    stop_background_poller();
}

void WorkbenchApp::set_window_handle(void* hwnd_raw, float dpi_scale) {
    hwnd_ = hwnd_raw;
    dpi_scale_ = std::clamp(dpi_scale, 0.85f, 2.5f);
    titlebar_h_ = std::round(38.0f * dpi_scale_);
}

void WorkbenchApp::start_background_poller(void* hwnd_raw) {
    notify_hwnd_ = hwnd_raw;
    poller_running_.store(true);
    poller_thread_ = std::thread([this]() {
        StatusSnapshot prev{};
        bool first = true;
        while (poller_running_.load(std::memory_order_relaxed)) {
            StatusSnapshot snap = engine_.get_status();
            bool changed = first || status_materially_changed(prev, snap);
            first = false;
            if (changed) {
                if (prev.seq == 0 || snap.attached != prev.attached) {
                    actionlog::write("INFO", snap.attached ? std::string("Game attached: ") + snap.exe : std::string("Game not connected"));
                }
                prev = snap;
                {
                    std::lock_guard<std::mutex> g(status_mtx_);
                    latest_status_ = std::move(snap);
                }
                if (notify_hwnd_) {
                    PostMessageW(static_cast<HWND>(notify_hwnd_), kWmStatusUpdated, 0, 0);
                }
            }
            // Fast 250ms polling when active (C++ get_status takes <0.05ms), 2000ms when minimized/offline
            int sleep_ms = (minimized_.load(std::memory_order_relaxed) || !prev.attached) ? 2000 : 250;
            // Com trava ativa o ciclo nao passa de 500 ms, mesmo com o app minimizado (a trava so escreve ao dar o ciclo)
            if (prev.val_lock_active) sleep_ms = std::min(sleep_ms, 500);
            std::unique_lock<std::mutex> lk(poll_cv_mtx_);
            poll_cv_.wait_for(lk, std::chrono::milliseconds(sleep_ms), [this]() {
                return !poller_running_.load(std::memory_order_relaxed) || poll_wake_flag_;
            });
            poll_wake_flag_ = false;
        }
    });
}

void WorkbenchApp::stop_background_poller() {
    poller_running_.store(false);
    {
        std::lock_guard<std::mutex> lk(poll_cv_mtx_);
        poll_wake_flag_ = true;
    }
    poll_cv_.notify_all();
    if (poller_thread_.joinable()) poller_thread_.join();
}

void WorkbenchApp::wake_poller() {
    {
        std::lock_guard<std::mutex> lk(poll_cv_mtx_);
        poll_wake_flag_ = true;
    }
    poll_cv_.notify_all();
}

bool WorkbenchApp::is_point_in_titlebar_caption(int client_x, int client_y) const {
    float x = static_cast<float>(client_x);
    float y = static_cast<float>(client_y);
    if (y < 0.0f || y > titlebar_h_) return false;
    for (const auto& r : titlebar_interactive_) {
        if (x >= r.x0 && x <= r.x1 && y >= r.y0 && y <= r.y1) return false;
    }
    return true;
}

const char* WorkbenchApp::view_label(View v) const {
    Lang L = settings_.lang;
    switch (v) {
        case View::Dashboard: return T(L, "Visão Geral", "Overview");
        case View::Squad:     return T(L, "Meu Elenco", "My Squad");
        case View::Finances:  return T(L, "Finanças", "Finances");
        case View::Player:    return T(L, "Jogador Atual", "Current Player");
        case View::Search:    return T(L, "Buscar Jogador", "Search Player");
        case View::Scouts:    return T(L, "Olheiros & Favoritos", "Scouts & Shortlist");
        case View::Match:     return T(L, "Partida Ao Vivo", "Live Match");
        case View::Training:  return T(L, "Treino & BAL", "Training & BAL");
    }
    return "";
}

void WorkbenchApp::switch_view(View v, bool skip_auto_load, bool from_back) {
    if (!from_back && v != current_view_) {
        nav_history_.push_back(current_view_);
        if (nav_history_.size() > 20) nav_history_.erase(nav_history_.begin());
    }
    current_view_ = v;
    if (skip_auto_load) return;
    if (v == View::Squad) reload_squad();
    else if (v == View::Player) reload_player();
    else if (v == View::Finances) reload_finances();
    else if (v == View::Scouts || v == View::Search) reload_scouts();
}

void WorkbenchApp::trigger_back() {
    if (nav_history_.empty()) return;
    View prev = nav_history_.back();
    nav_history_.pop_back();
    switch_view(prev, false, true);
}

void WorkbenchApp::trigger_undo() {
    auto r = engine_.undo_last();
    if (r.ok) {
        toasts_.push(T(settings_.lang, "Alteração desfeita!", "Change undone!"));
        if (current_view_ == View::Player) reload_player();
        else if (current_view_ == View::Squad) reload_squad();
        else if (current_view_ == View::Finances) reload_finances();
        wake_poller();
    } else {
        toast_code(r.code, true);
    }
}

void WorkbenchApp::open_command_palette() {
    open_cmd_modal_ = true;
    cmd_filter_[0] = '\0';
    cmd_selected_ = 0;
}

void WorkbenchApp::toast_code(const std::string& code, bool is_err, int orig, int fin, int pri, int pos, int reg_pos, bool needs_grade) {
    toasts_.push(fmt_code_msg(settings_.lang, code, orig, fin, pri, pos, reg_pos, needs_grade), is_err);
}

std::string WorkbenchApp::fmt_econ_delta_line(const EconSyncDelta& ed) const {
    if (!ed.applied) return {};
    Lang L = settings_.lang;
    std::string out;
    auto add_part = [&](const char* lbl, int32_t ov, int32_t nv) {
        if (ov == nv) return;
        if (!out.empty()) out += " | ";
        out += lbl;
        out += ": ";
        out += fmt_money(ov, settings_.currency_idx, L);
        out += " -> ";
        out += fmt_money(nv, settings_.currency_idx, L);
    };
    add_part(T(L, "Mercado", "Market"), ed.old_market_raw, ed.new_market_raw);
    add_part(T(L, "Salário", "Salary"), ed.old_salary_raw, ed.new_salary_raw);
    if (ed.has_release_fee) {
        add_part(T(L, "Rescisão", "Release"), ed.old_release_raw, ed.new_release_raw);
    }
    return out;
}

void WorkbenchApp::capture_last_edit(const PlayerDetails& r, const char* lbl_pt, const char* lbl_en) {
    if (!r.ok) return;
    bool has_ovr = (r.action_orig > 0 && r.action_final > 0 && r.action_orig != r.action_final);
    bool has_econ = r.econ_delta.applied && (
        r.econ_delta.old_market_raw != r.econ_delta.new_market_raw ||
        r.econ_delta.old_salary_raw != r.econ_delta.new_salary_raw ||
        (r.econ_delta.has_release_fee && r.econ_delta.old_release_raw != r.econ_delta.new_release_raw));
    bool has_diffs = !r.action_diffs.empty();
    if (!has_ovr && !has_econ && !has_diffs) return;

    last_edit_.active = true;
    last_edit_.player_addr = r.addr;
    last_edit_.time_str = cur_hms();
    last_edit_.label_pt = lbl_pt ? lbl_pt : "Edição";
    last_edit_.label_en = lbl_en ? lbl_en : "Edit";
    last_edit_.old_ovr = r.action_orig;
    last_edit_.new_ovr = r.action_final;
    last_edit_.econ = r.econ_delta;
    last_edit_.diffs = r.action_diffs;
}

void WorkbenchApp::record_solver_action(const PlayerDetails& r, const char* tag_pt, const char* tag_en) {
    Lang L = settings_.lang;
    capture_last_edit(r, tag_pt, tag_en);
    std::string head = fmt_code_msg(L, r.code, r.action_orig, r.action_final, r.action_pri, r.action_pos, r.action_reg_pos, r.action_needs_grade);
    if (!r.action_diffs.empty()) {
        PosLabLogEntry e;
        e.time_str = cur_hms();
        e.pos_id = (r.action_pos >= 0) ? r.action_pos : r.reg_pos;
        e.reg_pos = (r.action_reg_pos >= 0) ? r.action_reg_pos : r.reg_pos;
        e.orig_ovr = r.action_orig;
        e.final_ovr = r.action_final;
        e.pri_ovr = (r.action_pri > 0) ? r.action_pri : r.ovr;
        e.tag_pt = tag_pt;
        e.tag_en = tag_en;
        e.diffs = r.action_diffs;
        poslab_log_.insert(poslab_log_.begin(), std::move(e));
        if (poslab_log_.size() > 30) poslab_log_.resize(30);

        std::string extra;
        size_t lim = std::min<size_t>(3, r.action_diffs.size());
        for (size_t i = 0; i < lim; ++i) {
            const auto& df = r.action_diffs[i];
            const auto* sd = find_stat_def(df.fid);
            const char* sn = sd ? T(L, sd->pt, sd->en) : "?";
            int delta = df.new_val - df.old_val;
            char seg[64];
            std::snprintf(seg, sizeof(seg), "%s%s %+d", (i > 0 ? ", " : ""), sn, delta);
            extra += seg;
        }
        if (r.action_diffs.size() > 3) {
            char more[24];
            std::snprintf(more, sizeof(more), " (+%zu)", r.action_diffs.size() - 3);
            extra += more;
        }
        head += "\n" + extra;
    }
    if (r.econ_delta.applied) {
        std::string econ_line = fmt_econ_delta_line(r.econ_delta);
        if (!econ_line.empty()) head += "\n" + econ_line;
    }
    // In Position Lab, inline "Última alteração" + persistent "Última Edição" strip already show every diff;
    // avoid spamming floating toasts on successful PosLab steps so buttons stay 100% unobstructed.
    bool suppress_poslab_toast = (current_view_ == View::Player && current_ptab_ == PlayerTab::PosLab && r.action_ok);
    if (!suppress_poslab_toast) {
        toasts_.push(head, !r.action_ok);
    }
}

const char* WorkbenchApp::log_tag(Lang L, const std::string& pt, const std::string& en, const std::string& es) {
    if (L == Lang::ES && !es.empty()) return es.c_str();
    return T(L, pt.c_str(), en.c_str());
}

void WorkbenchApp::record_meta_action(int pos_id, int orig_ovr, int final_ovr, const std::string& note_pt, const std::string& note_en, const std::string& note_es) {
    PosLabLogEntry e;
    e.time_str = cur_hms();
    e.pos_id = pos_id;
    e.reg_pos = player_.reg_pos;
    e.orig_ovr = orig_ovr;
    e.final_ovr = final_ovr;
    e.pri_ovr = player_.ovr;
    e.tag_pt = note_pt;
    e.tag_en = note_en;
    e.tag_es = note_es;
    poslab_log_.insert(poslab_log_.begin(), std::move(e));
    if (poslab_log_.size() > 30) poslab_log_.resize(30);
}

void WorkbenchApp::open_player_in_studio(uint64_t addr) {
    if (engine_.select_player(addr)) {
        highlighted_pos_ = -1;
        switch_view(View::Player, true);
        reload_player(addr);
        wake_poller();
    }
}

void WorkbenchApp::reload_player(uint64_t addr) {
    PlayerDetails d = engine_.get_player_details(addr);
    if (!d.ok) {
        has_player_details_ = false;
        return;
    }
    bool switched_player = (!has_player_details_ || d.addr != player_.addr);
    player_ = std::move(d);
    has_player_details_ = true;
    last_player_addr_ = player_.addr;
    if (switched_player) {
        highlighted_pos_ = -1;
        target_ovr_inp_ = std::min(99, player_.ovr + 1);
        std::snprintf(target_ovr_buf_, sizeof(target_ovr_buf_), "%d", target_ovr_inp_);
        poslab_base_stats_ = player_.stats;
        poslab_base_valid_ = true;
        poslab_log_.clear();
        last_edit_ = {};
    }
    sync_player_form_buffers();
}

void WorkbenchApp::sync_player_form_buffers() {
    copy_str(bio_name_, sizeof(bio_name_), player_.name);
    copy_str(bio_shirt_, sizeof(bio_shirt_), player_.shirt);
    bio_nat_ = player_.nationality;
    bio_age_ = player_.age;
    bio_height_ = player_.height;
    bio_weight_ = player_.weight;
    bio_foot_ = player_.foot;
    bio_contract_ = player_.contract_yr;
    copy_str(bio_salary_, sizeof(bio_salary_), fmt_money(player_.salary_raw, settings_.currency_idx, settings_.lang));
    copy_str(bio_market_, sizeof(bio_market_), fmt_money(player_.market_raw, settings_.currency_idx, settings_.lang));
    bio_injury_ = player_.injury;
    bio_wfu_ = player_.wf_usage;
    bio_wfa_ = player_.wf_acc;
    bio_form_ = player_.form_cons;
    bio_injres_ = player_.inj_res;
    bio_tlist_ = player_.transfer_listed;
    bio_llist_ = player_.loan_listed;
    bio_has_rf_ = player_.has_release_fee;
    bio_affection_ = player_.affection;
    quick_stam_ = player_.stamina;
    edit_role_id_ = player_.team_role;
    edit_role_lvl_ = player_.team_role_lvl;
}

void WorkbenchApp::reload_squad() {
    squad_ = engine_.get_squad_roster();
}

void WorkbenchApp::reload_finances() {
    fin_ = engine_.get_finances();
    if (fin_.ok) {
        copy_str(fin_trf_buf_, sizeof(fin_trf_buf_), fmt_money(fin_.transfer_raw, settings_.currency_idx, settings_.lang));
        copy_str(fin_sal_buf_, sizeof(fin_sal_buf_), fmt_money(fin_.salary_avail_raw, settings_.currency_idx, settings_.lang));
    }
}

void WorkbenchApp::reload_scouts() {
    scouts_ = engine_.get_scout_lists();
    scout_mask_.clear();
    for (const auto& l : scouts_.lists) {
        for (const auto& e : l.entries) scout_mask_[e.addr] |= static_cast<uint8_t>(1u << (l.slot - 1));
    }
}

uint8_t WorkbenchApp::scout_mask_of(uint64_t addr) const {
    auto it = scout_mask_.find(addr);
    return it == scout_mask_.end() ? 0 : it->second;
}

void WorkbenchApp::scout_report(const ActionResult& r, const char* verb_pt, const char* verb_en, int target_slot) {
    Lang L = settings_.lang;
    const char* lname = "";
    for (const auto& sl : data::SCOUT_LISTS) if (sl.slot == target_slot) { lname = T(L, sl.pt, sl.en); break; }
    if (!r.ok) {
        if (r.code == "scout_full") toasts_.push(T(L, "Lista de destino cheia. Nada foi alterado.", "Target list is full. Nothing changed."), true);
        else if (r.code == "scout_dup") toasts_.push(T(L, "Esses atletas já estão na lista.", "Those players are already on the list."), true);
        else toast_code(r.code, true);
        return;
    }
    char msg[256];
    std::snprintf(msg, sizeof(msg), T(L, "%d atleta(s) %s%s%s%s", "%d player(s) %s%s%s%s"),
                  r.count, T(L, verb_pt, verb_en), lname[0] ? " - " : "", lname,
                  r.flag ? T(L, ". Lista cheia: alguns ficaram de fora.", ". List full: some were left out.") : "");
    toasts_.push(msg);
}

void WorkbenchApp::render_scout_list_menu(const char* popup_id, uint64_t addr) {
    Lang L = settings_.lang;
    if (!ImGui::BeginPopup(popup_id)) return;
    ImGui::TextColored(col::Muted, "%s", T(L, "Listas deste atleta", "This player's lists"));
    ImGui::Spacing();
    const uint8_t mask = scout_mask_of(addr);
    int toggle_slot = 0;
    bool was_in = false;
    for (const auto& sl : data::SCOUT_LISTS) {
        bool in_l = (mask & (1u << (sl.slot - 1))) != 0;
        char lbl[96];
        std::snprintf(lbl, sizeof(lbl), "%s##mn%d", T(L, sl.pt, sl.en), sl.slot);
        if (ImGui::Checkbox(lbl, &in_l)) { toggle_slot = sl.slot; was_in = !in_l; }
    }
    ImGui::EndPopup();
    if (toggle_slot == 0) return;
    if (was_in) {
        uint64_t handle = 0;
        for (const auto& l : scouts_.lists) if (l.slot == toggle_slot) for (const auto& e : l.entries) if (e.addr == addr) handle = e.handle;
        if (handle) scout_report(engine_.remove_from_scout_list(toggle_slot, handle), "removido(s) da lista", "removed from list", toggle_slot);
    } else {
        scout_report(engine_.add_to_scout_list(toggle_slot, addr), "adicionado(s) a", "added to", toggle_slot);
    }
    reload_scouts();
}

void WorkbenchApp::run_search() {
    search_res_ = engine_.search_players(srch_query_, srch_pos_, srch_min_ovr_, srch_max_age_, srch_free_, 150);
    if (!search_res_.ok) toast_code(search_res_.code, true);
}

void WorkbenchApp::run_squad_cmd(const char* mode) { run_squad_cmd_impl(mode, nullptr); }

void WorkbenchApp::run_squad_cmd_sel(const char* mode) {
    std::vector<uint64_t> v;
    for (const auto& p : squad_.players) if (squad_sel_.count(p.addr)) v.push_back(p.addr);
    if (v.empty()) return;
    run_squad_cmd_impl(mode, &v);
}

void WorkbenchApp::run_squad_cmd_impl(const char* mode, const std::vector<uint64_t>* only) {
    auto r = engine_.run_squad_commander(mode, only);
    if (r.ok) {
        char msg[256];
        std::snprintf(msg, sizeof(msg), T(settings_.lang, "Aplicado a %d atletas do elenco!%s", "Applied to %d squad players!%s"), r.count,
                      r.flag ? T(settings_.lang, " Backup do elenco salvo em Packs\\Backups.", " Squad backup saved in Packs\\Backups.") : "");
        toasts_.push(msg);
        if (current_view_ == View::Squad) squad_reload_pending_ = true; // recarrega depois da tabela (rows aponta para squad_)
        if (current_view_ == View::Player) reload_player();
        wake_poller();
    } else {
        toast_code(r.code, true);
    }
}

bool WorkbenchApp::render_frame(const FontBank& fonts) {
    start_services_once();
    // Pull latest status snapshot from background thread (lock held <1us)
    {
        std::lock_guard<std::mutex> g(status_mtx_);
        if (latest_status_.seq != last_seen_seq_) {
            last_seen_seq_ = latest_status_.seq;
            st_ = latest_status_;
            if (st_.career != last_career_) {
                last_career_ = st_.career;
                bal_dirty_ = false;
            }
            if (!bal_dirty_ && st_.career > 0) {
                bal_focus_inp_ = st_.bal_focus;
                bal_lock_chk_ = st_.bal_lock;
            }
            if (st_.has_player) {
                bool addr_diff = (st_.player.addr != last_player_addr_);
                bool live_changed = has_player_details_ && (
                    st_.player.ovr != player_.ovr ||
                    st_.player.stamina != player_.stamina ||
                    st_.player.arrow != player_.arrow || st_.player.blink != player_.blink ||
                    st_.player.injury != player_.injury);
                if (addr_diff) {
                    last_player_addr_ = st_.player.addr;
                    if (current_view_ == View::Player) reload_player(st_.player.addr);
                } else if (live_changed && current_view_ == View::Player && !ImGui::GetIO().WantTextInput) {
                    reload_player(st_.player.addr);
                }
            } else {
                last_player_addr_ = 0;
                has_player_details_ = false;
            }
            if (st_.in_match && !prev_in_match_ && st_.match_clock_valid) {
                std::snprintf(match_min_buf_, sizeof(match_min_buf_), "%02d", st_.match_min);
                std::snprintf(match_sec_buf_, sizeof(match_sec_buf_), "%02d", st_.match_sec);
            }
            prev_in_match_ = st_.in_match;
        }
    }

    if (st_.val_lock_lost != last_lock_lost_) {
        const bool more = st_.val_lock_lost > last_lock_lost_;
        last_lock_lost_ = st_.val_lock_lost;
        if (more) {
            lock_ui_ = ValueLockConfig{};   // a trava caiu: a tela volta ao estado desligado
            lock_ui_addr_ = 0;
            toasts_.push(T(settings_.lang,
                "Travas desligadas: o jogo fechou ou o atleta travado mudou.",
                "Locks turned off: the game closed or the locked player changed."), true);
        }
    }

    // Global keyboard shortcuts (disabled while any modal popup is open)
    ImGuiIO& io = ImGui::GetIO();
    const bool any_popup = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId);
    if (!any_popup) {
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_K, false)) open_command_palette();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false) && !io.WantTextInput) trigger_undo();
        if (io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false) && !io.WantTextInput) trigger_back();
        if (io.KeyAlt && !io.WantTextInput) {
            if (ImGui::IsKeyPressed(ImGuiKey_1, false)) switch_view(View::Dashboard);
            else if (ImGui::IsKeyPressed(ImGuiKey_2, false)) switch_view(View::Squad);
            else if (ImGui::IsKeyPressed(ImGuiKey_3, false)) switch_view(View::Finances);
            else if (ImGui::IsKeyPressed(ImGuiKey_4, false)) switch_view(View::Player);
            else if (ImGui::IsKeyPressed(ImGuiKey_5, false)) switch_view(View::Search);
            else if (ImGui::IsKeyPressed(ImGuiKey_6, false)) switch_view(View::Scouts);
            else if (ImGui::IsKeyPressed(ImGuiKey_7, false)) switch_view(View::Match);
            else if (ImGui::IsKeyPressed(ImGuiKey_8, false)) switch_view(View::Training);
        }
        if (io.KeyCtrl && !io.WantTextInput && current_view_ == View::Player) {
            if (ImGui::IsKeyPressed(ImGuiKey_1, false)) current_ptab_ = PlayerTab::Bio;
            else if (ImGui::IsKeyPressed(ImGuiKey_2, false)) current_ptab_ = PlayerTab::Stats;
            else if (ImGui::IsKeyPressed(ImGuiKey_3, false)) current_ptab_ = PlayerTab::PosLab;
            else if (ImGui::IsKeyPressed(ImGuiKey_4, false)) current_ptab_ = PlayerTab::Skills;
            else if (ImGui::IsKeyPressed(ImGuiKey_5, false)) current_ptab_ = PlayerTab::Role;
        }
    }

    int fi = std::clamp(settings_.font_idx, 0, fonts.count - 1);
    ImFont* f_reg = fonts.regular[fi] ? fonts.regular[fi] : ImGui::GetFont();
    ImFont* f_bold = fonts.bold[fi] ? fonts.bold[fi] : f_reg;

    ImGui::PushFont(f_reg);

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGuiWindowFlags root_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::Begin("##WorkbenchRoot", nullptr, root_flags);
    ImGui::PopStyleVar(2);

    // 1. Custom Frameless Studio Titlebar (top 38px * dpi)
    const float tb_h = titlebar_h_;
    render_titlebar(fonts, tb_h);

    const float body_h = std::max(100.0f, vp->Size.y - tb_h);
    const bool auto_compact = (vp->Size.x < 920.0f * dpi_scale_);
    const bool compact_side = auto_compact || settings_.sidebar_collapsed;
    const float side_w = compact_side ? std::round(64.0f * dpi_scale_) : std::round(220.0f * dpi_scale_);
    const float top_h = std::round(52.0f * dpi_scale_);

    // 2. Left Navigation Sidebar
    ImGui::PushStyleColor(ImGuiCol_ChildBg, col::BgSidebar);
    ImGui::PushStyleColor(ImGuiCol_Border, col::Border);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(std::round((compact_side ? 10.0f : 14.0f) * dpi_scale_), std::round(16.0f * dpi_scale_)));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::BeginChild("##Sidebar", ImVec2(side_w, body_h),
                      ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding, 0);
    render_sidebar(fonts, compact_side, auto_compact);
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    // 3. Right column: Sub-Toolbar + Scrollable Main Workspace
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginGroup();

    const float right_w = std::max(200.0f, vp->Size.x - side_w);
    const float ws_pad_x = std::round(24.0f * dpi_scale_);
    const float ws_pad_y = std::round(20.0f * dpi_scale_);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, col::BgSurface);
    ImGui::PushStyleColor(ImGuiCol_Border, col::Border);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ws_pad_x, std::round(8.0f * dpi_scale_)));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::BeginChild("##Topbar", ImVec2(right_w, top_h),
                      ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
    render_topbar(fonts);
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, col::BgCanvas);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ws_pad_x, ws_pad_y));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    // Barra de rolagem sempre reservada: a largura util nunca muda quando o conteudo cresce ou encolhe
    ImGui::BeginChild("##MainWorkspace", ImVec2(right_w, std::max(100.0f, body_h - top_h)),
                      ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    draw_workspace_ambient_glow(ImGui::GetWindowDrawList(), ImGui::GetWindowPos(), ImGui::GetWindowSize());

    // Dual-Workbench hook conflict warning banner
    if (st_.conflict_other_wb) {
        Lang L = settings_.lang;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.961f, 0.620f, 0.043f, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.961f, 0.620f, 0.043f, 0.55f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(std::round(16.0f * dpi_scale_), std::round(12.0f * dpi_scale_)));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, std::round(10.0f * dpi_scale_));
        ImGui::BeginChild("##conflict_banner", ImVec2(0, 0),
                          ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
        if (f_bold) ImGui::PushFont(f_bold);
        ImGui::TextColored(col::GoldSoft, "%s",
            T(L, "Aviso de conflito de hooks", "Hook conflict warning"));
        if (f_bold) ImGui::PopFont();
        ImGui::TextWrapped("%s",
            T(L, "A versão antiga (PES 2021 Workbench.exe) está aberta ao mesmo tempo e disputa a captura de jogador no Plano de Jogo. Feche a versão antiga para captura perfeita.",
                 "The legacy version (PES 2021 Workbench.exe) is running concurrently and competing for Game Plan player capture. Close the legacy app for reliable capture."));
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        ImGui::Dummy(ImVec2(0, std::round(10.0f * dpi_scale_)));
    }

    render_update_banner(fonts);

    switch (current_view_) {
        case View::Dashboard: render_view_dashboard(fonts); break;
        case View::Squad:     render_view_squad(fonts); break;
        case View::Finances:  render_view_finances(fonts); break;
        case View::Player:    render_view_player(fonts); break;
        case View::Search:    render_view_search(fonts); break;
        case View::Scouts:    render_view_scouts(fonts); break;
        case View::Match:     render_view_match(fonts); break;
        case View::Training:  render_view_training(fonts); break;
    }
    ImGui::Dummy(ImVec2(0, 16));
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    ImGui::EndGroup();

    render_modals(fonts);
    render_process_picker(fonts);

    ImGui::End();
    ImGui::PopFont();

    double now = ImGui::GetTime();
    bool has_toasts = toasts_.render(now, f_bold);
    bool animating = has_toasts || widgets_is_animating() || stat_row_st_.dragging || stat_row_st_.hold_fid >= 0 ||
                     (all99_armed_until_ > now) || (clear_scout_armed_until_ > now);
    return animating;
}

void WorkbenchApp::render_titlebar(const FontBank& fonts, float tb_h) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];
    titlebar_interactive_.clear();

    auto track_interactive = [&]() {
        ImVec2 rmin = ImGui::GetItemRectMin();
        ImVec2 rmax = ImGui::GetItemRectMax();
        titlebar_interactive_.push_back({rmin.x, rmin.y, rmax.x, rmax.y});
    };

    ImGui::PushStyleColor(ImGuiCol_ChildBg, col::BgTitlebar);
    ImGui::PushStyleColor(ImGuiCol_Border, col::Border);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::BeginChild("##CustomTitlebar", ImVec2(ImGui::GetContentRegionAvail().x, tb_h), 0, ImGuiWindowFlags_NoScrollbar);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos();
    float ww = ImGui::GetWindowWidth();
    // Bottom subtle divider line
    dl->AddLine(ImVec2(wp.x, wp.y + tb_h - 1.0f), ImVec2(wp.x + ww, wp.y + tb_h - 1.0f), ImGui::GetColorU32(col::Border), 1.0f);

    // Left: Emblem + App Title + NATIVE badge
    float emb_sz = std::round(22.0f * dpi_scale_);
    float emb_y = std::round(wp.y + (tb_h - emb_sz) * 0.5f);
    draw_brand_emblem(ImVec2(wp.x + 12.0f, emb_y), emb_sz, bold);

    float tx = wp.x + 12.0f + emb_sz + 10.0f;
    if (bold) ImGui::PushFont(bold);
    const char* title_s = brand_title();
    ImVec2 tsz = ImGui::CalcTextSize(title_s);
    dl->AddText(ImVec2(tx, wp.y + (tb_h - tsz.y) * 0.5f), ImGui::GetColorU32(col::Text), title_s);
    if (bold) ImGui::PopFont();

    tx += tsz.x + 8.0f;
    if (fonts.small_bold) ImGui::PushFont(fonts.small_bold);
    const char* nat_tag = brand_tag();
    ImVec2 nsz = ImGui::CalcTextSize(nat_tag);
    float pw = nsz.x + 12.0f, ph = nsz.y + 4.0f;
    float py = wp.y + (tb_h - ph) * 0.5f;
    dl->AddRectFilled(ImVec2(tx, py), ImVec2(tx + pw, py + ph), ImGui::GetColorU32(ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.18f)), 5.0f);
    dl->AddRect(ImVec2(tx, py), ImVec2(tx + pw, py + ph), ImGui::GetColorU32(ImVec4(col::AccentSoft.x, col::AccentSoft.y, col::AccentSoft.z, 0.48f)), 5.0f, 0, 1.0f);
    dl->AddText(ImVec2(tx + 6.0f, py + 2.0f), ImGui::GetColorU32(col::AccentSoft), nat_tag);
    if (fonts.small_bold) ImGui::PopFont();

    // Right: Connection Status Pill + Native Window Controls (Min / Max / Close)
    float wbtn_w = std::round(44.0f * dpi_scale_);
    float right_ctrl_w = wbtn_w * 3.0f;

    char conn_lbl[64];
    if (st_.attached) std::snprintf(conn_lbl, sizeof(conn_lbl), "   %s", st_.exe.c_str());
    else std::snprintf(conn_lbl, sizeof(conn_lbl), "   %s", st_.access_denied ? T(L, "Admin", "Admin") : T(L, "Offline", "Offline"));
    if (fonts.small_bold) ImGui::PushFont(fonts.small_bold);
    float conn_w = calc_btn_w(conn_lbl) + 8.0f * dpi_scale_;
    float conn_h = std::round(24.0f * dpi_scale_);
    float conn_x = ww - right_ctrl_w - conn_w - 10.0f * dpi_scale_;
    if (conn_x > (tx + pw + 14.0f)) {
        ImGui::SetCursorPos(ImVec2(conn_x, (tb_h - conn_h) * 0.5f));
        ImVec4 c_fg = st_.attached ? col::GreenSoft : col::RedSoft;
        ImVec4 c_bg(c_fg.x, c_fg.y, c_fg.z, 0.14f);
        ImVec4 c_bd(c_fg.x, c_fg.y, c_fg.z, 0.45f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, conn_h * 0.5f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f * dpi_scale_, 2.0f));
        const bool conn_clicked = styled_button(conn_lbl, c_bg, c_bd, c_fg, ImVec2(conn_w, conn_h));
        ImVec2 bmin = ImGui::GetItemRectMin();
        ImVec2 bmax = ImGui::GetItemRectMax();
        if (conn_clicked) {
            ImGui::SetNextWindowPos(ImVec2(bmax.x, bmax.y + 6.0f * dpi_scale_), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
            ImGui::OpenPopup("##conn_menu");
        }
        if (ImGui::BeginPopup("##conn_menu")) {
            if (ImGui::MenuItem(T(L, "Reconectar", "Reconnect"))) {
                bool ok = engine_.ensure_attached();
                st_ = engine_.get_status();
                wake_poller();
                std::string msg;
                if (ok) msg = std::string(T(L, "Conectado a ", "Connected to ")) + st_.exe;
                else if (st_.access_denied) msg = T(L, "Acesso negado: execute como Administrador.", "Access denied: run as Administrator.");
                else if (st_.target_unsupported) msg = T(L, "O processo escolhido não parece ser um jogo compatível. Nada foi gravado nele.", "The chosen process does not look like a compatible game. Nothing was written to it.");
                else if (!st_.target_exe.empty()) msg = std::string(T(L, "Processo não encontrado: ", "Process not found: ")) + st_.target_exe;
                else msg = T(L, "PES 2021 não encontrado.", "PES 2021 not found.");
                toasts_.push(msg, !ok);
            }
            if (ImGui::MenuItem(T(L, "Escolher processo...", "Choose process..."))) {
                open_proc_picker_ = true;
            }
            if (!settings_.game_process.empty() && ImGui::MenuItem(T(L, "Voltar à detecção automática", "Back to automatic detection"))) {
                apply_process_target(std::string(), 0);
            }
            ImGui::EndPopup();
        }
        draw_led_dot(dl, ImVec2(bmin.x + 12.0f * dpi_scale_, (bmin.y + bmax.y) * 0.5f),
                     3.6f * dpi_scale_, c_fg, st_.attached);
        track_interactive();
        show_tip_if_hovered(T(L, "Conexão com o jogo", "Game Connection"),
                            T(L, "Clique para reconectar ou escolher outro processo, como no Cheat Engine.", "Click to reconnect or pick another process, like in Cheat Engine."));
        ImGui::PopStyleVar(2);
    }
    if (fonts.small_bold) ImGui::PopFont();

    // Window Minimize / Maximize-Restore / Close buttons
    HWND hw = static_cast<HWND>(hwnd_);
    bool zoomed = hw && (IsZoomed(hw) != FALSE);
    ImGui::SetCursorPos(ImVec2(ww - right_ctrl_w, 0.0f));
    if (draw_titlebar_win_btn("##tb_min", 0, zoomed, wbtn_w, tb_h)) {
        if (hw) ShowWindow(hw, SW_MINIMIZE);
    }
    track_interactive();
    ImGui::SameLine(0.0f, 0.0f);
    if (draw_titlebar_win_btn("##tb_max", 1, zoomed, wbtn_w, tb_h)) {
        if (hw) ShowWindow(hw, zoomed ? SW_RESTORE : SW_MAXIMIZE);
    }
    track_interactive();
    ImGui::SameLine(0.0f, 0.0f);
    if (draw_titlebar_win_btn("##tb_close", 2, zoomed, wbtn_w, tb_h)) {
        if (hw) PostMessageW(hw, WM_CLOSE, 0, 0);
    }
    track_interactive();

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void WorkbenchApp::render_sidebar(const FontBank& fonts, bool compact, bool auto_compact) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    // Top row: Collapse/Expand sidebar button (only when window is wide enough to expand)
    if (!auto_compact) {
        const char* col_lbl = compact ? "»##sfold" : T(L, "« Recolher menu##sfold", "« Collapse menu##sfold");
        if (fonts.small_bold) ImGui::PushFont(fonts.small_bold);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * dpi_scale_, 4.0f * dpi_scale_));
        if (styled_button(col_lbl, col::BgSurface, col::Border, col::Subtext, ImVec2(-1.0f, std::round(30.0f * dpi_scale_)))) {
            settings_.sidebar_collapsed = !settings_.sidebar_collapsed;
            save_settings(settings_);
        }
        ImGui::PopStyleVar();
        if (fonts.small_bold) ImGui::PopFont();
        show_tip_if_hovered(T(L, "Menu Lateral", "Sidebar"), T(L, "Alternar largura do menu lateral", "Toggle sidebar width"));
        ImGui::Spacing();
    }

    auto nav_section = [&](const char* lbl) {
        if (!compact) {
            ImGui::Spacing();
            section_label(lbl, fonts);
            ImGui::Spacing();
        } else {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }
    };

    const float nav_h = std::round(36.0f * dpi_scale_);
    auto nav_btn = [&](View v, const char* /*short_tag*/) {
        bool active = (current_view_ == v);
        ImVec4 bg = active ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.19f) : ImVec4(0, 0, 0, 0);
        ImVec4 brd = active ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.42f) : ImVec4(0, 0, 0, 0);
        ImVec4 fg = active ? col::AccentSoft : col::Subtext;
        char txt[64];
        if (compact) std::snprintf(txt, sizeof(txt), "##nv%d", static_cast<int>(v));
        else std::snprintf(txt, sizeof(txt), "       %s##nv%d", view_label(v), static_cast<int>(v));

        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        if (active && bold) ImGui::PushFont(bold);
        if (styled_button(txt, bg, brd, fg, ImVec2(-1.0f, nav_h))) {
            switch_view(v);
        }
        bool hov = ImGui::IsItemHovered();
        ImVec2 bmin = ImGui::GetItemRectMin();
        ImVec2 bmax = ImGui::GetItemRectMax();
        if (active && bold) ImGui::PopFont();
        ImGui::PopStyleVar();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (active) {
            // Glowing 3px left cobalt pillar (matching Python .nav-btn.active::before)
            dl->AddRectFilled(
                ImVec2(bmin.x, bmin.y + 6.0f * dpi_scale_),
                ImVec2(bmin.x + 3.5f * dpi_scale_, bmax.y - 6.0f * dpi_scale_),
                ImGui::GetColorU32(col::Accent), 2.0f);
            dl->AddRectFilled(
                ImVec2(bmin.x, bmin.y + 4.0f * dpi_scale_),
                ImVec2(bmin.x + 8.0f * dpi_scale_, bmax.y - 4.0f * dpi_scale_),
                ImGui::GetColorU32(ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.22f)), 3.0f);
        }
        // Crisp GPU vector icon
        float ico_sz = std::round(20.0f * dpi_scale_);
        ImVec2 ico_c = compact
            ? ImVec2((bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f)
            : ImVec2(bmin.x + 20.0f * dpi_scale_, (bmin.y + bmax.y) * 0.5f);
        ImU32 ico_col = ImGui::GetColorU32(active ? col::AccentSoft : (hov ? col::Text : col::Subtext));
        draw_nav_icon(dl, ico_c, ico_sz, static_cast<int>(v), ico_col);
        if (compact) show_tip_if_hovered(view_label(v), nullptr);
    };

    nav_section(T(L, "CARREIRA", "CAREER"));
    nav_btn(View::Dashboard, "OV");
    nav_btn(View::Squad, "EL");
    nav_btn(View::Finances, "$$");

    nav_section(T(L, "ATLETA", "PLAYER"));
    nav_btn(View::Player, "JG");
    nav_btn(View::Search, "30K");
    nav_btn(View::Scouts, T(L, "OL", "SC"));

    nav_section(T(L, "MOTOR & JOGO", "ENGINE & GAME"));
    nav_btn(View::Match, "AO");
    nav_btn(View::Training, "XP");

    // Footer pinned cleanly near bottom (DPI-scaled height so Ctrl+K never clips!)
    if (!compact) {
        float footer_h = std::round(126.0f * dpi_scale_);
        float rem = ImGui::GetContentRegionAvail().y;
        if (rem > footer_h + 8.0f) ImGui::Dummy(ImVec2(0, rem - footer_h));
        else ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, col::BgSurface);
        ImGui::PushStyleColor(ImGuiCol_Border, col::Border);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f * dpi_scale_, 9.0f * dpi_scale_));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 9.0f);
        ImGui::BeginChild("##side_footer", ImVec2(-1.0f, 0),
                          ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
        if (fonts.small_bold) ImGui::PushFont(fonts.small_bold);
        ImGui::TextColored(col::AccentSoft, "%s", fmt_mode(L, st_.mode_code));
        if (fonts.small_bold) ImGui::PopFont();
        if (!st_.club.empty()) {
            if (bold) ImGui::PushFont(bold);
            ImGui::TextWrapped("%s", st_.club.c_str());
            if (bold) ImGui::PopFont();
        }
        ImGui::Spacing();
        if (fonts.small_bold) ImGui::PushFont(fonts.small_bold);
        if (styled_button(T(L, "Comandos (Ctrl+K)", "Commands (Ctrl+K)"), col::BgCard, col::BorderLit, col::Text, ImVec2(-1.0f, std::round(28.0f * dpi_scale_)))) {
            open_command_palette();
        }
        if (fonts.small_bold) ImGui::PopFont();
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }
}

void WorkbenchApp::render_topbar(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    // Pre-calculate exact width of right-hand controls so NOTHING ever clips or collides!
    int u_cnt = st_.has_player ? st_.player.undo_count : 0;
    char u_lbl[48];
    std::snprintf(u_lbl, sizeof(u_lbl), "%s (%d)###tb_undo", T(L, "Desfazer", "Undo"), u_cnt);
    char u_tip[48];
    std::snprintf(u_tip, sizeof(u_tip), "%s (%d)", T(L, "Desfazer", "Undo"), u_cnt);
    float undo_w = icon_button_width(u_lbl);

    float font_combo_w = calc_combo_width(FontBank::kNames, fonts.count, 130.0f * dpi_scale_, 210.0f * dpi_scale_);

    char cur_btn[24];
    std::snprintf(cur_btn, sizeof(cur_btn), "%s %s", kCurrencies[settings_.currency_idx].sym, kCurrencies[settings_.currency_idx].code);
    float cur_w = calc_btn_w(cur_btn);

    const char* lang_btn = (L == Lang::EN) ? "EN" : ((L == Lang::ES) ? "ES" : "PT-BR");
    float lang_w = calc_btn_w(lang_btn);

    const float gap = std::round(8.0f * dpi_scale_);
    const char* prefs_lbl = T(L, "Ajustes", "Settings");
    float prefs_w = icon_button_width(prefs_lbl);
    const char* hist_lbl = T(L, "Histórico", "History");
    float hist_w = icon_button_width(hist_lbl);
    float avail_w = ImGui::GetContentRegionAvail().x;

    // Janelas estreitas: primeiro some o seletor de fonte, depois os botoes viram so icone (a dica mostra o nome)
    const float sq = std::round(32.0f * dpi_scale_);
    const float min_left = 210.0f * dpi_scale_;
    auto right_total = [&](bool compact, bool font) {
        float t = (compact ? sq * 3.0f : undo_w + hist_w + prefs_w) + cur_w + lang_w + (font ? font_combo_w : 0.0f);
        return t + gap * (font ? 5.0f : 4.0f);
    };
    bool show_font = true, compact_tb = false;
    if (right_total(false, true) > avail_w - min_left) show_font = false;
    if (right_total(false, show_font) > avail_w - min_left) compact_tb = true;
    if (compact_tb) { undo_w = sq; hist_w = sq; prefs_w = sq; }
    float right_total_w = right_total(compact_tb, show_font);
    char undo_id[64]; std::snprintf(undo_id, sizeof(undo_id), "%s", compact_tb ? "##tb_undo" : u_lbl);
    const char* hist_btn_lbl = compact_tb ? "##tb_hist" : hist_lbl;
    const char* prefs_btn_lbl = compact_tb ? "##tb_prefs" : prefs_lbl;
    float left_max_w = std::max(120.0f * dpi_scale_, avail_w - right_total_w - 16.0f * dpi_scale_);
    float ctrl_h = std::round(32.0f * dpi_scale_);
    float top_y = std::max(4.0f, (ImGui::GetWindowHeight() - ctrl_h) * 0.5f);
    ImGui::SetCursorPosY(top_y);

    // Left: Back button + Active Player Quick Pill
    ImGui::BeginGroup();
    float used_left = 0.0f;
    if (!nav_history_.empty()) {
        char bk[64];
        std::snprintf(bk, sizeof(bk), "← %s##tbk", view_label(nav_history_.back()));
        float bkw = calc_btn_w(bk);
        if (bkw < left_max_w * 0.55f) {
            if (styled_button(bk, col::BgCard, col::Border, col::Text, ImVec2(0, ctrl_h))) trigger_back();
            show_tip_if_hovered(T(L, "Voltar", "Back"), T(L, "Voltar para a tela anterior (Alt+←)", "Go back to previous view (Alt+←)"));
            ImGui::SameLine(0, gap);
            used_left += bkw + gap;
        }
    }

    if (st_.has_player) {
        float rem_pill_w = std::max(140.0f * dpi_scale_, left_max_w - used_left);
        if (draw_active_player_pill(st_.player.ovr, st_.player.pos, st_.player.name.c_str(), L, rem_pill_w, ctrl_h, bold)) {
            switch_view(View::Player);
        }
        show_tip_if_hovered(st_.player.name.c_str(), T(L, "Abrir editor do jogador atual", "Open Current Player editor"));
    } else {
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(col::Muted, "%s", T(L, "Nenhum atleta selecionado", "No player selected"));
    }
    ImGui::EndGroup();

    // Right controls (exact pixel-aligned, zero overlap, zero truncation)
    float right_x = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - right_total_w;
    if (right_x > ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x + 8.0f * dpi_scale_) {
        ImGui::SameLine(right_x);
    } else {
        ImGui::SameLine(0, gap);
    }
    ImGui::SetCursorPosY(top_y);

    ImGui::BeginDisabled(u_cnt == 0);
    if (icon_button(Icon::Undo, undo_id, col::BgCard, col::Border, u_cnt > 0 ? col::Text : col::Muted, ImVec2(undo_w, ctrl_h))) {
        trigger_undo();
    }
    ImGui::EndDisabled();
    show_tip_if_hovered(u_tip, T(L, "Desfazer última edição do atleta (Ctrl+Z)", "Undo last edit on this player (Ctrl+Z)"));

    ImGui::SameLine(0, gap);
    ImVec2 hist_pos = ImGui::GetCursorScreenPos();
    if (icon_button(Icon::History, hist_btn_lbl, col::BgCard, col::Border, col::Text, ImVec2(hist_w, ctrl_h))) {
        ImGui::SetNextWindowPos(ImVec2(hist_pos.x, hist_pos.y + ctrl_h + 6.0f * dpi_scale_), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
        ImGui::OpenPopup("##hist_popup");
    }
    show_tip_if_hovered(hist_lbl, T(L, "Linha do tempo de todas as edições da sessão, com desfazer até qualquer ponto.", "Timeline of every edit this session, with undo to any point."));
    render_history_popup(fonts);

    if (show_font) {
    ImGui::SameLine(0, gap);
    ImGui::SetNextItemWidth(font_combo_w);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f * dpi_scale_, (ctrl_h - ImGui::GetTextLineHeight()) * 0.5f));
    if (ImGui::BeginCombo("##font_sel", FontBank::kNames[settings_.font_idx])) {
        for (int i = 0; i < fonts.count; ++i) {
            if (ImGui::Selectable(FontBank::kNames[i], settings_.font_idx == i)) {
                settings_.font_idx = i;
                save_settings(settings_);
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopStyleVar();
    show_tip_if_hovered(T(L, "Fonte da Interface", "UI Font"), T(L, "Escolher fonte nativa do Windows", "Choose native Windows UI font"));
    }

    ImGui::SameLine(0, gap);
    if (styled_button(cur_btn, col::BgCard, col::Border, col::Text, ImVec2(cur_w, ctrl_h))) {
        settings_.currency_idx = (settings_.currency_idx + 1) % 4;
        save_settings(settings_);
        if (has_player_details_) sync_player_form_buffers();
    }
    show_tip_if_hovered(T(L, "Moeda", "Currency"), T(L, "Alternar moeda de exibição (€, R$, $, £)", "Cycle display currency (€, R$, $, £)"));

    ImGui::SameLine(0, gap);
    if (styled_button(lang_btn, col::BgCard, col::Border, col::AccentSoft, ImVec2(lang_w, ctrl_h))) {
        // Ciclo PT -> EN -> ES -> PT
        if (L == Lang::PT) settings_.lang = Lang::EN;
        else if (L == Lang::EN) settings_.lang = Lang::ES;
        else settings_.lang = Lang::PT;
        save_settings(settings_);
        if (has_player_details_) sync_player_form_buffers();
    }
    show_tip_if_hovered(T(L, "Idioma", "Language"), T(L, "Alternar idioma (Português / Inglês / Espanhol)", "Cycle language (Portuguese / English / Spanish)"));

    ImGui::SameLine(0, gap);
    ImVec2 prefs_pos = ImGui::GetCursorScreenPos();
    bool prefs_clicked = icon_button(Icon::Gear, prefs_btn_lbl, col::BgCard, col::Border, col::Text, ImVec2(prefs_w, ctrl_h));
#ifdef PES_QA_DUMP
    if (qa_open_prefs_) { prefs_clicked = true; qa_open_prefs_ = false; }
#endif
    if (prefs_clicked) {
        ImGui::SetNextWindowPos(ImVec2(prefs_pos.x + prefs_w, prefs_pos.y + ctrl_h + 6.0f * dpi_scale_), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::OpenPopup("##prefs_popup");
    }
    show_tip_if_hovered(T(L, "Ajustes", "Settings"), T(L, "Sempre no topo, escala da interface e cor de destaque", "Always on top, UI scale and accent colour"));
    render_prefs_popup(fonts);
}

void WorkbenchApp::render_prefs_popup(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];
    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f * dpi_scale_, 0.0f), ImVec2(420.0f * dpi_scale_, 640.0f * dpi_scale_));
    if (!ImGui::BeginPopup("##prefs_popup")) return;
    if (ImGui::IsWindowAppearing()) refresh_backup_info();   // a aba Servicos mostra o espaco usado por backups

    {
        const float es = std::round(30.0f * dpi_scale_);
        ImVec2 ep = ImGui::GetCursorScreenPos();
        draw_brand_emblem(ImVec2(std::round(ep.x), std::round(ep.y)), es, bold);
        ImGui::Dummy(ImVec2(es, es));
        ImGui::SameLine(0, 10.0f * dpi_scale_);
        ImGui::BeginGroup();
        ImGui::PushFont(bold);
        ImGui::TextColored(col::Text, "%s", brand_title());
        ImGui::PopFont();
        ImGui::TextColored(col::Muted, "%s", T(L, "Ajustes do aplicativo", "Application settings"));
        ImGui::EndGroup();
    }
    ImGui::Spacing();

    // Duas abas: Aparencia (tema, escala, destaque) e Servicos (versao, backups, registro). Evita um popup gigante.
    {
        const float tab_gap = std::round(6.0f * dpi_scale_);
        const float tab_w = std::max(80.0f * dpi_scale_, (ImGui::GetContentRegionAvail().x - tab_gap) * 0.5f);
        const float tab_h = std::round(30.0f * dpi_scale_);
        const char* tab_lbl[2] = {T(L, "Aparência##ptab0", "Appearance##ptab0"), T(L, "Serviços##ptab1", "Services##ptab1")};
        for (int i = 0; i < 2; ++i) {
            if (i > 0) ImGui::SameLine(0, tab_gap);
            const bool act = (prefs_tab_ == i);
            if (styled_button(tab_lbl[i], act ? col::Accent : col::BgCard, act ? col::AccentSoft : col::Border, act ? ImVec4(1, 1, 1, 1) : col::Subtext, ImVec2(tab_w, tab_h))) {
                if (i == 1 && prefs_tab_ != 1) refresh_backup_info();
                prefs_tab_ = i;
            }
        }
        ImGui::Spacing();
    }
    if (prefs_tab_ == 1) {
        render_prefs_services();
        ImGui::EndPopup();
        return;
    }

    // Tema visual: cartoes de pre-visualizacao clicaveis
    ImGui::TextColored(col::Subtext, "%s", T(L, "Tema visual", "Visual theme"));
    {
        const float tgap = std::round(8.0f * dpi_scale_);
        const float tw = std::max(120.0f * dpi_scale_, (ImGui::GetContentRegionAvail().x - tgap) * 0.5f);
        const float th = std::round(62.0f * dpi_scale_);
        for (int i = 0; i < kThemeCount; ++i) {
            if (i > 0) ImGui::SameLine(0, tgap);
            char tid[24]; std::snprintf(tid, sizeof(tid), "##theme%d", i);
            ImVec2 tp = ImGui::GetCursorScreenPos();
            bool tclick = ImGui::InvisibleButton(tid, ImVec2(tw, th));
            bool thov = ImGui::IsItemHovered();
            if (thov) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImVec4 pv[4];
            theme_preview(i, pv);
            const bool tact = (settings_.theme_idx == i);
            ImDrawList* tdl = ImGui::GetWindowDrawList();
            const float trnd = std::round(10.0f * dpi_scale_);
            tdl->AddRectFilled(tp, ImVec2(tp.x + tw, tp.y + th), ImGui::GetColorU32(pv[0]), trnd);
            tdl->AddRectFilled(ImVec2(tp.x + 8.0f * dpi_scale_, tp.y + 8.0f * dpi_scale_), ImVec2(tp.x + tw * 0.62f, tp.y + th - 8.0f * dpi_scale_), ImGui::GetColorU32(pv[1]), trnd * 0.7f);
            tdl->AddRectFilled(ImVec2(tp.x + 14.0f * dpi_scale_, tp.y + 14.0f * dpi_scale_), ImVec2(tp.x + tw * 0.40f, tp.y + 20.0f * dpi_scale_), ImGui::GetColorU32(pv[2]), 3.0f);
            tdl->AddRectFilled(ImVec2(tp.x + 14.0f * dpi_scale_, tp.y + 26.0f * dpi_scale_), ImVec2(tp.x + tw * 0.52f, tp.y + 30.0f * dpi_scale_), ImGui::GetColorU32(pv[3]), 2.0f);
            tdl->AddCircleFilled(ImVec2(tp.x + tw * 0.80f, tp.y + th * 0.42f), 9.0f * dpi_scale_, ImGui::GetColorU32(pv[2]), 20);
            tdl->AddCircleFilled(ImVec2(tp.x + tw * 0.80f + 5.0f * dpi_scale_, tp.y + th * 0.42f - 4.0f * dpi_scale_), 4.0f * dpi_scale_, ImGui::GetColorU32(pv[3]), 16);
            const char* tn = theme_name(i, L);
            ImVec2 tsz = ImGui::CalcTextSize(tn);
            tdl->AddText(ImVec2(tp.x + 14.0f * dpi_scale_, tp.y + th - 8.0f * dpi_scale_ - tsz.y - 2.0f), ImGui::GetColorU32(ImVec4(0.93f, 0.96f, 0.92f, 1.0f)), tn);
            tdl->AddRect(tp, ImVec2(tp.x + tw, tp.y + th), tact ? ImGui::GetColorU32(pv[3]) : ImGui::GetColorU32(thov ? col::BorderLit : col::Border), trnd, 0, tact ? 2.0f : 1.0f);
            if (tact) draw_icon(tdl, Icon::Check, ImVec2(tp.x + tw - 15.0f * dpi_scale_, tp.y + 15.0f * dpi_scale_), 14.0f * dpi_scale_, ImGui::GetColorU32(pv[3]));
            if (tclick && !tact) {
                // Mantem a cor de destaque escolhida pelo usuario; so troca se ainda era o padrao do tema anterior
                if (settings_.accent_idx == theme_default_accent(settings_.theme_idx)) settings_.accent_idx = theme_default_accent(i);
                settings_.theme_idx = i;
                save_settings(settings_);
                theme_rebuild_ = true;
            }
        }
    }
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 330.0f * dpi_scale_);
    ImGui::TextColored(col::Muted, "%s", T(L, "O nome e o logo Lettuce Live Editor valem para os dois temas. Alface é o verde padrão, Azul é o azul-marinho.", "The Lettuce Live Editor name and logo apply to both themes. Lettuce is the default green, Blue is the navy look."));
    ImGui::PopTextWrapPos();
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    // Always-on-Top
    bool top_v = settings_.topmost;
    if (toggle_switch("##sw_topmost", T(L, "Sempre no topo (Always-on-Top)", "Always on top"), &top_v)) {
        settings_.topmost = top_v;
        if (hwnd_) SetWindowPos(static_cast<HWND>(hwnd_), top_v ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        save_settings(settings_);
    }
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 330.0f * dpi_scale_);
    wrapped_colored(col::Muted, "%s", T(L, "Mantém o Lettuce Live Editor visível sobre o jogo em modo janela ou sem borda.", "Keeps Lettuce Live Editor visible over the game in windowed or borderless mode."));
    ImGui::PopTextWrapPos();
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    // UI scale
    ImGui::TextColored(col::Subtext, "%s", T(L, "Escala da interface", "UI scale"));
    const float gap = std::round(6.0f * dpi_scale_);
    const float bw = std::max(48.0f * dpi_scale_, (ImGui::GetContentRegionAvail().x - gap * 4.0f) / 5.0f);
    const float bh = std::round(30.0f * dpi_scale_);
    for (int i = 0; i < 5; ++i) {
        if (i > 0) ImGui::SameLine(0, gap);
        char lbl[24];
        std::snprintf(lbl, sizeof(lbl), "%d%%##uis%d", static_cast<int>(kUiScales[i] * 100.0f + 0.5f), i);
        bool act = (settings_.ui_scale_idx == i);
        if (styled_button(lbl, act ? col::Accent : col::BgCard, act ? col::AccentSoft : col::Border, col::Text, ImVec2(bw, bh)) && !act) {
            settings_.ui_scale_idx = i;
            save_settings(settings_);
            theme_rebuild_ = true;
        }
    }
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 330.0f * dpi_scale_);
    wrapped_colored(col::Muted, "%s", T(L, "Multiplica a escala automática do Windows. Útil para 4K, ultrawide ou segundo monitor.", "Multiplies the automatic Windows scale. Useful for 4K, ultrawide or a second monitor."));
    ImGui::PopTextWrapPos();
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    // Accent colour
    ImGui::TextColored(col::Subtext, "%s", T(L, "Cor de destaque", "Accent colour"));
    const float sw_w = std::max(26.0f * dpi_scale_, (ImGui::GetContentRegionAvail().x - gap * static_cast<float>(kAccentCount - 1)) / static_cast<float>(kAccentCount));
    for (int i = 0; i < kAccentCount; ++i) {
        if (i > 0) ImGui::SameLine(0, gap);
        char lbl[32];
        std::snprintf(lbl, sizeof(lbl), "##acc%d", i);
        bool act = (settings_.accent_idx == i);
        ImVec4 sw = accent_swatch(i);
        const bool sw_click = styled_button(lbl, ImVec4(sw.x, sw.y, sw.z, act ? 1.0f : 0.55f), act ? col::Text : sw, col::Text, ImVec2(sw_w, bh));
        show_tip_if_hovered(accent_name(i, L), T(L, "Cor de destaque", "Accent colour"));
        if (act) {
            ImVec2 a0 = ImGui::GetItemRectMin(), a1 = ImGui::GetItemRectMax();
            draw_icon(ImGui::GetWindowDrawList(), Icon::Check, ImVec2((a0.x + a1.x) * 0.5f, (a0.y + a1.y) * 0.5f), bh * 0.5f, ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
        }
        if (sw_click && !act) {
            settings_.accent_idx = i;
            save_settings(settings_);
            theme_rebuild_ = true;
        }
    }
    ImGui::Spacing();
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 330.0f * dpi_scale_);
    wrapped_colored(col::Muted, "%s", T(L, "O modo escuro é mantido. Só botões, seleção e realces mudam de cor.", "Dark mode is kept. Only buttons, selection and highlights change colour."));
    ImGui::PopTextWrapPos();
    ImGui::EndPopup();
}

void WorkbenchApp::render_view_dashboard(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    // Page header
    page_title(T(L, "Visão Geral", "Overview"), fonts, bold);
    ImGui::SameLine(0, 12);
    if (st_.in_match) draw_pill(T(L, "● EM PARTIDA", "● IN MATCH"), col::GreenSoft, ImVec4(0.063f, 0.725f, 0.506f, 0.18f), ImVec4(0.063f, 0.725f, 0.506f, 0.55f));
    else draw_pill(T(L, "● Fora de Partida", "● Not in Match"), col::Subtext, ImVec4(col::Subtext.x, col::Subtext.y, col::Subtext.z, 0.12f), col::Border);

    render_connection_hint(fonts);
    ImGui::Spacing();

    // 3 Responsive KPI Cards (equal height when 3-column, illuminated top accent + icon medallions)
    float avail_w = ImGui::GetContentRegionAvail().x;
    int kpi_cols = (avail_w < 840.0f * dpi_scale_) ? 1 : 3;
    float kpi_min_h = (kpi_cols == 3) ? std::round(170.0f * dpi_scale_) : 0.0f;
    float med_sz = std::round(28.0f * dpi_scale_);
    if (ScopedCardGrid g{"##dash_kpis", kpi_cols, dpi_scale_}) {
        // KPI 1: Mode & Club
        ImGui::TableNextColumn();
        begin_studio_card("##kpi_club", kpi_min_h, false, &col::Accent);
        draw_kpi_medallion(0, med_sz, col::AccentSoft);
        ImGui::SameLine(0, 10.0f * dpi_scale_);
        ImGui::BeginGroup();
        section_label(T(L, "MODO & CLUBE", "MODE & CLUB"), fonts);
        ImGui::TextColored(col::AccentSoft, "%s", fmt_mode(L, st_.mode_code));
        ImGui::EndGroup();
        ImGui::Spacing();
        if (fonts.title_bold) ImGui::PushFont(fonts.title_bold);
        ImGui::TextWrapped("%s", st_.club.empty() ? (st_.attached ? st_.exe.c_str() : "---") : st_.club.c_str());
        if (fonts.title_bold) ImGui::PopFont();
        ImGui::Spacing();
        if (ImGui::Button(T(L, "Meu Elenco##dkel", "My Squad##dkel"))) switch_view(View::Squad);
        flow_next(calc_btn_w(T(L, "Finanças", "Finances")));
        if (ImGui::Button(T(L, "Finanças##dkfn", "Finances##dkfn"))) switch_view(View::Finances);
        end_studio_card();

        // KPI 2: Transfer Budget + Inline Edit
        ImGui::TableNextColumn();
        begin_studio_card("##kpi_trf", kpi_min_h, false, &col::Green);
        draw_kpi_medallion(1, med_sz, col::GreenSoft);
        ImGui::SameLine(0, 10.0f * dpi_scale_);
        ImGui::BeginGroup();
        section_label(T(L, "ORÇAMENTO DE TRANSFERÊNCIAS", "TRANSFER BUDGET"), fonts);
        ImGui::TextColored(col::Muted, "%s", T(L, "Caixa imediato para contratações", "Immediate transfer funds"));
        ImGui::EndGroup();
        ImGui::Spacing();
        if (fonts.large_num) ImGui::PushFont(fonts.large_num);
        ImGui::TextColored(col::GreenSoft, "%s", fmt_money(st_.has_finances ? st_.finances.transfer_raw : 0, settings_.currency_idx, L).c_str());
        if (fonts.large_num) ImGui::PopFont();
        ImGui::Spacing();
        if (!edit_inline_trf_) {
            if (btn_green("+10M##dt10")) { auto r = engine_.add_finance_delta(true, 100000); if (r.ok) { st_.finances = r; toasts_.push(T(L, "Orçamento atualizado!", "Budget updated!")); } else toast_code(r.code, true); }
            flow_next(calc_btn_w("+100M"));
            if (btn_green("+100M##dt100")) { auto r = engine_.add_finance_delta(true, 1000000); if (r.ok) { st_.finances = r; toasts_.push(T(L, "Orçamento atualizado!", "Budget updated!")); } else toast_code(r.code, true); }
            flow_next(icon_button_width(T(L, "Editar", "Edit")));
            if (icon_btn_plain(Icon::Sliders, T(L, "Editar##etrf", "Edit##etrf"))) {
                edit_inline_trf_ = true;
                inline_trf_buf_[0] = '\0';
            }
        } else {
            ImGui::SetNextItemWidth(std::round(120.0f * dpi_scale_));
            bool enter = ImGui::InputTextWithHint("##itrf", "ex: 50M", inline_trf_buf_, sizeof(inline_trf_buf_), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();
            if (btn_primary("OK##itrfok") || enter) {
                auto r = engine_.set_finances(std::string_view(inline_trf_buf_), std::nullopt);
                if (r.ok) { st_.finances = r; edit_inline_trf_ = false; toasts_.push(T(L, "Orçamento atualizado!", "Budget updated!")); }
                else toast_code(r.code, true);
            }
            ImGui::SameLine();
            if (icon_only_button(Icon::Close, "##itrfx", col::BgCard, col::Border, col::Subtext, ImGui::GetFrameHeight())) edit_inline_trf_ = false;
        }
        end_studio_card();

        // KPI 3: Available Salary + Mini Utilization Gauge + Inline Edit
        ImGui::TableNextColumn();
        int32_t sal_v = st_.has_finances ? st_.finances.salary_avail_raw : 0;
        const ImVec4& sal_acc = (sal_v < 0) ? col::Red : col::Gold;
        begin_studio_card("##kpi_sal", kpi_min_h, false, &sal_acc);
        draw_kpi_medallion(2, med_sz, sal_v < 0 ? col::RedSoft : col::GoldSoft);
        ImGui::SameLine(0, 10.0f * dpi_scale_);
        ImGui::BeginGroup();
        section_label(T(L, "SALÁRIOS DISPONÍVEIS", "AVAILABLE SALARY BUDGET"), fonts);
        show_tip_if_hovered(T(L, "Salários Disponíveis", "Available Salary Budget"),
            T(L, "Valor real livre para novos contratos (Teto Bruto menos folha ativa e pendentes).",
                 "Actual budget available for new contracts (Gross Cap minus active wages and pending overrides)."));
        float wage_pct = (st_.has_finances && st_.finances.salary_gross_raw > 0)
            ? std::clamp(static_cast<float>(st_.finances.squad_wages_raw) / static_cast<float>(st_.finances.salary_gross_raw), 0.0f, 1.0f)
            : 0.0f;
        ImGui::TextColored(col::Muted, "%s %.0f%%", T(L, "Folha comprometida:", "Wage cap used:"), wage_pct * 100.0f);
        ImGui::EndGroup();
        ImGui::Spacing();
        if (fonts.large_num) ImGui::PushFont(fonts.large_num);
        ImGui::TextColored(sal_v < 0 ? col::RedSoft : col::GoldSoft, "%s", fmt_money(sal_v, settings_.currency_idx, L).c_str());
        if (fonts.large_num) ImGui::PopFont();
        // Sleek inline wage gauge bar
        {
            ImVec2 gp = ImGui::GetCursorScreenPos();
            float gw = ImGui::GetContentRegionAvail().x;
            float gh = std::round(5.0f * dpi_scale_);
            ImGui::Dummy(ImVec2(gw, gh + 4.0f * dpi_scale_));
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(gp, ImVec2(gp.x + gw, gp.y + gh), ImGui::GetColorU32(col::BgInput), gh * 0.5f);
            dl->AddRectFilled(gp, ImVec2(gp.x + gw, gp.y + gh), ImGui::GetColorU32(ImVec4(col::Green.x, col::Green.y, col::Green.z, 0.35f)), gh * 0.5f);
            if (wage_pct > 0.01f) {
                dl->AddRectFilled(gp, ImVec2(gp.x + gw * wage_pct, gp.y + gh),
                                  ImGui::GetColorU32(sal_v < 0 ? col::Red : col::Gold), gh * 0.5f);
            }
        }
        if (!edit_inline_sal_) {
            if (btn_gold("+10M##ds10")) { auto r = engine_.add_finance_delta(false, 100000); if (r.ok) { st_.finances = r; toasts_.push(T(L, "Salários atualizados!", "Salary updated!")); } else toast_code(r.code, true); }
            flow_next(calc_btn_w("+50M"));
            if (btn_gold("+50M##ds50")) { auto r = engine_.add_finance_delta(false, 500000); if (r.ok) { st_.finances = r; toasts_.push(T(L, "Salários atualizados!", "Salary updated!")); } else toast_code(r.code, true); }
            flow_next(icon_button_width(T(L, "Editar", "Edit")));
            if (icon_btn_plain(Icon::Sliders, T(L, "Editar##esal", "Edit##esal"))) {
                edit_inline_sal_ = true;
                inline_sal_buf_[0] = '\0';
            }
        } else {
            ImGui::SetNextItemWidth(std::round(120.0f * dpi_scale_));
            bool enter = ImGui::InputTextWithHint("##isal", "ex: 15M", inline_sal_buf_, sizeof(inline_sal_buf_), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();
            if (btn_primary("OK##isalok") || enter) {
                auto r = engine_.set_finances(std::nullopt, std::string_view(inline_sal_buf_));
                if (r.ok) { st_.finances = r; edit_inline_sal_ = false; toasts_.push(T(L, "Salários atualizados!", "Salary updated!")); }
                else toast_code(r.code, true);
            }
            ImGui::SameLine();
            if (icon_only_button(Icon::Close, "##isalx", col::BgCard, col::Border, col::Subtext, ImGui::GetFrameHeight())) edit_inline_sal_ = false;
        }
        end_studio_card();
    }

    ImGui::Dummy(ImVec2(0, std::round(8.0f * dpi_scale_)));

    // Row 2: Active Player Card + Native x64 Hooks Status
    int row2_cols = (avail_w < 820.0f * dpi_scale_) ? 1 : 2;
    float row2_min_h = (row2_cols == 2) ? std::round(244.0f * dpi_scale_) : 0.0f;
    if (ScopedCardGrid g{"##dash_r2", row2_cols, dpi_scale_}) {
        ImGui::TableNextColumn();
        begin_studio_card("##dash_active_p", row2_min_h);
        {
            float msz = std::round(24.0f * dpi_scale_);
            float row_y = ImGui::GetCursorPosY();
            draw_kpi_medallion(3, msz, col::AccentSoft);
            ImGui::SameLine(0, 9.0f * dpi_scale_);
            ImGui::SetCursorPosY(row_y + std::max(0.0f, (msz - ImGui::GetTextLineHeight()) * 0.5f));
            card_header(T(L, "Atleta em Foco (Ao Vivo)", "Active Player (Live)"), bold);
            const char* open_lbl = T(L, "Abrir Editor", "Open Editor");
            align_right_or_wrap(icon_button_width(open_lbl));
            ImGui::SetCursorPosY(row_y + std::max(0.0f, (msz - ImGui::GetFrameHeight()) * 0.5f));
            if (icon_btn_primary(Icon::Move, open_lbl)) switch_view(View::Player);
            ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), row_y + msz + 6.0f * dpi_scale_));
        }
        ImGui::Separator();
        ImGui::Spacing();
        if (st_.has_player) {
            draw_ovr_shield(st_.player.ovr, std::round(54.0f * dpi_scale_), fonts.large_num ? fonts.large_num : bold, fonts.small_bold);
            ImGui::SameLine(0, 14.0f * dpi_scale_);
            ImGui::BeginGroup();
            if (bold) ImGui::PushFont(bold);
            ImGui::Text("%s", st_.player.name.c_str());
            if (bold) ImGui::PopFont();
            ImGui::SameLine(0, 8.0f * dpi_scale_);
            draw_pos_chip(L, st_.player.pos, std::round(38.0f * dpi_scale_), std::round(22.0f * dpi_scale_), fonts.small_bold ? fonts.small_bold : bold);
            ImGui::SameLine(0, 6.0f * dpi_scale_);
            draw_arrow_badge(st_.player.arrow, std::round(22.0f * dpi_scale_), st_.player.blink, settings_.lang);
            ImGui::TextColored(col::Subtext, "%s  |  %d%s", fmt_club(L, st_.player.club).c_str(), st_.player.age, T(L, "a", "y"));
            draw_stamina_bar(st_.player.stamina, 92.0f);
            ImGui::EndGroup();
            ImGui::Spacing();
            if (icon_btn_green(Icon::Battery, T(L, "100% Fôlego##dp", "100% Stamina##dp"))) {
                auto r = engine_.set_player_quick_condition(100, std::nullopt, false);
                if (r.ok) { wake_poller(); toasts_.push(T(L, "Fôlego 100%!", "Stamina 100%!")); } else toast_code(r.code, true);
            }
            flow_next(icon_button_width(T(L, "Seta Azul", "Blue Arrow")));
            if (icon_btn_primary(Icon::ArrowUp, T(L, "Seta Azul##dp", "Blue Arrow##dp"))) {
                auto r = engine_.set_player_quick_condition(std::nullopt, 4, false, 0, true);
                if (r.ok) { wake_poller(); toasts_.push(T(L, "Seta Azul aplicada!", "Blue Arrow applied!")); } else toast_code(r.code, true);
            }
            flow_next(icon_button_width(T(L, "Curar Lesão", "Heal Injury")));
            ImGui::BeginDisabled(st_.player.injury == 0);
            if (icon_btn_green(Icon::Cross, T(L, "Curar Lesão##dp", "Heal Injury##dp"))) {
                auto r = engine_.set_player_quick_condition(std::nullopt, std::nullopt, true);
                if (r.ok) { wake_poller(); toasts_.push(T(L, "Lesão curada!", "Injury healed!")); } else toast_code(r.code, true);
            }
            ImGui::EndDisabled();
        } else {
            ImGui::TextWrapped("%s", T(L, "Nenhum atleta selecionado. Destaque um jogador no Plano de Jogo ou selecione em Meu Elenco.", "No player selected. Highlight a player in Game Plan or pick one from My Squad."));
        }
        end_studio_card();

        ImGui::TableNextColumn();
        begin_studio_card("##dash_hooks", row2_min_h);
        {
            float msz = std::round(24.0f * dpi_scale_);
            float row_y = ImGui::GetCursorPosY();
            draw_kpi_medallion(4, msz, col::GreenSoft);
            ImGui::SameLine(0, 9.0f * dpi_scale_);
            ImGui::SetCursorPosY(row_y + std::max(0.0f, (msz - ImGui::GetTextLineHeight()) * 0.5f));
            card_header(T(L, "Status dos Recursos x64", "Native x64 Features Status"), bold,
                T(L, "Os hooks x64 são verificados continuamente e restaurados com segurança ao fechar.", "Native x64 hooks are verified continuously and restored cleanly on exit."));
            ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), row_y + msz + 6.0f * dpi_scale_));
        }
        ImGui::Separator();
        ImGui::Spacing();
        struct HItem { bool on; const char* lbl; };
        const HItem items[5] = {
            {st_.hooks[1], T(L, "Captura Automática (Plano de Jogo / Elenco)", "Auto Player Capture (Game Plan / Squad)")},
            {st_.hooks[3] && st_.training_mode > 0, T(L, "Multiplicador de Treino (XP)", "Training XP Multiplier")},
            {st_.hooks[4] && (st_.bal_lock || st_.bal_focus > 21), T(L, "Desbloqueio de Foco BAL (21 -> 99)", "BAL Focus Cap Unlock")},
            {st_.hooks[5] && st_.freeze_clock, T(L, "Relógio Congelado na Partida", "Match Clock Freeze")},
            {st_.hooks[6] && st_.stamina_mode > 0, T(L, "Fôlego Seletivo em Partida", "Selective Match Stamina")},
        };
        if (ImGui::BeginTable("##hooks_tbl", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX)) {
            ImGui::TableSetupColumn("##hlbl", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("##hst", ImGuiTableColumnFlags_WidthFixed, std::round(92.0f * dpi_scale_));
            for (const auto& hi : items) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(hi.on ? col::Text : col::Subtext, "%s", hi.lbl);
                ImGui::TableSetColumnIndex(1);
                ImVec2 lp = ImGui::GetCursorScreenPos();
                float lh = ImGui::GetTextLineHeight();
                draw_led_dot(ImGui::GetWindowDrawList(),
                             ImVec2(lp.x + 5.0f * dpi_scale_, lp.y + lh * 0.5f),
                             3.5f * dpi_scale_, hi.on ? col::GreenSoft : col::Muted, hi.on);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 14.0f * dpi_scale_);
                if (fonts.small_bold) ImGui::PushFont(fonts.small_bold);
                ImGui::TextColored(hi.on ? col::GreenSoft : col::Muted, "%s",
                    hi.on ? T(L, "ATIVO", "ACTIVE") : T(L, "Espera", "Standby"));
                if (fonts.small_bold) ImGui::PopFont();
            }
            ImGui::EndTable();
        }
        end_studio_card();
    }

    ImGui::Dummy(ImVec2(0, std::round(8.0f * dpi_scale_)));

    // Row 3: Squad Commander (auto-wrapping responsive buttons)
    begin_studio_card("##dash_sq_cmd");
    {
        float msz = std::round(24.0f * dpi_scale_);
        float row_y = ImGui::GetCursorPosY();
        draw_kpi_medallion(5, msz, col::GoldSoft);
        ImGui::SameLine(0, 9.0f * dpi_scale_);
        ImGui::SetCursorPosY(row_y + std::max(0.0f, (msz - ImGui::GetTextLineHeight()) * 0.5f));
        card_header(T(L, "Comandante do Elenco (Ações em Massa)", "Squad Commander (Bulk Actions)"), bold);
        ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), row_y + msz + 4.0f * dpi_scale_));
    }
    wrapped_colored(col::Subtext, "%s", T(L, "Aplica instantaneamente a todos os jogadores do seu clube.", "Applies instantly to every player in your club."));
    ImGui::Spacing();
    if (icon_btn_green(Icon::Battery, T(L, "100% Fôlego (Elenco)", "100% Stamina (Squad)"))) run_squad_cmd("stamina");
    flow_next(icon_button_width(T(L, "Seta Azul (Elenco)", "Blue Arrow (Squad)")));
    if (icon_btn_primary(Icon::ArrowUp, T(L, "Seta Azul (Elenco)", "Blue Arrow (Squad)"))) run_squad_cmd("arrow");
    flow_next(icon_button_width(T(L, "Curar Todas Lesões", "Heal All Injuries")));
    if (icon_btn_green(Icon::Cross, T(L, "Curar Todas Lesões", "Heal All Injuries"))) run_squad_cmd("heal");
    flow_next(icon_button_width(T(L, "Renovar Contratos +5 Anos", "Extend Contracts +5 Years")));
    if (icon_btn_gold(Icon::Contract, T(L, "Renovar Contratos +5 Anos", "Extend Contracts +5 Years"))) run_squad_cmd("contract");
    flow_next(icon_button_width(T(L, "+1 OVR no Elenco (DNA)", "+1 OVR to Squad (DNA)")));
    if (icon_btn_plain(Icon::ArrowUp, T(L, "+1 OVR no Elenco (DNA)", "+1 OVR to Squad (DNA)"))) run_squad_cmd("ovr1");
    show_tip_if_hovered("+1 OVR (DNA)", T(L, "Sobe +1 OVR em cada atleta preservando o DNA de atributos e sincronizando a economia conforme o modo ativo.", "Raises every squad player by +1 OVR while preserving stat DNA and syncing economy per active mode."));
    flow_next(icon_button_width(T(L, "Recalibrar Economia (Elenco)", "Recalibrate Economy (Squad)")));
    if (icon_btn_gold(Icon::Coins, T(L, "Recalibrar Economia (Elenco)", "Recalibrate Economy (Squad)"))) run_squad_cmd("econ_recal");
    show_tip_if_hovered(T(L, "Recalibrar Economia do Elenco", "Recalibrate Squad Economy"),
        T(L, "Recalcula valor de mercado, salario e rescisao de todos os atletas do elenco pela curva calibrada Transfermarkt 2025/26.",
             "Recalculates market value, salary and release fee for every squad player using the calibrated 2025/26 curve."));
    end_studio_card();
}

void WorkbenchApp::apply_preset_cmd(bool to_selection) {
    Lang L = settings_.lang;
    std::vector<uint64_t> v;
    if (to_selection) {
        for (const auto& p : squad_.players) if (squad_sel_.count(p.addr)) v.push_back(p.addr);
        if (v.empty()) return;
    }
    auto r = engine_.apply_skill_preset(preset_idx_, to_selection ? &v : nullptr, preset_replace_);
    if (r.ok) {
        char msg[256];
        std::snprintf(msg, sizeof(msg), T(L, "Preset de habilidades aplicado a %d atleta(s)!%s", "Skill preset applied to %d player(s)!%s"), r.count,
                      r.flag ? T(L, " Backup do elenco salvo em Packs\\Backups.", " Squad backup saved in Packs\\Backups.") : "");
        toasts_.push(msg);
        if (current_view_ == View::Squad) squad_reload_pending_ = true;
        if (current_view_ == View::Player || has_player_details_) reload_player();
        wake_poller();
    } else {
        toast_code(r.code, true);
    }
}

void WorkbenchApp::render_preset_controls(const FontBank& fonts, bool compact) {
    (void)fonts;
    Lang L = settings_.lang;
    const auto& list = presets::kSkillPresets;
    preset_idx_ = std::clamp(preset_idx_, 0, static_cast<int>(list.size()) - 1);
    const auto& cur = list[static_cast<size_t>(preset_idx_)];
    ImGui::SetNextItemWidth(std::round((compact ? 190.0f : 260.0f) * dpi_scale_));
    if (ImGui::BeginCombo("##skill_preset", T(L, cur.pt, cur.en))) {
        for (size_t i = 0; i < list.size(); ++i) {
            if (ImGui::Selectable(T(L, list[i].pt, list[i].en), static_cast<int>(i) == preset_idx_)) preset_idx_ = static_cast<int>(i);
        }
        ImGui::EndCombo();
    }
    show_tip_if_hovered(T(L, cur.pt, cur.en), T(L, cur.desc_pt, cur.desc_en));
    flow_next(48.0f * dpi_scale_ + ImGui::CalcTextSize(T(L, "Substituir habilidades", "Replace skills")).x, 12.0f * dpi_scale_);
    toggle_switch("##sw_preset_repl", T(L, "Substituir habilidades", "Replace skills"), &preset_replace_);
    show_tip_if_hovered(T(L, "Substituir habilidades", "Replace skills"),
        T(L, "Desligado: só adiciona as habilidades do preset, respeitando o limite nativo de 10, e preserva o DNA do atleta. Ligado: remove as 41 habilidades antes de aplicar.",
             "Off: only adds the preset skills, respecting the native cap of 10, preserving the player's DNA. On: clears all 41 skills first."));
}

namespace {
const char* journal_action_label(const std::string& code, Lang L) {
    struct M { const char* c; const char* pt; const char* en; };
    static const M k[] = {
        {"ovr_dna", "OVR com DNA", "OVR with DNA"}, {"econ_recal", "Recalibrar economia", "Recalibrate economy"},
        {"skill", "Habilidade", "Skill"}, {"skill_preset", "Preset de habilidades", "Skill preset"},
        {"style", "Estilo de jogo", "Playing style"}, {"condition", "Condição", "Condition"},
        {"pack_import", "Importar pack", "Import pack"},
        {"batch_econ_recal", "Lote: recalibrar economia", "Batch: recalibrate economy"},
        {"batch_ovr1", "Lote: +1 OVR (DNA)", "Batch: +1 OVR (DNA)"}, {"batch_stamina", "Lote: fôlego 100%", "Batch: stamina 100%"},
        {"batch_arrow", "Lote: seta azul piscando", "Batch: blinking blue arrow"}, {"batch_heal", "Lote: curar lesões", "Batch: heal injuries"},
        {"batch_contract", "Lote: contrato +5 anos", "Batch: contract +5 years"},
        {"squad_econ_recal", "Recalibrar economia (elenco)", "Recalibrate economy (squad)"},
        {"squad_stamina", "Fôlego 100%", "Stamina 100%"}, {"squad_arrow", "Seta azul piscando", "Blinking blue arrow"},
        {"squad_heal", "Curar lesões", "Heal injuries"}, {"squad_contract", "Contrato +5 anos", "Contract +5 years"},
        {"bio", "Bio e contrato", "Bio and contract"}, {"import_json", "Importar JSON", "Import JSON"},
        {"pos_grade", "Grau de posição", "Position grade"}, {"pos_ovr", "OVR de posição", "Position OVR"},
        {"reg_pos", "Posição principal", "Primary position"}, {"stat", "Atributo", "Attribute"},
        {"stats_batch", "Atributos em lote", "Attributes batch"}, {"team_role", "Função no time", "Team role"},
    };
    for (const auto& m : k) if (code == m.c) return T(L, m.pt, m.en);
    return code.c_str();
}
} // namespace

void WorkbenchApp::render_history_popup(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];
    ImGui::SetNextWindowSizeConstraints(ImVec2(440.0f * dpi_scale_, 0.0f), ImVec2(640.0f * dpi_scale_, 560.0f * dpi_scale_));
    if (!ImGui::BeginPopup("##hist_popup")) return;

    if (journal_rev_seen_ != engine_.journal_revision() || ImGui::IsWindowAppearing()) {
        journal_cache_ = engine_.get_journal();
        journal_rev_seen_ = engine_.journal_revision();
    }
    ImGui::PushFont(bold);
    ImGui::TextColored(col::Text, "%s", T(L, "Histórico de alterações", "Edit history"));
    ImGui::PopFont();
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 440.0f * dpi_scale_);
    wrapped_colored(col::Muted, "%s", T(L, "Do mais recente para o mais antigo. Desfazer até aqui reverte esta edição e todas as posteriores.",
                                              "Newest first. Undo to here reverts this edit and every later one."));
    ImGui::PopTextWrapPos();
    ImGui::Spacing();
    if (journal_cache_.empty()) {
        ImGui::TextColored(col::Subtext, "%s", T(L, "Nenhuma edição nesta sessão.", "No edits in this session."));
        ImGui::EndPopup();
        return;
    }
    ImGui::BeginChild("##hist_list", ImVec2(0, std::min(380.0f * dpi_scale_, 34.0f * dpi_scale_ * static_cast<float>(journal_cache_.size()) + 8.0f)), false);
    uint64_t undo_to = 0;
    for (const auto& j : journal_cache_) {
        ImGui::PushID(static_cast<int>(j.seq));
        char when[24];
        if (j.age_sec < 5) std::snprintf(when, sizeof(when), "%s", T(L, "agora", "now"));
        else if (j.age_sec < 90) std::snprintf(when, sizeof(when), "%us", j.age_sec);
        else if (j.age_sec < 5400) std::snprintf(when, sizeof(when), "%umin", j.age_sec / 60);
        else std::snprintf(when, sizeof(when), "%uh", j.age_sec / 3600);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(col::Muted, "%-6s", when);
        ImGui::SameLine(0, 10.0f * dpi_scale_);
        char who[96];
        if (j.players > 1) std::snprintf(who, sizeof(who), "%d %s", j.players, T(L, "atletas", "players"));
        else std::snprintf(who, sizeof(who), "%s", j.label.c_str());
        ImGui::TextColored(col::AccentSoft, "%s", journal_action_label(j.action, L));
        ImGui::SameLine(0, 8.0f * dpi_scale_);
        ImGui::TextColored(col::Subtext, "%s", who);
        const char* ub = T(L, "Desfazer até aqui", "Undo to here");
        float bw = calc_btn_w(ub);
        ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 12.0f * dpi_scale_, ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - bw));
        if (styled_button(ub, col::BgCard, col::Border, col::Text, ImVec2(bw, 0))) undo_to = j.seq;
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::EndPopup();

    if (undo_to != 0) {
        auto r = engine_.undo_journal_to(undo_to);
        if (r.ok) {
            char msg[128];
            std::snprintf(msg, sizeof(msg), T(L, "%d atleta(s) restaurado(s) em %d edição(ões).", "%d player(s) restored across %d edit(s)."), r.count, r.value);
            toasts_.push(msg);
            if (current_view_ == View::Squad) reload_squad();
            if (has_player_details_) reload_player();
            wake_poller();
        } else toast_code(r.code, true);
        journal_rev_seen_ = ~0ULL;
    }
}

void WorkbenchApp::render_view_squad(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    char hdr[128];
    std::snprintf(hdr, sizeof(hdr), "%s%s%s (%zu)", T(L, "Meu Elenco", "My Squad"),
                  squad_.club.empty() ? "" : " - ", squad_.club.c_str(), squad_.players.size());
    page_title(hdr, fonts, bold);

    float flt_w = std::round(190.0f * dpi_scale_);
    float ref_w = icon_button_width(T(L, "Atualizar", "Refresh"));
    align_right_or_wrap(flt_w + ref_w + 8.0f * dpi_scale_);
    ImGui::SetNextItemWidth(flt_w);
    ImGui::InputTextWithHint("##sq_flt", T(L, "Filtrar nome...", "Filter name..."), squad_search_, sizeof(squad_search_));
    ImGui::SameLine(0, 8.0f * dpi_scale_);
    if (icon_btn_primary(Icon::Refresh, T(L, "Atualizar", "Refresh"))) reload_squad();

    ImGui::Spacing();

    // Conectado a uma carreira e sem nenhum atleta: diz o que houve em vez de mostrar uma tabela em branco
    if (squad_.players.empty() && st_.attached && st_.career > 0) {
        wrapped_colored(col::GoldSoft, "%s", fmt_code_msg(L, "squad_not_found").c_str());
        ImGui::Spacing();
    }

    int n = static_cast<int>(squad_.players.size());
    int exp_c = 0, inj_c = 0, u23_c = 0;
    double sum_ovr = 0.0, sum_age = 0.0;
    for (const auto& p : squad_.players) {
        sum_ovr += p.ovr;
        sum_age += p.age;
        if (p.is_expiring) ++exp_c;
        if (p.injury > 0) ++inj_c;
        if (p.age <= 23) ++u23_c;
    }

    int kcols = (ImGui::GetContentRegionAvail().x < 720.0f * dpi_scale_) ? 2 : 4;
    if (ScopedCardGrid g{"##sq_kpis", kcols, dpi_scale_}) {
        auto kpi = [&](const char* id, const char* lbl, const char* val, const ImVec4& vc, const ImVec4* acc = nullptr) {
            ImGui::TableNextColumn();
            begin_studio_card(id, 0.0f, false, acc);
            section_label(lbl, fonts);
            if (fonts.title_bold) ImGui::PushFont(fonts.title_bold);
            ImGui::TextColored(vc, "%s", val);
            if (fonts.title_bold) ImGui::PopFont();
            end_studio_card();
        };
        char b1[16], b2[16], b3[16], b4[24];
        std::snprintf(b1, sizeof(b1), "%d", n);
        if (n > 0) std::snprintf(b2, sizeof(b2), "%.1f", sum_ovr / n); else std::snprintf(b2, sizeof(b2), "--");
        if (n > 0) std::snprintf(b3, sizeof(b3), "%.1f%s", sum_age / n, T(L, "a", "y")); else std::snprintf(b3, sizeof(b3), "--");
        std::snprintf(b4, sizeof(b4), "%d / %d", inj_c, exp_c);
        kpi("##sk1", T(L, "ATLETAS", "PLAYERS"), b1, col::Text, &col::BorderLit);
        kpi("##sk2", T(L, "OVR MÉDIO", "AVG OVR"), b2, col::AccentSoft, &col::Accent);
        kpi("##sk3", T(L, "IDADE MÉDIA", "AVG AGE"), b3, col::Text, &col::Cyan);
        kpi("##sk4", T(L, "LESIONADOS / FIM CONTR.", "INJURED / EXPIRING"), b4, col::GoldSoft, &col::Gold);
    }

    ImGui::Spacing();
    // Filter pills + quick squad commands (auto-wrapping)
    auto flt_btn = [&](int idx, const char* lbl, int cnt, bool first) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s (%d)###f%d", lbl, cnt, idx);
        if (!first) flow_next(calc_btn_w(buf));
        bool act = (squad_filter_ == idx);
        if (styled_button(buf, act ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.24f) : col::BgCard, act ? col::Accent : col::Border, act ? col::AccentSoft : col::Subtext)) {
            squad_filter_ = idx;
        }
    };
    flt_btn(0, T(L, "Todos", "All"), n, true);
    flt_btn(1, T(L, "Fim de Contrato", "Expiring"), exp_c, false);
    flt_btn(2, T(L, "Lesionados", "Injured"), inj_c, false);
    flt_btn(3, T(L, "Sub-23", "U23"), u23_c, false);

    ImGui::Spacing();
    if (icon_btn_green(Icon::Battery, T(L, "100% Fôlego##sq", "100% Stamina##sq"))) run_squad_cmd("stamina");
    flow_next(icon_button_width(T(L, "Seta Azul", "Blue Arrow")));
    if (icon_btn_primary(Icon::ArrowUp, T(L, "Seta Azul##sq", "Blue Arrow##sq"))) run_squad_cmd("arrow");
    flow_next(icon_button_width(T(L, "Curar Lesões", "Heal Injuries")));
    if (icon_btn_green(Icon::Cross, T(L, "Curar Lesões##sq", "Heal Injuries##sq"))) run_squad_cmd("heal");
    flow_next(icon_button_width(T(L, "Contratos +5a", "Contracts +5y")));
    if (icon_btn_gold(Icon::Contract, T(L, "Contratos +5a##sq", "Contracts +5y##sq"))) run_squad_cmd("contract");
    flow_next(icon_button_width("+1 OVR"));
    if (icon_btn_plain(Icon::ArrowUp, "+1 OVR##sq")) run_squad_cmd("ovr1");
    flow_next(icon_button_width(T(L, "Recalibrar Economia", "Recalibrate Economy")));
    if (icon_btn_gold(Icon::Coins, T(L, "Recalibrar Economia##sq", "Recalibrate Economy##sq"))) run_squad_cmd("econ_recal");
    show_tip_if_hovered(T(L, "Recalibrar Economia do Elenco", "Recalibrate Squad Economy"),
        T(L, "Recalcula mercado, salario e rescisao de todo o elenco pela curva Transfermarkt 2025/26.",
             "Recalculates market value, salary and release fee for the entire squad."));
    flow_next(icon_button_width(T(L, "Exportar Elenco", "Export Squad")));
    if (icon_btn_primary(Icon::Export, T(L, "Exportar Elenco##sq", "Export Squad##sq"))) open_pack_studio(true, true);
    show_tip_if_hovered(T(L, "Exportar Pack do Elenco", "Export Squad Pack"),
        T(L, "Salva OVR, atributos, posicoes, habilidades, estilo, bio e economia do elenco inteiro em JSON para reaplicar em outros saves.",
             "Saves OVR, stats, positions, skills, style, bio and economy of the whole squad to JSON for cross-save import."));
    flow_next(icon_button_width(T(L, "Importar / Cruzar Pack", "Import / Match Pack")));
    if (icon_btn_green(Icon::Import, T(L, "Importar / Cruzar Pack##sq", "Import / Match Pack##sq"))) open_pack_studio(false, true);
    show_tip_if_hovered(T(L, "Importar e Cruzar Elenco", "Import & Match Squad Pack"),
        T(L, "Cruza um pack salvo contra o elenco atual ou base global por ID/nome, mostra preview completo e aplica seletivamente.",
             "Matches a saved pack against your current squad or global database by ID/name with full preview before applying."));
    int batch_cnt = engine_.last_batch_undo_count();
    {
        // Sempre presente (desativado sem lote): evita uma nova linha surgindo depois de uma acao em massa
        char blbl[64];
        std::snprintf(blbl, sizeof(blbl), T(L, "Desfazer Lote (%d)###sqbu", "Undo Batch (%d)###sqbu"), batch_cnt);
        flow_next(calc_btn_w(blbl));
        ImGui::BeginDisabled(batch_cnt <= 0);
        if (btn_red(blbl)) {
            auto r = engine_.undo_last_batch();
            if (r.ok) {
                char msg[96];
                std::snprintf(msg, sizeof(msg), T(L, "Lote desfeito: %d atleta(s) restaurado(s)!", "Batch undone: %d player(s) restored!"), r.count);
                toasts_.push(msg);
                reload_squad();
                if (has_player_details_) reload_player();
                wake_poller();
            } else toast_code(r.code, true);
        }
        ImGui::EndDisabled();
    }

    ImGui::Spacing();

    std::string qn = norm_text_utf8(squad_search_);
    std::vector<const SquadRow*> rows;
    rows.reserve(squad_.players.size());
    for (const auto& p : squad_.players) {
        if (squad_filter_ == 1 && !p.is_expiring) continue;
        if (squad_filter_ == 2 && p.injury <= 0) continue;
        if (squad_filter_ == 3 && p.age > 23) continue;
        if (!qn.empty() && p.name_norm.find(qn) == std::string::npos) continue;
        rows.push_back(&p);
    }

    // ---- Selecao em lote + presets de habilidades ----
    {
        int nsel = 0;
        for (const auto& p : squad_.players) if (squad_sel_.count(p.addr)) ++nsel;
        begin_studio_card("##sq_sel", 0.0f, false, nsel > 0 ? &col::Accent : &col::BorderLit);
        char sh[96];
        std::snprintf(sh, sizeof(sh), "%s  (%d)", T(L, "SELEÇÃO EM LOTE", "BATCH SELECTION"), nsel);
        section_label(sh, fonts);
        auto sel_btn = [&](const char* lbl, bool first) {
            if (!first) flow_next(calc_btn_w(lbl));
            return styled_button(lbl, col::BgCard, col::Border, col::Subtext);
        };
        if (sel_btn(T(L, "Visíveis##ssel", "Visible##ssel"), true)) { for (const SquadRow* p : rows) squad_sel_.insert(p->addr); }
        show_tip_if_hovered(T(L, "Selecionar visíveis", "Select visible"), T(L, "Marca todos os atletas que passam no filtro atual e na busca.", "Selects every player matching the current filter and search."));
        if (sel_btn(T(L, "Nenhum##ssel", "None##ssel"), false)) squad_sel_.clear();
        if (sel_btn(T(L, "Inverter##ssel", "Invert##ssel"), false)) {
            for (const SquadRow* p : rows) { if (!squad_sel_.erase(p->addr)) squad_sel_.insert(p->addr); }
        }
        if (sel_btn(T(L, "Só Sub-23##ssel", "U23 only##ssel"), false)) { squad_sel_.clear(); for (const auto& p : squad_.players) if (p.age <= 23) squad_sel_.insert(p.addr); }
        if (sel_btn(T(L, "Só fim de contrato##ssel", "Expiring only##ssel"), false)) { squad_sel_.clear(); for (const auto& p : squad_.players) if (p.is_expiring) squad_sel_.insert(p.addr); }
        if (sel_btn(T(L, "Só lesionados##ssel", "Injured only##ssel"), false)) { squad_sel_.clear(); for (const auto& p : squad_.players) if (p.injury > 0) squad_sel_.insert(p.addr); }

        ImGui::Spacing();
        // Acoes sempre visiveis (desativadas sem selecao): marcar atletas nao muda a altura do card nem desloca a tabela
        wrapped_colored(col::Muted, "%s", T(L, "Marque atletas na tabela (caixa à esquerda do OVR) ou use os atalhos acima para liberar as ações em lote.",
                                                "Tick players in the table (box left of OVR) or use the shortcuts above to unlock batch actions."));
        ImGui::BeginDisabled(nsel == 0);
        {
        if (icon_btn_gold(Icon::Coins, T(L, "Recalibrar Economia##selb", "Recalibrate Economy##selb"))) run_squad_cmd_sel("econ_recal");
        show_tip_if_hovered(T(L, "Recalibrar economia da seleção", "Recalibrate selection economy"),
            T(L, "Recalcula mercado, salário e rescisão só dos atletas marcados. Salva um backup JSON do elenco antes.", "Recalculates market, salary and release fee for the ticked players only. Saves a JSON squad backup first."));
        flow_next(icon_button_width("+1 OVR (DNA)"));
        if (icon_btn_plain(Icon::ArrowUp, "+1 OVR (DNA)##selb")) run_squad_cmd_sel("ovr1");
        flow_next(icon_button_width(T(L, "Fôlego 100%", "Stamina 100%")));
        if (icon_btn_green(Icon::Battery, T(L, "Fôlego 100%##selb", "Stamina 100%##selb"))) run_squad_cmd_sel("stamina");
        flow_next(icon_button_width(T(L, "Curar", "Heal")));
        if (icon_btn_green(Icon::Cross, T(L, "Curar##selb", "Heal##selb"))) run_squad_cmd_sel("heal");
        flow_next(icon_button_width(T(L, "Contrato +5a", "Contract +5y")));
        if (icon_btn_gold(Icon::Contract, T(L, "Contrato +5a##selb", "Contract +5y##selb"))) run_squad_cmd_sel("contract");
        flow_next(icon_button_width(T(L, "Seta azul piscando", "Blinking blue arrow")));
        if (icon_btn_primary(Icon::ArrowUp, T(L, "Seta azul piscando##selb", "Blinking blue arrow##selb"))) run_squad_cmd_sel("arrow");

        ImGui::Spacing();
        render_preset_controls(fonts, true);
        flow_next(icon_button_width(T(L, "Aplicar preset à seleção##selb", "Apply preset to selection##selb")), 8.0f * dpi_scale_);
        if (icon_btn_primary(Icon::Check, T(L, "Aplicar preset à seleção##selb", "Apply preset to selection##selb"))) apply_preset_cmd(true);
        }
        ImGui::EndDisabled();
        end_studio_card();
        ImGui::Spacing();
    }

    ImGuiTableFlags tf = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                         ImGuiTableFlags_ScrollX | ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX;
    float tbl_h = std::max(260.0f * dpi_scale_, ImGui::GetContentRegionAvail().y - 6.0f);
    float w_ovr = std::round(48.0f * dpi_scale_ + 30.0f * dpi_scale_);
    float w_pos = std::round(54.0f * dpi_scale_);
    float w_age = std::round(44.0f * dpi_scale_);
    float w_con = std::round(76.0f * dpi_scale_);
    float w_sal = std::round(98.0f * dpi_scale_);
    float w_frm = std::round(54.0f * dpi_scale_);
    float w_stm = std::round(92.0f * dpi_scale_);
    float w_hlt = std::round(48.0f * dpi_scale_);
    float w_act = std::round(110.0f * dpi_scale_);
    // Colunas opcionais somem em janelas estreitas (ordem: barra de folego, forma) para nunca cortar as acoes
    bool show_stm = true, show_frm = true;
    const float sq_avail = ImGui::GetContentRegionAvail().x;
    const float sq_name_min = std::round(140.0f * dpi_scale_);
    auto sq_need = [&]() {
        int ncols = 8 + (show_stm ? 1 : 0) + (show_frm ? 1 : 0);
        float fixed = w_ovr + w_pos + w_age + w_con + w_sal + w_hlt + w_act + (show_stm ? w_stm : 0.0f) + (show_frm ? w_frm : 0.0f);
        return fixed + sq_name_min + ImGui::GetStyle().CellPadding.x * 2.0f * static_cast<float>(ncols) + ImGui::GetStyle().ScrollbarSize + 14.0f * dpi_scale_;
    };
    if (sq_need() > sq_avail) show_stm = false;
    if (sq_need() > sq_avail) show_frm = false;
    float pad_est = ImGui::GetStyle().CellPadding.x * 2.0f * static_cast<float>(8 + (show_stm ? 1 : 0) + (show_frm ? 1 : 0)) + ImGui::GetStyle().ScrollbarSize + 14.0f * dpi_scale_;
    float w_name = std::max(sq_name_min,
                            sq_avail - (w_ovr + w_pos + w_age + w_con + w_sal + w_hlt + w_act + (show_stm ? w_stm : 0.0f) + (show_frm ? w_frm : 0.0f) + pad_est));
    if (ImGui::BeginTable("##squad_tbl", 10, tf, ImVec2(0, tbl_h))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("OVR", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, w_ovr, 0);
        ImGui::TableSetupColumn("POS", ImGuiTableColumnFlags_WidthFixed, w_pos, 1);
        ImGui::TableSetupColumn(T(L, "ATLETA", "PLAYER"), ImGuiTableColumnFlags_WidthFixed, w_name, 2);
        ImGui::TableSetupColumn(T(L, "IDADE", "AGE"), ImGuiTableColumnFlags_WidthFixed, w_age, 3);
        ImGui::TableSetupColumn(T(L, "CONTRATO", "CONTRACT"), ImGuiTableColumnFlags_WidthFixed, w_con, 4);
        ImGui::TableSetupColumn(T(L, "SALÁRIO", "SALARY"), ImGuiTableColumnFlags_WidthFixed, w_sal, 5);
        ImGui::TableSetupColumn(T(L, "FORMA", "FORM"), ImGuiTableColumnFlags_WidthFixed | (show_frm ? 0 : ImGuiTableColumnFlags_Disabled), w_frm, 6);
        ImGui::TableSetupColumn(T(L, "FÔLEGO", "STAMINA"), ImGuiTableColumnFlags_WidthFixed | (show_stm ? 0 : ImGuiTableColumnFlags_Disabled), w_stm, 7);
        ImGui::TableSetupColumn(T(L, "SAÚDE", "HEALTH"), ImGuiTableColumnFlags_WidthFixed, w_hlt, 8);
        ImGui::TableSetupColumn(T(L, "AÇÕES", "ACTIONS"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, w_act, 9);
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
            if (specs->SpecsCount > 0) {
                squad_sort_col_ = specs->Specs[0].ColumnUserID;
                squad_sort_asc_ = (specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending);
            }
        }
        std::sort(rows.begin(), rows.end(), [&](const SquadRow* a, const SquadRow* b) {
            int cmp = 0;
            switch (squad_sort_col_) {
                case 0: cmp = a->ovr - b->ovr; break;
                case 1: cmp = a->pos - b->pos; break;
                case 2: cmp = a->name.compare(b->name); break;
                case 3: cmp = a->age - b->age; break;
                case 4: cmp = a->exp_y - b->exp_y; break;
                case 5: cmp = (a->salary_raw < b->salary_raw) ? -1 : (a->salary_raw > b->salary_raw ? 1 : 0); break;
                case 6: cmp = a->arrow - b->arrow; break;
                case 7: cmp = a->stamina - b->stamina; break;
                case 8: cmp = a->injury - b->injury; break;
            }
            if (cmp == 0) return a->ovr > b->ovr;
            return squad_sort_asc_ ? (cmp < 0) : (cmp > 0);
        });

        const float row_h = std::round(30.0f * dpi_scale_);
        bool need_reload_squad = false;
        for (const SquadRow* p : rows) {
            ImGui::TableNextRow(0, row_h);
            ImGui::PushID(static_cast<int>(p->addr & 0x7FFFFFFF));

            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable("##sq_row", p->addr == last_player_addr_,
                                  ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap,
                                  ImVec2(0.0f, row_h))) {
                open_player_in_studio(p->addr);
            }
            ImGui::SameLine(0, 0);
            {
                bool sel_v = squad_sel_.count(p->addr) != 0;
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f * dpi_scale_, 2.0f * dpi_scale_));
                if (ImGui::Checkbox("##sel", &sel_v)) { if (sel_v) squad_sel_.insert(p->addr); else squad_sel_.erase(p->addr); }
                ImGui::PopStyleVar();
                ImGui::SameLine(0, 6.0f * dpi_scale_);
            }
            draw_ovr_badge(p->ovr, std::round(38.0f * dpi_scale_), std::round(23.0f * dpi_scale_), bold);
            ImGui::TableSetColumnIndex(1); draw_pos_chip(L, p->pos, std::round(44.0f * dpi_scale_), std::round(23.0f * dpi_scale_), bold);
            ImGui::TableSetColumnIndex(2);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(p->name.c_str());
            ImGui::TableSetColumnIndex(3); ImGui::Text("%d%s", p->age, T(L, "a", "y"));
            ImGui::TableSetColumnIndex(4);
            if (p->exp_y > 0) ImGui::TextColored(p->is_expiring ? col::GoldSoft : col::Subtext, "%d", p->exp_y);
            else ImGui::TextColored(col::Muted, "-");
            ImGui::TableSetColumnIndex(5); ImGui::Text("%s", fmt_money(p->salary_raw, settings_.currency_idx, L).c_str());
            ImGui::TableSetColumnIndex(6); draw_arrow_badge(p->arrow, std::round(22.0f * dpi_scale_), p->blink, settings_.lang);
            ImGui::TableSetColumnIndex(7); draw_stamina_bar(p->stamina, 62.0f);
            ImGui::TableSetColumnIndex(8);
            if (p->injury > 0) {
                char ib[16]; std::snprintf(ib, sizeof(ib), "%dd", p->injury);
                draw_pill(ib, col::RedSoft, ImVec4(0.937f, 0.267f, 0.267f, 0.16f), ImVec4(0.937f, 0.267f, 0.267f, 0.45f));
            } else {
                draw_pill("OK", col::GreenSoft, ImVec4(0.063f, 0.725f, 0.506f, 0.16f), ImVec4(0.063f, 0.725f, 0.506f, 0.45f));
            }
            ImGui::TableSetColumnIndex(9);
            if (p->injury > 0) {
                if (icon_btn_green(Icon::Cross, T(L, "Curar", "Heal"))) {
                    engine_.set_player_quick_condition(std::nullopt, std::nullopt, true, p->addr);
                    need_reload_squad = true;
                }
            } else if (p->stamina < 100) {
                if (btn_green("100%")) {
                    engine_.set_player_quick_condition(100, std::nullopt, false, p->addr);
                    need_reload_squad = true;
                }
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
        if (need_reload_squad) reload_squad();
    }
    if (squad_reload_pending_) {
        squad_reload_pending_ = false;
        reload_squad();
    }
}

void WorkbenchApp::render_view_finances(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    page_title(T(L, "Finanças da Master League", "Master League Finances"), fonts, bold);
    const char* ref_lbl = T(L, "Atualizar##fin", "Refresh##fin");
    align_right_or_wrap(calc_btn_w(ref_lbl));
    if (btn_primary(ref_lbl)) reload_finances();
    wrapped_colored(col::Subtext, "%s", T(L, "Cálculo exato de Salários Disponíveis descontando folha ativa e pendentes (0x14156D170).",
                                                "Exact Available Salary Budget deducting active squad wages and pending overrides (0x14156D170)."));
    ImGui::Spacing();

    int kcols = (ImGui::GetContentRegionAvail().x < 820.0f * dpi_scale_) ? 2 : 4;
    float med_sz = std::round(24.0f * dpi_scale_);
    if (ScopedCardGrid g{"##fin_kpis", kcols, dpi_scale_}) {
        auto fk = [&](const char* id, int med_kind, const char* lbl, int32_t raw, const ImVec4& c, const ImVec4& acc) {
            ImGui::TableNextColumn();
            begin_studio_card(id, 0.0f, false, &acc);
            draw_kpi_medallion(med_kind, med_sz, c);
            ImGui::SameLine(0, 9.0f * dpi_scale_);
            ImGui::BeginGroup();
            section_label(lbl, fonts);
            if (fonts.title_bold) ImGui::PushFont(fonts.title_bold);
            ImGui::TextColored(c, "%s", fmt_money(raw, settings_.currency_idx, L).c_str());
            if (fonts.title_bold) ImGui::PopFont();
            ImGui::EndGroup();
            end_studio_card();
        };
        fk("##fk1", 1, T(L, "TRANSFERÊNCIAS", "TRANSFER BUDGET"), fin_.transfer_raw, col::GreenSoft, col::Green);
        fk("##fk2", 2, T(L, "SALÁRIOS DISPONÍVEIS", "AVAILABLE SALARY"), fin_.salary_avail_raw,
           fin_.salary_avail_raw < 0 ? col::RedSoft : col::GoldSoft,
           fin_.salary_avail_raw < 0 ? col::Red : col::Gold);
        fk("##fk3", 7, T(L, "FOLHA ANUAL ATIVA", "ACTIVE ANNUAL WAGES"), fin_.squad_wages_raw, col::Orange, col::Orange);
        fk("##fk4", 6, T(L, "TETO SALARIAL BRUTO", "GROSS SALARY CAP"), fin_.salary_gross_raw, col::AccentSoft, col::Accent);
    }

    ImGui::Spacing();
    begin_studio_card("##fin_bar_card");
    float ratio = (fin_.salary_gross_raw > 0) ? std::clamp(static_cast<float>(fin_.squad_wages_raw) / static_cast<float>(fin_.salary_gross_raw), 0.0f, 1.0f) : 0.0f;
    ImGui::TextColored(col::Subtext, "%s: %.1f%%", T(L, "Uso da Folha Salarial sobre o Teto Bruto", "Wage Bill Utilization vs Gross Cap"), ratio * 100.0f);
    {
        char leg1[48], leg2[48];
        std::snprintf(leg1, sizeof(leg1), "● %s %.1f%%", T(L, "Folha Ativa", "Active Wages"), ratio * 100.0f);
        std::snprintf(leg2, sizeof(leg2), "● %s %.1f%%", T(L, "Livre", "Free"), (1.0f - ratio) * 100.0f);
        float leg_w = ImGui::CalcTextSize(leg1).x + ImGui::CalcTextSize(leg2).x + 20.0f * dpi_scale_;
        align_right_or_wrap(leg_w);
        if (fonts.small_bold) ImGui::PushFont(fonts.small_bold);
        ImGui::TextColored(col::GoldSoft, "%s", leg1);
        ImGui::SameLine(0, 14.0f * dpi_scale_);
        ImGui::TextColored(col::GreenSoft, "%s", leg2);
        if (fonts.small_bold) ImGui::PopFont();
    }
    ImGui::Spacing();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    float bar_h = std::round(12.0f * dpi_scale_);
    ImGui::Dummy(ImVec2(w, bar_h));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + bar_h), ImGui::GetColorU32(col::Green), bar_h * 0.5f);
    if (ratio > 0.01f) dl->AddRectFilled(p, ImVec2(p.x + w * ratio, p.y + bar_h), ImGui::GetColorU32(col::Gold), bar_h * 0.5f);
    dl->AddRect(p, ImVec2(p.x + w, p.y + bar_h), ImGui::GetColorU32(col::Border), bar_h * 0.5f);
    end_studio_card();

    ImGui::Spacing();
    begin_studio_card("##fin_edit_card");
    card_header(T(L, "Editar Orçamentos", "Edit Budgets"), bold);
    ImGui::Separator();
    ImGui::Spacing();

    int ecols = (ImGui::GetContentRegionAvail().x < 620.0f * dpi_scale_) ? 1 : 2;
    bool e1 = false, e2 = false;
    if (ScopedCardGrid g{"##fin_inputs", ecols, dpi_scale_}) {
        {
            ScopedFormCell c;
            section_label(T(L, "ORÇAMENTO DE TRANSFERÊNCIAS (EX: 50M, 500K)", "TRANSFER BUDGET (E.G. 50M, 500K)"), fonts);
            ImGui::SetNextItemWidth(-1.0f);
            e1 = ImGui::InputText("##fin_trf", fin_trf_buf_, sizeof(fin_trf_buf_), ImGuiInputTextFlags_EnterReturnsTrue);
        }
        {
            ScopedFormCell c;
            section_label(T(L, "SALÁRIOS DISPONÍVEIS (EX: 15M, 2.5M)", "AVAILABLE SALARY BUDGET (E.G. 15M, 2.5M)"), fonts);
            ImGui::SetNextItemWidth(-1.0f);
            e2 = ImGui::InputText("##fin_sal", fin_sal_buf_, sizeof(fin_sal_buf_), ImGuiInputTextFlags_EnterReturnsTrue);
        }
    }

    ImGui::Spacing();
    if (icon_btn_primary(Icon::Check, T(L, "Aplicar Valores", "Apply Values")) || e1 || e2) {
        auto r = engine_.set_finances(std::string_view(fin_trf_buf_), std::string_view(fin_sal_buf_));
        if (r.ok) { fin_ = r; reload_finances(); toasts_.push(T(L, "Finanças atualizadas!", "Finances updated!")); }
        else toast_code(r.code, true);
    }
    flow_next(calc_btn_w(T(L, "+10M Transf.", "+10M Transfer")));
    if (btn_green(T(L, "+10M Transf.", "+10M Transfer"))) { auto r = engine_.add_finance_delta(true, 100000); if (r.ok) { fin_ = r; reload_finances(); } else toast_code(r.code, true); }
    flow_next(calc_btn_w(T(L, "+100M Transf.", "+100M Transfer")));
    if (btn_green(T(L, "+100M Transf.", "+100M Transfer"))) { auto r = engine_.add_finance_delta(true, 1000000); if (r.ok) { fin_ = r; reload_finances(); } else toast_code(r.code, true); }
    flow_next(calc_btn_w(T(L, "+10M Salários", "+10M Salary")));
    if (btn_gold(T(L, "+10M Salários", "+10M Salary"))) { auto r = engine_.add_finance_delta(false, 100000); if (r.ok) { fin_ = r; reload_finances(); } else toast_code(r.code, true); }
    flow_next(calc_btn_w(T(L, "+50M Salários", "+50M Salary")));
    if (btn_gold(T(L, "+50M Salários", "+50M Salary"))) { auto r = engine_.add_finance_delta(false, 500000); if (r.ok) { fin_ = r; reload_finances(); } else toast_code(r.code, true); }
    end_studio_card();
}

void WorkbenchApp::render_view_player(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    if (!has_player_details_) {
        begin_studio_card("##p_empty");
        page_title(T(L, "Nenhum atleta selecionado", "No player selected"), fonts, bold);
        ImGui::TextWrapped("%s", T(L, "Destaque um jogador no Plano de Jogo do PES 2021 ou selecione diretamente em Meu Elenco ou Buscar Jogador.",
                                      "Highlight a player in PES 2021 Game Plan or pick one from My Squad or Search Player."));
        ImGui::Spacing();
        if (btn_primary(T(L, "Abrir Meu Elenco", "Open My Squad"))) switch_view(View::Squad);
        flow_next(calc_btn_w(T(L, "Buscar nos 30.000+ Atletas", "Search 30,000+ Players")));
        if (ImGui::Button(T(L, "Buscar nos 30.000+ Atletas", "Search 30,000+ Players"))) switch_view(View::Search);
        end_studio_card();
        return;
    }

    // Broadcast Player Hero Card (gradient surface + illuminated 58px OVR Shield + Right-Aligned Actions on Wide + Economy Sync + Compact Persistent Delta Strip)
    begin_studio_card("##p_hero", 0.0f, true);
    const float hero_avail_w = ImGui::GetContentRegionAvail().x;
    const char* fav_lbl = T(L, "+ Listas", "+ Lists");
    const char* exp_lbl = T(L, "Exportar JSON", "Export JSON");
    const char* imp_lbl = T(L, "Importar JSON", "Import JSON");
    const float act_gap = std::round(6.0f * dpi_scale_);
    const float acts_total_w = icon_button_width("-1 OVR") + icon_button_width("+1 OVR") + icon_button_width(fav_lbl) +
                               icon_button_width(exp_lbl) + icon_button_width(imp_lbl) + act_gap * 4.0f;
    const bool hero_inline_acts = (hero_avail_w >= std::max(860.0f * dpi_scale_, acts_total_w + 460.0f * dpi_scale_));

    auto draw_hero_actions = [&](bool inline_row) {
        if (icon_btn_plain(Icon::ArrowDown, "-1 OVR##hero")) {
            auto r = engine_.step_player_ovr(-1);
            if (r.ok) { player_ = r; sync_player_form_buffers(); record_solver_action(r, "-1 OVR DNA", "-1 OVR DNA"); wake_poller(); }
            else toast_code(r.code, true);
        }
        show_tip_if_hovered("-1 OVR", T(L, "Desce 1 ponto de Overall geral mantendo o DNA de atributos.", "Lowers Overall by 1 while preserving stat DNA."));
        if (inline_row) ImGui::SameLine(0, act_gap); else flow_next(icon_button_width("+1 OVR"), act_gap);
        if (icon_btn_primary(Icon::ArrowUp, "+1 OVR##hero")) {
            auto r = engine_.step_player_ovr(1);
            if (r.ok) { player_ = r; sync_player_form_buffers(); record_solver_action(r, "+1 OVR DNA", "+1 OVR DNA"); wake_poller(); }
            else toast_code(r.code, true);
        }
        show_tip_if_hovered("+1 OVR", T(L, "Sobe 1 ponto de Overall geral mantendo o DNA de atributos.", "Raises Overall by 1 while preserving stat DNA."));
        if (inline_row) ImGui::SameLine(0, act_gap); else flow_next(icon_button_width(fav_lbl), act_gap);
        if (icon_btn_green(Icon::List, fav_lbl)) {
            reload_scouts();
            ImGui::OpenPopup("##hero_lists");
        }
        show_tip_if_hovered(T(L, "Olheiros e Favoritos", "Scouts & Shortlist"),
            T(L, "Marca ou desmarca este atleta em Olheiro 1, 2, 3 ou Favoritos.", "Ticks or unticks this player in Scout 1, 2, 3 or Shortlist."));
        render_scout_list_menu("##hero_lists", player_.addr);
        if (inline_row) ImGui::SameLine(0, act_gap); else flow_next(icon_button_width(exp_lbl), act_gap);
        if (icon_btn_plain(Icon::Export, exp_lbl)) {
            open_pack_studio(true, false);
        }
        if (inline_row) ImGui::SameLine(0, act_gap); else flow_next(icon_button_width(imp_lbl), act_gap);
        if (icon_btn_plain(Icon::Import, imp_lbl)) {
            open_pack_studio(false, false);
        }
    };

    float hero_top_y = ImGui::GetCursorPosY();
    draw_ovr_shield(player_.ovr, std::round(58.0f * dpi_scale_), fonts.large_num ? fonts.large_num : bold, fonts.small_bold);
    ImGui::SameLine(0, 14.0f * dpi_scale_);
    ImGui::BeginGroup();
    draw_pos_chip(L, player_.reg_pos, std::round(42.0f * dpi_scale_), std::round(24.0f * dpi_scale_), bold);
    ImGui::SameLine(0, 9.0f * dpi_scale_);
    page_title(player_.name.c_str(), fonts, bold);
    ImGui::SameLine(0, 9.0f * dpi_scale_);
    draw_arrow_badge(player_.arrow, std::round(23.0f * dpi_scale_), player_.blink, settings_.lang);

    const char* foot_txt = player_.foot ? T(L, "Canhoto", "Left Foot") : T(L, "Destro", "Right Foot");
    ImGui::PushStyleColor(ImGuiCol_Text, col::Subtext);
    ImGui::Text("%s  |  ID %u  |  %d%s  |  %s  |  %s  |  %s/%s",
        fmt_club(L, player_.club).c_str(), player_.pid, player_.age, T(L, "a", "y"), foot_txt,
        fmt_money(player_.market_raw, settings_.currency_idx, L).c_str(),
        fmt_money(player_.salary_raw, settings_.currency_idx, L).c_str(), T(L, "ano", "yr"));
    ImGui::PopStyleColor();
    draw_stamina_bar(player_.stamina, 96.0f);
    ImGui::EndGroup();

    if (hero_inline_acts) {
        float right_x = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - acts_total_w;
        float after_info_x = ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x;
        if (right_x > after_info_x + 12.0f * dpi_scale_) {
            float saved_bottom_y = ImGui::GetCursorPosY();
            ImGui::SetCursorPos(ImVec2(right_x, hero_top_y + std::round(8.0f * dpi_scale_)));
            draw_hero_actions(true);
            ImGui::SetCursorPosY(std::max(saved_bottom_y, ImGui::GetCursorPosY()));
        } else {
            ImGui::Spacing();
            draw_hero_actions(false);
        }
    } else {
        ImGui::Spacing();
        draw_hero_actions(false);
    }

    // Integrated OVR-Linked Economy Sync Strip
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    {
        const char* econ_opts[4] = {
            T(L, "Desativado (Manual)", "Off (Manual)"),
            T(L, "Tudo (Mercado + Salário + Rescisão)", "All (Market + Salary + Release)"),
            T(L, "Só Mercado e Rescisão", "Market & Release Only"),
            T(L, "Só Contrato (Salário)", "Contract Salary Only"),
        };
        int cur_em = std::clamp(settings_.econ_sync_mode, 0, 3);
        ImGui::AlignTextToFramePadding();
        if (bold) ImGui::PushFont(bold);
        ImGui::TextColored(cur_em == 0 ? col::Muted : col::GreenSoft, "%s",
            T(L, "Economia ao editar OVR:", "Auto-sync Economy on OVR:"));
        if (bold) ImGui::PopFont();
        show_tip_if_hovered(
            T(L, "Ajuste Econômico Automático por OVR", "OVR-Linked Economy Auto-Sync"),
            T(L, "Ajusta imediatamente Valor de Mercado, Salário do Contrato e Taxa de Rescisão ao alterar o OVR. Escala a partir do valor original do atleta levando em conta OVR, idade, posição e país.",
                 "Immediately scales Market Value, Contract Salary and Release Fee whenever OVR changes. Anchored to the player's original value considering OVR, age, position and country."));

        float econ_w = calc_combo_width(econ_opts, 4, 240.0f * dpi_scale_, 380.0f * dpi_scale_);
        flow_next(econ_w);
        ImGui::SetNextItemWidth(std::min(econ_w, ImGui::GetContentRegionAvail().x));
        if (ImGui::BeginCombo("##econ_sync_sel", econ_opts[cur_em])) {
            for (int i = 0; i < 4; ++i) {
                if (ImGui::Selectable(econ_opts[i], cur_em == i)) {
                    settings_.econ_sync_mode = i;
                    engine_.set_econ_sync_mode(static_cast<EconSyncMode>(i));
                    save_settings(settings_);
                }
            }
            ImGui::EndCombo();
        }
        show_tip_if_hovered(
            T(L, "Modo de Sincronização Econômica", "Economy Sync Mode"),
            T(L, "Escolha se ao mudar o OVR devem subir ou descer Mercado + Salário + Rescisão, apenas Mercado e Rescisão, apenas Salário ou ficar manual.",
                 "Choose whether OVR edits update Market + Salary + Release Fee, Market & Release only, Salary only, or stay manual."));

        std::string rf_lbl;
        ImVec4 rf_fg = col::Muted;
        if (player_.has_release_fee && player_.release_fee_raw > 0) {
            rf_lbl = std::string(T(L, "Rescisão: ", "Release: ")) + fmt_money(player_.release_fee_raw, settings_.currency_idx, L);
            rf_fg = col::GoldSoft;
        } else {
            rf_lbl = T(L, "Rescisão: Sem multa", "Release: No clause");
        }
        flow_next(ImGui::CalcTextSize(rf_lbl.c_str()).x + 22.0f, 10.0f);
        draw_pill(rf_lbl.c_str(), rf_fg, ImVec4(rf_fg.x, rf_fg.y, rf_fg.z, 0.14f), ImVec4(rf_fg.x, rf_fg.y, rf_fg.z, 0.45f));
        show_tip_if_hovered(
            T(L, "Taxa de Rescisão (Multa Contratual)", "Release Fee Clause"),
            T(L, "Calculada em tempo real pela fórmula nativa do PES 2021 a partir do Valor de Mercado, idade, afinidade e OVR. Ative ou desative a multa na aba Condição & Bio.",
                 "Computed live by PES 2021's native formula from Market Value, age, affection and OVR. Toggle the clause in Condition & Bio."));

        const char* recal_lbl = T(L, "Recalibrar Economia", "Recalibrate Economy");
        flow_next(calc_btn_w(recal_lbl));
        bool snap_mismatch = (player_.snap_ovr >= 40 && player_.snap_ovr <= 99 && player_.snap_ovr != player_.ovr);
        if (snap_mismatch ? btn_gold(recal_lbl) : ImGui::Button(recal_lbl)) {
            auto r = engine_.recalibrate_player_economy(player_.addr);
            if (r.ok) {
                player_ = r;
                sync_player_form_buffers();
                capture_last_edit(r, "Recalibrar Economia", "Recalibrate Economy");
                wake_poller();
                std::string el = fmt_econ_delta_line(r.econ_delta);
                toasts_.push(el.empty() ? T(L, "Economia recalibrada!", "Economy recalibrated!")
                                        : (std::string(T(L, "Economia recalibrada!\n", "Economy recalibrated!\n")) + el));
            } else toast_code(r.code, true);
        }
        show_tip_if_hovered(T(L, "Recalibrar Economia", "Recalibrate Economy"),
            T(L, "Recalcula de forma deterministica o Valor de Mercado, Salario e Rescisao pela curva Transfermarkt 2025/26 (OVR, idade, posicao e pais) e define a nova base para edicoes futuras.",
                 "Deterministically recalculates Market Value, Salary and Release Fee from the calibrated 2025/26 curve (OVR, age, position and country) and re-anchors future edits."));
    }

    // Compact Persistent Inline Last-Edit Delta Strip (stays visible until dismissed or player changes!)
    {
        const bool le_on = last_edit_.active && last_edit_.player_addr == player_.addr;
        // Faixa sempre presente com altura fixa: aparecer ou sumir depois de uma edicao nao desloca o resto da pagina
        const float le_h = std::round(3.0f * (ImGui::GetFrameHeight() + 4.0f * dpi_scale_) + 12.0f * dpi_scale_);
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(col::BgInput.x, col::BgInput.y, col::BgInput.z, le_on ? 0.92f : 0.45f));
        ImGui::PushStyleColor(ImGuiCol_Border, le_on ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.45f) : col::Border);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(std::round(11.0f * dpi_scale_), std::round(6.0f * dpi_scale_)));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, std::round(8.0f * dpi_scale_));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(std::round(6.0f * dpi_scale_), std::round(4.0f * dpi_scale_)));
        ImGui::BeginChild("##p_last_edit_strip", ImVec2(-1.0f, le_h),
                          ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
        if (!le_on) {
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(col::Muted, "%s", T(L, "Nenhuma alteração recente. O que você mudar aparece aqui.",
                                                       "No recent change. Whatever you edit shows up here."));
        }
        if (le_on) {
        {
            ImVec2 bp = ImGui::GetWindowPos();
            ImVec2 bsz = ImGui::GetWindowSize();
            ImGui::GetWindowDrawList()->AddRectFilled(
                bp, ImVec2(bp.x + 3.5f * dpi_scale_, bp.y + bsz.y),
                ImGui::GetColorU32(col::Accent), std::round(8.0f * dpi_scale_), ImDrawFlags_RoundCornersLeft);
        }
        if (fonts.small_bold) ImGui::PushFont(fonts.small_bold);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(col::AccentSoft, "%s (%s)",
            T(L, last_edit_.label_pt.c_str(), last_edit_.label_en.c_str()), last_edit_.time_str.c_str());
        const char* undo_s = T(L, "Desfazer##le_u", "Undo##le_u");
        float right_w = (player_.undo_count > 0 ? calc_btn_w(undo_s) + 6.0f * dpi_scale_ : 0.0f) + ImGui::GetFrameHeight();
        align_right_or_wrap(right_w);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * dpi_scale_, 2.0f * dpi_scale_));
        if (player_.undo_count > 0) {
            if (btn_gold(undo_s)) { trigger_undo(); last_edit_.active = false; }
            ImGui::SameLine(0, 5.0f * dpi_scale_);
        }
        if (icon_only_button(Icon::Close, "##le_x", col::BgCard, col::Border, col::Subtext, ImGui::GetFrameHeight())) last_edit_.active = false;
        ImGui::PopStyleVar();

        bool first_chip = true;
        auto push_delta_pill = [&](const std::string& txt, const ImVec4& fg) {
            float pw = ImGui::CalcTextSize(txt.c_str()).x + 16.0f * dpi_scale_;
            if (!first_chip) flow_next(pw, 5.0f * dpi_scale_);
            first_chip = false;
            draw_pill(txt.c_str(), fg, ImVec4(fg.x, fg.y, fg.z, 0.15f), ImVec4(fg.x, fg.y, fg.z, 0.45f), true);
        };

        if (last_edit_.old_ovr > 0 && last_edit_.new_ovr > 0 && last_edit_.old_ovr != last_edit_.new_ovr) {
            int d = last_edit_.new_ovr - last_edit_.old_ovr;
            char b[48];
            std::snprintf(b, sizeof(b), "OVR %d -> %d (%+d)", last_edit_.old_ovr, last_edit_.new_ovr, d);
            push_delta_pill(b, d > 0 ? col::GreenSoft : col::RedSoft);
        }
        if (last_edit_.econ.applied) {
            auto add_money_pill = [&](const char* lbl, int32_t ov, int32_t nv) {
                if (ov == nv) return;
                bool up = (nv > ov);
                std::string s = std::string(lbl) + ": " + fmt_money(ov, settings_.currency_idx, L) +
                                " -> " + fmt_money(nv, settings_.currency_idx, L);
                push_delta_pill(s, up ? col::GreenSoft : col::GoldSoft);
            };
            add_money_pill(T(L, "Mercado", "Market"), last_edit_.econ.old_market_raw, last_edit_.econ.new_market_raw);
            add_money_pill(T(L, "Salário", "Salary"), last_edit_.econ.old_salary_raw, last_edit_.econ.new_salary_raw);
            if (last_edit_.econ.has_release_fee) {
                add_money_pill(T(L, "Rescisão", "Release"), last_edit_.econ.old_release_raw, last_edit_.econ.new_release_raw);
            }
        }
        size_t lim = std::min<size_t>(6, last_edit_.diffs.size());
        for (size_t i = 0; i < lim; ++i) {
            const auto& df = last_edit_.diffs[i];
            const auto* sd = find_stat_def(df.fid);
            const char* sn = sd ? T(L, sd->pt, sd->en) : "?";
            int d = df.new_val - df.old_val;
            char sb[64];
            std::snprintf(sb, sizeof(sb), "%s %+d (%d)", sn, d, df.new_val);
            push_delta_pill(sb, d > 0 ? col::AccentSoft : col::RedSoft);
        }
        if (last_edit_.diffs.size() > 6) {
            char mb[24];
            std::snprintf(mb, sizeof(mb), "+%zu %s", last_edit_.diffs.size() - 6, T(L, "atributos", "stats"));
            push_delta_pill(mb, col::Subtext);
        }
        if (fonts.small_bold) ImGui::PopFont();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(2);
    }
    end_studio_card();

    ImGui::Spacing();

    // 5 Player Sub-tabs (responsive auto-wrapping pills)
    auto ptab_btn = [&](PlayerTab pt, const char* lbl, bool first) {
        if (!first) flow_next(calc_btn_w(lbl), 6.0f);
        bool act = (current_ptab_ == pt);
        if (act && bold) ImGui::PushFont(bold);
        if (styled_button(lbl, act ? col::Accent : col::BgCard, act ? col::AccentSoft : col::Border, act ? ImVec4(1, 1, 1, 1) : col::Subtext)) {
            current_ptab_ = pt;
        }
        if (act && bold) ImGui::PopFont();
    };
    ptab_btn(PlayerTab::Bio, T(L, "Condição & Bio", "Condition & Bio"), true);
    ptab_btn(PlayerTab::Stats, T(L, "Atributos (25)", "Attributes (25)"), false);
    ptab_btn(PlayerTab::PosLab, T(L, "Lab de Posições", "Position Lab"), false);
    ptab_btn(PlayerTab::Skills, T(L, "Estilo & Habilidades", "Style & Skills"), false);
    ptab_btn(PlayerTab::Role, T(L, "Função no time", "Team Role"), false);
    ImGui::Spacing();

    switch (current_ptab_) {
        case PlayerTab::Bio:    render_ptab_bio(fonts); break;
        case PlayerTab::Stats:  render_ptab_stats(fonts); break;
        case PlayerTab::PosLab: render_ptab_poslab(fonts); break;
        case PlayerTab::Skills: render_ptab_skills(fonts); break;
        case PlayerTab::Role:   render_ptab_role(fonts); break;
    }
}

void WorkbenchApp::render_ptab_bio(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    int cols = (ImGui::GetContentRegionAvail().x < 820.0f * dpi_scale_) ? 1 : 2;
    float top_min_h = (cols == 2) ? std::round(214.0f * dpi_scale_) : 0.0f;
    if (ScopedCardGrid g{"##bio_top", cols, dpi_scale_}) {
        ImGui::TableNextColumn();
        begin_studio_card("##bio_stam", top_min_h, false, &col::Green);
        card_header(T(L, "Fôlego & Saúde Rápida", "Quick Stamina & Health"), bold);
        ImGui::SameLine(0, 10.0f * dpi_scale_);
        if (player_.injury > 0) {
            char ib[16]; std::snprintf(ib, sizeof(ib), "%dd", player_.injury);
            draw_pill(ib, col::RedSoft, ImVec4(0.937f, 0.267f, 0.267f, 0.16f), ImVec4(0.937f, 0.267f, 0.267f, 0.5f), true);
        } else {
            draw_pill("OK", col::GreenSoft, ImVec4(0.063f, 0.725f, 0.506f, 0.16f), ImVec4(0.063f, 0.725f, 0.506f, 0.5f), true);
        }
        ImGui::Spacing();
        if (icon_btn_green(Icon::Battery, T(L, "100% Fôlego##bio", "100% Stamina##bio"))) {
            auto r = engine_.set_player_quick_condition(100, std::nullopt, false);
            if (r.ok) { player_ = r; sync_player_form_buffers(); wake_poller(); toasts_.push(T(L, "Fôlego 100%!", "Stamina 100%!")); }
        }
        flow_next(icon_button_width(T(L, "Curar Lesão", "Heal Injury")));
        ImGui::BeginDisabled(player_.injury == 0);
        if (icon_btn_green(Icon::Cross, T(L, "Curar Lesão##bio", "Heal Injury##bio"))) {
            auto r = engine_.set_player_quick_condition(std::nullopt, std::nullopt, true);
            if (r.ok) { player_ = r; sync_player_form_buffers(); wake_poller(); toasts_.push(T(L, "Lesão curada!", "Injury healed!")); }
        }
        ImGui::EndDisabled();
        flow_next(std::round(165.0f * dpi_scale_));
        float stam_w = std::max(140.0f * dpi_scale_, ImGui::GetContentRegionAvail().x);
        if (draw_stamina_slider("##qstam", &quick_stam_, stam_w, bold)) {
            auto r = engine_.set_player_quick_condition(quick_stam_, std::nullopt, false);
            if (r.ok) { player_ = r; sync_player_form_buffers(); wake_poller(); }
        }
        ImGui::Spacing();
        ImGui::TextColored(col::Subtext, "%s", T(L, "Atalhos de fôlego", "Stamina shortcuts"));
        for (int sv : {25, 50, 75, 100}) {
            char sb[24]; std::snprintf(sb, sizeof(sb), "%d%%##qs%d", sv, sv);
            if (sv != 25) ImGui::SameLine(0, 6.0f * dpi_scale_);
            if (styled_button(sb, col::BgInput, col::Border, col::Subtext, ImVec2(std::round(58.0f * dpi_scale_), 0))) {
                quick_stam_ = sv;
                auto r = engine_.set_player_quick_condition(sv, std::nullopt, false);
                if (r.ok) { player_ = r; sync_player_form_buffers(); wake_poller(); }
            }
        }
        ImGui::Spacing();
        ImGui::PushTextWrapPos(0.0f);
        wrapped_colored(col::Muted, "%s", T(L, "Fôlego baixo deixa o atleta mais lento e fraco na disputa de bola. Lesão ativa impede o atleta de jogar até curar.",
                                                 "Low stamina makes the player slower and weaker in challenges. An active injury keeps him out until healed."));
        ImGui::PopTextWrapPos();
        end_studio_card();

        ImGui::TableNextColumn();
        ImVec4 cur_ac = arrow_color(player_.arrow);
        begin_studio_card("##bio_arr", top_min_h, false, &cur_ac);
        card_header(T(L, "Seta de Forma (Match Form)", "Match Form Arrow"), bold,
            T(L, "Define a condição pré-jogo. Afeta temporariamente os atributos na partida.",
                 "Sets pre-match condition arrow. Temporarily boosts or lowers live match stats."));
        ImGui::Spacing();
        int picked_arr = draw_arrow_picker("##bio_apick", player_.arrow, L, ImGui::GetContentRegionAvail().x, bold);
        if (picked_arr >= 0 && picked_arr != player_.arrow) {
            auto r = engine_.set_player_quick_condition(std::nullopt, picked_arr, false);
            if (r.ok) { player_ = r; sync_player_form_buffers(); wake_poller(); toasts_.push(T(L, "Seta de forma aplicada!", "Form arrow updated!")); }
        }
        ImGui::Spacing();
        bool blink_v = player_.blink;
        if (toggle_switch("##sw_blink", T(L, "Seta piscando (Blinking Arrow)", "Blinking Arrow (flashing)"), &blink_v)) {
            auto r = engine_.set_player_quick_condition(std::nullopt, std::nullopt, false, 0, blink_v);
            if (r.ok) { player_ = r; sync_player_form_buffers(); wake_poller(); toasts_.push(blink_v ? T(L, "Seta piscando ativada!", "Blinking arrow on!") : T(L, "Seta piscando desativada.", "Blinking arrow off.")); }
        }
        show_tip_if_hovered(T(L, "Seta piscando", "Blinking arrow"),
            T(L, "Flag separada da cor da seta (bit 7 do byte 0x146 do registro). Funciona com qualquer cor. Segundo o manual da Konami, o ícone piscando marca um atleta influenciado por uma função no time, e dar mais minutos a ele pode gerar uma evolução real (breakthrough).",
                 "Flag separate from the arrow colour (bit 7 of record byte 0x146). Works with any colour. According to Konami's manual, the blinking icon marks a player influenced by a Team Role, and giving him more minutes can lead to a real breakthrough."));
        ImGui::PushStyleColor(ImGuiCol_Text, col::Muted);
        ImGui::TextWrapped("%s", T(L, "Faz o ícone da seta piscar nas listas do jogo. A Master Liga usa esse aviso para marcar um atleta que está sendo influenciado por uma função no time. Aqui você só liga ou desliga o aviso: a cor da seta e os atributos não mudam.",
                                      "Makes the arrow icon blink in the game's lists. Master League uses this marker for a player being influenced by a Team Role. Here you only turn the marker on or off: the arrow colour and the attributes do not change."));
        ImGui::PopStyleColor();
        end_studio_card();
    }

    ImGui::Spacing();
    render_value_lock_card(fonts);
    ImGui::Spacing();

    // Dedicated Quick Contract & Finances Card (1-click updates without digging through Bio)
    begin_studio_card("##contract_quick", 0.0f, true, &col::Gold);
    card_header(T(L, "Contrato & Finanças (Edição Rápida)", "Contract & Finances (Quick Edit)"), bold,
        T(L, "Ajuste ano do contrato, valor de mercado, salário, multa rescisória e afinidade com 1 clique ou valor exato.",
             "Adjust contract expiry, market value, salary, release clause, and affection in 1 click or exact input."));

    const char* l_recal_c = T(L, "Recalibrar Economia", "Recalibrate Economy");
    const char* l_apply_c = T(L, "Aplicar Contrato", "Apply Contract");
    align_right_or_wrap(calc_btn_w(l_recal_c) + calc_btn_w(l_apply_c) + 8.0f * dpi_scale_);

    auto apply_contract_update = [&](const BioUpdate& cu, const char* pt_lbl, const char* en_lbl) {
        auto r = engine_.update_player_bio(cu);
        if (r.ok) {
            player_ = r; sync_player_form_buffers(); wake_poller();
            capture_last_edit(r, pt_lbl, en_lbl);
            std::string el = fmt_econ_delta_line(r.econ_delta);
            toasts_.push(el.empty() ? T(L, "Contrato atualizado!", "Contract updated!")
                                    : (std::string(T(L, "Contrato atualizado!\n", "Contract updated!\n")) + el));
        } else {
            toast_code(r.code, true);
        }
    };

    if (styled_button(l_recal_c, ImVec4(0.32f, 0.22f, 0.52f, 0.45f), ImVec4(0.65f, 0.54f, 0.98f, 0.65f), col::Text)) {
        auto r = engine_.recalibrate_player_economy(player_.addr);
        if (r.ok) {
            player_ = r; sync_player_form_buffers(); wake_poller();
            capture_last_edit(r, "Recalibrar Economia", "Recalibrate Economy");
            std::string el = fmt_econ_delta_line(r.econ_delta);
            toasts_.push(el.empty() ? T(L, "Economia já calibrada!", "Economy already calibrated!")
                                    : (std::string(T(L, "Economia recalibrada!\n", "Economy recalibrated!\n")) + el));
        } else toast_code(r.code, true);
    }
    ImGui::SameLine(0, 8.0f * dpi_scale_);
    bool apply_contract_clicked = btn_gold(l_apply_c);

    ImGui::Separator();
    ImGui::Spacing();

    float cw = ImGui::GetContentRegionAvail().x;
    int ccols = (cw >= 780.0f * dpi_scale_) ? 3 : 1;
    if (ScopedCardGrid cg{"##contract_grid", ccols, dpi_scale_}) {
        // Col 1: Contract Expiry Year + quick steppers
        {
            ScopedFormCell c;
            section_label(T(L, "FIM DO CONTRATO (ANO)", "CONTRACT EXPIRY (YEAR)"), fonts);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputInt("##bc", &bio_contract_, 0, 0, ImGuiInputTextFlags_CharsDecimal);
            if (ImGui::IsItemDeactivatedAfterEdit())
                bio_contract_ = std::clamp(bio_contract_, 2019, 2150);
            ImGui::Spacing();
            const int yr_steps[4] = {-1, 1, 3, 5};
            const char* yr_lbls[4] = {"-1a##cy", "+1a##cy", "+3a##cy", "+5a##cy"};
            for (int k = 0; k < 4; ++k) {
                if (k > 0) ImGui::SameLine(0, 4.0f * dpi_scale_);
                bool hi = (yr_steps[k] > 0);
                if (styled_button(yr_lbls[k], hi ? ImVec4(0.961f, 0.620f, 0.043f, 0.16f) : col::BgInput,
                                  hi ? ImVec4(0.961f, 0.620f, 0.043f, 0.45f) : col::Border,
                                  hi ? col::GoldSoft : col::Subtext, ImVec2(0, std::round(24.0f * dpi_scale_)))) {
                    BioUpdate cu;
                    cu.contract_yr = std::clamp(player_.contract_yr + yr_steps[k], 2019, 2150);
                    apply_contract_update(cu, "Ano do Contrato", "Contract Year");
                }
            }
        }
        // Col 2: Market Value + quick percentage steppers
        {
            ScopedFormCell c;
            section_label(T(L, "VALOR DE MERCADO (EX: 30M)", "MARKET VALUE (E.G. 30M)"), fonts);
            ImGui::SetNextItemWidth(-1.0f);
            bool m_enter = ImGui::InputText("##bmkt", bio_market_, sizeof(bio_market_), ImGuiInputTextFlags_EnterReturnsTrue);
            if (m_enter) {
                BioUpdate cu; cu.market_str = bio_market_;
                apply_contract_update(cu, "Valor de Mercado", "Market Value");
            }
            ImGui::Spacing();
            const double m_mults[4] = {0.75, 0.90, 1.10, 1.25};
            const char* m_lbls[4] = {"-25%##mk", "-10%##mk", "+10%##mk", "+25%##mk"};
            for (int k = 0; k < 4; ++k) {
                if (k > 0) ImGui::SameLine(0, 4.0f * dpi_scale_);
                bool up = (m_mults[k] > 1.0);
                if (styled_button(m_lbls[k], up ? ImVec4(0.063f, 0.725f, 0.506f, 0.15f) : col::BgInput,
                                  up ? ImVec4(0.063f, 0.725f, 0.506f, 0.45f) : col::Border,
                                  up ? col::GreenSoft : col::Subtext, ImVec2(0, std::round(24.0f * dpi_scale_)))) {
                    double base_raw = std::max(100.0, static_cast<double>(player_.market_raw));
                    int32_t nv = econ::round_pes_money(base_raw * m_mults[k], false);
                    BioUpdate cu; cu.market_str = fmt_money(nv, settings_.currency_idx, L);
                    apply_contract_update(cu, "Valor de Mercado", "Market Value");
                }
            }
        }
        // Col 3: Annual Salary + quick percentage steppers
        {
            ScopedFormCell c;
            section_label(T(L, "SALÁRIO ANUAL (EX: 2.5M)", "ANNUAL SALARY (E.G. 2.5M)"), fonts);
            ImGui::SetNextItemWidth(-1.0f);
            bool s_enter = ImGui::InputText("##bsal", bio_salary_, sizeof(bio_salary_), ImGuiInputTextFlags_EnterReturnsTrue);
            if (s_enter) {
                BioUpdate cu; cu.salary_str = bio_salary_;
                apply_contract_update(cu, "Salário Anual", "Annual Salary");
            }
            ImGui::Spacing();
            const double s_mults[4] = {0.75, 0.90, 1.10, 1.25};
            const char* s_lbls[4] = {"-25%##sl", "-10%##sl", "+10%##sl", "+25%##sl"};
            for (int k = 0; k < 4; ++k) {
                if (k > 0) ImGui::SameLine(0, 4.0f * dpi_scale_);
                bool up = (s_mults[k] > 1.0);
                if (styled_button(s_lbls[k], up ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.16f) : col::BgInput,
                                  up ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.45f) : col::Border,
                                  up ? col::AccentSoft : col::Subtext, ImVec2(0, std::round(24.0f * dpi_scale_)))) {
                    double base_raw = std::max(10.0, static_cast<double>(player_.salary_raw));
                    int32_t nv = econ::round_pes_money(base_raw * s_mults[k], true);
                    BioUpdate cu; cu.salary_str = fmt_money(nv, settings_.currency_idx, L);
                    apply_contract_update(cu, "Salário Anual", "Annual Salary");
                }
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Release Clause toggle + live calculated fee + Affection quick slider
    const char* t_rf = T(L, "Possui Taxa de Rescisão (Multa)", "Has Release Clause");
    bool rf_changed = toggle_switch("##sw_rf", t_rf, &bio_has_rf_);
    show_tip_if_hovered(
        T(L, "Possui Taxa de Rescisão", "Has Release Clause"),
        T(L, "Ativa ou remove a cláusula de multa rescisória do contrato. Quando ativa, o valor acompanha automaticamente o Valor de Mercado e a Afinidade.",
             "Enables or removes the contract release fee clause. When active, the fee scales automatically with Market Value and Affection."));
    if (rf_changed) {
        BioUpdate cu; cu.has_release_fee = bio_has_rf_;
        apply_contract_update(cu, "Taxa de Rescisão", "Release Clause");
    }

    int32_t preview_rf = econ::calc_konami_release_fee_raw(player_.market_raw, bio_has_rf_, player_.age, bio_affection_, player_.ovr);
    std::string rf_pill = bio_has_rf_
        ? (std::string(T(L, "Multa: ", "Clause: ")) + fmt_money(preview_rf, settings_.currency_idx, L))
        : std::string(T(L, "Sem Multa", "No Clause"));
    flow_next(ImGui::CalcTextSize(rf_pill.c_str()).x + 24.0f * dpi_scale_, 14.0f * dpi_scale_);
    draw_pill(rf_pill.c_str(), bio_has_rf_ ? col::GoldSoft : col::Muted,
              bio_has_rf_ ? ImVec4(0.961f, 0.620f, 0.043f, 0.14f) : ImVec4(col::Subtext.x, col::Subtext.y, col::Subtext.z, 0.12f),
              bio_has_rf_ ? ImVec4(0.961f, 0.620f, 0.043f, 0.45f) : col::Border);

    const char* aff_lbl = T(L, "Afinidade:", "Affection:");
    float aff_slider_w = std::round(130.0f * dpi_scale_);
    flow_next(ImGui::CalcTextSize(aff_lbl).x + aff_slider_w + 165.0f * dpi_scale_, 16.0f * dpi_scale_);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(col::Subtext, "%s", aff_lbl);
    ImGui::SameLine(0, 6.0f * dpi_scale_);
    ImGui::SetNextItemWidth(aff_slider_w);
    ImGui::SliderInt("##baff", &bio_affection_, 0, 100, "");
    const bool aff_committed = ImGui::IsItemDeactivatedAfterEdit();
    show_tip_if_hovered(T(L, "Afinidade Clube (0-100)", "Club Affection (0-100)"),
        T(L, "Afeta diretamente a Taxa de Rescisão nativa calculada pelo PES 2021.",
             "Directly affects PES 2021's native Release Fee formula."));
    ImGui::SameLine(0, 6.0f * dpi_scale_);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(col::AccentSoft, "%3d", bio_affection_);
    if (aff_committed) {
        BioUpdate cu; cu.affection = bio_affection_;
        apply_contract_update(cu, "Afinidade Clube", "Club Affection");
    }
    for (int av : {50, 75, 100}) {
        ImGui::SameLine(0, 4.0f * dpi_scale_);
        char ab[16]; std::snprintf(ab, sizeof(ab), "%d##af", av);
        bool cur = (bio_affection_ == av);
        if (styled_button(ab, cur ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.22f) : col::BgInput,
                          cur ? col::Accent : col::Border, cur ? col::AccentSoft : col::Subtext,
                          ImVec2(0, std::round(24.0f * dpi_scale_)))) {
            bio_affection_ = av;
            BioUpdate cu; cu.affection = av;
            apply_contract_update(cu, "Afinidade Clube", "Club Affection");
        }
    }

    if (apply_contract_clicked) {
        BioUpdate cu;
        cu.contract_yr = bio_contract_;
        cu.salary_str = bio_salary_;
        cu.market_str = bio_market_;
        cu.has_release_fee = bio_has_rf_;
        cu.affection = bio_affection_;
        apply_contract_update(cu, "Contrato & Finanças", "Contract & Finances");
    }
    end_studio_card();

    ImGui::Spacing();
    begin_studio_card("##bio_form");
    card_header(T(L, "Dados Biográficos", "Biography"), bold);

    const char* l_rejuv = T(L, "-3 Anos", "-3 Years");
    const char* l_age18 = T(L, "Idade 18", "Age 18");
    const char* l_save  = T(L, "Salvar Alterações", "Save Changes");
    align_right_or_wrap(calc_btn_w(l_rejuv) + calc_btn_w(l_age18) + calc_btn_w(l_save) + 16.0f * dpi_scale_);
    if (btn_gold(l_rejuv)) {
        BioUpdate u; u.age = std::max(15, player_.age - 3);
        auto r = engine_.update_player_bio(u);
        if (r.ok) {
            player_ = r; sync_player_form_buffers(); wake_poller();
            capture_last_edit(r, "Idade -3 Anos", "Age -3 Years");
            std::string el = fmt_econ_delta_line(r.econ_delta);
            toasts_.push(el.empty() ? T(L, "Idade ajustada!", "Age updated!")
                                    : (std::string(T(L, "Idade ajustada!\n", "Age updated!\n")) + el));
        }
    }
    ImGui::SameLine(0, 8.0f * dpi_scale_);
    if (ImGui::Button(l_age18)) {
        BioUpdate u; u.age = 18;
        auto r = engine_.update_player_bio(u);
        if (r.ok) {
            player_ = r; sync_player_form_buffers(); wake_poller();
            capture_last_edit(r, "Idade 18", "Age 18");
            std::string el = fmt_econ_delta_line(r.econ_delta);
            toasts_.push(el.empty() ? T(L, "Idade ajustada para 18!", "Age set to 18!")
                                    : (std::string(T(L, "Idade ajustada para 18!\n", "Age set to 18!\n")) + el));
        }
    }
    ImGui::SameLine(0, 8.0f * dpi_scale_);
    bool save_clicked = btn_primary(l_save);

    ImGui::Separator();
    ImGui::Spacing();

    float fw = ImGui::GetContentRegionAvail().x;
    int fcols = (fw >= 900.0f * dpi_scale_) ? 4 : ((fw >= 520.0f * dpi_scale_) ? 2 : 1);
    if (ScopedCardGrid g{"##bio_grid", fcols, dpi_scale_}) {
        auto field_str = [&](const char* id, const char* lbl, char* buf, size_t sz, const char* tip = nullptr) {
            ScopedFormCell c;
            section_label(lbl, fonts);
            if (tip) show_tip_if_hovered(lbl, tip);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText(id, buf, sz);
        };
        auto field_int = [&](const char* id, const char* lbl, int& v, int lo, int hi, const char* tip = nullptr) {
            ScopedFormCell c;
            section_label(lbl, fonts);
            if (tip) show_tip_if_hovered(lbl, tip);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputInt(id, &v, 0, 0, ImGuiInputTextFlags_CharsDecimal);
            if (ImGui::IsItemDeactivatedAfterEdit()) v = std::clamp(v, lo, hi);
        };
        auto field_seg = [&](const char* id, const char* lbl, int& v, int hi, const ImVec4& col_active, const char* tip = nullptr) {
            ScopedFormCell c;
            section_label(lbl, fonts);
            if (tip) show_tip_if_hovered(lbl, tip);
            draw_rating_segments_interactive(id, &v, hi, col_active, fonts.small_bold ? fonts.small_bold : bold);
            if (tip) show_tip_if_hovered(lbl, tip);
        };

        field_str("##bn", T(L, "NOME DO JOGADOR", "PLAYER NAME"), bio_name_, sizeof(bio_name_));
        field_str("##bs", T(L, "NOME NA CAMISA", "SHIRT NAME"), bio_shirt_, sizeof(bio_shirt_));

        {
            ScopedFormCell c;
            section_label(T(L, "NACIONALIDADE", "NATIONALITY"), fonts);
            ImGui::SetNextItemWidth(-1.0f);
            const char* nat_preview = "---";
            for (const auto& n : data::NATIONALITIES) if (n.id == bio_nat_) { nat_preview = n.name; break; }
            if (ImGui::BeginCombo("##bio_nat", nat_preview)) {
                for (const auto& n : data::NATIONALITIES) {
                    if (ImGui::Selectable(n.name, n.id == bio_nat_)) bio_nat_ = n.id;
                }
                ImGui::EndCombo();
            }
        }

        field_int("##ba", T(L, "IDADE (15-50)", "AGE (15-50)"), bio_age_, 15, 50);
        field_int("##bh", T(L, "ALTURA (CM)", "HEIGHT (CM)"), bio_height_, 145, 215);
        field_int("##bw", T(L, "PESO (KG)", "WEIGHT (KG)"), bio_weight_, 45, 125);

        {
            ScopedFormCell c;
            section_label(T(L, "PÉ PREFERIDO", "STRONGER FOOT"), fonts);
            ImGui::SetNextItemWidth(-1.0f);
            const char* foot_opts[2] = {T(L, "Direito", "Right"), T(L, "Esquerdo", "Left")};
            if (ImGui::BeginCombo("##bio_foot", foot_opts[bio_foot_ ? 1 : 0])) {
                if (ImGui::Selectable(foot_opts[0], bio_foot_ == 0)) bio_foot_ = 0;
                if (ImGui::Selectable(foot_opts[1], bio_foot_ == 1)) bio_foot_ = 1;
                ImGui::EndCombo();
            }
        }

        field_int("##binj", T(L, "DIAS LESIONADO (0 = OK)", "INJURY DAYS (0 = OK)"), bio_injury_, 0, 255);
        field_seg("##bwfu", T(L, "PIOR PÉ (FREQ.) (1-4)", "WEAK FOOT USAGE (1-4)"), bio_wfu_, 4, col::AccentSoft,
            T(L, "Clique nos blocos para ajustar a frequência com que usa o pé não dominante (1 = raro, 4 = frequente).",
                 "Click blocks to set how often the player uses his weaker foot (1 = rarely, 4 = frequently)."));
        field_seg("##bwfa", T(L, "PIOR PÉ (PREC.) (1-4)", "WEAK FOOT ACCURACY (1-4)"), bio_wfa_, 4, col::Cyan,
            T(L, "Clique nos blocos para ajustar a qualidade de passes e chutes com o pé não dominante (1-4).",
                 "Click blocks to set quality of passes and shots with the weaker foot (1-4)."));
        field_seg("##bfrm", T(L, "FORMA FÍSICA (1-8)", "FORM (1-8)"), bio_form_, 8, col::GreenSoft,
            T(L, "Estabilidade de 1 a 8. Quanto maior, maior a chance de seta verde ou azul nas partidas.",
                 "Consistency from 1 to 8. Higher values increase odds of green or blue match arrows."));
        field_seg("##bir", T(L, "RESISTÊNCIA A LESÃO (1-3)", "INJURY RESISTANCE (1-3)"), bio_injres_, 3, col::GoldSoft,
            T(L, "Resistência física contra lesões em partidas e treinos (1 = frágil, 3 = muito resistente).",
                 "Resistance to injuries in matches and training (1 = fragile, 3 = highly resistant)."));
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const char* t_tl = T(L, "Listado para Transferência", "Transfer Listed");
    const char* t_ll = T(L, "Listado para Empréstimo", "Loan Listed");
    toggle_switch("##sw_tl", t_tl, &bio_tlist_);
    flow_next(48.0f * dpi_scale_ + ImGui::CalcTextSize(t_ll).x, 22.0f);
    toggle_switch("##sw_ll", t_ll, &bio_llist_);

    if (save_clicked) {
        BioUpdate u;
        u.name = bio_name_;
        u.shirt = bio_shirt_;
        u.nationality = bio_nat_;
        u.age = bio_age_;
        u.height = bio_height_;
        u.weight = bio_weight_;
        u.foot = bio_foot_;
        u.contract_yr = bio_contract_;
        u.salary_str = bio_salary_;
        u.market_str = bio_market_;
        u.injury = bio_injury_;
        u.wf_usage = bio_wfu_;
        u.wf_acc = bio_wfa_;
        u.form_cons = bio_form_;
        u.inj_res = bio_injres_;
        u.transfer_listed = bio_tlist_;
        u.loan_listed = bio_llist_;
        u.has_release_fee = bio_has_rf_;
        u.affection = bio_affection_;
        auto r = engine_.update_player_bio(u);
        if (r.ok) {
            player_ = r; sync_player_form_buffers(); wake_poller();
            capture_last_edit(r, "Dados Biográficos", "Bio Update");
            std::string el = fmt_econ_delta_line(r.econ_delta);
            toasts_.push(el.empty() ? T(L, "Dados biográficos salvos!", "Bio saved!")
                                    : (std::string(T(L, "Dados biográficos salvos!\n", "Bio saved!\n")) + el));
        }
        else toast_code(r.code, true);
    }
    end_studio_card();
}

void WorkbenchApp::render_ptab_stats(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];
    double now = ImGui::GetTime();

    // Highlighted DNA Overall Evolution Bar (matches Python .ovr-dna-bar)
    begin_studio_card("##ovr_dna_bar", 0.0f, true, &col::Accent);
    card_header(T(L, "Evolução de OVR Geral (Mantendo DNA)", "Overall Evolution (DNA Preserving)"), bold,
        T(L, "Usa a fórmula exata da Konami com distribuição proporcional de Hamilton para evoluir o OVR sem distorcer o estilo do atleta.",
             "Uses Konami's exact formula with Hamilton proportional scaling to raise or lower OVR without breaking player identity."));
    wrapped_colored(col::Subtext, "%s", T(L, "Sobe ou desce o Overall distribuindo pontos na proporção exata dos atributos do atleta.",
                                                "Raises or lowers Overall by scaling stats in the exact natural proportion of the player."));
    ImGui::Spacing();

    if (icon_btn_plain(Icon::ArrowDown, "-1 OVR##dna")) {
        auto r = engine_.step_player_ovr(-1);
        if (r.ok) { player_ = r; sync_player_form_buffers(); record_solver_action(r, "-1 OVR DNA", "-1 OVR DNA"); wake_poller(); }
        else toast_code(r.code, true);
    }
    ImGui::SameLine(0, 6.0f * dpi_scale_);
    draw_ovr_badge(player_.ovr, std::round(44.0f * dpi_scale_), ImGui::GetFrameHeight(), bold);
    ImGui::SameLine(0, 6.0f * dpi_scale_);
    if (icon_btn_primary(Icon::ArrowUp, "+1 OVR##dna")) {
        auto r = engine_.step_player_ovr(1);
        if (r.ok) { player_ = r; sync_player_form_buffers(); record_solver_action(r, "+1 OVR DNA", "+1 OVR DNA"); wake_poller(); }
        else toast_code(r.code, true);
    }

    const char* tgt_lbl = T(L, "Alvo:", "Target:");
    const char* app_lbl = T(L, "Aplicar##tgt", "Apply##tgt");
    float tgt_w = std::round(64.0f * dpi_scale_);
    flow_next(ImGui::CalcTextSize(tgt_lbl).x + tgt_w + calc_btn_w(app_lbl) + 20.0f * dpi_scale_, 18.0f * dpi_scale_);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(col::Subtext, "%s", tgt_lbl);
    ImGui::SameLine(0, 6.0f * dpi_scale_);
    ImGui::SetNextItemWidth(tgt_w);
    bool ent = ImGui::InputText("##tgt_ovr", target_ovr_buf_, sizeof(target_ovr_buf_),
                                ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine(0, 6);
    if (btn_primary(app_lbl) || ent) {
        int parsed = std::atoi(target_ovr_buf_);
        if (parsed <= 0) parsed = player_.ovr;
        target_ovr_inp_ = std::clamp(parsed, 40, 99);
        std::snprintf(target_ovr_buf_, sizeof(target_ovr_buf_), "%d", target_ovr_inp_);
        auto r = engine_.set_player_ovr(target_ovr_inp_);
        if (r.ok) { player_ = r; sync_player_form_buffers(); record_solver_action(r, "Alvo OVR DNA", "Target OVR DNA"); wake_poller(); }
        else toast_code(r.code, true);
    }

    bool armed99 = (all99_armed_until_ > now);
    const char* lbl99 = armed99 ? T(L, "Confirmar Tudo 99?", "Confirm All 99?") : T(L, "Tudo 99", "All 99");
    align_right_or_wrap(calc_btn_w(lbl99));
    bool click99 = armed99 ? btn_red(lbl99) : ImGui::Button(lbl99);
    show_tip_if_hovered(T(L, "Tudo 99", "All 99"), T(L, "Define todos os atributos ativos para 99 (descaracteriza o perfil original).", "Sets all active attributes to 99 (replaces original profile)."));
    if (click99) {
        if (!armed99) {
            all99_armed_until_ = now + 3.0;
        } else {
            all99_armed_until_ = 0.0;
            std::vector<std::pair<int, int>> batch;
            bool is_gk = (player_.reg_pos == 0);
            for (const auto& s : data::STAT_DEFS) {
                if (is_gk || s.group != data::StatGroup::GK) batch.emplace_back(s.fid, 99);
            }
            auto r = engine_.set_player_stats_batch(batch);
            if (r.ok) {
                player_ = r;
                sync_player_form_buffers();
                wake_poller();
                std::string msg = T(L, "Atributos definidos para 99!", "Attributes set to 99!");
                if (r.econ_delta.applied) {
                    std::string el = fmt_econ_delta_line(r.econ_delta);
                    if (!el.empty()) msg += "\n" + el;
                }
                toasts_.push(msg);
            }
        }
    }
    end_studio_card();

    ImGui::Spacing();
    wrapped_colored(col::Muted, "%s", T(L, "Clique no número para digitar (40-99), arraste com precisão ou segure Shift para ajuste fino.",
                                              "Click any number to type (40-99), drag smoothly, or hold Shift for ultra-fine control."));
    ImGui::Spacing();

    auto preview_stats = player_.stats;
    if (stat_row_st_.dragging && stat_row_st_.drag_fid >= 0) preview_stats[stat_row_st_.drag_fid] = stat_row_st_.drag_cur_val;
    else if (stat_row_st_.hold_fid >= 0) preview_stats[stat_row_st_.hold_fid] = stat_row_st_.hold_cur_val;

    const float avail_w = ImGui::GetContentRegionAvail().x;
    const bool wide = (avail_w >= 880.0f * dpi_scale_);

    auto render_radar_card = [&]() {
        begin_studio_card("##radar_card");
        card_header(T(L, "Perfil Técnico (6 Eixos)", "Technical Profile (6 Axes)"), bold);
        ImGui::Separator();
        draw_radar_6axis(preview_stats, L, ImGui::GetContentRegionAvail().x, bold, fonts.small_bold);
        end_studio_card();
    };

    auto render_stat_groups = [&]() {
        struct GDef { data::StatGroup grp; const char* pt; const char* en; };
        const GDef order_out[5] = {
            {data::StatGroup::Attack, "Ataque", "Attack"},
            {data::StatGroup::Tech,   "Técnica & Passe", "Technique & Passing"},
            {data::StatGroup::Phys,   "Físico", "Physical"},
            {data::StatGroup::Def,    "Defesa", "Defense"},
            {data::StatGroup::GK,     "Goleiro", "Goalkeeper"},
        };
        const GDef order_gk[5] = {
            {data::StatGroup::GK,     "Goleiro", "Goalkeeper"},
            {data::StatGroup::Phys,   "Físico", "Physical"},
            {data::StatGroup::Tech,   "Técnica & Passe", "Technique & Passing"},
            {data::StatGroup::Def,    "Defesa", "Defense"},
            {data::StatGroup::Attack, "Ataque", "Attack"},
        };
        const GDef* ord = (player_.reg_pos == 0) ? order_gk : order_out;
        int gcols = (ImGui::GetContentRegionAvail().x >= 800.0f * dpi_scale_) ? 2 : 1;
        if (ScopedCardGrid g{"##stat_groups_tbl", gcols, dpi_scale_}) {
            for (int gi = 0; gi < 5; ++gi) {
                ImGui::TableNextColumn();
                char cid[32]; std::snprintf(cid, sizeof(cid), "##sg_%d", gi);
                begin_studio_card(cid);
                card_header(T(L, ord[gi].pt, ord[gi].en), bold);
                ImGui::Separator();
                ImGui::Spacing();
                for (const auto& s : data::STAT_DEFS) {
                    if (s.group != ord[gi].grp) continue;
                    int old_stat_v = player_.stats[s.fid];
                    int old_ovr_v = player_.ovr;
                    int prev_v = old_stat_v;
                    int comm_v = prev_v;
                    if (draw_stat_row(s, old_stat_v, L, stat_row_st_, prev_v, comm_v, bold)) {
                        auto r = engine_.set_player_stat(s.fid, comm_v);
                        if (r.ok) {
                            r.action_orig = old_ovr_v;
                            r.action_final = r.ovr;
                            r.action_diffs = {{s.fid, old_stat_v, comm_v}};
                            capture_last_edit(r, s.pt, s.en);
                            player_ = r;
                            wake_poller();
                            if (r.econ_delta.applied) {
                                sync_player_form_buffers();
                                std::string el = fmt_econ_delta_line(r.econ_delta);
                                if (!el.empty()) {
                                    char hdr_b[64];
                                    std::snprintf(hdr_b, sizeof(hdr_b), "OVR %d -> %d\n", r.econ_delta.old_ovr, r.econ_delta.new_ovr);
                                    toasts_.push(std::string(hdr_b) + el);
                                }
                            }
                        } else toast_code(r.code, true);
                    }
                    preview_stats[s.fid] = prev_v;
                }
                end_studio_card();
            }
        }
    };

    if (wide) {
        if (ScopedCardGrid g{"##stats_split", 2, dpi_scale_, ImGuiTableFlags_SizingStretchProp}) {
            ImGui::TableSetupColumn("##rad", ImGuiTableColumnFlags_WidthFixed, std::round(256.0f * dpi_scale_));
            ImGui::TableSetupColumn("##grp", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextColumn();
            render_radar_card();
            ImGui::TableNextColumn();
            render_stat_groups();
        }
    } else {
        render_radar_card();
        ImGui::Spacing();
        render_stat_groups();
    }
}

void WorkbenchApp::render_poslab_log_card(const FontBank& fonts, float max_h) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    begin_studio_card("##poslab_diff_card");
    char hdr[64];
    std::snprintf(hdr, sizeof(hdr), "%s (%zu)", T(L, "Histórico de Atributos", "Attribute Diff Log"), poslab_log_.size());
    card_header(hdr, bold,
        T(L, "Mostra exatamente quais atributos subiram ou foram compensados em cada ajuste do Lab de Posições.",
             "Shows exactly which attributes increased or counterbalanced on every Position Lab step."));

    ImGui::Spacing();
    bool is_steps = (poslab_log_mode_ == 0);
    if (styled_button(T(L, "Passo a passo##lm0", "Step-by-step##lm0"),
                      is_steps ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.25f) : col::BgSidebar,
                      is_steps ? col::Accent : col::Border, is_steps ? col::AccentSoft : col::Subtext)) {
        poslab_log_mode_ = 0;
    }
    ImGui::SameLine(0, 6);
    if (styled_button(T(L, "Acumulado##lm1", "Net Summary##lm1"),
                      !is_steps ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.25f) : col::BgSidebar,
                      !is_steps ? col::Accent : col::Border, !is_steps ? col::AccentSoft : col::Subtext)) {
        poslab_log_mode_ = 1;
    }
    ImGui::SameLine(0, 8);
    if (is_steps) {
        ImGui::BeginDisabled(poslab_log_.empty());
        if (ImGui::Button(T(L, "Limpar##clrl", "Clear##clrl"))) poslab_log_.clear();
        ImGui::EndDisabled();
    } else {
        if (ImGui::Button(T(L, "Zerar base##rstb", "Reset base##rstb"))) {
            poslab_base_stats_ = player_.stats;
            poslab_base_valid_ = true;
            toasts_.push(T(L, "Base de comparação zerada.", "Comparison baseline reset."));
        }
    }
    ImGui::Separator();

    auto render_diff_chips = [&](const std::vector<StatDiff>& diffs) {
        const float max_x = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
        for (size_t i = 0; i < diffs.size(); ++i) {
            const auto& df = diffs[i];
            const auto* sd = find_stat_def(df.fid);
            const char* sn = sd ? T(L, sd->pt, sd->en) : "?";
            const char* desc = sd ? T(L, sd->desc_pt, sd->desc_en) : "";
            int d = df.new_val - df.old_val;
            char chip[96];
            std::snprintf(chip, sizeof(chip), "%s %d->%d (%+d)", sn, df.old_val, df.new_val, d);
            float cw = ImGui::CalcTextSize(chip).x + 20.0f;
            if (i > 0 && ImGui::GetItemRectMax().x + 6.0f + cw < max_x) ImGui::SameLine(0, 6.0f);
            ImVec4 fg = (d > 0) ? col::GreenSoft : col::Orange;
            draw_pill(chip, fg, ImVec4(fg.x, fg.y, fg.z, 0.16f), ImVec4(fg.x, fg.y, fg.z, 0.48f));
            show_tip_if_hovered(sn, desc);
        }
    };

    float inner_h = (max_h > 0.0f) ? max_h : 0.0f;
    ImGuiChildFlags cf = (inner_h > 0.0f) ? 0 : ImGuiChildFlags_AutoResizeY;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::BeginChild("##poslab_log_scroll", ImVec2(-1.0f, inner_h), cf);

    if (poslab_log_mode_ == 0) {
        if (poslab_log_.empty()) {
            ImGui::TextWrapped("%s",
                T(L, "Clique em +1/-1 OVR ou Igualar Principal para acompanhar aqui cada atributo modificado.",
                     "Click +1/-1 OVR or Match Primary to see every modified attribute tracked here."));
        } else {
            for (size_t ei = 0; ei < poslab_log_.size(); ++ei) {
                const auto& e = poslab_log_[ei];
                ImGui::PushID(static_cast<int>(ei));
                ImGui::PushStyleColor(ImGuiCol_ChildBg, col::BgInput);
                ImGui::PushStyleColor(ImGuiCol_Border, col::Border);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(std::round(12.0f * dpi_scale_), std::round(9.0f * dpi_scale_)));
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, std::round(8.0f * dpi_scale_));
                ImGui::BeginChild("##le", ImVec2(-1.0f, 0),
                                  ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);

                ImGui::TextColored(col::Muted, "%s", e.time_str.c_str());
                ImGui::SameLine(0, 6.0f * dpi_scale_);
                if (e.pos_id >= 0) {
                    draw_pos_chip(L, e.pos_id, std::round(36.0f * dpi_scale_), std::round(20.0f * dpi_scale_), bold);
                    ImGui::SameLine(0, 6.0f * dpi_scale_);
                }
                if (e.orig_ovr != e.final_ovr) {
                    bool up = (e.final_ovr > e.orig_ovr);
                    char ob[32];
                    std::snprintf(ob, sizeof(ob), "%d -> %d (%+d)", e.orig_ovr, e.final_ovr, e.final_ovr - e.orig_ovr);
                    ImGui::TextColored(up ? col::GreenSoft : col::RedSoft, "%s", ob);
                } else {
                    ImGui::TextColored(col::AccentSoft, "%s", log_tag(L, e.tag_pt, e.tag_en, e.tag_es));
                }
                if (e.reg_pos >= 0 && e.pos_id != e.reg_pos && e.pri_ovr > 0) {
                    ImGui::SameLine(0, 6);
                    ImGui::TextColored(col::Muted, "(%s %d)", pos_short(L, e.reg_pos), e.pri_ovr);
                }

                if (!e.diffs.empty()) {
                    ImGui::Spacing();
                    render_diff_chips(e.diffs);
                } else if (e.orig_ovr != e.final_ovr) {
                    ImGui::TextColored(col::Subtext, "%s", log_tag(L, e.tag_pt, e.tag_en, e.tag_es));
                }

                ImGui::EndChild();
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(2);
                ImGui::Spacing();
                ImGui::PopID();
            }
        }
    } else {
        std::vector<StatDiff> net;
        int sum_up = 0, sum_dn = 0;
        if (poslab_base_valid_) {
            for (const auto& s : data::STAT_DEFS) {
                int ov = poslab_base_stats_[s.fid];
                int nv = player_.stats[s.fid];
                if (ov != nv) {
                    net.push_back({s.fid, ov, nv});
                    if (nv > ov) sum_up += (nv - ov);
                    else sum_dn += (ov - nv);
                }
            }
            std::sort(net.begin(), net.end(), [](const StatDiff& a, const StatDiff& b) {
                int da = a.new_val - a.old_val;
                int db = b.new_val - b.old_val;
                if ((da > 0) != (db > 0)) return da > 0;
                return std::abs(da) > std::abs(db);
            });
        }
        if (net.empty()) {
            ImGui::TextWrapped("%s",
                T(L, "Nenhuma diferença acumulada nos atributos desde o carregamento deste atleta.",
                     "No net attribute changes since loading this player."));
        } else {
            ImGui::TextColored(col::GreenSoft, "+%d %s", sum_up, T(L, "ganhos", "gained"));
            ImGui::SameLine(0, 12);
            ImGui::TextColored(col::Orange, "-%d %s", sum_dn, T(L, "compensados", "counterbalanced"));
            ImGui::SameLine(0, 12);
            ImGui::TextColored(col::Subtext, "(%zu %s)", net.size(), T(L, "atributos", "stats"));
            ImGui::Spacing();
            render_diff_chips(net);
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
    end_studio_card();
}

void WorkbenchApp::render_ptab_poslab(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];
    const float avail_w = ImGui::GetContentRegionAvail().x;
    ImGuiViewport* vp = ImGui::GetMainViewport();
    const bool auto_horiz = (avail_w < 960.0f * dpi_scale_) || (vp && vp->Size.y > vp->Size.x * 0.78f);
    const bool horiz = (pitch_orient_mode_ == 1) ? true : ((pitch_orient_mode_ == 2) ? false : auto_horiz);

    auto render_pitch_card = [&]() {
        begin_studio_card("##pitch_card");
        card_header(T(L, "Mapa Tático (13 Pos.)", "Tactical Pitch (13)"), bold);
        const char* orient_lbl = (pitch_orient_mode_ == 0)
            ? (horiz ? "Auto (H)##po" : "Auto (V)##po")
            : ((pitch_orient_mode_ == 1) ? "Horiz##po" : "Vert##po");
        if (fonts.small_bold) ImGui::PushFont(fonts.small_bold);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * dpi_scale_, 3.0f * dpi_scale_));
        align_right_or_wrap(calc_btn_w(orient_lbl));
        if (styled_button(orient_lbl, col::BgCard2, col::BorderLit, col::AccentSoft)) {
            pitch_orient_mode_ = (pitch_orient_mode_ + 1) % 3;
        }
        ImGui::PopStyleVar();
        if (fonts.small_bold) ImGui::PopFont();
        show_tip_if_hovered(T(L, "Orientação do Campo", "Pitch Orientation"),
            T(L, "Alternar layout do campo entre Automático, Horizontal e Vertical.", "Cycle pitch orientation between Auto, Horizontal and Vertical."));
        ImGui::Spacing();
        int clicked = draw_tactical_pitch(player_.positions, highlighted_pos_, L, horiz, ImGui::GetContentRegionAvail().x, bold);
        if (clicked >= 0) {
            highlighted_pos_ = clicked;
            scroll_poslab_to_hl_ = true;
        }
        ImGui::Spacing();
        if (fonts.small_bold) ImGui::PushFont(fonts.small_bold);
        ImGui::TextColored(col::AccentSoft, "● %s", T(L, "Principal", "Primary"));
        ImGui::SameLine(0, 10.0f * dpi_scale_);
        ImGui::TextColored(col::GreenSoft, "● %s", T(L, "Grau A", "Grade A"));
        ImGui::SameLine(0, 10.0f * dpi_scale_);
        ImGui::TextColored(col::GoldSoft, "● %s", T(L, "Grau B", "Grade B"));
        ImGui::SameLine(0, 10.0f * dpi_scale_);
        ImGui::TextColored(col::Muted, "● %s", T(L, "Grau C", "Grade C"));
        if (fonts.small_bold) ImGui::PopFont();
        end_studio_card();
    };

    auto render_poslab_table = [&]() {
        begin_studio_card("##poslab_card");
        card_header(T(L, "Lab de Posições (Precisão Cirúrgica)", "Position Lab (Surgical Precision)"), bold);
        wrapped_colored(col::Subtext, "%s", T(L, "OVR já inclui bônus do Grau. +1/-1 move 1 por vez sem alterar a posição principal.",
                                                    "OVR already includes Grade bonus. +1/-1 moves 1 step at a time without changing primary OVR."));

        if (horiz) {
            const bool ll_on = !poslab_log_.empty() && !poslab_log_[0].diffs.empty();
            // Faixa de ultima alteracao com altura fixa (nao empurra a tabela quando surge)
            const float ll_h = std::round(2.0f * (ImGui::GetFrameHeight() + 4.0f * dpi_scale_) + 16.0f * dpi_scale_);
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ll_on ? col::BgInput : ImVec4(col::BgInput.x, col::BgInput.y, col::BgInput.z, 0.45f));
            ImGui::PushStyleColor(ImGuiCol_Border, ll_on ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.45f) : col::Border);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(std::round(12.0f * dpi_scale_), std::round(8.0f * dpi_scale_)));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, std::round(8.0f * dpi_scale_));
            ImGui::BeginChild("##poslab_last_strip", ImVec2(-1.0f, ll_h),
                              ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
            if (!ll_on) {
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(col::Muted, "%s", T(L, "Nenhuma alteração ainda. O último ajuste aparece aqui.",
                                                           "No change yet. The latest adjustment shows up here."));
            } else {
            const auto& last = poslab_log_[0];
            ImGui::TextColored(col::AccentSoft, "%s:", T(L, "Última alteração", "Last change"));
            ImGui::SameLine(0, 6.0f * dpi_scale_);
            if (last.pos_id >= 0) draw_pos_chip(L, last.pos_id, std::round(36.0f * dpi_scale_), std::round(20.0f * dpi_scale_), bold);
            const float mx = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - 10.0f;
            for (size_t i = 0; i < last.diffs.size(); ++i) {
                const auto& df = last.diffs[i];
                const auto* sd = find_stat_def(df.fid);
                const char* sn = sd ? T(L, sd->pt, sd->en) : "?";
                int d = df.new_val - df.old_val;
                char cp[80];
                std::snprintf(cp, sizeof(cp), "%s %+d", sn, d);
                float cw = ImGui::CalcTextSize(cp).x + 20.0f;
                if (i > 0) {
                    if (ImGui::GetItemRectMax().x + 6.0f + cw < mx) ImGui::SameLine(0, 6.0f);
                } else {
                    ImGui::SameLine(0, 6.0f);
                }
                ImVec4 fg = (d > 0) ? col::GreenSoft : col::Orange;
                draw_pill(cp, fg, ImVec4(fg.x, fg.y, fg.z, 0.16f), ImVec4(fg.x, fg.y, fg.z, 0.48f));
                if (sd) show_tip_if_hovered(sn, T(L, sd->desc_pt, sd->desc_en));
            }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);
        }

        ImGui::Spacing();

        std::vector<int> order;
        order.reserve(13);
        order.push_back(player_.reg_pos);
        for (int pid : data::TACTICAL_ORDER) if (pid != player_.reg_pos) order.push_back(pid);

        const char* match_lbl = T(L, "Igualar", "Match");
        const char* setp_lbl  = T(L, "Principal", "Primary");
        float act_col_w = calc_btn_w(match_lbl) + calc_btn_w(setp_lbl) + 14.0f * dpi_scale_;
        float step_col_w = icon_button_width("-1") + icon_button_width("+1") + 12.0f * dpi_scale_;
        float grade_btn_w = std::round(26.0f * dpi_scale_);
        float grade_col_w = grade_btn_w * 3.0f + 12.0f * dpi_scale_;
        float ovr_col_w = std::round(50.0f * dpi_scale_);
        float pad_est = ImGui::GetStyle().CellPadding.x * 10.0f + 12.0f * dpi_scale_;
        float pos_col_w = std::max(std::round(176.0f * dpi_scale_),
                                   ImGui::GetContentRegionAvail().x - (ovr_col_w + grade_col_w + step_col_w + act_col_w + pad_est));

        ImGuiTableFlags tf = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX;
        if (ImGui::BeginTable("##poslab_tbl", 5, tf)) {
            ImGui::TableSetupColumn(T(L, "POSIÇÃO", "POSITION"), ImGuiTableColumnFlags_WidthFixed, pos_col_w);
            ImGui::TableSetupColumn("OVR", ImGuiTableColumnFlags_WidthFixed, ovr_col_w);
            ImGui::TableSetupColumn(T(L, "GRAU", "GRADE"), ImGuiTableColumnFlags_WidthFixed, grade_col_w);
            ImGui::TableSetupColumn(T(L, "AJUSTE ISOLADO", "ISOLATED STEP"), ImGuiTableColumnFlags_WidthFixed, step_col_w);
            ImGui::TableSetupColumn(T(L, "AÇÕES", "ACTIONS"), ImGuiTableColumnFlags_WidthFixed, act_col_w);
            ImGui::TableHeadersRow();

            for (int pid : order) {
                const auto& pos = player_.positions[pid];
                ImGui::TableNextRow();
                if (highlighted_pos_ == pid) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.22f)));
                    if (scroll_poslab_to_hl_) {
                        ImGui::SetScrollHereY(0.4f);
                        scroll_poslab_to_hl_ = false;
                    }
                }
                ImGui::PushID(pid);

                ImGui::TableSetColumnIndex(0);
                draw_pos_chip(L, pid, std::round(40.0f * dpi_scale_), std::round(23.0f * dpi_scale_), bold);
                ImGui::SameLine(0, 7.0f * dpi_scale_);
                ImGui::Text("%s", pos_full(L, pid));
                if (pos.is_reg) {
                    ImGui::SameLine(0, 6.0f * dpi_scale_);
                    draw_pill(T(L, "PRINC.", "PRI."), col::GreenSoft, ImVec4(0.063f, 0.725f, 0.506f, 0.18f), ImVec4(0.063f, 0.725f, 0.506f, 0.5f), true);
                }

                ImGui::TableSetColumnIndex(1);
                draw_ovr_badge(pos.ovr, std::round(38.0f * dpi_scale_), std::round(23.0f * dpi_scale_), bold);

                ImGui::TableSetColumnIndex(2);
                auto grade_btn = [&](int g, const char* gl, const ImVec4& act_bg, const ImVec4& act_fg) {
                    bool act = (pos.apt == g);
                    if (styled_button(gl, act ? act_bg : col::BgInput, act ? act_bg : col::Border, act ? act_fg : col::Subtext, ImVec2(grade_btn_w, std::round(24.0f * dpi_scale_)))) {
                        int old_g = pos.apt;
                        int old_o = pos.ovr;
                        auto r = engine_.set_position_aptitude(pid, g);
                        if (r.ok) {
                            player_ = r;
                            if (r.econ_delta.applied) sync_player_form_buffers();
                            wake_poller();
                            if (old_g != g) {
                                const char* gn[3] = {"C", "B", "A"};
                                record_meta_action(pid, old_o, player_.positions[pid].ovr,
                                    std::string("Grau ") + gn[old_g] + " -> " + gn[g],
                                    std::string("Grade ") + gn[old_g] + " -> " + gn[g]);
                            }
                        } else toast_code(r.code, true);
                    }
                };
                grade_btn(0, "C", col::Sky, ImVec4(0.03f, 0.12f, 0.20f, 1.0f));
                ImGui::SameLine(0, 3.0f * dpi_scale_);
                grade_btn(1, "B", col::Gold, ImVec4(0.15f, 0.08f, 0.0f, 1.0f));
                ImGui::SameLine(0, 3.0f * dpi_scale_);
                grade_btn(2, "A", col::Green, ImVec4(0.02f, 0.18f, 0.08f, 1.0f));

                ImGui::TableSetColumnIndex(3);
                if (icon_btn_plain(Icon::ArrowDown, "-1##pl")) {
                    highlighted_pos_ = pid;
                    auto r = engine_.step_position_ovr(pid, -1);
                    if (r.ok) { player_ = r; if (r.econ_delta.applied) sync_player_form_buffers(); record_solver_action(r, "-1 OVR", "-1 OVR"); wake_poller(); }
                    else toast_code(r.code, true);
                }
                ImGui::SameLine(0, 5);
                if (icon_btn_primary(Icon::ArrowUp, "+1##pl")) {
                    highlighted_pos_ = pid;
                    auto r = engine_.step_position_ovr(pid, 1);
                    if (r.ok) { player_ = r; if (r.econ_delta.applied) sync_player_form_buffers(); record_solver_action(r, "+1 OVR", "+1 OVR"); wake_poller(); }
                    else toast_code(r.code, true);
                }

                ImGui::TableSetColumnIndex(4);
                if (!pos.is_reg) {
                    if (btn_gold(match_lbl)) {
                        highlighted_pos_ = pid;
                        auto r = engine_.match_position_to_primary(pid);
                        if (r.ok) { player_ = r; if (r.econ_delta.applied) sync_player_form_buffers(); record_solver_action(r, "Igualar Principal", "Match Primary"); wake_poller(); }
                        else toast_code(r.code, true);
                    }
                    show_tip_if_hovered(T(L, "Igualar à Posição Principal", "Match Primary Position"),
                        T(L, "Eleva o OVR desta posição até igualar a posição principal sem alterar o OVR principal.",
                             "Raises this position's OVR to match the primary position without changing primary OVR."));
                    ImGui::SameLine(0, 5.0f * dpi_scale_);
                    if (ImGui::Button(setp_lbl)) {
                        int old_reg = player_.reg_pos;
                        int old_o = pos.ovr;
                        highlighted_pos_ = pid;
                        auto r = engine_.set_registered_position(pid);
                        if (r.ok) {
                            player_ = r;
                            if (r.econ_delta.applied) sync_player_form_buffers();
                            wake_poller();
                            record_meta_action(pid, old_o, player_.positions[pid].ovr,
                                std::string("Principal: ") + pos_short(Lang::PT, old_reg) + " -> " + pos_short(Lang::PT, pid),
                                std::string("Primary: ") + pos_short(Lang::EN, old_reg) + " -> " + pos_short(Lang::EN, pid),
                                std::string("Principal: ") + pos_short(Lang::ES, old_reg) + " -> " + pos_short(Lang::ES, pid));
                            std::string msg = T(L, "Posição principal alterada!", "Primary position updated!");
                            if (r.econ_delta.applied) {
                                std::string el = fmt_econ_delta_line(r.econ_delta);
                                if (!el.empty()) msg += "\n" + el;
                            }
                            toasts_.push(msg);
                        } else toast_code(r.code, true);
                    }
                    show_tip_if_hovered(T(L, "Tornar Posição Principal", "Set as Primary Position"),
                        T(L, "Define esta posição como a posição registrada principal do atleta.",
                             "Sets this position as the player's registered primary position."));
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        end_studio_card();
    };

    if (horiz) {
        render_pitch_card();
        ImGui::Spacing();
        render_poslab_table();
        ImGui::Spacing();
        render_poslab_log_card(fonts, std::round(230.0f * dpi_scale_));
    } else {
        if (ScopedCardGrid g{"##poslab_split", 2, dpi_scale_, ImGuiTableFlags_SizingStretchProp}) {
            ImGui::TableSetupColumn("##pcol", ImGuiTableColumnFlags_WidthFixed, std::round(248.0f * dpi_scale_));
            ImGui::TableSetupColumn("##tcol", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextColumn();
            render_pitch_card();
            ImGui::Spacing();
            render_poslab_log_card(fonts, std::round(240.0f * dpi_scale_));
            ImGui::TableNextColumn();
            render_poslab_table();
        }
    }
}

void WorkbenchApp::render_ptab_skills(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    begin_studio_card("##sk_presets");
    card_header(T(L, "Presets de habilidades", "Skill presets"), bold,
        T(L, "Aplica um conjunto de habilidades coerente com a função em campo, sem passar do limite nativo de 10.",
             "Applies a skill set that fits an on-pitch role, without exceeding the native cap of 10."));
    ImGui::Spacing();
    render_preset_controls(fonts, false);
    flow_next(icon_button_width(T(L, "Aplicar preset##skp", "Apply preset##skp")), 8.0f * dpi_scale_);
    if (icon_btn_primary(Icon::Check, T(L, "Aplicar preset##skp", "Apply preset##skp"))) apply_preset_cmd(false);
    {
        const auto& cp = presets::kSkillPresets[static_cast<size_t>(std::clamp(preset_idx_, 0, static_cast<int>(presets::kSkillPresets.size()) - 1))];
        ImGui::Spacing();
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(col::Muted, "%s", T(L, cp.desc_pt, cp.desc_en));
        ImGui::PopTextWrapPos();
    }
    end_studio_card();

    ImGui::Spacing();

    begin_studio_card("##sk_style");
    int cur_st = std::clamp(player_.playing_style, 0, static_cast<int>(data::PLAYING_STYLES.size()) - 1);
    const auto& ps = data::PLAYING_STYLES[cur_st];
    card_header(T(L, "Estilo de jogo", "Playing Style"), bold,
        T(L, "Define a inteligência de movimentação sem bola nas posições compatíveis.", "Controls off-the-ball movement intelligence in compatible positions."));
    ImGui::SameLine(0, 10);
    char pos_tag[96];
    std::snprintf(pos_tag, sizeof(pos_tag), "%s: %s", T(L, "Ativa em", "Active at"), T(L, ps.pos_pt, ps.pos_en));
    draw_pill(pos_tag, col::AccentSoft, ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.16f), ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.42f));
    ImGui::Spacing();
    ImGui::SetNextItemWidth(std::min(380.0f, ImGui::GetContentRegionAvail().x));
    if (ImGui::BeginCombo("##pstyle_sel", T(L, ps.pt, ps.en))) {
        for (const auto& s : data::PLAYING_STYLES) {
            if (ImGui::Selectable(T(L, s.pt, s.en), s.id == cur_st)) {
                auto r = engine_.set_playing_style(s.id);
                if (r.ok) { player_ = r; toasts_.push(T(L, "Estilo de jogo atualizado!", "Playing style updated!")); }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::Spacing();
    ImGui::TextColored(col::Subtext, "%s", "");
    ImGui::SameLine(0, 0);
    ImGui::TextWrapped("%s", T(L, ps.desc_pt, ps.desc_en));
    end_studio_card();

    ImGui::Spacing();

    int sk_count = 0;
    for (bool b : player_.skills) if (b) ++sk_count;
    begin_studio_card("##sk_list");
    card_header(T(L, "Habilidades do jogador", "Player Skills"), bold,
        T(L, "Passe o mouse em qualquer habilidade para ver o efeito exato em campo. O limite nativo é 10.",
             "Hover any skill to see its exact on-pitch effect. Native game cap is 10 skills."));
    ImGui::SameLine(0, 12.0f * dpi_scale_);
    ImGui::TextColored(sk_count > 10 ? col::GoldSoft : col::GreenSoft, "%d / 10%s", sk_count,
        sk_count > 10 ? T(L, " (acima do limite padrão)", " (above default cap)") : "");

    // Right-aligned quick skill search filter
    float sk_flt_w = std::round(190.0f * dpi_scale_);
    align_right_or_wrap(sk_flt_w);
    ImGui::SetNextItemWidth(sk_flt_w);
    ImGui::InputTextWithHint("##sk_flt", T(L, "Filtrar habilidade...", "Filter skill..."), skill_filter_, sizeof(skill_filter_));
    ImGui::Separator();

    std::string sk_qn = norm_text_utf8(skill_filter_);
    struct CatDef { data::SkillCat cat; const char* pt; const char* en; };
    const CatDef cats[5] = {
        {data::SkillCat::Dribble,  "DRIBLE & CONTROLE", "DRIBBLING & CONTROL"},
        {data::SkillCat::Shoot,    "FINALIZAÇÃO & CHUTES", "SHOOTING & FINISHING"},
        {data::SkillCat::Pass,     "PASSE & CRUZAMENTO", "PASSING & CROSSING"},
        {data::SkillCat::Def,      "DEFESA & FÍSICO", "DEFENDING & PHYSICAL"},
        {data::SkillCat::GkMental, "GOLEIRO, BOLA PARADA & MENTAL", "GOALKEEPING, SET PIECES & MENTAL"},
    };
    const float max_x = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
    for (const auto& c : cats) {
        bool any_in_cat = false;
        for (size_t i = 0; i < data::PLAYER_SKILLS.size(); ++i) {
            if (data::PLAYER_SKILLS[i].cat != c.cat) continue;
            const char* nm = T(L, data::PLAYER_SKILLS[i].pt, data::PLAYER_SKILLS[i].en);
            if (sk_qn.empty() || norm_text_utf8(nm).find(sk_qn) != std::string::npos) { any_in_cat = true; break; }
        }
        if (!any_in_cat) continue;

        ImGui::Spacing();
        section_label(T(L, c.pt, c.en), fonts);
        bool first_in_row = true;
        for (size_t i = 0; i < data::PLAYER_SKILLS.size(); ++i) {
            const auto& sk = data::PLAYER_SKILLS[i];
            if (sk.cat != c.cat) continue;
            const char* nm = T(L, sk.pt, sk.en);
            if (!sk_qn.empty() && norm_text_utf8(nm).find(sk_qn) == std::string::npos) continue;
            bool on = player_.skills[i];
            char chip_lbl[96];
            if (on) std::snprintf(chip_lbl, sizeof(chip_lbl), "   %s", nm);
            else std::snprintf(chip_lbl, sizeof(chip_lbl), "%s", nm);
            float chip_w = calc_btn_w(chip_lbl);
            if (!first_in_row && ImGui::GetItemRectMax().x + 7.0f * dpi_scale_ + chip_w < max_x) ImGui::SameLine(0, 7.0f * dpi_scale_);
            first_in_row = false;
            ImGui::PushID(static_cast<int>(i));
            if (styled_button(chip_lbl, on ? ImVec4(0.063f, 0.725f, 0.506f, 0.22f) : col::BgInput,
                              on ? ImVec4(0.063f, 0.725f, 0.506f, 0.65f) : col::Border,
                              on ? col::GreenSoft : col::Subtext)) {
                auto r = engine_.set_player_skill_idx(static_cast<int>(i), false, !on);
                if (r.ok) player_ = r;
            }
            if (on) {
                ImVec2 bmin = ImGui::GetItemRectMin();
                ImVec2 bmax = ImGui::GetItemRectMax();
                draw_led_dot(ImGui::GetWindowDrawList(),
                             ImVec2(bmin.x + 11.0f * dpi_scale_, (bmin.y + bmax.y) * 0.5f),
                             3.2f * dpi_scale_, col::GreenSoft, true);
            }
            show_tip_if_hovered(nm, T(L, sk.desc_pt, sk.desc_en));
            ImGui::PopID();
        }
    }
    end_studio_card();

    ImGui::Spacing();

    int ai_count = 0;
    for (bool b : player_.com_ai) if (b) ++ai_count;
    begin_studio_card("##sk_com");
    card_header(T(L, "Estilos de jogo do COMP", "COM Playing Styles"), bold,
        T(L, "Define tendências quando o jogador está sob controle da IA (COMP ou modo SIM).",
             "Controls decision tendencies when managed by AI (COM or SIM mode)."));
    ImGui::SameLine(0, 12.0f * dpi_scale_);
    ImGui::TextColored(col::Cyan, "%d %s", ai_count, T(L, "ativos", "active"));
    ImGui::Separator();
    ImGui::Spacing();
    const float max_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
    for (size_t i = 0; i < data::COM_AI_STYLES.size(); ++i) {
        const auto& ai = data::COM_AI_STYLES[i];
        const char* nm = T(L, ai.pt, ai.en);
        bool on = player_.com_ai[i];
        char chip_lbl[96];
        if (on) std::snprintf(chip_lbl, sizeof(chip_lbl), "   %s", nm);
        else std::snprintf(chip_lbl, sizeof(chip_lbl), "%s", nm);
        float chip_w = calc_btn_w(chip_lbl);
        if (i > 0 && ImGui::GetItemRectMax().x + 7.0f * dpi_scale_ + chip_w < max_x2) ImGui::SameLine(0, 7.0f * dpi_scale_);
        ImGui::PushID(100 + static_cast<int>(i));
        if (styled_button(chip_lbl, on ? ImVec4(0.024f, 0.714f, 0.831f, 0.20f) : col::BgInput,
                          on ? ImVec4(0.024f, 0.714f, 0.831f, 0.62f) : col::Border,
                          on ? col::Cyan : col::Subtext)) {
            auto r = engine_.set_player_skill_idx(static_cast<int>(i), true, !on);
            if (r.ok) player_ = r;
        }
        if (on) {
            ImVec2 bmin = ImGui::GetItemRectMin();
            ImVec2 bmax = ImGui::GetItemRectMax();
            draw_led_dot(ImGui::GetWindowDrawList(),
                         ImVec2(bmin.x + 11.0f * dpi_scale_, (bmin.y + bmax.y) * 0.5f),
                         3.2f * dpi_scale_, col::Cyan, true);
        }
        show_tip_if_hovered(nm, T(L, ai.desc_pt, ai.desc_en));
        ImGui::PopID();
    }
    end_studio_card();
}

void WorkbenchApp::render_ptab_role(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    begin_studio_card("##role_card");
    card_header(T(L, "Função no time", "Team Role"), bold,
        T(L, "Função do atleta no elenco da Master League. Gera bônus de XP em treino, Espírito de equipe, receitas e efeitos de capitão.",
             "Master League squad role. Grants training EXP boosts, Team Spirit, club revenue, and captain perks."));
    ImGui::Separator();
    ImGui::Spacing();

    int rid = std::clamp(edit_role_id_, 0, static_cast<int>(data::TEAM_ROLES.size()) - 1);
    const auto& cur_role = data::TEAM_ROLES[rid];

    auto fmt_role_opt = [&](const data::TeamRoleInfo& r) -> std::string {
        std::string base = T(L, r.pt, r.en);
        if (r.captain) base += T(L, "  ★ Capitão", "  ★ Captain");
        return base;
    };

    int rcols = (ImGui::GetContentRegionAvail().x < 620.0f * dpi_scale_) ? 1 : 2;
    if (ScopedCardGrid g{"##role_top_grid", rcols, dpi_scale_, ImGuiTableFlags_SizingStretchProp}) {
        ImGui::TableSetupColumn("##rc1", ImGuiTableColumnFlags_WidthStretch, 1.8f);
        if (rcols == 2) ImGui::TableSetupColumn("##rc2", ImGuiTableColumnFlags_WidthStretch, 1.2f);

        {
            ScopedFormCell c;
            section_label(T(L, "FUNÇÃO NO TIME", "TEAM ROLE"), fonts);
            ImGui::SetNextItemWidth(-1.0f);
            std::string cur_lbl = fmt_role_opt(cur_role);
            if (ImGui::BeginCombo("##role_sel", cur_lbl.c_str())) {
                for (const auto& r : data::TEAM_ROLES) {
                    std::string ol = fmt_role_opt(r);
                    if (ImGui::Selectable(ol.c_str(), r.id == rid)) edit_role_id_ = r.id;
                }
                ImGui::EndCombo();
            }
        }

        {
            ScopedFormCell c;
            section_label(T(L, "NÍVEL (0 A 3)", "LEVEL (0 TO 3)"), fonts);
            int cur_l = std::clamp(edit_role_lvl_, 0, 3);
            const char* lvl_tags[4] = {
                "0##rl0", "1##rl1", "2##rl2", T(L, "3 Máx##rl3", "3 Max##rl3")
            };
            float gap = std::round(6.0f * dpi_scale_);
            float bw = std::max(46.0f * dpi_scale_, (ImGui::GetContentRegionAvail().x - gap * 3.0f) / 4.0f);
            float bh = ImGui::GetFrameHeight();
            for (int i = 0; i < 4; ++i) {
                if (i > 0) ImGui::SameLine(0.0f, gap);
                bool act = (i == cur_l);
                if (act && bold) ImGui::PushFont(bold);
                if (styled_button(lvl_tags[i],
                                  act ? col::Accent : col::BgInput,
                                  act ? col::AccentSoft : col::Border,
                                  act ? ImVec4(1, 1, 1, 1) : col::Subtext,
                                  ImVec2(bw, bh))) {
                    edit_role_lvl_ = i;
                }
                if (act && bold) ImGui::PopFont();
            }
        }
    }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, col::BgInput);
    ImGui::PushStyleColor(ImGuiCol_Border, cur_role.captain ? ImVec4(0.961f, 0.620f, 0.043f, 0.55f) : col::Border);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(std::round(16.0f * dpi_scale_), std::round(13.0f * dpi_scale_)));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, std::round(10.0f * dpi_scale_));
    ImGui::BeginChild("##role_info_box", ImVec2(-1.0f, 0),
                      ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
    {
        ImVec2 bp = ImGui::GetWindowPos();
        ImVec2 bsz = ImGui::GetWindowSize();
        ImGui::GetWindowDrawList()->AddRectFilled(
            bp, ImVec2(bp.x + 3.5f * dpi_scale_, bp.y + bsz.y),
            ImGui::GetColorU32(cur_role.captain ? col::Gold : col::Accent),
            std::round(10.0f * dpi_scale_), ImDrawFlags_RoundCornersLeft);
    }

    if (cur_role.id != 0) {
        if (cur_role.captain) {
            draw_pill(T(L, "★ Bônus de Capitão", "★ Captain Bonus"),
                      col::GoldSoft, ImVec4(0.961f, 0.620f, 0.043f, 0.20f), ImVec4(0.961f, 0.620f, 0.043f, 0.55f));
        } else {
            draw_pill(T(L, "Sem bônus de capitão", "No captain bonus"),
                      col::Subtext, ImVec4(col::Subtext.x, col::Subtext.y, col::Subtext.z, 0.12f), col::Border);
        }
        auto tag_pill = [&](uint8_t bit, const char* pt, const char* en) {
            if (cur_role.tag_mask & bit) {
                flow_next(ImGui::CalcTextSize(T(L, pt, en)).x + 22.0f, 6.0f);
                draw_pill(T(L, pt, en), col::AccentSoft, ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.15f), ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.40f));
            }
        };
        tag_pill(data::ROLE_TAG_FIN, "Finanças", "Finances");
        tag_pill(data::ROLE_TAG_COND, "Condição", "Condition");
        tag_pill(data::ROLE_TAG_XP, "Treino / XP", "Training / EXP");
        tag_pill(data::ROLE_TAG_SPIRIT, "Espírito de equipe", "Team Spirit");
        ImGui::Spacing();
    }

    ImGui::TextWrapped("%s", T(L, cur_role.desc_pt, cur_role.desc_en));

    const char* cap_desc = T(L, cur_role.captain_pt, cur_role.captain_en);
    if (cur_role.captain && cap_desc && cap_desc[0]) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.961f, 0.620f, 0.043f, 0.10f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.961f, 0.620f, 0.043f, 0.42f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(std::round(14.0f * dpi_scale_), std::round(10.0f * dpi_scale_)));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, std::round(8.0f * dpi_scale_));
        ImGui::BeginChild("##cap_callout", ImVec2(-1.0f, 0),
                          ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
        if (fonts.small_bold) ImGui::PushFont(fonts.small_bold);
        ImGui::TextColored(col::GoldSoft, "★ %s", T(L, "EFEITO DE CAPITÃO (COM A BRAÇADEIRA)", "CAPTAIN EFFECT (WHEN WEARING THE ARMBAND)"));
        if (fonts.small_bold) ImGui::PopFont();
        ImGui::TextWrapped("%s", cap_desc);
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    ImGui::Spacing();
    if (icon_btn_primary(Icon::Check, T(L, "Aplicar Função no time", "Apply Team Role"))) {
        auto r = engine_.set_team_role(edit_role_id_, edit_role_lvl_);
        if (r.ok) { player_ = r; sync_player_form_buffers(); toasts_.push(T(L, "Função no time salva!", "Team role saved!")); }
        else toast_code(r.code, true);
    }
    end_studio_card();
}

void WorkbenchApp::render_view_search(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    page_title(T(L, "Buscar Jogador (Base Completa 30.000+)", "Search Player (Full 30,000+ Database)"), fonts, bold);
    if (search_res_.ok) {
        ImGui::SameLine(0, 12);
        char sinfo[64];
        std::snprintf(sinfo, sizeof(sinfo), "%zu / %d  |  %.1f ms", search_res_.players.size(), search_res_.total, search_res_.elapsed_ms);
        draw_pill(sinfo, col::GreenSoft, ImVec4(0.063f, 0.725f, 0.506f, 0.14f), ImVec4(0.063f, 0.725f, 0.506f, 0.45f));
    }
    ImGui::Spacing();

    begin_studio_card("##srch_bar");
    int scols = (ImGui::GetContentRegionAvail().x >= 840.0f * dpi_scale_) ? 4 : 2;
    bool enter = false;
    if (ScopedCardGrid g{"##srch_grid", scols, dpi_scale_, ImGuiTableFlags_SizingStretchProp}) {
        ImGui::TableSetupColumn("##sg1", ImGuiTableColumnFlags_WidthStretch, 2.2f);
        ImGui::TableSetupColumn("##sg2", ImGuiTableColumnFlags_WidthStretch, 1.5f);
        if (scols == 4) {
            ImGui::TableSetupColumn("##sg3", ImGuiTableColumnFlags_WidthStretch, 0.8f);
            ImGui::TableSetupColumn("##sg4", ImGuiTableColumnFlags_WidthStretch, 0.8f);
        }

        {
            ScopedFormCell c;
            section_label(T(L, "NOME DO JOGADOR", "PLAYER NAME"), fonts);
            ImGui::SetNextItemWidth(-1.0f);
            enter = ImGui::InputTextWithHint("##sq", T(L, "Ex: Messi, Vinicius, Endrick...", "e.g. Messi, Vinicius, Endrick..."), srch_query_, sizeof(srch_query_), ImGuiInputTextFlags_EnterReturnsTrue);
        }
        {
            ScopedFormCell c;
            section_label(T(L, "POSIÇÃO", "POSITION"), fonts);
            ImGui::SetNextItemWidth(-1.0f);
            std::string pos_prev = (srch_pos_ < 0) ? T(L, "Todas Posições", "All Positions") : (std::string(pos_short(L, srch_pos_)) + " (" + pos_full(L, srch_pos_) + ")");
            if (ImGui::BeginCombo("##spos", pos_prev.c_str())) {
                if (ImGui::Selectable(T(L, "Todas Posições", "All Positions"), srch_pos_ == -1)) srch_pos_ = -1;
                for (int i = 0; i < 13; ++i) {
                    std::string pl = std::string(pos_short(L, i)) + " (" + pos_full(L, i) + ")";
                    if (ImGui::Selectable(pl.c_str(), srch_pos_ == i)) srch_pos_ = i;
                }
                ImGui::EndCombo();
            }
        }
        {
            ScopedFormCell c;
            section_label(T(L, "OVR MÍN.", "MIN OVR"), fonts);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputInt("##so", &srch_min_ovr_, 0, 0, ImGuiInputTextFlags_CharsDecimal);
            if (ImGui::IsItemDeactivatedAfterEdit()) srch_min_ovr_ = std::clamp(srch_min_ovr_, 40, 99);
        }
        {
            ScopedFormCell c;
            section_label(T(L, "IDADE MÁX.", "MAX AGE"), fonts);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputInt("##sa", &srch_max_age_, 0, 0, ImGuiInputTextFlags_CharsDecimal);
            if (ImGui::IsItemDeactivatedAfterEdit()) srch_max_age_ = std::clamp(srch_max_age_, 15, 50);
        }
    }

    ImGui::Spacing();
    if (icon_btn_primary(Icon::Search, T(L, "Pesquisar", "Search")) || enter) run_search();
    flow_next(calc_btn_w(T(L, "Limpar", "Clear")));
    if (ImGui::Button(T(L, "Limpar", "Clear"))) {
        srch_query_[0] = '\0'; srch_pos_ = -1; srch_min_ovr_ = 75; srch_max_age_ = 45; srch_free_ = false;
        search_res_ = {};
    }
    const char* free_lbl = T(L, "Apenas Sem Clube", "Free Agents Only");
    flow_next(48.0f * dpi_scale_ + ImGui::CalcTextSize(free_lbl).x, 18.0f);
    toggle_switch("##sw_free", free_lbl, &srch_free_);

    const char* sc_opts[4] = {
        T(L, data::SCOUT_LISTS[0].pt, data::SCOUT_LISTS[0].en),
        T(L, data::SCOUT_LISTS[1].pt, data::SCOUT_LISTS[1].en),
        T(L, data::SCOUT_LISTS[2].pt, data::SCOUT_LISTS[2].en),
        T(L, data::SCOUT_LISTS[3].pt, data::SCOUT_LISTS[3].en),
    };
    float sc_w = calc_combo_width(sc_opts, 4, 175.0f * dpi_scale_, 260.0f * dpi_scale_);
    const char* send_lbl = T(L, "Enviar para:", "Send to:");
    flow_next(ImGui::CalcTextSize(send_lbl).x + sc_w + 14.0f, 18.0f);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(col::Subtext, "%s", send_lbl);
    ImGui::SameLine(0, 6);
    ImGui::SetNextItemWidth(sc_w);
    const char* sc_lbl = sc_opts[0];
    for (const auto& sl : data::SCOUT_LISTS) if (sl.slot == srch_target_scout_) { sc_lbl = T(L, sl.pt, sl.en); break; }
    if (ImGui::BeginCombo("##stgt", sc_lbl)) {
        for (const auto& sl : data::SCOUT_LISTS) {
            if (ImGui::Selectable(T(L, sl.pt, sl.en), srch_target_scout_ == sl.slot)) srch_target_scout_ = sl.slot;
        }
        ImGui::EndCombo();
    }
    // Envio em lote dos atletas marcados para a lista escolhida
    {
        int nsel = 0;
        for (const auto& p : search_res_.players) if (srch_sel_.count(p.addr)) ++nsel;
        const char* all_lbl = T(L, "Marcar todos##sra", "Tick all##sra");
        flow_next(calc_btn_w(all_lbl), 6.0f * dpi_scale_);
        if (styled_button(all_lbl, col::BgCard, col::Border, col::Subtext)) { for (const auto& p : search_res_.players) srch_sel_.insert(p.addr); }
        const char* none_lbl = T(L, "Nenhum##srn", "None##srn");
        flow_next(calc_btn_w(none_lbl), 6.0f * dpi_scale_);
        if (styled_button(none_lbl, col::BgCard, col::Border, col::Subtext)) srch_sel_.clear();
        char send_sel[96];
        std::snprintf(send_sel, sizeof(send_sel), "%s (%d)###srs", T(L, "Enviar marcados", "Send ticked"), nsel);
        flow_next(icon_button_width(send_sel), 6.0f * dpi_scale_);
        ImGui::BeginDisabled(nsel == 0);
        if (icon_btn_green(Icon::Move, send_sel)) {
            std::vector<uint64_t> v;
            for (const auto& p : search_res_.players) if (srch_sel_.count(p.addr)) v.push_back(p.addr);
            auto r = engine_.add_many_to_scout_list(srch_target_scout_, v);
            scout_report(r, "enviado(s) para", "sent to", srch_target_scout_);
            if (r.ok) srch_sel_.clear();
            reload_scouts();
        }
        ImGui::EndDisabled();
    }
    end_studio_card();

    ImGui::Spacing();
    float tbl_h = std::max(260.0f * dpi_scale_, ImGui::GetContentRegionAvail().y - 6.0f);
    ImGuiTableFlags tf = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX;
    float s_ovr = std::round(54.0f * dpi_scale_ + 30.0f * dpi_scale_);
    float s_pos = std::round(58.0f * dpi_scale_);
    float s_age = std::round(56.0f * dpi_scale_);
    float s_con = std::round(78.0f * dpi_scale_);
    float s_sal = std::round(118.0f * dpi_scale_);
    float s_act = std::round(196.0f * dpi_scale_);
    float s_pad = ImGui::GetStyle().CellPadding.x * 18.0f + ImGui::GetStyle().ScrollbarSize + 24.0f * dpi_scale_;
    float s_rem = std::max(std::round(250.0f * dpi_scale_), ImGui::GetContentRegionAvail().x - (s_ovr + s_pos + s_age + s_con + s_sal + s_act + s_pad));
    float s_name = std::round(s_rem * 0.52f);
    float s_club = std::max(std::round(110.0f * dpi_scale_), s_rem - s_name);
    if (ImGui::BeginTable("##srch_tbl", 8, tf, ImVec2(0, tbl_h))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("OVR", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, s_ovr, 0);
        ImGui::TableSetupColumn("POS", ImGuiTableColumnFlags_WidthFixed, s_pos, 1);
        ImGui::TableSetupColumn(T(L, "ATLETA", "PLAYER"), ImGuiTableColumnFlags_WidthFixed, s_name, 2);
        ImGui::TableSetupColumn(T(L, "IDADE", "AGE"), ImGuiTableColumnFlags_WidthFixed, s_age, 3);
        ImGui::TableSetupColumn(T(L, "CLUBE", "CLUB"), ImGuiTableColumnFlags_WidthFixed, s_club, 4);
        ImGui::TableSetupColumn(T(L, "CONTRATO", "CONTRACT"), ImGuiTableColumnFlags_WidthFixed, s_con, 5);
        ImGui::TableSetupColumn(T(L, "SALÁRIO", "SALARY"), ImGuiTableColumnFlags_WidthFixed, s_sal, 6);
        ImGui::TableSetupColumn(T(L, "AÇÕES", "ACTIONS"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, s_act, 7);
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
            if (specs->SpecsCount > 0) {
                srch_sort_col_ = specs->Specs[0].ColumnUserID;
                srch_sort_asc_ = (specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending);
            }
        }
        std::vector<const SearchRow*> rows;
        rows.reserve(search_res_.players.size());
        for (const auto& p : search_res_.players) rows.push_back(&p);
        std::sort(rows.begin(), rows.end(), [&](const SearchRow* a, const SearchRow* b) {
            int cmp = 0;
            switch (srch_sort_col_) {
                case 0: cmp = a->ovr - b->ovr; break;
                case 1: cmp = a->pos - b->pos; break;
                case 2: cmp = a->name.compare(b->name); break;
                case 3: cmp = a->age - b->age; break;
                case 4: cmp = a->club.compare(b->club); break;
                case 5: cmp = a->exp_y - b->exp_y; break;
                case 6: cmp = (a->salary_raw < b->salary_raw) ? -1 : (a->salary_raw > b->salary_raw ? 1 : 0); break;
            }
            if (cmp == 0) return a->ovr > b->ovr;
            return srch_sort_asc_ ? (cmp < 0) : (cmp > 0);
        });

        const float row_h = std::round(30.0f * dpi_scale_);
        for (const SearchRow* p : rows) {
            ImGui::TableNextRow(0, row_h);
            ImGui::PushID(static_cast<int>(p->addr & 0x7FFFFFFF));
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable("##sr_row", p->addr == last_player_addr_,
                                  ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap,
                                  ImVec2(0.0f, row_h))) {
                open_player_in_studio(p->addr);
            }
            ImGui::SameLine(0, 0);
            {
                bool sel_v = srch_sel_.count(p->addr) != 0;
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f * dpi_scale_, 2.0f * dpi_scale_));
                if (ImGui::Checkbox("##srsel", &sel_v)) { if (sel_v) srch_sel_.insert(p->addr); else srch_sel_.erase(p->addr); }
                ImGui::PopStyleVar();
                ImGui::SameLine(0, 6.0f * dpi_scale_);
            }
            draw_ovr_badge(p->ovr, std::round(38.0f * dpi_scale_), std::round(23.0f * dpi_scale_), bold);
            ImGui::TableSetColumnIndex(1); draw_pos_chip(L, p->pos, std::round(44.0f * dpi_scale_), std::round(23.0f * dpi_scale_), bold);
            ImGui::TableSetColumnIndex(2); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(p->name.c_str());
            {
                const uint8_t mk = scout_mask_of(p->addr);
                if (mk) {
                    ImGui::SameLine(0, 8.0f * dpi_scale_);
                    const char* tags[4] = {"O1", "O2", "O3", "FAV"};
                    for (int b = 0; b < 4; ++b) {
                        if (!(mk & (1u << b))) continue;
                        draw_pill(tags[b], col::GoldSoft, ImVec4(0.96f, 0.62f, 0.04f, 0.14f), ImVec4(0.96f, 0.62f, 0.04f, 0.45f), true);
                        ImGui::SameLine(0, 3.0f * dpi_scale_);
                    }
                }
            }
            ImGui::TableSetColumnIndex(3); ImGui::Text("%d%s", p->age, T(L, "a", "y"));
            ImGui::TableSetColumnIndex(4); ImGui::TextColored(col::Subtext, "%s", fmt_club(L, p->club).c_str());
            ImGui::TableSetColumnIndex(5); if (p->exp_y) ImGui::Text("%d", p->exp_y); else ImGui::TextColored(col::Muted, "-");
            ImGui::TableSetColumnIndex(6); ImGui::Text("%s", fmt_money(p->salary_raw, settings_.currency_idx, L).c_str());
            ImGui::TableSetColumnIndex(7);
            if (icon_btn_primary(Icon::Sliders, T(L, "Editar", "Edit"))) open_player_in_studio(p->addr);
            ImGui::SameLine(0, 5);
            const bool in_target = (scout_mask_of(p->addr) & (1u << (srch_target_scout_ - 1))) != 0;
            ImGui::BeginDisabled(in_target);
            if (btn_green(in_target ? T(L, "Na lista", "On list") : T(L, "+ Lista", "+ List"))) {
                auto r = engine_.add_to_scout_list(srch_target_scout_, p->addr);
                toast_code(r.code, !r.ok);
                reload_scouts();
            }
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

void WorkbenchApp::render_view_scouts(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];
    double now = ImGui::GetTime();
    const int exp_limit = std::max(2019, scouts_.cur_year) + 1;

    page_title(T(L, "Olheiros & Favoritos", "Scouts & Shortlist"), fonts, bold);
    wrapped_colored(col::Subtext, "%s", T(L, "Observe atletas nas 3 listas de olheiros e nos Favoritos. Grava só o ponteiro nativo da Master Liga, sem mexer em contrato ou cláusula.",
                                                "Watch players on the 3 scout lists and the Shortlist. Writes only the native handle into Master League RAM, no contract or clause changes."));
    ImGui::Spacing();

    // ---- Operacao pendente (executada depois da tabela, para nunca invalidar ponteiros durante o desenho) ----
    enum PendingOp { OpNone, OpMove, OpCopy, OpRemove, OpAddCurrent, OpClear, OpUndo, OpExport, OpImport };
    PendingOp op = OpNone;

    // ---- Cartoes das 4 listas ----
    {
        const ImVec4* accents[4] = {&col::Accent, &col::Cyan, &col::Green, &col::Gold};
        int kcols = (ImGui::GetContentRegionAvail().x < 880.0f * dpi_scale_) ? 2 : 4;
        if (ScopedCardGrid g{"##sc_cards", kcols, dpi_scale_}) {
            int ci = 0;
            for (const auto& sl : data::SCOUT_LISTS) {
                const ScoutListState* st = nullptr;
                for (const auto& s2 : scouts_.lists) if (s2.slot == sl.slot) { st = &s2; break; }
                int n = st ? static_cast<int>(st->entries.size()) : 0;
                double sum_ovr = 0, sum_age = 0; long long sum_mkt = 0; int expiring = 0;
                if (st) for (const auto& e : st->entries) { sum_ovr += e.ovr; sum_age += e.age; sum_mkt += e.market_raw; if (e.exp_y > 0 && e.exp_y <= exp_limit) ++expiring; }
                const bool act = (active_scout_slot_ == sl.slot);
                ImGui::TableNextColumn();
                char cid[24]; std::snprintf(cid, sizeof(cid), "##scc%d", sl.slot);
                begin_studio_card(cid, std::round(186.0f * dpi_scale_), false, act ? &col::AccentSoft : accents[ci]);
                section_label(T(L, sl.pt, sl.en), fonts);
                char cnt[24]; std::snprintf(cnt, sizeof(cnt), "%d / %d", n, sl.cap);
                if (fonts.title_bold) ImGui::PushFont(fonts.title_bold);
                ImGui::TextColored(n >= sl.cap ? col::RedSoft : col::Text, "%s", cnt);
                if (fonts.title_bold) ImGui::PopFont();
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, *accents[ci]);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, col::BgInput);
                ImGui::ProgressBar(static_cast<float>(n) / static_cast<float>(sl.cap), ImVec2(-1.0f, std::round(5.0f * dpi_scale_)), "");
                ImGui::PopStyleColor(2);
                if (n > 0) {
                    ImGui::TextColored(col::Subtext, "OVR %.1f  |  %.1f%s", sum_ovr / n, sum_age / n, T(L, "a", "y"));
                    ImGui::TextColored(col::Muted, "%s", fmt_money(static_cast<int32_t>(std::min<long long>(sum_mkt, 2000000000LL)), settings_.currency_idx, L).c_str());
                } else {
                    ImGui::TextColored(col::Muted, "%s", T(L, "Lista vazia", "Empty list"));
                }
                ImGui::Spacing();
                char bl[48]; std::snprintf(bl, sizeof(bl), "%s##scb%d", act ? T(L, "Ativa", "Active") : T(L, "Abrir", "Open"), sl.slot);
                if (act ? btn_primary(bl) : ImGui::Button(bl)) {
                    if (!act) { active_scout_slot_ = sl.slot; scout_sel_.clear(); if (scout_move_target_ == sl.slot) scout_move_target_ = (sl.slot % 4) + 1; }
                }
                (void)expiring;
                end_studio_card();
                ++ci;
            }
        }
    }
    ImGui::Spacing();

    // Resolve a lista ativa depois de qualquer reload acima: o ponteiro vale ate o fim do quadro
    const ScoutListState* cur_list = nullptr;
    for (const auto& s2 : scouts_.lists) if (s2.slot == active_scout_slot_) { cur_list = &s2; break; }

    // ---- Filtros e linhas visiveis ----
    const std::string qn = norm_text_utf8(scout_filter_);
    std::vector<const ScoutEntry*> rows;
    if (cur_list) {
        rows.reserve(cur_list->entries.size());
        for (const auto& e : cur_list->entries) {
            if (scout_pos_filter_ == 1 && e.pos != 0) continue;
            if (scout_pos_filter_ == 2 && !(e.pos >= 1 && e.pos <= 3)) continue;
            if (scout_pos_filter_ == 3 && !(e.pos >= 4 && e.pos <= 8)) continue;
            if (scout_pos_filter_ == 4 && e.pos < 9) continue;
            if (scout_only_free_ && !e.club.empty()) continue;
            if (scout_only_exp_ && !(e.exp_y > 0 && e.exp_y <= exp_limit)) continue;
            if (!qn.empty() && norm_text_utf8(e.name + " " + e.club).find(qn) == std::string::npos) continue;
            rows.push_back(&e);
        }
    }
    int nsel = 0;
    if (cur_list) for (const auto& e : cur_list->entries) if (scout_sel_.count(e.handle)) ++nsel;

    // ---- Barra de ferramentas ----
    begin_studio_card("##sc_tools", 0.0f, false, nsel > 0 ? &col::Accent : &col::BorderLit);
    {
        float flt_w = std::round(210.0f * dpi_scale_);
        ImGui::SetNextItemWidth(flt_w);
        ImGui::InputTextWithHint("##sc_flt", T(L, "Filtrar nome ou clube...", "Filter name or club..."), scout_filter_, sizeof(scout_filter_));
        const char* pf[5] = {T(L, "Todos", "All"), "GO", T(L, "DEF", "DEF"), T(L, "MEI", "MID"), T(L, "ATA", "FWD")};
        for (int i = 0; i < 5; ++i) {
            char lb[32]; std::snprintf(lb, sizeof(lb), "%s##scp%d", pf[i], i);
            ImGui::SameLine(0, 6.0f * dpi_scale_);
            bool a2 = (scout_pos_filter_ == i);
            if (styled_button(lb, a2 ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.24f) : col::BgCard, a2 ? col::Accent : col::Border, a2 ? col::AccentSoft : col::Subtext)) scout_pos_filter_ = i;
        }
        const char* fl = T(L, "Sem clube", "No club");
        flow_next(48.0f * dpi_scale_ + ImGui::CalcTextSize(fl).x, 14.0f);
        toggle_switch("##sc_free", fl, &scout_only_free_);
        char el_buf[48];
        std::snprintf(el_buf, sizeof(el_buf), T(L, "Contrato até %d", "Contract to %d"), exp_limit);
        const char* el = el_buf;
        flow_next(48.0f * dpi_scale_ + ImGui::CalcTextSize(el).x, 14.0f);
        toggle_switch("##sc_exp", el, &scout_only_exp_);
    }
    ImGui::Spacing();
    {
        const char* addl = T(L, "+ Adicionar atleta atual", "+ Add current player");
        if (icon_btn_green(Icon::Plus, addl)) op = OpAddCurrent;
        show_tip_if_hovered(T(L, "Atleta atual", "Current player"), T(L, "Adiciona o atleta destacado no jogo ou aberto no editor à lista ativa.", "Adds the player highlighted in game or open in the editor to the active list."));
        const char* sv = T(L, "Marcar visíveis##scv", "Tick visible##scv");
        flow_next(calc_btn_w(sv), 6.0f * dpi_scale_);
        if (styled_button(sv, col::BgCard, col::Border, col::Subtext)) { for (const ScoutEntry* e : rows) scout_sel_.insert(e->handle); }
        const char* sn = T(L, "Nenhum##scn", "None##scn");
        flow_next(calc_btn_w(sn), 6.0f * dpi_scale_);
        if (styled_button(sn, col::BgCard, col::Border, col::Subtext)) scout_sel_.clear();

        {
            // Acoes de lote sempre presentes (desativadas sem selecao) para a linha nao mudar de altura
            ImGui::BeginDisabled(nsel == 0);
            // Destino: qualquer lista diferente da ativa
            if (scout_move_target_ == active_scout_slot_) scout_move_target_ = (active_scout_slot_ % 4) + 1;
            const char* tgt_lbl = "";
            for (const auto& sl : data::SCOUT_LISTS) if (sl.slot == scout_move_target_) tgt_lbl = T(L, sl.pt, sl.en);
            float cw = std::round(190.0f * dpi_scale_);
            flow_next(cw, 10.0f * dpi_scale_);
            ImGui::SetNextItemWidth(cw);
            if (ImGui::BeginCombo("##sc_tgt", tgt_lbl)) {
                for (const auto& sl : data::SCOUT_LISTS) {
                    if (sl.slot == active_scout_slot_) continue;
                    if (ImGui::Selectable(T(L, sl.pt, sl.en), scout_move_target_ == sl.slot)) scout_move_target_ = sl.slot;
                }
                ImGui::EndCombo();
            }
            char mv[64]; std::snprintf(mv, sizeof(mv), "%s (%d)###scm", T(L, "Mover", "Move"), nsel);
            flow_next(icon_button_width(mv), 6.0f * dpi_scale_);
            if (icon_btn_primary(Icon::Move, mv)) op = OpMove;
            char cp[64]; std::snprintf(cp, sizeof(cp), "%s (%d)###scc", T(L, "Copiar", "Copy"), nsel);
            flow_next(icon_button_width(cp), 6.0f * dpi_scale_);
            if (icon_btn_plain(Icon::Copy, cp)) op = OpCopy;
            char rm[64]; std::snprintf(rm, sizeof(rm), "%s (%d)###scr", T(L, "Remover", "Remove"), nsel);
            flow_next(icon_button_width(rm), 6.0f * dpi_scale_);
            if (icon_btn_red(Icon::Trash, rm)) op = OpRemove;
            ImGui::EndDisabled();
        }
    }
    ImGui::Spacing();
    {
        bool armed_clr = (clear_scout_armed_until_ > now);
        const char* clr_lbl = armed_clr ? T(L, "Confirmar limpeza?", "Confirm clear?") : T(L, "Limpar lista", "Clear list");
        if (armed_clr ? icon_btn_red(Icon::Trash, clr_lbl) : icon_btn_plain(Icon::Trash, clr_lbl)) {
            if (!armed_clr) clear_scout_armed_until_ = now + 3.0; else { clear_scout_armed_until_ = 0.0; op = OpClear; }
        }
        int ud = engine_.scout_undo_depth();
        char ul[96];
        std::snprintf(ul, sizeof(ul), "%s (%d)###scu", T(L, "Desfazer alteração", "Undo change"), ud);
        flow_next(icon_button_width(ul), 6.0f * dpi_scale_);
        ImGui::BeginDisabled(ud == 0);
        if (icon_btn_plain(Icon::Undo, ul)) op = OpUndo;
        ImGui::EndDisabled();
        show_tip_if_hovered(T(L, "Desfazer nas listas", "Undo on lists"), T(L, "Volta as 4 listas ao estado anterior à última alteração (até 12 passos).", "Restores the 4 lists to the state before the last change (up to 12 steps)."));
        const char* exl = T(L, "Exportar listas", "Export lists");
        flow_next(icon_button_width(exl), 6.0f * dpi_scale_);
        if (icon_btn_primary(Icon::Export, exl)) op = OpExport;
        show_tip_if_hovered(T(L, "Exportar listas", "Export lists"), T(L, "Salva as 4 listas em JSON (ID + nome) para reaplicar em outro save.", "Saves the 4 lists to JSON (ID + name) to reapply on another save."));
        const char* iml = T(L, "Importar listas", "Import lists");
        flow_next(icon_button_width(iml), 6.0f * dpi_scale_);
        if (icon_btn_green(Icon::Import, iml)) op = OpImport;
        show_tip_if_hovered(T(L, "Importar listas", "Import lists"), T(L, "Cruza o JSON salvo com o save atual por ID e, se preciso, por nome único. Adiciona nas vagas livres, sem apagar o que já existe.", "Matches the saved JSON against the current save by ID and, if needed, unique name. Fills free slots without erasing existing entries."));
    }
    end_studio_card();
    ImGui::Spacing();

    // ---- Tabela ----
    float tbl_h = std::max(240.0f * dpi_scale_, ImGui::GetContentRegionAvail().y - 6.0f);
    float c_ovr = std::round(54.0f * dpi_scale_ + 30.0f * dpi_scale_);
    float c_pos = std::round(52.0f * dpi_scale_);
    float c_age = std::round(46.0f * dpi_scale_);
    float c_con = std::round(82.0f * dpi_scale_);
    float c_mkt = std::round(100.0f * dpi_scale_);
    float c_sal = std::round(96.0f * dpi_scale_);
    float c_frm = std::round(46.0f * dpi_scale_);
    float c_lst = std::round(112.0f * dpi_scale_);
    float c_act = std::round(186.0f * dpi_scale_);
    float c_club = std::round(140.0f * dpi_scale_);
    // Colunas opcionais somem em janelas estreitas (ordem: outras listas, forma, salario) para nunca cortar as acoes
    bool show_lst = true, show_frm = true, show_sal = true;
    const float avail_w = ImGui::GetContentRegionAvail().x;
    const float name_min = std::round(150.0f * dpi_scale_);
    auto table_need = [&]() {
        int ncols = 8 + (show_lst ? 1 : 0) + (show_frm ? 1 : 0) + (show_sal ? 1 : 0);
        float fixed = c_ovr + c_pos + c_age + c_con + c_mkt + c_act + c_club + (show_lst ? c_lst : 0.0f) + (show_frm ? c_frm : 0.0f) + (show_sal ? c_sal : 0.0f);
        return fixed + name_min + ImGui::GetStyle().CellPadding.x * 2.0f * static_cast<float>(ncols) + ImGui::GetStyle().ScrollbarSize + 14.0f * dpi_scale_;
    };
    if (table_need() > avail_w) show_lst = false;
    if (table_need() > avail_w) show_frm = false;
    if (table_need() > avail_w) show_sal = false;
    float c_pad = ImGui::GetStyle().CellPadding.x * 2.0f * static_cast<float>(8 + (show_lst ? 1 : 0) + (show_frm ? 1 : 0) + (show_sal ? 1 : 0)) + ImGui::GetStyle().ScrollbarSize + 14.0f * dpi_scale_;
    float c_name = std::max(name_min,
                            avail_w - (c_ovr + c_pos + c_age + c_con + c_mkt + c_act + c_club + (show_lst ? c_lst : 0.0f) + (show_frm ? c_frm : 0.0f) + (show_sal ? c_sal : 0.0f) + c_pad));
    ImGuiTableFlags tf = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX |
                         ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX;
    uint64_t remove_one_handle = 0;
    if (ImGui::BeginTable("##scout_tbl", 11, tf, ImVec2(0, tbl_h))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("OVR", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, c_ovr, 0);
        ImGui::TableSetupColumn("POS", ImGuiTableColumnFlags_WidthFixed, c_pos, 1);
        ImGui::TableSetupColumn(T(L, "ATLETA", "PLAYER"), ImGuiTableColumnFlags_WidthFixed, c_name, 2);
        ImGui::TableSetupColumn(T(L, "IDADE", "AGE"), ImGuiTableColumnFlags_WidthFixed, c_age, 3);
        ImGui::TableSetupColumn(T(L, "CLUBE", "CLUB"), ImGuiTableColumnFlags_WidthFixed, c_club, 4);
        ImGui::TableSetupColumn(T(L, "CONTRATO", "CONTRACT"), ImGuiTableColumnFlags_WidthFixed, c_con, 5);
        ImGui::TableSetupColumn(T(L, "MERCADO", "MARKET"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, c_mkt, 6);
        ImGui::TableSetupColumn(T(L, "SALÁRIO", "SALARY"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending | (show_sal ? 0 : ImGuiTableColumnFlags_Disabled), c_sal, 7);
        ImGui::TableSetupColumn(T(L, "FORMA", "FORM"), ImGuiTableColumnFlags_WidthFixed | (show_frm ? 0 : ImGuiTableColumnFlags_Disabled), c_frm, 8);
        ImGui::TableSetupColumn(T(L, "OUTRAS LISTAS", "OTHER LISTS"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort | (show_lst ? 0 : ImGuiTableColumnFlags_Disabled), c_lst, 9);
        ImGui::TableSetupColumn(T(L, "AÇÕES", "ACTIONS"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, c_act, 10);
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
            if (specs->SpecsCount > 0) {
                scout_sort_col_ = specs->Specs[0].ColumnUserID;
                scout_sort_asc_ = (specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending);
            }
        }
        std::sort(rows.begin(), rows.end(), [&](const ScoutEntry* a, const ScoutEntry* b) {
            int cmp = 0;
            switch (scout_sort_col_) {
                case 0: cmp = a->ovr - b->ovr; break;
                case 1: cmp = a->pos - b->pos; break;
                case 2: cmp = a->name.compare(b->name); break;
                case 3: cmp = a->age - b->age; break;
                case 4: cmp = a->club.compare(b->club); break;
                case 5: cmp = a->exp_y - b->exp_y; break;
                case 6: cmp = (a->market_raw < b->market_raw) ? -1 : (a->market_raw > b->market_raw ? 1 : 0); break;
                case 7: cmp = (a->salary_raw < b->salary_raw) ? -1 : (a->salary_raw > b->salary_raw ? 1 : 0); break;
                case 8: cmp = a->arrow - b->arrow; break;
            }
            if (cmp == 0) return a->ovr > b->ovr;
            return scout_sort_asc_ ? (cmp < 0) : (cmp > 0);
        });

        const float row_h = std::round(30.0f * dpi_scale_);
        const uint8_t own_bit = static_cast<uint8_t>(1u << (active_scout_slot_ - 1));
        for (const ScoutEntry* e : rows) {
            ImGui::TableNextRow(0, row_h);
            ImGui::PushID(static_cast<int>(e->handle & 0x7FFFFFFF));
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable("##sc_row", e->addr == last_player_addr_,
                                  ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, ImVec2(0.0f, row_h))) {
                open_player_in_studio(e->addr);
            }
            ImGui::SameLine(0, 0);
            {
                bool sel_v = scout_sel_.count(e->handle) != 0;
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f * dpi_scale_, 2.0f * dpi_scale_));
                if (ImGui::Checkbox("##scsel", &sel_v)) { if (sel_v) scout_sel_.insert(e->handle); else scout_sel_.erase(e->handle); }
                ImGui::PopStyleVar();
                ImGui::SameLine(0, 6.0f * dpi_scale_);
            }
            draw_ovr_badge(e->ovr, std::round(38.0f * dpi_scale_), std::round(23.0f * dpi_scale_), bold);
            ImGui::TableSetColumnIndex(1); draw_pos_chip(L, e->pos, std::round(44.0f * dpi_scale_), std::round(23.0f * dpi_scale_), bold);
            ImGui::TableSetColumnIndex(2); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(e->name.c_str());
            ImGui::TableSetColumnIndex(3); ImGui::Text("%d%s", e->age, T(L, "a", "y"));
            ImGui::TableSetColumnIndex(4);
            if (e->club.empty()) draw_pill(T(L, "Sem clube", "Free agent"), col::GreenSoft, ImVec4(0.063f, 0.725f, 0.506f, 0.16f), ImVec4(0.063f, 0.725f, 0.506f, 0.45f), true);
            else ImGui::TextColored(col::Subtext, "%s", fmt_club(L, e->club).c_str());
            ImGui::TableSetColumnIndex(5);
            if (e->exp_y > 0) ImGui::TextColored(e->exp_y <= exp_limit ? col::GoldSoft : col::Subtext, "%d", e->exp_y);
            else ImGui::TextColored(col::Muted, "-");
            ImGui::TableSetColumnIndex(6); ImGui::Text("%s", fmt_money(e->market_raw, settings_.currency_idx, L).c_str());
            ImGui::TableSetColumnIndex(7); ImGui::Text("%s", fmt_money(e->salary_raw, settings_.currency_idx, L).c_str());
            ImGui::TableSetColumnIndex(8); draw_arrow_badge(e->arrow, std::round(22.0f * dpi_scale_), e->blink, settings_.lang);
            ImGui::TableSetColumnIndex(9);
            {
                const uint8_t mk = static_cast<uint8_t>(scout_mask_of(e->addr) & ~own_bit);
                const char* tags[4] = {"O1", "O2", "O3", "FAV"};
                bool any = false;
                for (int b = 0; b < 4; ++b) {
                    if (!(mk & (1u << b))) continue;
                    if (any) ImGui::SameLine(0, 3.0f * dpi_scale_);
                    draw_pill(tags[b], col::AccentSoft, ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.16f), ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.45f), true);
                    any = true;
                }
                if (!any) ImGui::TextColored(col::Muted, "-");
            }
            ImGui::TableSetColumnIndex(10);
            if (icon_btn_primary(Icon::Sliders, T(L, "Editar", "Edit"))) open_player_in_studio(e->addr);
            ImGui::SameLine(0, 5);
            if (icon_btn_red(Icon::Trash, T(L, "Remover", "Remove"))) remove_one_handle = e->handle;
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    // ---- Executa a operacao pendente (depois de todo o desenho) ----
    std::vector<uint64_t> chosen;
    if (cur_list && (op == OpMove || op == OpCopy || op == OpRemove)) {
        for (const auto& e : cur_list->entries) if (scout_sel_.count(e.handle)) chosen.push_back(e.handle);
    }
    bool dirty = false;
    if (remove_one_handle) {
        auto r = engine_.remove_from_scout_list(active_scout_slot_, remove_one_handle);
        if (r.ok) { scout_sel_.erase(remove_one_handle); toasts_.push(T(L, "Removido da lista!", "Removed from list!")); } else toast_code(r.code, true);
        dirty = true;
    }
    switch (op) {
        case OpAddCurrent: {
            auto r = engine_.add_to_scout_list(active_scout_slot_);
            toast_code(r.code, !r.ok);
            dirty = true;
        } break;
        case OpMove:
        case OpCopy: {
            auto r = engine_.move_scout_entries(active_scout_slot_, scout_move_target_, chosen, op == OpCopy);
            scout_report(r, op == OpCopy ? "copiado(s) para" : "movido(s) para", op == OpCopy ? "copied to" : "moved to", scout_move_target_);
            if (r.ok) scout_sel_.clear();
            dirty = true;
        } break;
        case OpRemove: {
            auto r = engine_.remove_many_from_scout_list(active_scout_slot_, chosen);
            if (r.ok) { scout_sel_.clear(); char m[96]; std::snprintf(m, sizeof(m), T(L, "%d atleta(s) removido(s).", "%d player(s) removed."), r.count); toasts_.push(m); }
            else toast_code(r.code, true);
            dirty = true;
        } break;
        case OpClear: {
            auto r = engine_.clear_scout_list(active_scout_slot_);
            if (r.ok) { scout_sel_.clear(); toasts_.push(T(L, "Lista limpa. Dá para desfazer.", "List cleared. Undo is available.")); } else toast_code(r.code, true);
            dirty = true;
        } break;
        case OpUndo: {
            auto r = engine_.undo_scout_change();
            if (r.ok) { scout_sel_.clear(); toasts_.push(T(L, "Alteração nas listas desfeita!", "List change undone!")); } else toast_code(r.code, true);
            dirty = true;
        } break;
        case OpExport: {
            auto r = engine_.export_scout_lists_json();
            if (!r.ok) { toast_code(r.code, true); break; }
            std::string dst = pick_save_json_dialog(static_cast<HWND>(hwnd_), get_packs_directory(), "Olheiros_e_Favoritos.json");
            if (!dst.empty()) {
                if (save_text_file_utf8(dst, r.text)) {
                    char m[128]; std::snprintf(m, sizeof(m), T(L, "%d atleta(s) exportado(s) das listas.", "%d player(s) exported from the lists."), r.count);
                    toasts_.push(m);
                } else toasts_.push(T(L, "Erro ao salvar arquivo.", "Error saving file."), true);
            }
        } break;
        case OpImport: {
            std::string pth = pick_open_json_dialog(static_cast<HWND>(hwnd_), get_packs_directory());
            if (pth.empty()) break;
            std::string raw;
            if (!load_text_file_utf8(pth, raw)) { toasts_.push(T(L, "Não foi possível ler o arquivo selecionado.", "Could not read selected file."), true); break; }
            auto r = engine_.import_scout_lists_json(raw, false);
            if (!r.ok) { toast_code(r.code, true); break; }
            char m[320];
            std::snprintf(m, sizeof(m), T(L, "Importados %d atleta(s). Sem correspondência: %d.%s%s", "Imported %d player(s). Unmatched: %d.%s%s"),
                          r.count, r.value, r.text.empty() ? "" : " ", utf8_prefix(r.text, 120).c_str());
            toasts_.push(m);
            dirty = true;
        } break;
        default: break;
    }
    if (dirty) reload_scouts();
}

void WorkbenchApp::render_view_match(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    auto period_label = [&](int p) -> const char* {
        switch (p) {
            case 0: return T(L, "Pré-Jogo", "Pre-Match");
            case 1: return T(L, "1º Tempo", "1st Half");
            case 2: return T(L, "Intervalo", "Half-Time");
            case 3: return T(L, "2º Tempo", "2nd Half");
            case 4: return T(L, "Fim 90'", "End 90'");
            case 5: return T(L, "1ª Prorr.", "1st ET");
            case 6: return T(L, "Int. Prorr.", "ET Break");
            case 7: return T(L, "2ª Prorr.", "2nd ET");
            case 8: return T(L, "Pênaltis", "Penalties");
            default: return T(L, "Em Jogo", "In Play");
        }
    };

    page_title(T(L, "Partida Ao Vivo", "Live Match"), fonts, bold);
    ImGui::SameLine(0, 12.0f * dpi_scale_);
    if (st_.in_match) draw_pill(T(L, "● EM PARTIDA", "● IN MATCH"), col::GreenSoft, ImVec4(0.063f, 0.725f, 0.506f, 0.18f), ImVec4(0.063f, 0.725f, 0.506f, 0.55f));
    else draw_pill(T(L, "● Fora de Partida", "● Not in Match"), col::Subtext, ImVec4(col::Subtext.x, col::Subtext.y, col::Subtext.z, 0.12f), col::Border);
    wrapped_colored(col::Subtext, "%s", T(L, "Controle exato do cronômetro, fôlego seletivo por time e preparo rápido do elenco em tempo real.",
                                                "Exact match clock control, selective per-team stamina, and live squad prep in real time."));
    ImGui::Spacing();

    // 1. Live Match Clock Control Center (Exact Time + Shift + Half Actions + Freeze)
    begin_studio_card("##match_clock_card", 0.0f, true, st_.freeze_clock ? &col::Accent : &col::Gold);
    card_header(T(L, "Cronômetro da Partida (Tempo Exato)", "Match Clock (Exact Time)"), bold,
        T(L, "Defina exatamente o minuto e segundo do tempo atual, avance/volte minutos, reinicie ou encerre o tempo, ou congele o relógio.",
             "Set the exact minute and second of the current half, shift minutes forward/back, reset/end the half, or freeze the clock."));

    ImGui::SameLine(0, 10.0f * dpi_scale_);
    if (!st_.in_match) {
        draw_pill(T(L, "FORA DE PARTIDA", "NOT IN MATCH"), col::Muted, ImVec4(col::Subtext.x, col::Subtext.y, col::Subtext.z, 0.12f), col::Border, true);
    } else {
        if (st_.match_clock_valid) {
            char clk_txt[48];
            std::snprintf(clk_txt, sizeof(clk_txt), "%02d:%02d  •  %s", st_.match_min, st_.match_sec, period_label(st_.match_period));
            draw_pill(clk_txt, col::GoldSoft, ImVec4(0.961f, 0.620f, 0.043f, 0.16f), ImVec4(0.961f, 0.620f, 0.043f, 0.50f), true);
            ImGui::SameLine(0, 8.0f * dpi_scale_);
        }
        if (st_.freeze_clock) {
            draw_pill(T(L, "CONGELADO", "FROZEN"), col::AccentSoft, ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.22f), ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.58f), true);
        } else {
            draw_pill(T(L, "CORRENDO", "RUNNING"), col::GreenSoft, ImVec4(0.063f, 0.725f, 0.506f, 0.14f), ImVec4(0.063f, 0.725f, 0.506f, 0.42f), true);
        }
    }

    ImGui::Separator();
    ImGui::Spacing();
    if (!st_.in_match) {
        wrapped_colored(col::Muted, "%s", T(L, "Inicie uma partida no PES 2021 para ler e ajustar o cronômetro ao vivo (o hook de relógio arma automaticamente ao entrar em campo).",
                                                  "Start a match in PES 2021 to read and adjust the live clock (the clock tap auto-arms as soon as you enter the pitch)."));
        ImGui::Spacing();
    }
    ImGui::BeginDisabled(!st_.in_match);

    auto sync_clock_inputs = [&](int mm, int ss) {
        std::snprintf(match_min_buf_, sizeof(match_min_buf_), "%02d", std::clamp(mm, 0, 130));
        std::snprintf(match_sec_buf_, sizeof(match_sec_buf_), "%02d", std::clamp(ss, 0, 59));
    };

    auto apply_exact_clock = [&](int m_val, int s_val) {
        auto r = engine_.set_match_clock(m_val, s_val);
        if (r.ok) {
            sync_clock_inputs(r.count, r.value);
            wake_poller();
            char msg[96];
            std::snprintf(msg, sizeof(msg), T(L, "Relógio ajustado para %02d:%02d!", "Match clock set to %02d:%02d!"), r.count, r.value);
            toasts_.push(msg);
        } else toast_code(r.code, true);
    };

    // Exact Minute : Second inputs + Apply + Sync + Freeze toggle
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(col::Subtext, "%s", T(L, "Tempo exato:", "Exact time:"));
    ImGui::SameLine(0, 8.0f * dpi_scale_);
    ImGui::SetNextItemWidth(std::round(52.0f * dpi_scale_));
    bool m_ent = ImGui::InputText("##m_min", match_min_buf_, sizeof(match_min_buf_),
                                  ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine(0, 4.0f * dpi_scale_);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(col::Text, ":");
    ImGui::SameLine(0, 4.0f * dpi_scale_);
    ImGui::SetNextItemWidth(std::round(52.0f * dpi_scale_));
    bool s_ent = ImGui::InputText("##m_sec", match_sec_buf_, sizeof(match_sec_buf_),
                                  ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine(0, 8.0f * dpi_scale_);
    if (icon_btn_primary(Icon::Stopwatch, T(L, "Definir Tempo##mc", "Set Time##mc")) || m_ent || s_ent) {
        int mv = std::clamp(std::atoi(match_min_buf_), 0, 130);
        int sv = std::clamp(std::atoi(match_sec_buf_), 0, 59);
        apply_exact_clock(mv, sv);
    }
    ImGui::SameLine(0, 6.0f * dpi_scale_);
    if (icon_btn_plain(Icon::Refresh, T(L, "Ler Atual##mcs", "Read Live##mcs")) && st_.match_clock_valid) {
        sync_clock_inputs(st_.match_min, st_.match_sec);
    }

    const char* fz_lbl = T(L, "Congelar Cronômetro", "Freeze Clock");
    flow_next(56.0f * dpi_scale_ + ImGui::CalcTextSize(fz_lbl).x, 20.0f * dpi_scale_);
    bool clk_v = st_.freeze_clock;
    if (toggle_switch("##sw_clk", fz_lbl, &clk_v)) {
        auto r = engine_.set_freeze_clock(clk_v);
        if (r.ok) {
            st_.freeze_clock = r.flag;
            wake_poller();
            toasts_.push(r.flag ? T(L, "Relógio congelado!", "Clock frozen!") : T(L, "Relógio liberado!", "Clock resumed!"));
        } else toast_code(r.code, true);
    }

    ImGui::Spacing();

    // Quick minute shifts & half presets
    const int shifts[4] = {-10, -5, 5, 10};
    const char* sh_lbls[4] = {"-10 min##ms", "-5 min##ms", "+5 min##ms", "+10 min##ms"};
    for (int k = 0; k < 4; ++k) {
        if (k > 0) ImGui::SameLine(0, 6.0f * dpi_scale_);
        if (ImGui::Button(sh_lbls[k])) {
            auto r = engine_.shift_match_clock(shifts[k]);
            if (r.ok) {
                sync_clock_inputs(r.count, r.value);
                wake_poller();
                char msg[96];
                std::snprintf(msg, sizeof(msg), T(L, "Relógio ajustado para %02d:%02d!", "Match clock set to %02d:%02d!"), r.count, r.value);
                toasts_.push(msg);
            } else toast_code(r.code, true);
        }
    }

    const char* l_rst = T(L, "Reiniciar Tempo (00')##mr", "Reset Half (00')##mr");
    const char* l_44  = "44:00##m44";
    const char* l_89  = "89:00##m89";
    const char* l_end = T(L, "Encerrar Tempo##me", "End Half##me");
    flow_next(calc_btn_w(l_rst), 12.0f * dpi_scale_);
    if (btn_gold(l_rst)) {
        auto r = engine_.reset_match_half();
        if (r.ok) {
            sync_clock_inputs(r.count, r.value);
            wake_poller();
            toasts_.push(T(L, "Tempo atual reiniciado!", "Current half reset!"));
        } else toast_code(r.code, true);
    }
    flow_next(calc_btn_w(l_44), 6.0f * dpi_scale_);
    if (ImGui::Button(l_44)) apply_exact_clock(44, 0);
    flow_next(calc_btn_w(l_89), 6.0f * dpi_scale_);
    if (ImGui::Button(l_89)) apply_exact_clock(89, 0);
    flow_next(calc_btn_w(l_end), 6.0f * dpi_scale_);
    if (btn_red(l_end)) {
        auto r = engine_.end_match_half();
        if (r.ok) {
            sync_clock_inputs(r.count, r.value);
            wake_poller();
            toasts_.push(T(L, "Fim do tempo atual acionado!", "End of current half triggered!"));
        } else toast_code(r.code, true);
    }
    ImGui::EndDisabled();
    end_studio_card();

    ImGui::Spacing();

    // 2. Selective Match Stamina
    begin_studio_card("##match_stam_card");
    card_header(T(L, "Fôlego Seletivo em Partida", "Selective Match Stamina"), bold);
    ImGui::Separator();
    ImGui::Spacing();
    struct ModeDef { int id; const char* t_pt; const char* t_en; const char* d_pt; const char* d_en; };
    const ModeDef modes[6] = {
        {0, "Normal", "Normal", "Desgaste padrão", "Default drain"},
        {1, "Casa 100%", "Home 100%", "Time da casa infinito", "Infinite home stamina"},
        {2, "Fora 100%", "Away 100%", "Visitante infinito", "Infinite away stamina"},
        {3, "Ambos 100%", "Both 100%", "Todos em campo 100%", "All players 100%"},
        {4, "Casa 100% / Fora 5%", "Home 100% / Away 5%", "Pressão total em casa", "Total home press"},
        {5, "Fora 100% / Casa 5%", "Away 100% / Home 5%", "Pressão total visitante", "Total away press"},
    };
    int cols = (ImGui::GetContentRegionAvail().x < 680.0f * dpi_scale_) ? 2 : 3;
    float card_h = std::round(64.0f * dpi_scale_);
    if (ScopedCardGrid g{"##sm_grid", cols, dpi_scale_}) {
        for (const auto& m : modes) {
            ImGui::TableNextColumn();
            bool act = (st_.stamina_mode == m.id);
            char cid[24];
            std::snprintf(cid, sizeof(cid), "##sm_%d", m.id);
            if (draw_mode_card(cid, T(L, m.t_pt, m.t_en), T(L, m.d_pt, m.d_en), act, card_h, bold, fonts.small_bold)) {
                auto r = engine_.set_stamina_mode(m.id);
                if (r.ok) { st_.stamina_mode = m.id; toasts_.push(T(L, "Modo de fôlego atualizado!", "Match stamina updated!")); }
                else toast_code(r.code, true);
            }
        }
    }
    end_studio_card();

    // 3. Quick Squad Prep strip (connects Live Match directly to Career Squad state)
    if (st_.career > 0x10000ULL) {
        ImGui::Spacing();
        begin_studio_card("##match_squad_prep");
        card_header(T(L, "Preparo Rápido do Elenco (Carreira)", "Quick Squad Prep (Career)"), bold);
        wrapped_colored(col::Subtext, "%s", T(L, "Aplica imediatamente no seu elenco ativo da carreira sem sair da aba de partida.",
                                                    "Applies immediately to your active career squad without leaving the Live Match tab."));
        ImGui::Spacing();
        if (icon_btn_green(Icon::Battery, T(L, "Fôlego 100% Elenco##mqs", "Squad 100% Stamina##mqs"))) {
            run_squad_cmd("stamina");
        }
        flow_next(icon_button_width(T(L, "Seta Azul Elenco##mqa", "Squad Blue Arrows##mqa")), 8.0f * dpi_scale_);
        if (icon_btn_primary(Icon::ArrowUp, T(L, "Seta Azul Elenco##mqa", "Squad Blue Arrows##mqa"))) {
            run_squad_cmd("arrow");
        }
        flow_next(icon_button_width(T(L, "Curar Lesões##mqh", "Heal Injuries##mqh")), 8.0f * dpi_scale_);
        if (icon_btn_gold(Icon::Cross, T(L, "Curar Lesões##mqh", "Heal Injuries##mqh"))) {
            run_squad_cmd("heal");
        }
        end_studio_card();
    }
}

void WorkbenchApp::render_view_training(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    page_title(T(L, "Treino & Rumo ao Estrelato (BAL)", "Training & Become a Legend (BAL)"), fonts, bold);
    wrapped_colored(col::Subtext, "%s", T(L, "Acelere a evolução de Sub-XP na Master League/BAL e desbloqueie o limite de 21 Pontos de Foco no BAL.",
                                                "Accelerate Sub-XP growth in Master League/BAL and unlock the 21 Focus Points cap in BAL."));
    ImGui::Spacing();

    begin_studio_card("##train_xp_card");
    card_header(T(L, "Velocidade de Treino (Multiplicador Sub-XP)", "Training Speed (Sub-XP Multiplier)"), bold);
    ImGui::Separator();
    ImGui::Spacing();
    struct TMode { int id; const char* t_pt; const char* t_en; const char* d_pt; const char* d_en; };
    const TMode tmodes[6] = {
        {0, "Normal (1x)", "Normal (1x)", "Evolução padrão", "Default growth"},
        {1, "Rápido (2x)", "Fast (2x)", "Dobro de XP", "Double XP"},
        {2, "Acelerado (5x)", "Accelerated (5x)", "5x mais rápido", "5x faster"},
        {3, "Turbo (10x)", "Turbo (10x)", "10x mais rápido", "10x faster"},
        {4, "Ultra (50x)", "Ultra (50x)", "Evolução extrema", "Extreme growth"},
        {5, "Instantâneo", "Instant", "XP máximo imediato", "Max XP immediately"},
    };
    int cols = (ImGui::GetContentRegionAvail().x < 680.0f * dpi_scale_) ? 2 : 3;
    float card_h = std::round(64.0f * dpi_scale_);
    if (ScopedCardGrid g{"##tm_grid", cols, dpi_scale_}) {
        for (const auto& m : tmodes) {
            ImGui::TableNextColumn();
            bool act = (st_.training_mode == m.id);
            char cid[24];
            std::snprintf(cid, sizeof(cid), "##tm_%d", m.id);
            if (draw_mode_card(cid, T(L, m.t_pt, m.t_en), T(L, m.d_pt, m.d_en), act, card_h, bold, fonts.small_bold)) {
                auto r = engine_.set_training_mode(m.id);
                if (r.ok) { st_.training_mode = m.id; toasts_.push(T(L, "Multiplicador de treino atualizado!", "Training multiplier updated!")); }
                else toast_code(r.code, true);
            }
        }
    }
    end_studio_card();

    ImGui::Spacing();
    begin_studio_card("##bal_card");
    card_header(T(L, "Pontos de Foco (Rumo ao Estrelato / BAL)", "Focus Points (Become a Legend / BAL)"), bold,
        T(L, "Desbloqueia o teto de 21 Pontos de Foco no Rumo ao Estrelato, permitindo até 99 pontos.",
             "Unlocks the 21 Focus Points cap in Become a Legend, allowing up to 99 points."));
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SetNextItemWidth(std::round(96.0f * dpi_scale_));
    if (ImGui::InputInt("##bal_pts", &bal_focus_inp_, 0, 0, ImGuiInputTextFlags_CharsDecimal)) {
        bal_dirty_ = true;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        bal_focus_inp_ = std::clamp(bal_focus_inp_, 0, 99);
    }
    ImGui::SameLine(0, 14);
    if (toggle_switch("##sw_bal", T(L, "Travar valor", "Lock value"), &bal_lock_chk_)) bal_dirty_ = true;
    ImGui::SameLine(0, 14);
    if (icon_btn_primary(Icon::Check, T(L, "Aplicar##bal", "Apply##bal"))) {
        auto r = engine_.set_bal_focus(bal_focus_inp_, bal_lock_chk_);
        if (r.ok) {
            bal_dirty_ = false;
            char m[96];
            std::snprintf(m, sizeof(m), T(L, "Pontos de foco BAL: %d%s", "BAL Focus Points: %d%s"), r.value, r.flag ? T(L, " (Travado)", " (Locked)") : "");
            toasts_.push(m);
        } else toast_code(r.code, true);
    }
    if (bal_dirty_) {
        ImGui::SameLine(0, 12);
        ImGui::TextColored(col::GoldSoft, "%s", T(L, "Alterações não salvas", "Unsaved changes"));
    }
    end_studio_card();
}

void WorkbenchApp::render_modals(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];
    ImGuiViewport* vp = ImGui::GetMainViewport();

    // 1. Command Palette (Ctrl+K)
    if (open_cmd_modal_) {
        ImGui::OpenPopup("##CmdPalette");
        open_cmd_modal_ = false;
    }
    ImVec2 cmd_sz(std::min(560.0f * dpi_scale_, vp->Size.x - 48.0f), std::min(436.0f * dpi_scale_, vp->Size.y - 64.0f));
    ImGui::SetNextWindowSize(cmd_sz, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(std::round(20.0f * dpi_scale_), std::round(16.0f * dpi_scale_)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, std::round(14.0f * dpi_scale_));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, col::BgCard);
    ImGui::PushStyleColor(ImGuiCol_Border, col::BorderLit);
    if (ImGui::BeginPopupModal("##CmdPalette", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        card_header(T(L, "Comandos Rápidos (Ctrl+K)", "Quick Commands (Ctrl+K)"), bold);
        align_right_or_wrap(ImGui::GetFrameHeight());
        if (icon_only_button(Icon::Close, "##cmdx", col::BgCard, col::Border, col::Subtext, ImGui::GetFrameHeight())) ImGui::CloseCurrentPopup();

        ImGui::Spacing();
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        ImGui::InputTextWithHint("##cmd_q", T(L, "Digite um comando ou tela...", "Type a command or view..."), cmd_filter_, sizeof(cmd_filter_));

        struct Cmd { const char* name; const char* cat; int id; };
        const Cmd all_cmds[] = {
            {T(L, "Visão Geral", "Overview"), T(L, "Tela", "View"), 0},
            {T(L, "Meu Elenco", "My Squad"), T(L, "Tela", "View"), 1},
            {T(L, "Finanças", "Finances"), T(L, "Tela", "View"), 2},
            {T(L, "Jogador Atual", "Current Player"), T(L, "Tela", "View"), 3},
            {T(L, "Buscar Jogador (30k+)", "Search Player (30k+)"), T(L, "Tela", "View"), 4},
            {T(L, "Olheiros & Favoritos", "Scouts & Shortlist"), T(L, "Tela", "View"), 5},
            {T(L, "Partida Ao Vivo", "Live Match"), T(L, "Tela", "View"), 6},
            {T(L, "Treino & BAL", "Training & BAL"), T(L, "Tela", "View"), 7},
            {T(L, "Elenco: 100% Fôlego", "Squad: 100% Stamina"), T(L, "Ação", "Action"), 10},
            {T(L, "Elenco: Seta Azul", "Squad: Blue Arrow"), T(L, "Ação", "Action"), 11},
            {T(L, "Elenco: Curar Todas Lesões", "Squad: Heal All Injuries"), T(L, "Ação", "Action"), 12},
            {T(L, "Elenco: Contratos +5 Anos", "Squad: Extend Contracts +5y"), T(L, "Ação", "Action"), 13},
            {T(L, "Elenco: +1 OVR (DNA)", "Squad: +1 OVR (DNA)"), T(L, "Ação", "Action"), 14},
            {T(L, "Elenco: Recalibrar Economia", "Squad: Recalibrate Economy"), T(L, "Ação", "Action"), 16},
            {T(L, "Exportar Elenco (Pack JSON)", "Export Squad (Pack JSON)"), T(L, "Pack", "Pack"), 17},
            {T(L, "Importar / Cruzar Pack", "Import / Match Pack"), T(L, "Pack", "Pack"), 18},
            {T(L, "Desfazer Última Edição (Ctrl+Z)", "Undo Last Edit (Ctrl+Z)"), T(L, "Ação", "Action"), 15},
        };
        std::string fn = norm_text_utf8(cmd_filter_);
        std::vector<Cmd> filtered;
        for (const auto& c : all_cmds) {
            if (fn.empty() || norm_text_utf8(c.name).find(fn) != std::string::npos) filtered.push_back(c);
        }
        if (filtered.empty()) cmd_selected_ = 0;
        else cmd_selected_ = std::clamp(cmd_selected_, 0, static_cast<int>(filtered.size()) - 1);

        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && !filtered.empty()) cmd_selected_ = (cmd_selected_ + 1) % static_cast<int>(filtered.size());
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && !filtered.empty()) cmd_selected_ = (cmd_selected_ - 1 + static_cast<int>(filtered.size())) % static_cast<int>(filtered.size());
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();

        auto exec_cmd = [&](int id) {
            if (id <= 7) switch_view(static_cast<View>(id));
            else if (id == 10) run_squad_cmd("stamina");
            else if (id == 11) run_squad_cmd("arrow");
            else if (id == 12) run_squad_cmd("heal");
            else if (id == 13) run_squad_cmd("contract");
            else if (id == 14) run_squad_cmd("ovr1");
            else if (id == 15) trigger_undo();
            else if (id == 16) run_squad_cmd("econ_recal");
            else if (id == 17) open_pack_studio(true, true);
            else if (id == 18) open_pack_studio(false, true);
            ImGui::CloseCurrentPopup();
        };

        if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && !filtered.empty()) exec_cmd(filtered[cmd_selected_].id);

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, col::BgInput);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(std::round(10.0f * dpi_scale_), std::round(8.0f * dpi_scale_)));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, std::round(9.0f * dpi_scale_));
        float list_h = std::max(100.0f * dpi_scale_, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() - 12.0f * dpi_scale_);
        ImGui::BeginChild("##cmd_list", ImVec2(-1.0f, list_h),
                          ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
        for (int i = 0; i < static_cast<int>(filtered.size()); ++i) {
            bool sel = (i == cmd_selected_);
            if (ImGui::Selectable(filtered[i].name, sel, 0, ImVec2(0, std::round(28.0f * dpi_scale_)))) exec_cmd(filtered[i].id);
            float cat_w = ImGui::CalcTextSize(filtered[i].cat).x;
            float sb_w = (ImGui::GetScrollMaxY() > 0.0f) ? ImGui::GetStyle().ScrollbarSize : 0.0f;
            float right_lim = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - sb_w - cat_w - 8.0f * dpi_scale_;
            ImGui::SameLine(std::max(100.0f * dpi_scale_, right_lim));
            ImGui::TextColored(col::Muted, "%s", filtered[i].cat);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        ImGui::Spacing();
        section_label(T(L, "SETAS NAVEGAR  |  ENTER EXECUTAR  |  ESC FECHAR", "ARROWS NAVIGATE  |  ENTER RUN  |  ESC CLOSE"), fonts);
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    // 2. JSON Export / Import Modal
    if (open_json_modal_) {
        ImGui::OpenPopup("##JsonModal");
        open_json_modal_ = false;
    }
    ImVec2 jm_sz(std::min(620.0f * dpi_scale_, vp->Size.x - 48.0f), std::min(430.0f * dpi_scale_, vp->Size.y - 64.0f));
    ImGui::SetNextWindowSize(jm_sz, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(std::round(20.0f * dpi_scale_), std::round(18.0f * dpi_scale_)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, std::round(14.0f * dpi_scale_));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, col::BgCard);
    ImGui::PushStyleColor(ImGuiCol_Border, col::BorderLit);
    if (ImGui::BeginPopupModal("##JsonModal", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        card_header(json_title_.c_str(), bold);
        align_right_or_wrap(ImGui::GetFrameHeight());
        if (icon_only_button(Icon::Close, "##jmx", col::BgCard, col::Border, col::Subtext, ImGui::GetFrameHeight())) ImGui::CloseCurrentPopup();
        ImGui::Separator();
        ImGui::Spacing();

        float ta_h = std::max(120.0f * dpi_scale_, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - 10.0f * dpi_scale_);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, col::BgInput);
        ImGui::InputTextMultiline("##json_ta", &json_buf_, ImVec2(-1.0f, ta_h),
            json_is_export_ ? ImGuiInputTextFlags_ReadOnly : 0);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        if (json_is_export_) {
            if (icon_btn_primary(Icon::Copy, T(L, "Copiar JSON", "Copy JSON"))) {
                ImGui::SetClipboardText(json_buf_.c_str());
                toasts_.push(T(L, "JSON copiado para a área de transferência!", "JSON copied to clipboard!"));
                ImGui::CloseCurrentPopup();
            }
        } else {
            if (ImGui::Button(T(L, "Colar da Área de Transferência", "Paste from Clipboard"))) {
                if (const char* cb = ImGui::GetClipboardText()) json_buf_ = cb;
            }
            ImGui::SameLine(0, 8);
            if (icon_btn_primary(Icon::Import, T(L, "Importar Backup", "Import Backup"))) {
                auto r = engine_.import_player_json(json_buf_);
                if (r.ok) {
                    player_ = r;
                    sync_player_form_buffers();
                    wake_poller();
                    toasts_.push(T(L, "Backup JSON importado!", "JSON backup imported!"));
                    ImGui::CloseCurrentPopup();
                } else toast_code(r.code, true);
            }
        }
        ImGui::SameLine(0, 8);
        if (ImGui::Button(T(L, "Fechar", "Close")) || ImGui::IsKeyPressed(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    // 3. Squad / Player Pack Studio Modal (Export + Cross-Save Match Preview + Selective Import)
    render_pack_studio_modal(fonts);
}

void WorkbenchApp::refresh_pack_library() {
    pack_library_ = list_saved_packs();
    if (pack_library_.empty()) {
        pack_lib_selected_ = -1;
    } else {
        pack_lib_selected_ = std::clamp(pack_lib_selected_, 0, static_cast<int>(pack_library_.size()) - 1);
    }
}

void WorkbenchApp::run_pack_preview(int forced_idx) {
    pack_forced_idx_ = forced_idx;
    if (pack_import_raw_.empty()) {
        pack_preview_ = {};
        return;
    }
    pack_preview_ = engine_.preview_pack_import(pack_import_raw_, pack_scope_, pack_opts_, forced_idx);
}

void WorkbenchApp::open_pack_studio(bool start_on_export, bool squad_mode) {
    open_pack_modal_ = true;
    pack_tab_ = start_on_export ? 1 : 0;
    pack_export_is_squad_ = squad_mode;
    pack_scope_ = squad_mode ? PackMatchScope::UserSquad : PackMatchScope::CurrentPlayer;
    pack_show_paste_box_ = false;
    pack_forced_idx_ = -1;

    refresh_pack_library();

    if (start_on_export) {
        std::string def_lbl = squad_mode
            ? (st_.club.empty() ? std::string("Meu Elenco") : st_.club)
            : (has_player_details_ ? player_.name : std::string("Jogador"));
        copy_str(pack_label_buf_, sizeof(pack_label_buf_), def_lbl);
        ActionResult r = squad_mode ? engine_.export_squad_pack_json(pack_label_buf_) : engine_.export_player_pack_json();
        if (r.ok) {
            pack_export_json_ = std::move(r.text);
            pack_export_count_ = r.count;
        } else {
            pack_export_json_.clear();
            pack_export_count_ = 0;
            toast_code(r.code, true);
        }
    } else if (!pack_library_.empty() && pack_import_raw_.empty()) {
        pack_lib_selected_ = 0;
        if (load_text_file_utf8(pack_library_[0].path, pack_import_raw_)) {
            run_pack_preview(-1);
        }
    } else if (!pack_import_raw_.empty()) {
        run_pack_preview(-1);
    }
}

void WorkbenchApp::render_pack_studio_modal(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];
    ImGuiViewport* vp = ImGui::GetMainViewport();

    if (open_pack_modal_) {
        ImGui::OpenPopup("##PackStudioModal");
        open_pack_modal_ = false;
    }

    ImVec2 m_sz(std::min(920.0f * dpi_scale_, vp->Size.x - 40.0f),
                std::min(680.0f * dpi_scale_, vp->Size.y - 48.0f));
    ImGui::SetNextWindowSize(m_sz, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(std::round(20.0f * dpi_scale_), std::round(16.0f * dpi_scale_)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, std::round(14.0f * dpi_scale_));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, col::BgCard);
    ImGui::PushStyleColor(ImGuiCol_Border, col::BorderLit);

    if (ImGui::BeginPopupModal("##PackStudioModal", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        // Header + Tabs + Close button
        card_header(T(L, "Central de Packs (Elenco & Atletas)", "Pack Studio (Squad & Players)"), bold);
        align_right_or_wrap(ImGui::GetFrameHeight());
        if (icon_only_button(Icon::Close, "##pkx", col::BgCard, col::Border, col::Subtext, ImGui::GetFrameHeight()) || ImGui::IsKeyPressed(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();

        ImGui::Spacing();
        const char* t_imp = T(L, "Importar & Cruzar Save##pkt0", "Import & Match Save##pkt0");
        const char* t_exp = T(L, "Exportar Pack##pkt1", "Export Pack##pkt1");
        if (styled_button(t_imp, pack_tab_ == 0 ? col::Accent : col::BgInput,
                          pack_tab_ == 0 ? col::AccentSoft : col::Border,
                          pack_tab_ == 0 ? ImVec4(1, 1, 1, 1) : col::Subtext)) {
            pack_tab_ = 0;
            refresh_pack_library();
        }
        ImGui::SameLine(0, 8.0f * dpi_scale_);
        if (styled_button(t_exp, pack_tab_ == 1 ? col::Accent : col::BgInput,
                          pack_tab_ == 1 ? col::AccentSoft : col::Border,
                          pack_tab_ == 1 ? ImVec4(1, 1, 1, 1) : col::Subtext)) {
            pack_tab_ = 1;
            ActionResult r = pack_export_is_squad_ ? engine_.export_squad_pack_json(pack_label_buf_) : engine_.export_player_pack_json();
            if (r.ok) { pack_export_json_ = std::move(r.text); pack_export_count_ = r.count; }
        }
        const char* dir_btn = T(L, "Abrir Pasta Packs##pkdir", "Open Packs Folder##pkdir");
        align_right_or_wrap(calc_btn_w(dir_btn));
        if (ImGui::Button(dir_btn)) {
            std::wstring wdir = utf8_to_wide(get_packs_directory());
            ShellExecuteW(static_cast<HWND>(hwnd_), L"open", wdir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        ImGui::Separator();
        ImGui::Spacing();

        const float footer_h = ImGui::GetFrameHeightWithSpacing() + 12.0f * dpi_scale_;

        if (pack_tab_ == 1) {
            // ==================== EXPORT TAB ====================
            const char* m_sq = T(L, "Elenco Completo##pke0", "Full Squad##pke0");
            const char* m_pl = T(L, "Jogador Atual##pke1", "Current Player##pke1");
            if (styled_button(m_sq, pack_export_is_squad_ ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.24f) : col::BgInput,
                              pack_export_is_squad_ ? col::Accent : col::Border,
                              pack_export_is_squad_ ? col::AccentSoft : col::Subtext)) {
                pack_export_is_squad_ = true;
                copy_str(pack_label_buf_, sizeof(pack_label_buf_), st_.club.empty() ? "Meu Elenco" : st_.club);
                auto r = engine_.export_squad_pack_json(pack_label_buf_);
                if (r.ok) { pack_export_json_ = std::move(r.text); pack_export_count_ = r.count; }
                else toast_code(r.code, true);
            }
            ImGui::SameLine(0, 6.0f * dpi_scale_);
            if (styled_button(m_pl, !pack_export_is_squad_ ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.24f) : col::BgInput,
                              !pack_export_is_squad_ ? col::Accent : col::Border,
                              !pack_export_is_squad_ ? col::AccentSoft : col::Subtext)) {
                pack_export_is_squad_ = false;
                copy_str(pack_label_buf_, sizeof(pack_label_buf_), has_player_details_ ? player_.name : "Jogador");
                auto r = engine_.export_player_pack_json();
                if (r.ok) { pack_export_json_ = std::move(r.text); pack_export_count_ = r.count; }
                else toast_code(r.code, true);
            }
            ImGui::SameLine(0, 14.0f * dpi_scale_);
            char cnt_tag[64];
            std::snprintf(cnt_tag, sizeof(cnt_tag), T(L, "%d atleta(s) pronto(s)", "%d player(s) ready"), pack_export_count_);
            draw_pill(cnt_tag, col::GreenSoft, ImVec4(0.063f, 0.725f, 0.506f, 0.16f), ImVec4(0.063f, 0.725f, 0.506f, 0.45f), true);

            ImGui::Spacing();
            section_label(T(L, "NOME DO PACK (IDENTIFICADOR NA BIBLIOTECA)", "PACK NAME (LIBRARY LABEL)"), fonts);
            ImGui::SetNextItemWidth(std::min(420.0f * dpi_scale_, ImGui::GetContentRegionAvail().x));
            if (ImGui::InputText("##pk_lbl", pack_label_buf_, sizeof(pack_label_buf_))) {
                // Regenerate JSON lazily on save/copy
            }
            ImGui::TextColored(col::Subtext, "%s",
                T(L, "Inclui: ID Konami, 25 atributos (OVR exato), 13 aptidões, estilo, 41 habilidades, 7 COMP, pior pé, forma, função no time, bio, contrato e economia.",
                     "Includes: Konami ID, 25 stats (exact OVR), 13 positions, style, 41 skills, 7 COM, weak foot, form, team role, bio, contract and economy."));

            ImGui::Spacing();
            auto ensure_export_fresh = [&]() -> bool {
                ActionResult r = pack_export_is_squad_
                    ? engine_.export_squad_pack_json(pack_label_buf_)
                    : engine_.export_player_pack_json();
                if (r.ok) {
                    pack_export_json_ = std::move(r.text);
                    pack_export_count_ = r.count;
                    return true;
                }
                return !pack_export_json_.empty();
            };

            if (btn_green(T(L, "Salvar na Biblioteca##pksv", "Save to Library##pksv"))) {
                if (ensure_export_fresh()) {
                    std::string fn = make_default_pack_filename(pack_label_buf_, st_.club);
                    std::filesystem::path full = std::filesystem::path(utf8_to_wide(get_packs_directory())) / std::filesystem::path(utf8_to_wide(fn));
                    auto u8 = full.u8string();
                    std::string full_s(reinterpret_cast<const char*>(u8.data()), u8.size());
                    if (save_text_file_utf8(full_s, pack_export_json_)) {
                        refresh_pack_library();
                        toasts_.push(std::string(T(L, "Pack salvo na biblioteca!\n", "Pack saved to library!\n")) + fn);
                    } else {
                        toasts_.push(T(L, "Falha ao gravar arquivo do pack.", "Failed to write pack file."), true);
                    }
                } else {
                    toasts_.push(T(L, "Nenhum elenco/jogador conectado para exportar.", "No squad/player connected to export."), true);
                }
            }
            flow_next(calc_btn_w(T(L, "Salvar Arquivo (.json)...", "Save File (.json)...")));
            if (btn_primary(T(L, "Salvar Arquivo (.json)...##pksvf", "Save File (.json)...##pksvf"))) {
                if (ensure_export_fresh()) {
                    std::string fn = make_default_pack_filename(pack_label_buf_, st_.club);
                    std::string dst = pick_save_json_dialog(static_cast<HWND>(hwnd_), get_packs_directory(), fn);
                    if (!dst.empty()) {
                        if (save_text_file_utf8(dst, pack_export_json_)) {
                            refresh_pack_library();
                            toasts_.push(T(L, "Arquivo JSON exportado com sucesso!", "JSON file exported successfully!"));
                        } else {
                            toasts_.push(T(L, "Erro ao salvar arquivo.", "Error saving file."), true);
                        }
                    }
                }
            }
            flow_next(icon_button_width(T(L, "Copiar JSON", "Copy JSON")));
            if (icon_btn_plain(Icon::Copy, T(L, "Copiar JSON##pkcp", "Copy JSON##pkcp"))) {
                if (ensure_export_fresh()) {
                    ImGui::SetClipboardText(pack_export_json_.c_str());
                    toasts_.push(T(L, "JSON do pack copiado!", "Pack JSON copied!"));
                }
            }

            ImGui::Spacing();
            float ta_h = std::max(120.0f * dpi_scale_, ImGui::GetContentRegionAvail().y - footer_h);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, col::BgInput);
            ImGui::InputTextMultiline("##pk_exp_ta", &pack_export_json_, ImVec2(-1.0f, ta_h), ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor();
        } else {
            // ==================== IMPORT & CROSS-SAVE MATCH TAB ====================
            // Row 1: Pack source selector (Library combo + Open File + Paste JSON)
            const char* open_f_lbl = T(L, "Abrir .json...##pkof", "Open .json...##pkof");
            const char* paste_lbl = T(L, "Colar JSON##pkps", "Paste JSON##pkps");
            const char* ref_lbl = T(L, "Atualizar##pkrf", "Refresh##pkrf");
            float btns_w = calc_btn_w(open_f_lbl) + calc_btn_w(paste_lbl) + calc_btn_w(ref_lbl) + 24.0f * dpi_scale_;
            float combo_w = std::max(180.0f * dpi_scale_, ImGui::GetContentRegionAvail().x - btns_w);

            std::string lib_prev = T(L, "Selecionar pack salvo na biblioteca...", "Select saved pack from library...");
            if (pack_lib_selected_ >= 0 && pack_lib_selected_ < static_cast<int>(pack_library_.size())) {
                const auto& it = pack_library_[pack_lib_selected_];
                char b[180];
                std::snprintf(b, sizeof(b), "%s  (%d %s | OVR %.1f)",
                              it.label.c_str(), it.player_count, T(L, "jog.", "pl."), it.avg_ovr);
                lib_prev = b;
            }
            ImGui::SetNextItemWidth(combo_w);
            if (ImGui::BeginCombo("##pk_lib_sel", lib_prev.c_str())) {
                if (pack_library_.empty()) {
                    wrapped_colored(col::Muted, "%s", T(L, "Nenhum pack salvo ainda em Documentos/PES 2021 Workbench/Packs", "No saved packs yet"));
                }
                for (int i = 0; i < static_cast<int>(pack_library_.size()); ++i) {
                    const auto& it = pack_library_[i];
                    char opt[220];
                    std::snprintf(opt, sizeof(opt), "%s  [%d | OVR %.1f | %s]##pkl%d",
                                  it.label.c_str(), it.player_count, it.avg_ovr, it.created_at.c_str(), i);
                    if (ImGui::Selectable(opt, i == pack_lib_selected_)) {
                        pack_lib_selected_ = i;
                        pack_show_paste_box_ = false;
                        if (load_text_file_utf8(it.path, pack_import_raw_)) {
                            run_pack_preview(-1);
                        }
                    }
                }
                ImGui::EndCombo();
            }
            flow_next(calc_btn_w(open_f_lbl), 6.0f * dpi_scale_);
            if (btn_primary(open_f_lbl)) {
                std::string pth = pick_open_json_dialog(static_cast<HWND>(hwnd_), get_packs_directory());
                if (!pth.empty()) {
                    if (load_text_file_utf8(pth, pack_import_raw_)) {
                        pack_lib_selected_ = -1;
                        pack_show_paste_box_ = false;
                        run_pack_preview(-1);
                        if (!pack_preview_.ok) toast_code(pack_preview_.code, true);
                    } else {
                        toasts_.push(T(L, "Nao foi possivel ler o arquivo selecionado.", "Could not read selected file."), true);
                    }
                }
            }
            flow_next(calc_btn_w(paste_lbl), 6.0f * dpi_scale_);
            if (ImGui::Button(paste_lbl)) {
                pack_show_paste_box_ = !pack_show_paste_box_;
                if (pack_show_paste_box_) {
                    if (const char* cb = ImGui::GetClipboardText()) pack_paste_buf_ = cb;
                }
            }
            flow_next(calc_btn_w(ref_lbl), 6.0f * dpi_scale_);
            if (ImGui::Button(ref_lbl)) {
                refresh_pack_library();
                if (!pack_import_raw_.empty()) run_pack_preview(pack_forced_idx_);
            }

            if (pack_show_paste_box_) {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, col::BgInput);
                ImGui::InputTextMultiline("##pk_paste_ta", &pack_paste_buf_, ImVec2(-1.0f, std::round(84.0f * dpi_scale_)));
                ImGui::PopStyleColor();
                if (btn_primary(T(L, "Analisar JSON Colado##pkpa", "Analyze Pasted JSON##pkpa"))) {
                    pack_import_raw_ = pack_paste_buf_;
                    pack_show_paste_box_ = false;
                    run_pack_preview(-1);
                    if (!pack_preview_.ok) toast_code(pack_preview_.code, true);
                }
                ImGui::SameLine(0, 8.0f * dpi_scale_);
                if (ImGui::Button(T(L, "Cancelar##pkpc", "Cancel##pkpc"))) pack_show_paste_box_ = false;
            }

            ImGui::Spacing();
            // Row 2: Cross-save Match Scope selector
            bool opts_dirty = false;
            auto scope_btn = [&](PackMatchScope sc, const char* lbl, bool first) {
                if (!first) flow_next(calc_btn_w(lbl), 6.0f * dpi_scale_);
                bool act = (pack_scope_ == sc);
                if (styled_button(lbl, act ? ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.24f) : col::BgInput,
                                  act ? col::Accent : col::Border, act ? col::AccentSoft : col::Subtext)) {
                    if (pack_scope_ != sc) {
                        pack_scope_ = sc;
                        pack_forced_idx_ = -1;
                        opts_dirty = true;
                    }
                }
            };
            ImGui::AlignTextToFramePadding();
            section_label(T(L, "CRUZAR COM:", "MATCH AGAINST:"), fonts);
            ImGui::SameLine(0, 8.0f * dpi_scale_);
            scope_btn(PackMatchScope::UserSquad, T(L, "Meu Elenco (Time Atual)##sc0", "My Squad (Current Club)##sc0"), true);
            scope_btn(PackMatchScope::GlobalDatabase, T(L, "Base Global (30.000+ Save)##sc1", "Global Database (30k+ Save)##sc1"), false);
            scope_btn(PackMatchScope::CurrentPlayer, T(L, "Atleta Atual (1 Jogador)##sc2", "Current Player (Single)##sc2"), false);

            ImGui::Spacing();
            // Row 3: Selective category checkboxes (full user control)
            auto opt_chk = [&](const char* lbl, bool* v, bool first) {
                float cw = ImGui::CalcTextSize(lbl).x + 34.0f * dpi_scale_;
                if (!first) flow_next(cw, 10.0f * dpi_scale_);
                if (ImGui::Checkbox(lbl, v)) opts_dirty = true;
            };
            opt_chk(T(L, "Atributos + Posições (OVR)", "Stats + Positions (OVR)"), &pack_opts_.apply_stats_pos, true);
            opt_chk(T(L, "Estilo + Habilidades + COMP", "Style + Skills + COM"), &pack_opts_.apply_skills_style, false);
            opt_chk(T(L, "Pior Pé / Forma / Lesão", "Weak Foot / Form / Inj."), &pack_opts_.apply_traits, false);
            opt_chk(T(L, "Função no Time", "Team Role"), &pack_opts_.apply_team_role, false);
            opt_chk(T(L, "Bio (Idade/Físico/Nome)", "Bio (Age/Body/Name)"), &pack_opts_.apply_bio, false);
            bool prev_pe = pack_opts_.apply_pack_economy;
            opt_chk(T(L, "Economia exata do Pack", "Exact Pack Economy"), &pack_opts_.apply_pack_economy, false);
            if (pack_opts_.apply_pack_economy && !prev_pe) pack_opts_.auto_recal_economy = false;
            bool prev_ar = pack_opts_.auto_recal_economy;
            opt_chk(T(L, "Escalar Economia Auto", "Auto-Scale Economy"), &pack_opts_.auto_recal_economy, false);
            if (pack_opts_.auto_recal_economy && !prev_ar) pack_opts_.apply_pack_economy = false;
            opt_chk(T(L, "Condição (Fôlego/Seta)", "Condition (Stamina/Form)"), &pack_opts_.apply_condition, false);

            if (opts_dirty && !pack_import_raw_.empty()) {
                run_pack_preview(pack_forced_idx_);
            }

            ImGui::Spacing();
            // Row 4: Preview Summary Bar + Dry-Run Match Table
            if (pack_preview_.ok) {
                char s1[48], s2[48], s3[48];
                std::snprintf(s1, sizeof(s1), T(L, "ID Exato: %d", "Exact ID: %d"), pack_preview_.matched_pid);
                std::snprintf(s2, sizeof(s2), T(L, "Nome/Manual: %d", "Name/Manual: %d"), pack_preview_.matched_name);
                std::snprintf(s3, sizeof(s3), T(L, "Fora do escopo: %d", "Unmatched: %d"), pack_preview_.unmatched);
                draw_pill(s1, col::GreenSoft, ImVec4(0.063f, 0.725f, 0.506f, 0.16f), ImVec4(0.063f, 0.725f, 0.506f, 0.48f), true);
                ImGui::SameLine(0, 6.0f * dpi_scale_);
                draw_pill(s2, col::GoldSoft, ImVec4(0.961f, 0.620f, 0.043f, 0.16f), ImVec4(0.961f, 0.620f, 0.043f, 0.48f), true);
                ImGui::SameLine(0, 6.0f * dpi_scale_);
                draw_pill(s3, col::Subtext, ImVec4(col::Subtext.x, col::Subtext.y, col::Subtext.z, 0.12f), col::Border, true);

                const char* m_all = T(L, "Marcar Encontrados##pka", "Select Matched##pka");
                const char* m_non = T(L, "Desmarcar##pkn", "Clear##pkn");
                align_right_or_wrap(calc_btn_w(m_all) + calc_btn_w(m_non) + 8.0f * dpi_scale_);
                if (ImGui::Button(m_all)) {
                    for (auto& r : pack_preview_.rows) r.selected = (r.match_kind != PackMatchKind::None);
                }
                ImGui::SameLine(0, 6.0f * dpi_scale_);
                if (ImGui::Button(m_non)) {
                    for (auto& r : pack_preview_.rows) r.selected = false;
                }

                ImGui::Spacing();
                float tbl_h = std::max(140.0f * dpi_scale_, ImGui::GetContentRegionAvail().y - footer_h);
                float c_chk = std::round(34.0f * dpi_scale_);
                float c_mtc = std::round(92.0f * dpi_scale_);
                float c_ovr = std::round(118.0f * dpi_scale_);
                float c_skl = std::round(78.0f * dpi_scale_);
                float c_mkt = std::round(168.0f * dpi_scale_);
                float c_pad = ImGui::GetStyle().CellPadding.x * 14.0f + ImGui::GetStyle().ScrollbarSize + 18.0f * dpi_scale_;
                float c_nam = std::max(std::round(170.0f * dpi_scale_),
                                       ImGui::GetContentRegionAvail().x - (c_chk + c_mtc + c_ovr + c_skl + c_mkt + c_pad));

                ImGuiTableFlags ptf = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                                      ImGuiTableFlags_ScrollX | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX;
                if (ImGui::BeginTable("##pack_prev_tbl", 6, ptf, ImVec2(0, tbl_h))) {
                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableSetupColumn("##pksel", ImGuiTableColumnFlags_WidthFixed, c_chk);
                    ImGui::TableSetupColumn("MATCH", ImGuiTableColumnFlags_WidthFixed, c_mtc);
                    ImGui::TableSetupColumn(T(L, "ATLETA", "PLAYER"), ImGuiTableColumnFlags_WidthFixed, c_nam);
                    ImGui::TableSetupColumn("OVR", ImGuiTableColumnFlags_WidthFixed, c_ovr);
                    ImGui::TableSetupColumn("SKILLS", ImGuiTableColumnFlags_WidthFixed, c_skl);
                    ImGui::TableSetupColumn(T(L, "MERCADO", "MARKET"), ImGuiTableColumnFlags_WidthFixed, c_mkt);
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < pack_preview_.rows.size(); ++i) {
                        auto& row = pack_preview_.rows[i];
                        bool matched = (row.match_kind != PackMatchKind::None);
                        ImGui::TableNextRow();
                        ImGui::PushID(static_cast<int>(i));

                        ImGui::TableSetColumnIndex(0);
                        ImGui::BeginDisabled(!matched);
                        ImGui::Checkbox("##rchk", &row.selected);
                        ImGui::EndDisabled();

                        ImGui::TableSetColumnIndex(1);
                        if (row.match_kind == PackMatchKind::ExactPid) {
                            draw_pill(T(L, "ID Exato", "Exact ID"), col::GreenSoft, ImVec4(0.063f, 0.725f, 0.506f, 0.16f), ImVec4(0.063f, 0.725f, 0.506f, 0.45f), true);
                        } else if (row.match_kind == PackMatchKind::ExactName) {
                            draw_pill(T(L, "Nome", "Name"), col::GoldSoft, ImVec4(0.961f, 0.620f, 0.043f, 0.16f), ImVec4(0.961f, 0.620f, 0.043f, 0.45f), true);
                        } else if (row.match_kind == PackMatchKind::ManualCurrent) {
                            draw_pill("Manual", col::AccentSoft, ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.18f), ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.48f), true);
                        } else if (pack_scope_ == PackMatchScope::CurrentPlayer && has_player_details_) {
                            if (ImGui::SmallButton(T(L, "Escolher##pkch", "Pick##pkch"))) {
                                run_pack_preview(row.pack_idx);
                                ImGui::PopID();
                                break;
                            }
                        } else {
                            draw_pill(T(L, "Fora", "Miss"), col::Muted, ImVec4(col::Subtext.x, col::Subtext.y, col::Subtext.z, 0.10f), col::Border, true);
                        }

                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextColored(matched ? col::Text : col::Muted, "%s", row.target_name.c_str());

                        ImGui::TableSetColumnIndex(3);
                        if (matched) {
                            int d = row.new_ovr - row.cur_ovr;
                            if (d != 0) {
                                ImGui::TextColored(d > 0 ? col::GreenSoft : col::RedSoft, "%d -> %d (%+d)", row.cur_ovr, row.new_ovr, d);
                            } else {
                                ImGui::TextColored(col::Subtext, "%d (=)", row.cur_ovr);
                            }
                        } else {
                            ImGui::TextColored(col::Muted, "Pack %d", row.new_ovr);
                        }

                        ImGui::TableSetColumnIndex(4);
                        if (matched) {
                            if (row.new_skills_cnt != row.cur_skills_cnt) {
                                ImGui::TextColored(col::AccentSoft, "%d -> %d", row.cur_skills_cnt, row.new_skills_cnt);
                            } else {
                                ImGui::TextColored(col::Subtext, "%d", row.cur_skills_cnt);
                            }
                        } else {
                            ImGui::TextColored(col::Muted, "%d", row.new_skills_cnt);
                        }

                        ImGui::TableSetColumnIndex(5);
                        if (matched) {
                            if (row.new_market_raw != row.cur_market_raw) {
                                ImGui::TextColored(row.new_market_raw > row.cur_market_raw ? col::GreenSoft : col::GoldSoft,
                                                   "%s -> %s",
                                                   fmt_money(row.cur_market_raw, settings_.currency_idx, L).c_str(),
                                                   fmt_money(row.new_market_raw, settings_.currency_idx, L).c_str());
                            } else {
                                ImGui::TextColored(col::Subtext, "%s", fmt_money(row.cur_market_raw, settings_.currency_idx, L).c_str());
                            }
                        } else {
                            ImGui::TextColored(col::Muted, "-");
                        }

                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
            } else {
                float empty_h = std::max(140.0f * dpi_scale_, ImGui::GetContentRegionAvail().y - footer_h);
                ImGui::BeginChild("##pk_empty", ImVec2(-1.0f, empty_h), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
                ImGui::TextWrapped("%s",
                    T(L, "Selecione um pack salvo na Biblioteca, abra um arquivo .json ou cole um JSON para cruzar automaticamente com o elenco atual, com a base global de 30.000+ atletas ou com o jogador em foco.",
                         "Pick a saved pack from the Library, open a .json file, or paste JSON to automatically match against your current squad, the 30,000+ database, or the active player."));
                ImGui::EndChild();
            }
        }

        // Footer bar
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (pack_tab_ == 0) {
            int sel_cnt = 0;
            if (pack_preview_.ok) {
                for (const auto& r : pack_preview_.rows) if (r.selected && r.match_kind != PackMatchKind::None) ++sel_cnt;
            }
            char app_lbl[64];
            std::snprintf(app_lbl, sizeof(app_lbl), T(L, "Aplicar em %d Atleta(s)##pkgo", "Apply to %d Player(s)##pkgo"), sel_cnt);
            ImGui::BeginDisabled(sel_cnt == 0);
            if (btn_green(app_lbl)) {
                auto r = engine_.apply_pack_import(pack_preview_, pack_opts_);
                if (r.ok) {
                    char msg[128];
                    std::snprintf(msg, sizeof(msg),
                                  T(L, "Pack aplicado com sucesso em %d atleta(s)!\nUse Desfazer Lote caso queira reverter.",
                                       "Pack applied cleanly to %d player(s)!\nUse Undo Batch to revert if needed."),
                                  r.count);
                    toasts_.push(msg);
                    reload_squad();
                    if (has_player_details_) reload_player();
                    reload_finances();
                    wake_poller();
                    run_pack_preview(pack_forced_idx_);
                } else {
                    toast_code(r.code, true);
                }
            }
            ImGui::EndDisabled();

            int bcnt = engine_.last_batch_undo_count();
            if (bcnt > 0) {
                ImGui::SameLine(0, 8.0f * dpi_scale_);
                char ulbl[64];
                std::snprintf(ulbl, sizeof(ulbl), T(L, "Desfazer Última Importação (%d)###pkub", "Undo Last Import (%d)###pkub"), bcnt);
                if (btn_gold(ulbl)) {
                    auto r = engine_.undo_last_batch();
                    if (r.ok) {
                        char msg[96];
                        std::snprintf(msg, sizeof(msg), T(L, "Lote restaurado (%d atletas)!", "Batch restored (%d players)!"), r.count);
                        toasts_.push(msg);
                        reload_squad();
                        if (has_player_details_) reload_player();
                        reload_finances();
                        wake_poller();
                        run_pack_preview(pack_forced_idx_);
                    }
                }
            }
        }
        const char* cls_lbl = T(L, "Fechar##pkcls", "Close##pkcls");
        align_right_or_wrap(calc_btn_w(cls_lbl));
        if (ImGui::Button(cls_lbl)) ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

#ifdef PES_QA_DUMP
void WorkbenchApp::qa_populate_fixture() {
    if (const char* ql = std::getenv("PES_QA_LANG")) {   // QA: idioma so em memoria, nunca grava no settings.ini
        settings_.lang = (ql[0] == 'e' && ql[1] == 's') ? Lang::ES : ((ql[0] == 'e') ? Lang::EN : Lang::PT);
    }
    st_.attached = true;
    st_.exe = "PES2021.exe";
    st_.pid = 26960;
    st_.career = 0x7FF4DA370010ULL;
    st_.mode_code = "master_league";
    st_.club = "FLAMENGO [FLA]";
    st_.hooks = {false, true, false, true, false, false, true};
    st_.training_mode = 2;
    st_.stamina_mode = 1;
    st_.has_finances = true;
    st_.finances.ok = true;
    st_.finances.transfer_raw = 209300;
    st_.finances.salary_avail_raw = 25624;
    st_.finances.salary_gross_raw = 420000;
    st_.finances.squad_wages_raw = 394376;
    fin_ = st_.finances;
    copy_str(fin_trf_buf_, sizeof(fin_trf_buf_), "20.93M");
    copy_str(fin_sal_buf_, sizeof(fin_sal_buf_), "2.56M");

    st_.has_player = true;
    st_.player.addr = 0x7FF4DA441100ULL;
    st_.player.name = "G. DE ARRASCAETA";
    st_.player.club = "FLAMENGO [FLA]";
    st_.player.ovr = 84;
    st_.player.pos = 8;
    st_.player.age = 32;
    st_.player.stamina = 94;
    st_.player.arrow = 4;
    st_.player.blink = true;
    st_.player.injury = 0;
    st_.player.undo_count = 3;

    player_ = {};
    player_.ok = true;
    player_.addr = st_.player.addr;
    player_.pid = 63497;
    player_.name = "G. DE ARRASCAETA";
    player_.shirt = "ARRASCAETA";
    player_.club = "FLAMENGO [FLA]";
    player_.ovr = 84;
    player_.reg_pos = 8;
    player_.nationality = 152;
    player_.age = 32;
    player_.height = 174;
    player_.weight = 73;
    player_.foot = 0;
    player_.playing_style = 5;
    player_.wf_usage = 3;
    player_.wf_acc = 3;
    player_.form_cons = 6;
    player_.inj_res = 2;
    player_.stamina = 94;
    player_.arrow = 4;
    player_.blink = true;
    player_.injury = 0;
    player_.contract_yr = 2028;
    player_.salary_raw = 39100;
    player_.market_raw = 180000;
    player_.has_release_fee = true;
    player_.release_fee_raw = 270000;
    player_.snap_ovr = 83;
    player_.team_role = 13;
    player_.team_role_lvl = 3;
    player_.undo_count = 3;
    player_.stats.fill(72);
    player_.stats[14] = 85; player_.stats[17] = 89; player_.stats[18] = 87; player_.stats[19] = 88;
    player_.stats[20] = 83; player_.stats[21] = 88; player_.stats[22] = 86; player_.stats[23] = 84;
    player_.stats[26] = 85; player_.stats[27] = 86; player_.stats[33] = 80; player_.stats[34] = 83;
    player_.stats[35] = 84; player_.stats[37] = 86; player_.stats[38] = 82; player_.stats[39] = 83;
    player_.stats[15] = 56; player_.stats[24] = 54; player_.stats[25] = 51;
    player_.stats[16] = 40; player_.stats[28] = 40; player_.stats[29] = 40; player_.stats[30] = 40; player_.stats[31] = 40;

    static constexpr int kPosOvr[13] = {45, 58, 63, 62, 71, 80, 79, 78, 84, 82, 81, 83, 79};
    static constexpr int kPosApt[13] = {0, 0, 0, 0, 1, 2, 2, 1, 2, 2, 2, 2, 1};
    for (int i = 0; i < 13; ++i) {
        player_.positions[i] = {i, kPosOvr[i], kPosApt[i], i == 8};
    }
    player_.skills.fill(false);
    player_.skills[0] = true; player_.skills[3] = true; player_.skills[8] = true;
    player_.skills[12] = true; player_.skills[15] = true; player_.skills[20] = true;
    player_.com_ai.fill(false);
    player_.com_ai[1] = true; player_.com_ai[3] = true;

    has_player_details_ = true;
    last_player_addr_ = player_.addr;
    poslab_base_stats_ = player_.stats;
    poslab_base_stats_[20] -= 2;
    poslab_base_stats_[21] -= 1;
    poslab_base_valid_ = true;
    sync_player_form_buffers();

    last_edit_.active = true;
    last_edit_.player_addr = player_.addr;
    last_edit_.time_str = "03:45:12";
    last_edit_.label_pt = "+1 OVR DNA";
    last_edit_.label_en = "+1 OVR DNA";
    last_edit_.old_ovr = 83;
    last_edit_.new_ovr = 84;
    last_edit_.econ.applied = true;
    last_edit_.econ.old_market_raw = 140000;
    last_edit_.econ.new_market_raw = 180000;
    last_edit_.econ.old_salary_raw = 32800;
    last_edit_.econ.new_salary_raw = 39100;
    last_edit_.econ.has_release_fee = true;
    last_edit_.econ.old_release_raw = 210000;
    last_edit_.econ.new_release_raw = 270000;
    last_edit_.diffs = {{20, 81, 83}, {21, 87, 88}, {17, 88, 89}, {14, 84, 85}};

    poslab_log_.clear();
    poslab_log_.push_back({"03:45:12", 9, 8, 81, 82, 84, "+1 OVR LWF", "+1 OVR LWF", {{17, 88, 89}, {34, 82, 83}}});
    poslab_log_.push_back({"03:44:50", 8, 8, 83, 84, 84, "+1 OVR DNA", "+1 OVR DNA", {{20, 81, 83}, {21, 87, 88}, {14, 84, 85}}});

    squad_.ok = true;
    squad_.club = "FLAMENGO [FLA]";
    squad_.cur_year = 2026;
    squad_.players = {
        {player_.addr, "G. DE ARRASCAETA", "g. de arrascaeta", 84, 8, 32, 2028, false, 39100, 94, 4, true, 0},
        {0x7FF4DA441200ULL, "PEDRO", "pedro", 83, 12, 29, 2028, false, 31200, 100, 4, false, 0},
        {0x7FF4DA441300ULL, "LUCAS PAQUETÁ", "lucas paqueta", 83, 8, 29, 2029, false, 31900, 91, 3, false, 0},
        {0x7FF4DA441400ULL, "SAMUEL LINO", "samuel lino", 83, 9, 26, 2029, false, 28000, 88, 3, false, 0},
        {0x7FF4DA441500ULL, "JORGINHO", "jorginho", 83, 4, 34, 2026, true, 35800, 79, 2, false, 0},
        {0x7FF4DA441600ULL, "BRUNO HENRIQUE", "bruno henrique", 81, 12, 35, 2026, true, 27400, 68, 2, false, 14},
        {0x7FF4DA441700ULL, "AGUSTÍN ROSSI", "agustin rossi", 80, 0, 31, 2028, false, 21000, 100, 3, false, 0},
        {0x7FF4DA441800ULL, "LÉO ORTIZ", "leo ortiz", 80, 1, 30, 2029, false, 22200, 95, 3, false, 0},
        {0x7FF4DA441900ULL, "NICOLÁS DE LA CRUZ", "nicolas de la cruz", 80, 5, 29, 2028, false, 22400, 86, 2, false, 0},
        {0x7FF4DA441A00ULL, "LORRAN", "lorran", 76, 8, 20, 2029, false, 7800, 98, 4, false, 0},
    };

    search_res_.ok = true;
    search_res_.total = 24433;
    search_res_.elapsed_ms = 14.2;
    search_res_.players = {
        {0x101, "ERLING HAALAND", 94, 12, 26, "MANCHESTER CITY", 248300, 2030},
        {0x102, "KYLIAN MBAPPÉ", 91, 12, 27, "REAL MADRID", 214900, 2029},
        {0x103, "LAMINE YAMAL", 91, 10, 19, "FC BARCELONA", 165500, 2030},
        {0x104, "JUDE BELLINGHAM", 89, 5, 23, "REAL MADRID", 140500, 2029},
        {0x105, "LIONEL MESSI", 89, 10, 37, "INTER MIAMI", 129400, 2026},
        {0x106, "ENDRICK", 82, 12, 20, "REAL MADRID", 29000, 2030},
    };

    scouts_.ok = true;
    auto mk = [](int idx, uint64_t handle, uint64_t addr, const char* name, int ovr, int pos, int age, const char* club,
                 int exp_y, int32_t mkt, int32_t sal, int arrow, bool blink) {
        ScoutEntry e;
        e.slot_idx = idx; e.handle = handle; e.addr = addr; e.pid = static_cast<uint32_t>(handle & 0xFFFF);
        e.name = name; e.ovr = ovr; e.pos = pos; e.age = age; e.club = club; e.exp_y = exp_y;
        e.market_raw = mkt; e.salary_raw = sal; e.arrow = arrow; e.blink = blink;
        return e;
    };
    ScoutListState fav; fav.slot = 4; fav.cap = 100; fav.pt = "Favoritos"; fav.en = "Shortlist";
    fav.entries = {
        mk(0, 0x1001, 0x103, "LAMINE YAMAL", 91, 10, 19, "FC BARCELONA", 2030, 165500, 21000, 4, true),
        mk(1, 0x1002, 0x106, "ENDRICK", 82, 12, 20, "REAL MADRID", 2030, 52000, 9000, 3, false),
        mk(2, 0x1003, 0x107, "ALEXIS MAC ALLISTER", 85, 5, 26, "LIVERPOOL", 2028, 78000, 14500, 2, false),
        mk(3, 0x1004, 0x108, "MARCOS LEONARDO", 79, 12, 21, "AL HILAL", 2027, 21000, 6400, 3, false),
        mk(4, 0x1005, 0x109, "JOAO NEVES", 84, 4, 21, "", 2026, 70000, 12000, 4, false),
    };
    ScoutListState s1; s1.slot = 1; s1.cap = 30; s1.pt = "Olheiro 1"; s1.en = "Scout 1";
    s1.entries = {
        mk(0, 0x1001, 0x103, "LAMINE YAMAL", 91, 10, 19, "FC BARCELONA", 2030, 165500, 21000, 4, true),
        mk(1, 0x1006, 0x10A, "ESTEVAO", 80, 10, 18, "CHELSEA", 2032, 38000, 5200, 3, false),
    };
    ScoutListState s2; s2.slot = 2; s2.cap = 30; s2.pt = "Olheiro 2"; s2.en = "Scout 2";
    ScoutListState s3; s3.slot = 3; s3.cap = 30; s3.pt = "Olheiro 3"; s3.en = "Scout 3";
    scouts_.lists = {fav, s1, s2, s3};
    scout_mask_.clear();
    for (const auto& l : scouts_.lists) for (const auto& e : l.entries) scout_mask_[e.addr] |= static_cast<uint8_t>(1u << (l.slot - 1));
    active_scout_slot_ = 4;
    scout_sel_ = {0x1002, 0x1003};
}

void WorkbenchApp::qa_select_screen(int view_idx, int ptab_idx, int modal_kind) {
    current_view_ = static_cast<View>(std::clamp(view_idx, 0, 7));
    current_ptab_ = static_cast<PlayerTab>(std::clamp(ptab_idx, 0, 4));
    open_cmd_modal_ = (modal_kind == 1);
    open_json_modal_ = (modal_kind == 2);
    open_pack_modal_ = (modal_kind == 3);
    qa_open_prefs_ = (modal_kind == 5 || modal_kind == 9);
    prefs_tab_ = (modal_kind == 9) ? 1 : 0;
    if (modal_kind == 6) update_manual_ = true;
    if (modal_kind == 6) update_checker_.qa_force(update::State::Available, update::Release{"v14.2.0", std::string(kReleasesPagePrefix) + "tag/v14.2.0"});
    if (modal_kind == 10) {   // seletor de processo com uma lista fixa
        qa_fake_procs_ = true;
        proc_list_.clear();
        auto add = [&](uint32_t pid, const char* exe, const char* title, bool win, bool rec, bool is64 = true) {
            ProcessInfo p; p.pid = pid; p.exe = exe; p.title = title; p.has_window = win; p.recognized = rec; p.is_64bit = is64;
            proc_list_.push_back(std::move(p));
        };
        add(31892, "PES2021.exe", "eFootball PES 2021", true, true);
        add(18420, "FL_2027.exe", "Football Life 2027", true, true);
        add(5120, "chrome.exe", "Google Chrome", true, false);
        add(7344, "Discord.exe", "Discord", true, false);
        add(9020, "explorer.exe", "Explorador de Arquivos", true, false);
        add(2210, "notepad.exe", "Sem título - Bloco de Notas", true, false);
        add(4408, "game32.exe", "Jogo antigo", true, false, false);
        proc_selected_pid_ = 18420;
        proc_selected_exe_ = "FL_2027.exe";
        open_proc_picker_ = true;
    }
    if (modal_kind == 8) {   // trava ativa no atleta aberto
        st_.val_lock_active = true;
        st_.val_lock.stamina = true; st_.val_lock.stamina_value = 100;
        st_.val_lock.arrow = true; st_.val_lock.arrow_value = 4;
        st_.val_lock.injury = true;
        st_.val_lock_addr = player_.addr;
        st_.val_lock_name = player_.name;
    }
    if (modal_kind == 7) {   // jogo desconectado: mostra o cartao de primeiros passos
        st_.attached = false; st_.career = 0; st_.has_player = false; st_.access_denied = false;
    }
    if (modal_kind == 2) {
        json_is_export_ = true;
        json_title_ = "Exportar JSON - G. DE ARRASCAETA";
        json_buf_ = "{\n  \"name\": \"G. DE ARRASCAETA\",\n  \"ovr\": 84,\n  \"age\": 32,\n  \"market_raw\": 180000,\n  \"salary_raw\": 39100\n}";
    } else if (modal_kind == 3) {
        pack_tab_ = 0;
        pack_scope_ = PackMatchScope::UserSquad;
        pack_library_ = {
            {"C:\\Docs\\Flamengo_2026_Atualizado.json", "Flamengo_2026_Atualizado.json", "Flamengo 2026 Atualizado", "FLAMENGO [FLA]", "2026-11-23 15:30:00", "squad", 28, 81.4},
            {"C:\\Docs\\Arrascaeta_Prime.json", "Arrascaeta_Prime.json", "Arrascaeta Prime", "FLAMENGO [FLA]", "2026-11-22 20:10:00", "player", 1, 88.0},
        };
        pack_lib_selected_ = 0;
        pack_preview_ = {};
        pack_preview_.ok = true;
        pack_preview_.code = "ok";
        pack_preview_.scope = PackMatchScope::UserSquad;
        pack_preview_.matched_pid = 4;
        pack_preview_.matched_name = 1;
        pack_preview_.unmatched = 1;
        pack_preview_.rows = {
            {0, true, PackMatchKind::ExactPid, 0x101, 63497, "G. DE ARRASCAETA", "FLAMENGO [FLA]", 83, 86, 8, 8, 6, 8, 140000, 260000, 32800, 52000, 14},
            {1, true, PackMatchKind::ExactPid, 0x102, 114200, "PEDRO", "FLAMENGO [FLA]", 83, 85, 12, 12, 5, 7, 240000, 340000, 33800, 44000, 10},
            {2, true, PackMatchKind::ExactPid, 0x103, 118500, "LUCAS PAQUETÁ", "FLAMENGO [FLA]", 83, 85, 8, 8, 7, 9, 235000, 335000, 33800, 44000, 9},
            {3, true, PackMatchKind::ExactPid, 0x104, 132001, "LÉO ORTIZ", "FLAMENGO [FLA]", 80, 82, 1, 1, 4, 6, 85000, 145000, 20400, 28500, 11},
            {4, true, PackMatchKind::ExactName, 0x105, 141100, "LORRAN", "FLAMENGO [FLA]", 75, 79, 8, 8, 2, 5, 47500, 165000, 6100, 13500, 18},
            {5, false, PackMatchKind::None, 0, 0, "GABIGOL", "CRUZEIRO", 0, 82, 12, 12, 0, 6, 0, 0, 0, 0, 0},
        };
    }
}
#endif

// ---------------------------------------------------------------------------------------------
// Servicos opcionais (v14.1): aviso de versao, retencao de backups, log local e guia de primeiros passos
// ---------------------------------------------------------------------------------------------

void WorkbenchApp::start_services_once() {
    if (services_started_) return;
    services_started_ = true;
    actionlog::write("INFO", std::string("Lettuce Live Editor v") + kAppVersion + " started");
    if (settings_.check_updates) start_update_check();
}

void WorkbenchApp::start_update_check() {
    void* h = hwnd_;
    update_checker_.start(kAppVersion, [h]() {
        if (h) PostMessageW(static_cast<HWND>(h), kWmStatusUpdated, 0, 0);
    });
}

void WorkbenchApp::apply_backup_retention() {
    const int idx = std::clamp(settings_.backup_keep_idx, 0, backups::kKeepOptionCount - 1);
    engine_.set_backup_keep(backups::kKeepOptions[idx]);
}

void WorkbenchApp::refresh_backup_info() {
    const auto entries = backups::list(std::filesystem::path(utf8_to_wide(get_backups_directory())));
    backup_info_.count = static_cast<int>(entries.size());
    backup_info_.bytes = backups::total_bytes(entries);
}

void WorkbenchApp::render_update_banner(const FontBank& fonts) {
    const update::Snapshot snap = update_checker_.snapshot();
    if (snap.state != update::State::Available) return;
    if (!settings_.check_updates && !update_manual_) return;   // opcao desligada depois de uma consulta: some o aviso
    if (snap.release.tag == settings_.update_dismissed) return;
    Lang L = settings_.lang;
    ImFont* f_bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.13f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(col::Accent.x, col::Accent.y, col::Accent.z, 0.55f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(std::round(16.0f * dpi_scale_), std::round(10.0f * dpi_scale_)));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, std::round(10.0f * dpi_scale_));
    ImGui::BeginChild("##update_banner", ImVec2(0, 0),
                      ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
    if (f_bold) ImGui::PushFont(f_bold);
    ImGui::TextColored(col::AccentSoft, "%s %s", T(L, "Nova versão disponível:", "New version available:"), snap.release.tag.c_str());
    if (f_bold) ImGui::PopFont();
    ImGui::TextColored(col::Subtext, "%s v%s", T(L, "Você está na versão", "You are on"), kAppVersion);
    ImGui::Spacing();
    if (btn_primary(T(L, "Abrir página da versão##upd_open", "Open release page##upd_open")) && update::is_trusted_release_url(snap.release.url)) {
        const std::wstring w = utf8_to_wide(snap.release.url);
        ShellExecuteW(static_cast<HWND>(hwnd_), L"open", w.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
    ImGui::SameLine(0, std::round(8.0f * dpi_scale_));
    if (ImGui::Button(T(L, "Dispensar esta versão##upd_dismiss", "Dismiss this version##upd_dismiss"))) {
        settings_.update_dismissed = snap.release.tag;
        save_settings(settings_);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    ImGui::Dummy(ImVec2(0, std::round(10.0f * dpi_scale_)));
}

void WorkbenchApp::render_prefs_services() {
    Lang L = settings_.lang;
    const float wrap_w = 330.0f * dpi_scale_;
    const float gap = std::round(6.0f * dpi_scale_);
    const float bh = std::round(30.0f * dpi_scale_);

    // ---- Conexao com o jogo (seletor de processo)
    ImGui::Spacing();
    ImGui::TextColored(col::Subtext, "%s", T(L, "Conexão com o jogo", "Game connection"));
    {
        std::string cur = settings_.game_process.empty()
            ? std::string(T(L, "Automática (PES 2021 e Football Life)", "Automatic (PES 2021 and Football Life)"))
            : std::string(T(L, "Processo escolhido: ", "Chosen process: ")) + settings_.game_process;
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrap_w);
        wrapped_colored(col::Muted, "%s", cur.c_str());
        ImGui::PopTextWrapPos();
        if (ImGui::Button(T(L, "Escolher processo...##proc_open", "Choose process...##proc_open"))) {
            open_proc_picker_ = true;
            ImGui::CloseCurrentPopup();
        }
        if (!settings_.game_process.empty()) {
            ImGui::SameLine(0, gap);
            if (ImGui::Button(T(L, "Automática##proc_auto", "Automatic##proc_auto"))) apply_process_target(std::string(), 0);
        }
    }

    // ---- Atualizacoes
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextColored(col::Subtext, "%s", T(L, "Atualizações", "Updates"));
    bool upd_v = settings_.check_updates;
    if (toggle_switch("##sw_updates", T(L, "Avisar quando houver nova versão", "Notify me about new versions"), &upd_v)) {
        settings_.check_updates = upd_v;
        save_settings(settings_);
        if (upd_v) start_update_check();
    }
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrap_w);
    wrapped_colored(col::Muted, "%s", T(L,
        "Consulta o GitHub uma vez ao abrir o app. Não envia nenhum dado seu e não baixa nada: só mostra o link da versão. Desligado por padrão.",
        "Checks GitHub once when the app opens. It sends no data about you and downloads nothing: it only shows the release link. Off by default."));
    ImGui::PopTextWrapPos();
    {
        const update::Snapshot snap = update_checker_.snapshot();
        const bool busy = (snap.state == update::State::Checking);
        ImGui::BeginDisabled(busy);
        if (ImGui::Button(T(L, "Verificar agora##upd_now", "Check now##upd_now"))) { update_manual_ = true; start_update_check(); }
        ImGui::EndDisabled();
        ImGui::SameLine(0, gap);
        switch (snap.state) {
            case update::State::Checking:
                ImGui::TextColored(col::Subtext, "%s", T(L, "Verificando...", "Checking...")); break;
            case update::State::UpToDate:
                ImGui::TextColored(col::GreenSoft, "%s (v%s)", T(L, "Você está na versão mais recente", "You are on the latest version"), kAppVersion); break;
            case update::State::Available:
                ImGui::TextColored(col::AccentSoft, "%s %s", T(L, "Disponível:", "Available:"), snap.release.tag.c_str()); break;
            case update::State::Failed:
                ImGui::TextColored(col::GoldSoft, "%s", T(L, "Não foi possível verificar agora.", "Could not check right now.")); break;
            default:
                ImGui::TextColored(col::Muted, "%s v%s", T(L, "Versão atual:", "Current version:"), kAppVersion); break;
        }
    }

    // ---- Backups do elenco
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextColored(col::Subtext, "%s", T(L, "Backups do elenco", "Squad backups"));
    {
        const float bw = std::max(40.0f * dpi_scale_, (ImGui::GetContentRegionAvail().x - gap * static_cast<float>(backups::kKeepOptionCount - 1)) / static_cast<float>(backups::kKeepOptionCount));
        for (int i = 0; i < backups::kKeepOptionCount; ++i) {
            if (i > 0) ImGui::SameLine(0, gap);
            char lbl[48];
            if (backups::kKeepOptions[i] == 0) std::snprintf(lbl, sizeof(lbl), "%s##bk%d", T(L, "Todos", "All"), i);
            else std::snprintf(lbl, sizeof(lbl), "%d##bk%d", backups::kKeepOptions[i], i);
            const bool act = (settings_.backup_keep_idx == i);
            if (styled_button(lbl, act ? col::Accent : col::BgCard, act ? col::AccentSoft : col::Border, col::Text, ImVec2(bw, bh)) && !act) {
                settings_.backup_keep_idx = i;
                save_settings(settings_);
                apply_backup_retention();
            }
        }
    }
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrap_w);
    wrapped_colored(col::Muted, "%s", T(L,
        "Quantos backups JSON manter. Os mais antigos só são apagados depois que um novo backup é salvo, e nunca o novo. Padrão: todos.",
        "How many JSON backups to keep. The oldest are only deleted after a new backup is saved, and never the new one. Default: all."));
    ImGui::PopTextWrapPos();
    {
        char info[96];
        const double mb = static_cast<double>(backup_info_.bytes) / (1024.0 * 1024.0);
        std::snprintf(info, sizeof(info), "%d %s, %.1f MB", backup_info_.count, T(L, "backups em disco", "backups on disk"), mb);
        ImGui::TextColored(col::Subtext, "%s", info);
        ImGui::SameLine(0, gap);
        if (ImGui::Button(T(L, "Abrir pasta##bk_open", "Open folder##bk_open"))) {
            const std::wstring w = utf8_to_wide(get_backups_directory());
            std::error_code ec;
            std::filesystem::create_directories(std::filesystem::path(w), ec);
            ShellExecuteW(static_cast<HWND>(hwnd_), L"open", w.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    }

    // ---- Registro local
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextColored(col::Subtext, "%s", T(L, "Registro de ações", "Action log"));
    bool log_v = settings_.action_log;
    if (toggle_switch("##sw_actionlog", T(L, "Salvar registro local das ações", "Save a local action log"), &log_v)) {
        settings_.action_log = log_v;
        actionlog::set_enabled(log_v);
        save_settings(settings_);
    }
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrap_w);
    wrapped_colored(col::Muted, "%s", T(L,
        "Grava no seu computador, em Documentos\\Lettuce Live Editor\\Logs, os avisos exibidos para ajudar a reproduzir problemas. Nada é enviado.",
        "Written on your computer, in Documents\\Lettuce Live Editor\\Logs, it keeps the notices shown to help reproduce problems. Nothing is sent."));
    ImGui::PopTextWrapPos();
    if (ImGui::Button(T(L, "Abrir pasta##log_open", "Open folder##log_open"))) {
        const std::wstring w = utf8_to_wide(get_logs_directory());
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(w), ec);
        ShellExecuteW(static_cast<HWND>(hwnd_), L"open", w.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    // ---- Diagnostico
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextColored(col::Subtext, "%s", T(L, "Diagnóstico", "Diagnostics"));
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrap_w);
    wrapped_colored(col::Muted, "%s", T(L,
        "Se algo não funcionar (por exemplo em outro jogo ou versão), copie o relatório e cole ao pedir ajuda, junto com o arquivo workbench.log da pasta de registros.",
        "If something does not work (for example on another game or version), copy the report and paste it when asking for help, together with workbench.log from the logs folder."));
    ImGui::PopTextWrapPos();
    if (ImGui::Button(T(L, "Copiar relatório##diag_copy", "Copy report##diag_copy"))) {
        const std::string rep = std::string("Lettuce Live Editor v") + kAppVersion + "\r\n" + engine_.diagnostics_report();
        ImGui::SetClipboardText(rep.c_str());
        toasts_.push(T(L, "Relatório copiado!", "Report copied!"));
    }
}

void WorkbenchApp::render_connection_hint(const FontBank& fonts) {
    (void)fonts;
    Lang L = settings_.lang;
    const bool career_ok = st_.attached && st_.career > 0;
    // Executavel com assinaturas que nao casaram: parte dos recursos fica desligada por seguranca e o usuario precisa saber
    const bool compat_warn = st_.attached && !st_.access_denied && st_.compat_missing != 0;
    // Conectado e com carreira: volta a descricao normal da tela. A dica ocupa a mesma linha, entao nada se desloca.
    if (career_ok && !st_.access_denied && !compat_warn) {
        wrapped_colored(col::Subtext, "%s", T(L, "Controle nativo em tempo real da Master League, Rumo ao Estrelato e Partidas sem Cheat Engine.",
                                                 "Real-time native control of Master League, Become a Legend, and Matches without Cheat Engine."));
        return;
    }
    std::string msg;
    ImVec4 tone = col::AccentSoft;
    bool can_pick = !st_.attached;
    if (st_.access_denied) {
        msg = T(L, "O jogo está como administrador. Abra este app também como administrador.", "The game is elevated. Open this app as Administrator too.");
        tone = col::GoldSoft;
        can_pick = false;
    } else if (st_.target_unsupported) {
        msg = T(L, "O processo escolhido não parece ser um jogo compatível. Nada foi gravado nele.", "The chosen process does not look like a compatible game. Nothing was written to it.");
        tone = col::GoldSoft;
    } else if (!st_.attached) {
        msg = st_.target_exe.empty()
            ? std::string(T(L, "Aguardando o PES 2021 ou o Football Life. A conexão é automática.", "Waiting for PES 2021 or Football Life. The connection is automatic."))
            : std::string(T(L, "Procurando o processo escolhido: ", "Looking for the chosen process: ")) + st_.target_exe;
        tone = col::Subtext;
    } else if (compat_warn) {
        msg = T(L, "Executável diferente: recursos incompatíveis desligados. Veja Serviços > Diagnóstico.",
                   "Different executable: unsupported features are off. See Services > Diagnostics.");
        tone = col::GoldSoft;
    } else {
        msg = T(L, "Jogo conectado. Entre na Master Liga ou no Rumo ao Estrelato para liberar o resto.", "Game connected. Enter Master League or Become a Legend to unlock the rest.");
        tone = col::GreenSoft;
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float lh = ImGui::GetTextLineHeight();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    draw_led_dot(dl, ImVec2(p.x + 4.0f * dpi_scale_, p.y + lh * 0.5f), 3.4f * dpi_scale_, tone, !st_.attached);
    ImGui::Dummy(ImVec2(14.0f * dpi_scale_, lh));
    ImGui::SameLine(0, 0);
    ImGui::TextColored(tone, "%s", msg.c_str());
    if (can_pick) {
        ImGui::SameLine(0, 12.0f * dpi_scale_);
        if (ImGui::SmallButton(T(L, "Escolher processo...##hint_proc", "Choose process...##hint_proc"))) open_proc_picker_ = true;
    }
}

void WorkbenchApp::render_value_lock_card(const FontBank& fonts) {
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];
    const bool lock_on = st_.val_lock_active;
    const bool lock_here = lock_on && player_.addr != 0 && st_.val_lock_addr == player_.addr;

    begin_studio_card("##value_lock", 0.0f, false, lock_on ? &col::Gold : nullptr);
    card_header(T(L, "Travar valores", "Lock values"), bold,
        T(L, "Mantém o fôlego, a seta de forma e a lesão do atleta nos valores escolhidos, regravando a cada ciclo. Vale para um atleta por vez.",
             "Keeps the player's stamina, form arrow and injury at the chosen values, rewriting them every cycle. Works for one player at a time."));
    ImGui::SameLine(0, 10.0f * dpi_scale_);
    if (lock_on) draw_pill(T(L, "ATIVO", "ACTIVE"), col::GoldSoft, ImVec4(col::Gold.x, col::Gold.y, col::Gold.z, 0.16f), ImVec4(col::Gold.x, col::Gold.y, col::Gold.z, 0.5f), true);
    else draw_pill(T(L, "Desligado", "Off"), col::Subtext, ImVec4(col::Subtext.x, col::Subtext.y, col::Subtext.z, 0.12f), col::Border, true);
    ImGui::Spacing();

    // Os controles mostram o que foi enviado por ultimo para este atleta (o retorno do poller demora um ciclo e
    // faria o interruptor piscar). Sem envio recente, seguem o motor. Em outro atleta, tudo comeca desligado.
    ValueLockConfig cfg;
    if (lock_ui_addr_ != 0 && lock_ui_addr_ == player_.addr) cfg = lock_ui_;
    else if (lock_here) cfg = st_.val_lock;
    else { cfg = lock_ui_; cfg.stamina = cfg.arrow = cfg.injury = false; }
    bool changed = false;
    bool toggled = false;

    if (lock_on && !lock_here) {
        ImGui::PushTextWrapPos(0.0f);
        const std::string lock_msg = std::string(T(L, "Trava ativa em: ", "Lock active on: ")) + st_.val_lock_name + ". " +
                                     T(L, "Ligar uma trava aqui a move para este atleta.", "Turning a lock on here moves it to this player.");
        wrapped_colored(col::GoldSoft, "%s", lock_msg.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
    }

    const float ctrl_w = std::max(160.0f * dpi_scale_, ImGui::GetContentRegionAvail().x - 24.0f * dpi_scale_);
    {
        bool v = cfg.stamina;
        if (toggle_switch("##lk_stam", T(L, "Manter fôlego", "Keep stamina"), &v)) { cfg.stamina = v; changed = toggled = true; }
        ImGui::BeginDisabled(!cfg.stamina);
        ImGui::SetNextItemWidth(ctrl_w);
        if (ImGui::SliderInt("##lk_stam_v", &cfg.stamina_value, 0, 100, "%d%%")) changed = true;
        ImGui::EndDisabled();
        ImGui::Spacing();
    }
    {
        bool v = cfg.arrow;
        if (toggle_switch("##lk_arrow", T(L, "Manter seta de forma", "Keep form arrow"), &v)) { cfg.arrow = v; changed = toggled = true; }
        ImGui::BeginDisabled(!cfg.arrow);
        const int picked = draw_arrow_picker("##lk_apick", cfg.arrow_value, L, ctrl_w, bold);
        ImGui::EndDisabled();
        if (cfg.arrow && picked >= 0 && picked != cfg.arrow_value) { cfg.arrow_value = picked; changed = true; }
        ImGui::Spacing();
    }
    {
        bool v = cfg.injury;
        if (toggle_switch("##lk_inj", T(L, "Manter sem lesão", "Keep injury-free"), &v)) { cfg.injury = v; changed = toggled = true; }
    }

    if (changed) {
        lock_ui_ = cfg;
        lock_ui_addr_ = player_.addr;
        auto r = engine_.set_value_lock(cfg, player_.addr);
        if (r.ok) {
            wake_poller();
            if (toggled) toasts_.push(cfg.any() ? T(L, "Trava aplicada!", "Lock applied!") : T(L, "Travas desligadas.", "Locks turned off."));
        } else {
            lock_ui_ = ValueLockConfig{};   // nao deixa a tela marcada se o motor recusou
            lock_ui_addr_ = 0;
            toast_code(r.code, true);
        }
    }

    ImGui::Spacing();
    if (lock_on) {
        if (ImGui::Button(T(L, "Desligar todas as travas##lk_off", "Turn off all locks##lk_off"))) {
            engine_.clear_value_lock();
            lock_ui_ = ValueLockConfig{};
            lock_ui_addr_ = 0;
            wake_poller();
            toasts_.push(T(L, "Travas desligadas.", "Locks turned off."));
        }
        ImGui::Spacing();
    }
    ImGui::PushTextWrapPos(0.0f);
    wrapped_colored(col::Muted, "%s", T(L,
        "A trava se desliga sozinha se o jogo fechar ou se o atleta travado mudar. Ela não entra no histórico de desfazer.",
        "The lock turns itself off if the game closes or the locked player changes. It is not part of the undo history."));
    ImGui::PopTextWrapPos();
    end_studio_card();
}

// ---------------------------------------------------------------------------------------------
// Seletor de processo: escolher qualquer processo em execucao para anexar, como o "Open Process" do Cheat Engine
// ---------------------------------------------------------------------------------------------

void WorkbenchApp::refresh_process_list() {
#ifdef PES_QA_DUMP
    if (qa_fake_procs_) return;   // captura de tela: lista fixa, sem expor os processos da maquina de teste
#endif
    proc_list_ = list_processes();
    bool still_there = false;
    for (const auto& p : proc_list_) {
        if (p.pid == proc_selected_pid_) { still_there = true; break; }
    }
    if (!still_there) {
        proc_selected_pid_ = 0;
        proc_selected_exe_.clear();
    }
}

void WorkbenchApp::apply_process_target(const std::string& exe, uint32_t pid) {
    Lang L = settings_.lang;
    settings_.game_process = exe;
    save_settings(settings_);
    engine_.set_target_process(exe, pid);
    wake_poller();
    actionlog::write("INFO", exe.empty() ? std::string("Process target: automatic")
                                         : std::string("Process target: ") + exe + " (pid " + std::to_string(pid) + ")");
    toasts_.push(exe.empty() ? std::string(T(L, "Detecção automática ativada.", "Automatic detection on."))
                             : std::string(T(L, "Processo escolhido: ", "Chosen process: ")) + exe);
}

namespace {
std::string ascii_lower(const std::string& s) {
    std::string o = s;
    for (char& c : o) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return o;
}
} // namespace

void WorkbenchApp::render_process_picker(const FontBank& fonts) {
    if (open_proc_picker_) {
        refresh_process_list();
        ImGui::OpenPopup("##ProcPicker");
        open_proc_picker_ = false;
    }
    Lang L = settings_.lang;
    ImFont* bold = fonts.bold[std::clamp(settings_.font_idx, 0, fonts.count - 1)];
    ImGuiViewport* vp = ImGui::GetMainViewport();
    const float w = std::min(780.0f * dpi_scale_, vp->Size.x - 40.0f);
    const float h = std::min(600.0f * dpi_scale_, vp->Size.y - 40.0f);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(std::round(20.0f * dpi_scale_), std::round(16.0f * dpi_scale_)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, std::round(14.0f * dpi_scale_));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, col::BgCard);
    ImGui::PushStyleColor(ImGuiCol_Border, col::BorderLit);
    if (ImGui::BeginPopupModal("##ProcPicker", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        if (bold) ImGui::PushFont(bold);
        ImGui::TextColored(col::Text, "%s", T(L, "Escolher processo do jogo", "Choose the game process"));
        if (bold) ImGui::PopFont();
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(col::Muted, "%s", T(L,
            "Escolha o processo ao qual o app deve se conectar. Os jogos conhecidos (PES 2021 e Football Life) aparecem primeiro. A escolha fica salva até você voltar à detecção automática.",
            "Pick the process the app should connect to. Known games (PES 2021 and Football Life) are listed first. The choice is saved until you go back to automatic detection."));
        ImGui::TextColored(col::GoldSoft, "%s", T(L,
            "Use só em jogos offline. O app grava na memória do processo escolhido, mas só em processos compatíveis com o PES 2021: em qualquer outro nada é gravado.",
            "Use it in offline games only. The app writes to the chosen process's memory, but only in processes compatible with PES 2021: in any other nothing is written."));
        ImGui::PopTextWrapPos();
        ImGui::Spacing();

        // Filtro, "so com janela" e atualizar
        const float btn_w = calc_btn_w(T(L, "Atualizar##proc_ref", "Refresh##proc_ref"));
        ImGui::SetNextItemWidth(std::max(140.0f * dpi_scale_, ImGui::GetContentRegionAvail().x - btn_w - 220.0f * dpi_scale_));
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        ImGui::InputTextWithHint("##proc_flt", T(L, "Filtrar por nome, janela ou PID...", "Filter by name, window or PID..."), proc_filter_, sizeof(proc_filter_));
        ImGui::SameLine(0, 10.0f * dpi_scale_);
        toggle_switch("##proc_win", T(L, "Só com janela", "Windowed only"), &proc_only_windows_);
        ImGui::SameLine(0, 10.0f * dpi_scale_);
        if (ImGui::Button(T(L, "Atualizar##proc_ref", "Refresh##proc_ref"))) refresh_process_list();
        ImGui::Spacing();

        const std::string flt = ascii_lower(proc_filter_);
        const float footer_h = std::round(78.0f * dpi_scale_);
        const ImGuiTableFlags tf = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingFixedFit;
        int shown = 0;
        if (ImGui::BeginTable("##proc_tbl", 3, tf, ImVec2(0, ImGui::GetContentRegionAvail().y - footer_h))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn(T(L, "PROCESSO", "PROCESS"), ImGuiTableColumnFlags_WidthFixed, std::round(250.0f * dpi_scale_));
            ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, std::round(70.0f * dpi_scale_));
            ImGui::TableSetupColumn(T(L, "JANELA", "WINDOW"), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            for (const auto& p : proc_list_) {
                if (proc_only_windows_ && !p.has_window && !p.recognized) continue;
                if (!flt.empty()) {
                    const std::string hay = ascii_lower(p.exe) + " " + ascii_lower(p.title) + " " + std::to_string(p.pid);
                    if (hay.find(flt) == std::string::npos) continue;
                }
                ++shown;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                const bool sel = (p.pid == proc_selected_pid_);
                char rid[48];
                std::snprintf(rid, sizeof(rid), "##prow%u", p.pid);
                const ImVec2 row_min = ImGui::GetCursorScreenPos();
                ImGui::BeginDisabled(!p.is_64bit);
                if (ImGui::Selectable(rid, sel, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
                    proc_selected_pid_ = p.pid;
                    proc_selected_exe_ = p.exe;
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        apply_process_target(p.exe, p.pid);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndDisabled();
                {
                    // Nome e selo desenhados por cima da linha selecionavel (a linha ocupa a largura toda)
                    ImDrawList* rdl = ImGui::GetWindowDrawList();
                    const ImVec4 name_col = p.recognized ? col::GreenSoft : (p.is_64bit ? col::Text : col::Muted);
                    rdl->AddText(row_min, ImGui::GetColorU32(name_col), p.exe.c_str());
                    const char* badge = p.recognized ? T(L, "conhecido", "known") : (!p.is_64bit ? "32-bit" : nullptr);
                    if (badge) {
                        const float nx = row_min.x + ImGui::CalcTextSize(p.exe.c_str()).x + 8.0f * dpi_scale_;
                        rdl->AddText(ImVec2(nx, row_min.y), ImGui::GetColorU32(p.recognized ? col::AccentSoft : col::Muted), badge);
                    }
                }
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(col::Subtext, "%u", p.pid);
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(col::Subtext, "%s", p.title.c_str());
            }
            ImGui::EndTable();
        }
        if (shown == 0) {
            ImGui::TextColored(col::Muted, "%s", T(L, "Nenhum processo encontrado com esse filtro.", "No process matches this filter."));
        }

        ImGui::Spacing();
        {
            const std::string cur = settings_.game_process.empty()
                ? std::string(T(L, "Conexão atual: automática", "Current connection: automatic"))
                : std::string(T(L, "Conexão atual: ", "Current connection: ")) + settings_.game_process;
            ImGui::TextColored(col::Muted, "%s", cur.c_str());
        }
        ImGui::BeginDisabled(proc_selected_pid_ == 0);
        const std::string attach_lbl = std::string(T(L, "Anexar a este processo", "Attach to this process")) + "##proc_go";
        if (btn_primary(attach_lbl.c_str())) {
            apply_process_target(proc_selected_exe_, proc_selected_pid_);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, 8.0f * dpi_scale_);
        if (ImGui::Button(T(L, "Detecção automática##proc_auto2", "Automatic detection##proc_auto2"))) {
            apply_process_target(std::string(), 0);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine(0, 8.0f * dpi_scale_);
        if (ImGui::Button(T(L, "Fechar##proc_close", "Close##proc_close")) || ImGui::IsKeyPressed(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

} // namespace pes::ui
