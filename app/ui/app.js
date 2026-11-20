/* PES 2021 Ultimate Studio - Round 2 Senior Frontend (AAA Studio + 100% PT-BR / EN) */

let META = null;
let LANG = (function () {
  try { return localStorage.getItem("pes21_studio_lang") === "en" ? "en" : "pt"; }
  catch (e) { return "pt"; }
})();
const CURRENCIES = [
  { code: "EUR", sym: "€", localePt: "pt-BR", localeEn: "en-US" },
  { code: "BRL", sym: "R$", localePt: "pt-BR", localeEn: "en-US" },
  { code: "USD", sym: "$", localePt: "pt-BR", localeEn: "en-US" },
  { code: "GBP", sym: "£", localePt: "pt-BR", localeEn: "en-US" },
];
let CUR_IDX = (function () {
  try { const v = Number(localStorage.getItem("pes21_studio_cur") || 0); return (v >= 0 && v < 4) ? v : 0; }
  catch (e) { return 0; }
})();

const FONTS = {
  inter:       '"Inter", "Segoe UI", system-ui, sans-serif',
  jakarta:     '"Plus Jakarta Sans", "Segoe UI", system-ui, sans-serif',
  barlow:      '"Barlow", "Bahnschrift", "Segoe UI", sans-serif',
  space:       '"Space Grotesk", "Bahnschrift", "Segoe UI", sans-serif',
  bahnschrift: '"Bahnschrift", "Segoe UI", sans-serif',
  mono:        '"JetBrains Mono", "Cascadia Code", Consolas, monospace',
  segoe:       '"Segoe UI Variable", "Segoe UI", system-ui, sans-serif',
};
let CUR_FONT = (function () {
  try { const f = localStorage.getItem("pes21_studio_font") || "inter"; return FONTS[f] ? f : "inter"; }
  catch (e) { return "inter"; }
})();

const TACTICAL_ORDER = [12, 11, 9, 10, 8, 6, 5, 7, 4, 2, 1, 3, 0];

let CURRENT_VIEW = "dashboard";
let NAV_HISTORY = [];
let CURRENT_PTAB = "bio";
let LAST_STATUS = null;
let LAST_CAREER = 0;
let LAST_FINANCES = null;
let LAST_PLAYER_ADDR = 0;
let CURRENT_PLAYER = null;
let HIGHLIGHTED_POS = null;
let SQUAD_DATA = [];
let SQUAD_CLUB = "";
let SQUAD_FILTER = "all";
let SQUAD_SORT = { col: "ovr", asc: false };
let SEARCH_DATA = [];
let SEARCH_TOTAL = 0;
let SEARCH_SORT = { col: "ovr", asc: false };
let SCOUT_LISTS_DATA = [];
let ACTIVE_SCOUT_SLOT = 4;
let FREEZE_CLOCK = false;
let JSON_MODAL_MODE = "export";
let IS_POLLING = false;
let IS_SEARCHING = false;
let IS_SQUAD_BUSY = false;
let BAL_DIRTY = false;
let CMD_SELECTED = 0;

const api = () => (window.pywebview && window.pywebview.api) || null;
const T = (pt, en) => (LANG === "en" ? en : pt);

function escapeHtml(str) {
  return String(str ?? "")
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&#39;");
}

const I18N = {
  pt: {
    nav_career: "CARREIRA",
    nav_overview: "Visão Geral",
    nav_squad: "Meu Elenco",
    nav_finances: "Finanças",
    nav_player_sec: "ATLETA",
    nav_player: "Jogador Atual",
    nav_search: "Buscar Jogador",
    nav_scouts: "Olheiros & Favoritos",
    nav_engine_sec: "MOTOR & JOGO",
    nav_match_live: "Partida Ao Vivo",
    nav_training_bal: "Treino & BAL",
    cmd_palette_btn: "Comandos Rápidos",
    ph_cmd: "Digite um comando ou tela...",
    cmd_nav_hint: "↑↓ Navegar • ↵ Executar • Esc Fechar",
    waiting_game: "Aguardando PES 2021...",
    disconnected: "Desconectado",
    no_player: "Nenhum atleta selecionado",
    no_player_sub: "Destaque um jogador no jogo ou selecione em Meu Elenco / Buscar Jogador.",
    tip_open_player: "Abrir editor do jogador atual",
    tip_back: "Voltar para a tela anterior (Alt+←)",
    tip_undo: "Desfazer última edição do atleta (Ctrl+Z)",
    tip_font: "Escolher fonte da interface",
    tip_currency: "Alternar moeda de exibição",
    tip_reconnect: "Clique para reconectar ao PES 2021 / Football Life",
    btn_back: "Voltar",
    btn_open_finances: "Ver Finanças Completas",
    btn_undo: "Desfazer",
    btn_refresh: "Atualizar",
    btn_edit: "Editar",
    btn_open_editor: "Abrir Editor",
    btn_reload: "Recarregar",
    btn_save: "Salvar Alterações",
    btn_apply: "Aplicar",
    btn_cancel: "Cancelar",
    btn_clear: "Limpar",
    btn_clear_list: "Limpar Lista",
    btn_search: "Pesquisar",
    btn_add_fav: "+ Favoritos",
    btn_rejuv_3: "-3 Anos",
    btn_age_18: "Idade 18",
    btn_export_json: "Exportar JSON",
    btn_import_json: "Importar JSON",
    dash_title: "Visão Geral do Workbench",
    dash_sub: "Controle em tempo real da Master League, Rumo ao Estrelato e Partidas sem Cheat Engine.",
    dash_active_player: "Atleta em Foco (Ao Vivo)",
    match_idle: "Fora de Partida",
    lbl_club_mode: "Modo & Clube",
    lbl_transfer_budget: "Orçamento de Transferências",
    lbl_salary_avail: "Salários Disponíveis",
    dash_squad_cmd: "Comandante do Elenco (Ações em Massa)",
    dash_squad_hint: "Aplica instantaneamente a todos os jogadores do seu clube",
    cmd_stam_squad: "100% Fôlego (Elenco)",
    cmd_arrow_squad: "Seta Azul (Elenco)",
    cmd_heal_squad: "Curar Todas Lesões",
    cmd_contract_squad: "Renovar Contratos +5 Anos",
    cmd_ovr1_squad: "+1 OVR no Elenco",
    dash_active_hooks: "Status dos Recursos x64",
    short_squad: "Abrir Meu Elenco",
    short_search: "Buscar nos 30.000+ Atletas",
    squad_sub: "Clique em qualquer coluna para ordenar. Clique no atleta para abrir no Editor.",
    kpi_players: "Atletas",
    kpi_avg_ovr: "OVR Médio",
    kpi_avg_age: "Idade Média",
    kpi_alerts: "Lesionados / Fim Contr.",
    kpi_wage_bill: "Folha Anual Ativa",
    kpi_gross_cap: "Teto Salarial Bruto",
    ph_filter_name: "Filtrar nome...",
    flt_all: "Todos",
    flt_exp: "Fim de Contrato",
    flt_inj: "Lesionados",
    flt_u23: "Sub-23",
    cmd_stam_short: "100% Fôlego",
    cmd_arrow_short: "Seta Azul",
    cmd_heal_short: "Curar Lesões",
    cmd_contract_short: "Contratos +5a",
    th_pos: "POS",
    th_player: "ATLETA",
    th_age: "IDADE",
    th_contract: "CONTRATO",
    th_salary: "SALÁRIO",
    th_form: "FORMA",
    th_stamina: "FÔLEGO",
    th_health: "SAÚDE",
    th_club: "CLUBE",
    th_actions: "AÇÕES",
    th_position: "POSIÇÃO",
    th_grade: "GRAU",
    th_isolated_step: "AJUSTE ISOLADO (1 EM 1)",
    empty_squad: "Carregue uma Master League para visualizar o elenco.",
    ptab_bio: "Condição & Bio",
    ptab_stats: "Atributos (25)",
    ptab_poslab: "Lab de Posições",
    ptab_skills: "Estilo & Habilidades",
    ptab_role: "Função de Equipe",
    card_quick_stam: "Fôlego & Saúde Rápida",
    btn_heal_one: "Curar Lesão",
    card_form_arrow: "Seta de Forma (Match Form)",
    arr_blue: "Azul",
    arr_green: "Verde",
    arr_yellow: "Amarela",
    arr_orange: "Laranja",
    arr_red: "Vermelha",
    card_bio: "Dados Biográficos & Contrato",
    lbl_name: "Nome do Jogador",
    lbl_shirt: "Nome na Camisa",
    lbl_nat: "Nacionalidade",
    lbl_age: "Idade (15-50)",
    lbl_height: "Altura (cm)",
    lbl_weight: "Peso (kg)",
    lbl_foot: "Pé Preferido",
    lbl_contract_yr: "Fim do Contrato (Ano)",
    lbl_salary_yr: "Salário Anual (ex: 2.5M)",
    lbl_market_val: "Valor de Mercado (ex: 30M)",
    lbl_inj_days: "Dias Lesionado (0 = OK)",
    lbl_wfu: "Uso Pé Ruim (1-4)",
    lbl_wfa: "Precisão Pé Ruim (1-4)",
    lbl_form: "Estabilidade / Forma (1-8)",
    lbl_injres: "Resistência a Lesões (1-3)",
    lbl_transfer_listed: "Listado para Transferência",
    lbl_loan_listed: "Listado para Empréstimo",
    stats_hint: "Ajuste qualquer atributo em tempo real. O OVR e o radar recalculam instantaneamente.",
    radar_title: "Perfil Técnico (6 Eixos)",
    pitch_title: "Mapa Tático (13 Posições)",
    grade_a: "Grau A",
    grade_b: "Grau B",
    grade_c: "Grau C",
    btn_all_99: "Tudo 99",
    btn_all_90: "Tudo 90",
    btn_all_80: "Tudo 80",
    btn_plus5_all: "+5 em Todos",
    poslab_title: "Lab de Posições (Precisão Cirúrgica)",
    poslab_hint: "OVR já inclui bônus do Grau. +1/-1 move 1 por vez e a posição principal nunca é alterada.",
    card_playing_style: "Estilo de Jogo (Playing Style)",
    card_player_skills: "Habilidades do Jogador",
    card_com_ai: "Estilos de Jogo do COM (AI)",
    card_team_role: "Função de Equipe (Team Role)",
    lbl_role: "Função",
    lbl_role_lvl: "Nível (0 a 3)",
    btn_apply_role: "Aplicar Função de Equipe",
    srch_title: "Buscar Jogador (Base Completa)",
    srch_sub: "Pesquise instantaneamente entre todos os 30.000+ jogadores carregados na Master League.",
    lbl_send_to: "Enviar para:",
    ph_search: "Ex: Messi, Vinicius, Endrick...",
    lbl_min_ovr: "OVR Mín.",
    lbl_max_age: "Idade Máx.",
    lbl_free_agent: "Sem Clube",
    empty_search: "Use os filtros acima e clique em Pesquisar.",
    scouts_title: "Olheiros & Favoritos (Nativo)",
    scouts_sub: "Grava apenas o ponteiro nativo na lista da Master League, sem alterar contratos ou cláusulas.",
    btn_refresh_lists: "Atualizar Listas",
    scout_shortlist: "Favoritos (Shortlist)",
    scout_1: "Olheiro 1",
    scout_2: "Olheiro 2",
    scout_3: "Olheiro 3",
    btn_scout_add_cur: "+ Adicionar Jogador Atual nesta Lista",
    empty_scouts: "Clique em Atualizar Listas para carregar.",
    fin_title: "Finanças da Master League",
    fin_sub: "Cálculo exato de Salários Disponíveis descontando folha ativa e pendentes (0x14156D170).",
    card_edit_budgets: "Editar Orçamentos",
    lbl_trf_input: "Orçamento de Transferências (ex: 50M, 500k)",
    lbl_sal_input: "Salários Disponíveis (ex: 15M, 2.5M)",
    btn_apply_values: "Aplicar Valores",
    btn_p10m_trf: "+10M Transf.",
    btn_p100m_trf: "+100M Transf.",
    btn_p10m_sal: "+10M Salários",
    btn_p50m_sal: "+50M Salários",
    match_live_title: "Partida Ao Vivo",
    match_live_sub: "Controles em tempo real durante a partida (hooks x64 ativados sob demanda e restaurados ao fechar).",
    training_bal_title: "Treino & Rumo ao Estrelato (BAL)",
    training_bal_sub: "Acelere a evolução de Sub-XP na Master League/BAL e desbloqueie o limite de 21 Pontos de Foco no BAL.",
    card_training_xp: "Velocidade de Treino (Multiplicador Sub-XP)",
    tm_0: "Normal (1x)", tm_0_d: "Evolução padrão do jogo",
    tm_1: "Rápido (2x)", tm_1_d: "Dobro de XP por treino",
    tm_2: "Acelerado (5x)", tm_2_d: "5x mais rápido",
    tm_3: "Turbo (10x)", tm_3_d: "10x mais rápido",
    tm_4: "Ultra (50x)", tm_4_d: "Evolução extrema",
    tm_5: "Instantâneo", tm_5_d: "XP máximo imediato",
    card_match_stam: "Fôlego Seletivo em Partida",
    sm_0: "Normal", sm_0_d: "Desgaste padrão",
    sm_1: "Casa 100%", sm_1_d: "Time da casa infinito",
    sm_2: "Fora 100%", sm_2_d: "Visitante infinito",
    sm_3: "Ambos 100%", sm_3_d: "Todos em campo 100%",
    sm_4: "Casa 100% / Fora 5%", sm_4_d: "Pressão total em casa",
    sm_5: "Fora 100% / Casa 5%", sm_5_d: "Pressão total visitante",
    card_clock: "Relógio da Partida",
    clock_desc: "Congelar cronômetro durante a partida",
    clock_off: "Congelar: OFF",
    card_bal: "Pontos de Foco (Rumo ao Estrelato / BAL)",
    lbl_lock_99: "Travar valor",
    lbl_unsaved: "Alterações não salvas",
  },
  en: {
    nav_career: "CAREER",
    nav_overview: "Overview",
    nav_squad: "My Squad",
    nav_finances: "Finances",
    nav_player_sec: "PLAYER",
    nav_player: "Current Player",
    nav_search: "Search Player",
    nav_scouts: "Scouts & Shortlist",
    nav_engine_sec: "ENGINE & GAME",
    nav_match_live: "Live Match",
    nav_training_bal: "Training & BAL",
    cmd_palette_btn: "Quick Commands",
    ph_cmd: "Type a command or view...",
    cmd_nav_hint: "↑↓ Navigate • ↵ Run • Esc Close",
    waiting_game: "Waiting for PES 2021...",
    disconnected: "Disconnected",
    no_player: "No player selected",
    no_player_sub: "Highlight a player in-game or pick one from My Squad / Search.",
    tip_open_player: "Open Current Player editor",
    tip_back: "Go back to previous view (Alt+←)",
    tip_undo: "Undo last edit on this player (Ctrl+Z)",
    tip_font: "Choose UI font family",
    tip_currency: "Cycle display currency",
    tip_reconnect: "Click to reconnect to PES 2021 / Football Life",
    btn_back: "Back",
    btn_open_finances: "Full Finances View",
    btn_undo: "Undo",
    btn_refresh: "Refresh",
    btn_edit: "Edit",
    btn_open_editor: "Open Editor",
    btn_reload: "Reload",
    btn_save: "Save Changes",
    btn_apply: "Apply",
    btn_cancel: "Cancel",
    btn_clear: "Clear",
    btn_clear_list: "Clear List",
    btn_search: "Search",
    btn_add_fav: "+ Shortlist",
    btn_rejuv_3: "-3 Years",
    btn_age_18: "Age 18",
    btn_export_json: "Export JSON",
    btn_import_json: "Import JSON",
    dash_title: "Workbench Overview",
    dash_sub: "Real-time control of Master League, Become a Legend, and Matches without Cheat Engine.",
    dash_active_player: "Active Player (Live)",
    match_idle: "Not in Match",
    lbl_club_mode: "Mode & Club",
    lbl_transfer_budget: "Transfer Budget",
    lbl_salary_avail: "Available Salary Budget",
    dash_squad_cmd: "Squad Commander (Bulk Actions)",
    dash_squad_hint: "Applies instantly to every player in your club",
    cmd_stam_squad: "100% Stamina (Squad)",
    cmd_arrow_squad: "Blue Arrow (Squad)",
    cmd_heal_squad: "Heal All Injuries",
    cmd_contract_squad: "Extend Contracts +5 Years",
    cmd_ovr1_squad: "+1 OVR to Squad",
    dash_active_hooks: "Native x64 Features Status",
    short_squad: "Open My Squad",
    short_search: "Search 30,000+ Players",
    squad_sub: "Click any column header to sort. Click a player to open in the Editor.",
    kpi_players: "Players",
    kpi_avg_ovr: "Avg OVR",
    kpi_avg_age: "Avg Age",
    kpi_alerts: "Injured / Expiring",
    kpi_wage_bill: "Active Annual Wages",
    kpi_gross_cap: "Gross Salary Cap",
    ph_filter_name: "Filter by name...",
    flt_all: "All",
    flt_exp: "Expiring Contract",
    flt_inj: "Injured",
    flt_u23: "Under-23",
    cmd_stam_short: "100% Stamina",
    cmd_arrow_short: "Blue Arrow",
    cmd_heal_short: "Heal Injuries",
    cmd_contract_short: "Contracts +5y",
    th_pos: "POS",
    th_player: "PLAYER",
    th_age: "AGE",
    th_contract: "CONTRACT",
    th_salary: "SALARY",
    th_form: "FORM",
    th_stamina: "STAMINA",
    th_health: "HEALTH",
    th_club: "CLUB",
    th_actions: "ACTIONS",
    th_position: "POSITION",
    th_grade: "GRADE",
    th_isolated_step: "ISOLATED STEP (1 BY 1)",
    empty_squad: "Load a Master League save to view your squad.",
    ptab_bio: "Condition & Bio",
    ptab_stats: "Attributes (25)",
    ptab_poslab: "Position Lab",
    ptab_skills: "Style & Skills",
    ptab_role: "Team Role",
    card_quick_stam: "Quick Stamina & Health",
    btn_heal_one: "Heal Injury",
    card_form_arrow: "Match Form Arrow",
    arr_blue: "Blue",
    arr_green: "Green",
    arr_yellow: "Yellow",
    arr_orange: "Orange",
    arr_red: "Red",
    card_bio: "Biography & Contract",
    lbl_name: "Player Name",
    lbl_shirt: "Shirt Name",
    lbl_nat: "Nationality",
    lbl_age: "Age (15-50)",
    lbl_height: "Height (cm)",
    lbl_weight: "Weight (kg)",
    lbl_foot: "Stronger Foot",
    lbl_contract_yr: "Contract Expiry (Year)",
    lbl_salary_yr: "Annual Salary (e.g. 2.5M)",
    lbl_market_val: "Market Value (e.g. 30M)",
    lbl_inj_days: "Injury Days (0 = OK)",
    lbl_wfu: "Weak Foot Usage (1-4)",
    lbl_wfa: "Weak Foot Accuracy (1-4)",
    lbl_form: "Form Stability (1-8)",
    lbl_injres: "Injury Resistance (1-3)",
    lbl_transfer_listed: "Transfer Listed",
    lbl_loan_listed: "Loan Listed",
    stats_hint: "Adjust any attribute in real time. Overall and radar recalculate immediately.",
    radar_title: "Technical Profile (6 Axes)",
    pitch_title: "Tactical Pitch (13 Positions)",
    grade_a: "Grade A",
    grade_b: "Grade B",
    grade_c: "Grade C",
    btn_all_99: "All 99",
    btn_all_90: "All 90",
    btn_all_80: "All 80",
    btn_plus5_all: "+5 All Stats",
    poslab_title: "Position Lab (Surgical Precision)",
    poslab_hint: "Displayed OVR includes Grade bonus. +1/-1 steps 1 by 1 and primary position is never altered.",
    card_playing_style: "Playing Style",
    card_player_skills: "Player Skills",
    card_com_ai: "COM Playing Styles (AI)",
    card_team_role: "Team Role",
    lbl_role: "Role",
    lbl_role_lvl: "Level (0 to 3)",
    btn_apply_role: "Apply Team Role",
    srch_title: "Search Player (Full Database)",
    srch_sub: "Instantly filter all 30,000+ players loaded in Master League.",
    lbl_send_to: "Send to:",
    ph_search: "e.g. Messi, Vinicius, Endrick...",
    lbl_min_ovr: "Min OVR",
    lbl_max_age: "Max Age",
    lbl_free_agent: "Free Agent",
    empty_search: "Set filters above and click Search.",
    scouts_title: "Scouts & Shortlist (Native)",
    scouts_sub: "Writes only the native player handle into Master League RAM, with zero side effects.",
    btn_refresh_lists: "Refresh Lists",
    scout_shortlist: "Shortlist (Favorites)",
    scout_1: "Scout 1",
    scout_2: "Scout 2",
    scout_3: "Scout 3",
    btn_scout_add_cur: "+ Add Current Player to This List",
    empty_scouts: "Click Refresh Lists to load.",
    fin_title: "Master League Finances",
    fin_sub: "Exact Available Salary Budget deducting active squad wages and pending overrides (0x14156D170).",
    card_edit_budgets: "Edit Budgets",
    lbl_trf_input: "Transfer Budget (e.g. 50M, 500k)",
    lbl_sal_input: "Available Salary Budget (e.g. 15M, 2.5M)",
    btn_apply_values: "Apply Values",
    btn_p10m_trf: "+10M Transfer",
    btn_p100m_trf: "+100M Transfer",
    btn_p10m_sal: "+10M Salary",
    btn_p50m_sal: "+50M Salary",
    match_live_title: "Live Match",
    match_live_sub: "Real-time match controls (native x64 hooks activate on demand and restore cleanly on exit).",
    training_bal_title: "Training & Become a Legend (BAL)",
    training_bal_sub: "Accelerate Sub-XP growth in Master League/BAL and unlock the 21 Focus Points cap in BAL.",
    card_training_xp: "Training Speed (Sub-XP Multiplier)",
    tm_0: "Normal (1x)", tm_0_d: "Default game progression",
    tm_1: "Fast (2x)", tm_1_d: "Double XP per session",
    tm_2: "Accelerated (5x)", tm_2_d: "5x faster growth",
    tm_3: "Turbo (10x)", tm_3_d: "10x faster growth",
    tm_4: "Ultra (50x)", tm_4_d: "Extreme growth",
    tm_5: "Instant", tm_5_d: "Maximum XP immediately",
    card_match_stam: "Selective Match Stamina",
    sm_0: "Normal", sm_0_d: "Default stamina drain",
    sm_1: "Home 100%", sm_1_d: "Infinite home stamina",
    sm_2: "Away 100%", sm_2_d: "Infinite away stamina",
    sm_3: "Both 100%", sm_3_d: "All players 100% stamina",
    sm_4: "Home 100% / Away 5%", sm_4_d: "Total home press",
    sm_5: "Away 100% / Home 5%", sm_5_d: "Total away press",
    card_clock: "Match Clock",
    clock_desc: "Freeze match timer during gameplay",
    clock_off: "Freeze: OFF",
    card_bal: "Focus Points (Become a Legend / BAL)",
    lbl_lock_99: "Lock value",
    lbl_unsaved: "Unsaved changes",
  },
};

