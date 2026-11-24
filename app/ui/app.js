/* PES 2021 Workbench - Standalone Frontend (WebView2 + 100% PT-BR / EN) */

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
let ECON_SYNC_MODE = (function () {
  try {
    const raw = localStorage.getItem("pes21_studio_econ_sync");
    if (raw === null) return 1;
    const v = Number(raw);
    return (v >= 0 && v <= 3) ? v : 1;
  } catch (e) { return 1; }
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
let PITCH_ORIENT_MODE = 0; // 0=Auto, 1=Horizontal, 2=Vertical
let POSLAB_LOG_MODE = 0;   // 0=Steps, 1=Net Summary
let POSLAB_LOG_ADDR = 0;
let POSLAB_BASE_STATS = null;
let POSLAB_LOG = [];
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
let IS_OVR_BUSY = false;
let BAL_DIRTY = false;
let CMD_SELECTED = 0;

const api = () => (window.pywebview && window.pywebview.api) || null;
const T = (pt, en) => (LANG === "en" ? en : pt);

function normText(str) {
  return String(str ?? "")
    .normalize("NFD")
    .replace(/[\u0300-\u036f]/g, "")
    .toLowerCase();
}

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
    tip_currency: "Alternar moeda de exibição (€, R$, $, £)",
    tip_lang: "Alternar idioma entre Português e Inglês",
    tip_reconnect: "Clique para reconectar ao PES 2021 / Football Life",
    tip_salary_avail: "Valor real livre para novos contratos (Teto Bruto menos folha ativa e pendentes)",
    tip_wage_bill: "Soma dos salários anuais de todos os atletas atualmente no seu clube",
    tip_gross_cap: "Orçamento salarial bruto interno da Master League antes de descontar a folha",
    tip_hooks_status: "Os recursos x64 são ligados automaticamente só quando ativados e restaurados ao fechar",
    tip_arrow_blue: "Aplica seta azul (forma máxima) elevando o rendimento em partida",
    tip_sq_stam: "Restaura o fôlego de todos os atletas do seu clube para 100%",
    tip_sq_arrow: "Coloca seta azul (forma máxima) em todo o elenco",
    tip_sq_heal: "Zera os dias de lesão de todos os atletas lesionados do elenco",
    tip_sq_contract: "Estende em +5 anos os contratos mais curtos sem encurtar quem já tem vínculo longo",
    tip_sq_ovr1: "Sobe +1 OVR em cada jogador do elenco preservando a proporção natural de atributos",
    tip_ovr_m1: "Desce 1 ponto de Overall geral mantendo a proporção natural dos atributos (DNA)",
    tip_ovr_p1: "Sobe 1 ponto de Overall geral mantendo a proporção natural dos atributos (DNA)",
    tip_ovr_dna: "Usa a fórmula exata da Konami com distribuição proporcional de Hamilton para evoluir o OVR sem distorcer o estilo do atleta",
    tip_target_ovr: "Digite o Overall desejado (40 a 99) e pressione Enter para ajustar mantendo o DNA",
    tip_all_99: "Define todos os atributos ativos para 99 (descaracteriza o perfil original do atleta)",
    tip_add_fav: "Adiciona o atleta à lista nativa de Favoritos (Shortlist) sem alterar contrato ou cláusulas",
    tip_export_json: "Exporta todos os atributos, bio e habilidades deste jogador em formato JSON",
    tip_import_json: "Restaura os dados de um jogador a partir de um backup JSON",
    tip_stam_input: "Digite o fôlego exato (0 a 100) ou arraste a barra ao lado",
    tip_form_arrow: "Define a condição pré-jogo do atleta. Afeta temporariamente os atributos durante a partida",
    tip_arr_4: "Forma máxima: bônus alto nos atributos em campo",
    tip_arr_3: "Boa forma: leve aumento nos atributos em campo",
    tip_arr_2: "Forma normal: atributos padrão sem bônus ou penalidade",
    tip_arr_1: "Forma baixa: leve queda de rendimento em campo",
    tip_arr_0: "Péssima forma: queda acentuada nos atributos em campo",
    tip_rejuv_3: "Reduz a idade atual em 3 anos",
    tip_age_18: "Define a idade para 18 anos para maximizar o ciclo de evolução",
    tip_wfu: "Frequência com que o atleta escolhe usar o pé não dominante (1 = raro, 4 = frequente)",
    tip_wfa: "Qualidade de passes e chutes feitos com o pé não dominante (1 = baixa, 4 = quase igual ao pé bom)",
    tip_form_cons: "Estabilidade de 1 a 8. Quanto maior, maior a chance de vir com seta verde ou azul nas partidas",
    tip_injres: "Resistência física contra lesões durante partidas e treinos (1 = frágil, 3 = muito resistente)",
    tip_tlist: "Marca o atleta na lista de transferências da Master League",
    tip_llist: "Marca o atleta como disponível para empréstimo na Master League",
    tip_grade_a: "Grau A (Total): libera o bônus completo de OVR na posição secundária (+4 a +5 OVR)",
    tip_grade_b: "Grau B (Parcial): libera bônus intermediário de OVR na posição secundária (+2 a +3 OVR)",
    tip_grade_c: "Grau C (Sem familiaridade): sem bônus de Grau na posição secundária (+0 OVR)",
    tip_pos_grade: "Familiaridade na posição (C, B ou A). Subir para A libera bônus direto no OVR da posição",
    tip_pos_iso: "Sobe ou desce 1 OVR na posição secundária usando contrapeso exato sem alterar o OVR principal",
    tip_pitch_orient: "Clique para alternar entre Automático (vira na horizontal em janelas verticais), Horizontal ou Vertical",
    tip_poslab_log: "Mostra exatamente quais atributos subiram ou foram compensados em cada ajuste do Lab de Posições",
    log_steps: "Passo a passo",
    log_net: "Acumulado",
    tip_playing_style: "Define a inteligência de movimentação sem bola nas posições compatíveis",
    tip_player_skills: "Passe o mouse em qualquer habilidade para ver o efeito exato em campo. O limite nativo do jogo é 10",
    tip_com_ai: "Define tendências de decisão quando o jogador está sob controle da IA (CPU ou modo SIM)",
    tip_team_role: "Função do atleta no elenco da Master League. Gera bônus de XP em treino, Espírito de equipe, receitas e efeitos de capitão",
    tip_role_lvl: "Níveis mais altos amplificam os bônus de treino, moral e receita da função escolhida",
    tip_match_stam: "Controla o desgaste de fôlego em tempo real durante a partida",
    tip_press_home: "Mantém o time da casa em 100% e reduz o visitante para 5% de fôlego",
    tip_press_away: "Mantém o visitante em 100% e reduz o time da casa para 5% de fôlego",
    tip_sub_xp: "Multiplica o ganho de experiência interna (Sub-XP) nos treinos do seu clube, base e BAL",
    tip_bal_focus: "Desbloqueia o teto de 21 Pontos de Foco no Rumo ao Estrelato, permitindo até 99 pontos",
    tip_bal_lock: "Mantém os Pontos de Foco travados no valor definido enquanto você distribui as barras",
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
    cmd_ovr1_squad: "+1 OVR no Elenco (DNA)",
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
    ptab_role: "Função no time",
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
    lbl_wfu: "Pior pé (frequência) (1-4)",
    lbl_wfa: "Pior pé (precisão) (1-4)",
    lbl_form: "Forma física (1-8)",
    lbl_injres: "Resistência a lesão (1-3)",
    lbl_transfer_listed: "Listado para Transferência",
    lbl_loan_listed: "Listado para Empréstimo",
    dna_bar_title: "Evolução de OVR Geral (Mantendo DNA)",
    dna_bar_sub: "Sobe ou desce o Overall distribuindo pontos na proporção exata dos atributos do atleta.",
    lbl_target_ovr: "Alvo:",
    stats_hint: "Clique no número para digitar (40-99), arraste com precisão ou segure Shift para ajuste fino.",
    radar_title: "Perfil Técnico (6 Eixos)",
    pitch_title: "Mapa Tático (13 Posições)",
    grade_a: "Grau A",
    grade_b: "Grau B",
    grade_c: "Grau C",
    btn_all_99: "Tudo 99",
    poslab_title: "Lab de Posições (Precisão Cirúrgica)",
    poslab_hint: "OVR já inclui bônus do Grau. +1/-1 move 1 por vez e a posição principal nunca é alterada.",
    card_playing_style: "Estilo de jogo",
    card_player_skills: "Habilidades do jogador",
    card_com_ai: "Estilos de jogo do COMP",
    card_team_role: "Função no time",
    lbl_role: "Função",
    lbl_role_lvl: "Nível (0 a 3)",
    btn_apply_role: "Aplicar Função no time",
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
    lbl_econ_sync: "Economia ao editar OVR:",
    econ_opt_0: "Desativado (Manual)",
    econ_opt_1: "Tudo (Mercado + Salário + Rescisão)",
    econ_opt_2: "Só Mercado e Rescisão",
    econ_opt_3: "Só Contrato (Salário)",
    btn_recal_econ: "Recalibrar Economia",
    lbl_has_rf: "Possui Taxa de Rescisão (Multa)",
    tip_econ_sync: "Ajusta imediatamente Valor de Mercado, Salário do Contrato e Taxa de Rescisão ao alterar o OVR. Escala a partir do valor original do atleta levando em conta OVR, idade, posição e país.",
    tip_econ_sync_sel: "Escolha se ao mudar o OVR devem subir ou descer Mercado + Salário + Rescisão, apenas Mercado e Rescisão, apenas Salário ou ficar manual.",
    tip_release_pill: "Calculada em tempo real pela fórmula nativa do PES 2021 a partir do Valor de Mercado, idade, afinidade e OVR. Ative ou desative a multa na aba Condição & Bio.",
    tip_recal_econ: "Recalcula Valor de Mercado, Salário e Rescisão realistas para o OVR, idade, posição e país atuais.",
    tip_has_rf: "Ativa ou remove a cláusula de multa rescisória do contrato. Quando ativa, o valor acompanha automaticamente o Valor de Mercado.",
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
    tip_currency: "Cycle display currency (€, R$, $, £)",
    tip_lang: "Toggle language between Portuguese and English",
    tip_reconnect: "Click to reconnect to PES 2021 / Football Life",
    tip_salary_avail: "Actual budget available for new contracts (Gross Cap minus active wages and pending overrides)",
    tip_wage_bill: "Sum of annual salaries for all players currently in your club",
    tip_gross_cap: "Internal Master League gross salary cap before deducting squad wages",
    tip_hooks_status: "Native x64 features arm automatically only when enabled and restore cleanly on exit",
    tip_arrow_blue: "Applies top form arrow (Blue) for maximum in-match boost",
    tip_sq_stam: "Restores stamina to 100% for every player in your club",
    tip_sq_arrow: "Sets top Blue form arrow for your entire squad",
    tip_sq_heal: "Clears injury days for every injured player in your squad",
    tip_sq_contract: "Extends shorter contracts by +5 years without reducing longer deals",
    tip_sq_ovr1: "Raises every squad player by +1 OVR while preserving each player's stat proportions",
    tip_ovr_m1: "Lowers Overall rating by 1 while keeping the player's natural stat proportions (DNA)",
    tip_ovr_p1: "Raises Overall rating by 1 while keeping the player's natural stat proportions (DNA)",
    tip_ovr_dna: "Uses Konami's exact formula with Hamilton proportional scaling to raise or lower OVR without breaking player identity",
    tip_target_ovr: "Type any target Overall (40 to 99) and press Enter to scale while preserving DNA",
    tip_all_99: "Sets all active attributes to 99 (replaces the player's original profile)",
    tip_add_fav: "Adds this player to the native Master League Shortlist without altering contracts or clauses",
    tip_export_json: "Exports this player's bio, attributes, and skills as JSON",
    tip_import_json: "Restores a player from a saved JSON backup",
    tip_stam_input: "Type exact stamina (0 to 100) or drag the bar",
    tip_form_arrow: "Sets pre-match condition arrow. Temporarily boosts or lowers match stats",
    tip_arr_4: "Top form: strong temporary boost to match attributes",
    tip_arr_3: "Good form: slight boost to match attributes",
    tip_arr_2: "Normal form: base attributes with no modifier",
    tip_arr_1: "Poor form: slight drop in match attributes",
    tip_arr_0: "Worst form: significant drop in match attributes",
    tip_rejuv_3: "Lowers current age by 3 years",
    tip_age_18: "Sets age to 18 to maximize the development cycle",
    tip_wfu: "How often the player chooses to use his weaker foot (1 = rarely, 4 = frequently)",
    tip_wfa: "Quality of passes and shots with the weaker foot (1 = low, 4 = near strong foot)",
    tip_form_cons: "Consistency from 1 to 8. Higher values increase the chance of green or blue match arrows",
    tip_injres: "Resistance to injuries in matches and training (1 = fragile, 3 = highly resistant)",
    tip_tlist: "Places the player on the Master League transfer list",
    tip_llist: "Makes the player available for loan in Master League",
    tip_grade_a: "Grade A (Full): unlocks the full secondary position OVR bonus (+4 to +5 OVR)",
    tip_grade_b: "Grade B (Partial): unlocks intermediate secondary position OVR bonus (+2 to +3 OVR)",
    tip_grade_c: "Grade C (Unfamiliar): zero Grade bonus in this secondary position (+0 OVR)",
    tip_pos_grade: "Position familiarity (C, B, or A). Raising to A adds a direct OVR bonus in that position",
    tip_pos_iso: "Steps secondary position OVR 1 by 1 using exact counterbalance while locking primary OVR",
    tip_pitch_orient: "Click to cycle Auto (flips horizontal on vertical/narrow windows), Horizontal, or Vertical",
    tip_poslab_log: "Shows exactly which attributes increased or counterbalanced on every Position Lab step",
    log_steps: "Step-by-step",
    log_net: "Net Summary",
    tip_playing_style: "Controls off-the-ball movement intelligence when playing in compatible positions",
    tip_player_skills: "Hover any skill to see its exact on-pitch effect. Default game cap is 10 skills",
    tip_com_ai: "Controls decision tendencies when the player is managed by AI (CPU or SIM mode)",
    tip_team_role: "Master League squad role. Grants training EXP boosts, Team Spirit, club revenue, and captain perks",
    tip_role_lvl: "Higher levels amplify the training, morale, and financial perks of the selected role",
    tip_match_stam: "Controls real-time stamina drain during live matches",
    tip_press_home: "Locks Home team at 100% stamina and drains Away team to 5%",
    tip_press_away: "Locks Away team at 100% stamina and drains Home team to 5%",
    tip_sub_xp: "Multiplies internal training experience (Sub-XP) for your club, youth team, and BAL",
    tip_bal_focus: "Unlocks the 21 Focus Points cap in Become a Legend, allowing up to 99 points",
    tip_bal_lock: "Keeps Focus Points locked at the target value while allocating training bars",
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
    cmd_ovr1_squad: "+1 OVR to Squad (DNA)",
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
    lbl_form: "Form (1-8)",
    lbl_injres: "Injury Resistance (1-3)",
    lbl_transfer_listed: "Transfer Listed",
    lbl_loan_listed: "Loan Listed",
    dna_bar_title: "Overall Evolution (DNA Preserving)",
    dna_bar_sub: "Raises or lowers Overall by scaling stats in the exact natural proportion of the player.",
    lbl_target_ovr: "Target:",
    stats_hint: "Click any number to type (40-99), drag smoothly, or hold Shift for ultra-fine control.",
    radar_title: "Technical Profile (6 Axes)",
    pitch_title: "Tactical Pitch (13 Positions)",
    grade_a: "Grade A",
    grade_b: "Grade B",
    grade_c: "Grade C",
    btn_all_99: "All 99",
    poslab_title: "Position Lab (Surgical Precision)",
    poslab_hint: "Displayed OVR includes Grade bonus. +1/-1 steps 1 by 1 and primary position is never altered.",
    card_playing_style: "Playing Style",
    card_player_skills: "Player Skills",
    card_com_ai: "COM Playing Styles",
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
    lbl_econ_sync: "Auto-sync Economy on OVR:",
    econ_opt_0: "Off (Manual)",
    econ_opt_1: "All (Market + Salary + Release)",
    econ_opt_2: "Market & Release Only",
    econ_opt_3: "Contract Salary Only",
    btn_recal_econ: "Recalibrate Economy",
    lbl_has_rf: "Has Release Clause",
    tip_econ_sync: "Immediately scales Market Value, Contract Salary and Release Fee whenever OVR changes. Anchored to the player's original value considering OVR, age, position and country.",
    tip_econ_sync_sel: "Choose whether OVR edits update Market + Salary + Release Fee, Market & Release only, Salary only, or stay manual.",
    tip_release_pill: "Computed live by PES 2021's native formula from Market Value, age, affection and OVR. Toggle the clause in Condition & Bio.",
    tip_recal_econ: "Recalculates realistic Market Value, Salary and Release Fee for the current OVR, age, position and country.",
    tip_has_rf: "Enables or removes the contract release fee clause. When active, the fee scales automatically with Market Value.",
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
    case "not_in_match": return T("Entre em uma partida para congelar o relógio.", "Start a match first to freeze the clock.");
    case "hook_failed": return T("Não foi possível ativar o hook x64 neste executável.", "Could not activate x64 hook on this executable.");
    case "already_at_ovr": return T(`Já está em ${res.ovr} OVR.`, `Already at ${res.ovr} OVR.`);
    case "pri_updated": return T(`OVR Principal: ${res.orig} -> ${res.final}`, `Primary OVR: ${res.orig} -> ${res.final}`);
    case "pos_updated": {
      const sp = res.pos != null ? posShort(res.pos) : T("Posição", "Position");
      const rp = res.reg_pos != null ? posShort(res.reg_pos) : T("Principal", "Primary");
      return T(`${sp}: ${res.orig} -> ${res.final} OVR (${rp} mantido em ${res.pri})`, `${sp}: ${res.orig} -> ${res.final} OVR (${rp} kept at ${res.pri})`);
    }
    case "pos_partial": {
      const sp = res.pos != null ? posShort(res.pos) : T("Posição", "Position");
      const rp = res.reg_pos != null ? posShort(res.reg_pos) : T("Principal", "Primary");
      const hint = res.needs_grade ? T(" Suba o Grau para A para liberar mais OVR.", " Raise Grade to A to unlock more OVR.") : "";
      return T(`${sp}: ${res.orig} -> ${res.final} OVR (máximo sem alterar ${rp} ${res.pri}).${hint}`, `${sp}: ${res.orig} -> ${res.final} OVR (max without changing ${rp} ${res.pri}).${hint}`);
    }
    case "iso_limit": {
      const rp = res.reg_pos != null ? posShort(res.reg_pos) : T("principal", "primary");
      if (res.needs_grade) {
        return T(
          `Não dá para subir mais sem alterar ${rp} (${res.pri} OVR). Suba o Grau para B ou A primeiro.`,
          `Cannot raise further without changing ${rp} (${res.pri} OVR). Raise Grade to B or A first.`
        );
      }
      return T(
        `Teto máximo atingido (${res.orig} OVR) mantendo ${rp} em ${res.pri} OVR.`,
        `Maximum possible reached (${res.orig} OVR) while keeping ${rp} at ${res.pri} OVR.`
      );
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
  const dur = (msg && String(msg).length > 52) ? 9000 : 6500;
  let timer = setTimeout(dismiss, dur);
  el.addEventListener("mouseenter", () => { if (timer) { clearTimeout(timer); timer = null; } });
  el.addEventListener("mouseleave", () => { if (!timer) timer = setTimeout(dismiss, 2400); });
}

function fmtMoney(raw) {
  const cur = CURRENCIES[CUR_IDX] || CURRENCIES[0];
  const euros = Math.round((raw || 0) * 100);
  const abs = Math.abs(euros);
  const s = abs.toLocaleString(LANG === "en" ? cur.localeEn : cur.localePt);
  return (euros < 0 ? `-${cur.sym} ` : `${cur.sym} `) + s;
}

function fmtEconDeltaLine(ed) {
  if (!ed || !ed.applied) return "";
  const parts = [];
  if (ed.old_market_raw !== ed.new_market_raw) {
    parts.push(`${T("Mercado", "Market")}: ${fmtMoney(ed.old_market_raw)} -> ${fmtMoney(ed.new_market_raw)}`);
  }
  if (ed.old_salary_raw !== ed.new_salary_raw) {
    parts.push(`${T("Salário", "Salary")}: ${fmtMoney(ed.old_salary_raw)} -> ${fmtMoney(ed.new_salary_raw)}`);
  }
  if (ed.has_release_fee && ed.old_release_raw !== ed.new_release_raw) {
    parts.push(`${T("Rescisão", "Release")}: ${fmtMoney(ed.old_release_raw)} -> ${fmtMoney(ed.new_release_raw)}`);
  }
  return parts.join(" | ");
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
// CUSTOM FLOATING DARK TOOLTIP ENGINE
// -------------------------------------------------------------------
let TIP_TIMER = null;
let TIP_TARGET = null;

function hideTooltip() {
  clearTimeout(TIP_TIMER);
  TIP_TIMER = null;
  TIP_TARGET = null;
  const box = document.getElementById("wb-tooltip");
  if (box) box.classList.remove("show");
}

function showTooltipFor(el) {
  const box = document.getElementById("wb-tooltip");
  if (!box || !el || !document.body.contains(el)) return;
  const i18nKey = el.getAttribute("data-i18n-tip");
  const body = i18nKey ? tKey(i18nKey) : (el.getAttribute("data-tip") || "");
  const title = el.getAttribute("data-tip-title") || "";
  if (!body && !title) return;

  box.innerHTML =
    (title ? `<div class="wb-tip-title">${escapeHtml(title)}</div>` : "") +
    (body ? `<div class="wb-tip-body">${escapeHtml(body)}</div>` : "");

  box.style.left = "-9999px";
  box.style.top = "-9999px";
  box.classList.add("show");

  const r = el.getBoundingClientRect();
  const bw = box.offsetWidth;
  const bh = box.offsetHeight;
  const pad = 10;

  let x = r.left + (r.width - bw) / 2;
  x = Math.max(pad, Math.min(window.innerWidth - bw - pad, x));

  let y = r.top - bh - 8;
  if (y < pad) y = r.bottom + 8;
  if (y + bh > window.innerHeight - pad) {
    y = Math.max(pad, window.innerHeight - bh - pad);
  }

  box.style.left = `${Math.round(x)}px`;
  box.style.top = `${Math.round(y)}px`;
}

function initTooltips() {
  document.addEventListener("mouseover", e => {
    const t = e.target && e.target.closest ? e.target.closest("[data-tip], [data-i18n-tip]") : null;
    if (!t) {
      if (TIP_TARGET) hideTooltip();
      return;
    }
    if (t === TIP_TARGET) return;
    hideTooltip();
    TIP_TARGET = t;
    TIP_TIMER = setTimeout(() => showTooltipFor(t), 120);
  });
  document.addEventListener("mouseout", e => {
    if (!TIP_TARGET) return;
    const rel = e.relatedTarget;
    if (!rel || !TIP_TARGET.contains(rel)) hideTooltip();
  });
  document.addEventListener("mousedown", hideTooltip);
  document.addEventListener("scroll", hideTooltip, true);
  window.addEventListener("blur", hideTooltip);
}

// -------------------------------------------------------------------
// LANGUAGE & CURRENCY
// -------------------------------------------------------------------
function updatePlayingStyleInfo() {
  if (!META) return;
  const sel = document.getElementById("p-playing-style");
  const descEl = document.getElementById("p-style-desc");
  const posEl = document.getElementById("p-style-pos");
  if (!sel || !descEl) return;
  const sid = Number(sel.value || 0);
  const st = META.playing_styles.find(x => x.id === sid);
  if (!st) {
    descEl.textContent = "";
    if (posEl) posEl.textContent = "";
    return;
  }
  descEl.textContent = LANG === "en" ? st.desc_en : st.desc_pt;
  if (posEl) {
    const posTxt = LANG === "en" ? st.pos_en : st.pos_pt;
    posEl.textContent = `${T("Ativa em", "Active at")}: ${posTxt}`;
  }
}

function updateTeamRoleInfo() {
  if (!META) return;
  const sel = document.getElementById("p-team-role");
  const infoEl = document.getElementById("p-role-info");
  if (!sel || !infoEl) return;
  const rid = Number(sel.value || 0);
  const role = META.team_roles.find(x => x.id === rid);
  if (!role) {
    infoEl.innerHTML = "";
    infoEl.classList.remove("has-captain");
    return;
  }
  const hasCap = !!role.captain;
  infoEl.classList.toggle("has-captain", hasCap);

  const tagLabels = {
    fin: T("Finanças", "Finances"),
    cond: T("Condição", "Condition"),
    xp: T("Treino / XP", "Training / EXP"),
    spirit: T("Espírito de equipe", "Team Spirit"),
  };
  const chips = [];
  if (role.id !== 0) {
    if (hasCap) {
      chips.push(`<span class="role-tag role-tag-cap">★ ${escapeHtml(T("Bônus de Capitão", "Captain Bonus"))}</span>`);
    } else {
      chips.push(`<span class="role-tag role-tag-muted">${escapeHtml(T("Sem bônus de capitão", "No captain bonus"))}</span>`);
    }
    (role.tags || []).forEach(tg => {
      if (tagLabels[tg]) chips.push(`<span class="role-tag">${escapeHtml(tagLabels[tg])}</span>`);
    });
  }

  const desc = LANG === "en" ? role.desc_en : role.desc_pt;
  const capDesc = LANG === "en" ? (role.captain_en || "") : (role.captain_pt || "");
  const capTitle = T("Efeito de Capitão (com a braçadeira)", "Captain Effect (when wearing the armband)");

  infoEl.innerHTML =
    (chips.length ? `<div class="role-tags">${chips.join("")}</div>` : "") +
    `<div class="role-desc-text">${escapeHtml(desc)}</div>` +
    (hasCap && capDesc
      ? `<div class="role-captain-box"><div class="role-captain-title">★ ${escapeHtml(capTitle)}</div><div class="role-captain-body">${escapeHtml(capDesc)}</div></div>`
      : "");
}

function populateLocalizedSelects() {
  if (!META) return;
  const footEl = document.getElementById("bio-foot");
  const curFoot = footEl.value || "0";
  footEl.innerHTML = `<option value="0">${escapeHtml(T("Direito", "Right"))}</option><option value="1">${escapeHtml(T("Esquerdo", "Left"))}</option>`;
  footEl.value = curFoot;

  const styleEl = document.getElementById("p-playing-style");
  const curStyle = styleEl.value || "0";
  styleEl.innerHTML = META.playing_styles.map(s => `<option value="${s.id}">${escapeHtml(LANG === "en" ? s.en : s.pt)}</option>`).join("");
  styleEl.value = curStyle;
  updatePlayingStyleInfo();

  const roleEl = document.getElementById("p-team-role");
  const curRole = roleEl.value || "0";
  const capSuffix = T(" ★ Capitão", " ★ Captain");
  roleEl.innerHTML = META.team_roles.map(r => {
    const base = LANG === "en" ? r.en : r.pt;
    return `<option value="${r.id}">${escapeHtml(r.captain ? `${base}${capSuffix}` : base)}</option>`;
  }).join("");
  roleEl.value = curRole;
  updateTeamRoleInfo();

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

function setRangeVisual(rangeEl, val) {
  if (!rangeEl) return;
  const min = Number(rangeEl.min || 0);
  const max = Number(rangeEl.max || 100);
  const pct = max > min ? Math.max(0, Math.min(100, ((Number(val) - min) / (max - min)) * 100)) : 0;
  rangeEl.value = String(val);
  rangeEl.style.setProperty("--pct", `${pct.toFixed(1)}%`);
}

function updateHeroHeader(d) {
  const heroOvr = document.getElementById("p-hero-ovr");
  heroOvr.innerHTML = `${d.ovr}<small>OVR</small>`;
  heroOvr.className = `hero-shield ${ovrClass(d.ovr)}`;

  const statsOvr = document.getElementById("stats-ovr-now");
  if (statsOvr) {
    statsOvr.textContent = String(d.ovr);
    statsOvr.className = `ovr-badge ${ovrClass(d.ovr)}`;
  }
  const tgtInp = document.getElementById("inp-stats-target-ovr");
  if (tgtInp && document.activeElement !== tgtInp) {
    tgtInp.placeholder = String(Math.min(99, d.ovr + 1));
  }

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

  // Economy Sync strip & Release Fee pill
  const selEcon = document.getElementById("sel-econ-sync");
  if (selEcon) selEcon.value = String(ECON_SYNC_MODE);
  const lblEcon = document.getElementById("lbl-econ-sync");
  if (lblEcon) lblEcon.classList.toggle("off", ECON_SYNC_MODE === 0);

  const rfPill = document.getElementById("p-release-pill");
  if (rfPill) {
    if (d.has_release_fee && d.release_fee_raw > 0) {
      rfPill.textContent = `${T("Rescisão", "Release")}: ${fmtMoney(d.release_fee_raw)}`;
      rfPill.classList.add("active");
    } else {
      rfPill.textContent = T("Rescisão: Sem multa", "Release: No clause");
      rfPill.classList.remove("active");
    }
  }
  const recalBtn = document.getElementById("btn-recal-econ");
  if (recalBtn) {
    const snapMismatch = (d.snap_ovr >= 40 && d.snap_ovr <= 99 && d.snap_ovr !== d.ovr);
    recalBtn.classList.toggle("btn-gold", snapMismatch);
  }
}

function renderPlayerDetails(d) {
  const scroller = document.getElementById("main-scroll");
  const prevScroll = scroller ? scroller.scrollTop : 0;

  showPlayerEmptyState(false);
  updateHeroHeader(d);

  // Quick Condition & Injury pill
  const stamRange = document.getElementById("inp-p-stam");
  setRangeVisual(stamRange, d.stamina);
  const stamNum = document.getElementById("inp-p-stam-num");
  if (stamNum && document.activeElement !== stamNum) {
    stamNum.value = String(d.stamina);
    stamNum.className = `stat-num-inp ${ovrClass(d.stamina)}`;
  }
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
  const rfChk = document.getElementById("bio-has-rf");
  if (rfChk) rfChk.checked = !!d.has_release_fee;

  renderStatsRadar(d.stats);
  renderStatsGroups(d);
  if (d.addr !== POSLAB_LOG_ADDR) {
    POSLAB_LOG_ADDR = d.addr;
    POSLAB_BASE_STATS = { ...d.stats };
    POSLAB_LOG = [];
  }
  renderPositionPitch(d);
  renderPositionLab(d);
  renderPosLabLog();
  renderSkillsAndStyle(d);
  document.getElementById("p-team-role").value = String(d.team_role);
  document.getElementById("p-team-role-lvl").value = String(d.team_role_lvl);
  updateTeamRoleInfo();

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
    { lbl: T("GO", "GK"),   val: avg([16, 28, 29, 30, 31]) },
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
  const groups = d.reg_pos === 0
    ? [
        { id: "gk", title: T("Goleiro", "Goalkeeper") },
        { id: "phys", title: T("Físico", "Physical") },
        { id: "tech", title: T("Técnica & Passe", "Technique & Passing") },
        { id: "def", title: T("Defesa", "Defense") },
        { id: "attack", title: T("Ataque", "Attack") },
      ]
    : [
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
      const pct = (((val - 40) / 59) * 100).toFixed(1);
      const lbl = LANG === "en" ? s.en : s.pt;
      const desc = LANG === "en" ? (s.desc_en || "") : (s.desc_pt || "");
      return `<div class="stat-row" data-stat-row="${s.fid}">
        <span class="stat-name" data-tip-title="${escapeHtml(lbl)}" data-tip="${escapeHtml(desc)}">${escapeHtml(lbl)}</span>
        <input type="range" class="studio-range" min="40" max="99" value="${val}" style="--pct:${pct}%;" data-stat-fid="${s.fid}" />
        <input type="number" class="stat-num-inp ${ovrClass(val)}" min="40" max="99" value="${val}" data-stat-num="${s.fid}" />
        <div style="display:flex; gap:3px;">
          <button class="btn btn-xs" type="button" data-stat-dec="${s.fid}">-</button>
          <button class="btn btn-xs" type="button" data-stat-inc="${s.fid}">+</button>
        </div>
      </div>`;
    }).join("");
    return `<div class="card"><div class="card-title">${escapeHtml(g.title)}</div><div>${rows}</div></div>`;
  }).join("");
}

function previewStatUI(fid, val) {
  const clamped = Math.max(40, Math.min(99, Math.round(Number(val) || 40)));
  const range = document.querySelector(`input[data-stat-fid="${fid}"]`);
  setRangeVisual(range, clamped);
  const numInp = document.querySelector(`input[data-stat-num="${fid}"]`);
  if (numInp) {
    if (document.activeElement !== numInp) numInp.value = String(clamped);
    numInp.className = `stat-num-inp ${ovrClass(clamped)}`;
  }
  if (CURRENT_PLAYER && CURRENT_PLAYER.stats) {
    CURRENT_PLAYER.stats[String(fid)] = clamped;
    renderStatsRadar(CURRENT_PLAYER.stats);
  }
  return clamped;
}

// In-place stat update: preserves slider/input/button keyboard focus & zero DOM flicker!
async function onStatChange(fid, val) {
  if (!api()) return;
  const clamped = Math.max(40, Math.min(99, Math.round(Number(val) || 40)));
  const res = await api().set_player_stat(Number(fid), clamped);
  if (res && res.ok) {
    const ovrChanged = CURRENT_PLAYER && res.ovr !== CURRENT_PLAYER.ovr;
    CURRENT_PLAYER = res;
    const newVal = res.stats[String(fid)] || clamped;
    const range = document.querySelector(`input[data-stat-fid="${fid}"]`);
    setRangeVisual(range, newVal);
    const numInp = document.querySelector(`input[data-stat-num="${fid}"]`);
    if (numInp) {
      numInp.value = String(newVal);
      numInp.className = `stat-num-inp ${ovrClass(newVal)}`;
    }
    updateHeroHeader(res);
    if (res.econ_delta && res.econ_delta.applied) {
      document.getElementById("bio-salary").value = fmtMoney(res.salary_raw);
      document.getElementById("bio-market").value = fmtMoney(res.market_raw);
      const el = fmtEconDeltaLine(res.econ_delta);
      if (el) toast(`OVR ${res.econ_delta.old_ovr} -> ${res.econ_delta.new_ovr} | ${el}`);
    }
    renderStatsRadar(res.stats);
    renderPositionPitch(res);
    renderPositionLab(res);
    if (ovrChanged) pulseOvrShield();
  } else {
    toast(fmtCodeMsg(res), "err");
  }
}

async function stepPlayerOvrDNA(delta) {
  if (!api() || IS_OVR_BUSY) return;
  IS_OVR_BUSY = true;
  try {
    const r = await api().step_player_ovr(delta);
    if (r && r.ok) {
      CURRENT_PLAYER = r;
      recordPosLabAction(r, delta > 0 ? "+1 OVR DNA" : "-1 OVR DNA", delta > 0 ? "+1 OVR DNA" : "-1 OVR DNA");
      renderPlayerDetails(r);
      pulseOvrShield();
    } else {
      toast(fmtCodeMsg(r), "err");
    }
  } finally {
    IS_OVR_BUSY = false;
  }
}

async function setTargetOvrDNA() {
  if (!api() || IS_OVR_BUSY) return;
  if (!CURRENT_PLAYER) return toast(tKey("no_player"), "err");
  const inp = document.getElementById("inp-stats-target-ovr");
  const raw = inp ? inp.value.trim() : "";
  const val = Number(raw);
  if (!raw || Number.isNaN(val) || val < 40 || val > 99) {
    toast(T("Digite um OVR alvo entre 40 e 99.", "Enter a target OVR between 40 and 99."), "err");
    if (inp) inp.focus();
    return;
  }
  IS_OVR_BUSY = true;
  try {
    const r = await api().set_player_ovr(Math.round(val));
    if (r && r.ok) {
      CURRENT_PLAYER = r;
      recordPosLabAction(r, "Alvo OVR DNA", "Target OVR DNA");
      renderPlayerDetails(r);
      pulseOvrShield();
      if (inp) inp.value = "";
    } else {
      toast(fmtCodeMsg(r), "err");
    }
  } finally {
    IS_OVR_BUSY = false;
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
    const el = fmtEconDeltaLine(res.econ_delta);
    const baseMsg = T(`Atributos definidos para ${val}`, `Attributes set to ${val}`);
    toast(el ? `${baseMsg} | ${el}` : baseMsg);
  } else toast(fmtCodeMsg(res), "err");
}

// -------------------------------------------------------------------
// INTERACTIVE TACTICAL MINI-PITCH (VERTICAL & HORIZONTAL) + DIFF LOG
// -------------------------------------------------------------------
function isPitchHorizontal() {
  if (PITCH_ORIENT_MODE === 1) return true;
  if (PITCH_ORIENT_MODE === 2) return false;
  return window.innerWidth <= 1160 || window.innerHeight > window.innerWidth * 0.78;
}

const PITCH_COORDS_VERT = {
  12: [110, 34], 11: [110, 74], 9: [44, 62], 10: [176, 62],
  8: [110, 114], 6: [40, 146], 5: [110, 154], 7: [180, 146],
  4: [110, 194], 2: [40, 232], 1: [110, 240], 3: [180, 232], 0: [110, 286],
};

const PITCH_COORDS_HORIZ = {
  0: [36, 102],
  2: [98, 36], 1: [98, 102], 3: [98, 168],
  4: [160, 102],
  6: [224, 34], 5: [224, 102], 7: [224, 170],
  8: [288, 102],
  9: [352, 38], 11: [352, 102], 10: [352, 166],
  12: [416, 102],
};

function renderPositionPitch(d) {
  const svg = document.getElementById("poslab-pitch-svg");
  if (!svg || !d || !d.positions) return;
  const isHoriz = isPitchHorizontal();
  const layout = document.getElementById("poslab-layout");
  if (layout) layout.classList.toggle("stacked", isHoriz);
  const oBtn = document.getElementById("btn-pitch-orient");
  if (oBtn) {
    oBtn.textContent = PITCH_ORIENT_MODE === 0
      ? (isHoriz ? "Auto (H)" : "Auto (V)")
      : (PITCH_ORIENT_MODE === 1 ? "Horiz" : "Vert");
  }
  svg.setAttribute("viewBox", isHoriz ? "0 0 460 204" : "0 0 220 320");
  svg.classList.toggle("horizontal", isHoriz);

  const pitchLines = isHoriz
    ? `
      <rect x="8" y="8" width="444" height="188" rx="6" fill="none" stroke="rgba(16,185,129,0.32)" stroke-width="1.4"/>
      <line x1="230" y1="8" x2="230" y2="196" stroke="rgba(16,185,129,0.28)" stroke-width="1.2"/>
      <circle cx="230" cy="102" r="28" fill="none" stroke="rgba(16,185,129,0.28)" stroke-width="1.2"/>
      <rect x="8" y="46" width="52" height="112" fill="none" stroke="rgba(16,185,129,0.25)" stroke-width="1.2"/>
      <rect x="400" y="46" width="52" height="112" fill="none" stroke="rgba(16,185,129,0.25)" stroke-width="1.2"/>
    `
    : `
      <rect x="8" y="8" width="204" height="304" rx="6" fill="none" stroke="rgba(16,185,129,0.32)" stroke-width="1.4"/>
      <line x1="8" y1="160" x2="212" y2="160" stroke="rgba(16,185,129,0.28)" stroke-width="1.2"/>
      <circle cx="110" cy="160" r="28" fill="none" stroke="rgba(16,185,129,0.28)" stroke-width="1.2"/>
      <rect x="56" y="8" width="108" height="44" fill="none" stroke="rgba(16,185,129,0.25)" stroke-width="1.2"/>
      <rect x="56" y="268" width="108" height="44" fill="none" stroke="rgba(16,185,129,0.25)" stroke-width="1.2"/>
    `;

  const coords = isHoriz ? PITCH_COORDS_HORIZ : PITCH_COORDS_VERT;
  const fallback = isHoriz ? [230, 102] : [110, 160];
  const nodes = d.positions.map(pos => {
    const [x, y] = coords[pos.id] || fallback;
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

function statDefByFid(fid) {
  return (META && META.stats) ? META.stats.find(s => Number(s.fid) === Number(fid)) : null;
}

function fmtStatDiffChips(diffs, compact = false) {
  return (diffs || []).map(df => {
    const sd = statDefByFid(df.fid);
    const name = sd ? (LANG === "en" ? sd.en : sd.pt) : `#${df.fid}`;
    const desc = sd ? (LANG === "en" ? (sd.desc_en || "") : (sd.desc_pt || "")) : "";
    const d = Number(df.new) - Number(df.old);
    const sign = d > 0 ? `+${d}` : String(d);
    const lbl = compact ? `${name} ${sign}` : `${name} ${df.old}->${df.new} (${sign})`;
    return `<span class="diff-chip ${d > 0 ? "up" : "dn"}" data-tip-title="${escapeHtml(name)}" data-tip="${escapeHtml(desc)}">${escapeHtml(lbl)}</span>`;
  }).join("");
}

function recordPosLabAction(res, tagPt, tagEn) {
  const diffs = Array.isArray(res.stat_diffs) ? res.stat_diffs : [];
  let msg = fmtCodeMsg(res);
  if (diffs.length) {
    const now = new Date();
    const ts = now.toTimeString().slice(0, 8);
    POSLAB_LOG.unshift({
      time: ts,
      pos: res.pos != null ? res.pos : (res.reg_pos != null ? res.reg_pos : -1),
      reg_pos: res.reg_pos != null ? res.reg_pos : -1,
      orig: res.orig || 0,
      final: res.final || 0,
      pri: res.pri || res.ovr || 0,
      tagPt,
      tagEn,
      diffs,
    });
    if (POSLAB_LOG.length > 30) POSLAB_LOG.length = 30;
    const top = diffs.slice(0, 3).map(df => {
      const sd = statDefByFid(df.fid);
      const nm = sd ? (LANG === "en" ? sd.en : sd.pt) : `#${df.fid}`;
      const d = df.new - df.old;
      return `${nm} ${d > 0 ? "+" : ""}${d}`;
    }).join(", ");
    const more = diffs.length > 3 ? ` (+${diffs.length - 3})` : "";
    msg += ` | ${top}${more}`;
  }
  const el = fmtEconDeltaLine(res.econ_delta);
  if (el) msg += ` | ${el}`;
  toast(msg, res.action_ok === false ? "err" : "ok");
}

function recordPosLabMeta(posId, origOvr, finalOvr, tagPt, tagEn) {
  const ts = new Date().toTimeString().slice(0, 8);
  POSLAB_LOG.unshift({
    time: ts,
    pos: posId,
    reg_pos: CURRENT_PLAYER ? CURRENT_PLAYER.reg_pos : -1,
    orig: origOvr,
    final: finalOvr,
    pri: CURRENT_PLAYER ? CURRENT_PLAYER.ovr : 0,
    tagPt,
    tagEn,
    diffs: [],
  });
  if (POSLAB_LOG.length > 30) POSLAB_LOG.length = 30;
}

function renderPosLabLog() {
  const titleEl = document.getElementById("poslab-log-title");
  const bodyEl = document.getElementById("poslab-log-body");
  const stripEl = document.getElementById("poslab-last-strip");
  const actBtn = document.getElementById("btn-log-action");
  if (!titleEl || !bodyEl) return;

  titleEl.textContent = `${T("Histórico de Atributos", "Attribute Diff Log")} (${POSLAB_LOG.length})`;
  document.getElementById("btn-log-mode-steps").classList.toggle("active", POSLAB_LOG_MODE === 0);
  document.getElementById("btn-log-mode-net").classList.toggle("active", POSLAB_LOG_MODE === 1);
  if (actBtn) actBtn.textContent = POSLAB_LOG_MODE === 0 ? T("Limpar", "Clear") : T("Zerar base", "Reset base");

  // Compact 1-line last-change strip above table when stacked/horizontal
  if (stripEl) {
    const showStrip = isPitchHorizontal() && POSLAB_LOG.length > 0 && POSLAB_LOG[0].diffs.length > 0;
    if (showStrip) {
      const last = POSLAB_LOG[0];
      stripEl.style.display = "flex";
      stripEl.innerHTML =
        `<span style="color:#60a5fa; font-weight:700;">${escapeHtml(T("Última alteração", "Last change"))}:</span>` +
        (last.pos >= 0 ? `<span class="pos-chip ${posClass(last.pos)}" style="min-width:34px; height:19px; font-size:10px;">${escapeHtml(posShort(last.pos))}</span>` : "") +
        `<div class="poslab-diff-chips">${fmtStatDiffChips(last.diffs, true)}</div>`;
    } else {
      stripEl.style.display = "none";
    }
  }

  if (POSLAB_LOG_MODE === 0) {
    if (!POSLAB_LOG.length) {
      bodyEl.innerHTML = `<div style="color:var(--muted); font-size:12px;">${escapeHtml(T("Clique em +1/-1 OVR ou Igualar Principal para acompanhar aqui cada atributo modificado.", "Click +1/-1 OVR or Match Primary to see every modified attribute tracked here."))}</div>`;
      return;
    }
    bodyEl.innerHTML = POSLAB_LOG.map(e => {
      const delta = e.final - e.orig;
      const ovrSpan = e.orig !== e.final
        ? `<span style="font-weight:800; color:${delta > 0 ? "#34d399" : "#f87171"};">${e.orig} -> ${e.final} (${delta > 0 ? "+" : ""}${delta})</span>`
        : `<span style="font-weight:700; color:#60a5fa;">${escapeHtml(LANG === "en" ? e.tagEn : e.tagPt)}</span>`;
      const priTag = (e.reg_pos >= 0 && e.pos !== e.reg_pos && e.pri > 0)
        ? `<span style="color:var(--muted);">(${escapeHtml(posShort(e.reg_pos))} ${e.pri})</span>`
        : "";
      const posChip = e.pos >= 0
        ? `<span class="pos-chip ${posClass(e.pos)}" style="min-width:34px; height:19px; font-size:10px;">${escapeHtml(posShort(e.pos))}</span>`
        : "";
      const chips = e.diffs.length
        ? `<div class="poslab-diff-chips">${fmtStatDiffChips(e.diffs, false)}</div>`
        : (e.orig !== e.final ? `<div style="font-size:11px; color:var(--subtext);">${escapeHtml(LANG === "en" ? e.tagEn : e.tagPt)}</div>` : "");
      return `<div class="poslab-log-entry"><div class="poslab-log-head"><span style="color:var(--muted);">${escapeHtml(e.time)}</span>${posChip}${ovrSpan}${priTag}</div>${chips}</div>`;
    }).join("");
  } else {
    const net = [];
    let sumUp = 0, sumDn = 0;
    if (CURRENT_PLAYER && CURRENT_PLAYER.stats && POSLAB_BASE_STATS && META) {
      META.stats.forEach(s => {
        const k = String(s.fid);
        const ov = Number(POSLAB_BASE_STATS[k] || 40);
        const nv = Number(CURRENT_PLAYER.stats[k] || 40);
        if (ov !== nv) {
          net.push({ fid: s.fid, old: ov, new: nv });
          if (nv > ov) sumUp += (nv - ov);
          else sumDn += (ov - nv);
        }
      });
      net.sort((a, b) => {
        const da = a.new - a.old, db = b.new - b.old;
        if ((da > 0) !== (db > 0)) return da > 0 ? -1 : 1;
        return Math.abs(db) - Math.abs(da);
      });
    }
    if (!net.length) {
      bodyEl.innerHTML = `<div style="color:var(--muted); font-size:12px;">${escapeHtml(T("Nenhuma diferença acumulada nos atributos desde o carregamento deste atleta.", "No net attribute changes since loading this player."))}</div>`;
      return;
    }
    bodyEl.innerHTML =
      `<div style="display:flex; gap:10px; font-size:11.5px; font-weight:700;">
        <span style="color:#34d399;">+${sumUp} ${escapeHtml(T("ganhos", "gained"))}</span>
        <span style="color:#fb923c;">-${sumDn} ${escapeHtml(T("compensados", "counterbalanced"))}</span>
        <span style="color:var(--subtext);">(${net.length} ${escapeHtml(T("atributos", "stats"))})</span>
      </div>
      <div class="poslab-diff-chips">${fmtStatDiffChips(net, false)}</div>`;
  }
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
  const tipMatch = T("Sobe esta posição até igualar o OVR da posição principal sem alterá-la", "Raises this position to match the primary OVR without changing primary OVR");
  const tipSetPri = T("Define esta como a nova posição registrada do atleta (com Grau A)", "Sets this as the player's new registered primary position (with Grade A)");
  const tipStepPos = T("Ajuste isolado de 1 em 1 sem alterar a posição principal", "Isolated 1-by-1 step without changing primary position OVR");
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
          <button type="button" class="${pos.apt === 0 ? "active" : ""}" data-pos-apt="${pos.id}" data-grade="0" data-i18n-tip="tip_grade_c">C</button>
          <button type="button" class="${pos.apt === 1 ? "active g-b" : ""}" data-pos-apt="${pos.id}" data-grade="1" data-i18n-tip="tip_grade_b">B</button>
          <button type="button" class="${pos.apt === 2 ? "active g-a" : ""}" data-pos-apt="${pos.id}" data-grade="2" data-i18n-tip="tip_grade_a">A</button>
        </div>
      </td>
      <td>
        <div style="display:flex; gap:6px;">
          <button class="btn btn-xs" type="button" data-pos-step="${pos.id}" data-delta="-1" data-tip="${escapeHtml(pos.is_reg ? tKey("tip_ovr_m1") : tipStepPos)}">-1 OVR</button>
          <button class="btn btn-xs btn-primary" type="button" data-pos-step="${pos.id}" data-delta="1" data-tip="${escapeHtml(pos.is_reg ? tKey("tip_ovr_p1") : tipStepPos)}">+1 OVR</button>
        </div>
      </td>
      <td class="act-col">
        <div style="display:flex; gap:6px; justify-content:flex-end;">
          ${pos.is_reg ? "" : `<button class="btn btn-xs btn-gold" type="button" data-pos-match="${pos.id}" data-tip="${escapeHtml(tipMatch)}">${escapeHtml(matchBtn)}</button>`}
          ${pos.is_reg ? "" : `<button class="btn btn-xs" type="button" data-pos-reg="${pos.id}" data-tip="${escapeHtml(tipSetPri)}">${escapeHtml(setPriBtn)}</button>`}
        </div>
      </td>
    </tr>`;
  }).join("");
}

function renderSkillsAndStyle(d) {
  document.getElementById("p-playing-style").value = String(d.playing_style);
  updatePlayingStyleInfo();

  const skillCats = [
    { id: "dribble", title: T("Drible & Controle", "Dribbling & Control") },
    { id: "shoot", title: T("Finalização & Chutes", "Shooting & Finishing") },
    { id: "pass", title: T("Passe & Cruzamento", "Passing & Crossing") },
    { id: "def", title: T("Defesa & Físico", "Defending & Physical") },
    { id: "gk_mental", title: T("Goleiro, Bola Parada & Mental", "Goalkeeping, Set Pieces & Mental") },
  ];

  let skCount = 0;
  document.getElementById("p-skills-grid").innerHTML = skillCats.map(cat => {
    const items = META.skills.filter(s => (s.cat || "gk_mental") === cat.id);
    if (!items.length) return "";
    const chips = items.map(s => {
      const on = !!d.skills[s.key];
      if (on) skCount++;
      const name = LANG === "en" ? s.en : s.pt;
      const tip = LANG === "en" ? (s.desc_en || "") : (s.desc_pt || "");
      return `<button class="skill-chip ${on ? "on" : ""}" type="button" data-skill-key="${escapeHtml(s.key)}" data-skill-next="${on ? "0" : "1"}" data-tip-title="${escapeHtml(name)}" data-tip="${escapeHtml(tip)}">${escapeHtml(name)}</button>`;
    }).join("");
    return `<div class="skill-cat-block"><div class="skill-cat-title">${escapeHtml(cat.title)}</div><div class="chip-grid">${chips}</div></div>`;
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
    const name = LANG === "en" ? s.en : s.pt;
    const tip = LANG === "en" ? (s.desc_en || "") : (s.desc_pt || "");
    return `<button class="skill-chip ${on ? "on" : ""}" type="button" data-skill-key="${escapeHtml(s.key)}" data-skill-next="${on ? "0" : "1"}" data-tip-title="${escapeHtml(name)}" data-tip="${escapeHtml(tip)}">${escapeHtml(name)}</button>`;
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

  const q = normText(document.getElementById("squad-search").value).trim();
  let rows = SQUAD_DATA.filter(p => {
    if (SQUAD_FILTER === "exp" && !p.is_expiring) return false;
    if (SQUAD_FILTER === "inj" && p.injury <= 0) return false;
    if (SQUAD_FILTER === "u23" && p.age > 23) return false;
    if (q && !normText(p.name).includes(q)) return false;
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
  const cPlayer = T("ATLETA", "PLAYER");
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
    { label: T("+1 OVR Geral (Mantendo DNA)", "+1 Overall (Keep DNA)"), cat: cPlayer, run: () => stepPlayerOvrDNA(1) },
    { label: T("-1 OVR Geral (Mantendo DNA)", "-1 Overall (Keep DNA)"), cat: cPlayer, run: () => stepPlayerOvrDNA(-1) },
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
  const q = normText(query).trim();
  const list = getCommands().filter(c => !q || normText(c.label).includes(q) || normText(c.cat).includes(q));
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

  applyFont(CUR_FONT);
  applyLanguage();
  initTooltips();

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

  // Player Hero & Attributes DNA OVR controls (-1 OVR, +1 OVR, Target OVR, + Favoritos, All 99 2-click confirm)
  document.getElementById("btn-hero-ovr-m1").addEventListener("click", () => stepPlayerOvrDNA(-1));
  document.getElementById("btn-hero-ovr-p1").addEventListener("click", () => stepPlayerOvrDNA(1));
  document.getElementById("btn-stats-ovr-m1").addEventListener("click", () => stepPlayerOvrDNA(-1));
  document.getElementById("btn-stats-ovr-p1").addEventListener("click", () => stepPlayerOvrDNA(1));
  document.getElementById("btn-stats-target-ovr").addEventListener("click", setTargetOvrDNA);
  document.getElementById("inp-stats-target-ovr").addEventListener("keydown", e => {
    if (e.key === "Enter") { e.preventDefault(); setTargetOvrDNA(); }
  });
  let all99Timer = null;
  const all99Btn = document.getElementById("btn-stats-all99");
  all99Btn.addEventListener("click", async () => {
    if (!all99Btn.dataset.armed) {
      all99Btn.dataset.armed = "1";
      all99Btn.textContent = T("Confirmar Tudo 99?", "Confirm All 99?");
      all99Btn.classList.add("btn-red");
      clearTimeout(all99Timer);
      all99Timer = setTimeout(() => {
        delete all99Btn.dataset.armed;
        all99Btn.textContent = tKey("btn_all_99");
        all99Btn.classList.remove("btn-red");
      }, 3000);
      return;
    }
    delete all99Btn.dataset.armed;
    clearTimeout(all99Timer);
    all99Btn.textContent = tKey("btn_all_99");
    all99Btn.classList.remove("btn-red");
    await presetStats(99);
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

  // High-Precision Damped Drag for all .studio-range sliders (9px/step normal, 18px/step with Shift)
  document.addEventListener("pointerdown", e => {
    const range = e.target && e.target.closest ? e.target.closest("input[type='range'].studio-range") : null;
    if (!range || e.button !== 0) return;
    e.preventDefault();
    hideTooltip();

    const min = Number(range.min || 0);
    const max = Number(range.max || 100);
    const rect = range.getBoundingClientRect();
    const origVal = Number(range.value || min);
    const curFrac = max > min ? (origVal - min) / (max - min) : 0;
    const thumbX = rect.left + curFrac * rect.width;

    // Clicking far from the thumb jumps softly near the click; grabbing near the thumb keeps exact value for fine drag
    let baseVal = origVal;
    if (Math.abs(e.clientX - thumbX) > 22 && rect.width > 0) {
      const clickFrac = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
      baseVal = Math.round(min + clickFrac * (max - min));
    }

    const startX = e.clientX;
    let curVal = baseVal;
    const isStat = !!range.dataset.statFid;
    const isStam = range.id === "inp-p-stam";
    const fid = isStat ? Number(range.dataset.statFid) : -1;

    const applyPreview = v => {
      if (isStat) {
        previewStatUI(fid, v);
      } else if (isStam) {
        setRangeVisual(range, v);
        const sNum = document.getElementById("inp-p-stam-num");
        if (sNum) {
          sNum.value = String(v);
          sNum.className = `stat-num-inp ${ovrClass(v)}`;
        }
      } else {
        setRangeVisual(range, v);
      }
    };

    range.classList.add("dragging");
    range.focus();
    applyPreview(curVal);

    const onMove = ev => {
      const pxPerUnit = ev.shiftKey ? 18 : 9;
      const steps = Math.round((ev.clientX - startX) / pxPerUnit);
      const next = Math.max(min, Math.min(max, baseVal + steps));
      if (next !== curVal) {
        curVal = next;
        applyPreview(curVal);
      }
    };

    const onUp = async () => {
      range.classList.remove("dragging");
      window.removeEventListener("pointermove", onMove);
      window.removeEventListener("pointerup", onUp);
      window.removeEventListener("pointercancel", onUp);
      if (curVal !== origVal) {
        if (isStat) {
          await onStatChange(fid, curVal);
        } else if (isStam && api()) {
          const r = await api().set_quick_condition(curVal, null, false, null);
          if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); }
          else toast(fmtCodeMsg(r), "err");
        }
      }
    };

    window.addEventListener("pointermove", onMove);
    window.addEventListener("pointerup", onUp);
    window.addEventListener("pointercancel", onUp);
  });

  // Attributes keyboard/direct-input & press-and-hold +/- handlers
  const statsBox = document.getElementById("stats-groups-container");
  statsBox.addEventListener("input", e => {
    const range = e.target.closest("input[data-stat-fid]");
    if (range) {
      previewStatUI(Number(range.dataset.statFid), Number(range.value));
      return;
    }
    const num = e.target.closest("input[data-stat-num]");
    if (num) {
      delete num.dataset.enterHandled;
      const v = Number(num.value);
      if (!Number.isNaN(v) && v >= 40 && v <= 99) {
        previewStatUI(Number(num.dataset.statNum), v);
      }
    }
  });
  statsBox.addEventListener("change", e => {
    const range = e.target.closest("input[data-stat-fid]");
    if (range) return onStatChange(Number(range.dataset.statFid), Number(range.value));
    const num = e.target.closest("input[data-stat-num]");
    if (num) {
      if (num.dataset.enterHandled === "1") {
        delete num.dataset.enterHandled;
        return;
      }
      const fid = Number(num.dataset.statNum);
      const raw = num.value.trim();
      const fallback = Number(num.dataset.origVal || 40);
      const v = raw === "" ? fallback : Number(raw);
      num.dataset.origVal = String(Math.max(40, Math.min(99, Math.round(v || fallback))));
      onStatChange(fid, v);
    }
  });
  statsBox.addEventListener("focusin", e => {
    const num = e.target.closest("input[data-stat-num]");
    if (num) {
      num.dataset.origVal = num.value;
      delete num.dataset.enterHandled;
      num.select();
    }
  });
  statsBox.addEventListener("keydown", e => {
    const num = e.target.closest("input[data-stat-num]");
    if (!num) return;
    const fid = Number(num.dataset.statNum);
    if (e.key === "Enter") {
      e.preventDefault();
      const raw = num.value.trim();
      const v = raw === "" ? Number(num.dataset.origVal || 40) : Number(raw);
      num.dataset.enterHandled = "1";
      num.dataset.origVal = String(Math.max(40, Math.min(99, Math.round(v || 40))));
      onStatChange(fid, v);
      num.blur();
    } else if (e.key === "Escape") {
      e.preventDefault();
      const orig = Number(num.dataset.origVal || 40);
      num.dataset.enterHandled = "1";
      previewStatUI(fid, orig);
      num.value = String(orig);
      num.blur();
    } else if ((e.key === "ArrowUp" || e.key === "ArrowDown") && e.shiftKey) {
      e.preventDefault();
      const step = e.key === "ArrowUp" ? 5 : -5;
      const nv = Math.max(40, Math.min(99, (Number(num.value) || 40) + step));
      num.value = String(nv);
      num.dataset.origVal = String(nv);
      onStatChange(fid, nv);
    }
  });

  // Press-and-hold +/- buttons (single click = 1 step, hold = smooth repeat with 1 undo commit on release)
  statsBox.addEventListener("pointerdown", e => {
    const btn = e.target.closest("button[data-stat-dec], button[data-stat-inc]");
    if (!btn || !CURRENT_PLAYER || e.button !== 0) return;
    e.preventDefault();
    const isDec = btn.hasAttribute("data-stat-dec");
    const fid = Number(isDec ? btn.dataset.statDec : btn.dataset.statInc);
    const delta = isDec ? -1 : 1;
    let cur = Number(CURRENT_PLAYER.stats[String(fid)] || 40);
    const startVal = cur;

    const tick = () => {
      const next = Math.max(40, Math.min(99, cur + delta));
      if (next !== cur) {
        cur = next;
        previewStatUI(fid, cur);
      }
    };
    tick();

    let holdTimer = setTimeout(() => {
      holdTimer = setInterval(tick, 85);
    }, 360);

    const finish = () => {
      clearTimeout(holdTimer);
      clearInterval(holdTimer);
      window.removeEventListener("pointerup", finish);
      window.removeEventListener("pointercancel", finish);
      if (cur !== startVal) onStatChange(fid, cur);
    };
    window.addEventListener("pointerup", finish);
    window.addEventListener("pointercancel", finish);
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
      const pid = Number(aptBtn.dataset.posApt);
      const g = Number(aptBtn.dataset.grade);
      const prevPos = CURRENT_PLAYER && CURRENT_PLAYER.positions.find(x => x.id === pid);
      const oldG = prevPos ? prevPos.apt : 0;
      const oldO = prevPos ? prevPos.ovr : 0;
      const r = await api().set_position_aptitude(pid, g);
      if (r && r.ok) {
        CURRENT_PLAYER = r;
        if (oldG !== g) {
          const gn = ["C", "B", "A"];
          const np = r.positions.find(x => x.id === pid);
          recordPosLabMeta(pid, oldO, np ? np.ovr : oldO, `Grau ${gn[oldG]} -> ${gn[g]}`, `Grade ${gn[oldG]} -> ${gn[g]}`);
        }
        renderPlayerDetails(r);
      } else toast(fmtCodeMsg(r), "err");
      return;
    }
    const stepBtn = e.target.closest("[data-pos-step]");
    if (stepBtn) {
      HIGHLIGHTED_POS = Number(stepBtn.dataset.posStep);
      const dl = Number(stepBtn.dataset.delta);
      const r = await api().step_position_ovr(HIGHLIGHTED_POS, dl);
      if (r && r.ok) {
        CURRENT_PLAYER = r;
        recordPosLabAction(r, dl > 0 ? "+1 OVR" : "-1 OVR", dl > 0 ? "+1 OVR" : "-1 OVR");
        renderPlayerDetails(r);
      } else toast(fmtCodeMsg(r), "err");
      return;
    }
    const matchBtn = e.target.closest("[data-pos-match]");
    if (matchBtn) {
      HIGHLIGHTED_POS = Number(matchBtn.dataset.posMatch);
      const r = await api().match_position_to_primary(HIGHLIGHTED_POS);
      if (r && r.ok) {
        CURRENT_PLAYER = r;
        recordPosLabAction(r, "Igualar Principal", "Match Primary");
        renderPlayerDetails(r);
      } else toast(fmtCodeMsg(r), "err");
      return;
    }
    const regBtn = e.target.closest("[data-pos-reg]");
    if (regBtn) {
      const oldReg = CURRENT_PLAYER ? CURRENT_PLAYER.reg_pos : -1;
      const pid = Number(regBtn.dataset.posReg);
      const prevPos = CURRENT_PLAYER && CURRENT_PLAYER.positions.find(x => x.id === pid);
      HIGHLIGHTED_POS = pid;
      const r = await api().set_registered_position(HIGHLIGHTED_POS);
      if (r && r.ok) {
        CURRENT_PLAYER = r;
        const np = r.positions.find(x => x.id === pid);
        recordPosLabMeta(pid, prevPos ? prevPos.ovr : 0, np ? np.ovr : 0, `Principal: ${posShort(oldReg)} -> ${posShort(pid)}`, `Primary: ${posShort(oldReg)} -> ${posShort(pid)}`);
        renderPlayerDetails(r);
        pulseOvrShield();
        const el = fmtEconDeltaLine(r.econ_delta);
        const baseMsg = T("Posição principal alterada!", "Primary position updated!");
        toast(el ? `${baseMsg} | ${el}` : baseMsg);
      } else toast(fmtCodeMsg(r), "err");
    }
  });

  // Pitch orientation toggle & Attribute Diff Log controls
  document.getElementById("btn-pitch-orient").addEventListener("click", () => {
    PITCH_ORIENT_MODE = (PITCH_ORIENT_MODE + 1) % 3;
    if (CURRENT_PLAYER) {
      renderPositionPitch(CURRENT_PLAYER);
      renderPosLabLog();
    }
  });
  document.getElementById("btn-log-mode-steps").addEventListener("click", () => {
    POSLAB_LOG_MODE = 0;
    renderPosLabLog();
  });
  document.getElementById("btn-log-mode-net").addEventListener("click", () => {
    POSLAB_LOG_MODE = 1;
    renderPosLabLog();
  });
  document.getElementById("btn-log-action").addEventListener("click", () => {
    if (POSLAB_LOG_MODE === 0) {
      POSLAB_LOG = [];
    } else if (CURRENT_PLAYER && CURRENT_PLAYER.stats) {
      POSLAB_BASE_STATS = { ...CURRENT_PLAYER.stats };
      toast(T("Base de comparação zerada.", "Comparison baseline reset."));
    }
    renderPosLabLog();
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
      if (CURRENT_VIEW === "player") { await loadPlayerDetails(); pulseOvrShield(); }
      if (CURRENT_VIEW === "squad") loadSquad();
      if (CURRENT_VIEW === "finances") loadFinances();
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
    applyLanguage();
  });
  document.getElementById("btn-lang").addEventListener("click", () => {
    LANG = LANG === "pt" ? "en" : "pt";
    try { localStorage.setItem("pes21_studio_lang", LANG); } catch (e) {}
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
  const sqSearch = document.getElementById("squad-search");
  sqSearch.addEventListener("input", renderSquadTable);
  sqSearch.addEventListener("keydown", e => {
    if (e.key === "Escape" && sqSearch.value) {
      e.preventDefault();
      sqSearch.value = "";
      renderSquadTable();
    }
  });
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
    const v = Math.max(0, Math.min(100, Number(e.target.value) || 0));
    setRangeVisual(e.target, v);
    const sNum = document.getElementById("inp-p-stam-num");
    if (sNum) { sNum.value = String(v); sNum.className = `stat-num-inp ${ovrClass(v)}`; }
  });
  document.getElementById("inp-p-stam").addEventListener("change", async e => {
    if (!api()) return;
    const r = await api().set_quick_condition(Number(e.target.value), null, false, null);
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); }
    else toast(fmtCodeMsg(r), "err");
  });
  const stamNumInp = document.getElementById("inp-p-stam-num");
  stamNumInp.addEventListener("focus", () => stamNumInp.select());
  stamNumInp.addEventListener("change", async () => {
    if (!api()) return;
    const v = Math.max(0, Math.min(100, Math.round(Number(stamNumInp.value) || 0)));
    const r = await api().set_quick_condition(v, null, false, null);
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); }
    else toast(fmtCodeMsg(r), "err");
  });
  stamNumInp.addEventListener("keydown", e => {
    if (e.key === "Enter") { e.preventDefault(); stamNumInp.blur(); }
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
      has_release_fee: document.getElementById("bio-has-rf") ? document.getElementById("bio-has-rf").checked : undefined,
    });
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast(T("Dados biográficos salvos!", "Bio saved!")); }
    else toast(fmtCodeMsg(r), "err");
  });

  // OVR-Linked Economy Sync Selector & Recalibrate button
  const selEcon = document.getElementById("sel-econ-sync");
  if (selEcon) {
    selEcon.value = String(ECON_SYNC_MODE);
    selEcon.addEventListener("change", async e => {
      ECON_SYNC_MODE = Math.max(0, Math.min(3, Number(e.target.value) || 0));
      try { localStorage.setItem("pes21_studio_econ_sync", String(ECON_SYNC_MODE)); } catch (err) {}
      const lblEcon = document.getElementById("lbl-econ-sync");
      if (lblEcon) lblEcon.classList.toggle("off", ECON_SYNC_MODE === 0);
      if (api()) await api().set_econ_sync_mode(ECON_SYNC_MODE);
    });
  }
  const btnRecal = document.getElementById("btn-recal-econ");
  if (btnRecal) {
    btnRecal.addEventListener("click", async () => {
      if (!api() || !CURRENT_PLAYER) return toast(tKey("no_player"), "err");
      const r = await api().recalibrate_player_economy(CURRENT_PLAYER.addr);
      if (r && r.ok) {
        CURRENT_PLAYER = r;
        renderPlayerDetails(r);
        const el = fmtEconDeltaLine(r.econ_delta);
        toast(el ? `${T("Economia recalibrada!", "Economy recalibrated!")} | ${el}` : T("Economia recalibrada!", "Economy recalibrated!"));
      } else toast(fmtCodeMsg(r), "err");
    });
  }
  document.getElementById("p-playing-style").addEventListener("change", async e => {
    updatePlayingStyleInfo();
    if (!api()) return;
    const r = await api().set_playing_style(Number(e.target.value));
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast(T("Estilo de jogo atualizado!", "Playing style updated!")); }
    else toast(fmtCodeMsg(r), "err");
  });
  document.getElementById("p-team-role").addEventListener("change", updateTeamRoleInfo);
  document.getElementById("btn-save-role").addEventListener("click", async () => {
    if (!api()) return;
    const r = await api().set_team_role(
      Number(document.getElementById("p-team-role").value),
      Number(document.getElementById("p-team-role-lvl").value)
    );
    if (r && r.ok) { CURRENT_PLAYER = r; renderPlayerDetails(r); toast(T("Função no time salva!", "Team role saved!")); }
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
  ["fin-transfer-inp", "fin-salary-inp"].forEach(id => {
    document.getElementById(id).addEventListener("keydown", e => {
      if (e.key === "Enter") { e.preventDefault(); document.getElementById("btn-fin-save").click(); }
    });
  });
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
  document.getElementById("bal-focus-inp").addEventListener("keydown", e => {
    if (e.key === "Enter") { e.preventDefault(); document.getElementById("btn-bal-apply").click(); }
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
    const rawPts = document.getElementById("bal-focus-inp").value.trim();
    const pts = rawPts === "" ? 21 : Number(rawPts);
    const lck = document.getElementById("bal-lock-chk").checked;
    const r = await api().set_bal_focus(pts, lck);
    if (r && r.ok) {
      BAL_DIRTY = false;
      document.getElementById("bal-unsaved").style.display = "none";
      toast(T(`Pontos de foco BAL: ${r.points}${r.lock ? " (Travado)" : ""}`, `BAL Focus Points: ${r.points}${r.lock ? " (Locked)" : ""}`));
    } else toast(fmtCodeMsg(r), "err");
  });

  window.addEventListener("resize", () => {
    if (CURRENT_PLAYER) {
      renderPositionPitch(CURRENT_PLAYER);
      renderPosLabLog();
    }
  });

  if (api()) {
    try { await api().set_econ_sync_mode(ECON_SYNC_MODE); } catch (e) {}
  }
  await pollStatus();
  setInterval(pollStatus, 600);
}

window.addEventListener("pywebviewready", bootstrap);
