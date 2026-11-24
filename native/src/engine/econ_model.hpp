#pragma once
// Empirically calibrated PES 2021 Master League Economic Valuation Engine (V3).
// Cliff-free smooth curve calibrated on 19,256 live ML players + Transfermarkt 2025/26,
// with continuous youth potential boost, elite GK cap, legend veteran resilience,
// strict global monotonicity, and decompiled Konami Release Fee formula (0x14156BE00).

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace pes {

enum class EconSyncMode : int {
    Off = 0,              // Manual (no financial changes on OVR/bio edit)
    All = 1,              // Market Value + Active Salary + Release Fee
    MarketAndRelease = 2, // Market Value + Release Fee only
    SalaryOnly = 3,       // Active Contract Salary only
};

struct EconAnchor {
    uint32_t pid = 0;
    int ovr = 40;
    int age = 25;
    int pos = 12;
    int nat = 0;
    double vers_mult = 1.0;
    int32_t market_raw = 0;
    int32_t salary_raw = 0;
    bool has_release_fee = false;
    int32_t release_fee_raw = 0;
    bool valid = false;
};

struct EconSyncDelta {
    bool applied = false;
    EconSyncMode mode = EconSyncMode::Off;
    int old_ovr = 0;
    int new_ovr = 0;
    int32_t old_market_raw = 0;
    int32_t new_market_raw = 0;
    int32_t old_salary_raw = 0;
    int32_t new_salary_raw = 0;
    int32_t old_release_raw = 0;
    int32_t new_release_raw = 0;
    bool has_release_fee = false;
};

namespace econ {

struct Knot {
    double x;
    double y;
};

// Smooth, cliff-free OVR -> Market Value (raw * 100 = EUR, Age 25, Outfield base = 1.00, Neutral Nation = 1.00)
inline constexpr std::array<Knot, 20> kMarketOvrKnots = {{
    {40.0,     250.0},
    {50.0,     580.0},
    {60.0,    1300.0},
    {65.0,    2500.0},
    {68.0,    4000.0},
    {70.0,    6000.0},
    {72.0,   10000.0},
    {74.0,   19000.0},
    {76.0,   38000.0},
    {78.0,   76000.0},
    {80.0,  148000.0},
    {82.0,  255000.0},
    {84.0,  420000.0},
    {86.0,  680000.0},
    {88.0, 1020000.0},
    {90.0, 1400000.0},
    {92.0, 1720000.0},
    {94.0, 2020000.0},
    {96.0, 2320000.0},
    {99.0, 2750000.0},
}};

// Smooth OVR -> Annual Salary (raw * 100 = EUR, Age 25, Neutral Nation = 1.00)
inline constexpr std::array<Knot, 21> kSalaryOvrKnots = {{
    {40.0,    120.0},
    {50.0,    210.0},
    {60.0,    400.0},
    {64.0,    580.0},
    {66.0,    950.0},
    {68.0,   1600.0},
    {70.0,   2600.0},
    {72.0,   3900.0},
    {74.0,   5800.0},
    {76.0,   8600.0},
    {78.0,  12800.0},
    {80.0,  19000.0},
    {82.0,  27000.0},
    {84.0,  38000.0},
    {86.0,  57000.0},
    {88.0,  89000.0},
    {90.0, 140000.0},
    {92.0, 198000.0},
    {94.0, 250000.0},
    {96.0, 305000.0},
    {99.0, 395000.0},
}};

// Transfermarkt-calibrated Age multiplier for Market Value (Age 25 = 1.00)
inline constexpr std::array<Knot, 27> kAgeMarketKnots = {{
    {15.0, 1.50}, {16.0, 1.46}, {17.0, 1.41}, {18.0, 1.36}, {19.0, 1.31}, {20.0, 1.25},
    {21.0, 1.20}, {22.0, 1.15}, {23.0, 1.10}, {24.0, 1.05}, {25.0, 1.00}, {26.0, 0.94},
    {27.0, 0.87}, {28.0, 0.79}, {29.0, 0.70}, {30.0, 0.61}, {31.0, 0.52}, {32.0, 0.43},
    {33.0, 0.35}, {34.0, 0.27}, {35.0, 0.205}, {36.0, 0.155}, {37.0, 0.120},
    {38.0, 0.095}, {39.0, 0.080}, {40.0, 0.068}, {45.0, 0.038},
}};

// Empirical Age multiplier for Annual Salary (Age 25 = 1.00, veterans retain peak contracts)
inline constexpr std::array<Knot, 9> kAgeSalaryKnots = {{
    {15.0, 0.82}, {18.0, 0.86}, {21.0, 0.92}, {25.0, 1.00},
    {28.0, 1.08}, {30.0, 1.12}, {32.0, 1.14}, {35.0, 1.10}, {45.0, 0.92},
}};

// Empirical Position Market Value multipliers (pos 0..12: GK .. CF)
inline constexpr std::array<double, 13> kPosMarketMult = {
    0.68, // 0 GK (with additional elite cap compression above OVR 82)
    0.96, // 1 CB
    0.98, // 2 LB
    0.98, // 3 RB
    0.98, // 4 DMF
    1.04, // 5 CMF
    1.02, // 6 LMF
    1.02, // 7 RMF
    1.06, // 8 AMF
    1.07, // 9 LWF
    1.07, // 10 RWF
    1.05, // 11 SS
    1.08, // 12 CF
};

template <size_t N>
inline double log_interp(double x, const std::array<Knot, N>& knots) {
    if (x <= knots.front().x) return knots.front().y;
    if (x >= knots.back().x) return knots.back().y;
    for (size_t i = 0; i + 1 < N; ++i) {
        if (x >= knots[i].x && x <= knots[i + 1].x) {
            double t = (x - knots[i].x) / (knots[i + 1].x - knots[i].x);
            return std::exp(std::log(knots[i].y) * (1.0 - t) + std::log(knots[i + 1].y) * t);
        }
    }
    return knots.back().y;
}

inline double nat_market_mult(int nat_id) {
    if (nat_id == 204) return 1.16; // England (homegrown Premier League premium)
    switch (nat_id) {
        case 208: case 236: case 210: case 215: case 197: case 228: case 224:
            return 1.10; // France, Spain, Germany, Italy, Belgium, Portugal, Netherlands
        case 226: case 203: case 200: case 194: case 238: case 227: case 237:
        case 202: case 232: case 241: case 214: case 303: case 239: case 211: case 190:
            return 1.05; // Strong European markets
        case 146: case 144:
            return 0.98; // Brazil, Argentina
        case 152: case 148: case 13: case 135: case 124:
        case 76: case 83: case 80: case 56: case 64: case 50: case 44: case 73:
            return 0.95; // Uruguay, Colombia, Japan, USA, Mexico, top African markets
        case 147: case 150: case 149: case 151: case 153: case 145:
            return 0.88; // Other South American markets
        default:
            break;
    }
    if ((nat_id >= 190 && nat_id <= 260) || (nat_id >= 298 && nat_id <= 315)) return 1.01;
    if (nat_id >= 144 && nat_id <= 155) return 0.90;
    return 0.90;
}

inline double nat_salary_mult(int nat_id) {
    if (nat_id == 204) return 1.08;
    switch (nat_id) {
        case 208: case 236: case 210: case 215: case 197: case 228: case 224:
            return 1.04;
        case 146: case 144:
            return 0.96;
        case 152: case 148:
            return 0.95;
        default:
            break;
    }
    if ((nat_id >= 190 && nat_id <= 260) || (nat_id >= 298 && nat_id <= 315)) return 1.01;
    if (nat_id >= 144 && nat_id <= 155) return 0.92;
    return 0.95;
}

inline double ref_market_raw(int ovr, int age, int pos, int nat) {
    int p = std::clamp(pos, 0, 12);
    double ovr_d = static_cast<double>(std::clamp(ovr, 40, 99));
    // Goalkeepers peak later and retain market value ~35% slower past age 26
    double eff_age = static_cast<double>(std::clamp(age, 15, 50));
    if (p == 0 && eff_age > 26.0) {
        eff_age = 26.0 + (eff_age - 26.0) * 0.65;
    }
    double base_age_m = log_interp(eff_age, kAgeMarketKnots);

    // Smooth C-infinity wonderkid potential boost (Gaussian around OVR 77)
    // plus soft ceiling compression above OVR 85 so 18y OVR 90 stays realistic (~190-205M EUR)
    if (eff_age < 24.0) {
        double yf = std::min(1.35, (24.0 - eff_age) / 6.0);
        double z = (ovr_d - 77.0) / 5.5;
        double wk = std::exp(-(z * z));
        base_age_m *= (1.0 + 0.62 * yf * wk);
        if (ovr_d > 85.0) {
            double comp = 1.0 - std::exp(-((ovr_d - 85.0) / 5.0));
            base_age_m = 1.0 + (base_age_m - 1.0) * (1.0 - 0.52 * comp);
        }
    }

    // Smooth legend resilience for OVR >= 84 veterans (age 34+)
    if (eff_age >= 34.0 && ovr_d >= 84.0) {
        double vf = std::min(1.0, (eff_age - 33.0) / 6.0);
        double of = std::min(1.0, (ovr_d - 83.0) / 6.0);
        base_age_m *= (1.0 + 0.55 * vf * of);
    }

    double pos_m = kPosMarketMult[p];
    // Elite GK market cap compression (world-class GKs peak ~45-58M EUR vs 150M+ forwards)
    if (p == 0 && ovr_d > 82.0) {
        double gk_c = std::min(1.0, (ovr_d - 82.0) / 10.0);
        pos_m *= (1.0 - 0.24 * gk_c);
    }

    return log_interp(ovr_d, kMarketOvrKnots) * base_age_m * pos_m * nat_market_mult(nat);
}

inline double ref_salary_raw(int ovr, int age, int pos, int nat) {
    (void)pos;
    double ovr_d = static_cast<double>(std::clamp(ovr, 40, 99));
    double age_d = static_cast<double>(std::clamp(age, 15, 50));
    return log_interp(ovr_d, kSalaryOvrKnots)
         * log_interp(age_d, kAgeSalaryKnots)
         * nat_salary_mult(nat);
}

inline double money_round_step(double v, bool is_salary) {
    if (is_salary) {
        if (v < 500.0) return 1.0;          // 100 EUR
        if (v < 2000.0) return 10.0;        // 1k EUR
        if (v < 10000.0) return 50.0;       // 5k EUR
        if (v < 50000.0) return 100.0;      // 10k EUR
        if (v < 200000.0) return 500.0;     // 50k EUR
        return 1000.0;                      // 100k EUR
    }
    if (v < 1000.0) return 50.0;            // 5k EUR
    if (v < 5000.0) return 100.0;           // 10k EUR
    if (v < 20000.0) return 500.0;          // 50k EUR
    if (v < 100000.0) return 1000.0;        // 100k EUR
    if (v < 500000.0) return 5000.0;        // 500k EUR
    return 10000.0;                         // 1M EUR
}

inline int32_t round_pes_money(double raw_val, bool is_salary) {
    double v = std::clamp(raw_val, 1.0, 99999900.0);
    double step = money_round_step(v, is_salary);
    double rounded = std::round(v / step) * step;
    if (rounded < 1.0) rounded = 1.0;
    if (rounded > 99999900.0) rounded = 99999900.0;
    return static_cast<int32_t>(rounded);
}

// Modest versatility multiplier [1.00 .. 1.07] from playable secondary positions (apt >= 1).
// Original/registered position OVR remains 93%+ of valuation; secondary OVRs count much less.
inline double secondary_versatility_mult(
    int reg_pos, int pri_ovr,
    const std::array<int, 13>& pos_ovrs,
    const std::array<int, 13>& pos_apts)
{
    if (reg_pos == 0) return 1.0;
    double sec_sum = 0.0;
    double denom = std::max(12.0, static_cast<double>(std::clamp(pri_ovr, 40, 99) - 58));
    for (int p = 1; p < 13; ++p) {
        if (p == reg_pos) continue;
        int apt = pos_apts[p];
        if (apt <= 0) continue; // Grade C does not count as playable secondary
        int ovr = std::clamp(pos_ovrs[p], 40, 99);
        if (ovr < 62) continue;
        double rel = std::clamp(static_cast<double>(ovr - 58) / denom, 0.0, 1.05);
        double qual = std::clamp(static_cast<double>(ovr - 60) / 35.0, 0.0, 1.0);
        double grade_w = (apt == 2) ? 1.0 : 0.45;
        sec_sum += grade_w * rel * qual;
    }
    return 1.0 + 0.07 * (1.0 - std::exp(-0.32 * sec_sum));
}

inline double salary_versatility_mult(double market_vers_mult) {
    return 1.0 + (market_vers_mult - 1.0) * 0.45;
}

// Baseline-anchored realistic scaling with gentle mean-reversion on large jumps,
// guaranteed responsiveness on +/-1 OVR steps, and exact reversibility back to orig_raw.
inline int32_t scale_economy_value(
    int32_t orig_raw,
    int old_ovr, int old_age, int old_pos, int old_nat,
    int new_ovr, int new_age, int new_pos, int new_nat,
    bool is_salary,
    double old_vers = 1.0,
    double new_vers = 1.0)
{
    double vm_old = is_salary ? salary_versatility_mult(old_vers) : old_vers;
    double vm_new = is_salary ? salary_versatility_mult(new_vers) : new_vers;
    double r_old = std::max(1.0, (is_salary ? ref_salary_raw(old_ovr, old_age, old_pos, old_nat)
                                            : ref_market_raw(old_ovr, old_age, old_pos, old_nat)) * vm_old);
    double r_new = std::max(1.0, (is_salary ? ref_salary_raw(new_ovr, new_age, new_pos, new_nat)
                                            : ref_market_raw(new_ovr, new_age, new_pos, new_nat)) * vm_new);

    if (orig_raw <= 0) {
        return round_pes_money(r_new, is_salary);
    }
    bool vers_changed = std::abs(new_vers - old_vers) > 1e-6;
    if (old_ovr == new_ovr && old_age == new_age && old_pos == new_pos && old_nat == new_nat && !vers_changed) {
        return orig_raw;
    }

    double k_raw = static_cast<double>(orig_raw) / r_old;
    double k_clamped = std::clamp(k_raw, 0.35, 2.85);
    double dist = std::abs(new_ovr - old_ovr) + 0.5 * std::abs(new_age - old_age);
    double u = dist / 26.0;
    double decay = std::exp(-(u * u));
    double k_eff = std::pow(k_clamped, decay) * std::pow(k_raw / k_clamped, decay * decay);
    double target = r_new * k_eff;

    // Guarantee strict monotonicity when primary OVR moves at fixed age/pos/nat
    bool pure_ovr_move = (new_age == old_age && new_pos == old_pos && new_nat == old_nat && new_ovr != old_ovr);
    if (pure_ovr_move) {
        if (new_ovr > old_ovr) {
            target = std::max(target, static_cast<double>(orig_raw) * (1.0 + 0.04 * (new_ovr - old_ovr)));
        } else if (new_ovr < old_ovr) {
            target = std::min(target, static_cast<double>(orig_raw) / (1.0 + 0.04 * (old_ovr - new_ovr)));
        }
    }

    int32_t res = round_pes_money(target, is_salary);
    if (pure_ovr_move) {
        int32_t step_i = static_cast<int32_t>(money_round_step(static_cast<double>(orig_raw), is_salary));
        if (new_ovr > old_ovr && res <= orig_raw) {
            res = std::min<int32_t>(99999900, orig_raw + std::max<int32_t>(1, step_i));
        } else if (new_ovr < old_ovr && res >= orig_raw) {
            res = std::max<int32_t>(1, orig_raw - std::max<int32_t>(1, step_i));
        }
    }
    return res;
}

// Exact Konami Release Fee formula decompiled from fcn.14156BE00 + fcn.14156BF80
inline int32_t calc_konami_release_fee_raw(int32_t mkt_raw, bool has_release_fee, int age, int affection, int ovr) {
    if (!has_release_fee || mkt_raw <= 0) return 0;
    double f12 = static_cast<double>(affection) * 0.7 - 50.0;
    int i4 = static_cast<int>(std::floor((50 - age) - 20 + f12));
    i4 = std::clamp(i4, -50, 100);
    double f8 = (static_cast<double>(i4) / 100.0) + 1.0;
    double f7 = 1.0;
    if (i4 > 0) {
        double f6 = (static_cast<double>(affection) * 0.001 - 1.0) + (static_cast<double>(ovr) * 0.1 - 7.5);
        f7 = std::clamp(f6, 1.0, 2.0);
    }
    double val = std::round(static_cast<double>(mkt_raw) * f8 * f7);
    if (val < 1.0) val = 1.0;
    if (val > 99999900.0) val = 99999900.0;
    return static_cast<int32_t>(val);
}

} // namespace econ
} // namespace pes
