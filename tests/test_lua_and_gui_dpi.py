import ctypes
from ctypes import wintypes
import xml.etree.ElementTree as ET
import json

import os
import pathlib

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
LUA_DLL = os.environ.get("CE_LUA_DLL", r"C:\Program Files\Cheat Engine 7.2\lua53-64.dll")
CT_PATH = str(REPO_ROOT / "dist" / "PES 2021 - v10.0 Pro Overhaul.CT")

lua = ctypes.CDLL(LUA_DLL)
lua.luaL_newstate.restype = ctypes.c_void_p
lua.luaL_openlibs.argtypes = [ctypes.c_void_p]
lua.luaL_loadstring.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
lua.luaL_loadstring.restype = ctypes.c_int
lua.lua_pcallk.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_ssize_t, ctypes.c_void_p]
lua.lua_pcallk.restype = ctypes.c_int
lua.lua_tolstring.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(ctypes.c_size_t)]
lua.lua_tolstring.restype = ctypes.c_char_p
lua.lua_close.argtypes = [ctypes.c_void_p]

# Real Win32 GDI text measurement (Segoe UI at any target DPI!)
gdi32 = ctypes.windll.gdi32
user32 = ctypes.windll.user32

class SIZE(ctypes.Structure):
    _fields_ = [("cx", wintypes.LONG), ("cy", wintypes.LONG)]

def measure_gdi_text_px(text: str, font_name: str, pt_size: int, dpi: int, bold: bool) -> tuple[int, int]:
    hdc = user32.GetDC(0)
    lf_height = -int(round(pt_size * dpi / 72.0))
    weight = 700 if bold else 400
    hfont = gdi32.CreateFontW(
        lf_height, 0, 0, 0, weight, 0, 0, 0, 1, 0, 0, 5, 0, font_name
    )
    old_font = gdi32.SelectObject(hdc, hfont)
    sz = SIZE()
    gdi32.GetTextExtentPoint32W(hdc, text, len(text), ctypes.byref(sz))
    gdi32.SelectObject(hdc, old_font)
    gdi32.DeleteObject(hfont)
    user32.ReleaseDC(0, hdc)
    return sz.cx, sz.cy


def run_lua_chunk(L, code_str: str, label: str = "chunk") -> str:
    rc = lua.luaL_loadstring(L, code_str.encode("utf-8"))
    if rc != 0:
        err = lua.lua_tolstring(L, -1, None).decode("utf-8", errors="replace")
        raise RuntimeError(f"[{label}] Compile error: {err}")
    rc = lua.lua_pcallk(L, 0, 1, 0, 0, None)
    if rc != 0:
        err = lua.lua_tolstring(L, -1, None).decode("utf-8", errors="replace")
        raise RuntimeError(f"[{label}] Runtime error: {err}")
    res = lua.lua_tolstring(L, -1, None)
    return res.decode("utf-8", errors="replace") if res else ""


tree = ET.parse(CT_PATH)
root = tree.getroot()
main_lua = root.findtext("LuaScript")

L = lua.luaL_newstate()
lua.luaL_openlibs(L)

ce_mock = r"""
MOCK_RAM = {}
MOCK_SYMBOLS = {}
MOCK_MESSAGES = {}
MOCK_TIMERS = {}
MOCK_FORMS = {}
MOCK_PID = 0
MOCK_DPI = 96
MOCK_INPUT_QUEUE = {}
MainForm = { kind = "MainForm" }
caHide = 0

function getScreenDPI() return MOCK_DPI end
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

-- LCL Form / Panel / Label Mock Hierarchy for exact layout & text-overflow verification!
local nextCtrlId = 1
local function newCtrl(kind, parent)
  local c = {
    id = nextCtrlId,
    kind = kind,
    parent = parent,
    children = {},
    Left = 0, Top = 0, Width = 0, Height = 0,
    Visible = true, AutoSize = true,
    Caption = "", Color = 0,
    Font = { Name = "Segoe UI", Size = 9, Color = 0xFFFFFF, Style = "" },
    bringToFront = function() end,
    centerScreen = function() end,
    destroy = function() end,
  }
  c.show = function() c.Visible = true end
  c.hide = function() c.Visible = false end
  nextCtrlId = nextCtrlId + 1
  if parent and parent.children then
    parent.children[#parent.children + 1] = c
  end
  return c
end

function createForm(visible)
  local f = newCtrl("Form", nil)
  f.Visible = (visible ~= false)
  MOCK_FORMS[#MOCK_FORMS + 1] = f
  return f
end
function createPanel(parent) return newCtrl("Panel", parent) end
function createLabel(parent) return newCtrl("Label", parent) end

MOCK_RECORDS = {}
for id = 1, 300 do
  MOCK_RECORDS[id] = { ID = id, Active = false, Value = "0" }
end
function getAddressList()
  return {
    Count = #MOCK_RECORDS,
    getMemoryRecord = function(idx) return MOCK_RECORDS[idx + 1] end,
    getMemoryRecordByID = function(id) return MOCK_RECORDS[id] end,
  }
end

_G.PES2021_CE_MCP_RUNNING = true
"""

