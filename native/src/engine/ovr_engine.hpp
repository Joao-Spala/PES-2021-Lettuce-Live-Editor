#pragma once
// High-performance Exact Konami OVR Engine (fcn.1414e7a50 + fcn.141e640a0),
// Hamilton Proportional DNA Overall Solver, and Isolated Position Lab Solver.

#include "../data/game_data.hpp"

#include <array>
#include <cstdint>
#include <cstddef>
#include <string>
#include <utility>

namespace pes {

// Inline bitfield helpers on raw player record buffer (0x17C bytes)
inline uint32_t get_bits_buf(const uint8_t* buf, size_t len, size_t off, int bit_start, int bit_len) {
    uint32_t v = 0;
    if (off + 4 <= len) {
        std::memcpy(&v, buf + off, 4);
    } else if (off < len) {
        std::memcpy(&v, buf + off, len - off);
    }
    uint32_t mask = (bit_len >= 32) ? 0xFFFFFFFFu : ((1u << bit_len) - 1u);
    return (v >> bit_start) & mask;
}

inline void set_bits_buf(uint8_t* buf, size_t len, size_t off, int bit_start, int bit_len, uint32_t val) {
    uint32_t v = 0;
    size_t n = (off + 4 <= len) ? 4 : (off < len ? len - off : 0);
    if (n == 0) return;
    std::memcpy(&v, buf + off, n);
    uint32_t mask = ((bit_len >= 32) ? 0xFFFFFFFFu : ((1u << bit_len) - 1u)) << bit_start;
    v = (v & ~mask) | ((val << bit_start) & mask);
    std::memcpy(buf + off, &v, n);
}

int get_fid_from_buf(const uint8_t* buf, int fid);
void set_fid_in_buf(uint8_t* buf, int fid, int val);

int get_apt_from_buf(const uint8_t* buf, int pos_idx);
void set_apt_in_buf(uint8_t* buf, int pos_idx, int grade);

// Exact Konami OVR formula (~40ns/call, bit-for-bit parity with PES2021.exe)
std::pair<int, int> calc_ovr_from_buf(const uint8_t* buf, size_t len, int eval_pos = -1, bool assume_apt_a = false);

struct SolverResult {
    bool action_ok = false;
    std::string code;
    int orig = 0;
    int final_ovr = 0;
    int pri = 0;
    int pos = -1;
    int reg_pos = -1;
    bool needs_grade = false;
    std::array<uint8_t, 0x28> out_header{};
    bool has_write = false;
};

// Hamilton Proportional DNA Overall Solver (keeps player's stat proportions)
SolverResult solve_primary_ovr_dna(const uint8_t* buf, size_t len, int target_ovr);

// Isolated Secondary Position Solver (Stage 1 single-stat + Stage 2 counterbalance, 0 primary drift)
SolverResult solve_secondary_isolated(const uint8_t* buf, size_t len, int target_pos, int target_ovr);

} // namespace pes
