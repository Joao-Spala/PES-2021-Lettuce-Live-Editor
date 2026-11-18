import argparse
import json
import os
import pathlib
import shutil
import xml.etree.ElementTree as ET

SRC_DIR = pathlib.Path(__file__).resolve().parent
REPO_ROOT = SRC_DIR.parent
DATA_DIR = SRC_DIR / "data"
OUTPUT_CT = REPO_ROOT / "dist" / "PES 2021 - v10.0 Pro Overhaul.CT"

with open(DATA_DIR / "player_schema.json", "r", encoding="utf-8") as f:
    legacy = json.load(f)

with open(DATA_DIR / "bmpes_dicts.json", "r", encoding="utf-8") as f:
    bmpes = json.load(f)

with open(SRC_DIR / "pes10_lua_engine.lua", "r", encoding="utf-8") as f:
    lua_template = f.read()

DROPDOWNS = legacy["dropdowns"]

# Traduções ricas PT-BR + Siglas Clássicas para os 129 campos originais
PT_PLAYER_LABELS = {
    "Player ID": "ID do Jogador (Player ID)",
    "Commentary ID": "ID de Narração (Commentary ID)",
    "Player Name": "Nome do Jogador (UTF-8 60B)",
    "Name on Strip (Club)": "Nome na Camisa — Clube (UTF-8 60B)",
    "Name on Strip (National Team)": "Nome na Camisa — Seleção (UTF-8 60B)",
    "Nationality": "Nacionalidade / País",
    "Height (cm)": "Altura (cm)",
    "Weight (kg)": "Peso (kg)",
    "Age": "Idade (15–50)",
    "Stronger Foot": "Pé Dominante",
    "Registred Position": "Posição de Origem (Registered Position)",
    "CF - Center Forward": "CA (CF) — Centroavante",
    "SS - Second Striker": "SA (SS) — Segundo Atacante",
    "LWF - Left Wing Forward": "PTE (LWF) — Ponta Esquerda",
    "RWF - Right Wing Forward": "PTD (RWF) — Ponta Direita",
    "AMF - Attacking Midfielder": "MAT (AMF) — Meia Atacante",
    "DMF - Defensive Midfielder": "VOL (DMF) — Volante",
    "CMF - Center Midfielder": "MLG (CMF) — Meia de Ligação",
    "LMF - Left Midfielder": "MLE (LMF) — Meia Esquerda",
    "RMF - Right Midfielder": "MLD (RMF) — Meia Direita",
    "CB - Center Back": "ZAG (CB) — Zagueiro",
    "LB - Left Back": "LE (LB) — Lateral Esquerdo",
    "RB - Right Back": "LD (RB) — Lateral Direito",
    "GK - Goalkeeper": "GL (GK) — Goleiro",
    "Weak Foot Usage": "Uso do Pé Ruim (1–4)",
    "Weak Foot Accuracy": "Precisão do Pé Ruim (1–4)",
    "Form": "Forma Física / Condicionamento (1–8)",
    "Injury Resistance": "Resistência a Lesões (1–3)",
    "Offensive Awareness": "Talento Ofensivo (Offensive Awareness)",
    "Ball Control": "Controle de Bola (Ball Control)",
    "Dribbling": "Drible (Dribbling)",
    "Tight Possession": "Condução Firme (Tight Possession)",
    "Low Pass": "Passe rasteiro (Low Pass)",
    "Lofted Pass": "Passe Alto (Lofted Pass)",
    "Finishing": "Finalização (Finishing)",
    "Heading": "Cabeceio (Heading)",
    "Place Kicking": "Chute Colocado / Bola Parada (Place Kicking)",
    "Curl": "Curva (Curl)",
    "Speed": "Velocidade (Speed)",
    "Acceleration": "Aceleração (Acceleration)",
    "Kicking Power": "Força do Chute (Kicking Power)",
    "Jump": "Impulsão (Jump)",
    "Physical Contact": "Contato Físico (Physical Contact)",
    "Balance": "Equilíbrio (Balance)",
    "Stamina": "Resistência / Fôlego (Stamina)",
    "Defensive Awareness": "Talento Defensivo (Defensive Awareness)",
    "Ball Winning": "Desarme (Ball Winning)",
    "Aggression": "Agressividade (Aggression)",
    "GK Awareness": "Talento de Goleiro (GK Awareness)",
    "GK Catching": "Firmeza de Goleiro (GK Catching)",
    "GK Clearing": "Afastar de Goleiro (GK Clearing)",
    "GK Reflexes": "Reflexos de Goleiro (GK Reflexes)",
    "GK Reach": "Alcance / Cobertura (GK Reach)",
    "Playing Style": "Estilo de Jogo do Atleta (Playing Style)",
    "Scissors Feint": "Pedalada Simples (Scissors Feint)",
    "Double Touch": "Toque Duplo (Double Touch)",
    "Flip Flap": "Elástico (Flip Flap)",
    "Marseille Turn": "Roleta / Giro de Marselha (Marseille Turn)",
    "Sombrero": "Chapéu / Sombreiro (Sombrero)",
    "Cross Over Turn": "Corte de Letra (Cross Over Turn)",
    "Cut Behind & Turn": "Puxada e Giro (Cut Behind & Turn)",
    "Scotch Move": "Toque Escocês (Scotch Move)",
    "Step On Skill control": "Domínio Pisando na Bola (Step On Control)",
    "Long Range Drive": "Chute Colocado de Longe (Long Range Drive)",
    "Chip shot control": "Cavadinha Precisa (Chip Shot Control)",
    "Long Range Shooting": "Chute de Longa Distância (Long Range Shooting)",
    "Knuckle Shot": "Folha Seca (Knuckle Shot)",
    "Dipping Shot": "Chute com Queda Rápida (Dipping Shot)",
    "Rising Shots": "Chute Ascendente (Rising Shots)",
    "Acrobatic Finishing": "Finalização Acrobática (Acrobatic Finishing)",
    "Heel Trick": "Toque de Calcanhar (Heel Trick)",
    "First-time Shot": "Chute de Primeira (First-time Shot)",
    "One-touch Pass": "Passe de Primeira (One-touch Pass)",
    "Through Passing": "Passe em Profundidade (Through Passing)",
    "Weighted Pass": "Passe na Medida (Weighted Pass)",
    "Pinpoint Crossing": "Cruzamento Preciso (Pinpoint Crossing)",
    "Outside Curler": "Trivela (Outside Curler)",
    "Rabona": "Letra (Rabona)",
    "No Look Pass": "Passe Sem Olhar (No Look Pass)",
    "Low Lofted Pass": "Lançamento Tenso (Low Lofted Pass)",
    "GK Low Punt": "Reposição Baixa de Goleiro (GK Low Punt)",
    "GK High Punt": "Reposição Alta de Goleiro (GK High Punt)",
    "Long Throw": "Lateral Longo (Long Throw)",
    "GK Long Throw": "Arremesso Longo do Goleiro (GK Long Throw)",
    "Penalty Specialist": "Especialista em Pênaltis (Penalty Specialist)",
    "GK Penalty Saver": "Pegador de Pênaltis (GK Penalty Saver)",
    "Gamesmanship": "Malícia / Cavar Falta (Gamesmanship)",
    "Man Marking": "Marcação Individual (Man Marking)",
    "Track Back": "Voltar para Marcar (Track Back)",
    "Interception": "Interceptação (Interception)",
    "Acrobatic Clear": "Corte Acrobático (Acrobatic Clear)",
    "Captaincy": "Liderança / Capitão (Captaincy)",
    "Super-Sub": "Super Substituto / Talismã (Super-Sub)",
    "Fighting Spirit": "Espírito Guerreiro (Fighting Spirit)",
    "Trickster": "[IA COM] Driblador (Trickster)",
    "Mazing Run": "[IA COM] Arrancada em Zigue-Zague (Mazing Run)",
    "Speeding Bullet": "[IA COM] Bala Veloz (Speeding Bullet)",
    "Incisive Run": "[IA COM] Corrida Incisiva (Incisive Run)",
    "Long Ball Expert": "[IA COM] Especialista em Bola Longa (Long Ball Expert)",
    "Early Cross": "[IA COM] Cruzamento Antecipado (Early Cross)",
    "Long Ranger": "[IA COM] Chutador de Longe (Long Ranger)",
    "Team Role": "Papel no Time (Team Role)",
    "Level": "Nível do Papel no Time (Level)",
    "Team Player - Lone Wolf": "Espírito Coletivo <-> Lobo Solitário",
    "Passion - Composure": "Paixão <-> Frieza",
    "Technique - Strength": "Técnica <-> Força",
    "Insight - Instinct": "Visão <-> Instinto",
    "Impact": "Impacto / Carisma (Impact)",
}

