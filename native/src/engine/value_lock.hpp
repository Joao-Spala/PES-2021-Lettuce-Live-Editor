#pragma once
// Travas de valores do atleta (condicao): quais campos regravar e com que valor.
// Logica pura (sem memoria de processo), para ser testada sem o jogo aberto.

#include "ovr_engine.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace pes {

struct ValueLockConfig {
    bool stamina = false;
    int stamina_value = 100;   // 0..100
    bool arrow = false;
    int arrow_value = 4;       // 0..4 (seta de forma)
    bool injury = false;       // mantem 0 dias de lesao
    bool any() const { return stamina || arrow || injury; }
    bool operator==(const ValueLockConfig& o) const {
        return stamina == o.stamina && stamina_value == o.stamina_value && arrow == o.arrow &&
               arrow_value == o.arrow_value && injury == o.injury;
    }
};

// Campos do registro do atleta (offsets validados no motor): folego 0x146 bits 0..6 (7 bits),
// seta de forma 0x147 bits 0..2 (3 bits) e dias de lesao no byte 0x148. O id do registro fica em 0x30.
inline constexpr size_t kLockStaminaByte = 0x146;
inline constexpr size_t kLockArrowByte = 0x147;
inline constexpr size_t kLockInjuryByte = 0x148;
inline constexpr size_t kRecordIdByte = 0x30;

enum class LockField : int { Stamina = 0, Arrow = 1, Injury = 2 };

struct LockWrite {
    LockField field = LockField::Stamina;
    uint32_t value = 0;
};

inline ValueLockConfig clamp_lock_config(ValueLockConfig c) {
    c.stamina_value = std::clamp(c.stamina_value, 0, 100);
    c.arrow_value = std::clamp(c.arrow_value, 0, 4);
    return c;
}

inline uint32_t record_player_id(const uint8_t* buf) {
    uint32_t id = 0;
    std::memcpy(&id, buf + kRecordIdByte, 4);
    return id;
}

// Escritas necessarias para o registro `buf` ficar como a trava manda. So campos que diferem.
// Retorna quantas escritas foram planejadas (0 a 3).
inline int plan_lock_writes(const uint8_t* buf, size_t len, const ValueLockConfig& cfg, LockWrite (&out)[3]) {
    if (!buf || len <= kLockInjuryByte) return 0;
    const ValueLockConfig c = clamp_lock_config(cfg);
    int n = 0;
    if (c.stamina) {
        const uint32_t cur = get_bits_buf(buf, len, kLockStaminaByte, 0, 7);
        const uint32_t want = static_cast<uint32_t>(c.stamina_value);
        if (cur != want) out[n++] = {LockField::Stamina, want};
    }
    if (c.arrow) {
        const uint32_t cur = get_bits_buf(buf, len, kLockArrowByte, 0, 3);
        const uint32_t want = static_cast<uint32_t>(c.arrow_value);
        if (cur != want) out[n++] = {LockField::Arrow, want};
    }
    if (c.injury) {
        if (buf[kLockInjuryByte] != 0) out[n++] = {LockField::Injury, 0u};
    }
    return n;
}

} // namespace pes
