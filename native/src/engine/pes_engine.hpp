#pragma once
// Thread-safe Standalone PES 2021 Master League / BAL / Match / Player Engine.
// Refactored for minimal Win32 syscalls, zero idle RAM churn, and crash-safe teardown.

#include "win32_mem.hpp"
#include "hooks.hpp"
#include "ovr_engine.hpp"
#include "econ_model.hpp"
#include "../data/game_data.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pes {

// Accent-insensitive lowercase UTF-8 normalizer (ASCII fast-path)
std::string norm_text_utf8(std::string_view s);

// Parse human money ('25M', '500k', '1.5B', '2.712.400', currency symbols) -> raw Konami int (Euros / 100)
std::optional<int32_t> parse_human_money_to_raw(std::string_view text);

struct FinancesSnapshot {
    bool ok = false;
    std::string code;
    int32_t transfer_raw = 0;
    int32_t salary_avail_raw = 0;
    int32_t salary_gross_raw = 0;
    int32_t squad_wages_raw = 0;
};

struct PlayerSummary {
    uint64_t addr = 0;
    std::string name;
    int ovr = 40;
    int pos = 0;
    int age = 20;
    int stamina = 100;
    int arrow = 2;
    int injury = 0;
    std::string club;
    int undo_count = 0;
};

struct StatusSnapshot {
    bool attached = false;
    bool access_denied = false;
    bool conflict_other_wb = false;
    std::string exe;
    uint32_t pid = 0;
    std::string mode_code = "offline"; // offline, menu, ml, bal
    std::string club;
    uint64_t career = 0;
    bool in_match = false;
    bool has_player = false;
    PlayerSummary player;
    bool has_finances = false;
    FinancesSnapshot finances;
    std::array<bool, 8> hooks{};
    int training_mode = 0;
    int stamina_mode = 0;
    bool freeze_clock = false;
    bool match_clock_valid = false;
    int match_period = 0;
    int match_min = 0;
    int match_sec = 0;
    int bal_focus = 0;
    bool bal_lock = false;
    uint64_t seq = 0;
};

struct PositionEntry {
    int id = 0;
    int ovr = 40;
    int apt = 0;
    bool is_reg = false;
};

struct StatDiff {
    int fid = 0;
    int old_val = 40;
    int new_val = 40;
};

struct PlayerDetails {
    bool ok = false;
    std::string code;
    bool action_ok = true;
    int action_orig = 0;
    int action_final = 0;
    int action_pri = 0;
    int action_pos = -1;
    int action_reg_pos = -1;
    bool action_needs_grade = false;
    std::vector<StatDiff> action_diffs;

    uint64_t addr = 0;
    uint32_t pid = 0;
    std::string name;
    std::string shirt;
    std::string club;
    int ovr = 40;
    int reg_pos = 0;
    int nationality = 0;
    int age = 20;
    int height = 180;
    int weight = 75;
    int foot = 0;
    int playing_style = 0;
    int wf_usage = 2;
    int wf_acc = 2;
    int form_cons = 5;
    int inj_res = 2;
    int stamina = 100;
    int arrow = 2;
    int injury = 0;
    int contract_yr = 2026;
    int32_t salary_raw = 0;
    int32_t market_raw = 0;
    bool has_release_fee = false;
    int32_t release_fee_raw = 0;
    int affection = 50;
    int snap_age = 0;
    int snap_ovr = 0;
    int team_role = 0;
    int team_role_lvl = 0;
    bool transfer_listed = false;
    bool loan_listed = false;
    EconSyncDelta econ_delta{};

    std::array<int, 48> stats{}; // indexed by fid
    std::array<PositionEntry, 13> positions{};
    std::array<bool, 41> skills{};
    std::array<bool, 7> com_ai{};
    int undo_count = 0;
};

struct BioUpdate {
    std::optional<std::string> name;
    std::optional<std::string> shirt;
    std::optional<int> nationality;
    std::optional<int> age;
    std::optional<int> height;
    std::optional<int> weight;
    std::optional<int> foot;
    std::optional<int> contract_yr;
    std::optional<std::string> salary_str;
    std::optional<std::string> market_str;
    std::optional<int> injury;
    std::optional<int> wf_usage;
    std::optional<int> wf_acc;
    std::optional<int> form_cons;
    std::optional<int> inj_res;
    std::optional<bool> transfer_listed;
    std::optional<bool> loan_listed;
    std::optional<bool> has_release_fee;
    std::optional<int> affection;
};

