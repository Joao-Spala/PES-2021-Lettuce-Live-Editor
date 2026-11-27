#pragma once
// Studio Pro Native Dear ImGui UI Controller for PES 2021 Workbench
// Custom frameless titlebar, responsive multi-scale layout, 8 Views + 5 Player Subtabs.

#include "i18n.hpp"
#include "theme.hpp"
#include "widgets.hpp"
#include "icons.hpp"
#include "../engine/pes_engine.hpp"
#include "../engine/update_check.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pes::ui {

enum class View : int {
    Dashboard = 0,
    Squad = 1,
    Finances = 2,
    Player = 3,
    Search = 4,
    Scouts = 5,
    Match = 6,
    Training = 7,
};

enum class PlayerTab : int {
    Bio = 0,
    Stats = 1,
    PosLab = 2,
    Skills = 3,
    Role = 4,
};

struct HitRect {
    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;
};

class WorkbenchApp {
public:
    explicit WorkbenchApp(PESEngine& engine);
    ~WorkbenchApp();

    void set_window_handle(void* hwnd_raw, float dpi_scale);
    void start_background_poller(void* hwnd_raw);
    void stop_background_poller();
    void wake_poller();
    void set_minimized(bool minimized) { minimized_.store(minimized, std::memory_order_relaxed); }

    // Used by Win32 WM_NCHITTEST to enable native window dragging on empty titlebar areas
    bool is_point_in_titlebar_caption(int client_x, int client_y) const;

    // Render one frame; returns true if animation/toast/drag requires continuous redraw.
    bool render_frame(const FontBank& fonts);

    // Preferencias: true uma vez quando escala de UI ou tema de destaque mudou (main reconstroi fontes/estilo)
    bool take_theme_rebuild() { bool r = theme_rebuild_; theme_rebuild_ = false; return r; }
    const AppSettings& settings() const { return settings_; }

    void trigger_back();
    void trigger_undo();
    void open_command_palette();

#ifdef PES_QA_DUMP
    void qa_populate_fixture();
    void qa_select_screen(int view_idx, int ptab_idx, int modal_kind = 0);
#endif

private:
    const char* view_label(View v) const;
    void switch_view(View v, bool skip_auto_load = false, bool from_back = false);
    void open_player_in_studio(uint64_t addr);
    void reload_player(uint64_t addr = 0);
    void sync_player_form_buffers();
    void reload_squad();
    void reload_finances();
    void reload_scouts();
    uint8_t scout_mask_of(uint64_t addr) const;
    void scout_report(const ActionResult& r, const char* verb_pt, const char* verb_en, int target_slot);
    void render_scout_list_menu(const char* popup_id, uint64_t addr);
    void run_search();
    void run_squad_cmd(const char* mode);
    void run_squad_cmd_sel(const char* mode);
    void run_squad_cmd_impl(const char* mode, const std::vector<uint64_t>* only);
    void apply_preset_cmd(bool to_selection);
    void render_history_popup(const FontBank& fonts);
    void render_preset_controls(const FontBank& fonts, bool compact);
    void toast_code(const std::string& code, bool is_err, int orig = 0, int fin = 0, int pri = 0, int pos = -1, int reg_pos = -1, bool needs_grade = false);
    std::string fmt_econ_delta_line(const EconSyncDelta& ed) const;
    void record_solver_action(const PlayerDetails& r, const char* tag_pt, const char* tag_en);
    static const char* log_tag(Lang L, const std::string& pt, const std::string& en, const std::string& es);
    void record_meta_action(int pos_id, int orig_ovr, int final_ovr, const std::string& note_pt, const std::string& note_en, const std::string& note_es = std::string());
    void render_poslab_log_card(const FontBank& fonts, float max_h);

    struct PosLabLogEntry {
        std::string time_str;
        int pos_id = -1;
        int reg_pos = -1;
        int orig_ovr = 0;
        int final_ovr = 0;
        int pri_ovr = 0;
        std::string tag_pt;
        std::string tag_en;
        std::vector<StatDiff> diffs;
        std::string tag_es;   // opcional (no fim para nao quebrar a inicializacao posicional): textos montados em tempo de execucao
    };

    struct LastEditSummary {
        bool active = false;
        uint64_t player_addr = 0;
        std::string time_str;
        std::string label_pt;
        std::string label_en;
        int old_ovr = 0;
        int new_ovr = 0;
        EconSyncDelta econ;
        std::vector<StatDiff> diffs;
    };

