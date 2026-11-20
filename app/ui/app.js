/* PES 2021 Ultimate Studio - Standalone Frontend Controller */

let META = null;
let LANG = "pt";
let CURRENT_VIEW = "dashboard";
let CURRENT_PTAB = "bio";
let LAST_PLAYER_ADDR = 0;
let CURRENT_PLAYER = null;
let SQUAD_DATA = [];
let SQUAD_FILTER = "all";
let SQUAD_SORT = { col: "ovr", asc: false };
let SCOUT_LISTS_DATA = [];
let ACTIVE_SCOUT_SLOT = 4;
let FREEZE_CLOCK = false;

const api = () => window.pywebview && window.pywebview.api;

function T(pt, en) { return LANG === "en" ? en : pt; }

function toast(msg, type = "ok") {
  const wrap = document.getElementById("toast-container");
  const el = document.createElement("div");
  el.className = `toast ${type}`;
  el.textContent = msg;
  wrap.appendChild(el);
  setTimeout(() => { el.style.opacity = "0"; setTimeout(() => el.remove(), 220); }, 3200);
}

function fmtMoney(raw) {
  const euros = Math.round((raw || 0) * 100);
  const abs = Math.abs(euros);
  const s = abs.toLocaleString(LANG === "en" ? "en-US" : "pt-BR");
  return (euros < 0 ? "-€ " : "€ ") + s;
}

function posShort(id) {
  if (!META) return "---";
  const p = META.positions[id];
  return p ? (LANG === "en" ? p.en : p.pt) : "---";
}

function posClass(id) {
  if (id === 0) return "pos-gk";
  if (id <= 3) return "pos-def";
  if (id <= 8) return "pos-mid";
  return "pos-att";
}

function ovrClass(ovr) {
  if (ovr >= 90) return "ovr-90";
  if (ovr >= 80) return "ovr-80";
  if (ovr >= 70) return "ovr-70";
  return "ovr-low";
}

function stamColor(v) {
  if (v >= 85) return "#10b981";
  if (v >= 65) return "#f59e0b";
  return "#ef4444";
}

const ARROW_ICONS = [
  { ico: "⬇", col: "#f87171", pt: "Vermelha", en: "Red" },
  { ico: "↘", col: "#fb923c", pt: "Laranja", en: "Orange" },
  { ico: "➡", col: "#facc15", pt: "Amarela", en: "Yellow" },
  { ico: "↗", col: "#4ade80", pt: "Verde", en: "Green" },
  { ico: "⬆", col: "#38bdf8", pt: "Azul", en: "Blue" },
];

// -------------------------------------------------------------------
// NAVIGATION
// -------------------------------------------------------------------
function switchView(viewName) {
  CURRENT_VIEW = viewName;
  document.querySelectorAll(".nav-btn").forEach(b => b.classList.toggle("active", b.dataset.view === viewName));
  document.querySelectorAll(".view-panel").forEach(p => p.classList.toggle("active", p.id === `view-${viewName}`));
  if (viewName === "squad") loadSquad();
  if (viewName === "player") loadPlayerDetails();
  if (viewName === "scouts") loadScouts();
  if (viewName === "finances") loadFinances();
}

function switchPlayerTab(tabName) {
  CURRENT_PTAB = tabName;
  document.querySelectorAll("#player-subtabs .tab-btn").forEach(b => b.classList.toggle("active", b.dataset.ptab === tabName));
  document.querySelectorAll(".ptab-pane").forEach(p => {
    p.style.display = (p.id === `ptab-${tabName}`) ? "block" : "none";
  });
}

// -------------------------------------------------------------------
// LIVE POLLING (600ms)
// -------------------------------------------------------------------
async function pollStatus() {
  if (!api()) return;
  try {
    const st = await api().get_status();
    renderStatus(st);
  } catch (e) {}
}