# Dropdowns traduzidos PT-BR
DD_YES_NO = "0:Não\n1:Sim"
DD_FOOT = "0:Direito (Right)\n1:Esquerdo (Left)"
DD_POS_GRADE = "0:C (Sem Aptidão)\n1:B (Secundária)\n2:A (Domínio Total)"
DD_FORM_ARROW = (
    "0:0 — Péssima (Seta Vermelha p/ Baixo)\n"
    "1:1 — Ruim (Seta Laranja Diagonal)\n"
    "2:2 — Normal (Seta Amarela Horizontal)\n"
    "3:3 — Boa (Seta Verde Diagonal)\n"
    "4:4 — Excelente / Top Form (Seta Azul p/ Cima)"
)
DD_REG_POS = (
    "0:GL (GK) — Goleiro\n"
    "1:ZAG (CB) — Zagueiro\n"
    "2:LE (LB) — Lateral Esquerdo\n"
    "3:LD (RB) — Lateral Direito\n"
    "4:VOL (DMF) — Volante\n"
    "5:MLG (CMF) — Meia de Ligação\n"
    "6:MLE (LMF) — Meia Esquerda\n"
    "7:MLD (RMF) — Meia Direita\n"
    "8:MAT (AMF) — Meia Atacante\n"
    "9:PTE (LWF) — Ponta Esquerda\n"
    "10:PTD (RWF) — Ponta Direita\n"
    "11:SA (SS) — Segundo Atacante\n"
    "12:CA (CF) — Centroavante"
)
DD_PLAYING_STYLE = (
    "0:Nenhum (-)\n"
    "1:Artilheiro (Goal Poacher)\n"
    "2:Puxador de Marcação (Dummy Runner)\n"
    "3:Homem de Área (Fox in the Box)\n"
    "4:Ala Produtivo (Prolific Winger)\n"
    "5:Clássico Camisa 10 (Classic No. 10)\n"
    "6:Jogador de Infiltração (Hole Player)\n"
    "7:Área a Área (Box-to-Box)\n"
    "8:Primeiro Volante (Anchor Man)\n"
    "9:O Destruidor (The Destroyer)\n"
    "10:Atacante Extra (Extra Frontman)\n"
    "11:Lateral Ofensivo (Offensive Full-back)\n"
    "12:Lateral Defensivo (Defensive Full-back)\n"
    "13:Pivô (Target Man)\n"
    "14:Armador Criativo (Creative Playmaker)\n"
    "15:Provocador / Saída de Bola (Build Up)\n"
    "16:Goleiro Ofensivo (Offensive Goalkeeper)\n"
    "17:Goleiro Defensivo (Defensive Goalkeeper)\n"
    "18:Ala Móvel (Roaming Flank)\n"
    "19:Especialista em Cruzamento (Cross Specialist)\n"
    "20:Orquestrador (Orchestrator)\n"
    "21:Lateral Finalizador (Full-back Finisher)"
)
DD_GROWTH_CURVE = (
    "0:0 — Precoce (Pico Cedo)\n"
    "1:1 — Padrão Equilibrado\n"
    "2:2 — Desenvolvimento Tardio\n"
    "3:3 — Duradouro / Longevo (Pouco Declínio 30+)\n"
    "4:4 — Precoce + Duradouro\n"
    "5:5 — Padrão + Duradouro\n"
    "6:6 — Tardio + Duradouro (Eterno Craque)\n"
    "7:7 — Curva Especial Lenda"
)
DD_BAL_SKILL_IDS = "\n".join([
    "60:Pedalada Simples (Scissors Feint)",
    "61:Toque Duplo (Double Touch)",
    "62:Elástico (Flip Flap)",
    "63:Roleta (Marseille Turn)",
    "64:Chapéu (Sombrero)",
    "65:Corte de Letra (Cross Over Turn)",
    "66:Puxada e Giro (Cut Behind & Turn)",
    "67:Toque Escocês (Scotch Move)",
    "68:Domínio Pisando na Bola (Step On Skill)",
    "69:Cabeceio (Heading)",
    "70:Chute Colocado de Longe (Long Range Drive)",
    "71:Cavadinha Precisa (Chip Shot Control)",
    "72:Chute de Longa Distância (Long Range Shooting)",
    "73:Folha Seca (Knuckle Shot)",
    "74:Chute com Queda (Dipping Shot)",
    "75:Chute Ascendente (Rising Shots)",
    "76:Finalização Acrobática (Acrobatic Finishing)",
    "77:Toque de Calcanhar (Heel Trick)",
    "78:Chute de Primeira (First-time Shot)",
    "79:Passe de Primeira (One-touch Pass)",
    "80:Passe em Profundidade (Through Passing)",
    "81:Passe na Medida (Weighted Pass)",
    "82:Cruzamento Preciso (Pinpoint Crossing)",
    "83:Trivela (Outside Curler)",
    "84:Letra (Rabona)",
    "85:Passe Sem Olhar (No Look Pass)",
    "86:Lançamento Tenso (Low Lofted Pass)",
    "87:Reposição Baixa GK",
    "88:Reposição Alta GK",
    "89:Lateral Longo (Long Throw)",
    "90:Arremesso Longo GK",
    "91:Especialista em Pênaltis",
    "92:Pegador de Pênaltis GK",
    "93:Malícia (Gamesmanship)",
    "94:Marcação Individual (Man Marking)",
    "95:Voltar para Marcar (Track Back)",
    "96:Interceptação (Interception)",
    "97:Corte Acrobático (Acrobatic Clear)",
    "98:Capitão / Liderança (Captaincy)",
    "99:Super Substituto (Super-Sub)",
    "100:Espírito Guerreiro (Fighting Spirit)",
    "108:Nenhuma (Sem Treino de Skill)",
])
DD_BAL_POS_IDS = (
    "1:CA (CF) — Centroavante\n"
    "2:SA (SS) — Segundo Atacante\n"
    "3:PTE (LWF) — Ponta Esquerda\n"
    "4:PTD (RWF) — Ponta Direita\n"
    "5:MAT (AMF) — Meia Atacante\n"
    "6:VOL (DMF) — Volante\n"
    "7:MLG (CMF) — Meia de Ligação\n"
    "8:MLE (LMF) — Meia Esquerda\n"
    "9:MLD (RMF) — Meia Direita\n"
    "10:ZAG (CB) — Zagueiro\n"
    "11:LE (LB) — Lateral Esquerdo\n"
    "12:LD (RB) — Lateral Direito\n"
    "13:GL (GK) — Goleiro\n"
    "108:Nenhuma (Sem Treino de Posição)"
)

# ============================================================================
# SCRIPTS AUTO ASSEMBLER x64 (100% VERIFICADOS EM PES2021.exe v1.7.2.0)
# ============================================================================
ASM_HOOK_1_PLAYER = """{ PES 2021 v10.0 Pro — Hook 1: Captura Automática de Jogador (Edit / Squad / Transfer) }
[ENABLE]
aobscanmodule(INJECT_ptrPlayer,PES2021.exe,CB B8 02 00 00 00 0F 1F 40 00 66 0F 1F 84 00 00 00 00 00 0F 10 02 0F 11 01)
alloc(newmem_p1,$1000,INJECT_ptrPlayer)

label(code_p1)
label(ret_p1)

newmem_p1:
  cmp eax,2
  jne code_p1
  mov [ptrPlayer],rdx

code_p1:
  movups xmm0,[rdx]
  movups [rcx],xmm0
  jmp ret_p1

INJECT_ptrPlayer+13:
  jmp newmem_p1
  nop
ret_p1:
registersymbol(INJECT_ptrPlayer)

aobscanmodule(INJECT_ptrPlayerTwo,PES2021.exe,48 8B 40 2C 48 89 02)
alloc(newmem_p2,$1000,INJECT_ptrPlayerTwo)

label(code_p2)
label(ret_p2)

newmem_p2:
  mov [ptrPlayer],rax

code_p2:
  mov rax,[rax+2C]
  mov [rdx],rax
  jmp ret_p2

INJECT_ptrPlayerTwo:
  jmp newmem_p2
  nop
  nop
ret_p2:
registersymbol(INJECT_ptrPlayerTwo)

[DISABLE]
INJECT_ptrPlayer+13:
  db 0F 10 02 0F 11 01

unregistersymbol(INJECT_ptrPlayer)
dealloc(newmem_p1)

INJECT_ptrPlayerTwo:
  db 48 8B 40 2C 48 89 02

unregistersymbol(INJECT_ptrPlayerTwo)
dealloc(newmem_p2)
"""

ASM_HOOK_2_RELEASE_FEE = """{ PES 2021 v10.0 Pro — Hook 2: Override de Taxa de Rescisão Exata (VA 0x14156BE0C) }
[ENABLE]
aobscanmodule(INJECT_ReleaseFeeGetter,PES2021.exe,0F 29 74 24 40 48 8B DA 48 8B F1 0F 29 7C 24 30 48 8B 09)
alloc(newmem_rfee,$1000,INJECT_ReleaseFeeGetter)

label(loop_rfee)
label(next_rfee)
label(miss_rfee)
label(orig_rfee)
label(hit_rfee)
label(ret_rfee)

newmem_rfee:
  test rcx,rcx
  jz orig_rfee
  push r8
  push r9
  push rdx
  mov r8,[rcx]
  test r8,r8
  jz miss_rfee
  lea r9,[rfeeOverrideTable]
  xor edx,edx
loop_rfee:
  cmp dword ptr [r9+0C],1
  jne next_rfee
  cmp [r9],r8
  je hit_rfee
next_rfee:
  add r9,10
  inc edx
  cmp edx,40
  jb loop_rfee

miss_rfee:
  pop rdx
  pop r9
  pop r8
orig_rfee:
  movaps [rsp+40],xmm6
  jmp ret_rfee

hit_rfee:
  mov eax,[r9+08]
  pop rdx
  pop r9
  pop r8
  mov rbx,[rsp+70]
  add rsp,50
  pop rdi
  pop rsi
  pop rbp
  ret

INJECT_ReleaseFeeGetter:
  jmp newmem_rfee
ret_rfee:
registersymbol(INJECT_ReleaseFeeGetter)

[DISABLE]
INJECT_ReleaseFeeGetter:
  db 0F 29 74 24 40

unregistersymbol(INJECT_ReleaseFeeGetter)
dealloc(newmem_rfee)
"""

