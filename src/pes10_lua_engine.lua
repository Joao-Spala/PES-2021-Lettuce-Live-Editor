-- ============================================================================
-- PES 2021 - v12.0 ULTIMATE STUDIO OVERHAUL (SIDER READY)
-- Bilingual Core Engine (PT-BR & EN) + Konami Exact OVR DNA Solver + Canvas GUI
-- + Visual Studio Modals (Radar 30K, Position Lab, Team Role, Real-RAM Scouts)
-- + 10-Level Automatic Player Undo Stack
-- Validated live on PES2021.exe v1.7.2.0 + Sider 7.3.3
-- ============================================================================

if PES10 then
  if PES10.bgTimer then pcall(function() PES10.bgTimer.Enabled = false; PES10.bgTimer.destroy() end) end
  if PES10.dashboardForm then pcall(function() PES10.dashboardForm.close(); PES10.dashboardForm.destroy() end) end
  if PES10.openModals then
    for _, mf in pairs(PES10.openModals) do pcall(function() mf.close(); mf.destroy() end) end
  end
end

PES10 = PES10 or {}
PES10.VERSION = "12.0 Ultimate Studio"
PES10.PROCESS_NAME = "PES2021.exe"
PES10.SUPPORTED_EXES = { "PES2021.exe", "FL_2026.exe", "FL_2025.exe", "FL_2024.exe", "FL_2023.exe", "FL_2025_U.exe", "FL_2024_U.exe" }
PES10.openModals = {}
PES10.lang = "__DEFAULT_LANG__"
if PES10.lang ~= "en" and PES10.lang ~= "pt" then
  PES10.lang = "pt"
end

PES10.attachedPid = 0
PES10.autoArmedPid = 0
PES10.autoArmHooks = true
PES10.isInitializing = false
PES10.stateBase = 0
PES10.addrCareerOwner = 0
PES10.addrMatchMgr = 0
PES10.dashboardForm = nil
PES10.dashboardState = nil

function PES10.T(ptStr, enStr)
  if PES10.lang == "en" then return enStr end
  return ptStr
end

PES10.lastToastMsg = PES10.T(
  "✓ Pronto - Passe o mouse e clique em qualquer ação rápida.",
  "✓ Ready - Hover and click any quick action card."
)

-- Supported currencies (Defaults to 0 = Euro €!)
PES10.CURRENCIES = {
  [0] = { symbol = "€",  code = "EUR", sep = ".", dec = "," },
  [1] = { symbol = "R$", code = "BRL", sep = ".", dec = "," },
  [2] = { symbol = "$",  code = "USD", sep = ",", dec = "." },
  [3] = { symbol = "£",  code = "GBP", sep = ",", dec = "." },
}
PES10.currencyIndex = 0 -- Default: Euro (€)

-- Position short codes (PT-BR & EN)
PES10.POS_SHORT_PT = {
  [0]="GL", [1]="ZAG", [2]="LE", [3]="LD", [4]="VOL", [5]="MLG",
  [6]="MLE", [7]="MLD", [8]="MAT", [9]="PTE", [10]="PTD", [11]="SA", [12]="CA",
}
PES10.POS_SHORT_EN = {
  [0]="GK", [1]="CB", [2]="LB", [3]="RB", [4]="DMF", [5]="CMF",
  [6]="LMF", [7]="RMF", [8]="AMF", [9]="LWF", [10]="RWF", [11]="SS", [12]="CF",
}
function PES10.posShort(posIdx)
  local tbl = (PES10.lang == "en") and PES10.POS_SHORT_EN or PES10.POS_SHORT_PT
  return tbl[posIdx] or "OVR"
end

-- Full Position names in PT-BR & EN
PES10.POS_NAMES = {
  [0]  = { short = "GK",  pt = "Goleiro",            en = "Goalkeeper" },
  [1]  = { short = "CB",  pt = "Zagueiro",           en = "Center Back" },
  [2]  = { short = "LB",  pt = "Lateral Esquerdo",   en = "Left Back" },
  [3]  = { short = "RB",  pt = "Lateral Direito",    en = "Right Back" },
  [4]  = { short = "DMF", pt = "Volante",            en = "Defensive Midfielder" },
  [5]  = { short = "CMF", pt = "Meia Central",       en = "Central Midfielder" },
  [6]  = { short = "LMF", pt = "Meia Esquerda",      en = "Left Midfielder" },
  [7]  = { short = "RMF", pt = "Meia Direita",       en = "Right Midfielder" },
  [8]  = { short = "AMF", pt = "Meia Atacante",      en = "Attacking Midfielder" },
  [9]  = { short = "LWF", pt = "Ponta Esquerda",     en = "Left Wing Forward" },
  [10] = { short = "RWF", pt = "Ponta Direita",      en = "Right Wing Forward" },
  [11] = { short = "SS",  pt = "Segundo Atacante",   en = "Second Striker" },
  [12] = { short = "CF",  pt = "Centroavante",       en = "Center Forward" },
}

-- Team Roles (0 to 22) in PT-BR & EN
PES10.TEAM_ROLES = {
  [0]  = { pt = "Nenhum (-)",              en = "None (-)" },
  [1]  = { pt = "Jovem Promessa",          en = "Youth Prospect" },
  [2]  = { pt = "Pupilo",                  en = "Protégé" },
  [3]  = { pt = "Jogador de Equipe",       en = "Team Player" },
  [4]  = { pt = "Armador",                 en = "Playmaker" },
  [5]  = { pt = "Estrela",                 en = "Star Player" },
  [6]  = { pt = "Líder",                   en = "Leader" },
  [7]  = { pt = "General",                 en = "General" },
  [8]  = { pt = "Criador",                 en = "Creator" },
  [9]  = { pt = "Maestro",                 en = "Maestro" },
  [10] = { pt = "Incansável",              en = "Workhorse" },
  [11] = { pt = "Jogador Inteligente",     en = "Smart Player" },
  [12] = { pt = "Guerreiro",               en = "Fighter" },
  [13] = { pt = "Jogador Chave",           en = "Key Player" },
  [14] = { pt = "Superestrela",            en = "Superstar" },
  [15] = { pt = "Herói",                   en = "Hero" },
  [16] = { pt = "Virtuoso",                en = "Virtuoso" },
  [17] = { pt = "Regente",                 en = "Conductor" },
  [18] = { pt = "Bandeira",                en = "Bandiera" },
  [19] = { pt = "Lenda",                   en = "Legend" },
  [20] = { pt = "Estrela em Ascensão",     en = "Rising Star" },
  [21] = { pt = "Garoto Rebelde",          en = "Bad Boy" },
  [22] = { pt = "Ousado",                  en = "Risk-Taker" },
}

-- Master League Real RAM Scout Observation Lists (verified via PES2021.exe fcn.140efe53f + live RAM)
-- Empty slot sentinel in Konami engine (*0x143528f30): 0x000000000000FFFF
PES10.SCOUT_EMPTY_SENTINEL = 0x0000FFFF
PES10.SCOUT_LISTS = {
  [1] = { base = 0x01676054, cap = 30,  pt = "Olheiro 1", en = "Scout 1" },
  [2] = { base = 0x01676144, cap = 30,  pt = "Olheiro 2", en = "Scout 2" },
  [3] = { base = 0x01676234, cap = 30,  pt = "Olheiro 3", en = "Scout 3" },
  [4] = { base = 0x01675D34, cap = 100, pt = "Lista de Favoritos (Shortlist)", en = "Shortlist (Favorites)" },
}

-- Automatic 10-Level Player Undo Stack (stores full 0x17C byte snapshot per playerAddr)
PES10.undoStack = {}
PES10.UNDO_MAX_DEPTH = 10

function PES10.pushPlayerUndo(playerAddr, actionLabel)
  if not playerAddr or playerAddr <= 0x10000 or not PES10.isAttached() then return false end
  local snap = readBytes(playerAddr, 0x17C, true)
  if not snap or #snap < 0x17C then return false end
  local stack = PES10.undoStack[playerAddr]
  if not stack then stack = {}; PES10.undoStack[playerAddr] = stack end
  stack[#stack + 1] = {
    bytes = snap,
    label = tostring(actionLabel or PES10.T("Edição", "Edit")),
    time = os.date and os.date("%H:%M:%S") or "",
  }
  while #stack > PES10.UNDO_MAX_DEPTH do
    table.remove(stack, 1)
  end
  return true
end

function PES10.canUndoPlayer(playerAddr)
  local stack = playerAddr and PES10.undoStack[playerAddr]
  return (stack ~= nil and #stack > 0) and #stack or 0
end

function PES10.undoLastPlayerEdit(playerAddr)
  local p = playerAddr or PES10.getSelectedPlayerOrWarn()
  if not p or p == 0 then return false end
  local stack = PES10.undoStack[p]
  if not stack or #stack == 0 then
    PES10.toast(PES10.T("Nenhuma alteração recente para desfazer neste atleta.", "No recent edit to undo for this player."))
    return false
  end
  local entry = table.remove(stack)
  writeBytes(p, entry.bytes)
  PES10.syncAllDynamicPointers()
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local ovr, regPos = PES10.calcPlayerOVR(p)
  PES10.toast(string.format(
    PES10.T("Desfeito (%s) em %s - restaurado para %d %s! (%d no histórico)", "Undid (%s) on %s - restored to %d %s! (%d left)"),
    entry.label, pName, ovr, PES10.posShort(regPos), #stack
  ))
  if PES10.dashboardState and PES10.dashboardState.render then
    PES10.dashboardState.render(true)
  end
  return true
end

function PES10.setLang(langCode)
  if langCode ~= "pt" and langCode ~= "en" then return end
  PES10.lang = langCode
  PES10.toast(PES10.T("Idioma alterado para Português (PT-BR).", "Language switched to English (EN)."))
  if PES10.dashboardState and PES10.dashboardState.render then
    PES10.dashboardState.render(true)
  end
end

function PES10.toggleLang()
  PES10.setLang((PES10.lang == "en") and "pt" or "en")
end

-- Static fallback dictionaries + Native Konami OVR tables
PES10.CLUB_TEAMS = __CLUB_TEAMS_LUA__
PES10.CLUB_COMPS = __CLUB_COMPS_LUA__
PES10.OVR_DATA    = __KONAMI_OVR_LUA__

-- Shared state block layout ($1000 bytes allocated near PES2021.exe)
PES10.SYM_OFFSETS = {
  ptrCareer               = 0x000,
  ptrBudget               = 0x000,
  ptrPlayer               = 0x008,
  ptrMatchMgr             = 0x010,
  ptrMatchState           = 0x018,
  ptrMatchHud             = 0x020,
  ptrMatchCore            = 0x028,
  ptrPlayerTraining       = 0x030,
  ptrPlayerSquadBonus     = 0x038,
  ptrPlayerReleaseSlot    = 0x040,
  ptrPlayerGrowthDesc     = 0x048,
  ptrBALTrainingRec       = 0x050,
  cfgCurrency             = 0x070,
  cfgTrainingMultMode     = 0x074,
  cfgStaminaMode          = 0x078,
  cfgFreezeClock          = 0x07C,
  valProtectedMatchTicks  = 0x080,
  valFrozenTotalTicks     = 0x084,
  valComputedReleaseFee   = 0x088,
  valCustomReleaseFee     = 0x08C,
  cfgLockBALFocusPoints   = 0x090,
  valComputedOVR          = 0x094,
  rfeeOverrideTable       = 0x100, -- 64 slots * 16 bytes (0x100..0x4FF)
}

-- ============================================================================
-- SAFE MEMORY R/W & OFFLINE GUARD (ZERO PASCAL/SEH EXCEPTIONS)
-- ============================================================================
function PES10.isAttached()
  local pid = getOpenedProcessID()
  if not pid or pid == 0 then return false end
  local base = getAddressSafe(PES10.PROCESS_NAME)
  return base ~= nil and base ~= 0
end

function PES10.safeRegisterSymbol(name, addr)
  if name and type(addr) == "number" and addr > 0 then
    registerSymbol(name, addr, true)
  end
end

function PES10.symAddr(name)
  if not PES10.isAttached() or PES10.stateBase == 0 then return 0 end
  local off = PES10.SYM_OFFSETS[name]
  if not off then return 0 end
  return PES10.stateBase + off
end

function PES10.readU64(addr)
  if not addr or addr <= 0x10000 or not PES10.isAttached() then return 0 end
  return readQword(addr) or 0
end

function PES10.readU32(addr)
  if not addr or addr <= 0x10000 or not PES10.isAttached() then return 0 end
  local v = readInteger(addr)
  if not v then return 0 end
  if v < 0 then v = v + 0x100000000 end
  return v
end

function PES10.readI32(addr)
  if not addr or addr <= 0x10000 or not PES10.isAttached() then return 0 end
  local v = readInteger(addr)
  if not v then return 0 end
  if v >= 0x80000000 then v = v - 0x100000000 end
  return v
end

function PES10.readU16(addr)
  if not addr or addr <= 0x10000 or not PES10.isAttached() then return 0 end
  local v = readSmallInteger(addr)
  if not v then return 0 end
  if v < 0 then v = v + 0x10000 end
  return v
end

function PES10.readU8(addr)
  if not addr or addr <= 0x10000 or not PES10.isAttached() then return 0 end
  return readBytes(addr, 1, false) or 0
end

function PES10.setBitsInDword(addr, bitStart, bitLen, newVal)
  if not addr or addr <= 0x10000 or not PES10.isAttached() then return end
  local mask = ((1 << bitLen) - 1) << bitStart
  local cur = PES10.readU32(addr)
  local updated = ((cur & (~mask)) | ((newVal << bitStart) & mask)) & 0xFFFFFFFF
  if updated >= 0x80000000 then updated = updated - 0x100000000 end
  writeInteger(addr, updated)
end

function PES10.getBitsInDword(addr, bitStart, bitLen)
  local cur = PES10.readU32(addr)
  return (cur >> bitStart) & ((1 << bitLen) - 1)
end

function PES10.normalizeText(s)
  if not s then return "" end
  local map = {
    ["á"]="a", ["à"]="a", ["â"]="a", ["ã"]="a", ["ä"]="a",
    ["Á"]="a", ["À"]="a", ["Â"]="a", ["Ã"]="a", ["Ä"]="a",
    ["é"]="e", ["è"]="e", ["ê"]="e", ["ë"]="e",
    ["É"]="e", ["È"]="e", ["Ê"]="e", ["Ë"]="e",
    ["í"]="i", ["ì"]="i", ["î"]="i", ["ï"]="i",
    ["Í"]="i", ["Ì"]="i", ["Î"]="i", ["Ï"]="i",
    ["ó"]="o", ["ò"]="o", ["ô"]="o", ["õ"]="o", ["ö"]="o",
    ["Ó"]="o", ["Ò"]="o", ["Ô"]="o", ["Õ"]="o", ["Ö"]="o",
    ["ú"]="u", ["ù"]="u", ["û"]="u", ["ü"]="u",
    ["Ú"]="u", ["Ù"]="u", ["Û"]="u", ["Ü"]="u",
    ["ç"]="c", ["Ç"]="c", ["ñ"]="n", ["Ñ"]="n",
  }
  local out = tostring(s)
  for k, v in pairs(map) do out = out:gsub(k, v) end
  return out:lower()
end

function PES10.toast(msg)
  PES10.lastToastMsg = "✓ " .. tostring(msg)
end

PES10.HOOK_IDS_BY_NUM = {}

-- Exact v1.7.2.0 hook RVAs + original bytes for automatic recovery if a previous CE session closed without disarming
PES10.HOOK_ORIG_BYTES = {
  [1] = {
    { rva = 0xC6ED40, checkOff = -19, checkByte = 0xCB, orig = {0x0F,0x10,0x02,0x0F,0x11,0x01} },
    { rva = 0x8E24D7, checkOff = -7,  checkByte = 0x48, orig = {0x48,0x8B,0x40,0x2C,0x48,0x89,0x02} },
    { rva = 0xDDCA5C, checkOff = 7,   checkByte = 0x89, orig = {0x0F,0xB6,0x80,0x50,0x01,0x00,0x00} },
    { rva = 0xF27462, checkOff = 7,   checkByte = 0x89, orig = {0x0F,0xB6,0x80,0x50,0x01,0x00,0x00} },
  },
  [2] = { { rva = 0x156BE0C, checkOff = 5,  checkByte = 0x48, orig = {0x0F,0x29,0x74,0x24,0x40} } },
  [3] = { { rva = 0x12E14A0, checkOff = 6,  checkByte = 0x0F, orig = {0x45,0x8B,0xC8,0x83,0xFA,0x6C} } },
  [4] = { { rva = 0xBBFE79,  checkOff = -8, checkByte = 0x0F, orig = {0x15}, staleByte = 0x7F } },
  [5] = {
    { rva = 0x20F9FD6, checkOff = 7, checkByte = 0x88, orig = {0x49,0x8B,0xF8,0x44,0x0F,0xB6,0xE2} },
    { rva = 0xA3C787,  checkOff = 8, checkByte = 0xF3, orig = {0x8B,0x44,0x24,0x40,0x89,0x44,0x24,0x40} },
  },
  [6] = { { rva = 0xA58EAC, checkOff = -2, checkByte = 0x00, orig = {0x6B,0x45,0x10,0x64,0x48,0x8B,0x5C,0x24,0x50}, anyNeighbor = true } },
}

function PES10.restoreStaleHookBytes(hookNum)
  local list = PES10.HOOK_ORIG_BYTES[hookNum]
  if not list or not PES10.isAttached() then return end
  local base = getAddressSafe(PES10.PROCESS_NAME)
  if not base or base == 0 then return end
  for _, h in ipairs(list) do
    local addr = base + h.rva
    local b0 = PES10.readU8(addr)
    local isStale = h.staleByte and (b0 == h.staleByte) or (b0 == 0xE9)
    if isStale then
      local okNeighbor = h.anyNeighbor or (PES10.readU8(addr + h.checkOff) == h.checkByte)
      if okNeighbor then
        pcall(function() writeBytes(addr, h.orig) end)
      end
    end
  end
end

function PES10.ensureHookActive(hookNum)
  if not PES10.isAttached() or PES10.stateBase == 0 then return end
  local id = PES10.HOOK_IDS_BY_NUM and PES10.HOOK_IDS_BY_NUM[hookNum]
  if not id then return end
  local al = getAddressList()
  if not al then return end
  local mr = al.getMemoryRecordByID(id)
  if mr and not mr.Active then
    PES10.restoreStaleHookBytes(hookNum)
    pcall(function() mr.Active = true end)
  end
end

function PES10.activateAllHooks(silent)
  if not PES10.isAttached() then
    if not silent then
      PES10.toast(PES10.T("Abra o PES2021.exe primeiro!", "Open PES2021.exe first!"))
    end
    return 0
  end
  PES10.initProcessState()
  if PES10.stateBase == 0 then return 0 end
  local al = getAddressList()
  if not al then return 0 end
  local okCount = 0
  for num = 1, 6 do
    local id = PES10.HOOK_IDS_BY_NUM[num]
    local mr = id and al.getMemoryRecordByID(id) or nil
    if mr then
      if not mr.Active then
        PES10.restoreStaleHookBytes(num)
        pcall(function() mr.Active = true end)
      end
      if mr.Active then okCount = okCount + 1 end
    end
  end
  if not silent then
    PES10.toast(string.format(
      PES10.T("Motor interno conectado (%d / 6 ativos).", "Internal engine connected (%d / 6 active)."),
      okCount
    ))
  end
  return okCount
end

function PES10.scheduleAutoArmHooks()
  if not PES10.autoArmHooks or not PES10.isAttached() then return end
  local pid = getOpenedProcessID()
  if not pid or pid == 0 or PES10.autoArmedPid == pid then return end
  PES10.autoArmedPid = pid
  local t = createTimer(MainForm, true)
  t.Interval = 900
  t.OnTimer = function(tmr)
    tmr.destroy()
    if PES10.isAttached() and getOpenedProcessID() == pid then
      PES10.activateAllHooks(true)
    end
  end
end

-- ============================================================================
-- HUMAN MONEY FORMATTER (€ 125.500.000) & PARSER (50M, 500k, 1.5B)
-- ============================================================================
function PES10.getCurrentCurrency()
  local cAddr = PES10.symAddr("cfgCurrency")
  if cAddr ~= 0 then
    local idx = PES10.readU32(cAddr)
    if PES10.CURRENCIES[idx] then
      PES10.currencyIndex = idx
    end
  end
  return PES10.CURRENCIES[PES10.currencyIndex] or PES10.CURRENCIES[0]
end

function PES10.setCurrency(idx)
  if not PES10.CURRENCIES[idx] then return end
  PES10.currencyIndex = idx
  local cAddr = PES10.symAddr("cfgCurrency")
  if cAddr ~= 0 then
    writeInteger(cAddr, idx)
  end
  PES10.toast(string.format(
    PES10.T("Moeda alterada para %s (%s)", "Currency changed to %s (%s)"),
    PES10.CURRENCIES[idx].symbol, PES10.CURRENCIES[idx].code
  ))
end

function PES10.formatThousands(n, sep)
  sep = sep or "."
  local neg = n < 0
  local s = string.format("%.0f", math.floor(math.abs(n) + 0.5))
  local out = s:reverse():gsub("(%d%d%d)", "%1" .. sep):reverse()
  if out:sub(1, 1) == sep then out = out:sub(2) end
  return (neg and "-" or "") .. out
end

function PES10.formatMoneyFromRaw(rawVal)
  local c = PES10.getCurrentCurrency()
  if rawVal == nil then return c.symbol .. " 0" end
  local signed = tonumber(rawVal)
  if not signed then return tostring(rawVal) end
  if signed >= 0x80000000 then signed = signed - 0x100000000 end
  local realMoney = signed * 100
  return string.format("%s %s", c.symbol, PES10.formatThousands(realMoney, c.sep))
end

function PES10.parseHumanMoneyToRaw(inputStr)
  if not inputStr then return nil end
  local s = tostring(inputStr):upper()
  s = s:gsub("R%$", ""):gsub("[€%$£%s]", "")
  if s == "" then return nil end

  local mult = 1
  if s:sub(-1) == "B" then
    mult = 1000000000; s = s:sub(1, -2)
  elseif s:sub(-1) == "M" then
    mult = 1000000; s = s:sub(1, -2)
  elseif s:sub(-1) == "K" then
    mult = 1000; s = s:sub(1, -2)
  end

  if mult > 1 then
    s = s:gsub(",", ".")
  else
    s = s:gsub("[.,]", "")
  end

  local val = tonumber(s)
  if not val then return nil end
  local raw = math.floor((val * mult + 0.5) / 100)
  if raw > 2147483647 then raw = 2147483647 end
  if raw < -2147483648 then raw = -2147483648 end
  return raw
end

-- ============================================================================
-- LIVE CLUB / NATIONAL TEAM RAM DECODER (+0xADF4BC, STRIDE 0x690)
-- ============================================================================
function PES10.decodeClubHandle(rawU32)
  local v = tonumber(rawU32)
  if not v then return 0, 0, PES10.T("Desconhecido", "Unknown") end
  if v < 0 then v = v + 0x100000000 end
  if v == 0xFFFFFFFF or v == 0 then
    return 0, 0, PES10.T("Sem Clube / Livre", "Free Agent")
  end
  local slot = v % 16384
  local teamId = math.floor(v / 16384)

  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career ~= 0 and slot < 1200 then
    local clubEntry = career + 0xADF4BC + (slot * 0x690)
    local entryHandle = PES10.readU32(clubEntry)
    if entryHandle == v or (entryHandle % 16384) == slot then
      local liveName = readString(clubEntry + 0x04, 64, false)
      local abbr = readString(clubEntry + 0x4A, 6, false)
      if liveName and liveName ~= "" then
        if abbr and abbr ~= "" then
          return teamId, slot, string.format("%s [%s] (ID %d)", liveName, abbr, teamId)
        end
        return teamId, slot, string.format("%s (ID %d)", liveName, teamId)
      end
    end
  end

  local fallback = PES10.CLUB_TEAMS[teamId]
  if fallback then
    return teamId, slot, string.format("%s (ID %d)", fallback, teamId)
  end
  return teamId, slot, string.format(PES10.T("Time ID #%d (Slot %d)", "Team ID #%d (Slot %d)"), teamId, slot)
end

PES10._clubCacheKey = ""
PES10._clubCacheName = ""

function PES10.getCareerModeAndClubInfo()
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    PES10._clubCacheKey = ""
    return PES10.T("Menu / Exibição", "Menu / Exhibition"), "", 0
  end
  local balHandle = PES10.readU64(career + 0x01819540)
  local isBAL = (math.floor(balHandle / 0x100000000) ~= 0)
  if isBAL then
    return PES10.T("Rumo ao Estrelato (BAL)", "Become a Legend (BAL)"), "", career
  end
  local h0 = PES10.readU64(career + 0x016EE854)
  local cacheKey = tostring(career) .. ":" .. tostring(h0)
  if PES10._clubCacheKey == cacheKey then
    return "Master League", PES10._clubCacheName, career
  end
  local clubName = ""
  if math.floor(h0 / 0x100000000) ~= 0 then
    local lowSlot = h0 % 0x100000000
    local count = PES10.readU32(career + 0x00D0BCE8)
    if count > 30001 then count = 30000 end
    if lowSlot < count then
      local cand = career + (lowSlot * 0x17C)
      if PES10.readU64(cand + 0x2C) == h0 then
        local _, _, cname = PES10.decodeClubHandle(PES10.readU32(cand + 0x12C))
        clubName = cname
      end
    end
    if clubName == "" then
      for i = 0, math.min(count - 1, 4000) do
        local p = career + (i * 0x17C)
        if PES10.readU64(p + 0x2C) == h0 then
          local _, _, cname = PES10.decodeClubHandle(PES10.readU32(p + 0x12C))
          clubName = cname
          break
        end
      end
    end
  end
  PES10._clubCacheKey = cacheKey
  PES10._clubCacheName = clubName
  return "Master League", clubName, career
end

function PES10.getUserClubHandle(career)
  if not career or career == 0 then return 0 end
  -- Primary: Master League active user club handle at career + 0x01642A2C (verified live)
  local uc = PES10.readU32(career + 0x01642A2C)
  if uc ~= 0 and uc ~= 0xFFFFFFFF then return uc end
  -- Fallback 1: ML Squad slot 0 handle at career + 0x016EE854
  local h0 = PES10.readU64(career + 0x016EE854)
  if h0 ~= 0 and math.floor(h0 / 0x100000000) ~= 0 then
    local lo = h0 % 0x100000000
    if lo < 30000 then
      local c0 = PES10.readU32(career + lo * 0x17C + 0x12C)
      if c0 ~= 0 and c0 ~= 0xFFFFFFFF then return c0 end
    end
  end
  -- Fallback 2: BAL player handle at career + 0x01819540
  local balH = PES10.readU64(career + 0x01819540)
  if balH ~= 0 and math.floor(balH / 0x100000000) ~= 0 then
    local lo = balH % 0x100000000
    if lo < 30000 then
      local c1 = PES10.readU32(career + lo * 0x17C + 0x12C)
      if c1 ~= 0 and c1 ~= 0xFFFFFFFF then return c1 end
    end
  end
  return 0
end

-- Exact Konami Available Salary Budget (0x14156D170): Gross (0x016ECC08) - Active Squad Wages
PES10._wageCacheTime = 0
PES10._wageCacheCareer = 0
PES10._wageCacheSum = 0

function PES10.getAvailableSalaryBudget(career, forceRefresh)
  if not career or career == 0 then return 0, 0, 0 end
  local gross = PES10.readI32(career + 0x016ECC08)
  local now = (os and os.clock and os.clock()) or 0
  if not forceRefresh and PES10._wageCacheCareer == career and (now - PES10._wageCacheTime) < 2.5 then
    return gross - PES10._wageCacheSum, gross, PES10._wageCacheSum
  end
  local uc = PES10.getUserClubHandle(career)
  if uc == 0 then
    PES10._wageCacheCareer = career
    PES10._wageCacheTime = now
    PES10._wageCacheSum = 0
    return gross, gross, 0
  end
  -- Pending/override salary table (34 entries * 12 bytes at career + 0x016F0764)
  local overrides = {}
  local ovSum = 0
  for i = 0, 33 do
    local base = career + 0x016F0764 + i * 12
    local ph = PES10.readU64(base)
    if ph ~= 0 and (ph % 0x10000) ~= 0xFFFF and math.floor(ph / 0x100000000) ~= 0 then
      local ov = PES10.readI32(base + 8)
      overrides[ph] = ov
      ovSum = ovSum + ov
    end
  end
  -- Squad address cache: the 30K-player DB is scanned only when the club/save changes
  -- or every 45s (transfers); otherwise only the ~31 cached squad records are re-summed (< 1 ms).
  local sc = PES10._squadAddrCache
  local needRescan = (not sc) or sc.career ~= career or sc.uc ~= uc or (now - sc.time) > 45
  if not needRescan then
    for _, a in ipairs(sc.addrs) do
      if PES10.readU32(a + 0x12C) ~= uc then needRescan = true; break end
    end
  end
  if needRescan then
    local addrs = {}
    local count = PES10.readU32(career + 0x00D0BCE8)
    if count == 0 or count > 30001 then count = 30000 end
    for i = 0, count - 1 do
      local p = career + i * 0x17C
      if PES10.readU32(p + 0x12C) == uc then
        local pid = PES10.readU32(p + 0x30)
        if pid ~= 0 and pid ~= 0xFFFFFFFF then addrs[#addrs + 1] = p end
      end
    end
    sc = { career = career, uc = uc, time = now, addrs = addrs }
    PES10._squadAddrCache = sc
  end
  local sumSal = 0
  local seen = {}
  for _, p in ipairs(sc.addrs) do
    local ph = PES10.readU64(p + 0x2C)
    if not seen[ph] then
      seen[ph] = true
      if not (overrides[ph] and overrides[ph] ~= 0) then
        sumSal = sumSal + PES10.readI32(p + 0x15C)
      end
    end
  end
  sumSal = sumSal + ovSum
  PES10._wageCacheCareer = career
  PES10._wageCacheTime = now
  PES10._wageCacheSum = sumSal
  return gross - sumSal, gross, sumSal
end

-- ============================================================================
-- pes10State INITIALIZATION & RIP-RELATIVE SINGLETON DISCOVERY
-- ============================================================================
function PES10.scanUniqueModuleAOB(aobStr)
  local base = getAddressSafe(PES10.PROCESS_NAME)
  if not base or base == 0 then return 0 end
  local ms = createMemScan()
  ms.firstScan(
    soExactValue, vtByteArray, rtRounded,
    aobStr, "",
    base + 0x1000, base + 0x2540000,
    "", fsmNotAligned, "", true, false, false, false
  )
  ms.waitTillDone()
  local fl = createFoundList(ms)
  fl.initialize()
  local addr = 0
  if fl.Count >= 1 and fl.Address[0] then
    local s = tostring(fl.Address[0]):gsub("^0[xX]", "")
    addr = tonumber(s, 16) or getAddressSafe(fl.Address[0]) or 0
  end
  fl.destroy()
  ms.destroy()
  return addr
end

function PES10.initProcessState()
  if PES10.isInitializing then return false end
  if not PES10.isAttached() then return false end
  local pid = getOpenedProcessID()
  if PES10.attachedPid == pid and PES10.stateBase ~= 0 then
    return true
  end

  PES10.isInitializing = true
  local okRun, resRun = pcall(function()
    local exeBase = getAddressSafe(PES10.PROCESS_NAME)
    if not exeBase or exeBase == 0 then return false end
    if PES10.PROCESS_NAME ~= "PES2021.exe" then
      PES10.safeRegisterSymbol("PES2021.exe", exeBase)
    end

    local existing = getAddressSafe("pes10State")
    if existing and existing > 0 then
      PES10.stateBase = existing
    else
      local mem = allocateMemory(0x1000, exeBase)
      if not mem or mem == 0 then mem = allocateMemory(0x1000) end
      if not mem or mem == 0 then return false end
      local zeros = {}
      for i = 1, 0x1000 do zeros[i] = 0 end
      writeBytes(mem, zeros)
      PES10.safeRegisterSymbol("pes10State", mem)
      PES10.stateBase = mem
    end

    for sym, off in pairs(PES10.SYM_OFFSETS) do
      PES10.safeRegisterSymbol(sym, PES10.stateBase + off)
    end
    writeInteger(PES10.stateBase + PES10.SYM_OFFSETS.cfgCurrency, PES10.currencyIndex or 0)

    -- Fast-path for standard PES2021.exe v1.7.2.0 (avoids blocking ms.waitTillDone() on main UI thread!)
    local fastCareer = exeBase + 0x3714240
    local fastMatch  = exeBase + 0x3702208
    if PES10.readU64(fastCareer) > 0x10000 then
      PES10.addrCareerOwner = fastCareer
      PES10.safeRegisterSymbol("addrCareerOwner", PES10.addrCareerOwner)
      PES10.addrMatchMgr = fastMatch
      PES10.safeRegisterSymbol("addrMatchMgr", PES10.addrMatchMgr)
    else
      local callSite = PES10.scanUniqueModuleAOB("E8 ?? ?? ?? ?? 48 8B 48 48 48 8D 81 E0 CB 6E 01")
      if callSite ~= 0 then
        local relCall = PES10.readI32(callSite + 1)
        local fnGetter = callSite + 5 + relCall
        if PES10.readU8(fnGetter) == 0x48 and PES10.readU8(fnGetter + 1) == 0x8B and PES10.readU8(fnGetter + 2) == 0x05 then
          local disp = PES10.readI32(fnGetter + 3)
          PES10.addrCareerOwner = fnGetter + 7 + disp
          PES10.safeRegisterSymbol("addrCareerOwner", PES10.addrCareerOwner)
        end
      end
      if PES10.addrCareerOwner == 0 then
        PES10.addrCareerOwner = fastCareer
        PES10.safeRegisterSymbol("addrCareerOwner", PES10.addrCareerOwner)
      end

      local matchSite = PES10.scanUniqueModuleAOB("48 8B 05 ?? ?? ?? ?? 4C 8B E2 45 33 FF 48 8B B0 70 02 00 00")
      if matchSite ~= 0 then
        local disp = PES10.readI32(matchSite + 3)
        PES10.addrMatchMgr = matchSite + 7 + disp
        PES10.safeRegisterSymbol("addrMatchMgr", PES10.addrMatchMgr)
      else
        PES10.addrMatchMgr = fastMatch
        PES10.safeRegisterSymbol("addrMatchMgr", PES10.addrMatchMgr)
      end
    end

    PES10.attachedPid = pid
    PES10.syncAllDynamicPointers()
    PES10.scheduleAutoArmHooks()
    return true
  end)
  PES10.isInitializing = false
  return okRun and resRun or false
end

-- ============================================================================
-- REAL-TIME DYNAMIC POINTER SYNC & KONAMI RELEASE FEE FORMULA
-- ============================================================================
function PES10.findRecordByHandle64(tableBase, count, stride, targetHandle)
  if tableBase == 0 or targetHandle == 0 then return 0 end
  local targetHigh = math.floor(targetHandle / 0x100000000)
  if targetHigh == 0 then return 0 end
  for i = 0, count - 1 do
    local slotAddr = tableBase + (i * stride)
    local h = PES10.readU64(slotAddr)
    if h ~= 0 then
      if h == targetHandle or math.floor(h / 0x100000000) == targetHigh then
        return slotAddr
      end
    end
  end
  return 0
end

function PES10.estimateReleaseFeeRaw(career, playerAddr, pHandle)
  if playerAddr == 0 then return 0 end
  local hasRfee = ((PES10.readU32(playerAddr + 0x144) >> 30) & 1) == 1
  if PES10.stateBase ~= 0 and pHandle ~= 0 then
    local tbl = PES10.stateBase + PES10.SYM_OFFSETS.rfeeOverrideTable
    for i = 0, 63 do
      local entry = tbl + i * 16
      if PES10.readU32(entry + 0x0C) == 1 and PES10.readU64(entry) == pHandle then
        return PES10.readI32(entry + 0x08)
      end
    end
  end
  if not hasRfee then return 0 end
  local mkt = PES10.readI32(playerAddr + 0x174)
  if mkt <= 0 then return 0 end

  -- Decompiled from Konami fcn.14156BE00 + fcn.14156BF80
  local age = PES10.readU8(playerAddr + 0x1C) & 0x3F
  local affection = PES10.getBitsInDword(playerAddr + 0x13C, 18, 7)
  local f12 = (affection * 0.7) - 50.0
  local i4 = math.floor((50 - age) - 20 + f12)
  if i4 > 100 then i4 = 100 elseif i4 < -50 then i4 = -50 end
  local f8 = (i4 / 100.0) + 1.0
  return math.max(1, math.floor(mkt * f8 + 0.5))
end

PES10._lastBalKey = ""
PES10._lastPlayerKey = ""

function PES10.syncAllDynamicPointers()
  if not PES10.isAttached() or PES10.stateBase == 0 then return end
  local sb = PES10.stateBase

  local career = 0
  if PES10.addrCareerOwner ~= 0 then
    local owner = PES10.readU64(PES10.addrCareerOwner)
    if owner ~= 0 then career = PES10.readU64(owner + 0x48) end
  end
  writeQword(sb + PES10.SYM_OFFSETS.ptrCareer, career)

  local matchMgr, matchState = 0, 0
  if PES10.addrMatchMgr ~= 0 then
    matchMgr = PES10.readU64(PES10.addrMatchMgr)
    if matchMgr ~= 0 then matchState = PES10.readU64(matchMgr + 0x270) end
  end
  writeQword(sb + PES10.SYM_OFFSETS.ptrMatchMgr, matchMgr)
  writeQword(sb + PES10.SYM_OFFSETS.ptrMatchState, matchState)

  local mHud = PES10.readU64(sb + PES10.SYM_OFFSETS.ptrMatchHud)
  if mHud ~= 0 then
    writeQword(sb + PES10.SYM_OFFSETS.ptrMatchCore, PES10.readU64(mHud + 0x38))
  end

  if PES10.readU32(sb + PES10.SYM_OFFSETS.cfgTrainingMultMode) > 0 then PES10.ensureHookActive(3) end
  if PES10.readU32(sb + PES10.SYM_OFFSETS.cfgStaminaMode) > 0 then PES10.ensureHookActive(6) end
  if PES10.readU32(sb + PES10.SYM_OFFSETS.cfgFreezeClock) > 0 then PES10.ensureHookActive(5) end

  if career ~= 0 then
    if PES10.readU32(sb + PES10.SYM_OFFSETS.cfgLockBALFocusPoints) == 1 then
      PES10.ensureHookActive(4)
      writeBytes(career + 0x016ECBEA, 99)
    elseif PES10.readU8(career + 0x016ECBEA) > 21 then
      PES10.ensureHookActive(4)
    end
    local balHandle = PES10.readU64(career + 0x01819540)
    local balKey = tostring(career) .. ":" .. tostring(balHandle)
    if PES10._lastBalKey ~= balKey then
      PES10._lastBalKey = balKey
      local balRec = (math.floor(balHandle / 0x100000000) ~= 0) and PES10.findRecordByHandle64(career + 0x016BF710, 82, 0x170, balHandle) or 0
      writeQword(sb + PES10.SYM_OFFSETS.ptrBALTrainingRec, balRec)
    end
  else
    PES10._lastBalKey = ""
    writeQword(sb + PES10.SYM_OFFSETS.ptrBALTrainingRec, 0)
  end

  local pAddr = PES10.readU64(sb + PES10.SYM_OFFSETS.ptrPlayer)
  if pAddr ~= 0 and career ~= 0 then
    local pHandle = PES10.readU64(pAddr + 0x2C)
    local pKey = tostring(career) .. ":" .. tostring(pAddr) .. ":" .. tostring(pHandle)
    if PES10._lastPlayerKey ~= pKey then
      PES10._lastPlayerKey = pKey
      local pId = PES10.readU32(pAddr + 0x30)
      local lowHandle = PES10.readU32(pAddr + 0x2C)

      local trnSlot = PES10.findRecordByHandle64(career + 0x016BF710, 82, 0x170, pHandle)
      local bonusSlot = PES10.findRecordByHandle64(career + 0x016EE854, 40, 0x34, pHandle)
      local rfeeSlot = PES10.findRecordByHandle64(career + 0x01674D98, 40, 0x18, pHandle)
      local growthAddr = 0
      if (lowHandle % 0x10000) < 0x61A9 then
        growthAddr = career + 0x01676324 + ((lowHandle % 0x10000) * 4)
      elseif (pId % 0x10000) < 0x61A9 then
        growthAddr = career + 0x01676324 + ((pId % 0x10000) * 4)
      end

      writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerTraining, trnSlot)
      writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerSquadBonus, bonusSlot)
      writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerReleaseSlot, rfeeSlot)
      writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerGrowthDesc, growthAddr)
    end
    writeInteger(sb + PES10.SYM_OFFSETS.valComputedReleaseFee, PES10.estimateReleaseFeeRaw(career, pAddr, pHandle))
    -- OVR only recomputed when the player's 0x28 attribute header actually changed (skips 99% of ticks)
    local hdrBytes = readBytes(pAddr, 0x28, true)
    local hdrKey = hdrBytes and string.char(table.unpack(hdrBytes)) or ""
    if PES10._ovrCacheAddr ~= pAddr or PES10._ovrCacheKey ~= hdrKey then
      PES10._ovrCacheAddr = pAddr
      PES10._ovrCacheKey = hdrKey
      PES10._ovrCacheVal = PES10.calcPlayerOVR(pAddr)
    end
    writeInteger(sb + PES10.SYM_OFFSETS.valComputedOVR, PES10._ovrCacheVal or 0)
    if pAddr ~= PES10._lastNotifiedPlayer then
      PES10._lastNotifiedPlayer = pAddr
      PES10.notifyModalsPlayerChanged(pAddr)
    end
  else
    PES10._lastPlayerKey = ""
    PES10._lastNotifiedPlayer = 0
    writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerTraining, 0)
    writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerSquadBonus, 0)
    writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerReleaseSlot, 0)
    writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerGrowthDesc, 0)
    writeInteger(sb + PES10.SYM_OFFSETS.valComputedOVR, 0)
  end
