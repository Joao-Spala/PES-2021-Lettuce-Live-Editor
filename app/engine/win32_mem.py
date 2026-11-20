"""
Pure ctypes 64-bit Win32 Memory Engine for PES 2021 / SP Football Life.
Zero external dependencies. Supports RPM/WPM, AOB region scanning,
near-module VirtualAllocEx (for 5-byte rel32 x64 hooks), and process auto-attach.
"""

import ctypes
import ctypes.wintypes as wt
import struct
import re
from typing import Optional, Tuple, List

kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
psapi = ctypes.WinDLL("psapi", use_last_error=True)

PROCESS_ALL_ACCESS = 0x001F0FFF
PROCESS_VM_READ = 0x0010
PROCESS_VM_WRITE = 0x0020
PROCESS_VM_OPERATION = 0x0008
PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_ACCESS = (
    PROCESS_VM_READ
    | PROCESS_VM_WRITE
    | PROCESS_VM_OPERATION
    | PROCESS_QUERY_INFORMATION
)

TH32CS_SNAPPROCESS = 0x00000002
TH32CS_SNAPMODULE = 0x00000008
TH32CS_SNAPMODULE32 = 0x00000010
INVALID_HANDLE_VALUE = ctypes.c_void_p(-1).value

MEM_COMMIT = 0x1000
MEM_RESERVE = 0x2000
MEM_RELEASE = 0x8000
PAGE_EXECUTE_READWRITE = 0x40
PAGE_READWRITE = 0x04

SUPPORTED_EXES = [
    "PES2021.exe",
    "FL_2026.exe",
    "FL_2025.exe",
    "FL_2024.exe",
    "FL_2023.exe",
]


class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [
        ("dwSize", wt.DWORD),
        ("cntUsage", wt.DWORD),
        ("th32ProcessID", wt.DWORD),
        ("th32DefaultHeapID", ctypes.c_size_t),
        ("th32ModuleID", wt.DWORD),
        ("cntThreads", wt.DWORD),
        ("th32ParentProcessID", wt.DWORD),
        ("pcPriClassBase", ctypes.c_long),
        ("dwFlags", wt.DWORD),
        ("szExeFile", wt.WCHAR * 260),
    ]


class MODULEENTRY32W(ctypes.Structure):
    _fields_ = [
        ("dwSize", wt.DWORD),
        ("th32ModuleID", wt.DWORD),
        ("th32ProcessID", wt.DWORD),
        ("GlblcntUsage", wt.DWORD),
        ("ProccntUsage", wt.DWORD),
        ("modBaseAddr", ctypes.POINTER(ctypes.c_byte)),
        ("modBaseSize", wt.DWORD),
        ("hModule", wt.HMODULE),
        ("szModule", wt.WCHAR * 256),
        ("szExePath", wt.WCHAR * 260),
    ]


# Configure 64-bit argtypes / restypes
kernel32.CreateToolhelp32Snapshot.argtypes = [wt.DWORD, wt.DWORD]
kernel32.CreateToolhelp32Snapshot.restype = wt.HANDLE

kernel32.Process32FirstW.argtypes = [wt.HANDLE, ctypes.POINTER(PROCESSENTRY32W)]
kernel32.Process32FirstW.restype = wt.BOOL
kernel32.Process32NextW.argtypes = [wt.HANDLE, ctypes.POINTER(PROCESSENTRY32W)]
kernel32.Process32NextW.restype = wt.BOOL

kernel32.Module32FirstW.argtypes = [wt.HANDLE, ctypes.POINTER(MODULEENTRY32W)]
kernel32.Module32FirstW.restype = wt.BOOL
kernel32.Module32NextW.argtypes = [wt.HANDLE, ctypes.POINTER(MODULEENTRY32W)]
kernel32.Module32NextW.restype = wt.BOOL

kernel32.OpenProcess.argtypes = [wt.DWORD, wt.BOOL, wt.DWORD]
kernel32.OpenProcess.restype = wt.HANDLE

kernel32.CloseHandle.argtypes = [wt.HANDLE]
kernel32.CloseHandle.restype = wt.BOOL

kernel32.GetExitCodeProcess.argtypes = [wt.HANDLE, ctypes.POINTER(wt.DWORD)]
kernel32.GetExitCodeProcess.restype = wt.BOOL

kernel32.ReadProcessMemory.argtypes = [
    wt.HANDLE,
    ctypes.c_void_p,
    ctypes.c_void_p,
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_size_t),
]
kernel32.ReadProcessMemory.restype = wt.BOOL

kernel32.WriteProcessMemory.argtypes = [
    wt.HANDLE,
    ctypes.c_void_p,
    ctypes.c_void_p,
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_size_t),
]
kernel32.WriteProcessMemory.restype = wt.BOOL

kernel32.VirtualAllocEx.argtypes = [
    wt.HANDLE,
    ctypes.c_void_p,
    ctypes.c_size_t,
    wt.DWORD,
    wt.DWORD,
]
kernel32.VirtualAllocEx.restype = ctypes.c_void_p