struct SquadRow {
    uint64_t addr = 0;
    std::string name;
    std::string name_norm;
    int ovr = 40;
    int pos = 0;
    int age = 20;
    int exp_y = 0;
    bool is_expiring = false;
    int32_t salary_raw = 0;
    int stamina = 100;
    int arrow = 2;
    int injury = 0;
};

struct SquadRoster {
    bool ok = false;
    std::string code;
    std::string club;
    int cur_year = 2026;
    std::vector<SquadRow> players;
};

struct SearchRow {
    uint64_t addr = 0;
    std::string name;
    int ovr = 40;
    int pos = 0;
    int age = 20;
    std::string club;
    int32_t salary_raw = 0;
    int exp_y = 0;
};

struct SearchResult {
    bool ok = false;
    std::string code;
    int total = 0;
    double elapsed_ms = 0.0;
    std::vector<SearchRow> players;
};

struct ScoutEntry {
    int slot_idx = 0;
    uint64_t handle = 0;
    uint64_t addr = 0;
    std::string name;
    int ovr = 40;
    int pos = 0;
    int age = 20;
    std::string club;
};

struct ScoutListState {
    int slot = 4;
    int cap = 100;
    const char* pt = "";
    const char* en = "";
    std::vector<ScoutEntry> entries;
};

struct ScoutListsResult {
    bool ok = false;
    std::string code;
    std::vector<ScoutListState> lists;
};

struct ActionResult {
    bool ok = false;
    std::string code;
    int count = 0;
    int value = 0;
    bool flag = false;
    std::string text;
};

// Full structured Player / Squad Pack snapshot (JSON v13, cross-save matching)
struct PackPlayerData {
    uint32_t pid = 0;
    std::string name;
    std::string name_norm;
    std::string shirt;
    std::string club;
    int ovr = 40;
    int reg_pos = 0;
    int nationality = 0;
    int age = 20;
    int height = 180;
    int weight = 75;
    int foot = 0;
    int playing_style = 0;
    int wf_usage = 2;
    int wf_acc = 2;
    int form_cons = 5;
    int inj_res = 2;
    int stamina = 100;
    int arrow = 2;
    int injury = 0;
    int contract_yr = 2026;
    int32_t salary_raw = 0;
    int32_t market_raw = 0;
    bool has_release_fee = false;
    int affection = 50;
    bool transfer_listed = false;
    bool loan_listed = false;
    int team_role = 0;
    int team_role_lvl = 0;
    std::array<int, 48> stats{};
    std::array<int, 13> pos_apt{};
    std::array<bool, 41> skills{};
    std::array<bool, 7> com_ai{};
};

struct RosterPack {
    std::string version = "13.0";
    std::string kind = "squad"; // "squad" or "player"
    std::string label;
    std::string club;
    std::string created_at;
    int cur_year = 2026;
    std::string source_path;
    std::vector<PackPlayerData> players;
};

enum class PackMatchScope : int {
    UserSquad = 0,
    GlobalDatabase = 1,
    CurrentPlayer = 2,
};

enum class PackMatchKind : int {
    None = 0,
    ExactPid = 1,
    ExactName = 2,
    ManualCurrent = 3,
};

struct PackApplyOptions {
    bool apply_stats_pos = true;
    bool apply_skills_style = true;
    bool apply_traits = true;
    bool apply_team_role = true;
    bool apply_bio = false;
    bool apply_pack_economy = false;
    bool auto_recal_economy = true;
    bool apply_condition = false;
};