function renderStatus(st) {
  const connPill = document.getElementById("conn-pill");
  const connText = document.getElementById("conn-text");
  if (st.attached) {
    connPill.classList.add("online");
    connText.textContent = `${st.exe} (${st.pid})`;
  } else {
    connPill.classList.remove("online");
    connText.textContent = T("Aguardando PES 2021...", "Waiting for PES 2021...");
  }

  document.getElementById("side-mode-text").textContent = st.mode || "";
  document.getElementById("side-club-text").textContent = st.club || "";
  document.getElementById("dash-club").textContent = st.club || (st.attached ? st.exe : "---");
  document.getElementById("dash-mode").textContent = st.mode || "";

  if (st.finances) {
    document.getElementById("dash-transfer").textContent = fmtMoney(st.finances.transfer_raw);
    document.getElementById("dash-salary").textContent = fmtMoney(st.finances.salary_avail_raw);
  }

  // Hooks status on dashboard
  const hooksEl = document.getElementById("dash-hooks-list");
  if (hooksEl) {
    const hk = st.hooks || {};
    const items = [
      [1, T("Captura Automática de Jogador", "Auto Player Capture")],
      [3, T("Multiplicador de Treino (XP)", "Training XP Multiplier")],
      [4, T("Desbloqueio de Foco BAL (21->99)", "BAL Focus Cap Unlock")],
      [5, T("HUD & Relógio da Partida", "Match HUD & Clock Freeze")],
      [6, T("Fôlego Seletivo em Partida", "Selective Match Stamina")],
    ];
    hooksEl.innerHTML = items.map(([num, label]) => {
      const on = !!hk[num];
      return `<div style="display:flex; justify-content:space-between; align-items:center;">
        <span style="color:var(--subtext);">${label}</span>
        <span class="${on ? "pill-ok" : ""}" style="${on ? "" : "color:var(--muted); font-size:11px;"}">${on ? "ATIVO" : "Standby"}</span>
      </div>`;
    }).join("");
  }

  // Sync Training / Stamina / Freeze / BAL cards
  document.querySelectorAll("#training-mode-grid .mode-card").forEach(c => {
    c.classList.toggle("active", Number(c.dataset.tmode) === Number(st.training_mode || 0));
  });
  document.querySelectorAll("#stamina-mode-grid .mode-card").forEach(c => {
    c.classList.toggle("active", Number(c.dataset.smode) === Number(st.stamina_mode || 0));
  });
  FREEZE_CLOCK = !!st.freeze_clock;
  const fBtn = document.getElementById("btn-freeze-clock");
  if (fBtn) {
    fBtn.textContent = FREEZE_CLOCK ? T("Congelar: ON", "Freeze: ON") : T("Congelar: OFF", "Freeze: OFF");
    fBtn.className = FREEZE_CLOCK ? "btn btn-primary" : "btn";
  }

  // Top Active Player Pill + Auto-Sync Player Studio when in-game player changes!
  const p = st.player;
  if (p) {
    const ovrEl = document.getElementById("top-p-ovr");
    ovrEl.textContent = p.ovr;
    ovrEl.className = `ovr-badge ${ovrClass(p.ovr)}`;
    const posEl = document.getElementById("top-p-pos");
    posEl.textContent = posShort(p.pos);
    posEl.className = `pos-chip ${posClass(p.pos)}`;
    document.getElementById("top-p-name").textContent = p.name;
    const arr = ARROW_ICONS[p.arrow] || ARROW_ICONS[2];
    document.getElementById("top-p-meta").innerHTML =
      `${p.club} • ${p.age}a • ${p.stamina}% <span style="color:${arr.col}; font-weight:700;">${arr.ico}</span>`;
    document.getElementById("undo-badge").textContent = `(${p.undo_count || 0})`;

    if (p.addr !== LAST_PLAYER_ADDR) {
      LAST_PLAYER_ADDR = p.addr;
      if (CURRENT_VIEW === "player") {
        loadPlayerDetails(p.addr);
      }
    }
  } else {
    document.getElementById("top-p-ovr").textContent = "--";
    document.getElementById("top-p-pos").textContent = "---";
    document.getElementById("top-p-name").textContent = T("Nenhum atleta selecionado", "No player selected");
    document.getElementById("top-p-meta").textContent = T("Destaque um jogador no jogo ou na lista", "Highlight a player in-game or in a list");
    document.getElementById("undo-badge").textContent = "(0)";
  }
}

// -------------------------------------------------------------------
// PLAYER STUDIO
// -------------------------------------------------------------------
async function loadPlayerDetails(addr = null) {
  if (!api()) return;
  const d = await api().get_player_details(addr);
  if (!d || !d.ok) return;
  CURRENT_PLAYER = d;
  LAST_PLAYER_ADDR = d.addr;
  renderPlayerDetails(d);
}