kernel32.VirtualFreeEx.argtypes = [
    wt.HANDLE,
    ctypes.c_void_p,
    ctypes.c_size_t,
    wt.DWORD,
]
kernel32.VirtualFreeEx.restype = wt.BOOL

kernel32.VirtualProtectEx.argtypes = [
    wt.HANDLE,
    ctypes.c_void_p,
    ctypes.c_size_t,
    wt.DWORD,
    ctypes.POINTER(wt.DWORD),
]
kernel32.VirtualProtectEx.restype = wt.BOOL


def find_game_process() -> Optional[Tuple[int, str]]:
    """Find a running supported PES 2021 / Football Life process. Returns (pid, exe_name)."""
    snap = kernel32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    if snap == INVALID_HANDLE_VALUE or not snap:
        return None
    try:
        pe = PROCESSENTRY32W()
        pe.dwSize = ctypes.sizeof(PROCESSENTRY32W)
        wanted = {name.lower(): name for name in SUPPORTED_EXES}
        if kernel32.Process32FirstW(snap, ctypes.byref(pe)):
            while True:
                low = pe.szExeFile.lower()
                if low in wanted:
                    return (int(pe.th32ProcessID), pe.szExeFile)
                if not kernel32.Process32NextW(snap, ctypes.byref(pe)):
                    break
    finally:
        kernel32.CloseHandle(snap)
    return None


def get_module_info(pid: int, module_name: str) -> Optional[Tuple[int, int]]:
    """Return (base_address, size) for module_name in pid."""
    snap = kernel32.CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid
    )
    if snap == INVALID_HANDLE_VALUE or not snap:
        return None
    try:
        me = MODULEENTRY32W()
        me.dwSize = ctypes.sizeof(MODULEENTRY32W)
        target = module_name.lower()
        if kernel32.Module32FirstW(snap, ctypes.byref(me)):
            while True:
                if me.szModule.lower() == target:
                    base = ctypes.cast(me.modBaseAddr, ctypes.c_void_p).value or 0
                    return (int(base), int(me.modBaseSize))
                if not kernel32.Module32NextW(snap, ctypes.byref(me)):
                    break
    finally:
        kernel32.CloseHandle(snap)
    return None


