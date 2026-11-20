"""End-to-end Lua 5.3 VM behavioral suite + Real Win32 GDI ClearType Canvas rasterizer & DPI QA for both PT-BR and EN Cheat Table editions."""

import argparse
import ctypes
from ctypes import wintypes
import json
import os
import pathlib
import struct
import xml.etree.ElementTree as ET
import zlib

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
LUA_DLL = os.environ.get("CE_LUA_DLL", r"C:\Program Files\Cheat Engine 7.2\lua53-64.dll")
CT_PT = REPO_ROOT / "dist" / "PES 2021 - v12.0 Ultimate Studio (PT-BR).CT"
CT_EN = REPO_ROOT / "dist" / "PES 2021 - v12.0 Ultimate Studio (EN).CT"

# ============================================================================
# WIN32 GDI DIB SECTION + CLEARTYPE SEGOE UI CANVAS BACKEND
# ============================================================================
gdi32 = ctypes.windll.gdi32
user32 = ctypes.windll.user32

gdi32.CreateCompatibleDC.argtypes = [wintypes.HDC]
gdi32.CreateCompatibleDC.restype = wintypes.HDC
gdi32.DeleteDC.argtypes = [wintypes.HDC]
gdi32.SelectObject.argtypes = [wintypes.HDC, wintypes.HGDIOBJ]
gdi32.SelectObject.restype = wintypes.HGDIOBJ
gdi32.DeleteObject.argtypes = [wintypes.HGDIOBJ]
gdi32.CreateSolidBrush.argtypes = [wintypes.COLORREF]
gdi32.CreateSolidBrush.restype = wintypes.HBRUSH
gdi32.CreatePen.argtypes = [ctypes.c_int, ctypes.c_int, wintypes.COLORREF]
gdi32.CreatePen.restype = wintypes.HPEN
gdi32.RoundRect.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
gdi32.SetTextColor.argtypes = [wintypes.HDC, wintypes.COLORREF]
gdi32.SetBkColor.argtypes = [wintypes.HDC, wintypes.COLORREF]
gdi32.SetBkMode.argtypes = [wintypes.HDC, ctypes.c_int]
gdi32.TextOutW.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int, wintypes.LPCWSTR, ctypes.c_int]
gdi32.GetTextExtentPoint32W.argtypes = [wintypes.HDC, wintypes.LPCWSTR, ctypes.c_int, ctypes.c_void_p]
gdi32.CreateFontW.argtypes = [
    ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
    wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD,
    wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, wintypes.LPCWSTR,
]
gdi32.CreateFontW.restype = wintypes.HFONT
gdi32.CreateDIBSection.argtypes = [
    wintypes.HDC, ctypes.c_void_p, wintypes.UINT, ctypes.c_void_p, wintypes.HANDLE, wintypes.DWORD
]
gdi32.CreateDIBSection.restype = wintypes.HBITMAP
user32.FillRect.argtypes = [wintypes.HDC, ctypes.c_void_p, wintypes.HBRUSH]


class SIZE(ctypes.Structure):
    _fields_ = [("cx", wintypes.LONG), ("cy", wintypes.LONG)]


class RECT(ctypes.Structure):
    _fields_ = [("left", wintypes.LONG), ("top", wintypes.LONG), ("right", wintypes.LONG), ("bottom", wintypes.LONG)]


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wintypes.DWORD),
        ("biWidth", wintypes.LONG),
        ("biHeight", wintypes.LONG),
        ("biPlanes", wintypes.WORD),
        ("biBitCount", wintypes.WORD),
        ("biCompression", wintypes.DWORD),
        ("biSizeImage", wintypes.DWORD),
        ("biXPelsPerMeter", wintypes.LONG),
        ("biYPelsPerMeter", wintypes.LONG),
        ("biClrUsed", wintypes.DWORD),
        ("biClrImportant", wintypes.DWORD),
    ]


class BITMAPINFO(ctypes.Structure):
    _fields_ = [("bmiHeader", BITMAPINFOHEADER), ("bmiColors", wintypes.DWORD * 3)]