function renderPlayerDetails(d) {
  const heroOvr = document.getElementById("p-hero-ovr");
  heroOvr.textContent = d.ovr;
  heroOvr.className = `ovr-badge ${ovrClass(d.ovr)}`;
  document.getElementById("p-hero-name").textContent = `${d.name} (${posShort(d.reg_pos)})`;
  document.getElementById("p-hero-sub").textContent = `${d.club} • ID ${d.pid} • ${d.age}a • ${fmtMoney(d.salary_raw)}/ano`;
  document.getElementById("undo-badge").textContent = `(${d.undo_count || 0})`;

  // Quick Condition
  document.getElementById("inp-p-stam").value = d.stamina;
  document.getElementById("lbl-p-stam").textContent = `${d.stamina}%`;
  document.querySelectorAll("#p-arrow-group .arrow-btn").forEach(b => {
    b.classList.toggle("active", Number(b.dataset.arrow) === Number(d.arrow));
  });

  // Bio fields
  document.getElementById("bio-name").value = d.name;
  document.getElementById("bio-shirt").value = d.shirt;
  document.getElementById("bio-nat").value = String(d.nationality);
  document.getElementById("bio-age").value = d.age;
  document.getElementById("bio-height").value = d.height;
  document.getElementById("bio-weight").value = d.weight;
  document.getElementById("bio-foot").value = String(d.foot);
  document.getElementById("bio-contract").value = d.contract_yr;
  document.getElementById("bio-salary").value = fmtMoney(d.salary_raw);
  document.getElementById("bio-injury").value = d.injury;
  document.getElementById("bio-wfu").value = d.wf_usage;
  document.getElementById("bio-wfa").value = d.wf_acc;
  document.getElementById("bio-form").value = d.form_cons;
  document.getElementById("bio-injres").value = d.inj_res;

  // Stats Groups
  renderStatsGroups(d);
  // Position Lab
  renderPositionLab(d);
  // Skills & Style
  renderSkillsAndStyle(d);
  // Team Role
  document.getElementById("p-team-role").value = String(d.team_role);
  document.getElementById("p-team-role-lvl").value = String(d.team_role_lvl);
}

function renderStatsGroups(d) {
  if (!META) return;
  const groups = [
    { id: "attack", title: T("Ataque", "Attack") },
    { id: "tech", title: T("Técnica & Passe", "Technique & Passing") },
    { id: "phys", title: T("Físico", "Physical") },
    { id: "def", title: T("Defesa", "Defense") },
    { id: "gk", title: T("Goleiro", "Goalkeeper") },
  ];
  const container = document.getElementById("stats-groups-container");
  container.innerHTML = groups.map(g => {
    const defs = META.stats.filter(s => s.group === g.id);
    const rows = defs.map(s => {
      const val = d.stats[String(s.fid)] || 40;
      const lbl = LANG === "en" ? s.en : s.pt;
      return `<div class="stat-row">
        <span style="font-size:12px; color:var(--subtext);">${lbl}</span>
        <input type="range" min="40" max="99" value="${val}"
          oninput="this.nextElementSibling.textContent=this.value"
          onchange="onStatChange(${s.fid}, this.value)" />
        <span class="ovr-badge ${ovrClass(val)}" style="min-width:36px; height:22px; font-size:11.5px;">${val}</span>
        <div style="display:flex; gap:3px;">
          <button class="btn btn-xs" onclick="onStatChange(${s.fid}, ${Math.max(40, val - 1)})">-</button>
          <button class="btn btn-xs" onclick="onStatChange(${s.fid}, ${Math.min(99, val + 1)})">+</button>
        </div>
      </div>`;
    }).join("");
    return `<div class="card"><div class="card-title">${g.title}</div><div>${rows}</div></div>`;
  }).join("");
}

async function onStatChange(fid, val) {
  const res = await api().set_player_stat(Number(fid), Number(val));
  if (res && res.ok) { CURRENT_PLAYER = res; renderPlayerDetails(res); }
}

async function presetStats(val) {
  if (!META || !CURRENT_PLAYER) return;
  const map = {};
  const isGK = CURRENT_PLAYER.reg_pos === 0;
  META.stats.forEach(s => {
    if (isGK || s.group !== "gk") map[String(s.fid)] = val;
  });
  const res = await api().set_player_stats_batch(map);
  if (res && res.ok) { CURRENT_PLAYER = res; renderPlayerDetails(res); toast(T(`Atributos definidos para ${val}`, `Stats set to ${val}`)); }
}

