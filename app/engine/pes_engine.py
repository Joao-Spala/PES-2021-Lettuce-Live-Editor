"""
PES 2021 Workbench - Standalone Game Engine (Master League / BAL / Match / Player).
100% pure Python + Win32Memory + native x64 HookManager. Zero Cheat Engine required.
Returns structured status codes so UI handles 100% PT-BR / EN localization cleanly.
"""

import json
import struct
import threading
import time
import unicodedata
from typing import Any, Dict, List, Optional, Tuple, Union

from .win32_mem import Win32Memory
from .hooks import HookManager
from . import econ_model
from .pes_data import (
    OVR_DATA,
    CLUB_TEAMS,
    SCOUT_LISTS,
    SCOUT_EMPTY_SENTINEL,
    FID_LOC,
    OUTFIELD_FIDS,
    GK_FIDS,
    POS_REMAP,
    POS_APT_LOC,
    STAT_DEFS,
    PLAYER_SKILLS,
    COM_AI_STYLES,
)

PLAYER_RECORD_SIZE = 0x17C
UNDO_MAX_DEPTH = 10
BufLike = Union[bytes, bytearray, memoryview]


def _safe_int(val: Any, default: Optional[int] = None) -> Optional[int]:
    if val is None or val == "":
        return default
    if isinstance(val, bool):
        return int(val)
    if isinstance(val, int):
        return val
    try:
        s = str(val).strip()
        if not s:
            return default
        # Preserve exact 64-bit integer precision (avoids float 53-bit truncation on player handles/pointers)
        return int(s, 0)
    except ValueError:
        try:
            return int(float(val))
        except (ValueError, TypeError):
            return default


def _norm_text(s: str) -> str:
    if not s:
        return ""
    nfd = unicodedata.normalize("NFD", s)
    return "".join(c for c in nfd if unicodedata.category(c) != "Mn").lower()


def _get_bits_buf(buf: BufLike, off: int, bit_start: int, bit_len: int) -> int:
    if off + 4 <= len(buf):
        v = struct.unpack_from("<I", buf, off)[0]
    else:
        chunk = bytes(buf[off : off + 4]).ljust(4, b"\x00")
        v = struct.unpack("<I", chunk)[0]
    return (v >> bit_start) & ((1 << bit_len) - 1)


def _set_bits_buf(buf: bytearray, off: int, bit_start: int, bit_len: int, val: int) -> None:
    if off + 4 <= len(buf):
        v = struct.unpack_from("<I", buf, off)[0]
        mask = ((1 << bit_len) - 1) << bit_start
        v = (v & ~mask) | ((int(val) << bit_start) & mask)
        struct.pack_into("<I", buf, off, v)
    else:
        chunk = bytes(buf[off : off + 4]).ljust(4, b"\x00")
        v = struct.unpack("<I", chunk)[0]
        mask = ((1 << bit_len) - 1) << bit_start
        v = (v & ~mask) | ((int(val) << bit_start) & mask)
        packed = struct.pack("<I", v)
        for i in range(min(4, len(buf) - off)):
            buf[off + i] = packed[i]


def get_fid_from_buf(buf: BufLike, fid: int) -> int:
    off, bs = FID_LOC[fid]
    if off == 0x03:
        return buf[3] & 0x7F
    return _get_bits_buf(buf, off, bs, 7)


def set_fid_in_buf(buf: bytearray, fid: int, val: int) -> None:
    off, bs = FID_LOC[fid]
    v = max(40, min(99, int(val)))
    if off == 0x03:
        buf[3] = (buf[3] & 0x80) | (v & 0x7F)
    else:
        _set_bits_buf(buf, off, bs, 7, v)


def get_apt_from_buf(buf: BufLike, pos_idx: int) -> int:
    off, bs = POS_APT_LOC[pos_idx]
    return _get_bits_buf(buf, off, bs, 2)


def set_apt_in_buf(buf: bytearray, pos_idx: int, grade: int) -> None:
    off, bs = POS_APT_LOC[pos_idx]
    _set_bits_buf(buf, off, bs, 2, max(0, min(2, int(grade))))


