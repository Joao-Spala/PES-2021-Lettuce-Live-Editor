<div align="center">

<img src="docs/assets/logo.png" alt="PES 2021 - Lettuce Live Editor" width="144" height="144">

# PES 2021 - Lettuce Live Editor

**Editor de carreira em tempo real para o eFootball PES 2021**<br>
**Real-time career editor for eFootball PES 2021**

![Native C++20](https://img.shields.io/badge/Native-C%2B%2B20_%2B_DX11_%2B_ImGui-84CC16?style=for-the-badge)
![PES 2021](https://img.shields.io/badge/eFootball_PES_2021-v1.7.2.0_(DP_7.0)-0095D9?style=for-the-badge)
![Languages](https://img.shields.io/badge/Languages-PT--BR_%7C_English-2EA043?style=for-the-badge)
![Patches](https://img.shields.io/badge/Sider_7.3_%2F_BMPES_%2F_Football_Life-Compatible-F59E0B?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-8B5CF6?style=for-the-badge)

**[Português (PT-BR)](#português-pt-br)** · **[English](#english)** · **[Build & Testes / Build & Tests](#build--testes--build--tests)**

*Antes conhecido como PES 2021 Workbench. Formerly known as PES 2021 Workbench.*

</div>

---

## Português (PT-BR)

Aplicativo desktop nativo em **C++20 + DirectX 11 + Dear ImGui** (cerca de 2,6 MB, standalone, sem dependências externas) para edição e gerenciamento de carreira em tempo real no **eFootball PES 2021 (`v1.7.2.0`)** e derivados (**Football Life 2023-2026**, **BMPES**, **SmokePatch**, **VirtuaRED**, **Dream Patch**), compatível com o **Sider 7.3+**.

Não requer Cheat Engine nem runtimes externos. Abra o jogo e execute o `PES 2021 Workbench Native.exe` (o nome do arquivo foi mantido para não quebrar atalhos).

### Como usar

1. Compile o executável ou use o binário em `dist/PES 2021 Workbench Native.exe`.
2. Abra o **PES 2021** (ou o **Football Life**) e entre na **Master League** ou no **Rumo ao Estrelato (BAL)**.
3. Execute o Lettuce Live Editor, como Administrador se o jogo também estiver elevado.
4. A conexão com o processo e a captura automática do atleta em foco acontecem em segundo plano.

### Funcionalidades

| Área | O que faz |
| --- | --- |
| **Telas** | 8 telas e 5 sub-abas de atleta: `Visão Geral`, `Meu Elenco`, `Finanças`, `Jogador Atual` (`Bio & Contrato`, `Atributos`, `Lab de Posições`, `Estilo & Habilidades`, `Função no Time`), `Buscar Jogador (30k+)`, `Olheiros & Favoritos`, `Partida Ao Vivo` e `Treino & BAL`. |
| **Motor de OVR** | Motor de OVR Konami exato e solver DNA: `+1 OVR`, `-1 OVR` e `OVR alvo (40-99)` preservando o perfil natural de atributos. 30.000 cálculos em cerca de 2,5 ms. |
| **Lab de Posições** | Mapa tático de 13 posições, ajuste isolado `+1/-1` em posições secundárias sem mexer na principal, `Igualar`, `Principal` e Grau `A / B / C`. |
| **Economia** | Valor de mercado, salário e multa Konami sincronizados com OVR, idade, posição, nacionalidade e versatilidade, mais card de contrato rápido. |
| **Setas de forma** | As 5 setas de forma e a flag independente de seta piscando (bit 7 do byte `0x146`), com pulso animado nas tabelas e descrição baseada no manual oficial da Konami. |
| **Elenco em lote** | Seleção por filtro ou marcação para recalibrar economia, `+1 OVR (DNA)`, fôlego, cura, contrato e seta. Antes de lotes que mexem em valores o app salva um backup JSON do elenco em `Documentos/PES 2021 Workbench/Packs/Backups`. |
| **Presets de habilidades** | 10 funções em campo (finalizador, ponta driblador, meia criativo, regista, zagueiro, goleiro e outras), respeitando o limite nativo de 10 habilidades. |
| **Histórico** | Linha do tempo global com desfazer até qualquer ponto, além do `Ctrl+Z` por atleta. |
| **Olheiros & Favoritos** | Cartões das 4 listas com estatísticas, filtros, mover, copiar e remover em lote, desfazer nas listas (12 passos), exportar e importar em JSON cruzando por ID e nome, e marcação direta pela Busca e pelo editor do atleta. |
| **Partida Ao Vivo** | Detecção automática, minuto e período ao vivo, tempo exato, ajustes rápidos, congelar cronômetro e 6 modos de fôlego seletivo. |
| **Pack Studio (JSON v13)** | Exporta e importa atletas e elencos entre saves, com cruzamento por ID, nome ou escolha manual, prévia de diferenças e rollback do lote. |
| **Treino & BAL** | Multiplicador de Sub-XP do seu clube e desbloqueio dos Pontos de Foco do BAL. |

### Identidade visual e interface

- **Marca:** nome Lettuce Live Editor e logo de alface, o mesmo em todos os temas, no ícone do executável e na barra de título.
- **Dois temas de cor:** `Azul` (azul-marinho, padrão, com destaque verde Alface) e `Alface` (verde-floresta). A troca é ao vivo, em `Ajustes`, e fica salva.
- **Ajustes:** sempre no topo, escala da interface (90% a 150%), 6 cores de destaque, tamanho e posição da janela lembrados.
- **Extras:** ícones vetoriais próprios, Per-Monitor DPI, radar hexagonal, idioma `PT-BR / EN`, moeda `EUR / BRL / USD / GBP`, 7 fontes, Command Palette (`Ctrl+K`) e histórico de navegação (`Alt+Left`).

---

## English

Native standalone desktop application built with **C++20 + DirectX 11 + Dear ImGui** (about 2.6 MB, no runtime dependencies) for real-time career editing in **eFootball PES 2021 (`v1.7.2.0`)** and derivatives (**Football Life 2023-2026**, **BMPES**, **SmokePatch**, **VirtuaRED**, **Dream Patch**), compatible with **Sider 7.3+**.

No Cheat Engine or external runtimes required. Launch the game and run `PES 2021 Workbench Native.exe` (the file name was kept so existing shortcuts keep working).

### Quick Start

1. Build the executable or launch `dist/PES 2021 Workbench Native.exe`.
2. Start **PES 2021** (or **Football Life**) and load your **Master League** or **Become a Legend (BAL)** save.
3. Run Lettuce Live Editor, as Administrator if the game is elevated.
4. Process attachment and active player capture run automatically in the background.

### Features

| Area | What it does |
| --- | --- |
| **Views** | 8 views and 5 player subtabs: `Overview`, `My Squad`, `Finances`, `Current Player` (`Bio & Contract`, `Attributes`, `Position Lab`, `Style & Skills`, `Team Role`), `Search Player (30k+)`, `Scouts & Shortlist`, `Live Match` and `Training & BAL`. |
| **OVR engine** | Exact Konami OVR engine and DNA solver: `+1 OVR`, `-1 OVR` and `Target OVR (40-99)` while preserving each player's stat DNA. 30,000 calculations take about 2.5 ms. |
| **Position Lab** | 13-position tactical pitch, isolated `+1/-1` on secondary positions without moving the primary one, `Match`, `Primary` and `A / B / C` grades. |
| **Economy** | Market value, salary and Konami release clause synced with OVR, age, position, nationality and versatility, plus a quick contract card. |
| **Form arrows** | The 5 form arrows and the independent blinking-arrow flag (bit 7 of byte `0x146`), with an animated pulse in tables and a description based on Konami's official manual. |
| **Batch squad tools** | Pick by filter or by ticking, then recalibrate economy, `+1 OVR (DNA)`, stamina, heal, contract and arrow. Before value-changing batches the app saves a JSON squad backup in `Documents/PES 2021 Workbench/Packs/Backups`. |
| **Skill presets** | 10 on-pitch roles (poacher, dribbling winger, playmaker, deep-lying playmaker, centre-back, goalkeeper and more), respecting the native cap of 10 skills. |
| **History** | A global timeline with undo to any point, besides per-player `Ctrl+Z`. |
| **Scouts & Shortlist** | Cards for the 4 lists with stats, filters, batch move, copy and remove, list-level undo (12 steps), JSON export and import matched by ID and name, and direct tagging from Search and from the player editor. |
| **Live Match** | Automatic detection, live minute and period, exact clock, quick shifts, clock freeze and 6 selective stamina modes. |
| **Pack Studio (JSON v13)** | Export and import players and squads across saves with matching by ID, name or manual pick, diff preview and batch rollback. |
| **Training & BAL** | User-club Sub-XP multiplier and BAL Focus Points unlock. |

### Visual identity and interface

- **Brand:** the Lettuce Live Editor name and lettuce logo are the same in every theme, in the executable icon and in the title bar.
- **Two colour themes:** `Blue` (navy, default, with the Lettuce green accent) and `Lettuce` (forest green). Switching is live, in `Settings`, and persisted.
- **Settings:** always on top, UI scale (90% to 150%), 6 accent colours, remembered window size and position.
- **Extras:** own vector icons, Per-Monitor DPI, hexagonal radar, `PT-BR / EN`, currency `EUR / BRL / USD / GBP`, 7 fonts, Command Palette (`Ctrl+K`) and navigation history (`Alt+Left`).

---

## Build & Testes / Build & Tests

Requer / Requires `clang++` (LLVM-MinGW UCRT x86_64) e/and Python 3 (gera / generates `native/src/data/game_data.hpp`).

```powershell
powershell -ExecutionPolicy Bypass -File native\build.ps1
```

Saída / Output: `dist/PES 2021 Workbench Native.exe`.

Testes unitários da engine C++ (OVR solver, Position Lab, parser monetário, UTF-8 folding, Team Roles, presets de habilidades e Economy Model) / C++ engine unit tests (OVR solver, Position Lab, money parser, UTF-8 folding, Team Roles, skill presets and Economy Model):

```powershell
cd native
clang++ -std=c++20 -O2 -DUNICODE -D_UNICODE -Isrc -Ithird_party/json tests/test_engine.cpp src/engine/ovr_engine.cpp src/engine/pes_engine.cpp src/engine/hooks.cpp src/engine/win32_mem.cpp -lole32 -lshell32 -luser32 -o tests/test_engine.exe
.\tests\test_engine.exe
```

O logo e o ícone são gerados por `native/assets/make_logo.ps1` (GDI+), que escreve `native/assets/app_icon_lettuce.ico` e `native/src/ui/logo_data.inc`. A imagem do README fica em `docs/assets/logo.png`.<br>
The logo and icon are generated by `native/assets/make_logo.ps1` (GDI+), which writes `native/assets/app_icon_lettuce.ico` and `native/src/ui/logo_data.inc`. The README image lives in `docs/assets/logo.png`.

## Licença / License

[MIT](LICENSE)