async function shiftAllStats(delta) {
  if (!META || !CURRENT_PLAYER) return;
  const map = {};
  const isGK = CURRENT_PLAYER.reg_pos === 0;
  META.stats.forEach(s => {
    if (isGK || s.group !== "gk") {
      const cur = CURRENT_PLAYER.stats[String(s.fid)] || 40;
      map[String(s.fid)] = Math.max(40, Math.min(99, cur + delta));
    }
  });
  const res = await api().set_player_stats_batch(map);
  if (res && res.ok) { CURRENT_PLAYER = res; renderPlayerDetails(res); toast(`+${delta} em todos os atributos`); }
}

function renderPositionLab(d) {
  const tbody = document.getElementById("poslab-tbody");
  const sorted = [...d.positions].sort((a, b) => (b.is_reg - a.is_reg) || (b.ovr - a.ovr));
  tbody.innerHTML = sorted.map(pos => {
    const pMeta = META.positions[pos.id];
    const fullName = LANG === "en" ? pMeta.name_en : pMeta.name_pt;
    const regTag = pos.is_reg ? `<span class="pill-ok" style="margin-left:6px;">PRINCIPAL</span>` : "";
    return `<tr>
      <td>
        <span class="pos-chip ${posClass(pos.id)}">${posShort(pos.id)}</span>
        <span style="margin-left:8px; font-weight:600;">${fullName}</span>${regTag}
      </td>
      <td><span class="ovr-badge ${ovrClass(pos.ovr)}">${pos.ovr}</span></td>
      <td>
        <div class="seg">
          <button class="${pos.apt === 0 ? "active" : ""}" onclick="setPosApt(${pos.id}, 0)">C</button>
          <button class="${pos.apt === 1 ? "active g-b" : ""}" onclick="setPosApt(${pos.id}, 1)">B</button>
          <button class="${pos.apt === 2 ? "active g-a" : ""}" onclick="setPosApt(${pos.id}, 2)">A</button>
        </div>
      </td>
      <td>
        <div style="display:flex; gap:6px;">
          <button class="btn btn-xs" onclick="stepPosOVR(${pos.id}, -1)">-1 OVR</button>
          <button class="btn btn-xs btn-primary" onclick="stepPosOVR(${pos.id}, 1)">+1 OVR</button>
        </div>
      </td>
      <td>
        <div style="display:flex; gap:6px;">
          ${pos.is_reg ? "" : `<button class="btn btn-xs btn-gold" onclick="matchPosPrimary(${pos.id})">Igualar Principal</button>`}
          ${pos.is_reg ? "" : `<button class="btn btn-xs" onclick="setRegPos(${pos.id})">Tornar Principal</button>`}
        </div>
      </td>
    </tr>`;
  }).join("");
}

async function setPosApt(posId, grade) {
  const res = await api().set_position_aptitude(posId, grade);
  if (res && res.ok) { CURRENT_PLAYER = res; renderPlayerDetails(res); }
}

async function setRegPos(posId) {
  const res = await api().set_registered_position(posId);
  if (res && res.ok) { CURRENT_PLAYER = res; renderPlayerDetails(res); toast(T("Posição principal alterada", "Registered position updated")); }
}

async function stepPosOVR(posId, delta) {
  const res = await api().step_position_ovr(posId, delta);
  if (res && res.ok) {
    CURRENT_PLAYER = res;
    renderPlayerDetails(res);
    toast(res.msg || "OK", res.action_ok === false ? "err" : "ok");
  }
}

async function matchPosPrimary(posId) {
  const res = await api().match_position_to_primary(posId);
  if (res && res.ok) {
    CURRENT_PLAYER = res;
    renderPlayerDetails(res);
    toast(res.msg || "OK", res.action_ok === false ? "err" : "ok");
  }
}

function renderSkillsAndStyle(d) {
  document.getElementById("p-playing-style").value = String(d.playing_style);
  let skCount = 0;
  document.getElementById("p-skills-grid").innerHTML = META.skills.map(s => {
    const on = !!d.skills[s.key];
    if (on) skCount++;
    return `<button class="skill-chip ${on ? "on" : ""}" onclick="toggleSkill('${s.key.replace(/'/g, "\\'")}', ${!on})">${LANG === "en" ? s.en : s.pt}</button>`;
  }).join("");
  document.getElementById("skills-count").textContent = `${skCount} / 10`;

  let aiCount = 0;
  document.getElementById("p-comai-grid").innerHTML = META.com_ai.map(s => {
    const on = !!d.com_ai[s.key];
    if (on) aiCount++;
    return `<button class="skill-chip ${on ? "on" : ""}" onclick="toggleSkill('${s.key.replace(/'/g, "\\'")}', ${!on})">${LANG === "en" ? s.en : s.pt}</button>`;
  }).join("");
  document.getElementById("comai-count").textContent = `${aiCount} ativos`;
}

