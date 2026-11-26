#include "../src/engine/ovr_engine.hpp"
#include "../src/engine/pes_engine.hpp"
#include "../src/engine/skill_presets.hpp"
#include "../src/data/game_data.hpp"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>

int main() {
    using namespace pes;
    using namespace pes::data;

    // 1. Test synthetic outfielder + DNA OVR solver + Position Lab isolated solver
    uint8_t buf[PLAYER_RECORD_SIZE]{};
    // Reg pos = 12 (CF / CA)
    set_bits_buf(buf, sizeof(buf), 0x04, 28, 4, 12);
    set_apt_in_buf(buf, 12, 2);
    set_apt_in_buf(buf, 9, 2);  // LWF Grade A
    set_apt_in_buf(buf, 10, 2); // RWF Grade A
    for (int f : OUTFIELD_FIDS) set_fid_in_buf(buf, f, 75);
    set_fid_in_buf(buf, 20, 88); // Finishing
    set_fid_in_buf(buf, 14, 86); // Offensive Awareness
    set_fid_in_buf(buf, 33, 84); // Speed
    set_fid_in_buf(buf, 37, 85); // Acceleration
    for (int f : GK_FIDS) set_fid_in_buf(buf, f, 40);

    auto [ovr0, reg0] = calc_ovr_from_buf(buf, sizeof(buf));
    std::printf("Base CF OVR=%d (reg=%d)\n", ovr0, reg0);
    assert(reg0 == 12);
    assert(ovr0 >= 75 && ovr0 <= 90);

    // DNA +1 OVR
    auto sr1 = solve_primary_ovr_dna(buf, sizeof(buf), ovr0 + 1);
    assert(sr1.action_ok && sr1.has_write && sr1.final_ovr == ovr0 + 1);
    // GK stats untouched!
    for (int f : GK_FIDS) assert(get_fid_from_buf(sr1.out_header.data(), f) == 40);

    // DNA +5 OVR then back to ovr0
    auto sr5 = solve_primary_ovr_dna(buf, sizeof(buf), ovr0 + 5);
    assert(sr5.action_ok && sr5.final_ovr == ovr0 + 5);
    auto sr_back = solve_primary_ovr_dna(sr5.out_header.data(), 0x28, ovr0);
    assert(sr_back.action_ok && sr_back.final_ovr == ovr0);
    std::printf("DNA OVR Solver: %d -> %d -> %d -> %d PASS\n", ovr0, sr1.final_ovr, sr5.final_ovr, sr_back.final_ovr);

    // Isolated Position Lab: raise LWF (9) by +1 without changing CF (12)
    int lwf0 = calc_ovr_from_buf(buf, sizeof(buf), 9, false).first;
    auto iso = solve_secondary_isolated(buf, sizeof(buf), 9, lwf0 + 1);
    assert(iso.action_ok && iso.has_write);
    int pri_after = calc_ovr_from_buf(iso.out_header.data(), 0x28, 12, false).first;
    int lwf_after = calc_ovr_from_buf(iso.out_header.data(), 0x28, 9, false).first;
    assert(pri_after == ovr0);
    assert(lwf_after == lwf0 + 1);
    std::printf("Isolated Position Lab: LWF %d->%d (CF locked at %d) PASS\n", lwf0, lwf_after, pri_after);

    // GK DNA test
    uint8_t gk[PLAYER_RECORD_SIZE]{};
    set_bits_buf(gk, sizeof(gk), 0x04, 28, 4, 0); // GK
    set_apt_in_buf(gk, 0, 2);
    for (int f : OUTFIELD_FIDS) set_fid_in_buf(gk, f, 40);
    for (int f : GK_FIDS) set_fid_in_buf(gk, f, 72);
    auto [gk0, gk_reg] = calc_ovr_from_buf(gk, sizeof(gk));
    auto gk_up = solve_primary_ovr_dna(gk, sizeof(gk), gk0 + 3);
    assert(gk_up.action_ok && gk_up.final_ovr == gk0 + 3);
    for (int f : OUTFIELD_FIDS) assert(get_fid_from_buf(gk_up.out_header.data(), f) == 40);
    std::printf("GK DNA Solver: %d -> %d (outfield untouched) PASS\n", gk0, gk_up.final_ovr);

    // Money parser tests
    assert(parse_human_money_to_raw("25M").value() == 250000);
    assert(parse_human_money_to_raw("500k").value() == 5000);
    assert(parse_human_money_to_raw("1.5B").value() == 15000000);
    assert(parse_human_money_to_raw("€ 2.712.400").value() == 27124);
    assert(parse_human_money_to_raw("R$ 15,5M").value() == 155000);
    assert(!parse_human_money_to_raw("abc").has_value());
    std::printf("Money parser edge cases: PASS\n");

    // UTF-8 normalizer tests
    assert(norm_text_utf8("Vinícius Júnior") == "vinicius junior");
    assert(norm_text_utf8("Kylian Mbappé") == "kylian mbappe");
    assert(norm_text_utf8("Ídolo / Clássica") == "idolo / classica");
    std::printf("UTF-8 accent folding: PASS\n");

    // Verify 13 Captain Team Roles
    int cap_cnt = 0;
    for (const auto& r : TEAM_ROLES) {
        if (r.captain) {
            ++cap_cnt;
            assert(r.captain_pt[0] != '\0' && r.captain_en[0] != '\0');
        }
    }
    assert(cap_cnt == 13);
    std::printf("Team Roles table (%zu roles, %d captain roles): PASS\n", TEAM_ROLES.size(), cap_cnt);

    // Skill presets: todo nome precisa existir em PLAYER_SKILLS, sem repeticao e dentro do limite nativo de 10
    for (const auto& pr : presets::kSkillPresets) {
        int n = 0;
        for (size_t i = 0; i < pr.skills.size(); ++i) {
            if (pr.skills[i].empty()) continue;
            bool found = false;
            for (const auto& sk : PLAYER_SKILLS) if (pr.skills[i] == std::string_view(sk.key)) { found = true; break; }
            if (!found) std::printf("Preset '%s': skill desconhecida '%.*s'\n", pr.en, static_cast<int>(pr.skills[i].size()), pr.skills[i].data());
            assert(found);
            for (size_t j = i + 1; j < pr.skills.size(); ++j) assert(pr.skills[i].empty() || pr.skills[i] != pr.skills[j]);
            ++n;
        }
        assert(n >= 3 && n <= 10);
    }
    std::printf("Skill presets (%zu presets, nomes validos, sem repeticao): PASS\n", presets::kSkillPresets.size());

    // Economy Model tests: monotonicity, exact reversibility, age/nation/position factors, Release Fee
    assert(econ::scale_economy_value(100000, 77, 26, 1, 146, 77, 26, 1, 146, false) == 100000);
    assert(econ::scale_economy_value(15684, 77, 26, 1, 146, 77, 26, 1, 146, true) == 15684);
    for (int o0 : {55, 68, 77, 83, 90}) {
        for (double km : {0.35, 1.0, 2.4}) {
            int32_t om = static_cast<int32_t>(econ::ref_market_raw(o0, 25, 12, 146) * km);
            int32_t os = static_cast<int32_t>(econ::ref_salary_raw(o0, 25, 12, 146) * km);
            int32_t pm = -1, ps = -1;
            for (int o = 40; o <= 99; ++o) {
                int32_t m = econ::scale_economy_value(om, o0, 25, 12, 146, o, 25, 12, 146, false);
                int32_t s = econ::scale_economy_value(os, o0, 25, 12, 146, o, 25, 12, 146, true);
                assert(m >= pm && s >= ps);
                assert(m >= 1 && m <= 99999900 && s >= 1 && s <= 99999900);
                pm = m;
                ps = s;
            }
        }
    }
    // Age effect: 19y wonderkid > 25y prime > 34y veteran at same OVR 82
    assert(econ::ref_market_raw(82, 19, 12, 146) > econ::ref_market_raw(82, 25, 12, 146));
    assert(econ::ref_market_raw(82, 25, 12, 146) > econ::ref_market_raw(82, 34, 12, 146));
    // Nationality effect: England (204) > Spain (236) > Brazil (146) > Chile (147)
    assert(econ::ref_market_raw(80, 24, 12, 204) > econ::ref_market_raw(80, 24, 12, 236));
    assert(econ::ref_market_raw(80, 24, 12, 236) > econ::ref_market_raw(80, 24, 12, 146));
    assert(econ::ref_market_raw(80, 24, 12, 146) > econ::ref_market_raw(80, 24, 12, 147));
    // GK discount vs CF
    assert(econ::ref_market_raw(82, 25, 0, 146) < econ::ref_market_raw(82, 25, 12, 146));
    // Konami Release Fee formula: 0 when clause disabled, scales proportionally with Market Value when enabled
    assert(econ::calc_konami_release_fee_raw(200000, false, 26, 55, 83) == 0);
    int32_t rf1 = econ::calc_konami_release_fee_raw(100000, true, 26, 55, 77);
    int32_t rf2 = econ::calc_konami_release_fee_raw(250000, true, 26, 55, 82);
    assert(rf1 > 0 && rf2 > rf1);
    std::printf("Economy Model (monotonicity, reversibility, age, country, GK, Konami Release Fee): PASS\n");

    // Benchmark 30,000 calc_ovr_from_buf calls (compare vs Python 987 ms!)
    std::vector<uint8_t> table(30000 * 0x28);
    std::mt19937 rng(42);
    for (int i = 0; i < 30000; ++i) {
        uint8_t* p = table.data() + i * 0x28;
        std::memcpy(p, buf, 0x28);
        set_bits_buf(p, 0x28, 0x04, 28, 4, i % 13);
        set_fid_in_buf(p, 20, 40 + (rng() % 60));
    }
    auto t0 = std::chrono::steady_clock::now();
    volatile int64_t sink = 0;
    for (int i = 0; i < 30000; ++i) {
        sink += calc_ovr_from_buf(table.data() + i * 0x28, 0x28).first;
    }
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::printf("BENCHMARK C++ calc_ovr_from_buf x 30,000: %.2f ms (%.1f ns/call, sink=%lld)\n",
                ms, (ms * 1e6) / 30000.0, static_cast<long long>(sink));

    return 0;
}