ASM_HOOK_3_TRAINING_MULT = """{ PES 2021 v10.0 Pro — Hook 3: Multiplicador Universal de Sub-XP (Apenas Meu Clube / Base / BAL) }
[ENABLE]
aobscanmodule(INJECT_TrainingXpAdder,PES2021.exe,45 8B C8 83 FA 6C 0F 8D 1E 01 00 00)
alloc(newmem_txp,$1000,INJECT_TrainingXpAdder)

label(check_clamp_txp)
label(orig_txp)
label(ret_txp)

newmem_txp:
  mov r9d,[cfgTrainingMultMode]
  test r9d,r9d
  jz orig_txp
  test r8d,r8d
  jle orig_txp
  cmp r9d,1
  jne @f
  imul r8d,r8d,2
  jmp check_clamp_txp
@@:
  cmp r9d,2
  jne @f
  imul r8d,r8d,5
  jmp check_clamp_txp
@@:
  cmp r9d,3
  jne @f
  imul r8d,r8d,0A
  jmp check_clamp_txp
@@:
  cmp r9d,4
  jne @f
  imul r8d,r8d,32
  jmp check_clamp_txp
@@:
  mov r8d,2710

check_clamp_txp:
  cmp r8d,2710
  jle orig_txp
  mov r8d,2710

orig_txp:
  mov r9d,r8d
  cmp edx,6C
  jmp ret_txp

INJECT_TrainingXpAdder:
  jmp newmem_txp
  nop
ret_txp:
registersymbol(INJECT_TrainingXpAdder)

[DISABLE]
INJECT_TrainingXpAdder:
  db 45 8B C8 83 FA 6C

unregistersymbol(INJECT_TrainingXpAdder)
dealloc(newmem_txp)
"""

ASM_HOOK_4_BAL_CAP = """{ PES 2021 v10.0 Pro — Hook 4: Destravar Limite de 21 Focus Points nas Setas do BAL (Até 127 s/ Underflow) }
[ENABLE]
aobscanmodule(INJECT_BALFocusCap,PES2021.exe,0F BE 41 0A 2B C2 83 F8 15 77 09 48 63 C3)
INJECT_BALFocusCap+08:
  db 7F
registersymbol(INJECT_BALFocusCap)

[DISABLE]
INJECT_BALFocusCap+08:
  db 15
unregistersymbol(INJECT_BALFocusCap)
"""

ASM_HOOK_5_MATCH_HUD_CLOCK = """{ PES 2021 v10.0 Pro — Hook 5: Captura Sider-Safe de Partida, Placar, Pênaltis & Relógio FNV-Safe }
[ENABLE]
aobscanmodule(INJECT_MatchHud,PES2021.exe,49 8B F8 44 0F B6 E2 88 55 B7)
alloc(newmem_mhud,$1000,INJECT_MatchHud)

label(ret_mhud)

newmem_mhud:
  mov [ptrMatchHud],rcx
  push rax
  mov rax,[rcx+38]
  mov [ptrMatchCore],rax
  pop rax
  mov rdi,r8
  movzx r12d,dl
  jmp ret_mhud

INJECT_MatchHud:
  jmp newmem_mhud
  nop
  nop
ret_mhud:
registersymbol(INJECT_MatchHud)

aobscanmodule(INJECT_MatchClockTap,PES2021.exe,8B 44 24 40 89 44 24 40 F3 0F 10 44 24 40 48 83 C4 30 5B C3)
alloc(newmem_mclk,$1000,INJECT_MatchClockTap)

label(done_mclk)
label(ret_mclk)

newmem_mclk:
  mov eax,[rsp+40]
  push rcx
  mov rcx,[ptrMatchState]
  test rcx,rcx
  jz done_mclk
  lea rcx,[rcx+1168]
  cmp rbx,rcx
  jne done_mclk
  mov [valProtectedMatchTicks],eax
  cmp dword ptr [cfgFreezeClock],1
  jne done_mclk
  movss xmm0,[valFrozenTotalTicks]
  subss xmm0,[valProtectedMatchTicks]
  movss [rbx+0C],xmm0

done_mclk:
  pop rcx
  mov [rsp+40],eax
  jmp ret_mclk

INJECT_MatchClockTap:
  jmp newmem_mclk
  nop
  nop
  nop
ret_mclk:
registersymbol(INJECT_MatchClockTap)

[DISABLE]
INJECT_MatchHud:
  db 49 8B F8 44 0F B6 E2

unregistersymbol(INJECT_MatchHud)
dealloc(newmem_mhud)

INJECT_MatchClockTap:
  db 8B 44 24 40 89 44 24 40

unregistersymbol(INJECT_MatchClockTap)
dealloc(newmem_mclk)
"""

ASM_HOOK_6_STAMINA = """{ PES 2021 v10.0 Pro — Hook 6: Stamina Seletiva em Campo (Casa / Fora / Ambos / Pressão Total) }
[ENABLE]
aobscanmodule(INJECT_SelectiveStamina,PES2021.exe,6B 45 10 64 48 8B 5C 24 50)
alloc(newmem_stam,$1000,INJECT_SelectiveStamina)

label(loop_stam)
label(found_stam)
label(is_away_stam)
label(give_100_stam)
label(drain_stam)
label(orig_stam)
label(ret_stam)

newmem_stam:
  cmp dword ptr [cfgStaminaMode],0
  je orig_stam
  mov rcx,[ptrMatchMgr]
  test rcx,rcx
  jz orig_stam
  xor edx,edx
loop_stam:
  mov rax,rdx
  add rax,F3
  lea rax,[rax+rax*4]
  cmp [rcx+rax*8],rbx
  je found_stam
  inc edx
  cmp edx,16
  jb loop_stam
  jmp orig_stam

found_stam:
  mov eax,[cfgStaminaMode]
  cmp edx,0B
  jae is_away_stam

  cmp eax,1
  je give_100_stam
  cmp eax,3
  je give_100_stam
  cmp eax,4
  je give_100_stam
  cmp eax,5
  je drain_stam
  jmp orig_stam

is_away_stam:
  cmp eax,2
  je give_100_stam
  cmp eax,3
  je give_100_stam
  cmp eax,5
  je give_100_stam
  cmp eax,4
  je drain_stam
  jmp orig_stam

give_100_stam:
  mov rbx,[rsp+50]
  mov rdi,[rsp+58]
  mov eax,64
  add rsp,30
  pop rbp
  ret

drain_stam:
  mov rbx,[rsp+50]
  mov rdi,[rsp+58]
  mov eax,05
  add rsp,30
  pop rbp
  ret

orig_stam:
  imul eax,[rbp+10],64
  mov rbx,[rsp+50]
  jmp ret_stam

INJECT_SelectiveStamina:
  jmp newmem_stam
  nop
  nop
  nop
  nop
ret_stam:
registersymbol(INJECT_SelectiveStamina)

[DISABLE]
INJECT_SelectiveStamina:
  db 6B 45 10 64 48 8B 5C 24 50

unregistersymbol(INJECT_SelectiveStamina)
dealloc(newmem_stam)
"""


def make_lua_button_script(lua_body: str) -> str:
    return (
        "[ENABLE]\n"
        "{$lua}\n"
        "if syntaxcheck then return end\n"
        "PES10.asyncActionButton(memrec, function()\n"
        f"  {lua_body}\n"
        "end)\n"
        "{$asm}\n"
        "[DISABLE]\n"
    )


class CTBuilder:
    def __init__(self):
        self.next_id = 1
        self.money_ids = []
        self.club_handle_ids = []
        self.clock_id = -1
        self.status_id = -1
        self.custom_rfee_id = -1
        self.score_sync = {}
        self.hook_ids = []

    def alloc_id(self) -> int:
        cid = self.next_id
        self.next_id += 1
        return cid

    def group(self, parent_entries: ET.Element, desc: str, color: str = None, hide_children: bool = True) -> ET.Element:
        ce = ET.SubElement(parent_entries, "CheatEntry")
        ET.SubElement(ce, "ID").text = str(self.alloc_id())
        ET.SubElement(ce, "Description").text = f'"{desc}"'
        if hide_children:
            opts = ET.SubElement(ce, "Options")
            opts.set("moHideChildren", "1")
            opts.set("moManualExpandCollapse", "1")
            opts.set("moAllowManualCollapseAndExpand", "1")
        ET.SubElement(ce, "GroupHeader").text = "1"
        if color:
            ET.SubElement(ce, "Color").text = color
        return ET.SubElement(ce, "CheatEntries")

    def add_script(self, parent_entries: ET.Element, desc: str, code: str, color: str = None, is_core_hook: bool = False) -> int:
        ce = ET.SubElement(parent_entries, "CheatEntry")
        cid = self.alloc_id()
        ET.SubElement(ce, "ID").text = str(cid)
        ET.SubElement(ce, "Description").text = f'"{desc}"'
        if color:
            ET.SubElement(ce, "Color").text = color
        ET.SubElement(ce, "VariableType").text = "Auto Assembler Script"
        ET.SubElement(ce, "AssemblerScript").text = code
        if is_core_hook:
            self.hook_ids.append(cid)
        return cid

    def add_record(
        self,
        parent_entries: ET.Element,
        desc: str,
        vtype: str,
        addr: str,
        offsets: list[str] = None,
        dropdown: str = None,
        dropdown_only: bool = False,
        signed: bool = False,
        bitstart: str = None,
        bitlen: str = None,
        str_len: int = None,
        utf8: bool = False,
        color: str = None,
        is_money: bool = False,
        is_club_handle: bool = False,
    ) -> int:
        ce = ET.SubElement(parent_entries, "CheatEntry")
        cid = self.alloc_id()
        ET.SubElement(ce, "ID").text = str(cid)
        ET.SubElement(ce, "Description").text = f'"{desc}"'
        if dropdown:
            ddl = ET.SubElement(ce, "DropDownList")
            ddl.set("DisplayValueAsItem", "1")
            if dropdown_only:
                ddl.set("DescriptionOnly", "1")
                ddl.set("ReadOnly", "1")
            ddl.text = dropdown
        ET.SubElement(ce, "ShowAsSigned").text = "1" if signed else "0"
        if color:
            ET.SubElement(ce, "Color").text = color
        ET.SubElement(ce, "VariableType").text = vtype
        if vtype == "Binary":
            ET.SubElement(ce, "BitStart").text = str(bitstart)
            ET.SubElement(ce, "BitLength").text = str(bitlen)
            ET.SubElement(ce, "ShowAsBinary").text = "0"
        elif vtype == "String":
            ET.SubElement(ce, "Length").text = str(str_len or 60)
            ET.SubElement(ce, "Unicode").text = "0"
            ET.SubElement(ce, "CodePage").text = "1" if utf8 else "0"
            ET.SubElement(ce, "ZeroTerminate").text = "1"
        ET.SubElement(ce, "Address").text = addr
        if offsets:
            offs_el = ET.SubElement(ce, "Offsets")
            for o in offsets:
                ET.SubElement(offs_el, "Offset").text = o.upper()

        if is_money:
            self.money_ids.append(cid)
        if is_club_handle:
            self.club_handle_ids.append(cid)
        return cid


