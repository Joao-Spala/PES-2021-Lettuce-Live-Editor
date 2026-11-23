"""
Native x64 Code-Cave Hook Manager for PES 2021 / SP Football Life.
Installs, verifies, and safely restores all 5 verified x64 hooks without Cheat Engine:
  - Hook 1: Automatic Player Capture (4 UI sites -> ptrPlayer)
  - Hook 3: Universal Training Sub-XP Multiplier (1x/2x/5x/10x/50x/Instant)
  - Hook 4: BAL Focus Points 21-Cap Unlock (0x15 -> 0x7F)
  - Hook 5: Match HUD & Clock Freeze Tap
  - Hook 6: Selective Match Stamina (6 modes)
"""

import struct
import time
from typing import Dict, List, Tuple
from .win32_mem import Win32Memory

SYM = {
    "ptrCareer": 0x000,
    "ptrPlayer": 0x008,
    "ptrMatchMgr": 0x010,
    "ptrMatchState": 0x018,
    "ptrMatchHud": 0x020,
    "ptrMatchCore": 0x028,
    "cfgTrainingMultMode": 0x074,
    "cfgStaminaMode": 0x078,
    "cfgFreezeClock": 0x07C,
    "valProtectedMatchTicks": 0x080,
    "valFrozenTotalTicks": 0x084,
    "cfgLockBALFocusPoints": 0x090,
}

# Verified PES2021.exe v1.7.2.0 hook RVAs + neighbor signature for safe stale-hook recovery
HOOK_ORIG_BYTES = {
    1: [
        (0xC6ED40, -19, 0xCB, bytes([0x0F, 0x10, 0x02, 0x0F, 0x11, 0x01]), None),
        (0x8E24D7, -7, 0x48, bytes([0x48, 0x8B, 0x40, 0x2C, 0x48, 0x89, 0x02]), None),
        (0xDDCA5C, 7, 0x89, bytes([0x0F, 0xB6, 0x80, 0x50, 0x01, 0x00, 0x00]), None),
        (0xF27462, 7, 0x89, bytes([0x0F, 0xB6, 0x80, 0x50, 0x01, 0x00, 0x00]), None),
    ],
    3: [
        (0x12E14A0, 6, 0x0F, bytes([0x45, 0x8B, 0xC8, 0x83, 0xFA, 0x6C]), None),
    ],
    4: [
        (0xBBFE79, -8, 0x0F, bytes([0x15]), 0x7F),
    ],
    5: [
        (0x20F9FD6, 7, 0x88, bytes([0x49, 0x8B, 0xF8, 0x44, 0x0F, 0xB6, 0xE2]), None),
        (0xA3C787, 8, 0xF3, bytes([0x8B, 0x44, 0x24, 0x40, 0x89, 0x44, 0x24, 0x40]), None),
    ],
    6: [
        (0xA58EAC, 9, 0x48, bytes([0x6B, 0x45, 0x10, 0x64, 0x48, 0x8B, 0x5C, 0x24, 0x50]), None),
    ],
}


def _check_rel32(rel: int, ctx: str) -> int:
    if not (-0x80000000 <= rel <= 0x7FFFFFFF):
        raise ValueError(f"rel32 out of range for {ctx}: {hex(rel)}")
    return rel


class CodeBuf:
    """Deterministic x64 code buffer with automatic label & RIP-relative patching."""

    def __init__(self, base_addr: int) -> None:
        self.base = base_addr
        self.buf = bytearray()
        self.labels: Dict[str, int] = {}
        self.fixups_rel8: List[Tuple[int, str]] = []
        self.fixups_rel32: List[Tuple[int, str]] = []

    @property
    def cur_addr(self) -> int:
        return self.base + len(self.buf)

    def emit(self, raw: bytes) -> None:
        self.buf.extend(raw)

    def label(self, name: str) -> None:
        self.labels[name] = len(self.buf)

    def emit_rip(self, prefix: bytes, target_abs: int, imm_tail: bytes = b"") -> None:
        self.buf.extend(prefix)
        next_rip = self.cur_addr + 4 + len(imm_tail)
        rel = _check_rel32(target_abs - next_rip, "emit_rip")
        self.buf.extend(struct.pack("<i", rel))
        if imm_tail:
            self.buf.extend(imm_tail)

    def jmp_abs(self, target_abs: int) -> None:
        self.buf.append(0xE9)
        rel = _check_rel32(target_abs - (self.cur_addr + 4), "jmp_abs")
        self.buf.extend(struct.pack("<i", rel))

    def jcc8(self, opcode: int, label_name: str) -> None:
        self.buf.append(opcode)
        self.fixups_rel8.append((len(self.buf), label_name))
        self.buf.append(0x00)

    def jcc32(self, opcodes: bytes, label_name: str) -> None:
        self.buf.extend(opcodes)
        self.fixups_rel32.append((len(self.buf), label_name))
        self.buf.extend(b"\x00\x00\x00\x00")

    def finalize(self) -> bytes:
        for pos, name in self.fixups_rel8:
            rel = self.labels[name] - (pos + 1)
            if not (-128 <= rel <= 127):
                raise ValueError(f"rel8 overflow for label {name}: {rel}")
            self.buf[pos] = rel & 0xFF
        for pos, name in self.fixups_rel32:
            rel = _check_rel32(self.labels[name] - (pos + 4), f"label:{name}")
            self.buf[pos : pos + 4] = struct.pack("<i", rel)
        return bytes(self.buf)