function tKey(key) {
  return (I18N[LANG] && I18N[LANG][key]) || (I18N.pt && I18N.pt[key]) || key;
}

function fmtCodeMsg(res) {
  if (!res) return T("Erro de comunicação.", "Communication error.");
  switch (res.code) {
    case "not_connected": return T("Jogo não conectado.", "Game not connected.");
    case "no_career": return T("Abra uma Master League ou BAL primeiro.", "Open a Master League or BAL save first.");
    case "no_player": return tKey("no_player");
    case "nothing_to_undo": return T("Nenhuma alteração para desfazer.", "Nothing to undo.");
    case "invalid_money": return T("Valor monetário inválido (use ex: 25M, 500k ou 1.5B).", "Invalid money amount (use e.g. 25M, 500k or 1.5B).");
    case "hook_failed": return T("Não foi possível ativar o hook x64 neste executável.", "Could not activate x64 hook on this executable.");
    case "already_at_ovr": return T(`Já está em ${res.ovr} OVR.`, `Already at ${res.ovr} OVR.`);
    case "pri_updated": return T(`OVR Principal: ${res.orig} -> ${res.final}`, `Primary OVR: ${res.orig} -> ${res.final}`);
    case "pos_updated": return T(`Posição ajustada: ${res.orig} -> ${res.final} (Principal mantido em ${res.pri})`, `Position updated: ${res.orig} -> ${res.final} (Primary kept at ${res.pri})`);
    case "pos_partial": {
      const hint = res.needs_grade ? T(" - suba o Grau para A para ir além", " - raise Grade to A to go higher") : "";
      return T(`Posição: ${res.orig} -> ${res.final} (teto isolado sem alterar principal ${res.pri}${hint})`, `Position: ${res.orig} -> ${res.final} (isolated cap without changing primary ${res.pri}${hint})`);
    }
    case "iso_limit": {
      const hint = res.needs_grade ? T(" (suba o Grau para B ou A para liberar mais OVR)", " (raise Grade to B or A to unlock more OVR)") : "";
      return T(`Limite isolado atingido (${res.orig} OVR, principal mantido em ${res.pri})${hint}`, `Isolated limit reached (${res.orig} OVR, primary kept at ${res.pri})${hint}`);
    }
    case "scout_added": return T("Adicionado na lista nativa!", "Added to native list!");
    case "scout_dup": return T("Jogador já está nesta lista.", "Player is already in this list.");
    case "scout_full": return T("Esta lista está cheia.", "This list is full.");
    case "invalid_json": return T("Backup JSON inválido.", "Invalid JSON backup.");
    default: return T("Operação não pôde ser concluída.", "Operation could not be completed.");
  }
}

function toast(msg, type = "ok") {
  const wrap = document.getElementById("toast-container");
  const el = document.createElement("div");
  el.className = `toast ${type}`;
  el.innerHTML = `<span class="toast-ico">${type === "err" ? "!" : "✓"}</span><span>${escapeHtml(msg)}</span>`;
  const dismiss = () => {
    el.style.opacity = "0";
    el.style.transform = "translateY(4px)";
    setTimeout(() => el.remove(), 220);
  };
  el.addEventListener("click", dismiss);
  wrap.appendChild(el);
  setTimeout(dismiss, 3400);
}