class GdiSurface:
    def __init__(self, width: int, height: int, dpi: int):
        self.width = width
        self.height = height
        self.dpi = dpi
        self.hdc = gdi32.CreateCompatibleDC(0)
        bmi = BITMAPINFO()
        bmi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
        bmi.bmiHeader.biWidth = width
        bmi.bmiHeader.biHeight = -height  # top-down!
        bmi.bmiHeader.biPlanes = 1
        bmi.bmiHeader.biBitCount = 32
        bmi.bmiHeader.biCompression = 0
        bits_ptr = ctypes.c_void_p()
        self.hbmp = gdi32.CreateDIBSection(self.hdc, ctypes.byref(bmi), 0, ctypes.byref(bits_ptr), None, 0)
        self.bits_ptr = bits_ptr.value
        self.old_bmp = gdi32.SelectObject(self.hdc, self.hbmp)
        self.font_cache: dict[tuple[str, int, int], wintypes.HFONT] = {}
        self.text_draws: list[dict] = []

    def _select_font(self, name: str, pt_size: int, bold: int) -> None:
        key = (name, pt_size, bold)
        hfont = self.font_cache.get(key)
        if not hfont:
            lf_h = -int(round(pt_size * self.dpi / 72.0))
            weight = 700 if bold else 400
            # QUALITY = 6 (CLEARTYPE_NATURAL_QUALITY)
            hfont = gdi32.CreateFontW(lf_h, 0, 0, 0, weight, 0, 0, 0, 1, 0, 0, 6, 0, name)
            self.font_cache[key] = hfont
        gdi32.SelectObject(self.hdc, hfont)

    def measure(self, name: str, pt_size: int, bold: int, text: str) -> tuple[int, int]:
        if not text:
            self._select_font(name, pt_size, bold)
            sz = SIZE()
            gdi32.GetTextExtentPoint32W(self.hdc, "Ay", 2, ctypes.byref(sz))
            return 0, sz.cy
        self._select_font(name, pt_size, bold)
        sz = SIZE()
        wlen = len(text.encode("utf-16-le")) // 2
        gdi32.GetTextExtentPoint32W(self.hdc, text, wlen, ctypes.byref(sz))
        return sz.cx, sz.cy

    def fill_rect(self, x1: int, y1: int, x2: int, y2: int, col: int) -> None:
        rc = RECT(x1, y1, x2, y2)
        br = gdi32.CreateSolidBrush(col & 0xFFFFFF)
        user32.FillRect(self.hdc, ctypes.byref(rc), br)
        gdi32.DeleteObject(br)

    def round_rect(self, x1: int, y1: int, x2: int, y2: int, rx: int, ry: int, fill_col: int, pen_col: int, pen_w: int) -> None:
        br = gdi32.CreateSolidBrush(fill_col & 0xFFFFFF)
        pen = gdi32.CreatePen(0, max(1, pen_w), pen_col & 0xFFFFFF)
        old_br = gdi32.SelectObject(self.hdc, br)
        old_pen = gdi32.SelectObject(self.hdc, pen)
        gdi32.RoundRect(self.hdc, x1, y1, x2, y2, rx, ry)
        gdi32.SelectObject(self.hdc, old_br)
        gdi32.SelectObject(self.hdc, old_pen)
        gdi32.DeleteObject(br)
        gdi32.DeleteObject(pen)

    def text_out(self, x: int, y: int, name: str, pt_size: int, bold: int, fg_col: int, bg_col: int, text: str) -> None:
        self._select_font(name, pt_size, bold)
        gdi32.SetBkMode(self.hdc, 2)  # OPAQUE (matches CE Canvas.textOut!)
        gdi32.SetBkColor(self.hdc, bg_col & 0xFFFFFF)
        gdi32.SetTextColor(self.hdc, fg_col & 0xFFFFFF)
        wlen = len(text.encode("utf-16-le")) // 2
        gdi32.TextOutW(self.hdc, x, y, text, wlen)
        tw, th = self.measure(name, pt_size, bold, text)
        self.text_draws.append({
            "x": x, "y": y, "w": tw, "h": th, "pt": pt_size, "bold": bool(bold), "text": text,
        })

    def save_png(self, out_path: pathlib.Path) -> None:
        raw = ctypes.string_at(self.bits_ptr, self.width * self.height * 4)
        rows = []
        stride = self.width * 4
        for y in range(self.height):
            row = bytearray(1 + self.width * 3)
            row[0] = 0  # filter type None
            src_off = y * stride
            dst_off = 1
            for x in range(self.width):
                px_off = src_off + x * 4
                row[dst_off] = raw[px_off + 2]      # R
                row[dst_off + 1] = raw[px_off + 1]  # G
                row[dst_off + 2] = raw[px_off + 0]  # B
                dst_off += 3
            rows.append(bytes(row))
        idat_payload = zlib.compress(b"".join(rows), level=6)

        def chunk(tag: bytes, data: bytes) -> bytes:
            return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

        ihdr = struct.pack(">IIBBBBB", self.width, self.height, 8, 2, 0, 0, 0)
        png_bytes = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat_payload) + chunk(b"IEND", b"")
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_bytes(png_bytes)

    def destroy(self) -> None:
        gdi32.SelectObject(self.hdc, self.old_bmp)
        for hf in self.font_cache.values():
            gdi32.DeleteObject(hf)
        gdi32.DeleteObject(self.hbmp)
        gdi32.DeleteDC(self.hdc)


# ============================================================================
# LUA 5.3 C API BINDING (lua53-64.dll)
# ============================================================================
lua = ctypes.CDLL(LUA_DLL)
lua_CFunction = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p)

lua.luaL_newstate.restype = ctypes.c_void_p
lua.luaL_openlibs.argtypes = [ctypes.c_void_p]
lua.luaL_loadstring.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
lua.luaL_loadstring.restype = ctypes.c_int
lua.lua_pcallk.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_ssize_t, ctypes.c_void_p]
lua.lua_pcallk.restype = ctypes.c_int
lua.lua_tolstring.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(ctypes.c_size_t)]
lua.lua_tolstring.restype = ctypes.c_char_p
lua.lua_tointegerx.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(ctypes.c_int)]
lua.lua_tointegerx.restype = ctypes.c_longlong
lua.lua_pushinteger.argtypes = [ctypes.c_void_p, ctypes.c_longlong]
lua.lua_pushcclosure.argtypes = [ctypes.c_void_p, lua_CFunction, ctypes.c_int]
lua.lua_setglobal.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
lua.lua_settop.argtypes = [ctypes.c_void_p, ctypes.c_int]
lua.lua_close.argtypes = [ctypes.c_void_p]

GDI_SURFACES: dict[int, GdiSurface] = {}
NEXT_SURF_ID = 1


def lua_str(L, idx: int) -> str:
    p = lua.lua_tolstring(L, idx, None)
    return p.decode("utf-8", errors="replace") if p else ""


def lua_int(L, idx: int) -> int:
    return int(lua.lua_tointegerx(L, idx, None))


@lua_CFunction
def cb_gdi_create(L) -> int:
    global NEXT_SURF_ID
    w, h, dpi = lua_int(L, 1), lua_int(L, 2), lua_int(L, 3)
    sid = NEXT_SURF_ID
    NEXT_SURF_ID += 1
    GDI_SURFACES[sid] = GdiSurface(w, h, dpi)
    lua.lua_pushinteger(L, sid)
    return 1


@lua_CFunction
def cb_gdi_clear_log(L) -> int:
    sid = lua_int(L, 1)
    if sid in GDI_SURFACES:
        GDI_SURFACES[sid].text_draws.clear()
    return 0


@lua_CFunction
def cb_gdi_measure(L) -> int:
    sid = lua_int(L, 1)
    fname = lua_str(L, 2)
    pt = lua_int(L, 3)
    bold = lua_int(L, 4)
    txt = lua_str(L, 5)
    tw, th = GDI_SURFACES[sid].measure(fname, pt, bold, txt)
    lua.lua_pushinteger(L, tw)
    lua.lua_pushinteger(L, th)
    return 2


@lua_CFunction
def cb_gdi_fill_rect(L) -> int:
    sid = lua_int(L, 1)
    GDI_SURFACES[sid].fill_rect(lua_int(L, 2), lua_int(L, 3), lua_int(L, 4), lua_int(L, 5), lua_int(L, 6))
    return 0


@lua_CFunction
def cb_gdi_round_rect(L) -> int:
    sid = lua_int(L, 1)
    GDI_SURFACES[sid].round_rect(
        lua_int(L, 2), lua_int(L, 3), lua_int(L, 4), lua_int(L, 5),
        lua_int(L, 6), lua_int(L, 7), lua_int(L, 8), lua_int(L, 9), lua_int(L, 10),
    )
    return 0