    void capture_last_edit(const PlayerDetails& r, const char* lbl_pt, const char* lbl_en);

    void render_titlebar(const FontBank& fonts, float tb_h);
    void render_sidebar(const FontBank& fonts, bool compact, bool auto_compact);
    void render_topbar(const FontBank& fonts);
    void render_view_dashboard(const FontBank& fonts);
    void render_view_squad(const FontBank& fonts);
    void render_view_finances(const FontBank& fonts);
    void render_view_player(const FontBank& fonts);
    void render_ptab_bio(const FontBank& fonts);
    void render_ptab_stats(const FontBank& fonts);
    void render_ptab_poslab(const FontBank& fonts);
    void render_ptab_skills(const FontBank& fonts);
    void render_ptab_role(const FontBank& fonts);
    void render_view_search(const FontBank& fonts);
    void render_view_scouts(const FontBank& fonts);
    void render_view_match(const FontBank& fonts);
    void render_view_training(const FontBank& fonts);
    void render_modals(const FontBank& fonts);
    void open_pack_studio(bool start_on_export, bool squad_mode);
    void refresh_pack_library();
    void run_pack_preview(int forced_idx = -1);
    void render_pack_studio_modal(const FontBank& fonts);

    PESEngine& engine_;
    AppSettings settings_;
    ToastManager toasts_;

    // Native window integration
    void* hwnd_ = nullptr;
    float dpi_scale_ = 1.0f;
    bool theme_rebuild_ = false;
    void render_prefs_popup(const FontBank& fonts);
    int prefs_tab_ = 0;   // 0 = Aparencia, 1 = Servicos

    // Servicos opcionais (v14.1): aviso de versao, retencao de backups, log local e guia de primeiros passos
    update::Checker update_checker_;
    bool services_started_ = false;
    bool update_manual_ = false;   // o usuario pediu "Verificar agora": o aviso aparece mesmo com a opcao desligada
    bool open_welcome_guide_ = false;
#ifdef PES_QA_DUMP
    bool qa_open_prefs_ = false;
#endif
    struct BackupInfo { int count = 0; uint64_t bytes = 0; };
    BackupInfo backup_info_;
    void start_services_once();
    void start_update_check();
    void apply_backup_retention();
    void refresh_backup_info();
    void render_prefs_services();
    void render_update_banner(const FontBank& fonts);
    void render_setup_checklist(const FontBank& fonts);
    void render_welcome_guide(const FontBank& fonts);
    void render_value_lock_card(const FontBank& fonts);
    ValueLockConfig lock_ui_;
    uint64_t lock_ui_addr_ = 0;   // atleta ao qual os interruptores de lock_ui_ se referem
    uint32_t last_lock_lost_ = 0;
    float titlebar_h_ = 38.0f;
    std::vector<HitRect> titlebar_interactive_;

    // Background polling thread with instant wake-on-focus
    std::atomic<bool> poller_running_{false};
    std::atomic<bool> minimized_{false};
    std::thread poller_thread_;
    std::mutex poll_cv_mtx_;
    std::condition_variable poll_cv_;
    bool poll_wake_flag_ = false;
    void* notify_hwnd_ = nullptr;
    std::mutex status_mtx_;
    StatusSnapshot latest_status_;
    uint64_t last_seen_seq_ = 0;

    // UI state
    StatusSnapshot st_;
    View current_view_ = View::Dashboard;
    std::vector<View> nav_history_;
    PlayerTab current_ptab_ = PlayerTab::Bio;

    uint64_t last_career_ = 0;
    uint64_t last_player_addr_ = 0;
    bool has_player_details_ = false;
    PlayerDetails player_;
    int highlighted_pos_ = -1;
    bool scroll_poslab_to_hl_ = false;
    int pitch_orient_mode_ = 0; // 0=Auto, 1=Horizontal, 2=Vertical
    int poslab_log_mode_ = 0;   // 0=Step-by-step, 1=Net Summary
    bool poslab_base_valid_ = false;
    std::array<int, 48> poslab_base_stats_{};
    std::vector<PosLabLogEntry> poslab_log_;
    LastEditSummary last_edit_;