async function toggleSkill(key, enabled) {
  const res = await api().set_player_skill(key, enabled);
  if (res && res.ok) { CURRENT_PLAYER = res; renderPlayerDetails(res); }
}

// -------------------------------------------------------------------
// SQUAD ROSTER
// -------------------------------------------------------------------
async function loadSquad() {
  if (!api()) return;
  const res = await api().get_squad_roster();
  if (!res || !res.ok) return;
  SQUAD_DATA = res.players || [];
  document.getElementById("squad-title").textContent = `${T("Meu Elenco", "My Squad")} - ${res.club} (${SQUAD_DATA.length})`;
  renderSquadTable();
}

function renderSquadTable() {
  const q = (document.getElementById("squad-search").value || "").toLowerCase().trim();
  let rows = SQUAD_DATA.filter(p => {
    if (SQUAD_FILTER === "exp" && !p.is_expiring) return false;
    if (SQUAD_FILTER === "inj" && p.injury <= 0) return false;
    if (SQUAD_FILTER === "u23" && p.age > 23) return false;
    if (q && !p.name.toLowerCase().includes(q)) return false;
    return true;
  });
  const col = SQUAD_SORT.col;
  const dir = SQUAD_SORT.asc ? 1 : -1;
  rows.sort((a, b) => {
    const va = a[col], vb = b[col];
    if (typeof va === "string") return va.localeCompare(vb) * dir;
    return ((va || 0) - (vb || 0)) * dir;
  });

  const tbody = document.getElementById("squad-tbody");
  if (!rows.length) {
    tbody.innerHTML = `<tr><td colspan="8" class="empty-state">${T("Nenhum jogador encontrado com este filtro.", "No players match this filter.")}</td></tr>`;
    return;
  }
  tbody.innerHTML = rows.map(p => `<tr onclick="openPlayerInStudio(${p.addr})">
    <td><span class="ovr-badge ${ovrClass(p.ovr)}">${p.ovr}</span></td>
    <td><span class="pos-chip ${posClass(p.pos)}">${posShort(p.pos)}</span></td>
    <td style="font-weight:600;">${p.name}</td>
    <td>${p.age}a</td>
    <td style="color:${p.is_expiring ? "var(--gold)" : "var(--subtext)"};">${p.exp_y || "-"}</td>
    <td>${fmtMoney(p.salary_raw)}</td>
    <td>
      <div class="stam-bar-wrap">
        <div class="stam-bar"><div class="stam-fill" style="width:${p.stamina}%; background:${stamColor(p.stamina)};"></div></div>
        <span style="font-size:11.5px; color:var(--subtext);">${p.stamina}%</span>
      </div>
    </td>
    <td>${p.injury > 0 ? `<span class="pill-inj">${p.injury}d</span>` : `<span class="pill-ok">OK</span>`}</td>
  </tr>`).join("");
}

async function openPlayerInStudio(addr) {
  await api().select_player(addr);
  switchView("player");
  loadPlayerDetails(addr);
}

async function runSquadCmd(mode) {
  const res = await api().run_squad_commander(mode);
  if (res && res.ok) {
    toast(T(`Aplicado a ${res.count} atletas do elenco!`, `Applied to ${res.count} squad players!`));
    if (CURRENT_VIEW === "squad") loadSquad();
    if (CURRENT_VIEW === "player") loadPlayerDetails();
  } else {
    toast(res && res.msg ? res.msg : T("Abra uma Master League primeiro.", "Open Master League first."), "err");
  }
}