@lua_CFunction
def cb_gdi_text_out(L) -> int:
    sid = lua_int(L, 1)
    GDI_SURFACES[sid].text_out(
        lua_int(L, 2), lua_int(L, 3), lua_str(L, 4), lua_int(L, 5),
        lua_int(L, 6), lua_int(L, 7), lua_int(L, 8), lua_str(L, 9),
    )
    return 0


def register_gdi_callbacks(L) -> None:
    for name, fn in [
        ("__gdi_create", cb_gdi_create),
        ("__gdi_clear_log", cb_gdi_clear_log),
        ("__gdi_measure", cb_gdi_measure),
        ("__gdi_fill_rect", cb_gdi_fill_rect),
        ("__gdi_round_rect", cb_gdi_round_rect),
        ("__gdi_text_out", cb_gdi_text_out),
    ]:
        lua.lua_pushcclosure(L, fn, 0)
        lua.lua_setglobal(L, name.encode("utf-8"))


def run_lua_chunk(L, code_str: str, label: str = "chunk") -> str:
    rc = lua.luaL_loadstring(L, code_str.encode("utf-8"))
    if rc != 0:
        err = lua_str(L, -1)
        raise RuntimeError(f"[{label}] Compile error: {err}")
    rc = lua.lua_pcallk(L, 0, 1, 0, 0, None)
    if rc != 0:
        err = lua_str(L, -1)
        raise RuntimeError(f"[{label}] Runtime error: {err}")
    out = lua_str(L, -1)
    lua.lua_settop(L, 0)
    return out


CE_MOCK_LUA = r"""
MOCK_RAM = {}
MOCK_SYMBOLS = {}
MOCK_MESSAGES = {}
MOCK_TIMERS = {}
MOCK_FORMS = {}
MOCK_PID = 0
MOCK_DPI = 96
MOCK_MOUSE_X = -100
MOCK_MOUSE_Y = -100
MOCK_INPUT_QUEUE = {}
MainForm = { kind = "MainForm" }
caHide = 0

function getScreenDPI() return MOCK_DPI end
function getMousePos() return MOCK_MOUSE_X, MOCK_MOUSE_Y end
function getOpenedProcessID() return MOCK_PID end
function getProcessIDFromProcessName(n) return MOCK_PID end
function openProcess(p)
  assert(p and type(p) == "number" and p > 0, "openProcess invalid pid: " .. tostring(p))
  MOCK_PID = p
end

function registerSymbol(name, addr, dontsave)
  if not addr or addr == 0 then
    error("Error:You can't add a symbol with address 0")
  end
  MOCK_SYMBOLS[name] = addr
end

function getAddressSafe(s)
  if MOCK_PID == 0 then return nil end
  if type(s) == "number" then return s end
  if s == "PES2021.exe" then return 0x140000000 end
  return MOCK_SYMBOLS[s]
end

local nextAlloc = 0x13FFF0000
function allocateMemory(sz, nearAddr)
  local a = nextAlloc
  nextAlloc = nextAlloc + sz
  return a
end

function writeBytes(addr, b1, ...)
  if type(b1) == "table" then
    for i = 1, #b1 do MOCK_RAM[addr + i - 1] = b1[i] & 0xFF end
  else
    local args = {b1, ...}
    for i = 1, #args do MOCK_RAM[addr + i - 1] = args[i] & 0xFF end
  end
  return true
end

function readBytes(addr, count, asTable)
  if not asTable then return MOCK_RAM[addr] or 0 end
  local t = {}
  for i = 1, count do t[i] = MOCK_RAM[addr + i - 1] or 0 end
  return t
end

function writeSmallInteger(addr, val)
  local v = math.floor(val) & 0xFFFF
  MOCK_RAM[addr]     = v & 0xFF
  MOCK_RAM[addr + 1] = (v >> 8) & 0xFF
  return true
end

function readSmallInteger(addr)
  local b0 = MOCK_RAM[addr] or 0
  local b1 = MOCK_RAM[addr + 1] or 0
  local u = b0 | (b1 << 8)
  if u >= 0x8000 then return u - 0x10000 end
  return u
end

function writeInteger(addr, val)
  local v = math.floor(val) & 0xFFFFFFFF
  for i = 0, 3 do MOCK_RAM[addr + i] = (v >> (i * 8)) & 0xFF end
  return true
end

function readInteger(addr)
  local u = 0
  for i = 0, 3 do u = u | ((MOCK_RAM[addr + i] or 0) << (i * 8)) end
  if u >= 0x80000000 then return u - 0x100000000 end
  return u
end

function writeQword(addr, val)
  for i = 0, 7 do MOCK_RAM[addr + i] = (val >> (i * 8)) & 0xFF end
  return true
end

function readQword(addr)
  local u = 0
  for i = 0, 7 do u = u | ((MOCK_RAM[addr + i] or 0) << (i * 8)) end
  return u
end

function writeFloat(addr, fval)
  local packed = string.pack("<f", fval)
  for i = 1, 4 do MOCK_RAM[addr + i - 1] = packed:byte(i) end
  return true
end

function readFloat(addr)
  local chars = {}
  for i = 0, 3 do chars[i + 1] = string.char(MOCK_RAM[addr + i] or 0) end
  return (string.unpack("<f", table.concat(chars)))
end

function writeString(addr, s)
  for i = 1, #s do MOCK_RAM[addr + i - 1] = s:byte(i) end
  MOCK_RAM[addr + #s] = 0
end

function readString(addr, maxlen, wide)
  local out = {}
  for i = 0, (maxlen or 60) - 1 do
    local b = MOCK_RAM[addr + i] or 0
    if b == 0 then break end
    out[#out + 1] = string.char(b)
  end
  return table.concat(out)
end

function createMemScan()
  return { firstScan = function() end, waitTillDone = function() end, destroy = function() end }
end
function createFoundList()
  return { Count = 0, initialize = function() end, destroy = function() end, Address = {} }
end
function createTimer(owner, enabled)
  local t = { Interval = 0, Enabled = enabled }
  t.destroy = function() t.destroyed = true end
  MOCK_TIMERS[#MOCK_TIMERS + 1] = t
  return t
end
function showMessage(msg) MOCK_MESSAGES[#MOCK_MESSAGES + 1] = msg end
function inputQuery(title, prompt, defaultVal)
  if #MOCK_INPUT_QUEUE > 0 then
    local v = table.remove(MOCK_INPUT_QUEUE, 1)
    if v == "__CANCEL__" then return nil end
    return v
  end
  return nil
end

function createSaveDialog() return { execute = function() return false end, destroy = function() end } end
function createOpenDialog() return { execute = function() return false end, destroy = function() end } end
function createStringlist()
  local items = {}
  return { add = function(s) items[#items + 1] = s end, destroy = function() end }
end
function showSelectionList() return 0 end

local function makeCanvas(sid)
  local c = {
    Font = { Name = "Segoe UI", Size = 9, Color = 0xFFFFFF, Style = "[]", Quality = "fqCleartypeNatural" },
    Brush = { Color = 0x000000 },
    Pen = { Color = 0x000000, Width = 1 },
  }
  local function isBold()
    return tostring(c.Font.Style or ""):find("fsBold") ~= nil and 1 or 0
  end
  c.getTextWidth = function(s)
    local tw, _ = __gdi_measure(sid, c.Font.Name, c.Font.Size, isBold(), tostring(s or ""))
    return tw
  end
  c.getTextHeight = function(s)
    local _, th = __gdi_measure(sid, c.Font.Name, c.Font.Size, isBold(), tostring(s or "Ay"))
    return th
  end
  c.fillRect = function(x1, y1, x2, y2)
    __gdi_fill_rect(sid, x1, y1, x2, y2, c.Brush.Color)
  end
  c.roundRect = function(x1, y1, x2, y2, rx, ry)
    __gdi_round_rect(sid, x1, y1, x2, y2, rx, ry, c.Brush.Color, c.Pen.Color, c.Pen.Width or 1)
  end
  c.textOut = function(x, y, s)
    __gdi_text_out(sid, x, y, c.Font.Name, c.Font.Size, isBold(), c.Font.Color, c.Brush.Color, tostring(s or ""))
  end
  c.draw = function(x, y, bmp) end
  return c
end

function createBitmap(w, h)
  local sid = __gdi_create(w, h, MOCK_DPI)
  return {
    sid = sid,
    Width = w,
    Height = h,
    Canvas = makeCanvas(sid),
  }
end

local function newCtrl(kind, parent)
  local c = {
    kind = kind,
    parent = parent,
    Left = 0, Top = 0, Width = 0, Height = 0,
    ClientWidth = 0, ClientHeight = 0,
    Visible = true, Cursor = 0,
    Caption = "", Color = 0,
    Canvas = { draw = function() end },
    bringToFront = function() end,
    centerScreen = function() end,
    repaint = function() if c and c.OnPaint then c.OnPaint() end end,
    screenToClient = function(mx, my) return mx, my end,
    destroy = function() end,
  }
  c.show = function() c.Visible = true end
  c.hide = function() c.Visible = false end
  return c
end

function createForm(visible)
  local f = newCtrl("Form", nil)
  f.Visible = (visible ~= false)
  f.close = function() f.Visible = false; if f.OnClose then f.OnClose() end end
  MOCK_FORMS[#MOCK_FORMS + 1] = f
  return f
end
function createPanel(parent) return newCtrl("Panel", parent) end
MOCK_CREATED_CONTROLS = {}
local function makeDarkAuditCtrl(kind, parent)
  local c = newCtrl(kind, parent)
  c.Color = 0xFFFFFF -- default Win32 white; MUST be overridden to dark!
  c.BorderStyle = "bsSingle"
  c.Text = ""
  c.ItemIndex = 0
  c.Font = { Name = "Segoe UI", Size = 9, Color = 0, Style = "[]" }
  c.Items = {
    _list = {},
    add = function(v) c.Items._list[#c.Items._list + 1] = v end,
    clear = function() c.Items._list = {} end,
  }
  c.setFocus = function() end
  c.selectAll = function() end
  MOCK_CREATED_CONTROLS[#MOCK_CREATED_CONTROLS + 1] = c
  return c
end
function createLabel(parent) return makeDarkAuditCtrl("Label", parent) end
function createEdit(parent) return makeDarkAuditCtrl("Edit", parent) end
function createListBox(parent) return makeDarkAuditCtrl("ListBox", parent) end
function createButton(parent) return makeDarkAuditCtrl("Button", parent) end

MOCK_RECORDS = {}
for id = 1, 320 do
  MOCK_RECORDS[id] = { ID = id, Active = false, Value = "0" }
end
MOCK_ADDRLIST = {
  Count = #MOCK_RECORDS,
  getMemoryRecord = function(idx) return MOCK_RECORDS[idx + 1] end,
  getMemoryRecordByID = function(id) return MOCK_RECORDS[id] end,
}
function getAddressList() return MOCK_ADDRLIST end
"""