builder = CTBuilder()
root = ET.Element("CheatTable", {"CheatEngineTableVersion": "34"})
top_entries = ET.SubElement(root, "CheatEntries")

# Paleta BGR nítida em temas claros e escuros
COL_AMBER   = "0066CC"  # Central / Moeda / Status
COL_GREEN   = "3B8E1E"  # Master League / Finanças
COL_PURPLE  = "A8326E"  # Treino ML & BAL
COL_BLUE    = "CC5500"  # Squad Commander / Macros
COL_TEAL    = "807A00"  # Edit Player & Contratos Ativos
COL_CRIMSON = "2828C6"  # Partida ao Vivo / Gameplay

# ============================================================================
# SEÇÃO 1: [ CENTRAL v10.0 ] PAINEL, STATUS, MOEDA & HOOKS MESTRES
# ============================================================================
sec_central = builder.group(top_entries, "[ CENTRAL v10.0 ] Painel, Status, Moeda & Hooks Mestres", COL_AMBER, hide_children=False)

builder.status_id = builder.add_record(
    sec_central,
    "[ STATUS ] Telemetria ao Vivo (Processo / Modo / Jogador)",
    "4 Bytes",
    "cfgCurrency",
    color=COL_AMBER,
)

builder.add_record(
    sec_central,
    "[ MOEDA ] Seletor Rápido de Moeda (Inicia em Euro €)",
    "4 Bytes",
    "cfgCurrency",
    dropdown="0:Euro (€) — Padrão\n1:Real (R$)\n2:Dólar ($)\n3:Libra (£)",
    dropdown_only=True,
    color=COL_AMBER,
)

builder.add_script(
    sec_central,
    "[ PAINEL GUI ] Abrir Dashboard Gráfico PES 2021 v10.0 Pro",
    make_lua_button_script("PES10.openDashboard()"),
    color=COL_BLUE,
)

# Placeholder p/ botão de ativar todos os hooks (código preenchido após criar os 6 hooks)
activate_all_ce_placeholder_parent = sec_central

grp_hooks = builder.group(sec_central, "[ MÓDULOS x64 ] Hooks de Injeção (Ative Todos ou Individualmente)", COL_AMBER, hide_children=True)
builder.add_script(grp_hooks, "[ HOOK 1 ] Captura Automática de Jogador (Edit / Elenco / Negociações)", ASM_HOOK_1_PLAYER, is_core_hook=True)
builder.add_script(grp_hooks, "[ HOOK 2 ] Motor de Taxa de Rescisão Exata (Release Fee Override)", ASM_HOOK_2_RELEASE_FEE, is_core_hook=True)
builder.add_script(grp_hooks, "[ HOOK 3 ] Multiplicador Universal de Sub-XP (Treino ML, Base & BAL)", ASM_HOOK_3_TRAINING_MULT, is_core_hook=True)
builder.add_script(grp_hooks, "[ HOOK 4 ] Destravar Limite de 21 Focus Points nas Setas do BAL", ASM_HOOK_4_BAL_CAP, is_core_hook=True)
builder.add_script(grp_hooks, "[ HOOK 5 ] Partida ao Vivo — Placar, Pênaltis & Relógio (Zero Colisão Sider)", ASM_HOOK_5_MATCH_HUD_CLOCK, is_core_hook=True)
builder.add_script(grp_hooks, "[ HOOK 6 ] Stamina Seletiva em Campo (Casa / Fora / Ambos / Pressão)", ASM_HOOK_6_STAMINA, is_core_hook=True)

hook_id_list_lua = "{" + ", ".join(str(x) for x in builder.hook_ids) + "}"
builder.add_script(
    sec_central,
    "[ ATIVAR TUDO ] Ligar Todos os 6 Hooks Mestres v10.0 de Uma Vez",
    make_lua_button_script(
        f"PES10.initProcessState(); local al=getAddressList(); for _, id in ipairs({hook_id_list_lua}) do local mr=al.getMemoryRecordByID(id); if mr and not mr.Active then mr.Active=true end end"
    ),
    color=COL_GREEN,
)

# ============================================================================
# SEÇÃO 2: [ MASTER LEAGUE ] FINANÇAS, ORÇAMENTOS FORMATADOS & CALENDÁRIO
# ============================================================================
sec_ml_fin = builder.group(top_entries, "[ MASTER LEAGUE ] Finanças Formatadas (€), Receitas & Calendário", COL_GREEN)

builder.add_record(sec_ml_fin, "Orçamento de Transferências (Duplo-Clique: ex. 150M, 500k)", "4 Bytes", "ptrCareer", ["016ECBF4"], signed=True, is_money=True, color=COL_GREEN)
builder.add_record(sec_ml_fin, "Orçamento de Salários (Duplo-Clique: ex. 30M, 500k)", "4 Bytes", "ptrCareer", ["016ECC08"], signed=True, is_money=True, color=COL_GREEN)

builder.add_script(
    sec_ml_fin,
    "[ +50M ] Somar + 50 Milhões no Orçamento de Transferências",
    make_lua_button_script("local c=PES10.readU64(PES10.symAddr('ptrCareer')); if c~=0 then writeInteger(c+0x016ECBF4, math.min(2147483647, PES10.readI32(c+0x016ECBF4)+500000)); PES10.toast('+50M adicionados!') end"),
    color=COL_BLUE,
)
builder.add_script(
    sec_ml_fin,
    "[ +500M ] Somar + 500 Milhões no Orçamento de Transferências",
    make_lua_button_script("local c=PES10.readU64(PES10.symAddr('ptrCareer')); if c~=0 then writeInteger(c+0x016ECBF4, math.min(2147483647, PES10.readI32(c+0x016ECBF4)+5000000)); PES10.toast('+500M adicionados!') end"),
    color=COL_BLUE,
)
builder.add_script(
    sec_ml_fin,
    "[ +25M ] Somar + 25 Milhões no Orçamento de Salários",
    make_lua_button_script("local c=PES10.readU64(PES10.symAddr('ptrCareer')); if c~=0 then writeInteger(c+0x016ECC08, math.min(2147483647, PES10.readI32(c+0x016ECC08)+250000)); PES10.toast('+25M Salários adicionados!') end"),
    color=COL_BLUE,
)
builder.add_script(
    sec_ml_fin,
    "[ BILIONÁRIO ] Definir € 1,5 Bilhão (Transf.) + € 250 Milhões (Salários)",
    make_lua_button_script("local c=PES10.readU64(PES10.symAddr('ptrCareer')); if c~=0 then writeInteger(c+0x016ECBF4, 15000000); writeInteger(c+0x016ECC08, 2500000); PES10.toast('Pacote € 1,5B ativado!') end"),
    color=COL_GREEN,
)

grp_ml_ledger = builder.group(sec_ml_fin, "Receitas Comerciais & Balanço da Temporada", COL_GREEN)
builder.add_record(grp_ml_ledger, "Receita do Clube de Sócios / Torcedores", "4 Bytes", "ptrCareer", ["016ECC20"], signed=True, is_money=True)
builder.add_record(grp_ml_ledger, "Receita de Merchandising / Produtos", "4 Bytes", "ptrCareer", ["016ECC24"], signed=True, is_money=True)
builder.add_record(grp_ml_ledger, "Receita de Patrocínios", "4 Bytes", "ptrCareer", ["016ECC28"], signed=True, is_money=True)
builder.add_record(grp_ml_ledger, "Balanço de Transferências na Temporada", "4 Bytes", "ptrCareer", ["016EF298"], signed=True, is_money=True)
builder.add_record(grp_ml_ledger, "Balanço Salarial na Temporada", "4 Bytes", "ptrCareer", ["016EF290"], signed=True, is_money=True)
builder.add_record(grp_ml_ledger, "Outras Despesas / Indenizações", "4 Bytes", "ptrCareer", ["016EF288"], signed=True, is_money=True)