function fmtMoney(raw) {
  const cur = CURRENCIES[CUR_IDX] || CURRENCIES[0];
  const euros = Math.round((raw || 0) * 100);
  const abs = Math.abs(euros);
  const s = abs.toLocaleString(LANG === "en" ? cur.localeEn : cur.localePt);
  return (euros < 0 ? `-${cur.sym} ` : `${cur.sym} `) + s;
}

function fmtClub(club) { return club || T("Sem Clube / Livre", "Free Agent"); }
function fmtMode(code) {
  if (code === "ml") return "Master League";
  if (code === "bal") return T("Rumo ao Estrelato (BAL)", "Become a Legend (BAL)");
  if (code === "menu") return T("Menu / Exibição", "Menu / Exhibition");
  return tKey("waiting_game");
}
function posShort(id) {
  if (!META) return "---";
  const p = META.positions[id];
  return p ? (LANG === "en" ? p.en : p.pt) : "---";
}
function posClass(id) { return id === 0 ? "pos-gk" : id <= 3 ? "pos-def" : id <= 8 ? "pos-mid" : "pos-att"; }
function ovrClass(ovr) { return ovr >= 90 ? "ovr-90" : ovr >= 80 ? "ovr-80" : ovr >= 70 ? "ovr-70" : "ovr-low"; }
function stamColor(v) { return v >= 85 ? "#10b981" : v >= 65 ? "#f59e0b" : "#ef4444"; }

const ARROW_ICONS = [
  { ico: "⬇", col: "#f87171" },
  { ico: "↘", col: "#fb923c" },
  { ico: "➡", col: "#facc15" },
  { ico: "↗", col: "#4ade80" },
  { ico: "⬆", col: "#38bdf8" },
];

// -------------------------------------------------------------------
// LANGUAGE & CURRENCY
// -------------------------------------------------------------------
function populateLocalizedSelects() {
  if (!META) return;
  const footEl = document.getElementById("bio-foot");
  const curFoot = footEl.value || "0";
  footEl.innerHTML = `<option value="0">${escapeHtml(T("Direito", "Right"))}</option><option value="1">${escapeHtml(T("Esquerdo", "Left"))}</option>`;
  footEl.value = curFoot;

  const roleEl = document.getElementById("p-team-role");
  const curRole = roleEl.value || "0";
  roleEl.innerHTML = META.team_roles.map(r => `<option value="${r.id}">${escapeHtml(LANG === "en" ? r.en : r.pt)}</option>`).join("");
  roleEl.value = curRole;

  const lvlEl = document.getElementById("p-team-role-lvl");
  const curLvl = lvlEl.value || "0";
  lvlEl.innerHTML = [
    `<option value="0">${escapeHtml(T("Nível 0", "Level 0"))}</option>`,
    `<option value="1">${escapeHtml(T("Nível 1", "Level 1"))}</option>`,
    `<option value="2">${escapeHtml(T("Nível 2", "Level 2"))}</option>`,
    `<option value="3">${escapeHtml(T("Nível 3 (Máximo)", "Level 3 (Max)"))}</option>`,
  ].join("");
  lvlEl.value = curLvl;

  const spEl = document.getElementById("srch-pos");
  const curSp = spEl.value || "-1";
  spEl.innerHTML = `<option value="-1">${escapeHtml(T("Todas", "All"))}</option>` +
    META.positions.map(p => `<option value="${p.id}">${escapeHtml(LANG === "en" ? `${p.en} (${p.name_en})` : `${p.pt} (${p.name_pt})`)}</option>`).join("");
  spEl.value = curSp;

  const scSel = document.getElementById("srch-target-scout");
  const curSc = scSel.value || "4";
  scSel.innerHTML = [
    `<option value="4">${escapeHtml(tKey("scout_shortlist"))}</option>`,
    `<option value="1">${escapeHtml(tKey("scout_1"))}</option>`,
    `<option value="2">${escapeHtml(tKey("scout_2"))}</option>`,
    `<option value="3">${escapeHtml(tKey("scout_3"))}</option>`,
  ].join("");
  scSel.value = curSc;
}

function applyFont(key) {
  const k = FONTS[key] ? key : "inter";
  CUR_FONT = k;
  document.documentElement.style.setProperty("--font-main", FONTS[k]);
  const sel = document.getElementById("sel-font");
  if (sel) sel.value = k;
  try { localStorage.setItem("pes21_studio_font", k); } catch (e) {}
}

function updateCurrencyBtn() {
  const cur = CURRENCIES[CUR_IDX] || CURRENCIES[0];
  document.getElementById("btn-currency").textContent = `${cur.sym} ${cur.code}`;
}

function applyLanguage() {
  document.documentElement.lang = LANG === "en" ? "en" : "pt-BR";
  document.getElementById("btn-lang").textContent = LANG === "en" ? "EN" : "PT-BR";
  updateCurrencyBtn();

  document.querySelectorAll("[data-i18n]").forEach(el => {
    const k = el.dataset.i18n;
    if (k && I18N[LANG][k]) el.textContent = I18N[LANG][k];
  });
  document.querySelectorAll("[data-i18n-placeholder]").forEach(el => {
    const k = el.dataset.i18nPlaceholder;
    if (k && I18N[LANG][k]) el.placeholder = I18N[LANG][k];
  });
  document.querySelectorAll("[data-i18n-title]").forEach(el => {
    const k = el.dataset.i18nTitle;
    if (k && I18N[LANG][k]) el.title = I18N[LANG][k];
  });

  populateLocalizedSelects();
  updateBackButton();
  if (LAST_STATUS) renderStatus(LAST_STATUS);
  if (CURRENT_PLAYER) renderPlayerDetails(CURRENT_PLAYER);
  renderSquadTable();
  renderSearchTable();
  renderScoutTable();
  if (LAST_FINANCES) renderFinances(LAST_FINANCES);
}

// -------------------------------------------------------------------
// NAVIGATION (with real Back History Stack + Alt+Left / Mouse Back)
// -------------------------------------------------------------------
const VIEW_KEY_MAP = {
  dashboard: "nav_overview",
  squad: "nav_squad",
  finances: "nav_finances",
  player: "nav_player",
  search: "nav_search",
  scouts: "nav_scouts",
  match: "nav_match_live",
  training: "nav_training_bal",
};

function updateBackButton() {
  const btn = document.getElementById("btn-nav-back");
  const lbl = document.getElementById("nav-back-label");
  if (!btn || !lbl) return;
  if (NAV_HISTORY.length > 0) {
    const prev = NAV_HISTORY[NAV_HISTORY.length - 1];
    lbl.textContent = tKey(VIEW_KEY_MAP[prev] || "btn_back");
    btn.style.display = "inline-flex";
  } else {
    btn.style.display = "none";
  }
}

function switchView(viewName, skipAutoLoad = false, fromBack = false) {
  if (!fromBack && viewName !== CURRENT_VIEW) {
    NAV_HISTORY.push(CURRENT_VIEW);
    if (NAV_HISTORY.length > 20) NAV_HISTORY.shift();
  }
  CURRENT_VIEW = viewName;
  updateBackButton();
  document.querySelectorAll(".nav-btn").forEach(b => b.classList.toggle("active", b.dataset.view === viewName));
  document.querySelectorAll(".view-panel").forEach(p => p.classList.toggle("active", p.id === `view-${viewName}`));
  if (skipAutoLoad) return;
  if (viewName === "squad") loadSquad();
  if (viewName === "player") loadPlayerDetails();
  if (viewName === "scouts") loadScouts();
  if (viewName === "finances") loadFinances();
}

function goBack() {
  if (!NAV_HISTORY.length) return;
  const prev = NAV_HISTORY.pop();
  switchView(prev, false, true);
}

function switchPlayerTab(tabName) {
  CURRENT_PTAB = tabName;
  document.querySelectorAll("#player-subtabs .tab-btn").forEach(b => b.classList.toggle("active", b.dataset.ptab === tabName));
  document.querySelectorAll(".ptab-pane").forEach(p => {
    p.style.display = (p.id === `ptab-${tabName}`) ? "block" : "none";
  });
}

// -------------------------------------------------------------------
// LIVE POLLING
// -------------------------------------------------------------------
async function pollStatus() {
  if (!api() || IS_POLLING) return;
  IS_POLLING = true;
  try {
    const st = await api().get_status();
    LAST_STATUS = st;
    renderStatus(st);
  } catch (e) {
  } finally {
    IS_POLLING = false;
  }
}

function renderStatus(st) {
  if (st.career !== LAST_CAREER) {
    LAST_CAREER = st.career || 0;
    BAL_DIRTY = false;
    document.getElementById("bal-unsaved").style.display = "none";
  }

  const connPill = document.getElementById("conn-pill");
  const connText = document.getElementById("conn-text");
  if (st.attached) {
    connPill.classList.add("online");
    connText.textContent = `${st.exe} (${st.pid})`;
  } else {
    connPill.classList.remove("online");
    connText.textContent = st.access_denied ? T("Execute como Admin", "Run as Admin") : tKey("waiting_game");
  }

  // Live Match Badges (Dashboard + Match View)
  const inMatch = !!st.in_match;
  const mText = inMatch ? T("EM PARTIDA", "IN MATCH") : tKey("match_idle");
  [["dash-match-badge", "dash-match-text"], ["match-view-badge", "match-view-badge-text"]].forEach(([bId, tId]) => {
    const bEl = document.getElementById(bId);
    const tEl = document.getElementById(tId);
    if (bEl) bEl.classList.toggle("in-match", inMatch);
    if (tEl) tEl.textContent = mText;
  });

  const modeLabel = fmtMode(st.mode_code);
  document.getElementById("side-mode-text").textContent = modeLabel;
  document.getElementById("side-club-text").textContent = st.club || "";
  document.getElementById("dash-club").textContent = st.club || (st.attached ? st.exe : "---");
  document.getElementById("dash-mode").textContent = modeLabel;

  if (st.finances) {
    document.getElementById("dash-transfer").textContent = fmtMoney(st.finances.transfer_raw);
    const salEl = document.getElementById("dash-salary");
    salEl.textContent = fmtMoney(st.finances.salary_avail_raw);
    salEl.style.color = st.finances.salary_avail_raw < 0 ? "var(--red)" : "var(--gold)";
  } else {
    document.getElementById("dash-transfer").textContent = fmtMoney(0);
    document.getElementById("dash-salary").textContent = fmtMoney(0);
  }

  // Honest feature status (only ACTIVE when the feature is actually enabled!)
  const hooksEl = document.getElementById("dash-hooks-list");
  if (hooksEl) {
    const hk = st.hooks || {};
    const items = [
      [!!hk[1], T("Captura Automática de Jogador", "Auto Player Capture")],
      [!!hk[3] && Number(st.training_mode || 0) > 0, T("Multiplicador de Treino (XP)", "Training XP Multiplier")],
      [!!hk[4] && (!!st.bal_lock || Number(st.bal_focus || 0) > 21), T("Desbloqueio de Foco BAL (21->99)", "BAL Focus Cap Unlock")],
      [!!hk[5] && !!st.freeze_clock, T("Relógio Congelado na Partida", "Match Clock Freeze")],
      [!!hk[6] && Number(st.stamina_mode || 0) > 0, T("Fôlego Seletivo em Partida", "Selective Match Stamina")],
    ];
    const onLbl = T("ATIVO", "ACTIVE");
    const offLbl = T("Em espera", "Standby");
    hooksEl.innerHTML = items.map(([on, label]) => `
      <div style="display:flex; justify-content:space-between; align-items:center;">
        <span style="color:var(--subtext);">${escapeHtml(label)}</span>
        <span class="${on ? "pill-ok" : ""}" style="${on ? "" : "color:var(--muted); font-size:11px;"}">${escapeHtml(on ? onLbl : offLbl)}</span>
      </div>`).join("");
  }

  // Sync Match/Training/BAL controls
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
  if (!BAL_DIRTY && st.career > 0) {
    const balInp = document.getElementById("bal-focus-inp");
    if (balInp && document.activeElement !== balInp) balInp.value = String(st.bal_focus || 0);
    const balChk = document.getElementById("bal-lock-chk");
    if (balChk && document.activeElement !== balChk) balChk.checked = !!st.bal_lock;
  }

  // Topbar + Dashboard Active Player Quick Card
  const p = st.player;
  const undoBtn = document.getElementById("btn-undo");
  if (p) {
    const ovrEl = document.getElementById("top-p-ovr");
    ovrEl.textContent = p.ovr;
    ovrEl.className = `ovr-badge ${ovrClass(p.ovr)}`;
    const posEl = document.getElementById("top-p-pos");
    posEl.textContent = posShort(p.pos);
    posEl.className = `pos-chip ${posClass(p.pos)}`;
    document.getElementById("top-p-name").textContent = p.name;
    const arr = ARROW_ICONS[p.arrow] || ARROW_ICONS[2];
    const ageUnit = T("a", "y");
    const metaHtml = `${escapeHtml(fmtClub(p.club))} • ${p.age}${ageUnit} • ${p.stamina}% <span style="color:${arr.col}; font-weight:700;">${arr.ico}</span>`;
    document.getElementById("top-p-meta").innerHTML = metaHtml;

    const dpOvr = document.getElementById("dp-ovr");
    dpOvr.textContent = p.ovr;
    dpOvr.className = `ovr-badge ${ovrClass(p.ovr)}`;
    document.getElementById("dp-name").textContent = `${p.name} (${posShort(p.pos)})`;
    document.getElementById("dp-meta").innerHTML = metaHtml;
    document.getElementById("btn-dp-heal").disabled = (p.injury || 0) === 0;

    const uCnt = p.undo_count || 0;
    document.getElementById("undo-badge").textContent = `(${uCnt})`;
    undoBtn.disabled = uCnt === 0;

    if (p.addr !== LAST_PLAYER_ADDR) {
      LAST_PLAYER_ADDR = p.addr;
      HIGHLIGHTED_POS = null;
      if (CURRENT_VIEW === "player") loadPlayerDetails(p.addr);
    }
  } else {
    LAST_PLAYER_ADDR = 0;
    CURRENT_PLAYER = null;
    HIGHLIGHTED_POS = null;
    document.getElementById("top-p-ovr").textContent = "--";
    document.getElementById("top-p-ovr").className = "ovr-badge ovr-low";
    document.getElementById("top-p-pos").textContent = "---";
    document.getElementById("top-p-pos").className = "pos-chip pos-mid";
    document.getElementById("top-p-name").textContent = tKey("no_player");
    document.getElementById("top-p-meta").textContent = tKey("no_player_sub");
    document.getElementById("dp-ovr").textContent = "--";
    document.getElementById("dp-ovr").className = "ovr-badge ovr-low";
    document.getElementById("dp-name").textContent = tKey("no_player");
    document.getElementById("dp-meta").textContent = "---";
    document.getElementById("undo-badge").textContent = "(0)";
    undoBtn.disabled = true;
    showPlayerEmptyState(true);
  }
}

