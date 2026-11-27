<div align="center">

<img src="docs/assets/logo.png" alt="PES 2021 - Lettuce Live Editor" width="132" height="132">

# PES 2021 - Lettuce Live Editor

**Editor de carreira em tempo real para o eFootball PES 2021**

![Version](docs/assets/badges/version.svg)
![Native C++20](docs/assets/badges/stack.svg)
![PES 2021](docs/assets/badges/game.svg)
![Sider](docs/assets/badges/compat.svg)
![Languages](docs/assets/badges/languages.svg)
![License](docs/assets/badges/license.svg)

[![Ko-fi](docs/assets/badges/kofi-pt.svg)](https://ko-fi.com/AlfaceJao) [![Pix](docs/assets/badges/pix.svg)](#apoie-o-projeto)

<br>

<img src="docs/assets/screens/dashboard.png" alt="Lettuce Live Editor" width="920">

<sub>Capturas com dados de exemplo.</sub>

[English](README.md) · **Português** · [Apoie](#apoie-o-projeto)

### [Baixar a última versão](https://github.com/Joao-Spala/PES-2021-Lettuce-Live-Editor/releases/latest)

</div>

---

### Visão geral

Aplicativo desktop nativo para Windows que lê e edita a carreira do **eFootball PES 2021 (`v1.7.2.0`)** e de derivados compatíveis enquanto o jogo está aberto. Cobre atletas, elenco, finanças, olheiros, partida ao vivo e treino, em modos offline (Master Liga, Rumo ao Estrelato e partidas locais).

O foco é editar com consistência e com volta. Os princípios abaixo orientam o projeto.

<table>
<tr>
<td width="20%" valign="top"><b>1. Coerência</b></td>
<td>Editar um número nunca deixa o resto do atleta inconsistente. O OVR é recalculado pela fórmula da Konami, os atributos mudam na proporção do perfil do jogador (o <i>DNA</i>), e mercado, salário e multa acompanham idade, posição e país.</td>
</tr>
<tr>
<td valign="top"><b>2. Reversibilidade</b></td>
<td>Toda alteração tem volta, em quatro escalas: por atleta (<code>Ctrl+Z</code>), em lote, nas listas de olheiros e por uma linha do tempo global. Antes de lotes que mexem em valores, o app grava um backup JSON do elenco.</td>
</tr>
<tr>
<td valign="top"><b>3. Transparência</b></td>
<td>Nada acontece escondido. Cada ação mostra o que mudou, de quanto para quanto, e fica registrada no histórico com a hora.</td>
</tr>
<tr>
<td valign="top"><b>4. Respeito ao jogo</b></td>
<td>O limite nativo de 10 habilidades por atleta é mantido, a posição principal não se mexe quando você ajusta as secundárias e as listas de olheiros só gravam o ponteiro nativo, sem tocar em contrato ou cláusula.</td>
</tr>
<tr>
<td valign="top"><b>5. Leveza</b></td>
<td>Cerca de 2,8 MB, um único <code>.exe</code>, sem Cheat Engine e sem runtimes. O laço é guiado por eventos: com a janela parada, o consumo de CPU e GPU é praticamente zero.</td>
</tr>
</table>

### Em 30 segundos

1. [Baixe a última versão](https://github.com/Joao-Spala/PES-2021-Lettuce-Live-Editor/releases/latest), extraia o zip e execute `Lettuce Live Editor.exe` (ou [compile](#build-e-testes)).
2. Abra o **PES 2021** ou o **Football Life** e entre na **Master Liga** ou no **Rumo ao Estrelato**.
3. Abra o Lettuce Live Editor, como Administrador se o jogo estiver elevado.
4. Destaque um atleta no Plano de Jogo. O app o captura em segundo plano e abre tudo sobre ele.

Na primeira abertura o app mostra um guia curto, e a Visão Geral mantém um cartão de primeiros passos até o jogo estar conectado.

### Como funciona

```mermaid
flowchart LR
    G["PES 2021 / Football Life<br/>processo em execução"] <-->|"leitura e escrita<br/>de memória"| M["Camada de memória"]
    M --> E["Engine<br/>OVR Konami · economia<br/>posições · diário de desfazer"]
    E --> U["Interface nativa<br/>C++20 · DirectX 11 · ImGui"]
    U -->|"sua ação"| E
```

A interface nunca escreve direto no jogo. Toda ação passa pela engine, que valida, calcula as consequências (atributos, economia, posições), registra o estado anterior no diário e só então grava na memória. Por isso o desfazer é confiável e um lote inteiro pode ser revertido de uma vez.

---

### Telas

<table>
<tr>
<td width="50%" valign="top">
<img src="docs/assets/screens/attributes.png" alt="Atributos e DNA">
<h4>Atributos e solver de DNA</h4>
Peça <code>+1 OVR</code>, <code>-1 OVR</code> ou um <b>OVR alvo</b> entre 40 e 99. O solver redistribui os pontos na proporção do perfil do atleta e, depois, recalibra mercado, salário e multa. O jogador sobe de nível mantendo o perfil original.
</td>
<td width="50%" valign="top">
<img src="docs/assets/screens/position-lab.png" alt="Lab de Posições">
<h4>Lab de Posições</h4>
Um mapa tático com as 13 posições. Suba ou desça uma secundária de 1 em 1 sem encostar na principal, iguale à principal ou defina o grau <code>A / B / C</code>. O OVR mostrado já inclui o bônus do grau.
</td>
</tr>
<tr>
<td width="50%" valign="top">
<img src="docs/assets/screens/squad.png" alt="Meu Elenco">
<h4>Meu Elenco, em lote</h4>
Filtre por fim de contrato, lesão ou Sub-23, marque quem quiser e aplique de uma vez: recalibrar economia, <code>+1 OVR (DNA)</code>, fôlego, cura, contrato e seta. Dez <b>presets de habilidades</b> por função em campo (finalizador, ponta driblador, regista, zagueiro, goleiro e outros) sem estourar o limite de 10.
</td>
<td width="50%" valign="top">
<img src="docs/assets/screens/scouts.png" alt="Olheiros e Favoritos">
<h4>Olheiros e Favoritos</h4>
As 4 listas nativas (3 de olheiros e a de favoritos) como cartões com OVR médio, idade média e valor. Filtre, mova, copie e remova em lote, desfaça até 12 passos e exporte ou importe tudo em JSON, cruzando por ID e por nome para levar a lista a outro save.
</td>
</tr>
<tr>
<td width="50%" valign="top">
<img src="docs/assets/screens/live-match.png" alt="Partida Ao Vivo">
<h4>Partida Ao Vivo</h4>
O app detecta a partida sozinho e mostra minuto e período. Defina o tempo exato, congele o cronômetro e escolha entre 6 modos de fôlego seletivo, de "Casa 100%" a "Fora 100% / Casa 5%".
</td>
<td width="50%" valign="top">
<img src="docs/assets/screens/search.png" alt="Busca de jogadores">
<h4>Busca em mais de 30 mil atletas</h4>
A base inteira do jogo, não só o seu elenco. Encontre qualquer jogador, abra o editor dele ou marque direto nas listas de olheiros.
</td>
</tr>
</table>

<div align="center">
<img src="docs/assets/screens/command-palette.png" alt="Paleta de comandos" width="760">
<br><sub><b>Paleta de comandos</b> (<code>Ctrl+K</code>): qualquer ação a poucas teclas de distância.</sub>
</div>

---

### Detalhes técnicos

| Pergunta | Resposta |
| --- | --- |
| **Como o OVR muda sem desequilibrar o atleta?** | O app usa a fórmula exata da Konami como função de custo e um solver que preserva a proporção entre os atributos. 30.000 cálculos levam poucos milissegundos. |
| **E o dinheiro?** | Mercado, salário e multa vêm de um modelo econômico calibrado (curva 2025/26) que depende de OVR, idade, posição, país e versatilidade. É determinístico e reversível: voltar ao OVR antigo volta ao valor antigo. |
| **O que fazem as setas de forma?** | Durante a partida o jogo multiplica os atributos do jogador por um fator que depende da seta. Azul: +12% em talento, atributos de goleiro e força de chute, +9% em técnica, finalização, passe, velocidade e salto, +6% em contato físico, equilíbrio e resistência. Verde: +6%, +5% e +3%. Amarela: sem alteração. Laranja: -8%, -6% e -6%. Vermelha: -16%, -12% e -12%. Posse apertada e Agressividade nunca mudam. Os valores vêm da tabela interna do próprio jogo (v1.7.2.0), o efeito não fica gravado no atleta, e cada seta no app tem um tooltip com a lista completa. |
| **O que é a seta piscando?** | Uma flag separada da cor da seta (bit 7 do byte `0x146` do registro do atleta). A Master Liga usa o ícone piscando para marcar um atleta influenciado por uma função no time, e o manual da Konami diz que dar mais minutos a ele pode gerar uma evolução real. No app você só liga ou desliga esse aviso: a cor da seta e os atributos não mudam. |
| **Dá para desfazer?** | Sim. Cada atleta guarda 10 estados de desfazer, a linha do tempo global guarda até 300 ações, lotes têm "Desfazer Lote" e há backup JSON antes das operações pesadas. |
| **Funciona com patches?** | Foi feito e testado no PES 2021 v1.7.2.0 e funciona junto com o Sider 7.3+. Todo gancho e ponteiro é achado por uma assinatura de bytes única, e não por endereço fixo, então outras versões da mesma engine funcionam quando o código bate, e o que não bate fica desligado. O `FL_2027.exe` foi conferido offline: as 11 assinaturas casam e a estrutura da carreira é idêntica, mas não rodei ao vivo nele. `Ajustes > Serviços > Diagnóstico` mostra o que foi achado. Detalhes em [docs/COMPATIBILITY.pt-BR.md](docs/COMPATIBILITY.pt-BR.md). Se uma versão antiga do Workbench estiver aberta ao mesmo tempo, o app avisa do conflito. |
| **Dá para escolher o processo ao qual anexar?** | Dá, como o Open Process do Cheat Engine. Clique no indicador de conexão na barra de título, escolha `Escolher processo...`, filtre por nome, janela ou PID e anexe. A escolha fica salva até você voltar à detecção automática. Por segurança o app só aloca e grava em processos que parecem um jogo baseado no PES 2021: em qualquer outro ele não faz nada. |
| **Compartilho edições entre saves?** | O **Pack Studio** (JSON v13) exporta atletas e elencos e importa em outro save, cruzando por ID, nome ou escolha manual, com prévia das diferenças e rollback do lote. |
| **E o treino?** | A tela **Treino & BAL** tem o multiplicador de Sub-XP do seu clube e o desbloqueio dos Pontos de Foco do Rumo ao Estrelato. |
| **Dá para travar um valor?** | Sim. **Travar valores** (aba Condição & Bio) mantém o fôlego, a seta de forma e a lesão do atleta nos valores escolhidos, regravando cerca de duas vezes por segundo. A trava se desliga sozinha se o jogo fechar ou o atleta travado mudar, e não entra no histórico de desfazer. |
| **O app usa a internet?** | Só se você ativar. Em `Ajustes > Serviços` o aviso de nova versão vem desligado. Quando ligado, consulta a página pública de releases do GitHub uma vez ao abrir o app, não envia nada sobre você e não baixa nada, só mostra um link. Todo o resto roda no seu computador. |
| **Onde ficam backups e logs?** | Os backups do elenco vão para `Documentos/Lettuce Live Editor/Packs/Backups`, com limite opcional dos mais recentes (10, 20, 50 ou 100) e indicador de espaço usado. Um registro local das ações em `Documentos/Lettuce Live Editor/Logs` ajuda a reproduzir problemas, gira em 1 MB e pode ser desligado. Nada é enviado. |
| **Quais idiomas?** | Português, inglês e espanhol. Em espanhol, os nomes de habilidades, estilos de jogo e funções no time ficam em inglês por enquanto. |

### Atalhos que valem a pena

| Atalho | Ação |
| --- | --- |
| `Ctrl+K` | Paleta de comandos |
| `Ctrl+Z` | Desfazer a última edição do atleta |
| `Alt+Left` | Voltar na navegação |
| `Alt+1` a `Alt+8` | Ir direto para cada tela |
| `Ctrl+1` a `Ctrl+5` | Alternar as sub-abas do atleta |

### Limitações

- **Foco em offline.** O app foi pensado para Master Liga, Rumo ao Estrelato e partidas locais. Não o use em modos online.
- **Edita a memória de um processo em execução**, com validação, desfazer e backup. Mesmo assim, **faça backup do seu save** antes de sessões longas.
- **Só Windows**, para as versões `v1.7.2.0` do PES 2021 e derivados compatíveis.
- **Respeita as regras do jogo.** O que o jogo não aceita, o app não força. Por exemplo, ele nunca passa de 10 habilidades por atleta.

### Identidade visual

O nome e o logo de alface são os mesmos em qualquer tema. O que muda é a paleta: **`Azul`** (azul-marinho com destaque verde Alface, o padrão) e **`Alface`** (verde-floresta). A troca é ao vivo, em `Ajustes`, e fica salva. Ainda em `Ajustes`: sempre no topo, escala da interface de 90% a 150%, 6 cores de destaque, e tamanho e posição da janela lembrados. Completam o conjunto ícones vetoriais próprios, suporte a múltiplos monitores com DPI diferente, 7 fontes, idioma `PT-BR / EN / ES` e moeda `EUR / BRL / USD / GBP`.

---

## Build e testes

Requer `clang++` (LLVM-MinGW UCRT x86_64) e Python 3 (gera `native/src/data/game_data.hpp`).

```powershell
powershell -ExecutionPolicy Bypass -File native\build.ps1
```

Saída: `dist/PES 2021 Workbench Native.exe`.

Testes unitários (OVR solver, Lab de Posições, parser monetário, acentos, funções de time, presets de habilidades, modelo econômico, dicionário de espanhol, backups, ajustes, seletor de processo e mais). O script compila e roda os dois programas de teste:

```powershell
powershell -ExecutionPolicy Bypass -File native\tests\build_tests.ps1
```

O logo e o ícone são gerados por `native/assets/make_logo.ps1` (GDI+). As imagens deste README ficam em `docs/assets`.

## Apoie o projeto

<table>
<tr>
<td width="160" align="center"><img src="docs/assets/pix_qr.png" alt="QR Code Pix" width="140"></td>
<td>

Este editor é gratuito e de código aberto, e vai continuar assim. Se ele economizou seu tempo e você quiser me pagar um café, vou ficar muito agradecido. Um relato de bug ou um obrigado vale tanto quanto.

**Pix:** escaneie o QR ou use a chave aleatória `925fd607-2011-4ca2-83a4-b3cf145b408b`<br>
**De fora do Brasil:** [![Ko-fi](docs/assets/badges/kofi-pt.svg)](https://ko-fi.com/AlfaceJao)

</td>
</tr>
</table>

## Licença

[MIT](LICENSE)