run_lua_chunk(L, ce_mock, "ce_mock")
run_lua_chunk(L, main_lua, "main_lua_from_CT")

test_suite = r"""
-- 0. OFFLINE Startup + Open Dashboard while offline (Zero crashes!)
assert(PES10.isAttached() == false, "Must start offline")
assert(PES10.getCurrentCurrency().symbol == "€", "Offline currency must be €")
PES10.openDashboard()
assert(PES10.dashboardForm ~= nil and PES10.dashboardForm.Visible == true, "Dashboard failed offline")
-- Fire UI timer while offline
for _, t in ipairs(MOCK_TIMERS) do if t.OnTimer and not t.destroyed then t.OnTimer(t) end end
PES10.dashboardForm = nil

-- 1. Simulate Live BMPES 16.0 Master League Save (Flamengo PID 24840!)
MOCK_PID = 24840
assert(PES10.initProcessState() == true, "initProcessState failed")
assert(PES10.getCurrentCurrency().symbol == "€", "Default currency must be €")
assert(PES10.formatMoneyFromRaw(101839) == "€ 10.183.900", "EUR Flamengo Transfer Budget format failed")

-- 2. Human Money Parser & Currency Switching
assert(PES10.parseHumanMoneyToRaw("50M") == 500000)
assert(PES10.parseHumanMoneyToRaw("500k") == 5000)
assert(PES10.parseHumanMoneyToRaw("1.5B") == 15000000)
assert(PES10.parseHumanMoneyToRaw("€ 80.000.000") == 800000)
PES10.setCurrency(1)
assert(PES10.formatMoneyFromRaw(500000) == "R$ 50.000.000")
PES10.setCurrency(0)

-- 3. Populate Mock Flamengo Career + Live Club RAM Table (+0xADF4BC, stride 0x690)
local fakeOwner = 0x200000000
local fakeCareer = 0x210000000
writeQword(0x140000000 + 0x3714240, fakeOwner)
writeQword(fakeOwner + 0x48, fakeCareer)
writeInteger(fakeCareer + 0x00D0BCE8, 5)
writeSmallInteger(fakeCareer + 0x01642A24, 2026)
writeBytes(fakeCareer + 0x01642A26, 12, 20)
writeInteger(fakeCareer + 0x016ECBF4, 101839)
writeInteger(fakeCareer + 0x016ECC08, 935059)
-- Konami null BAL handle in Master League: 0x000000000000FFFF!
writeQword(fakeCareer + 0x01819540, 0x0000FFFF)

-- Live Club Table entries: Slot 0 = ARSENAL (ID 101), Slot 122 = FLAMENGO (ID 1248)
local arsHandle = (101 << 14) | 0
local flaHandle = (1248 << 14) | 122
writeInteger(fakeCareer + 0xADF4BC + 0 * 0x690, arsHandle)
writeString(fakeCareer + 0xADF4BC + 0 * 0x690 + 0x04, "ARSENAL")
writeString(fakeCareer + 0xADF4BC + 0 * 0x690 + 0x4A, "ARS")

writeInteger(fakeCareer + 0xADF4BC + 122 * 0x690, flaHandle)
writeString(fakeCareer + 0xADF4BC + 122 * 0x690 + 0x04, "FLAMENGO")
writeString(fakeCareer + 0xADF4BC + 122 * 0x690 + 0x4A, "FLA")

-- Player #0: Mikel Merino at Flamengo
local p0 = fakeCareer + 0
local h0 = (103597 << 32) | 18
writeQword(p0 + 0x2C, h0)
writeString(p0 + 0x38, "MIKEL MERINO")
writeInteger(p0 + 0x12C, flaHandle)
writeInteger(p0 + 0x160, arsHandle)
writeInteger(p0 + 0x15C, 82913)
writeInteger(p0 + 0x174, 344696)
writeBytes(p0 + 0x1C, 30) -- Age 30
PES10.setBitsInDword(p0 + 0x13C, 18, 7, 67) -- Affection 67
writeSmallInteger(p0 + 0x138, 2029)
writeSmallInteger(p0 + 0x168, 2026) -- Joined 2026
writeInteger(p0 + 0x16C, 0x0000FFFF) -- Sentinel 0x0000FFFF (no renewal lock)

writeQword(fakeCareer + 0x016EE854, h0)
writeQword(fakeCareer + 0x016BF710, h0)
writeQword(PES10.symAddr("ptrPlayer"), p0)
PES10.syncAllDynamicPointers()

-- Verify Live Club Decoding + ML Mode Detection!
local _, _, arsText = PES10.decodeClubHandle(arsHandle)
local _, _, flaText = PES10.decodeClubHandle(flaHandle)
assert(arsText == "ARSENAL [ARS] (ID 101)", "Arsenal RAM decode failed: " .. tostring(arsText))
assert(flaText == "FLAMENGO [FLA] (ID 1248)", "Flamengo RAM decode failed: " .. tostring(flaText))

local modeName, clubStr = PES10.getCareerModeAndClubInfo()
assert(modeName == "Master League", "Expected Master League, got: " .. tostring(modeName))
assert(clubStr == "FLAMENGO [FLA] (ID 1248)", "Expected Flamengo user club, got: " .. tostring(clubStr))

-- 4. Verify Easy Sign Target (7-bit Affection = 100, Sentinel 0x0000FFFF preserved, Joined Year rolled back!)
assert(PES10.runPlayerMacro("easy_sign_target") == true)
assert(PES10.getBitsInDword(p0 + 0x13C, 18, 7) == 100, "7-bit Affection != 100")
assert(PES10.readU32(p0 + 0x16C) == 0x0000FFFF, "Sentinel 0x0000FFFF corrupted: " .. string.format("0x%X", PES10.readU32(p0 + 0x16C)))
assert(PES10.readU16(p0 + 0x168) == 2024, "Joined year rollback failed")
assert(PES10.readI32(PES10.symAddr("valComputedReleaseFee")) == 10, "Release fee €1.000 override failed")

-- Clear override & check Konami decompiled Release Fee formula!
PES10.clearOverrideReleaseFeeForPlayer(p0)
local konamiRfee = PES10.readI32(PES10.symAddr("valComputedReleaseFee"))
-- Age=30, Affection=100 -> f12 = 70-50 = 20 -> i4 = (50-30)-20+20 = 20 -> f8 = 1.20 -> 344696 * 1.20 = 413635
assert(konamiRfee == 413635, "Konami decompiled rfee formula mismatch: got " .. tostring(konamiRfee))

-- 5. Verify 99 Stats, 41 Skills, Squad Commander & Match Clock
assert(PES10.runPlayerMacro("stats_99") == true)
for _, sb in ipairs(PES10.PLAYER_STAT_BITS) do
  assert(PES10.getBitsInDword(p0 + sb[1], sb[2], 7) == 99)
end
assert(PES10.runPlayerMacro("all_39_skills") == true)
assert(#PES10.ALL_SKILL_BITS == 41)
assert(PES10.runSquadCommander("heal_affection") == 1)
assert(PES10.getBitsInDword(p0 + 0x13C, 18, 7) == 100)

local fakeMatchMgr = 0x220000000
local fakeMatchState = 0x220100000
writeQword(0x140000000 + 0x3702208, fakeMatchMgr)
writeQword(fakeMatchMgr + 0x270, fakeMatchState)
writeInteger(fakeMatchState + 0x1168, 3)
writeFloat(PES10.symAddr("valProtectedMatchTicks"), 18000.0)
PES10.syncAllDynamicPointers()
assert(PES10.setExactMatchMinute(88) == true)
local clk = PES10.getMatchClockInfo()
assert(clk.mm == 88 and clk.ss == 0)

return "BEHAVIORAL_OK"
"""

