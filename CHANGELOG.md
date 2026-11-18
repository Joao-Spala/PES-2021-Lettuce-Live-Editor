# Changelog

Todas as mudanças relevantes deste projeto são documentadas neste arquivo.

## [10.0.0] — 2026-11-18

### Adicionado
- **Painel Gráfico Dark Studio (`v10.0 Pro Studio`)**: Interface flutuante nativa em 4 abas (*Master League & Elenco*, *Jogador & Contratos*, *Treino ML & Estrelato* e *Partida ao Vivo*), 100% responsiva ao DPI do Windows (`100%`, `125%`, `150%`), com cards escuros largos, efeito *hover*, pílulas rápidas de moeda e telemetria em tempo real.
- **Formatação Financeira Humana (Euro `€` por padrão)**: Conversão bidirecional automática (`/ 100`) em 16 campos financeiros da tabela, com troca instantânea entre `€`, `R$`, `$` e `£` e entrada humana (`50M`, `500k`, `1.5B`, `80.000.000`).
- **Engenharia Completa de Contratos Ativos & Taxa de Rescisão (`Hook 2`)**: Ativação/remoção de multa rescisória em contratos vigentes (`+0x147:b6`), cálculo fiel da multa Konami (`fcn.14156BE00`), override de valor exato para até 64 atletas simultâneos, edição de bônus da folha salarial (`+0x16EE854`), trava/destrava de renovação recente (`+0x16C` / `+0x168`) e macro **Contratação Fácil (`€ 1.000`)**.
- **Multiplicador Universal de Sub-XP (`Hook 3`)**: Evolução acelerada (`2x`, `5x`, `10x`, `50x` ou `Instantâneo 10.000 XP/dia`) exclusiva para os 82 slots do usuário (`+0x016BF710` — Elenco ML, Base/Youth e BAL), sem afetar clubes da CPU.
- **Suite Rumo ao Estrelato (BAL)**: Destrava do limite de `21` Focus Points nas setas (`Hook 4` até `127` pontos sem underflow), edição individual das 10 barras de foco (`+0x16ECBE0..E9`) e conclusão instantânea de Habilidade (`Skill`) e Posição em treinamento.
- **Squad Commander (Master League)**: Comandos de 1 clique para todo o elenco do usuário (*100% Stamina*, *Seta Azul / Top Form*, *Curar Lesões + Afeição 100%*, *Blindar Contratos +5 Anos*, *Ativar Breakthrough + Anti-Declínio 30+* e *+1 em Todos os Atributos*).
- **Busca Global de 30.000 Atletas + Leitor Ao Vivo de Clubes na RAM (`+0xADF4BC`)**: Pesquisa insensível a acentos por nome ou ID e decodificação automática de 100% dos clubes e seleções de qualquer patch (BMPES 16.0, SmokePatch, VirtuaRED, etc.) diretamente da memória da carreira, além de Backup/Restore `.json` de atletas.
- **Partida ao Vivo Sider-Safe (`Hook 5` & `Hook 6`)**: Pulo para minuto exato sem crash FNV-1a (`MatchState + 0x1174`), congelamento preciso do cronômetro, edição sincronizada de placar/pênaltis (`+0x1AC` distante do hook do Sider) e Stamina Seletiva (*Só Casa*, *Só Fora*, *Ambos* ou *Pressão Total*).