    // Bio edit buffers (synced on player load)
    char bio_name_[64]{};
    char bio_shirt_[32]{};
    int bio_nat_ = 0;
    int bio_age_ = 20;
    int bio_height_ = 180;
    int bio_weight_ = 75;
    int bio_foot_ = 0;
    int bio_contract_ = 2026;
    char bio_salary_[32]{};
    char bio_market_[32]{};
    int bio_injury_ = 0;
    int bio_wfu_ = 2;
    int bio_wfa_ = 2;
    int bio_form_ = 5;
    int bio_injres_ = 2;
    bool bio_tlist_ = false;
    bool bio_llist_ = false;
    bool bio_has_rf_ = false;
    int bio_affection_ = 50;
    int quick_stam_ = 100;

    // Stats / DNA OVR state
    int target_ovr_inp_ = 85;
    char target_ovr_buf_[8]{"85"};
    char match_min_buf_[8]{"44"};
    char match_sec_buf_[8]{"00"};
    bool prev_in_match_ = false;
    StatRowState stat_row_st_;
    double all99_armed_until_ = 0.0;

    // Team Role & Skills buffers
    int edit_role_id_ = 0;
    int edit_role_lvl_ = 0;
    char skill_filter_[64]{};

    // Dashboard inline budget edit
    bool edit_inline_trf_ = false;
    char inline_trf_buf_[32]{};
    bool edit_inline_sal_ = false;
    char inline_sal_buf_[32]{};

    // Finances view
    FinancesSnapshot fin_;
    char fin_trf_buf_[32]{};
    char fin_sal_buf_[32]{};

    // Squad view
    SquadRoster squad_;
    int squad_filter_ = 0; // 0=all, 1=exp, 2=inj, 3=u23
    std::unordered_set<uint64_t> squad_sel_; // selecao em lote do Meu Elenco
    bool squad_reload_pending_ = false;
    int preset_idx_ = 0;
    bool preset_replace_ = false;
    std::vector<JournalRow> journal_cache_;
    uint64_t journal_rev_seen_ = ~0ULL;
    char squad_search_[64]{};
    int squad_sort_col_ = 0;
    bool squad_sort_asc_ = false;

    // Search view
    char srch_query_[64]{};
    int srch_pos_ = -1;
    int srch_min_ovr_ = 75;
    int srch_max_age_ = 45;
    bool srch_free_ = false;
    int srch_target_scout_ = 4;
    SearchResult search_res_;
    int srch_sort_col_ = 0;
    bool srch_sort_asc_ = false;

    // Scouts view
    ScoutListsResult scouts_;
    int active_scout_slot_ = 4;
    double clear_scout_armed_until_ = 0.0;
    std::unordered_map<uint64_t, uint8_t> scout_mask_;   // addr do atleta -> mascara de listas (bit = slot-1)
    std::unordered_set<uint64_t> scout_sel_;             // handles marcados na tela de Olheiros
    std::unordered_set<uint64_t> srch_sel_;              // addrs marcados na Busca
    char scout_filter_[64]{};
    int scout_pos_filter_ = 0;                           // 0 todos, 1 GO, 2 DEF, 3 MEI, 4 ATA
    bool scout_only_free_ = false;
    bool scout_only_exp_ = false;
    int scout_sort_col_ = 0;
    bool scout_sort_asc_ = false;
    int scout_move_target_ = 1;

    // Training / BAL
    int bal_focus_inp_ = 21;
    bool bal_lock_chk_ = false;
    bool bal_dirty_ = false;

    // Modals
    bool open_cmd_modal_ = false;
    char cmd_filter_[64]{};
    int cmd_selected_ = 0;

    bool open_json_modal_ = false;
    bool json_is_export_ = true;
    std::string json_title_;
    std::string json_buf_;

    // Squad / Player Pack Studio modal state
    bool open_pack_modal_ = false;
    int pack_tab_ = 0; // 0=Import & Match, 1=Export
    bool pack_export_is_squad_ = true;
    char pack_label_buf_[96]{};
    std::string pack_export_json_;
    int pack_export_count_ = 0;

    std::vector<PackLibraryItem> pack_library_;
    int pack_lib_selected_ = -1;
    std::string pack_import_raw_;
    PackMatchScope pack_scope_ = PackMatchScope::UserSquad;
    PackApplyOptions pack_opts_{};
    PackPreviewResult pack_preview_;
    int pack_forced_idx_ = -1;
    bool pack_show_paste_box_ = false;
    std::string pack_paste_buf_;
};

} // namespace pes::ui
