#include "ovr_engine.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <numeric>
#include <vector>

namespace pes {

namespace {

// O(1) lookup table for fid -> (offset, bit_start)
struct FidTable {
    std::array<data::BitLoc, 48> loc{};
    std::array<bool, 48> valid{};
    std::array<int, 13> pos_code_to_col{};

    constexpr FidTable() {
        for (const auto& s : data::STAT_DEFS) {
            if (s.fid >= 0 && s.fid < 48) {
                loc[s.fid] = {s.offset, s.bit_start};
                valid[s.fid] = true;
            }
        }
        for (int ci = 0; ci < 13; ++ci) {
            int pc = data::OVR_POS_ORDER[ci];
            if (pc >= 0 && pc < 13) pos_code_to_col[pc] = ci;
        }
    }
};

constexpr FidTable kFidTab{};

std::array<double, 48> get_pos_weights(int pos_idx) {
    std::array<double, 48> out{};
    int target_code = (pos_idx >= 0 && pos_idx < 13) ? data::POS_REMAP[pos_idx] : 0;
    int col = kFidTab.pos_code_to_col[target_code];

    std::array<int, 48> slot_to_fid{};
    slot_to_fid.fill(-1);
    for (const auto& p : data::OVR_FIELD_TO_SLOT) {
        if (p.slot >= 0 && p.slot < 48) slot_to_fid[p.slot] = p.fid;
    }

    for (int r = 0; r < 23; ++r) {
        double w = static_cast<double>(data::OVR_WEIGHTS_23X13[r][col]);
        if (w == 0.0) continue;
        int sidx = data::OVR_STAT_INDICES_23[r];
        if (sidx == 0x21) {
            for (int gk_slot : {33, 37}) {
                int fid = slot_to_fid[gk_slot];
                if (fid >= 0) out[fid] += w * 0.5;
            }
        } else if (sidx == 0x22) {
            for (int gk_slot : {38, 39, 40}) {
                int fid = slot_to_fid[gk_slot];
                if (fid >= 0) out[fid] += w / 3.0;
            }
        } else if (sidx >= 0 && sidx < 48) {
            int fid = slot_to_fid[sidx];
            if (fid >= 0) out[fid] += w;
        }
    }
    return out;
}

} // namespace

int get_fid_from_buf(const uint8_t* buf, int fid) {
    if (fid < 0 || fid >= 48 || !kFidTab.valid[fid]) return 40;
    const auto loc = kFidTab.loc[fid];
    if (loc.off == 0x03) return buf[3] & 0x7F;
    uint32_t dw = 0;
    std::memcpy(&dw, buf + loc.off, 4);
    return static_cast<int>((dw >> loc.bit) & 0x7Fu);
}

void set_fid_in_buf(uint8_t* buf, int fid, int val) {
    if (fid < 0 || fid >= 48 || !kFidTab.valid[fid]) return;
    const auto loc = kFidTab.loc[fid];
    int v = std::clamp(val, 40, 99);
    if (loc.off == 0x03) {
        buf[3] = static_cast<uint8_t>((buf[3] & 0x80u) | static_cast<uint32_t>(v & 0x7F));
    } else {
        set_bits_buf(buf, 0x28, loc.off, loc.bit, 7, static_cast<uint32_t>(v));
    }
}

int get_apt_from_buf(const uint8_t* buf, int pos_idx) {
    if (pos_idx < 0 || pos_idx >= 13) return 0;
    const auto loc = data::POS_APT_LOC[pos_idx];
    return static_cast<int>(get_bits_buf(buf, 0x28, loc.off, loc.bit, 2));
}

void set_apt_in_buf(uint8_t* buf, int pos_idx, int grade) {
    if (pos_idx < 0 || pos_idx >= 13) return;
    const auto loc = data::POS_APT_LOC[pos_idx];
    set_bits_buf(buf, 0x28, loc.off, loc.bit, 2, static_cast<uint32_t>(std::clamp(grade, 0, 2)));
}

std::pair<int, int> calc_ovr_from_buf(const uint8_t* buf, size_t len, int eval_pos, bool assume_apt_a) {
    if (!buf || len < 0x28) return {40, 0};

    // Single fast stack copy of the 0x28-byte header (10 dwords)
    uint32_t w[10];
    std::memcpy(w, buf, 0x28);

    int real_reg_pos = static_cast<int>((w[1] >> 28) & 0x0Fu);
    if (real_reg_pos > 12) real_reg_pos = 12;
    int target_pos = (eval_pos < 0 || eval_pos > 12) ? real_reg_pos : eval_pos;

    int reg_code = data::POS_REMAP[real_reg_pos];
    int target_code = data::POS_REMAP[target_pos];

    int arr[44];
    for (int i = 0; i < 44; ++i) arr[i] = -1;
    arr[0] = reg_code;

    // Unpack all 25 core stats + fid 32 directly from w[]
    for (const auto& pair : data::OVR_FIELD_TO_SLOT) {
        if (pair.slot < 41) {
            if (pair.fid >= 0 && pair.fid < 48 && kFidTab.valid[pair.fid]) {
                const auto loc = kFidTab.loc[pair.fid];
                if (loc.off == 0x03) {
                    arr[pair.slot] = static_cast<int>(buf[3] & 0x7Fu);
                } else {
                    arr[pair.slot] = static_cast<int>((w[loc.off >> 2] >> loc.bit) & 0x7Fu);
                }
            } else if (pair.fid == 32) {
                arr[pair.slot] = static_cast<int>((w[8] >> 20) & 3u);
            }
        }
    }

    int col = kFidTab.pos_code_to_col[target_code];
    int acc = 0;
    for (int r = 0; r < 23; ++r) {
        int wt = data::OVR_WEIGHTS_23X13[r][col];
        if (wt == 0) continue;
        int sidx = data::OVR_STAT_INDICES_23[r];
        int v;
        if (sidx == 0x21) {
            int avg_gk1 = (arr[33] + arr[37]) >> 1;
            v = (avg_gk1 < 25) ? 1 : (avg_gk1 - 25);
        } else if (sidx == 0x22) {
            int avg_gk2 = (arr[38] + arr[40] + arr[39]) / 3;
            v = (avg_gk2 < 25) ? 1 : (avg_gk2 - 25);
        } else {
            v = arr[sidx];
            if (sidx != 0x24) v = (v < 25) ? 1 : (v - 25);
        }
        acc += wt * v;
    }

    int u1 = (acc + 50) / 100;
    if (target_code == reg_code) {
        u1 += data::OVR_BASE_REG[col];
        if (target_code != 12) {
            int s6 = 0;
            for (int sidx : data::OVR_AVG_INDICES_22) {
                if (sidx == 0x21) s6 += (arr[37] + arr[33]) >> 1;
                else if (sidx == 0x22) s6 += (arr[38] + arr[40] + arr[39]) / 3;
                else s6 += arr[sidx];
            }
            int u3 = ((s6 * 100) / 22) + 50;
            int cval = 70;
            if (u3 < 10000) {
                int c_idx = u3 / 100;
                if (c_idx >= 0 && c_idx < static_cast<int>(data::OVR_CURVE_100.size())) {
                    cval = data::OVR_CURVE_100[c_idx];
                }
            }
            u1 = (cval * 40 + 50 + u1 * 60) / 100;
        }
    } else {
        int apt = assume_apt_a ? 2 : get_apt_from_buf(buf, target_pos);
        if (apt >= 2) u1 += data::OVR_BASE_APT_A[col];
        else if (apt == 1) u1 += data::OVR_BASE_APT_B[col];
    }

    return {std::clamp(u1, 40, 109), real_reg_pos};
}

SolverResult solve_primary_ovr_dna(const uint8_t* buf, size_t len, int target_ovr) {
    SolverResult res;
    if (!buf || len < 0x28) {
        res.code = "read_failed";
        return res;
    }
    auto [cur_ovr, reg_pos] = calc_ovr_from_buf(buf, len);
    int target = std::clamp(target_ovr, 40, 109);
    res.orig = cur_ovr;
    res.final_ovr = cur_ovr;
    res.reg_pos = reg_pos;
    res.pri = cur_ovr;

    if (cur_ovr == target) {
        res.action_ok = true;
        res.code = "already_at_ovr";
        return res;
    }

    std::vector<int> active_fids;
    active_fids.reserve(25);
    if (reg_pos == 0) {
        for (int f : data::GK_FIDS) active_fids.push_back(f);
        for (int f : data::OUTFIELD_FIDS) {
            if (get_fid_from_buf(buf, f) > 43) active_fids.push_back(f);
        }
    } else {
        for (int f : data::OUTFIELD_FIDS) active_fids.push_back(f);
    }

    std::array<int, 48> base_stats{};
    std::array<int, 48> weights{};
    double total_w = 0.0;
    for (int f : active_fids) {
        base_stats[f] = get_fid_from_buf(buf, f);
        weights[f] = std::max(4, base_stats[f] - 30);
        total_w += static_cast<double>(weights[f]);
    }
    if (total_w <= 0.0) total_w = 1.0;

    int direction = (target > cur_ovr) ? 1 : -1;
    std::array<uint8_t, 0x28> orig_hdr{};
    std::memcpy(orig_hdr.data(), buf, 0x28);

    std::vector<std::array<uint8_t, 0x28>> history;
    history.reserve(64);
    history.push_back(orig_hdr);

    auto best_ba = orig_hdr;

    auto commit = [&](const std::array<uint8_t, 0x28>& final_ba) -> SolverResult {
        auto [fin_ovr, _] = calc_ovr_from_buf(final_ba.data(), 0x28, reg_pos, false);
        res.action_ok = true;
        res.code = "pri_updated";
        res.orig = cur_ovr;
        res.final_ovr = fin_ovr;
        res.pri = fin_ovr;
        res.out_header = final_ba;
        res.has_write = true;
        return res;
    };

    struct RemItem { double frac; int wt; int fid; };
    std::vector<RemItem> rems;
    rems.reserve(active_fids.size());
    std::array<int, 48> floors{};

    const int max_k = static_cast<int>(active_fids.size()) * 60;
    for (int k = 1; k <= max_k; ++k) {
        auto cand = orig_hdr;
        rems.clear();
        int sum_fl = 0;
        for (int f : active_fids) {
            double q = (static_cast<double>(k) * static_cast<double>(weights[f])) / total_w;
            int fl = static_cast<int>(q);
            floors[f] = fl;
            sum_fl += fl;
            rems.push_back({q - static_cast<double>(fl), weights[f], f});
        }
        std::sort(rems.begin(), rems.end(), [](const RemItem& a, const RemItem& b) {
            if (a.frac != b.frac) return a.frac > b.frac;
            if (a.wt != b.wt) return a.wt > b.wt;
            return a.fid > b.fid;
        });
        int rem_need = k - sum_fl;
        for (int idx = 0; idx < std::min(rem_need, static_cast<int>(rems.size())); ++idx) {
            floors[rems[idx].fid] += 1;
        }
        for (int f : active_fids) {
            set_fid_in_buf(cand.data(), f, base_stats[f] + direction * floors[f]);
        }

        auto [new_ovr, _] = calc_ovr_from_buf(cand.data(), 0x28, reg_pos, false);
        best_ba = cand;
        if (new_ovr == target) return commit(cand);

        if ((direction == 1 && new_ovr > target) || (direction == -1 && new_ovr < target)) {
            // Try trimming 1 point from lowest-remainder stat
            for (int idx = static_cast<int>(rems.size()) - 1; idx >= 0; --idx) {
                int tf = rems[idx].fid;
                if (get_fid_from_buf(cand.data(), tf) != base_stats[tf]) {
                    auto alt = cand;
                    set_fid_in_buf(alt.data(), tf, get_fid_from_buf(alt.data(), tf) - direction);
                    if (calc_ovr_from_buf(alt.data(), 0x28, reg_pos, false).first == target) {
                        return commit(alt);
                    }
                }
            }

            // Core DNA refinement from recent history steps
            auto w_pos = get_pos_weights(reg_pos);
            auto dna_ranked = active_fids;
            std::sort(dna_ranked.begin(), dna_ranked.end(), [&](int a, int b) {
                double sa = (w_pos[a] + 4.0) * static_cast<double>(weights[a]);
                double sb = (w_pos[b] + 4.0) * static_cast<double>(weights[b]);
                return sa > sb;
            });

            int h_start = std::max(0, static_cast<int>(history.size()) - 10);
            for (int hi = static_cast<int>(history.size()) - 1; hi >= h_start; --hi) {
                auto trial = history[hi];
                for (int f : dna_ranked) {
                    int cv = get_fid_from_buf(trial.data(), f);
                    int nv = std::clamp(cv + direction, 40, 99);
                    if (nv != cv) {
                        set_fid_in_buf(trial.data(), f, nv);
                        int to = calc_ovr_from_buf(trial.data(), 0x28, reg_pos, false).first;
                        if (to == target) return commit(trial);
                        if ((direction == 1 && to > target) || (direction == -1 && to < target)) {
                            set_fid_in_buf(trial.data(), f, cv);
                        }
                    }
                }
            }
            return commit(cand);
        }

        if (history.size() >= 12) history.erase(history.begin());
        history.push_back(cand);
    }

    return commit(best_ba);
}

SolverResult solve_secondary_isolated(const uint8_t* buf, size_t len, int target_pos, int target_ovr) {
    SolverResult res;
    if (!buf || len < 0x28) {
        res.code = "read_failed";
        return res;
    }
    auto [orig_pri, reg_pos] = calc_ovr_from_buf(buf, len);
    if (target_pos == reg_pos) {
        return solve_primary_ovr_dna(buf, len, target_ovr);
    }

    int orig_sec = calc_ovr_from_buf(buf, len, target_pos, false).first;
    int target = std::clamp(target_ovr, 40, 99);
    res.pos = target_pos;
    res.reg_pos = reg_pos;
    res.orig = orig_sec;
    res.final_ovr = orig_sec;
    res.pri = orig_pri;

    if (orig_sec == target) {
        res.action_ok = true;
        res.code = "already_at_ovr";
        return res;
    }

    int direction = (target > orig_sec) ? 1 : -1;
    auto w_pri = get_pos_weights(reg_pos);
    auto w_sec = get_pos_weights(target_pos);

    std::vector<int> fids;
    if (target_pos == 0) fids.assign(data::GK_FIDS.begin(), data::GK_FIDS.end());
    else fids.assign(data::OUTFIELD_FIDS.begin(), data::OUTFIELD_FIDS.end());

    std::vector<int> up_base;
    for (int f : fids) {
        if (w_sec[f] > 0.0) up_base.push_back(f);
    }
    if (up_base.empty()) up_base = fids;
    std::sort(up_base.begin(), up_base.end(), [&](int a, int b) {
        double da = w_sec[a] - 0.6 * w_pri[a];
        double db = w_sec[b] - 0.6 * w_pri[b];
        if (da != db) return da > db;
        return w_sec[a] > w_sec[b];
    });

    std::vector<int> cb_pool;
    if (reg_pos != 0) {
        cb_pool.assign(data::OUTFIELD_FIDS.begin(), data::OUTFIELD_FIDS.end());
    } else {
        cb_pool.assign(data::GK_FIDS.begin(), data::GK_FIDS.end());
        cb_pool.insert(cb_pool.end(), data::OUTFIELD_FIDS.begin(), data::OUTFIELD_FIDS.end());
    }
    std::sort(cb_pool.begin(), cb_pool.end(), [&](int a, int b) {
        int za = (w_sec[a] == 0.0) ? 1 : 0;
        int zb = (w_sec[b] == 0.0) ? 1 : 0;
        if (za != zb) return za > zb;
        double da = w_pri[a] - w_sec[a];
        double db = w_pri[b] - w_sec[b];
        if (da != db) return da > db;
        return -w_sec[a] > -w_sec[b];
    });

    std::array<uint8_t, 0x28> ba{};
    std::memcpy(ba.data(), buf, 0x28);
    auto best_ba = ba;
    int best_sec = orig_sec;

    std::vector<int> up_ranked = up_base;
    std::vector<int> cb_live;
    cb_live.reserve(cb_pool.size());

    for (int iter = 0; iter < 1500; ++iter) {
        int cur_sec = calc_ovr_from_buf(ba.data(), 0x28, target_pos, false).first;
        if (cur_sec == target) break;
        bool moved = false;

        std::sort(up_ranked.begin(), up_ranked.end(), [&](int a, int b) {
            double sa = (w_sec[a] - 0.6 * w_pri[a]) * 2.0 - static_cast<double>(get_fid_from_buf(ba.data(), a) * direction) * 0.15;
            double sb = (w_sec[b] - 0.6 * w_pri[b]) * 2.0 - static_cast<double>(get_fid_from_buf(ba.data(), b) * direction) * 0.15;
            return sa > sb;
        });

        // Stage 1: pure single-stat nudge
        for (int f : up_ranked) {
            int cv = get_fid_from_buf(ba.data(), f);
            int nv = cv + direction;
            if (nv < 40 || nv > 99) continue;
            set_fid_in_buf(ba.data(), f, nv);
            int tp = calc_ovr_from_buf(ba.data(), 0x28, reg_pos, false).first;
            int ts = calc_ovr_from_buf(ba.data(), 0x28, target_pos, false).first;
            bool overshoot = (direction > 0 && ts > target) || (direction < 0 && ts < target);
            if (tp == orig_pri && !overshoot) {
                moved = true;
                if ((direction > 0 && ts > best_sec) || (direction < 0 && ts < best_sec)) {
                    best_sec = ts;
                    best_ba = ba;
                }
                break;
            }
            set_fid_in_buf(ba.data(), f, cv);
        }
        if (moved) continue;

        // Stage 2: exact multi-stat counterbalance
        for (int f_up : up_ranked) {
            int cv_u = get_fid_from_buf(ba.data(), f_up);
            int nv_u = cv_u + direction;
            if (nv_u < 40 || nv_u > 99) continue;
            double ws_u = w_sec[f_up];
            auto snap = ba;
            set_fid_in_buf(ba.data(), f_up, nv_u);
            int tp = calc_ovr_from_buf(ba.data(), 0x28, reg_pos, false).first;
            if (tp == orig_pri) {
                ba = snap;
                continue;
            }

            cb_live.clear();
            for (int f : cb_pool) {
                if (f != f_up && w_sec[f] < ws_u) cb_live.push_back(f);
            }
            std::sort(cb_live.begin(), cb_live.end(), [&](int a, int b) {
                int za = (w_sec[a] == 0.0) ? 1 : 0;
                int zb = (w_sec[b] == 0.0) ? 1 : 0;
                if (za != zb) return za > zb;
                double sa = (w_pri[a] - w_sec[a]) * 2.0 + static_cast<double>(get_fid_from_buf(ba.data(), a) * direction) * 0.15;
                double sb = (w_pri[b] - w_sec[b]) * 2.0 + static_cast<double>(get_fid_from_buf(ba.data(), b) * direction) * 0.15;
                return sa > sb;
            });

            bool fixed = false;
            for (int step = 0; step < 8; ++step) {
                bool step_done = false;
                for (int f_cb : cb_live) {
                    int cv_c = get_fid_from_buf(ba.data(), f_cb);
                    int nv_c = cv_c - direction;
                    if (nv_c < 40 || nv_c > 99) continue;
                    set_fid_in_buf(ba.data(), f_cb, nv_c);
                    tp = calc_ovr_from_buf(ba.data(), 0x28, reg_pos, false).first;
                    step_done = true;
                    if (tp == orig_pri) {
                        fixed = true;
                        break;
                    }
                }
                if (fixed || !step_done) break;
            }

            if (fixed) {
                int ts = calc_ovr_from_buf(ba.data(), 0x28, target_pos, false).first;
                bool overshoot = (direction > 0 && ts > target) || (direction < 0 && ts < target);
                if (!overshoot && (direction * (ts - cur_sec) >= 0)) {
                    moved = true;
                    if ((direction > 0 && ts > best_sec) || (direction < 0 && ts < best_sec)) {
                        best_sec = ts;
                        best_ba = ba;
                    }
                    break;
                }
            }
            ba = snap;
        }

        if (!moved) break;
    }

    int apt = get_apt_from_buf(best_ba.data(), target_pos);
    res.needs_grade = (direction > 0 && apt < 2);
    if (best_sec == orig_sec) {
        res.action_ok = false;
        res.code = "iso_limit";
        return res;
    }

    res.action_ok = true;
    res.code = (best_sec != target) ? "pos_partial" : "pos_updated";
    res.final_ovr = best_sec;
    res.out_header = best_ba;
    res.has_write = true;
    return res;
}

} // namespace pes