grp_ml_date = builder.group(sec_ml_fin, "Data Atual do Calendário da Carreira", COL_GREEN)
builder.add_record(grp_ml_date, "Ano Atual", "2 Bytes", "ptrCareer", ["01642A24"])
builder.add_record(grp_ml_date, "Mês Atual (1–12)", "Byte", "ptrCareer", ["01642A26"])
builder.add_record(grp_ml_date, "Dia Atual (1–31)", "Byte", "ptrCareer", ["01642A27"])

# ============================================================================
# SEÇÃO 3: [ MASTER LEAGUE ] TREINO DO CLUBE, BASE & EVOLUÇÃO REAL (SUB-XP)
# ============================================================================
sec_ml_trn = builder.group(top_entries, "[ MASTER LEAGUE ] Treino do Clube, Base & Evolução Real (Sub-XP)", COL_PURPLE)

builder.add_record(
    sec_ml_trn,
    "Modo Multiplicador de Treino (Requer Hook 3 Ativo — Só Meu Clube/Base/BAL)",
    "4 Bytes",
    "cfgTrainingMultMode",
    dropdown=(
        "0:1x — Evolução Original\n"
        "1:2x — Evolução Dobrada\n"
        "2:5x — Evolução Rápida\n"
        "3:10x — Super Treino\n"
        "4:50x — Turbo Extremo\n"
        "5:Instantâneo (10.000 XP — Nível / Posição / Skill em 1 Dia!)"
    ),
    dropdown_only=True,
    color=COL_PURPLE,
)

builder.add_script(
    sec_ml_trn,
    "[ 1-CLIQUE ] Encher Todas as 25 Barras de Sub-XP do Jogador Selecionado (9.999 / 10.000)",
    make_lua_button_script("PES10.runPlayerMacro('instant_subxp_player')"),
    color=COL_BLUE,
)

grp_subxp_stats = builder.group(sec_ml_trn, "Inspector de Barras Reais de Sub-XP (0 a 10.000) — Jogador Selecionado", COL_PURPLE)
SUBXP_FIELD_NAMES = [
    (14, "Sub-XP: Talento Ofensivo"),
    (15, "Sub-XP: Controle de Bola"),
    (16, "Sub-XP: Drible"),
    (17, "Sub-XP: Condução Firme"),
    (18, "Sub-XP: Passe Rasteiro"),
    (19, "Sub-XP: Passe Alto"),
    (20, "Sub-XP: Finalização"),
    (21, "Sub-XP: Cabeceio"),
    (22, "Sub-XP: Bola Parada"),
    (23, "Sub-XP: Curva"),
    (24, "Sub-XP: Velocidade"),
    (25, "Sub-XP: Aceleração"),
    (26, "Sub-XP: Força do Chute"),
    (27, "Sub-XP: Impulsão"),
    (28, "Sub-XP: Contato Físico"),
    (29, "Sub-XP: Equilíbrio"),
    (30, "Sub-XP: Resistência (Stamina)"),
    (31, "Sub-XP: Talento Defensivo"),
    (33, "Sub-XP: Desarme"),
    (34, "Sub-XP: Agressividade"),
    (35, "Sub-XP: Talento de Goleiro"),
    (36, "Sub-XP: Firmeza GK"),
    (37, "Sub-XP: Afastar GK"),
    (38, "Sub-XP: Reflexos GK"),
    (39, "Sub-XP: Alcance GK"),
]
for fid, label in SUBXP_FIELD_NAMES:
    off_hex = f"{0x16 + fid * 2:X}"
    builder.add_record(grp_subxp_stats, f"{label} (0..10000)", "2 Bytes", "ptrPlayerTraining", [off_hex], signed=True)

grp_growth = builder.group(sec_ml_trn, "Auge (Breakthrough), Pico de Idade & Anti-Declínio 30+ (Jogador Selecionado)", COL_PURPLE)
builder.add_record(grp_growth, "Curva de Desenvolvimento / Declínio Pós-30", "Binary", "ptrPlayer", ["1B"], dropdown=DD_GROWTH_CURVE, bitstart="4", bitlen="3")
builder.add_record(grp_growth, "Estado de Auge / Breakthrough (2 = Em Explosão!)", "Binary", "ptrPlayerGrowthDesc", ["3"], bitstart="0", bitlen="4")
builder.add_record(grp_growth, "Idade de Pico Calculada", "Binary", "ptrPlayerGrowthDesc", ["2"], bitstart="1", bitlen="7")

# ============================================================================
# SEÇÃO 4: [ MASTER LEAGUE ] SQUAD COMMANDER — AÇÕES EM TODO O MEU ELENCO
# ============================================================================
sec_squad = builder.group(top_entries, "[ MASTER LEAGUE ] Squad Commander — 1 Clique em Todo o Meu Elenco", COL_BLUE)

builder.add_script(sec_squad, "[ ELENCO ] Descansar Todo o Meu Elenco (100% Barra de Stamina)", make_lua_button_script("local n=PES10.runSquadCommander('stamina_100'); if n>0 then showMessage('Stamina 100% aplicada em '..n..' atletas!') end"), color=COL_GREEN)
builder.add_script(sec_squad, "[ ELENCO ] Seta Azul Piscando (Top Form) em Todo o Meu Elenco", make_lua_button_script("local n=PES10.runSquadCommander('blue_form'); if n>0 then showMessage('Seta Azul aplicada em '..n..' atletas!') end"), color=COL_BLUE)
builder.add_script(sec_squad, "[ ELENCO ] Curar Todas as Lesões + Afeição 100% no Elenco", make_lua_button_script("local n=PES10.runSquadCommander('heal_affection'); if n>0 then showMessage('Lesões curadas e Afeição 100% em '..n..' atletas!') end"), color=COL_TEAL)
builder.add_script(sec_squad, "[ ELENCO ] Blindar Contratos do Meu Elenco (+5 Anos + Remover Multas)", make_lua_button_script("local n=PES10.runSquadCommander('shield_contracts'); if n>0 then showMessage('Contratos blindados em '..n..' atletas!') end"), color=COL_AMBER)
builder.add_script(sec_squad, "[ ELENCO ] Ativar Breakthrough (Auge) + Zero Declínio 30+ no Elenco", make_lua_button_script("local n=PES10.runSquadCommander('eternal_peak_breakthrough'); if n>0 then showMessage('Auge e Anti-Declínio ativados em '..n..' atletas!') end"), color=COL_PURPLE)
builder.add_script(sec_squad, "[ ELENCO ] Evoluir +1 Ponto em Todos os Atributos de Todo o Elenco", make_lua_button_script("local n=PES10.runSquadCommander('plus_one_all_stats'); if n>0 then showMessage('+1 em todos os atributos de '..n..' atletas!') end"), color=COL_CRIMSON)

# ============================================================================
# SEÇÃO 5: [ RUMO AO ESTRELATO ] SUITE BAL COMPLETA (TREINO, SKILLS & CARREIRA)
# ============================================================================
sec_bal = builder.group(top_entries, "[ RUMO AO ESTRELATO ] Suite BAL Completa (Treino, Skills & Carreira)", COL_PURPLE)

builder.add_script(sec_bal, "[ BAL 1-CLIQUE ] Apontar Edit Player para Meu Jogador do Estrelato", make_lua_button_script("PES10.pointToBALMyPlayer()"), color=COL_BLUE)
builder.add_record(sec_bal, "Focus Points Restantes (Ative o Hook 4 p/ usar > 21 nas setas!)", "Byte", "ptrCareer", ["016ECBEA"], color=COL_PURPLE)
builder.add_record(sec_bal, "Travar Focus Points Sempre em 99", "4 Bytes", "cfgLockBALFocusPoints", dropdown="0:Desativado\n1:Ativado (99 Pontos Infinitos)", dropdown_only=True)
builder.add_script(sec_bal, "[ BAL 1-CLIQUE ] Colocar Todas as 10 Barras de Treino no Máximo + 99 Pontos", make_lua_button_script("PES10.runPlayerMacro('bal_max_sliders')"), color=COL_GREEN)

grp_bal_bars = builder.group(sec_bal, "As 10 Barras Individuais de Treino do BAL (+0x16ECBE0..E9)", COL_PURPLE)
BAL_BARS = [
    ("016ECBE0", "Barra 1 — Chute (Shooting)"),
    ("016ECBE1", "Barra 2 — Passe (Passing)"),
    ("016ECBE2", "Barra 3 — Drible (Dribbling)"),
    ("016ECBE3", "Barra 4 — Destreza / Ataque (Dexterity)"),
    ("016ECBE4", "Barra 5 — Velocidade / Explosão (Lower Body)"),
    ("016ECBE5", "Barra 6 — Jogo Aéreo / Físico (Aerial Strength)"),
    ("016ECBE6", "Barra 7 — Defesa (Defending)"),
    ("016ECBE7", "Barra 8 — Goleiro / Extra (GK)"),
    ("016ECBE8", "Barra 9 — Foco em Habilidade (Skill Points)"),
    ("016ECBE9", "Barra 10 — Foco em Nova Posição (Position Points)"),
]
for off_h, bdesc in BAL_BARS:
    builder.add_record(grp_bal_bars, bdesc, "Byte", "ptrCareer", [off_h])