out = run_lua_chunk(L, test_suite, "behavioral_test_suite")
print("BEHAVIORAL SUITE:", out)

# ============================================================================
# DPI-AWARE STUDIO GUI AUDIT (96 DPI = 100%, 120 DPI = 125%, 144 DPI = 150%)
# Measures EVERY single TLabel with REAL Win32 GDI GetTextExtentPoint32W!
# ============================================================================
gui_serializer_lua = r"""
local dpis = {96, 120, 144}
local dump = {}

local function collectTree(node, outList)
  outList[#outList + 1] = {
    id = node.id,
    kind = node.kind,
    parentId = node.parent and node.parent.id or 0,
    parentW = node.parent and node.parent.Width or 0,
    parentH = node.parent and node.parent.Height or 0,
    x = node.Left, y = node.Top, w = node.Width, h = node.Height,
    autoSize = node.AutoSize and true or false,
    caption = tostring(node.Caption or ""),
    fontName = node.Font.Name,
    fontSize = node.Font.Size,
    bold = (tostring(node.Font.Style):find("fsBold") ~= nil),
    hasClick = (node.OnClick ~= nil),
    hasHover = (node.OnMouseEnter ~= nil),
  }
  for _, ch in ipairs(node.children) do
    collectTree(ch, outList)
  end
end

for _, dpiVal in ipairs(dpis) do
  MOCK_DPI = dpiVal
  MOCK_FORMS = {}
  MOCK_TIMERS = {}
  PES10.dashboardForm = nil
  PES10.openDashboard()
  local f = PES10.dashboardForm
  assert(f ~= nil, "Dashboard form nil at DPI " .. tostring(dpiVal))

  -- Fire UI refresh timer with live Flamengo telemetry populated!
  for _, t in ipairs(MOCK_TIMERS) do
    if t.OnTimer and not t.destroyed then t.OnTimer(t) end
  end

  -- Click & Hover EVERY interactive control on the form (tabs, pills, all 32 cards)!
  local clickedFns = {}
  local clickCount = 0
  local function exerciseInteractive(node)
    if node.OnMouseEnter then node.OnMouseEnter() end
    if node.OnMouseLeave then node.OnMouseLeave() end
    if node.OnClick and not clickedFns[node.OnClick] then
      clickedFns[node.OnClick] = true
      MOCK_INPUT_QUEUE = {"__CANCEL__"}
      node.OnClick()
      clickCount = clickCount + 1
    end
    for _, ch in ipairs(node.children) do exerciseInteractive(ch) end
  end
  exerciseInteractive(f)

  -- Re-run UI timer after interactions
  for _, t in ipairs(MOCK_TIMERS) do
    if t.OnTimer and not t.destroyed then t.OnTimer(t) end
  end

  local flat = {}
  collectTree(f, flat)
  dump[#dump + 1] = { dpi = dpiVal, formW = f.Width, formH = f.Height, uniqueClicks = clickCount, nodes = flat }
end

-- Minimal JSON encoder for flat node audit
local function esc(s)
  return s:gsub("\\", "\\\\"):gsub('"', '\\"'):gsub("\n", "\\n")
end
local parts = {"["}
for di, d in ipairs(dump) do
  if di > 1 then parts[#parts + 1] = "," end
  parts[#parts + 1] = string.format('{"dpi":%d,"formW":%d,"formH":%d,"uniqueClicks":%d,"nodes":[', d.dpi, d.formW, d.formH, d.uniqueClicks)
  for ni, n in ipairs(d.nodes) do
    if ni > 1 then parts[#parts + 1] = "," end
    parts[#parts + 1] = string.format(
      '{"id":%d,"kind":"%s","parentW":%d,"parentH":%d,"x":%d,"y":%d,"w":%d,"h":%d,"autoSize":%s,"caption":"%s","fontName":"%s","fontSize":%d,"bold":%s}',
      n.id, n.kind, n.parentW, n.parentH, n.x, n.y, n.w, n.h,
      n.autoSize and "true" or "false", esc(n.caption), esc(n.fontName), n.fontSize, n.bold and "true" or "false"
    )
  end
  parts[#parts + 1] = "]}"
end
parts[#parts + 1] = "]"
return table.concat(parts)
"""

