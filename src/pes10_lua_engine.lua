-- ============================================================================
-- PES 2021 - v11.0 ULTIMATE STUDIO OVERHAUL (BMPES 16.0 / SIDER READY)
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
PES10.VERSION = "11.0 Ultimate Studio"
PES10.PROCESS_NAME = "PES2021.exe"
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
PES10.BMPES_TEAMS = __BMPES_TEAMS_LUA__
PES10.BMPES_COMPS = __BMPES_COMPS_LUA__
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

  local fallback = PES10.BMPES_TEAMS[teamId]
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
    writeInteger(sb + PES10.SYM_OFFSETS.valComputedOVR, PES10.calcPlayerOVR(pAddr))
  else
    PES10._lastPlayerKey = ""
    writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerTraining, 0)
    writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerSquadBonus, 0)
    writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerReleaseSlot, 0)
    writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerGrowthDesc, 0)
    writeInteger(sb + PES10.SYM_OFFSETS.valComputedOVR, 0)
  end
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
  showMessage(table.concat(lines, "\n"))
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

-- Adjusts targetPos UP or DOWN to any targetOVR (40-99) while isolating attributes so primary OVR does not drift.
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

  local curApt = PES10.getPosAptitude(playerAddr, targetPos)
  local useAptA = (targetOVR > PES10.calcPlayerOVRFromBuf(origBuf, targetPos, false)) or (curApt >= 2)
  local curTgtOVR = PES10.calcPlayerOVRFromBuf(origBuf, targetPos, useAptA)
  if curTgtOVR == targetOVR then
    if useAptA and curApt < 2 then
      if not skipUndo then PES10.pushPlayerUndo(playerAddr, PES10.T("Grau A " .. PES10.posShort(targetPos), "Grade A " .. PES10.posShort(targetPos))) end
      PES10.setPosAptitude(playerAddr, targetPos, 2, true)
    end
    return true, curTgtOVR, curTgtOVR, origPriOVR, origPriOVR, ""
  end

  local dir = (targetOVR > curTgtOVR) and 1 or -1
  if not skipUndo then
    PES10.pushPlayerUndo(playerAddr, string.format("Iso %s->%d", PES10.posShort(targetPos), targetOVR))
  end

  local profile = PES10.getPositionIsolationProfile(regPos, targetPos)
  if #profile == 0 then
    local ok, o1, o2 = PES10.scalePlayerToTargetOVR(playerAddr, targetOVR, targetPos, true)
    local newPri = PES10.calcPlayerOVR(playerAddr)
    return ok, o1, o2, origPriOVR, newPri, ""
  end

  local cand = copyByteBuf(origBuf)
  if useAptA then bufSetPosAptitude(cand, targetPos, 2) end
  local changed = {}
  for _, item in ipairs(profile) do changed[item.fid] = 0 end

  local function reachedGoal()
    local cur = PES10.calcPlayerOVRFromBuf(cand, targetPos, useAptA)
    return (dir == 1 and cur >= targetOVR) or (dir == -1 and cur <= targetOVR)
  end

  -- Progressive relaxation: Phase 0 = STRICT zero primary OVR drift (0), then 1, 2, 99 if saturated
  local maxPriDrifts = { 0, 1, 2, 99 }
  for _, drift in ipairs(maxPriDrifts) do
    local safety = 0
    while not reachedGoal() and safety < 320 do
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
            bufSetStat(cand, item.fid, cv)
            local priOk = (dir == 1 and testPri <= origPriOVR + drift) or (dir == -1 and testPri >= origPriOVR - drift)
            if priOk then
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
    if reachedGoal() then break end
  end

  writeBytes(playerAddr, cand)
  if useAptA then PES10.setPosAptitude(playerAddr, targetPos, 2, true) end
  PES10.syncAllDynamicPointers()

  local newTgtOVR = PES10.calcPlayerOVRFromBuf(cand, targetPos, useAptA)
  local newPriOVR = PES10.calcPlayerOVRFromBuf(cand, regPos)
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