def calc_ovr_from_buf(
    buf: BufLike, eval_pos: Optional[int] = None, assume_apt_a: bool = False
) -> Tuple[int, int]:
    """Exact Konami OVR formula (fcn.1414e7a50 + fcn.141e640a0). Accepts bytes or bytearray directly."""
    if not buf or len(buf) < 0x28:
        return 40, 0
    real_reg_pos = (struct.unpack_from("<I", buf, 0x04)[0] >> 28) & 0x0F
    if real_reg_pos > 12:
        real_reg_pos = 12
    target_pos = real_reg_pos if eval_pos is None else int(eval_pos)
    if target_pos < 0 or target_pos > 12:
        target_pos = real_reg_pos

    reg_code = POS_REMAP.get(real_reg_pos, 0)
    target_code = POS_REMAP.get(target_pos, 0)

    arr = [-1] * 44
    arr[0] = reg_code

    for pair in OVR_DATA["field_to_slot"]:
        fid, slot = int(pair[0]), int(pair[1])
        if slot < 41:
            if fid in FID_LOC:
                arr[slot] = get_fid_from_buf(buf, fid)
            elif fid == 32:
                arr[slot] = (struct.unpack_from("<I", buf, 0x20)[0] >> 20) & 3

    col = 0
    for ci, pc in enumerate(OVR_DATA["pos_order"]):
        if pc == target_code:
            col = ci
            break

    weights_23x13 = OVR_DATA["weights_23x13"]
    acc = 0
    for r, sidx in enumerate(OVR_DATA["stat_indices_23"]):
        if sidx == 0x21:
            avg_gk1 = (arr[33] + arr[37]) >> 1
            v = 1 if avg_gk1 < 25 else (avg_gk1 - 25)
        elif sidx == 0x22:
            avg_gk2 = (arr[38] + arr[40] + arr[39]) // 3
            v = 1 if avg_gk2 < 25 else (avg_gk2 - 25)
        else:
            v = arr[sidx]
            if sidx != 0x24:
                v = 1 if v < 25 else (v - 25)
        acc += weights_23x13[r][col] * v

    u1 = (acc + 50) // 100
    if target_code == reg_code:
        u1 += OVR_DATA["base_reg"][col]
        if target_code != 12:
            s6 = 0
            for sidx in OVR_DATA["avg_indices_22"]:
                if sidx == 0x21:
                    s6 += (arr[37] + arr[33]) >> 1
                elif sidx == 0x22:
                    s6 += (arr[38] + arr[40] + arr[39]) // 3
                else:
                    s6 += arr[sidx]
            u3 = ((s6 * 100) // 22) + 50
            if u3 < 10000:
                c_idx = u3 // 100
                curve = OVR_DATA["curve_100"]
                cval = curve[c_idx] if 0 <= c_idx < len(curve) else 70
            else:
                cval = 70
            u1 = (cval * 40 + 50 + u1 * 60) // 100
    else:
        apt = 2 if assume_apt_a else get_apt_from_buf(buf, target_pos)
        if apt >= 2:
            u1 += OVR_DATA["base_apt_a"][col]
        elif apt == 1:
            u1 += OVR_DATA["base_apt_b"][col]

    return max(40, min(109, int(u1))), real_reg_pos


def parse_human_money_to_raw(text: str) -> Optional[int]:
    """Parse '25M', '500k', '1.5B', '2.712.400', etc. into Konami raw int (Euros / 100)."""
    if text is None:
        return None
    s = str(text).strip().upper()
    for sym in ("€", "R$", "$", "£", "EUR", "BRL", "USD", "GBP", " "):
        s = s.replace(sym, "")
    if not s:
        return None
    mult = 1.0
    if s.endswith("B"):
        mult = 1_000_000_000.0
        s = s[:-1]
    elif s.endswith("M"):
        mult = 1_000_000.0
        s = s[:-1]
    elif s.endswith("K"):
        mult = 1_000.0
        s = s[:-1]
    if mult > 1.0:
        s = s.replace(",", ".")
    else:
        if s.count(".") > 1 or (s.count(".") == 1 and len(s.split(".")[1]) == 3):
            s = s.replace(".", "")
        if s.count(",") > 1 or (s.count(",") == 1 and len(s.split(",")[1]) == 3):
            s = s.replace(",", "")
        s = s.replace(",", ".")
    try:
        euros = float(s) * mult
        return max(0, min(2_147_483_647, int(round(euros / 100.0))))
    except ValueError:
        return None


class PESEngine:
    """Thread-safe standalone PES 2021 Master League / BAL / Match / Player Engine."""

    def __init__(self) -> None:
        self.lock = threading.RLock()
        self.mem = Win32Memory()
        self.hooks = HookManager(self.mem)
        self.addr_career_owner: int = 0
        self.addr_match_mgr: int = 0
        self.undo_stack: Dict[int, List[Dict[str, Any]]] = {}
        self._squad_cache: Optional[Dict[str, Any]] = None
        self._wage_cache_time: float = 0.0
        self._wage_cache_career: int = 0
        self._wage_cache_sum: int = 0
        self._last_pid: int = 0
        self._last_career: int = 0
        self._last_in_match: bool = False
        self._manual_player_addr: int = 0
        self._last_hook_player: int = 0
        self._bal_lock_target: int = 99
        self._club_name_cache: Dict[Tuple[int, int], Tuple[int, str]] = {}
        self.econ_sync_mode: int = econ_model.ECON_MODE_ALL
        self._econ_anchors: Dict[int, Dict[str, Any]] = {}

    # ------------------------------------------------------------------
    # ATTACH, INIT & LIFECYCLE
    # ------------------------------------------------------------------
    def ensure_attached(self) -> bool:
        with self.lock:
            if self.mem.attached and self.mem.pid == self._last_pid and self.hooks.state_base:
                if not self.hooks.active_hooks.get(1):
                    self.hooks.enable_hook_1_player()
                return True
            if not self.mem.attach():
                self._reset_session()
                return False
            if self.mem.pid != self._last_pid:
                self._reset_session()
                self._last_pid = self.mem.pid
            if not self.hooks.init_state():
                return False
            self._discover_singletons()
            self.hooks.enable_hook_1_player()
            self.sync_pointers()
            return True

    def _reset_session(self) -> None:
        self.addr_career_owner = 0
        self.addr_match_mgr = 0
        self._last_career = 0
        self._reset_career_caches()

    def _reset_career_caches(self) -> None:
        self._squad_cache = None
        self._wage_cache_career = 0
        self._wage_cache_time = 0.0
        self._manual_player_addr = 0
        self._last_hook_player = 0
        self._club_name_cache.clear()
        self.undo_stack.clear()
        self._econ_anchors.clear()

    def shutdown(self) -> None:
        with self.lock:
            self.hooks.disable_all()
            self.mem.detach()

    def _discover_singletons(self) -> None:
        base = self.mem.module_base
        fast_career = base + 0x3714240
        fast_match = base + 0x3702208
        if self.mem.read_u64(fast_career) > 0x10000:
            self.addr_career_owner = fast_career
            self.addr_match_mgr = fast_match
            return
        call_site = self.mem.aob_scan_region(
            base + 0x1000,
            base + 0x2540000,
            "E8 ?? ?? ?? ?? 48 8B 48 48 48 8D 81 E0 CB 6E 01",
        )
        if call_site:
            rel_call = self.mem.read_i32(call_site + 1)
            fn_getter = call_site + 5 + rel_call
            hdr = self.mem.read_bytes(fn_getter, 7)
            if hdr and hdr[0] == 0x48 and hdr[1] == 0x8B and hdr[2] == 0x05:
                disp = struct.unpack_from("<i", hdr, 3)[0]
                self.addr_career_owner = fn_getter + 7 + disp
        if not self.addr_career_owner:
            self.addr_career_owner = fast_career

        match_site = self.mem.aob_scan_region(
            base + 0x1000,
            base + 0x2540000,
            "48 8B 05 ?? ?? ?? ?? 4C 8B E2 45 33 FF 48 8B B0 70 02 00 00",
        )
        if match_site:
            disp = self.mem.read_i32(match_site + 3)
            self.addr_match_mgr = match_site + 7 + disp
        else:
            self.addr_match_mgr = fast_match

    def sync_pointers(self) -> Tuple[int, int, int]:
        if not self.mem.attached or not self.hooks.state_base:
            return 0, 0, 0
        career = 0
        if self.addr_career_owner:
            owner = self.mem.read_u64(self.addr_career_owner)
            if owner > 0x10000:
                career = self.mem.read_u64(owner + 0x48)

        # Invalidate stale caches & pointers when user switches save or exits ML/BAL
        if career != self._last_career:
            self._last_career = career
            self._reset_career_caches()
            self.mem.write_u64(self.hooks.sym_addr("ptrPlayer"), 0)

        self.mem.write_u64(self.hooks.sym_addr("ptrCareer"), career)

        match_mgr = 0
        match_state = 0
        if self.addr_match_mgr:
            match_mgr = self.mem.read_u64(self.addr_match_mgr)
            if match_mgr > 0x10000:
                match_state = self.mem.read_u64(match_mgr + 0x270)
        self.mem.write_u64(self.hooks.sym_addr("ptrMatchMgr"), match_mgr)
        self.mem.write_u64(self.hooks.sym_addr("ptrMatchState"), match_state)

        # Auto-release clock freeze when leaving a match so the next match never freezes at stale ticks
        in_match = match_state > 0x10000
        if self._last_in_match and not in_match:
            self.mem.write_u32(self.hooks.sym_addr("cfgFreezeClock"), 0)
            self.mem.write_f32(self.hooks.sym_addr("valFrozenTotalTicks"), 0.0)
            self.mem.write_f32(self.hooks.sym_addr("valProtectedMatchTicks"), 0.0)
        self._last_in_match = in_match

        # BAL Focus lock respects the user's chosen target value
        if career > 0x10000:
            if self.mem.read_u32(self.hooks.sym_addr("cfgLockBALFocusPoints")) == 1:
                self.hooks.enable_hook_4_bal_cap()
                self.mem.write_u8(career + 0x016ECBEA, self._bal_lock_target)

        hook_p = self.mem.read_u64(self.hooks.sym_addr("ptrPlayer"))
        if hook_p > 0x10000 and hook_p != self._last_hook_player:
            self._last_hook_player = hook_p
            self._manual_player_addr = hook_p

        active_p = self._manual_player_addr if self._manual_player_addr > 0x10000 else hook_p
        return career, active_p, match_state

    def select_player(self, addr: int) -> bool:
        with self.lock:
            a = _safe_int(addr, 0) or 0
            if not self.ensure_attached() or a <= 0x10000:
                return False
            self._manual_player_addr = a
            self._last_hook_player = a
            self.mem.write_u64(self.hooks.sym_addr("ptrPlayer"), a)
            return True

    # ------------------------------------------------------------------
    # UNDO STACK (10-level full 0x17C snapshot per player)
    # ------------------------------------------------------------------
    def push_undo(self, p_addr: int, action_code: str) -> None:
        snap = self.mem.read_bytes(p_addr, PLAYER_RECORD_SIZE)
        if not snap or len(snap) < PLAYER_RECORD_SIZE:
            return
        st = self.undo_stack.setdefault(p_addr, [])
        prev_anc = dict(self._econ_anchors[p_addr]) if p_addr in self._econ_anchors else None
        st.append({"bytes": snap, "action": action_code, "time": time.strftime("%H:%M:%S"), "prev_anchor": prev_anc})
        if len(st) > UNDO_MAX_DEPTH:
            st.pop(0)

    def undo_last(self, p_addr: Optional[int] = None) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, cur_p, _ = self.sync_pointers()
            p = p_addr or cur_p
            st = self.undo_stack.get(p)
            if not st:
                return {"ok": False, "code": "nothing_to_undo"}
            entry = st.pop()
            self.mem.write_bytes(p, entry["bytes"])
            prev_anc = entry.get("prev_anchor")
            if prev_anc is not None:
                self._econ_anchors[p] = prev_anc
            else:
                self._econ_anchors.pop(p, None)
            self._wage_cache_time = 0.0
            return {"ok": True, "action": entry["action"], "remaining": len(st)}

    # ------------------------------------------------------------------
    # CAREER, CLUB & EXACT FINANCES
    # ------------------------------------------------------------------
    def decode_club_handle(self, raw_u32: int, career: int = 0) -> Tuple[int, str]:
        if raw_u32 == 0 or raw_u32 == 0xFFFFFFFF:
            return 0, ""
        c = career or self.mem.read_u64(self.hooks.sym_addr("ptrCareer"))
        key = (c, raw_u32)
        cached = self._club_name_cache.get(key)
        if cached is not None:
            return cached
        slot = raw_u32 % 16384
        team_id = raw_u32 // 16384
        res: Tuple[int, str]
        if c > 0x10000 and slot < 1200:
            club_entry = c + 0xADF4BC + (slot * 0x690)
            entry_h = self.mem.read_u32(club_entry)
            if entry_h == raw_u32 or (entry_h % 16384) == slot:
                live_name = self.mem.read_str(club_entry + 0x04, 64)
                abbr = self.mem.read_str(club_entry + 0x4A, 6)
                if live_name:
                    res = (team_id, f"{live_name} [{abbr}]" if abbr else live_name)
                    self._club_name_cache[key] = res
                    return res
        name = CLUB_TEAMS.get(team_id) or f"Club #{team_id}"
        res = (team_id, name)
        self._club_name_cache[key] = res
        return res

    def get_user_club_handle(self, career: int) -> int:
        if not career:
            return 0
        uc = self.mem.read_u32(career + 0x01642A2C)
        if uc and uc != 0xFFFFFFFF:
            return uc
        h0 = self.mem.read_u64(career + 0x016EE854)
        if (h0 >> 32) != 0:
            lo = h0 & 0xFFFFFFFF
            if lo < 30000:
                c0 = self.mem.read_u32(career + lo * PLAYER_RECORD_SIZE + 0x12C)
                if c0 and c0 != 0xFFFFFFFF:
                    return c0
        bal_h = self.mem.read_u64(career + 0x01819540)
        if (bal_h >> 32) != 0:
            lo = bal_h & 0xFFFFFFFF
            if lo < 30000:
                c1 = self.mem.read_u32(career + lo * PLAYER_RECORD_SIZE + 0x12C)
                if c1 and c1 != 0xFFFFFFFF:
                    return c1
        return 0

    def get_user_squad_addresses(self, career: int, force_rescan: bool = False) -> List[int]:
        uc = self.get_user_club_handle(career)
        if not uc:
            return []
        now = time.monotonic()
        sc = self._squad_cache
        need = (
            force_rescan
            or not sc
            or sc["career"] != career
            or sc["uc"] != uc
            or (now - sc["time"]) > 30.0
        )
        if not need and sc:
            for a in sc["addrs"]:
                if self.mem.read_u32(a + 0x12C) != uc:
                    need = True
                    break
        if need:
            count = self.mem.read_u32(career + 0x00D0BCE8)
            if count == 0 or count > 30001:
                count = 30000
            raw = self.mem.read_bytes(career, count * PLAYER_RECORD_SIZE)
            addrs: List[int] = []
            if raw:
                for i in range(count):
                    off = i * PLAYER_RECORD_SIZE
                    club_h = struct.unpack_from("<I", raw, off + 0x12C)[0]
                    if club_h == uc:
                        pid = struct.unpack_from("<I", raw, off + 0x30)[0]
                        if pid != 0 and pid != 0xFFFFFFFF:
                            addrs.append(career + off)
            sc = {"career": career, "uc": uc, "time": now, "addrs": addrs}
            self._squad_cache = sc
        return sc["addrs"] if sc else []

    def get_available_salary_budget(
        self, career: int, force_refresh: bool = False
    ) -> Tuple[int, int, int]:
        """Exact Konami 0x14156D170 formula: Gross (0x016ECC08) - Active Squad Wages - Pending Overrides."""
        if not career:
            return 0, 0, 0
        gross = self.mem.read_i32(career + 0x016ECC08)
        now = time.monotonic()
        if (
            not force_refresh
            and self._wage_cache_career == career
            and (now - self._wage_cache_time) < 2.0
        ):
            return gross - self._wage_cache_sum, gross, self._wage_cache_sum

        overrides: Dict[int, int] = {}
        ov_sum = 0
        ov_raw = self.mem.read_bytes(career + 0x016F0764, 34 * 12)
        if ov_raw:
            for i in range(34):
                ph, ov = struct.unpack_from("<Qi", ov_raw, i * 12)
                if ph != 0 and (ph & 0xFFFF) != 0xFFFF and (ph >> 32) != 0:
                    overrides[ph] = ov
                    ov_sum += ov

        addrs = self.get_user_squad_addresses(career, force_rescan=force_refresh)
        sum_sal = 0
        seen = set()
        for p in addrs:
            ph = self.mem.read_u64(p + 0x2C)
            if ph not in seen:
                seen.add(ph)
                if ph not in overrides:
                    sum_sal += self.mem.read_i32(p + 0x15C)
        sum_sal += ov_sum
        self._wage_cache_career = career
        self._wage_cache_time = now
        self._wage_cache_sum = sum_sal
        return gross - sum_sal, gross, sum_sal

    def get_finances(self) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            career, _, _ = self.sync_pointers()
            if not career:
                return {"ok": False, "code": "no_career"}
            trf = self.mem.read_i32(career + 0x016ECBF4)
            avail, gross, wages = self.get_available_salary_budget(career, force_refresh=True)
            return {
                "ok": True,
                "transfer_raw": trf,
                "salary_avail_raw": avail,
                "salary_gross_raw": gross,
                "squad_wages_raw": wages,
            }

    def set_finances(self, transfer_str: Optional[str], salary_avail_str: Optional[str]) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            career, _, _ = self.sync_pointers()
            if not career:
                return {"ok": False, "code": "no_career"}
            t_raw = None
            if transfer_str is not None and str(transfer_str).strip() != "":
                t_raw = parse_human_money_to_raw(transfer_str)
                if t_raw is None:
                    return {"ok": False, "code": "invalid_money"}
            w_raw = None
            if salary_avail_str is not None and str(salary_avail_str).strip() != "":
                w_raw = parse_human_money_to_raw(salary_avail_str)
                if w_raw is None:
                    return {"ok": False, "code": "invalid_money"}
            if t_raw is not None:
                self.mem.write_i32(career + 0x016ECBF4, t_raw)
            if w_raw is not None:
                _, _, wages = self.get_available_salary_budget(career, force_refresh=True)
                new_gross = max(0, min(2_147_483_647, w_raw + wages))
                self.mem.write_i32(career + 0x016ECC08, new_gross)
            return self.get_finances()

    def add_finance_delta(self, kind: str, raw_delta: int) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            career, _, _ = self.sync_pointers()
            if not career:
                return {"ok": False, "code": "no_career"}
            delta = _safe_int(raw_delta, 0) or 0
            off = 0x016ECBF4 if kind == "transfer" else 0x016ECC08
            cur = self.mem.read_i32(career + off)
            nv = max(0, min(2_147_483_647, cur + delta))
            self.mem.write_i32(career + off, nv)
            return self.get_finances()

    # ------------------------------------------------------------------
    # LIVE STATUS SNAPSHOT (Polled every 600ms by UI)
    # ------------------------------------------------------------------
    def get_status(self) -> Dict[str, Any]:
        with self.lock:
            attached = self.ensure_attached()
            if not attached:
                return {
                    "attached": False,
                    "access_denied": self.mem.access_denied,
                    "exe": "",
                    "pid": 0,
                    "mode_code": "offline",
                    "club": "",
                    "career": 0,
                    "in_match": False,
                    "player": None,
                    "finances": None,
                    "hooks": {},
                    "training_mode": 0,
                    "stamina_mode": 0,
                    "freeze_clock": False,
                    "bal_focus": 0,
                    "bal_lock": False,
                }
            career, p_addr, match_state = self.sync_pointers()
            mode_code = "menu"
            club_name = ""
            finances = None
            bal_focus = 0
            if career > 0x10000:
                bal_h = self.mem.read_u64(career + 0x01819540)
                mode_code = "bal" if (bal_h >> 32) != 0 else "ml"
                uc = self.get_user_club_handle(career)
                if uc:
                    _, club_name = self.decode_club_handle(uc, career)
                trf = self.mem.read_i32(career + 0x016ECBF4)
                avail, _, wages = self.get_available_salary_budget(career, force_refresh=False)
                finances = {
                    "transfer_raw": trf,
                    "salary_avail_raw": avail,
                    "squad_wages_raw": wages,
                }
                bal_focus = self.mem.read_u8(career + 0x016ECBEA)

            player_summary = None
            if p_addr > 0x10000:
                buf = self.mem.read_bytes(p_addr, PLAYER_RECORD_SIZE)
                if buf and len(buf) == PLAYER_RECORD_SIZE:
                    ovr, reg_pos = calc_ovr_from_buf(buf)
                    name = buf[0x38:0x66].split(b"\x00", 1)[0].decode("utf-8", errors="replace").strip()
                    age = buf[0x1C] & 0x3F
                    stam = _get_bits_buf(buf, 0x146, 0, 7)
                    arrow = min(4, _get_bits_buf(buf, 0x147, 0, 3))
                    inj = buf[0x148]
                    club_h = struct.unpack_from("<I", buf, 0x12C)[0]
                    _, p_club = self.decode_club_handle(club_h, career)
                    player_summary = {
                        "addr": p_addr,
                        "name": name or "?",
                        "ovr": ovr,
                        "pos": reg_pos,
                        "age": age,
                        "stamina": stam,
                        "arrow": arrow,
                        "injury": inj,
                        "club": p_club,
                        "undo_count": len(self.undo_stack.get(p_addr, [])),
                    }

            return {
                "attached": True,
                "access_denied": False,
                "exe": self.mem.exe_name,
                "pid": self.mem.pid,
                "mode_code": mode_code,
                "club": club_name,
                "career": career,
                "in_match": match_state > 0x10000,
                "player": player_summary,
                "finances": finances,
                "hooks": dict(self.hooks.active_hooks),
                "training_mode": self.mem.read_u32(self.hooks.sym_addr("cfgTrainingMultMode")),
                "stamina_mode": self.mem.read_u32(self.hooks.sym_addr("cfgStaminaMode")),
                "freeze_clock": self.mem.read_u32(self.hooks.sym_addr("cfgFreezeClock")) == 1,
                "bal_focus": bal_focus,
                "bal_lock": self.mem.read_u32(self.hooks.sym_addr("cfgLockBALFocusPoints")) == 1,
            }

    # ------------------------------------------------------------------
    # FULL PLAYER DETAILS & EDITING
    # ------------------------------------------------------------------
    def get_player_details(self, addr: Optional[int] = None) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            career, cur_p, _ = self.sync_pointers()
            p = _safe_int(addr) or cur_p
            if not p or p <= 0x10000:
                return {"ok": False, "code": "no_player"}
            buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
            if not buf or len(buf) < PLAYER_RECORD_SIZE:
                return {"ok": False, "code": "read_failed"}

            ovr, reg_pos = calc_ovr_from_buf(buf)
            name = buf[0x38:0x66].split(b"\x00", 1)[0].decode("utf-8", errors="replace").strip()
            shirt = buf[0x75:0x87].split(b"\x00", 1)[0].decode("utf-8", errors="replace").strip()
            pid = struct.unpack_from("<I", buf, 0x30)[0]
            club_h = struct.unpack_from("<I", buf, 0x12C)[0]
            _, club_name = self.decode_club_handle(club_h, career)
            nat_id = struct.unpack_from("<H", buf, 0x144)[0] & 0x1FF
            age = buf[0x1C] & 0x3F
            height = buf[0x00]
            weight = buf[0x01]
            foot = (buf[0x03] >> 7) & 1
            playing_style = _get_bits_buf(buf, 0x1D, 3, 5)
            wf_usage = _get_bits_buf(buf, 0x23, 0, 2) + 1
            wf_acc = _get_bits_buf(buf, 0x22, 4, 2) + 1
            form_cons = _get_bits_buf(buf, 0x17, 4, 3) + 1
            inj_res = _get_bits_buf(buf, 0x22, 6, 2) + 1
            stamina = _get_bits_buf(buf, 0x146, 0, 7)
            arrow = min(4, _get_bits_buf(buf, 0x147, 0, 3))
            injury = buf[0x148]
            contract_yr = struct.unpack_from("<H", buf, 0x138)[0]
            salary_raw = struct.unpack_from("<i", buf, 0x15C)[0]
            market_raw = struct.unpack_from("<i", buf, 0x174)[0]
            has_release_fee = _get_bits_buf(buf, 0x147, 6, 1) == 1
            affection = (struct.unpack_from("<I", buf, 0x13C)[0] >> 18) & 0x7F
            snap_age = buf[0x178]
            snap_ovr = buf[0x179]
            release_fee_raw = econ_model.calc_konami_release_fee_raw(
                market_raw, has_release_fee, age, affection, ovr
            )
            team_role = _get_bits_buf(buf, 0x150, 0, 5)
            team_role_lvl = _get_bits_buf(buf, 0x143, 6, 2)
            transfer_listed = _get_bits_buf(buf, 0x14A, 1, 1) == 1
            loan_listed = _get_bits_buf(buf, 0x14A, 2, 1) == 1

            stats = {str(s["fid"]): get_fid_from_buf(buf, s["fid"]) for s in STAT_DEFS}

            positions = []
            for pos_idx in range(13):
                p_ovr, _ = calc_ovr_from_buf(buf, pos_idx, False)
                apt = get_apt_from_buf(buf, pos_idx)
                positions.append(
                    {"id": pos_idx, "ovr": p_ovr, "apt": apt, "is_reg": pos_idx == reg_pos}
                )

            skills = {
                sk["key"]: _get_bits_buf(buf, sk["offset"], sk["bit"], 1) == 1
                for sk in PLAYER_SKILLS
            }
            com_ai = {
                ai["key"]: _get_bits_buf(buf, ai["offset"], ai["bit"], 1) == 1
                for ai in COM_AI_STYLES
            }

            return {
                "ok": True,
                "addr": p,
                "pid": pid,
                "name": name,
                "shirt": shirt,
                "club": club_name,
                "ovr": ovr,
                "reg_pos": reg_pos,
                "nationality": nat_id,
                "age": age,
                "height": height,
                "weight": weight,
                "foot": foot,
                "playing_style": playing_style,
                "wf_usage": wf_usage,
                "wf_acc": wf_acc,
                "form_cons": form_cons,
                "inj_res": inj_res,
                "stamina": stamina,
                "arrow": arrow,
                "injury": injury,
                "contract_yr": contract_yr,
                "salary_raw": salary_raw,
                "market_raw": market_raw,
                "has_release_fee": has_release_fee,
                "release_fee_raw": release_fee_raw,
                "affection": affection,
                "snap_age": snap_age,
                "snap_ovr": snap_ovr,
                "team_role": team_role,
                "team_role_lvl": team_role_lvl,
                "transfer_listed": transfer_listed,
                "loan_listed": loan_listed,
                "stats": stats,
                "positions": positions,
                "skills": skills,
                "com_ai": com_ai,
                "undo_count": len(self.undo_stack.get(p, [])),
            }

    def set_player_quick_condition(
        self,
        stamina: Optional[int] = None,
        arrow: Optional[int] = None,
        heal: bool = False,
        addr: Optional[int] = None,
    ) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, cur_p, _ = self.sync_pointers()
            p = _safe_int(addr) or cur_p
            if not p:
                return {"ok": False, "code": "no_player"}
            self.push_undo(p, "condition")
            st_val = _safe_int(stamina)
            if st_val is not None:
                self.mem.set_bits(p + 0x146, 0, 7, max(0, min(100, st_val)))
            ar_val = _safe_int(arrow)
            if ar_val is not None:
                a = max(0, min(4, ar_val))
                self.mem.set_bits(p + 0x147, 0, 3, a)
                self.mem.set_bits(p + 0x146, 7, 1, 1 if a == 4 else 0)
            if heal:
                self.mem.write_u8(p + 0x148, 0)
            return self.get_player_details(p)

    def update_player_bio(self, payload: Dict[str, Any]) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            if not p or not isinstance(payload, dict):
                return {"ok": False, "code": "no_player"}
            # Validate money strings BEFORE pushing undo or writing any bytes to RAM (atomic validation)
            sr = None
            if payload.get("salary_str") is not None and str(payload["salary_str"]).strip() != "":
                sr = parse_human_money_to_raw(str(payload["salary_str"]))
                if sr is None:
                    return {"ok": False, "code": "invalid_money"}
            mr = None
            if payload.get("market_str") is not None and str(payload["market_str"]).strip() != "":
                mr = parse_human_money_to_raw(str(payload["market_str"]))
                if mr is None:
                    return {"ok": False, "code": "invalid_money"}

            self.push_undo(p, "bio")
            if payload.get("name") is not None and str(payload["name"]).strip() != "":
                self.mem.write_str(p + 0x38, str(payload["name"]).strip(), 46)
            if payload.get("shirt") is not None:
                self.mem.write_str(p + 0x75, str(payload["shirt"]).strip().upper(), 18)
            nat = _safe_int(payload.get("nationality"))
            if nat is not None:
                cur = self.mem.read_u16(p + 0x144)
                self.mem.write_u16(p + 0x144, (cur & ~0x1FF) | (nat & 0x1FF))
            age = _safe_int(payload.get("age"))
            if age is not None:
                self.mem.set_bits(p + 0x1C, 0, 6, max(15, min(50, age)))
            height = _safe_int(payload.get("height"))
            if height is not None:
                self.mem.write_u8(p + 0x00, max(145, min(215, height)))
            weight = _safe_int(payload.get("weight"))
            if weight is not None:
                self.mem.write_u8(p + 0x01, max(45, min(125, weight)))
            foot = _safe_int(payload.get("foot"))
            if foot is not None:
                self.mem.set_bits(p + 0x03, 7, 1, 1 if foot else 0)
            cyr = _safe_int(payload.get("contract_yr"))
            if cyr is not None:
                self.mem.write_u16(p + 0x138, max(2019, min(2150, cyr)))
            if sr is not None:
                self.mem.write_i32(p + 0x15C, sr)
                self._wage_cache_time = 0.0
            if mr is not None:
                self.mem.write_i32(p + 0x174, mr)
                cur_buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
                if cur_buf:
                    cur_ovr, _ = calc_ovr_from_buf(cur_buf)
                    self.mem.write_bytes(
                        p + 0x178,
                        bytes([cur_buf[0x1C] & 0x3F, max(40, min(99, cur_ovr))]),
                    )
            if "has_release_fee" in payload and payload["has_release_fee"] is not None:
                self.mem.set_bits(p + 0x147, 6, 1, 1 if payload["has_release_fee"] else 0)
            if sr is not None or mr is not None or age is not None or nat is not None:
                self._econ_anchors.pop(p, None)
            if "transfer_listed" in payload and payload["transfer_listed"] is not None:
                self.mem.set_bits(p + 0x14A, 1, 1, 1 if payload["transfer_listed"] else 0)
            if "loan_listed" in payload and payload["loan_listed"] is not None:
                self.mem.set_bits(p + 0x14A, 2, 1, 1 if payload["loan_listed"] else 0)
            wfu = _safe_int(payload.get("wf_usage"))
            if wfu is not None:
                self.mem.set_bits(p + 0x23, 0, 2, max(0, min(3, wfu - 1)))
            wfa = _safe_int(payload.get("wf_acc"))
            if wfa is not None:
                self.mem.set_bits(p + 0x22, 4, 2, max(0, min(3, wfa - 1)))
            fc = _safe_int(payload.get("form_cons"))
            if fc is not None:
                self.mem.set_bits(p + 0x17, 4, 3, max(0, min(7, fc - 1)))
            ir = _safe_int(payload.get("inj_res"))
            if ir is not None:
                self.mem.set_bits(p + 0x22, 6, 2, max(0, min(2, ir - 1)))
            inj = _safe_int(payload.get("injury"))
            if inj is not None:
                self.mem.write_u8(p + 0x148, max(0, min(255, inj)))
            return self.get_player_details(p)

    def get_econ_sync_mode(self) -> int:
        with self.lock:
            return self.econ_sync_mode

    def set_econ_sync_mode(self, mode: int) -> Dict[str, Any]:
        with self.lock:
            m = _safe_int(mode, econ_model.ECON_MODE_ALL)
            self.econ_sync_mode = max(0, min(3, 1 if m is None else m))
            return {"ok": True, "mode": self.econ_sync_mode}

    def _apply_economy_sync_locked(self, p: int, before_buf: bytes) -> Dict[str, Any]:
        mode = self.econ_sync_mode
        delta: Dict[str, Any] = {"applied": False, "mode": mode}
        if mode == econ_model.ECON_MODE_OFF or not before_buf or len(before_buf) < PLAYER_RECORD_SIZE:
            return delta
        after_buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
        if not after_buf or len(after_buf) < PLAYER_RECORD_SIZE:
            return delta

        old_ovr, old_pos = calc_ovr_from_buf(before_buf)
        new_ovr, new_pos = calc_ovr_from_buf(after_buf)
        if new_ovr == old_ovr:
            return delta

        pid = struct.unpack_from("<I", before_buf, 0x30)[0]
        old_age = before_buf[0x1C] & 0x3F
        new_age = after_buf[0x1C] & 0x3F
        old_nat = struct.unpack_from("<H", before_buf, 0x144)[0] & 0x1FF
        new_nat = struct.unpack_from("<H", after_buf, 0x144)[0] & 0x1FF
        cur_mkt = struct.unpack_from("<i", before_buf, 0x174)[0]
        cur_sal = struct.unpack_from("<i", before_buf, 0x15C)[0]
        has_rf = _get_bits_buf(before_buf, 0x147, 6, 1) == 1
        aff = (struct.unpack_from("<I", before_buf, 0x13C)[0] >> 18) & 0x7F

        anc = self._econ_anchors.get(p)
        if not anc or anc.get("pid") != pid:
            anc = {
                "pid": pid,
                "ovr": old_ovr,
                "age": old_age,
                "pos": old_pos,
                "nat": old_nat,
                "market_raw": cur_mkt,
                "salary_raw": cur_sal,
                "has_release_fee": has_rf,
                "release_fee_raw": econ_model.calc_konami_release_fee_raw(cur_mkt, has_rf, old_age, aff, old_ovr),
            }
            self._econ_anchors[p] = anc

        old_rel = econ_model.calc_konami_release_fee_raw(cur_mkt, has_rf, old_age, aff, old_ovr)
        new_mkt = cur_mkt
        new_sal = cur_sal
        new_rel = old_rel

        do_market = mode in (econ_model.ECON_MODE_ALL, econ_model.ECON_MODE_MARKET_RELEASE)
        do_salary = mode in (econ_model.ECON_MODE_ALL, econ_model.ECON_MODE_SALARY_ONLY)

        if do_market:
            new_mkt = econ_model.scale_economy_value(
                anc["market_raw"], anc["ovr"], anc["age"], anc["pos"], anc["nat"],
                new_ovr, new_age, new_pos, new_nat, False,
            )
            self.mem.write_i32(p + 0x174, new_mkt)
            self.mem.write_bytes(p + 0x178, bytes([max(15, min(63, new_age)), max(40, min(99, new_ovr))]))
            new_rel = econ_model.calc_konami_release_fee_raw(new_mkt, has_rf, new_age, aff, new_ovr)

        if do_salary:
            new_sal = econ_model.scale_economy_value(
                anc["salary_raw"], anc["ovr"], anc["age"], anc["pos"], anc["nat"],
                new_ovr, new_age, new_pos, new_nat, True,
            )
            self.mem.write_i32(p + 0x15C, new_sal)
            self._wage_cache_time = 0.0

        applied = (new_mkt != cur_mkt) or (new_sal != cur_sal) or (new_rel != old_rel)
        return {
            "applied": applied,
            "mode": mode,
            "old_ovr": old_ovr,
            "new_ovr": new_ovr,
            "old_market_raw": cur_mkt,
            "new_market_raw": new_mkt,
            "old_salary_raw": cur_sal,
            "new_salary_raw": new_sal,
            "old_release_raw": old_rel,
            "new_release_raw": new_rel,
            "has_release_fee": has_rf,
        }

    def recalibrate_player_economy(self, addr: Optional[int] = None) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, cur_p, _ = self.sync_pointers()
            p = _safe_int(addr) or cur_p
            if not p or p <= 0x10000:
                return {"ok": False, "code": "no_player"}
            buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
            if not buf or len(buf) < PLAYER_RECORD_SIZE:
                return {"ok": False, "code": "read_failed"}

            cur_ovr, reg_pos = calc_ovr_from_buf(buf)
            age = buf[0x1C] & 0x3F
            nat = struct.unpack_from("<H", buf, 0x144)[0] & 0x1FF
            old_mkt = struct.unpack_from("<i", buf, 0x174)[0]
            old_sal = struct.unpack_from("<i", buf, 0x15C)[0]
            has_rf = _get_bits_buf(buf, 0x147, 6, 1) == 1
            aff = (struct.unpack_from("<I", buf, 0x13C)[0] >> 18) & 0x7F
            snap_age = buf[0x178]
            snap_ovr = buf[0x179]

            # Deterministic, idempotent recalibration directly from V3 calibrated curves
            new_mkt = econ_model.round_pes_money(econ_model.ref_market_raw(cur_ovr, age, reg_pos, nat), False)
            new_sal = econ_model.round_pes_money(econ_model.ref_salary_raw(cur_ovr, age, reg_pos, nat), True)

            self.push_undo(p, "econ_recal")
            mode = econ_model.ECON_MODE_ALL if self.econ_sync_mode == econ_model.ECON_MODE_OFF else self.econ_sync_mode
            old_rel = econ_model.calc_konami_release_fee_raw(old_mkt, has_rf, age, aff, cur_ovr)
            fin_mkt, fin_sal, fin_rel = old_mkt, old_sal, old_rel

            if mode in (econ_model.ECON_MODE_ALL, econ_model.ECON_MODE_MARKET_RELEASE):
                self.mem.write_i32(p + 0x174, new_mkt)
                self.mem.write_bytes(p + 0x178, bytes([max(15, min(63, age)), max(40, min(99, cur_ovr))]))
                fin_mkt = new_mkt
                fin_rel = econ_model.calc_konami_release_fee_raw(new_mkt, has_rf, age, aff, cur_ovr)
            if mode in (econ_model.ECON_MODE_ALL, econ_model.ECON_MODE_SALARY_ONLY):
                self.mem.write_i32(p + 0x15C, new_sal)
                self._wage_cache_time = 0.0
                fin_sal = new_sal

            self._econ_anchors.pop(p, None)
            res = self.get_player_details(p)
            res["econ_delta"] = {
                "applied": True,
                "mode": mode,
                "old_ovr": snap_ovr if (40 <= snap_ovr <= 99) else cur_ovr,
                "new_ovr": cur_ovr,
                "old_market_raw": old_mkt,
                "new_market_raw": fin_mkt,
                "old_salary_raw": old_sal,
                "new_salary_raw": fin_sal,
                "old_release_raw": old_rel,
                "new_release_raw": fin_rel,
                "has_release_fee": has_rf,
            }
            return res

    def set_player_stat(self, fid: int, val: int) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            f = _safe_int(fid, -1)
            v = _safe_int(val)
            if not p or f not in FID_LOC or v is None:
                return {"ok": False, "code": "invalid_arg"}
            before_buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
            if not before_buf:
                return {"ok": False, "code": "read_failed"}
            self.push_undo(p, "stat")
            off, bs = FID_LOC[f]
            clamped = max(40, min(99, v))
            if off == 0x03:
                cur = self.mem.read_u8(p + 3)
                self.mem.write_u8(p + 3, (cur & 0x80) | (clamped & 0x7F))
            else:
                self.mem.set_bits(p + off, bs, 7, clamped)
            ed = self._apply_economy_sync_locked(p, before_buf)
            res = self.get_player_details(p)
            res["econ_delta"] = ed
            return res

    def set_player_stats_batch(self, stats_map: Dict[str, int]) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            if not p or not isinstance(stats_map, dict):
                return {"ok": False, "code": "no_player"}
            buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
            if not buf:
                return {"ok": False, "code": "read_failed"}
            self.push_undo(p, "stats_batch")
            ba = bytearray(buf)
            for k, v in stats_map.items():
                f = _safe_int(k, -1)
                iv = _safe_int(v)
                if f in FID_LOC and iv is not None:
                    set_fid_in_buf(ba, f, iv)
            self.mem.write_bytes(p, bytes(ba[:0x28]))
            ed = self._apply_economy_sync_locked(p, buf)
            res = self.get_player_details(p)
            res["econ_delta"] = ed
            return res

    def set_player_skill(self, skill_key: str, enabled: bool) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            if not p:
                return {"ok": False, "code": "no_player"}
            for item in PLAYER_SKILLS + COM_AI_STYLES:
                if item["key"] == skill_key:
                    self.push_undo(p, "skill")
                    self.mem.set_bits(p + item["offset"], item["bit"], 1, 1 if enabled else 0)
                    return self.get_player_details(p)
            return {"ok": False, "code": "invalid_arg"}

    def set_playing_style(self, style_id: int) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            sid = _safe_int(style_id)
            if not p or sid is None:
                return {"ok": False, "code": "no_player"}
            self.push_undo(p, "style")
            self.mem.set_bits(p + 0x1D, 3, 5, max(0, min(21, sid)))
            return self.get_player_details(p)

    def set_team_role(self, role_id: int, level: int) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            rid = _safe_int(role_id)
            lvl = _safe_int(level)
            if not p or rid is None or lvl is None:
                return {"ok": False, "code": "no_player"}
            self.push_undo(p, "team_role")
            self.mem.set_bits(p + 0x150, 0, 5, max(0, min(22, rid)))
            self.mem.set_bits(p + 0x143, 6, 2, max(0, min(3, lvl)))
            return self.get_player_details(p)

    # ------------------------------------------------------------------
    # POSITION LAB (Strict 1-by-1 stepping & Zero Primary OVR Drift!)
    # ------------------------------------------------------------------
    def set_position_aptitude(self, pos_idx: int, grade: int) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            pi = _safe_int(pos_idx, -1)
            gr = _safe_int(grade, -1)
            if not p or pi not in POS_APT_LOC or gr not in (0, 1, 2):
                return {"ok": False, "code": "invalid_arg"}
            self.push_undo(p, "pos_grade")
            off, bs = POS_APT_LOC[pi]
            self.mem.set_bits(p + off, bs, 2, gr)
            return self.get_player_details(p)

    def set_registered_position(self, pos_idx: int) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            pi = _safe_int(pos_idx, -1)
            if not p or pi not in POS_APT_LOC:
                return {"ok": False, "code": "invalid_arg"}
            before_buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
            if not before_buf:
                return {"ok": False, "code": "read_failed"}
            self.push_undo(p, "reg_pos")
            self.mem.set_bits(p + 0x07, 4, 4, pi)
            off, bs = POS_APT_LOC[pi]
            self.mem.set_bits(p + off, bs, 2, 2)
            ed = self._apply_economy_sync_locked(p, before_buf)
            res = self.get_player_details(p)
            res["econ_delta"] = ed
            return res

    def _get_pos_weights(self, pos_idx: int) -> Dict[int, float]:
        target_code = POS_REMAP.get(pos_idx, 0)
        col = 0
        for ci, pc in enumerate(OVR_DATA["pos_order"]):
            if pc == target_code:
                col = ci
                break
        stat_indices = OVR_DATA["stat_indices_23"]
        weights_23x13 = OVR_DATA["weights_23x13"]
        slot_to_fid = {int(pair[1]): int(pair[0]) for pair in OVR_DATA["field_to_slot"]}
        out: Dict[int, float] = {}
        for row_i, s_idx in enumerate(stat_indices):
            w = float(weights_23x13[row_i][col])
            if w == 0.0:
                continue
            if s_idx == 0x21:
                # Composite GK1 (slots 33 & 37 -> GK Awareness & GK Catching)
                for gk_slot in (33, 37):
                    fid = slot_to_fid.get(gk_slot)
                    if fid is not None:
                        out[fid] = out.get(fid, 0.0) + w * 0.5
            elif s_idx == 0x22:
                # Composite GK2 (slots 38, 39, 40 -> GK Clearing, Reflexes, Reach)
                for gk_slot in (38, 39, 40):
                    fid = slot_to_fid.get(gk_slot)
                    if fid is not None:
                        out[fid] = out.get(fid, 0.0) + w / 3.0
            else:
                fid = slot_to_fid.get(s_idx)
                if fid is not None:
                    out[fid] = out.get(fid, 0.0) + w
        return out

    @staticmethod
    def _compute_stat_diffs(before_buf: BufLike, after_buf: BufLike) -> List[Dict[str, int]]:
        diffs: List[Dict[str, int]] = []
        for s in STAT_DEFS:
            f = s["fid"]
            ov = get_fid_from_buf(before_buf, f)
            nv = get_fid_from_buf(after_buf, f)
            if ov != nv:
                diffs.append({"fid": f, "old": ov, "new": nv})
        diffs.sort(key=lambda d: (0 if d["new"] > d["old"] else 1, -abs(d["new"] - d["old"]), d["fid"]))
        return diffs

    def _scale_primary_ovr(self, p: int, target_ovr: int) -> Dict[str, Any]:
        """Proportional DNA-Preserving Overall Solver (Hamilton Largest-Remainder + Core DNA Refinement)."""
        buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
        if not buf:
            return {"action_ok": False, "code": "read_failed"}
        cur_ovr, reg_pos = calc_ovr_from_buf(buf)
        target = max(40, min(109, int(target_ovr)))
        if cur_ovr == target:
            return {"action_ok": True, "code": "already_at_ovr", "ovr": cur_ovr, "stat_diffs": []}

        self.push_undo(p, "ovr_dna")

        active_fids: List[int] = []
        if reg_pos == 0:
            active_fids.extend(GK_FIDS)
            for f in OUTFIELD_FIDS:
                if get_fid_from_buf(buf, f) > 43:
                    active_fids.append(f)
        else:
            active_fids.extend(OUTFIELD_FIDS)

        base_stats = {f: get_fid_from_buf(buf, f) for f in active_fids}
        weights = {f: max(4, base_stats[f] - 30) for f in active_fids}
        total_w = float(sum(weights.values())) or 1.0

        direction = 1 if target > cur_ovr else -1
        history: List[bytearray] = [bytearray(buf[:0x28])]
        best_ba = bytearray(buf[:0x28])
        best_ovr = cur_ovr

        def _commit(final_ba: bytearray) -> Dict[str, Any]:
            self.mem.write_bytes(p, bytes(final_ba[:0x28]))
            ed = self._apply_economy_sync_locked(p, buf)
            fin_ovr, _ = calc_ovr_from_buf(final_ba, reg_pos, False)
            return {
                "action_ok": True,
                "code": "pri_updated",
                "pos": reg_pos,
                "reg_pos": reg_pos,
                "orig": cur_ovr,
                "final": fin_ovr,
                "pri": fin_ovr,
                "stat_diffs": self._compute_stat_diffs(buf[:0x28], final_ba[:0x28]),
                "econ_delta": ed,
            }

        for k in range(1, len(active_fids) * 60 + 1):
            cand = bytearray(buf[:0x28])
            floors: Dict[int, int] = {}
            rems = []
            sum_fl = 0
            for f in active_fids:
                q = (k * weights[f]) / total_w
                fl = int(q)
                floors[f] = fl
                sum_fl += fl
                rems.append((q - fl, weights[f], f))
            rems.sort(key=lambda x: (x[0], x[1]), reverse=True)
            rem_need = k - sum_fl
            for idx in range(min(rem_need, len(rems))):
                floors[rems[idx][2]] += 1

            for f in active_fids:
                set_fid_in_buf(cand, f, base_stats[f] + direction * floors[f])

            new_ovr, _ = calc_ovr_from_buf(cand, reg_pos, False)
            best_ba, best_ovr = cand, new_ovr
            if new_ovr == target:
                return _commit(cand)

            if (direction == 1 and new_ovr > target) or (direction == -1 and new_ovr < target):
                # Try trimming 1 point from the lowest-remainder stat to hit exact target
                for idx in range(len(rems) - 1, -1, -1):
                    tf = rems[idx][2]
                    if get_fid_from_buf(cand, tf) != base_stats[tf]:
                        alt = bytearray(cand)
                        set_fid_in_buf(alt, tf, get_fid_from_buf(alt, tf) - direction)
                        if calc_ovr_from_buf(alt, reg_pos, False)[0] == target:
                            return _commit(alt)

                # Core DNA refinement from recent history steps
                w_pos = self._get_pos_weights(reg_pos)
                dna_ranked = sorted(
                    active_fids,
                    key=lambda f: (w_pos.get(f, 0.0) + 4.0) * weights[f],
                    reverse=True,
                )
                for prev in reversed(history[-10:]):
                    trial = bytearray(prev)
                    for f in dna_ranked:
                        cv = get_fid_from_buf(trial, f)
                        nv = max(40, min(99, cv + direction))
                        if nv != cv:
                            set_fid_in_buf(trial, f, nv)
                            to = calc_ovr_from_buf(trial, reg_pos, False)[0]
                            if to == target:
                                return _commit(trial)
                            if (direction == 1 and to > target) or (direction == -1 and to < target):
                                set_fid_in_buf(trial, f, cv)

                return _commit(cand)

            history.append(cand)

        return _commit(best_ba)

    def _boost_secondary_isolated(self, p: int, target_pos: int, target_ovr: int) -> Dict[str, Any]:
        buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
        if not buf:
            return {"action_ok": False, "code": "read_failed"}
        orig_pri, reg_pos = calc_ovr_from_buf(buf)
        if target_pos == reg_pos:
            return self._scale_primary_ovr(p, target_ovr)

        orig_sec, _ = calc_ovr_from_buf(buf, target_pos, False)
        target = max(40, min(99, int(target_ovr)))
        if orig_sec == target:
            return {"action_ok": True, "code": "already_at_ovr", "ovr": orig_sec}

        direction = 1 if target > orig_sec else -1
        w_pri = self._get_pos_weights(reg_pos)
        w_sec = self._get_pos_weights(target_pos)
        fids = GK_FIDS if target_pos == 0 else OUTFIELD_FIDS

        # Exact Konami 0x141E640A0 sensitivity:
        # Primary OVR = 60% position weighted score (u1) + 40% curve_100(avg22)
        # Secondary OVR = 100% position weighted score (u1) + Grade bonus (A:+4/+5, B:+2/+3, C:0), 0% avg22
        up_base = sorted(
            [f for f in fids if w_sec.get(f, 0.0) > 0.0],
            key=lambda f: (w_sec.get(f, 0.0) - 0.6 * w_pri.get(f, 0.0), w_sec.get(f, 0.0)),
            reverse=True,
        ) or list(fids)

        # Counterbalance pool: stats that move Primary (via w_pri or avg22) with w_sec < w_sec[f_up].
        # Priority: 1) w_sec == 0 & w_pri > 0 (pure primary), 2) w_sec == 0 & w_pri == 0 (pure avg22), 3) w_pri > w_sec
        cb_pool = list(OUTFIELD_FIDS) if reg_pos != 0 else list(GK_FIDS) + list(OUTFIELD_FIDS)
        cb_ranked = sorted(
            cb_pool,
            key=lambda f: (
                1 if w_sec.get(f, 0.0) == 0.0 else 0,
                w_pri.get(f, 0.0) - w_sec.get(f, 0.0),
                -w_sec.get(f, 0.0),
            ),
            reverse=True,
        )

        ba = bytearray(buf)
        best_ba = bytearray(buf)
        best_sec = orig_sec

        for _ in range(1500):
            cur_sec, _ = calc_ovr_from_buf(ba, target_pos, False)
            if cur_sec == target:
                break
            moved = False

            # Natural spread: prefer strong secondary advantage stats with room to grow
            up_ranked = sorted(
                up_base,
                key=lambda f: (
                    (w_sec.get(f, 0.0) - 0.6 * w_pri.get(f, 0.0)) * 2.0
                    - (get_fid_from_buf(ba, f) * direction * 0.15)
                ),
                reverse=True,
            )

            # Stage 1: Pure single-stat nudge (zero primary drift, zero counterbalance needed)
            for f in up_ranked:
                cv = get_fid_from_buf(ba, f)
                nv = cv + direction
                if not (40 <= nv <= 99):
                    continue
                set_fid_in_buf(ba, f, nv)
                tp, _ = calc_ovr_from_buf(ba, reg_pos, False)
                ts, _ = calc_ovr_from_buf(ba, target_pos, False)
                overshoot = (direction > 0 and ts > target) or (direction < 0 and ts < target)
                if tp == orig_pri and not overshoot:
                    moved = True
                    if (direction > 0 and ts > best_sec) or (direction < 0 and ts < best_sec):
                        best_sec = ts
                        best_ba = bytearray(ba)
                    break
                set_fid_in_buf(ba, f, cv)

            if moved:
                continue

            # Stage 2: Exact Konami multi-stat counterbalance (works even on identical mirror positions LE<->LD, PTE<->PTD, MLE<->MLD!)
            for f_up in up_ranked:
                cv_u = get_fid_from_buf(ba, f_up)
                nv_u = cv_u + direction
                if not (40 <= nv_u <= 99):
                    continue
                ws_u = w_sec.get(f_up, 0.0)
                snap = bytearray(ba[:0x28])
                set_fid_in_buf(ba, f_up, nv_u)
                tp, _ = calc_ovr_from_buf(ba, reg_pos, False)
                if tp == orig_pri:
                    ba[:0x28] = snap
                    continue
                cb_live = sorted(
                    [f for f in cb_ranked if f != f_up and w_sec.get(f, 0.0) < ws_u],
                    key=lambda f: (
                        1 if w_sec.get(f, 0.0) == 0.0 else 0,
                        (w_pri.get(f, 0.0) - w_sec.get(f, 0.0)) * 2.0 + (get_fid_from_buf(ba, f) * direction * 0.15),
                    ),
                    reverse=True,
                )
                fixed = False
                for _step in range(8):
                    step_done = False
                    for f_cb in cb_live:
                        cv_c = get_fid_from_buf(ba, f_cb)
                        nv_c = cv_c - direction
                        if not (40 <= nv_c <= 99):
                            continue
                        set_fid_in_buf(ba, f_cb, nv_c)
                        tp, _ = calc_ovr_from_buf(ba, reg_pos, False)
                        step_done = True
                        if tp == orig_pri:
                            fixed = True
                            break
                    if fixed or not step_done:
                        break
                if fixed:
                    ts, _ = calc_ovr_from_buf(ba, target_pos, False)
                    overshoot = (direction > 0 and ts > target) or (direction < 0 and ts < target)
                    if not overshoot and (direction * (ts - cur_sec) >= 0):
                        moved = True
                        if (direction > 0 and ts > best_sec) or (direction < 0 and ts < best_sec):
                            best_sec = ts
                            best_ba = bytearray(ba)
                        break
                ba[:0x28] = snap

            if not moved:
                break

        apt = get_apt_from_buf(best_ba, target_pos)
        needs_grade = direction > 0 and apt < 2
        if best_sec == orig_sec:
            return {
                "action_ok": False,
                "code": "iso_limit",
                "pos": target_pos,
                "reg_pos": reg_pos,
                "orig": orig_sec,
                "final": orig_sec,
                "pri": orig_pri,
                "needs_grade": needs_grade,
            }

        self.push_undo(p, "pos_ovr")
        self.mem.write_bytes(p, bytes(best_ba[:0x28]))
        ed = self._apply_economy_sync_locked(p, buf)
        return {
            "action_ok": True,
            "code": "pos_partial" if best_sec != target else "pos_updated",
            "pos": target_pos,
            "reg_pos": reg_pos,
            "orig": orig_sec,
            "final": best_sec,
            "pri": orig_pri,
            "needs_grade": needs_grade,
            "stat_diffs": self._compute_stat_diffs(buf[:0x28], best_ba[:0x28]),
            "econ_delta": ed,
        }

    def step_player_ovr(self, delta: int) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            if not p:
                return {"ok": False, "code": "no_player"}
            buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
            if not buf:
                return {"ok": False, "code": "read_failed"}
            cur_ovr, _ = calc_ovr_from_buf(buf)
            dl = _safe_int(delta, 0) or 0
            if dl > 0 and cur_ovr >= 99:
                info = {"action_ok": True, "code": "already_at_ovr", "ovr": cur_ovr}
            else:
                info = self._scale_primary_ovr(p, max(40, cur_ovr + dl))
            res = self.get_player_details(p)
            res.update(info)
            return res

    def set_player_ovr(self, target_ovr: int) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            if not p:
                return {"ok": False, "code": "no_player"}
            tgt = _safe_int(target_ovr)
            if tgt is None:
                return {"ok": False, "code": "invalid_arg"}
            info = self._scale_primary_ovr(p, max(40, min(99, tgt)))
            res = self.get_player_details(p)
            res.update(info)
            return res

    def step_position_ovr(self, pos_idx: int, delta: int) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            pi = _safe_int(pos_idx, -1)
            dl = _safe_int(delta, 0) or 0
            if not p or pi not in POS_APT_LOC:
                return {"ok": False, "code": "invalid_arg"}
            buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
            if not buf:
                return {"ok": False, "code": "read_failed"}
            cur_ovr, _ = calc_ovr_from_buf(buf, pi, False)
            info = self._boost_secondary_isolated(p, pi, cur_ovr + dl)
            res = self.get_player_details(p)
            res.update(info)
            return res

    def match_position_to_primary(self, pos_idx: int) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            pi = _safe_int(pos_idx, -1)
            if not p or pi not in POS_APT_LOC:
                return {"ok": False, "code": "invalid_arg"}
            buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
            if not buf:
                return {"ok": False, "code": "read_failed"}
            pri_ovr, _ = calc_ovr_from_buf(buf)
            info = self._boost_secondary_isolated(p, pi, pri_ovr)
            res = self.get_player_details(p)
            res.update(info)
            return res

    # ------------------------------------------------------------------
    # SQUAD ROSTER & SQUAD COMMANDER
    # ------------------------------------------------------------------
    def get_squad_roster(self) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected", "players": []}
            career, _, _ = self.sync_pointers()
            if not career:
                return {"ok": False, "code": "no_career", "players": []}
            cur_year = self.mem.read_u16(career + 0x01642A24)
            if cur_year < 2019 or cur_year > 2150:
                cur_year = 2026
            addrs = self.get_user_squad_addresses(career, force_rescan=True)
            players = []
            for p in addrs:
                buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
                if not buf:
                    continue
                ovr, reg_pos = calc_ovr_from_buf(buf)
                name = buf[0x38:0x66].split(b"\x00", 1)[0].decode("utf-8", errors="replace").strip()
                age = buf[0x1C] & 0x3F
                exp_y = struct.unpack_from("<H", buf, 0x138)[0]
                sal_raw = struct.unpack_from("<i", buf, 0x15C)[0]
                stam = _get_bits_buf(buf, 0x146, 0, 7)
                arrow = min(4, _get_bits_buf(buf, 0x147, 0, 3))
                inj = buf[0x148]
                players.append(
                    {
                        "addr": p,
                        "name": name or "?",
                        "ovr": ovr,
                        "pos": reg_pos,
                        "age": age,
                        "exp_y": exp_y if 2019 <= exp_y <= 2150 else 0,
                        "is_expiring": 2019 <= exp_y <= cur_year + 1,
                        "salary_raw": sal_raw,
                        "stamina": stam,
                        "arrow": arrow,
                        "injury": inj,
                    }
                )
            players.sort(key=lambda x: (-x["ovr"], x["name"]))
            uc = self.get_user_club_handle(career)
            _, club_name = self.decode_club_handle(uc, career)
            return {"ok": True, "club": club_name, "cur_year": cur_year, "players": players}

    def run_squad_commander(self, mode: str) -> Dict[str, Any]:
        with self.lock:
            if mode not in ("stamina", "arrow", "heal", "contract", "ovr1"):
                return {"ok": False, "code": "invalid_arg"}
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            career, _, _ = self.sync_pointers()
            if not career:
                return {"ok": False, "code": "no_career"}
            cur_year = self.mem.read_u16(career + 0x01642A24)
            if cur_year < 2019 or cur_year > 2150:
                cur_year = 2026
            addrs = self.get_user_squad_addresses(career, force_rescan=True)
            for p in addrs:
                if mode == "stamina":
                    self.push_undo(p, "squad_stamina")
                    self.mem.set_bits(p + 0x146, 0, 7, 100)
                elif mode == "arrow":
                    self.push_undo(p, "squad_arrow")
                    self.mem.set_bits(p + 0x147, 0, 3, 4)
                    self.mem.set_bits(p + 0x146, 7, 1, 1)
                elif mode == "heal":
                    self.push_undo(p, "squad_heal")
                    self.mem.write_u8(p + 0x148, 0)
                elif mode == "contract":
                    self.push_undo(p, "squad_contract")
                    cur_exp = self.mem.read_u16(p + 0x138)
                    new_exp = max(cur_exp if 2019 <= cur_exp <= 2150 else cur_year, cur_year + 5)
                    self.mem.write_u16(p + 0x138, min(2150, new_exp))
                elif mode == "ovr1":
                    buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
                    if buf:
                        cur_ovr, _ = calc_ovr_from_buf(buf)
                        if cur_ovr < 99:
                            self._scale_primary_ovr(p, min(99, cur_ovr + 1))
            return {"ok": True, "count": len(addrs)}

    # ------------------------------------------------------------------
    # GLOBAL 30K+ PLAYER SEARCH
    # ------------------------------------------------------------------
    def search_players(
        self,
        query: str = "",
        pos_filter: int = -1,
        min_ovr: int = 0,
        max_age: int = 99,
        free_only: bool = False,
        limit: int = 120,
    ) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected", "players": []}
            career, _, _ = self.sync_pointers()
            if not career:
                return {"ok": False, "code": "no_career", "players": []}
            count = self.mem.read_u32(career + 0x00D0BCE8)
            if count == 0 or count > 30001:
                count = 30000
            raw = self.mem.read_bytes(career, count * PLAYER_RECORD_SIZE)
            if not raw:
                return {"ok": False, "code": "read_failed", "players": []}

            mv = memoryview(raw)
            q_norm = _norm_text(query)
            pf = _safe_int(pos_filter, -1)
            movr = _safe_int(min_ovr, 0) or 0
            mage = _safe_int(max_age, 99) or 99
            results = []
            for i in range(count):
                off = i * PLAYER_RECORD_SIZE
                pid = struct.unpack_from("<I", mv, off + 0x30)[0]
                if pid == 0 or pid == 0xFFFFFFFF:
                    continue
                age = mv[off + 0x1C] & 0x3F
                if age > mage:
                    continue
                reg_pos = (mv[off + 0x07] >> 4) & 0x0F
                if pf is not None and pf >= 0 and reg_pos != pf:
                    continue
                club_h = struct.unpack_from("<I", mv, off + 0x12C)[0]
                if free_only and club_h != 0 and club_h != 0xFFFFFFFF:
                    continue
                name_b = raw[off + 0x38 : off + 0x66].split(b"\x00", 1)[0]
                if not name_b:
                    continue
                name = name_b.decode("utf-8", errors="replace").strip()
                if q_norm and q_norm not in _norm_text(name):
                    continue
                ovr, _ = calc_ovr_from_buf(mv[off : off + 0x28])
                if ovr < movr:
                    continue
                _, club_name = self.decode_club_handle(club_h, career)
                sal_raw = struct.unpack_from("<i", mv, off + 0x15C)[0]
                exp_y = struct.unpack_from("<H", mv, off + 0x138)[0]
                results.append(
                    {
                        "addr": career + off,
                        "name": name,
                        "ovr": ovr,
                        "pos": reg_pos,
                        "age": age,
                        "club": club_name,
                        "salary_raw": sal_raw,
                        "exp_y": exp_y if 2019 <= exp_y <= 2150 else 0,
                    }
                )
            results.sort(key=lambda x: (-x["ovr"], x["name"]))
            return {"ok": True, "total": len(results), "players": results[:limit]}

    # ------------------------------------------------------------------
    # PURE NATIVE SCOUT LISTS & SHORTLIST (0 side effects!)
    # ------------------------------------------------------------------
    def get_scout_lists(self) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected", "lists": []}
            career, _, _ = self.sync_pointers()
            if not career:
                return {"ok": False, "code": "no_career", "lists": []}
            count = self.mem.read_u32(career + 0x00D0BCE8)
            if count == 0 or count > 30001:
                count = 30000
            out_lists = []
            for sl in SCOUT_LISTS:
                entries = []
                raw = self.mem.read_bytes(career + sl["base"], sl["cap"] * 8)
                if raw:
                    for idx in range(sl["cap"]):
                        h = struct.unpack_from("<Q", raw, idx * 8)[0]
                        if h != 0 and (h & 0xFFFF) != SCOUT_EMPTY_SENTINEL and (h >> 32) != 0:
                            lo = h & 0xFFFFFFFF
                            p_addr = 0
                            if lo < count and self.mem.read_u64(career + lo * PLAYER_RECORD_SIZE + 0x2C) == h:
                                p_addr = career + lo * PLAYER_RECORD_SIZE
                            if p_addr:
                                buf = self.mem.read_bytes(p_addr, PLAYER_RECORD_SIZE)
                                if buf:
                                    ovr, reg_pos = calc_ovr_from_buf(buf)
                                    name = buf[0x38:0x66].split(b"\x00", 1)[0].decode("utf-8", errors="replace").strip()
                                    age = buf[0x1C] & 0x3F
                                    club_h = struct.unpack_from("<I", buf, 0x12C)[0]
                                    _, cname = self.decode_club_handle(club_h, career)
                                    entries.append(
                                        {
                                            "slot_idx": idx,
                                            "handle": str(h),
                                            "addr": p_addr,
                                            "name": name,
                                            "ovr": ovr,
                                            "pos": reg_pos,
                                            "age": age,
                                            "club": cname,
                                        }
                                    )
                out_lists.append(
                    {
                        "slot": sl["slot"],
                        "pt": sl["pt"],
                        "en": sl["en"],
                        "cap": sl["cap"],
                        "entries": entries,
                    }
                )
            return {"ok": True, "lists": out_lists}

    def add_to_scout_list(self, slot_num: int, player_addr: Optional[int] = None) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            career, cur_p, _ = self.sync_pointers()
            p = _safe_int(player_addr) or cur_p
            if not career:
                return {"ok": False, "code": "no_career"}
            if not p:
                return {"ok": False, "code": "no_player"}
            sn = _safe_int(slot_num, -1)
            sl = next((x for x in SCOUT_LISTS if x["slot"] == sn), None)
            if not sl:
                return {"ok": False, "code": "invalid_arg"}
            p_handle = self.mem.read_u64(p + 0x2C)
            if not p_handle or (p_handle >> 32) == 0:
                return {"ok": False, "code": "invalid_handle"}

            raw = self.mem.read_bytes(career + sl["base"], sl["cap"] * 8)
            if not raw:
                return {"ok": False, "code": "read_failed"}
            free_idx = -1
            for i in range(sl["cap"]):
                cur_h = struct.unpack_from("<Q", raw, i * 8)[0]
                if cur_h == p_handle:
                    return {"ok": True, "code": "scout_dup", "slot": sn}
                if free_idx == -1 and (cur_h == 0 or (cur_h & 0xFFFF) == SCOUT_EMPTY_SENTINEL):
                    free_idx = i
            if free_idx == -1:
                return {"ok": False, "code": "scout_full", "slot": sn}
            self.mem.write_u64(career + sl["base"] + free_idx * 8, p_handle)
            return {"ok": True, "code": "scout_added", "slot": sn}

    def remove_from_scout_list(self, slot_num: int, handle_str: str) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            career, _, _ = self.sync_pointers()
            sn = _safe_int(slot_num, -1)
            target_h = _safe_int(handle_str, 0) or 0
            sl = next((x for x in SCOUT_LISTS if x["slot"] == sn), None)
            if not career or not sl or not target_h:
                return {"ok": False, "code": "invalid_arg"}
            raw = self.mem.read_bytes(career + sl["base"], sl["cap"] * 8)
            if not raw:
                return {"ok": False, "code": "read_failed"}
            for i in range(sl["cap"]):
                if struct.unpack_from("<Q", raw, i * 8)[0] == target_h:
                    self.mem.write_u64(career + sl["base"] + i * 8, SCOUT_EMPTY_SENTINEL)
                    return {"ok": True}
            return {"ok": False, "code": "not_found"}

    def clear_scout_list(self, slot_num: int) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            career, _, _ = self.sync_pointers()
            sn = _safe_int(slot_num, -1)
            sl = next((x for x in SCOUT_LISTS if x["slot"] == sn), None)
            if not career or not sl:
                return {"ok": False, "code": "invalid_arg"}
            empty_buf = struct.pack("<Q", SCOUT_EMPTY_SENTINEL) * sl["cap"]
            self.mem.write_bytes(career + sl["base"], empty_buf)
            return {"ok": True, "slot": sn}

    # ------------------------------------------------------------------
    # TRAINING, BAL & MATCH HOOK CONTROLS
    # ------------------------------------------------------------------
    def set_training_mode(self, mode: int) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            m = max(0, min(5, _safe_int(mode, 0) or 0))
            if m > 0:
                if not self.hooks.enable_hook_3_training():
                    return {"ok": False, "code": "hook_failed"}
            self.mem.write_u32(self.hooks.sym_addr("cfgTrainingMultMode"), m)
            return {"ok": True, "mode": m, "hook_active": self.hooks.active_hooks.get(3, False)}

    def set_stamina_mode(self, mode: int) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            m = max(0, min(5, _safe_int(mode, 0) or 0))
            if m > 0:
                if not self.hooks.enable_hook_6_stamina():
                    return {"ok": False, "code": "hook_failed"}
            self.mem.write_u32(self.hooks.sym_addr("cfgStaminaMode"), m)
            return {"ok": True, "mode": m, "hook_active": self.hooks.active_hooks.get(6, False)}

    def set_bal_focus(self, points: int, lock_enabled: bool) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            career, _, _ = self.sync_pointers()
            if not career:
                return {"ok": False, "code": "no_career"}
            pts = max(0, min(99, _safe_int(points, 21) or 0))
            if pts > 21 or lock_enabled:
                self.hooks.enable_hook_4_bal_cap()
            self._bal_lock_target = pts
            self.mem.write_u8(career + 0x016ECBEA, pts)
            self.mem.write_u32(self.hooks.sym_addr("cfgLockBALFocusPoints"), 1 if lock_enabled else 0)
            return {"ok": True, "points": pts, "lock": bool(lock_enabled)}

    def set_freeze_clock(self, freeze: bool) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            if freeze:
                _, _, ms = self.sync_pointers()
                if ms <= 0x10000:
                    return {"ok": False, "code": "not_in_match"}
                if not self.hooks.enable_hook_5_match():
                    return {"ok": False, "code": "hook_failed"}
                # Wait briefly for newly installed clock tap to capture live ticks before latching
                prot = self.mem.read_f32(self.hooks.sym_addr("valProtectedMatchTicks"))
                if prot == 0.0:
                    for _ in range(15):
                        time.sleep(0.01)
                        prot = self.mem.read_f32(self.hooks.sym_addr("valProtectedMatchTicks"))
                        if prot != 0.0:
                            break
                off_t = self.mem.read_f32(ms + 0x1168 + 0x0C)
                self.mem.write_f32(self.hooks.sym_addr("valFrozenTotalTicks"), prot + off_t)
            self.mem.write_u32(self.hooks.sym_addr("cfgFreezeClock"), 1 if freeze else 0)
            return {"ok": True, "freeze": bool(freeze)}

    # ------------------------------------------------------------------
    # JSON BACKUP EXPORT / IMPORT
    # ------------------------------------------------------------------
    def export_player_json(self) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            if not p:
                return {"ok": False, "code": "no_player"}
            buf = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
            if not buf:
                return {"ok": False, "code": "read_failed"}
            name = buf[0x38:0x66].split(b"\x00", 1)[0].decode("utf-8", errors="replace").strip()
            ovr, _ = calc_ovr_from_buf(buf)
            return {
                "ok": True,
                "name": name,
                "ovr": ovr,
                "json": json.dumps({"version": "12.0", "name": name, "hex": buf.hex()}, indent=2),
            }

    def import_player_json(self, json_str: str) -> Dict[str, Any]:
        with self.lock:
            if not self.ensure_attached():
                return {"ok": False, "code": "not_connected"}
            _, p, _ = self.sync_pointers()
            if not p:
                return {"ok": False, "code": "no_player"}
            try:
                data = json.loads(json_str)
                raw = bytes.fromhex(data["hex"])
                if len(raw) != PLAYER_RECORD_SIZE:
                    return {"ok": False, "code": "invalid_json"}
                cur = self.mem.read_bytes(p, PLAYER_RECORD_SIZE)
                if not cur:
                    return {"ok": False, "code": "read_failed"}
                self.push_undo(p, "import_json")
                merged = bytearray(raw)
                merged[0x2C:0x38] = cur[0x2C:0x38]
                merged[0x12C:0x134] = cur[0x12C:0x134]
                merged[0x160:0x168] = cur[0x160:0x168]
                self.mem.write_bytes(p, bytes(merged))
                self._wage_cache_time = 0.0
                return self.get_player_details(p)
            except Exception:
                return {"ok": False, "code": "invalid_json"}