function showPlayerEmptyState(isEmpty) {
  document.getElementById("player-empty-state").style.display = isEmpty ? "flex" : "none";
  document.getElementById("player-studio-body").style.display = isEmpty ? "none" : "flex";
}

// -------------------------------------------------------------------
// PLAYER STUDIO
// -------------------------------------------------------------------
async function loadPlayerDetails(addr = null) {
  if (!api()) return;
  const d = await api().get_player_details(addr);
  if (!d || !d.ok) {
    CURRENT_PLAYER = null;
    showPlayerEmptyState(true);
    return;
  }
  CURRENT_PLAYER = d;
  LAST_PLAYER_ADDR = d.addr;
  showPlayerEmptyState(false);
  renderPlayerDetails(d);
}

function pulseOvrShield() {
  const el = document.getElementById("p-hero-ovr");
  if (!el) return;
  el.classList.remove("pulse");
  void el.offsetWidth;
  el.classList.add("pulse");
}

function updateHeroHeader(d) {
  const heroOvr = document.getElementById("p-hero-ovr");
  heroOvr.innerHTML = `${d.ovr}<small>OVR</small>`;
  heroOvr.className = `hero-shield ${ovrClass(d.ovr)}`;

  const heroPos = document.getElementById("p-hero-pos");
  heroPos.textContent = posShort(d.reg_pos);
  heroPos.className = `pos-chip ${posClass(d.reg_pos)}`;

  document.getElementById("p-hero-name").textContent = d.name;
  const arr = ARROW_ICONS[d.arrow] || ARROW_ICONS[2];
  const footTxt = d.foot ? T("Canhoto", "Left Foot") : T("Destro", "Right Foot");
  document.getElementById("p-hero-sub").innerHTML =
    `<span>${escapeHtml(fmtClub(d.club))}</span> • <span>ID ${d.pid}</span> • <span>${d.age}${T("a", "y")}</span> • <span>${escapeHtml(footTxt)}</span> • <span>${escapeHtml(fmtMoney(d.market_raw))}</span> • <span>${escapeHtml(fmtMoney(d.salary_raw))}/${T("ano", "yr")}</span> • <span style="color:${arr.col}; font-weight:700;">${arr.ico} ${d.stamina}%</span>`;

  const uCnt = d.undo_count || 0;
  document.getElementById("undo-badge").textContent = `(${uCnt})`;
  document.getElementById("btn-undo").disabled = uCnt === 0;
}

function renderPlayerDetails(d) {
  const scroller = document.getElementById("main-scroll");
  const prevScroll = scroller ? scroller.scrollTop : 0;

  showPlayerEmptyState(false);
  updateHeroHeader(d);

  // Quick Condition & Injury pill
  document.getElementById("inp-p-stam").value = d.stamina;
  document.getElementById("lbl-p-stam").textContent = `${d.stamina}%`;
  const injPill = document.getElementById("p-inj-pill");
  if (d.injury > 0) {
    injPill.className = "pill-inj";
    injPill.textContent = `${d.injury}d`;
    document.getElementById("btn-p-heal").disabled = false;
  } else {
    injPill.className = "pill-ok";
    injPill.textContent = "OK";
    document.getElementById("btn-p-heal").disabled = true;
  }
  document.querySelectorAll("#p-arrow-group .arrow-btn").forEach(b => {
    b.classList.toggle("active", Number(b.dataset.arrow) === Number(d.arrow));
  });

  // Bio
  document.getElementById("bio-name").value = d.name;
  document.getElementById("bio-shirt").value = d.shirt;
  document.getElementById("bio-nat").value = String(d.nationality);
  document.getElementById("bio-age").value = d.age;
  document.getElementById("bio-height").value = d.height;
  document.getElementById("bio-weight").value = d.weight;
  document.getElementById("bio-foot").value = String(d.foot);
  document.getElementById("bio-contract").value = d.contract_yr;
  document.getElementById("bio-salary").value = fmtMoney(d.salary_raw);
  document.getElementById("bio-market").value = fmtMoney(d.market_raw);
  document.getElementById("bio-injury").value = d.injury;
  document.getElementById("bio-wfu").value = d.wf_usage;
  document.getElementById("bio-wfa").value = d.wf_acc;
  document.getElementById("bio-form").value = d.form_cons;
  document.getElementById("bio-injres").value = d.inj_res;
  document.getElementById("bio-tlist").checked = !!d.transfer_listed;
  document.getElementById("bio-llist").checked = !!d.loan_listed;

  renderStatsRadar(d.stats);
  renderStatsGroups(d);
  renderPositionPitch(d);
  renderPositionLab(d);
  renderSkillsAndStyle(d);
  document.getElementById("p-team-role").value = String(d.team_role);
  document.getElementById("p-team-role-lvl").value = String(d.team_role_lvl);

  if (scroller) scroller.scrollTop = prevScroll;
}

// -------------------------------------------------------------------
// LIVE 6-AXIS SVG RADAR (100% Localized PT-BR / EN)
// -------------------------------------------------------------------
function computeRadarAxes(stats) {
  const avg = fids => Math.round(fids.reduce((s, f) => s + Number(stats[String(f)] || 40), 0) / fids.length);
  return [
    { lbl: T("ATQ", "ATK"), val: avg([14, 20, 23, 26, 27]) },
    { lbl: "DRI",           val: avg([17, 18, 19]) },
    { lbl: "PAS",           val: avg([21, 22]) },
    { lbl: T("FIS", "PHY"), val: avg([33, 34, 35, 36, 37, 38, 39]) },
    { lbl: "DEF",           val: avg([15, 24, 25]) },
    { lbl: T("GL", "GK"),   val: avg([16, 28, 29, 30, 31]) },
  ];
}

function renderStatsRadar(stats) {
  const svg = document.getElementById("stats-radar-svg");
  const leg = document.getElementById("radar-legend");
  if (!svg || !stats) return;
  const axes = computeRadarAxes(stats);
  const cx = 120, cy = 115, maxR = 82;
  const pt = (i, frac) => {
    const a = (-Math.PI / 2) + (i * Math.PI * 2 / 6);
    return [cx + Math.cos(a) * maxR * frac, cy + Math.sin(a) * maxR * frac];
  };
  const rings = [0.25, 0.5, 0.75, 1.0].map(f => {
    const pts = axes.map((_, i) => pt(i, f).map(n => n.toFixed(1)).join(",")).join(" ");
    return `<polygon points="${pts}" fill="none" stroke="rgba(148,163,184,0.18)" stroke-width="1"/>`;
  }).join("");
  const spokes = axes.map((_, i) => {
    const [x, y] = pt(i, 1);
    return `<line x1="${cx}" y1="${cy}" x2="${x.toFixed(1)}" y2="${y.toFixed(1)}" stroke="rgba(148,163,184,0.16)" stroke-width="1"/>`;
  }).join("");
  const polyPts = axes.map((ax, i) => pt(i, Math.max(0.15, (ax.val - 30) / 70)).map(n => n.toFixed(1)).join(",")).join(" ");
  const dots = axes.map((ax, i) => {
    const [x, y] = pt(i, Math.max(0.15, (ax.val - 30) / 70));
    return `<circle cx="${x.toFixed(1)}" cy="${y.toFixed(1)}" r="3.5" fill="#60a5fa" stroke="#0b111e" stroke-width="1.5"/>`;
  }).join("");
  const labels = axes.map((ax, i) => {
    const [x, y] = pt(i, 1.19);
    return `<text x="${x.toFixed(1)}" y="${(y + 4).toFixed(1)}" text-anchor="middle" fill="#94a3b8" font-size="10.5" font-weight="700">${escapeHtml(ax.lbl)}</text>`;
  }).join("");

  svg.innerHTML = `${rings}${spokes}
    <polygon points="${polyPts}" fill="rgba(59,130,246,0.28)" stroke="#3b82f6" stroke-width="2"/>
    ${dots}${labels}`;

  leg.innerHTML = axes.map(ax => `<div style="background:#0b111e; border:1px solid var(--border); border-radius:6px; padding:4px 6px;">
      <div style="color:var(--muted); font-size:10px; font-weight:700;">${escapeHtml(ax.lbl)}</div>
      <div class="num" style="font-weight:800; font-size:13px;">${ax.val}</div>
    </div>`).join("");
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
      return `<div class="stat-row" data-stat-row="${s.fid}">
        <span style="font-size:12px; color:var(--subtext);">${escapeHtml(lbl)}</span>
        <input type="range" min="40" max="99" value="${val}" data-stat-fid="${s.fid}" />
        <span class="ovr-badge ${ovrClass(val)}" data-stat-badge="${s.fid}" style="min-width:36px; height:22px; font-size:11.5px;">${val}</span>
        <div style="display:flex; gap:3px;">
          <button class="btn btn-xs" type="button" data-stat-dec="${s.fid}">-</button>
          <button class="btn btn-xs" type="button" data-stat-inc="${s.fid}">+</button>
        </div>
      </div>`;
    }).join("");
    return `<div class="card"><div class="card-title">${escapeHtml(g.title)}</div><div>${rows}</div></div>`;
  }).join("");
}

// In-place stat update: preserves slider/button keyboard focus & zero DOM flicker!
async function onStatChange(fid, val) {
  if (!api()) return;
  const clamped = Math.max(40, Math.min(99, Number(val)));
  const res = await api().set_player_stat(Number(fid), clamped);
  if (res && res.ok) {
    const ovrChanged = CURRENT_PLAYER && res.ovr !== CURRENT_PLAYER.ovr;
    CURRENT_PLAYER = res;
    const newVal = res.stats[String(fid)] || clamped;
    const inp = document.querySelector(`input[data-stat-fid="${fid}"]`);
    if (inp) inp.value = String(newVal);
    const badge = document.querySelector(`[data-stat-badge="${fid}"]`);
    if (badge) { badge.textContent = String(newVal); badge.className = `ovr-badge ${ovrClass(newVal)}`; }
    updateHeroHeader(res);
    renderStatsRadar(res.stats);
    renderPositionPitch(res);
    renderPositionLab(res);
    if (ovrChanged) pulseOvrShield();
  } else {
    toast(fmtCodeMsg(res), "err");
  }
}

async function presetStats(val) {
  if (!api() || !META || !CURRENT_PLAYER) return toast(tKey("no_player"), "err");
  const map = {};
  const isGK = CURRENT_PLAYER.reg_pos === 0;
  META.stats.forEach(s => { if (isGK || s.group !== "gk") map[String(s.fid)] = val; });
  const res = await api().set_player_stats_batch(map);
  if (res && res.ok) {
    CURRENT_PLAYER = res;
    renderPlayerDetails(res);
    pulseOvrShield();
    toast(T(`Atributos definidos para ${val}`, `Attributes set to ${val}`));
  } else toast(fmtCodeMsg(res), "err");
}

async function shiftAllStats(delta) {
  if (!api() || !META || !CURRENT_PLAYER) return toast(tKey("no_player"), "err");
  const map = {};
  const isGK = CURRENT_PLAYER.reg_pos === 0;
  META.stats.forEach(s => {
    if (isGK || s.group !== "gk") {
      const cur = CURRENT_PLAYER.stats[String(s.fid)] || 40;
      map[String(s.fid)] = Math.max(40, Math.min(99, cur + delta));
    }
  });
  const res = await api().set_player_stats_batch(map);
  if (res && res.ok) {
    CURRENT_PLAYER = res;
    renderPlayerDetails(res);
    pulseOvrShield();
    toast(T(`+${delta} aplicado em todos os atributos`, `+${delta} applied to all attributes`));
  } else toast(fmtCodeMsg(res), "err");
}

// -------------------------------------------------------------------
// INTERACTIVE TACTICAL MINI-PITCH + STABLE POSITION LAB
// -------------------------------------------------------------------
const PITCH_COORDS = {
  12: [110, 34], 11: [110, 74], 9: [44, 62], 10: [176, 62],
  8: [110, 114], 6: [40, 146], 5: [110, 154], 7: [180, 146],
  4: [110, 194], 2: [40, 232], 1: [110, 240], 3: [180, 232], 0: [110, 286],
};

