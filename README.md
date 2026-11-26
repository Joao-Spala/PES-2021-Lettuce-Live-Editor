# Lettuce Live Editor

Editor de carreira em tempo real para o eFootball PES 2021, antes conhecido como PES 2021 Workbench.

![Native C++20](https://img.shields.io/badge/Native-C%2B%2B20_%2B_DX11_%2B_ImGui-84CC16?style=for-the-badge)
![PES 2021](https://img.shields.io/badge/eFootball_PES_2021-v1.7.2.0_(DP_7.0)-0095D9?style=for-the-badge)
![Languages](https://img.shields.io/badge/Languages-PT--BR_%7C_English-2EA043?style=for-the-badge)
![Patches](https://img.shields.io/badge/Sider_7.3_%2F_BMPES_%2F_Football_Life-Compatible-F59E0B?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-8B5CF6?style=for-the-badge)

> **[Portugues (PT-BR)](#portugues-pt-br)** | **[English](#english)** | **[Build & Testes](#build--testes)**

---

## Portugues (PT-BR)

Aplicativo desktop nativo em **C++20 + DirectX 11 + Dear ImGui** (cerca de 2.6 MB, standalone, sem dependencias externas) para edicao e gerenciamento de carreira em tempo real no **eFootball PES 2021 (`v1.7.2.0`)** e derivados (**Football Life 2023-2026**, **BMPES**, **SmokePatch**, **VirtuaRED**, **Dream Patch**), com compatibilidade total com **Sider 7.3+**.

Nao requer Cheat Engine nem runtimes externos. Basta abrir o jogo e executar o `PES 2021 Workbench Native.exe` (o nome do arquivo foi mantido para nao quebrar atalhos).

### Como usar

1. Compile o executavel ou use o binario em `dist/PES 2021 Workbench Native.exe`.
2. Abra o **PES 2021** (ou o **Football Life**) e entre na **Master League** ou no **Rumo ao Estrelato (BAL)**.
3. Execute o Lettuce Live Editor, como Administrador se o jogo tambem estiver elevado.
4. A conexao com o processo e a captura automatica do atleta em foco acontecem em segundo plano.

### Funcionalidades

- **8 telas e 5 sub-abas de atleta**: `Visao Geral`, `Meu Elenco`, `Financas`, `Jogador Atual` (`Bio & Contrato`, `Atributos`, `Lab de Posicoes`, `Estilo & Habilidades`, `Funcao no Time`), `Buscar Jogador (30k+)`, `Olheiros & Favoritos`, `Partida Ao Vivo` e `Treino & BAL`.
- **Motor de OVR Konami e solver DNA**: `+1 OVR`, `-1 OVR` e `OVR alvo (40-99)` preservando o perfil natural de atributos. Benchmark de 30.000 calculos em cerca de 2.5 ms.
- **Laboratorio de Posicoes**: mapa tatico de 13 posicoes, ajuste isolado `+1/-1` em posicoes secundarias sem mexer na principal, `Igualar`, `Principal` e Grau `A / B / C`.
- **Modelo economico calibrado**: valor de mercado, salario e multa Konami sincronizados com OVR, idade, posicao, nacionalidade e versatilidade, mais card de contrato rapido.
- **Seta de forma e seta piscando**: as 5 setas de forma e a flag independente de seta piscando (bit 7 do byte `0x146`), com pulso animado nas tabelas e descricao baseada no manual oficial da Konami.
- **Elenco em lote**: selecao por filtro ou por marcacao, recalibrar economia, `+1 OVR (DNA)`, folego, cura, contrato e seta nos atletas marcados. Antes de lotes que mexem em valores o app salva um backup JSON do elenco em `Documentos/PES 2021 Workbench/Packs/Backups`.
- **Presets de habilidades**: 10 funcoes em campo (finalizador, ponta driblador, meia criativo, regista, zagueiro, goleiro e outras) respeitando o limite nativo de 10 habilidades.
- **Historico de alteracoes**: linha do tempo global com desfazer ate qualquer ponto, alem do `Ctrl+Z` por atleta.
- **Olheiros & Favoritos**: cartoes das 4 listas com estatisticas, filtros, mover, copiar e remover em lote, desfazer nas listas (12 passos), exportar e importar listas em JSON cruzando por ID e nome, e marcacao direta pela Busca e pelo editor do atleta.
- **Partida Ao Vivo**: deteccao automatica, minuto e periodo ao vivo, tempo exato, ajustes rapidos, congelar cronometro e 6 modos de folego seletivo.
- **Pack Studio (JSON v13)**: exporta e importa atletas e elencos entre saves, com cruzamento por ID, nome ou escolha manual, previa de diferencas e rollback do lote.
- **Treino & BAL**: multiplicador de Sub-XP do seu clube e desbloqueio dos Pontos de Foco do BAL.

### Identidade visual e interface

- **Marca**: nome Lettuce Live Editor e logo de alface, o mesmo em todos os temas, no icone do executavel e na barra de titulo.
- **Dois temas de cor**: `Alface` (verde-floresta, padrao) e `Azul` (azul-marinho). A troca e ao vivo, em `Ajustes`, e fica salva.
- **Ajustes**: sempre no topo, escala da interface (90% a 150%), 6 cores de destaque, tamanho e posicao da janela lembrados.
- **Icones vetoriais proprios**, Per-Monitor DPI, radar hexagonal, idioma `PT-BR / EN`, moeda `EUR / BRL / USD / GBP`, 7 fontes, Command Palette (`Ctrl+K`) e historico de navegacao (`Alt+Left`).

---

## English

Real-time career editor for eFootball PES 2021, formerly known as PES 2021 Workbench.

Native standalone desktop application built with **C++20 + DirectX 11 + Dear ImGui** (about 2.6 MB, no runtime dependencies) for real-time career editing in **eFootball PES 2021 (`v1.7.2.0`)** and derivatives (**Football Life 2023-2026**, **BMPES**, **SmokePatch**, **VirtuaRED**, **Dream Patch**), fully compatible with **Sider 7.3+**.

No Cheat Engine or external runtimes required. Launch the game and run `PES 2021 Workbench Native.exe` (the file name was kept so existing shortcuts keep working).

### Quick Start

1. Build the executable or launch `dist/PES 2021 Workbench Native.exe`.
2. Start **PES 2021** (or **Football Life**) and load your **Master League** or **Become a Legend (BAL)** save.
3. Run Lettuce Live Editor, as Administrator if the game is elevated.
4. Process attachment and active player capture run automatically in the background.

### Features

- **8 views and 5 player subtabs**: `Overview`, `My Squad`, `Finances`, `Current Player` (`Bio & Contract`, `Attributes`, `Position Lab`, `Style & Skills`, `Team Role`), `Search Player (30k+)`, `Scouts & Shortlist`, `Live Match` and `Training & BAL`.
- **Exact Konami OVR engine and DNA solver**: `+1 OVR`, `-1 OVR` and `Target OVR (40-99)` while preserving each player's stat DNA. A benchmark of 30,000 calculations takes about 2.5 ms.
- **Position Lab**: 13-position tactical pitch, isolated `+1/-1` on secondary positions without moving the primary one, `Match`, `Primary` and `A / B / C` grades.
- **Calibrated economy model**: market value, salary and Konami release clause synced with OVR, age, position, nationality and versatility, plus a quick contract card.
- **Form arrow and blinking arrow**: the 5 form arrows and the independent blinking-arrow flag (bit 7 of byte `0x146`), with an animated pulse in tables and a description based on Konami's official manual.
- **Batch squad tools**: pick by filter or by ticking, then recalibrate economy, `+1 OVR (DNA)`, stamina, heal, contract and arrow on the ticked players. Before value-changing batches the app saves a JSON squad backup in `Documents/PES 2021 Workbench/Packs/Backups`.
- **Skill presets**: 10 on-pitch roles (poacher, dribbling winger, playmaker, deep-lying playmaker, centre-back, goalkeeper and more) respecting the native cap of 10 skills.
- **Edit history**: a global timeline with undo to any point, besides per-player `Ctrl+Z`.
- **Scouts & Shortlist**: cards for the 4 lists with stats, filters, batch move, copy and remove, list-level undo (12 steps), JSON export and import matched by ID and name, and direct tagging from Search and from the player editor.
- **Live Match**: automatic detection, live minute and period, exact clock, quick shifts, clock freeze and 6 selective stamina modes.
- **Pack Studio (JSON v13)**: export and import players and squads across saves with matching by ID, name or manual pick, diff preview and batch rollback.
- **Training & BAL**: user-club Sub-XP multiplier and BAL Focus Points unlock.

### Visual identity and interface

- **Brand**: the Lettuce Live Editor name and lettuce logo are the same in every theme, in the executable icon and in the title bar.
- **Two colour themes**: `Lettuce` (forest green, default) and `Blue` (navy). Switching is live, in `Settings`, and persisted.
- **Settings**: always on top, UI scale (90% to 150%), 6 accent colours, remembered window size and position.
- **Own vector icons**, Per-Monitor DPI, hexagonal radar, `PT-BR / EN`, currency `EUR / BRL / USD / GBP`, 7 fonts, Command Palette (`Ctrl+K`) and navigation history (`Alt+Left`).

---

## Build & Testes

Requer `clang++` (LLVM-MinGW UCRT x86_64) e Python 3 para gerar `native/src/data/game_data.hpp`:

```powershell
powershell -ExecutionPolicy Bypass -File native\build.ps1
```

Saida: `dist/PES 2021 Workbench Native.exe`.

O logo e o icone sao gerados por um script (`native/assets/make_logo.ps1`, GDI+) que escreve `native/assets/app_icon_lettuce.ico` e `native/src/ui/logo_data.inc`.

Testes unitarios da engine C++ (OVR solver, Position Lab, parser monetario, UTF-8 folding, Team Roles, presets de habilidades e Economy Model):

```powershell
cd native
clang++ -std=c++20 -O2 -DUNICODE -D_UNICODE -Isrc -Ithird_party/json tests/test_engine.cpp src/engine/ovr_engine.cpp src/engine/pes_engine.cpp src/engine/hooks.cpp src/engine/win32_mem.cpp -lole32 -lshell32 -luser32 -o tests/test_engine.exe
.\tests\test_engine.exe
```

## License

[MIT](LICENSE)