class HookManager:
    """Manages shared state page and all native x64 hooks with crash-safe teardown."""

    def __init__(self, mem: Win32Memory) -> None:
        self.mem = mem
        self.state_base: int = 0
        self.cave_cursor: int = 0
        self.patches: Dict[str, List[Tuple[int, bytes]]] = {}
        self.active_hooks: Dict[int, bool] = {}

    def sym_addr(self, name: str) -> int:
        if not self.state_base or name not in SYM:
            return 0
        return self.state_base + SYM[name]

    def init_state(self) -> bool:
        if self.state_base and self.mem.attached:
            return True
        if not self.mem.attached:
            return False
        ptr = self.mem.alloc_near(self.mem.module_base, 0x2000)
        if not ptr:
            return False
        if not self.mem.write_bytes(ptr, b"\x00" * 0x2000):
            self.mem.free_mem(ptr)
            return False
        self.state_base = ptr
        self.cave_cursor = ptr + 0x500
        self.patches.clear()
        self.active_hooks.clear()
        return True

    def _alloc_cave(self, code: bytes) -> int:
        addr = (self.cave_cursor + 15) & ~15
        if addr + len(code) > self.state_base + 0x2000:
            return 0
        if not self.mem.write_bytes(addr, code):
            return 0
        # Ensure CPU instruction cache sees newly written executable cave bytes
        from .win32_mem import kernel32
        import ctypes
        kernel32.FlushInstructionCache(self.mem.handle, ctypes.c_void_p(addr), len(code))
        self.cave_cursor = addr + len(code)
        return addr

    def _install_jmp(self, hook_key: str, site: int, cave_addr: int, orig_len: int) -> bool:
        if not cave_addr:
            return False
        orig = self.mem.read_bytes(site, orig_len)
        if not orig:
            return False
        rel = cave_addr - (site + 5)
        if not (-0x80000000 <= rel <= 0x7FFFFFFF):
            return False
        patch = b"\xE9" + struct.pack("<i", rel) + (b"\x90" * (orig_len - 5))
        if not self.mem.write_code_bytes(site, patch):
            return False
        self.patches.setdefault(hook_key, []).append((site, orig))
        return True

    def _scan(self, rva_start: int, rva_end: int, pattern: str) -> int:
        b = self.mem.module_base
        return self.mem.aob_scan_region(b + rva_start, b + rva_end, pattern)

    def restore_stale_hook_bytes(self, hook_num: int) -> None:
        """Restore original bytes if a previous CE/Studio run left stale JMPs on PES2021.exe v1.7.2.0."""
        if self.mem.exe_name.lower() != "pes2021.exe":
            return
        entries = HOOK_ORIG_BYTES.get(hook_num)
        if not entries or not self.mem.attached or not self.mem.module_base:
            return
        base = self.mem.module_base
        for rva, check_off, check_byte, orig, stale_byte in entries:
            addr = base + rva
            b0 = self.mem.read_u8(addr)
            is_stale = (b0 == stale_byte) if stale_byte is not None else (b0 == 0xE9)
            if is_stale and self.mem.read_u8(addr + check_off) == check_byte:
                self.mem.write_code_bytes(addr, orig)

    # ------------------------------------------------------------------
    # HOOK 1: Automatic Player Capture (4 sites)
    # ------------------------------------------------------------------
    def enable_hook_1_player(self) -> bool:
        if self.active_hooks.get(1):
            return True
        if not self.init_state():
            return False
        self.restore_stale_hook_bytes(1)
        ptr_player = self.sym_addr("ptrPlayer")
        installed = 0

        s1 = self._scan(
            0xC6DC00,
            0xC6FE00,
            "CB B8 02 00 00 00 0F 1F 40 00 66 0F 1F 84 00 00 00 00 00 0F 10 02 0F 11 01",
        )
        if s1:
            site = s1 + 0x13
            cave_addr = (self.cave_cursor + 15) & ~15
            cb = CodeBuf(cave_addr)
            cb.emit(b"\x83\xF8\x02")
            cb.jcc8(0x75, "code")
            cb.emit_rip(b"\x48\x89\x15", ptr_player)
            cb.label("code")
            cb.emit(b"\x0F\x10\x02\x0F\x11\x01")
            cb.jmp_abs(site + 6)
            if self._install_jmp("hook1", site, self._alloc_cave(cb.finalize()), 6):
                installed += 1

        s2 = self._scan(0x8E1400, 0x8E3500, "48 8B 40 2C 48 89 02")
        if s2:
            cave_addr = (self.cave_cursor + 15) & ~15
            cb = CodeBuf(cave_addr)
            cb.emit_rip(b"\x48\x89\x05", ptr_player)
            cb.emit(b"\x48\x8B\x40\x2C\x48\x89\x02")
            cb.jmp_abs(s2 + 7)
            if self._install_jmp("hook1", s2, self._alloc_cave(cb.finalize()), 7):
                installed += 1

        s3 = self._scan(0xDDB900, 0xDDDB00, "0F B6 80 50 01 00 00 89 87 98 00 00 00")
        if s3:
            cave_addr = (self.cave_cursor + 15) & ~15
            cb = CodeBuf(cave_addr)
            cb.emit(b"\x48\x85\xC0")
            cb.jcc8(0x74, "code")
            cb.emit_rip(b"\x48\x89\x05", ptr_player)
            cb.label("code")
            cb.emit(b"\x0F\xB6\x80\x50\x01\x00\x00")
            cb.jmp_abs(s3 + 7)
            if self._install_jmp("hook1", s3, self._alloc_cave(cb.finalize()), 7):
                installed += 1

        s4 = self._scan(0xF26400, 0xF28500, "0F B6 80 50 01 00 00 89 45 67")
        if s4:
            cave_addr = (self.cave_cursor + 15) & ~15
            cb = CodeBuf(cave_addr)
            cb.emit(b"\x48\x85\xC0")
            cb.jcc8(0x74, "code")
            cb.emit_rip(b"\x48\x89\x05", ptr_player)
            cb.label("code")
            cb.emit(b"\x0F\xB6\x80\x50\x01\x00\x00")
            cb.jmp_abs(s4 + 7)
            if self._install_jmp("hook1", s4, self._alloc_cave(cb.finalize()), 7):
                installed += 1

        self.active_hooks[1] = installed > 0
        return self.active_hooks[1]

    # ------------------------------------------------------------------
    # HOOK 3: Training XP Multiplier
    # ------------------------------------------------------------------
    def enable_hook_3_training(self) -> bool:
        if self.active_hooks.get(3):
            return True
        if not self.init_state():
            return False
        self.restore_stale_hook_bytes(3)
        s = self._scan(0x12E0400, 0x12E2500, "45 8B C8 83 FA 6C 0F 8D 1E 01 00 00")
        if not s:
            return False
        cfg = self.sym_addr("cfgTrainingMultMode")
        cave_addr = (self.cave_cursor + 15) & ~15
        cb = CodeBuf(cave_addr)
        cb.emit_rip(b"\x44\x8B\x0D", cfg)
        cb.emit(b"\x45\x85\xC9")
        cb.jcc8(0x74, "orig")
        cb.emit(b"\x45\x85\xC0")
        cb.jcc8(0x7E, "orig")

        cb.emit(b"\x41\x83\xF9\x01")
        cb.jcc8(0x75, "m2")
        cb.emit(b"\x45\x6B\xC0\x02")
        cb.jcc8(0xEB, "clamp")

        cb.label("m2")
        cb.emit(b"\x41\x83\xF9\x02")
        cb.jcc8(0x75, "m3")
        cb.emit(b"\x45\x6B\xC0\x05")
        cb.jcc8(0xEB, "clamp")

        cb.label("m3")
        cb.emit(b"\x41\x83\xF9\x03")
        cb.jcc8(0x75, "m4")
        cb.emit(b"\x45\x6B\xC0\x0A")
        cb.jcc8(0xEB, "clamp")

        cb.label("m4")
        cb.emit(b"\x41\x83\xF9\x04")
        cb.jcc8(0x75, "m5")
        cb.emit(b"\x45\x6B\xC0\x32")
        cb.jcc8(0xEB, "clamp")

        cb.label("m5")
        cb.emit(b"\x41\xB8\x10\x27\x00\x00")

        cb.label("clamp")
        cb.emit(b"\x41\x81\xF8\x10\x27\x00\x00")
        cb.jcc8(0x76, "orig")  # jbe (unsigned <= 10000, handles overflow safely)
        cb.emit(b"\x41\xB8\x10\x27\x00\x00")

        cb.label("orig")
        cb.emit(b"\x45\x8B\xC8\x83\xFA\x6C")
        cb.jmp_abs(s + 6)
        ok = self._install_jmp("hook3", s, self._alloc_cave(cb.finalize()), 6)
        self.active_hooks[3] = ok
        return ok

    # ------------------------------------------------------------------
    # HOOK 4: BAL Focus 21-Cap Unlock
    # ------------------------------------------------------------------
    def enable_hook_4_bal_cap(self) -> bool:
        if self.active_hooks.get(4):
            return True
        if not self.init_state():
            return False
        self.restore_stale_hook_bytes(4)
        s = self._scan(0xBBEE00, 0xBC0F00, "0F BE 41 0A 2B C2 83 F8 15 77 09 48 63 C3")
        if not s:
            return False
        site = s + 8
        orig = self.mem.read_bytes(site, 1)
        if not orig:
            return False
        if not self.mem.write_code_bytes(site, b"\x7F"):
            return False
        self.patches.setdefault("hook4", []).append((site, orig))
        self.active_hooks[4] = True
        return True

    # ------------------------------------------------------------------
    # HOOK 5: Match HUD & Clock Freeze
    # ------------------------------------------------------------------
    def enable_hook_5_match(self) -> bool:
        if self.active_hooks.get(5):
            return True
        if not self.init_state():
            return False
        self.restore_stale_hook_bytes(5)
        ok_clk = False

        s_hud = self._scan(0x20F8F00, 0x20FB000, "49 8B F8 44 0F B6 E2 88 55 B7")
        if s_hud:
            cave_addr = (self.cave_cursor + 15) & ~15
            cb = CodeBuf(cave_addr)
            cb.emit_rip(b"\x48\x89\x0D", self.sym_addr("ptrMatchHud"))
            cb.emit(b"\x50\x48\x8B\x41\x38")
            cb.emit_rip(b"\x48\x89\x05", self.sym_addr("ptrMatchCore"))
            cb.emit(b"\x58\x49\x8B\xF8\x44\x0F\xB6\xE2")
            cb.jmp_abs(s_hud + 7)
            self._install_jmp("hook5", s_hud, self._alloc_cave(cb.finalize()), 7)

        s_clk = self._scan(
            0xA3B700,
            0xA3D800,
            "8B 44 24 40 89 44 24 40 F3 0F 10 44 24 40 48 83 C4 30 5B C3",
        )
        if s_clk:
            cave_addr = (self.cave_cursor + 15) & ~15
            cb = CodeBuf(cave_addr)
            cb.emit(b"\x8B\x44\x24\x40\x51")
            cb.emit_rip(b"\x48\x8B\x0D", self.sym_addr("ptrMatchState"))
            cb.emit(b"\x48\x85\xC9")
            cb.jcc8(0x74, "done")
            cb.emit(b"\x48\x8D\x89\x68\x11\x00\x00\x48\x39\xCB")
            cb.jcc8(0x75, "done")
            cb.emit_rip(b"\x89\x05", self.sym_addr("valProtectedMatchTicks"))
            cb.emit_rip(b"\x83\x3D", self.sym_addr("cfgFreezeClock"), b"\x01")
            cb.jcc8(0x75, "done")
            cb.emit_rip(b"\xF3\x0F\x10\x05", self.sym_addr("valFrozenTotalTicks"))
            cb.emit_rip(b"\xF3\x0F\x5C\x05", self.sym_addr("valProtectedMatchTicks"))
            cb.emit(b"\xF3\x0F\x11\x43\x0C")
            cb.label("done")
            cb.emit(b"\x59\x89\x44\x24\x40")
            cb.jmp_abs(s_clk + 8)
            ok_clk = self._install_jmp("hook5", s_clk, self._alloc_cave(cb.finalize()), 8)

        if not ok_clk and "hook5" in self.patches:
            # Roll back partial HUD patch if clock tap failed so no stale JMP remains
            for site, orig in self.patches.pop("hook5"):
                self.mem.write_code_bytes(site, orig)

        self.active_hooks[5] = ok_clk
        return ok_clk

    # ------------------------------------------------------------------
    # HOOK 6: Selective Match Stamina (6 modes)
    # ------------------------------------------------------------------
    def enable_hook_6_stamina(self) -> bool:
        if self.active_hooks.get(6):
            return True
        if not self.init_state():
            return False
        self.restore_stale_hook_bytes(6)
        s = self._scan(0xA57E00, 0xA59F00, "6B 45 10 64 48 8B 5C 24 50")
        if not s:
            return False
        cave_addr = (self.cave_cursor + 15) & ~15
        cb = CodeBuf(cave_addr)
        cfg = self.sym_addr("cfgStaminaMode")
        mgr = self.sym_addr("ptrMatchMgr")

        cb.emit_rip(b"\x83\x3D", cfg, b"\x00")
        cb.jcc32(b"\x0F\x84", "orig")
        cb.emit_rip(b"\x48\x8B\x0D", mgr)
        cb.emit(b"\x48\x85\xC9")
        cb.jcc32(b"\x0F\x84", "orig")
        cb.emit(b"\x31\xD2")

        cb.label("loop")
        cb.emit(b"\x48\x89\xD0\x48\x05\xF3\x00\x00\x00\x48\x8D\x04\x80\x48\x39\x1C\xC1")
        cb.jcc8(0x74, "found")
        cb.emit(b"\xFF\xC2\x83\xFA\x16")
        cb.jcc8(0x72, "loop")
        cb.jcc32(b"\xE9", "orig")

        cb.label("found")
        cb.emit_rip(b"\x8B\x05", cfg)
        cb.emit(b"\x83\xFA\x0B")
        cb.jcc8(0x73, "away")

        cb.emit(b"\x83\xF8\x01")
        cb.jcc8(0x74, "give100")
        cb.emit(b"\x83\xF8\x03")
        cb.jcc8(0x74, "give100")
        cb.emit(b"\x83\xF8\x04")
        cb.jcc8(0x74, "give100")
        cb.emit(b"\x83\xF8\x05")
        cb.jcc8(0x74, "drain")
        cb.jcc8(0xEB, "orig")

        cb.label("away")
        cb.emit(b"\x83\xF8\x02")
        cb.jcc8(0x74, "give100")
        cb.emit(b"\x83\xF8\x03")
        cb.jcc8(0x74, "give100")
        cb.emit(b"\x83\xF8\x05")
        cb.jcc8(0x74, "give100")
        cb.emit(b"\x83\xF8\x04")
        cb.jcc8(0x74, "drain")
        cb.jcc8(0xEB, "orig")

        cb.label("give100")
        cb.emit(b"\x48\x8B\x5C\x24\x50\x48\x8B\x7C\x24\x58\xB8\x64\x00\x00\x00\x48\x83\xC4\x30\x5D\xC3")

        cb.label("drain")
        cb.emit(b"\x48\x8B\x5C\x24\x50\x48\x8B\x7C\x24\x58\xB8\x05\x00\x00\x00\x48\x83\xC4\x30\x5D\xC3")

        cb.label("orig")
        cb.emit(b"\x6B\x45\x10\x64\x48\x8B\x5C\x24\x50")
        cb.jmp_abs(s + 9)

        ok = self._install_jmp("hook6", s, self._alloc_cave(cb.finalize()), 9)
        self.active_hooks[6] = ok
        return ok

    def disable_all(self) -> None:
        """Restore every patched byte safely and wait for in-flight game threads before freeing."""
        if self.mem.attached:
            all_restored = True
            for patch_list in self.patches.values():
                for site, orig in patch_list:
                    try:
                        if not self.mem.write_code_bytes(site, orig):
                            all_restored = False
                    except Exception:
                        all_restored = False
            if self.state_base and all_restored:
                # Allow any game thread currently executing inside a cave to return before freeing
                time.sleep(0.08)
                try:
                    self.mem.free_mem(self.state_base)
                except Exception:
                    pass
        self.patches.clear()
        self.active_hooks.clear()
        self.state_base = 0
        self.cave_cursor = 0