function renderPositionPitch(d) {
  const svg = document.getElementById("poslab-pitch-svg");
  if (!svg || !d || !d.positions) return;
  const pitchLines = `
    <rect x="8" y="8" width="204" height="304" rx="6" fill="none" stroke="rgba(16,185,129,0.32)" stroke-width="1.4"/>
    <line x1="8" y1="160" x2="212" y2="160" stroke="rgba(16,185,129,0.28)" stroke-width="1.2"/>
    <circle cx="110" cy="160" r="28" fill="none" stroke="rgba(16,185,129,0.28)" stroke-width="1.2"/>
    <rect x="56" y="8" width="108" height="44" fill="none" stroke="rgba(16,185,129,0.25)" stroke-width="1.2"/>
    <rect x="56" y="268" width="108" height="44" fill="none" stroke="rgba(16,185,129,0.25)" stroke-width="1.2"/>
  `;
  const nodes = d.positions.map(pos => {
    const [x, y] = PITCH_COORDS[pos.id] || [110, 160];
    const fill = pos.is_reg ? "#2563eb" : (pos.apt === 2 ? "#059669" : (pos.apt === 1 ? "#b45309" : "#1e293b"));
    const stroke = (HIGHLIGHTED_POS === pos.id) ? "#ffffff" : (pos.is_reg ? "#93c5fd" : (pos.apt === 2 ? "#34d399" : (pos.apt === 1 ? "#fbbf24" : "#475569")));
    const sw = (HIGHLIGHTED_POS === pos.id || pos.is_reg) ? 2.2 : 1.2;
    return `<g class="pitch-node" data-pitch-pos="${pos.id}" transform="translate(${x},${y})">
      <rect x="-23" y="-14" width="46" height="28" rx="6" fill="${fill}" stroke="${stroke}" stroke-width="${sw}"/>
      <text y="-2" text-anchor="middle" fill="#f8fafc" font-size="9" font-weight="800">${escapeHtml(posShort(pos.id))}</text>
      <text y="10" text-anchor="middle" fill="#e2e8f0" font-size="10.5" font-weight="900">${pos.ovr}</text>
    </g>`;
  }).join("");
  svg.innerHTML = pitchLines + nodes;
}

function renderPositionLab(d) {
  const tbody = document.getElementById("poslab-tbody");
  // Stable order: Registered Position pinned at top, then fixed tactical order (rows NEVER jump during +1/-1!)
  const ordered = [...d.positions].sort((a, b) => {
    if (a.is_reg !== b.is_reg) return b.is_reg - a.is_reg;
    return TACTICAL_ORDER.indexOf(a.id) - TACTICAL_ORDER.indexOf(b.id);
  });
  const priTag = T("PRINCIPAL", "PRIMARY");
  const matchBtn = T("Igualar Principal", "Match Primary");
  const setPriBtn = T("Tornar Principal", "Set Primary");
  tbody.innerHTML = ordered.map(pos => {
    const pMeta = META.positions[pos.id];
    const fullName = LANG === "en" ? pMeta.name_en : pMeta.name_pt;
    const regTag = pos.is_reg ? `<span class="pill-ok" style="margin-left:6px;">${escapeHtml(priTag)}</span>` : "";
    const hl = HIGHLIGHTED_POS === pos.id ? " row-highlight" : "";
    return `<tr class="${hl}" data-pos-row="${pos.id}">
      <td>
        <span class="pos-chip ${posClass(pos.id)}">${escapeHtml(posShort(pos.id))}</span>
        <span style="margin-left:8px; font-weight:600;">${escapeHtml(fullName)}</span>${regTag}
      </td>
      <td class="ctr-col"><span class="ovr-badge ${ovrClass(pos.ovr)}">${pos.ovr}</span></td>
      <td class="ctr-col">
        <div class="seg">
          <button type="button" class="${pos.apt === 0 ? "active" : ""}" data-pos-apt="${pos.id}" data-grade="0">C</button>
          <button type="button" class="${pos.apt === 1 ? "active g-b" : ""}" data-pos-apt="${pos.id}" data-grade="1">B</button>
          <button type="button" class="${pos.apt === 2 ? "active g-a" : ""}" data-pos-apt="${pos.id}" data-grade="2">A</button>
        </div>
      </td>
      <td>
        <div style="display:flex; gap:6px;">
          <button class="btn btn-xs" type="button" data-pos-step="${pos.id}" data-delta="-1">-1 OVR</button>
          <button class="btn btn-xs btn-primary" type="button" data-pos-step="${pos.id}" data-delta="1">+1 OVR</button>
        </div>
      </td>
      <td class="act-col">
        <div style="display:flex; gap:6px; justify-content:flex-end;">
          ${pos.is_reg ? "" : `<button class="btn btn-xs btn-gold" type="button" data-pos-match="${pos.id}">${escapeHtml(matchBtn)}</button>`}
          ${pos.is_reg ? "" : `<button class="btn btn-xs" type="button" data-pos-reg="${pos.id}">${escapeHtml(setPriBtn)}</button>`}
        </div>
      </td>
    </tr>`;
  }).join("");
}

function renderSkillsAndStyle(d) {
  document.getElementById("p-playing-style").value = String(d.playing_style);
  let skCount = 0;
  document.getElementById("p-skills-grid").innerHTML = META.skills.map(s => {
    const on = !!d.skills[s.key];
    if (on) skCount++;
    return `<button class="skill-chip ${on ? "on" : ""}" type="button" data-skill-key="${escapeHtml(s.key)}" data-skill-next="${on ? "0" : "1"}">${escapeHtml(LANG === "en" ? s.en : s.pt)}</button>`;
  }).join("");
  const skEl = document.getElementById("skills-count");
  skEl.textContent = skCount > 10
    ? `${skCount} / 10 (${T("acima do limite padrão", "above default cap")})`
    : `${skCount} / 10`;
  skEl.style.color = skCount > 10 ? "var(--gold)" : "var(--green)";

  let aiCount = 0;
  document.getElementById("p-comai-grid").innerHTML = META.com_ai.map(s => {
    const on = !!d.com_ai[s.key];
    if (on) aiCount++;
    return `<button class="skill-chip ${on ? "on" : ""}" type="button" data-skill-key="${escapeHtml(s.key)}" data-skill-next="${on ? "0" : "1"}">${escapeHtml(LANG === "en" ? s.en : s.pt)}</button>`;
  }).join("");
  document.getElementById("comai-count").textContent = `${aiCount} ${T("ativos", "active")}`;
}

// -------------------------------------------------------------------
// SORT HELPERS
// -------------------------------------------------------------------
function updateSortIndicators(tableId, attrName, state) {
  document.querySelectorAll(`#${tableId} thead th[${attrName}]`).forEach(th => {
    const col = th.getAttribute(attrName);
    const arrowEl = th.querySelector(".sort-arrow");
    if (col === state.col) {
      th.classList.add("sorted");
      if (arrowEl) arrowEl.textContent = state.asc ? "▲" : "▼";
    } else {
      th.classList.remove("sorted");
      if (arrowEl) arrowEl.textContent = "";
    }
  });
}

function safeCompare(a, b, col, dir) {
  const va = a[col], vb = b[col];
  if (typeof va === "string" || typeof vb === "string") {
    return String(va || "").localeCompare(String(vb || "")) * dir;
  }
  return ((Number(va) || 0) - (Number(vb) || 0)) * dir;
}

// -------------------------------------------------------------------
// SQUAD ROSTER + KPI STRIP + FILTER COUNTS + PER-ROW QUICK ACTIONS
// -------------------------------------------------------------------
async function loadSquad() {
  if (!api()) return;
  const res = await api().get_squad_roster();
  if (!res || !res.ok) {
    SQUAD_DATA = [];
    SQUAD_CLUB = "";
    renderSquadTable();
    return;
  }
  SQUAD_DATA = res.players || [];
  SQUAD_CLUB = res.club || "";
  renderSquadTable();
}

function renderSquadTable() {
  document.getElementById("squad-title").textContent =
    `${tKey("nav_squad")}${SQUAD_CLUB ? ` - ${SQUAD_CLUB}` : ""} (${SQUAD_DATA.length})`;

  const n = SQUAD_DATA.length;
  const expC = SQUAD_DATA.filter(p => p.is_expiring).length;
  const injC = SQUAD_DATA.filter(p => p.injury > 0).length;
  const u23C = SQUAD_DATA.filter(p => p.age <= 23).length;
  document.getElementById("cnt-all").textContent = String(n);
  document.getElementById("cnt-exp").textContent = String(expC);
  document.getElementById("cnt-inj").textContent = String(injC);
  document.getElementById("cnt-u23").textContent = String(u23C);

  document.getElementById("squad-kpi-count").textContent = String(n);
  if (n > 0) {
    document.getElementById("squad-kpi-ovr").textContent = (SQUAD_DATA.reduce((s, p) => s + p.ovr, 0) / n).toFixed(1);
    document.getElementById("squad-kpi-age").textContent = `${(SQUAD_DATA.reduce((s, p) => s + p.age, 0) / n).toFixed(1)}${T("a", "y")}`;
    document.getElementById("squad-kpi-alerts").textContent = `${injC} / ${expC}`;
  } else {
    document.getElementById("squad-kpi-ovr").textContent = "--";
    document.getElementById("squad-kpi-age").textContent = "--";
    document.getElementById("squad-kpi-alerts").textContent = "0 / 0";
  }

  updateSortIndicators("squad-table", "data-sort", SQUAD_SORT);

  const q = (document.getElementById("squad-search").value || "").toLowerCase().trim();
  let rows = SQUAD_DATA.filter(p => {
    if (SQUAD_FILTER === "exp" && !p.is_expiring) return false;
    if (SQUAD_FILTER === "inj" && p.injury <= 0) return false;
    if (SQUAD_FILTER === "u23" && p.age > 23) return false;
    if (q && !p.name.toLowerCase().includes(q)) return false;
    return true;
  });
  rows.sort((a, b) => safeCompare(a, b, SQUAD_SORT.col, SQUAD_SORT.asc ? 1 : -1));

  const tbody = document.getElementById("squad-tbody");
  if (!rows.length) {
    const msg = SQUAD_DATA.length ? T("Nenhum jogador encontrado com este filtro.", "No players match this filter.") : tKey("empty_squad");
    tbody.innerHTML = `<tr><td colspan="10" class="empty-state">${escapeHtml(msg)}</td></tr>`;
    return;
  }
  const ageUnit = T("a", "y");
  const healLbl = T("Curar", "Heal");
  tbody.innerHTML = rows.map(p => {
    const arr = ARROW_ICONS[p.arrow] || ARROW_ICONS[2];
    const quickBtn = p.injury > 0
      ? `<button class="btn btn-xs btn-green" type="button" data-sq-heal="${p.addr}">${escapeHtml(healLbl)}</button>`
      : (p.stamina < 100 ? `<button class="btn btn-xs btn-green" type="button" data-sq-stam="${p.addr}">100%</button>` : "");
    const isCur = p.addr === LAST_PLAYER_ADDR ? " row-highlight" : "";
    return `<tr class="clickable${isCur}" data-open-player="${p.addr}">
      <td class="ctr-col"><span class="ovr-badge ${ovrClass(p.ovr)}">${p.ovr}</span></td>
      <td class="ctr-col"><span class="pos-chip ${posClass(p.pos)}">${escapeHtml(posShort(p.pos))}</span></td>
      <td style="font-weight:600;">${escapeHtml(p.name)}</td>
      <td class="num-col">${p.age}${ageUnit}</td>
      <td class="num-col" style="color:${p.is_expiring ? "var(--gold)" : "var(--subtext)"};">${p.exp_y || "-"}</td>
      <td class="num-col">${escapeHtml(fmtMoney(p.salary_raw))}</td>
      <td class="ctr-col" style="font-size:15px; color:${arr.col}; font-weight:800;">${arr.ico}</td>
      <td>
        <div class="stam-bar-wrap">
          <div class="stam-bar"><div class="stam-fill" style="width:${p.stamina}%; background:${stamColor(p.stamina)};"></div></div>
          <span style="font-size:11.5px; color:var(--subtext);">${p.stamina}%</span>
        </div>
      </td>
      <td class="ctr-col">${p.injury > 0 ? `<span class="pill-inj">${p.injury}d</span>` : `<span class="pill-ok">OK</span>`}</td>
      <td class="act-col">${quickBtn}</td>
    </tr>`;
  }).join("");
}

async function openPlayerInStudio(addr) {
  if (!api()) return;
  const d = await api().select_player(addr);
  switchView("player", true);
  if (d && d.ok) {
    CURRENT_PLAYER = d;
    LAST_PLAYER_ADDR = d.addr;
    HIGHLIGHTED_POS = null;
    showPlayerEmptyState(false);
    renderPlayerDetails(d);
  }
}

async function runSquadCmd(mode) {
  if (!api() || IS_SQUAD_BUSY) return;
  IS_SQUAD_BUSY = true;
  try {
    const res = await api().run_squad_commander(mode);
    if (res && res.ok) {
      toast(T(`Aplicado a ${res.count} atletas do elenco!`, `Applied to ${res.count} squad players!`));
      if (CURRENT_VIEW === "squad") loadSquad();
      if (CURRENT_VIEW === "player") loadPlayerDetails();
    } else {
      toast(fmtCodeMsg(res), "err");
    }
  } finally {
    IS_SQUAD_BUSY = false;
  }
}