struct PackPreviewRow {
    int pack_idx = 0;
    bool selected = false;
    PackMatchKind match_kind = PackMatchKind::None;
    uint64_t target_addr = 0;
    uint32_t target_pid = 0;
    std::string target_name;
    std::string target_club;
    int cur_ovr = 0;
    int new_ovr = 0;
    int cur_pos = 0;
    int new_pos = 0;
    int cur_skills_cnt = 0;
    int new_skills_cnt = 0;
    int32_t cur_market_raw = 0;
    int32_t new_market_raw = 0;
    int32_t cur_salary_raw = 0;
    int32_t new_salary_raw = 0;
    int stat_changes = 0;
};

struct PackPreviewResult {
    bool ok = false;
    std::string code;
    RosterPack pack;
    PackMatchScope scope = PackMatchScope::UserSquad;
    int matched_pid = 0;
    int matched_name = 0;
    int unmatched = 0;
    std::vector<PackPreviewRow> rows;
};

struct PackLibraryItem {
    std::string path;
    std::string filename;
    std::string label;
    std::string club;
    std::string created_at;
    std::string kind;
    int player_count = 0;
    double avg_ovr = 0.0;
};

// Parse structured v13 pack JSON OR legacy v12 hex JSON into a RosterPack
bool parse_roster_pack_json(std::string_view json_str, RosterPack& out_pack);

// Serialize a RosterPack to human-readable JSON
std::string serialize_roster_pack_json(const RosterPack& pack);

// Local Packs directory helpers (%USERPROFILE%\Documents\PES 2021 Workbench\Packs)
std::string get_packs_directory();
std::string make_default_pack_filename(std::string_view label, std::string_view club);
bool save_text_file_utf8(const std::string& path, std::string_view content);
bool load_text_file_utf8(const std::string& path, std::string& out_content);
std::vector<PackLibraryItem> list_saved_packs();

class PESEngine {
public:
    PESEngine();
    ~PESEngine() { shutdown(); }

    PESEngine(const PESEngine&) = delete;
    PESEngine& operator=(const PESEngine&) = delete;

    bool ensure_attached();
    void shutdown();

    StatusSnapshot get_status();
    bool select_player(uint64_t addr);

    PlayerDetails get_player_details(uint64_t addr = 0);
    PlayerDetails set_player_quick_condition(std::optional<int> stamina, std::optional<int> arrow, bool heal, uint64_t addr = 0);
    PlayerDetails update_player_bio(const BioUpdate& upd);
    PlayerDetails set_player_stat(int fid, int val);
    PlayerDetails set_player_stats_batch(const std::vector<std::pair<int, int>>& items);
    PlayerDetails set_player_skill_idx(int idx, bool is_com_ai, bool enabled);
    PlayerDetails set_playing_style(int style_id);
    PlayerDetails set_team_role(int role_id, int level);

    PlayerDetails set_position_aptitude(int pos_idx, int grade);
    PlayerDetails set_registered_position(int pos_idx);
    PlayerDetails step_player_ovr(int delta);
    PlayerDetails set_player_ovr(int target_ovr);
    PlayerDetails step_position_ovr(int pos_idx, int delta);
    PlayerDetails match_position_to_primary(int pos_idx);

    EconSyncMode get_econ_sync_mode();
    void set_econ_sync_mode(EconSyncMode mode);
    PlayerDetails recalibrate_player_economy(uint64_t addr = 0);

    ActionResult undo_last(uint64_t addr = 0);

    FinancesSnapshot get_finances();
    FinancesSnapshot set_finances(std::optional<std::string_view> trf_str, std::optional<std::string_view> sal_str);
    FinancesSnapshot add_finance_delta(bool is_transfer, int32_t raw_delta);

    SquadRoster get_squad_roster();
    ActionResult run_squad_commander(std::string_view mode);

    SearchResult search_players(std::string_view query, int pos_filter, int min_ovr, int max_age, bool free_only, int limit = 150);

    ScoutListsResult get_scout_lists();
    ActionResult add_to_scout_list(int slot_num, uint64_t player_addr = 0);
    ActionResult remove_from_scout_list(int slot_num, uint64_t handle);
    ActionResult clear_scout_list(int slot_num);

    ActionResult set_training_mode(int mode);
    ActionResult set_stamina_mode(int mode);
    ActionResult set_bal_focus(int points, bool lock_enabled);
    ActionResult set_freeze_clock(bool freeze);
    ActionResult set_match_clock(int target_min, int target_sec);
    ActionResult shift_match_clock(int delta_min);
    ActionResult reset_match_half();
    ActionResult end_match_half();

