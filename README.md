# PES 2021 Workbench

![Native C++20](https://img.shields.io/badge/Native-C%2B%2B20_%2B_DX11_%2B_ImGui-38BDF8?style=for-the-badge)
![PES 2021](https://img.shields.io/badge/eFootball_PES_2021-v1.7.2.0_(DP_7.0)-0095D9?style=for-the-badge)
![Languages](https://img.shields.io/badge/Languages-PT--BR_%7C_English-2EA043?style=for-the-badge)
![Patches](https://img.shields.io/badge/Sider_7.3_%2F_BMPES_%2F_Football_Life-Compatible-F59E0B?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-8B5CF6?style=for-the-badge)

> **[Portugues (PT-BR)](#portugues-pt-br)** | **[English](#english)** | **[Build & Testes](#build--testes)**

---

## Portugues (PT-BR)

Aplicativo desktop nativo em **C++20 + DirectX 11 + Dear ImGui** (~2.4 MB, standalone, zero dependencias externas) para edicao e gerenciamento de carreira em tempo real no **eFootball PES 2021 (`v1.7.2.0`)** e derivados (**Football Life 2023-2026**, **BMPES**, **SmokePatch**, **VirtuaRED**, **Dream Patch**), com compatibilidade total para **Sider 7.3+**.

Nao requer Cheat Engine nem runtimes externos. Basta abrir o jogo e executar o `PES 2021 Workbench Native.exe`.

### Como usar

1. Compile o executavel (ou use o binario gerado em `dist/PES 2021 Workbench Native.exe`).
2. Abra o **PES 2021** (ou **Football Life**) e entre na sua **Master League** ou **Rumo ao Estrelato (BAL)**.
3. Execute o **PES 2021 Workbench Native** (como Administrador, se o jogo tambem estiver elevado).
4. A conexao com o processo e a captura automatica do atleta em foco acontecem em segundo plano.

### Funcionalidades

- **Arquitetura de 8 Telas + 5 Sub-abas de Atleta**: `Visao Geral`, `Meu Elenco`, `Financas`, `Jogador Atual` (`Bio & Contrato`, `Atributos`, `Lab de Posicoes`, `Habilidades & COMP`, `Funcao no Time`), `Buscar Jogador (30k+)`, `Olheiros & Favoritos`, `Partida Ao Vivo` e `Treino & BAL`.
- **Motor de OVR Konami (`0x141E640A0`) e Solver DNA**: `+1 OVR`, `-1 OVR`, `OVR Alvo (40-99)` e `Elenco +1 OVR` preservando o perfil natural de atributos do jogador. Benchmark C++ de 30.000 calculos em ~2.5 ms.
- **Laboratorio de Posicoes (Isolamento Cirurgico)**: mapa tatico interativo de 13 posicoes, ajuste `+1/-1 OVR` isolado em posicoes secundarias sem alterar o OVR da posicao principal, `Igualar OVR`, `Tornar Principal` e edicao de Grau `A / B / C`.
- **Modelo Economico Calibrado e Contrato Rapido**: sincronizacao automatica ou manual de Valor de Mercado, Salario e Multa Rescisoria Konami considerando OVR principal, idade, posicao, nacionalidade e versatilidade secundaria (teto de `+7%` no mercado e `+3.15%` no salario). Inclui card dedicado de `Contrato & Financas (Edicao Rapida)` na aba Bio.
- **Partida Ao Vivo**: deteccao automatica ao entrar em campo (`Hook 5` armado automaticamente), leitura ao vivo de minuto/segundo/periodo, definicao de tempo exato (`MM:SS`), ajustes rapidos (`-10/-5/+5/+10 min`, `44:00`, `89:00`, `Reiniciar Tempo`, `Encerrar Tempo`), botao `Ler Atual`, congelamento de cronometro, 6 modos de folego seletivo e preparo rapido do elenco.
- **Pack Studio (JSON v13)**: exportacao e importacao de atletas individuais ou elencos completos entre saves diferentes, com cruzamento automatico (ID exato, nome normalizado ou selecao manual), preview de diferencas e recalibracao economica opcional.
- **Busca Global (30.000+ Atletas) e Olheiros na RAM Real**: pesquisa instantanea na base inteira da carreira e edicao direta das 3 listas de Olheiros (30 vagas cada) e da Shortlist (100 vagas) sem efeitos colaterais em contratos.
- **Treino & Rumo ao Estrelato (BAL)**: multiplicador de Sub-XP restrito ao seu clube (`1x` ate `Instantaneo`) e desbloqueio do teto de 21 Pontos de Foco no BAL (ate 99, com opcao de travar valor).
- **Interface Studio Pro**: dark mode nativo, Per-Monitor DPI, radar hexagonal de atributos, troca ao vivo de idioma (`PT-BR / EN`), moeda (`EUR / BRL / USD / GBP`) e fonte (7 familias), Command Palette (`Ctrl+K`), historico de navegacao (`Alt+Left`) e `Undo` de 10 niveis (`Ctrl+Z`).

---

## English

Native standalone desktop application built with **C++20 + DirectX 11 + Dear ImGui** (~2.4 MB, zero runtime dependencies) for real-time career editing in **eFootball PES 2021 (`v1.7.2.0`)** and derivatives (**Football Life 2023-2026**, **BMPES**, **SmokePatch**, **VirtuaRED**, **Dream Patch**), fully compatible with **Sider 7.3+**.

No Cheat Engine or external runtimes required. Just launch the game and run `PES 2021 Workbench Native.exe`.

### Quick Start

1. Build the executable (or launch `dist/PES 2021 Workbench Native.exe`).
2. Start **PES 2021** (or **Football Life**) and load your **Master League** or **Become a Legend (BAL)** save.
3. Run **PES 2021 Workbench Native** (as Administrator if the game is elevated).
4. Process attachment and active player capture run automatically in the background.

### Features

- **8 Views + 5 Player Subtabs**: `Overview`, `My Squad`, `Finances`, `Current Player` (`Bio & Contract`, `Attributes`, `Position Lab`, `Skills & COM AI`, `Team Role`), `Search Player (30k+)`, `Scouts & Shortlist`, `Live Match`, and `Training & BAL`.
- **Exact Konami OVR Engine (`0x141E640A0`) & DNA Solver**: `+1 OVR`, `-1 OVR`, `Target OVR (40-99)`, and `Squad +1 OVR` while preserving each player's natural stat DNA. C++ benchmark evaluates 30,000 OVR calculations in ~2.5 ms.
- **Surgical Position Lab**: interactive 13-position tactical pitch, isolated `+1/-1 OVR` secondary position solver without shifting primary OVR, `Match Primary OVR`, `Set as Primary`, and `A / B / C` grade editing.
- **Calibrated Economy Model & Quick Contract Card**: automatic or manual sync for Market Value, Salary, and Konami Release Clause based on primary OVR, age, position, nationality, and secondary versatility (capped at `+7%` market and `+3.15%` salary). Includes a dedicated `Contract & Finances (Quick Edit)` card in the Bio tab.
- **Live Match Control**: automatic match detection (`Hook 5` auto-arms upon entering a match), live minute/second/period readout, exact clock setter (`MM:SS`), quick shifts (`-10/-5/+5/+10 min`, `44:00`, `89:00`, `Reset Half`, `End Half`), `Read Live` sync button, clock freeze, 6 selective stamina modes, and quick squad prep.
- **Pack Studio (JSON v13)**: export and import single players or full squads across saves with automatic matching (Exact ID, normalized name, or manual pick), diff preview, and optional economy recalibration.
- **Global Search (30,000+ Players) & Real-RAM Scouts**: instant full-career search and direct RAM editing for Scouts 1-3 (30 slots each) and Shortlist (100 slots) with zero contract side effects.
- **Training & Become a Legend (BAL)**: user-club Sub-XP multiplier (`1x` to `Instant`) and BAL 21 Focus Points cap unlock (up to 99, with optional lock).
- **Studio Pro UX**: native dark mode, Per-Monitor DPI scaling, hexagonal attribute radar, live `PT-BR / EN` language switch, currency selector (`EUR / BRL / USD / GBP`), 7 selectable fonts, Command Palette (`Ctrl+K`), navigation history (`Alt+Left`), and 10-level `Undo` (`Ctrl+Z`).

---

## Build & Testes

Requer `clang++` (LLVM-MinGW UCRT x86_64) e Python 3 para gerar `native/src/data/game_data.hpp`:

```powershell
cd native
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Saida: `dist/PES 2021 Workbench Native.exe`.

Testes unitarios da engine C++ (OVR solver, Position Lab, parser monetario, UTF-8 folding, Team Roles e Economy Model):

```powershell
cd native
clang++ -std=c++20 -O2 -DUNICODE -D_UNICODE -Isrc -Ivendor/nlohmann tests/test_engine.cpp src/engine/ovr_engine.cpp src/engine/pes_engine.cpp src/engine/hooks.cpp src/engine/win32_mem.cpp -lpsapi -o tests/test_engine.exe
.\tests\test_engine.exe
```

## License

[MIT](LICENSE)
