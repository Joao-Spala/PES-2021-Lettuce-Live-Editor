"""
Native x64 Code-Cave Hook Manager for PES 2021 / SP Football Life.
Installs, verifies, and cleanly restores the verified x64 hooks without Cheat Engine:
  - Hook 1: Automatic Player Capture (4 UI sites -> ptrPlayer)
  - Hook 3: Universal Training Sub-XP Multiplier (1x/2x/5x/10x/50x/Instant)
  - Hook 4: BAL Focus Points 21-Cap Unlock (0x15 -> 0x7F)
  - Hook 5: Match HUD & Clock Freeze Tap
  - Hook 6: Selective Match Stamina (6 modes)
"""

import struct
from typing import Dict, List, Tuple
from .win32_mem import Win32Memory

# Shared state block offsets (allocated at state_base, 0x1000 bytes RWX)
# Data region: 0x000..0x4FF | Code caves: 0x500..0xFFF
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


class CodeBuf:
    """Tiny deterministic x64 code buffer with automatic label & RIP-relative patching."""

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
        """Emit instruction with [RIP + rel32] operand and optional immediate tail."""
        self.buf.extend(prefix)
        next_rip = self.cur_addr + 4 + len(imm_tail)
        rel = target_abs - next_rip
        self.buf.extend(struct.pack("<i", rel))
        if imm_tail:
            self.buf.extend(imm_tail)

    def jmp_abs(self, target_abs: int) -> None:
        """Emit 5-byte E9 rel32 jump to absolute address within +-2GB."""
        self.buf.append(0xE9)
        rel = target_abs - (self.cur_addr + 4)
        self.buf.extend(struct.pack("<i", rel))

    def jcc8(self, opcode: int, label_name: str) -> None:
        """Emit 2-byte short branch (0x74=je, 0x75=jne, 0x7E=jle, 0x72=jb, 0x73=jae, 0xEB=jmp)."""
        self.buf.append(opcode)
        self.fixups_rel8.append((len(self.buf), label_name))
        self.buf.append(0x00)

    def jcc32(self, opcodes: bytes, label_name: str) -> None:
        """Emit near rel32 branch (e.g. b'\\x0F\\x84'=je, b'\\x0F\\x85'=jne, b'\\xE9'=jmp)."""
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
            rel = self.labels[name] - (pos + 4)
            self.buf[pos : pos + 4] = struct.pack("<i", rel)
        return bytes(self.buf)