    ActionResult export_player_json();
    PlayerDetails import_player_json(std::string_view json_str);

    // Full Squad / Player Pack Studio (cross-save export, dry-run match preview, selective apply, batch rollback)
    ActionResult export_squad_pack_json(std::string_view custom_label = {});
    ActionResult export_player_pack_json(uint64_t addr = 0);
    PackPreviewResult preview_pack_import(
        std::string_view json_str,
        PackMatchScope scope,
        const PackApplyOptions& opts,
        int forced_pack_idx_for_current = -1
    );
    ActionResult apply_pack_import(const PackPreviewResult& preview, const PackApplyOptions& opts);
    ActionResult undo_last_batch();
    int last_batch_undo_count() const;

private:
    struct UndoEntry {
        std::array<uint8_t, data::PLAYER_RECORD_SIZE> bytes{};
        std::string action;
        std::optional<EconAnchor> prev_anchor;
    };

    struct BatchUndoItem {
        uint64_t addr = 0;
        std::array<uint8_t, data::PLAYER_RECORD_SIZE> bytes{};
        std::optional<EconAnchor> prev_anchor;
    };

    struct SquadCache {
        uint64_t career = 0;
        uint32_t uc = 0;
        double timestamp = 0.0;
        std::vector<uint64_t> addrs;
    };

    static double now_sec();
    static bool is_sane_player_buf(const uint8_t* buf);

    void reset_session();
    void reset_career_caches();
    void discover_singletons();
    void sync_pointers(uint64_t& out_career, uint64_t& out_player, uint64_t& out_match_state, HookStateBlock* out_state = nullptr);
    float read_protected_match_ticks_locked(bool just_armed);

    void push_undo(uint64_t p_addr, const char* action_code, const uint8_t* existing_buf = nullptr);

    std::pair<int, std::string> decode_club_handle(uint32_t raw_u32, uint64_t career);
    uint32_t get_user_club_handle(uint64_t career);
    const std::vector<uint64_t>& get_user_squad_addresses(uint64_t career, bool force_rescan, std::vector<uint8_t>* out_raw_table = nullptr);
    void get_available_salary_budget(uint64_t career, bool force_refresh, int32_t& out_avail, int32_t& out_gross, int32_t& out_wages);

    PlayerDetails build_player_details_from_buf(uint64_t p, uint64_t career, const uint8_t* buf);
    EconSyncDelta apply_economy_sync_locked(
        uint64_t p, uint8_t* after_buf, const uint8_t* before_buf,
        bool allow_market = true, bool allow_salary = true);

    std::recursive_mutex lock_;
    Win32Memory mem_;
    HookManager hooks_;
    std::atomic<bool> shutting_down_{false};

    uint64_t addr_career_owner_ = 0;
    uint64_t addr_match_mgr_ = 0;
    uint32_t last_pid_ = 0;
    uint64_t last_career_ = 0;
    uint64_t last_written_career_ = UINT64_MAX;
    uint64_t last_written_mmgr_ = UINT64_MAX;
    uint64_t last_written_mstate_ = UINT64_MAX;
    bool last_in_match_ = false;
    bool hook5_auto_attempted_ = false;
    uint64_t manual_player_addr_ = 0;
    uint64_t last_hook_player_ = 0;
    uint8_t bal_lock_target_ = 99;
    uint64_t status_seq_ = 0;

    std::unordered_map<uint64_t, std::vector<UndoEntry>> undo_stack_;
    std::vector<BatchUndoItem> last_batch_undo_;
    std::string last_batch_label_;
    EconSyncMode econ_sync_mode_ = EconSyncMode::All;
    std::unordered_map<uint64_t, EconAnchor> econ_anchors_;
    SquadCache squad_cache_;
    double wage_cache_time_ = 0.0;
    uint64_t wage_cache_career_ = 0;
    int32_t wage_cache_sum_ = 0;
    std::unordered_map<uint64_t, std::pair<int, std::string>> club_name_cache_;
};

} // namespace pes