// -------------------------------------------------------------------
// GLOBAL 30K+ SEARCH
// -------------------------------------------------------------------
async function doSearch() {
  const q = document.getElementById("srch-query").value;
  const pos = Number(document.getElementById("srch-pos").value);
  const ovr = Number(document.getElementById("srch-ovr").value || 0);
  const age = Number(document.getElementById("srch-age").value || 99);
  const free = document.getElementById("srch-free").checked;
  const tbody = document.getElementById("search-tbody");
  tbody.innerHTML = `<tr><td colspan="7" class="empty-state">${T("Pesquisando base...", "Searching database...")}</td></tr>`;
  const res = await api().search_players(q, pos, ovr, age, free);
  if (!res || !res.ok || !res.players.length) {
    tbody.innerHTML = `<tr><td colspan="7" class="empty-state">${T("Nenhum atleta encontrado.", "No players found.")}</td></tr>`;
    return;
  }
  tbody.innerHTML = res.players.map(p => `<tr>
    <td><span class="ovr-badge ${ovrClass(p.ovr)}">${p.ovr}</span></td>
    <td><span class="pos-chip ${posClass(p.pos)}">${posShort(p.pos)}</span></td>
    <td style="font-weight:600;">${p.name}</td>
    <td>${p.age}a</td>
    <td style="color:var(--subtext);">${p.club}</td>
    <td>${fmtMoney(p.salary_raw)}</td>
    <td>
      <div style="display:flex; gap:6px;">
        <button class="btn btn-xs btn-primary" onclick="openPlayerInStudio(${p.addr})">Editar</button>
        <button class="btn btn-xs btn-green" onclick="addPlayerToScout(4, ${p.addr})">+ Favoritos</button>
      </div>
    </td>
  </tr>`).join("");
}

// -------------------------------------------------------------------
// SCOUTS & SHORTLIST
// -------------------------------------------------------------------
async function loadScouts() {
  const res = await api().get_scout_lists();
  if (!res || !res.ok) return;
  SCOUT_LISTS_DATA = res.lists || [];
  renderScoutTable();
}

function renderScoutTable() {
  const sl = SCOUT_LISTS_DATA.find(x => x.slot === ACTIVE_SCOUT_SLOT);
  const tbody = document.getElementById("scout-tbody");
  if (!sl || !sl.entries.length) {
    tbody.innerHTML = `<tr><td colspan="6" class="empty-state">${T("Lista vazia.", "List is empty.")}</td></tr>`;
    return;
  }
  tbody.innerHTML = sl.entries.map(e => `<tr>
    <td><span class="ovr-badge ${ovrClass(e.ovr)}">${e.ovr}</span></td>
    <td><span class="pos-chip ${posClass(e.pos)}">${posShort(e.pos)}</span></td>
    <td style="font-weight:600;">${e.name}</td>
    <td>${e.age}a</td>
    <td style="color:var(--subtext);">${e.club}</td>
    <td>
      <div style="display:flex; gap:6px;">
        <button class="btn btn-xs btn-primary" onclick="openPlayerInStudio(${e.addr})">Editar</button>
        <button class="btn btn-xs btn-red" onclick="removeScoutEntry(${ACTIVE_SCOUT_SLOT}, '${e.handle}')">Remover</button>
      </div>
    </td>
  </tr>`).join("");
}

async function addPlayerToScout(slotNum, addr = null) {
  const res = await api().add_to_scout_list(slotNum, addr);
  toast(res && res.msg ? res.msg : "OK", res && res.ok ? "ok" : "err");
  if (CURRENT_VIEW === "scouts") loadScouts();
}

async function removeScoutEntry(slotNum, handleStr) {
  await api().remove_from_scout_list(slotNum, handleStr);
  toast(T("Removido da lista", "Removed from list"));
  loadScouts();
}

// -------------------------------------------------------------------
// FINANCES
// -------------------------------------------------------------------
async function loadFinances() {
  const res = await api().get_finances();
  if (!res || !res.ok) return;
  document.getElementById("fin-transfer-inp").value = fmtMoney(res.transfer_raw);
  document.getElementById("fin-salary-inp").value = fmtMoney(res.salary_avail_raw);
  document.getElementById("fin-wage-bill").textContent = `${T("Folha anual do elenco", "Squad annual wage bill")}: ${fmtMoney(res.squad_wages_raw)}`;
}

async function quickFinance(kind, deltaRaw) {
  const res = await api().add_finance_delta(kind, deltaRaw);
  if (res && res.ok) {
    toast(T("Orçamento atualizado!", "Budget updated!"));
    pollStatus();
    if (CURRENT_VIEW === "finances") loadFinances();
  }
}