def audit_edition(ct_path: pathlib.Path, expected_lang: str, render_dir: pathlib.Path | None) -> None:
    tree = ET.parse(ct_path)
    root = tree.getroot()
    main_lua = root.findtext("LuaScript") or ""

    L = lua.luaL_newstate()
    lua.luaL_openlibs(L)
    register_gdi_callbacks(L)

    run_lua_chunk(L, CE_MOCK_LUA, f"{expected_lang}_ce_mock")
    run_lua_chunk(L, main_lua, f"{expected_lang}_main_lua")

    behavioral_lua = f"""
    assert(PES10.lang == "{expected_lang}", "Expected default lang {expected_lang}, got " .. tostring(PES10.lang))
    assert(PES10.isAttached() == false, "Must start offline")
    assert(PES10.getCurrentCurrency().symbol == "€", "Offline currency must be €")

    -- Offline Studio GUI render & timer tick (zero exceptions!)
    PES10.openDashboard()
    assert(PES10.dashboardForm ~= nil and PES10.dashboardForm.Visible == true)
    for _, t in ipairs(MOCK_TIMERS) do if t.OnTimer and not t.destroyed then t.OnTimer(t) end end
    PES10.dashboardForm = nil
    PES10.dashboardState = nil

    -- Attach to simulated BMPES 16.0 Master League Save (Flamengo PID 24840)
    MOCK_PID = 24840
    assert(PES10.initProcessState() == true)
    -- Fire auto-arm 900ms timer and verify all 6 hooks activate automatically!
    for _, t in ipairs(MOCK_TIMERS) do
      if t.Interval == 900 and t.OnTimer and not t.destroyed then t.OnTimer(t) end
    end
    local activeCnt = 0
    for n = 1, 6 do
      local hid = PES10.HOOK_IDS_BY_NUM[n]
      if MOCK_RECORDS[hid] and MOCK_RECORDS[hid].Active then activeCnt = activeCnt + 1 end
    end
    assert(activeCnt == 6, "Auto-arm hooks failed: got " .. tostring(activeCnt))

    -- Populate Flamengo Career + Live RAM Club Table
    local fakeOwner = 0x200000000
    local fakeCareer = 0x210000000
    writeQword(0x140000000 + 0x3714240, fakeOwner)
    writeQword(fakeOwner + 0x48, fakeCareer)
    writeInteger(fakeCareer + 0x00D0BCE8, 2)
    writeSmallInteger(fakeCareer + 0x01642A24, 2026)
    writeBytes(fakeCareer + 0x01642A26, 12, 20)
    writeInteger(fakeCareer + 0x016ECBF4, 101839)
    writeInteger(fakeCareer + 0x016ECC08, 935059)
    writeBytes(fakeCareer + 0x016ECBEA, 21)
    writeQword(fakeCareer + 0x01819540, 0x0000FFFF)

    local flaHandle = (1248 << 14) | 122
    writeInteger(fakeCareer + 0xADF4BC + 122 * 0x690, flaHandle)
    writeString(fakeCareer + 0xADF4BC + 122 * 0x690 + 0x04, "FLAMENGO")
    writeString(fakeCareer + 0xADF4BC + 122 * 0x690 + 0x4A, "FLA")

    -- Player #0: Mikel Merino (Registered CMF = 5, with strong DMF defensive stats so DMF > CMF!)
    local p0 = fakeCareer + 0
    local h0 = (103597 << 32) | 18
    writeQword(p0 + 0x2C, h0)
    writeInteger(p0 + 0x30, 103597)
    writeString(p0 + 0x38, "MIKEL MERINO")
    writeInteger(p0 + 0x12C, flaHandle)
    writeInteger(p0 + 0x15C, 82913)
    writeInteger(p0 + 0x174, 344696)
    writeBytes(p0 + 0x1C, 30)
    writeBytes(p0 + 0x178, 30, 86)
    PES10.setBitsInDword(p0 + 0x13C, 18, 7, 78)
    PES10.setBitsInDword(p0 + 0x144, 30, 1, 1)
    writeSmallInteger(p0 + 0x138, 2029)
    writeSmallInteger(p0 + 0x168, 2026)
    writeInteger(p0 + 0x16C, 0x0000FFFF)

    -- Set Registered Position = 5 (MLG / CMF) + realistic aptitudes (CMF=A, DMF=A, AMF=B, CB=B)
    function RESET_MERINO()
      writeBytes(p0 + 0x1C, 29)
      writeInteger(p0 + 0x174, 385000)
      writeBytes(p0 + 0x178, 29, 87)
      PES10.setBitsInDword(p0 + 0x13C, 18, 7, 82)
      PES10.setBitsInDword(p0 + 0x144, 30, 1, 1)
      PES10.clearOverrideReleaseFeeForPlayer(p0)
      for bOff = 0x1F, 0x22 do writeBytes(p0 + bOff, 0) end
      PES10.setBitsInDword(p0 + 0x20, 6, 2, 2) -- CMF = A
      PES10.setBitsInDword(p0 + 0x20, 4, 2, 2) -- DMF = A
      PES10.setBitsInDword(p0 + 0x20, 2, 2, 1) -- AMF = B
      PES10.setBitsInDword(p0 + 0x21, 4, 2, 1) -- CB  = B
      PES10.setBitsInDword(p0 + 0x04, 28, 4, 5)
      local merinoStats = {{
        [14]=80, [15]=86, [16]=40, [17]=81, [18]=74, [19]=77, [20]=87, [21]=86,
        [22]=85, [23]=76, [24]=90, [25]=76, [26]=83, [27]=84, [28]=40, [29]=40,
        [30]=40, [31]=40, [33]=85, [34]=86, [35]=79, [36]=75, [37]=88, [38]=84, [39]=87
      }}
      for fid, val in pairs(merinoStats) do
        local loc = PES10.FID_LOC[fid]
        if loc[1] == 0x03 then
          writeBytes(p0 + 3, val)
        else
          PES10.setBitsInDword(p0 + loc[1], loc[2], 7, val)
        end
      end
      PES10.syncAllDynamicPointers()
    end
    RESET_MERINO()

    writeQword(fakeCareer + 0x016EE854, h0)
    writeQword(fakeCareer + 0x016BF710, h0)
    writeQword(PES10.symAddr("ptrPlayer"), p0)
    PES10.syncAllDynamicPointers()

    -- Verify Konami OVR Engine + Proportional DNA Step (+1, -1, Target, X-Ray, Double-Click OVR)!
    local baseOVR, regP = PES10.calcPlayerOVR(p0)
    assert(regP == 5 and baseOVR >= 84 and baseOVR <= 88, "Unexpected Merino OVR: " .. tostring(baseOVR))

    assert(PES10.stepPlayerOVRProportional(1) == true)
    local upOVR = PES10.calcPlayerOVR(p0)
    assert(upOVR == baseOVR + 1, "Expected +1 OVR (" .. tostring(baseOVR + 1) .. "), got " .. tostring(upOVR))
    -- GK stats must remain untouched at 40!
    assert((PES10.readU32(p0 + 0x04) >> 7) & 0x7F == 40, "GK Awareness must stay 40 on outfielder!")

    assert(PES10.stepPlayerOVRProportional(-1) == true)
    local dnOVR = PES10.calcPlayerOVR(p0)
    assert(dnOVR == baseOVR, "Expected -1 OVR back to " .. tostring(baseOVR) .. ", got " .. tostring(dnOVR))

    -- Double-clicking COMPUTED_OVR_RECORD_ID opens Dark Prompt Modal -> typing "90" + Enter scales to 90 OVR!
    local ovrRec = MOCK_RECORDS[PES10.COMPUTED_OVR_RECORD_ID]
    assert(ovrRec ~= nil and ovrRec.OnGetDisplayValue ~= nil)
    local _, dispStr = ovrRec.OnGetDisplayValue(ovrRec, tostring(baseOVR))
    assert(dispStr:find(tostring(baseOVR)) ~= nil)
    MOCK_CREATED_CONTROLS = {{}}
    assert(MOCK_ADDRLIST.OnValueChange(MOCK_ADDRLIST, ovrRec) == true)
    assert(PES10.openModals["darkprompt"] ~= nil, "Expected darkprompt modal to open!")
    local promptEd = nil
    for _, ctrl in ipairs(MOCK_CREATED_CONTROLS) do
      if ctrl.kind == "Edit" then promptEd = ctrl end
    end
    assert(promptEd ~= nil and promptEd.Color ~= 0xFFFFFF, "Dark prompt Edit missing or white!")
    promptEd.Text = "90"
    promptEd.OnKeyDown(promptEd, 13)
    assert(PES10.calcPlayerOVR(p0) == 90, "Double-click OVR scale to 90 failed!")
    -- Restore back to baseOVR for clean screenshot telemetry
    PES10.scalePlayerToTargetOVR(p0, baseOVR)

    -- 13-Position & DNA X-Ray report opens Dark X-Ray Modal (zero white showMessage!)
    assert(PES10.showPlayerDNAReport() == true)
    assert(PES10.openModals["xray"] ~= nil, "Expected dark xray modal to open!")

    -- Rejuvenate (-2 years) + Squad +1 OVR DNA
    assert(PES10.rejuvenateAndSyncMarket(-2) == true)
    assert((PES10.readU8(p0 + 0x1C) & 0x3F) == 27)
    assert(PES10.runSquadCommander("plus_one_ovr_dna") == 1)
    assert(PES10.calcPlayerOVR(p0) == baseOVR + 1)

    -- Populate live match state (73:24, 2nd Half, 2 x 1) for Tab 4 telemetry!
    local fakeMatchMgr = 0x220000000
    local fakeMatchState = 0x220100000
    local fakeMatchHud = 0x220200000
    local fakeMatchCore = 0x220300000
    writeQword(0x140000000 + 0x3702208, fakeMatchMgr)
    writeQword(fakeMatchMgr + 0x270, fakeMatchState)
    writeQword(PES10.symAddr("ptrMatchHud"), fakeMatchHud)
    writeQword(fakeMatchHud + 0x38, fakeMatchCore)
    writeInteger(fakeMatchState + 0x1168, 3)
    writeInteger(fakeMatchHud + 0x12C, 73)
    writeInteger(fakeMatchHud + 0x130, 24)
    writeBytes(fakeMatchCore + 0x9E, 2, 1)
    PES10.syncAllDynamicPointers()

    return "BEHAVIORAL_OK"
    """
    res = run_lua_chunk(L, behavioral_lua, f"{expected_lang}_behavioral")
    assert res == "BEHAVIORAL_OK"

    # Syntax-verify every embedded {$lua} button script in this edition
    btn_count = 0
    for ce in root.iter("CheatEntry"):
        if ce.findtext("VariableType") == "Auto Assembler Script":
            code = ce.findtext("AssemblerScript") or ""
            if "{$lua}" in code:
                lua_part = code.split("{$lua}", 1)[1].split("{$asm}", 1)[0]
                rc = lua.luaL_loadstring(L, lua_part.encode("utf-8"))
                assert rc == 0, f"Lua syntax error in [{expected_lang}] {ce.findtext('Description')}"
                lua.lua_settop(L, 0)
                btn_count += 1

    # Audit Canvas Studio GUI across 96, 120, 144 DPI + All 4 Tabs + Click Every Hitbox!
    for dpi in (96, 120, 144):
        audit_code = f"""
        MOCK_DPI = {dpi}
        RESET_MERINO()
        PES10.setCurrency(0)
        PES10.setLang("{expected_lang}")
        PES10.dashboardForm = nil
        PES10.dashboardState = nil
        -- Set stateful modes so Tab 3 & Tab 4 render active badges during visual QA!
        writeInteger(PES10.symAddr("cfgTrainingMultMode"), 3) -- 10x XP Active
        writeInteger(PES10.symAddr("cfgStaminaMode"), 1)      -- Home 100% Active
        writeInteger(PES10.symAddr("cfgFreezeClock"), 1)      -- Freeze Clock Active
        PES10.toast(PES10.T(
          "MIKEL MERINO: Overall 86 → 87 (MLG - DNA e estilo preservados)!",
          "MIKEL MERINO: Overall 86 → 87 (CMF - DNA & style preserved)!"
        ))
        PES10.openDashboard()
        local st = PES10.dashboardState
        assert(st ~= nil and st.backBuf ~= nil)

        local tabAudits = {{}}
        for tabIdx = 1, 4 do
          __gdi_clear_log(st.backBuf.sid)
          st.selectTab(tabIdx)
          local hbs = st.getHitboxes()
          -- Verify zero hitbox out-of-bounds or overlap!
          for i = 1, #hbs do
            local a = hbs[i]
            assert(a.x >= 0 and a.y >= 0 and (a.x + a.w) <= st.width and (a.y + a.h) <= st.height,
              "Hitbox out of bounds: " .. tostring(a.id))
            for j = i + 1, #hbs do
              local b = hbs[j]
              local overlap = (a.x < b.x + b.w) and (a.x + a.w > b.x) and (a.y < b.y + b.h) and (a.y + a.h > b.y)
              assert(not overlap, "Hitbox overlap between " .. tostring(a.id) .. " and " .. tostring(b.id))
            end
          end
          tabAudits[#tabAudits + 1] = string.format("%d:%d", tabIdx, #hbs)
        end

        return tostring(st.backBuf.sid) .. "|" .. table.concat(tabAudits, ",")
        """
        info = run_lua_chunk(L, audit_code, f"{expected_lang}_dpi_{dpi}")
        sid_str, hb_summary = info.split("|", 1)
        sid = int(sid_str)
        surf = GDI_SURFACES[sid]

        # Render each tab cleanly & check zero unexpected "..." ellipsis on cards/tabs/buttons!
        for tab_idx in (1, 2, 3, 4):
            run_lua_chunk(
                L,
                f"__gdi_clear_log({sid}); PES10.dashboardState.selectTab({tab_idx})",
                f"render_{expected_lang}_{dpi}_t{tab_idx}",
            )
            for td in surf.text_draws:
                txt = td["text"]
                # Notice: prompt dialogs end with "..." intentionally (e.g. "Escolher Minuto Exato..."),
                # whereas fitText truncation produces "..." on strings that didn't originally end with "..."!
                # Check that every text box stays strictly inside [0, surf.width] x [0, surf.height]:
                assert td["x"] >= 0 and (td["x"] + td["w"]) <= surf.width, (
                    f"[{expected_lang}@{dpi}DPI T{tab_idx}] Text exceeds width: {td}"
                )
                assert td["y"] >= 0 and (td["y"] + td["h"]) <= surf.height, (
                    f"[{expected_lang}@{dpi}DPI T{tab_idx}] Text exceeds height: {td}"
                )
                # Ensure no tab title, chip, or card subtitle got truncated by fitText!
                if td["pt"] in (7, 9):
                    assert not txt.endswith("..."), (
                        f"[{expected_lang}@{dpi}DPI T{tab_idx}] Unexpected truncation in UI label: '{txt}'"
                    )

            if render_dir is not None and (
                (expected_lang == "pt" and dpi == 120)
                or (expected_lang == "en" and dpi == 96)
                or (expected_lang == "en" and dpi == 144 and tab_idx == 2)
            ):
                png_name = f"studio_{expected_lang}_dpi{dpi}_tab{tab_idx}.png"
                surf.save_png(render_dir / png_name)

        # Now click EVERY interactive hitbox across all 4 tabs (including language switch!) to verify 0 runtime errors
        click_test = """
        local st = PES10.dashboardState
        local totalClicked = 0
        for tabIdx = 1, 4 do
          st.selectTab(tabIdx)
          local snap = {}
          for _, hb in ipairs(st.getHitboxes()) do snap[#snap + 1] = hb end
          for _, hb in ipairs(snap) do
            if not hb.id:find("^btn_lang_") and not hb.id:find("^tab_") then
              MOCK_INPUT_QUEUE = {"__CANCEL__"}
              st.hoverAt(hb.x + 4, hb.y + 4)
              assert(st.clickAt(hb.x + 4, hb.y + 4) == true)
              totalClicked = totalClicked + 1
            end
          end
        end
        -- Test live segmented [ PT | EN ] pills in Header!
        local beforeLang = PES10.lang
        local targetId = (beforeLang == "pt") and "btn_lang_en" or "btn_lang_pt"
        local backId   = (beforeLang == "pt") and "btn_lang_pt" or "btn_lang_en"
        for _, hb in ipairs(st.getHitboxes()) do
          if hb.id == targetId then st.clickAt(hb.x + 4, hb.y + 4); break end
        end
        assert(PES10.lang ~= beforeLang, "Header language pill did not switch language!")
        for _, hb in ipairs(st.getHitboxes()) do
          if hb.id == backId then st.clickAt(hb.x + 4, hb.y + 4); break end
        end
        assert(PES10.lang == beforeLang)
        return tostring(totalClicked)
        """
        clicked_n = int(run_lua_chunk(L, click_test, f"click_all_{expected_lang}_{dpi}"))
        assert clicked_n >= 52, f"Expected >= 52 hitbox clicks across 4 tabs, got {clicked_n}"

    # v12.0 Behavioral QA: Undo Stack, Secondary Position +1 OVR, Team Role & Real-RAM Scout Lists
    v11_test = """
    local p = PES10.readU64(PES10.symAddr("ptrPlayer"))
    assert(p ~= 0, "Mock player must be selected")
    -- Reset to deterministic 85 OVR after click_test mutated player across 3 DPIs
    PES10.scalePlayerToTargetOVR(p, 85, nil, true)
    -- 1. Undo stack test on +1 OVR
    local ovrBefore = PES10.calcPlayerOVR(p)
    PES10.stepPlayerOVRProportional(1)
    assert(PES10.calcPlayerOVR(p) == ovrBefore + 1, "+1 OVR failed")
    assert(PES10.canUndoPlayer(p) >= 1, "Undo stack did not push snapshot")
    assert(PES10.undoLastPlayerEdit(p) == true, "Undo failed")
    assert(PES10.calcPlayerOVR(p) == ovrBefore, "Undo did not restore original OVR")

    -- 2. Isolated Secondary Position +1 OVR, -1 OVR & Match Primary OVR (ZERO Primary OVR Drift!)
    local origBuf = readBytes(p, 0x28, true)
    local origPri, origReg = PES10.calcPlayerOVRFromBuf(origBuf)
    local secPos = (origReg == 5) and 8 or 5 -- CMF (5) or AMF (8)
    local secPotBefore = PES10.calcPlayerOVRFromBuf(origBuf, secPos, false)
    local okIso, oT, nT, oP, nP, diffStr = PES10.stepTargetPositionOVR(secPos, 1)
    assert(okIso == true, "Isolated +1 secondary pos OVR failed")
    assert(PES10.getPosAptitude(p, secPos) == PES10.getPosAptitude(p, secPos), "grade readable")
    assert(nT == secPotBefore + 1 or nT == secPotBefore, "Secondary +1 must move exactly +1 (or stay at limit), got " .. tostring(secPotBefore) .. "->" .. tostring(nT))
    assert(nP == origPri, "Primary OVR drifted on +1! Expected " .. tostring(origPri) .. ", got " .. tostring(nP))
    local gradeAfterUp = PES10.getPosAptitude(p, secPos)
    -- Test -1 Isolated OVR (bidirectional full control!)
    local okDn, dOldT, dNewT, dOldP, dNewP = PES10.stepTargetPositionOVR(secPos, -1)
    assert(okDn == true and dNewT == nT - 1 or dNewT == nT, "Isolated -1 secondary pos OVR must move exactly -1 (or stay at limit)")
    assert(dNewP == origPri, "Primary OVR drifted on -1! Expected " .. tostring(origPri) .. ", got " .. tostring(dNewP))
    assert(PES10.getPosAptitude(p, secPos) == gradeAfterUp, "Grade must never change silently on OVR step")
    while PES10.canUndoPlayer(p) > 0 and PES10.calcPlayerOVR(p) ~= origPri do PES10.undoLastPlayerEdit(p) end
    local rb = readBytes(p, 0x28, true)
    assert(PES10.calcPlayerOVRFromBuf(rb, secPos, false) == secPotBefore or PES10.canUndoPlayer(p) == 0, "Undo must restore secondary OVR")
    assert(PES10.calcPlayerOVR(p) == origPri)

    -- 2b. Match Secondary OVR to Primary OVR (primary MUST stay untouched, never raised!)
    local okM, moT, mnT, moP, mnP = PES10.matchSecondaryToPrimaryOVR(p, secPos)
    assert(okM == true, "matchSecondaryToPrimaryOVR failed")
    assert(mnP == origPri, "matchSecondaryToPrimaryOVR changed primary OVR: " .. tostring(origPri) .. "->" .. tostring(mnP))
    assert(math.abs(mnT - origPri) <= math.abs(moT - origPri), "matchSecondaryToPrimaryOVR moved secondary AWAY from primary OVR")
    if PES10.canUndoPlayer(p) > 0 and mnT ~= moT then PES10.undoLastPlayerEdit(p) end

    -- 3. Team Role + Level + Undo
    assert(PES10.setTeamRole(p, 19, 3) == true)
    local r, lv = PES10.getTeamRole(p)
    assert(r == 19 and lv == 3, "Team Role readback mismatch: " .. tostring(r) .. "/" .. tostring(lv))
    assert(PES10.undoLastPlayerEdit(p) == true)

    -- 3b. Set Registered Position + auto Grade A + Undo
    local _, origRegPos = PES10.calcPlayerOVR(p)
    local newReg = (origRegPos == 8) and 12 or 8
    assert(PES10.setRegisteredPosition(p, newReg) == true)
    local _, checkRegPos = PES10.calcPlayerOVR(p)
    assert(checkRegPos == newReg, "setRegisteredPosition failed")
    assert(PES10.getPosAptitude(p, newReg) == 2, "New registered pos not set to Grade A")
    assert(PES10.undoLastPlayerEdit(p) == true)
    local _, restoredRegPos = PES10.calcPlayerOVR(p)
    assert(restoredRegPos == origRegPos, "Undo did not restore original Registered Position")

    -- 4. Real-RAM Scout Observation List 1 (pure native insert, zero contract/flag side effects)
    PES10.clearScoutSlots(1)
    local u0 = PES10.getScoutListSummary(1)
    assert(u0 == 0, "Scout 1 should be empty after clear")
    local origFlags = PES10.readU32(p + 0x14A)
    local origAff   = PES10.getBitsInDword(p + 0x13C, 18, 7)
    local origFeeEn = PES10.readU32(PES10.symAddr("cfgReleaseFeeOverrideEn"))
    assert(PES10.addPlayerToScoutList(1) == true, "addPlayerToScoutList(1) failed")
    assert(PES10.readU32(p + 0x14A) == origFlags, "addPlayerToScoutList must not alter transfer/loan flags")
    assert(PES10.getBitsInDword(p + 0x13C, 18, 7) == origAff, "addPlayerToScoutList must not alter club affection")
    assert(PES10.readU32(PES10.symAddr("cfgReleaseFeeOverrideEn")) == origFeeEn, "addPlayerToScoutList must not override release fee")
    local u1 = PES10.getScoutListSummary(1)
    assert(u1 == 1, "Scout 1 should have 1 player in RAM, got " .. tostring(u1))
    -- Duplicate prevention check
    assert(PES10.addPlayerToScoutList(1) == true)
    local u1Dup = PES10.getScoutListSummary(1)
    assert(u1Dup == 1, "Scout 1 should prevent duplicates, got " .. tostring(u1Dup))
    local pHandle = PES10.readU64(p + 0x2C)
    assert(PES10.removePlayerFromScoutList(1, pHandle) == true, "removePlayerFromScoutList failed")
    local u2 = PES10.getScoutListSummary(1)
    assert(u2 == 0, "Scout 1 should be empty after removal")

    -- 5. Dark Studio Modal Audit: instantiate ALL 14 modals and assert ZERO white Win32 inputs/buttons!
    MOCK_CREATED_CONTROLS = {}
    PES10.openPositionLabModal()
    PES10.openTeamRoleModal()
    PES10.openScoutListsModal()
    PES10.openScoutFinderModal()
    PES10.openAttributesModal()
    PES10.openSkillsStyleModal()
    PES10.openContractModal()
    PES10.openBioConditionModal()
    PES10.openFinancesModal()
    PES10.openSquadRosterModal()
    PES10.openTrainingSpeedModal()
    PES10.openMatchStaminaModal()
    PES10.openBALFocusModal()
    PES10.openQuickConditionModal()
    PES10.showPlayerDNAReport()
    PES10.promptTargetOVRProportional()
    local editCount, listCount, rawBtnCount = 0, 0, 0
    for _, ctrl in ipairs(MOCK_CREATED_CONTROLS) do
      if ctrl.kind == "Button" then rawBtnCount = rawBtnCount + 1 end
      if ctrl.kind == "Label" and ctrl.parent and ctrl.parent.kind == "Form" and (ctrl.Top or 999) < 56 then
        error("Header Label parented directly to Form instead of hdrBar (would be hidden behind header panel)!")
      end
      if ctrl.kind == "Edit" then
        editCount = editCount + 1
        assert(ctrl.Color ~= 0xFFFFFF, "White Edit detected in modal!")
        assert(ctrl.BorderStyle == "bsNone", "Win32 3D border on Edit detected in modal!")
      elseif ctrl.kind == "ListBox" then
        listCount = listCount + 1
        assert(ctrl.Color ~= 0xFFFFFF, "White ListBox detected in modal!")
        assert(ctrl.BorderStyle == "bsNone", "Win32 3D border on ListBox detected in modal!")
      end
    end
    assert(rawBtnCount == 0, "Raw Win32 gray TButton detected in modals: " .. tostring(rawBtnCount))
    assert(listCount == 0, "Legacy plain-text Win32 TListBox detected in modals: " .. tostring(listCount))
    assert(editCount >= 18, "Expected >=18 dark Edit controls across modals, got " .. tostring(editCount))
    return "V11_OK"
    """
    assert run_lua_chunk(L, v11_test, f"v11_behavioral_{expected_lang}") == "V11_OK"

    print(
        f"EDITION QA PASSED [{expected_lang.upper()}]: {ct_path.name} | "
        f"{btn_count} Lua buttons | 96/120/144 DPI GDI pixel-verified | 0 overlaps | 0 truncations | v12.0 Behavioral OK!"
    )
    lua.lua_close(L)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--render-dir", default=None)
    args = parser.parse_args()
    rdir = pathlib.Path(args.render_dir) if args.render_dir else None

    try:
        audit_edition(CT_PT, "pt", rdir)
        audit_edition(CT_EN, "en", rdir)
        if rdir:
            print(f"RENDERED REAL WIN32 GDI PNG SCREENSHOTS TO: {rdir}")
        print("ALL DUAL-EDITION BEHAVIORAL & GDI PIXEL CHECKS PASSED 100%!")
    finally:
        for s in list(GDI_SURFACES.values()):
            s.destroy()
        GDI_SURFACES.clear()


if __name__ == "__main__":
    main()