end

function PES10.notifyModalsPlayerChanged(newP)
  if not newP or newP <= 0x10000 or not PES10.openModals then return end
  if PES10._inNotifyModals then return end
  PES10._inNotifyModals = true
  local newName = readString(newP + 0x38, 46, false) or PES10.T("Atleta", "Player")
  for _, m in pairs(PES10.openModals) do
    if m and m._onPlayerChange then
      local okVis, vis = pcall(function() return m.Visible end)
      if okVis and vis then
        pcall(m._onPlayerChange, newP, newName)
      end
    end
  end
  PES10._inNotifyModals = false
end

-- ============================================================================
-- KONAMI EXACT OVERALL ENGINE (fcn.1414e7a50 + fcn.141e640a0) & DNA SOLVER
-- ============================================================================
PES10.FID_LOC = {
  [14]={0x03,0},  [15]={0x04,0},  [16]={0x04,7},  [17]={0x04,14}, [18]={0x04,21},
  [19]={0x08,0},  [20]={0x08,7},  [21]={0x08,14}, [22]={0x08,21},
  [23]={0x0C,0},  [24]={0x0C,7},  [25]={0x0C,14}, [26]={0x0C,21},
  [27]={0x10,0},  [28]={0x10,7},  [29]={0x10,14}, [30]={0x10,21},
  [31]={0x14,0},  [33]={0x14,7},  [34]={0x14,14}, [35]={0x14,21},
  [36]={0x18,0},  [37]={0x18,7},  [38]={0x18,14}, [39]={0x18,21},
}
PES10.OUTFIELD_FIDS = {14,15,17,18,19,20,21,22,23,24,25,26,27,33,34,35,36,37,38,39}
PES10.GK_FIDS       = {16,28,29,30,31}
PES10.POS_REMAP     = {[0]=12,[1]=9,[2]=10,[3]=11,[4]=5,[5]=6,[6]=7,[7]=8,[8]=4,[9]=2,[10]=3,[11]=1,[12]=0}
PES10.POS_APT_LOC   = {
  [0]={0x22,2}, [1]={0x21,4}, [2]={0x21,6}, [3]={0x22,0}, [4]={0x20,4}, [5]={0x20,6},
  [6]={0x21,0}, [7]={0x21,2}, [8]={0x20,2}, [9]={0x1F,5}, [10]={0x20,0}, [11]={0x1F,3}, [12]={0x1F,1},
}

PES10.STAT_LABELS = {
  [14] = {"Talento Ofensivo",    "Offensive Awareness"},
  [15] = {"Talento Defensivo",   "Defensive Awareness"},
  [16] = {"Talento de Goleiro",  "GK Awareness"},
  [17] = {"Drible",              "Dribbling"},
  [18] = {"Bola Parada",         "Place Kicking"},
  [19] = {"Finalização",         "Finishing"},
  [20] = {"Passe Rasteiro",      "Low Pass"},
  [21] = {"Passe Alto",          "Lofted Pass"},
  [22] = {"Cabeceio",            "Heading"},
  [23] = {"Curva",               "Curl"},
  [24] = {"Resistência",         "Stamina"},
  [25] = {"Velocidade",          "Speed"},
  [26] = {"Força do Chute",      "Kicking Power"},
  [27] = {"Impulsão",            "Jump"},
  [28] = {"Alcance GK",          "GK Reach"},
  [29] = {"Afastar GK",          "GK Clearing"},
  [30] = {"Reflexos GK",         "GK Reflexes"},
  [31] = {"Firmeza GK",          "GK Catching"},
  [33] = {"Controle de Bola",    "Ball Control"},
  [34] = {"Contato Físico",      "Physical Contact"},
  [35] = {"Equilíbrio",          "Balance"},
  [36] = {"Aceleração",          "Acceleration"},
  [37] = {"Desarme",             "Ball Winning"},
  [38] = {"Condução Firme",      "Tight Possession"},
  [39] = {"Agressividade",       "Aggression"},
}

local function bufU32(b, off)
  return (b[off + 1] or 0)
       | ((b[off + 2] or 0) << 8)
       | ((b[off + 3] or 0) << 16)
       | ((b[off + 4] or 0) << 24)
end

local function bufSetU32(b, off, v)
  b[off + 1] = v & 0xFF
  b[off + 2] = (v >> 8) & 0xFF
  b[off + 3] = (v >> 16) & 0xFF
  b[off + 4] = (v >> 24) & 0xFF
end

local function bufGetStat(b, fid)
  local loc = PES10.FID_LOC[fid]
  if not loc then return 40 end
  if loc[1] == 0x03 then return (b[4] or 0) & 0x7F end
  return (bufU32(b, loc[1]) >> loc[2]) & 0x7F
end

local function bufSetStat(b, fid, val)
  local loc = PES10.FID_LOC[fid]
  if not loc then return end
  local v = math.max(40, math.min(99, math.floor(val + 0.5)))
  if loc[1] == 0x03 then
    b[4] = ((b[4] or 0) & 0x80) | (v & 0x7F)
  else
    local cur = bufU32(b, loc[1])
    local mask = 0x7F << loc[2]
    bufSetU32(b, loc[1], (cur & (~mask)) | ((v & 0x7F) << loc[2]))
  end
end

local function copyByteBuf(src)
  local out = {}
  for i = 1, #src do out[i] = src[i] end
  return out
end

local function bufGetPosAptitude(b, posIdx)
  local loc = PES10.POS_APT_LOC[posIdx]
  if not loc then return 0 end
  return ((b[loc[1] + 1] or 0) >> loc[2]) & 3
end

local function bufSetPosAptitude(b, posIdx, grade)
  local loc = PES10.POS_APT_LOC[posIdx]
  if not loc then return end
  local cur = b[loc[1] + 1] or 0
  local mask = 3 << loc[2]
  b[loc[1] + 1] = (cur & (~mask)) | (((grade or 2) & 3) << loc[2])
end

-- Exact decompiled Konami OVR (fcn.141e640a0):
-- When evalPos == nil or evalPos == realRegPos: Registered-Position OVR (base_reg + curve_100 blend).
-- When evalPos ~= realRegPos and assumeAptA == false: Exact Game Plan Pitch OVR using player's C/B/A aptitude!
-- When evalPos ~= realRegPos and assumeAptA == true: Potential Game Plan OVR if trained to Grade 'A'.
function PES10.calcPlayerOVRFromBuf(b, evalPos, assumeAptA)
  local T = PES10.OVR_DATA
  if not b or not T then return 40, 0 end
  local realRegPos = (bufU32(b, 0x04) >> 28) & 0x0F
  local targetPos  = (evalPos ~= nil) and evalPos or realRegPos
  local regCode    = PES10.POS_REMAP[realRegPos] or 0
  local targetCode = PES10.POS_REMAP[targetPos] or 0

  local arr = {}
  for i = 0, 43 do arr[i] = -1 end
  arr[0] = regCode

  for _, pair in ipairs(T.field_to_slot) do
    local fid, slot = pair[1], pair[2]
    if slot < 41 then
      if PES10.FID_LOC[fid] then
        arr[slot] = bufGetStat(b, fid)
      elseif fid == 32 then
        arr[slot] = (bufU32(b, 0x20) >> 20) & 3
      end
    end
  end

  local col = 1
  for ci, pc in ipairs(T.pos_order) do
    if pc == targetCode then col = ci; break end
  end

  local acc = 0
  for r, sidx in ipairs(T.stat_indices_23) do
    local v
    if sidx == 0x21 then
      local avgGK1 = (arr[33] + arr[37]) >> 1
      v = (avgGK1 < 25) and 1 or (avgGK1 - 25)
    elseif sidx == 0x22 then
      local avgGK2 = math.floor((arr[38] + arr[40] + arr[39]) / 3)
      v = (avgGK2 < 25) and 1 or (avgGK2 - 25)
    else
      v = arr[sidx]
      if sidx ~= 0x24 then
        v = (v < 25) and 1 or (v - 25)
      end
    end
    acc = acc + (T.weights_23x13[r][col] * v)
  end

  local u1 = math.floor((acc + 50) / 100)
  if targetCode == regCode then
    u1 = u1 + T.base_reg[col]
    if targetCode ~= 12 then
      local s6 = 0
      for _, sidx in ipairs(T.avg_indices_22) do
        if sidx == 0x21 then
          s6 = s6 + ((arr[37] + arr[33]) >> 1)
        elseif sidx == 0x22 then
          s6 = s6 + math.floor((arr[38] + arr[40] + arr[39]) / 3)
        else
          s6 = s6 + arr[sidx]
        end
      end
      local u3 = math.floor((s6 * 100) / 22) + 50
      local cval = (u3 < 10000) and (T.curve_100[math.floor(u3 / 100) + 1] or 70) or 70
      u1 = math.floor((cval * 40 + 50 + u1 * 60) / 100)
    end
  else
    local apt = assumeAptA and 2 or bufGetPosAptitude(b, targetPos)
    if apt >= 2 then
      u1 = u1 + T.base_apt_a[col]
    elseif apt == 1 then
      u1 = u1 + T.base_apt_b[col]
    end
  end

  return math.max(40, math.min(109, u1)), realRegPos
end

function PES10.calcPlayerOVR(playerAddr)
  if not playerAddr or playerAddr <= 0x10000 or not PES10.isAttached() then return 0, 0 end
  local b = readBytes(playerAddr, 0x28, true)
  if not b or #b < 0x28 then return 0, 0 end
  return PES10.calcPlayerOVRFromBuf(b)
end