// -------------------------------------------------------------------
// GLOBAL 30K+ SEARCH (Contract Expiry + Target Scout Selector)
// -------------------------------------------------------------------
async function doSearch() {
  if (!api() || IS_SEARCHING) return;
  IS_SEARCHING = true;
  const btn = document.getElementById("btn-do-search");
  btn.disabled = true;
  const q = document.getElementById("srch-query").value;
  const pos = Number(document.getElementById("srch-pos").value);
  const ovr = Number(document.getElementById("srch-ovr").value || 0);
  const age = Number(document.getElementById("srch-age").value || 99);
  const free = document.getElementById("srch-free").checked;
  const tbody = document.getElementById("search-tbody");
  tbody.innerHTML = `<tr><td colspan="8" class="empty-state">${escapeHtml(T("Pesquisando base...", "Searching database..."))}</td></tr>`;
  try {
    const res = await api().search_players(q, pos, ovr, age, free);
    if (!res || !res.ok) {
      SEARCH_DATA = [];
      SEARCH_TOTAL = 0;
      toast(fmtCodeMsg(res), "err");
    } else {
      SEARCH_DATA = res.players || [];
      SEARCH_TOTAL = res.total || 0;
    }
    renderSearchTable();
  } finally {
    IS_SEARCHING = false;
    btn.disabled = false;
  }
}

function renderSearchTable() {
  updateSortIndicators("search-table", "data-ssort", SEARCH_SORT);
  const tbody = document.getElementById("search-tbody");
  const sub = document.getElementById("srch-sub");
  if (!SEARCH_DATA.length) {
    sub.textContent = tKey("srch_sub");
    tbody.innerHTML = `<tr><td colspan="8" class="empty-state">${escapeHtml(tKey("empty_search"))}</td></tr>`;
    return;
  }
  sub.textContent = T(
    `Mostrando ${SEARCH_DATA.length} de ${SEARCH_TOTAL} atletas encontrados.`,
    `Showing ${SEARCH_DATA.length} of ${SEARCH_TOTAL} matching players.`
  );
  const rows = [...SEARCH_DATA];
  rows.sort((a, b) => safeCompare(a, b, SEARCH_SORT.col, SEARCH_SORT.asc ? 1 : -1));
  const ageUnit = T("a", "y");
  const editLbl = T("Editar", "Edit");
  const addLbl = T("+ Lista", "+ List");
  tbody.innerHTML = rows.map(p => `<tr>
    <td class="ctr-col"><span class="ovr-badge ${ovrClass(p.ovr)}">${p.ovr}</span></td>
    <td class="ctr-col"><span class="pos-chip ${posClass(p.pos)}">${escapeHtml(posShort(p.pos))}</span></td>
    <td style="font-weight:600;">${escapeHtml(p.name)}</td>
    <td class="num-col">${p.age}${ageUnit}</td>
    <td style="color:var(--subtext);">${escapeHtml(fmtClub(p.club))}</td>
    <td class="num-col" style="color:var(--subtext);">${p.exp_y || "-"}</td>
    <td class="num-col">${escapeHtml(fmtMoney(p.salary_raw))}</td>
    <td class="act-col">
      <div style="display:flex; gap:6px; justify-content:flex-end;">
        <button class="btn btn-xs btn-primary" type="button" data-open-player="${p.addr}">${escapeHtml(editLbl)}</button>
        <button class="btn btn-xs btn-green" type="button" data-add-scout-addr="${p.addr}">${escapeHtml(addLbl)}</button>
      </div>
    </td>
  </tr>`).join("");
}

// -------------------------------------------------------------------
// SCOUTS & SHORTLIST (Live Capacity Counters + Clear List)
// -------------------------------------------------------------------
async function loadScouts() {
  if (!api()) return;
  const res = await api().get_scout_lists();
  if (!res || !res.ok) {
    SCOUT_LISTS_DATA = [];
    renderScoutTable();
    return;
  }
  SCOUT_LISTS_DATA = res.lists || [];
  SCOUT_LISTS_DATA.forEach(sl => {
    const el = document.getElementById(`sc-cnt-${sl.slot}`);
    if (el) el.textContent = `${sl.entries.length}/${sl.cap}`;
  });
  renderScoutTable();
}

function renderScoutTable() {
  const sl = SCOUT_LISTS_DATA.find(x => x.slot === ACTIVE_SCOUT_SLOT);
  const tbody = document.getElementById("scout-tbody");
  if (!sl || !sl.entries.length) {
    const msg = SCOUT_LISTS_DATA.length ? T("Lista vazia.", "List is empty.") : tKey("empty_scouts");
    tbody.innerHTML = `<tr><td colspan="6" class="empty-state">${escapeHtml(msg)}</td></tr>`;
    return;
  }
  const ageUnit = T("a", "y");
  const editLbl = T("Editar", "Edit");
  const remLbl = T("Remover", "Remove");
  tbody.innerHTML = sl.entries.map(e => `<tr>
    <td class="ctr-col"><span class="ovr-badge ${ovrClass(e.ovr)}">${e.ovr}</span></td>
    <td class="ctr-col"><span class="pos-chip ${posClass(e.pos)}">${escapeHtml(posShort(e.pos))}</span></td>
    <td style="font-weight:600;">${escapeHtml(e.name)}</td>
    <td class="num-col">${e.age}${ageUnit}</td>
    <td style="color:var(--subtext);">${escapeHtml(fmtClub(e.club))}</td>
    <td class="act-col">
      <div style="display:flex; gap:6px; justify-content:flex-end;">
        <button class="btn btn-xs btn-primary" type="button" data-open-player="${e.addr}">${escapeHtml(editLbl)}</button>
        <button class="btn btn-xs btn-red" type="button" data-rem-scout-slot="${ACTIVE_SCOUT_SLOT}" data-rem-scout-handle="${escapeHtml(e.handle)}">${escapeHtml(remLbl)}</button>
      </div>
    </td>
  </tr>`).join("");
}

async function addPlayerToScout(slotNum, addr = null) {
  if (!api()) return;
  const res = await api().add_to_scout_list(slotNum, addr);
  toast(fmtCodeMsg(res), res && res.ok ? "ok" : "err");
  if (CURRENT_VIEW === "scouts") loadScouts();
}

// -------------------------------------------------------------------
// FINANCES (4 KPIs + Accurate Deficit Bar)
// -------------------------------------------------------------------
async function loadFinances() {
  if (!api()) return;
  const res = await api().get_finances();
  if (!res || !res.ok) return;
  LAST_FINANCES = res;
  renderFinances(res);
}

function renderFinances(res) {
  document.getElementById("fin-kpi-trf").textContent = fmtMoney(res.transfer_raw);
  const availEl = document.getElementById("fin-kpi-avail");
  availEl.textContent = fmtMoney(res.salary_avail_raw);
  availEl.style.color = res.salary_avail_raw < 0 ? "var(--red)" : "var(--gold)";
  document.getElementById("fin-kpi-wages").textContent = fmtMoney(res.squad_wages_raw);
  document.getElementById("fin-kpi-gross").textContent = fmtMoney(res.salary_gross_raw);

  document.getElementById("fin-transfer-inp").value = fmtMoney(res.transfer_raw);
  document.getElementById("fin-salary-inp").value = fmtMoney(res.salary_avail_raw);
  document.getElementById("fin-wage-bill").textContent =
    `${T("Folha anual do elenco", "Squad annual wage bill")}: ${fmtMoney(res.squad_wages_raw)}`;

  const used = Math.max(0, res.squad_wages_raw || 0);
  const rawAvail = res.salary_avail_raw || 0;
  if (rawAvail < 0) {
    document.getElementById("fin-bar-used").style.width = "100%";
    document.getElementById("fin-bar-free").style.width = "0%";
    document.getElementById("fin-bar-lbl-used").textContent = `${T("Folha ativa", "Active wages")}: ${fmtMoney(used)}`;
    document.getElementById("fin-bar-lbl-free").innerHTML = `<span style="color:var(--red); font-weight:700;">${escapeHtml(T("Déficit salarial", "Salary deficit"))}: ${escapeHtml(fmtMoney(rawAvail))}</span>`;
  } else {
    const total = Math.max(1, used + rawAvail);
    const usedPct = Math.min(100, Math.round((used / total) * 100));
    document.getElementById("fin-bar-used").style.width = `${usedPct}%`;
    document.getElementById("fin-bar-free").style.width = `${100 - usedPct}%`;
    document.getElementById("fin-bar-lbl-used").textContent = `${T("Folha ativa", "Active wages")}: ${usedPct}% (${fmtMoney(used)})`;
    document.getElementById("fin-bar-lbl-free").textContent = `${T("Disponível", "Available")}: ${100 - usedPct}% (${fmtMoney(rawAvail)})`;
  }
}

async function quickFinance(kind, deltaRaw) {
  if (!api()) return;
  const res = await api().add_finance_delta(kind, deltaRaw);
  if (res && res.ok) {
    LAST_FINANCES = res;
    toast(T("Orçamento atualizado!", "Budget updated!"));
    pollStatus();
    if (CURRENT_VIEW === "finances") renderFinances(res);
  } else toast(fmtCodeMsg(res), "err");
}

// -------------------------------------------------------------------
// DARK JSON MODAL & FULL KEYBOARD COMMAND PALETTE (Ctrl+K)
// -------------------------------------------------------------------
function openJsonModal(mode, initialText = "", playerName = "") {
  JSON_MODAL_MODE = mode;
  const modal = document.getElementById("json-modal");
  const title = document.getElementById("json-modal-title");
  const ta = document.getElementById("json-modal-text");
  const actBtn = document.getElementById("btn-json-action");
  if (mode === "export") {
    title.textContent = T(`Exportar JSON - ${playerName}`, `Export JSON - ${playerName}`);
    ta.value = initialText;
    ta.readOnly = true;
    actBtn.textContent = T("Copiar JSON", "Copy JSON");
  } else {
    title.textContent = T("Importar Backup JSON do Jogador", "Import Player JSON Backup");
    ta.value = "";
    ta.readOnly = false;
    actBtn.textContent = T("Aplicar Backup", "Apply Backup");
  }
  modal.classList.add("open");
  ta.focus();
  if (mode === "export") ta.select();
}

function closeAllModals() {
  document.getElementById("json-modal").classList.remove("open");
  document.getElementById("cmd-modal").classList.remove("open");
}

function getCommands() {
  const cView = T("TELA", "VIEW");
  const cSquad = T("ELENCO", "SQUAD");
  const cAct = T("AÇÃO", "ACTION");
  return [
    { label: tKey("nav_overview"), cat: cView, run: () => switchView("dashboard") },
    { label: tKey("nav_squad"), cat: cView, run: () => switchView("squad") },
    { label: tKey("nav_player"), cat: cView, run: () => switchView("player") },
    { label: tKey("nav_search"), cat: cView, run: () => switchView("search") },
    { label: tKey("nav_scouts"), cat: cView, run: () => switchView("scouts") },
    { label: tKey("nav_finances"), cat: cView, run: () => switchView("finances") },
    { label: tKey("nav_match_live"), cat: cView, run: () => switchView("match") },
    { label: tKey("nav_training_bal"), cat: cView, run: () => switchView("training") },
    { label: tKey("cmd_stam_squad"), cat: cSquad, run: () => runSquadCmd("stamina") },
    { label: tKey("cmd_arrow_squad"), cat: cSquad, run: () => runSquadCmd("arrow") },
    { label: tKey("cmd_heal_squad"), cat: cSquad, run: () => runSquadCmd("heal") },
    { label: tKey("cmd_contract_squad"), cat: cSquad, run: () => runSquadCmd("contract") },
    { label: tKey("cmd_ovr1_squad"), cat: cSquad, run: () => runSquadCmd("ovr1") },
    { label: tKey("btn_undo"), cat: cAct, run: () => document.getElementById("btn-undo").click() },
    { label: T("Alternar Idioma (PT-BR / EN)", "Toggle Language (PT-BR / EN)"), cat: cAct, run: () => document.getElementById("btn-lang").click() },
    { label: T("Alternar Moeda (€ / R$ / $ / £)", "Cycle Currency (€ / R$ / $ / £)"), cat: cAct, run: () => document.getElementById("btn-currency").click() },
  ];
}

function openCmdPalette() {
  const modal = document.getElementById("cmd-modal");
  const inp = document.getElementById("cmd-input");
  inp.value = "";
  CMD_SELECTED = 0;
  renderCmdList("");
  modal.classList.add("open");
  inp.focus();
}

function renderCmdList(query) {
  const q = (query || "").toLowerCase().trim();
  const list = getCommands().filter(c => !q || c.label.toLowerCase().includes(q) || c.cat.toLowerCase().includes(q));
  if (CMD_SELECTED >= list.length) CMD_SELECTED = Math.max(0, list.length - 1);
  const box = document.getElementById("cmd-list");
  if (!list.length) {
    box.innerHTML = `<div class="empty-state" style="padding:18px;">${escapeHtml(T("Nenhum comando encontrado.", "No matching commands."))}</div>`;
    box._items = [];
    return;
  }
  box.innerHTML = list.map((c, idx) =>
    `<button class="cmd-item${idx === CMD_SELECTED ? " selected" : ""}" type="button" data-cmd-idx="${idx}">
      <span>${escapeHtml(c.label)}</span><span class="cmd-cat">${escapeHtml(c.cat)}</span>
    </button>`
  ).join("");
  box._items = list;
}

