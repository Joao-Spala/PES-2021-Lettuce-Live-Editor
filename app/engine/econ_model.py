"""
Empirically calibrated PES 2021 Master League Economic Valuation Engine (V3).
Bit-for-bit parity with native/src/engine/econ_model.hpp.
Cliff-free smooth curve calibrated on 19,256 live ML players + Transfermarkt 2025/26,
with continuous youth potential boost, elite GK cap, legend veteran resilience,
strict global monotonicity, and decompiled Konami Release Fee formula (0x14156BE00).
"""

import math
from typing import List, Tuple

ECON_MODE_OFF = 0
ECON_MODE_ALL = 1
ECON_MODE_MARKET_RELEASE = 2
ECON_MODE_SALARY_ONLY = 3

MKT_OVR_KNOTS: List[Tuple[float, float]] = [
    (40.0, 250.0),
    (50.0, 580.0),
    (60.0, 1300.0),
    (65.0, 2500.0),
    (68.0, 4000.0),
    (70.0, 6000.0),
    (72.0, 10000.0),
    (74.0, 19000.0),
    (76.0, 38000.0),
    (78.0, 76000.0),
    (80.0, 148000.0),
    (82.0, 255000.0),
    (84.0, 420000.0),
    (86.0, 680000.0),
    (88.0, 1020000.0),
    (90.0, 1400000.0),
    (92.0, 1720000.0),
    (94.0, 2020000.0),
    (96.0, 2320000.0),
    (99.0, 2750000.0),
]

SAL_OVR_KNOTS: List[Tuple[float, float]] = [
    (40.0, 120.0),
    (50.0, 210.0),
    (60.0, 400.0),
    (64.0, 580.0),
    (66.0, 950.0),
    (68.0, 1600.0),
    (70.0, 2600.0),
    (72.0, 3900.0),
    (74.0, 5800.0),
    (76.0, 8600.0),
    (78.0, 12800.0),
    (80.0, 19000.0),
    (82.0, 27000.0),
    (84.0, 38000.0),
    (86.0, 57000.0),
    (88.0, 89000.0),
    (90.0, 140000.0),
    (92.0, 198000.0),
    (94.0, 250000.0),
    (96.0, 305000.0),
    (99.0, 395000.0),
]

AGE_MKT_KNOTS: List[Tuple[float, float]] = [
    (15.0, 1.50), (16.0, 1.46), (17.0, 1.41), (18.0, 1.36), (19.0, 1.31), (20.0, 1.25),
    (21.0, 1.20), (22.0, 1.15), (23.0, 1.10), (24.0, 1.05), (25.0, 1.00), (26.0, 0.94),
    (27.0, 0.87), (28.0, 0.79), (29.0, 0.70), (30.0, 0.61), (31.0, 0.52), (32.0, 0.43),
    (33.0, 0.35), (34.0, 0.27), (35.0, 0.205), (36.0, 0.155), (37.0, 0.120),
    (38.0, 0.095), (39.0, 0.080), (40.0, 0.068), (45.0, 0.038),
]

AGE_SAL_KNOTS: List[Tuple[float, float]] = [
    (15.0, 0.82), (18.0, 0.86), (21.0, 0.92), (25.0, 1.00),
    (28.0, 1.08), (30.0, 1.12), (32.0, 1.14), (35.0, 1.10), (45.0, 0.92),
]

POS_MKT_MULT: Tuple[float, ...] = (
    0.68, 0.96, 0.98, 0.98, 0.98, 1.04, 1.02, 1.02, 1.06, 1.07, 1.07, 1.05, 1.08
)

_TOP_EU_NATS = {208, 236, 210, 215, 197, 228, 224}
_STRONG_EU_NATS = {226, 203, 200, 194, 238, 227, 237, 202, 232, 241, 214, 303, 239, 211, 190}
_TIER_A_NATS = {152, 148, 13, 135, 124, 76, 83, 80, 56, 64, 50, 44, 73}
_OTHER_SA_NATS = {147, 150, 149, 151, 153, 145}