class Win32Memory:
    """Safe, high-performance Win32 process memory accessor."""

    def __init__(self) -> None:
        self.handle: Optional[int] = None
        self.pid: int = 0
        self.exe_name: str = ""
        self.module_base: int = 0
        self.module_size: int = 0

    @property
    def attached(self) -> bool:
        if not self.handle or not self.pid:
            return False
        code = wt.DWORD(0)
        if not kernel32.GetExitCodeProcess(self.handle, ctypes.byref(code)):
            self.detach()
            return False
        if code.value != 259:  # STILL_ACTIVE
            self.detach()
            return False
        return True

    def attach(self) -> bool:
        if self.attached:
            return True
        found = find_game_process()
        if not found:
            return False
        pid, exe_name = found
        h = kernel32.OpenProcess(PROCESS_ACCESS, False, pid)
        if not h:
            return False
        mod = get_module_info(pid, exe_name)
        if not mod or mod[0] == 0:
            kernel32.CloseHandle(h)
            return False
        self.handle = int(h)
        self.pid = pid
        self.exe_name = exe_name
        self.module_base, self.module_size = mod
        return True

    def detach(self) -> None:
        if self.handle:
            try:
                kernel32.CloseHandle(self.handle)
            except Exception:
                pass
        self.handle = None
        self.pid = 0
        self.exe_name = ""
        self.module_base = 0
        self.module_size = 0

    def read_bytes(self, addr: int, size: int) -> Optional[bytes]:
        if not self.handle or addr <= 0x10000 or size <= 0:
            return None
        buf = (ctypes.c_ubyte * size)()
        read_n = ctypes.c_size_t(0)
        ok = kernel32.ReadProcessMemory(
            self.handle,
            ctypes.c_void_p(addr),
            buf,
            size,
            ctypes.byref(read_n),
        )
        if not ok or read_n.value < size:
            return None
        return bytes(buf)

    def write_bytes(self, addr: int, data: bytes) -> bool:
        if not self.handle or addr <= 0x10000 or not data:
            return False
        size = len(data)
        buf = (ctypes.c_ubyte * size).from_buffer_copy(data)
        written = ctypes.c_size_t(0)
        ok = kernel32.WriteProcessMemory(
            self.handle,
            ctypes.c_void_p(addr),
            buf,
            size,
            ctypes.byref(written),
        )
        return bool(ok and written.value == size)

    def write_code_bytes(self, addr: int, data: bytes) -> bool:
        """Write bytes into executable .text section by temporarily changing page protection."""
        if not self.handle or addr <= 0x10000 or not data:
            return False
        old_prot = wt.DWORD(0)
        ok_p = kernel32.VirtualProtectEx(
            self.handle,
            ctypes.c_void_p(addr),
            len(data),
            PAGE_EXECUTE_READWRITE,
            ctypes.byref(old_prot),
        )
        try:
            ok_w = self.write_bytes(addr, data)
        finally:
            if ok_p:
                dummy = wt.DWORD(0)
                kernel32.VirtualProtectEx(
                    self.handle,
                    ctypes.c_void_p(addr),
                    len(data),
                    old_prot.value,
                    ctypes.byref(dummy),
                )
        return ok_w

    def alloc_near(self, target_addr: int, size: int = 0x1000) -> int:
        """Allocate RWX memory within +-1.5GB of target_addr so 5-byte rel32 JMP reaches."""
        if not self.handle:
            return 0
        # Scan outward from target_addr in 64KB increments
        base_aligned = (target_addr & ~0xFFFF)
        step = 0x10000
        # Try below and above within 1.5GB
        for delta in range(step, 0x60000000, step):
            for cand in (base_aligned - delta, base_aligned + delta):
                if cand <= 0x10000:
                    continue
                ptr = kernel32.VirtualAllocEx(
                    self.handle,
                    ctypes.c_void_p(cand),
                    size,
                    MEM_COMMIT | MEM_RESERVE,
                    PAGE_EXECUTE_READWRITE,
                )
                if ptr:
                    return int(ptr)
        # Fallback anywhere
        ptr = kernel32.VirtualAllocEx(
            self.handle,
            None,
            size,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE,
        )
        return int(ptr) if ptr else 0

    def free_mem(self, addr: int) -> bool:
        if not self.handle or not addr:
            return False
        return bool(
            kernel32.VirtualFreeEx(
                self.handle, ctypes.c_void_p(addr), 0, MEM_RELEASE
            )
        )

    # Typed readers
    def read_u8(self, addr: int) -> int:
        b = self.read_bytes(addr, 1)
        return b[0] if b else 0

    def read_u16(self, addr: int) -> int:
        b = self.read_bytes(addr, 2)
        return struct.unpack("<H", b)[0] if b else 0

    def read_u32(self, addr: int) -> int:
        b = self.read_bytes(addr, 4)
        return struct.unpack("<I", b)[0] if b else 0

    def read_i32(self, addr: int) -> int:
        b = self.read_bytes(addr, 4)
        return struct.unpack("<i", b)[0] if b else 0

    def read_u64(self, addr: int) -> int:
        b = self.read_bytes(addr, 8)
        return struct.unpack("<Q", b)[0] if b else 0

    def read_f32(self, addr: int) -> float:
        b = self.read_bytes(addr, 4)
        return struct.unpack("<f", b)[0] if b else 0.0

    # Typed writers
    def write_u8(self, addr: int, val: int) -> bool:
        return self.write_bytes(addr, struct.pack("<B", val & 0xFF))

    def write_u16(self, addr: int, val: int) -> bool:
        return self.write_bytes(addr, struct.pack("<H", val & 0xFFFF))

    def write_u32(self, addr: int, val: int) -> bool:
        return self.write_bytes(addr, struct.pack("<I", val & 0xFFFFFFFF))

    def write_i32(self, addr: int, val: int) -> bool:
        return self.write_bytes(addr, struct.pack("<i", int(val)))

    def write_u64(self, addr: int, val: int) -> bool:
        return self.write_bytes(addr, struct.pack("<Q", val & 0xFFFFFFFFFFFFFFFF))

    def write_f32(self, addr: int, val: float) -> bool:
        return self.write_bytes(addr, struct.pack("<f", float(val)))

    def read_str(self, addr: int, max_len: int = 46) -> str:
        b = self.read_bytes(addr, max_len)
        if not b:
            return ""
        nul = b.find(b"\x00")
        if nul != -1:
            b = b[:nul]
        return b.decode("utf-8", errors="replace").strip()

    def write_str(self, addr: int, text: str, max_len: int) -> bool:
        raw = text.encode("utf-8", errors="ignore")[: max_len - 1]
        padded = raw + b"\x00" * (max_len - len(raw))
        return self.write_bytes(addr, padded)

    def get_bits(self, addr: int, bit_start: int, bit_len: int) -> int:
        v = self.read_u32(addr)
        mask = (1 << bit_len) - 1
        return (v >> bit_start) & mask

    def set_bits(self, addr: int, bit_start: int, bit_len: int, new_val: int) -> bool:
        v = self.read_u32(addr)
        mask = ((1 << bit_len) - 1) << bit_start
        v = (v & ~mask) | ((int(new_val) << bit_start) & mask)
        return self.write_u32(addr, v)

    def aob_scan_region(
        self, start_addr: int, end_addr: int, pattern: str
    ) -> int:
        """Scan [start_addr, end_addr) for hex pattern with optional '??' wildcards."""
        size = end_addr - start_addr
        if size <= 0 or size > 0x4000000:
            return 0
        data = self.read_bytes(start_addr, size)
        if not data:
            return 0
        tokens = pattern.strip().split()
        regex_parts: List[bytes] = []
        for tok in tokens:
            if tok in ("??", "?"):
                regex_parts.append(b".")
            else:
                regex_parts.append(re.escape(bytes([int(tok, 16)])))
        rx = re.compile(b"".join(regex_parts), re.DOTALL)
        m = rx.search(data)
        return (start_addr + m.start()) if m else 0