// -------------------------------------------------------------------
// BOOTSTRAP & EVENT WIRING
// -------------------------------------------------------------------
async function bootstrap() {
  META = await api().get_metadata();

  // Populate dropdowns
  document.getElementById("bio-nat").innerHTML = META.nationalities.map(n => `<option value="${n.id}">${n.name}</option>`).join("");
  document.getElementById("p-playing-style").innerHTML = META.playing_styles.map(s => `<option value="${s.id}">${s.name}</option>`).join("");
  document.getElementById("p-team-role").innerHTML = META.team_roles.map(r => `<option value="${r.id}">${LANG === "en" ? r.en : r.pt}</option>`).join("");
  document.getElementById("srch-pos").innerHTML = `<option value="-1">Todas</option>` + META.positions.map(p => `<option value="${p.id}">${p.pt} (${p.en})</option>`).join("");

  // Sidebar navigation
  document.querySelectorAll(".nav-btn").forEach(b => b.addEventListener("click", () => switchView(b.dataset.view)));
  document.getElementById("top-player-pill").addEventListener("click", () => switchView("player"));
  document.querySelectorAll("#player-subtabs .tab-btn").forEach(b => b.addEventListener("click", () => switchPlayerTab(b.dataset.ptab)));

  // Topbar actions
  document.getElementById("conn-pill").addEventListener("click", async () => {
    const st = await api().try_attach();
    renderStatus(st);
    toast(st.attached ? T(`Conectado a ${st.exe}`, `Connected to ${st.exe}`) : T("PES 2021 não encontrado.", "PES 2021 not found."), st.attached ? "ok" : "err");
  });
  document.getElementById("btn-undo").addEventListener("click", async () => {
    const res = await api().undo_last();
    toast(res.ok ? T(`Desfeito: ${res.label}`, `Undid: ${res.label}`) : res.msg, res.ok ? "ok" : "err");
    if (res.ok && CURRENT_VIEW === "player") loadPlayerDetails();
  });
  document.getElementById("btn-lang").addEventListener("click", () => {
    LANG = LANG === "pt" ? "en" : "pt";
    document.getElementById("btn-lang").textContent = LANG === "pt" ? "PT-BR" : "EN";
    if (CURRENT_PLAYER) renderPlayerDetails(CURRENT_PLAYER);
    if (CURRENT_VIEW === "squad") renderSquadTable();
    toast(LANG === "pt" ? "Idioma: Português (PT-BR)" : "Language: English (EN)");
  });

  // Dashboard
  document.getElementById("btn-dash-refresh").addEventListener("click", pollStatus);

  // Squad filters & sorting
  document.querySelectorAll("#squad-filters .tab-btn").forEach(b => b.addEventListener("click", () => {
    SQUAD_FILTER = b.dataset.filter;
    document.querySelectorAll("#squad-filters .tab-btn").forEach(x => x.classList.toggle("active", x === b));
    renderSquadTable();
  }));
  document.getElementById("squad-search").addEventListener("input", renderSquadTable);
  document.getElementById("btn-squad-refresh").addEventListener("click", loadSquad);
  document.querySelectorAll("#squad-table thead th").forEach(th => th.addEventListener("click", () => {
    const col = th.dataset.sort;
    if (!col) return;
    if (SQUAD_SORT.col === col) SQUAD_SORT.asc = !SQUAD_SORT.asc;
    else { SQUAD_SORT.col = col; SQUAD_SORT.asc = (col === "name"); }
    renderSquadTable();
  }));

  // Player Quick Condition & Bio
  document.getElementById("btn-p-stam100").addEventListener("click", async () => {
    const r = await api().set_quick_condition(100, null, false);
    if (r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast("Fôlego 100%!"); }
  });
  document.getElementById("btn-p-heal").addEventListener("click", async () => {
    const r = await api().set_quick_condition(null, null, true);
    if (r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast("Lesão curada!"); }
  });
  document.getElementById("inp-p-stam").addEventListener("input", e => {
    document.getElementById("lbl-p-stam").textContent = `${e.target.value}%`;
  });
  document.getElementById("inp-p-stam").addEventListener("change", async e => {
    const r = await api().set_quick_condition(Number(e.target.value), null, false);
    if (r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); }
  });
  document.querySelectorAll("#p-arrow-group .arrow-btn").forEach(b => b.addEventListener("click", async () => {
    const r = await api().set_quick_condition(null, Number(b.dataset.arrow), false);
    if (r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast("Seta de forma aplicada!"); }
  }));
  document.getElementById("btn-save-bio").addEventListener("click", async () => {
    const r = await api().update_player_bio({
      name: document.getElementById("bio-name").value,
      shirt: document.getElementById("bio-shirt").value,
      nationality: Number(document.getElementById("bio-nat").value),
      age: Number(document.getElementById("bio-age").value),
      height: Number(document.getElementById("bio-height").value),
      weight: Number(document.getElementById("bio-weight").value),
      foot: Number(document.getElementById("bio-foot").value),
      contract_yr: Number(document.getElementById("bio-contract").value),
      salary_str: document.getElementById("bio-salary").value,
      injury: Number(document.getElementById("bio-injury").value),
      wf_usage: Number(document.getElementById("bio-wfu").value),
      wf_acc: Number(document.getElementById("bio-wfa").value),
      form_cons: Number(document.getElementById("bio-form").value),
      inj_res: Number(document.getElementById("bio-injres").value),
    });
    if (r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast("Dados biográficos salvos!"); }
  });
  document.getElementById("p-playing-style").addEventListener("change", async e => {
    const r = await api().set_playing_style(Number(e.target.value));
    if (r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast("Estilo de jogo atualizado!"); }
  });
  document.getElementById("btn-save-role").addEventListener("click", async () => {
    const r = await api().set_team_role(
      Number(document.getElementById("p-team-role").value),
      Number(document.getElementById("p-team-role-lvl").value)
    );
    if (r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast("Função de equipe salva!"); }
  });
  document.getElementById("btn-export-json").addEventListener("click", async () => {
    const r = await api().export_player_json();
    if (r && r.ok) {
      navigator.clipboard.writeText(r.json);
      toast(`Backup JSON de ${r.name} copiado para a área de transferência!`);
    }
  });
  document.getElementById("btn-import-json").addEventListener("click", async () => {
    const txt = prompt("Cole o JSON de backup do jogador:");
    if (!txt) return;
    const r = await api().import_player_json(txt);
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast("Backup JSON importado!"); }
    else toast(r && r.msg ? r.msg : "Falha ao importar JSON", "err");
  });

  // Search
  document.getElementById("btn-do-search").addEventListener("click", doSearch);
  document.getElementById("srch-query").addEventListener("keydown", e => { if (e.key === "Enter") doSearch(); });

  // Scouts
  document.getElementById("btn-scouts-refresh").addEventListener("click", loadScouts);
  document.querySelectorAll("#scout-tabs .tab-btn").forEach(b => b.addEventListener("click", () => {
    ACTIVE_SCOUT_SLOT = Number(b.dataset.slot);
    document.querySelectorAll("#scout-tabs .tab-btn").forEach(x => x.classList.toggle("active", x === b));
    renderScoutTable();
  }));
  document.getElementById("btn-scout-add-cur").addEventListener("click", () => addPlayerToScout(ACTIVE_SCOUT_SLOT));

  // Finances
  document.getElementById("btn-fin-refresh").addEventListener("click", loadFinances);
  document.getElementById("btn-fin-save").addEventListener("click", async () => {
    const r = await api().set_finances(
      document.getElementById("fin-transfer-inp").value,
      document.getElementById("fin-salary-inp").value
    );
    if (r && r.ok) { toast("Finanças atualizadas!"); loadFinances(); pollStatus(); }
  });

  // Match, Training & BAL
  document.querySelectorAll("#training-mode-grid .mode-card").forEach(c => c.addEventListener("click", async () => {
    const r = await api().set_training_mode(Number(c.dataset.tmode));
    if (r && r.ok) { toast("Multiplicador de treino atualizado!"); pollStatus(); }
  }));
  document.querySelectorAll("#stamina-mode-grid .mode-card").forEach(c => c.addEventListener("click", async () => {
    const r = await api().set_stamina_mode(Number(c.dataset.smode));
    if (r && r.ok) { toast("Modo de fôlego em partida atualizado!"); pollStatus(); }
  }));
  document.getElementById("btn-freeze-clock").addEventListener("click", async () => {
    const r = await api().set_freeze_clock(!FREEZE_CLOCK);
    if (r && r.ok) { toast(!FREEZE_CLOCK ? "Relógio congelado!" : "Relógio liberado!"); pollStatus(); }
  });
  document.getElementById("btn-bal-apply").addEventListener("click", async () => {
    const pts = Number(document.getElementById("bal-focus-inp").value || 21);
    const lck = document.getElementById("bal-lock-chk").checked;
    const r = await api().set_bal_focus(pts, lck);
    if (r && r.ok) toast(`Pontos de foco BAL: ${r.points}${r.lock ? " (Travado)" : ""}`);
    else toast(r && r.msg ? r.msg : "Abra uma carreira BAL", "err");
  });

  await pollStatus();
  setInterval(pollStatus, 600);
}

window.addEventListener("pywebviewready", bootstrap);