def _log_interp(x: float, knots: List[Tuple[float, float]]) -> float:
    if x <= knots[0][0]:
        return knots[0][1]
    if x >= knots[-1][0]:
        return knots[-1][1]
    for i in range(len(knots) - 1):
        x0, y0 = knots[i]
        x1, y1 = knots[i + 1]
        if x0 <= x <= x1:
            t = (x - x0) / (x1 - x0)
            return math.exp(math.log(y0) * (1.0 - t) + math.log(y1) * t)
    return knots[-1][1]


def nat_market_mult(nat_id: int) -> float:
    if nat_id == 204:
        return 1.16
    if nat_id in _TOP_EU_NATS:
        return 1.10
    if nat_id in _STRONG_EU_NATS:
        return 1.05
    if nat_id in (146, 144):
        return 0.98
    if nat_id in _TIER_A_NATS:
        return 0.95
    if nat_id in _OTHER_SA_NATS:
        return 0.88
    if (190 <= nat_id <= 260) or (298 <= nat_id <= 315):
        return 1.01
    if 144 <= nat_id <= 155:
        return 0.90
    return 0.90


def nat_salary_mult(nat_id: int) -> float:
    if nat_id == 204:
        return 1.08
    if nat_id in _TOP_EU_NATS:
        return 1.04
    if nat_id in (146, 144):
        return 0.96
    if nat_id in (152, 148):
        return 0.95
    if (190 <= nat_id <= 260) or (298 <= nat_id <= 315):
        return 1.01
    if 144 <= nat_id <= 155:
        return 0.92
    return 0.95


def ref_market_raw(ovr: int, age: int, pos: int, nat: int) -> float:
    p = max(0, min(12, int(pos)))
    ovr_d = float(max(40, min(99, int(ovr))))
    eff_age = float(max(15, min(50, int(age))))
    if p == 0 and eff_age > 26.0:
        eff_age = 26.0 + (eff_age - 26.0) * 0.65
    base_age_m = _log_interp(eff_age, AGE_MKT_KNOTS)

    if eff_age < 24.0:
        yf = min(1.35, (24.0 - eff_age) / 6.0)
        z = (ovr_d - 77.0) / 5.5
        wk = math.exp(-(z * z))
        base_age_m *= (1.0 + 0.62 * yf * wk)
        if ovr_d > 85.0:
            comp = 1.0 - math.exp(-((ovr_d - 85.0) / 5.0))
            base_age_m = 1.0 + (base_age_m - 1.0) * (1.0 - 0.52 * comp)

    if eff_age >= 34.0 and ovr_d >= 84.0:
        vf = min(1.0, (eff_age - 33.0) / 6.0)
        of = min(1.0, (ovr_d - 83.0) / 6.0)
        base_age_m *= (1.0 + 0.55 * vf * of)

    pos_m = POS_MKT_MULT[p]
    if p == 0 and ovr_d > 82.0:
        gk_c = min(1.0, (ovr_d - 82.0) / 10.0)
        pos_m *= (1.0 - 0.24 * gk_c)

    return _log_interp(ovr_d, MKT_OVR_KNOTS) * base_age_m * pos_m * nat_market_mult(int(nat))


def ref_salary_raw(ovr: int, age: int, pos: int, nat: int) -> float:
    ovr_d = float(max(40, min(99, int(ovr))))
    age_d = float(max(15, min(50, int(age))))
    return _log_interp(ovr_d, SAL_OVR_KNOTS) * _log_interp(age_d, AGE_SAL_KNOTS) * nat_salary_mult(int(nat))


def _round_half_away(v: float) -> float:
    return math.floor(v + 0.5) if v >= 0.0 else math.ceil(v - 0.5)


def money_round_step(v: float, is_salary: bool) -> float:
    if is_salary:
        if v < 500.0:
            return 1.0
        if v < 2000.0:
            return 10.0
        if v < 10000.0:
            return 50.0
        if v < 50000.0:
            return 100.0
        if v < 200000.0:
            return 500.0
        return 1000.0
    if v < 1000.0:
        return 50.0
    if v < 5000.0:
        return 100.0
    if v < 20000.0:
        return 500.0
    if v < 100000.0:
        return 1000.0
    if v < 500000.0:
        return 5000.0
    return 10000.0


def round_pes_money(raw_val: float, is_salary: bool) -> int:
    v = max(1.0, min(99_999_900.0, float(raw_val)))
    step = money_round_step(v, is_salary)
    rounded = _round_half_away(v / step) * step
    return max(1, min(99_999_900, int(rounded)))