grp_bal_learn = builder.group(sec_bal, "Aprender Habilidade (Skill) & Posição Instantaneamente no BAL", COL_PURPLE)
builder.add_record(grp_bal_learn, "Habilidade (Skill) em Treinamento Atual", "4 Bytes", "ptrCareer", ["016ECBEC"], dropdown=DD_BAL_SKILL_IDS)
builder.add_script(grp_bal_learn, "[ BAL 1-CLIQUE ] Aprender Habilidade em Treino Agora (9.999 / 10.000 XP)", make_lua_button_script("PES10.runPlayerMacro('bal_instant_skill')"), color=COL_GREEN)
builder.add_record(grp_bal_learn, "Posição em Treinamento Atual", "4 Bytes", "ptrCareer", ["016ECBF0"], dropdown=DD_BAL_POS_IDS)
builder.add_script(grp_bal_learn, "[ BAL 1-CLIQUE ] Aprender Posição em Treino Agora (9.999 / 10.000 XP)", make_lua_button_script("PES10.runPlayerMacro('bal_instant_pos')"), color=COL_GREEN)

grp_bal_status = builder.group(sec_bal, "Status no Elenco & Confiança do Técnico (BAL)", COL_PURPLE)
builder.add_record(grp_bal_status, "Status no Clube Principal (0=Reserva .. 3=Titular/Ídolo)", "Byte", "ptrCareer", ["01818429"])
builder.add_record(grp_bal_status, "Flags de Moral / Titularidade no Clube", "Byte", "ptrCareer", ["0181842C"])
builder.add_record(grp_bal_status, "Status Secundário / Seleção", "Byte", "ptrCareer", ["0181952D"])

# ============================================================================
# SEÇÃO 6: [ JOGADOR & CONTRATOS ] EDIT PLAYER v10.0 PRO (150+ CAMPOS + MACROS)
# ============================================================================
sec_player = builder.group(top_entries, "[ JOGADOR & CONTRATOS ] Edit Player v10.0 Pro (Contratos, BMPES & 150+ Campos)", COL_TEAL)

builder.add_script(
    sec_player,
    "[ BUSCA GLOBAL ] Procurar Qualquer Jogador da Carreira por Nome ou ID (30.000 Atletas)",
    make_lua_button_script("PES10.searchCareerPlayerDialog()"),
    color=COL_BLUE,
)

# 6.1 Contratos Ativos & Taxa de Rescisão
grp_contract = builder.group(sec_player, "1. Contrato Ativo, Taxa de Rescisão (Multa) & Transferência [NOVO v10.0]", COL_TEAL)
builder.add_record(grp_contract, "Possui Taxa de Rescisão / Multa Rescisória", "Binary", "ptrPlayer", ["147"], dropdown="0:Não (Sem Multa)\n1:Sim (Com Multa Rescisória)", bitstart="6", bitlen="1", color=COL_TEAL)
builder.add_record(grp_contract, "Taxa de Rescisão Atual (Visualização Calculada)", "4 Bytes", "valComputedReleaseFee", signed=True, is_money=True, color=COL_TEAL)
builder.custom_rfee_id = builder.add_record(
    grp_contract,
    "Fixar Taxa de Rescisão Exata neste Atleta (Duplo-Clique: ex. 1k, 80M, 1B — Requer Hook 2)",
    "4 Bytes",
    "valCustomReleaseFee",
    signed=True,
    is_money=True,
    color=COL_GREEN,
)
builder.add_script(
    grp_contract,
    "[ 1-CLIQUE ] Remover Override de Multa Exata deste Jogador",
    make_lua_button_script("local p=PES10.getSelectedPlayerOrWarn(); if p~=0 then PES10.clearOverrideReleaseFeeForPlayer(p) end"),
)
builder.add_record(grp_contract, "Fator de Valorização na Base (Youth Team Slot)", "4 Bytes", "ptrPlayerReleaseSlot", ["08"], signed=True)
builder.add_record(grp_contract, "Salário Anual Ativo (Duplo-Clique: ex. 5M, 500k)", "4 Bytes", "ptrPlayer", ["15C"], signed=True, is_money=True, color=COL_GREEN)
builder.add_record(grp_contract, "Valor de Mercado Base (Duplo-Clique: ex. 120M)", "4 Bytes", "ptrPlayer", ["174"], signed=True, is_money=True, color=COL_GREEN)
builder.add_record(grp_contract, "Idade Base do Valor de Mercado (+0x178 — Use 0 p/ Fixar Valor Exato)", "Byte", "ptrPlayer", ["178"])
builder.add_record(grp_contract, "Overall (OVR) Base do Valor de Mercado (+0x179)", "Byte", "ptrPlayer", ["179"])

builder.add_script(
    grp_contract,
    "[ MACRO ALVO ] Contratação Fácil neste Jogador (Ativa Multa € 1.000 + Afeição 100% + Destrava Data)",
    make_lua_button_script("if PES10.runPlayerMacro('easy_sign_target') then showMessage('Alvo preparado: Multa € 1.000 + Afeição 100%! Certifique-se de que o Hook 2 esteja ativo.') end"),
    color=COL_CRIMSON,
)

grp_c_bonus = builder.group(grp_contract, "Bônus Ativos na Folha Salarial (Quando no Meu Elenco ML)", COL_TEAL)
builder.add_record(grp_c_bonus, "Salário Comprometido na Folha do Clube", "4 Bytes", "ptrPlayerSquadBonus", ["08"], signed=True, is_money=True)
builder.add_record(grp_c_bonus, "Bônus Contratual #1 (Partida / Atuação)", "4 Bytes", "ptrPlayerSquadBonus", ["10"], signed=True, is_money=True)
builder.add_record(grp_c_bonus, "Bônus Contratual #2 (Gol / Vitória)", "4 Bytes", "ptrPlayerSquadBonus", ["18"], signed=True, is_money=True)
builder.add_record(grp_c_bonus, "Bônus Contratual #3 (Clean Sheet / Extra)", "4 Bytes", "ptrPlayerSquadBonus", ["28"], signed=True, is_money=True)

grp_c_dates = builder.group(grp_contract, "Datas de Término, Chegada & Última Renovação (Destravar Negociação)", COL_TEAL)
builder.add_record(grp_c_dates, "Término do Contrato — Ano", "2 Bytes", "ptrPlayer", ["138"])
builder.add_record(grp_c_dates, "Término do Contrato — Mês (1–12)", "Byte", "ptrPlayer", ["13A"])
builder.add_record(grp_c_dates, "Término do Contrato — Dia (1–31)", "Byte", "ptrPlayer", ["13B"])
builder.add_record(grp_c_dates, "Última Assinatura / Renovação — Ano (65535 = Sem Trava)", "2 Bytes", "ptrPlayer", ["16C"])
builder.add_record(grp_c_dates, "Última Assinatura / Renovação — Mês", "Byte", "ptrPlayer", ["16E"])
builder.add_record(grp_c_dates, "Última Assinatura / Renovação — Dia", "Byte", "ptrPlayer", ["16F"])
builder.add_record(grp_c_dates, "Chegada ao Clube — Ano", "2 Bytes", "ptrPlayer", ["168"])
builder.add_record(grp_c_dates, "Chegada ao Clube — Mês", "Byte", "ptrPlayer", ["16A"])
builder.add_record(grp_c_dates, "Chegada ao Clube — Dia", "Byte", "ptrPlayer", ["16B"])
builder.add_script(grp_c_dates, "[ 1-CLIQUE ] Destravar Renegociação Imediata (Limpa Trava de Assinatura Recente)", make_lua_button_script("PES10.runPlayerMacro('unlock_renegotiation')"), color=COL_BLUE)

grp_c_loan = builder.group(grp_contract, "Empréstimo, Lista de Transferências & Afeição Real (0–100%)", COL_TEAL)
builder.add_record(grp_c_loan, "Afeição pelo Clube / Treinador (0–100%)", "Binary", "ptrPlayer", ["13E"], bitstart="2", bitlen="7", color=COL_GREEN)
builder.add_record(grp_c_loan, "Está Emprestado (Is On Loan)", "Binary", "ptrPlayer", ["14A"], dropdown=DD_YES_NO, bitstart="0", bitlen="1")
builder.add_record(grp_c_loan, "Clube Original / Detentor do Passe (BMPES)", "4 Bytes", "ptrPlayer", ["164"], is_club_handle=True)
builder.add_record(grp_c_loan, "Clube Anterior / Origem do Empréstimo (BMPES)", "4 Bytes", "ptrPlayer", ["160"], is_club_handle=True)
builder.add_record(grp_c_loan, "Na Lista de Transferências", "Binary", "ptrPlayer", ["14A"], dropdown=DD_YES_NO, bitstart="1", bitlen="1")
builder.add_record(grp_c_loan, "Na Lista de Empréstimo", "Binary", "ptrPlayer", ["14A"], dropdown=DD_YES_NO, bitstart="2", bitlen="1")
builder.add_record(grp_c_loan, "Jogador Listado", "Binary", "ptrPlayer", ["13F"], dropdown=DD_YES_NO, bitstart="1", bitlen="1")