function PES10.matchSecondaryToPrimaryOVR(playerAddr, targetPos)
  local p = playerAddr or PES10.getSelectedPlayerOrWarn()
  if not p or p == 0 then return false end
  local origPri, regPos = PES10.calcPlayerOVR(p)
  if targetPos == regPos then
    PES10.toast(PES10.T("Escolha uma posição secundária diferente da principal!", "Pick a secondary position different from primary!"))
    return false
  end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local ok, oT, nT, oP, nP, diffStr = PES10.boostSecondaryPositionIsolated(p, targetPos, origPri, false)
  if ok then
    local priDelta = nP - oP
    local priNote = (priDelta == 0)
      and string.format(PES10.T("%s mantido em %d (+0)", "%s kept at %d (+0)"), PES10.posShort(regPos), nP)
      or string.format("%s %d→%d (+%d)", PES10.posShort(regPos), oP, nP, priDelta)
    PES10.toast(string.format(
      "%s: %s %d→%d | %s%s",
      pName, PES10.posShort(targetPos), oT, nT, priNote, (diffStr ~= "" and (" [" .. diffStr .. "]") or "")
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

-- Step +1 or -1 OVR (or exact target OVR) in any target position with isolated primary protection!
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
      local priDelta = nP - oP
      local priSign = (priDelta > 0) and "+" or ""
      local priNote = (priDelta == 0)
        and string.format(PES10.T("%s mantido em %d (0)", "%s kept at %d (0)"), PES10.posShort(regPos), nP)
        or string.format("%s %d->%d (%s%d)", PES10.posShort(regPos), oP, nP, priSign, priDelta)
      PES10.toast(string.format(
        "%s: %s %d->%d | %s%s",
        pName, posCodeLbl, oT, nT, priNote, (diffStr ~= "" and (" [" .. diffStr .. "]") or "")
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
  local _, regPos = PES10.calcPlayerOVRFromBuf(origBuf)
  local d = delta or 1
  local useAptA = (d > 0) or (PES10.getPosAptitude(p, targetPos) >= 2)
  local curPosOVR = PES10.calcPlayerOVRFromBuf(origBuf, targetPos, (targetPos ~= regPos) and useAptA or false)
  local newTargetOVR = math.max(40, math.min(99, curPosOVR + d))
  if newTargetOVR == curPosOVR then
    PES10.toast(string.format(
      PES10.T("Já está no limite (%d OVR em %s).", "Already at limit (%d OVR in %s)."),
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
  local ans = inputQuery(
    PES10.T("Definir Overall Proporcional (", "Set Proportional Overall (") .. pName .. ")",
    string.format(
      PES10.T(
        "Overall atual: %d (%s). Digite o novo OVR desejado (40-99) mantendo o DNA/estilo:",
        "Current Overall: %d (%s). Enter target OVR (40-99) preserving player DNA/style:"
      ),
      curOVR, PES10.posShort(regPos)
    ),
    tostring(math.min(99, curOVR + 3))
  )
  if not ans then return false end
  local t = tonumber(ans)
  if not t then
    PES10.toast(PES10.T("Overall inválido.", "Invalid Overall value."))
    return false
  end
  local ok, oldO, newO = PES10.scalePlayerToTargetOVR(p, t)
  if ok then
    PES10.toast(string.format(
      PES10.T("%s ajustado de %d → %d OVR preservando proporções!", "%s scaled from %d → %d OVR preserving proportions!"),
      pName, oldO, newO
    ))
  end
  return ok
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

  local handles = {}
  local userClubHandle = nil
  for slot = 0, 39 do
    local entry = career + 0x016EE854 + (slot * 0x34)
    local h = PES10.readU64(entry)
    if h ~= 0 and math.floor(h / 0x100000000) ~= 0 then
      handles[h] = true
    end
  end

  local count = PES10.readU32(career + 0x00D0BCE8)
  if count == 0 or count > 30001 then count = 30000 end

  local list = {}
  for i = 0, count - 1 do
    local p = career + (i * 0x17C)
    local h = PES10.readU64(p + 0x2C)
    if handles[h] then
      list[#list + 1] = p
      if not userClubHandle then
        local ch = PES10.readU32(p + 0x12C)
        if ch ~= 0 and ch ~= 0xFFFFFFFF then userClubHandle = ch end
      end
    end
  end

  if #list == 0 and userClubHandle then
    for i = 0, count - 1 do
      local p = career + (i * 0x17C)
      if PES10.readU32(p + 0x12C) == userClubHandle then
        list[#list + 1] = p
      end
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
      local ans = inputQuery(
        PES10.T("Editar Valor Financeiro (", "Edit Financial Value (") .. c.symbol .. ")",
        PES10.T(
          "Digite o valor (aceita 50M, 500k, 1.5B, 1000 ou 80.000.000):",
          "Enter amount (accepts 50M, 500k, 1.5B, 1000 or 80,000,000):"
        ),
        PES10.formatMoneyFromRaw(curRaw)
      )
      if not ans then return true end
      local newRaw = PES10.parseHumanMoneyToRaw(ans)
      if not newRaw then
        showMessage(PES10.T("Valor inválido: ", "Invalid amount: ") .. tostring(ans))
        return true
      end
      if id == PES10.CUSTOM_RFEE_RECORD_ID then
        local p = PES10.getSelectedPlayerOrWarn()
        if p ~= 0 then PES10.setOverrideReleaseFeeForPlayer(p, newRaw) end
        return true
      end
      memrec.Value = tostring(newRaw)
      return true
    end

    if id == PES10.CLOCK_RECORD_ID then
      local info = PES10.getMatchClockInfo()
      if not info then
        showMessage(PES10.T("Entre em uma partida ao vivo primeiro!", "Enter a live match first!"))
        return true
      end
      local ans = inputQuery(
        PES10.T("Definir Minuto Exato da Partida", "Set Exact Match Minute"),
        PES10.T("Digite o minuto desejado (ex: 15, 44, 88 ou 44:30):", "Enter desired minute (e.g., 15, 44, 88 or 44:30):"),
        string.format("%02d:%02d", info.mm, info.ss)
      )
      if not ans then return true end
      local mm, ss = ans:match("^(%d+)%s*:%s*(%d+)$")
      local targetMin = mm and (tonumber(mm) + tonumber(ss) / 60.0) or tonumber((ans:gsub(",", ".")))
      if targetMin then PES10.setExactMatchMinute(targetMin) end
      return true
    end

    if PES10.SCORE_SYNC_IDS[id] then
      local side = PES10.SCORE_SYNC_IDS[id]
      local ans = inputQuery(
        PES10.T("Editar Placar ao Vivo", "Edit Live Score"),
        PES10.T("Digite o novo número de gols (0-99):", "Enter new goal count (0-99):"),
        memrec.Value
      )
      if not ans then return true end
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
-- PES 2021 v11.0 ULTIMATE STUDIO - VISUAL INTERACTIVE MODALS (DPI-AWARE)
-- Zero clunky text prompts: 1-click Position Lab, Team Role, Real-RAM Scouts & Radar 30K
-- ============================================================================
function PES10.uiScale()
  local dpi = (getScreenDPI and getScreenDPI()) or 96
  if dpi < 96 then dpi = 96 end
  local sc = dpi / 96.0
  return function(px) return math.floor(px * sc + 0.5) end
end

local function makeStudioModal(modalKey, title, logW, logH)
  if modalKey and PES10.openModals and PES10.openModals[modalKey] then
    pcall(function() PES10.openModals[modalKey].close(); PES10.openModals[modalKey].destroy() end)
    PES10.openModals[modalKey] = nil
  end
  local S = PES10.uiScale()
  local f = createForm(false)
  f.Caption = title
  pcall(function() f.BorderStyle = "bsSingle" end)
  f.Color = 0x170F0B
  f.DoNotSaveInTable = true
  local W, H = S(logW), S(logH)
  pcall(function() f.ClientWidth = W; f.ClientHeight = H end)
  if not f.ClientWidth or f.ClientWidth ~= W then f.Width = W; f.Height = H end
  f.centerScreen()
  f.OnClose = function()
    if modalKey and PES10.openModals and PES10.openModals[modalKey] == f then
      PES10.openModals[modalKey] = nil
    end
    return caFree
  end
  if modalKey and PES10.openModals then PES10.openModals[modalKey] = f end
  return f, S, W, H
end

local function makeModalLabel(parent, x, y, w, h, txt, ptSize, color, bold)
  local lbl = createLabel(parent)
  lbl.Left = x; lbl.Top = y; lbl.Width = w; lbl.Height = h
  lbl.AutoSize = false
  lbl.Caption = txt
  lbl.Font.Name = "Segoe UI"
  lbl.Font.Size = ptSize or 9
  lbl.Font.Color = color or 0xFCFAF8
  pcall(function() lbl.Font.Style = bold and "[fsBold]" or "[]" end)
  return lbl
end

local function makeModalBtn(parent, x, y, w, h, txt, onClick)
  local b = createButton(parent)
  b.Left = x; b.Top = y; b.Width = w; b.Height = h
  b.Caption = txt
  b.Font.Name = "Segoe UI"
  b.Font.Size = 9
  b.OnClick = onClick
  return b
end

-- MODAL 1: VISUAL POSITION LAB (ISOLATED +1 OVR, MATCH PRIMARY OVR, SET PRIMARY & C/B/A APTITUDE)
function PES10.openPositionLabModal()
  if not createListBox or not createButton then return PES10.promptTargetPositionOVRPlusOne() end
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local f, S, W, H = makeStudioModal("poslab", PES10.T("Laboratório de Posições (Isolamento OVR) - ", "Position Lab (Isolated OVR) - ") .. pName, 670, 560)

  local hdr = makeModalLabel(f, S(16), S(12), W - S(32), S(24), "", 11, 0xFCFAF8, true)
  local sub = makeModalLabel(f, S(16), S(38), W - S(32), S(20),
    PES10.T("Selecione uma posição para subir +1 OVR isolado (sem inflar a principal), igualar OVR ou mudar Grau:",
            "Pick a position to boost +1 isolated OVR (without inflating primary), match primary OVR, or edit Grade:"),
    8, 0xC4B2A0, false)

  local lb = createListBox(f)
  lb.Left = S(16); lb.Top = S(64); lb.Width = W - S(32); lb.Height = S(286)
  lb.Color = 0x241811
  lb.Font.Name = "Consolas"
  lb.Font.Size = 10
  lb.Font.Color = 0xFCFAF8

  local lblIso = makeModalLabel(f, S(16), S(358), W - S(32), S(20), "", 8, 0xF8BD38, true)
  local lblLog = makeModalLabel(f, S(16), S(380), W - S(32), S(20),
    PES10.T("Dica: Duplo-clique em qualquer posição aplica +1 OVR Isolado imediatamente.",
            "Tip: Double-click any position to apply +1 Isolated OVR right away."),
    8, 0x99D334, false)

  local posOrder = {12, 11, 9, 10, 8, 6, 7, 5, 4, 2, 3, 1, 0}
  local function getPickedPos()
    local idx = lb.ItemIndex
    if not idx or idx < 0 or idx >= #posOrder then return nil end
    return posOrder[idx + 1], idx
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
        "Posição Principal selecionada: +1 OVR usa o motor proporcional DNA completo.",
        "Primary Position selected: +1 OVR uses the full proportional DNA solver."
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
      PES10.T("Atributos isolados (%s sem subir %s): %s", "Isolated stats (%s vs %s): %s"),
      PES10.posShort(tp), PES10.posShort(regPos), table.concat(top, "  •  ")
    )
  end

  local function refreshList(keepIdx, logText)
    local b = readBytes(p, 0x28, true)
    if not b or #b < 0x28 then return end
    local regOVR, regPos = PES10.calcPlayerOVRFromBuf(b)
    local undoN = PES10.canUndoPlayer(p)
    hdr.Caption = string.format(
      PES10.T("%s  •  Principal: %d %s  •  Histórico Undo: %d", "%s  •  Primary: %d %s  •  Undo History: %d"),
      pName, regOVR, PES10.posShort(regPos), undoN
    )
    local prev = keepIdx or lb.ItemIndex
    lb.Items.clear()
    for _, pIdx in ipairs(posOrder) do
      local info = PES10.POS_NAMES[pIdx]
      local pShort = PES10.posShort(pIdx)
      local apt = (pIdx == regPos) and 2 or bufGetPosAptitude(b, pIdx)
      local aptL = (apt == 2 and "A") or (apt == 1 and "B") or "C"
      local curO = PES10.calcPlayerOVRFromBuf(b, pIdx, false)
      local potO = PES10.calcPlayerOVRFromBuf(b, pIdx, true)
      local mark = (pIdx == regPos) and PES10.T(" [PRINCIPAL]", " [PRIMARY]") or ""
      local gain = (apt < 2) and string.format(" (c/ A: %d -> +1: %d)", potO, math.min(99, potO + 1)) or string.format(" (+1 -> %d)", math.min(99, potO + 1))
      lb.Items.add(string.format("%-3s | %-18s | OVR %2d [%s]%s%s", pShort, PES10.T(info.pt, info.en), curO, aptL, gain, mark))
    end
    if prev and prev >= 0 and prev < #posOrder then lb.ItemIndex = prev else lb.ItemIndex = 0 end
    if logText and logText ~= "" then lblLog.Caption = logText end
    updateIsoPreview()
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  pcall(function() lb.OnSelectionChange = updateIsoPreview; lb.OnClick = updateIsoPreview end)
  local function doBoostOne()
    local tp, idx = getPickedPos()
    if tp ~= nil then
      PES10.stepTargetPositionOVR(tp, 1)
      refreshList(idx, PES10.lastToastMsg)
    end
  end
  lb.OnDblClick = doBoostOne

  local y1, y2, y3 = S(408), S(450), S(494)
  makeModalBtn(f, S(16),  y1, S(78),  S(34), "-1 OVR", function()
    local tp, idx = getPickedPos()
    if tp ~= nil then PES10.stepTargetPositionOVR(tp, -1); refreshList(idx, PES10.lastToastMsg) end
  end)
  makeModalBtn(f, S(100), y1, S(78),  S(34), "+1 OVR", doBoostOne)

  edTgt = createEdit(f)
  edTgt.Left = S(186); edTgt.Top = y1 + S(4); edTgt.Width = S(52); edTgt.Height = S(26); edTgt.Text = "85"
  local function applyExact()
    local tp, idx = getPickedPos()
    local val = tonumber(edTgt.Text)
    if tp ~= nil and val then
      PES10.setTargetPositionExactOVR(tp, val)
      refreshList(idx, PES10.lastToastMsg)
    end
  end
  pcall(function() edTgt.OnKeyDown = function(sender, key) if key == 13 then applyExact() end; return key end end)
  makeModalBtn(f, S(244), y1, S(94),  S(34), PES10.T("Definir OVR", "Set OVR"), applyExact)

  makeModalBtn(f, S(344), y1, S(152), S(34), PES10.T("Igualar Principal", "Match Primary"), function()
    local tp, idx = getPickedPos()
    if tp ~= nil then
      PES10.matchSecondaryToPrimaryOVR(p, tp)
      refreshList(idx, PES10.lastToastMsg)
    end
  end)
  makeModalBtn(f, S(502), y1, S(152), S(34), PES10.T("Tornar Principal", "Set as Primary"), function()
    local tp, idx = getPickedPos()
    if tp ~= nil and PES10.setRegisteredPosition(p, tp) then
      local newOvr = PES10.calcPlayerOVR(p)
      PES10.toast(string.format(PES10.T("%s: Posição Principal alterada p/ %s (%d OVR)!", "%s: Primary Position set to %s (%d OVR)!"), pName, PES10.posShort(tp), newOvr))
      refreshList(idx, PES10.lastToastMsg)
    end
  end)

  makeModalBtn(f, S(16),  y2, S(124), S(34), PES10.T("Definir Grau A", "Set Grade A"), function()
    local tp, idx = getPickedPos()
    if tp ~= nil then PES10.setPosAptitude(p, tp, 2); PES10.toast(pName .. ": " .. PES10.posShort(tp) .. " -> A"); refreshList(idx, PES10.lastToastMsg) end
  end)
  makeModalBtn(f, S(146), y2, S(124), S(34), PES10.T("Definir Grau B", "Set Grade B"), function()
    local tp, idx = getPickedPos()
    if tp ~= nil then PES10.setPosAptitude(p, tp, 1); PES10.toast(pName .. ": " .. PES10.posShort(tp) .. " -> B"); refreshList(idx, PES10.lastToastMsg) end
  end)
  makeModalBtn(f, S(276), y2, S(124), S(34), PES10.T("Definir Grau C", "Set Grade C"), function()
    local tp, idx = getPickedPos()
    if tp ~= nil then PES10.setPosAptitude(p, tp, 0); PES10.toast(pName .. ": " .. PES10.posShort(tp) .. " -> C"); refreshList(idx, PES10.lastToastMsg) end
  end)
  makeModalBtn(f, S(406), y2, S(248), S(34), PES10.T("Promover Todas Secundárias B -> A", "Promote All Secondary B -> A"), function()
    local n = PES10.promoteSecondaryPositionsToA(p)
    PES10.toast(string.format(PES10.T("%d secundária(s) promovida(s) p/ A!", "%d secondary pos promoted to A!"), n))
    refreshList(lb.ItemIndex, PES10.lastToastMsg)
  end)

  makeModalBtn(f, S(16),  y3, S(210), S(34), PES10.T("Todas as 13 Posições em 'A'", "All 13 Positions to Grade 'A'"), function()
    PES10.runPlayerMacro("all_positions_a")
    refreshList(lb.ItemIndex, PES10.lastToastMsg)
  end)
  makeModalBtn(f, S(232), y3, S(210), S(34), PES10.T("Desfazer Última Edição (Undo)", "Undo Last Edit"), function()
    PES10.undoLastPlayerEdit(p)
    refreshList(lb.ItemIndex, PES10.lastToastMsg)
  end)
  makeModalBtn(f, S(448), y3, S(206), S(34), PES10.T("Fechar", "Close"), function() f.close() end)

  refreshList(0)
  f.show()
end

-- MODAL 2: VISUAL TEAM ROLE SELECTOR (22 ROLES + LEVEL 1-3 + QUICK PRESETS)
function PES10.openTeamRoleModal()
  if not createListBox or not createButton then return PES10.promptSetTeamRole() end
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local f, S, W, H = makeStudioModal("teamrole", PES10.T("Função de Equipe (Team Role) - ", "Team Role & Level - ") .. pName, 560, 470)

  local hdr = makeModalLabel(f, S(16), S(12), W - S(32), S(24), "", 11, 0xFCFAF8, true)
  local sub = makeModalLabel(f, S(16), S(38), W - S(32), S(20),
    PES10.T("Selecione qualquer uma das 22 Funções de Equipe + Nível (1-3) ou use um Preset rápido:",
            "Select any of the 22 Team Roles + Level (1-3) or click a Quick Preset:"),
    8, 0xC4B2A0, false)

  local lb = createListBox(f)
  lb.Left = S(16); lb.Top = S(64); lb.Width = W - S(32); lb.Height = S(268)
  lb.Color = 0x241811
  lb.Font.Name = "Segoe UI"
  lb.Font.Size = 10
  lb.Font.Color = 0xFCFAF8

  for r = 0, 22 do
    local info = PES10.TEAM_ROLES[r]
    lb.Items.add(string.format("[%2d]  %s", r, info and PES10.T(info.pt, info.en) or "-"))
  end

  local cbLvl = createComboBox(f)
  cbLvl.Left = S(16); cbLvl.Top = S(344); cbLvl.Width = S(130); cbLvl.Height = S(28)
  cbLvl.Style = "csDropDownList"
  cbLvl.Items.add(PES10.T("Nível 1", "Level 1"))
  cbLvl.Items.add(PES10.T("Nível 2", "Level 2"))
  cbLvl.Items.add(PES10.T("Nível 3 (Máx)", "Level 3 (Max)"))

  local function refreshRole()
    local cr, cl = PES10.getTeamRole(p)
    local rn = PES10.TEAM_ROLES[cr] and PES10.T(PES10.TEAM_ROLES[cr].pt, PES10.TEAM_ROLES[cr].en) or "-"
    hdr.Caption = string.format(PES10.T("%s  •  Atual: %s (Nível %d)", "%s  •  Current: %s (Level %d)"), pName, rn, cl)
    lb.ItemIndex = cr
    cbLvl.ItemIndex = math.max(0, math.min(2, cl - 1))
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  local function applyRole(rNum, lNum)
    PES10.setTeamRole(p, rNum, lNum)
    local rInfo = PES10.TEAM_ROLES[rNum]
    local rTxt = rInfo and PES10.T(rInfo.pt, rInfo.en) or tostring(rNum)
    PES10.toast(string.format(PES10.T("%s agora é '%s' (Nível %d)!", "%s is now '%s' (Level %d)!"), pName, rTxt, lNum))
    refreshRole()
  end
  lb.OnDblClick = function()
    local idx = lb.ItemIndex
    if idx and idx >= 0 and idx <= 22 then applyRole(idx, (cbLvl.ItemIndex or 2) + 1) end
  end

  makeModalBtn(f, S(156), S(342), S(200), S(32), PES10.T("Aplicar Função Selecionada", "Apply Selected Role"), function()
    local idx = lb.ItemIndex
    if idx and idx >= 0 and idx <= 22 then
      applyRole(idx, (cbLvl.ItemIndex or 2) + 1)
    end
  end)
  makeModalBtn(f, S(364), S(342), S(90), S(32), PES10.T("Desfazer", "Undo"), function()
    PES10.undoLastPlayerEdit(p); refreshRole()
  end)
  makeModalBtn(f, S(462), S(342), S(82), S(32), PES10.T("Fechar", "Close"), function() f.close() end)

  -- 5 One-Click Quick Presets
  local yP = S(390)
  makeModalLabel(f, S(16), yP - S(18), W - S(32), S(16), PES10.T("PRESETS DE 1 CLIQUE (NÍVEL 3 MÁXIMO):", "1-CLICK QUICK PRESETS (LEVEL 3 MAX):"), 8, 0x24BFFB, true)
  makeModalBtn(f, S(16),  yP, S(100), S(34), PES10.T("Lenda", "Legend"),       function() applyRole(19, 3) end)
  makeModalBtn(f, S(122), yP, S(100), S(34), PES10.T("Bandeira", "Bandiera"),  function() applyRole(18, 3) end)
  makeModalBtn(f, S(228), yP, S(100), S(34), PES10.T("Maestro", "Maestro"),    function() applyRole(9, 3) end)
  makeModalBtn(f, S(334), yP, S(100), S(34), PES10.T("General", "General"),    function() applyRole(7, 3) end)
  makeModalBtn(f, S(440), yP, S(104), S(34), PES10.T("Superestrela", "Superstar"), function() applyRole(14, 3) end)

  refreshRole()
  f.show()
end

-- MODAL 3: VISUAL REAL-RAM SCOUT OBSERVATION HUB (SCOUTS 1, 2, 3 & SHORTLIST)
function PES10.openScoutListsModal()
  if not createListBox or not createButton then return PES10.promptAddPlayerToScoutList() end
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    PES10.toast(PES10.T("Carregue um save de Master League primeiro!", "Load a Master League save first!"))
    return
  end

  local f, S, W, H = makeStudioModal("scouthub", PES10.T("Central de Olheiros & Observação (RAM Real)", "Scout Observation Hub (Live RAM)"), 660, 500)
  local hdr = makeModalLabel(f, S(16), S(12), W - S(32), S(24), "", 11, 0xFCFAF8, true)
  local sub = makeModalLabel(f, S(16), S(38), W - S(32), S(20), "", 8, 0x99D334, false)

  local curSlot = 1
  local curEntries = {}

  local lb = createListBox(f)
  lb.Left = S(16); lb.Top = S(104); lb.Width = W - S(32); lb.Height = S(268)
  lb.Color = 0x241811
  lb.Font.Name = "Consolas"
  lb.Font.Size = 9
  lb.Font.Color = 0xFCFAF8

  local tabBtns = {}
  local function refreshHub()
    local p = PES10.readU64(PES10.symAddr("ptrPlayer"))
    if p ~= 0 then
      local nm = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
      local ovr, rp = PES10.calcPlayerOVR(p)
      hdr.Caption = string.format(PES10.T("Atleta Selecionado: %s (%d %s)", "Selected Player: %s (%d %s)"), nm, ovr, PES10.posShort(rp))
    else
      hdr.Caption = PES10.T("Nenhum atleta selecionado - Use o Radar 30K ou passe o cursor no jogo", "No player selected - Use Radar 30K or hover a player in-game")
    end

    for s = 1, 4 do
      local def = PES10.SCOUT_LISTS[s]
      local u, cap = PES10.getScoutListSummary(s)
      local tag = (s == curSlot) and ">> " or ""
      tabBtns[s].Caption = string.format("%s%s (%d/%d)", tag, PES10.T(def.pt, def.en):gsub(" %(Shortlist%)", ""):gsub("Lista de ", ""), u, cap)
    end

    curEntries = PES10.getScoutListEntries(curSlot)
    local def = PES10.SCOUT_LISTS[curSlot]
    sub.Caption = string.format(PES10.T("Exibindo na RAM: %s - %d / %d vagas ocupadas", "Live RAM View: %s - %d / %d slots used"), PES10.T(def.pt, def.en), #curEntries, def.cap)
    lb.Items.clear()
    if #curEntries == 0 then
      lb.Items.add(PES10.T("  [ Lista vazia - Clique em '+ Adicionar Atleta Atual' abaixo ]", "  [ List empty - Click '+ Add Current Player' below ]"))
    else
      for idx, e in ipairs(curEntries) do
        lb.Items.add(string.format("%2d. %-24s | %2d %-3s | %2d %s | %s", idx, e.name, e.ovr, PES10.posShort(e.pos), e.age, PES10.T("a", "y"), e.club))
      end
      lb.ItemIndex = 0
    end
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  local tW = S(152)
  for s = 1, 4 do
    local sCopy = s
    tabBtns[s] = makeModalBtn(f, S(16) + (s - 1) * (tW + S(6)), S(64), tW, S(32), "Slot " .. s, function()
      curSlot = sCopy; refreshHub()
    end)
  end

  local y1, y2 = S(384), S(428)
  makeModalBtn(f, S(16),  y1, S(240), S(34), PES10.T("+ Adicionar Atleta Atual nesta Lista", "+ Add Current Player to This List"), function()
    if PES10.addPlayerToScoutList(curSlot) then refreshHub() end
  end)
  local function loadScoutEntry()
    local idx = lb.ItemIndex
    if idx and curEntries[idx + 1] and curEntries[idx + 1].playerAddr ~= 0 then
      local e = curEntries[idx + 1]
      writeQword(PES10.symAddr("ptrPlayer"), e.playerAddr)
      PES10.syncAllDynamicPointers()
      PES10.toast(PES10.T("Carregado do Olheiro: ", "Loaded from Scout: ") .. e.name)
      refreshHub()
    end
  end
  lb.OnDblClick = loadScoutEntry
  makeModalBtn(f, S(264), y1, S(196), S(34), PES10.T("Carregar Selecionado no Studio", "Load Picked Player to Studio"), loadScoutEntry)
  makeModalBtn(f, S(468), y1, S(176), S(34), PES10.T("Remover Item da Lista", "Remove Selected Item"), function()
    local idx = lb.ItemIndex
    if idx and curEntries[idx + 1] then
      local e = curEntries[idx + 1]
      PES10.removePlayerFromScoutList(curSlot, e.handle)
      PES10.toast(PES10.T("Removido da lista: ", "Removed from list: ") .. e.name)
      refreshHub()
    end
  end)

  makeModalBtn(f, S(16),  y2, S(176), S(34), PES10.T("Negócio Irrecusável", "Irresistible Deal"), function()
    PES10.makeDealIrresistible(); refreshHub()
  end)
  makeModalBtn(f, S(198), y2, S(116), S(34), PES10.T("Radar 30K...", "Radar 30K..."), function()
    PES10.openScoutFinderModal()
  end)
  makeModalBtn(f, S(320), y2, S(116), S(34), PES10.T("Limpar Lista", "Clear List"), function()
    PES10.clearScoutSlots(curSlot); refreshHub()
  end)
  makeModalBtn(f, S(442), y2, S(116), S(34), PES10.T("Limpar 1-3", "Clear 1-3"), function()
    PES10.clearScoutSlots(); refreshHub()
  end)
  makeModalBtn(f, S(564), y2, S(80),  S(34), PES10.T("Fechar", "Close"), function() f.close() end)

  refreshHub()
  f.show()
end

-- MODAL 4: RADAR 30K - ADVANCED VISUAL SCOUT FINDER (NAME, POS, OVR, AGE, EXPIRING CONTRACT)
function PES10.openScoutFinderModal()
  if not createListBox or not createEdit or not createComboBox or not createButton then
    return PES10.searchCareerPlayerDialog()
  end
  PES10.syncAllDynamicPointers()
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    PES10.toast(PES10.T("Carregue um save de Master League ou BAL primeiro!", "Load a Master League or BAL save first!"))
    return
  end

  local f, S, W, H = makeStudioModal("radar30k", PES10.T("Radar 30K - Busca Avançada & Caçador de Joias", "Radar 30K - Advanced Scout & Wonderkid Finder"), 740, 568)
  makeModalLabel(f, S(16), S(10), S(160), S(16), PES10.T("Nome ou Player ID:", "Name or Player ID:"), 8, 0xC4B2A0, true)
  local edName = createEdit(f)
  edName.Left = S(16); edName.Top = S(28); edName.Width = S(180); edName.Height = S(26); edName.Text = ""

  makeModalLabel(f, S(204), S(10), S(90), S(16), PES10.T("Posição:", "Position:"), 8, 0xC4B2A0, true)
  local cbPos = createComboBox(f)
  cbPos.Left = S(204); cbPos.Top = S(28); cbPos.Width = S(96); cbPos.Height = S(26); cbPos.Style = "csDropDownList"
  cbPos.Items.add(PES10.T("Todas", "All"))
  local posOrder = {12, 11, 9, 10, 8, 6, 7, 5, 4, 2, 3, 1, 0}
  for _, pIdx in ipairs(posOrder) do cbPos.Items.add(PES10.posShort(pIdx)) end
  cbPos.ItemIndex = 0

  makeModalLabel(f, S(308), S(10), S(68), S(16), PES10.T("OVR Mín:", "Min OVR:"), 8, 0xC4B2A0, true)
  local edOvr = createEdit(f)
  edOvr.Left = S(308); edOvr.Top = S(28); edOvr.Width = S(64); edOvr.Height = S(26); edOvr.Text = "80"

  makeModalLabel(f, S(380), S(10), S(74), S(16), PES10.T("Idade Máx:", "Max Age:"), 8, 0xC4B2A0, true)
  local edAge = createEdit(f)
  edAge.Left = S(380); edAge.Top = S(28); edAge.Width = S(64); edAge.Height = S(26); edAge.Text = "45"

  makeModalLabel(f, S(452), S(10), S(150), S(16), PES10.T("Status Especial:", "Special Filter:"), 8, 0xC4B2A0, true)
  local cbSpec = createComboBox(f)
  cbSpec.Left = S(452); cbSpec.Top = S(28); cbSpec.Width = S(160); cbSpec.Height = S(26); cbSpec.Style = "csDropDownList"
  cbSpec.Items.add(PES10.T("Todos os Jogadores", "All Players"))
  cbSpec.Items.add(PES10.T("Contrato <= 1 Ano", "Contract <= 1 Yr"))
  cbSpec.Items.add(PES10.T("Lista de Transferência", "Transfer Listed"))
  cbSpec.Items.add(PES10.T("Sem Clube / Livre", "Free Agents"))
  cbSpec.ItemIndex = 0

  local lblStat = makeModalLabel(f, S(16), S(98), W - S(32), S(18),
    PES10.T("Use os Presets rápidos acima ou digite Nome/Filtros e aperte ENTER (Duplo-Clique carrega o atleta):",
            "Use the Quick Presets above or enter Name/Filters and press ENTER (Double-Click loads player):"),
    8, 0xF8BD38, false)

  local lb = createListBox(f)
  lb.Left = S(16); lb.Top = S(120); lb.Width = W - S(32); lb.Height = S(348)
  lb.Color = 0x241811
  lb.Font.Name = "Consolas"
  lb.Font.Size = 9
  lb.Font.Color = 0xFCFAF8

  local results = {}
  local function runSearch()
    local qRaw = (edName.Text or ""):gsub("^%s+", ""):gsub("%s+$", "")
    local qNorm = PES10.normalizeText(qRaw)
    local qNum = tonumber(qRaw)
    local posSel = (cbPos.ItemIndex and cbPos.ItemIndex > 0) and posOrder[cbPos.ItemIndex] or nil
    local minOvr = tonumber(edOvr.Text) or 0
    local maxAge = tonumber(edAge.Text) or 99
    local specMode = cbSpec.ItemIndex or 0
    local curYear = PES10.readU16(career + 0x01642A24)
    if curYear < 2019 or curYear > 2150 then curYear = 2026 end

    local count = PES10.readU32(career + 0x00D0BCE8)
    if count == 0 or count > 30001 then count = 30000 end
    local allMatches = {}
    for i = 0, count - 1 do
      local p = career + (i * 0x17C)
      local pid = PES10.readU32(p + 0x30)
      if pid ~= 0 and pid ~= 0xFFFFFFFF then
        -- Fast 1-byte pre-filters BEFORE reading name or computing full Konami OVR (~10x faster across 30k players!)
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
    results = {}
    lb.Items.clear()
    for idx = 1, showLimit do
      local m = allMatches[idx]
      local _, _, clubDesc = PES10.decodeClubHandle(PES10.readU32(m.addr + 0x12C))
      local mkt = PES10.formatMoneyFromRaw(PES10.readI32(m.addr + 0x174))
      m.club = clubDesc; m.mkt = mkt
      results[idx] = m
      lb.Items.add(string.format("OVR %2d %-3s | %-22s | %2d%s | %-11s | %s", m.ovr, PES10.posShort(m.pos), m.name, m.age, PES10.T("a", "y"), m.mkt, m.club))
    end
    if showLimit > 0 then lb.ItemIndex = 0 end
    lblStat.Caption = string.format(
      PES10.T("Encontrados %d atleta(s) no banco 30K - exibindo Top %d por maior OVR.", "Found %d player(s) across 30K DB - showing Top %d by highest OVR."),
      totalFound, showLimit
    )
  end

  local function onEnterSearch(sender, key)
    if key == 13 then runSearch() end
    return key
  end
  pcall(function() edName.OnKeyDown = onEnterSearch; edOvr.OnKeyDown = onEnterSearch; edAge.OnKeyDown = onEnterSearch end)

  local function loadPicked()
    local idx = lb.ItemIndex
    if not idx or not results[idx + 1] then return nil end
    local m = results[idx + 1]
    writeQword(PES10.symAddr("ptrPlayer"), m.addr)
    PES10.syncAllDynamicPointers()
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
    return m
  end
  lb.OnDblClick = function()
    local m = loadPicked()
    if m then PES10.toast(string.format(PES10.T("Carregado: %s (%d %s)", "Loaded: %s (%d %s)"), m.name, m.ovr, PES10.posShort(m.pos))) end
  end

  makeModalBtn(f, S(620), S(26), S(104), S(30), PES10.T("Filtrar", "Search"), runSearch)

  -- 4 One-Click Smart Presets
  local yPre = S(62)
  makeModalBtn(f, S(16),  yPre, S(168), S(28), PES10.T("Joias Sub-21 (74+)", "Wonderkids U21 (74+)"), function()
    edName.Text = ""; cbPos.ItemIndex = 0; edOvr.Text = "74"; edAge.Text = "21"; cbSpec.ItemIndex = 0; runSearch()
  end)
  makeModalBtn(f, S(190), yPre, S(168), S(28), PES10.T("Craques Mundiais (85+)", "World Stars (85+)"), function()
    edName.Text = ""; cbPos.ItemIndex = 0; edOvr.Text = "85"; edAge.Text = "45"; cbSpec.ItemIndex = 0; runSearch()
  end)
  makeModalBtn(f, S(364), yPre, S(174), S(28), PES10.T("Fim de Contrato (75+)", "Expiring Deals (75+)"), function()
    edName.Text = ""; cbPos.ItemIndex = 0; edOvr.Text = "75"; edAge.Text = "45"; cbSpec.ItemIndex = 1; runSearch()
  end)
  makeModalBtn(f, S(544), yPre, S(180), S(28), PES10.T("Sem Clube / Livres", "Free Agents"), function()
    edName.Text = ""; cbPos.ItemIndex = 0; edOvr.Text = "70"; edAge.Text = "45"; cbSpec.ItemIndex = 3; runSearch()
  end)

  local yB = S(480)
  makeModalBtn(f, S(16),  yB, S(156), S(36), PES10.T("Carregar no Studio", "Load into Studio"), function()
    local m = loadPicked()
    if m then PES10.toast(string.format(PES10.T("Carregado: %s (%d %s)", "Loaded: %s (%d %s)"), m.name, m.ovr, PES10.posShort(m.pos))) end
  end)
  makeModalBtn(f, S(178), yB, S(104), S(36), PES10.T("+ Olheiro 1", "+ Scout 1"), function()
    if loadPicked() then PES10.addPlayerToScoutList(1) end
  end)
  makeModalBtn(f, S(288), yB, S(104), S(36), PES10.T("+ Olheiro 2", "+ Scout 2"), function()
    if loadPicked() then PES10.addPlayerToScoutList(2) end
  end)
  makeModalBtn(f, S(398), yB, S(104), S(36), PES10.T("+ Olheiro 3", "+ Scout 3"), function()
    if loadPicked() then PES10.addPlayerToScoutList(3) end
  end)
  makeModalBtn(f, S(508), yB, S(124), S(36), PES10.T("Negócio 100%", "100% Deal"), function()
    if loadPicked() then PES10.makeDealIrresistible() end
  end)
  makeModalBtn(f, S(638), yB, S(86),  S(36), PES10.T("Fechar", "Close"), function() f.close() end)

  runSearch()
  f.show()
end

-- MODAL 5: VISUAL 25 ATTRIBUTES EDITOR (40-99, -5/-1/+1/+5/EXACT, LIVE OVR & UNDO)
function PES10.openAttributesModal()
  if not createListBox or not createEdit or not createButton then return end
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local f, S, W, H = makeStudioModal("attrs", PES10.T("Editor de 25 Atributos (40-99) - ", "25 Attributes Editor (40-99) - ") .. pName, 600, 520)

  local hdr = makeModalLabel(f, S(16), S(12), W - S(32), S(24), "", 11, 0xFCFAF8, true)
  local sub = makeModalLabel(f, S(16), S(38), W - S(32), S(18),
    PES10.T("Selecione qualquer atributo para ajustar (-5 / -1 / +1 / +5 ou digite 40-99 e aperte ENTER):",
            "Pick any attribute to adjust (-5 / -1 / +1 / +5 or type 40-99 and press ENTER):"),
    8, 0xC4B2A0, false)

  local lb = createListBox(f)
  lb.Left = S(16); lb.Top = S(62); lb.Width = W - S(32); lb.Height = S(340)
  lb.Color = 0x241811; lb.Font.Name = "Consolas"; lb.Font.Size = 10; lb.Font.Color = 0xFCFAF8

  local fidOrder = {14,33,17,38,20,21,19,22,18,23,25,36,26,27,34,35,24,15,37,39,16,31,29,30,28}
  local langIdx = (PES10.lang == "en") and 2 or 1
  local edVal = nil

  local function getPickedFid()
    local idx = lb.ItemIndex
    if not idx or idx < 0 or idx >= #fidOrder then return nil end
    return fidOrder[idx + 1], idx
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
      PES10.T("%s  •  OVR Atual: %d %s  •  Undo: %d", "%s  •  Current OVR: %d %s  •  Undo: %d"),
      pName, ovr, PES10.posShort(regPos), PES10.canUndoPlayer(p)
    )
    local prev = keepIdx or lb.ItemIndex
    lb.Items.clear()
    for _, fid in ipairs(fidOrder) do
      local lbl = PES10.STAT_LABELS[fid] and PES10.STAT_LABELS[fid][langIdx] or tostring(fid)
      local v = bufGetStat(b, fid)
      local bar = string.rep("#", math.floor((v - 40) / 3))
      lb.Items.add(string.format("%-22s : %2d  |%s", lbl, v, bar))
    end
    if prev and prev >= 0 and prev < #fidOrder then lb.ItemIndex = prev else lb.ItemIndex = 0 end
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

  pcall(function() lb.OnSelectionChange = syncEditBox; lb.OnClick = syncEditBox end)
  lb.OnDblClick = function() setFidVal(1, true) end

  local y1, y2 = S(414), S(458)
  makeModalBtn(f, S(16),  y1, S(54), S(34), "-5", function() setFidVal(-5, true) end)
  makeModalBtn(f, S(76),  y1, S(54), S(34), "-1", function() setFidVal(-1, true) end)
  makeModalBtn(f, S(136), y1, S(54), S(34), "+1", function() setFidVal(1, true) end)
  makeModalBtn(f, S(196), y1, S(54), S(34), "+5", function() setFidVal(5, true) end)
  edVal = createEdit(f)
  edVal.Left = S(260); edVal.Top = y1 + S(4); edVal.Width = S(58); edVal.Height = S(26); edVal.Text = "80"
  local function applyTyped()
    local v = tonumber(edVal.Text)
    if v then setFidVal(v, false) end
  end
  pcall(function() edVal.OnKeyDown = function(s, key) if key == 13 then applyTyped() end; return key end end)
  makeModalBtn(f, S(324), y1, S(96),  S(34), PES10.T("Definir", "Set Value"), applyTyped)
  makeModalBtn(f, S(426), y1, S(158), S(34), PES10.T("Desfazer (Undo)", "Undo Edit"), function()
    PES10.undoLastPlayerEdit(p); refreshAttrs(lb.ItemIndex)
  end)

  makeModalBtn(f, S(16),  y2, S(180), S(34), PES10.T("+5 em Todos os Atributos", "+5 to All Attributes"), function()
    PES10.runPlayerMacro("stats_plus_5"); refreshAttrs(lb.ItemIndex)
  end)
  makeModalBtn(f, S(202), y2, S(180), S(34), PES10.T("Todos os 25 em 99", "Max All 25 to 99"), function()
    PES10.runPlayerMacro("stats_99"); refreshAttrs(lb.ItemIndex)
  end)
  makeModalBtn(f, S(388), y2, S(196), S(34), PES10.T("Fechar", "Close"), function() f.close() end)

  refreshAttrs(0)
  f.show()
end

-- MODAL 6: VISUAL PLAYING STYLE & 41 PLAYER SKILLS EDITOR
function PES10.openSkillsStyleModal()
  if not createListBox or not createComboBox or not createButton then return end
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local f, S, W, H = makeStudioModal("skills", PES10.T("Estilo de Jogo & 41 Habilidades - ", "Playing Style & 41 Skills - ") .. pName, 600, 520)

  makeModalLabel(f, S(16), S(12), S(160), S(20), PES10.T("Estilo de Jogo (Playing Style):", "Playing Style:"), 9, 0xFCFAF8, true)
  local cbStyle = createComboBox(f)
  cbStyle.Left = S(180); cbStyle.Top = S(10); cbStyle.Width = S(268); cbStyle.Height = S(26); cbStyle.Style = "csDropDownList"
  local psMap = PES10.PLAYING_STYLE_NAMES or {}
  for i = 0, 21 do
    cbStyle.Items.add(string.format("%2d: %s", i, psMap[i] or ("Style " .. tostring(i))))
  end
  local curPs = PES10.getBitsInDword(p + 0x1D, 3, 5)
  cbStyle.ItemIndex = math.max(0, math.min(21, curPs))

  makeModalBtn(f, S(456), S(8), S(128), S(30), PES10.T("Aplicar Estilo", "Apply Style"), function()
    local sel = cbStyle.ItemIndex or 0
    PES10.pushPlayerUndo(p, "Playing Style")
    PES10.setBitsInDword(p + 0x1D, 3, 5, sel)
    PES10.syncAllDynamicPointers()
    PES10.toast(pName .. ": " .. (psMap[sel] or tostring(sel)))
  end)

  local sub = makeModalLabel(f, S(16), S(46), W - S(32), S(18),
    PES10.T("Clique duplo em qualquer habilidade (Skill) para ligar/desligar individualmente:",
            "Double-click any Player Skill below to toggle ON/OFF individually:"),
    8, 0xC4B2A0, false)

  local lb = createListBox(f)
  lb.Left = S(16); lb.Top = S(68); lb.Width = W - S(32); lb.Height = S(344)
  lb.Color = 0x241811; lb.Font.Name = "Consolas"; lb.Font.Size = 9; lb.Font.Color = 0xFCFAF8

  local items = PES10.ALL_SKILL_ITEMS or {}
  local isEn = (PES10.lang == "en")

  local function refreshSkills(keepIdx)
    local prev = keepIdx or lb.ItemIndex
    local activeCnt = 0
    lb.Items.clear()
    for _, sk in ipairs(items) do
      local on = (PES10.getBitsInDword(p + sk[1], sk[2], 1) == 1)
      if on then activeCnt = activeCnt + 1 end
      local tag = on and "[ON ]" or "[   ]"
      lb.Items.add(string.format("%s  %s", tag, isEn and sk[4] or sk[3]))
    end
    sub.Caption = string.format(
      PES10.T("Habilidades ativas: %d / %d (Duplo-clique alterna ON/OFF)", "Active Skills: %d / %d (Double-click toggles ON/OFF)"),
      activeCnt, #items
    )
    if prev and prev >= 0 and prev < #items then lb.ItemIndex = prev else lb.ItemIndex = 0 end
  end

  local function togglePicked()
    local idx = lb.ItemIndex
    if not idx or not items[idx + 1] then return end
    local sk = items[idx + 1]
    local cur = PES10.getBitsInDword(p + sk[1], sk[2], 1)
    PES10.pushPlayerUndo(p, "Skill")
    PES10.setBitsInDword(p + sk[1], sk[2], 1, cur == 1 and 0 or 1)
    refreshSkills(idx)
  end
  lb.OnDblClick = togglePicked

  local y1, y2 = S(424), S(466)
  makeModalBtn(f, S(16),  y1, S(186), S(34), PES10.T("Alternar Skill (ON/OFF)", "Toggle Skill (ON/OFF)"), togglePicked)
  makeModalBtn(f, S(208), y1, S(184), S(34), PES10.T("Desbloquear Todas (41)", "Unlock All 41 Skills"), function()
    PES10.runPlayerMacro("all_39_skills"); refreshSkills(lb.ItemIndex)
  end)
  makeModalBtn(f, S(398), y1, S(186), S(34), PES10.T("Limpar Todas as Skills", "Clear All Skills"), function()
    PES10.pushPlayerUndo(p, "Clear Skills")
    for _, sk in ipairs(items) do PES10.setBitsInDword(p + sk[1], sk[2], 1, 0) end
    refreshSkills(lb.ItemIndex)
  end)
  makeModalBtn(f, S(16),  y2, S(280), S(34), PES10.T("Desfazer Última Edição (Undo)", "Undo Last Edit"), function()
    PES10.undoLastPlayerEdit(p); cbStyle.ItemIndex = PES10.getBitsInDword(p + 0x1D, 3, 5); refreshSkills(lb.ItemIndex)
  end)
  makeModalBtn(f, S(304), y2, S(280), S(34), PES10.T("Fechar", "Close"), function() f.close() end)

  refreshSkills(0)
  f.show()
end

-- MODAL 7: VISUAL CONTRACT, SALARY, MARKET VALUE & EXACT RELEASE CLAUSE EDITOR
function PES10.openContractModal()
  if not createEdit or not createButton then return end
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local f, S, W, H = makeStudioModal("contract", PES10.T("Contrato, Finanças & Multa Exata - ", "Contract, Finances & Release Fee - ") .. pName, 580, 440)

  local c = PES10.getCurrentCurrency()
  makeModalLabel(f, S(16), S(12), W - S(32), S(22), pName .. "  (" .. c.symbol .. " " .. c.code .. ")", 11, 0xFCFAF8, true)
  makeModalLabel(f, S(16), S(36), W - S(32), S(18),
    PES10.T("Aceita valores naturais: 50M, 500k, 1.5B, 1000 (ou 0 na Multa Exata para remover override):",
            "Accepts natural amounts: 50M, 500k, 1.5B, 1000 (or 0 in Release Fee to clear override):"),
    8, 0xC4B2A0, false)

  local function addField(y, lblPt, lblEn, initVal)
    makeModalLabel(f, S(16), y + S(4), S(240), S(20), PES10.T(lblPt, lblEn), 9, 0xFCFAF8, false)
    local ed = createEdit(f)
    ed.Left = S(264); ed.Top = y; ed.Width = S(296); ed.Height = S(26); ed.Text = tostring(initVal or "")
    return ed
  end

  local edSal  = addField(S(68),  "Salário Anual Ativo:",         "Active Annual Salary:",          "")
  local edMkt  = addField(S(104), "Valor de Mercado Base:",       "Base Market Value:",             "")
  local edRfee = addField(S(140), "Multa Rescisória Exata:",      "Exact Custom Release Clause:",   "")
  local edExpY = addField(S(176), "Ano Término do Contrato:",     "Contract Expiry Year:",          "")
  local edAff  = addField(S(212), "Afeição pelo Clube (0-100%):", "Club Affection (0-100%):",       "")
  local initRfeeText = ""

  local lblFlags = makeModalLabel(f, S(16), S(252), W - S(32), S(20), "", 9, 0xF8BD38, true)
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

  local y1, y2, y3 = S(284), S(328), S(374)
  makeModalBtn(f, S(16),  y1, S(176), S(34), PES10.T("Aplicar Valores", "Apply Values"), function()
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
    PES10.toast(PES10.T("Contrato e valores financeiros atualizados para ", "Contract & finances updated for ") .. pName .. "!")
    refreshContract(true)
  end)
  makeModalBtn(f, S(198), y1, S(180), S(34), PES10.T("Alternar Multa (S/N)", "Toggle Release Clause"), function()
    PES10.pushPlayerUndo(p, "Toggle Clause")
    local v = PES10.getBitsInDword(p + 0x144, 30, 1)
    PES10.setBitsInDword(p + 0x144, 30, 1, v == 1 and 0 or 1)
    PES10.syncAllDynamicPointers(); refreshContract(false)
  end)
  makeModalBtn(f, S(384), y1, S(176), S(34), PES10.T("Alternar Lista Transf.", "Toggle Transfer List"), function()
    PES10.pushPlayerUndo(p, "Toggle Transfer List")
    local v = PES10.getBitsInDword(p + 0x14A, 1, 1)
    PES10.setBitsInDword(p + 0x14A, 1, 1, v == 1 and 0 or 1)
    refreshContract(false)
  end)

  makeModalBtn(f, S(16),  y2, S(180), S(34), PES10.T("Destravar Renegociação", "Unlock Renegotiation"), function()
    PES10.runPlayerMacro("unlock_renegotiation"); refreshContract(false)
  end)
  makeModalBtn(f, S(202), y2, S(180), S(34), PES10.T("Negociação 100% (€ 1k)", "Irresistible Deal (€ 1k)"), function()
    PES10.makeDealIrresistible(); refreshContract(true)
  end)
  makeModalBtn(f, S(388), y2, S(172), S(34), PES10.T("Alternar Empréstimo", "Toggle Loan List"), function()
    PES10.pushPlayerUndo(p, "Toggle Loan")
    local v = PES10.getBitsInDword(p + 0x14A, 2, 1)
    PES10.setBitsInDword(p + 0x14A, 2, 1, v == 1 and 0 or 1)
    refreshContract(false)
  end)

  makeModalBtn(f, S(16),  y3, S(268), S(34), PES10.T("Desfazer (Undo)", "Undo Edit"), function()
    PES10.undoLastPlayerEdit(p); refreshContract(true)
  end)
  makeModalBtn(f, S(292), y3, S(268), S(34), PES10.T("Fechar", "Close"), function() f.close() end)

  refreshContract(true)
  f.show()
end

-- MODAL 8: VISUAL PROFILE, PHYSICAL, CONDITION, LONGEVITY & JSON BACKUP EDITOR
function PES10.openBioConditionModal()
  if not createEdit or not createComboBox or not createButton then return end
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local pName = readString(p + 0x38, 34, false) or PES10.T("Atleta", "Player")
  local f, S, W, H = makeStudioModal("bio", PES10.T("Perfil, Físico, Condição & Backup - ", "Profile, Physical, Condition & Backup - ") .. pName, 600, 470)

  makeModalLabel(f, S(16), S(12), W - S(32), S(22), pName, 11, 0xFCFAF8, true)

  local function addPair(x, y, lbl, val)
    makeModalLabel(f, x, y + S(4), S(165), S(18), lbl, 9, 0xC4B2A0, false)
    local ed = createEdit(f)
    ed.Left = x + S(170); ed.Top = y; ed.Width = S(100); ed.Height = S(26); ed.Text = tostring(val)
    return ed
  end

  local edAge  = addPair(S(16),  S(48),  PES10.T("Idade (15-50):", "Age (15-50):"),             PES10.readU8(p + 0x1C) & 0x3F)
  local edHgt  = addPair(S(310), S(48),  PES10.T("Altura (cm):", "Height (cm):"),               PES10.readU8(p + 0x00))
  local edWgt  = addPair(S(16),  S(84),  PES10.T("Peso (kg):", "Weight (kg):"),                 PES10.readU8(p + 0x01))
  local edStam = addPair(S(310), S(84),  PES10.T("Stamina (0-100):", "Stamina (0-100):"),       PES10.getBitsInDword(p + 0x146, 0, 7))
  local edWfu  = addPair(S(16),  S(120), PES10.T("Uso Pé Ruim (1-4):", "Weak Foot Use (1-4):"), PES10.getBitsInDword(p + 0x23, 0, 2) + 1)
  local edWfa  = addPair(S(310), S(120), PES10.T("Prec. Pé Ruim (1-4):", "WF Accuracy (1-4):"), PES10.getBitsInDword(p + 0x22, 4, 2) + 1)
  local edFrm  = addPair(S(16),  S(156), PES10.T("Regularidade (1-8):", "Form / Cond (1-8):"),  PES10.getBitsInDword(p + 0x17, 4, 3) + 1)
  local edInjR = addPair(S(310), S(156), PES10.T("Res. Lesões (1-3):", "Injury Res (1-3):"),    PES10.getBitsInDword(p + 0x22, 6, 2) + 1)
  local edInjD = addPair(S(16),  S(192), PES10.T("Dias Lesionado (0=OK):", "Injury Days (0=OK):"), PES10.readU8(p + 0x148))

  makeModalLabel(f, S(310), S(196), S(115), S(18), PES10.T("Pé Dominante:", "Strong Foot:"), 9, 0xC4B2A0, false)
  local cbFoot = createComboBox(f)
  cbFoot.Left = S(430); cbFoot.Top = S(192); cbFoot.Width = S(150); cbFoot.Height = S(26); cbFoot.Style = "csDropDownList"
  cbFoot.Items.add(PES10.T("Direito (Destro)", "Right Foot"))
  cbFoot.Items.add(PES10.T("Esquerdo (Canhoto)", "Left Foot"))
  cbFoot.ItemIndex = PES10.getBitsInDword(p + 0x03, 7, 1)

  makeModalLabel(f, S(16), S(236), S(165), S(18), PES10.T("Seta de Forma Próx. Jogo:", "Next Match Form Arrow:"), 9, 0xC4B2A0, false)
  local cbArr = createComboBox(f)
  cbArr.Left = S(186); cbArr.Top = S(232); cbArr.Width = S(394); cbArr.Height = S(26); cbArr.Style = "csDropDownList"
  cbArr.Items.add(PES10.T("0: Vermelha (Péssima)", "0: Red Down (Worst)"))
  cbArr.Items.add(PES10.T("1: Laranja (Ruim)", "1: Orange Down-Right (Poor)"))
  cbArr.Items.add(PES10.T("2: Amarela (Normal)", "2: Yellow Flat (Normal)"))
  cbArr.Items.add(PES10.T("3: Verde (Boa)", "3: Green Up-Right (Good)"))
  cbArr.Items.add(PES10.T("4: Azul (Excelente / Topo)", "4: Blue Up (Top Form)"))
  cbArr.ItemIndex = math.max(0, math.min(4, PES10.getBitsInDword(p + 0x147, 0, 3)))

  local y1, y2, y3 = S(284), S(330), S(380)
  makeModalBtn(f, S(16),  y1, S(276), S(36), PES10.T("Aplicar Todas as Alterações", "Apply All Changes"), function()
    PES10.pushPlayerUndo(p, "Bio & Condition")
    local age = tonumber(edAge.Text); if age then PES10.setBitsInDword(p + 0x1C, 0, 6, math.max(15, math.min(50, math.floor(age)))) end
    local hgt = tonumber(edHgt.Text); if hgt then writeBytes(p + 0x00, math.max(140, math.min(220, math.floor(hgt)))) end
    local wgt = tonumber(edWgt.Text); if wgt then writeBytes(p + 0x01, math.max(45, math.min(130, math.floor(wgt)))) end
    local stm = tonumber(edStam.Text); if stm then PES10.setBitsInDword(p + 0x146, 0, 7, math.max(0, math.min(100, math.floor(stm)))) end
    local wfu = tonumber(edWfu.Text); if wfu then PES10.setBitsInDword(p + 0x23, 0, 2, math.max(0, math.min(3, math.floor(wfu) - 1))) end
    local wfa = tonumber(edWfa.Text); if wfa then PES10.setBitsInDword(p + 0x22, 4, 2, math.max(0, math.min(3, math.floor(wfa) - 1))) end
    local frm = tonumber(edFrm.Text); if frm then PES10.setBitsInDword(p + 0x17, 4, 3, math.max(0, math.min(7, math.floor(frm) - 1))) end
    local ijr = tonumber(edInjR.Text); if ijr then PES10.setBitsInDword(p + 0x22, 6, 2, math.max(0, math.min(2, math.floor(ijr) - 1))) end
    local ijd = tonumber(edInjD.Text); if ijd then writeBytes(p + 0x148, math.max(0, math.min(255, math.floor(ijd)))) end
    PES10.setBitsInDword(p + 0x03, 7, 1, cbFoot.ItemIndex or 0)
    PES10.setBitsInDword(p + 0x147, 0, 3, cbArr.ItemIndex or 4)
    PES10.setBitsInDword(p + 0x146, 7, 1, (cbArr.ItemIndex == 4) and 1 or 0)
    PES10.syncAllDynamicPointers()
    PES10.toast(PES10.T("Perfil, físico e condição atualizados para ", "Profile, physical & condition updated for ") .. pName .. "!")
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end)
  makeModalBtn(f, S(300), y1, S(280), S(36), PES10.T("Auge (Breakthrough) + Anti-Declínio 30+", "Breakthrough Surge + Anti-Decline 30+"), function()
    PES10.applyPlayerEternalPeak(p)
  end)

  local function reloadBioFields()
    edAge.Text  = tostring(PES10.readU8(p + 0x1C) & 0x3F)
    edHgt.Text  = tostring(PES10.readU8(p + 0x00))
    edWgt.Text  = tostring(PES10.readU8(p + 0x01))
    edStam.Text = tostring(PES10.getBitsInDword(p + 0x146, 0, 7))
    edWfu.Text  = tostring(PES10.getBitsInDword(p + 0x23, 0, 2) + 1)
    edWfa.Text  = tostring(PES10.getBitsInDword(p + 0x22, 4, 2) + 1)
    edFrm.Text  = tostring(PES10.getBitsInDword(p + 0x17, 4, 3) + 1)
    edInjR.Text = tostring(PES10.getBitsInDword(p + 0x22, 6, 2) + 1)
    edInjD.Text = tostring(PES10.readU8(p + 0x148))
    cbFoot.ItemIndex = PES10.getBitsInDword(p + 0x03, 7, 1)
    cbArr.ItemIndex  = math.max(0, math.min(4, PES10.getBitsInDword(p + 0x147, 0, 3)))
    if PES10.dashboardState and PES10.dashboardState.render then PES10.dashboardState.render(true) end
  end

  makeModalBtn(f, S(16),  y2, S(182), S(34), PES10.T("Rejuvenescer (-2 Anos)", "Rejuvenate (-2 Yrs)"), function()
    PES10.rejuvenateAndSyncMarket(-2)
    reloadBioFields()
  end)
  makeModalBtn(f, S(204), y2, S(186), S(34), PES10.T("Exportar Backup (.json)", "Export Backup (.json)"), function()
    PES10.exportPlayerBackupJson()
  end)
  makeModalBtn(f, S(396), y2, S(184), S(34), PES10.T("Importar Backup (.json)", "Import Backup (.json)"), function()
    PES10.importPlayerBackupJson()
    reloadBioFields()
  end)

  makeModalBtn(f, S(16),  y3, S(276), S(34), PES10.T("Desfazer (Undo)", "Undo Edit"), function()
    PES10.undoLastPlayerEdit(p)
    reloadBioFields()
  end)
  makeModalBtn(f, S(300), y3, S(280), S(34), PES10.T("Fechar", "Close"), function() f.close() end)

  f.show()
end

-- ============================================================================
-- PES 2021 v11.0 ULTIMATE STUDIO - DOUBLE-BUFFERED OFFSCREEN CANVAS GUI (PT / EN)
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
  f.Caption = "PES 2021 - v11.0 Ultimate Studio (PT-BR / EN)"
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
      { col=1, row=1, accent=C_GREEN,
        tag_pt="+50M", tag_en="+50M",
        title_pt="+50M Transferências", title_en="+50M Transfer Budget",
        sub_pt="Soma +50M ao caixa de contratações", sub_en="Adds +50M to transfer funds",
        fn=function()
          local c = PES10.readU64(PES10.symAddr("ptrCareer"))
          if c == 0 then PES10.toast(PES10.T("Carregue sua Master League primeiro!", "Load your Master League save first!")); return end
          writeInteger(c + 0x016ECBF4, math.min(2147483647, PES10.readI32(c + 0x016ECBF4) + 500000))
          PES10.toast(PES10.T("+50M adicionados em Transferências!", "+50M added to Transfer Budget!"))
        end },
      { col=2, row=1, accent=C_GOLD,
        tag_pt="CAIXA", tag_en="TYCOON",
        title_pt="+500M Transf. / +50M Sal.", title_en="+500M Transf / +50M Wage",
        sub_pt="Injeta caixa imediato nos dois orçamentos", sub_en="Boosts both transfer and wage budgets",
        fn=function()
          local c = PES10.readU64(PES10.symAddr("ptrCareer"))
          if c == 0 then PES10.toast(PES10.T("Carregue sua Master League primeiro!", "Load your Master League save first!")); return end
          writeInteger(c + 0x016ECBF4, math.min(2147483647, PES10.readI32(c + 0x016ECBF4) + 5000000))
          writeInteger(c + 0x016ECC08, math.min(2147483647, PES10.readI32(c + 0x016ECC08) + 500000))
          PES10.toast(PES10.T("+500M Transf. e +50M Salários!", "+500M Transfer & +50M Wage added!"))
        end },
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
        title_pt="Estilo & 41 Habilidades", title_en="Style & 41 Skills",
        sub_pt="Playing Style e toggle por Skill", sub_en="Playing Style & per-skill toggle",
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
        title_pt="Perfil, Físico & JSON", title_en="Profile, Form & JSON",
        sub_pt="Idade, pé ruim, forma, auge e backup", sub_en="Age, weak foot, form, peak & backup",
        fn=function() PES10.openBioConditionModal() end },
      { col=1, row=4, accent=C_GOLD,
        tag_pt="OVR ALVO", tag_en="TARGET",
        title_pt="Overall Alvo (40-99)", title_en="Target Overall (40-99)",
        sub_pt="Escala OVR principal mantendo DNA", sub_en="Scales primary OVR keeping DNA",
        fn=function() PES10.promptTargetOVRProportional() end },
      { col=2, row=4, accent=C_CRIMSON,
        tag_pt="FULL", tag_en="FULL",
        title_pt="41 Skills + 13 Posições 'A'", title_en="41 Skills + All 'A' Pos",
        sub_pt="Skills, posições A, pé 4/4 e forma 8", sub_en="All skills, 'A' pos, WF 4/4 & Form 8",
        fn=function()
          local p = PES10.getSelectedPlayerOrWarn()
          if p == 0 then return end
          PES10.runPlayerMacro("all_positions_a")
          PES10.runPlayerMacro("all_39_skills")
          PES10.runPlayerMacro("perfect_wf_form_injury")
          PES10.toast(PES10.T("Pacote Completo aplicado!", "Full Pack applied!"))
        end },
    },

    [3] = {
      { col=1, row=1, accent=C_GREEN,
        tag_pt="10X XP", tag_en="10X XP",
        title_pt="Treino 10x", title_en="10x Training",
        sub_pt="Evolução 10x no seu clube/base/BAL", sub_en="10x Sub-XP for club, youth & BAL",
        isActive=function(tel) return tel.trnMode == 3 end,
        fn=function()
          local a = PES10.symAddr("cfgTrainingMultMode")
          if a ~= 0 then writeInteger(a, 3); PES10.ensureHookActive(3); PES10.toast(PES10.T("Treino 10x ativado!", "10x Training active!")) end
        end },
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
        tag_pt="99 PTS", tag_en="99 PTS",
        title_pt="BAL: 10 Barras + 99 Pts", title_en="BAL: Max Sliders + 99 Pts",
        sub_pt="Libera trava de 21 Focus Points", sub_en="Unlocks 21-pt cap & maxes bars",
        fn=function() PES10.runPlayerMacro("bal_max_sliders") end },
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
          local ans = inputQuery(
            PES10.T("Minuto da Partida", "Match Minute"),
            PES10.T("Digite o minuto (ex: 15, 44, 88 ou 44:30):", "Enter minute (e.g. 15, 44, 88 or 44:30):"),
            tostring(info.mm)
          )
          if ans then
            local mm, ss = ans:match("^(%d+)%s*:%s*(%d+)$")
            local tMin = mm and (tonumber(mm) + tonumber(ss) / 60.0) or tonumber((ans:gsub(",", ".")))
            if tMin then PES10.setExactMatchMinute(tMin) end
          end
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
          local ans = inputQuery(
            PES10.T("Placar (Casa x Fora)", "Score (Home - Away)"),
            PES10.T("Formato CASA-FORA (ex: 3-0):", "Format HOME-AWAY (e.g. 3-0):"),
            string.format("%d-%d", hCur, aCur)
          )
          if ans then
            local hg, ag = ans:match("^(%d+)%s*[%-%s:xX]%s*(%d+)$")
            if hg and ag then
              local hi, ai = tonumber(hg), tonumber(ag)
              if mCore ~= 0 then writeBytes(mCore + 0x9E, hi); writeBytes(mCore + 0x9F, ai) end
              if mHud ~= 0 then writeInteger(mHud + 0x120, hi); writeInteger(mHud + 0x124, ai) end
              PES10.toast(string.format(PES10.T("Placar: %d x %d!", "Score: %d - %d!"), hi, ai))
            end
          end
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
      { col=2, row=4, accent=C_GREEN,
        tag_pt="FORA", tag_en="AWAY",
        title_pt="Stamina 100% (Fora)", title_en="100% Stamina (Away)",
        sub_pt="Liga ou desliga fôlego infinito", sub_en="Toggle infinite Away stamina",
        isActive=function(tel) return tel.stamMode == 2 end,
        fn=function()
          local a = PES10.symAddr("cfgStaminaMode")
          if a == 0 then return end
          if PES10.readU32(a) == 2 then
            writeInteger(a, 0)
            PES10.toast(PES10.T("Stamina restaurada.", "Stamina restored."))
          else
            writeInteger(a, 2); PES10.ensureHookActive(6)
            PES10.toast(PES10.T("Stamina 100% (Fora) ativa!", "Away 100% Stamina active!"))
          end
        end },
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
      local sb = PES10.formatMoneyFromRaw(PES10.readI32(career + 0x016ECC08))
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
    drawTextLeft(c, PAD_X, S(12), C_HDR, "PES 2021  •  v11.0 ULTIMATE STUDIO", maxHdrTextW)
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
    setFont(c, 8, C_SUBTEXT, true)
    drawTextCentered(c, expX, jBtnY, jBtnW, jBtnH, expBg, PES10.T("Exportar .JSON", "Export .JSON"))
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

  local tickCounter = 0
  local uiTimer = createTimer(f, true)
  uiTimer.Interval = 75
  uiTimer.OnTimer = function()
    if not f.Visible then return end
    if getMousePos then
      local mx, my = getMousePos()
      if mx and my then
        local cx, cy = surface.screenToClient(mx, my)
        if cx and cy and cx >= 0 and cx < W and cy >= 0 and cy < H then
          updateHoverAt(cx, cy)
        elseif hoverId ~= nil then
          updateHoverAt(-1, -1)
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
      local pid = getProcessIDFromProcessName(PES10.PROCESS_NAME)
      if pid and pid > 0 then
        openProcess(pid)
        PES10.initProcessState()
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