def secondary_versatility_mult(
    reg_pos: int, pri_ovr: int, pos_ovrs: list[int], pos_apts: list[int]
) -> float:
    if reg_pos == 0:
        return 1.0
    sec_sum = 0.0
    denom = max(12.0, float(max(40, min(99, int(pri_ovr))) - 58))
    for p in range(1, 13):
        if p == reg_pos:
            continue
        apt = int(pos_apts[p])
        if apt <= 0:
            continue
        ovr = max(40, min(99, int(pos_ovrs[p])))
        if ovr < 62:
            continue
        rel = max(0.0, min(1.05, float(ovr - 58) / denom))
        qual = max(0.0, min(1.0, float(ovr - 60) / 35.0))
        grade_w = 1.0 if apt == 2 else 0.45
        sec_sum += grade_w * rel * qual
    return 1.0 + 0.07 * (1.0 - math.exp(-0.32 * sec_sum))


def salary_versatility_mult(market_vers_mult: float) -> float:
    return 1.0 + (float(market_vers_mult) - 1.0) * 0.45


def scale_economy_value(
    orig_raw: int,
    old_ovr: int, old_age: int, old_pos: int, old_nat: int,
    new_ovr: int, new_age: int, new_pos: int, new_nat: int,
    is_salary: bool,
    old_vers: float = 1.0,
    new_vers: float = 1.0,
) -> int:
    vm_old = salary_versatility_mult(old_vers) if is_salary else float(old_vers)
    vm_new = salary_versatility_mult(new_vers) if is_salary else float(new_vers)
    ref_fn = ref_salary_raw if is_salary else ref_market_raw
    r_old = max(1.0, ref_fn(old_ovr, old_age, old_pos, old_nat) * vm_old)
    r_new = max(1.0, ref_fn(new_ovr, new_age, new_pos, new_nat) * vm_new)

    if orig_raw <= 0:
        return round_pes_money(r_new, is_salary)
    vers_changed = abs(new_vers - old_vers) > 1e-6
    if old_ovr == new_ovr and old_age == new_age and old_pos == new_pos and old_nat == new_nat and not vers_changed:
        return int(orig_raw)

    k_raw = float(orig_raw) / r_old
    k_clamped = max(0.35, min(2.85, k_raw))
    dist = abs(new_ovr - old_ovr) + 0.5 * abs(new_age - old_age)
    u = dist / 26.0
    decay = math.exp(-(u * u))
    k_eff = (k_clamped ** decay) * ((k_raw / k_clamped) ** (decay * decay))
    target = r_new * k_eff

    pure_ovr_move = (new_age == old_age and new_pos == old_pos and new_nat == old_nat and new_ovr != old_ovr)
    if pure_ovr_move:
        if new_ovr > old_ovr:
            target = max(target, float(orig_raw) * (1.0 + 0.04 * (new_ovr - old_ovr)))
        elif new_ovr < old_ovr:
            target = min(target, float(orig_raw) / (1.0 + 0.04 * (old_ovr - new_ovr)))

    res = round_pes_money(target, is_salary)
    if pure_ovr_move:
        step_i = max(1, int(money_round_step(float(orig_raw), is_salary)))
        if new_ovr > old_ovr and res <= orig_raw:
            res = min(99_999_900, int(orig_raw) + step_i)
        elif new_ovr < old_ovr and res >= orig_raw:
            res = max(1, int(orig_raw) - step_i)
    return res


def calc_konami_release_fee_raw(
    mkt_raw: int, has_release_fee: bool, age: int, affection: int, ovr: int
) -> int:
    if not has_release_fee or mkt_raw <= 0:
        return 0
    f12 = float(affection) * 0.7 - 50.0
    i4 = int(math.floor((50 - int(age)) - 20 + f12))
    i4 = max(-50, min(100, i4))
    f8 = (float(i4) / 100.0) + 1.0
    f7 = 1.0
    if i4 > 0:
        f6 = (float(affection) * 0.001 - 1.0) + (float(ovr) * 0.1 - 7.5)
        f7 = max(1.0, min(2.0, f6))
    val = _round_half_away(float(mkt_raw) * f8 * f7)
    return max(1, min(99_999_900, int(val)))
