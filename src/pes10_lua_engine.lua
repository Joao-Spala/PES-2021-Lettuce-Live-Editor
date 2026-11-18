-- ============================================================================
-- PES 2021 - v10.0 PRO OVERHAUL (BMPES 16.0 READY) — CORE LUA & STUDIO GUI
-- Validado ao vivo em PES2021.exe v1.7.2.0 + BMPES 16.0 Sider 7.3.3
-- ============================================================================

PES10 = PES10 or {}
PES10.VERSION = "10.0 Pro Studio (BMPES 16.0 Ready)"
PES10.PROCESS_NAME = "PES2021.exe"
PES10.attachedPid = 0
PES10.stateBase = 0
PES10.addrCareerOwner = 0
PES10.addrMatchMgr = 0
PES10.dashboardForm = nil
PES10.lastToastMsg = "Pronto — Selecione qualquer ação rápida ou edite direto na tabela."

-- Moedas suportadas (Inicia em 0 = Euro € por padrão!)
PES10.CURRENCIES = {
  [0] = { symbol = "€",  code = "EUR", sep = ".", dec = "," },
  [1] = { symbol = "R$", code = "BRL", sep = ".", dec = "," },
  [2] = { symbol = "$",  code = "USD", sep = ",", dec = "." },
  [3] = { symbol = "£",  code = "GBP", sep = ",", dec = "." },
}
PES10.currencyIndex = 0 -- Padrão inicial: Euro (€)

-- Dicionários estáticos de fallback BMPES 16.0 (além do leitor direto da RAM em +0xADF4BC!)
PES10.BMPES_TEAMS = __BMPES_TEAMS_LUA__
PES10.BMPES_COMPS = __BMPES_COMPS_LUA__

-- Layout do bloco compartilhado pes10State ($1000 bytes próximo ao PES2021.exe)
PES10.SYM_OFFSETS = {
  ptrCareer               = 0x000,
  ptrBudget               = 0x000, -- Alias de compatibilidade
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
  rfeeOverrideTable       = 0x100, -- 64 slots * 16 bytes (0x100..0x4FF)
}

-- ============================================================================
-- LEITURA / ESCRITA SEGURA & PROTEÇÃO OFFLINE (ZERO EXCEÇÕES PASCAL/SEH)
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

-- Normalização sem acentos PT-BR/ES para busca inteligente
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

PES10.HOOK_IDS_BY_NUM = {}
function PES10.ensureHookActive(hookNum)
  if not PES10.isAttached() then return end
  local id = PES10.HOOK_IDS_BY_NUM and PES10.HOOK_IDS_BY_NUM[hookNum]
  if not id then return end
  local al = getAddressList()
  if not al then return end
  local mr = al.getMemoryRecordByID(id)
  if mr and not mr.Active then
    pcall(function() mr.Active = true end)
  end
end

function PES10.activateAllHooks()
  if not PES10.isAttached() then
    PES10.toast("Abra o PES2021.exe primeiro para ativar os hooks!")
    return 0
  end
  PES10.initProcessState()
  local al = getAddressList()
  local okCount = 0
  for num = 1, 6 do
    local id = PES10.HOOK_IDS_BY_NUM[num]
    local mr = id and al.getMemoryRecordByID(id) or nil
    if mr then
      if not mr.Active then pcall(function() mr.Active = true end) end
      if mr.Active then okCount = okCount + 1 end
    end
  end
  PES10.toast(string.format("Hooks Mestres ativos: %d / 6 ligados com sucesso!", okCount))
  return okCount
end

function PES10.toast(msg)
  PES10.lastToastMsg = "✓ " .. tostring(msg)
end

-- ============================================================================
-- FORMATAÇÃO MONETÁRIA HUMANA (€ 125.500.000) & PARSER (50M, 500k, 1.5B)
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
  PES10.toast("Moeda alterada para " .. PES10.CURRENCIES[idx].symbol .. " (" .. PES10.CURRENCIES[idx].code .. ")")
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
-- DECODIFICADOR AO VIVO DE CLUBES/SELEÇÕES DIRETO DA RAM (+0xADF4BC, STRIDE 0x690)
-- ============================================================================
function PES10.decodeClubHandle(rawU32)
  local v = tonumber(rawU32)
  if not v then return 0, 0, "Desconhecido" end
  if v < 0 then v = v + 0x100000000 end
  if v == 0xFFFFFFFF or v == 0 then
    return 0, 0, "Sem Clube / Livre"
  end
  local slot = v % 16384
  local teamId = math.floor(v / 16384)

  -- Lê o nome UTF-8 real e a sigla de 3 letras direto da tabela de clubes da carreira!
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
  return teamId, slot, string.format("Time ID #%d (Slot %d)", teamId, slot)
end

-- Detecta se a carreira aberta é BAL ou Master League + Nome do Clube do Usuário!
function PES10.getCareerModeAndClubInfo()
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    return "Menu / Exibição", "", 0
  end
  local balHandle = PES10.readU64(career + 0x01819540)
  local isBAL = (math.floor(balHandle / 0x100000000) ~= 0)
  if isBAL then
    return "Rumo ao Estrelato (BAL)", "", career
  end
  -- Na Master League, obtém o clube do usuário a partir do 1º atleta da folha (+0x16EE854)
  local clubName = ""
  local h0 = PES10.readU64(career + 0x016EE854)
  if math.floor(h0 / 0x100000000) ~= 0 then
    local lowSlot = h0 % 0x100000000
    -- Verifica índice rápido ou busca na lista
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
  return "Master League", clubName, career
end

-- ============================================================================
-- INICIALIZAÇÃO DO BLOCO pes10State & SINGLETONS RIP-RELATIVE
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
  if not PES10.isAttached() then return false end
  local pid = getOpenedProcessID()
  if PES10.attachedPid == pid and PES10.stateBase ~= 0 then
    return true
  end

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
    PES10.addrCareerOwner = exeBase + 0x3714240
    PES10.safeRegisterSymbol("addrCareerOwner", PES10.addrCareerOwner)
  end

  local matchSite = PES10.scanUniqueModuleAOB("48 8B 05 ?? ?? ?? ?? 4C 8B E2 45 33 FF 48 8B B0 70 02 00 00")
  if matchSite ~= 0 then
    local disp = PES10.readI32(matchSite + 3)
    PES10.addrMatchMgr = matchSite + 7 + disp
    PES10.safeRegisterSymbol("addrMatchMgr", PES10.addrMatchMgr)
  else
    PES10.addrMatchMgr = exeBase + 0x3702208
    PES10.safeRegisterSymbol("addrMatchMgr", PES10.addrMatchMgr)
  end

  PES10.attachedPid = pid
  PES10.syncAllDynamicPointers()
  return true
end

-- ============================================================================
-- SINCRONIZAÇÃO DINÂMICA EM TEMPO REAL (PONTEIROS & FÓRMULA DE MULTA KONAMI)
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

  -- Fórmula fiel decompilada de fcn.14156BE00 + fcn.14156BF80!
  local age = PES10.readU8(playerAddr + 0x1C) & 0x3F
  local affection = PES10.getBitsInDword(playerAddr + 0x13C, 18, 7)
  local f12 = (affection * 0.7) - 50.0
  local i4 = math.floor((50 - age) - 20 + f12)
  if i4 > 100 then i4 = 100 elseif i4 < -50 then i4 = -50 end
  local f8 = (i4 / 100.0) + 1.0
  return math.max(1, math.floor(mkt * f8 + 0.5))
