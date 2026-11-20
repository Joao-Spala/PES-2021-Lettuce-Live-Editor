# PES 2021 - Cheat Table v11.1 Ultimate Studio (PT-BR & EN)

![Version](https://img.shields.io/badge/Version-v11.1_Ultimate_Studio-38BDF8?style=for-the-badge)
![PES 2021](https://img.shields.io/badge/eFootball_PES_2021-v1.7.2.0_(DP_7.0)-0095D9?style=for-the-badge)
![Editions](https://img.shields.io/badge/Editions-PT--BR_%7C_English-2EA043?style=for-the-badge)
![Sider Compatible](https://img.shields.io/badge/Sider_7.3_%2F_BMPES_16.0-100%25_Compatible-F59E0B?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-8B5CF6?style=for-the-badge)

> **[Português (PT-BR)](#português-pt-br)** | **[English](#english)** | **[Build & Testes](#build--testes)**

---

## Português (PT-BR)

Cheat Table bilíngue (**PT-BR** e **EN**) para **eFootball PES 2021 (`v1.7.2.0`)**, feita via engenharia reversa x64 e compatível com **Sider 7.3+** e grandes patches (**BMPES 16.0**, **SmokePatch / Football Life**, **VirtuaRED**, **Dream Patch**).

### Como usar

1. Baixe a versão no seu idioma em [`dist/`](dist):
   - **PT-BR**: [`PES 2021 - v11.1 Ultimate Studio (PT-BR).CT`](dist/PES%202021%20-%20v11.1%20Ultimate%20Studio%20(PT-BR).CT)
   - **EN**: [`PES 2021 - v11.1 Ultimate Studio (EN).CT`](dist/PES%202021%20-%20v11.1%20Ultimate%20Studio%20(EN).CT)
2. Abra o **PES 2021** e abra o `.CT` no **Cheat Engine 7.2+ (64-bit)** (aceite executar o script Lua).
3. O anexo ao `PES2021.exe` e a ativação dos **6 Hooks Mestres** acontecem automaticamente (moeda padrão: `€`).
4. Clique em **`[ ★ ABRIR PAINEL STUDIO ]`** para abrir a interface gráfica (troca de idioma `PT | EN` e moeda `€ / R$ / $ / £` no topo).

### Funcionalidades

- **Laboratório de Posições (`+1 OVR Isolado` e `Igualar OVR`)**: usa a matriz real `23x13` da Konami (`fcn.141e640a0`) para subir o OVR em uma posição secundária isolando os atributos exclusivos dela, sem subir (ou subindo o mínimo possível) o OVR da posição principal. Inclui `Igualar OVR à Principal`, `Tornar Principal` e edição de Grau `A / B / C` (ou promover todas `B -> A`).
- **Overall Proporcional (`OVR DNA`)**: `+1 OVR`, `-1 OVR`, `OVR Alvo (40-99)`, `Elenco +1 OVR` e `Raio-X de 13 Posições`, preservando a proporção e o estilo de jogo do atleta.
- **Desfazer Automático (`Undo`)**: salva até 10 snapshots completos (`0x17C` bytes) por jogador antes de cada edição, com reversão em 1 clique.
- **Olheiros na RAM Real (`fcn.140efe53f`)**: gerencia diretamente na RAM da Master League os **3 Olheiros** (30 vagas cada) e a **Shortlist** (100 vagas), além de **Negociação Irrecusável** (lista de transferência, multa `€ 1.000`, afeição 100% e destravar chegada).
- **Team Roles e Longevidade**: seletor visual com todas as **22 funções de equipe** (nível 1-3) + 5 presets rápidos (*Lenda*, *Bandeira*, *Maestro*, *General*, *Superestrela*), **Auge (Breakthrough) + Anti-Declínio 30+** e **Rejuvenescer (-2 anos)**.
- **Finanças, Sub-XP, BAL e Partida ao Vivo**: valores formatados com entrada natural (`50M`, `500k`, `1.5B`), override de multa rescisória para até 64 atletas, multiplicador de treino (`2x` a `Instantâneo`) restrito ao seu clube/base/BAL, 99 Focus Points no BAL, pulo de minuto sem crash FNV-1a, congelar relógio, placar sincronizado e Stamina seletiva.

---

## English

Bilingual (**EN** & **PT-BR**) Cheat Table for **eFootball PES 2021 (`v1.7.2.0`)**, built via x64 reverse engineering with full compatibility for **Sider 7.3+** and major patches (**SmokePatch / Football Life**, **VirtuaRED**, **Dream Patch**, **BMPES 16.0**).

### Quick Start

1. Download your edition from [`dist/`](dist):
   - **EN**: [`PES 2021 - v11.1 Ultimate Studio (EN).CT`](dist/PES%202021%20-%20v11.1%20Ultimate%20Studio%20(EN).CT)
   - **PT-BR**: [`PES 2021 - v11.1 Ultimate Studio (PT-BR).CT`](dist/PES%202021%20-%20v11.1%20Ultimate%20Studio%20(PT-BR).CT)
2. Launch **PES 2021** and open the `.CT` file in **Cheat Engine 7.2+ (64-bit)** (click **Yes** to run the Lua script).
3. Auto-attaches to `PES2021.exe` and arms all **6 Master Hooks** automatically (default currency: `€`).
4. Click **`[ ★ OPEN PRO STUDIO GUI ]`** to launch the Studio Dashboard (`PT | EN` and `€ / R$ / $ / £` toggles in the header).

### Features

- **Position Lab (`+1 Isolated OVR` & `Match Primary OVR`)**: uses Konami's `23x13` weight matrix (`fcn.141e640a0`) to boost secondary position OVR via exclusive attributes with zero (or minimal) primary OVR drift. Includes `Match Primary OVR`, `Set as Primary`, and `A / B / C` mastery editing.
- **DNA-Preserving Overall (`OVR DNA`)**: `+1 OVR`, `-1 OVR`, `Target OVR (40-99)`, `Squad +1 OVR`, and `13-Position X-Ray` without breaking playing style proportions.
- **10-Level Auto `Undo`**: automatic full-struct (`0x17C`) snapshots per player before every edit, with 1-click revert.
- **Real-RAM Scout Hub (`fcn.140efe53f`)**: directly reads/writes Master League **Scouts 1-3** (30 slots each) and **Shortlist** (100 slots) in RAM, plus **Irresistible Deal** (Transfer Listed, `€ 1,000` clause, 100% affection, arrival unlock).
- **Team Roles & Longevity**: visual picker for all **22 Team Roles** (Level 1-3) + 5 quick presets (*Legend*, *Bandiera*, *Maestro*, *General*, *Superstar*), **Breakthrough + Anti-Decline 30+**, and **Rejuvenate (-2 yrs)**.
- **Finances, Sub-XP, BAL & Sider-Safe Match Control**: shorthand inputs (`50M`, `500k`, `1.5B`), 64-slot Exact Release Clause Override, user-only training multiplier (`2x` to `Instant`), 99 BAL Focus Points, FNV-1a crash-free exact minute jump, clock freeze, live score sync, and selective stamina.

---

## Build & Testes

```bash
python src/build_pes2021_v10_ct.py
python tests/test_ct_linter.py
python tests/test_lua_and_gui_dpi.py
```

## License

[MIT](LICENSE)
