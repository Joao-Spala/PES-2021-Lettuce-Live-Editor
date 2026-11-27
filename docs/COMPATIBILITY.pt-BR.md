# Compatibilidade

[English](COMPATIBILITY.md) | Português

Como o app acha as coisas dentro do jogo, o que já foi verificado e como conferir ou incluir uma versão.

## Princípios

1. **Nenhum endereço fixo.** Todo ponto de gancho e todo ponteiro global é achado por uma assinatura de bytes (com curingas) procurada nas seções executáveis do processo em execução. O mesmo código serve para qualquer executável feito com a engine do PES 2021.
2. **Única ou recusa.** A assinatura precisa casar exatamente uma vez. Zero ou várias ocorrências desligam o recurso que depende dela e a linha de conexão no topo de cada tela mostra um aviso. O app nunca chuta um endereço.
3. **Reler antes de gravar.** Os bytes do ponto são comparados com a assinatura logo antes do patch, então um resultado antigo nunca é gravado por cima de algo que mudou.
4. **Guardas de layout.** As duas assinaturas de ponteiro contêm o deslocamento de um campo da estrutura da carreira (`E0 CB 6E 01`). Se a estrutura mudar, elas deixam de casar e o app recusa editar.
5. **Tudo o que um gancho presume está dentro da assinatura.** O gancho de fôlego refaz o epílogo da função na mão, então a assinatura vai até o `RET`.
6. **Checagem de estrutura dos dados.** As assinaturas acham o código, mas os dados da carreira ficam em offsets fixos. Ao conectar e a cada minuto o app cruza o ano, o contador de atletas e se o seu clube aponta para um elenco de verdade (11 jogadores ou mais) cujos registros têm nome de texto, idade plausível e atributos de 0 a 99. Se uma versão mexeu nos dados, pelo menos um desses testes falha e as edições que gravam na carreira ficam desligadas, com uma mensagem. Em `Ajustes > Serviços` dá para desligar a trava.
7. **Rede de segurança.** Cada gancho é registrado num diário dentro da área auxiliar que o app aloca no jogo. Se o app morrer, a instância seguinte reverte os saltos e libera a área, mas só quando nada mais salta para ela e só quando o processo dono (PID e horário de criação) acabou. Uma instância viva nunca é tocada, mesmo que o arquivo tenha outro nome.

## Assinaturas

As onze são únicas nas seções executáveis das duas versões abaixo.

| Id | Usada por | RVA PES 2021 | RVA FL_2027 |
| --- | --- | --- | --- |
| `player_capture_1` | Captura do atleta destacado (4 pontos) | `0xC6ED2D` | `0xC6D75D` |
| `player_capture_2` | | `0x8E24D7` | `0x8E1E47` |
| `player_capture_3` | | `0xDDCA5C` | `0xDDA8DC` |
| `player_capture_4` | | `0xF27462` | `0xF1B6B2` |
| `training_xp` | Multiplicador de XP de treino | `0x12E14A0` | `0x12DB3A0` |
| `bal_focus_cap` | Limite dos pontos de foco do BAL | `0xBBFE71` | `0xBB35C1` |
| `match_hud` | Ponteiros do HUD da partida | `0x20F9FD6` | `0x20F1786` |
| `match_clock` | Relógio da partida | `0xA3C787` | `0xA3C157` |
| `match_stamina` | Fôlego seletivo em partida | `0xA58EAC` | `0xA587EC` |
| `career_owner` | Ponteiro da carreira (pela chamada do getter) | `0xBBFEF3` | `0xBB3643` |
| `match_manager` | Ponteiro do gerente de partida | `0x4242AE` | `0x423A0E` |

O global da carreira fica no RVA `0x3714240` no PES 2021 e `0x3705E10` no FL_2027, o gerente de partida em `0x3702208` e `0x36F3DC8`. Os dois ficam numa seção de dados. Versões anteriores confiavam no primeiro valor e usavam às cegas, o que está errado em qualquer outra versão.

## Versões verificadas

| Versão | Carimbo do PE | Tamanho da imagem | Situação |
| --- | --- | --- | --- |
| PES 2021 Steam 1.7.2.0 | `0x62787C6A` | `0x1B48A000` | 11 de 11 únicas. Testada ao vivo. |
| `FL_2027.exe` | `0x5F5ED4BF` | `0x1BBA6000` | 11 de 11 únicas. Estrutura da carreira idêntica. **Conferida só offline**, não rodei ao vivo. |

Os nomes das seções são embaralhados a cada versão. A seção principal de código é `.text` no PES 2021 e `.trace` no FL_2027, e `.trace` é uma seção pequena de dados no PES 2021. O app nunca procura código pelo nome, usa a flag `IMAGE_SCN_MEM_EXECUTE`.

## Conferir um executável offline

```powershell
python native/check_exe_signatures.py "caminho\do\jogo.exe"
```

O script lê as assinaturas de `native/src/engine/signatures.hpp`, procura no arquivo sem executá-lo e conta as referências aos campos da carreira (contador de atletas, orçamento, teto salarial, salários, ano, clube do usuário, tamanho do registro). Uma versão compatível imprime `compativel pelas assinaturas e pela estrutura` e sai com 0. O arquivo não pode estar cifrado em disco.

## Diagnóstico

`Ajustes > Serviços > Diagnóstico > Copiar relatório` inclui a versão e se ela está na lista verificada, cada assinatura com o endereço, quanto das seções executáveis foi lido, a varredura do elenco (campo do contador, bytes lidos, registros do seu clube, registros rejeitados) e onde o atleta capturado está na tabela. Cole ao pedir ajuda.

## Limites

- Só imagens PE de 64 bits. Só o layout da carreira do PES 2021 é conhecido. Uma versão com layout diferente faz as assinaturas de ponteiro falharem ou, se elas ainda casarem, reprova a checagem de estrutura dos dados. Os dois são o resultado esperado.
- A checagem julga o elenco do seu clube, então uma carreira com menos de 11 jogadores registrados, ou um save ainda carregando, aparece como incompatível até poder ser lido. Ela confere de novo sozinha.
- Seções executáveis acima de 128 MB não são varridas, exceto uma vez como último recurso quando nenhuma assinatura foi achada.
- O `FL_2027.exe` não foi rodado ao vivo com este app. A edição de elenco e finanças nele depende da tabela de atletas, que só se confirma com o relatório de um save real.

## Incluir uma versão

1. Rode `check_exe_signatures.py` no executável.
2. Se o veredito for compatível, acrescente uma linha em `kKnown` em `native/src/engine/image_scan.cpp` com `reference_rvas` em `false`.
3. Teste ao vivo e envie o relatório de diagnóstico se algo parecer errado.