end

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
    local balRec = (math.floor(balHandle / 0x100000000) ~= 0) and PES10.findRecordByHandle64(career + 0x016BF710, 82, 0x170, balHandle) or 0
    writeQword(sb + PES10.SYM_OFFSETS.ptrBALTrainingRec, balRec)
  else
    writeQword(sb + PES10.SYM_OFFSETS.ptrBALTrainingRec, 0)
  end

  local pAddr = PES10.readU64(sb + PES10.SYM_OFFSETS.ptrPlayer)
  if pAddr ~= 0 and career ~= 0 then
    local pHandle = PES10.readU64(pAddr + 0x2C)
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
    writeInteger(sb + PES10.SYM_OFFSETS.valComputedReleaseFee, PES10.estimateReleaseFeeRaw(career, pAddr, pHandle))
  else
    writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerTraining, 0)
    writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerSquadBonus, 0)
    writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerReleaseSlot, 0)
    writeQword(sb + PES10.SYM_OFFSETS.ptrPlayerGrowthDesc, 0)
  end
end

-- ============================================================================
-- CONTROLE REAL DO RELÓGIO DA PARTIDA (SEM CRASH FNV-1a!)
-- ============================================================================
PES10.PERIOD_NAMES = {
  [0] = "Pré-Jogo", [1] = "1º Tempo", [2] = "Intervalo", [3] = "2º Tempo",
  [4] = "Fim 90'", [5] = "1ª Prorr.", [6] = "Int. Prorr.", [7] = "2ª Prorr.", [8] = "Pênaltis",
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

  return {
    matchState = mState, period = period,
    periodName = PES10.PERIOD_NAMES[period] or ("P" .. tostring(period)),
    baseMin = baseMin, protTicks = protTicks, offTicks = offTicks, halfTicks = halfTicks,
    mm = mm, ss = ss,
    formatted = string.format("%02d:%02d (%s)", mm, ss, PES10.PERIOD_NAMES[period] or tostring(period)),
  }
end

function PES10.setExactMatchMinute(targetMinuteFloat)
  PES10.ensureHookActive(5)
  local info = PES10.getMatchClockInfo()
  if not info then
    PES10.toast("Nenhuma partida ao vivo em andamento no momento.")
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
  PES10.toast(string.format("Relógio ajustado para %02d:%02d!", math.floor(targetMinuteFloat), math.floor((targetMinuteFloat % 1) * 60 + 0.5)))
  return true
end

function PES10.shiftMatchMinutes(deltaMinutes)
  local info = PES10.getMatchClockInfo()
  if not info then PES10.toast("Entre em uma partida primeiro!"); return end
  local curMin = info.baseMin + (info.halfTicks / 3600.0)
  PES10.setExactMatchMinute(math.max(info.baseMin, curMin + deltaMinutes))
end

function PES10.endCurrentHalfNow()
  PES10.ensureHookActive(5)
  local info = PES10.getMatchClockInfo()
  if not info then PES10.toast("Entre em uma partida primeiro!"); return end
  local maxHalfMin = (info.period >= 5) and 15.5 or 46.5
  writeFloat(info.matchState + 0x1174, (maxHalfMin * 3600.0) - info.protTicks)
  PES10.toast("Fim do tempo atual acionado!")
end

function PES10.resetCurrentHalfNow()
  PES10.ensureHookActive(5)
  local info = PES10.getMatchClockInfo()
  if not info then PES10.toast("Entre em uma partida primeiro!"); return end
  writeFloat(info.matchState + 0x1174, 0.0 - info.protTicks)
  if PES10.readU32(PES10.symAddr("cfgFreezeClock")) == 1 then
    writeFloat(PES10.symAddr("valFrozenTotalTicks"), 0.0)
  end
  PES10.toast("Tempo atual reiniciado para o início!")
end

-- ============================================================================
-- OVERRIDE EXATO DE TAXA DE RESCISÃO (64 SLOTS)
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
  PES10.toast("Override de Taxa de Rescisão removido para este atleta.")
end

-- ============================================================================
-- BUSCA GLOBAL DE JOGADORES (30.000 ATLETAS) & SELEÇÃO MEU PRO BAL
-- ============================================================================
function PES10.pointToBALMyPlayer()
  PES10.syncAllDynamicPointers()
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    PES10.toast("Carregue um save de Rumo ao Estrelato (BAL) primeiro!")
    return false
  end
  local balHandle = PES10.readU64(career + 0x01819540)
  if math.floor(balHandle / 0x100000000) == 0 then
    PES10.toast("Save atual é Master League (não há jogador único de BAL ativo).")
    return false
  end
  local count = PES10.readU32(career + 0x00D0BCE8)
  if count == 0 or count > 30001 then count = 30000 end
  for i = 0, count - 1 do
    local p = career + (i * 0x17C)
    if PES10.readU64(p + 0x2C) == balHandle then
      writeQword(PES10.symAddr("ptrPlayer"), p)
      PES10.syncAllDynamicPointers()
      local nm = readString(p + 0x38, 46, false) or "Meu Pro"
      PES10.toast("Selecionado atleta do BAL: " .. nm)
      return true
    end
  end
  return false
end

function PES10.searchCareerPlayerDialog()
  PES10.syncAllDynamicPointers()
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))
  if career == 0 then
    showMessage("Carregue um save de Master League ou Rumo ao Estrelato primeiro!")
    return
  end

  local query = inputQuery(
    "PES 2021 v10.0 — Buscar Jogador na Carreira",
    "Digite parte do Nome (com ou sem acento) ou o Player ID:",
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
        local _, _, clubDesc = PES10.decodeClubHandle(PES10.readU32(p + 0x12C))
        matches[#matches + 1] = { addr = p, id = pid, name = name, age = age, club = clubDesc }
        if #matches >= 80 then break end
      end
    end
  end

  if #matches == 0 then
    PES10.toast("Nenhum jogador encontrado para '" .. query .. "'.")
    showMessage("Nenhum jogador encontrado para: " .. query)
    return
  elseif #matches == 1 then
    writeQword(PES10.symAddr("ptrPlayer"), matches[1].addr)
    PES10.syncAllDynamicPointers()
    PES10.toast(string.format("Atleta carregado: %s (%s)", matches[1].name, matches[1].club))
    return
  end

  local sl = createStringlist()
  for _, m in ipairs(matches) do
    sl.add(string.format("%s  |  %d anos  |  %s  |  ID %d", m.name, m.age, m.club, m.id))
  end
  local idx = showSelectionList(
    "Jogadores Encontrados (" .. tostring(#matches) .. ")",
    "Dê duplo-clique no atleta desejado para carregá-lo no Edit Player v10.0:",
    sl,
    false
  )
  sl.destroy()
  if idx and idx >= 0 and matches[idx + 1] then
    writeQword(PES10.symAddr("ptrPlayer"), matches[idx + 1].addr)
    PES10.syncAllDynamicPointers()
    PES10.toast(string.format("Atleta carregado: %s (%s)", matches[idx + 1].name, matches[idx + 1].club))
  end
end

-- ============================================================================
-- SQUAD COMMANDER — AÇÕES EM TODO O MEU ELENCO (MASTER LEAGUE)
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
    PES10.toast("Carregue um save de Master League primeiro!")
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
      PES10.setBitsInDword(p + 0x13C, 18, 7, 100) -- Real 7-bit Affection = 100%!
    elseif mode == "shield_contracts" then
      writeSmallInteger(p + 0x138, curYear + 5)
      PES10.setBitsInDword(p + 0x13C, 18, 7, 100)
      PES10.setBitsInDword(p + 0x144, 30, 1, 0) -- Remove multa p/ blindar contra rivais!
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
    elseif mode == "plus_one_all_stats" then
      for _, sb in ipairs(PES10.PLAYER_STAT_BITS) do
        local cur = PES10.getBitsInDword(p + sb[1], sb[2], 7)
        if cur < 99 then PES10.setBitsInDword(p + sb[1], sb[2], 7, math.min(99, cur + 1)) end
      end
    end
  end
  PES10.toast(string.format("Comando aplicado em %d jogadores do seu elenco!", #squad))
  return #squad
end

-- ============================================================================
-- MACROS DE 1 CLIQUE PARA O JOGADOR SELECIONADO (ptrPlayer) & BACKUP JSON
-- ============================================================================
function PES10.getSelectedPlayerOrWarn()
  PES10.syncAllDynamicPointers()
  local p = PES10.readU64(PES10.symAddr("ptrPlayer"))
  if p == 0 then
    PES10.toast("Selecione um jogador no menu do jogo ou clique em 'Buscar Jogador na Carreira'!")
    return 0
  end
  return p
end

function PES10.runPlayerMacro(mode)
  local career = PES10.readU64(PES10.symAddr("ptrCareer"))

  if mode == "bal_instant_skill" then
    if career == 0 then PES10.toast("Abra seu save BAL primeiro!"); return false end
    local skillId = PES10.readI32(career + 0x016ECBEC)
    local balRec = PES10.readU64(PES10.symAddr("ptrBALTrainingRec"))
    if balRec ~= 0 and skillId >= 0 and skillId < 0x6C then
      writeSmallInteger(balRec + 0x16 + skillId * 2, 9999)
      PES10.toast("Skill em treino colocada em 99,99% (completa no próximo avanço)!")
      return true
    end
    PES10.toast("Escolha primeiro uma Skill para treinar no menu do BAL!")
    return false
  elseif mode == "bal_instant_pos" then
    if career == 0 then PES10.toast("Abra seu save BAL primeiro!"); return false end
    local posId = PES10.readI32(career + 0x016ECBF0)
    local balRec = PES10.readU64(PES10.symAddr("ptrBALTrainingRec"))
    if balRec ~= 0 and posId > 0 and posId < 0x0E then
      writeSmallInteger(balRec + 0x16 + posId * 2, 9999)
      PES10.toast("Posição em treino colocada em 99,99% (completa no próximo avanço)!")
      return true
    end
    PES10.toast("Escolha primeiro uma Posição para treinar no menu do BAL!")
    return false
  elseif mode == "bal_max_sliders" then
    if career == 0 then PES10.toast("Abra seu save BAL primeiro!"); return false end
    PES10.ensureHookActive(4)
    for i = 0, 9 do writeBytes(career + 0x016ECBE0 + i, 6) end
    writeBytes(career + 0x016ECBEA, 99)
    PES10.toast("10 Barras no máximo + 99 Focus Points liberados no BAL!")
    return true
  end

  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return false end
  local pName = readString(p + 0x38, 36, false) or "Atleta"

  if mode == "stats_99" then
    for _, sb in ipairs(PES10.PLAYER_STAT_BITS) do PES10.setBitsInDword(p + sb[1], sb[2], 7, 99) end
    PES10.toast("Todos os 25 atributos de " .. pName .. " definidos em 99!")
  elseif mode == "stats_plus_5" then
    for _, sb in ipairs(PES10.PLAYER_STAT_BITS) do
      local v = PES10.getBitsInDword(p + sb[1], sb[2], 7)
      PES10.setBitsInDword(p + sb[1], sb[2], 7, math.min(99, v + 5))
    end
    PES10.toast("+5 em todos os atributos aplicado para " .. pName .. "!")
  elseif mode == "all_positions_a" then
    local posBits = {
      {0x1F, 1}, {0x1F, 3}, {0x1F, 5}, {0x20, 0}, {0x20, 2}, {0x20, 4}, {0x20, 6},
      {0x21, 0}, {0x21, 2}, {0x21, 4}, {0x21, 6}, {0x22, 0}, {0x22, 2},
    }
    for _, pb in ipairs(posBits) do PES10.setBitsInDword(p + pb[1], pb[2], 2, 2) end
    PES10.toast("Todas as 13 posições definidas como 'A' para " .. pName .. "!")
  elseif mode == "all_39_skills" then
    for _, sk in ipairs(PES10.ALL_SKILL_BITS or {}) do PES10.setBitsInDword(p + sk[1], sk[2], 1, 1) end
    PES10.toast("Todas as 41 habilidades desbloqueadas para " .. pName .. "!")
  elseif mode == "perfect_wf_form_injury" then
    PES10.setBitsInDword(p + 0x23, 0, 2, 3)
    PES10.setBitsInDword(p + 0x22, 4, 2, 3)
    PES10.setBitsInDword(p + 0x17, 4, 3, 7)
    PES10.setBitsInDword(p + 0x22, 6, 2, 2)
    writeBytes(p + 0x148, 0)
    PES10.toast("Pé Ruim 4/4, Forma 8, Lesão 3 e saúde 100% em " .. pName .. "!")
  elseif mode == "unlock_renegotiation" then
    -- Limpa sentinel de renovação recente (0x0000FFFF) e recua chegada se recente!
    writeInteger(p + 0x16C, 0x0000FFFF)
    local joinYear = PES10.readU16(p + 0x168)
    if joinYear >= 1995 and joinYear <= 2200 then
      writeSmallInteger(p + 0x168, math.max(2018, joinYear - 2))
    end
    PES10.toast("Bloqueio de contrato recente removido para " .. pName .. "!")
  elseif mode == "easy_sign_target" then
    PES10.setOverrideReleaseFeeForPlayer(p, 10) -- € 1.000 (raw = 10)
    PES10.setBitsInDword(p + 0x13C, 18, 7, 100) -- Real 7-bit Affection = 100%
    writeInteger(p + 0x16C, 0x0000FFFF)
    local joinYear = PES10.readU16(p + 0x168)
    if joinYear >= 1995 and joinYear <= 2200 then
      writeSmallInteger(p + 0x168, math.max(2018, joinYear - 2))
    end
    PES10.toast("Contratação Fácil pronta em " .. pName .. ": Multa € 1.000 + Afeição 100%!")
  elseif mode == "instant_subxp_player" then
    local trn = PES10.readU64(PES10.symAddr("ptrPlayerTraining"))
    if trn == 0 then
      PES10.toast(pName .. " não pertence aos 82 slots de treino do seu clube/BAL.")
      return false
    end
    for fid = 14, 39 do writeSmallInteger(trn + 0x16 + fid * 2, 9999) end
    PES10.toast("Todas as 25 barras de Sub-XP de " .. pName .. " em 9.999 / 10.000!")
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
  dlg.Title = "Exportar Backup JSON do Jogador"
  dlg.Filter = "JSON Files (*.json)|*.json"
  dlg.FileName = string.format("PES2021_%s_%d.json", name, pid)
  if dlg.execute() then
    local hexArr = {}
    for i = 1, #rawBytes do hexArr[i] = string.format("%02X", rawBytes[i]) end
    local content = string.format(
      '{\n  "version": "PES2021_v10.0",\n  "player_id": %d,\n  "player_name": %q,\n  "struct_size": 380,\n  "hex_0x17C": "%s"\n}\n',
      pid, readString(p + 0x38, 60, false) or "", table.concat(hexArr, "")
    )
    local f = io.open(dlg.FileName, "w")
    if f then f:write(content); f:close(); PES10.toast("Backup JSON exportado: " .. dlg.FileName) end
  end
  dlg.destroy()
end

function PES10.importPlayerBackupJson()
  local p = PES10.getSelectedPlayerOrWarn()
  if p == 0 then return end
  local dlg = createOpenDialog(MainForm)
  dlg.Title = "Importar Backup JSON no Jogador Selecionado"
  dlg.Filter = "JSON Files (*.json)|*.json"
  if dlg.execute() then
    local f = io.open(dlg.FileName, "r")
    if f then
      local txt = f:read("*a"); f:close()
      local hex = txt:match('"hex_0x17C"%s*:%s*"([0-9A-Fa-f]+)"')
      if hex and #hex == 760 then
        local curBytes = readBytes(p, 0x17C, true)
        local newBytes = {}
        for i = 1, 0x17C do newBytes[i] = tonumber(hex:sub((i - 1) * 2 + 1, i * 2), 16) end
        for keep = 0x2C, 0x33 do newBytes[keep + 1] = curBytes[keep + 1] end
        for keep = 0x12C, 0x137 do newBytes[keep + 1] = curBytes[keep + 1] end
        for keep = 0x160, 0x167 do newBytes[keep + 1] = curBytes[keep + 1] end
        writeBytes(p, newBytes)
        PES10.syncAllDynamicPointers()
        PES10.toast("Backup JSON importado com sucesso (ID e Clube preservados)!")
      end
    end
  end
  dlg.destroy()
end

-- ============================================================================
-- HOOKS DE FORMATAÇÃO & DUPLO-CLIQUE NA ADDRESSLIST DO CHEAT ENGINE
-- ============================================================================
PES10.MONEY_DESC_TAGS = {}
PES10.CLUB_HANDLE_IDS = {}
PES10.CLOCK_RECORD_ID = -1
PES10.STATUS_RECORD_ID = -1
PES10.CUSTOM_RFEE_RECORD_ID = -1
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
    elseif id == PES10.CLOCK_RECORD_ID then
      mr.OnGetDisplayValue = function(rec, valStr)
        local info = PES10.getMatchClockInfo()
        if not info then return true, "Fora de Partida" end
        return true, info.formatted
      end
    elseif id == PES10.STATUS_RECORD_ID then
      mr.OnGetDisplayValue = function(rec, valStr)
        if not PES10.isAttached() then return true, "Aguardando PES2021.exe..." end
        local modeStr, clubStr = PES10.getCareerModeAndClubInfo()
        local p = PES10.readU64(PES10.symAddr("ptrPlayer"))
        local pName = (p ~= 0) and (readString(p + 0x38, 36, false) or "Atleta") or "Nenhum"
        if clubStr ~= "" then modeStr = modeStr .. " (" .. clubStr .. ")" end
        return true, string.format("Online | %s | Atleta: %s", modeStr, pName)
      end
    end
  end

  al.OnValueChange = function(addrlist, memrec)
    local id = memrec.ID
    if PES10.MONEY_DESC_TAGS[id] then
      local c = PES10.getCurrentCurrency()
      local curRaw = tonumber(memrec.Value) or 0
      local ans = inputQuery(
        "Editar Valor Financeiro (" .. c.symbol .. ")",
        "Digite o valor (aceita 50M, 500k, 1.5B, 1000 ou 80.000.000):",
        PES10.formatMoneyFromRaw(curRaw)
      )
      if not ans then return true end
      local newRaw = PES10.parseHumanMoneyToRaw(ans)
      if not newRaw then showMessage("Valor inválido: " .. tostring(ans)); return true end
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
      if not info then showMessage("Entre em uma partida ao vivo primeiro!"); return true end
      local ans = inputQuery(
        "Definir Minuto Exato da Partida",
        "Digite o minuto desejado (ex: 15, 44, 88 ou 44:30):",
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
      local ans = inputQuery("Editar Placar ao Vivo", "Digite o novo número de gols (0–99):", memrec.Value)
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
-- PES 2021 v10.0 PRO STUDIO — PAINEL GRÁFICO DARK MULTI-ABAS (DPI-RESPONSIVO!)
-- Zero botões nativos cinzas / Zero texto cortado em 100%, 125% ou 150% DPI!
-- ============================================================================
function PES10.openDashboard()
  if PES10.dashboardForm then
    PES10.dashboardForm.show()
    PES10.dashboardForm.bringToFront()
    return
  end

  local dpi = (getScreenDPI and getScreenDPI()) or 96
  if dpi < 96 then dpi = 96 end
  local scale = dpi / 96.0
  local function S(px) return math.floor(px * scale + 0.5) end

  -- Paleta Dark Studio Pro (BGR: 0xBBGGRR)
  local C_BG         = 0x16110D -- #0D1116 Deep Obsidian
  local C_HEADER     = 0x211913 -- #131921 Studio Header
  local C_TABBAR     = 0x1B1510 -- #10151B Navigation Strip
  local C_TAB_ACT    = 0x2B221A -- #1A222B Active Tab Surface
  local C_TAB_HOV    = 0x241C15 -- #151C24 Hovered Tab Surface
  local C_BORDER     = 0x423429 -- #293442 Crisp Slate 1px Border
  local C_BORDER_HOV = 0x6B5542 -- #42556B Hovered Card Border
  local C_CARD       = 0x261E17 -- #171E26 Action Card Surface
  local C_CARD_HOV   = 0x33281F -- #1F2833 Action Card Hover
  local C_CARD_FLASH = 0x47382B -- #2B3847 Click Feedback Flash
  local C_BANNER     = 0x221A14 -- #141A22 Telemetry Banner
  local C_FOOTER     = 0x1C1611 -- #11161C Status Footer
  local C_TEXT       = 0xFAF6F2 -- #F2F6FA Crisp White
  local C_MUTED      = 0xBAAB9C -- #9CABBA Soft Slate Subtitle
  local C_GOLD       = 0x24B4FB -- #FBB424 Amber Gold
  local C_BLUE       = 0xF8BD38 -- #38BDF8 Sky Cyan
  local C_GREEN      = 0x68D934 -- #34D968 Emerald
  local C_PURPLE     = 0xF768A8 -- #A868F7 Violet
  local C_CRIMSON    = 0x5E43F4 -- #F4435E Rose Crimson
  local C_HOOK_BG    = 0x27471B -- #1B4727 Emerald Pill Surface
  local C_HOOK_HOV   = 0x335E24 -- #245E33 Emerald Pill Hover

  local W = S(880)
  local H = S(580)

  local f = createForm(false)
  f.Caption = "PES 2021 — v10.0 Pro Studio (BMPES 16.0 Ready)"
  f.Width = W
  f.Height = H
  f.Color = C_BG
  f.DoubleBuffered = true
  f.DoNotSaveInTable = true
  pcall(function() f.BorderStyle = "bsSingle" end)
  f.centerScreen()
  f.OnClose = function() f.hide(); return caHide end

  local function setHandCursor(ctrl)
    pcall(function() ctrl.Cursor = -21 end)
  end

  local function mkPanel(parent, x, y, w, h, bg)
    local p = createPanel(parent)
    p.Left = x; p.Top = y; p.Width = w; p.Height = h
    p.BevelOuter = "bvNone"
    p.BevelInner = "bvNone"
    pcall(function()
      p.ParentColor = false
      p.ParentBackground = false
      p.FullRepaint = true
    end)
    p.Color = bg
    p.Caption = ""
    p.DoubleBuffered = true
    return p
  end

  local function mkText(parent, x, y, txt, ptSize, color, bold, fixedW, fixedH, center)
    local l = createLabel(parent)
    l.Left = x; l.Top = y
    pcall(function() l.ParentFont = false end)
    l.Font.Name = "Segoe UI"
    l.Font.Size = ptSize
    l.Font.Color = color or C_TEXT
    if bold then pcall(function() l.Font.Style = "[fsBold]" end) end
    if fixedW and fixedH then
      pcall(function()
        l.AutoSize = false
        l.Width = fixedW
        l.Height = fixedH
        if center then
          l.Alignment = "taCenter"
          l.Layout = "tlCenter"
        end
      end)
    end
    l.Caption = txt
    return l
  end

  local function bindInteractive(controls, onClickFn, onHoverFn)
    for _, c in ipairs(controls) do
      setHandCursor(c)
      if onClickFn then c.OnClick = onClickFn end
      if onHoverFn then
        pcall(function()
          c.OnMouseEnter = function() onHoverFn(true) end
          c.OnMouseLeave = function() onHoverFn(false) end
        end)
      end
    end
  end

  -- 1. HEADER BAR
  local hdrH, hdrLineH = S(68), S(2)
  local hdr = mkPanel(f, 0, 0, W, hdrH, C_HEADER)
  mkPanel(hdr, 0, hdrH - hdrLineH, W, hdrLineH, C_BLUE)
  mkText(hdr, S(20), S(10), "PES 2021  •  v10.0 PRO STUDIO", 12, C_BLUE, true)
  local lblHeaderSub = mkText(hdr, S(20), S(38), "Inicializando telemetria...", 9, C_MUTED, false)

  -- Botão rápido "Ativar 6 Hooks" + 4 Pílulas de Moeda no topo direito (centrados e espaçosos)
  local hookBorder = mkPanel(hdr, W - S(466), S(16), S(164), S(36), C_GREEN)
  local pillHooks  = mkPanel(hookBorder, 1, 1, S(164) - 2, S(36) - 2, C_HOOK_BG)
  mkPanel(pillHooks, 0, 0, S(4), S(36) - 2, C_GREEN)
  local lblPillHooks = mkText(pillHooks, S(4), 0, "⚡ Ativar 6 Hooks", 9, C_TEXT, true, S(160) - 2, S(36) - 2, true)
  bindInteractive(
    { hookBorder, pillHooks, lblPillHooks },
    function() PES10.activateAllHooks() end,
    function(isIn) pillHooks.Color = isIn and C_HOOK_HOV or C_HOOK_BG end
  )

  local currencyPills = {}
  local currDefs = { {0, "€ EUR"}, {1, "R$ BRL"}, {2, "$ USD"}, {3, "£ GBP"} }
  local pillW, pillH, pillGap = S(64), S(36), S(6)
  local startCurrX = W - S(20) - (4 * pillW + 3 * pillGap)
  for idx, cd in ipairs(currDefs) do
    local px = startCurrX + (idx - 1) * (pillW + pillGap)
    local pBorder = mkPanel(hdr, px, S(16), pillW, pillH, C_BORDER)
    local pill    = mkPanel(pBorder, 1, 1, pillW - 2, pillH - 2, C_CARD)
    local plbl    = mkText(pill, 0, 0, cd[2], 8, C_TEXT, true, pillW - 2, pillH - 2, true)
    local cIdx    = cd[1]
    bindInteractive(
      { pBorder, pill, plbl },
      function() PES10.setCurrency(cIdx) end,
      function(isIn)
        if PES10.currencyIndex ~= cIdx then
          pill.Color = isIn and C_CARD_HOV or C_CARD
        end
      end
    )
    currencyPills[cIdx] = { border = pBorder, panel = pill, label = plbl }
  end

  -- 2. NAVIGATION TAB BAR (4 Abas Espaçosas Centradas)
  local tabBarH = S(44)
  local tabBar = mkPanel(f, 0, hdrH, W, tabBarH, C_TABBAR)
  mkPanel(tabBar, 0, tabBarH - 1, W, 1, C_BORDER)
  local tabPages = {}
  local tabButtons = {}
  local currentTabIdx = 1
  local TAB_DEFS = {
    { title = "1. Master League & Elenco", accent = C_GREEN },
    { title = "2. Jogador & Contratos",    accent = C_BLUE },
    { title = "3. Treino ML & Estrelato",  accent = C_PURPLE },
    { title = "4. Partida ao Vivo",        accent = C_CRIMSON },
  }

  local function selectTab(activeIdx)
    currentTabIdx = activeIdx
    for i = 1, #TAB_DEFS do
      local isAct = (i == activeIdx)
      tabPages[i].Visible = isAct
      tabButtons[i].panel.Color = isAct and C_TAB_ACT or C_TABBAR
      tabButtons[i].bar.Visible = isAct
      tabButtons[i].label.Font.Color = isAct and C_TEXT or C_MUTED
    end
  end

  local tabW = math.floor(W / #TAB_DEFS)
  local tbH, barH = tabBarH - 1, S(3)
  local footerH = S(38)
  local pageTop = hdrH + tabBarH
  local pageH   = H - pageTop - footerH
  for i, td in ipairs(TAB_DEFS) do
    local tb   = mkPanel(tabBar, (i - 1) * tabW, 0, tabW, tbH, C_TABBAR)
    local bar  = mkPanel(tb, 0, tbH - barH, tabW, barH, td.accent)
    local tlbl = mkText(tb, 0, 0, td.title, 9, C_MUTED, true, tabW, tbH - barH, true)
    local idxCopy = i
    bindInteractive(
      { tb, bar, tlbl },
      function() selectTab(idxCopy) end,
      function(isIn)
        if currentTabIdx ~= idxCopy then
          tb.Color = isIn and C_TAB_HOV or C_TABBAR
        end
      end
    )
    tabButtons[i] = { panel = tb, bar = bar, label = tlbl }

    local page = mkPanel(f, 0, pageTop, W, pageH, C_BG)
    page.Visible = (i == 1)
    tabPages[i] = page
  end

  -- 3. FOOTER TOAST BAR
  local footer = mkPanel(f, 0, H - footerH, W, footerH, C_FOOTER)
  mkPanel(footer, 0, 0, W, 1, C_BORDER)
  local lblToast = mkText(footer, S(20), S(9), "✓ " .. PES10.lastToastMsg, 9, C_GREEN, false, W - S(40), S(22), false)

  -- Helper para criar Banner de Telemetria c/ borda 1px + Cards 2 Colunas Largos (Zero Overflow!)
  local function mkBanner(page, accentColor, initialText)
    local bBorder = mkPanel(page, S(20), S(12), W - S(40), S(46), C_BORDER)
    local b       = mkPanel(bBorder, 1, 1, W - S(40) - 2, S(46) - 2, C_BANNER)
    mkPanel(b, 0, 0, S(4), S(46) - 2, accentColor)
    local l = mkText(b, S(16), S(12), initialText, 9, C_TEXT, true, W - S(76), S(24), false)
    return l
  end

  local CARD_W = math.floor((W - S(56)) / 2) -- 412px * scale por card!
  local CARD_H = S(66)

  local function mkActionCard(page, col, row, accentColor, title, subtitle, onClickFn)
    local cx = S(20) + (col - 1) * (CARD_W + S(16))
    local cy = S(70) + (row - 1) * (CARD_H + S(12))
    local border = mkPanel(page, cx, cy, CARD_W, CARD_H, C_BORDER)
    local card   = mkPanel(border, 1, 1, CARD_W - 2, CARD_H - 2, C_CARD)
    local strip  = mkPanel(card, 0, 0, S(5), CARD_H - 2, accentColor)
    local textW  = CARD_W - S(28)
    local lTitle = mkText(card, S(18), S(9),  title,    10, C_TEXT,  true,  textW, S(24), false)
    local lSub   = mkText(card, S(18), S(36), subtitle, 8,  C_MUTED, false, textW, S(22), false)

    local flashing = false
    local function setHover(isIn)
      if flashing then return end
      border.Color = isIn and accentColor or C_BORDER
      card.Color   = isIn and C_CARD_HOV or C_CARD
    end

    local function trigger()
      flashing = true
      border.Color = accentColor
      card.Color   = C_CARD_FLASH
      local t = createTimer(f, true)
      t.Interval = 140
      t.OnTimer = function(tmr)
        tmr.destroy()
        flashing = false
        if card and border then
          border.Color = C_BORDER
          card.Color   = C_CARD
        end
      end
      pcall(onClickFn)
      lblToast.Caption = PES10.lastToastMsg
    end

    bindInteractive({ border, card, strip, lTitle, lSub }, trigger, setHover)
    return card
  end

  -- ==========================================================================
  -- ABA 1: MASTER LEAGUE & SQUAD COMMANDER (8 Cards em 2x4)
  -- ==========================================================================
  local lblBannerML = mkBanner(tabPages[1], C_GREEN, "Master League: Aguardando save carregado...")

  mkActionCard(tabPages[1], 1, 1, C_GREEN, "+ 50 Milhões (Transferências)", "Soma +50M imediatos ao caixa de contratações", function()
    local c = PES10.readU64(PES10.symAddr("ptrCareer"))
    if c == 0 then PES10.toast("Carregue sua Master League primeiro!"); return end
    writeInteger(c + 0x016ECBF4, math.min(2147483647, PES10.readI32(c + 0x016ECBF4) + 500000))
    PES10.toast("+50 Milhões adicionados em Transferências!")
  end)
  mkActionCard(tabPages[1], 2, 1, C_GREEN, "Pacote Bilionário (+500M / +50M)", "Soma +500M em Transferências e +50M em Salários", function()
    local c = PES10.readU64(PES10.symAddr("ptrCareer"))
    if c == 0 then PES10.toast("Carregue sua Master League primeiro!"); return end
    writeInteger(c + 0x016ECBF4, math.min(2147483647, PES10.readI32(c + 0x016ECBF4) + 5000000))
    writeInteger(c + 0x016ECC08, math.min(2147483647, PES10.readI32(c + 0x016ECC08) + 500000))
    PES10.toast("+500M Transferências e +50M Salários adicionados!")
  end)

  mkActionCard(tabPages[1], 1, 2, C_BLUE, "100% Fôlego em Todo o Elenco", "Descansa 100% da barra de Stamina do seu clube", function()
    PES10.runSquadCommander("stamina_100")
  end)
  mkActionCard(tabPages[1], 2, 2, C_BLUE, "Seta Azul em Todo o Elenco", "Ativa condição máxima (Top Form) p/ próximo jogo", function()
    PES10.runSquadCommander("blue_form")
  end)

  mkActionCard(tabPages[1], 1, 3, C_GREEN, "Curar Lesões + Afeição 100%", "Zera departamento médico e sobe afeição ao máximo", function()
    PES10.runSquadCommander("heal_affection")
  end)
  mkActionCard(tabPages[1], 2, 3, C_GOLD, "Blindar Contratos (+5 Anos)", "Estende contratos em +5 anos e remove multas", function()
    PES10.runSquadCommander("shield_contracts")
  end)

  mkActionCard(tabPages[1], 1, 4, C_PURPLE, "Ativar Auge (Breakthrough) Geral", "Explosão de evolução e zero declínio pós-30", function()
    PES10.runSquadCommander("eternal_peak_breakthrough")
  end)
  mkActionCard(tabPages[1], 2, 4, C_CRIMSON, "Evoluir +1 Overall no Elenco", "Soma +1 permanente nos 25 atributos do seu time", function()
    PES10.runSquadCommander("plus_one_all_stats")
  end)

  -- ==========================================================================
  -- ABA 2: JOGADOR SELECIONADO & CONTRATOS ATIVOS (8 Cards em 2x4)
  -- ==========================================================================
  local lblBannerPlayer = mkBanner(tabPages[2], C_BLUE, "Atleta Selecionado: Nenhum — Use a Busca Global ou passe o cursor no jogo.")

  mkActionCard(tabPages[2], 1, 1, C_BLUE, "Buscar Jogador por Nome ou ID", "Pesquisa instantânea entre 30.000 atletas na RAM", function()
    PES10.searchCareerPlayerDialog()
  end)
  mkActionCard(tabPages[2], 2, 1, C_CRIMSON, "Contratação Fácil (€ 1.000 Multa)", "Fixa multa € 1.000 + Afeição 100% + destrava data", function()
    PES10.runPlayerMacro("easy_sign_target")
  end)

  mkActionCard(tabPages[2], 1, 2, C_GOLD, "Definir Taxa de Rescisão Exata", "Escolher valor exato de multa (ex: 50M, 500k, 1B)", function()
    local p = PES10.getSelectedPlayerOrWarn()
    if p == 0 then return end
    local c = PES10.getCurrentCurrency()
    local ans = inputQuery("Fixar Taxa de Rescisão (" .. c.symbol .. ")", "Digite o valor exato da multa (ex: 1000, 80M, 1B):", "80M")
    if ans then
      local raw = PES10.parseHumanMoneyToRaw(ans)
      if raw then
        PES10.setOverrideReleaseFeeForPlayer(p, raw)
        PES10.toast("Multa fixada em " .. PES10.formatMoneyFromRaw(raw) .. "!")
      end
    end
  end)
  mkActionCard(tabPages[2], 2, 2, C_GOLD, "Destravar Renegociação Imediata", "Remove bloqueio de contrato assinado recentemente", function()
    PES10.runPlayerMacro("unlock_renegotiation")
  end)

  mkActionCard(tabPages[2], 1, 3, C_GREEN, "Todos os 25 Atributos em 99", "Coloca todas as notas técnicas e físicas em 99", function()
    PES10.runPlayerMacro("stats_99")
  end)
  mkActionCard(tabPages[2], 2, 3, C_PURPLE, "41 Habilidades + 13 Posições 'A'", "Libera todas as Skills, Posições A, Pé 4/4 e Forma", function()
    PES10.runPlayerMacro("all_positions_a")
    PES10.runPlayerMacro("all_39_skills")
    PES10.runPlayerMacro("perfect_wf_form_injury")
    PES10.toast("Pacote Completo (Posições A + 41 Skills + Pé 4/4) aplicado!")
  end)

  mkActionCard(tabPages[2], 1, 4, C_BLUE, "Exportar Backup JSON (.json)", "Salva os 380 bytes do atleta em arquivo externo", function()
    PES10.exportPlayerBackupJson()
  end)
  mkActionCard(tabPages[2], 2, 4, C_BLUE, "Importar Backup JSON (.json)", "Restaura stats/skills mantendo ID e Clube atuais", function()
    PES10.importPlayerBackupJson()
  end)

  -- ==========================================================================
  -- ABA 3: TREINO MASTER LEAGUE & RUMO AO ESTRELATO (8 Cards em 2x4)
  -- ==========================================================================
  local lblBannerTrain = mkBanner(tabPages[3], C_PURPLE, "Motor de Sub-XP: 1x Original | Exclusivo para Seu Clube, Base e BAL")

  mkActionCard(tabPages[3], 1, 1, C_GREEN, "Multiplicador de Treino: 10x", "Evolução 10x mais rápida p/ seu clube, base e BAL", function()
    local a = PES10.symAddr("cfgTrainingMultMode")
    if a ~= 0 then writeInteger(a, 3); PES10.ensureHookActive(3); PES10.toast("Multiplicador de Treino 10x ATIVADO!") end
  end)
  mkActionCard(tabPages[3], 2, 1, C_GOLD, "Treino Instantâneo (10.000 XP)", "Sobe atributo, Skill e Posição em apenas 1 dia!", function()
    local a = PES10.symAddr("cfgTrainingMultMode")
    if a ~= 0 then writeInteger(a, 5); PES10.ensureHookActive(3); PES10.toast("Modo Treino Instantâneo (10.000 XP/dia) ATIVADO!") end
  end)

  mkActionCard(tabPages[3], 1, 2, C_BLUE, "Multiplicador Normal (1x)", "Restaura velocidade original de evolução no treino", function()
    local a = PES10.symAddr("cfgTrainingMultMode")
    if a ~= 0 then writeInteger(a, 0); PES10.toast("Treino restaurado para 1x (Original).") end
  end)
  mkActionCard(tabPages[3], 2, 2, C_PURPLE, "Encher Sub-XP do Selecionado", "Coloca os 25 atributos do atleta em 99,99% de XP", function()
    PES10.runPlayerMacro("instant_subxp_player")
  end)

  mkActionCard(tabPages[3], 1, 3, C_BLUE, "BAL: Selecionar Meu Jogador", "Aponta o Edit Player p/ seu craque do Estrelato", function()
    PES10.pointToBALMyPlayer()
  end)
  mkActionCard(tabPages[3], 2, 3, C_GOLD, "BAL: 10 Barras + 99 Pontos", "Destrava trava de 21 pontos e enche as 10 barras", function()
    PES10.runPlayerMacro("bal_max_sliders")
  end)

  mkActionCard(tabPages[3], 1, 4, C_GREEN, "BAL: Aprender Skill Agora", "Deixa em 99,99% a Habilidade em treino no BAL", function()
    PES10.runPlayerMacro("bal_instant_skill")
  end)
  mkActionCard(tabPages[3], 2, 4, C_GREEN, "BAL: Aprender Posição Agora", "Deixa em 99,99% a nova Posição em treino no BAL", function()
    PES10.runPlayerMacro("bal_instant_pos")
  end)

  -- ==========================================================================
  -- ABA 4: PARTIDA AO VIVO — RELÓGIO, PLACAR & STAMINA (8 Cards em 2x4)
  -- ==========================================================================
  local lblBannerMatch = mkBanner(tabPages[4], C_CRIMSON, "Partida ao Vivo: Aguardando início da partida...")

  mkActionCard(tabPages[4], 1, 1, C_GOLD, "Escolher Minuto Exato...", "Pula p/ qualquer minuto (15, 44, 88) sem travar", function()
    local info = PES10.getMatchClockInfo()
    if not info then PES10.toast("Entre em uma partida ao vivo primeiro!"); return end
    local ans = inputQuery("Escolher Minuto da Partida", "Digite o minuto desejado (ex: 15, 44, 88 ou 44:30):", tostring(info.mm))
    if ans then
      local mm, ss = ans:match("^(%d+)%s*:%s*(%d+)$")
      local tMin = mm and (tonumber(mm) + tonumber(ss) / 60.0) or tonumber((ans:gsub(",", ".")))
      if tMin then PES10.setExactMatchMinute(tMin) end
    end
  end)
  mkActionCard(tabPages[4], 2, 1, C_BLUE, "Pular p/ 44:00 ou 88:00", "No 1ºT avança p/ 44:00 | No 2ºT avança p/ 88:00", function()
    local info = PES10.getMatchClockInfo()
    if not info then PES10.toast("Entre em uma partida ao vivo primeiro!"); return end
    PES10.setExactMatchMinute(info.period == 1 and 44 or 88)
  end)

  mkActionCard(tabPages[4], 1, 2, C_BLUE, "Reiniciar Tempo Atual (00:00)", "Volta o cronômetro para o início do tempo atual", function()
    PES10.resetCurrentHalfNow()
  end)
  mkActionCard(tabPages[4], 2, 2, C_CRIMSON, "Encerrar Tempo Atual Agora", "Avança o cronômetro para o apito imediatamente", function()
    PES10.endCurrentHalfNow()
  end)

  mkActionCard(tabPages[4], 1, 3, C_GREEN, "Alterar Placar ao Vivo...", "Define gols de Casa e Fora sincronizando placar", function()
    PES10.ensureHookActive(5)
    local mCore = PES10.readU64(PES10.symAddr("ptrMatchCore"))
    local mHud = PES10.readU64(PES10.symAddr("ptrMatchHud"))
    if mCore == 0 and mHud == 0 then PES10.toast("Entre em uma partida c/ Hook 5 ativo primeiro!"); return end
    local hCur = (mCore ~= 0) and PES10.readU8(mCore + 0x9E) or 0
    local aCur = (mCore ~= 0) and PES10.readU8(mCore + 0x9F) or 0
    local ans = inputQuery("Placar ao Vivo (Casa x Fora)", "Digite no formato CASA-FORA (ex: 3-0 ou 2-1):", string.format("%d-%d", hCur, aCur))
    if ans then
      local hg, ag = ans:match("^(%d+)%s*[%-%s:xX]%s*(%d+)$")
      if hg and ag then
        local hi, ai = tonumber(hg), tonumber(ag)
        if mCore ~= 0 then writeBytes(mCore + 0x9E, hi); writeBytes(mCore + 0x9F, ai) end
        if mHud ~= 0 then writeInteger(mHud + 0x120, hi); writeInteger(mHud + 0x124, ai) end
        PES10.toast(string.format("Placar atualizado para %d x %d!", hi, ai))
      end
    end
  end)
  mkActionCard(tabPages[4], 2, 3, C_PURPLE, "Congelar / Descongelar Relógio", "Alterna pausa exata do tempo durante a partida", function()
    local a = PES10.symAddr("cfgFreezeClock")
    local info = PES10.getMatchClockInfo()
    if a == 0 or not info then PES10.toast("Entre em uma partida primeiro!"); return end
    PES10.ensureHookActive(5)
    local cur = PES10.readU32(a)
    if cur == 0 then
      writeFloat(PES10.symAddr("valFrozenTotalTicks"), info.halfTicks)
      writeInteger(a, 1)
      PES10.toast("Relógio CONGELADO em " .. info.formatted)
    else
      writeInteger(a, 0)
      PES10.toast("Relógio DESCONGELADO!")
    end
  end)

  mkActionCard(tabPages[4], 1, 4, C_GREEN, "Stamina 100%: Só Mandante", "Fôlego infinito só para Casa (sem ajudar Fora)", function()
    local a = PES10.symAddr("cfgStaminaMode")
    if a ~= 0 then writeInteger(a, 1); PES10.ensureHookActive(6); PES10.toast("Stamina 100% ativada APENAS para o Mandante (Casa)!") end
  end)
  mkActionCard(tabPages[4], 2, 4, C_GREEN, "Stamina 100%: Só Visitante", "Fôlego infinito só para Fora (sem ajudar Casa)", function()
    local a = PES10.symAddr("cfgStaminaMode")
    if a ~= 0 then writeInteger(a, 2); PES10.ensureHookActive(6); PES10.toast("Stamina 100% ativada APENAS para o Visitante (Fora)!") end
  end)

  -- Timer de atualização suave da UI (400ms)
  local TRN_NAMES = { [0]="1x Original", [1]="2x", [2]="5x", [3]="10x Super", [4]="50x Turbo", [5]="Instantâneo (10.000 XP)" }
  local STAM_NAMES = { [0]="Original", [1]="Só Casa 100%", [2]="Só Fora 100%", [3]="Ambos 100%", [4]="Pressão Casa", [5]="Pressão Fora" }

  local uiTimer = createTimer(f, true)
  uiTimer.Interval = 400
  uiTimer.OnTimer = function()
    if not f.Visible then return end
    local c = PES10.getCurrentCurrency()
    for idx, cp in pairs(currencyPills) do
      local active = (idx == PES10.currencyIndex)
      cp.border.Color = active and C_GOLD or C_BORDER
      cp.panel.Color  = active and C_GOLD or C_CARD
      cp.label.Font.Color = active and C_BG or C_TEXT
    end
    lblToast.Caption = PES10.lastToastMsg

    if not PES10.isAttached() then
      lblHeaderSub.Caption = "○ Aguardando PES2021.exe...  |  Moeda: " .. c.symbol .. " (" .. c.code .. ")"
      lblHeaderSub.Font.Color = C_MUTED
      return
    end

    local modeStr, clubStr, career = PES10.getCareerModeAndClubInfo()
    local p = PES10.readU64(PES10.symAddr("ptrPlayer"))
    local pName = (p ~= 0) and (readString(p + 0x38, 30, false) or "Atleta") or "Nenhum"
    lblHeaderSub.Caption = string.format("● Conectado  |  %s%s  |  Atleta: %s", modeStr, clubStr ~= "" and (" — " .. clubStr) or "", pName)
    lblHeaderSub.Font.Color = C_GREEN

    if career ~= 0 then
      local tb = PES10.formatMoneyFromRaw(PES10.readI32(career + 0x016ECBF4))
      local sb = PES10.formatMoneyFromRaw(PES10.readI32(career + 0x016ECC08))
      local yr = PES10.readU16(career + 0x01642A24)
      local mo = PES10.readU8(career + 0x01642A26)
      local dy = PES10.readU8(career + 0x01642A27)
      lblBannerML.Caption = string.format("Transferências: %s   |   Salários: %s   |   Data: %02d/%02d/%04d", tb, sb, dy, mo, yr)
    else
      lblBannerML.Caption = "Nenhuma Carreira carregada — Entre na Master League ou BAL."
    end

    if p ~= 0 then
      local _, _, pClub = PES10.decodeClubHandle(PES10.readU32(p + 0x12C))
      local pSal = PES10.formatMoneyFromRaw(PES10.readI32(p + 0x15C))
      local rfeeRaw = PES10.readI32(PES10.symAddr("valComputedReleaseFee"))
      local rfeeStr = (rfeeRaw > 0) and PES10.formatMoneyFromRaw(rfeeRaw) or "Sem Multa"
      lblBannerPlayer.Caption = string.format("%s  |  %s  |  Salário: %s  |  Multa: %s", pName, pClub, pSal, rfeeStr)
    else
      lblBannerPlayer.Caption = "Nenhum atleta selecionado — Clique em 'Buscar Jogador' abaixo ou passe o cursor no jogo."
    end

    local tm = PES10.readU32(PES10.symAddr("cfgTrainingMultMode"))
    local fp = (career ~= 0) and PES10.readU8(career + 0x016ECBEA) or 0
    lblBannerTrain.Caption = string.format("Treino Ativo: %s   |   BAL Focus Points: %d", TRN_NAMES[tm] or "1x", fp)

    local clk = PES10.getMatchClockInfo()
    local sm = PES10.readU32(PES10.symAddr("cfgStaminaMode"))
    if clk then
      local mCore = PES10.readU64(PES10.symAddr("ptrMatchCore"))
      local sc = (mCore ~= 0) and string.format("%d x %d", PES10.readU8(mCore + 0x9E), PES10.readU8(mCore + 0x9F)) or "Ao Vivo"
      lblBannerMatch.Caption = string.format("Cronômetro: %s   |   Placar: %s   |   Stamina: %s", clk.formatted, sc, STAM_NAMES[sm] or "Original")
    else
      lblBannerMatch.Caption = "Fora de Partida   |   Modo Stamina Pré-Configurado: " .. (STAM_NAMES[sm] or "Original")
    end
  end

  selectTab(1)
  PES10.dashboardForm = f
  f.show()
end

-- ============================================================================
-- AUTO-ATTACH & TIMER DE TELEMETRIA (300ms, ZERO BLOQUEIO)
-- ============================================================================
function PES10.startBackgroundEngine()
  PES10.bindTableHooks()
  if PES10.isAttached() then
    PES10.initProcessState()
  end

  if PES10.bgTimer then
    pcall(function() PES10.bgTimer.destroy() end)
  end

  PES10.bgTimer = createTimer(MainForm, true)
  PES10.bgTimer.Interval = 300
  PES10.bgTimer.OnTimer = function()
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
    end
  end
end

-- __PES10_CONFIG_INJECTION__
PES10.startBackgroundEngine()

-- Hook opcional para automação de desenvolvimento via variável de ambiente (desativado por padrão)
pcall(function()
  local devBridge = os.getenv and os.getenv("PES10_DEV_BRIDGE")
  if devBridge and devBridge ~= "" then
    local f = io.open(devBridge, "r")
    if f then f:close(); dofile(devBridge) end
  end
end)