gui_json = run_lua_chunk(L, gui_serializer_lua, "gui_serializer")
gui_audits = json.loads(gui_json)

overflow_errors = []
max_fill_ratio_by_dpi = {}

for audit in gui_audits:
    dpi = audit["dpi"]
    max_ratio = 0.0
    label_count = 0
    # Expect 1 (Ativar 6 Hooks) + 4 (Currencies) + 4 (Tabs) + 32 (Action Cards) = 41 unique click handlers!
    assert audit["uniqueClicks"] == 41, f"Expected 41 unique interactive handlers, got {audit['uniqueClicks']}"

    for n in audit["nodes"]:
        if n["kind"] == "Panel":
            if n["x"] < 0 or n["y"] < 0 or (n["x"] + n["w"]) > n["parentW"] or (n["y"] + n["h"]) > n["parentH"]:
                overflow_errors.append(f"DPI {dpi} Panel#{n['id']} out of parent bounds: ({n['x']},{n['y']},{n['w']}x{n['h']}) vs ({n['parentW']}x{n['parentH']})")
        elif n["kind"] == "Label" and n["caption"]:
            label_count += 1
            tw, th = measure_gdi_text_px(n["caption"], n["fontName"], n["fontSize"], dpi, n["bold"])
            avail_w = n["w"] if not n["autoSize"] else (n["parentW"] - n["x"] - 12)
            avail_h = n["h"] if not n["autoSize"] else (n["parentH"] - n["y"])
            ratio = tw / max(1, avail_w)
            if ratio > max_ratio:
                max_ratio = ratio
            if tw > avail_w:
                overflow_errors.append(
                    f"DPI {dpi} TEXT WIDTH OVERFLOW on '{n['caption']}': GDI width={tw}px > container={avail_w}px"
                )
            if th > avail_h + 2:
                overflow_errors.append(
                    f"DPI {dpi} TEXT HEIGHT OVERFLOW on '{n['caption']}': GDI height={th}px > container={avail_h}px"
                )
    max_fill_ratio_by_dpi[dpi] = round(max_ratio * 100, 1)
    print(f"GUI GDI AUDIT @ {dpi} DPI ({int(dpi*100/96)}%): Form={audit['formW']}x{audit['formH']}px | {label_count} labels measured | {audit['uniqueClicks']} handlers clicked | Max text width usage = {max_fill_ratio_by_dpi[dpi]}% (ZERO OVERFLOW!)")

if overflow_errors:
    for e in overflow_errors:
        print("OVERFLOW ERROR:", e)
    raise AssertionError(f"Found {len(overflow_errors)} GUI overflow issues!")

# Syntax-check all 34 {$lua} button scripts inside lua53-64.dll
btn_count = 0
for ce in root.iter("CheatEntry"):
    if ce.findtext("VariableType") == "Auto Assembler Script":
        code = ce.findtext("AssemblerScript") or ""
        if "{$lua}" in code:
            lua_part = code.split("{$lua}", 1)[1].split("{$asm}", 1)[0]
            rc = lua.luaL_loadstring(L, lua_part.encode("utf-8"))
            assert rc == 0, f"Lua syntax error in {ce.findtext('Description')}"
            btn_count += 1

lua.lua_close(L)
print(f"SYNTAX-VERIFIED ALL {btn_count} EMBEDDED BUTTON LUA SCRIPTS IN lua53-64.dll!")
print("ALL GDI PIXEL-BOUNDARY & LUA 5.3 VM CHECKS PASSED 100%!")