# 6.2 Macros de 1 Clique & JSON do Jogador
grp_p_macros = builder.group(sec_player, "2. Macros de 1 Clique & Backup JSON (Jogador Selecionado)", COL_BLUE)
builder.add_script(grp_p_macros, "[ 1-CLIQUE ] Todos os 25 Atributos em 99", make_lua_button_script("PES10.runPlayerMacro('stats_99')"), color=COL_GREEN)
builder.add_script(grp_p_macros, "[ 1-CLIQUE ] Somar +5 em Todos os Atributos (Até 99)", make_lua_button_script("PES10.runPlayerMacro('stats_plus_5')"), color=COL_BLUE)
builder.add_script(grp_p_macros, "[ 1-CLIQUE ] Todas as 13 Posições Jogáveis em Aptidão 'A'", make_lua_button_script("PES10.runPlayerMacro('all_positions_a')"), color=COL_PURPLE)
builder.add_script(grp_p_macros, "[ 1-CLIQUE ] Desbloquear Todas as 41 Habilidades (Player Skills)", make_lua_button_script("PES10.runPlayerMacro('all_39_skills')"), color=COL_AMBER)
builder.add_script(grp_p_macros, "[ 1-CLIQUE ] Pé Ruim 4/4 + Forma 8 + Resistência a Lesão 3 + Curar", make_lua_button_script("PES10.runPlayerMacro('perfect_wf_form_injury')"), color=COL_TEAL)
builder.add_script(grp_p_macros, "[ BACKUP JSON ] Exportar Jogador Selecionado para Arquivo .json", make_lua_button_script("PES10.exportPlayerBackupJson()"))
builder.add_script(grp_p_macros, "[ RESTORE JSON ] Importar Arquivo .json no Jogador Selecionado", make_lua_button_script("PES10.importPlayerBackupJson()"))

# 6.3 Identidade, Nomes UTF-8 60B & Clubes BMPES
grp_p_info = builder.group(sec_player, "3. Identidade, Nomes UTF-8 (60B) & Clubes BMPES 16.0", COL_TEAL)
builder.add_record(grp_p_info, "ID do Jogador (Player ID)", "4 Bytes", "ptrPlayer", ["30"])
builder.add_record(grp_p_info, "Nome do Jogador (UTF-8 — Até 60 Bytes)", "String", "ptrPlayer", ["38"], str_len=60, utf8=True)
builder.add_record(grp_p_info, "Nome na Camisa — Clube (UTF-8 — Até 60 Bytes)", "String", "ptrPlayer", ["75"], str_len=60, utf8=True)
builder.add_record(grp_p_info, "Nome na Camisa — Seleção (UTF-8 — Até 60 Bytes)", "String", "ptrPlayer", ["B2"], str_len=60, utf8=True)
builder.add_record(grp_p_info, "Clube Atual (Handle Decodificado c/ Nome BMPES)", "4 Bytes", "ptrPlayer", ["12C"], is_club_handle=True, color=COL_GREEN)
builder.add_record(grp_p_info, "Seleção Nacional (Handle Decodificado c/ Nome BMPES)", "4 Bytes", "ptrPlayer", ["130"], is_club_handle=True)
builder.add_record(grp_p_info, "Nacionalidade / País (221 Países)", "2 Bytes", "ptrPlayer", ["144"], dropdown=DROPDOWNS["Nationality"])
builder.add_record(grp_p_info, "Idade (15–50)", "Binary", "ptrPlayer", ["1C"], bitstart="0", bitlen="6")
builder.add_record(grp_p_info, "Altura (cm)", "Binary", "ptrPlayer", ["0"], bitstart="0", bitlen="8")
builder.add_record(grp_p_info, "Peso (kg)", "Binary", "ptrPlayer", ["1"], bitstart="0", bitlen="8")
builder.add_record(grp_p_info, "Pé Dominante", "Binary", "ptrPlayer", ["3"], dropdown=DD_FOOT, bitstart="7", bitlen="1")
builder.add_record(grp_p_info, "Comemoração de Gol Principal (+0x02)", "Byte", "ptrPlayer", ["2"])
builder.add_record(grp_p_info, "ID de Narração (Commentary ID)", "4 Bytes", "ptrPlayer", ["34"])

# 6.4 Condição Física, Seta de Forma, Pé Ruim & Lesão
grp_p_cond = builder.group(sec_player, "4. Condição Física, Seta de Forma, Pé Ruim & Lesões", COL_TEAL)
builder.add_record(grp_p_cond, "Barra de Fôlego / Stamina (0–100)", "Binary", "ptrPlayer", ["146"], bitstart="0", bitlen="7")
builder.add_record(grp_p_cond, "Seta de Forma para a Próxima Partida", "Binary", "ptrPlayer", ["147"], dropdown=DD_FORM_ARROW, bitstart="0", bitlen="3")
builder.add_record(grp_p_cond, "Seta Piscando (Em Destaque)", "Binary", "ptrPlayer", ["146"], dropdown=DD_YES_NO, bitstart="7", bitlen="1")
builder.add_record(grp_p_cond, "Dias Indisponível por Lesão (0 = Saudável)", "Byte", "ptrPlayer", ["148"])
builder.add_record(grp_p_cond, "Uso do Pé Ruim (1–4)", "Binary", "ptrPlayer", ["23"], dropdown=DROPDOWNS["Weak Foot Usage"], bitstart="0", bitlen="2")
builder.add_record(grp_p_cond, "Precisão do Pé Ruim (1–4)", "Binary", "ptrPlayer", ["22"], dropdown=DROPDOWNS["Weak Foot Accuracy"], bitstart="4", bitlen="2")
builder.add_record(grp_p_cond, "Forma Física / Regularidade (1–8)", "Binary", "ptrPlayer", ["17"], dropdown=DROPDOWNS["Form"], bitstart="4", bitlen="3")
builder.add_record(grp_p_cond, "Resistência a Lesões (1–3)", "Binary", "ptrPlayer", ["22"], dropdown=DROPDOWNS["Injury Resistance"], bitstart="6", bitlen="2")

# 6.5 Posições, 6.6 Atributos, 6.7 Playing Style & Skills, 6.8 COM Styles, 6.9 Team Role (Extraídos com 100% de fidelidade de bits da v21.1.0!)
grp_p_pos = builder.group(sec_player, "5. Posição de Origem & 13 Posições Jogáveis (C / B / A)", COL_TEAL)
grp_p_attr = builder.group(sec_player, "6. Todos os 25 Atributos Técnicos, Físicos, Defensivos & GK (40–99)", COL_TEAL)
grp_p_skills = builder.group(sec_player, "7. Estilo de Jogo & Todas as 41 Habilidades (Player Skills)", COL_TEAL)
grp_p_com = builder.group(sec_player, "8. Estilos de Jogo da IA (7 COM Playing Styles)", COL_TEAL)
grp_p_role = builder.group(sec_player, "9. Papel no Time & Personalidade (Team Role)", COL_TEAL)

POS_DESCS = {
    "CF - Center Forward", "SS - Second Striker", "LWF - Left Wing Forward", "RWF - Right Wing Forward",
    "AMF - Attacking Midfielder", "LMF - Left Midfielder", "RMF - Right Midfielder", "CMF - Center Midfielder",
    "DMF - Defensive Midfielder", "LB - Left Back", "RB - Right Back", "CB - Center Back", "GK - Goalkeeper",
}
COM_DESCS = {
    "Trickster", "Mazing Run", "Speeding Bullet", "Incisive Run", "Long Ball Expert", "Early Cross", "Long Ranger",
}
ROLE_DESCS = {
    "Team Role", "Level", "Team Player - Lone Wolf", "Passion - Composure", "Technique - Strength", "Insight - Instinct", "Impact",
}

all_stat_bits = []
all_skill_bits = []

for idx, pe in enumerate(legacy["player_entries"]):
    d = pe["desc"]
    off = pe["offs"]
    vt = pe["type"]
    bs = pe["bitstart"]
    bl = pe["bitlen"]
    pt_name = PT_PLAYER_LABELS.get(d, d)

    if d == "Registred Position":
        builder.add_record(grp_p_pos, pt_name, vt, "ptrPlayer", off, dropdown=DD_REG_POS, bitstart=bs, bitlen=bl)
    elif d in POS_DESCS:
        builder.add_record(grp_p_pos, pt_name, vt, "ptrPlayer", off, dropdown=DD_POS_GRADE, bitstart=bs, bitlen=bl)
    elif 32 <= idx <= 56:
        # Os 25 atributos de 7 bits (índices 32..56 no dump da v21.1.0!)
        builder.add_record(grp_p_attr, pt_name, vt, "ptrPlayer", off, bitstart=bs, bitlen=bl)
        all_stat_bits.append((int(off[0], 16), int(bs)))
    elif d == "Playing Style":
        builder.add_record(grp_p_skills, pt_name, vt, "ptrPlayer", off, dropdown=DD_PLAYING_STYLE, bitstart=bs, bitlen=bl, color=COL_PURPLE)
    elif 57 <= idx <= 97:
        # As 39 Player Skills (índices 57..97; nota: índice 66 também chama "Heading" mas é 1 bit em off=['25']!)
        sk_label = "Cabeceio Preciso — Skill (Heading)" if (d == "Heading" and bl == "1") else pt_name
        builder.add_record(grp_p_skills, sk_label, vt, "ptrPlayer", off, dropdown=DD_YES_NO, bitstart=bs, bitlen=bl)
        all_skill_bits.append((int(off[0], 16), int(bs)))
    elif d in COM_DESCS:
        builder.add_record(grp_p_com, pt_name, vt, "ptrPlayer", off, dropdown=DD_YES_NO, bitstart=bs, bitlen=bl)
    elif d in ROLE_DESCS:
        dd = DROPDOWNS.get("Team Role") if d == "Team Role" else None
        builder.add_record(grp_p_role, pt_name, vt, "ptrPlayer", off, dropdown=dd, bitstart=bs, bitlen=bl)

