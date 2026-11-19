# PES 2021 — Cheat Table v11.0 Ultimate Studio (PT-BR & EN)

![Version](https://img.shields.io/badge/Version-v11.0_Ultimate_Studio-38BDF8?style=for-the-badge)
![PES 2021](https://img.shields.io/badge/eFootball_PES_2021-v1.7.2.0_(DP_7.0)-0095D9?style=for-the-badge)
![Editions](https://img.shields.io/badge/Editions-PT--BR_%7C_English-2EA043?style=for-the-badge)
![Sider Compatible](https://img.shields.io/badge/Sider_7.3_%2F_BMPES_16.0-100%25_Compatible-F59E0B?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-8B5CF6?style=for-the-badge)

> **[Português (PT-BR)](#português-pt-br)** &nbsp;•&nbsp; **[English](#english)** &nbsp;•&nbsp; **[Build & QA](#arquitetura-build--qa)**

---

## Português (PT-BR)

A **Cheat Table v11.0 Ultimate Studio** para **eFootball PES 2021 Season Update (`v1.7.2.0`)**, construída via engenharia reversa nativa x64, bilíngue (**Português PT-BR** e **Inglês EN**) e projetada para operar com **zero conflito com Sider 7.3+ e grandes patches** (**BMPES 16.0**, **SmokePatch / Football Life**, **VirtuaRED**, **Dream Patch**).

### Download & Início Rápido

1. Baixe a edição no seu idioma na pasta [`dist/`](dist):
   - **Português (PT-BR)**: [`PES 2021 - v11.0 Ultimate Studio (PT-BR).CT`](dist/PES%202021%20-%20v11.0%20Ultimate%20Studio%20(PT-BR).CT)
   - **English (EN)**: [`PES 2021 - v11.0 Ultimate Studio (EN).CT`](dist/PES%202021%20-%20v11.0%20Ultimate%20Studio%20(EN).CT)
2. Abra o **PES 2021** e abra o arquivo `.CT` no **Cheat Engine 7.2+ (64-bit)** (clique em **Yes / Sim** para autorizar o motor Lua).
3. **Zero Cliques Manuais**: faz **Auto-Attach** no `PES2021.exe`, inicia em **Euro (`€`)** e **ativa automaticamente todos os 6 Hooks Mestres**.
4. Clique em **`[ ★ ABRIR PAINEL STUDIO ]`** para abrir a interface gráfica *double-buffered* (com troca instantânea **`[ PT | EN ]`** e moedas **`€ / R$ / $ / £`** no cabeçalho).

### Principais Recursos

| Módulo | O que faz na prática |
|---|---|
| **Laboratório de Posições (`+1 OVR Isolado` & `Igualar`)** | Usa a matriz oficial `23 × 13` da Konami (`fcn.141e640a0`) para isolar atributos exclusivos de uma posição secundária — permite subir um **MAT 88** para **MLG 88** **sem subir (ou com impacto mínimo)** o OVR de MAT! Inclui prévia ao vivo dos atributos isolados, **`Igualar OVR à Principal`**, **`Tornar Principal`** e edição de Grau **`A / B / C`** (ou promover todas `B → A`). |
| **Motor Proporcional de Overall (`OVR DNA`)** | **`+1 OVR DNA`**, **`-1 OVR DNA`**, **`OVR Alvo (40–99)`**, **`Elenco +1 OVR DNA`** e **Raio-X de 13 Posições & DNA** — evolui o jogador cruzando atributos proporcionalmente sem descaracterizar seu estilo de jogo. |
| **Desfazer Automático (`Undo` de 10 Níveis)** | Salva automaticamente um snapshot completo (`0x17C` bytes) antes de cada edição no atleta, permitindo reverter em 1 clique pelo botão **`Desfazer (N)`** no Studio GUI, nos modais ou na árvore do CE. |
| **Central de Olheiros na RAM Real (`fcn.140efe53f`)** | Escreve e gerencia diretamente na RAM da Master League os **3 Olheiros (`30` vagas cada)** e a **Shortlist (`100` vagas)**, com prévia ao vivo, remoção individual e **Negociação Irrecusável** (Lista de Transferência + Multa `€ 1.000` + Afeição `100%` + Destravar chegada). |
| **Função de Equipe (`22 Team Roles`) & Longevidade** | Seletor visual clicável com todas as **22 funções** + **Nível `1–3`** e 5 presets de 1 clique (*Lenda*, *Bandeira*, *Maestro*, *General*, *Superestrela*), além de **Auge (Breakthrough) + Anti-Declínio 30+** e **Rejuvenescer (-2 Anos) com sincronia de valor**. |
| **Finanças, Contratos & Multa Exata (`Hook 2`)** | Formatação automática em `€`, `R$`, `$` ou `£` com entrada natural (`50M`, `500k`, `1.5B`), override exato de **Taxa de Rescisão** para até 64 atletas simultâneos e edição de bônus salariais. |
| **Treino Sub-XP Seletivo & Suite BAL (`Hooks 3 & 4`)** | Multiplicador Sub-XP (`2x` a `50x` e `Instantâneo 10.000 XP/dia`) restrito aos 82 slots do usuário (Clube ML, Base e BAL), teto de **99 Focus Points** destravado e conclusão instantânea de Skill/Posição em treino. |
| **Partida ao Vivo Sider-Safe (`Hooks 5 & 6`)** | Pulo para minuto exato sem crash FNV-1a (`15'`, `44'`, `88'`), congelamento de relógio, placar/pênaltis sincronizados e **Stamina Seletiva** (Só Casa, Só Fora, Ambos ou Pressão Total). |

---

## English

The **v11.0 Ultimate Studio Cheat Table** for **eFootball PES 2021 Season Update (`v1.7.2.0`)** — natively bilingual (**English** & **Portuguese PT-BR**), built via x64 reverse engineering, and engineered for **100% compatibility with Sider 7.3+ and major community patches** (**SmokePatch / Football Life**, **VirtuaRED**, **Dream Patch**, **BMPES 16.0**).

### Quick Download & Usage

1. Download your standalone edition from [`dist/`](dist):
   - **English (EN)**: [`PES 2021 - v11.0 Ultimate Studio (EN).CT`](dist/PES%202021%20-%20v11.0%20Ultimate%20Studio%20(EN).CT)
   - **Português (PT-BR)**: [`PES 2021 - v11.0 Ultimate Studio (PT-BR).CT`](dist/PES%202021%20-%20v11.0%20Ultimate%20Studio%20(PT-BR).CT)
2. Launch **PES 2021** and open the `.CT` file in **Cheat Engine 7.2+ (64-bit)** (click **Yes** when prompted to run the table Lua engine).
3. **Zero-Click Setup**: auto-attaches to `PES2021.exe`, defaults to **Euro (`€`)**, and **auto-arms all 6 Master Hooks** safely.
4. Click **`[ ★ OPEN PRO STUDIO GUI ]`** for the double-buffered Studio Dashboard (with live **`[ PT | EN ]`** and currency **`€ / R$ / $ / £`** controls in the header).

### Key Highlights

- **Visual Position Lab (`+1 Isolated OVR` & `Match Primary OVR`)**: Uses Konami's decompiled `23 × 13` rating weight matrix (`fcn.141e640a0`) to isolate attributes specific to a secondary position — letting you elevate an **AMF 88** to **CMF 88** with **zero (or minimal) primary OVR drift**, complete with live isolated-stat preview, **Set as Primary Position**, and **Grade `A / B / C`** editing.
- **Konami Exact `OVR` DNA Engine & 10-Level Auto `Undo`**: **`+1/-1 OVR DNA`**, **`Target OVR (40–99)`**, **`Squad +1 OVR DNA`**, **13-Position & DNA X-Ray**, and automatic 10-level full-struct (`0x17C`) **Undo** history right in the Studio Hero bar.
- **Real-RAM Scout Observation Hub (`fcn.140efe53f`)**: Directly reads & writes Master League's real in-game **Scouts 1–3 (`30` slots each)** and **Shortlist (`100` slots)** in RAM, paired with **Irresistible Deal** (Transfer Listed + `€ 1,000` Release Clause + `100%` Affection + Recent-Arrival Unlock) and an optional **Radar 30K** filter modal.
- **Visual Team Role Picker (`22 Roles`) & Longevity**: Interactive picker for all **22 Team Roles** + **Level `1–3`** with 5 one-click presets (*Legend*, *Bandiera*, *Maestro*, *General*, *Superstar*), **Breakthrough Surge + Anti-Decline 30+**, and **Rejuvenate (-2 Yrs) & Market Value Sync**.
- **Human-Readable Finances, User-Only Sub-XP, BAL Suite & Sider-Safe Live Match Control**: Natural shorthand inputs (`50M`, `500k`, `1.5B`), 64-slot Exact Release Clause Override, `2x..50x` / `Instant` User-Only training, 99 BAL Focus Points, FNV-1a crash-free exact match minute jump, clock freeze, score sync, and selective Home/Away stamina.

---

## Arquitetura, Build & QA

```text
├── dist/
│   ├── PES 2021 - v11.0 Ultimate Studio (PT-BR).CT  # Standalone Portuguese Cheat Table
│   └── PES 2021 - v11.0 Ultimate Studio (EN).CT     # Standalone English Cheat Table
├── src/
│   ├── pes10_lua_engine.lua                         # Bilingual Lua 5.3 Engine, Isolated OVR Solver & Studio GUI
│   ├── build_pes2021_v10_ct.py                      # Dual-edition deterministic compiler (PT-BR & EN)
│   └── data/
│       ├── konami_ovr_tables.json                   # Exact 23×13 weight matrices & curve from PES2021.exe
│       ├── player_schema.json                       # Bitfield & dropdown definitions (150+ fields)
│       └── bmpes_dicts.json                         # Fallback club & competition dictionaries
└── tests/
    ├── test_ct_linter.py                            # Static XML, x64 alloc/dealloc & bit-overlap auditor
    └── test_lua_and_gui_dpi.py                      # Lua 5.3 Behavioral + Real Win32 GDI DPI (96/120/144) QA
```

```bash
python src/build_pes2021_v10_ct.py
python tests/test_ct_linter.py
python tests/test_lua_and_gui_dpi.py
```

### Qualidade Verificada
- **301 registros auditados** (`0` conflitos de bits, `0` vazamentos de `alloc/dealloc`, `0` caminhos pessoais).
- **Renderização Win32 GDI ClearType** validada pixel a pixel em **`100%` (96 DPI)**, **`125%` (120 DPI)** e **`150%` (144 DPI)** — `0` sobreposições e `0` textos cortados em PT-BR e EN.
- **Testes comportamentais automatizados** para `Undo`, `+1 OVR Isolado` (zero drift principal), `Igualar OVR`, `Posição Principal`, `Team Role` e `Olheiros na RAM Real`.

## License

Distributed under the [MIT License](LICENSE).