// -------------------------------------------------------------------
// BOOTSTRAP & DELEGATED EVENTS
// -------------------------------------------------------------------
async function bootstrap() {
  META = await api().get_metadata();

  document.getElementById("bio-nat").innerHTML = META.nationalities.map(n => `<option value="${n.id}">${escapeHtml(n.name)}</option>`).join("");
  document.getElementById("p-playing-style").innerHTML = META.playing_styles.map(s => `<option value="${s.id}">${escapeHtml(s.name)}</option>`).join("");

  applyFont(CUR_FONT);
  applyLanguage();

  // Sidebar, Back button & tabs
  document.querySelectorAll(".nav-btn").forEach(b => b.addEventListener("click", () => switchView(b.dataset.view)));
  document.getElementById("btn-nav-back").addEventListener("click", goBack);
  document.getElementById("top-player-pill").addEventListener("click", () => switchView("player"));
  document.querySelectorAll("#player-subtabs .tab-btn").forEach(b => b.addEventListener("click", () => switchPlayerTab(b.dataset.ptab)));
  document.getElementById("btn-open-cmd").addEventListener("click", openCmdPalette);

  // Global keyboard & mouse shortcuts (Ctrl+K, Ctrl+Z, Alt+Left, Mouse Back, Escape)
  document.addEventListener("keydown", e => {
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "k") {
      e.preventDefault();
      openCmdPalette();
    } else if (e.altKey && e.key === "ArrowLeft") {
      e.preventDefault();
      goBack();
    } else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "z" && !["INPUT", "TEXTAREA"].includes(document.activeElement.tagName)) {
      e.preventDefault();
      const uBtn = document.getElementById("btn-undo");
      if (!uBtn.disabled) uBtn.click();
    } else if (e.key === "Escape") {
      closeAllModals();
    }
  });
  document.addEventListener("mouseup", e => {
    if (e.button === 3) { e.preventDefault(); goBack(); }
  });

  ["json-modal", "cmd-modal"].forEach(id => {
    document.getElementById(id).addEventListener("click", e => {
      if (e.target.id === id) closeAllModals();
    });
  });

  // Inline Dashboard Budget Edit (zero modal, zero screen jump!)
  const bindInlineBudget = (kind, btnId, actId, editId, inpId, okId, cancelId) => {
    const closeInline = () => {
      document.getElementById(editId).style.display = "none";
      document.getElementById(actId).style.display = "flex";
    };
    const saveInline = async () => {
      if (!api()) return;
      const val = document.getElementById(inpId).value;
      const r = kind === "transfer"
        ? await api().set_finances(val, "")
        : await api().set_finances("", val);
      if (r && r.ok) {
        LAST_FINANCES = r;
        toast(T("Orçamento atualizado!", "Budget updated!"));
        closeInline();
        pollStatus();
        if (CURRENT_VIEW === "finances") renderFinances(r);
      } else toast(fmtCodeMsg(r), "err");
    };
    document.getElementById(btnId).addEventListener("click", () => {
      const fin = LAST_STATUS && LAST_STATUS.finances;
      const inp = document.getElementById(inpId);
      inp.value = fin ? fmtMoney(kind === "transfer" ? fin.transfer_raw : fin.salary_avail_raw) : "";
      document.getElementById(actId).style.display = "none";
      document.getElementById(editId).style.display = "flex";
      inp.focus();
      inp.select();
    });
    document.getElementById(okId).addEventListener("click", saveInline);
    document.getElementById(cancelId).addEventListener("click", closeInline);
    document.getElementById(inpId).addEventListener("keydown", e => {
      if (e.key === "Enter") saveInline();
      else if (e.key === "Escape") closeInline();
    });
  };
  bindInlineBudget("transfer", "btn-inline-trf", "dash-trf-actions", "dash-trf-edit", "inp-inline-trf", "btn-inline-trf-ok", "btn-inline-trf-cancel");
  bindInlineBudget("salary", "btn-inline-sal", "dash-sal-actions", "dash-sal-edit", "inp-inline-sal", "btn-inline-sal-ok", "btn-inline-sal-cancel");

  // Command palette full ArrowUp/ArrowDown/Enter navigation
  const cmdInp = document.getElementById("cmd-input");
  cmdInp.addEventListener("input", e => { CMD_SELECTED = 0; renderCmdList(e.target.value); });
  cmdInp.addEventListener("keydown", e => {
    const box = document.getElementById("cmd-list");
    const items = box._items || [];
    if (!items.length) return;
    if (e.key === "ArrowDown") {
      e.preventDefault();
      CMD_SELECTED = (CMD_SELECTED + 1) % items.length;
      renderCmdList(cmdInp.value);
    } else if (e.key === "ArrowUp") {
      e.preventDefault();
      CMD_SELECTED = (CMD_SELECTED - 1 + items.length) % items.length;
      renderCmdList(cmdInp.value);
    } else if (e.key === "Enter") {
      e.preventDefault();
      const chosen = items[CMD_SELECTED] || items[0];
      if (chosen) { closeAllModals(); chosen.run(); }
    }
  });
  document.getElementById("cmd-list").addEventListener("click", e => {
    const btn = e.target.closest("[data-cmd-idx]");
    if (!btn) return;
    const box = document.getElementById("cmd-list");
    const item = box._items && box._items[Number(btn.dataset.cmdIdx)];
    if (item) { closeAllModals(); item.run(); }
  });

  // Dashboard Active Player quick actions
  document.getElementById("btn-dp-stam").addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().set_quick_condition(100, null, false, null);
    if (r && r.ok) { toast(T("Fôlego 100%!", "Stamina 100%!")); pollStatus(); }
    else toast(fmtCodeMsg(r), "err");
  });
  document.getElementById("btn-dp-arrow").addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().set_quick_condition(null, 4, false, null);
    if (r && r.ok) { toast(T("Seta Azul aplicada!", "Blue Arrow applied!")); pollStatus(); }
    else toast(fmtCodeMsg(r), "err");
  });
  document.getElementById("btn-dp-heal").addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().set_quick_condition(null, null, true, null);
    if (r && r.ok) { toast(T("Lesão curada!", "Injury healed!")); pollStatus(); }
    else toast(fmtCodeMsg(r), "err");
  });

  // Delegated Squad row & quick action clicks
  document.getElementById("squad-tbody").addEventListener("click", async e => {
    const healBtn = e.target.closest("[data-sq-heal]");
    if (healBtn && api()) {
      e.stopPropagation();
      const r = await api().set_quick_condition(null, null, true, Number(healBtn.dataset.sqHeal));
      toast(r && r.ok ? T("Lesão curada!", "Injury healed!") : fmtCodeMsg(r), r && r.ok ? "ok" : "err");
      return loadSquad();
    }
    const stamBtn = e.target.closest("[data-sq-stam]");
    if (stamBtn && api()) {
      e.stopPropagation();
      const r = await api().set_quick_condition(100, null, false, Number(stamBtn.dataset.sqStam));
      toast(r && r.ok ? T("Fôlego 100%!", "Stamina 100%!") : fmtCodeMsg(r), r && r.ok ? "ok" : "err");
      return loadSquad();
    }
    const row = e.target.closest("[data-open-player]");
    if (row) openPlayerInStudio(Number(row.dataset.openPlayer));
  });

  // Delegated Search clicks (uses target Scout List dropdown)
  document.getElementById("search-tbody").addEventListener("click", e => {
    const openBtn = e.target.closest("[data-open-player]");
    if (openBtn) return openPlayerInStudio(Number(openBtn.dataset.openPlayer));
    const addBtn = e.target.closest("[data-add-scout-addr]");
    if (addBtn) {
      const slot = Number(document.getElementById("srch-target-scout").value || 4);
      return addPlayerToScout(slot, Number(addBtn.dataset.addScoutAddr));
    }
  });

  // Delegated Scout clicks
  document.getElementById("scout-tbody").addEventListener("click", async e => {
    const openBtn = e.target.closest("[data-open-player]");
    if (openBtn) return openPlayerInStudio(Number(openBtn.dataset.openPlayer));
    const remBtn = e.target.closest("[data-rem-scout-handle]");
    if (remBtn && api()) {
      const res = await api().remove_from_scout_list(Number(remBtn.dataset.remScoutSlot), remBtn.dataset.remScoutHandle);
      toast(res && res.ok ? T("Removido da lista!", "Removed from list!") : fmtCodeMsg(res), res && res.ok ? "ok" : "err");
      loadScouts();
    }
  });

  // Player Hero quick buttons (-1 OVR, +1 OVR, + Favoritos)
  document.getElementById("btn-hero-ovr-m1").addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().step_player_ovr(-1);
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); pulseOvrShield(); toast(fmtCodeMsg(r)); }
    else toast(fmtCodeMsg(r), "err");
  });
  document.getElementById("btn-hero-ovr-p1").addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().step_player_ovr(1);
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); pulseOvrShield(); toast(fmtCodeMsg(r)); }
    else toast(fmtCodeMsg(r), "err");
  });
  document.getElementById("btn-hero-fav").addEventListener("click", () => addPlayerToScout(4));

  // Bio quick Rejuvenate
  document.getElementById("btn-bio-rejuv").addEventListener("click", async () => {
    if (!api() || !CURRENT_PLAYER) return;
    const targetAge = Math.max(15, (CURRENT_PLAYER.age || 25) - 3);
    const r = await api().update_player_bio({ age: targetAge });
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast(T(`Idade ajustada para ${targetAge}a!`, `Age set to ${targetAge}y!`)); }
    else toast(fmtCodeMsg(r), "err");
  });
  document.getElementById("btn-bio-age18").addEventListener("click", async () => {
    if (!api() || !CURRENT_PLAYER) return;
    const r = await api().update_player_bio({ age: 18 });
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast(T("Idade ajustada para 18a!", "Age set to 18y!")); }
    else toast(fmtCodeMsg(r), "err");
  });

  // Attributes delegated in-place slider & +/- handlers
  const statsBox = document.getElementById("stats-groups-container");
  statsBox.addEventListener("input", e => {
    const inp = e.target.closest("input[data-stat-fid]");
    if (!inp) return;
    const v = Number(inp.value);
    const badge = document.querySelector(`[data-stat-badge="${inp.dataset.statFid}"]`);
    if (badge) { badge.textContent = v; badge.className = `ovr-badge ${ovrClass(v)}`; }
    if (CURRENT_PLAYER && CURRENT_PLAYER.stats) {
      CURRENT_PLAYER.stats[String(inp.dataset.statFid)] = v;
      renderStatsRadar(CURRENT_PLAYER.stats);
    }
  });
  statsBox.addEventListener("change", e => {
    const inp = e.target.closest("input[data-stat-fid]");
    if (inp) onStatChange(Number(inp.dataset.statFid), Number(inp.value));
  });
  statsBox.addEventListener("click", e => {
    const dec = e.target.closest("button[data-stat-dec]");
    if (dec && CURRENT_PLAYER) {
      const fid = Number(dec.dataset.statDec);
      return onStatChange(fid, (CURRENT_PLAYER.stats[String(fid)] || 40) - 1);
    }
    const inc = e.target.closest("button[data-stat-inc]");
    if (inc && CURRENT_PLAYER) {
      const fid = Number(inc.dataset.statInc);
      return onStatChange(fid, (CURRENT_PLAYER.stats[String(fid)] || 40) + 1);
    }
  });

  // Tactical Pitch click -> highlight & scroll ONLY inner table container
  document.getElementById("poslab-pitch-svg").addEventListener("click", e => {
    const node = e.target.closest("[data-pitch-pos]");
    if (!node) return;
    HIGHLIGHTED_POS = Number(node.dataset.pitchPos);
    if (CURRENT_PLAYER) renderPositionPitch(CURRENT_PLAYER);
    document.querySelectorAll("#poslab-tbody tr").forEach(tr => tr.classList.toggle("row-highlight", Number(tr.dataset.posRow) === HIGHLIGHTED_POS));
    const targetRow = document.querySelector(`#poslab-tbody tr[data-pos-row="${HIGHLIGHTED_POS}"]`);
    const wrap = document.getElementById("poslab-scroll");
    if (targetRow && wrap) {
      wrap.scrollTo({ top: Math.max(0, targetRow.offsetTop - 90), behavior: "smooth" });
    }
  });

  // Position Lab delegated buttons
  document.getElementById("poslab-tbody").addEventListener("click", async e => {
    if (!api()) return;
    const aptBtn = e.target.closest("[data-pos-apt]");
    if (aptBtn) {
      const r = await api().set_position_aptitude(Number(aptBtn.dataset.posApt), Number(aptBtn.dataset.grade));
      if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); }
      else toast(fmtCodeMsg(r), "err");
      return;
    }
    const stepBtn = e.target.closest("[data-pos-step]");
    if (stepBtn) {
      HIGHLIGHTED_POS = Number(stepBtn.dataset.posStep);
      const r = await api().step_position_ovr(HIGHLIGHTED_POS, Number(stepBtn.dataset.delta));
      if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast(fmtCodeMsg(r), r.action_ok === false ? "err" : "ok"); }
      else toast(fmtCodeMsg(r), "err");
      return;
    }
    const matchBtn = e.target.closest("[data-pos-match]");
    if (matchBtn) {
      HIGHLIGHTED_POS = Number(matchBtn.dataset.posMatch);
      const r = await api().match_position_to_primary(HIGHLIGHTED_POS);
      if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast(fmtCodeMsg(r), r.action_ok === false ? "err" : "ok"); }
      else toast(fmtCodeMsg(r), "err");
      return;
    }
    const regBtn = e.target.closest("[data-pos-reg]");
    if (regBtn) {
      HIGHLIGHTED_POS = Number(regBtn.dataset.posReg);
      const r = await api().set_registered_position(HIGHLIGHTED_POS);
      if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); pulseOvrShield(); toast(T("Posição principal alterada!", "Primary position updated!")); }
      else toast(fmtCodeMsg(r), "err");
    }
  });

  // Skills & COM AI delegated toggles
  ["p-skills-grid", "p-comai-grid"].forEach(id => {
    document.getElementById(id).addEventListener("click", async e => {
      const btn = e.target.closest("[data-skill-key]");
      if (!btn || !api()) return;
      const r = await api().set_player_skill(btn.dataset.skillKey, btn.dataset.skillNext === "1");
      if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); }
      else toast(fmtCodeMsg(r), "err");
    });
  });

  // Topbar controls
  document.getElementById("conn-pill").addEventListener("click", async () => {
    if (!api()) return;
    const st = await api().try_attach();
    LAST_STATUS = st;
    renderStatus(st);
    toast(
      st.attached
        ? T(`Conectado a ${st.exe}`, `Connected to ${st.exe}`)
        : (st.access_denied ? T("Acesso negado: execute o Workbench como Administrador.", "Access denied: run Workbench as Administrator.") : T("PES 2021 não encontrado.", "PES 2021 not found.")),
      st.attached ? "ok" : "err"
    );
  });
  document.getElementById("btn-undo").addEventListener("click", async () => {
    if (!api()) return;
    const res = await api().undo_last();
    if (res && res.ok) {
      toast(T("Alteração desfeita!", "Edit undone!"));
      if (CURRENT_VIEW === "player") loadPlayerDetails();
      if (CURRENT_VIEW === "squad") loadSquad();
      pollStatus();
    } else toast(fmtCodeMsg(res), "err");
  });
  document.getElementById("sel-font").addEventListener("change", e => {
    applyFont(e.target.value);
    const label = e.target.options[e.target.selectedIndex].textContent.replace(/^Aa\s+/, "");
    toast(T(`Fonte alterada para ${label}`, `Font switched to ${label}`));
  });
  document.getElementById("btn-currency").addEventListener("click", () => {
    CUR_IDX = (CUR_IDX + 1) % CURRENCIES.length;
    try { localStorage.setItem("pes21_studio_cur", String(CUR_IDX)); } catch (e) {}
    applyFont(CUR_FONT);
  applyLanguage();
  });
  document.getElementById("btn-lang").addEventListener("click", () => {
    LANG = LANG === "pt" ? "en" : "pt";
    try { localStorage.setItem("pes21_studio_lang", LANG); } catch (e) {}
    applyFont(CUR_FONT);
  applyLanguage();
    toast(LANG === "pt" ? "Idioma alterado para Português (PT-BR)" : "Language switched to English (EN)");
  });

  // Dashboard
  document.getElementById("btn-dash-refresh").addEventListener("click", pollStatus);

  // Squad filters & sorting
  document.querySelectorAll("#squad-filters .pill-tab").forEach(b => b.addEventListener("click", () => {
    SQUAD_FILTER = b.dataset.filter;
    document.querySelectorAll("#squad-filters .pill-tab").forEach(x => x.classList.toggle("active", x === b));
    renderSquadTable();
  }));
  document.getElementById("squad-search").addEventListener("input", renderSquadTable);
  document.getElementById("btn-squad-refresh").addEventListener("click", loadSquad);
  document.querySelectorAll("#squad-table thead th[data-sort]").forEach(th => th.addEventListener("click", () => {
    const col = th.dataset.sort;
    if (SQUAD_SORT.col === col) SQUAD_SORT.asc = !SQUAD_SORT.asc;
    else { SQUAD_SORT.col = col; SQUAD_SORT.asc = (col === "name"); }
    renderSquadTable();
  }));

  // Search table sort, Clear & Enter
  document.querySelectorAll("#search-table thead th[data-ssort]").forEach(th => th.addEventListener("click", () => {
    const col = th.dataset.ssort;
    if (SEARCH_SORT.col === col) SEARCH_SORT.asc = !SEARCH_SORT.asc;
    else { SEARCH_SORT.col = col; SEARCH_SORT.asc = (col === "name" || col === "club"); }
    renderSearchTable();
  }));
  ["srch-query", "srch-ovr", "srch-age"].forEach(id => {
    document.getElementById(id).addEventListener("keydown", e => { if (e.key === "Enter") doSearch(); });
  });
  document.getElementById("btn-do-search").addEventListener("click", doSearch);
  document.getElementById("btn-clear-search").addEventListener("click", () => {
    document.getElementById("srch-query").value = "";
    document.getElementById("srch-pos").value = "-1";
    document.getElementById("srch-ovr").value = "75";
    document.getElementById("srch-age").value = "45";
    document.getElementById("srch-free").checked = false;
    SEARCH_DATA = [];
    SEARCH_TOTAL = 0;
    renderSearchTable();
  });

  // Player Condition & Bio
  document.getElementById("btn-p-stam100").addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().set_quick_condition(100, null, false, null);
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast(T("Fôlego 100%!", "Stamina 100%!")); }
    else toast(fmtCodeMsg(r), "err");
  });
  document.getElementById("btn-p-heal").addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().set_quick_condition(null, null, true, null);
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast(T("Lesão curada!", "Injury healed!")); }
    else toast(fmtCodeMsg(r), "err");
  });
  document.getElementById("inp-p-stam").addEventListener("input", e => {
    document.getElementById("lbl-p-stam").textContent = `${e.target.value}%`;
  });
  document.getElementById("inp-p-stam").addEventListener("change", async e => {
    if (!api()) return;
    const r = await api().set_quick_condition(Number(e.target.value), null, false, null);
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); }
    else toast(fmtCodeMsg(r), "err");
  });
  document.querySelectorAll("#p-arrow-group .arrow-btn").forEach(b => b.addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().set_quick_condition(null, Number(b.dataset.arrow), false, null);
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast(T("Seta de forma aplicada!", "Form arrow updated!")); }
    else toast(fmtCodeMsg(r), "err");
  }));
  document.querySelectorAll("#ptab-bio .form-grid input").forEach(inp => {
    inp.addEventListener("keydown", e => { if (e.key === "Enter") document.getElementById("btn-save-bio").click(); });
  });
  document.getElementById("btn-save-bio").addEventListener("click", async () => {
    if (!api()) return;
    const optNum = id => { const v = document.getElementById(id).value.trim(); return v === "" ? undefined : Number(v); };
    const r = await api().update_player_bio({
      name: document.getElementById("bio-name").value,
      shirt: document.getElementById("bio-shirt").value,
      nationality: optNum("bio-nat"),
      age: optNum("bio-age"),
      height: optNum("bio-height"),
      weight: optNum("bio-weight"),
      foot: optNum("bio-foot"),
      contract_yr: optNum("bio-contract"),
      salary_str: document.getElementById("bio-salary").value,
      market_str: document.getElementById("bio-market").value,
      injury: optNum("bio-injury"),
      wf_usage: optNum("bio-wfu"),
      wf_acc: optNum("bio-wfa"),
      form_cons: optNum("bio-form"),
      inj_res: optNum("bio-injres"),
      transfer_listed: document.getElementById("bio-tlist").checked,
      loan_listed: document.getElementById("bio-llist").checked,
    });
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast(T("Dados biográficos salvos!", "Bio saved!")); }
    else toast(fmtCodeMsg(r), "err");
  });
  document.getElementById("p-playing-style").addEventListener("change", async e => {
    if (!api()) return;
    const r = await api().set_playing_style(Number(e.target.value));
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast(T("Estilo de jogo atualizado!", "Playing style updated!")); }
    else toast(fmtCodeMsg(r), "err");
  });
  document.getElementById("btn-save-role").addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().set_team_role(
      Number(document.getElementById("p-team-role").value),
      Number(document.getElementById("p-team-role-lvl").value)
    );
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast(T("Função de equipe salva!", "Team role saved!")); }
    else toast(fmtCodeMsg(r), "err");
  });

  // JSON Modal
  document.getElementById("btn-export-json").addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().export_player_json();
    if (r && r.ok) openJsonModal("export", r.json, r.name);
    else toast(fmtCodeMsg(r), "err");
  });
  document.getElementById("btn-import-json").addEventListener("click", () => {
    if (!CURRENT_PLAYER) return toast(tKey("no_player"), "err");
    openJsonModal("import");
  });
  document.getElementById("btn-json-close").addEventListener("click", closeAllModals);
  document.getElementById("btn-json-cancel").addEventListener("click", closeAllModals);
  document.getElementById("btn-json-action").addEventListener("click", async () => {
    const ta = document.getElementById("json-modal-text");
    if (JSON_MODAL_MODE === "export") {
      ta.select();
      try { await navigator.clipboard.writeText(ta.value); } catch (e) { document.execCommand("copy"); }
      toast(T("JSON copiado para a área de transferência!", "JSON copied to clipboard!"));
      closeAllModals();
    } else if (api()) {
      const r = await api().import_player_json(ta.value);
      if (r && r.ok) {
        CURRENT_PLAYER = r;
        renderPlayerDetails(r);
        pulseOvrShield();
        toast(T("Backup JSON importado!", "JSON backup imported!"));
        closeAllModals();
      } else toast(fmtCodeMsg(r), "err");
    }
  });

  // Scouts
  document.getElementById("btn-scouts-refresh").addEventListener("click", loadScouts);
  document.querySelectorAll("#scout-tabs .pill-tab").forEach(b => b.addEventListener("click", () => {
    ACTIVE_SCOUT_SLOT = Number(b.dataset.slot);
    document.querySelectorAll("#scout-tabs .pill-tab").forEach(x => x.classList.toggle("active", x === b));
    renderScoutTable();
  }));
  document.getElementById("btn-scout-add-cur").addEventListener("click", () => addPlayerToScout(ACTIVE_SCOUT_SLOT));
  let clearConfirmTimer = null;
  const clearBtn = document.getElementById("btn-scout-clear");
  clearBtn.addEventListener("click", async () => {
    if (!api()) return;
    if (!clearBtn.dataset.armed) {
      clearBtn.dataset.armed = "1";
      clearBtn.textContent = T("Confirmar Limpeza?", "Confirm Clear?");
      clearTimeout(clearConfirmTimer);
      clearConfirmTimer = setTimeout(() => {
        delete clearBtn.dataset.armed;
        clearBtn.textContent = tKey("btn_clear_list");
      }, 3000);
      return;
    }
    delete clearBtn.dataset.armed;
    clearTimeout(clearConfirmTimer);
    clearBtn.textContent = tKey("btn_clear_list");
    const r = await api().clear_scout_list(ACTIVE_SCOUT_SLOT);
    if (r && r.ok) { toast(T("Lista limpa!", "List cleared!")); loadScouts(); }
    else toast(fmtCodeMsg(r), "err");
  });

  // Finances
  document.getElementById("btn-fin-refresh").addEventListener("click", loadFinances);
  document.getElementById("btn-fin-save").addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().set_finances(
      document.getElementById("fin-transfer-inp").value,
      document.getElementById("fin-salary-inp").value
    );
    if (r && r.ok) { LAST_FINANCES = r; toast(T("Finanças atualizadas!", "Finances updated!")); renderFinances(r); pollStatus(); }
    else toast(fmtCodeMsg(r), "err");
  });

  // Match & Training / BAL
  document.getElementById("bal-focus-inp").addEventListener("input", () => {
    BAL_DIRTY = true;
    document.getElementById("bal-unsaved").style.display = "inline";
  });
  document.getElementById("bal-lock-chk").addEventListener("change", () => {
    BAL_DIRTY = true;
    document.getElementById("bal-unsaved").style.display = "inline";
  });
  document.querySelectorAll("#training-mode-grid .mode-card").forEach(c => c.addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().set_training_mode(Number(c.dataset.tmode));
    if (r && r.ok) { toast(T("Multiplicador de treino atualizado!", "Training multiplier updated!")); pollStatus(); }
    else toast(fmtCodeMsg(r), "err");
  }));
  document.querySelectorAll("#stamina-mode-grid .mode-card").forEach(c => c.addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().set_stamina_mode(Number(c.dataset.smode));
    if (r && r.ok) { toast(T("Modo de fôlego em partida atualizado!", "Match stamina mode updated!")); pollStatus(); }
    else toast(fmtCodeMsg(r), "err");
  }));
  document.getElementById("btn-freeze-clock").addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().set_freeze_clock(!FREEZE_CLOCK);
    if (r && r.ok) { toast(!FREEZE_CLOCK ? T("Relógio congelado!", "Clock frozen!") : T("Relógio liberado!", "Clock resumed!")); pollStatus(); }
    else toast(fmtCodeMsg(r), "err");
  });
  document.getElementById("btn-bal-apply").addEventListener("click", async () => {
    if (!api()) return;
    const pts = Number(document.getElementById("bal-focus-inp").value || 21);
    const lck = document.getElementById("bal-lock-chk").checked;
    const r = await api().set_bal_focus(pts, lck);
    if (r && r.ok) {
      BAL_DIRTY = false;
      document.getElementById("bal-unsaved").style.display = "none";
      toast(T(`Pontos de foco BAL: ${r.points}${r.lock ? " (Travado)" : ""}`, `BAL Focus Points: ${r.points}${r.lock ? " (Locked)" : ""}`));
    } else toast(fmtCodeMsg(r), "err");
  });

  await pollStatus();
  setInterval(pollStatus, 600);
}

window.addEventListener("pywebviewready", bootstrap);