class HookManager:
    """Manages state page allocation and all native x64 hooks with clean teardown."""

    def __init__(self, mem: Win32Memory) -> None:
        self.mem = mem
        self.state_base: int = 0
        self.cave_cursor: int = 0
        # List of (patch_addr, orig_bytes) for clean restoration on exit
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
        # Allocate 2 pages (8KB) RWX near module_base: 0x000..0x4FF data, 0x500..0x1FFF code caves
        ptr = self.mem.alloc_near(self.mem.module_base, 0x2000)
        if not ptr:
            return False
        self.mem.write_bytes(ptr, b"\x00" * 0x2000)
        self.state_base = ptr
        self.cave_cursor = ptr + 0x500
        self.patches.clear()
        self.active_hooks.clear()
        return True

    def _alloc_cave(self, code: bytes) -> int:
        # 16-byte align
        addr = (self.cave_cursor + 15) & ~15
        if addr + len(code) > self.state_base + 0x2000:
            raise RuntimeError("Code cave buffer full")
        self.mem.write_bytes(addr, code)
        self.cave_cursor = addr + len(code)
        return addr

    def _install_jmp(self, hook_key: str, site: int, cave_addr: int, orig_len: int) -> bool:
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

    # ------------------------------------------------------------------
    # HOOK 1: Automatic Player Capture (4 sites)
    # ------------------------------------------------------------------
    def enable_hook_1_player(self) -> bool:
        if self.active_hooks.get(1):
            return True
        if not self.init_state():
            return False
        ptr_player = self.sym_addr("ptrPlayer")
        installed = 0

        # Site 1: INJECT_ptrPlayer + 0x13 (6 bytes: 0F 10 02 0F 11 01)
        s1 = self._scan(
            0xC6DC00,
            0xC6FE00,
            "CB B8 02 00 00 00 0F 1F 40 00 66 0F 1F 84 00 00 00 00 00 0F 10 02 0F 11 01",
        )
        if s1:
            site = s1 + 0x13
            cave_addr = (self.cave_cursor + 15) & ~15
            cb = CodeBuf(cave_addr)
            cb.emit(b"\x83\xF8\x02")  # cmp eax, 2
            cb.jcc8(0x75, "code")  # jne code
            cb.emit_rip(b"\x48\x89\x15", ptr_player)  # mov [ptrPlayer], rdx
            cb.label("code")
            cb.emit(b"\x0F\x10\x02\x0F\x11\x01")  # movups xmm0,[rdx]; movups [rcx],xmm0
            cb.jmp_abs(site + 6)
            self._alloc_cave(cb.finalize())
            if self._install_jmp("hook1", site, cave_addr, 6):
                installed += 1

        # Site 2: INJECT_ptrPlayerTwo (7 bytes: 48 8B 40 2C 48 89 02)
        s2 = self._scan(0x8E1400, 0x8E3500, "48 8B 40 2C 48 89 02")
        if s2:
            cave_addr = (self.cave_cursor + 15) & ~15
            cb = CodeBuf(cave_addr)
            cb.emit_rip(b"\x48\x89\x05", ptr_player)  # mov [ptrPlayer], rax
            cb.emit(b"\x48\x8B\x40\x2C\x48\x89\x02")  # mov rax,[rax+2C]; mov [rdx],rax
            cb.jmp_abs(s2 + 7)
            self._alloc_cave(cb.finalize())
            if self._install_jmp("hook1", s2, cave_addr, 7):
                installed += 1

        # Site 3: INJECT_ptrPlayerGamePlan (7 bytes: 0F B6 80 50 01 00 00)
        s3 = self._scan(0xDDB900, 0xDDDB00, "0F B6 80 50 01 00 00 89 87 98 00 00 00")
        if s3:
            cave_addr = (self.cave_cursor + 15) & ~15
            cb = CodeBuf(cave_addr)
            cb.emit(b"\x48\x85\xC0")  # test rax, rax
            cb.jcc8(0x74, "code")  # jz code
            cb.emit_rip(b"\x48\x89\x05", ptr_player)  # mov [ptrPlayer], rax
            cb.label("code")
            cb.emit(b"\x0F\xB6\x80\x50\x01\x00\x00")  # movzx eax, byte ptr [rax+150]
            cb.jmp_abs(s3 + 7)
            self._alloc_cave(cb.finalize())
            if self._install_jmp("hook1", s3, cave_addr, 7):
                installed += 1

        # Site 4: INJECT_ptrPlayerListUI (7 bytes: 0F B6 80 50 01 00 00)
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
            self._alloc_cave(cb.finalize())
            if self._install_jmp("hook1", s4, cave_addr, 7):
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
        s = self._scan(0x12E0400, 0x12E2500, "45 8B C8 83 FA 6C 0F 8D 1E 01 00 00")
        if not s:
            return False
        cfg = self.sym_addr("cfgTrainingMultMode")
        cave_addr = (self.cave_cursor + 15) & ~15
        cb = CodeBuf(cave_addr)
        cb.emit_rip(b"\x44\x8B\x0D", cfg)  # mov r9d, [cfgTrainingMultMode]
        cb.emit(b"\x45\x85\xC9")  # test r9d, r9d
        cb.jcc8(0x74, "orig")  # jz orig
        cb.emit(b"\x45\x85\xC0")  # test r8d, r8d
        cb.jcc8(0x7E, "orig")  # jle orig

        cb.emit(b"\x41\x83\xF9\x01")  # cmp r9d, 1
        cb.jcc8(0x75, "m2")
        cb.emit(b"\x45\x6B\xC0\x02")  # imul r8d, r8d, 2
        cb.jcc8(0xEB, "clamp")

        cb.label("m2")
        cb.emit(b"\x41\x83\xF9\x02")  # cmp r9d, 2
        cb.jcc8(0x75, "m3")
        cb.emit(b"\x45\x6B\xC0\x05")  # imul r8d, r8d, 5
        cb.jcc8(0xEB, "clamp")

        cb.label("m3")
        cb.emit(b"\x41\x83\xF9\x03")  # cmp r9d, 3
        cb.jcc8(0x75, "m4")
        cb.emit(b"\x45\x6B\xC0\x0A")  # imul r8d, r8d, 10
        cb.jcc8(0xEB, "clamp")

        cb.label("m4")
        cb.emit(b"\x41\x83\xF9\x04")  # cmp r9d, 4
        cb.jcc8(0x75, "m5")
        cb.emit(b"\x45\x6B\xC0\x32")  # imul r8d, r8d, 50
        cb.jcc8(0xEB, "clamp")

        cb.label("m5")
        cb.emit(b"\x41\xB8\x10\x27\x00\x00")  # mov r8d, 10000

        cb.label("clamp")
        cb.emit(b"\x41\x81\xF8\x10\x27\x00\x00")  # cmp r8d, 10000
        cb.jcc8(0x7E, "orig")
        cb.emit(b"\x41\xB8\x10\x27\x00\x00")  # mov r8d, 10000

        cb.label("orig")
        cb.emit(b"\x45\x8B\xC8\x83\xFA\x6C")  # mov r9d, r8d; cmp edx, 6C
        cb.jmp_abs(s + 6)
        self._alloc_cave(cb.finalize())
        ok = self._install_jmp("hook3", s, cave_addr, 6)
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
        ok_any = False

        s_hud = self._scan(0x20F8F00, 0x20FB000, "49 8B F8 44 0F B6 E2 88 55 B7")
        if s_hud:
            cave_addr = (self.cave_cursor + 15) & ~15
            cb = CodeBuf(cave_addr)
            cb.emit_rip(b"\x48\x89\x0D", self.sym_addr("ptrMatchHud"))  # mov [ptrMatchHud], rcx
            cb.emit(b"\x50")  # push rax
            cb.emit(b"\x48\x8B\x41\x38")  # mov rax, [rcx+38]
            cb.emit_rip(b"\x48\x89\x05", self.sym_addr("ptrMatchCore"))  # mov [ptrMatchCore], rax
            cb.emit(b"\x58")  # pop rax
            cb.emit(b"\x49\x8B\xF8\x44\x0F\xB6\xE2")  # mov rdi, r8; movzx r12d, dl
            cb.jmp_abs(s_hud + 7)
            self._alloc_cave(cb.finalize())
            if self._install_jmp("hook5", s_hud, cave_addr, 7):
                ok_any = True

        s_clk = self._scan(
            0xA3B700,
            0xA3D800,
            "8B 44 24 40 89 44 24 40 F3 0F 10 44 24 40 48 83 C4 30 5B C3",
        )
        if s_clk:
            cave_addr = (self.cave_cursor + 15) & ~15
            cb = CodeBuf(cave_addr)
            cb.emit(b"\x8B\x44\x24\x40")  # mov eax, [rsp+40]
            cb.emit(b"\x51")  # push rcx
            cb.emit_rip(b"\x48\x8B\x0D", self.sym_addr("ptrMatchState"))  # mov rcx, [ptrMatchState]
            cb.emit(b"\x48\x85\xC9")  # test rcx, rcx
            cb.jcc8(0x74, "done")
            cb.emit(b"\x48\x8D\x89\x68\x11\x00\x00")  # lea rcx, [rcx+1168]
            cb.emit(b"\x48\x39\xCB")  # cmp rbx, rcx
            cb.jcc8(0x75, "done")
            cb.emit_rip(b"\x89\x05", self.sym_addr("valProtectedMatchTicks"))  # mov [valProt], eax
            cb.emit_rip(b"\x83\x3D", self.sym_addr("cfgFreezeClock"), b"\x01")  # cmp dword [cfgFreeze], 1
            cb.jcc8(0x75, "done")
            cb.emit_rip(b"\xF3\x0F\x10\x05", self.sym_addr("valFrozenTotalTicks"))  # movss xmm0, [valFrozen]
            cb.emit_rip(b"\xF3\x0F\x5C\x05", self.sym_addr("valProtectedMatchTicks"))  # subss xmm0, [valProt]
            cb.emit(b"\xF3\x0F\x11\x43\x0C")  # movss [rbx+0C], xmm0
            cb.label("done")
            cb.emit(b"\x59")  # pop rcx
            cb.emit(b"\x89\x44\x24\x40")  # mov [rsp+40], eax
            cb.jmp_abs(s_clk + 8)
            self._alloc_cave(cb.finalize())
            if self._install_jmp("hook5", s_clk, cave_addr, 8):
                ok_any = True

        self.active_hooks[5] = ok_any
        return ok_any

    # ------------------------------------------------------------------
    # HOOK 6: Selective Match Stamina (6 modes)
    # ------------------------------------------------------------------
    def enable_hook_6_stamina(self) -> bool:
        if self.active_hooks.get(6):
            return True
        if not self.init_state():
            return False
        s = self._scan(0xA57E00, 0xA59F00, "6B 45 10 64 48 8B 5C 24 50")
        if not s:
            return False
        cave_addr = (self.cave_cursor + 15) & ~15
        cb = CodeBuf(cave_addr)
        cfg = self.sym_addr("cfgStaminaMode")
        mgr = self.sym_addr("ptrMatchMgr")

        cb.emit_rip(b"\x83\x3D", cfg, b"\x00")  # cmp dword [cfgStaminaMode], 0
        cb.jcc32(b"\x0F\x84", "orig")
        cb.emit_rip(b"\x48\x8B\x0D", mgr)  # mov rcx, [ptrMatchMgr]
        cb.emit(b"\x48\x85\xC9")  # test rcx, rcx
        cb.jcc32(b"\x0F\x84", "orig")
        cb.emit(b"\x31\xD2")  # xor edx, edx

        cb.label("loop")
        cb.emit(b"\x48\x89\xD0")  # mov rax, rdx
        cb.emit(b"\x48\x05\xF3\x00\x00\x00")  # add rax, 0xF3
        cb.emit(b"\x48\x8D\x04\x80")  # lea rax, [rax+rax*4]
        cb.emit(b"\x48\x39\x1C\xC1")  # cmp [rcx+rax*8], rbx
        cb.jcc8(0x74, "found")
        cb.emit(b"\xFF\xC2")  # inc edx
        cb.emit(b"\x83\xFA\x16")  # cmp edx, 0x16
        cb.jcc8(0x72, "loop")
        cb.jcc32(b"\xE9", "orig")

        cb.label("found")
        cb.emit_rip(b"\x8B\x05", cfg)  # mov eax, [cfgStaminaMode]
        cb.emit(b"\x83\xFA\x0B")  # cmp edx, 0x0B
        cb.jcc8(0x73, "away")

        # Home player (edx < 11)
        cb.emit(b"\x83\xF8\x01")
        cb.jcc8(0x74, "give100")
        cb.emit(b"\x83\xF8\x03")
        cb.jcc8(0x74, "give100")
        cb.emit(b"\x83\xF8\x04")
        cb.jcc8(0x74, "give100")
        cb.emit(b"\x83\xF8\x05")
        cb.jcc8(0x74, "drain")
        cb.jcc8(0xEB, "orig")

        # Away player (edx >= 11)
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
        cb.emit(b"\x48\x8B\x5C\x24\x50")  # mov rbx, [rsp+50]
        cb.emit(b"\x48\x8B\x7C\x24\x58")  # mov rdi, [rsp+58]
        cb.emit(b"\xB8\x64\x00\x00\x00")  # mov eax, 100
        cb.emit(b"\x48\x83\xC4\x30\x5D\xC3")  # add rsp, 30; pop rbp; ret

        cb.label("drain")
        cb.emit(b"\x48\x8B\x5C\x24\x50")
        cb.emit(b"\x48\x8B\x7C\x24\x58")
        cb.emit(b"\xB8\x05\x00\x00\x00")  # mov eax, 5
        cb.emit(b"\x48\x83\xC4\x30\x5D\xC3")

        cb.label("orig")
        cb.emit(b"\x6B\x45\x10\x64\x48\x8B\x5C\x24\x50")  # imul eax,[rbp+10],64; mov rbx,[rsp+50]
        cb.jmp_abs(s + 9)

        self._alloc_cave(cb.finalize())
        ok = self._install_jmp("hook6", s, cave_addr, 9)
        self.active_hooks[6] = ok
        return ok

    def disable_all(self) -> None:
        """Restore every patched byte in PES2021.exe and free our allocated memory."""
        if self.mem.attached:
            for patch_list in self.patches.values():
                for site, orig in patch_list:
                    try:
                        self.mem.write_code_bytes(site, orig)
                    except Exception:
                        pass
            if self.state_base:
                try:
                    self.mem.free_mem(self.state_base)
                except Exception:
                    pass
        self.patches.clear()
        self.active_hooks.clear()
        self.state_base = 0
        self.cave_cursor = 0