# Debug Pointers discretos no final do grupo Jogador
grp_p_dbg = builder.group(sec_player, "10. Diagnóstico de Ponteiros (Avançado)", None)
builder.add_record(grp_p_dbg, "Endereço Base ptrPlayer", "8 Bytes", "ptrPlayer", [])
builder.add_record(grp_p_dbg, "Endereço Base ptrCareer", "8 Bytes", "ptrCareer", [])
builder.add_record(grp_p_dbg, "Endereço Slot Sub-XP (ptrPlayerTraining)", "8 Bytes", "ptrPlayerTraining", [])
builder.add_record(grp_p_dbg, "Endereço Slot Bônus Folha (ptrPlayerSquadBonus)", "8 Bytes", "ptrPlayerSquadBonus", [])

# ============================================================================
# SEÇÃO 7: [ GAMEPLAY ] CONTROLE DE PARTIDA AO VIVO (RELÓGIO, PLACAR & STAMINA)
# ============================================================================
sec_match = builder.group(top_entries, "[ GAMEPLAY ] Partida ao Vivo — Relógio Exato, Placar, Pênaltis & Stamina", COL_CRIMSON)

builder.clock_id = builder.add_record(
    sec_match,
    "[ RELÓGIO ] Tempo da Partida (Duplo-Clique p/ Definir Minuto Exato! Ex: 15, 44, 88)",
    "Float",
    "ptrMatchState",
    ["1174"],
    color=COL_CRIMSON,
)
builder.add_record(
    sec_match,
    "Período da Partida",
    "4 Bytes",
    "ptrMatchState",
    ["1168"],
    dropdown="1:1º Tempo\n3:2º Tempo\n5:1ª Prorrogação\n7:2ª Prorrogação",
)
builder.add_record(
    sec_match,
    "Congelar Relógio da Partida (Requer Hook 5 Ativo)",
    "4 Bytes",
    "cfgFreezeClock",
    dropdown="0:Desativado (Relógio Correndo Normal)\n1:Congelado no Segundo Atual",
    dropdown_only=True,
)

builder.add_script(sec_match, "[ MINUTO 44' ] Pular para 44:00 (Fim do 1º Tempo)", make_lua_button_script("PES10.setExactMatchMinute(44)"), color=COL_BLUE)
builder.add_script(sec_match, "[ MINUTO 88' ] Pular para 88:00 (Minutos Finais)", make_lua_button_script("PES10.setExactMatchMinute(88)"), color=COL_BLUE)
builder.add_script(sec_match, "[ +10 MIN ] Avançar +10 Minutos no Relógio", make_lua_button_script("PES10.shiftMatchMinutes(10)"))
builder.add_script(sec_match, "[ -10 MIN ] Voltar -10 Minutos no Relógio", make_lua_button_script("PES10.shiftMatchMinutes(-10)"))
builder.add_script(sec_match, "[ 00:00 ] Reiniciar Tempo Atual (Reset Half)", make_lua_button_script("PES10.resetCurrentHalfNow()"))
builder.add_script(sec_match, "[ APITO FINAL ] Encerrar Tempo Atual Agora (End Current Half)", make_lua_button_script("PES10.endCurrentHalfNow()"), color=COL_CRIMSON)
builder.add_record(sec_match, "Minutos de Acréscimo no Placar (-1 = Sem Acréscimo)", "4 Bytes", "ptrMatchHud", ["144"], signed=True)

grp_m_score = builder.group(sec_match, "Placar ao Vivo & Disputa de Pênaltis (Requer Hook 5 Ativo)", COL_CRIMSON)
home_score_id = builder.add_record(grp_m_score, "Gols — Time Mandante (Casa) [Sincroniza Motor + Placar]", "Byte", "ptrMatchCore", ["9E"], color=COL_GREEN)
away_score_id = builder.add_record(grp_m_score, "Gols — Time Visitante (Fora) [Sincroniza Motor + Placar]", "Byte", "ptrMatchCore", ["9F"], color=COL_GREEN)
builder.score_sync[home_score_id] = "home"
builder.score_sync[away_score_id] = "away"
builder.add_record(grp_m_score, "Pênaltis Convertidos — Mandante (Casa)", "Byte", "ptrMatchCore", ["12C"])
builder.add_record(grp_m_score, "Pênaltis Convertidos — Visitante (Fora)", "Byte", "ptrMatchCore", ["12D"])

grp_m_stam = builder.group(sec_match, "Stamina Seletiva em Campo (Requer Hook 6 Ativo)", COL_CRIMSON)
builder.add_record(
    grp_m_stam,
    "Modo de Stamina durante a Partida",
    "4 Bytes",
    "cfgStaminaMode",
    dropdown=(
        "0:Desativado (Desgaste Original)\n"
        "1:Stamina 100% — Apenas Time Mandante (Casa)\n"
        "2:Stamina 100% — Apenas Time Visitante (Fora)\n"
        "3:Stamina 100% — Ambos os Times\n"
        "4:Pressão Total Casa (Casa 100% + Drenar Visitante p/ 5%)\n"
        "5:Pressão Total Fora (Fora 100% + Drenar Casa p/ 5%)"
    ),
    dropdown_only=True,
    color=COL_GREEN,
)

# ============================================================================
# GERAÇÃO DO BLOCO <LuaScript> COM TODOS OS DICIONÁRIOS E IDS VINCULADOS
# ============================================================================
teams_lua_lines = ["{"]
for tid_str, tname in sorted(bmpes["teams"].items(), key=lambda kv: int(kv[0])):
    safe_name = tname.replace("\\", "\\\\").replace('"', '\\"')
    teams_lua_lines.append(f'  [{int(tid_str)}] = "{safe_name}",')
teams_lua_lines.append("}")

comps_lua_lines = ["{"]
for cid_str, cname in sorted(bmpes["comps"].items(), key=lambda kv: int(kv[0])):
    safe_name = cname.replace("\\", "\\\\").replace('"', '\\"')
    comps_lua_lines.append(f'  [{int(cid_str)}] = "{safe_name}",')
comps_lua_lines.append("}")

money_map_lua = "{ " + ", ".join(f"[{i}]=true" for i in builder.money_ids) + " }"
club_map_lua = "{ " + ", ".join(f"[{i}]=true" for i in builder.club_handle_ids) + " }"
score_map_lua = "{ " + ", ".join(f'[{k}]="{v}"' for k, v in builder.score_sync.items()) + " }"
stats_bits_lua = "{ " + ", ".join(f"{{0x{o:02X},{b}}}" for o, b in all_stat_bits) + " }"
skills_bits_lua = "{ " + ", ".join(f"{{0x{o:02X},{b}}}" for o, b in all_skill_bits) + " }"

hook_map_lua = "{ " + ", ".join(f"[{idx+1}]={hid}" for idx, hid in enumerate(builder.hook_ids)) + " }"
lua_config_injection = f"""
PES10.TOP_FORM_VALUE = 4
PES10.HOOK_IDS_BY_NUM = {hook_map_lua}
PES10.PLAYER_STAT_BITS = {stats_bits_lua}
PES10.ALL_SKILL_BITS = {skills_bits_lua}
PES10.MONEY_DESC_TAGS = {money_map_lua}
PES10.CLUB_HANDLE_IDS = {club_map_lua}
PES10.CLOCK_RECORD_ID = {builder.clock_id}
PES10.STATUS_RECORD_ID = {builder.status_id}
PES10.CUSTOM_RFEE_RECORD_ID = {builder.custom_rfee_id}
PES10.SCORE_SYNC_IDS = {score_map_lua}
"""

final_lua = (
    lua_template
    .replace("__BMPES_TEAMS_LUA__", "\n".join(teams_lua_lines))
    .replace("__BMPES_COMPS_LUA__", "\n".join(comps_lua_lines))
    .replace("-- __PES10_CONFIG_INJECTION__", lua_config_injection)
)

ET.SubElement(root, "UserdefinedSymbols")
lua_el = ET.SubElement(root, "LuaScript")
lua_el.text = final_lua

# Pretty-print XML preservando scripts multilinhas intactos
ET.indent(root, space="  ")
xml_bytes = ET.tostring(root, encoding="utf-8", xml_declaration=True)

OUTPUT_CT.parent.mkdir(parents=True, exist_ok=True)
with open(OUTPUT_CT, "wb") as f:
    f.write(xml_bytes)

print(f"SUCCESS: Generated {OUTPUT_CT}")

# Deploy opcional para pasta local do jogo via argumento CLI ou .env.local (ignorado no Git)
parser = argparse.ArgumentParser(add_help=False)
parser.add_argument("--deploy-to", default=None)
args, _ = parser.parse_known_args()
deploy_target = args.deploy_to
env_local = REPO_ROOT / ".env.local"
if not deploy_target and env_local.exists():
    for line in env_local.read_text(encoding="utf-8").splitlines():
        if line.startswith("DEPLOY_CT_PATH="):
            deploy_target = line.split("=", 1)[1].strip()
if deploy_target:
    dp = pathlib.Path(deploy_target)
    if dp.parent.exists():
        shutil.copy2(OUTPUT_CT, dp)
        print(f"DEPLOYED: Synced copy to {dp}")
print(f"Total entries: {builder.next_id - 1}")
print(f"Money fields bound: {len(builder.money_ids)}")
print(f"Club handles bound: {len(builder.club_handle_ids)}")
print(f"Player skills bound: {len(all_skill_bits)}")