function PES10.getPlayerBestPositions(playerAddr)
  if not playerAddr or playerAddr <= 0x10000 or not PES10.isAttached() then
    return 0, 0, {}, {}
  end
  local b = readBytes(playerAddr, 0x28, true)
  if not b or #b < 0x28 then return 0, 0, {}, {} end

  local curOVR, regPos = PES10.calcPlayerOVRFromBuf(b)
  local posRanking = {}
  local aptNames = { [0]="C", [1]="B", [2]="A", [3]="A" }
  for pIdx = 0, 12 do
    local liveOvr = PES10.calcPlayerOVRFromBuf(b, pIdx, false)
    local potAOvr = PES10.calcPlayerOVRFromBuf(b, pIdx, true)
    local apt = (pIdx == regPos) and 2 or bufGetPosAptitude(b, pIdx)
    posRanking[#posRanking + 1] = {
      pos = pIdx,
      ovr = liveOvr,
      potA = potAOvr,
      apt = aptNames[apt] or "C",
      isReg = (pIdx == regPos),
    }
  end
  table.sort(posRanking, function(a, bItem)
    if a.ovr ~= bItem.ovr then return a.ovr > bItem.ovr end
    if a.isReg ~= bItem.isReg then return a.isReg end
    if a.potA ~= bItem.potA then return a.potA > bItem.potA end
    return a.pos > bItem.pos
  end)

  local topStats = {}
  local langIdx = (PES10.lang == "en") and 2 or 1
  for fid, lbls in pairs(PES10.STAT_LABELS) do
    topStats[#topStats + 1] = { fid = fid, val = bufGetStat(b, fid), name = lbls[langIdx] }
  end
  table.sort(topStats, function(a, bItem)
    if a.val ~= bItem.val then return a.val > bItem.val end
    return a.fid < bItem.fid
  end)

  return curOVR, regPos, posRanking, topStats
end

function PES10.showPlayerDNAReport()
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return false end
  local pName = readString(p + 0x38, 46, false) or PES10.T("Atleta", "Player")
  local age = PES10.readU8(p + 0x1C) & 0x3F
  local curOVR, regPos, posRanking, topStats = PES10.getPlayerBestPositions(p)
  if #posRanking == 0 then return false end

  local lines = {}
  lines[#lines + 1] = string.format(
    PES10.T("★ RAIO-X DE POSIÇÕES & DNA - %s (%d anos)", "★ 13-POSITION & DNA X-RAY - %s (Age %d)"),
    pName, age
  )
  lines[#lines + 1] = string.format(
    PES10.T("Posição de Origem: %s  |  Overall Atual: %d", "Registered Position: %s  |  Current Overall: %d"),
    PES10.posShort(regPos), curOVR
  )
  if posRanking[1].ovr > curOVR then
    lines[#lines + 1] = string.format(
      PES10.T("💡 DICA TÁTICA: Este atleta rende %d OVR (+%d) como %s!", "💡 TACTICAL INSIGHT: This player rates %d OVR (+%d) as %s!"),
      posRanking[1].ovr, posRanking[1].ovr - curOVR, PES10.posShort(posRanking[1].pos)
    )
  end
  lines[#lines + 1] = ""
  lines[#lines + 1] = PES10.T("── TOP 5 ATRIBUTOS ASSINATURA (DNA) ──", "── TOP 5 SIGNATURE ATTRIBUTES (DNA) ──")
  for i = 1, math.min(5, #topStats) do
    lines[#lines + 1] = string.format("  %d. %s: %d", i, topStats[i].name, topStats[i].val)
  end
  lines[#lines + 1] = ""
  lines[#lines + 1] = PES10.T("── PLANO DE JOGO (13 POSIÇÕES c/ APTIDÃO REAL) ──", "── GAME PLAN RATING (ALL 13 POSITIONS w/ REAL APTITUDE) ──")
  local rowParts = {}
  for idx, item in ipairs(posRanking) do
    local mark = item.isReg and "*" or ("[" .. item.apt .. "]")
    local extra = (item.potA > item.ovr) and string.format(" (A:%d)", item.potA) or ""
    rowParts[#rowParts + 1] = string.format("%s%s: %d%s", PES10.posShort(item.pos), mark, item.ovr, extra)
    if #rowParts == 3 or idx == #posRanking then
      lines[#lines + 1] = "  " .. table.concat(rowParts, "   |   ")
      rowParts = {}
    end
  end

  PES10.toast(string.format(
    PES10.T("Raio-X aberto: %s (Melhor Pos: %s %d OVR)", "X-Ray opened: %s (Best Pos: %s %d OVR)"),
    pName, PES10.posShort(posRanking[1].pos), posRanking[1].ovr
  ))
  if PES10.openDarkTextModal then
    PES10.openDarkTextModal(
      "xray",
      PES10.T("Raio-X 13 Posições & DNA - ", "13-Position & DNA X-Ray - ") .. pName,
      string.format(PES10.T("Principal: %d %s  •  Melhor Posição: %d %s", "Primary: %d %s  •  Best Position: %d %s"), curOVR, PES10.posShort(regPos), posRanking[1].ovr, PES10.posShort(posRanking[1].pos)),
      lines,
      0xFA8BA7,
      PES10.T("Abrir Lab de Posições", "Open Position Lab"),
      function() PES10.openPositionLabModal() end
    )
  else
    showMessage(table.concat(lines, "\n"))
  end
  return true
end

-- Position Familiarity (C=0, B=1, A=2)
function PES10.getPosAptitude(playerAddr, posIdx)
  if not playerAddr or playerAddr <= 0x10000 or not PES10.isAttached() then return 0 end
  local loc = PES10.POS_APT_LOC[posIdx]
  if not loc then return 0 end
  local b = readBytes(playerAddr + loc[1], 1, false)
  return (b and ((b >> loc[2]) & 3)) or 0
end

function PES10.setPosAptitude(playerAddr, posIdx, grade, skipUndo)
  if not playerAddr or playerAddr <= 0x10000 or not PES10.isAttached() then return false end
  local loc = PES10.POS_APT_LOC[posIdx]
  if not loc then return false end
  if not skipUndo then PES10.pushPlayerUndo(playerAddr, PES10.T("Afinidade " .. PES10.posShort(posIdx), "Aptitude " .. PES10.posShort(posIdx))) end
  grade = math.max(0, math.min(2, grade or 2))
  PES10.setBitsInDword(playerAddr + loc[1], loc[2], 2, grade)
  PES10.syncAllDynamicPointers()
  return true
end

function PES10.setRegisteredPosition(playerAddr, newPosIdx)
  if not playerAddr or playerAddr <= 0x10000 or not PES10.isAttached() then return false end
  if newPosIdx == nil or newPosIdx < 0 or newPosIdx > 12 then return false end
  PES10.pushPlayerUndo(playerAddr, PES10.T("Posição Principal " .. PES10.posShort(newPosIdx), "Primary Pos " .. PES10.posShort(newPosIdx)))
  -- Registered Position sits at playerAddr + 0x07, bitstart 4, bitlen 4 (verified in player_schema.json)
  PES10.setBitsInDword(playerAddr + 0x04, 28, 4, newPosIdx)
  -- Ensure Grade 'A' on the newly registered position
  PES10.setPosAptitude(playerAddr, newPosIdx, 2, true)
  PES10.syncAllDynamicPointers()
  return true
end

function PES10.promoteSecondaryPositionsToA(playerAddr)
  if not playerAddr or playerAddr <= 0x10000 or not PES10.isAttached() then return 0 end
  PES10.pushPlayerUndo(playerAddr, PES10.T("Promover B→A", "Promote B→A"))
  local count = 0
  for pIdx = 0, 12 do
    local apt = PES10.getPosAptitude(playerAddr, pIdx)
    if apt == 1 then -- B (Secondary)
      PES10.setPosAptitude(playerAddr, pIdx, 2, true) -- upgrade to A (Full Mastery)
      count = count + 1
    end
  end
  PES10.syncAllDynamicPointers()
  return count
end

-- Team Role (0 to 22) and Level (1 to 3)
function PES10.getTeamRole(playerAddr)
  if not playerAddr or playerAddr <= 0x10000 or not PES10.isAttached() then return 0, 1 end
  local role = PES10.getBitsInDword(playerAddr + 0x150, 0, 5)
  local lvl  = PES10.getBitsInDword(playerAddr + 0x143, 6, 2)
  return math.max(0, math.min(22, role or 0)), math.max(1, math.min(3, lvl or 1))
end

function PES10.setTeamRole(playerAddr, role, lvl)
  if not playerAddr or playerAddr <= 0x10000 or not PES10.isAttached() then return false end
  PES10.pushPlayerUndo(playerAddr, "Team Role")
  role = math.max(0, math.min(22, role or 0))
  lvl  = math.max(1, math.min(3, lvl or 3))
  PES10.setBitsInDword(playerAddr + 0x150, 0, 5, role)
  PES10.setBitsInDword(playerAddr + 0x143, 6, 2, lvl)
  PES10.syncAllDynamicPointers()
  return true
end

-- Proportional DNA-Preserving Overall Solver (Hamilton Largest-Remainder + Core DNA Refinement)
-- Supports both primary registered position (evalPos == nil) and target secondary position!
function PES10.scalePlayerToTargetOVR(playerAddr, targetOVR, evalPos, skipUndo)
  if not playerAddr or playerAddr <= 0x10000 or not PES10.isAttached() then return false, 0, 0 end
  local origBuf = readBytes(playerAddr, 0x28, true)
  if not origBuf or #origBuf < 0x28 then return false, 0, 0 end

  targetOVR = math.max(40, math.min(99, math.floor(targetOVR + 0.5)))
  local isSpecificPos = (evalPos ~= nil)
  local curOVR, regPos = PES10.calcPlayerOVRFromBuf(origBuf, evalPos, isSpecificPos)
  local targetPos = isSpecificPos and evalPos or regPos
  if curOVR == targetOVR then return true, curOVR, curOVR end
  if not skipUndo then
    local lbl = isSpecificPos and string.format("OVR %s→%d", PES10.posShort(targetPos), targetOVR) or string.format("OVR %d→%d", curOVR, targetOVR)
    PES10.pushPlayerUndo(playerAddr, lbl)
  end

  local activeFids = {}
  if targetPos == 0 then
    for _, f in ipairs(PES10.GK_FIDS) do activeFids[#activeFids + 1] = f end
    for _, f in ipairs(PES10.OUTFIELD_FIDS) do
      if bufGetStat(origBuf, f) > 43 then activeFids[#activeFids + 1] = f end
    end
  else
    for _, f in ipairs(PES10.OUTFIELD_FIDS) do activeFids[#activeFids + 1] = f end
  end

  local posWeightsByFid = nil
  if isSpecificPos and targetPos ~= regPos and PES10.OVR_DATA then
    local T = PES10.OVR_DATA
    local posCode = PES10.POS_REMAP[targetPos] or 0
    local col = 1
    for ci, pc in ipairs(T.pos_order) do if pc == posCode then col = ci; break end end
    local slotByFid = {}
    for _, pr in ipairs(T.field_to_slot) do slotByFid[pr[1]] = pr[2] end
    posWeightsByFid = {}
    for _, fid in ipairs(activeFids) do
      local sl = slotByFid[fid] or 0
      local pw = 0
      for ri, sx in ipairs(T.stat_indices_23) do
        if sx == sl then pw = T.weights_23x13[ri][col]; break end
      end
      posWeightsByFid[fid] = pw
    end
  end

  local baseStats, weights = {}, {}
  local totalW = 0
  for _, fid in ipairs(activeFids) do
    local sv = bufGetStat(origBuf, fid)
    baseStats[fid] = sv
    local dnaW = math.max(4, sv - 30)
    local w = dnaW
    if posWeightsByFid then
      local pw = posWeightsByFid[fid] or 0
      -- Blend Konami target-position weight with player's own DNA so secondary OVR gains stay surgical & realistic
      w = (pw > 0) and (dnaW * (pw + 4)) or math.max(1, math.floor(dnaW * 0.15))
    end
    weights[fid] = w
    totalW = totalW + w
  end

  local dir = (targetOVR > curOVR) and 1 or -1
  local history = { copyByteBuf(origBuf) }
  local bestBuf, bestOVR = copyByteBuf(origBuf), curOVR

  for k = 1, #activeFids * 60 do
    local cand = copyByteBuf(origBuf)
    if isSpecificPos then
      bufSetPosAptitude(cand, targetPos, 2)
    end

    local floors, rems = {}, {}
    local sumFl = 0
    for _, fid in ipairs(activeFids) do
      local q = (k * weights[fid]) / totalW
      local fl = math.floor(q)
      floors[fid] = fl
      sumFl = sumFl + fl
      rems[#rems + 1] = { rem = q - fl, w = weights[fid], fid = fid }
    end
    table.sort(rems, function(a, bItem)
      if a.rem ~= bItem.rem then return a.rem > bItem.rem end
      return a.w > bItem.w
    end)
    local remNeed = k - sumFl
    for idx = 1, math.min(remNeed, #rems) do
      local f = rems[idx].fid
      floors[f] = floors[f] + 1
    end
    for _, fid in ipairs(activeFids) do
      bufSetStat(cand, fid, baseStats[fid] + dir * floors[fid])
    end

    local newOVR = PES10.calcPlayerOVRFromBuf(cand, evalPos, isSpecificPos)
    bestBuf, bestOVR = cand, newOVR
    if newOVR == targetOVR then
      writeBytes(playerAddr, cand)
      if isSpecificPos then PES10.setPosAptitude(playerAddr, targetPos, 2, true) end
      PES10.syncAllDynamicPointers()
      return true, curOVR, newOVR
    end

    if (dir == 1 and newOVR > targetOVR) or (dir == -1 and newOVR < targetOVR) then
      for idx = #rems, 1, -1 do
        local tf = rems[idx].fid
        if bufGetStat(cand, tf) ~= baseStats[tf] then
          local alt = copyByteBuf(cand)
          bufSetStat(alt, tf, bufGetStat(alt, tf) - dir)
          if PES10.calcPlayerOVRFromBuf(alt, evalPos, isSpecificPos) == targetOVR then
            writeBytes(playerAddr, alt)
            if isSpecificPos then PES10.setPosAptitude(playerAddr, targetPos, 2, true) end
            PES10.syncAllDynamicPointers()
            return true, curOVR, targetOVR
          end
        end
      end

      local T = PES10.OVR_DATA
      local posCode = PES10.POS_REMAP[targetPos] or 0
      local col = 1
      for ci, pc in ipairs(T.pos_order) do if pc == posCode then col = ci; break end end
      local slotByFid = {}
      for _, p in ipairs(T.field_to_slot) do slotByFid[p[1]] = p[2] end
      local dnaList = {}
      for _, fid in ipairs(activeFids) do
        local sl = slotByFid[fid] or 0
        local pw = 8
        for ri, sx in ipairs(T.stat_indices_23) do
          if sx == sl then pw = T.weights_23x13[ri][col]; break end
        end
        dnaList[#dnaList + 1] = { score = pw * weights[fid], fid = fid }
      end
      table.sort(dnaList, function(a, bItem) return a.score > bItem.score end)

      for hi = #history, math.max(1, #history - 9), -1 do
        local trial = copyByteBuf(history[hi])
        if isSpecificPos then
          bufSetPosAptitude(trial, targetPos, 2)
        end
        for _, item in ipairs(dnaList) do
          local cv = bufGetStat(trial, item.fid)
          local nv = math.max(40, math.min(99, cv + dir))
          if nv ~= cv then
            bufSetStat(trial, item.fid, nv)
            local to = PES10.calcPlayerOVRFromBuf(trial, evalPos, isSpecificPos)
            if to == targetOVR then
              writeBytes(playerAddr, trial)
              if isSpecificPos then PES10.setPosAptitude(playerAddr, targetPos, 2, true) end
              PES10.syncAllDynamicPointers()
              return true, curOVR, targetOVR
            elseif (dir == 1 and to > targetOVR) or (dir == -1 and to < targetOVR) then
              bufSetStat(trial, item.fid, cv)
            end
          end
        end
      end

      writeBytes(playerAddr, cand)
      if isSpecificPos then PES10.setPosAptitude(playerAddr, targetPos, 2, true) end
      PES10.syncAllDynamicPointers()
      return true, curOVR, newOVR
    end

    history[#history + 1] = cand
  end

  writeBytes(playerAddr, bestBuf)
  if isSpecificPos then PES10.setPosAptitude(playerAddr, targetPos, 2, true) end
  PES10.syncAllDynamicPointers()
  return true, curOVR, bestOVR
end

function PES10.stepPlayerOVRProportional(delta)
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return false end
  local curOVR, regPos = PES10.calcPlayerOVR(p)
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local target = math.max(40, math.min(99, curOVR + delta))
  if target == curOVR then
    PES10.toast(string.format(
      PES10.T("%s já está no limite (%d OVR).", "%s is already at the limit (%d OVR)."),
      pName, curOVR
    ))
    return false
  end
  local ok, oldO, newO = PES10.scalePlayerToTargetOVR(p, target)
  if ok then
    PES10.toast(string.format(
      PES10.T("%s: Overall %d → %d (%s - DNA e estilo preservados)!", "%s: Overall %d → %d (%s - DNA & style preserved)!"),
      pName, oldO, newO, PES10.posShort(regPos)
    ))
  end
  return ok
end

-- ============================================================================
-- ISOLATED SECONDARY POSITION OVR OPTIMIZER (ZERO/MIN PRIMARY OVR DRIFT!)
-- Uses Konami's exact 23x13 weight matrix to isolate attributes that boost
-- targetPos while keeping the primary registered position OVR untouched!
-- ============================================================================
function PES10.getPositionIsolationProfile(regPos, targetPos)
  local T = PES10.OVR_DATA
  if not T then return {} end
  local langIdx = (PES10.lang == "en") and 2 or 1
  local regCode = PES10.POS_REMAP[regPos] or 0
  local tgtCode = PES10.POS_REMAP[targetPos] or 0
  local colP, colT = 1, 1
  for ci, pc in ipairs(T.pos_order) do
    if pc == regCode then colP = ci end
    if pc == tgtCode then colT = ci end
  end
  local slotByFid = {}
  for _, pr in ipairs(T.field_to_slot) do slotByFid[pr[1]] = pr[2] end
  local fids = (targetPos == 0) and PES10.GK_FIDS or PES10.OUTFIELD_FIDS

  local list = {}
  for _, fid in ipairs(fids) do
    local sl = slotByFid[fid] or -1
    local wP, wT, wOthersSum = 0, 0, 0
    for ri, sx in ipairs(T.stat_indices_23) do
      if sx == sl then
        local row = T.weights_23x13[ri]
        wP = row[colP] or 0
        wT = row[colT] or 0
        for ci = 1, #row do
          if ci ~= colT and ci ~= colP then
            wOthersSum = wOthersSum + (row[ci] or 0)
          end
        end
        break
      end
    end
    if wT > 0 then
      local avgOther = wOthersSum / 11.0
      local bleedDiv = 1.0 + (avgOther * 0.04)
      local isoScore
      local exclusive = (wP == 0)
      if exclusive then
        isoScore = (wT * 5.0) / bleedDiv
      elseif wT > wP then
        isoScore = ((wT - wP) * 3.0 + (wT * 0.6)) / bleedDiv
      else
        isoScore = (wT / (wP + 2.0)) / bleedDiv
      end
      local lbl = PES10.STAT_LABELS[fid]
      list[#list + 1] = {
        fid = fid,
        name = lbl and lbl[langIdx] or ("Stat " .. tostring(fid)),
        wT = wT,
        wP = wP,
        isoScore = isoScore,
        exclusive = exclusive,
      }
    end
  end
  table.sort(list, function(a, b)
    if a.isoScore ~= b.isoScore then return a.isoScore > b.isoScore end
    return a.wT > b.wT
  end)
  return list
end

-- Adjusts targetPos UP or DOWN to targetOVR (40-99) using ONLY stats that do not move the primary position.
-- STRICT: the primary registered-position OVR is NEVER changed, and the position GRADE (C/B/A) is NEVER changed
-- silently (the displayed OVR already includes the grade bonus; use the explicit Grade buttons to change it).
-- If the target cannot be reached without touching the primary, it stops at the closest reachable value.
-- Returns: ok, oldTgtOVR, newTgtOVR, oldPriOVR, newPriOVR, diffSummary
function PES10.boostSecondaryPositionIsolated(playerAddr, targetPos, targetOVR, skipUndo)
  if not playerAddr or playerAddr <= 0x10000 or not PES10.isAttached() then
    return false, 0, 0, 0, 0, ""
  end
  local origBuf = readBytes(playerAddr, 0x28, true)
  if not origBuf or #origBuf < 0x28 then return false, 0, 0, 0, 0, "" end

  local origPriOVR, regPos = PES10.calcPlayerOVRFromBuf(origBuf)
  targetOVR = math.max(40, math.min(99, math.floor(targetOVR + 0.5)))
  if targetPos == regPos then
    local ok, o1, o2 = PES10.scalePlayerToTargetOVR(playerAddr, targetOVR, nil, skipUndo)
    return ok, o1, o2, o1, o2, ""
  end

  -- OVR exactly as the game shows it: stats + the position's CURRENT grade bonus
  local curTgtOVR = PES10.calcPlayerOVRFromBuf(origBuf, targetPos, false)
  if curTgtOVR == targetOVR then
    return true, curTgtOVR, curTgtOVR, origPriOVR, origPriOVR, ""
  end

  local dir = (targetOVR > curTgtOVR) and 1 or -1
  local profile = PES10.getPositionIsolationProfile(regPos, targetPos)
  if #profile == 0 then
    return true, curTgtOVR, curTgtOVR, origPriOVR, origPriOVR, ""
  end

  local cand = copyByteBuf(origBuf)
  local changed = {}
  for _, item in ipairs(profile) do changed[item.fid] = 0 end

  local safety = 0
  while safety < 500 do
    local cur = PES10.calcPlayerOVRFromBuf(cand, targetPos, false)
    if cur == targetOVR or (dir == 1 and cur > targetOVR) or (dir == -1 and cur < targetOVR) then break end
    safety = safety + 1
    local bestFid, bestScore = nil, -1
    for _, item in ipairs(profile) do
      local cv = bufGetStat(cand, item.fid)
      local canStep = (dir == 1 and cv < 99) or (dir == -1 and cv > 40)
      if canStep then
        local sc = item.isoScore / (1.0 + 0.35 * (changed[item.fid] or 0))
        if sc > bestScore then
          bufSetStat(cand, item.fid, cv + dir)
          local testPri = PES10.calcPlayerOVRFromBuf(cand, regPos)
          local testTgt = PES10.calcPlayerOVRFromBuf(cand, targetPos, false)
          bufSetStat(cand, item.fid, cv)
          local overshoot = (dir == 1 and testTgt > targetOVR) or (dir == -1 and testTgt < targetOVR)
          if testPri == origPriOVR and not overshoot then
            bestScore = sc
            bestFid = item.fid
          end
        end
      end
    end
    if not bestFid then break end
    bufSetStat(cand, bestFid, bufGetStat(cand, bestFid) + dir)
    changed[bestFid] = (changed[bestFid] or 0) + 1
  end

  local newTgtOVR = PES10.calcPlayerOVRFromBuf(cand, targetPos, false)
  local newPriOVR = PES10.calcPlayerOVRFromBuf(cand, regPos)
  if newTgtOVR == curTgtOVR then
    -- Nothing reachable without touching the primary position: leave the player untouched
    return true, curTgtOVR, curTgtOVR, origPriOVR, origPriOVR, ""
  end

  if not skipUndo then
    PES10.pushPlayerUndo(playerAddr, string.format("Iso %s->%d", PES10.posShort(targetPos), newTgtOVR))
  end
  writeBytes(playerAddr, cand)
  PES10.syncAllDynamicPointers()

  local diffList = {}
  for _, item in ipairs(profile) do
    local d = changed[item.fid] or 0
    if d > 0 then diffList[#diffList + 1] = { name = item.name, d = d * dir } end
  end
  table.sort(diffList, function(a, b) return math.abs(a.d) > math.abs(b.d) end)
  local parts = {}
  for i = 1, math.min(4, #diffList) do
    local sign = (diffList[i].d > 0) and "+" or ""
    parts[#parts + 1] = string.format("%s %s%d", diffList[i].name, sign, diffList[i].d)
  end
  return true, curTgtOVR, newTgtOVR, origPriOVR, newPriOVR, table.concat(parts, ", ")
end

-- Human note when the exact requested OVR could not be reached without touching the primary position
local function isoLimitNote(requested, reached, curGradeIsA)
  if reached == requested then return "" end
  if curGradeIsA then
    return PES10.T(" | limite: " .. reached .. " sem mexer na principal", " | limit: " .. reached .. " without touching the primary")
  end
  return PES10.T(" | limite: " .. reached .. " sem mexer na principal (suba o Grau p/ ganhar bÃ´nus)", " | limit: " .. reached .. " without touching the primary (raise the Grade for a bonus)")
end

function PES10.matchSecondaryToPrimaryOVR(playerAddr, targetPos)
  local p = playerAddr or PES10.getSelectedPlayerOrWarn()
  if not p or p == 0 then return false end
  local origPri, regPos = PES10.calcPlayerOVR(p)
  if targetPos == regPos then
    PES10.toast(PES10.T("Escolha uma posiÃ§Ã£o secundÃ¡ria diferente da principal!", "Pick a secondary position different from primary!"))
    return false
  end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local ok, oT, nT, oP, nP, diffStr = PES10.boostSecondaryPositionIsolated(p, targetPos, origPri, false)
  if ok then
    local note = isoLimitNote(origPri, nT, PES10.getPosAptitude(p, targetPos) >= 2)
    local priNote = string.format(PES10.T("%s mantido em %d", "%s kept at %d"), PES10.posShort(regPos), nP)
    PES10.toast(string.format(
      "%s: %s %dâ†’%d | %s%s%s",
      pName, PES10.posShort(targetPos), oT, nT, priNote, (diffStr ~= "" and (" [" .. diffStr .. "]") or ""), note
    ))
  end
  return ok, oT, nT, oP, nP, diffStr
end

function PES10.applyPlayerEternalPeak(playerAddr)
  local p = playerAddr or PES10.getSelectedPlayerOrWarn()
  if not p or p == 0 then return false end
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  PES10.pushPlayerUndo(p, PES10.T("Auge + Anti-Declínio", "Peak + Anti-Decline"))
  -- Set Growth / Post-30 Decline Curve to Lasting Peak (3)
  PES10.setBitsInDword(p + 0x18, 28, 3, 3)
  if career ~= 0 then
    local lowH = PES10.readU32(p + 0x2C) % 0x10000
    if lowH < 0x61A9 then
      local gAddr = career + 0x01676324 + (lowH * 4)
      local g = PES10.readU32(gAddr)
      g = (g & 0xF00001FF) | (38 << 17) | (2 << 24)
      if g >= 0x80000000 then g = g - 0x100000000 end
      writeInteger(gAddr, g)
    end
  end
  PES10.syncAllDynamicPointers()
  PES10.toast(string.format(
    PES10.T("%s: Auge (Breakthrough) + Pico aos 38 anos + Anti-Declínio 30+ ativados!", "%s: Breakthrough Surge + Peak Age 38 + Anti-Decline 30+ activated!"),
    pName
  ))
  return true
end

-- Set the OVR of ANY position to an exact value (primary OVR is never touched when editing a secondary position)
function PES10.setTargetPositionExactOVR(targetPos, desiredOVR)
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return false end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local origBuf = readBytes(p, 0x28, true)
  if not origBuf or #origBuf < 0x28 then return false end

  local origPriOVR, regPos = PES10.calcPlayerOVRFromBuf(origBuf)
  local posCodeLbl = PES10.posShort(targetPos)
  local tgt = math.max(40, math.min(99, math.floor((desiredOVR or origPriOVR) + 0.5)))

  if targetPos ~= regPos then
    local ok, oT, nT, oP, nP, diffStr = PES10.boostSecondaryPositionIsolated(p, targetPos, tgt, false)
    if ok then
      local note = isoLimitNote(tgt, nT, PES10.getPosAptitude(p, targetPos) >= 2)
      local priNote = string.format(PES10.T("%s mantido em %d", "%s kept at %d"), PES10.posShort(regPos), nP)
      PES10.toast(string.format(
        "%s: %s %d->%d | %s%s%s",
        pName, posCodeLbl, oT, nT, priNote, (diffStr ~= "" and (" [" .. diffStr .. "]") or ""), note
      ))
    end
    return ok, oT, nT, oP, nP, diffStr
  end

  local ok, oldO, newO = PES10.scalePlayerToTargetOVR(p, tgt)
  if ok then
    PES10.toast(string.format(
      PES10.T("%s: %s %d -> %d OVR (DNA preservado)!", "%s: %s %d -> %d OVR (DNA preserved)!"),
      pName, posCodeLbl, oldO, newO
    ))
  end
  return ok, oldO, newO, oldO, newO, ""
end

function PES10.stepTargetPositionOVR(targetPos, delta)
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return false end
  local origBuf = readBytes(p, 0x28, true)
  if not origBuf or #origBuf < 0x28 then return false end
  local d = delta or 1
  -- Step is always relative to the OVR the game currently shows for that position (grade bonus included)
  local curPosOVR = PES10.calcPlayerOVRFromBuf(origBuf, targetPos, false)
  local newTargetOVR = math.max(40, math.min(99, curPosOVR + d))
  if newTargetOVR == curPosOVR then
    PES10.toast(string.format(
      PES10.T("JÃ¡ estÃ¡ no limite (%d OVR em %s).", "Already at limit (%d OVR in %s)."),
      curPosOVR, PES10.posShort(targetPos)
    ))
    return false
  end
  return PES10.setTargetPositionExactOVR(targetPos, newTargetOVR)
end

-- Interactive UI for picking any position and boosting +1 OVR
function PES10.promptTargetPositionOVRPlusOne()
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local origBuf = readBytes(p, 0x28, true)
  if not origBuf or #origBuf < 0x28 then return end

  local regOVR, regPos = PES10.calcPlayerOVRFromBuf(origBuf)
  local lines = {}
  table.insert(lines, PES10.T("== SUBIR +1 OVERALL EM POSIÇÃO ESPECÍFICA ==", "== BOOST +1 OVERALL IN SPECIFIC POSITION =="))
  table.insert(lines, string.format(PES10.T("Atleta: %s  [Principal: %d %s]", "Player: %s  [Primary: %d %s]"), pName, regOVR, PES10.posShort(regPos)))
  table.insert(lines, "----------------------------------------------------")

  local posOrderDisplay = {12, 11, 9, 10, 8, 6, 7, 5, 4, 2, 3, 1, 0}
  for idx, pIdx in ipairs(posOrderDisplay) do
    local pInfo = PES10.POS_NAMES[pIdx]
    local pShort = PES10.posShort(pIdx)
    local apt = (pIdx == regPos) and 2 or bufGetPosAptitude(origBuf, pIdx)
    local aptLetter = (apt == 2 and "A") or (apt == 1 and "B") or "C"
    local curPosOVR = PES10.calcPlayerOVRFromBuf(origBuf, pIdx, false)
    local potPosOVR = PES10.calcPlayerOVRFromBuf(origBuf, pIdx, true)
    local tag = (pIdx == regPos) and PES10.T(" *", " *") or ""
    local boostNote = (apt < 2) and string.format(" (A: %d → %d)", potPosOVR, math.min(99, potPosOVR + 1)) or string.format(" (+1 → %d)", math.min(99, potPosOVR + 1))
    table.insert(lines, string.format("[%2d] %-3s (%-16s): %d [%s]%s%s", idx, pShort, PES10.T(pInfo.pt, pInfo.en), curPosOVR, aptLetter, tag, boostNote))
  end
  table.insert(lines, "----------------------------------------------------")
  table.insert(lines, PES10.T("Digite o número (1-13) ou a sigla da posição (ex: MAT, CA, VOL):", "Enter number (1-13) or position code (e.g. AMF, CF, DMF):"))

  local ans = inputQuery(
    PES10.T("Subir +1 OVR em Posição Específica - ", "Boost +1 OVR in Specific Position - ") .. pName,
    table.concat(lines, "\n"),
    "1"
  )
  if not ans or ans:match("^%s*$") then return end
  local targetPos = nil
  local pick = tonumber(ans)
  if pick and pick >= 1 and pick <= 13 then
    targetPos = posOrderDisplay[pick]
  else
    local code = ans:upper():gsub("%s+", "")
    for pIdx = 0, 12 do
      if PES10.POS_SHORT_PT[pIdx] == code or PES10.POS_SHORT_EN[pIdx] == code then
        targetPos = pIdx; break
      end
    end
  end
  if targetPos ~= nil then
    PES10.stepTargetPositionOVR(targetPos, 1)
  else
    PES10.toast(PES10.T("Posição inválida: ", "Invalid position: ") .. tostring(ans))
  end
end

-- Interactive UI for Position Familiarity Editing (C / B / A)
function PES10.promptEditPositionAptitude()
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local curOVR, regPos = PES10.calcPlayerOVR(p)

  local lines = {}
  table.insert(lines, PES10.T("== AFINIDADE DE POSIÇÃO: ", "== POSITION MASTERY: ") .. pName .. " (" .. tostring(curOVR) .. " OVR) ==")
  table.insert(lines, PES10.T("0:  Promover Todas as Secundárias ('B' → 'A')", "0:  Upgrade All Secondary Positions ('B' → 'A')"))
  table.insert(lines, PES10.T("99: Maximizar TODAS as 13 Posições para 'A'", "99: Maximize ALL 13 Positions to 'A'"))
  table.insert(lines, "----------------------------------------------------")
  local posOrderDisplay = {12, 11, 9, 10, 8, 6, 7, 5, 4, 2, 3, 1, 0}
  for idx, posIdx in ipairs(posOrderDisplay) do
    local info = PES10.POS_NAMES[posIdx]
    local pShort = PES10.posShort(posIdx)
    local apt = (posIdx == regPos) and 2 or PES10.getPosAptitude(p, posIdx)
    local aptLetter = (apt == 2 and "A") or (apt == 1 and "B") or "C"
    local tag = (posIdx == regPos) and PES10.T(" [PRINCIPAL]", " [REGISTERED]") or ""
    table.insert(lines, string.format("[%2d] %-3s (%-16s): Grau %s%s", idx, pShort, PES10.T(info.pt, info.en), aptLetter, tag))
  end
  table.insert(lines, "----------------------------------------------------")
  table.insert(lines, PES10.T("Digite 0 (Promover B→A), 99 (Todas A), 1-13 ou sigla (ex: MAT, CA):", "Enter 0 (Upgrade B→A), 99 (All A), 1-13 or code (e.g. AMF, CF):"))

  local ans = inputQuery(
    PES10.T("Afinidades de Posição - ", "Position Familiarity - ") .. pName,
    table.concat(lines, "\n"),
    "0"
  )
  if not ans or ans:match("^%s*$") then return end
  local choice = tonumber(ans)
  if choice == 0 then
    local upCount = PES10.promoteSecondaryPositionsToA(p)
    PES10.toast(string.format(
      PES10.T("%d posição(ões) secundária(s) de %s promovidas para 'A'!", "%d secondary position(s) of %s upgraded to 'A'!"),
      upCount, pName
    ))
  elseif choice == 99 then
    PES10.runPlayerMacro("all_positions_a")
    PES10.toast(string.format(PES10.T("Todas as 13 posições de %s definidas como 'A'!", "All 13 positions of %s set to 'A'!"), pName))
  else
    local targetPos = nil
    if choice and choice >= 1 and choice <= 13 then
      targetPos = posOrderDisplay[choice]
    else
      local code = ans:upper():gsub("%s+", "")
      for pIdx = 0, 12 do
        if PES10.POS_SHORT_PT[pIdx] == code or PES10.POS_SHORT_EN[pIdx] == code then
          targetPos = pIdx; break
        end
      end
    end
    if targetPos == nil then
      PES10.toast(PES10.T("Posição inválida: ", "Invalid position: ") .. tostring(ans))
      return
    end
    local pInfo = PES10.POS_NAMES[targetPos]
    local pShort = PES10.posShort(targetPos)
    local curApt = PES10.getPosAptitude(p, targetPos)
    local curL = (curApt == 2 and "A") or (curApt == 1 and "B") or "C"
    local gradeAns = inputQuery(
      PES10.T("Alterar Grau da Posição ", "Change Position Grade ") .. pShort,
      string.format(
        PES10.T("Posição: %s (%s). Grau atual: %s.\nDigite o novo grau desejado (A, B ou C):", "Position: %s (%s). Current grade: %s.\nEnter desired new grade (A, B or C):"),
        pShort, PES10.T(pInfo.pt, pInfo.en), curL
      ),
      "A"
    )
    if gradeAns and not gradeAns:match("^%s*$") then
      local gUpper = gradeAns:upper():match("[ABC]")
      if not gUpper then
        PES10.toast(PES10.T("Grau inválido (digite A, B ou C).", "Invalid grade (enter A, B or C)."))
        return
      end
      local gVal = (gUpper == "A" and 2) or (gUpper == "B" and 1) or 0
      PES10.setPosAptitude(p, targetPos, gVal)
      PES10.toast(string.format(
        PES10.T("%s: Posição %s definida para Grau '%s' com sucesso!", "%s: Position %s set to Grade '%s' successfully!"),
        pName, pShort, gUpper
      ))
    end
  end
end

-- Interactive UI for Team Role (0 to 22) and Level (1 to 3)
function PES10.promptSetTeamRole()
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local curRole, curLvl = PES10.getTeamRole(p)
  local curRoleName = PES10.TEAM_ROLES[curRole] and PES10.T(PES10.TEAM_ROLES[curRole].pt, PES10.TEAM_ROLES[curRole].en) or "-"

  local lines = {}
  table.insert(lines, PES10.T("Atleta: ", "Player: ") .. pName)
  table.insert(lines, string.format(PES10.T("Função Atual: %s (Nível %d)", "Current Role: %s (Level %d)"), curRoleName, curLvl))
  table.insert(lines, "----------------------------------------------------")
  table.insert(lines, PES10.T("PRESETS RÁPIDOS (Digite o número correspondente):", "QUICK PRESETS (Enter corresponding role number):"))
  table.insert(lines, PES10.T("19: Tornar LENDA (Nível 3) - Espírito 99 + Bônus XP & Vendas", "19: Make LEGEND (Level 3) - 99 Team Spirit + XP Boost & Sales"))
  table.insert(lines, PES10.T("18: Tornar BANDEIRA (Nível 3) - Fidelidade & Bônus em Jogos Cruciais", "18: Make BANDIERA (Level 3) - Loyalty & Clutch Match Boost"))
  table.insert(lines, PES10.T("9:  Tornar MAESTRO (Nível 3) - Maestria Técnica & Visão p/ o Time", "9:  Make MAESTRO (Level 3) - Technical Mastery & Vision for Squad"))
  table.insert(lines, PES10.T("7:  Tornar GENERAL (Nível 3) - Liderança Tática & Força Defensiva", "7:  Make GENERAL (Level 3) - Tactical Command & Defensive Grit"))
  table.insert(lines, PES10.T("14: Tornar SUPERESTRELA (Nível 3) - Grande Impacto & Mercadorias", "14: Make SUPERSTAR (Level 3) - Star Appeal & Merchandise"))
  table.insert(lines, "----------------------------------------------------")
  table.insert(lines, PES10.T("Digite o número da Função (0 a 22) desejada:", "Enter desired Role Number (0 to 22):"))

  local ans = inputQuery(
    PES10.T("Função de Equipe (Team Role) - ", "Team Role & Level - ") .. pName,
    table.concat(lines, "\n"),
    "19"
  )
  if not ans or ans:match("^%s*$") then return end
  local roleNum = tonumber(ans)
  if roleNum and roleNum >= 0 and roleNum <= 22 then
    local lvlAns = inputQuery(
      PES10.T("Nível da Função (Level 1-3)", "Role Level (1-3)"),
      PES10.T("Digite o Nível da Função (1, 2 ou 3):", "Enter Role Level (1, 2 or 3):"),
      "3"
    )
    if not lvlAns or lvlAns:match("^%s*$") then return end
    local lvlNum = tonumber(lvlAns) or 3
    lvlNum = math.max(1, math.min(3, lvlNum))
    PES10.setTeamRole(p, roleNum, lvlNum)
    local rInfo = PES10.TEAM_ROLES[roleNum]
    local rText = rInfo and PES10.T(rInfo.pt, rInfo.en) or tostring(roleNum)
    PES10.toast(string.format(
      PES10.T("%s agora é '%s' (Nível %d) - Bônus de Espírito e Evolução ativos!", "%s is now '%s' (Level %d) - Team Spirit & XP boosts active!"),
      pName, rText, lvlNum
    ))
  else
    PES10.toast(PES10.T("Função inválida (digite de 0 a 22).", "Invalid role number (enter 0 to 22)."))
  end
end

-- Master League Real RAM Scout Observation Lists (1, 2, 3 & Shortlist) + Negotiation Engine
function PES10.getScoutListSummary(slotIdx)
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  local def = PES10.SCOUT_LISTS[slotIdx]
  if career == 0 or not def then return 0, (def and def.cap or 30), "" end
  local used = 0
  local preview = {}
  for i = 0, def.cap - 1 do
    local h = PES10.readU64(career + def.base + i * 8)
    if h ~= 0 and h ~= PES10.SCOUT_EMPTY_SENTINEL and math.floor(h / 0x100000000) ~= 0 then
      used = used + 1
      if #preview < 2 then
        local lo = h % 0x100000000
        if lo < 30000 then
          local p = career + lo * 0x17C
          local nm = readString(p + 0x38, 22, false)
          if nm and nm ~= "" then preview[#preview + 1] = nm end
        end
      end
    end
  end
  return used, def.cap, table.concat(preview, ", ")
end

function PES10.getScoutListEntries(slotIdx)
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  local def = PES10.SCOUT_LISTS[slotIdx]
  local entries = {}
  if career == 0 or not def then return entries end
  for i = 0, def.cap - 1 do
    local addr = career + def.base + i * 8
    local h = PES10.readU64(addr)
    if h ~= 0 and h ~= PES10.SCOUT_EMPTY_SENTINEL and math.floor(h / 0x100000000) ~= 0 then
      local lo = h % 0x100000000
      local p = (lo < 30000) and (career + lo * 0x17C) or 0
      local nm = (p ~= 0) and (readString(p + 0x38, 34, false) or "?") or "?"
      local ovr, regPos = (p ~= 0) and PES10.calcPlayerOVR(p) or 0, 0
      local age = (p ~= 0) and (PES10.readU8(p + 0x1C) & 0x3F) or 0
      local _, _, club = (p ~= 0) and PES10.decodeClubHandle(PES10.readU32(p + 0x12C)) or 0, 0, ""
      entries[#entries + 1] = {
        slotIndex = i, slotAddr = addr, handle = h, playerAddr = p,
        name = nm, ovr = ovr, pos = regPos, age = age, club = club,
      }
    end
  end
  return entries
end

function PES10.removePlayerFromScoutList(slotIdx, targetHandle)
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  local def = PES10.SCOUT_LISTS[slotIdx]
  if career == 0 or not def or not targetHandle then return false end
  for i = 0, def.cap - 1 do
    local addr = career + def.base + i * 8
    if PES10.readU64(addr) == targetHandle then
      writeQword(addr, PES10.SCOUT_EMPTY_SENTINEL)
      return true
    end
  end
  return false
end

function PES10.applyEasyNegotiationFlags(p)
  if not p or p <= 0x10000 then return end
  -- 1) Transfer Listed & Loan Listed (playerAddr + 0x14A, bits 1 & 2)
  PES10.setBitsInDword(p + 0x14A, 1, 1, 1)
  PES10.setBitsInDword(p + 0x14A, 2, 1, 1)
  -- 2) Club Affection 100%
  PES10.setBitsInDword(p + 0x13C, 18, 7, 100)
  -- 3) Clear contract renegotiation cooldown & recent arrival lock
  writeInteger(p + 0x16C, 0x0000FFFF)
  local joinYear = PES10.readU16(p + 0x168)
  if joinYear >= 1995 and joinYear <= 2200 then
    writeSmallInteger(p + 0x168, math.max(2018, joinYear - 2))
  end
  -- 4) Nominal Release Clause override (€ 1.000 / R$ 1.000)
  PES10.setOverrideReleaseFeeForPlayer(p, 10)
end

function PES10.addPlayerToScoutList(slotIdx)
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return false end
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    PES10.toast(PES10.T("Carregue um save de Master League primeiro!", "Load a Master League save first!"))
    return false
  end
  slotIdx = math.max(1, math.min(4, slotIdx or 1))
  local def = PES10.SCOUT_LISTS[slotIdx]
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local curOVR, regPos = PES10.calcPlayerOVR(p)
  local pHandle = PES10.readU64(p + 0x2C)
  if pHandle == 0 or math.floor(pHandle / 0x100000000) == 0 then
    PES10.toast(PES10.T("Handle de jogador inválido na RAM.", "Invalid player handle in RAM."))
    return false
  end

  -- Check if already present in target RAM list or find first free slot (0x000000000000FFFF)
  local freeSlot = -1
  local alreadyIn = false
  for i = 0, def.cap - 1 do
    local addr = career + def.base + i * 8
    local h = PES10.readU64(addr)
    if h == pHandle then
      alreadyIn = true
      break
    elseif freeSlot == -1 and (h == 0 or h == PES10.SCOUT_EMPTY_SENTINEL or math.floor(h / 0x100000000) == 0) then
      freeSlot = i
    end
  end

  if not alreadyIn then
    if freeSlot == -1 then freeSlot = def.cap - 1 end
    writeQword(career + def.base + freeSlot * 8, pHandle)
  end

  PES10.syncAllDynamicPointers()

  local listLabel = PES10.T(def.pt, def.en)
  if alreadyIn then
    PES10.toast(string.format(
      PES10.T("%s (%d %s) já está em %s.", "%s (%d %s) is already in %s."),
      pName, curOVR, PES10.posShort(regPos), listLabel
    ))
  else
    PES10.toast(string.format(
      PES10.T(
        "%s (%d %s) adicionado em %s na RAM (valores e contrato preservados).",
        "%s (%d %s) added to %s in RAM (values and contract preserved)."
      ),
      pName, curOVR, PES10.posShort(regPos), listLabel
    ))
  end
  return true
end

function PES10.clearScoutSlots(onlySlot)
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    PES10.toast(PES10.T("Carregue um save de Master League primeiro!", "Load a Master League save first!"))
    return false
  end
  local targets = onlySlot and { onlySlot } or { 1, 2, 3 }
  for _, sIdx in ipairs(targets) do
    local def = PES10.SCOUT_LISTS[sIdx]
    if def then
      for i = 0, def.cap - 1 do
        writeQword(career + def.base + i * 8, PES10.SCOUT_EMPTY_SENTINEL)
      end
    end
  end
  PES10.toast(PES10.T("Listas de Observação dos 3 Olheiros limpas na RAM!", "All 3 Scout Observation Lists cleared in RAM!"))
  return true
end

-- Make Deal Irresistible (Immediate transfer proposal acceptance at minimum fee)
function PES10.makeDealIrresistible()
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return false end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")

  PES10.applyEasyNegotiationFlags(p)
  PES10.syncAllDynamicPointers()
  PES10.toast(string.format(
    PES10.T(
      "Negociação Irrecusável em %s: Lista Transf. + Multa € 1k + Afeição 100%%!",
      "Irresistible Deal on %s: Transfer Listed + € 1k Clause + 100%% Affection!"
    ),
    pName
  ))
  return true
end

function PES10.promptAddPlayerToScoutList()
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local curOVR, regPos = PES10.calcPlayerOVR(p)

  local lines = {}
  table.insert(lines, PES10.T("== MENU DE OBSERVAÇÃO / OLHEIROS (RAM AO VIVO) ==", "== SCOUT OBSERVATION LISTS (LIVE RAM) =="))
  table.insert(lines, string.format(PES10.T("Atleta: %s [%d OVR - %s]", "Player: %s [%d OVR - %s]"), pName, curOVR, PES10.posShort(regPos)))
  table.insert(lines, "----------------------------------------------------")
  for s = 1, 4 do
    local def = PES10.SCOUT_LISTS[s]
    local used, cap, prev = PES10.getScoutListSummary(s)
    local occ = (used > 0) and string.format("%d/%d (%s)", used, cap, prev) or PES10.T("0/" .. cap .. " [Vazio]", "0/" .. cap .. " [Empty]")
    table.insert(lines, string.format("[%d] %s: %s", s, PES10.T(def.pt, def.en), occ))
  end
  table.insert(lines, "----------------------------------------------------")
  table.insert(lines, PES10.T("0: Limpar os 3 Olheiros de Observação (1, 2 e 3)", "0: Clear all 3 Scout Observation Lists (1, 2 & 3)"))
  table.insert(lines, "----------------------------------------------------")
  table.insert(lines, PES10.T("Digite 1, 2, 3 (Olheiro), 4 (Favoritos) ou 0 (Limpar):", "Enter 1, 2, 3 (Scout), 4 (Shortlist) or 0 (Clear):"))

  local ans = inputQuery(
    PES10.T("Menu de Observação (Olheiros 1-3) - ", "Scout Observation Menu (1-3) - ") .. pName,
    table.concat(lines, "\n"),
    "1"
  )
  if not ans or ans:match("^%s*$") then return end
  local choice = tonumber(ans)
  if choice == 0 then
    PES10.clearScoutSlots()
  elseif choice and choice >= 1 and choice <= 4 then
    PES10.addPlayerToScoutList(choice)
  else
    PES10.toast(PES10.T("Opção inválida (digite 0 a 4).", "Invalid option (enter 0 to 4)."))
  end
end

function PES10.promptTargetOVRProportional()
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return false end
  local curOVR, regPos = PES10.calcPlayerOVR(p)
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local title = PES10.T("Overall Alvo (DNA) - ", "Target Overall (DNA) - ") .. pName
  local sub = string.format(
    PES10.T("Atual: %d %s  •  Digite o novo OVR (40-99) preservando proporções:", "Current: %d %s  •  Enter target OVR (40-99) keeping DNA:"),
    curOVR, PES10.posShort(regPos)
  )
  local function applyVal(ans)
    local t = tonumber(ans)
    if not t then PES10.toast(PES10.T("Overall inválido.", "Invalid Overall value.")); return end
    local ok, oldO, newO = PES10.scalePlayerToTargetOVR(p, t)
    if ok then
      PES10.toast(string.format(
        PES10.T("%s ajustado de %d -> %d OVR (DNA preservado).", "%s scaled %d -> %d OVR (DNA preserved)."),
        pName, oldO, newO
      ))
      if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
    end
  end
  if PES10.openDarkPrompt then
    PES10.openDarkPrompt(title, sub, tostring(math.min(99, curOVR + 3)), 0xF8BD38, applyVal)
    return true
  end
  local ans = inputQuery(title, sub, tostring(math.min(99, curOVR + 3)))
  if not ans then return false end
  applyVal(ans)
  return true
end

function PES10.rejuvenateAndSyncMarket(yearsDelta)
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return false end
  PES10.pushPlayerUndo(p, PES10.T("Rejuvenescer", "Rejuvenate"))
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local oldAge = PES10.readU8(p + 0x1C) & 0x3F
  local newAge = math.max(16, math.min(45, oldAge + (yearsDelta or -2)))
  PES10.setBitsInDword(p + 0x1C, 0, 6, newAge)

  local curOVR = PES10.calcPlayerOVR(p)
  local snapOVR = PES10.readU8(p + 0x179)
  local mktRaw  = PES10.readI32(p + 0x174)
  if mktRaw > 0 and snapOVR >= 40 and curOVR ~= snapOVR then
    local ratio = curOVR / snapOVR
    mktRaw = math.max(500, math.min(20000000, math.floor(mktRaw * (ratio ^ 2.3) + 0.5)))
    writeInteger(p + 0x174, mktRaw)
  end
  writeBytes(p + 0x178, newAge, curOVR)
  PES10.syncAllDynamicPointers()
  PES10.toast(string.format(
    PES10.T("%s: Idade %d → %d anos | Valor & OVR %d sincronizados!", "%s: Age %d → %d yrs | Market & OVR %d synced!"),
    pName, oldAge, newAge, curOVR
  ))
  return true
end

-- ============================================================================
-- LIVE MATCH CLOCK CONTROL (FNV-1a CRASH-FREE!)
-- ============================================================================
PES10.PERIOD_NAMES_PT = {
  [0] = "Pré-Jogo", [1] = "1º Tempo", [2] = "Intervalo", [3] = "2º Tempo",
  [4] = "Fim 90'", [5] = "1ª Prorr.", [6] = "Int. Prorr.", [7] = "2ª Prorr.", [8] = "Pênaltis",
}
PES10.PERIOD_NAMES_EN = {
  [0] = "Pre-Match", [1] = "1st Half", [2] = "Half-Time", [3] = "2nd Half",
  [4] = "End 90'", [5] = "1st ET", [6] = "ET Break", [7] = "2nd ET", [8] = "Penalties",
}
PES10.PERIOD_BASE_MIN = { [1] = 0, [3] = 45, [5] = 90, [7] = 105 }

function PES10.getMatchClockInfo()
  local mState = PES10.readU64(PES10.symAddr("ptrMatchState"))
  if mState == 0 then return nil end

  local period = PES10.readU32(mState + 0x1168)
  local protTicks = readFloat(PES10.symAddr("valProtectedMatchTicks")) or 0.0
  local offTicks = readFloat(mState + 0x1174) or 0.0
  local halfTicks = math.max(0.0, protTicks + offTicks)

  local baseMin = PES10.PERIOD_BASE_MIN[period] or 0
  local totalMinutesFloat = baseMin + (halfTicks / 3600.0)
  local mm = math.floor(totalMinutesFloat)
  local ss = math.floor((totalMinutesFloat - mm) * 60.0 + 0.5)
  if ss >= 60 then mm = mm + 1; ss = 0 end

  local mHud = PES10.readU64(PES10.symAddr("ptrMatchHud"))
  if mHud ~= 0 then
    local hudMin = PES10.readI32(mHud + 0x12C)
    local hudSec = PES10.readI32(mHud + 0x130)
    if hudMin >= 0 and hudMin <= 150 and hudSec >= 0 and hudSec < 60 then
      mm, ss = hudMin, hudSec
    end
  end

  local pTbl = (PES10.lang == "en") and PES10.PERIOD_NAMES_EN or PES10.PERIOD_NAMES_PT
  local pName = pTbl[period] or ("P" .. tostring(period))
  return {
    matchState = mState, period = period,
    periodName = pName,
    baseMin = baseMin, protTicks = protTicks, offTicks = offTicks, halfTicks = halfTicks,
    mm = mm, ss = ss,
    formatted = string.format("%02d:%02d (%s)", mm, ss, pName),
  }
end

function PES10.setExactMatchMinute(targetMinuteFloat)
  PES10.ensureHookActive(5)
  local info = PES10.getMatchClockInfo()
  if not info then
    PES10.toast(PES10.T("Nenhuma partida ao vivo em andamento.", "No live match currently in progress."))
    return false
  end

  local p = info.period
  local baseMin = PES10.PERIOD_BASE_MIN[p] or 0
  local halfMin = targetMinuteFloat - baseMin
  if p == 1 and targetMinuteFloat >= 50 then
    writeInteger(info.matchState + 0x1168, 3)
    baseMin = 45; halfMin = targetMinuteFloat - 45
  elseif p == 3 and targetMinuteFloat < 45 then
    writeInteger(info.matchState + 0x1168, 1)
    baseMin = 0; halfMin = targetMinuteFloat
  end

  if halfMin < 0 then halfMin = 0 end
  local desiredHalfTicks = halfMin * 3600.0
  writeFloat(info.matchState + 0x1174, desiredHalfTicks - info.protTicks)

  if PES10.readU32(PES10.symAddr("cfgFreezeClock")) == 1 then
    writeFloat(PES10.symAddr("valFrozenTotalTicks"), desiredHalfTicks)
  end
  PES10.toast(string.format(
    PES10.T("Relógio ajustado para %02d:%02d!", "Match clock set to %02d:%02d!"),
    math.floor(targetMinuteFloat), math.floor((targetMinuteFloat % 1) * 60 + 0.5)
  ))
  return true
end

function PES10.shiftMatchMinutes(deltaMinutes)
  local info = PES10.getMatchClockInfo()
  if not info then PES10.toast(PES10.T("Entre em uma partida primeiro!", "Start a live match first!")); return end
  local curMin = info.baseMin + (info.halfTicks / 3600.0)
  PES10.setExactMatchMinute(math.max(info.baseMin, curMin + deltaMinutes))
end

function PES10.endCurrentHalfNow()
  PES10.ensureHookActive(5)
  local info = PES10.getMatchClockInfo()
  if not info then PES10.toast(PES10.T("Entre em uma partida primeiro!", "Start a live match first!")); return end
  local maxHalfMin = (info.period >= 5) and 15.5 or 46.5
  writeFloat(info.matchState + 0x1174, (maxHalfMin * 3600.0) - info.protTicks)
  PES10.toast(PES10.T("Fim do tempo atual acionado!", "End of current half triggered!"))
end

function PES10.resetCurrentHalfNow()
  PES10.ensureHookActive(5)
  local info = PES10.getMatchClockInfo()
  if not info then PES10.toast(PES10.T("Entre em uma partida primeiro!", "Start a live match first!")); return end
  writeFloat(info.matchState + 0x1174, 0.0 - info.protTicks)
  if PES10.readU32(PES10.symAddr("cfgFreezeClock")) == 1 then
    writeFloat(PES10.symAddr("valFrozenTotalTicks"), 0.0)
  end
  PES10.toast(PES10.T("Tempo atual reiniciado para o início!", "Current half reset to kick-off!"))
end

-- ============================================================================
-- EXACT RELEASE FEE OVERRIDE (64 SLOTS)
-- ============================================================================
function PES10.setOverrideReleaseFeeForPlayer(playerAddr, rawFeeDiv100)
  if not playerAddr or playerAddr == 0 or PES10.stateBase == 0 then return false end
  local pHandle = PES10.readU64(playerAddr + 0x2C)
  if pHandle == 0 then return false end

  PES10.ensureHookActive(2)
  PES10.setBitsInDword(playerAddr + 0x144, 30, 1, 1)

  local tbl = PES10.stateBase + PES10.SYM_OFFSETS.rfeeOverrideTable
  local freeSlot = -1
  for i = 0, 63 do
    local entry = tbl + i * 16
    local active = PES10.readU32(entry + 0x0C)
    if active == 1 and PES10.readU64(entry) == pHandle then
      writeInteger(entry + 0x08, rawFeeDiv100)
      writeInteger(PES10.stateBase + PES10.SYM_OFFSETS.valCustomReleaseFee, rawFeeDiv100)
      PES10.syncAllDynamicPointers()
      return true
    elseif active == 0 and freeSlot == -1 then
      freeSlot = i
    end
  end
  if freeSlot == -1 then freeSlot = 0 end
  local entry = tbl + freeSlot * 16
  writeQword(entry, pHandle)
  writeInteger(entry + 0x08, rawFeeDiv100)
  writeInteger(entry + 0x0C, 1)
  writeInteger(PES10.stateBase + PES10.SYM_OFFSETS.valCustomReleaseFee, rawFeeDiv100)
  PES10.syncAllDynamicPointers()
  return true
end

function PES10.clearOverrideReleaseFeeForPlayer(playerAddr)
  if not playerAddr or playerAddr == 0 or PES10.stateBase == 0 then return end
  local pHandle = PES10.readU64(playerAddr + 0x2C)
  local tbl = PES10.stateBase + PES10.SYM_OFFSETS.rfeeOverrideTable
  for i = 0, 63 do
    local entry = tbl + i * 16
    if PES10.readU32(entry + 0x0C) == 1 and PES10.readU64(entry) == pHandle then
      writeQword(entry, 0); writeInteger(entry + 0x08, 0); writeInteger(entry + 0x0C, 0)
    end
  end
  writeInteger(PES10.stateBase + PES10.SYM_OFFSETS.valCustomReleaseFee, 0)
  PES10.syncAllDynamicPointers()
  PES10.toast(PES10.T("Override de Taxa de Rescisão removido.", "Custom Release Fee override cleared."))
end

-- ============================================================================
-- GLOBAL CAREER PLAYER SEARCH (30,000 PLAYERS) & BAL MY PLAYER SELECTOR
-- ============================================================================
function PES10.pointToBALMyPlayer()
  PES10.syncAllDynamicPointers()
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    PES10.toast(PES10.T("Carregue um save de Rumo ao Estrelato (BAL) primeiro!", "Load a Become a Legend (BAL) save first!"))
    return false
  end
  local balHandle = PES10.readU64(career + 0x01819540)
  if math.floor(balHandle / 0x100000000) == 0 then
    PES10.toast(PES10.T("Save atual é Master League (sem jogador único de BAL).", "Current save is Master League (no single BAL pro active)."))
    return false
  end
  local count = PES10.readU32(career + 0x00D0BCE8)
  if count == 0 or count > 30001 then count = 30000 end
  for i = 0, count - 1 do
    local p = career + (i * 0x17C)
    if PES10.readU64(p + 0x2C) == balHandle then
      writeQword(PES10.symAddr("ptrPlayer"), p)
      PES10.syncAllDynamicPointers()
      local nm = readString(p + 0x38, 46, false) or "BAL Pro"
      PES10.toast(PES10.T("Selecionado atleta do BAL: ", "Selected BAL player: ") .. nm)
      return true
    end
  end
  return false
end

function PES10.searchCareerPlayerDialog()
  PES10.syncAllDynamicPointers()
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    showMessage(PES10.T(
      "Carregue um save de Master League ou Rumo ao Estrelato primeiro!",
      "Please load a Master League or Become a Legend save first!"
    ))
    return
  end

  local query = inputQuery(
    PES10.T("PES 2021 v10.2 - Buscar Jogador na Carreira", "PES 2021 v10.2 - Search Career Player"),
    PES10.T("Digite parte do Nome (com ou sem acento) ou o Player ID:", "Enter part of the Player Name (accents optional) or Player ID:"),
    ""
  )
  if not query or query == "" then return end
  local qNorm = PES10.normalizeText(query)
  local qNum = tonumber(query)

  local count = PES10.readU32(career + 0x00D0BCE8)
  if count == 0 or count > 30001 then count = 30000 end

  local matches = {}
  for i = 0, count - 1 do
    local p = career + (i * 0x17C)
    local pid = PES10.readU32(p + 0x30)
    if pid ~= 0 and pid ~= 0xFFFFFFFF then
      local name = readString(p + 0x38, 60, false) or ""
      if (qNum and pid == qNum) or (name ~= "" and PES10.normalizeText(name):find(qNorm, 1, true)) then
        local age = PES10.readU8(p + 0x1C) % 64
        local ovr, regPos = PES10.calcPlayerOVR(p)
        local posStr = PES10.posShort(regPos)
        local _, _, clubDesc = PES10.decodeClubHandle(PES10.readU32(p + 0x12C))
        matches[#matches + 1] = { addr = p, id = pid, name = name, age = age, ovr = ovr, pos = posStr, club = clubDesc }
        if #matches >= 80 then break end
      end
    end
  end

  if #matches == 0 then
    local msg = PES10.T("Nenhum jogador encontrado para: ", "No player found matching: ") .. query
    PES10.toast(msg)
    showMessage(msg)
    return
  elseif #matches == 1 then
    local m = matches[1]
    writeQword(PES10.symAddr("ptrPlayer"), m.addr)
    PES10.syncAllDynamicPointers()
    PES10.toast(string.format(PES10.T("Carregado: %s (%d %s - %s)", "Loaded: %s (%d %s - %s)"), m.name, m.ovr, m.pos, m.club))
    return
  end

  local sl = createStringlist()
  local ageUnit = PES10.T("anos", "yo")
  for _, m in ipairs(matches) do
    sl.add(string.format("%s  |  OVR %d (%s)  |  %d %s  |  %s  |  ID %d", m.name, m.ovr, m.pos, m.age, ageUnit, m.club, m.id))
  end
  local idx = showSelectionList(
    PES10.T("Jogadores Encontrados (", "Players Found (") .. tostring(#matches) .. ")",
    PES10.T("Dê duplo-clique no atleta desejado para carregá-lo:", "Double-click a player to load into Studio & Edit Player:"),
    sl,
    false
  )
  sl.destroy()
  if idx and idx >= 0 and matches[idx + 1] then
    local m = matches[idx + 1]
    writeQword(PES10.symAddr("ptrPlayer"), m.addr)
    PES10.syncAllDynamicPointers()
    PES10.toast(string.format(PES10.T("Carregado: %s (%d %s - %s)", "Loaded: %s (%d %s - %s)"), m.name, m.ovr, m.pos, m.club))
  end
end

-- ============================================================================
-- SQUAD COMMANDER - FULL USER SQUAD ACTIONS (MASTER LEAGUE)
-- ============================================================================
function PES10.getUserSquadPlayerAddresses()
  PES10.syncAllDynamicPointers()
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then return {}, 0 end

  local count = PES10.readU32(career + 0x00D0BCE8)
  if count == 0 or count > 30001 then count = 30000 end

  local list = {}
  local userClub = PES10.getUserClubHandle(career)
  if userClub ~= 0 and userClub ~= 0xFFFFFFFF then
    for i = 0, count - 1 do
      local p = career + (i * 0x17C)
      if PES10.readU32(p + 0x12C) == userClub then
        local pid = PES10.readU32(p + 0x30)
        if pid ~= 0 and pid ~= 0xFFFFFFFF then
          list[#list + 1] = p
        end
      end
    end
    if #list > 0 then return list, career end
  end

  -- Fallback: squad training/bonus slot list at career + 0x016EE854
  local handles = {}
  for slot = 0, 39 do
    local entry = career + 0x016EE854 + (slot * 0x34)
    local h = PES10.readU64(entry)
    if h ~= 0 and math.floor(h / 0x100000000) ~= 0 then
      handles[h] = true
    end
  end
  for i = 0, count - 1 do
    local p = career + (i * 0x17C)
    if handles[PES10.readU64(p + 0x2C)] then
      list[#list + 1] = p
    end
  end
  return list, career
end

PES10.PLAYER_STAT_BITS = {}

function PES10.runSquadCommander(mode)
  local squad, career = PES10.getUserSquadPlayerAddresses()
  if #squad == 0 then
    PES10.toast(PES10.T("Carregue um save de Master League primeiro!", "Load a Master League save first!"))
    return 0
  end

  local curYear = PES10.readU16(career + 0x01642A24)
  if curYear < 2019 or curYear > 2150 then curYear = 2026 end

  for _, p in ipairs(squad) do
    if mode == "stamina_100" then
      PES10.setBitsInDword(p + 0x144, 16, 7, 100)
    elseif mode == "blue_form" then
      PES10.setBitsInDword(p + 0x144, 23, 1, 1)
      PES10.setBitsInDword(p + 0x144, 24, 3, PES10.TOP_FORM_VALUE or 4)
    elseif mode == "heal_affection" then
      writeBytes(p + 0x148, 0)
      PES10.setBitsInDword(p + 0x13C, 18, 7, 100)
    elseif mode == "shield_contracts" then
      writeSmallInteger(p + 0x138, curYear + 5)
      PES10.setBitsInDword(p + 0x13C, 18, 7, 100)
      PES10.setBitsInDword(p + 0x144, 30, 1, 0)
    elseif mode == "eternal_peak_breakthrough" then
      PES10.setBitsInDword(p + 0x18, 28, 3, 3)
      local lowH = PES10.readU32(p + 0x2C) % 0x10000
      if lowH < 0x61A9 then
        local gAddr = career + 0x01676324 + (lowH * 4)
        local g = PES10.readU32(gAddr)
        g = (g & 0xF00001FF) | (38 << 17) | (2 << 24)
        if g >= 0x80000000 then g = g - 0x100000000 end
        writeInteger(gAddr, g)
      end
    elseif mode == "plus_one_all_stats" or mode == "plus_one_ovr_dna" then
      local curO = PES10.calcPlayerOVR(p)
      if curO < 99 then
        PES10.scalePlayerToTargetOVR(p, curO + 1)
      end
    end
  end
  PES10.syncAllDynamicPointers()
  PES10.toast(string.format(
    PES10.T("Comando aplicado em %d atletas do seu elenco!", "Squad command applied to %d players!"),
    #squad
  ))
  return #squad
end

-- ============================================================================
-- 1-CLICK SELECTED PLAYER MACROS & JSON BACKUP
-- ============================================================================
function PES10.getSelectedPlayerOrWarn()
  PES10.syncAllDynamicPointers()
  local p = PES10.readU64(PES10.symAddr("ptrPlayer"))
  if p == 0 then
    PES10.toast(PES10.T(
      "Selecione um jogador no menu do jogo ou clique em 'Buscar Jogador'!",
      "Hover a player in-game or click 'Search Career Player' first!"
    ))
    return 0
  end
  return p
end

function PES10.runPlayerMacro(mode)
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))

  if mode == "bal_instant_skill" then
    if career == 0 then PES10.toast(PES10.T("Abra seu save BAL primeiro!", "Open your BAL save first!")); return false end
    local skillId = PES10.readI32(career + 0x016ECBEC)
    local balRec = PES10.readU64(PES10.symAddr("ptrBALTrainingRec"))
    if balRec ~= 0 and skillId >= 0 and skillId < 0x6C then
      writeSmallInteger(balRec + 0x16 + skillId * 2, 9999)
      PES10.toast(PES10.T("Skill em treino colocada em 99,99%!", "Active BAL Skill set to 99.99% (completes next day)!"))
      return true
    end
    PES10.toast(PES10.T("Escolha primeiro uma Skill para treinar no BAL!", "Select a Skill to train in BAL menu first!"))
    return false
  elseif mode == "bal_instant_pos" then
    if career == 0 then PES10.toast(PES10.T("Abra seu save BAL primeiro!", "Open your BAL save first!")); return false end
    local posId = PES10.readI32(career + 0x016ECBF0)
    local balRec = PES10.readU64(PES10.symAddr("ptrBALTrainingRec"))
    if balRec ~= 0 and posId > 0 and posId < 0x0E then
      writeSmallInteger(balRec + 0x16 + posId * 2, 9999)
      PES10.toast(PES10.T("Posição em treino colocada em 99,99%!", "Active BAL Position set to 99.99% (completes next day)!"))
      return true
    end
    PES10.toast(PES10.T("Escolha primeiro uma Posição para treinar no BAL!", "Select a Position to train in BAL menu first!"))
    return false
  elseif mode == "bal_max_sliders" then
    if career == 0 then PES10.toast(PES10.T("Abra seu save BAL primeiro!", "Open your BAL save first!")); return false end
    PES10.ensureHookActive(4)
    for i = 0, 9 do writeBytes(career + 0x016ECBE0 + i, 6) end
    writeBytes(career + 0x016ECBEA, 99)
    PES10.toast(PES10.T("10 Barras no máximo + 99 Focus Points liberados no BAL!", "All 10 BAL Sliders maxed + 99 Focus Points unlocked!"))
    return true
  end

  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return false end
  if mode ~= "instant_subxp_player" then PES10.pushPlayerUndo(p, "Macro:" .. tostring(mode)) end
  local pName = readString(p + 0x38, 36, false) or PES10.T("Atleta", "Player")

  if mode == "stats_99" then
    for _, sb in ipairs(PES10.PLAYER_STAT_BITS) do PES10.setBitsInDword(p + sb[1], sb[2], 7, 99) end
    PES10.toast(PES10.T("Todos os 25 atributos em 99 para ", "All 25 attributes set to 99 for ") .. pName .. "!")
  elseif mode == "stats_plus_5" then
    for _, sb in ipairs(PES10.PLAYER_STAT_BITS) do
      local v = PES10.getBitsInDword(p + sb[1], sb[2], 7)
      PES10.setBitsInDword(p + sb[1], sb[2], 7, math.min(99, v + 5))
    end
    PES10.toast(PES10.T("+5 em todos os atributos para ", "+5 to all attributes applied for ") .. pName .. "!")
  elseif mode == "all_positions_a" then
    local posBits = {
      {0x1F, 1}, {0x1F, 3}, {0x1F, 5}, {0x20, 0}, {0x20, 2}, {0x20, 4}, {0x20, 6},
      {0x21, 0}, {0x21, 2}, {0x21, 4}, {0x21, 6}, {0x22, 0}, {0x22, 2},
    }
    for _, pb in ipairs(posBits) do PES10.setBitsInDword(p + pb[1], pb[2], 2, 2) end
    PES10.toast(PES10.T("Todas as 13 posições definidas como 'A' para ", "All 13 playable positions set to 'A' for ") .. pName .. "!")
  elseif mode == "all_39_skills" then
    for _, sk in ipairs(PES10.ALL_SKILL_BITS or {}) do PES10.setBitsInDword(p + sk[1], sk[2], 1, 1) end
    PES10.toast(PES10.T("Todas as 41 habilidades desbloqueadas para ", "All 41 Player Skills unlocked for ") .. pName .. "!")
  elseif mode == "perfect_wf_form_injury" then
    PES10.setBitsInDword(p + 0x23, 0, 2, 3)
    PES10.setBitsInDword(p + 0x22, 4, 2, 3)
    PES10.setBitsInDword(p + 0x17, 4, 3, 7)
    PES10.setBitsInDword(p + 0x22, 6, 2, 2)
    writeBytes(p + 0x148, 0)
    PES10.toast(PES10.T("Pé Ruim 4/4, Forma 8, Lesão 3 e saúde 100% em ", "Weak Foot 4/4, Form 8, Injury Res 3 & healed: ") .. pName .. "!")
  elseif mode == "unlock_renegotiation" then
    writeInteger(p + 0x16C, 0x0000FFFF)
    local joinYear = PES10.readU16(p + 0x168)
    if joinYear >= 1995 and joinYear <= 2200 then
      writeSmallInteger(p + 0x168, math.max(2018, joinYear - 2))
    end
    PES10.toast(PES10.T("Bloqueio de contrato recente removido para ", "Recent contract lock cleared for ") .. pName .. "!")
  elseif mode == "easy_sign_target" then
    PES10.setOverrideReleaseFeeForPlayer(p, 10) -- € 1.000 (raw = 10)
    PES10.setBitsInDword(p + 0x13C, 18, 7, 100)
    writeInteger(p + 0x16C, 0x0000FFFF)
    local joinYear = PES10.readU16(p + 0x168)
    if joinYear >= 1995 and joinYear <= 2200 then
      writeSmallInteger(p + 0x168, math.max(2018, joinYear - 2))
    end
    PES10.toast(PES10.T("Contratação Fácil pronta em ", "Easy Signing ready for ") .. pName .. " (€ 1,000 + 100%)!")
  elseif mode == "instant_subxp_player" then
    local trn = PES10.readU64(PES10.symAddr("ptrPlayerTraining"))
    if trn == 0 then
      PES10.toast(pName .. PES10.T(" não está nos 82 slots de treino do seu clube/BAL.", " is not in your club/BAL 82 training slots."))
      return false
    end
    for fid = 14, 39 do writeSmallInteger(trn + 0x16 + fid * 2, 9999) end
    PES10.toast(PES10.T("25 barras de Sub-XP em 9.999 / 10.000 para ", "All 25 Sub-XP bars set to 9,999 / 10,000 for ") .. pName .. "!")
  end
  PES10.syncAllDynamicPointers()
  return true
end

function PES10.exportPlayerBackupJson()
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local rawBytes = readBytes(p, 0x17C, true)
  if not rawBytes then return end
  local name = (readString(p + 0x38, 60, false) or "player"):gsub("[^%w_%-]", "_")
  local pid = PES10.readU32(p + 0x30)

  local dlg = createSaveDialog(MainForm)
  dlg.Title = PES10.T("Exportar Backup JSON do Jogador", "Export Player JSON Backup")
  dlg.Filter = "JSON Files (*.json)|*.json"
  dlg.FileName = string.format("PES2021_%s_%d.json", name, pid)
  if dlg.execute() then
    local hexArr = {}
    for i = 1, #rawBytes do hexArr[i] = string.format("%02X", rawBytes[i]) end
    local content = string.format(
      '{\n  "version": "PES2021_v10.2",\n  "player_id": %d,\n  "player_name": %q,\n  "struct_size": 380,\n  "hex_0x17C": "%s"\n}\n',
      pid, readString(p + 0x38, 60, false) or "", table.concat(hexArr, "")
    )
    local f = io.open(dlg.FileName, "w")
    if f then f:write(content); f:close(); PES10.toast(PES10.T("JSON exportado: ", "JSON exported: ") .. dlg.FileName) end
  end
  dlg.destroy()
end

function PES10.importPlayerBackupJson()
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local dlg = createOpenDialog(MainForm)
  dlg.Title = PES10.T("Importar Backup JSON no Jogador Selecionado", "Import JSON Backup into Selected Player")
  dlg.Filter = "JSON Files (*.json)|*.json"
  if dlg.execute() then
    local f = io.open(dlg.FileName, "r")
    if f then
      local txt = f:read("*a"); f:close()
      local hex = txt:match('"hex_0x17C"%s*:%s*"([0-9A-Fa-f]+)"')
      if hex and #hex == 760 then
        PES10.pushPlayerUndo(p, "Import JSON")
        local curBytes = readBytes(p, 0x17C, true)
        local newBytes = {}
        for i = 1, 0x17C do newBytes[i] = tonumber(hex:sub((i - 1) * 2 + 1, i * 2), 16) end
        for keep = 0x2C, 0x33 do newBytes[keep + 1] = curBytes[keep + 1] end
        for keep = 0x12C, 0x137 do newBytes[keep + 1] = curBytes[keep + 1] end
        for keep = 0x160, 0x167 do newBytes[keep + 1] = curBytes[keep + 1] end
        writeBytes(p, newBytes)
        PES10.syncAllDynamicPointers()
        PES10.toast(PES10.T("Backup JSON importado (ID e Clube preservados)!", "JSON Backup imported (ID & Club preserved)!"))
      end
    end
  end
  dlg.destroy()
end

-- ============================================================================
-- CHEAT ENGINE ADDRESSLIST DISPLAY & DOUBLE-CLICK HOOKS
-- ============================================================================
PES10.MONEY_DESC_TAGS = {}
PES10.CLUB_HANDLE_IDS = {}
PES10.CLOCK_RECORD_ID = -1
PES10.STATUS_RECORD_ID = -1
PES10.CUSTOM_RFEE_RECORD_ID = -1
PES10.COMPUTED_OVR_RECORD_ID = -1
PES10.SCORE_SYNC_IDS = {}

function PES10.bindTableHooks()
  local al = getAddressList()
  if not al then return end

  for i = 0, al.Count - 1 do
    local mr = al.getMemoryRecord(i)
    local id = mr.ID
    if PES10.MONEY_DESC_TAGS[id] then
      mr.OnGetDisplayValue = function(rec, valStr)
        local raw = tonumber(valStr)
        if not raw then return false, valStr end
        return true, PES10.formatMoneyFromRaw(raw)
      end
    elseif PES10.CLUB_HANDLE_IDS[id] then
      mr.OnGetDisplayValue = function(rec, valStr)
        local raw = tonumber(valStr)
        if not raw then return false, valStr end
        local _, _, label = PES10.decodeClubHandle(raw)
        return true, label
      end
    elseif id == PES10.COMPUTED_OVR_RECORD_ID then
      mr.OnGetDisplayValue = function(rec, valStr)
        local p = PES10.readU64(PES10.symAddr("ptrPlayer"))
        if p == 0 then return true, "--" end
        local ovr, regPos = PES10.calcPlayerOVR(p)
        if ovr <= 0 then return true, "--" end
        return true, string.format("%d  [%s]", ovr, PES10.posShort(regPos))
      end
    elseif id == PES10.CLOCK_RECORD_ID then
      mr.OnGetDisplayValue = function(rec, valStr)
        local info = PES10.getMatchClockInfo()
        if not info then return true, PES10.T("Fora de Partida", "Not in Match") end
        return true, info.formatted
      end
    elseif id == PES10.STATUS_RECORD_ID then
      mr.OnGetDisplayValue = function(rec, valStr)
        if not PES10.isAttached() then
          return true, PES10.T("Aguardando PES2021.exe...", "Waiting for PES2021.exe...")
        end
        local modeStr, clubStr = PES10.getCareerModeAndClubInfo()
        local p = PES10.readU64(PES10.symAddr("ptrPlayer"))
        local pDesc = PES10.T("Nenhum", "None")
        if p ~= 0 then
          local nm = readString(p + 0x38, 32, false) or PES10.T("Atleta", "Player")
          local ovr, rp = PES10.calcPlayerOVR(p)
          pDesc = string.format("%s (%d %s)", nm, ovr, PES10.posShort(rp))
        end
        if clubStr ~= "" then modeStr = modeStr .. " (" .. clubStr .. ")" end
        return true, string.format("Online | %s | %s", modeStr, pDesc)
      end
    end
  end

  al.OnValueChange = function(addrlist, memrec)
    local id = memrec.ID
    if id == PES10.COMPUTED_OVR_RECORD_ID then
      PES10.promptTargetOVRProportional()
      return true
    end

    if PES10.MONEY_DESC_TAGS[id] then
      local c = PES10.getCurrentCurrency()
      local curRaw = tonumber(memrec.Value) or 0
      PES10.openDarkPrompt(
        PES10.T("Editar Valor Financeiro (", "Edit Financial Value (") .. c.symbol .. ")",
        PES10.T("Aceita 50M, 500k, 1.5B, 1000 ou 80.000.000:", "Accepts 50M, 500k, 1.5B, 1000 or 80,000,000:"),
        PES10.formatMoneyFromRaw(curRaw),
        0x24BFFB,
        function(ans)
          local newRaw = PES10.parseHumanMoneyToRaw(ans)
          if not newRaw then
            PES10.toast(PES10.T("Valor inválido: ", "Invalid amount: ") .. tostring(ans))
            return
          end
          if id == PES10.CUSTOM_RFEE_RECORD_ID then
            local p = PES10.getSelectedPlayerOrWarn()
            if p ~= 0 then PES10.setOverrideReleaseFeeForPlayer(p, newRaw) end
            return
          end
          memrec.Value = tostring(newRaw)
        end
      )
      return true
    end

    if id == PES10.CLOCK_RECORD_ID then
      local info = PES10.getMatchClockInfo()
      if not info then
        PES10.toast(PES10.T("Entre em uma partida ao vivo primeiro!", "Enter a live match first!"))
        return true
      end
      PES10.openDarkPrompt(
        PES10.T("Minuto Exato da Partida", "Exact Match Minute"),
        PES10.T("Digite o minuto (ex: 15, 44, 88 ou 44:30):", "Enter minute (e.g. 15, 44, 88 or 44:30):"),
        string.format("%02d:%02d", info.mm, info.ss),
        0x24BFFB,
        function(ans)
          local mm, ss = ans:match("^(%d+)%s*:%s*(%d+)$")
          local targetMin = mm and (tonumber(mm) + tonumber(ss) / 60.0) or tonumber((ans:gsub(",", ".")))
          if targetMin then PES10.setExactMatchMinute(targetMin) end
        end
      )
      return true
    end

    if PES10.SCORE_SYNC_IDS[id] then
      local side = PES10.SCORE_SYNC_IDS[id]
      PES10.openDarkPrompt(
        PES10.T("Editar Placar ao Vivo", "Edit Live Score"),
        PES10.T("Digite o número de gols (0-99):", "Enter goal count (0-99):"),
        memrec.Value,
        0x99D334,
        function(ans)
          local g = tonumber(ans)
          if g and g >= 0 and g <= 99 then
            PES10.ensureHookActive(5)
            local mCore = PES10.readU64(PES10.symAddr("ptrMatchCore"))
            local mHud = PES10.readU64(PES10.symAddr("ptrMatchHud"))
            if side == "home" then
              if mCore ~= 0 then writeBytes(mCore + 0x9E, g) end
              if mHud ~= 0 then writeInteger(mHud + 0x120, g) end
            else
              if mCore ~= 0 then writeBytes(mCore + 0x9F, g) end
              if mHud ~= 0 then writeInteger(mHud + 0x124, g) end
            end
          end
        end
      )
      return true
    end
    return false
  end
end

function PES10.asyncActionButton(memrec, fn)
  if memrec then
    local t = createTimer(MainForm, true)
    t.Interval = 60
    t.OnTimer = function(tmr)
      tmr.destroy()
      pcall(fn)
      pcall(function() memrec.Active = false end)
    end
  else
    pcall(fn)
  end
end

-- ============================================================================
-- PES 2021 v12.0 ULTIMATE STUDIO - UNIFIED DARK STUDIO UI KIT (DPI-AWARE)
-- Zero white Win32 inputs, zero gray Win32 buttons, 100% cohesive dark theme
-- ============================================================================
function PES10.uiScale()
  local dpi = (getScreenDPI and getScreenDPI()) or 96
  if dpi < 96 then dpi = 96 end
  local sc = dpi / 96.0
  return function(px) return math.floor(px * sc + 0.5) end
end

local M_BG        = 0x170F0B -- #0B0F17 Deep Midnight
local M_HDR       = 0x20150F -- #0F1520 Header & Footer Surface
local M_SURFACE   = 0x241811 -- #111824 Card / List Surface
local M_INPUT_BG  = 0x1A110C -- #0C111A Recessed Dark Input
local M_INPUT_FOC = 0x221710 -- #101722 Focused Dark Input
local M_BTN_BG    = 0x2B1E16 -- #161E2B Button Idle
local M_BTN_HOV   = 0x3A291F -- #1F293A Button Hover
local M_BTN_DOWN  = 0x1E140E -- #0E141E Button Pressed
local M_BORDER    = 0x453226 -- #263245 Crisp 1px Border
local M_DIVIDER   = 0x33241B -- #1B2433 Subtle Divider
local M_TEXT      = 0xFCFAF8 -- #F8FAFC Primary White Text
local M_SUBTEXT   = 0xB8A394 -- #94A3B8 Muted Slate Text
local M_BLUE      = 0xF8BD38 -- #38BDF8 Sky Blue Accent
local M_GREEN     = 0x99D334 -- #34D399 Emerald Accent
local M_GOLD      = 0x24BFFB -- #FBBF24 Gold Accent
local M_CRIMSON   = 0x5E3FF4 -- #F43F5E Crimson Accent
local M_PURPLE    = 0xFA8BA7 -- #A78BFA Violet Accent

local M_VARIANTS = {
  default = { bg = M_BTN_BG,   hov = M_BTN_HOV,  down = M_BTN_DOWN, bd = M_BORDER,  bdHov = M_BLUE,    fg = M_TEXT,    fgHov = M_TEXT },
  primary = { bg = 0x3D2B14,   hov = 0x523A1C,   down = 0x291D0D,   bd = M_BLUE,    bdHov = 0xFCE08B,  fg = M_TEXT,    fgHov = M_TEXT },
  success = { bg = 0x273314,   hov = 0x35471C,   down = 0x1A220D,   bd = M_GREEN,   bdHov = 0xC2F07D,  fg = M_TEXT,    fgHov = M_TEXT },
  gold    = { bg = 0x18293A,   hov = 0x223A52,   down = 0x101C28,   bd = M_GOLD,    bdHov = 0x7AD9FC,  fg = M_TEXT,    fgHov = M_TEXT },
  danger  = { bg = 0x26193A,   hov = 0x362352,   down = 0x1A1128,   bd = M_CRIMSON, bdHov = 0x9A82F9,  fg = M_TEXT,    fgHov = M_TEXT },
  subtle  = { bg = M_HDR,      hov = M_BTN_BG,   down = M_BTN_DOWN, bd = M_DIVIDER, bdHov = M_BORDER,  fg = M_SUBTEXT, fgHov = M_TEXT },
}

local function utf8Len(s)
  if not s then return 0 end
  local _, cnt = tostring(s):gsub("[^\128-\191]", "")
  return cnt
end

local function padRightUtf8(s, width)
  s = tostring(s or "")
  local len = utf8Len(s)
  if len < width then
    return s .. string.rep(" ", width - len)
  elseif len > width then
    local out, n = {}, 0
    for ch in s:gmatch("[%z\1-\127\194-\244][\128-\191]*") do
      n = n + 1
      if n > width then break end
      out[#out + 1] = ch
    end
    return table.concat(out)
  end
  return s
end

local function makeDarkBox(parent, x, y, w, h, col)
  local p = createPanel(parent)
  p.Left = x; p.Top = y; p.Width = w; p.Height = h
  p.Caption = ""
  p.Color = col
  pcall(function() p.ParentColor = false; p.BevelOuter = "bvNone"; p.BevelInner = "bvNone"; p.BorderStyle = "bsNone" end)
  return p
end

local function makeStudioModal(modalKey, title, logW, logH, accentCol, hdrHLog)
  if modalKey and PES10.openModals and PES10.openModals[modalKey] then
    pcall(function() PES10.openModals[modalKey].close(); PES10.openModals[modalKey].destroy() end)
    PES10.openModals[modalKey] = nil
  end
  local S = PES10.uiScale()
  local f = createForm(false)
  f.Caption = title
  pcall(function() f.BorderStyle = "bsSingle"; f.KeyPreview = true end)
  f.Color = M_BG
  f.DoNotSaveInTable = true
  local W, H = S(logW), S(logH)
  pcall(function() f.ClientWidth = W; f.ClientHeight = H end)
  if not f.ClientWidth or f.ClientWidth ~= W then f.Width = W; f.Height = H end
  f.centerScreen()
  pcall(function()
    f.OnKeyDown = function(sender, key)
      if key == 27 then
        if f._activeComboClose then pcall(f._activeComboClose) else f.close() end
        return 0
      end
      if f._activeGrid and (key == 38 or key == 40 or key == 33 or key == 34) then
        f._activeGrid.handleKey(key)
        return 0
      end
      return key
    end
    f.OnMouseWheel = function(sender, shift, delta)
      if f._activePopupGrid then
        f._activePopupGrid.scrollBy(delta > 0 and -2 or 2)
        return true
      elseif f._activeGrid then
        f._activeGrid.scrollBy(delta > 0 and -3 or 3)
        return true
      end
    end
  end)
  f.OnClose = function()
    if modalKey and PES10.openModals and PES10.openModals[modalKey] == f then
      PES10.openModals[modalKey] = nil
    end
    return caFree
  end
  local hdrPanel = f
  if createPanel then
    local hh = S(hdrHLog or 58)
    hdrPanel = makeDarkBox(f, 0, 0, W, hh, M_HDR)
    makeDarkBox(hdrPanel, 0, 0, W, S(2), accentCol or M_BLUE)
    makeDarkBox(f, 0, hh, W, 1, M_DIVIDER)
  end
  if modalKey and PES10.openModals then PES10.openModals[modalKey] = f end
  if PES10.dashboardState and PES10.dashboardState.hoverAt then
    pcall(function() PES10.dashboardState.hoverAt(-1, -1) end)
  end
  return f, S, W, H, hdrPanel
end

local function makeModalLabel(parent, x, y, w, h, txt, ptSize, color, bold, alignCenter)
  local lbl = createLabel(parent)
  lbl.Left = x; lbl.Top = y; lbl.Width = w; lbl.Height = h
  lbl.AutoSize = false
  lbl.Caption = txt
  lbl.Font.Name = "Segoe UI"
  lbl.Font.Size = ptSize or 9
  lbl.Font.Color = color or M_TEXT
  pcall(function()
    lbl.Transparent = true
    lbl.Font.Style = bold and "[fsBold]" or "[]"
    if alignCenter then lbl.Alignment = "taCenter"; lbl.Layout = "tlCenter" end
  end)
  return lbl
end

local function makeModalBtn(parent, x, y, w, h, txt, onClick, variant)
  if not createPanel or not createLabel then
    local b = createButton(parent)
    b.Left = x; b.Top = y; b.Width = w; b.Height = h
    b.Caption = txt; b.Font.Name = "Segoe UI"; b.Font.Size = 9; b.OnClick = onClick
    return {
      ctrl = b,
      setCaption = function(s) b.Caption = s end,
      setActive = function() end,
    }
  end
  local v = M_VARIANTS[variant or "default"] or M_VARIANTS.default
  local active = false
  local hovered = false

  local outer = makeDarkBox(parent, x, y, w, h, v.bd)
  local inner = makeDarkBox(outer, 1, 1, math.max(2, w - 2), math.max(2, h - 2), v.bg)
  local lbl = createLabel(inner)
  lbl.Left = 2; lbl.Top = 0; lbl.Width = math.max(2, w - 6); lbl.Height = math.max(2, h - 2)
  lbl.AutoSize = false
  lbl.Caption = txt
  lbl.Font.Name = "Segoe UI"
  lbl.Font.Size = 9
  lbl.Font.Color = v.fg
  pcall(function()
    lbl.Transparent = true
    lbl.Alignment = "taCenter"
    lbl.Layout = "tlCenter"
    lbl.Font.Style = (variant == "primary" or variant == "success" or variant == "gold" or variant == "danger") and "[fsBold]" or "[]"
    outer.Cursor = -21; inner.Cursor = -21; lbl.Cursor = -21
  end)

  local function applyVisual(isDown)
    if active then
      outer.Color = M_BLUE
      inner.Color = isDown and 0x291D0D or 0x3D2B14
      lbl.Font.Color = M_TEXT
    elseif isDown then
      outer.Color = v.bdHov
      inner.Color = v.down
      lbl.Font.Color = v.fgHov
    elseif hovered then
      outer.Color = v.bdHov
      inner.Color = v.hov
      lbl.Font.Color = v.fgHov
    else
      outer.Color = v.bd
      inner.Color = v.bg
      lbl.Font.Color = v.fg
    end
  end

  local function onEnter() hovered = true; applyVisual(false) end
  local function onLeave() hovered = false; applyVisual(false) end
  local function onDown() applyVisual(true) end
  local function onUp() applyVisual(false) end
  local function onClk() if onClick then pcall(onClick) end end

  for _, obj in ipairs({ outer, inner, lbl }) do
    pcall(function()
      obj.OnMouseEnter = onEnter
      obj.OnMouseLeave = onLeave
      obj.OnMouseDown  = onDown
      obj.OnMouseUp    = onUp
      obj.OnClick      = onClk
    end)
  end

  return {
    outer = outer, inner = inner, label = lbl,
    setCaption = function(s) lbl.Caption = s end,
    setActive = function(state)
      active = (state == true)
      pcall(function() lbl.Font.Style = active and "[fsBold]" or "[]" end)
      applyVisual(false)
    end,
  }
end

local function makeDarkEdit(parent, x, y, w, h, initText, onEnter, mono)
  local S = PES10.uiScale()
  if not createPanel then
    local ed = createEdit(parent)
    ed.Left = x; ed.Top = y; ed.Width = w; ed.Height = h; ed.Text = tostring(initText or "")
    return ed
  end
  local outer = makeDarkBox(parent, x, y, w, h, M_BORDER)
  local inner = makeDarkBox(outer, 1, 1, math.max(2, w - 2), math.max(2, h - 2), M_INPUT_BG)
  local ed = createEdit(inner)
  local edH = S(18)
  local padY = math.max(2, math.floor((h - 2 - edH) / 2))
  ed.Left = S(7); ed.Top = padY; ed.Width = math.max(10, w - S(16)); ed.Height = edH
  ed.Text = tostring(initText or "")
  ed.Color = M_INPUT_BG
  ed.Font.Name = mono and "Consolas" or "Segoe UI"
  ed.Font.Size = 9
  ed.Font.Color = M_TEXT
  pcall(function()
    ed.ParentColor = false
    ed.BorderStyle = "bsNone"
    ed.OnEnter = function() outer.Color = M_BLUE; inner.Color = M_INPUT_FOC; ed.Color = M_INPUT_FOC end
    ed.OnExit  = function() outer.Color = M_BORDER; inner.Color = M_INPUT_BG; ed.Color = M_INPUT_BG end
    if onEnter then
      ed.OnKeyDown = function(sender, key)
        if key == 13 then onEnter() end
        return key
      end
    end
  end)
  pcall(function() inner.OnClick = function() pcall(function() ed.setFocus() end) end end)
  return ed
end

-- ============================================================================
-- CUSTOM DOUBLE-BUFFERED DARK STUDIO DATA GRID (ZERO WIN32 LISTBOX / SCROLLBAR)
-- Sortable headers, color OVR/POS badges, graphical progress bars, status pills
-- ============================================================================
local function gridOvrColors(ovr)
  local v = tonumber(ovr) or 0
  if v >= 90 then return 0x24BFFB, 0x170F0B end -- Gold / Midnight
  if v >= 85 then return 0x99D334, 0x170F0B end -- Emerald / Midnight
  if v >= 80 then return 0xF8BD38, 0x170F0B end -- Sky Blue / Midnight
  if v >= 75 then return 0x3D6A8A, 0xFCFAF8 end -- Steel Blue / White
  return 0x3B2E26, 0xC4B2A0                      -- Slate / Muted
end

local function gridPosColors(posIdx)
  local p = tonumber(posIdx)
  if p == 0 then return 0x18293A, 0x24BFFB, 0x80DAFC end                         -- GK Gold
  if p == 1 or p == 2 or p == 3 then return 0x3A2815, 0xF8BD38, 0xFCD97E end     -- DEF Sky
  if p == 4 or p == 5 or p == 6 or p == 7 or p == 8 then return 0x1E331B, 0x99D334, 0xB8F08B end -- MID Emerald
  return 0x281A3B, 0x5E3FF4, 0xA692FA                                            -- ATT Crimson/Violet
end

local function gridTierColor(val)
  local v = tonumber(val) or 0
  if v >= 90 then return 0xF8BD38 end -- Cyan-Sky
  if v >= 80 then return 0x99D334 end -- Emerald
  if v >= 70 then return 0x24BFFB end -- Gold
  if v >= 60 then return 0x3888F5 end -- Warm Orange
  return 0x5E3FF4                     -- Crimson
end

local function createStudioGrid(parent, x, y, w, h, cols, opts)
  opts = opts or {}
  local S = PES10.uiScale()
  local hostForm = opts.hostForm or parent
  local hideHeader = (opts.hideHeader == true)
  local hdrH = hideHeader and 0 or S(opts.headerH or 30)
  local rowH = S(opts.rowH or 30)
  local sbW  = S(10)
  local padX = S(8)

  local rows = {}
  local selIdx = 1
  local hovIdx = nil
  local hovCol = nil
  local scrollTop = 0
  local sortKey = opts.defaultSortKey
  local sortAsc = (opts.defaultSortAsc == true)
  local draggingSb = false
  local dragStartY = 0
  local dragStartScroll = 0

  local outer = makeDarkBox(parent, x, y, w, h, opts.borderColor or M_BORDER)
  local iw, ih = math.max(8, w - 2), math.max(8, h - 2)
  local pb = createPanel(outer)
  pb.Left = 1; pb.Top = 1; pb.Width = iw; pb.Height = ih
  pb.Caption = ""
  pcall(function() pb.BevelOuter = "bvNone"; pb.BevelInner = "bvNone"; pb.BorderStyle = "bsNone"; pb.DoubleBuffered = true; pb.Cursor = -21 end)
  local bmp = createBitmap(iw, ih)
  bmp.Width = iw; bmp.Height = ih

  -- Compute pixel column layout once
  local colRects = {}
  local contentW = iw - sbW
  local fixedSum = 0
  local flexIdx = nil
  for i, col in ipairs(cols) do
    if col.flex then flexIdx = i else fixedSum = fixedSum + S(col.w or 60) end
  end
  local curX = 0
  for i, col in ipairs(cols) do
    local cw = (i == flexIdx) and math.max(S(60), contentW - fixedSum) or S(col.w or 60)
    colRects[i] = { x = curX, w = cw, def = col }
    curX = curX + cw
  end

  local bodyH = math.max(rowH, ih - hdrH)
  local visRows = math.max(1, math.floor(bodyH / rowH))

  local function maxScroll()
    return math.max(0, #rows - visRows)
  end

  local function clampScroll()
    local ms = maxScroll()
    if scrollTop < 0 then scrollTop = 0 end
    if scrollTop > ms then scrollTop = ms end
  end

  local function fitText(c, txt, maxW)
    txt = tostring(txt or "")
    if maxW <= 6 then return "" end
    if c.getTextWidth(txt) <= maxW then return txt end
    local ell = ".."
    local ew = c.getTextWidth(ell)
    local out = txt
    while #out > 1 and (c.getTextWidth(out) + ew) > maxW do
      -- Strip one UTF-8 character from the end
      local i = #out
      while i > 1 and (string.byte(out, i) >= 128 and string.byte(out, i) < 192) do i = i - 1 end
      out = out:sub(1, i - 1)
    end
    return out .. ell
  end

  local function drawCellText(c, cx, cy, cw, ch, bg, txt, fg, pt, bold, align)
    c.Font.Name = "Segoe UI"
    c.Font.Size = pt or 9
    c.Font.Color = fg or M_TEXT
    pcall(function() c.Font.Style = bold and "[fsBold]" or "[]" end)
    c.Brush.Color = bg
    local maxW = math.max(4, cw - padX * 2)
    local s = fitText(c, txt, maxW)
    local tw = c.getTextWidth(s)
    local th = c.getTextHeight(s)
    local ty = cy + math.floor((ch - th) / 2)
    local tx = cx + padX
    if align == "center" then tx = cx + math.floor((cw - tw) / 2)
    elseif align == "right" then tx = cx + cw - padX - tw end
    c.textOut(tx, ty, s)
  end

  local function drawPill(c, px, py, pw, ph, bg, bd, txt, fg, pt, bold)
    c.Brush.Color = bg
    c.Pen.Color = bd or bg
    c.roundRect(px, py, px + pw, py + ph, S(6), S(6))
    c.Font.Name = "Segoe UI"
    c.Font.Size = pt or 8
    c.Font.Color = fg or M_TEXT
    pcall(function() c.Font.Style = bold and "[fsBold]" or "[]" end)
    local tw = c.getTextWidth(txt)
    local th = c.getTextHeight(txt)
    c.textOut(px + math.floor((pw - tw) / 2), py + math.floor((ph - th) / 2), txt)
  end

  local function getSbThumb()
    local total = #rows
    if total <= visRows then return nil end
    local trackY = hdrH + S(2)
    local trackH = math.max(S(20), ih - hdrH - S(4))
    local thumbH = math.max(S(26), math.floor(trackH * (visRows / total)))
    local ms = maxScroll()
    local ratio = (ms > 0) and (scrollTop / ms) or 0
    local thumbY = trackY + math.floor((trackH - thumbH) * ratio)
    return trackY, trackH, thumbY, thumbH
  end

  local function render()
    clampScroll()
    local c = bmp.Canvas
    -- Background
    c.Brush.Color = M_SURFACE
    c.Pen.Color = M_SURFACE
    c.fillRect(0, 0, iw, ih)

    -- 1. Column Header Strip
    if not hideHeader then
      c.Brush.Color = 0x20150F
      c.Pen.Color = 0x20150F
      c.fillRect(0, 0, iw, hdrH)
      c.Brush.Color = M_BORDER
      c.fillRect(0, hdrH - 1, iw, hdrH)
      for i, cr in ipairs(colRects) do
        local def = cr.def
        local isSorted = (sortKey ~= nil and def.sortKey == sortKey)
        local isHov = (hovCol == i and def.sortKey ~= nil)
        local lbl = def.title or ""
        if isSorted then lbl = lbl .. (sortAsc and " ▲" or " ▼") end
        local colFg = (isSorted or isHov) and M_BLUE or M_SUBTEXT
        drawCellText(c, cr.x, 0, cr.w, hdrH - 1, 0x20150F, lbl, colFg, 8, true, def.align or "left")
      end
    end

    -- 2. Rows
    if #rows == 0 then
      local emptyTxt = opts.emptyText or PES10.T("Nenhum item encontrado.", "No items found.")
      drawCellText(c, 0, hdrH, contentW, bodyH, M_SURFACE, emptyTxt, M_SUBTEXT, 9, false, "center")
    else
      local firstIdx = scrollTop + 1
      local lastIdx = math.min(#rows, firstIdx + visRows)
      for rIdx = firstIdx, lastIdx do
        local row = rows[rIdx]
        local ry = hdrH + (rIdx - firstIdx) * rowH
        if ry < ih then
          local isSel = (rIdx == selIdx)
          local isHov = (rIdx == hovIdx)
          local rowBg = isSel and 0x422B16 or (isHov and 0x35251A or ((rIdx % 2 == 0) and 0x281B13 or M_SURFACE))
          c.Brush.Color = rowBg
          c.Pen.Color = rowBg
          c.fillRect(0, ry, contentW, math.min(ih, ry + rowH))
          -- Subtle horizontal row divider
          c.Brush.Color = 0x2E2017
          c.fillRect(0, math.min(ih, ry + rowH) - 1, contentW, math.min(ih, ry + rowH))
          -- Crisp left accent bar on selected row
          if isSel then
            c.Brush.Color = M_BLUE
            c.fillRect(0, ry + S(2), S(3), math.min(ih, ry + rowH) - S(2))
          end

          for _, cr in ipairs(colRects) do
            local def = cr.def
            local val = row[def.key]
            local kind = def.kind or "text"

            if kind == "ovr" then
              local bg, fg = gridOvrColors(val)
              local pw, ph = S(34), S(20)
              local px = cr.x + math.floor((cr.w - pw) / 2)
              local py = ry + math.floor((rowH - ph) / 2)
              drawPill(c, px, py, pw, ph, bg, bg, tostring(val or "--"), fg, 8, true)

            elseif kind == "pos" then
              local posCode = row[def.posIdxKey or "pos"]
              local bg, bd, fg = gridPosColors(posCode)
              local pw, ph = S(38), S(20)
              local px = cr.x + math.floor((cr.w - pw) / 2)
              local py = ry + math.floor((rowH - ph) / 2)
              drawPill(c, px, py, pw, ph, bg, bd, tostring(val or "-"), fg, 7, true)

            elseif kind == "bar" then
              local num = tonumber(val) or 0
              local minV = def.minVal or 0
              local maxV = def.maxVal or 100
              local pct = math.max(0, math.min(1, (num - minV) / math.max(1, maxV - minV)))
              local lblTxt = def.suffix and (tostring(num) .. def.suffix) or tostring(num)
              local lblW = S(def.lblW or 38)
              local barX = cr.x + padX
              local barW = math.max(S(24), cr.w - padX * 2 - lblW - S(6))
              local barH = S(8)
              local barY = ry + math.floor((rowH - barH) / 2)
              c.Brush.Color = 0x1A110C
              c.Pen.Color = 0x38281E
              c.roundRect(barX, barY, barX + barW, barY + barH, S(4), S(4))
              local fillW = math.floor(barW * pct + 0.5)
              if fillW >= S(4) then
                local fc = gridTierColor(num)
                c.Brush.Color = fc
                c.Pen.Color = fc
                c.roundRect(barX, barY, barX + fillW, barY + barH, S(4), S(4))
              end
              drawCellText(c, barX + barW + S(4), ry, lblW + padX, rowH, rowBg, lblTxt, gridTierColor(num), 8, true, "right")

            elseif kind == "pill" and type(val) == "table" then
              local ptxt = tostring(val.txt or "")
              if ptxt ~= "" then
                local pw = math.min(cr.w - S(8), math.max(S(38), S(val.w or 54)))
                local ph = S(20)
                local px = cr.x + math.floor((cr.w - pw) / 2)
                local py = ry + math.floor((rowH - ph) / 2)
                drawPill(c, px, py, pw, ph, val.bg or M_BTN_BG, val.bd or M_BORDER, ptxt, val.fg or M_TEXT, 7, true)
              end

            else
              local fg = (kind == "muted") and M_SUBTEXT or (def.color or (isSel and M_TEXT or 0xEAE4DF))
              if type(def.colorFn) == "function" then fg = def.colorFn(row, val) or fg end
              drawCellText(c, cr.x, ry, cr.w, rowH, rowBg, tostring(val or ""), fg, def.pt or 9, def.bold or isSel, def.align or "left")
            end
          end
        end
      end
    end

    -- 3. Custom Dark Scrollbar Track & Thumb
    local sbX = contentW
    c.Brush.Color = 0x1B120C
    c.Pen.Color = 0x1B120C
    c.fillRect(sbX, hdrH, iw, ih)
    local trackY, trackH, thumbY, thumbH = getSbThumb()
    if thumbY then
      local tc = draggingSb and M_BLUE or 0x564133
      c.Brush.Color = tc
      c.Pen.Color = tc
      c.roundRect(sbX + S(2), thumbY, iw - S(2), thumbY + thumbH, S(4), S(4))
    end

    pb.repaint()
  end

  pb.OnPaint = function()
    pb.Canvas.draw(0, 0, bmp)
  end

  local api = {}

  local lastClickInBody = false
  local function applySort()
    if not sortKey or #rows <= 1 then return end
    for i, r in ipairs(rows) do r._sortOrd = i end
    local allNum = true
    for _, r in ipairs(rows) do
      local v = r[sortKey]
      if v ~= nil and type(v) ~= "number" then allNum = false; break end
    end
    table.sort(rows, function(a, b)
      local va, vb = a[sortKey], b[sortKey]
      if allNum then
        local na, nb = tonumber(va) or 0, tonumber(vb) or 0
        if na ~= nb then return sortAsc and (na < nb) or (na > nb) end
      else
        local sa, sb = tostring(va or ""):lower(), tostring(vb or ""):lower()
        if sa ~= sb then return sortAsc and (sa < sb) or (sa > sb) end
      end
      return (a._sortOrd or 0) < (b._sortOrd or 0)
    end)
  end

  local function rowAtY(my)
    if my < hdrH then return nil end
    local idx = scrollTop + math.floor((my - hdrH) / rowH) + 1
    if idx >= 1 and idx <= #rows then return idx end
    return nil
  end

  pb.OnMouseDown = function(sender, button, mx, my)
    lastClickInBody = false
    -- Close any open dropdown on hostForm when interacting with grid
    if not opts.isPopup and hostForm and hostForm._activeComboClose then
      pcall(hostForm._activeComboClose)
    end
    -- Scrollbar interaction
    if mx >= contentW and my >= hdrH then
      local trackY, trackH, thumbY, thumbH = getSbThumb()
      if thumbY then
        if my >= thumbY and my <= (thumbY + thumbH) then
          draggingSb = true
          dragStartY = my
          dragStartScroll = scrollTop
          render()
        else
          local ratio = math.max(0, math.min(1, (my - trackY - thumbH / 2) / math.max(1, trackH - thumbH)))
          scrollTop = math.floor(ratio * maxScroll() + 0.5)
          render()
        end
      end
      return
    end
    -- Column header click -> sort!
    if not hideHeader and my < hdrH then
      for _, cr in ipairs(colRects) do
        if mx >= cr.x and mx < (cr.x + cr.w) and cr.def.sortKey then
          if sortKey == cr.def.sortKey then sortAsc = not sortAsc
          else sortKey = cr.def.sortKey; sortAsc = (cr.def.defaultAsc == true) end
          applySort()
          selIdx = (#rows > 0) and 1 or nil
          scrollTop = 0
          render()
          if api.onSelect and selIdx then pcall(api.onSelect, rows[selIdx], selIdx) end
          return
        end
      end
      return
    end
    -- Row click
    local rIdx = rowAtY(my)
    if rIdx then
      lastClickInBody = true
      selIdx = rIdx
      render()
      if api.onSelect then pcall(api.onSelect, rows[selIdx], selIdx) end
      if opts.singleClickAction and api.onDblClick then pcall(api.onDblClick, rows[selIdx], selIdx) end
    end
  end

  pb.OnMouseMove = function(sender, mx, my)
    if draggingSb then
      local trackY, trackH, _, thumbH = getSbThumb()
      if trackH then
        local dy = my - dragStartY
        local ms = maxScroll()
        local avail = math.max(1, trackH - thumbH)
        scrollTop = math.max(0, math.min(ms, math.floor(dragStartScroll + (dy / avail) * ms + 0.5)))
        render()
      end
      return
    end
    local newHovCol = nil
    local newHovIdx = nil
    if not hideHeader and my < hdrH and mx < contentW then
      for i, cr in ipairs(colRects) do
        if mx >= cr.x and mx < (cr.x + cr.w) then newHovCol = i; break end
      end
    elseif mx < contentW then
      newHovIdx = rowAtY(my)
    end
    if newHovCol ~= hovCol or newHovIdx ~= hovIdx then
      hovCol = newHovCol
      hovIdx = newHovIdx
      render()
    end
  end

  pb.OnMouseUp = function()
    if draggingSb then draggingSb = false; render() end
  end

  pb.OnMouseLeave = function()
    if hovIdx ~= nil or hovCol ~= nil then hovIdx = nil; hovCol = nil; render() end
  end

  pb.OnDblClick = function()
    if lastClickInBody and selIdx and rows[selIdx] and api.onDblClick then
      pcall(api.onDblClick, rows[selIdx], selIdx)
    end
  end

  pcall(function()
    pb.OnMouseWheel = function(sender, shift, delta)
      scrollTop = scrollTop + (delta > 0 and -3 or 3)
      render()
      return true
    end
  end)

  api.outer = outer
  api.pb = pb
  api.bmp = bmp
  api.render = render
  api.scrollBy = function(d) scrollTop = scrollTop + d; render() end
  api.handleKey = function(key)
    if #rows == 0 then return end
    local cur = selIdx or 1
    if key == 38 then cur = math.max(1, cur - 1)
    elseif key == 40 then cur = math.min(#rows, cur + 1)
    elseif key == 33 then cur = math.max(1, cur - visRows)
    elseif key == 34 then cur = math.min(#rows, cur + visRows) end
    api.setSelectedIndex(cur, true)
    if api.onSelect then pcall(api.onSelect, rows[cur], cur) end
  end
  api.setRows = function(newRows, keepSel)
    rows = newRows or {}
    if sortKey then applySort() end
    if #rows == 0 then selIdx = nil; scrollTop = 0
    else
      if keepSel and keepSel >= 1 and keepSel <= #rows then selIdx = keepSel
      elseif selIdx and selIdx >= 1 and selIdx <= #rows then -- keep current
      else selIdx = 1 end
      clampScroll()
    end
    render()
  end
  api.getSelectedIndex = function() return selIdx end
  api.getSelectedRow = function() return (selIdx and rows[selIdx]) or nil end
  api.setSelectedIndex = function(idx, ensureVis)
    if not idx or idx < 1 or idx > #rows then return end
    selIdx = idx
    if ensureVis then
      if selIdx - 1 < scrollTop then scrollTop = selIdx - 1
      elseif selIdx > scrollTop + visRows then scrollTop = selIdx - visRows end
    end
    render()
  end

  if not opts.isPopup and hostForm then hostForm._activeGrid = api end
  render()
  return api
end

local function makeDarkFooter(parent, W, H, barH)
  local S = PES10.uiScale()
  if not createPanel then return parent, H - barH + S(12) end
  local ftr = makeDarkBox(parent, 0, H - barH, W, barH, M_HDR)
  makeDarkBox(ftr, 0, 0, W, 1, M_DIVIDER)
  return ftr, math.floor((barH - S(34)) / 2)
end

-- Zero-White Custom Dark Dropdown (panel + floating dark Canvas Grid popup)
local function makeDarkCombo(parent, x, y, w, h, itemsList, initIdx, onChange, popupHost)
  local S = PES10.uiScale()
  local host = popupHost or parent
  local curIdx = math.max(0, math.min(#itemsList - 1, initIdx or 0))
  local popup = nil

  local outer = makeDarkBox(parent, x, y, w, h, M_BORDER)
  local inner = makeDarkBox(outer, 1, 1, math.max(2, w - 2), math.max(2, h - 2), M_INPUT_BG)
  local lbl = createLabel(inner)
  lbl.Left = S(8); lbl.Top = 0; lbl.Width = math.max(10, w - S(28)); lbl.Height = math.max(2, h - 2)
  lbl.AutoSize = false; lbl.Font.Name = "Segoe UI"; lbl.Font.Size = 9; lbl.Font.Color = M_TEXT
  lbl.Caption = itemsList[curIdx + 1] or ""
  pcall(function() lbl.Transparent = true; lbl.Layout = "tlCenter"; lbl.Cursor = -21 end)

  local chev = createLabel(inner)
  chev.Left = math.max(2, w - S(22)); chev.Top = 0; chev.Width = S(18); chev.Height = math.max(2, h - 2)
  chev.AutoSize = false; chev.Font.Name = "Segoe UI"; chev.Font.Size = 9; chev.Font.Color = M_BLUE
  chev.Caption = "▾"
  pcall(function() chev.Transparent = true; chev.Alignment = "taCenter"; chev.Layout = "tlCenter"; chev.Cursor = -21; outer.Cursor = -21; inner.Cursor = -21 end)

  local function closePopup()
    if popup then
      pcall(function() popup.outer.Visible = false; popup.outer.destroy() end)
      popup = nil
      outer.Color = M_BORDER
      if host._activeComboClose == closePopup then host._activeComboClose = nil end
      host._activePopupGrid = nil
    end
  end

  local function setIndex(idx, fireCb)
    idx = math.max(0, math.min(#itemsList - 1, idx or 0))
    curIdx = idx
    lbl.Caption = itemsList[curIdx + 1] or ""
    if fireCb and onChange then pcall(onChange, curIdx) end
  end

  local function togglePopup()
    if popup then closePopup(); return end
    if host._activeComboClose and host._activeComboClose ~= closePopup then
      pcall(host._activeComboClose)
    end
    host._activeComboClose = closePopup
    outer.Color = M_BLUE
    local popH = math.min(#itemsList * S(25) + S(6), S(204))
    local formH = host.ClientHeight or host.Height or S(500)
    local popY = y + h + S(2)
    if popY + popH > formH - S(8) then popY = math.max(S(6), y - popH - S(2)) end
    local popGrid = createStudioGrid(host, x, popY, w, popH, {
      { key = "txt", title = "", flex = true, pt = 9 },
    }, { hideHeader = true, rowH = 24, borderColor = M_BLUE, isPopup = true, singleClickAction = true, hostForm = host })
    local pRows = {}
    for idx, it in ipairs(itemsList) do pRows[idx] = { idx = idx - 1, txt = it } end
    popGrid.setRows(pRows, curIdx + 1)
    popGrid.setSelectedIndex(curIdx + 1, true)
    popGrid.onDblClick = function(row)
      if row then setIndex(row.idx, true) end
      closePopup()
    end
    pcall(function() popGrid.outer.bringToFront() end)
    popup = popGrid
    host._activePopupGrid = popGrid
  end

  for _, obj in ipairs({ outer, inner, lbl, chev }) do
    pcall(function()
      obj.OnClick = togglePopup
      obj.OnMouseEnter = function() if not popup then outer.Color = M_BLUE end end
      obj.OnMouseLeave = function() if not popup then outer.Color = M_BORDER end end
    end)
  end

  return {
    getIndex = function() return curIdx end,
    setIndex = function(i) setIndex(i, false) end,
    closePopup = closePopup,
  }
end

-- Unified Dark Prompt Modal (replaces blinding white Windows inputQuery dialogs)
function PES10.openDarkPrompt(title, subtitle, defaultVal, accentCol, onConfirm)
  if not createForm or not createEdit or not createPanel then
    local ans = inputQuery(title, subtitle, tostring(defaultVal or ""))
    if ans and onConfirm then onConfirm(ans) end
    return
  end
  local f, S, W, H, hdrBar = makeStudioModal("darkprompt", title, 440, 198, accentCol or M_BLUE, 56)
  makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(20), title, 10, M_TEXT, true)
  makeModalLabel(hdrBar, S(16), S(32), W - S(32), S(18), subtitle, 8, M_SUBTEXT, false)

  local ed = nil
  local function submit()
    local val = ed and ed.Text or ""
    f.close()
    if onConfirm then pcall(onConfirm, val) end
  end
  ed = makeDarkEdit(f, S(16), S(74), W - S(32), S(34), tostring(defaultVal or ""), submit, false)

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, W - S(226), yF, S(102), S(34), PES10.T("Cancelar", "Cancel"), function() f.close() end, "subtle")
  makeModalBtn(ftr, W - S(116), yF, S(100), S(34), PES10.T("Aplicar", "Apply"), submit, "primary")

  f.show()
  pcall(function() ed.setFocus(); ed.selectAll() end)
end

-- Unified Dark X-Ray Modal (Double-Buffered Canvas Grid, zero white dialogs)
function PES10.openDarkTextModal(modalKey, title, subtitle, linesTbl, accentCol, extraBtnTxt, extraBtnFn)
  if not createForm or not createBitmap or not createPanel then
    showMessage(table.concat(linesTbl, "\n"))
    return
  end
  local f, S, W, H, hdrBar = makeStudioModal(modalKey or "xray", title, 640, 520, accentCol or M_PURPLE, 58)
  makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(22), title, 11, M_TEXT, true)
  makeModalLabel(hdrBar, S(16), S(34), W - S(32), S(18), subtitle, 8, M_SUBTEXT, false)

  local grid = createStudioGrid(f, S(16), S(70), W - S(32), H - S(142), {
    { key = "ln", title = PES10.T("RELATÓRIO TÁTICO & DNA", "TACTICAL & DNA REPORT"), flex = true, pt = 9 },
  }, { rowH = 24, hostForm = f })
  local gRows = {}
  for _, ln in ipairs(linesTbl) do
    if ln ~= "" then gRows[#gRows + 1] = { ln = ln } end
  end
  grid.setRows(gRows, 1)

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  if extraBtnTxt and extraBtnFn then
    makeModalBtn(ftr, S(16), yF, S(200), S(34), extraBtnTxt, function() f.close(); extraBtnFn() end, "primary")
  end
  makeModalBtn(ftr, W - S(116), yF, S(100), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")
  f.show()
end

-- MODAL 1: VISUAL POSITION LAB (DOUBLE-BUFFERED DATA GRID + ISOLATED OVR ENGINE)
function PES10.openPositionLabModal()
  if not createBitmap or not createPanel then return PES10.promptTargetPositionOVRPlusOne() end
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local f, S, W, H, hdrBar = makeStudioModal("poslab", PES10.T("Laboratório de Posições - ", "Position Lab - ") .. pName, 700, 584, M_GREEN, 58)

  local hdr = makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(22), "", 11, M_TEXT, true)
  local sub = makeModalLabel(hdrBar, S(16), S(34), W - S(32), S(18),
    PES10.T("OVR já inclui o bônus do Grau. A posição principal nunca é alterada ao editar outra.",
            "OVR includes the Grade bonus. The primary position is never altered when editing another."),
    8, M_SUBTEXT, false)

  local grid = createStudioGrid(f, S(16), S(70), W - S(32), S(300), {
    { key = "posShort", title = PES10.T("POS", "POS"),          w = 58,  align = "center", kind = "pos", posIdxKey = "pIdx" },
    { key = "posName",  title = PES10.T("POSIÇÃO", "POSITION"), flex = true, sortKey = "posName" },
    { key = "ovr",      title = "OVR",                          w = 64,  align = "center", kind = "ovr", sortKey = "ovr" },
    { key = "gradePill",title = PES10.T("GRAU", "GRADE"),       w = 66,  align = "center", kind = "pill", sortKey = "apt" },
    { key = "proj",     title = PES10.T("PROJEÇÃO", "PROJECTION"), w = 138, align = "left", kind = "muted" },
    { key = "statusPill", title = PES10.T("STATUS", "STATUS"),  w = 96,  align = "center", kind = "pill" },
  }, { rowH = 29, hostForm = f })

  local lblIso = makeModalLabel(f, S(16), S(378), W - S(32), S(18), "", 8, M_BLUE, true)
  local lblLog = makeModalLabel(f, S(16), S(398), W - S(32), S(18),
    PES10.T("Duplo-clique = +1 OVR isolado. Use Grau A/B/C abaixo para mudar o bônus da posição.",
            "Double-click = +1 isolated OVR. Use Grade A/B/C below to change the position bonus."),
    8, M_GREEN, false)

  local posOrder = {12, 11, 9, 10, 8, 6, 7, 5, 4, 2, 3, 1, 0}
  local function getPickedPos()
    local r = grid.getSelectedRow()
    if not r then return nil end
    return r.pIdx, grid.getSelectedIndex()
  end

  local edTgt = nil
  local function updateIsoPreview()
    local tp = getPickedPos()
    if tp == nil then return end
    local _, regPos = PES10.calcPlayerOVR(p)
    if edTgt then
      local b = readBytes(p, 0x28, true)
      if b and #b >= 0x28 then
        edTgt.Text = tostring(PES10.calcPlayerOVRFromBuf(b, tp, false))
      end
    end
    if tp == regPos then
      lblIso.Caption = PES10.T(
        "Posição Principal: ajustes usam o motor proporcional DNA completo.",
        "Primary Position: edits use the full proportional DNA solver."
      )
      return
    end
    local prof = PES10.getPositionIsolationProfile(regPos, tp)
    local top = {}
    for i = 1, math.min(4, #prof) do
      local it = prof[i]
      top[#top + 1] = string.format("%s (%d vs %d)", it.name, it.wT, it.wP)
    end
    lblIso.Caption = string.format(
      PES10.T("Atributos isolados (%s vs %s): %s", "Isolated stats (%s vs %s): %s"),
      PES10.posShort(tp), PES10.posShort(regPos), table.concat(top, "  •  ")
    )
  end

  local function refreshList(keepIdx, logText)
    local b = readBytes(p, 0x28, true)
    if not b or #b < 0x28 then return end
    local regOVR, regPos = PES10.calcPlayerOVRFromBuf(b)
    local undoN = PES10.canUndoPlayer(p)
    hdr.Caption = string.format(
      PES10.T("%s  •  Principal: %d %s  •  Undo: %d", "%s  •  Primary: %d %s  •  Undo: %d"),
      pName, regOVR, PES10.posShort(regPos), undoN
    )
    local gRows = {}
    for _, pIdx in ipairs(posOrder) do
      local info = PES10.POS_NAMES[pIdx]
      local pShort = PES10.posShort(pIdx)
      local apt = (pIdx == regPos) and 2 or bufGetPosAptitude(b, pIdx)
      local curO = PES10.calcPlayerOVRFromBuf(b, pIdx, false)
      local potO = PES10.calcPlayerOVRFromBuf(b, pIdx, true)
      local gPill = (apt == 2) and { txt = "A", bg = 0x1E331B, bd = M_GREEN, fg = M_GREEN, w = 36 }
                 or (apt == 1) and { txt = "B", bg = 0x18293A, bd = M_GOLD,  fg = M_GOLD,  w = 36 }
                 or                { txt = "C", bg = 0x2B1E16, bd = M_BORDER, fg = M_SUBTEXT, w = 36 }
      local sPill = (pIdx == regPos) and { txt = PES10.T("PRINCIPAL", "PRIMARY"), bg = 0x3D2B14, bd = M_BLUE, fg = M_BLUE, w = 78 } or { txt = "" }
      local projTxt = (apt < 2) and string.format("c/ A: %d -> %d", potO, math.min(99, potO + 1)) or string.format("+1 -> %d OVR", math.min(99, potO + 1))
      gRows[#gRows + 1] = {
        pIdx = pIdx, posShort = pShort, posName = PES10.T(info.pt, info.en),
        ovr = curO, apt = apt, gradePill = gPill, proj = projTxt, statusPill = sPill,
      }
    end
    grid.setRows(gRows, keepIdx or grid.getSelectedIndex() or 1)
    if logText and logText ~= "" then lblLog.Caption = logText end
    updateIsoPreview()
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  local function doBoostOne()
    local tp, idx = getPickedPos()
    if tp ~= nil then
      PES10.stepTargetPositionOVR(tp, 1)
      refreshList(idx, PES10.lastToastMsg)
    end
  end
  grid.onSelect = updateIsoPreview
  grid.onDblClick = doBoostOne

  local y1, y2 = S(426), S(470)
  makeModalBtn(f, S(16),  y1, S(78),  S(34), "-1 OVR", function()
    local tp, idx = getPickedPos()
    if tp ~= nil then PES10.stepTargetPositionOVR(tp, -1); refreshList(idx, PES10.lastToastMsg) end
  end, "default")
  makeModalBtn(f, S(100), y1, S(78),  S(34), "+1 OVR", doBoostOne, "success")

  local function applyExact()
    local tp, idx = getPickedPos()
    local val = tonumber(edTgt and edTgt.Text or "")
    if tp ~= nil and val then
      PES10.setTargetPositionExactOVR(tp, val)
      refreshList(idx, PES10.lastToastMsg)
    end
  end
  edTgt = makeDarkEdit(f, S(186), y1 + S(2), S(56), S(30), "85", applyExact, true)
  makeModalBtn(f, S(248), y1, S(94),  S(34), PES10.T("Definir OVR", "Set OVR"), applyExact, "primary")

  makeModalBtn(f, S(348), y1, S(164), S(34), PES10.T("Igualar Principal", "Match Primary"), function()
    local tp, idx = getPickedPos()
    if tp ~= nil then PES10.matchSecondaryToPrimaryOVR(p, tp); refreshList(idx, PES10.lastToastMsg) end
  end, "gold")
  makeModalBtn(f, S(518), y1, S(166), S(34), PES10.T("Tornar Principal", "Set as Primary"), function()
    local tp, idx = getPickedPos()
    if tp ~= nil and PES10.setRegisteredPosition(p, tp) then
      local newOvr = PES10.calcPlayerOVR(p)
      PES10.toast(string.format(PES10.T("%s: Principal -> %s (%d OVR)", "%s: Primary -> %s (%d OVR)"), pName, PES10.posShort(tp), newOvr))
      refreshList(idx, PES10.lastToastMsg)
    end
  end, "default")

  makeModalBtn(f, S(16),  y2, S(108), S(34), PES10.T("Grau A", "Grade A"), function()
    local tp, idx = getPickedPos()
    if tp ~= nil then PES10.setPosAptitude(p, tp, 2); PES10.toast(pName .. ": " .. PES10.posShort(tp) .. " -> A"); refreshList(idx, PES10.lastToastMsg) end
  end, "default")
  makeModalBtn(f, S(130), y2, S(108), S(34), PES10.T("Grau B", "Grade B"), function()
    local tp, idx = getPickedPos()
    if tp ~= nil then PES10.setPosAptitude(p, tp, 1); PES10.toast(pName .. ": " .. PES10.posShort(tp) .. " -> B"); refreshList(idx, PES10.lastToastMsg) end
  end, "default")
  makeModalBtn(f, S(244), y2, S(108), S(34), PES10.T("Grau C", "Grade C"), function()
    local tp, idx = getPickedPos()
    if tp ~= nil then PES10.setPosAptitude(p, tp, 0); PES10.toast(pName .. ": " .. PES10.posShort(tp) .. " -> C"); refreshList(idx, PES10.lastToastMsg) end
  end, "default")
  makeModalBtn(f, S(358), y2, S(168), S(34), PES10.T("Promover B -> A", "Promote B -> A"), function()
    local n = PES10.promoteSecondaryPositionsToA(p)
    PES10.toast(string.format(PES10.T("%d secundária(s) -> A", "%d secondary -> A"), n))
    refreshList(grid.getSelectedIndex(), PES10.lastToastMsg)
  end, "default")
  makeModalBtn(f, S(532), y2, S(152), S(34), PES10.T("Todas 13 em A", "All 13 to A"), function()
    PES10.runPlayerMacro("all_positions_a")
    refreshList(grid.getSelectedIndex(), PES10.lastToastMsg)
  end, "default")

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, S(16), yF, S(160), S(34), PES10.T("Desfazer (Undo)", "Undo Edit"), function()
    PES10.undoLastPlayerEdit(p)
    refreshList(grid.getSelectedIndex(), PES10.lastToastMsg)
  end, "gold")
  makeModalBtn(ftr, W - S(126), yF, S(110), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")

  f._onPlayerChange = function(newP, newName)
    p = newP; pName = newName
    f.Caption = PES10.T("Laboratório de Posições - ", "Position Lab - ") .. pName
    refreshList(grid.getSelectedIndex())
  end
  refreshList(1)
  f.show()
end

-- MODAL 2: VISUAL TEAM ROLE SELECTOR (DARK DATA GRID + LEVEL 1-3 + QUICK PRESETS)
function PES10.openTeamRoleModal()
  if not createBitmap or not createPanel then return PES10.promptSetTeamRole() end
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local f, S, W, H, hdrBar = makeStudioModal("teamrole", PES10.T("Função de Equipe - ", "Team Role - ") .. pName, 580, 514, M_GOLD, 58)

  local hdr = makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(22), "", 11, M_TEXT, true)
  makeModalLabel(hdrBar, S(16), S(34), W - S(32), S(18),
    PES10.T("Escolha qualquer uma das 22 Funções + Nível (1-3) ou clique em um Preset:",
            "Pick any of the 22 Team Roles + Level (1-3) or click a Quick Preset:"),
    8, M_SUBTEXT, false)

  local grid = createStudioGrid(f, S(16), S(70), W - S(32), S(264), {
    { key = "id",     title = "ID",                          w = 48,  align = "center", kind = "muted" },
    { key = "name",   title = PES10.T("FUNÇÃO DE EQUIPE", "TEAM ROLE"), flex = true },
    { key = "status", title = PES10.T("STATUS", "STATUS"),   w = 96,  align = "center", kind = "pill" },
  }, { rowH = 28, hostForm = f })

  local lvlItems = { PES10.T("Nível 1", "Level 1"), PES10.T("Nível 2", "Level 2"), PES10.T("Nível 3 (Máx)", "Level 3 (Max)") }
  local cbLvl = makeDarkCombo(f, S(16), S(346), S(156), S(34), lvlItems, 2)

  local function refreshRole()
    local cr, cl = PES10.getTeamRole(p)
    local rn = PES10.TEAM_ROLES[cr] and PES10.T(PES10.TEAM_ROLES[cr].pt, PES10.TEAM_ROLES[cr].en) or "-"
    hdr.Caption = string.format(PES10.T("%s  •  Atual: %s (Nv %d)", "%s  •  Current: %s (Lv %d)"), pName, rn, cl)
    local gRows = {}
    for r = 0, 22 do
      local info = PES10.TEAM_ROLES[r]
      local isCur = (r == cr)
      gRows[#gRows + 1] = {
        id = r,
        name = info and PES10.T(info.pt, info.en) or "-",
        status = isCur and { txt = PES10.T("ATUAL", "ACTIVE"), bg = 0x18293A, bd = M_GOLD, fg = M_GOLD, w = 68 } or { txt = "" },
      }
    end
    grid.setRows(gRows, cr + 1)
    grid.setSelectedIndex(cr + 1, true)
    cbLvl.setIndex(math.max(0, math.min(2, cl - 1)))
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  local function applyRole(rNum, lNum)
    PES10.setTeamRole(p, rNum, lNum)
    local rInfo = PES10.TEAM_ROLES[rNum]
    local rTxt = rInfo and PES10.T(rInfo.pt, rInfo.en) or tostring(rNum)
    PES10.toast(string.format(PES10.T("%s: %s (Nv %d)", "%s: %s (Lv %d)"), pName, rTxt, lNum))
    refreshRole()
  end
  grid.onDblClick = function(row)
    if row then applyRole(row.id, cbLvl.getIndex() + 1) end
  end

  makeModalBtn(f, S(180), S(346), S(220), S(34), PES10.T("Aplicar Função Selecionada", "Apply Selected Role"), function()
    local row = grid.getSelectedRow()
    if row then applyRole(row.id, cbLvl.getIndex() + 1) end
  end, "primary")

  local yP = S(406)
  makeModalLabel(f, S(16), yP - S(18), W - S(32), S(16), PES10.T("PRESETS NÍVEL 3:", "LEVEL 3 PRESETS:"), 8, M_GOLD, true)
  makeModalBtn(f, S(16),  yP, S(104), S(34), PES10.T("Lenda", "Legend"),           function() applyRole(19, 3) end, "default")
  makeModalBtn(f, S(126), yP, S(104), S(34), PES10.T("Bandeira", "Bandiera"),      function() applyRole(18, 3) end, "default")
  makeModalBtn(f, S(236), yP, S(104), S(34), PES10.T("Maestro", "Maestro"),        function() applyRole(9, 3) end, "default")
  makeModalBtn(f, S(346), yP, S(104), S(34), PES10.T("General", "General"),        function() applyRole(7, 3) end, "default")
  makeModalBtn(f, S(456), yP, S(108), S(34), PES10.T("Superestrela", "Superstar"), function() applyRole(14, 3) end, "default")

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, S(16), yF, S(140), S(34), PES10.T("Desfazer (Undo)", "Undo Edit"), function()
    PES10.undoLastPlayerEdit(p); refreshRole()
  end, "gold")
  makeModalBtn(ftr, W - S(126), yF, S(110), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")

  f._onPlayerChange = function(newP, newName)
    p = newP; pName = newName
    f.Caption = PES10.T("Função de Equipe - ", "Team Role - ") .. pName
    refreshRole()
  end
  refreshRole()
  f.show()
end

-- MODAL 3: VISUAL REAL-RAM SCOUT OBSERVATION HUB (DARK DATA GRID)
function PES10.openScoutListsModal()
  if not createBitmap or not createPanel then return PES10.promptAddPlayerToScoutList() end
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    PES10.toast(PES10.T("Carregue um save de Master League primeiro!", "Load a Master League save first!"))
    return
  end

  local f, S, W, H, hdrBar = makeStudioModal("scouthub", PES10.T("Olheiros & Favoritos (Master League)", "Scouts & Shortlist (Master League)"), 720, 536, M_GREEN, 58)
  local hdr = makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(22), "", 11, M_TEXT, true)
  local sub = makeModalLabel(hdrBar, S(16), S(34), W - S(32), S(18), "", 8, M_SUBTEXT, false)

  local curSlot = 1
  local grid = createStudioGrid(f, S(16), S(112), W - S(32), S(290), {
    { key = "num",      title = "#",                         w = 38,  align = "center", kind = "muted" },
    { key = "ovr",      title = "OVR",                       w = 56,  align = "center", kind = "ovr", sortKey = "ovr" },
    { key = "posShort", title = "POS",                       w = 56,  align = "center", kind = "pos", posIdxKey = "pos" },
    { key = "name",     title = PES10.T("ATLETA", "PLAYER"), w = 210, sortKey = "name" },
    { key = "ageStr",   title = PES10.T("IDADE", "AGE"),     w = 62,  align = "center", kind = "muted", sortKey = "age" },
    { key = "club",     title = PES10.T("CLUBE", "CLUB"),    flex = true, kind = "muted", sortKey = "club" },
  }, { rowH = 29, emptyText = PES10.T("Lista vazia. Selecione um atleta no jogo ou clique em 'Buscar Jogador'.", "List empty. Highlight a player in-game or click 'Find Player'."), hostForm = f })

  local tabBtns = {}
  local function refreshHub()
    local p = PES10.readU64(PES10.symAddr("ptrPlayer"))
    if p ~= 0 then
      local nm = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
      local ovr, rp = PES10.calcPlayerOVR(p)
      hdr.Caption = string.format(PES10.T("Olheiros & Favoritos  •  Atleta: %s (%d %s)", "Scouts & Shortlist  •  Player: %s (%d %s)"), nm, ovr, PES10.posShort(rp))
    else
      hdr.Caption = PES10.T("Olheiros & Favoritos  •  Nenhum atleta selecionado", "Scouts & Shortlist  •  No player selected")
    end

    for s = 1, 4 do
      local def = PES10.SCOUT_LISTS[s]
      local u, cap = PES10.getScoutListSummary(s)
      tabBtns[s].setCaption(string.format("%s (%d/%d)", PES10.T(def.pt, def.en):gsub(" %(Shortlist%)", ""):gsub("Lista de ", ""), u, cap))
      tabBtns[s].setActive(s == curSlot)
    end

    local entries = PES10.getScoutListEntries(curSlot)
    local def = PES10.SCOUT_LISTS[curSlot]
    sub.Caption = string.format(PES10.T("%s: %d de %d vagas  •  Duplo-clique carrega o atleta no Studio", "%s: %d of %d slots  •  Double-click loads player into Studio"), PES10.T(def.pt, def.en), #entries, def.cap)
    local gRows = {}
    for idx, e in ipairs(entries) do
      gRows[#gRows + 1] = {
        num = idx, ovr = e.ovr, pos = e.pos, posShort = PES10.posShort(e.pos),
        name = e.name, age = e.age, ageStr = tostring(e.age) .. PES10.T("a", "y"),
        club = e.club, handle = e.handle, playerAddr = e.playerAddr,
      }
    end
    grid.setRows(gRows, 1)
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  local tW = S(167)
  for s = 1, 4 do
    local sCopy = s
    tabBtns[s] = makeModalBtn(f, S(16) + (s - 1) * (tW + S(6)), S(70), tW, S(32), "Slot " .. s, function()
      curSlot = sCopy; refreshHub()
    end, "default")
  end

  local y1 = S(416)
  makeModalBtn(f, S(16), y1, S(214), S(34), PES10.T("+ Adicionar Atleta Atual", "+ Add Current Player"), function()
    if PES10.addPlayerToScoutList(curSlot) then refreshHub() end
  end, "success")
  local function loadScoutEntry()
    local e = grid.getSelectedRow()
    if e and e.playerAddr and e.playerAddr ~= 0 then
      writeQword(PES10.symAddr("ptrPlayer"), e.playerAddr)
      PES10.syncAllDynamicPointers()
      PES10.toast(PES10.T("Carregado: ", "Loaded: ") .. e.name)
      refreshHub()
    end
  end
  grid.onDblClick = loadScoutEntry
  makeModalBtn(f, S(238), y1, S(164), S(34), PES10.T("Carregar no Studio", "Load into Studio"), loadScoutEntry, "primary")
  makeModalBtn(f, S(410), y1, S(150), S(34), PES10.T("Buscar Jogador...", "Find Player..."), function()
    PES10.openScoutFinderModal()
  end, "default")
  makeModalBtn(f, S(568), y1, S(136), S(34), PES10.T("Remover Item", "Remove Item"), function()
    local e = grid.getSelectedRow()
    if e and e.handle then
      PES10.removePlayerFromScoutList(curSlot, e.handle)
      PES10.toast(PES10.T("Removido: ", "Removed: ") .. e.name)
      refreshHub()
    end
  end, "danger")

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, S(16), yF, S(156), S(34), PES10.T("Limpar Esta Lista", "Clear This List"), function()
    PES10.clearScoutSlots(curSlot); refreshHub()
  end, "danger")
  makeModalBtn(ftr, W - S(116), yF, S(100), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")

  f._onPlayerChange = function()
    refreshHub()
  end
  refreshHub()
  f.show()
end

-- MODAL 4: GLOBAL PLAYER SEARCH (DARK CANVAS DATA GRID, SORTABLE COLUMNS)
function PES10.openScoutFinderModal()
  if not createBitmap or not createEdit or not createPanel then
    return PES10.searchCareerPlayerDialog()
  end
  PES10.syncAllDynamicPointers()
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    PES10.toast(PES10.T("Carregue um save de Master League ou BAL primeiro!", "Load a Master League or BAL save first!"))
    return
  end

  local f, S, W, H, hdrBar = makeStudioModal("radar30k", PES10.T("Buscar Jogador no Save", "Find Player in Save"), 780, 600, M_BLUE, 66)
  local runSearch = nil
  local function triggerSearch() if runSearch then runSearch() end end

  makeModalLabel(hdrBar, S(16), S(8), S(190), S(16), PES10.T("Nome ou ID:", "Name or ID:"), 8, M_SUBTEXT, true)
  local edName = makeDarkEdit(hdrBar, S(16), S(26), S(196), S(30), "", triggerSearch, false)

  makeModalLabel(hdrBar, S(220), S(8), S(96), S(16), PES10.T("Posição:", "Position:"), 8, M_SUBTEXT, true)
  local posOrder = {12, 11, 9, 10, 8, 6, 7, 5, 4, 2, 3, 1, 0}
  local posLabels = { PES10.T("Todas", "All") }
  for _, pIdx in ipairs(posOrder) do posLabels[#posLabels + 1] = PES10.posShort(pIdx) end
  local cbPos = makeDarkCombo(hdrBar, S(220), S(26), S(96), S(30), posLabels, 0, nil, f)

  makeModalLabel(hdrBar, S(324), S(8), S(68), S(16), PES10.T("OVR Mín:", "Min OVR:"), 8, M_SUBTEXT, true)
  local edOvr = makeDarkEdit(hdrBar, S(324), S(26), S(64), S(30), "80", triggerSearch, true)

  makeModalLabel(hdrBar, S(396), S(8), S(74), S(16), PES10.T("Idade Máx:", "Max Age:"), 8, M_SUBTEXT, true)
  local edAge = makeDarkEdit(hdrBar, S(396), S(26), S(64), S(30), "45", triggerSearch, true)

  makeModalLabel(hdrBar, S(468), S(8), S(176), S(16), PES10.T("Situação:", "Status:"), 8, M_SUBTEXT, true)
  local specLabels = {
    PES10.T("Todos os Jogadores", "All Players"),
    PES10.T("Contrato <= 1 Ano", "Contract <= 1 Yr"),
    PES10.T("Lista de Transferência", "Transfer Listed"),
    PES10.T("Sem Clube / Livre", "Free Agents"),
  }
  local cbSpec = makeDarkCombo(hdrBar, S(468), S(26), S(178), S(30), specLabels, 0, nil, f)

  local lblStat = makeModalLabel(f, S(16), S(114), W - S(32), S(18),
    PES10.T("Clique nos cabeçalhos para ordenar  •  Duplo-clique carrega o atleta no Studio",
            "Click column headers to sort  •  Double-click loads player into Studio"),
    8, M_SUBTEXT, false)

  local grid = createStudioGrid(f, S(16), S(136), W - S(32), S(392), {
    { key = "ovr",      title = "OVR",                         w = 56,  align = "center", kind = "ovr", sortKey = "ovr" },
    { key = "posShort", title = "POS",                         w = 56,  align = "center", kind = "pos", posIdxKey = "pos" },
    { key = "name",     title = PES10.T("ATLETA", "PLAYER"),   w = 210, sortKey = "name" },
    { key = "ageStr",   title = PES10.T("IDADE", "AGE"),       w = 60,  align = "center", kind = "muted", sortKey = "age" },
    { key = "mkt",      title = PES10.T("MERCADO", "VALUE"),   w = 122, align = "right",  kind = "text",  sortKey = "mktRaw" },
    { key = "club",     title = PES10.T("CLUBE", "CLUB"),      flex = true, kind = "muted", sortKey = "club" },
  }, { rowH = 29, defaultSortKey = "ovr", defaultSortAsc = false, hostForm = f })

  runSearch = function()
    local qRaw = (edName.Text or ""):gsub("^%s+", ""):gsub("%s+$", "")
    local qNorm = PES10.normalizeText(qRaw)
    local qNum = tonumber(qRaw)
    local posIdxSel = cbPos.getIndex()
    local posSel = (posIdxSel and posIdxSel > 0) and posOrder[posIdxSel] or nil
    local minOvr = tonumber(edOvr.Text) or 0
    local maxAge = tonumber(edAge.Text) or 99
    local specMode = cbSpec.getIndex() or 0
    local curYear = PES10.readU16(career + 0x01642A24)
    if curYear < 2019 or curYear > 2150 then curYear = 2026 end

    local count = PES10.readU32(career + 0x00D0BCE8)
    if count == 0 or count > 30001 then count = 30000 end
    local allMatches = {}
    for i = 0, count - 1 do
      local p = career + (i * 0x17C)
      local pid = PES10.readU32(p + 0x30)
      if pid ~= 0 and pid ~= 0xFFFFFFFF then
        local age = PES10.readU8(p + 0x1C) % 64
        local regPosFast = (PES10.readU8(p + 0x07) >> 4) & 0x0F
        local snapOvr = PES10.readU8(p + 0x179)
        local passFast = (age <= maxAge)
          and (posSel == nil or regPosFast == posSel)
          and (qRaw ~= "" or minOvr < 60 or snapOvr == 0 or snapOvr >= (minOvr - 6))
        if passFast then
          local passSpec = true
          if specMode == 1 then
            local expY = PES10.readU16(p + 0x138)
            passSpec = (expY >= 2019 and expY <= curYear + 1)
          elseif specMode == 2 then
            passSpec = (PES10.getBitsInDword(p + 0x14A, 1, 1) == 1)
          elseif specMode == 3 then
            local ch = PES10.readU32(p + 0x12C)
            passSpec = (ch == 0 or ch == 0xFFFFFFFF)
          end
          if passSpec then
            local name = readString(p + 0x38, 40, false) or ""
            local passQ = (qRaw == "") or (qNum and pid == qNum) or (name ~= "" and PES10.normalizeText(name):find(qNorm, 1, true))
            if passQ then
              local ovr, regPos = PES10.calcPlayerOVR(p)
              if ovr >= minOvr and (posSel == nil or regPos == posSel) then
                allMatches[#allMatches + 1] = { addr = p, id = pid, name = name, age = age, ovr = ovr, pos = regPos }
              end
            end
          end
        end
      end
    end

    table.sort(allMatches, function(a, b)
      if a.ovr ~= b.ovr then return a.ovr > b.ovr end
      return a.age < b.age
    end)

    local totalFound = #allMatches
    local showLimit = math.min(150, totalFound)
    local gRows = {}
    for idx = 1, showLimit do
      local m = allMatches[idx]
      local _, _, clubDesc = PES10.decodeClubHandle(PES10.readU32(m.addr + 0x12C))
      local mktRaw = PES10.readI32(m.addr + 0x174)
      gRows[idx] = {
        addr = m.addr, id = m.id, name = m.name, age = m.age,
        ageStr = tostring(m.age) .. PES10.T("a", "y"),
        ovr = m.ovr, pos = m.pos, posShort = PES10.posShort(m.pos),
        mktRaw = mktRaw, mkt = PES10.formatMoneyFromRaw(mktRaw), club = clubDesc,
      }
    end
    grid.setRows(gRows, 1)
    lblStat.Caption = string.format(
      PES10.T("%d atleta(s) encontrados (exibindo Top %d)  •  Clique nas colunas para ordenar", "%d player(s) found (showing Top %d)  •  Click column headers to sort"),
      totalFound, showLimit
    )
  end

  local function loadPicked()
    local m = grid.getSelectedRow()
    if not m or not m.addr then return nil end
    writeQword(PES10.symAddr("ptrPlayer"), m.addr)
    PES10.syncAllDynamicPointers()
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
    return m
  end
  grid.onDblClick = function()
    local m = loadPicked()
    if m then PES10.toast(string.format(PES10.T("Carregado: %s (%d %s)", "Loaded: %s (%d %s)"), m.name, m.ovr, m.posShort)) end
  end

  makeModalBtn(hdrBar, S(654), S(26), S(110), S(30), PES10.T("Buscar", "Search"), runSearch, "primary")

  local yPre = S(76)
  makeModalBtn(f, S(16),  yPre, S(180), S(28), PES10.T("Jovens Sub-21 (74+)", "U21 Talents (74+)"), function()
    edName.Text = ""; cbPos.setIndex(0); edOvr.Text = "74"; edAge.Text = "21"; cbSpec.setIndex(0); runSearch()
  end, "default")
  makeModalBtn(f, S(202), yPre, S(180), S(28), PES10.T("Elite (85+ OVR)", "Elite (85+ OVR)"), function()
    edName.Text = ""; cbPos.setIndex(0); edOvr.Text = "85"; edAge.Text = "45"; cbSpec.setIndex(0); runSearch()
  end, "default")
  makeModalBtn(f, S(388), yPre, S(184), S(28), PES10.T("Fim de Contrato (75+)", "Expiring Deals (75+)"), function()
    edName.Text = ""; cbPos.setIndex(0); edOvr.Text = "75"; edAge.Text = "45"; cbSpec.setIndex(1); runSearch()
  end, "default")
  makeModalBtn(f, S(578), yPre, S(186), S(28), PES10.T("Sem Clube / Livres", "Free Agents"), function()
    edName.Text = ""; cbPos.setIndex(0); edOvr.Text = "70"; edAge.Text = "45"; cbSpec.setIndex(3); runSearch()
  end, "default")

  local ftr, yB = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, S(16),  yB, S(172), S(34), PES10.T("Carregar no Studio", "Load into Studio"), function()
    local m = loadPicked()
    if m then PES10.toast(string.format(PES10.T("Carregado: %s (%d %s)", "Loaded: %s (%d %s)"), m.name, m.ovr, m.posShort)) end
  end, "primary")
  makeModalBtn(ftr, S(196), yB, S(118), S(34), PES10.T("+ Olheiro 1", "+ Scout 1"), function()
    if loadPicked() then PES10.addPlayerToScoutList(1) end
  end, "success")
  makeModalBtn(ftr, S(320), yB, S(118), S(34), PES10.T("+ Olheiro 2", "+ Scout 2"), function()
    if loadPicked() then PES10.addPlayerToScoutList(2) end
  end, "success")
  makeModalBtn(ftr, S(444), yB, S(118), S(34), PES10.T("+ Olheiro 3", "+ Scout 3"), function()
    if loadPicked() then PES10.addPlayerToScoutList(3) end
  end, "success")
  makeModalBtn(ftr, W - S(116), yB, S(100), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")

  runSearch()
  f.show()
end

-- MODAL 5: VISUAL 25 ATTRIBUTES EDITOR (DOUBLE-BUFFERED DATA GRID + GRAPHICAL BARS)
function PES10.openAttributesModal()
  if not createBitmap or not createEdit or not createPanel then return end
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local f, S, W, H, hdrBar = makeStudioModal("attrs", PES10.T("Atributos (40-99) - ", "Attributes (40-99) - ") .. pName, 660, 566, M_BLUE, 58)

  local hdr = makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(22), "", 11, M_TEXT, true)
  makeModalLabel(hdrBar, S(16), S(34), W - S(32), S(18),
    PES10.T("Selecione um atributo para ajustar (-5 / -1 / +1 / +5 ou digite 40-99 e aperte ENTER)",
            "Pick any attribute to adjust (-5 / -1 / +1 / +5 or type 40-99 and press ENTER)"),
    8, M_SUBTEXT, false)

  local grid = createStudioGrid(f, S(16), S(70), W - S(32), S(366), {
    { key = "name",    title = PES10.T("ATRIBUTO", "ATTRIBUTE"),  w = 210, sortKey = "name" },
    { key = "catPill", title = PES10.T("CATEGORIA", "CATEGORY"),  w = 104, align = "center", kind = "pill", sortKey = "catOrder" },
    { key = "val",     title = PES10.T("NÍVEL (40-99)", "RATING (40-99)"), flex = true, kind = "bar", minVal = 40, maxVal = 99, lblW = 36, sortKey = "val" },
  }, { rowH = 29, hostForm = f })

  local fidOrder = {14,33,17,38,20,21,19,22,18,23,25,36,26,27,34,35,24,15,37,39,16,31,29,30,28}
  local fidCat = {
    [14]=1,[33]=1,[17]=1,[38]=1,[19]=1,[22]=1,[18]=1,[23]=1,
    [20]=2,[21]=2,
    [25]=3,[36]=3,[26]=3,[27]=3,[34]=3,[35]=3,[24]=3,
    [15]=4,[37]=4,[39]=4,
    [16]=5,[31]=5,[29]=5,[30]=5,[28]=5,
  }
  local catMeta = {
    [1] = { pt = "ATAQUE",  en = "ATTACK",  bg = 0x281A3B, bd = M_CRIMSON, fg = 0xA692FA },
    [2] = { pt = "PASSE",   en = "PASS",    bg = 0x1E331B, bd = M_GREEN,   fg = 0xB8F08B },
    [3] = { pt = "FÍSICO",  en = "PHYS",    bg = 0x18293A, bd = M_GOLD,    fg = 0x80DAFC },
    [4] = { pt = "DEFESA",  en = "DEFENSE", bg = 0x3A2815, bd = M_BLUE,    fg = 0xFCD97E },
    [5] = { pt = "GOLEIRO", en = "GK",      bg = 0x2B1E16, bd = M_BORDER,  fg = M_SUBTEXT },
  }
  local langIdx = (PES10.lang == "en") and 2 or 1
  local edVal = nil

  local function getPickedFid()
    local r = grid.getSelectedRow()
    if not r then return nil end
    return r.fid, grid.getSelectedIndex()
  end

  local function syncEditBox()
    local fid = getPickedFid()
    if fid and edVal then
      local b = readBytes(p, 0x28, true)
      if b and #b >= 0x28 then edVal.Text = tostring(bufGetStat(b, fid)) end
    end
  end

  local function refreshAttrs(keepIdx)
    local b = readBytes(p, 0x28, true)
    if not b or #b < 0x28 then return end
    local ovr, regPos = PES10.calcPlayerOVRFromBuf(b)
    hdr.Caption = string.format(
      PES10.T("%s  •  OVR: %d %s  •  Undo: %d", "%s  •  OVR: %d %s  •  Undo: %d"),
      pName, ovr, PES10.posShort(regPos), PES10.canUndoPlayer(p)
    )
    local gRows = {}
    for _, fid in ipairs(fidOrder) do
      local lbl = PES10.STAT_LABELS[fid] and PES10.STAT_LABELS[fid][langIdx] or tostring(fid)
      local v = bufGetStat(b, fid)
      local cId = fidCat[fid] or 1
      local cm = catMeta[cId]
      gRows[#gRows + 1] = {
        fid = fid, name = lbl, val = v, catOrder = cId,
        catPill = { txt = PES10.T(cm.pt, cm.en), bg = cm.bg, bd = cm.bd, fg = cm.fg, w = 74 },
      }
    end
    grid.setRows(gRows, keepIdx or grid.getSelectedIndex() or 1)
    syncEditBox()
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  local function setFidVal(newV, isDelta)
    local fid, idx = getPickedFid()
    if not fid then return end
    local b = readBytes(p, 0x28, true)
    if not b or #b < 0x28 then return end
    local cur = bufGetStat(b, fid)
    local target = isDelta and (cur + newV) or newV
    target = math.max(40, math.min(99, math.floor(target + 0.5)))
    if target == cur then return end
    PES10.pushPlayerUndo(p, "Stat " .. tostring(fid))
    bufSetStat(b, fid, target)
    writeBytes(p, b)
    PES10.syncAllDynamicPointers()
    refreshAttrs(idx)
  end

  grid.onSelect = syncEditBox
  grid.onDblClick = function() setFidVal(1, true) end

  local y1 = S(448)
  makeModalBtn(f, S(16),  y1, S(56), S(34), "-5", function() setFidVal(-5, true) end, "default")
  makeModalBtn(f, S(78),  y1, S(56), S(34), "-1", function() setFidVal(-1, true) end, "default")
  makeModalBtn(f, S(140), y1, S(56), S(34), "+1", function() setFidVal(1, true) end, "success")
  makeModalBtn(f, S(202), y1, S(56), S(34), "+5", function() setFidVal(5, true) end, "success")
  local function applyTyped()
    local v = tonumber(edVal and edVal.Text or "")
    if v then setFidVal(v, false) end
  end
  edVal = makeDarkEdit(f, S(268), y1 + S(2), S(64), S(30), "80", applyTyped, true)
  makeModalBtn(f, S(338), y1, S(98),  S(34), PES10.T("Definir", "Set Value"), applyTyped, "primary")
  makeModalBtn(f, S(444), y1, S(200), S(34), PES10.T("+5 em Todos os Atributos", "+5 All Attributes"), function()
    PES10.runPlayerMacro("stats_plus_5"); refreshAttrs(grid.getSelectedIndex())
  end, "default")

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, S(16),  yF, S(166), S(34), PES10.T("Desfazer (Undo)", "Undo Edit"), function()
    PES10.undoLastPlayerEdit(p); refreshAttrs(grid.getSelectedIndex())
  end, "gold")
  makeModalBtn(ftr, S(188), yF, S(166), S(34), PES10.T("Todos 25 em 99", "All 25 to 99"), function()
    PES10.runPlayerMacro("stats_99"); refreshAttrs(grid.getSelectedIndex())
  end, "danger")
  makeModalBtn(ftr, W - S(116), yF, S(100), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")

  f._onPlayerChange = function(newP, newName)
    p = newP; pName = newName
    f.Caption = PES10.T("Atributos (40-99) - ", "Attributes (40-99) - ") .. pName
    refreshAttrs(grid.getSelectedIndex())
  end
  refreshAttrs(1)
  f.show()
end

-- MODAL 6: VISUAL PLAYING STYLE, 41 PLAYER SKILLS & 7 COM AI STYLES EDITOR
function PES10.openSkillsStyleModal()
  if not createBitmap or not createPanel then return end
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local f, S, W, H, hdrBar = makeStudioModal("skills", PES10.T("Estilo & Habilidades - ", "Style & Skills - ") .. pName, 640, 560, M_PURPLE, 58)

  makeModalLabel(hdrBar, S(16), S(18), S(140), S(20), PES10.T("Estilo de Jogo:", "Playing Style:"), 9, M_TEXT, true)
  local psMap = PES10.PLAYING_STYLE_NAMES or {}
  local psItems = {}
  for i = 0, 21 do
    psItems[#psItems + 1] = string.format("%2d: %s", i, psMap[i] or ("Style " .. tostring(i)))
  end
  local curPs = PES10.getBitsInDword(p + 0x1D, 3, 5)
  local cbStyle = makeDarkCombo(hdrBar, S(160), S(13), S(320), S(32), psItems, math.max(0, math.min(21, curPs)), nil, f)

  makeModalBtn(hdrBar, S(488), S(13), S(136), S(32), PES10.T("Aplicar Estilo", "Apply Style"), function()
    local sel = cbStyle.getIndex() or 0
    PES10.pushPlayerUndo(p, "Playing Style")
    PES10.setBitsInDword(p + 0x1D, 3, 5, sel)
    PES10.syncAllDynamicPointers()
    PES10.toast(pName .. ": " .. (psMap[sel] or tostring(sel)))
  end, "primary")

  local sub = makeModalLabel(f, S(16), S(68), W - S(32), S(18),
    PES10.T("Duplo-clique em qualquer linha para ligar ou desligar individualmente:",
            "Double-click any row to toggle ON or OFF individually:"),
    8, M_SUBTEXT, false)

  local grid = createStudioGrid(f, S(16), S(90), W - S(32), S(350), {
    { key = "statePill", title = PES10.T("STATUS", "STATUS"),     w = 88,  align = "center", kind = "pill", sortKey = "onNum" },
    { key = "kindPill",  title = PES10.T("TIPO", "TYPE"),         w = 88,  align = "center", kind = "pill", sortKey = "kind" },
    { key = "label",     title = PES10.T("HABILIDADE", "SKILL"),  flex = true, sortKey = "label" },
  }, { rowH = 29, hostForm = f })

  local combined = {}
  for _, sk in ipairs(PES10.ALL_SKILL_ITEMS or {}) do
    combined[#combined + 1] = { off = sk[1], bit = sk[2], pt = sk[3], en = sk[4], kind = "SKILL" }
  end
  for _, cm in ipairs(PES10.ALL_COM_ITEMS or {}) do
    combined[#combined + 1] = { off = cm[1], bit = cm[2], pt = cm[3], en = cm[4], kind = "COM" }
  end
  local isEn = (PES10.lang == "en")

  local function refreshSkills(keepIdx)
    local skOn, comOn = 0, 0
    local gRows = {}
    for _, it in ipairs(combined) do
      local on = (PES10.getBitsInDword(p + it.off, it.bit, 1) == 1)
      if on then
        if it.kind == "COM" then comOn = comOn + 1 else skOn = skOn + 1 end
      end
      local stPill = on and { txt = PES10.T("● ATIVO", "● ON"), bg = 0x1E331B, bd = M_GREEN, fg = M_GREEN, w = 68 }
                         or { txt = PES10.T("○ OFF",   "○ OFF"), bg = 0x20150F, bd = M_BORDER, fg = M_SUBTEXT, w = 68 }
      local kPill = (it.kind == "COM")
        and { txt = "IA COM", bg = 0x2C1E3D, bd = M_PURPLE, fg = M_PURPLE, w = 66 }
        or  { txt = "SKILL",  bg = 0x3A2815, bd = M_BLUE,   fg = M_BLUE,   w = 66 }
      gRows[#gRows + 1] = {
        off = it.off, bit = it.bit, kind = it.kind, onNum = on and 1 or 0,
        statePill = stPill, kindPill = kPill, label = isEn and it.en or it.pt,
      }
    end
    sub.Caption = string.format(
      PES10.T("Ativas: %d Skills  •  %d Estilos IA COM  (Duplo-clique alterna ON/OFF)", "Active: %d Skills  •  %d COM AI Styles  (Double-click toggles ON/OFF)"),
      skOn, comOn
    )
    grid.setRows(gRows, keepIdx or grid.getSelectedIndex() or 1)
  end

  local function togglePicked()
    local it = grid.getSelectedRow()
    if not it then return end
    local cur = PES10.getBitsInDword(p + it.off, it.bit, 1)
    PES10.pushPlayerUndo(p, it.kind)
    PES10.setBitsInDword(p + it.off, it.bit, 1, cur == 1 and 0 or 1)
    refreshSkills(grid.getSelectedIndex())
  end
  grid.onDblClick = togglePicked

  local y1 = S(452)
  makeModalBtn(f, S(16),  y1, S(198), S(34), PES10.T("Ligar / Desligar Item", "Toggle Item ON/OFF"), togglePicked, "primary")
  makeModalBtn(f, S(220), y1, S(200), S(34), PES10.T("Ativar Todas as 41 Skills", "Enable All 41 Skills"), function()
    PES10.runPlayerMacro("all_39_skills"); refreshSkills(grid.getSelectedIndex())
  end, "success")
  makeModalBtn(f, S(426), y1, S(198), S(34), PES10.T("Limpar Todas", "Clear All"), function()
    PES10.pushPlayerUndo(p, "Clear Skills")
    for _, it in ipairs(combined) do PES10.setBitsInDword(p + it.off, it.bit, 1, 0) end
    refreshSkills(grid.getSelectedIndex())
  end, "danger")

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, S(16), yF, S(166), S(34), PES10.T("Desfazer (Undo)", "Undo Edit"), function()
    PES10.undoLastPlayerEdit(p); cbStyle.setIndex(PES10.getBitsInDword(p + 0x1D, 3, 5)); refreshSkills(grid.getSelectedIndex())
  end, "gold")
  makeModalBtn(ftr, W - S(116), yF, S(100), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")

  f._onPlayerChange = function(newP, newName)
    p = newP; pName = newName
    f.Caption = PES10.T("Estilo & Habilidades - ", "Style & Skills - ") .. pName
    cbStyle.setIndex(math.max(0, math.min(21, PES10.getBitsInDword(p + 0x1D, 3, 5))))
    refreshSkills(grid.getSelectedIndex())
  end
  refreshSkills(1)
  f.show()
end

-- MODAL 7: VISUAL CONTRACT, SALARY, MARKET VALUE & EXACT RELEASE CLAUSE EDITOR
function PES10.openContractModal()
  if not createEdit or not createPanel then return end
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local f, S, W, H, hdrBar = makeStudioModal("contract", PES10.T("Contrato & Multa - ", "Contract & Clause - ") .. pName, 600, 470, M_GOLD, 58)

  local c = PES10.getCurrentCurrency()
  local hdr = makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(22), pName .. "  (" .. c.symbol .. " " .. c.code .. ")", 11, M_TEXT, true)
  makeModalLabel(hdrBar, S(16), S(34), W - S(32), S(18),
    PES10.T("Aceita 50M, 500k, 1.5B, 1000 (ou 0 na Multa para remover override)",
            "Accepts 50M, 500k, 1.5B, 1000 (or 0 in Clause to clear override)"),
    8, M_SUBTEXT, false)

  local applyContract = nil
  local function onEnterContract() if applyContract then applyContract() end end
  local function addField(y, lblPt, lblEn, initVal)
    makeModalLabel(f, S(16), y + S(6), S(240), S(20), PES10.T(lblPt, lblEn), 9, M_SUBTEXT, false)
    return makeDarkEdit(f, S(264), y, S(320), S(30), tostring(initVal or ""), onEnterContract, true)
  end

  local edSal  = addField(S(74),  "Salário Anual Ativo:",         "Active Annual Salary:",          "")
  local edMkt  = addField(S(112), "Valor de Mercado Base:",       "Base Market Value:",             "")
  local edRfee = addField(S(150), "Multa Rescisória Exata:",      "Exact Custom Release Clause:",   "")
  local edExpY = addField(S(188), "Ano Término do Contrato:",     "Contract Expiry Year:",          "")
  local edAff  = addField(S(226), "Afeição pelo Clube (0-100%):", "Club Affection (0-100%):",       "")
  local initRfeeText = ""

  local lblFlags = makeModalLabel(f, S(16), S(268), W - S(32), S(20), "", 9, M_BLUE, true)
  local function refreshContract(reloadInputs)
    if reloadInputs then
      edSal.Text  = PES10.formatMoneyFromRaw(PES10.readI32(p + 0x15C))
      edMkt.Text  = PES10.formatMoneyFromRaw(PES10.readI32(p + 0x174))
      local curRf = PES10.readI32(PES10.symAddr("valComputedReleaseFee"))
      edRfee.Text = (curRf > 0) and PES10.formatMoneyFromRaw(curRf) or "0"
      initRfeeText = edRfee.Text
      edExpY.Text = tostring(PES10.readU16(p + 0x138))
      edAff.Text  = tostring(PES10.getBitsInDword(p + 0x13C, 18, 7))
    end
    local hasRf = PES10.getBitsInDword(p + 0x144, 30, 1) == 1
    local tList = PES10.getBitsInDword(p + 0x14A, 1, 1) == 1
    local lList = PES10.getBitsInDword(p + 0x14A, 2, 1) == 1
    local Y, N = PES10.T("SIM", "YES"), PES10.T("NÃO", "NO")
    lblFlags.Caption = string.format(
      PES10.T("Status:  Multa=%s   |   Lista Transf.=%s   |   Lista Empréstimo=%s", "Status:  Clause=%s   |   Transfer List=%s   |   Loan List=%s"),
      hasRf and Y or N, tList and Y or N, lList and Y or N
    )
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  applyContract = function()
    PES10.pushPlayerUndo(p, "Contract Edit")
    local sRaw = PES10.parseHumanMoneyToRaw(edSal.Text)
    if sRaw then writeInteger(p + 0x15C, sRaw) end
    local mRaw = PES10.parseHumanMoneyToRaw(edMkt.Text)
    if mRaw then writeInteger(p + 0x174, mRaw) end
    if edRfee.Text ~= initRfeeText then
      local rRaw = PES10.parseHumanMoneyToRaw(edRfee.Text)
      if rRaw ~= nil then
        if rRaw <= 0 then PES10.clearOverrideReleaseFeeForPlayer(p)
        else PES10.setOverrideReleaseFeeForPlayer(p, rRaw) end
      end
    end
    local ey = tonumber(edExpY.Text)
    if ey and ey >= 2019 and ey <= 2150 then writeSmallInteger(p + 0x138, ey) end
    local af = tonumber(edAff.Text)
    if af then PES10.setBitsInDword(p + 0x13C, 18, 7, math.max(0, math.min(100, math.floor(af)))) end
    PES10.syncAllDynamicPointers()
    PES10.toast(PES10.T("Contrato atualizado: ", "Contract updated: ") .. pName)
    refreshContract(true)
  end

  local y1, y2 = S(300), S(344)
  makeModalBtn(f, S(16),  y1, S(182), S(34), PES10.T("Aplicar Valores", "Apply Values"), applyContract, "primary")
  makeModalBtn(f, S(204), y1, S(186), S(34), PES10.T("Alternar Multa (S/N)", "Toggle Clause"), function()
    PES10.pushPlayerUndo(p, "Toggle Clause")
    local v = PES10.getBitsInDword(p + 0x144, 30, 1)
    PES10.setBitsInDword(p + 0x144, 30, 1, v == 1 and 0 or 1)
    PES10.syncAllDynamicPointers(); refreshContract(false)
  end, "default")
  makeModalBtn(f, S(396), y1, S(188), S(34), PES10.T("Alternar Lista Transf.", "Toggle Transfer List"), function()
    PES10.pushPlayerUndo(p, "Toggle Transfer List")
    local v = PES10.getBitsInDword(p + 0x14A, 1, 1)
    PES10.setBitsInDword(p + 0x14A, 1, 1, v == 1 and 0 or 1)
    refreshContract(false)
  end, "default")

  makeModalBtn(f, S(16),  y2, S(182), S(34), PES10.T("Destravar Renegociação", "Unlock Renegotiation"), function()
    PES10.runPlayerMacro("unlock_renegotiation"); refreshContract(false)
  end, "default")
  makeModalBtn(f, S(204), y2, S(186), S(34), PES10.T("Facilitar Compra (€ 1k)", "Easy Sign (€ 1k Clause)"), function()
    PES10.makeDealIrresistible(); refreshContract(true)
  end, "gold")
  makeModalBtn(f, S(396), y2, S(188), S(34), PES10.T("Alternar Empréstimo", "Toggle Loan List"), function()
    PES10.pushPlayerUndo(p, "Toggle Loan")
    local v = PES10.getBitsInDword(p + 0x14A, 2, 1)
    PES10.setBitsInDword(p + 0x14A, 2, 1, v == 1 and 0 or 1)
    refreshContract(false)
  end, "default")

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, S(16), yF, S(166), S(34), PES10.T("Desfazer (Undo)", "Undo Edit"), function()
    PES10.undoLastPlayerEdit(p); refreshContract(true)
  end, "gold")
  makeModalBtn(ftr, W - S(116), yF, S(100), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")

  f._onPlayerChange = function(newP, newName)
    p = newP; pName = newName
    f.Caption = PES10.T("Contrato & Multa - ", "Contract & Clause - ") .. pName
    hdr.Caption = pName .. "  (" .. c.symbol .. " " .. c.code .. ")"
    refreshContract(true)
  end
  refreshContract(true)
  f.show()
end

local function writeUtf8Fixed(addr, text, maxLen)
  if not addr or addr <= 0x10000 or not text then return end
  local s = tostring(text)
  if #s >= maxLen then s = s:sub(1, maxLen - 1) end
  local bytes = {}
  for i = 1, #s do bytes[i] = string.byte(s, i) end
  bytes[#bytes + 1] = 0
  writeBytes(addr, bytes)
end

-- MODAL 8: VISUAL PROFILE, IDENTITY, PHYSICAL, CONDITION, LONGEVITY & JSON BACKUP EDITOR
function PES10.openBioConditionModal()
  if not createEdit or not createPanel then return end
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 46, false) or PES10.T("Atleta", "Player")
  local pShirt = readString(p + 0x75, 46, false) or ""
  local f, S, W, H, hdrBar = makeStudioModal("bio", PES10.T("Perfil, Físico & Condição - ", "Profile, Physical & Condition - ") .. pName, 620, 598, M_BLUE, 56)

  local hdr = makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(20), pName, 11, M_TEXT, true)
  makeModalLabel(hdrBar, S(16), S(32), W - S(32), S(16), PES10.T("Nome, camisa, físico, pés, regularidade, seta, auge e backup JSON", "Name, shirt name, physical, feet, form, arrow, peak & JSON backup"), 8, M_SUBTEXT, false)

  local applyBio = nil
  local function onEnterBio() if applyBio then applyBio() end end
  local function addPair(x, y, lbl, val, wEdit, mono)
    makeModalLabel(f, x, y + S(6), S(168), S(18), lbl, 9, M_SUBTEXT, false)
    return makeDarkEdit(f, x + S(172), y, wEdit or S(108), S(30), tostring(val), onEnterBio, mono ~= false)
  end

  makeModalLabel(f, S(16), S(74), S(118), S(18), PES10.T("Nome Jogador:", "Player Name:"), 9, M_SUBTEXT, false)
  local edName  = makeDarkEdit(f, S(138), S(68), S(168), S(30), pName, onEnterBio, false)
  makeModalLabel(f, S(320), S(74), S(114), S(18), PES10.T("Nome Camisa:", "Shirt Name:"), 9, M_SUBTEXT, false)
  local edShirt = makeDarkEdit(f, S(438), S(68), S(166), S(30), pShirt, onEnterBio, false)
  local edAge   = addPair(S(16),  S(106), PES10.T("Idade (15-50):", "Age (15-50):"),             PES10.readU8(p + 0x1C) & 0x3F)
  local edHgt   = addPair(S(320), S(106), PES10.T("Altura (cm):", "Height (cm):"),               PES10.readU8(p + 0x00))
  local edWgt   = addPair(S(16),  S(144), PES10.T("Peso (kg):", "Weight (kg):"),                 PES10.readU8(p + 0x01))
  local edStam  = addPair(S(320), S(144), PES10.T("Stamina (0-100):", "Stamina (0-100):"),       PES10.getBitsInDword(p + 0x146, 0, 7))
  local edWfu   = addPair(S(16),  S(182), PES10.T("Uso Pé Ruim (1-4):", "Weak Foot Use (1-4):"), PES10.getBitsInDword(p + 0x23, 0, 2) + 1)
  local edWfa   = addPair(S(320), S(182), PES10.T("Prec. Pé Ruim (1-4):", "WF Accuracy (1-4):"), PES10.getBitsInDword(p + 0x22, 4, 2) + 1)
  local edFrm   = addPair(S(16),  S(220), PES10.T("Regularidade (1-8):", "Form / Cond (1-8):"),  PES10.getBitsInDword(p + 0x17, 4, 3) + 1)
  local edInjR  = addPair(S(320), S(220), PES10.T("Res. Lesões (1-3):", "Injury Res (1-3):"),    PES10.getBitsInDword(p + 0x22, 6, 2) + 1)
  local edInjD  = addPair(S(16),  S(258), PES10.T("Dias Lesionado (0=OK):", "Injury Days (0=OK):"), PES10.readU8(p + 0x148))

  makeModalLabel(f, S(320), S(264), S(115), S(18), PES10.T("Pé Dominante:", "Strong Foot:"), 9, M_SUBTEXT, false)
  local cbFoot = makeDarkCombo(f, S(440), S(258), S(160), S(30), {
    PES10.T("Direito (Destro)", "Right Foot"),
    PES10.T("Esquerdo (Canhoto)", "Left Foot"),
  }, PES10.getBitsInDword(p + 0x03, 7, 1))

  makeModalLabel(f, S(16), S(306), S(168), S(18), PES10.T("Seta Próx. Jogo:", "Next Match Arrow:"), 9, M_SUBTEXT, false)
  local cbArr = makeDarkCombo(f, S(188), S(300), S(412), S(30), {
    PES10.T("0: Vermelha (Péssima)", "0: Red Down (Worst)"),
    PES10.T("1: Laranja (Ruim)", "1: Orange Down-Right (Poor)"),
    PES10.T("2: Amarela (Normal)", "2: Yellow Flat (Normal)"),
    PES10.T("3: Verde (Boa)", "3: Green Up-Right (Good)"),
    PES10.T("4: Azul (Excelente / Topo)", "4: Blue Up (Top Form)"),
  }, math.max(0, math.min(4, PES10.getBitsInDword(p + 0x147, 0, 3))))

  local natList = PES10.NATIONALITIES or { {144, "Brazil"} }
  local natLabels = {}
  local curNatId = PES10.readU16(p + 0x144)
  local initNatIdx = 0
  for i, nt in ipairs(natList) do
    natLabels[i] = string.format("%s (ID %d)", nt[2], nt[1])
    if nt[1] == curNatId then initNatIdx = i - 1 end
  end
  makeModalLabel(f, S(16), S(346), S(168), S(18), PES10.T("Nacionalidade (221):", "Nationality (221):"), 9, M_SUBTEXT, false)
  local cbNat = makeDarkCombo(f, S(188), S(340), S(412), S(30), natLabels, initNatIdx)

  local function reloadBioFields()
    local nm = readString(p + 0x38, 46, false) or pName
    edName.Text  = nm
    edShirt.Text = readString(p + 0x75, 46, false) or ""
    hdr.Caption  = nm
    edAge.Text   = tostring(PES10.readU8(p + 0x1C) & 0x3F)
    edHgt.Text   = tostring(PES10.readU8(p + 0x00))
    edWgt.Text   = tostring(PES10.readU8(p + 0x01))
    edStam.Text  = tostring(PES10.getBitsInDword(p + 0x146, 0, 7))
    edWfu.Text   = tostring(PES10.getBitsInDword(p + 0x23, 0, 2) + 1)
    edWfa.Text   = tostring(PES10.getBitsInDword(p + 0x22, 4, 2) + 1)
    edFrm.Text   = tostring(PES10.getBitsInDword(p + 0x17, 4, 3) + 1)
    edInjR.Text  = tostring(PES10.getBitsInDword(p + 0x22, 6, 2) + 1)
    edInjD.Text  = tostring(PES10.readU8(p + 0x148))
    cbFoot.setIndex(PES10.getBitsInDword(p + 0x03, 7, 1))
    cbArr.setIndex(math.max(0, math.min(4, PES10.getBitsInDword(p + 0x147, 0, 3))))
    local nId = PES10.readU16(p + 0x144)
    for i, nt in ipairs(natList) do
      if nt[1] == nId then cbNat.setIndex(i - 1); break end
    end
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  applyBio = function()
    PES10.pushPlayerUndo(p, "Bio & Condition")
    local newNm = (edName.Text or ""):gsub("^%s+", ""):gsub("%s+$", "")
    if newNm ~= "" and newNm ~= readString(p + 0x38, 46, false) then
      writeUtf8Fixed(p + 0x38, newNm, 60)
      hdr.Caption = newNm
    end
    local newSh = (edShirt.Text or ""):gsub("^%s+", ""):gsub("%s+$", "")
    if newSh ~= "" and newSh ~= readString(p + 0x75, 46, false) then
      writeUtf8Fixed(p + 0x75, newSh, 60)
    end
    local age = tonumber(edAge.Text); if age then PES10.setBitsInDword(p + 0x1C, 0, 6, math.max(15, math.min(50, math.floor(age)))) end
    local hgt = tonumber(edHgt.Text); if hgt then writeBytes(p + 0x00, math.max(140, math.min(220, math.floor(hgt)))) end
    local wgt = tonumber(edWgt.Text); if wgt then writeBytes(p + 0x01, math.max(45, math.min(130, math.floor(wgt)))) end
    local stm = tonumber(edStam.Text); if stm then PES10.setBitsInDword(p + 0x146, 0, 7, math.max(0, math.min(100, math.floor(stm)))) end
    local wfu = tonumber(edWfu.Text); if wfu then PES10.setBitsInDword(p + 0x23, 0, 2, math.max(0, math.min(3, math.floor(wfu) - 1))) end
    local wfa = tonumber(edWfa.Text); if wfa then PES10.setBitsInDword(p + 0x22, 4, 2, math.max(0, math.min(3, math.floor(wfa) - 1))) end
    local frm = tonumber(edFrm.Text); if frm then PES10.setBitsInDword(p + 0x17, 4, 3, math.max(0, math.min(7, math.floor(frm) - 1))) end
    local ijr = tonumber(edInjR.Text); if ijr then PES10.setBitsInDword(p + 0x22, 6, 2, math.max(0, math.min(2, math.floor(ijr) - 1))) end
    local ijd = tonumber(edInjD.Text); if ijd then writeBytes(p + 0x148, math.max(0, math.min(255, math.floor(ijd)))) end
    local arrIdx = cbArr.getIndex()
    PES10.setBitsInDword(p + 0x03, 7, 1, cbFoot.getIndex() or 0)
    PES10.setBitsInDword(p + 0x147, 0, 3, arrIdx or 4)
    PES10.setBitsInDword(p + 0x146, 7, 1, (arrIdx == 4) and 1 or 0)
    local selNat = natList[(cbNat.getIndex() or 0) + 1]
    if selNat and selNat[1] then writeSmallInteger(p + 0x144, selNat[1]) end
    PES10.syncAllDynamicPointers()
    PES10.toast(PES10.T("Perfil atualizado: ", "Profile updated: ") .. (newNm ~= "" and newNm or pName))
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  local y1, y2 = S(402), S(448)
  makeModalBtn(f, S(16),  y1, S(288), S(36), PES10.T("Aplicar Todas as Alterações", "Apply All Changes"), applyBio, "primary")
  makeModalBtn(f, S(312), y1, S(288), S(36), PES10.T("Auge (Breakthrough) + Anti-Declínio", "Breakthrough + Anti-Decline 30+"), function()
    PES10.applyPlayerEternalPeak(p)
  end, "success")

  makeModalBtn(f, S(16),  y2, S(190), S(34), PES10.T("Rejuvenescer (-2 Anos)", "Rejuvenate (-2 Yrs)"), function()
    PES10.rejuvenateAndSyncMarket(-2); reloadBioFields()
  end, "default")
  makeModalBtn(f, S(212), y2, S(192), S(34), PES10.T("Exportar Backup (.json)", "Export Backup (.json)"), function()
    PES10.exportPlayerBackupJson()
  end, "default")
  makeModalBtn(f, S(410), y2, S(190), S(34), PES10.T("Importar Backup (.json)", "Import Backup (.json)"), function()
    PES10.importPlayerBackupJson(); reloadBioFields()
  end, "default")

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, S(16), yF, S(166), S(34), PES10.T("Desfazer (Undo)", "Undo Edit"), function()
    PES10.undoLastPlayerEdit(p); reloadBioFields()
  end, "gold")
  makeModalBtn(ftr, W - S(116), yF, S(100), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")

  f._onPlayerChange = function(newP, newName)
    p = newP; pName = newName
    f.Caption = PES10.T("Perfil, Físico & Condição - ", "Profile, Physical & Condition - ") .. pName
    reloadBioFields()
  end
  f.show()
end
function PES10.openFinancesModal()
  if not createEdit or not createPanel then return end
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    PES10.toast(PES10.T("Carregue um save de Master League primeiro!", "Load a Master League save first!"))
    return
  end
  local c = PES10.getCurrentCurrency()
  local f, S, W, H, hdrBar = makeStudioModal("finances", PES10.T("Central Financeira - Master League", "Finances Hub - Master League"), 540, 324, M_GOLD, 58)
  makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(22), PES10.T("Orçamentos da Master League (" .. c.symbol .. ")", "Master League Budgets (" .. c.symbol .. ")"), 11, M_TEXT, true)
  local subFin = makeModalLabel(hdrBar, S(16), S(34), W - S(32), S(18), PES10.T("Digite qualquer valor (ex: 25M, 500k, 1.5B ou 0) para subir ou reduzir:", "Type any amount (e.g. 25M, 500k, 1.5B or 0) to raise or lower:"), 8, M_SUBTEXT, false)

  local applyFin = nil
  local function onEnterFin() if applyFin then applyFin() end end

  makeModalLabel(f, S(16), S(82), S(210), S(20), PES10.T("Orçamento de Transferências:", "Transfer Budget:"), 9, M_SUBTEXT, false)
  local edTrf = makeDarkEdit(f, S(232), S(76), S(292), S(32), "", onEnterFin, true)

  makeModalLabel(f, S(16), S(124), S(210), S(20), PES10.T("Salários Disponíveis:", "Available Wage Budget:"), 9, M_SUBTEXT, false)
  local edWag = makeDarkEdit(f, S(232), S(118), S(292), S(32), "", onEnterFin, true)

  local curWageBill = 0
  local function reloadFin()
    edTrf.Text = PES10.formatMoneyFromRaw(PES10.readI32(career + 0x016ECBF4))
    local availSal, _, squadWages = PES10.getAvailableSalaryBudget(career, true)
    curWageBill = squadWages
    edWag.Text = PES10.formatMoneyFromRaw(availSal)
    subFin.Caption = string.format(
      PES10.T("Folha anual do elenco: %s  •  Aceita 25M, 500k, 0, etc.", "Current squad wage bill: %s  •  Accepts 25M, 500k, 0, etc."),
      PES10.formatMoneyFromRaw(squadWages)
    )
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  local function addDelta(off, rawDelta, label)
    local cur = PES10.readI32(career + off)
    local nv = math.max(0, math.min(2147483647, cur + rawDelta))
    writeInteger(career + off, nv)
    PES10.toast(label)
    reloadFin()
  end

  applyFin = function()
    local tRaw = PES10.parseHumanMoneyToRaw(edTrf.Text)
    if tRaw ~= nil then writeInteger(career + 0x016ECBF4, math.max(0, math.min(2147483647, tRaw))) end
    local wRaw = PES10.parseHumanMoneyToRaw(edWag.Text)
    if wRaw ~= nil then
      -- Convert desired Available Salary Budget back to Konami Gross Salary Cap (avail + squadWages)
      local newGross = math.max(0, math.min(2147483647, wRaw + curWageBill))
      writeInteger(career + 0x016ECC08, newGross)
    end
    PES10.toast(PES10.T("Orçamentos atualizados!", "Budgets updated!"))
    reloadFin()
  end

  makeModalLabel(f, S(16), S(166), W - S(32), S(16), PES10.T("AJUSTES RÁPIDOS:", "QUICK ADJUSTMENTS:"), 8, M_GOLD, true)
  local yQ = S(186)
  makeModalBtn(f, S(16),  yQ, S(118), S(34), PES10.T("-10M Transf.", "-10M Transf."), function() addDelta(0x016ECBF4, -100000, PES10.T("-10M em Transferências", "-10M Transfer Budget")) end, "default")
  makeModalBtn(f, S(142), yQ, S(118), S(34), PES10.T("+10M Transf.", "+10M Transf."), function() addDelta(0x016ECBF4,  100000, PES10.T("+10M em Transferências", "+10M Transfer Budget")) end, "success")
  makeModalBtn(f, S(268), yQ, S(124), S(34), PES10.T("+50M Transf.", "+50M Transf."), function() addDelta(0x016ECBF4,  500000, PES10.T("+50M em Transferências", "+50M Transfer Budget")) end, "success")
  makeModalBtn(f, S(400), yQ, S(124), S(34), PES10.T("+10M Salários", "+10M Wages"),  function() addDelta(0x016ECC08,  100000, PES10.T("+10M em Salários", "+10M Wage Budget")) end, "gold")

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, S(16), yF, S(180), S(34), PES10.T("Aplicar Valores Exatos", "Apply Exact Values"), applyFin, "primary")
  makeModalBtn(ftr, W - S(116), yF, S(100), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")

  reloadFin()
  f.show()
end

-- MODAL 10: SQUAD STUDIO (DOUBLE-BUFFERED CANVAS DATA GRID, SORTABLE COLUMNS)
function PES10.openSquadRosterModal()
  if not createBitmap or not createPanel then return end
  local modeStr, clubStr, career = PES10.getCareerModeAndClubInfo()
  if career == 0 then
    PES10.toast(PES10.T("Carregue um save de Master League primeiro!", "Load a Master League save first!"))
    return
  end
  local userClub = PES10.getUserClubHandle(career)
  if userClub == 0 or userClub == 0xFFFFFFFF then
    PES10.toast(PES10.T("Clube do usuário não encontrado na RAM.", "User club handle not found in RAM."))
    return
  end

  local cleanClub = (clubStr ~= "" and clubStr:gsub("%s*%(ID %d+%)", "")) or PES10.T("Meu Clube", "My Club")
  local f, S, W, H, hdrBar = makeStudioModal("squad", PES10.T("Meu Elenco - ", "My Squad - ") .. cleanClub, 820, 586, M_GREEN, 58)
  local hdr = makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(22), PES10.T("Elenco: ", "Squad: ") .. cleanClub, 11, M_TEXT, true)
  makeModalLabel(hdrBar, S(16), S(34), W - S(32), S(18),
    PES10.T("Clique nas colunas para ordenar  •  Duplo-clique carrega o atleta direto no Studio",
            "Click column headers to sort  •  Double-click loads player straight into Studio"),
    8, M_SUBTEXT, false)

  local filterMode = 1 -- 1=All, 2=Expiring<=1yr, 3=Injured, 4=U23
  local tabBtns = {}

  local grid = createStudioGrid(f, S(16), S(112), W - S(32), S(356), {
    { key = "num",      title = "#",                         w = 34,  align = "center", kind = "muted" },
    { key = "ovr",      title = "OVR",                       w = 54,  align = "center", kind = "ovr",  sortKey = "ovr" },
    { key = "posShort", title = "POS",                       w = 54,  align = "center", kind = "pos",  posIdxKey = "pos", sortKey = "pos" },
    { key = "name",     title = PES10.T("ATLETA", "PLAYER"), flex = true, sortKey = "name" },
    { key = "ageStr",   title = PES10.T("IDADE", "AGE"),     w = 56,  align = "center", kind = "muted", sortKey = "age" },
    { key = "yrTxt",    title = PES10.T("CONTR.", "EXP."),   w = 66,  align = "center", kind = "muted", sortKey = "expY" },
    { key = "sal",      title = PES10.T("SALÁRIO", "WAGE"),  w = 122, align = "right",  kind = "text",  sortKey = "salRaw" },
    { key = "stam",     title = PES10.T("FÔLEGO", "STAMINA"),w = 108, kind = "bar", minVal = 0, maxVal = 100, suffix = "%", lblW = 36, sortKey = "stam" },
    { key = "injPill",  title = PES10.T("SAÚDE", "HEALTH"),  w = 64,  align = "center", kind = "pill", sortKey = "inj" },
  }, { rowH = 29, defaultSortKey = "ovr", defaultSortAsc = false, emptyText = PES10.T("Nenhum atleta corresponde a este filtro.", "No players match this filter."), hostForm = f })

  local function refreshSquad()
    local curYear = PES10.readU16(career + 0x01642A24)
    if curYear < 2019 or curYear > 2150 then curYear = 2026 end
    local count = PES10.readU32(career + 0x00D0BCE8)
    if count == 0 or count > 30001 then count = 30000 end
    local list = {}
    local allCnt, expCnt, injCnt, u23Cnt = 0, 0, 0, 0
    for i = 0, count - 1 do
      local p = career + (i * 0x17C)
      if PES10.readU32(p + 0x12C) == userClub then
        local pid = PES10.readU32(p + 0x30)
        if pid ~= 0 and pid ~= 0xFFFFFFFF then
          allCnt = allCnt + 1
          local age = PES10.readU8(p + 0x1C) & 0x3F
          local expY = PES10.readU16(p + 0x138)
          local inj = PES10.readU8(p + 0x148)
          local isExp = (expY >= 2019 and expY <= curYear + 1)
          if isExp then expCnt = expCnt + 1 end
          if inj > 0 then injCnt = injCnt + 1 end
          if age <= 23 then u23Cnt = u23Cnt + 1 end
          local pass = (filterMode == 1)
            or (filterMode == 2 and isExp)
            or (filterMode == 3 and inj > 0)
            or (filterMode == 4 and age <= 23)
          if pass then
            local nm = readString(p + 0x38, 34, false) or "?"
            local ovr, regPos = PES10.calcPlayerOVR(p)
            local salRaw = PES10.readI32(p + 0x15C)
            local stam = PES10.getBitsInDword(p + 0x146, 0, 7)
            local yrTxt = (expY >= 2019 and expY <= 2150) and tostring(expY) or "-"
            local injPill = (inj > 0)
              and { txt = string.format("%dd", inj), bg = 0x26193A, bd = M_CRIMSON, fg = M_CRIMSON, w = 44 }
              or  { txt = "OK", bg = 0x1E331B, bd = M_GREEN, fg = M_GREEN, w = 44 }
            list[#list + 1] = {
              addr = p, name = nm, ovr = ovr, pos = regPos, posShort = PES10.posShort(regPos),
              age = age, ageStr = tostring(age) .. PES10.T("a", "y"),
              expY = expY, yrTxt = yrTxt, salRaw = salRaw, sal = PES10.formatMoneyFromRaw(salRaw),
              stam = stam, inj = inj, injPill = injPill,
            }
          end
        end
      end
    end
    table.sort(list, function(a, b) if a.ovr ~= b.ovr then return a.ovr > b.ovr end return a.name < b.name end)
    for idx, r in ipairs(list) do r.num = idx end

    local caps = {
      string.format(PES10.T("Todos (%d)", "All (%d)"), allCnt),
      string.format(PES10.T("Fim Contrato (%d)", "Expiring (%d)"), expCnt),
      string.format(PES10.T("Lesionados (%d)", "Injured (%d)"), injCnt),
      string.format(PES10.T("Sub-23 (%d)", "U23 (%d)"), u23Cnt),
    }
    for i = 1, 4 do
      tabBtns[i].setCaption(caps[i])
      tabBtns[i].setActive(i == filterMode)
    end

    hdr.Caption = string.format(PES10.T("%s  •  %d atletas no elenco", "%s  •  %d players in squad"), cleanClub, allCnt)
    grid.setRows(list, 1)
  end

  local tW = S(192)
  for i = 1, 4 do
    local iCopy = i
    tabBtns[i] = makeModalBtn(f, S(16) + (i - 1) * (tW + S(6)), S(70), tW, S(32), "Tab " .. i, function()
      filterMode = iCopy; refreshSquad()
    end, "default")
  end

  local function loadPicked()
    local r = grid.getSelectedRow()
    if r and r.addr then
      writeQword(PES10.symAddr("ptrPlayer"), r.addr)
      PES10.syncAllDynamicPointers()
      PES10.toast(string.format(PES10.T("Carregado: %s (%d %s)", "Loaded: %s (%d %s)"), r.name, r.ovr, r.posShort))
      if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
    end
  end
  grid.onDblClick = loadPicked

  local yA = S(480)
  makeModalBtn(f, S(16),  yA, S(190), S(34), PES10.T("Carregar no Studio", "Load into Studio"), loadPicked, "primary")
  makeModalBtn(f, S(214), yA, S(142), S(34), PES10.T("100% Fôlego", "100% Stamina"), function() PES10.runSquadCommander("stamina_100"); refreshSquad() end, "default")
  makeModalBtn(f, S(364), yA, S(142), S(34), PES10.T("Seta Azul", "Blue Form"), function() PES10.runSquadCommander("blue_form"); refreshSquad() end, "default")
  makeModalBtn(f, S(514), yA, S(142), S(34), PES10.T("Curar Lesões", "Heal Squad"), function() PES10.runSquadCommander("heal_affection"); refreshSquad() end, "success")
  makeModalBtn(f, S(664), yA, S(140), S(34), PES10.T("Contratos +5a", "+5 Yr Deals"), function() PES10.runSquadCommander("shield_contracts"); refreshSquad() end, "gold")

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, S(16), yF, S(160), S(34), PES10.T("Atualizar Lista", "Refresh Squad"), refreshSquad, "default")
  makeModalBtn(ftr, W - S(116), yF, S(100), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")

  refreshSquad()
  f.show()
end

-- MODAL 11: GRANULAR TRAINING SPEED SELECTOR (1x, 2x, 5x, 10x, 25x, INSTANT)
function PES10.openTrainingSpeedModal()
  if not createPanel then return end
  local f, S, W, H, hdrBar = makeStudioModal("trnspeed", PES10.T("Velocidade de Treino (Sub-XP)", "Training Speed (Sub-XP)"), 480, 310, M_PURPLE, 58)
  makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(22), PES10.T("Velocidade de Treino (Seu Clube, Base & BAL)", "Training Speed (Your Club, Youth & BAL)"), 11, M_TEXT, true)
  makeModalLabel(hdrBar, S(16), S(34), W - S(32), S(18), PES10.T("Escolha o multiplicador exato de evolução:", "Pick your exact Sub-XP progression multiplier:"), 8, M_SUBTEXT, false)

  local modes = {
    { id = 0, pt = "1x Normal (Original)",   en = "1x Normal (Default)" },
    { id = 1, pt = "2x Realista",            en = "2x Realistic" },
    { id = 2, pt = "5x Rápido",              en = "5x Fast" },
    { id = 3, pt = "10x Turbinado",          en = "10x Turbo" },
    { id = 4, pt = "50x Turbo",              en = "50x Turbo" },
    { id = 5, pt = "Instantâneo (10k/dia)",  en = "Instant (10k/day)" },
  }
  local btns = {}
  local function refreshModes()
    local cur = PES10.readU32(PES10.symAddr("cfgTrainingMultMode"))
    for i, m in ipairs(modes) do btns[i].setActive(cur == m.id) end
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  local bw = S(218)
  for idx, m in ipairs(modes) do
    local col = (idx - 1) % 2
    local row = math.floor((idx - 1) / 2)
    local bx = S(16) + col * (bw + S(12))
    local by = S(76) + row * S(50)
    btns[idx] = makeModalBtn(f, bx, by, bw, S(40), PES10.T(m.pt, m.en), function()
      local a = PES10.symAddr("cfgTrainingMultMode")
      if a ~= 0 then
        writeInteger(a, m.id)
        if m.id > 0 then PES10.ensureHookActive(3) end
        PES10.toast(PES10.T("Treino: ", "Training: ") .. PES10.T(m.pt, m.en))
        refreshModes()
      end
    end, "default")
  end

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, W - S(116), yF, S(100), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")

  refreshModes()
  f.show()
end

-- MODAL 12: ALL 6 LIVE MATCH STAMINA MODES
function PES10.openMatchStaminaModal()
  if not createPanel then return end
  local f, S, W, H, hdrBar = makeStudioModal("stamina", PES10.T("Controle de Stamina (Partida)", "Match Stamina Control"), 480, 310, M_GREEN, 58)
  makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(22), PES10.T("Modos de Stamina ao Vivo", "Live Match Stamina Modes"), 11, M_TEXT, true)
  makeModalLabel(hdrBar, S(16), S(34), W - S(32), S(18), PES10.T("Selecione qual time recebe 100% de fôlego ou drenagem:", "Choose which team gets 100% stamina or drain:"), 8, M_SUBTEXT, false)

  local modes = {
    { id = 0, pt = "Normal (Original)",          en = "Normal (Default)" },
    { id = 1, pt = "100% Casa (Home)",           en = "100% Home Team" },
    { id = 2, pt = "100% Fora (Away)",           en = "100% Away Team" },
    { id = 3, pt = "100% Ambos os Times",        en = "100% Both Teams" },
    { id = 4, pt = "Pressão Casa (Drena Fora)",  en = "Home Press (Drain Away)" },
    { id = 5, pt = "Pressão Fora (Drena Casa)",  en = "Away Press (Drain Home)" },
  }
  local btns = {}
  local function refreshModes()
    local cur = PES10.readU32(PES10.symAddr("cfgStaminaMode"))
    for i, m in ipairs(modes) do btns[i].setActive(cur == m.id) end
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  local bw = S(218)
  for idx, m in ipairs(modes) do
    local col = (idx - 1) % 2
    local row = math.floor((idx - 1) / 2)
    local bx = S(16) + col * (bw + S(12))
    local by = S(76) + row * S(50)
    btns[idx] = makeModalBtn(f, bx, by, bw, S(40), PES10.T(m.pt, m.en), function()
      local a = PES10.symAddr("cfgStaminaMode")
      if a ~= 0 then
        if m.id > 0 then PES10.ensureHookActive(6) end
        writeInteger(a, m.id)
        PES10.toast(PES10.T("Stamina: ", "Stamina: ") .. PES10.T(m.pt, m.en))
        refreshModes()
      end
    end, "default")
  end

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, W - S(116), yF, S(100), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")

  refreshModes()
  f.show()
end

-- MODAL 13: EXACT BAL FOCUS POINTS & CAP UNLOCK (0-99 OR MAX 10 BARS)
function PES10.openBALFocusModal()
  if not createEdit or not createPanel then return end
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    PES10.toast(PES10.T("Carregue um save de Rumo ao Estrelato (BAL) primeiro!", "Load a Become a Legend (BAL) save first!"))
    return
  end
  local f, S, W, H, hdrBar = makeStudioModal("balfocus", PES10.T("BAL - Focus Points & Barras", "BAL - Focus Points & Sliders"), 480, 276, M_GOLD, 58)
  makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(22), PES10.T("Focus Points do Rumo ao Estrelato (BAL)", "Become a Legend (BAL) Focus Points"), 11, M_TEXT, true)
  makeModalLabel(hdrBar, S(16), S(34), W - S(32), S(18), PES10.T("Defina qualquer valor exato (0-99) ou maximize 10 barras:", "Set any exact amount (0-99) or max all 10 sliders:"), 8, M_SUBTEXT, false)

  local curFp = PES10.readU8(career + 0x016ECBEA)
  local edFp = nil
  local function applyExactFp()
    local v = tonumber(edFp and edFp.Text or "")
    if not v then return end
    v = math.max(0, math.min(99, math.floor(v + 0.5)))
    if v > 21 then PES10.ensureHookActive(4) end
    writeBytes(career + 0x016ECBEA, v)
    PES10.toast(string.format(PES10.T("BAL Focus Points definido para %d!", "BAL Focus Points set to %d!"), v))
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  makeModalLabel(f, S(16), S(82), S(190), S(20), PES10.T("Focus Points Exatos (0-99):", "Exact Focus Points (0-99):"), 9, M_SUBTEXT, false)
  edFp = makeDarkEdit(f, S(212), S(76), S(90), S(32), tostring(curFp), applyExactFp, true)
  makeModalBtn(f, S(310), S(76), S(154), S(32), PES10.T("Aplicar Valor", "Apply Value"), applyExactFp, "primary")

  local yQ = S(130)
  makeModalBtn(f, S(16),  yQ, S(220), S(36), PES10.T("Max 99 Pts + 10 Barras", "Max 99 Pts + 10 Sliders"), function()
    PES10.runPlayerMacro("bal_max_sliders")
    edFp.Text = "99"
  end, "gold")
  local lockBtn = nil
  local function syncLockBtn()
    local isL = (PES10.readU32(PES10.symAddr("cfgLockBALFocusPoints")) == 1)
    lockBtn.setCaption(isL and PES10.T("Trava 99 Pts: ATIVA", "Lock 99 Pts: ON") or PES10.T("Travar em 99 Pts", "Lock at 99 Pts"))
    lockBtn.setActive(isL)
  end
  lockBtn = makeModalBtn(f, S(244), yQ, S(220), S(36), PES10.T("Travar em 99 Pts", "Lock at 99 Pts"), function()
    local a = PES10.symAddr("cfgLockBALFocusPoints")
    if a ~= 0 then
      local nv = (PES10.readU32(a) == 1) and 0 or 1
      writeInteger(a, nv)
      if nv == 1 then PES10.ensureHookActive(4); writeBytes(career + 0x016ECBEA, 99); edFp.Text = "99" end
      syncLockBtn()
    end
  end, "default")
  syncLockBtn()

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, W - S(116), yF, S(100), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")
  f.show()
end

-- MODAL 14: QUICK PLAYER STAMINA, FORM ARROW & INJURY CONTROL
function PES10.openQuickConditionModal()
  if not createPanel or not createEdit then return end
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local f, S, W, H, hdrBar = makeStudioModal("quickcond", PES10.T("Fôlego & Forma - ", "Stamina & Form - ") .. pName, 560, 356, M_GREEN, 58)

  local hdr = makeModalLabel(hdrBar, S(16), S(10), W - S(32), S(22), pName, 11, M_TEXT, true)
  local sub = makeModalLabel(hdrBar, S(16), S(34), W - S(32), S(18), "", 8, M_GREEN, false)

  local arrowDefs = {
    { id = 0, pt = "↓ Vermelha", en = "↓ Red",    var = "danger" },
    { id = 1, pt = "↘ Laranja",  en = "↘ Orange", var = "default" },
    { id = 2, pt = "→ Amarela",  en = "→ Yellow", var = "gold" },
    { id = 3, pt = "↗ Verde",    en = "↗ Green",  var = "success" },
    { id = 4, pt = "↑ Azul",     en = "↑ Blue",   var = "primary" },
  }
  local arrowBtns = {}
  local edStam = nil

  local function refreshCond()
    local stam = PES10.getBitsInDword(p + 0x146, 0, 7)
    local arr  = math.max(0, math.min(4, PES10.getBitsInDword(p + 0x147, 0, 3)))
    local inj  = PES10.readU8(p + 0x148)
    local arrLbl = PES10.T(arrowDefs[arr + 1].pt, arrowDefs[arr + 1].en)
    local injTxt = (inj > 0) and string.format(PES10.T("%d dia(s)", "%d day(s)"), inj) or PES10.T("Saudável", "Healthy")
    sub.Caption = string.format(PES10.T("Fôlego: %d%%  •  Seta: %s  •  Saúde: %s", "Stamina: %d%%  •  Arrow: %s  •  Health: %s"), stam, arrLbl, injTxt)
    if edStam then edStam.Text = tostring(stam) end
    for i, ad in ipairs(arrowDefs) do arrowBtns[i].setActive(arr == ad.id) end
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  local function setStam(val)
    val = math.max(0, math.min(100, math.floor((tonumber(val) or 100) + 0.5)))
    PES10.pushPlayerUndo(p, "Stamina")
    PES10.setBitsInDword(p + 0x146, 0, 7, val)
    PES10.syncAllDynamicPointers()
    PES10.toast(string.format(PES10.T("%s: Fôlego %d%%", "%s: Stamina %d%%"), pName, val))
    refreshCond()
  end

  local function setArrow(val)
    PES10.pushPlayerUndo(p, "Form Arrow")
    PES10.setBitsInDword(p + 0x147, 0, 3, val)
    PES10.setBitsInDword(p + 0x146, 7, 1, (val == 4) and 1 or 0)
    PES10.syncAllDynamicPointers()
    PES10.toast(string.format("%s: %s", pName, PES10.T(arrowDefs[val + 1].pt, arrowDefs[val + 1].en)))
    refreshCond()
  end

  makeModalLabel(f, S(16), S(72), W - S(32), S(16), PES10.T("FÔLEGO / STAMINA (0-100%):", "STAMINA / FATIGUE (0-100%):"), 8, M_SUBTEXT, true)
  local yS = S(94)
  makeModalBtn(f, S(16),  yS, S(78),  S(34), "50%",  function() setStam(50) end, "default")
  makeModalBtn(f, S(100), yS, S(78),  S(34), "75%",  function() setStam(75) end, "default")
  makeModalBtn(f, S(184), yS, S(148), S(34), PES10.T("100% Fôlego", "100% Stamina"), function() setStam(100) end, "success")
  edStam = makeDarkEdit(f, S(342), yS + S(2), S(74), S(30), "100", function() setStam(edStam.Text) end, true)
  makeModalBtn(f, S(422), yS, S(122), S(34), PES10.T("Definir %", "Set %"), function() setStam(edStam.Text) end, "primary")

  makeModalLabel(f, S(16), S(144), W - S(32), S(16), PES10.T("SETA DE FORMA (PRÓXIMO JOGO):", "NEXT MATCH FORM ARROW:"), 8, M_SUBTEXT, true)
  local yA = S(166)
  local aW = S(100)
  for i, ad in ipairs(arrowDefs) do
    arrowBtns[i] = makeModalBtn(f, S(16) + (i - 1) * (aW + S(7)), yA, aW, S(36), PES10.T(ad.pt, ad.en), function()
      setArrow(ad.id)
    end, ad.var)
  end

  local yQ = S(226)
  makeModalBtn(f, S(16),  yQ, S(260), S(38), PES10.T("100% Fôlego + Seta Azul", "100% Stamina + Blue Arrow"), function()
    PES10.pushPlayerUndo(p, "Full Condition")
    PES10.setBitsInDword(p + 0x146, 0, 7, 100)
    PES10.setBitsInDword(p + 0x147, 0, 3, 4)
    PES10.setBitsInDword(p + 0x146, 7, 1, 1)
    writeBytes(p + 0x148, 0)
    PES10.syncAllDynamicPointers()
    PES10.toast(pName .. ": " .. PES10.T("100% Fôlego + Seta Azul + Saudável!", "100% Stamina + Blue Arrow + Healthy!"))
    refreshCond()
  end, "primary")
  makeModalBtn(f, S(284), yQ, S(260), S(38), PES10.T("Curar Lesão (0 dias)", "Heal Injury (0 days)"), function()
    PES10.pushPlayerUndo(p, "Heal Injury")
    writeBytes(p + 0x148, 0)
    PES10.syncAllDynamicPointers()
    PES10.toast(pName .. ": " .. PES10.T("Lesão zerada!", "Injury cleared!"))
    refreshCond()
  end, "success")

  local ftr, yF = makeDarkFooter(f, W, H, S(58))
  makeModalBtn(ftr, S(16), yF, S(150), S(34), PES10.T("Desfazer (Undo)", "Undo Edit"), function()
    PES10.undoLastPlayerEdit(p); refreshCond()
  end, "gold")
  makeModalBtn(ftr, W - S(116), yF, S(100), S(34), PES10.T("Fechar", "Close"), function() f.close() end, "subtle")

  f._onPlayerChange = function(newP, newName)
    p = newP; pName = newName
    f.Caption = PES10.T("Fôlego & Forma - ", "Stamina & Form - ") .. pName
    hdr.Caption = pName
    refreshCond()
  end
  refreshCond()
  f.show()
end

-- ============================================================================
-- PES 2021 v12.0 ULTIMATE STUDIO - DOUBLE-BUFFERED OFFSCREEN CANVAS GUI (PT / EN)
-- Pixel-exact symmetric layout, DPI-aware, stateful active glow & zero clipping
-- ============================================================================
function PES10.openDashboard()
  if PES10.dashboardForm then
    PES10.dashboardForm.show()
    PES10.dashboardForm.bringToFront()
    if PES10.dashboardState and PES10.dashboardState.render then
      PES10.dashboardState.render(true)
    end
    return
  end

  local dpi = (getScreenDPI and getScreenDPI()) or 96
  if dpi < 96 then dpi = 96 end
  local scale = dpi / 96.0
  local function S(px) return math.floor(px * scale + 0.5) end

  -- Studio Obsidian Palette v10.2 (BGR: 0xBBGGRR)
  local C_BG         = 0x170F0B -- #0B0F17 Midnight Obsidian
  local C_HDR        = 0x241811 -- #111824 Header Surface (Uniform behind ClearType text)
  local C_HDR_GLOW   = 0x36251A -- #1A2536 Top Rim Highlight
  local C_NAV        = 0x1A120C -- #0C121A Segmented Tab Strip
  local C_TAB_IDLE   = 0x221810 -- #101822 Idle Pill Tab
  local C_TAB_HOV    = 0x2E2117 -- #17212E Hovered Pill Tab
  local C_TAB_ACT    = 0x3A2A1C -- #1C2A3A Active Pill Tab
  local C_HERO       = 0x291D14 -- #141D29 Hero Telemetry Surface
  local C_CARD       = 0x2B1E15 -- #151E2B Action Card Surface
  local C_CARD_ACT   = 0x342819 -- #192834 Active Stateful Mode Surface
  local C_CARD_HOV   = 0x3D2B1F -- #1F2B3D Action Card Hover
  local C_CARD_FLASH = 0x543D2B -- #2B3D54 Action Card Click Flash
  local C_CHIP_BG    = 0x1F150E -- #0E151F Card Tag Chip Surface
  local C_CHIP_ACT   = 0x233E19 -- #193E23 Active Mode Badge Surface
  local C_BORDER     = 0x4A3627 -- #27364A Crisp 1px Slate Border
  local C_DIVIDER    = 0x36281C -- #1C2836 Subtle Section Line
  local C_FOOTER     = 0x1B130D -- #0D131B Footer Bar
  local C_TEXT       = 0xFCFAF8 -- #F8FAFC Pure Crisp White
  local C_SUBTEXT    = 0xC4B2A0 -- #A0B2C4 Balanced Slate-Silver
  local C_BLUE       = 0xF8BD38 -- #38BDF8 Sky Cyan
  local C_GREEN      = 0x99D334 -- #34D399 Vibrant Emerald
  local C_LIME       = 0x35E6A3 -- #A3E635 Lime Rating
  local C_GOLD       = 0x24BFFB -- #FBBF24 Amber Gold
  local C_PURPLE     = 0xFA8BA7 -- #A78BFA Soft Violet
  local C_CRIMSON    = 0x8571FB -- #FB7185 Rose Accent
  local C_HOOK_BG    = 0x294A1D -- #1D4A29 Emerald Pill Surface
  local C_HOOK_HOV   = 0x366326 -- #266336 Emerald Pill Hover

  -- Symmetric 940 x 636 logical client grid (zero rounding remainder on 2-col & 4-tab!)
  local W        = S(940)
  local H        = S(636)
  local PAD_X    = S(24)
  local CONT_W   = W - 2 * PAD_X
  local HDR_H    = S(72)
  local NAV_H    = S(52)
  local HERO_Y   = HDR_H + NAV_H + S(16)
  local HERO_H   = S(74)
  local GRID_Y   = HERO_Y + HERO_H + S(16)
  local GAP_X    = S(18)
  local GAP_Y    = S(14)
  local CARD_W   = math.floor((CONT_W - GAP_X) / 2)
  local CARD_H   = S(76)
  local FOOTER_H = S(42)
  local FOOTER_Y = H - FOOTER_H

  local f = createForm(false)
  f.Caption = "PES 2021 - v12.0 Ultimate Studio (PT-BR / EN)"
  pcall(function() f.BorderStyle = "bsSingle" end)
  f.Color = C_BG
  f.DoubleBuffered = true
  f.DoNotSaveInTable = true
  pcall(function() f.ClientWidth = W; f.ClientHeight = H end)
  if not f.ClientWidth or f.ClientWidth ~= W then
    f.Width = W; f.Height = H
  end
  f.centerScreen()
  f.OnClose = function() f.hide(); return caHide end

  local surface = createPanel(f)
  surface.Left = 0; surface.Top = 0; surface.Width = W; surface.Height = H
  surface.BevelOuter = "bvNone"; surface.BevelInner = "bvNone"
  surface.Color = C_BG; surface.Caption = ""
  surface.DoubleBuffered = true

  local backBuf = createBitmap(W, H)

  local function setFont(c, ptSize, color, bold)
    c.Font.Name = "Segoe UI"
    c.Font.Size = ptSize
    c.Font.Color = color or C_TEXT
    pcall(function()
      c.Font.Style = bold and "[fsBold]" or "[]"
      c.Font.Quality = "fqCleartypeNatural"
    end)
  end

  local function fitText(c, str, maxW)
    local s = tostring(str or "")
    if maxW <= S(16) then return "" end
    if c.getTextWidth(s) <= maxW then return s end
    local ell = "..."
    local targetW = maxW - c.getTextWidth(ell)
    if targetW <= 4 then return ell end
    local n = #s
    while n > 0 do
      local b = s:byte(n)
      n = n - 1
      if not b or b < 0x80 or b >= 0xC0 then
        local sub = s:sub(1, n)
        if c.getTextWidth(sub) <= targetW then
          return sub .. ell
        end
      end
    end
    return ell
  end

  local function drawTextLeft(c, x, y, bgCol, str, maxW)
    c.Brush.Color = bgCol
    local out = maxW and fitText(c, str, maxW) or tostring(str or "")
    c.textOut(x, y, out)
  end

  local function drawTextCentered(c, rx, ry, rw, rh, bgCol, str)
    c.Brush.Color = bgCol
    local out = fitText(c, str, rw - S(8))
    local tw = c.getTextWidth(out)
    local th = c.getTextHeight(out)
    local tx = rx + math.floor((rw - tw) / 2 + 0.5)
    local ty = ry + math.floor((rh - th) / 2 + 0.5)
    c.textOut(tx, ty, out)
  end

  local function drawRoundBox(c, x, y, w, h, rad, fillCol, borderCol)
    c.Brush.Color = fillCol
    c.Pen.Color = borderCol or fillCol
    c.Pen.Width = 1
    c.roundRect(x, y, x + w, y + h, rad, rad)
  end

  local function drawFlatRect(c, x, y, w, h, fillCol)
    c.Brush.Color = fillCol
    c.Pen.Color = fillCol
    c.Pen.Width = 1
    c.fillRect(x, y, x + w, y + h)
  end

  local function ovrBadgeColor(ovr)
    if ovr >= 90 then return C_GOLD, 0x16110B end
    if ovr >= 85 then return C_GREEN, 0x131A0D end
    if ovr >= 80 then return C_BLUE, 0x18120B end
    if ovr >= 75 then return C_LIME, 0x131A0D end
    return C_SUBTEXT, 0x170F0B
  end

  local TAB_DEFS = {
    { pt = "1. Master League", en = "1. Master League", accent = C_GREEN },
    { pt = "2. Jogador",       en = "2. Player",        accent = C_BLUE },
    { pt = "3. Treino & BAL",  en = "3. Training & BAL",accent = C_PURPLE },
    { pt = "4. Partida",       en = "4. Live Match",    accent = C_CRIMSON },
  }

  local CARDS_BY_TAB = {
    [1] = {
      { col=1, row=1, accent=C_GOLD,
        tag_pt="CAIXA", tag_en="BUDGET",
        title_pt="Central Financeira", title_en="Finances Hub",
        sub_pt="Define valores exatos de Transf. e Sal.", sub_en="Set exact Transfer & Wage budgets",
        fn=function() PES10.openFinancesModal() end },
      { col=2, row=1, accent=C_GREEN,
        tag_pt="ELENCO", tag_en="SQUAD",
        title_pt="Meu Elenco (Squad Studio)", title_en="My Squad (Roster Hub)",
        sub_pt="Lista completa, filtros e carregar atleta", sub_en="Full roster, filters & instant load",
        fn=function() PES10.openSquadRosterModal() end },
      { col=1, row=2, accent=C_BLUE,
        tag_pt="ELENCO", tag_en="SQUAD",
        title_pt="100% Fôlego no Elenco", title_en="100% Squad Stamina",
        sub_pt="Restaura fadiga de todo o time", sub_en="Restores fatigue for all players",
        fn=function() PES10.runSquadCommander("stamina_100") end },
      { col=2, row=2, accent=C_BLUE,
        tag_pt="FORMA", tag_en="FORM",
        title_pt="Seta Azul no Elenco", title_en="Top Form Arrow (Squad)",
        sub_pt="Condição máxima no próximo jogo", sub_en="Flashing blue arrow for next match",
        fn=function() PES10.runSquadCommander("blue_form") end },
      { col=1, row=3, accent=C_GREEN,
        tag_pt="SAÚDE", tag_en="HEALTH",
        title_pt="Curar Lesões + Afeição 100%", title_en="Heal Injuries + 100% Affection",
        sub_pt="Zera lesões e maximiza afeição", sub_en="Clears injuries and maxes affection",
        fn=function() PES10.runSquadCommander("heal_affection") end },
      { col=2, row=3, accent=C_GOLD,
        tag_pt="+5 ANOS", tag_en="+5 YRS",
        title_pt="Blindar Contratos (+5 Anos)", title_en="Shield Contracts (+5 Yrs)",
        sub_pt="Estende vínculos e remove multas", sub_en="Extends deals and removes clauses",
        fn=function() PES10.runSquadCommander("shield_contracts") end },
      { col=1, row=4, accent=C_GREEN,
        tag_pt="OLHEIROS", tag_en="SCOUTS",
        title_pt="Olheiros (1-3 & Favoritos)", title_en="Scout Lists (1-3 & Shortlist)",
        sub_pt="Adiciona/remove jogadores na RAM", sub_en="Live RAM scout & shortlist hub",
        fn=function() PES10.openScoutListsModal() end },
      { col=2, row=4, accent=C_CRIMSON,
        tag_pt="+1 OVR", tag_en="+1 OVR",
        title_pt="Elenco +1 OVR (DNA)", title_en="Squad +1 OVR (DNA)",
        sub_pt="Sobe +1 no time mantendo estilos", sub_en="Boosts squad by +1 keeping DNA",
        fn=function() PES10.runSquadCommander("plus_one_ovr_dna") end },
    },

    [2] = {
      { col=1, row=1, accent=C_LIME,
        tag_pt="POSIÇÕES", tag_en="POSITIONS",
        title_pt="Lab de Posições", title_en="Position Lab",
        sub_pt="OVR isolado (+/- / alvo) e Grau A/B/C", sub_en="Isolated OVR (+/- / target) & A/B/C",
        fn=function() PES10.openPositionLabModal() end },
      { col=2, row=1, accent=C_BLUE,
        tag_pt="25 STATS", tag_en="25 STATS",
        title_pt="Atributos (40-99)", title_en="Attributes (40-99)",
        sub_pt="Edição livre dos 25 atributos", sub_en="Free edit of all 25 stats",
        fn=function() PES10.openAttributesModal() end },
      { col=1, row=2, accent=C_PURPLE,
        tag_pt="SKILLS", tag_en="SKILLS",
        title_pt="Estilo, 41 Skills & 7 COM", title_en="Style, 41 Skills & 7 COM",
        sub_pt="Playing Style, Skills e Estilos IA COM", sub_en="Playing Style, Skills & COM AI Styles",
        fn=function() PES10.openSkillsStyleModal() end },
      { col=2, row=2, accent=C_GOLD,
        tag_pt="PAPEL", tag_en="ROLE",
        title_pt="Team Role (22 Papéis)", title_en="Team Role (22 Roles)",
        sub_pt="Função de equipe e nível (1-3)", sub_en="Squad role and level (1-3)",
        fn=function() PES10.openTeamRoleModal() end },
      { col=1, row=3, accent=C_GREEN,
        tag_pt="CONTRATO", tag_en="CONTRACT",
        title_pt="Contrato & Finanças", title_en="Contract & Finances",
        sub_pt="Salário, valor, multa e negociação", sub_en="Wage, value, clause & deal flags",
        fn=function() PES10.openContractModal() end },
      { col=2, row=3, accent=C_BLUE,
        tag_pt="PERFIL", tag_en="PROFILE",
        title_pt="Perfil, Nome & JSON", title_en="Profile, Name & JSON",
        sub_pt="Nome, camisa, físico, forma e backup", sub_en="Name, shirt, physical, form & backup",
        fn=function() PES10.openBioConditionModal() end },
      { col=1, row=4, accent=C_GOLD,
        tag_pt="OVR ALVO", tag_en="TARGET",
        title_pt="Overall Alvo (40-99)", title_en="Target Overall (40-99)",
        sub_pt="Escala OVR principal mantendo DNA", sub_en="Scales primary OVR keeping DNA",
        fn=function() PES10.promptTargetOVRProportional() end },
      { col=2, row=4, accent=C_GREEN,
        tag_pt="FORMA", tag_en="FORM",
        title_pt="Fôlego 100% & Seta de Forma", title_en="Stamina 100% & Form Arrow",
        sub_pt="Ajuste rápido de fôlego, seta e lesão", sub_en="Quick stamina, form arrow & injury",
        fn=function() PES10.openQuickConditionModal() end },
    },

    [3] = {
      { col=1, row=1, accent=C_GREEN,
        tag_pt="1X-25X", tag_en="1X-25X",
        title_pt="Velocidade de Treino", title_en="Training Speed",
        sub_pt="Escolhe 1x, 2x, 5x, 10x, 25x ou 1-Dia", sub_en="Pick 1x, 2x, 5x, 10x, 25x or Instant",
        isActive=function(tel) return tel.trnMode >= 1 and tel.trnMode <= 4 end,
        fn=function() PES10.openTrainingSpeedModal() end },
      { col=2, row=1, accent=C_GOLD,
        tag_pt="1 DIA", tag_en="1-DAY",
        title_pt="Treino Instantâneo", title_en="Instant Training",
        sub_pt="10.000 XP/dia (sobe em 1 avanço)", sub_en="10,000 XP/day (level up in 1 day)",
        isActive=function(tel) return tel.trnMode == 5 end,
        fn=function()
          local a = PES10.symAddr("cfgTrainingMultMode")
          if a ~= 0 then writeInteger(a, 5); PES10.ensureHookActive(3); PES10.toast(PES10.T("Treino Instantâneo ativado!", "Instant Training active!")) end
        end },
      { col=1, row=2, accent=C_BLUE,
        tag_pt="1X", tag_en="1X",
        title_pt="Treino Normal (1x)", title_en="Normal Training (1x)",
        sub_pt="Restaura velocidade original", sub_en="Restores default training speed",
        isActive=function(tel) return tel.attached and tel.trnMode == 0 end,
        fn=function()
          local a = PES10.symAddr("cfgTrainingMultMode")
          if a ~= 0 then writeInteger(a, 0); PES10.toast(PES10.T("Treino restaurado (1x).", "Training restored (1x).")) end
        end },
      { col=2, row=2, accent=C_PURPLE,
        tag_pt="99% XP", tag_en="99% XP",
        title_pt="Encher Sub-XP do Atleta", title_en="Fill Player Sub-XP",
        sub_pt="25 barras em 9.999 / 10.000", sub_en="All 25 bars set to 9,999",
        fn=function() PES10.runPlayerMacro("instant_subxp_player") end },
      { col=1, row=3, accent=C_BLUE,
        tag_pt="BAL", tag_en="BAL",
        title_pt="Selecionar Meu Pro (BAL)", title_en="Select My BAL Pro",
        sub_pt="Carrega seu atleta do Estrelato", sub_en="Loads your BAL player",
        fn=function() PES10.pointToBALMyPlayer() end },
      { col=2, row=3, accent=C_GOLD,
        tag_pt="FOCUS", tag_en="FOCUS",
        title_pt="BAL: Focus Points (0-99)", title_en="BAL: Focus Points (0-99)",
        sub_pt="Valor exato, trava ou 10 barras max", sub_en="Exact value, lock or max 10 sliders",
        fn=function() PES10.openBALFocusModal() end },
      { col=1, row=4, accent=C_GREEN,
        tag_pt="SKILL", tag_en="SKILL",
        title_pt="BAL: Concluir Skill", title_en="BAL: Finish Skill",
        sub_pt="Skill em treino vai para 99,99%", sub_en="Active skill XP to 99.99%",
        fn=function() PES10.runPlayerMacro("bal_instant_skill") end },
      { col=2, row=4, accent=C_GREEN,
        tag_pt="POS", tag_en="POS",
        title_pt="BAL: Concluir Posição", title_en="BAL: Finish Position",
        sub_pt="Posição em treino vai para 99,99%", sub_en="Active position XP to 99.99%",
        fn=function() PES10.runPlayerMacro("bal_instant_pos") end },
    },

    [4] = {
      { col=1, row=1, accent=C_GOLD,
        tag_pt="MINUTO", tag_en="CLOCK",
        title_pt="Definir Minuto Exato", title_en="Set Exact Minute",
        sub_pt="Pula p/ qualquer minuto (15, 44, 88)", sub_en="Jump to any minute (15, 44, 88)",
        fn=function()
          local info = PES10.getMatchClockInfo()
          if not info then PES10.toast(PES10.T("Entre em uma partida primeiro!", "Enter a live match first!")); return end
          PES10.openDarkPrompt(
            PES10.T("Minuto da Partida", "Match Minute"),
            PES10.T("Digite o minuto (ex: 15, 44, 88 ou 44:30):", "Enter minute (e.g. 15, 44, 88 or 44:30):"),
            tostring(info.mm),
            0x24BFFB,
            function(ans)
              local mm, ss = ans:match("^(%d+)%s*:%s*(%d+)$")
              local tMin = mm and (tonumber(mm) + tonumber(ss) / 60.0) or tonumber((ans:gsub(",", ".")))
              if tMin then PES10.setExactMatchMinute(tMin) end
            end
          )
        end },
      { col=2, row=1, accent=C_BLUE,
        tag_pt="44/88", tag_en="44/88",
        title_pt="Pular p/ Fim (44' / 88')", title_en="Jump to End (44' / 88')",
        sub_pt="1ºT -> 44:00  •  2ºT -> 88:00", sub_en="1st -> 44:00  •  2nd -> 88:00",
        fn=function()
          local info = PES10.getMatchClockInfo()
          if not info then PES10.toast(PES10.T("Entre em uma partida primeiro!", "Enter a live match first!")); return end
          PES10.setExactMatchMinute(info.period == 1 and 44 or 88)
        end },
      { col=1, row=2, accent=C_BLUE,
        tag_pt="00:00", tag_en="00:00",
        title_pt="Reiniciar Tempo Atual", title_en="Restart Current Half",
        sub_pt="Zera cronômetro da etapa atual", sub_en="Resets current half clock",
        fn=function() PES10.resetCurrentHalfNow() end },
      { col=2, row=2, accent=C_CRIMSON,
        tag_pt="APITO", tag_en="END",
        title_pt="Encerrar Tempo Agora", title_en="End Half Now",
        sub_pt="Aciona o apito imediatamente", sub_en="Triggers referee whistle now",
        fn=function() PES10.endCurrentHalfNow() end },
      { col=1, row=3, accent=C_GREEN,
        tag_pt="PLACAR", tag_en="SCORE",
        title_pt="Editar Placar ao Vivo", title_en="Edit Live Score",
        sub_pt="Define gols de Casa x Fora", sub_en="Set Home - Away goals",
        fn=function()
          PES10.ensureHookActive(5)
          local mCore = PES10.readU64(PES10.symAddr("ptrMatchCore"))
          local mHud = PES10.readU64(PES10.symAddr("ptrMatchHud"))
          if mCore == 0 and mHud == 0 then PES10.toast(PES10.T("Entre em uma partida primeiro!", "Enter a live match first!")); return end
          local hCur = (mCore ~= 0) and PES10.readU8(mCore + 0x9E) or 0
          local aCur = (mCore ~= 0) and PES10.readU8(mCore + 0x9F) or 0
          PES10.openDarkPrompt(
            PES10.T("Placar (Casa x Fora)", "Score (Home - Away)"),
            PES10.T("Formato CASA-FORA (ex: 3-0):", "Format HOME-AWAY (e.g. 3-0):"),
            string.format("%d-%d", hCur, aCur),
            0x99D334,
            function(ans)
              local hg, ag = ans:match("^(%d+)%s*[%-%s:xX]%s*(%d+)$")
              if hg and ag then
                local hi, ai = tonumber(hg), tonumber(ag)
                if mCore ~= 0 then writeBytes(mCore + 0x9E, hi); writeBytes(mCore + 0x9F, ai) end
                if mHud ~= 0 then writeInteger(mHud + 0x120, hi); writeInteger(mHud + 0x124, ai) end
                PES10.toast(string.format(PES10.T("Placar: %d x %d!", "Score: %d - %d!"), hi, ai))
              end
            end
          )
        end },
      { col=2, row=3, accent=C_PURPLE,
        tag_pt="PAUSA", tag_en="FREEZE",
        title_pt="Congelar Relógio", title_en="Freeze Clock",
        sub_pt="Pausa ou retoma o cronômetro", sub_en="Toggles match clock pause",
        isActive=function(tel) return tel.frzMode == 1 end,
        fn=function()
          local a = PES10.symAddr("cfgFreezeClock")
          local info = PES10.getMatchClockInfo()
          if a == 0 or not info then PES10.toast(PES10.T("Entre em uma partida primeiro!", "Enter a live match first!")); return end
          PES10.ensureHookActive(5)
          local cur = PES10.readU32(a)
          if cur == 0 then
            writeFloat(PES10.symAddr("valFrozenTotalTicks"), info.halfTicks)
            writeInteger(a, 1)
            PES10.toast(PES10.T("Relógio congelado em ", "Clock frozen at ") .. info.formatted)
          else
            writeInteger(a, 0)
            PES10.toast(PES10.T("Relógio descongelado!", "Clock unfrozen!"))
          end
        end },
      { col=1, row=4, accent=C_GREEN,
        tag_pt="CASA", tag_en="HOME",
        title_pt="Stamina 100% (Casa)", title_en="100% Stamina (Home)",
        sub_pt="Liga ou desliga fôlego infinito", sub_en="Toggle infinite Home stamina",
        isActive=function(tel) return tel.stamMode == 1 end,
        fn=function()
          local a = PES10.symAddr("cfgStaminaMode")
          if a == 0 then return end
          if PES10.readU32(a) == 1 then
            writeInteger(a, 0)
            PES10.toast(PES10.T("Stamina restaurada.", "Stamina restored."))
          else
            writeInteger(a, 1); PES10.ensureHookActive(6)
            PES10.toast(PES10.T("Stamina 100% (Casa) ativa!", "Home 100% Stamina active!"))
          end
        end },
      { col=2, row=4, accent=C_CRIMSON,
        tag_pt="MODOS", tag_en="MODES",
        title_pt="Modos de Stamina (6)", title_en="Stamina Modes (6)",
        sub_pt="Normal, Casa, Fora, Ambos e Drenar CPU", sub_en="Normal, Home, Away, Both & Drain CPU",
        isActive=function(tel) return tel.stamMode >= 2 end,
        fn=function() PES10.openMatchStaminaModal() end },
    },
  }

  local TRN_NAMES_PT  = { [0]="1x Original", [1]="2x Dobrado", [2]="5x Rápido", [3]="10x Super", [4]="50x Turbo", [5]="Instantâneo (10.000 XP)" }
  local TRN_NAMES_EN  = { [0]="1x Default",  [1]="2x Double",  [2]="5x Fast",   [3]="10x Super", [4]="50x Turbo", [5]="Instant (10,000 XP)" }
  local STAM_NAMES_PT = { [0]="Original", [1]="Só Casa 100%", [2]="Só Fora 100%", [3]="Ambos 100%", [4]="Pressão Casa", [5]="Pressão Fora" }
  local STAM_NAMES_EN = { [0]="Default",  [1]="Home 100%",    [2]="Away 100%",    [3]="Both 100%",  [4]="Home Press",   [5]="Away Press" }

  local currentTab = 1
  local hoverId    = nil
  local flashId    = nil
  local hitboxes   = {}
  local lastSig    = ""

  local function countActiveHooks()
    local al = getAddressList()
    if not al then return 0 end
    local cnt = 0
    for num = 1, 6 do
      local id = PES10.HOOK_IDS_BY_NUM and PES10.HOOK_IDS_BY_NUM[num]
      local mr = id and al.getMemoryRecordByID(id) or nil
      if mr and mr.Active then cnt = cnt + 1 end
    end
    return cnt
  end

  local function collectTelemetry()
    local c = PES10.getCurrentCurrency()
    local hooksOn = countActiveHooks()
    local isEn = (PES10.lang == "en")
    local trnTbl = isEn and TRN_NAMES_EN or TRN_NAMES_PT
    local stamTbl = isEn and STAM_NAMES_EN or STAM_NAMES_PT

    local t = {
      lang     = PES10.lang,
      attached = PES10.isAttached(),
      currIdx  = PES10.currencyIndex,
      currSym  = c.symbol,
      currCode = c.code,
      hooksOn  = hooksOn,
      trnMode  = 0,
      stamMode = 0,
      frzMode  = 0,
      toast    = PES10.lastToastMsg,
      hdrSub   = "",
      mlTop    = PES10.T("FINANÇAS DO CLUBE & CALENDÁRIO MASTER LEAGUE", "CLUB FINANCES & MASTER LEAGUE CALENDAR"),
      mlMain   = PES10.T("Nenhuma Carreira carregada - Abra sua Master League ou BAL.", "No Career loaded - Open your Master League or BAL save."),
      pHas     = false,
      pOVR     = 0,
      pPos     = "OVR",
      pUndo    = 0,
      pTop     = PES10.T("Nenhum atleta selecionado no momento", "No player currently selected"),
      pSub     = PES10.T("Passe o cursor sobre qualquer jogador no Plano de Jogo, Lista do Elenco ou Negociações.", "Select or hover any player in Game Plan, Squad List or Negotiations."),
      trnTop   = PES10.T("MOTOR DE EVOLUÇÃO SUB-XP (EXCLUSIVO SEU CLUBE, BASE & BAL)", "SUB-XP PROGRESSION ENGINE (YOUR CLUB, YOUTH & BAL ONLY)"),
      trnMain  = PES10.T("Velocidade de Treino: 1x Original     •     BAL Focus Points: 0", "Training Speed: 1x Default     •     BAL Focus Points: 0"),
      mTop     = PES10.T("TELEMETRIA DE PARTIDA AO VIVO (SIDER-SAFE)", "LIVE MATCH TELEMETRY (SIDER-SAFE)"),
      mMain    = PES10.T("Fora de Partida     •     Modo Stamina Configurado: Original", "Not in Match     •     Configured Stamina Mode: Default"),
    }

    if not t.attached then
      t.hdrSub = string.format(
        PES10.T("○ Aguardando PES2021.exe...   •   Moeda: %s (%s)", "○ Waiting for PES2021.exe...   •   Currency: %s (%s)"),
        c.symbol, c.code
      )
      return t
    end

    local modeStr, clubStr, career = PES10.getCareerModeAndClubInfo()
    local shortMode = modeStr
    if modeStr == "Master League" and clubStr ~= "" then
      local cleanClub = clubStr:gsub("%s*%(ID %d+%)", "")
      shortMode = "ML: " .. cleanClub
    elseif clubStr ~= "" then
      shortMode = modeStr .. " - " .. clubStr:gsub("%s*%(ID %d+%)", "")
    end
    local p = PES10.readU64(PES10.symAddr("ptrPlayer"))
    local pSummary = PES10.T("Sem Atleta", "No Player")
    local pName = PES10.T("Nenhum", "None")
    if p ~= 0 then
      pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
      local hOvr, hPos = PES10.calcPlayerOVR(p)
      pSummary = string.format("%s (%d %s)", pName, hOvr, PES10.posShort(hPos))
    end
    t.hdrSub = string.format("● Online  •  %s  •  %s", shortMode, pSummary)

    if career ~= 0 then
      local tb = PES10.formatMoneyFromRaw(PES10.readI32(career + 0x016ECBF4))
      local availSal = PES10.getAvailableSalaryBudget(career, false)
      local sb = PES10.formatMoneyFromRaw(availSal)
      local yr = PES10.readU16(career + 0x01642A24)
      local mo = PES10.readU8(career + 0x01642A26)
      local dy = PES10.readU8(career + 0x01642A27)
      t.mlMain = string.format(
        PES10.T(
          "Transferências: %s     •     Salários: %s     •     Data: %02d/%02d/%04d",
          "Transfer Budget: %s     •     Wage Budget: %s     •     Date: %04d-%02d-%02d"
        ),
        tb, sb, isEn and yr or dy, mo, isEn and dy or yr
      )
    end

    if p ~= 0 then
      t.pHas = true
      local ovr, regPos, posRank = PES10.getPlayerBestPositions(p)
      local age = PES10.readU8(p + 0x1C) & 0x3F
      local aff = PES10.getBitsInDword(p + 0x13C, 18, 7)
      local _, _, pClub = PES10.decodeClubHandle(PES10.readU32(p + 0x12C))
      local pSal = PES10.formatMoneyFromRaw(PES10.readI32(p + 0x15C))
      local pMkt = PES10.formatMoneyFromRaw(PES10.readI32(p + 0x174))
      local rfeeRaw = PES10.readI32(PES10.symAddr("valComputedReleaseFee"))
      local rfeeStr = (rfeeRaw > 0) and PES10.formatMoneyFromRaw(rfeeRaw) or PES10.T("Sem Multa", "No Clause")
      t.pOVR = ovr
      t.pPos = PES10.posShort(regPos)
      t.pUndo = PES10.canUndoPlayer(p)
      local bestHint = ""
      if posRank[1] and posRank[1].ovr > ovr then
        bestHint = string.format("  •  ★ %d %s", posRank[1].ovr, PES10.posShort(posRank[1].pos))
      end
      t.pTop = string.format(
        PES10.T("%s  •  %d anos  •  %s%s", "%s  •  %d yo  •  %s%s"),
        pName, age, pClub, bestHint
      )
      t.pSub = string.format(
        PES10.T(
          "Sal: %s  •  Merc: %s  •  Multa: %s  •  Af: %d%%",
          "Wage: %s  •  Val: %s  •  Clause: %s  •  Aff: %d%%"
        ),
        pSal, pMkt, rfeeStr, aff
      )
    end

    local tm = PES10.readU32(PES10.symAddr("cfgTrainingMultMode"))
    local fp = (career ~= 0) and PES10.readU8(career + 0x016ECBEA) or 0
    t.trnMode = tm
    t.trnMain = string.format(
      PES10.T(
        "Velocidade de Treino: %s     •     BAL Focus Points: %d",
        "Training Speed: %s     •     BAL Focus Points: %d"
      ),
      trnTbl[tm] or trnTbl[0], fp
    )

    local clk = PES10.getMatchClockInfo()
    local sm = PES10.readU32(PES10.symAddr("cfgStaminaMode"))
    local fz = PES10.readU32(PES10.symAddr("cfgFreezeClock"))
    t.stamMode = sm
    t.frzMode  = fz
    if clk then
      local mCore = PES10.readU64(PES10.symAddr("ptrMatchCore"))
      local sc = (mCore ~= 0) and string.format("%d x %d", PES10.readU8(mCore + 0x9E), PES10.readU8(mCore + 0x9F)) or PES10.T("Ao Vivo", "Live")
      t.mMain = string.format(
        PES10.T(
          "Cronômetro: %s     •     Placar: %s     •     Stamina: %s",
          "Match Clock: %s     •     Score: %s     •     Stamina: %s"
        ),
        clk.formatted, sc, stamTbl[sm] or stamTbl[0]
      )
    else
      t.mMain = PES10.T("Fora de Partida     •     Modo Stamina Configurado: ", "Not in Match     •     Configured Stamina Mode: ") .. (stamTbl[sm] or stamTbl[0])
    end

    return t
  end

  local function renderAll(forceRepaint)
    local tel = collectTelemetry()
    local sig = table.concat({
      tel.lang, tostring(currentTab), tostring(hoverId), tostring(flashId),
      tostring(tel.currIdx), tostring(tel.hooksOn),
      tostring(tel.trnMode), tostring(tel.stamMode), tostring(tel.frzMode),
      tel.hdrSub, tel.mlMain, tostring(tel.pOVR), tostring(tel.pUndo), tel.pTop, tel.pSub, tel.trnMain, tel.mMain, tel.toast
    }, "|")
    if not forceRepaint and sig == lastSig then return end
    lastSig = sig

    local isEn = (tel.lang == "en")
    local c = backBuf.Canvas
    hitboxes = {}
    local function addHitbox(id, x, y, w, h, fn)
      hitboxes[#hitboxes + 1] = { id = id, x = x, y = y, w = w, h = h, fn = fn }
    end

    -- 0. Global Midnight Surface
    drawFlatRect(c, 0, 0, W, H, C_BG)

    -- 1. HEADER BAR & GLOBAL CONTROLS (Hooks + Currencies + PT|EN Toggle)
    drawFlatRect(c, 0, 0, W, HDR_H, C_HDR)
    drawFlatRect(c, 0, 0, W, S(2), C_HDR_GLOW)
    drawFlatRect(c, 0, HDR_H - S(2), W, S(2), TAB_DEFS[currentTab].accent)

    local pillH   = S(36)
    local pillY   = S(18)
    local segW    = S(35)
    local langW   = segW * 2
    local langX   = W - PAD_X - langW
    drawRoundBox(c, langX, pillY, langW, pillH, S(8), C_TAB_IDLE, C_BORDER)
    local ptAct   = not isEn
    local ptHov   = (hoverId == "btn_lang_pt")
    local enHov   = (hoverId == "btn_lang_en")
    local ptBg    = ptAct and C_BLUE or (ptHov and C_CARD_HOV or C_TAB_IDLE)
    local enBg    = isEn  and C_BLUE or (enHov and C_CARD_HOV or C_TAB_IDLE)
    drawRoundBox(c, langX, pillY, segW, pillH, S(8), ptBg, ptAct and C_BLUE or C_BORDER)
    drawRoundBox(c, langX + segW, pillY, segW, pillH, S(8), enBg, isEn and C_BLUE or C_BORDER)
    setFont(c, 8, ptAct and C_BG or C_SUBTEXT, true)
    drawTextCentered(c, langX, pillY, segW, pillH, ptBg, "PT")
    setFont(c, 8, isEn and C_BG or C_SUBTEXT, true)
    drawTextCentered(c, langX + segW, pillY, segW, pillH, enBg, "EN")
    addHitbox("btn_lang_pt", langX, pillY, segW, pillH, function() PES10.setLang("pt") end)
    addHitbox("btn_lang_en", langX + segW, pillY, segW, pillH, function() PES10.setLang("en") end)

    local pillW, pillGap = S(58), S(6)
    local currStartX = langX - S(10) - (4 * pillW + 3 * pillGap)
    local currDefs = { {0, "€ EUR"}, {1, "R$ BRL"}, {2, "$ USD"}, {3, "£ GBP"} }
    for idx, cd in ipairs(currDefs) do
      local cIdx = cd[1]
      local px = currStartX + (idx - 1) * (pillW + pillGap)
      local id = "curr_" .. tostring(cIdx)
      local isAct = (tel.currIdx == cIdx)
      local isHov = (hoverId == id)
      local bg = isAct and C_GOLD or (isHov and C_CARD_HOV or C_CARD)
      local bd = isAct and C_GOLD or (isHov and C_BLUE or C_BORDER)
      drawRoundBox(c, px, pillY, pillW, pillH, S(8), bg, bd)
      setFont(c, 8, isAct and C_BG or C_TEXT, true)
      drawTextCentered(c, px, pillY, pillW, pillH, bg, cd[2])
      addHitbox(id, px, pillY, pillW, pillH, function() PES10.setCurrency(cIdx) end)
    end

    local maxHdrTextW = currStartX - PAD_X - S(16)
    setFont(c, 12, C_TEXT, true)
    drawTextLeft(c, PAD_X, S(12), C_HDR, "PES 2021  •  v12.0 ULTIMATE STUDIO", maxHdrTextW)
    setFont(c, 8, tel.attached and C_GREEN or C_SUBTEXT, false)
    drawTextLeft(c, PAD_X, S(41), C_HDR, tel.hdrSub, maxHdrTextW)

    -- 2. SEGMENTED 4-TAB BAR
    drawFlatRect(c, 0, HDR_H, W, NAV_H, C_NAV)
    drawFlatRect(c, 0, HDR_H + NAV_H - 1, W, 1, C_DIVIDER)
    local tabGap = S(12)
    local tabW   = math.floor((CONT_W - 3 * tabGap) / 4)
    local tabH   = S(36)
    local tabY   = HDR_H + S(8)
    for i, td in ipairs(TAB_DEFS) do
      local tx = PAD_X + (i - 1) * (tabW + tabGap)
      local id = "tab_" .. tostring(i)
      local isAct = (currentTab == i)
      local isHov = (hoverId == id)
      local bg = isAct and C_TAB_ACT or (isHov and C_TAB_HOV or C_TAB_IDLE)
      local bd = isAct and td.accent or (isHov and C_BORDER or C_DIVIDER)
      drawRoundBox(c, tx, tabY, tabW, tabH, S(8), bg, bd)
      if isAct then
        drawFlatRect(c, tx + S(14), tabY + tabH - S(3), tabW - S(28), S(2), td.accent)
      end
      setFont(c, 9, isAct and C_TEXT or C_SUBTEXT, true)
      drawTextCentered(c, tx, tabY, tabW, tabH - S(2), bg, isEn and td.en or td.pt)
      local idxCopy = i
      addHitbox(id, tx, tabY, tabW, tabH, function() currentTab = idxCopy end)
    end

    -- 3. HERO TELEMETRY BANNER
    local activeAccent = TAB_DEFS[currentTab].accent
    drawRoundBox(c, PAD_X, HERO_Y, CONT_W, HERO_H, S(10), C_HERO, C_BORDER)
    drawRoundBox(c, PAD_X + S(10), HERO_Y + S(14), S(4), HERO_H - S(28), S(2), activeAccent, activeAccent)

    if currentTab == 2 then
      -- Clickable OVR Shield Badge (opens 13-Pos & DNA X-Ray!)
      local bx, by, bw, bh = PAD_X + S(22), HERO_Y + S(10), S(58), S(54)
      local bCol, bTxtCol = ovrBadgeColor(tel.pHas and tel.pOVR or 0)
      local badgeHov = (hoverId == "hero_ovr_badge")
      drawRoundBox(c, bx, by, bw, bh, S(8), bCol, badgeHov and C_TEXT or bCol)
      setFont(c, 13, bTxtCol, true)
      drawTextCentered(c, bx, by + S(3), bw, S(30), bCol, tel.pHas and tostring(tel.pOVR) or "--")
      setFont(c, 7, bTxtCol, true)
      drawTextCentered(c, bx, by + S(32), bw, S(18), bCol, tel.pPos)
      addHitbox("hero_ovr_badge", bx, by, bw, bh, function() PES10.showPlayerDNAReport() end)

      local undoTxt = (tel.pUndo > 0)
        and string.format(PES10.T("Desfazer (%d)", "Undo (%d)"), tel.pUndo)
        or PES10.T("Desfazer", "Undo")
      local stepDefs = {
        { id="hero_ovr_dn",  txt="-1 OVR",                     w=S(62),  bg=C_CARD,    bd=C_BORDER, fn=function() PES10.stepPlayerOVRProportional(-1) end },
        { id="hero_ovr_up",  txt="+1 OVR",                     w=S(66),  bg=C_HOOK_BG, bd=C_GREEN,  fn=function() PES10.stepPlayerOVRProportional(1) end },
        { id="hero_undo",    txt=undoTxt,                      w=S(102), bg=(tel.pUndo > 0 and C_TAB_ACT or C_CARD), bd=(tel.pUndo > 0 and C_GOLD or C_BORDER), fn=function() PES10.undoLastPlayerEdit() end },
        { id="hero_dna_x",   txt=PES10.T("Raio-X", "X-Ray"),   w=S(64),  bg=C_CARD,    bd=C_PURPLE, fn=function() PES10.showPlayerDNAReport() end },
      }
      local sGap = S(6)
      local totalStepW = stepDefs[1].w + stepDefs[2].w + stepDefs[3].w + stepDefs[4].w + 3 * sGap
      local sx = PAD_X + CONT_W - S(14) - totalStepW
      local sy, sh = HERO_Y + S(19), S(36)
      local curSx = sx
      for _, sd in ipairs(stepDefs) do
        local isHov = (hoverId == sd.id)
        local pBg = isHov and C_CARD_HOV or sd.bg
        drawRoundBox(c, curSx, sy, sd.w, sh, S(8), pBg, isHov and C_TEXT or sd.bd)
        setFont(c, 8, C_TEXT, true)
        drawTextCentered(c, curSx, sy, sd.w, sh, pBg, sd.txt)
        addHitbox(sd.id, curSx, sy, sd.w, sh, sd.fn)
        curSx = curSx + sd.w + sGap
      end

      local tx = bx + bw + S(14)
      local maxHeroW = sx - tx - S(14)
      setFont(c, 10, C_TEXT, true)
      drawTextLeft(c, tx, HERO_Y + S(14), C_HERO, tel.pTop, maxHeroW)
      setFont(c, 8, C_SUBTEXT, false)
      drawTextLeft(c, tx, HERO_Y + S(42), C_HERO, tel.pSub, maxHeroW)
    else
      local topLbl = (currentTab == 1 and tel.mlTop) or (currentTab == 3 and tel.trnTop) or tel.mTop
      local mainLbl = (currentTab == 1 and tel.mlMain) or (currentTab == 3 and tel.trnMain) or tel.mMain
      local tx = PAD_X + S(24)
      local maxW = CONT_W - S(40)
      setFont(c, 8, activeAccent, true)
      drawTextLeft(c, tx, HERO_Y + S(13), C_HERO, topLbl, maxW)
      setFont(c, 10, C_TEXT, true)
      drawTextLeft(c, tx, HERO_Y + S(38), C_HERO, mainLbl, maxW)
    end

    -- 4. 2x4 ACTION CARDS GRID (WITH STATEFUL ACTIVE GLOW)
    local cards = CARDS_BY_TAB[currentTab] or {}
    for idx, cd in ipairs(cards) do
      local cx = PAD_X + (cd.col - 1) * (CARD_W + GAP_X)
      local cy = GRID_Y + (cd.row - 1) * (CARD_H + GAP_Y)
      local id = string.format("card_%d_%d", currentTab, idx)
      local isFlash = (flashId == id)
      local isHov   = (hoverId == id)
      local isAct   = (cd.isActive and cd.isActive(tel)) or false

      local bg = isFlash and C_CARD_FLASH or (isHov and C_CARD_HOV or (isAct and C_CARD_ACT or C_CARD))
      local bd = (isFlash or isHov or isAct) and cd.accent or C_BORDER

      drawRoundBox(c, cx, cy, CARD_W, CARD_H, S(10), bg, bd)
      drawRoundBox(c, cx + S(11), cy + S(14), S(4), CARD_H - S(28), S(2), cd.accent, cd.accent)

      local chipTxt = isAct and PES10.T("● ATIVO", "● ACTIVE") or (isEn and cd.tag_en or cd.tag_pt)
      local chipBg  = isAct and C_CHIP_ACT or C_CHIP_BG
      local chipBd  = isAct and C_GREEN or C_DIVIDER
      local chipCol = isAct and C_GREEN or cd.accent

      setFont(c, 7, chipCol, true)
      local chipW = c.getTextWidth(chipTxt) + S(16)
      local chipH = S(20)
      local chipX = cx + CARD_W - chipW - S(14)
      local chipY = cy + S(13)
      drawRoundBox(c, chipX, chipY, chipW, chipH, S(6), chipBg, chipBd)
      drawTextCentered(c, chipX, chipY, chipW, chipH, chipBg, chipTxt)

      local titleStr  = isEn and cd.title_en or cd.title_pt
      local subStr    = isEn and cd.sub_en or cd.sub_pt
      local maxTitleW = chipX - (cx + S(24)) - S(10)
      local maxSubW   = CARD_W - S(38)
      setFont(c, 10, C_TEXT, true)
      drawTextLeft(c, cx + S(24), cy + S(13), bg, titleStr, maxTitleW)
      setFont(c, 8, C_SUBTEXT, false)
      drawTextLeft(c, cx + S(24), cy + S(44), bg, subStr, maxSubW)

      addHitbox(id, cx, cy, CARD_W, CARD_H, cd.fn)
    end

    -- 5. FOOTER STATUS & JSON BACKUP SHORTCUTS
    drawFlatRect(c, 0, FOOTER_Y, W, FOOTER_H, C_FOOTER)
    drawFlatRect(c, 0, FOOTER_Y, W, 1, C_DIVIDER)

    local jBtnW, jBtnH = S(112), S(26)
    local impX = W - PAD_X - jBtnW
    local expX = impX - S(8) - jBtnW
    local jBtnY = FOOTER_Y + math.floor((FOOTER_H - jBtnH) / 2 + 0.5)

    local expHov = (hoverId == "ftr_exp")
    local impHov = (hoverId == "ftr_imp")
    local expBg  = expHov and C_CARD_HOV or C_CARD
    local impBg  = impHov and C_CARD_HOV or C_CARD
    drawRoundBox(c, expX, jBtnY, jBtnW, jBtnH, S(6), expBg, expHov and C_BLUE or C_BORDER)
    drawRoundBox(c, impX, jBtnY, jBtnW, jBtnH, S(6), impBg, impHov and C_BLUE or C_BORDER)
    setFont(c, 8, expHov and C_TEXT or C_SUBTEXT, true)
    drawTextCentered(c, expX, jBtnY, jBtnW, jBtnH, expBg, PES10.T("Exportar .JSON", "Export .JSON"))
    setFont(c, 8, impHov and C_TEXT or C_SUBTEXT, true)
    drawTextCentered(c, impX, jBtnY, jBtnW, jBtnH, impBg, PES10.T("Importar .JSON", "Import .JSON"))
    addHitbox("ftr_exp", expX, jBtnY, jBtnW, jBtnH, function() PES10.exportPlayerBackupJson() end)
    addHitbox("ftr_imp", impX, jBtnY, jBtnW, jBtnH, function() PES10.importPlayerBackupJson() end)

    local maxToastW = expX - PAD_X - S(16)
    setFont(c, 9, C_GREEN, false)
    local th = c.getTextHeight(tel.toast)
    drawTextLeft(c, PAD_X, FOOTER_Y + math.floor((FOOTER_H - th) / 2 + 0.5), C_FOOTER, tel.toast, maxToastW)

    surface.repaint()
  end

  local function findHitboxAt(cx, cy)
    if not cx or not cy then return nil end
    for i = #hitboxes, 1, -1 do
      local hb = hitboxes[i]
      if cx >= hb.x and cx < (hb.x + hb.w) and cy >= hb.y and cy < (hb.y + hb.h) then
        return hb
      end
    end
    return nil
  end

  local function updateHoverAt(cx, cy)
    local hb = findHitboxAt(cx, cy)
    local newHover = hb and hb.id or nil
    if newHover ~= hoverId then
      hoverId = newHover
      pcall(function() surface.Cursor = hoverId and -21 or 0 end)
      renderAll(true)
    end
  end

  local function handleClickAt(cx, cy)
    local hb = findHitboxAt(cx, cy)
    if not hb then return false end
    flashId = hb.id
    renderAll(true)
    local t = createTimer(f, true)
    t.Interval = 130
    t.OnTimer = function(tmr)
      tmr.destroy()
      flashId = nil
      renderAll(true)
    end
    pcall(hb.fn)
    renderAll(true)
    return true
  end

  surface.OnPaint = function()
    surface.Canvas.draw(0, 0, backBuf)
  end

  surface.OnClick = function()
    local mx, my = getMousePos()
    local cx, cy = surface.screenToClient(mx, my)
    handleClickAt(cx, cy)
  end

  local function isHoverBlocked(mx, my)
    if f.Active == false then return true end
    if PES10.openModals then
      for _, m in pairs(PES10.openModals) do
        if m then
          local okVis, vis = pcall(function() return m.Visible end)
          if okVis and vis then
            local okRect, ml, mt, mw, mh = pcall(function() return m.Left, m.Top, m.Width, m.Height end)
            if okRect and ml and mt and mw and mh and mx >= ml and mx < (ml + mw) and my >= mt and my < (mt + mh) then
              return true
            end
          end
        end
      end
    end
    return false
  end

  local tickCounter = 0
  local uiTimer = createTimer(f, true)
  uiTimer.Interval = 75
  uiTimer.OnTimer = function()
    if not f.Visible then return end
    if getMousePos then
      local mx, my = getMousePos()
      if mx and my then
        if isHoverBlocked(mx, my) then
          if hoverId ~= nil then updateHoverAt(-1, -1) end
        else
          local cx, cy = surface.screenToClient(mx, my)
          if cx and cy and cx >= 0 and cx < W and cy >= 0 and cy < H then
            updateHoverAt(cx, cy)
          elseif hoverId ~= nil then
            updateHoverAt(-1, -1)
          end
        end
      end
    end
    tickCounter = tickCounter + 1
    if tickCounter >= 5 then
      tickCounter = 0
      renderAll(false)
    end
  end

  PES10.dashboardState = {
    form = f,
    surface = surface,
    backBuf = backBuf,
    width = W,
    height = H,
    render = renderAll,
    selectTab = function(idx) currentTab = idx; renderAll(true) end,
    hoverAt = updateHoverAt,
    clickAt = handleClickAt,
    getHitboxes = function() return hitboxes end,
  }

  renderAll(true)
  PES10.dashboardForm = f
  f.show()
end

-- ============================================================================
-- AUTO-ATTACH & 300ms NON-BLOCKING TELEMETRY ENGINE
-- ============================================================================
function PES10.startBackgroundEngine()
  -- Destroy any previous timer FIRST before any initialization to prevent re-entrant C stack overflow!
  if PES10.bgTimer then
    pcall(function() PES10.bgTimer.Enabled = false; PES10.bgTimer.destroy() end)
    PES10.bgTimer = nil
  end
  PES10.isInitializing = false
  pcall(PES10.bindTableHooks)

  local tickCount = 0
  PES10.bgTimer = createTimer(MainForm, true)
  PES10.bgTimer.Interval = 300
  PES10.bgTimer.OnTimer = function()
    if PES10.isInitializing then return end
    if not PES10.isAttached() then
      for _, exeName in ipairs(PES10.SUPPORTED_EXES or { PES10.PROCESS_NAME }) do
        local pid = getProcessIDFromProcessName(exeName)
        if pid and pid > 0 then
          PES10.PROCESS_NAME = exeName
          openProcess(pid)
          PES10.initProcessState()
          break
        end
      end
      return
    end
    if PES10.stateBase == 0 or PES10.attachedPid ~= getOpenedProcessID() then
      PES10.initProcessState()
    else
      PES10.syncAllDynamicPointers()
      tickCount = tickCount + 1
      -- Silent background auto-healing every ~2.4s: keeps all 6 internal modules armed (<5ms via aobscanregion)
      if (tickCount % 8) == 0 then
        PES10.activateAllHooks(true)
      end
    end
  end
end

-- __PES10_CONFIG_INJECTION__
PES10.startBackgroundEngine()

-- Optional dev automation bridge via env var (disabled by default)
pcall(function()
  local devBridge = os.getenv and os.getenv("PES10_DEV_BRIDGE")
  if devBridge and devBridge ~= "" then
    local f = io.open(devBridge, "r")
    if f then f:close(); dofile(devBridge) end
  end
end)
