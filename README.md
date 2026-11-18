# PES 2021 — Cheat Table v10.0 Pro Overhaul (BMPES / Sider Ready)

![PES 2021](https://img.shields.io/badge/eFootball_PES_2021-v1.7.2.0_(Data_Pack_7.0)-0095D9?style=for-the-badge)
![Cheat Engine](https://img.shields.io/badge/Cheat_Engine-7.2%2B_(64--bit)-2EA043?style=for-the-badge)
![Sider Compatible](https://img.shields.io/badge/Sider_7.3_%2F_BMPES_16.0-100%25_Compat%C3%ADvel-F59E0B?style=for-the-badge)
![License](https://img.shields.io/badge/Licen%C3%A7a-MIT-purple?style=for-the-badge)

Uma **Cheat Table v10.0 Pro Overhaul** completa, moderna e reescrita via engenharia reversa nativa x64 (`r2ghidra` + `Capstone/Keystone`) para **eFootball PES 2021 Season Update (`v1.7.2.0`)**, projetada desde a base para funcionar com **zero conflito com Sider 7.3+ e grandes patches** (**BMPES 16.0**, **SmokePatch / Football Life**, **VirtuaRED**, **Dream Patch**).

---

## Download Rápido & Como Usar

1. Baixe o arquivo pronto em **[`dist/PES 2021 - v10.0 Pro Overhaul.CT`](dist/PES%202021%20-%20v10.0%20Pro%20Overhaul.CT)**.
2. Abra o **PES 2021** (com ou sem Sider / Patch) e abra o arquivo `.CT` no **Cheat Engine 7.2+** (clique em **Sim / Yes** ao perguntar sobre executar o script Lua da tabela).
3. A tabela faz **Auto-Attach** sozinha ao `PES2021.exe` (mesmo que você abra a tabela antes de clicar em *Jogar* no launcher!) e já inicia configurada em **Euro (`€`)**.
4. Na seção **`[ CENTRAL v10.0 ]`**, marque **`[ ATIVAR TUDO ] Ligar Todos os 6 Hooks Mestres v10.0 de Uma Vez`** ou clique em **`[ PAINEL GUI ] Abrir Dashboard Gráfico PES 2021 v10.0 Pro`**.

---

## Principais Recursos da v10.0 Pro

### 1. Painel Gráfico Dark Studio (Multi-Abas & DPI-Responsivo)
- Interface flutuante moderna dividida em **4 abas temáticas** (*Master League & Elenco*, *Jogador & Contratos*, *Treino ML & Estrelato* e *Partida ao Vivo*).
- **Escala automática de DPI (`100%`, `125%`, `150%`)**: cards largos em duas colunas com bordas nítidas, efeito *hover*, pílulas rápidas de moeda (`€ EUR`, `R$ BRL`, `$ USD`, `£ GBP`), banners de telemetria ao vivo e zero texto cortado.

### 2. Finanças Formatadas em Moeda Real (`€`, `R$`, `$`, `£`)
- Inicia por padrão em **Euro (`€ 125.500.000`)** e converte automaticamente o formato interno `/ 100` da Konami em **16 campos financeiros** (Orçamento de Transferências, Salários, Receitas de Sócios/Merchandising/Patrocínios, Salário do Atleta, Valor de Mercado, Bônus e Multa Rescisória).
- **Entrada Humana Inteligente**: dê duplo-clique no valor e digite **`50M`**, **`500k`**, **`1.5B`** ou **`80.000.000`**.

### 3. Engenharia Real de Contratos Ativos & Taxa de Rescisão (`Release Fee`)
- **Ligar / Remover Multa Rescisória** em contratos já assinados (`ptrPlayer + 0x147` bit 6).
- **Fixar Taxa de Rescisão Exata (`Hook 2`)**: sobrescreve o cálculo dinâmico em `fcn.14156BE00` para até 64 atletas simultâneos (ex.: fixe **`€ 1.000`** para contratar qualquer craque rival pagando multa simbólica, ou **`€ 1 Bilhão`** para blindar titulares).
- **Destravar Renegociação Imediata**: limpa o sentinela de assinatura recente (`+0x16C`) e recua o ano de chegada (`+0x168`) para permitir renovação/proposta no mesmo dia.
- **Folha & Bônus Ativos**: edita os bônus de partida, gol e clean sheet comprometidos na folha do seu clube (`ptrCareer + 0x016EE854`).

### 4. Treino Real de Master League, Base & BAL (`Sub-XP Engine`)
- **Multiplicador Universal de Sub-XP (`Hook 3`)**: `2x`, `5x`, `10x`, `50x` ou **`Instantâneo (10.000 XP/dia)`** atuando exclusivamente sobre a tabela de 82 slots do jogador (`ptrCareer + 0x016BF710` — Elenco ML, Base/Youth e BAL), **sem acelerar times da CPU**.
- **Inspector das 25 Barras de Sub-XP (`0..10.000`)** + curva de **Auge (`Breakthrough`)** e **Anti-Declínio pós-30 anos**.
- **Suite Rumo ao Estrelato (BAL)**: remove a trava de `21` Focus Points nas setas (`Hook 4`), edita as 10 barras individuais e conclui em 1 clique a **Skill** ou **Posição** em treinamento.

### 5. Squad Commander, Busca Global (30.000 Atletas) & Leitor de Clubes na RAM
- **Squad Commander**: *100% Fôlego no Elenco*, *Seta Azul (Top Form)*, *Curar Lesões + Afeição Real 100%* (7 bits em `+0x13E`), *Blindar Contratos (+5 Anos + Sem Multa)*, *Breakthrough Geral* e *+1 Overall em Todo o Elenco*.
- **Busca Global Instantânea**: pesquise qualquer atleta da carreira por nome (com ou sem acento) ou `Player ID` sem precisar navegar pelos menus do jogo.
- **Clubes Decodificados Direto da RAM (`ptrCareer + 0xADF4BC`)**: exibe automaticamente o nome UTF-8 real e a sigla de todos os ~800+ clubes e seleções de qualquer patch instalado (ex.: `FLAMENGO [FLA] (ID 1248)`, `ARSENAL [ARS] (ID 101)`).
- **Edit Player Completo**: nomes UTF-8 de `60` bytes, 25 atributos, 13 posições, **41 Player Skills**, 7 estilos COM, Team Role e **Exportar/Importar Backup `.json`**.

### 6. Partida ao Vivo — Relógio Exato (FNV-Safe), Placar & Stamina Seletiva
- **Pular para Qualquer Minuto Sem Travar**: altera o acumulador aberto de ticks (`MatchState + 0x1174`, `1 min = 3600 ticks`) sem corromper a verificação de integridade FNV-1a do cronômetro, além de **Congelar Relógio**, **Reiniciar Tempo** e **Apito Imediato**.
- **Placar Oficial & Pênaltis Sincronizados (`Hook 5`)**: posicionado a `+0x1AC` bytes de distância do hook do `sider.dll` (`zero colisão`), atualizando motor interno e HUD simultaneamente.
- **Stamina Seletiva em Campo (`Hook 6`)**: *Só Mandante 100%*, *Só Visitante 100%*, *Ambos 100%* ou *Pressão Total (100% Meu Time + Drenar Rival p/ 5%)*.

---

## Estrutura do Repositório & Compilação a partir do Código-Fonte

```text
├── dist/
│   └── PES 2021 - v10.0 Pro Overhaul.CT   # Tabela compilada pronta para uso no Cheat Engine
├── src/
│   ├── pes10_lua_engine.lua               # Motor Lua 5.3 + Painel Gráfico Dark Studio DPI-Aware
│   ├── build_pes2021_v10_ct.py            # Compilador determinístico (.lua + .asm + .json -> .CT)
│   └── data/
│       ├── player_schema.json             # Mapeamento completo de bitfields e dropdowns
│       └── bmpes_dicts.json               # Dicionários de fallback de clubes e competições
└── tests/
    ├── test_ct_linter.py                  # Auditoria estática de XML, simetria x64 e bitfields
    └── test_lua_and_gui_dpi.py            # Testes na VM Lua 5.3 + medição GDI de DPI (100/125/150%)
```

Para recompilar a `.CT` e rodar os testes localmente (requer Python 3.10+):

```bash
python src/build_pes2021_v10_ct.py
python tests/test_ct_linter.py
python tests/test_lua_and_gui_dpi.py
```

---

## Créditos & Licença

Distribuído sob a licença [MIT](LICENSE). Inspirado na comunidade de modding de PES 2021 / Cheat Engine, com engenharia reversa x64, motor de contratos/Sub-XP/relógio FNV-safe e interface Studio desenvolvidos para a série v10.0 Pro.
