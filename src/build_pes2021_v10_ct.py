import argparse
import json
import pathlib
import shutil
import xml.etree.ElementTree as ET

SRC_DIR = pathlib.Path(__file__).resolve().parent
REPO_ROOT = SRC_DIR.parent
DATA_DIR = SRC_DIR / "data"
DIST_DIR = REPO_ROOT / "dist"

OUTPUT_PT = DIST_DIR / "PES 2021 - v11.0 Ultimate Studio (PT-BR).CT"
OUTPUT_EN = DIST_DIR / "PES 2021 - v11.0 Ultimate Studio (EN).CT"

with open(DATA_DIR / "player_schema.json", "r", encoding="utf-8") as f:
    legacy = json.load(f)

with open(DATA_DIR / "bmpes_dicts.json", "r", encoding="utf-8") as f:
    bmpes = json.load(f)

with open(DATA_DIR / "konami_ovr_tables.json", "r", encoding="utf-8") as f:
    ovr_tables = json.load(f)

with open(SRC_DIR / "pes10_lua_engine.lua", "r", encoding="utf-8") as f:
    lua_template = f.read()

DROPDOWNS = legacy["dropdowns"]

# Rich PT-BR translations for the 129 legacy player fields
PT_PLAYER_LABELS = {
    "Player ID": "ID do Jogador (Player ID)",
    "Commentary ID": "ID de Narração (Commentary ID)",
    "Player Name": "Nome do Jogador (UTF-8 60B)",
    "Name on Strip (Club)": "Nome na Camisa - Clube (UTF-8 60B)",
    "Name on Strip (National Team)": "Nome na Camisa - Seleção (UTF-8 60B)",
    "Nationality": "Nacionalidade / País",
    "Height (cm)": "Altura (cm)",
    "Weight (kg)": "Peso (kg)",
    "Age": "Idade (15-50)",
    "Stronger Foot": "Pé Dominante",
    "Registred Position": "Posição de Origem (Registered Position)",
    "CF - Center Forward": "CA (CF) - Centroavante",
    "SS - Second Striker": "SA (SS) - Segundo Atacante",
    "LWF - Left Wing Forward": "PTE (LWF) - Ponta Esquerda",
    "RWF - Right Wing Forward": "PTD (RWF) - Ponta Direita",
    "AMF - Attacking Midfielder": "MAT (AMF) - Meia Atacante",
    "DMF - Defensive Midfielder": "VOL (DMF) - Volante",
    "CMF - Center Midfielder": "MLG (CMF) - Meia de Ligação",
    "LMF - Left Midfielder": "MLE (LMF) - Meia Esquerda",
    "RMF - Right Midfielder": "MLD (RMF) - Meia Direita",
    "CB - Center Back": "ZAG (CB) - Zagueiro",
    "LB - Left Back": "LE (LB) - Lateral Esquerdo",
    "RB - Right Back": "LD (RB) - Lateral Direito",
    "GK - Goalkeeper": "GL (GK) - Goleiro",
    "Weak Foot Usage": "Uso do Pé Ruim (1-4)",
    "Weak Foot Accuracy": "Precisão do Pé Ruim (1-4)",
    "Form": "Forma Física / Condicionamento (1-8)",
    "Injury Resistance": "Resistência a Lesões (1-3)",
    "Offensive Awareness": "Talento Ofensivo (Offensive Awareness)",
    "Ball Control": "Controle de Bola (Ball Control)",
    "Dribbling": "Drible (Dribbling)",
    "Tight Possession": "Condução Firme (Tight Possession)",
    "Low Pass": "Passe Rasteiro (Low Pass)",
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

EN_PLAYER_LABELS = {
    "Registred Position": "Registered Position",
    "Step On Skill control": "Step On Skill Control",
    "Chip shot control": "Chip Shot Control",
    "Trickster": "[COM AI] Trickster",
    "Mazing Run": "[COM AI] Mazing Run",
    "Speeding Bullet": "[COM AI] Speeding Bullet",
    "Incisive Run": "[COM AI] Incisive Run",
    "Long Ball Expert": "[COM AI] Long Ball Expert",
    "Early Cross": "[COM AI] Early Cross",
    "Long Ranger": "[COM AI] Long Ranger",
    "Level": "Team Role Level",
}

# ============================================================================
# VERIFIED x64 AUTO ASSEMBLER SCRIPTS (PES2021.exe v1.7.2.0 + Sider 7.3.3)
# ============================================================================
ASM_HOOK_1_PLAYER = """{ PES 2021 v11.0 Pro - Hook 1: Automatic Player Capture (Edit / Squad / Transfer) }
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

ASM_HOOK_2_RELEASE_FEE = """{ PES 2021 v11.0 Pro - Hook 2: Exact Release Fee Override Engine (VA 0x14156BE0C) }
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

ASM_HOOK_3_TRAINING_MULT = """{ PES 2021 v11.0 Pro - Hook 3: Universal Sub-XP Multiplier (User Club / Youth / BAL Only) }
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

ASM_HOOK_4_BAL_CAP = """{ PES 2021 v11.0 Pro - Hook 4: Unlock 21 Focus Points Slider Cap in BAL (Up to 127) }
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

ASM_HOOK_5_MATCH_HUD_CLOCK = """{ PES 2021 v11.0 Pro - Hook 5: Sider-Safe Match HUD, Live Score, Penalties & FNV-Safe Clock }
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

ASM_HOOK_6_STAMINA = """{ PES 2021 v11.0 Pro - Hook 6: Selective Match Stamina (Home / Away / Both / Total Press) }
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


def serialize_ovr_tables_lua(data: dict) -> str:
    def int_list(arr: list[int]) -> str:
        return "{" + ",".join(str(int(x)) for x in arr) + "}"

    pairs_str = "{" + ",".join(f"{{{p[0]},{p[1]}}}" for p in data["field_to_slot"]) + "}"
    w_rows = [int_list(row) for row in data["weights_23x13"]]
    lines = [
        "{",
        f"  field_to_slot = {pairs_str},",
        f"  pos_order = {int_list(data['pos_order'])},",
        f"  stat_indices_23 = {int_list(data['stat_indices_23'])},",
        "  weights_23x13 = {",
        *[f"    {r}," for r in w_rows],
        "  },",
        f"  base_reg = {int_list(data['base_reg'])},",
        f"  base_apt_a = {int_list(data['base_apt_a'])},",
        f"  base_apt_b = {int_list(data['base_apt_b'])},",
        f"  avg_indices_22 = {int_list(data['avg_indices_22'])},",
        f"  curve_100 = {int_list(data['curve_100'])},",
        "}",
    ]
    return "\n".join(lines)


KONAMI_OVR_LUA_STR = serialize_ovr_tables_lua(ovr_tables)

TEAMS_LUA_LINES = ["{"]
for tid_str, tname in sorted(bmpes["teams"].items(), key=lambda kv: int(kv[0])):
    safe_name = tname.replace("\\", "\\\\").replace('"', '\\"')
    TEAMS_LUA_LINES.append(f'  [{int(tid_str)}] = "{safe_name}",')
TEAMS_LUA_LINES.append("}")
TEAMS_LUA_STR = "\n".join(TEAMS_LUA_LINES)

COMPS_LUA_LINES = ["{"]
for cid_str, cname in sorted(bmpes["comps"].items(), key=lambda kv: int(kv[0])):
    safe_name = cname.replace("\\", "\\\\").replace('"', '\\"')
    COMPS_LUA_LINES.append(f'  [{int(cid_str)}] = "{safe_name}",')
COMPS_LUA_LINES.append("}")
COMPS_LUA_STR = "\n".join(COMPS_LUA_LINES)


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
        self.computed_ovr_id = -1
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


def build_edition(lang: str, output_ct: pathlib.Path) -> dict:
    assert lang in ("pt", "en")
    is_en = lang == "en"

    def L(pt_s: str, en_s: str) -> str:
        return en_s if is_en else pt_s

    dd_yes_no = L("0:Não\n1:Sim", "0:No\n1:Yes")
    dd_foot = L("0:Direito (Right)\n1:Esquerdo (Left)", "0:Right Foot\n1:Left Foot")
    dd_pos_grade = L(
        "0:C (Sem Aptidão)\n1:B (Secundária)\n2:A (Domínio Total)",
        "0:C (Unfamiliar)\n1:B (Secondary)\n2:A (Full Mastery)",
    )
    dd_form_arrow = L(
        "0:0 - Péssima (Seta Vermelha p/ Baixo)\n"
        "1:1 - Ruim (Seta Laranja Diagonal)\n"
        "2:2 - Normal (Seta Amarela Horizontal)\n"
        "3:3 - Boa (Seta Verde Diagonal)\n"
        "4:4 - Excelente / Top Form (Seta Azul p/ Cima)",
        "0:0 - Terrible (Red Down Arrow)\n"
        "1:1 - Poor (Orange Diagonal Arrow)\n"
        "2:2 - Normal (Yellow Flat Arrow)\n"
        "3:3 - Good (Green Diagonal Arrow)\n"
        "4:4 - Top Form (Blue Up Arrow)",
    )
    dd_reg_pos = L(
        "0:GL (GK) - Goleiro\n"
        "1:ZAG (CB) - Zagueiro\n"
        "2:LE (LB) - Lateral Esquerdo\n"
        "3:LD (RB) - Lateral Direito\n"
        "4:VOL (DMF) - Volante\n"
        "5:MLG (CMF) - Meia de Ligação\n"
        "6:MLE (LMF) - Meia Esquerda\n"
        "7:MLD (RMF) - Meia Direita\n"
        "8:MAT (AMF) - Meia Atacante\n"
        "9:PTE (LWF) - Ponta Esquerda\n"
        "10:PTD (RWF) - Ponta Direita\n"
        "11:SA (SS) - Segundo Atacante\n"
        "12:CA (CF) - Centroavante",
        "0:GK - Goalkeeper\n"
        "1:CB - Center Back\n"
        "2:LB - Left Back\n"
        "3:RB - Right Back\n"
        "4:DMF - Defensive Midfielder\n"
        "5:CMF - Center Midfielder\n"
        "6:LMF - Left Midfielder\n"
        "7:RMF - Right Midfielder\n"
        "8:AMF - Attacking Midfielder\n"
        "9:LWF - Left Wing Forward\n"
        "10:RWF - Right Wing Forward\n"
        "11:SS - Second Striker\n"
        "12:CF - Center Forward",
    )
    dd_playing_style = L(
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
        "21:Lateral Finalizador (Full-back Finisher)",
        DROPDOWNS["Playing Style"],
    )
    dd_growth_curve = L(
        "0:0 - Precoce (Pico Cedo)\n"
        "1:1 - Padrão Equilibrado\n"
        "2:2 - Desenvolvimento Tardio\n"
        "3:3 - Duradouro / Longevo (Pouco Declínio 30+)\n"
        "4:4 - Precoce + Duradouro\n"
        "5:5 - Padrão + Duradouro\n"
        "6:6 - Tardio + Duradouro (Eterno Craque)\n"
        "7:7 - Curva Especial Lenda",
        "0:0 - Early Peak\n"
        "1:1 - Standard Balanced\n"
        "2:2 - Late Developer\n"
        "3:3 - Sustained / Longevity (Minimal 30+ Decline)\n"
        "4:4 - Early + Sustained\n"
        "5:5 - Standard + Sustained\n"
        "6:6 - Late + Sustained (Eternal Prime)\n"
        "7:7 - Special Legend Curve",
    )
    dd_bal_skill_ids = L(
        "\n".join([
            "60:Pedalada Simples (Scissors Feint)", "61:Toque Duplo (Double Touch)", "62:Elástico (Flip Flap)",
            "63:Roleta (Marseille Turn)", "64:Chapéu (Sombrero)", "65:Corte de Letra (Cross Over Turn)",
            "66:Puxada e Giro (Cut Behind & Turn)", "67:Toque Escocês (Scotch Move)", "68:Domínio Pisando na Bola (Step On Skill)",
            "69:Cabeceio (Heading)", "70:Chute Colocado de Longe (Long Range Drive)", "71:Cavadinha Precisa (Chip Shot Control)",
            "72:Chute de Longa Distância (Long Range Shooting)", "73:Folha Seca (Knuckle Shot)", "74:Chute com Queda (Dipping Shot)",
            "75:Chute Ascendente (Rising Shots)", "76:Finalização Acrobática (Acrobatic Finishing)", "77:Toque de Calcanhar (Heel Trick)",
            "78:Chute de Primeira (First-time Shot)", "79:Passe de Primeira (One-touch Pass)", "80:Passe em Profundidade (Through Passing)",
            "81:Passe na Medida (Weighted Pass)", "82:Cruzamento Preciso (Pinpoint Crossing)", "83:Trivela (Outside Curler)",
            "84:Letra (Rabona)", "85:Passe Sem Olhar (No Look Pass)", "86:Lançamento Tenso (Low Lofted Pass)",
            "87:Reposição Baixa GK", "88:Reposição Alta GK", "89:Lateral Longo (Long Throw)",
            "90:Arremesso Longo GK", "91:Especialista em Pênaltis", "92:Pegador de Pênaltis GK",
            "93:Malícia (Gamesmanship)", "94:Marcação Individual (Man Marking)", "95:Voltar para Marcar (Track Back)",
            "96:Interceptação (Interception)", "97:Corte Acrobático (Acrobatic Clear)", "98:Capitão / Liderança (Captaincy)",
            "99:Super Substituto (Super-Sub)", "100:Espírito Guerreiro (Fighting Spirit)", "108:Nenhuma (Sem Treino de Skill)",
        ]),
        "\n".join([
            "60:Scissors Feint", "61:Double Touch", "62:Flip Flap", "63:Marseille Turn", "64:Sombrero",
            "65:Cross Over Turn", "66:Cut Behind & Turn", "67:Scotch Move", "68:Step On Skill Control",
            "69:Heading", "70:Long Range Drive", "71:Chip Shot Control", "72:Long Range Shooting",
            "73:Knuckle Shot", "74:Dipping Shot", "75:Rising Shots", "76:Acrobatic Finishing",
            "77:Heel Trick", "78:First-time Shot", "79:One-touch Pass", "80:Through Passing",
            "81:Weighted Pass", "82:Pinpoint Crossing", "83:Outside Curler", "84:Rabona",
            "85:No Look Pass", "86:Low Lofted Pass", "87:GK Low Punt", "88:GK High Punt",
            "89:Long Throw", "90:GK Long Throw", "91:Penalty Specialist", "92:GK Penalty Saver",
            "93:Gamesmanship", "94:Man Marking", "95:Track Back", "96:Interception",
            "97:Acrobatic Clear", "98:Captaincy", "99:Super-Sub", "100:Fighting Spirit", "108:None (No Active Skill)",
        ]),
    )
    dd_bal_pos_ids = L(
        "1:CA (CF) - Centroavante\n2:SA (SS) - Segundo Atacante\n3:PTE (LWF) - Ponta Esquerda\n"
        "4:PTD (RWF) - Ponta Direita\n5:MAT (AMF) - Meia Atacante\n6:VOL (DMF) - Volante\n"
        "7:MLG (CMF) - Meia de Ligação\n8:MLE (LMF) - Meia Esquerda\n9:MLD (RMF) - Meia Direita\n"
        "10:ZAG (CB) - Zagueiro\n11:LE (LB) - Lateral Esquerdo\n12:LD (RB) - Lateral Direito\n"
        "13:GL (GK) - Goleiro\n108:Nenhuma (Sem Treino de Posição)",
        "1:CF - Center Forward\n2:SS - Second Striker\n3:LWF - Left Wing Forward\n"
        "4:RWF - Right Wing Forward\n5:AMF - Attacking Midfielder\n6:DMF - Defensive Midfielder\n"
        "7:CMF - Center Midfielder\n8:LMF - Left Midfielder\n9:RMF - Right Midfielder\n"
        "10:CB - Center Back\n11:LB - Left Back\n12:RB - Right Back\n"
        "13:GK - Goalkeeper\n108:None (No Active Position)",
    )

    builder = CTBuilder()
    root = ET.Element("CheatTable", {"CheatEngineTableVersion": "34"})
    top_entries = ET.SubElement(root, "CheatEntries")

    COL_AMBER   = "0066CC"
    COL_GREEN   = "3B8E1E"
    COL_PURPLE  = "A8326E"
    COL_BLUE    = "CC5500"
    COL_TEAL    = "807A00"
    COL_CRIMSON = "2828C6"

    # =========================================================================
    # SECTION 1: [ 1. CENTRAL STUDIO v11.0 ] (Expanded by default!)
    # =========================================================================
    sec_central = builder.group(
        top_entries,
        L("[ 1. CENTRAL STUDIO v11.0 ] Painel Gráfico, Telemetria, Moeda & Hooks", "[ 1. CENTRAL STUDIO v11.0 ] Studio GUI, Telemetry, Currency & Hooks"),
        COL_AMBER,
        hide_children=False,
    )

    builder.add_script(
        sec_central,
        L("[ ★ ABRIR PAINEL STUDIO ] Interface Gráfica Pro v11.0 (PT-BR / EN)", "[ ★ OPEN PRO STUDIO GUI ] Graphical Dashboard v11.0 (EN / PT-BR)"),
        make_lua_button_script("PES10.openDashboard()"),
        color=COL_BLUE,
    )

    builder.status_id = builder.add_record(
        sec_central,
        L("[ STATUS ] Telemetria ao Vivo (Modo / Clube / Jogador & OVR)", "[ STATUS ] Live Telemetry (Mode / Club / Player & OVR)"),
        "4 Bytes",
        "cfgCurrency",
        color=COL_AMBER,
    )

    builder.add_record(
        sec_central,
        L("[ MOEDA ] Seletor Rápido de Moeda (Inicia em Euro €)", "[ CURRENCY ] Quick Currency Selector (Defaults to Euro €)"),
        "4 Bytes",
        "cfgCurrency",
        dropdown=L(
            "0:Euro (€) - Padrão\n1:Real (R$)\n2:Dólar ($)\n3:Libra (£)",
            "0:Euro (€) - Default\n1:Brazilian Real (R$)\n2:US Dollar ($)\n3:British Pound (£)",
        ),
        dropdown_only=True,
        color=COL_AMBER,
    )

    grp_hooks = builder.group(
        sec_central,
        L("[ MÓDULOS x64 ] 6 Hooks Mestres (Ligados Automaticamente ao Conectar!)", "[ x64 MODULES ] 6 Master Hooks (Auto-Armed on Process Attach!)"),
        COL_AMBER,
        hide_children=True,
    )
    builder.add_script(grp_hooks, L("[ HOOK 1 ] Captura Automática de Jogador (Edit / Elenco / Negociações)", "[ HOOK 1 ] Automatic Player Capture (Edit / Squad / Negotiations)"), ASM_HOOK_1_PLAYER, is_core_hook=True)
    builder.add_script(grp_hooks, L("[ HOOK 2 ] Motor de Taxa de Rescisão Exata (Release Fee Override)", "[ HOOK 2 ] Exact Release Clause Override Engine"), ASM_HOOK_2_RELEASE_FEE, is_core_hook=True)
    builder.add_script(grp_hooks, L("[ HOOK 3 ] Multiplicador Universal de Sub-XP (Treino ML, Base & BAL)", "[ HOOK 3 ] Universal Sub-XP Multiplier (Club, Youth & BAL)"), ASM_HOOK_3_TRAINING_MULT, is_core_hook=True)
    builder.add_script(grp_hooks, L("[ HOOK 4 ] Destravar Limite de 21 Focus Points nas Setas do BAL", "[ HOOK 4 ] Unlock 21 Focus Points Cap on BAL Sliders"), ASM_HOOK_4_BAL_CAP, is_core_hook=True)
    builder.add_script(grp_hooks, L("[ HOOK 5 ] Partida ao Vivo - Placar, Pênaltis & Relógio (Sider-Safe)", "[ HOOK 5 ] Live Match - Score, Penalties & Clock (Sider-Safe)"), ASM_HOOK_5_MATCH_HUD_CLOCK, is_core_hook=True)
    builder.add_script(grp_hooks, L("[ HOOK 6 ] Stamina Seletiva em Campo (Casa / Fora / Ambos / Pressão)", "[ HOOK 6 ] Selective Match Stamina (Home / Away / Both / Press)"), ASM_HOOK_6_STAMINA, is_core_hook=True)

    builder.add_script(
        sec_central,
        L("[ REATIVAR HOOKS ] Garantir Todos os 6 Hooks Mestres Ligados Agora", "[ ARM ALL HOOKS ] Ensure All 6 Master Hooks Are Active Now"),
        make_lua_button_script("PES10.activateAllHooks(false)"),
        color=COL_GREEN,
    )

    # =========================================================================
    # SECTION 2: [ 2. MASTER LEAGUE - FINANCES & SQUAD COMMANDER ]
    # =========================================================================
    sec_ml = builder.group(
        top_entries,
        L("[ 2. MASTER LEAGUE ] Finanças Formatadas (€) & Squad Commander (Meu Elenco)", "[ 2. MASTER LEAGUE ] Formatted Finances (€) & Squad Commander"),
        COL_GREEN,
    )

    builder.add_record(sec_ml, L("Orçamento de Transferências (Duplo-Clique: ex. 150M, 500k)", "Transfer Budget (Double-Click: e.g. 150M, 500k)"), "4 Bytes", "ptrCareer", ["016ECBF4"], signed=True, is_money=True, color=COL_GREEN)
    builder.add_record(sec_ml, L("Orçamento de Salários (Duplo-Clique: ex. 30M, 500k)", "Wage / Salary Budget (Double-Click: e.g. 30M, 500k)"), "4 Bytes", "ptrCareer", ["016ECC08"], signed=True, is_money=True, color=COL_GREEN)

    builder.add_script(
        sec_ml,
        L("[ +50M ] Somar + 50 Milhões em Transferências", "[ +50M ] Add + 50 Million to Transfer Budget"),
        make_lua_button_script("local c=PES10.readU64(PES10.symAddr('ptrCareer')); if c~=0 then writeInteger(c+0x016ECBF4, math.min(2147483647, PES10.readI32(c+0x016ECBF4)+500000)); PES10.toast(PES10.T('+50M adicionados!','+50M added!')) end"),
        color=COL_BLUE,
    )
    builder.add_script(
        sec_ml,
        L("[ BILIONÁRIO ] Definir € 1,5 Bilhão (Transf.) + € 250 Milhões (Salários)", "[ TYCOON ] Set € 1.5 Billion (Transfer) + € 250 Million (Wages)"),
        make_lua_button_script("local c=PES10.readU64(PES10.symAddr('ptrCareer')); if c~=0 then writeInteger(c+0x016ECBF4, 15000000); writeInteger(c+0x016ECC08, 2500000); PES10.toast(PES10.T('Pacote € 1,5B ativado!','€ 1.5B Tycoon Pack applied!')) end"),
        color=COL_GREEN,
    )

    grp_squad = builder.group(
        sec_ml,
        L("Squad Commander - Comandos de 1 Clique em Todo o Meu Elenco", "Squad Commander - 1-Click Actions on Your Entire Squad"),
        COL_BLUE,
        hide_children=False,
    )
    builder.add_script(grp_squad, L("[ ELENCO ] 100% Fôlego (Stamina) em Todo o Meu Elenco", "[ SQUAD ] 100% Stamina Recovery for Entire Squad"), make_lua_button_script("PES10.runSquadCommander('stamina_100')"), color=COL_GREEN)
    builder.add_script(grp_squad, L("[ ELENCO ] Seta Azul Piscando (Top Form) em Todo o Elenco", "[ SQUAD ] Flashing Blue Arrow (Top Form) for Entire Squad"), make_lua_button_script("PES10.runSquadCommander('blue_form')"), color=COL_BLUE)
    builder.add_script(grp_squad, L("[ ELENCO ] Curar Todas as Lesões + Afeição 100% no Elenco", "[ SQUAD ] Heal All Injuries + 100% Club Affection"), make_lua_button_script("PES10.runSquadCommander('heal_affection')"), color=COL_TEAL)
    builder.add_script(grp_squad, L("[ ELENCO ] Blindar Contratos (+5 Anos + Remover Multas)", "[ SQUAD ] Shield All Squad Contracts (+5 Yrs & No Clause)"), make_lua_button_script("PES10.runSquadCommander('shield_contracts')"), color=COL_AMBER)
    builder.add_script(grp_squad, L("[ ELENCO ] Ativar Breakthrough (Auge) + Zero Declínio 30+", "[ SQUAD ] Trigger Squad Breakthrough + Zero 30+ Decline"), make_lua_button_script("PES10.runSquadCommander('eternal_peak_breakthrough')"), color=COL_PURPLE)
    builder.add_script(grp_squad, L("[ ★ ELENCO +1 OVR DNA ] Subir +1 Overall em Todo o Elenco (Mantém Estilo!)", "[ ★ SQUAD +1 OVR DNA ] Boost +1 Overall Across Squad (Preserves Style!)"), make_lua_button_script("PES10.runSquadCommander('plus_one_ovr_dna')"), color=COL_CRIMSON)

    grp_ml_scout = builder.group(
        sec_ml,
        L("Negociações, Radar 30K & Olheiros (RAM Real - 3 Listas + Shortlist)", "Negotiations, Radar 30K & Scouts (Live RAM - 3 Lists + Shortlist)"),
        COL_AMBER,
        hide_children=False,
    )
    builder.add_script(grp_ml_scout, L("[ CENTRAL OLHEIROS ] Abrir Gerenciador Visual dos 3 Olheiros & Favoritos (RAM Real)", "[ SCOUT HUB ] Open Visual Real-RAM Manager for Scouts 1-3 & Shortlist"), make_lua_button_script("PES10.openScoutListsModal()"), color=COL_GREEN)
    builder.add_script(grp_ml_scout, L("[ RADAR 30K ] Caçador Visual de Joias (Filtrar OVR / Idade / Posição / Contrato)", "[ RADAR 30K ] Visual Wonderkid Finder (Filter OVR / Age / Position / Contract)"), make_lua_button_script("PES10.openScoutFinderModal()"), color=COL_BLUE)
    builder.add_script(grp_ml_scout, L("[ NEGÓCIO 100% ] Negociação Irrecusável (Lista Transf. + Multa € 1k + 1º Contato)", "[ 100% DEAL ] Irresistible Deal (Transfer Listed + € 1k Fee + 1st Round)"), make_lua_button_script("PES10.makeDealIrresistible()"), color=COL_CRIMSON)
    builder.add_script(grp_ml_scout, L("[ LIMPAR ] Limpar as 3 Listas de Observação dos Olheiros na RAM", "[ CLEAR ] Clear All 3 Scout Observation Lists in RAM"), make_lua_button_script("PES10.clearScoutSlots()"), color=COL_BLUE)

    grp_ml_ledger = builder.group(sec_ml, L("Receitas Comerciais & Balanço da Temporada", "Commercial Revenue & Season Ledger"), COL_GREEN)
    builder.add_record(grp_ml_ledger, L("Receita do Clube de Sócios / Torcedores", "Fan Club Revenue"), "4 Bytes", "ptrCareer", ["016ECC20"], signed=True, is_money=True)
    builder.add_record(grp_ml_ledger, L("Receita de Merchandising / Produtos", "Merchandising Revenue"), "4 Bytes", "ptrCareer", ["016ECC24"], signed=True, is_money=True)
    builder.add_record(grp_ml_ledger, L("Receita de Patrocínios", "Sponsorship Revenue"), "4 Bytes", "ptrCareer", ["016ECC28"], signed=True, is_money=True)
    builder.add_record(grp_ml_ledger, L("Balanço de Transferências na Temporada", "Season Transfer Balance"), "4 Bytes", "ptrCareer", ["016EF298"], signed=True, is_money=True)
    builder.add_record(grp_ml_ledger, L("Balanço Salarial na Temporada", "Season Wage Balance"), "4 Bytes", "ptrCareer", ["016EF290"], signed=True, is_money=True)
    builder.add_record(grp_ml_ledger, L("Outras Despesas / Indenizações", "Other Expenses / Compensation"), "4 Bytes", "ptrCareer", ["016EF288"], signed=True, is_money=True)

    grp_ml_date = builder.group(sec_ml, L("Calendário da Carreira (Ano / Mês / Dia)", "Career Calendar (Year / Month / Day)"), COL_GREEN)
    builder.add_record(grp_ml_date, L("Ano Atual", "Current Year"), "2 Bytes", "ptrCareer", ["01642A24"])
    builder.add_record(grp_ml_date, L("Mês Atual (1-12)", "Current Month (1-12)"), "Byte", "ptrCareer", ["01642A26"])
    builder.add_record(grp_ml_date, L("Dia Atual (1-31)", "Current Day (1-31)"), "Byte", "ptrCareer", ["01642A27"])

    # =========================================================================
    # SECTION 3: [ 3. PLAYER, OVERALL DNA & CONTRACTS ]
    # =========================================================================
    sec_player = builder.group(
        top_entries,
        L("[ 3. JOGADOR, OVERALL DNA & CONTRATOS ] Edit Player v11.0 Pro (150+ Campos)", "[ 3. PLAYER, OVERALL DNA & CONTRACTS ] Edit Player v11.0 Pro (150+ Fields)"),
        COL_TEAL,
    )

    builder.add_script(
        sec_player,
        L("[ RADAR 30K ] Busca Visual Avançada por Nome, ID, Posição, OVR & Idade", "[ RADAR 30K ] Visual Search & Filter by Name, ID, Position, OVR & Age"),
        make_lua_button_script("PES10.openScoutFinderModal()"),
        color=COL_BLUE,
    )

    builder.computed_ovr_id = builder.add_record(
        sec_player,
        L("[ ★ OVERALL ATUAL ] Duplo-Clique p/ Escalar OVR Mantendo o DNA / Estilo!", "[ ★ CURRENT OVERALL ] Double-Click to Scale OVR Preserving DNA / Style!"),
        "4 Bytes",
        "valComputedOVR",
        signed=True,
        color=COL_GREEN,
    )

    builder.add_script(
        sec_player,
        L("[ +1 OVR DNA ] Subir +1 Overall Proporcionalmente (Sem Mudar Estilo de Jogo)", "[ +1 OVR DNA ] Boost +1 Overall Proportionally (Preserves Playing Style)"),
        make_lua_button_script("PES10.stepPlayerOVRProportional(1)"),
        color=COL_GREEN,
    )
    builder.add_script(
        sec_player,
        L("[ -1 OVR DNA ] Reduzir -1 Overall Proporcionalmente (Sem Mudar Estilo de Jogo)", "[ -1 OVR DNA ] Lower -1 Overall Proportionally (Preserves Playing Style)"),
        make_lua_button_script("PES10.stepPlayerOVRProportional(-1)"),
        color=COL_AMBER,
    )
    builder.add_script(
        sec_player,
        L("[ ★ LAB POSIÇÕES ] Abrir Laboratório Visual (+1 OVR Posição & Grau C / B / A)", "[ ★ POSITION LAB ] Open Visual Position Lab (+1 Pos OVR & Grade C / B / A)"),
        make_lua_button_script("PES10.openPositionLabModal()"),
        color=COL_GREEN,
    )
    builder.add_script(
        sec_player,
        L("[ TEAM ROLE ] Abrir Seletor Visual de Função de Equipe (22 Papéis + Nível 1-3)", "[ TEAM ROLE ] Open Visual Team Role Picker (22 Roles + Level 1-3)"),
        make_lua_button_script("PES10.openTeamRoleModal()"),
        color=COL_AMBER,
    )
    builder.add_script(
        sec_player,
        L("[ DESFAZER / UNDO ] Reverter Última Edição no Jogador Selecionado (Até 10 Níveis)", "[ UNDO EDIT ] Revert Last Edit on Selected Player (10-Level History)"),
        make_lua_button_script("PES10.undoLastPlayerEdit()"),
        color=COL_CRIMSON,
    )
    builder.add_script(
        sec_player,
        L("[ RAIO-X 13 POS ] Ver Melhor Posição Natural & Top 5 Atributos DNA", "[ 13-POS X-RAY ] View Best Natural Position & Top 5 DNA Attributes"),
        make_lua_button_script("PES10.showPlayerDNAReport()"),
        color=COL_PURPLE,
    )

    # 3.1 Active Contract & Release Clause
    grp_contract = builder.group(sec_player, L("1. Contrato Ativo, Salário, Multa Exata & Contratação € 1k", "1. Active Contract, Salary, Exact Release Fee & € 1k Sign"), COL_TEAL)
    builder.add_record(grp_contract, L("Possui Taxa de Rescisão / Multa Rescisória", "Has Release Clause"), "Binary", "ptrPlayer", ["147"], dropdown=L("0:Não (Sem Multa)\n1:Sim (Com Multa Rescisória)", "0:No (No Clause)\n1:Yes (Has Release Clause)"), bitstart="6", bitlen="1", color=COL_TEAL)
    builder.add_record(grp_contract, L("Taxa de Rescisão Atual (Visualização Calculada)", "Current Release Clause (Live Computed)"), "4 Bytes", "valComputedReleaseFee", signed=True, is_money=True, color=COL_TEAL)
    builder.custom_rfee_id = builder.add_record(
        grp_contract,
        L("Fixar Taxa de Rescisão Exata neste Atleta (Duplo-Clique: ex. 1k, 80M, 1B)", "Set Exact Custom Release Clause (Double-Click: e.g. 1k, 80M, 1B)"),
        "4 Bytes",
        "valCustomReleaseFee",
        signed=True,
        is_money=True,
        color=COL_GREEN,
    )
    builder.add_script(
        grp_contract,
        L("[ 1-CLIQUE ] Remover Override de Multa Exata deste Jogador", "[ 1-CLICK ] Clear Custom Release Clause Override for Player"),
        make_lua_button_script("local p=PES10.getSelectedPlayerOrWarn(); if p~=0 then PES10.clearOverrideReleaseFeeForPlayer(p) end"),
    )
    builder.add_record(grp_contract, L("Fator de Valorização na Base (Youth Slot)", "Youth Academy Valuation Factor"), "4 Bytes", "ptrPlayerReleaseSlot", ["08"], signed=True)
    builder.add_record(grp_contract, L("Salário Anual Ativo (Duplo-Clique: ex. 5M, 500k)", "Active Annual Salary (Double-Click: e.g. 5M, 500k)"), "4 Bytes", "ptrPlayer", ["15C"], signed=True, is_money=True, color=COL_GREEN)
    builder.add_record(grp_contract, L("Valor de Mercado Base (Duplo-Clique: ex. 120M)", "Base Market Value (Double-Click: e.g. 120M)"), "4 Bytes", "ptrPlayer", ["174"], signed=True, is_money=True, color=COL_GREEN)
    builder.add_record(grp_contract, L("Idade Base do Valor de Mercado (+0x178)", "Market Value Snapshot Age (+0x178)"), "Byte", "ptrPlayer", ["178"])
    builder.add_record(grp_contract, L("Overall (OVR) Base do Valor de Mercado (+0x179)", "Market Value Snapshot OVR (+0x179)"), "Byte", "ptrPlayer", ["179"])

    builder.add_script(
        grp_contract,
        L("[ ALVO € 1K ] Contratação Fácil (Multa € 1.000 + Afeição 100% + Destravar)", "[ € 1K TARGET ] Easy Signing (€ 1,000 Clause + 100% Affection + Unlock)"),
        make_lua_button_script("PES10.runPlayerMacro('easy_sign_target')"),
        color=COL_CRIMSON,
    )

    grp_c_bonus = builder.group(grp_contract, L("Bônus Ativos na Folha Salarial (Meu Elenco ML)", "Active Payroll Bonuses (User Squad ML)"), COL_TEAL)
    builder.add_record(grp_c_bonus, L("Salário Comprometido na Folha", "Committed Payroll Salary"), "4 Bytes", "ptrPlayerSquadBonus", ["08"], signed=True, is_money=True)
    builder.add_record(grp_c_bonus, L("Bônus Contratual #1 (Partida / Atuação)", "Appearance Bonus"), "4 Bytes", "ptrPlayerSquadBonus", ["10"], signed=True, is_money=True)
    builder.add_record(grp_c_bonus, L("Bônus Contratual #2 (Gol / Vitória)", "Goal / Win Bonus"), "4 Bytes", "ptrPlayerSquadBonus", ["18"], signed=True, is_money=True)
    builder.add_record(grp_c_bonus, L("Bônus Contratual #3 (Clean Sheet / Extra)", "Clean Sheet / Extra Bonus"), "4 Bytes", "ptrPlayerSquadBonus", ["28"], signed=True, is_money=True)

    grp_c_dates = builder.group(grp_contract, L("Datas de Término, Chegada & Última Renovação", "Contract Expiry, Arrival & Last Renewal Dates"), COL_TEAL)
    builder.add_record(grp_c_dates, L("Término do Contrato - Ano", "Contract Expiry - Year"), "2 Bytes", "ptrPlayer", ["138"])
    builder.add_record(grp_c_dates, L("Término do Contrato - Mês (1-12)", "Contract Expiry - Month (1-12)"), "Byte", "ptrPlayer", ["13A"])
    builder.add_record(grp_c_dates, L("Término do Contrato - Dia (1-31)", "Contract Expiry - Day (1-31)"), "Byte", "ptrPlayer", ["13B"])
    builder.add_record(grp_c_dates, L("Última Assinatura / Renovação - Ano (65535 = Sem Trava)", "Last Signing / Renewal - Year (65535 = Unlocked)"), "2 Bytes", "ptrPlayer", ["16C"])
    builder.add_record(grp_c_dates, L("Última Assinatura / Renovação - Mês", "Last Signing / Renewal - Month"), "Byte", "ptrPlayer", ["16E"])
    builder.add_record(grp_c_dates, L("Última Assinatura / Renovação - Dia", "Last Signing / Renewal - Day"), "Byte", "ptrPlayer", ["16F"])
    builder.add_record(grp_c_dates, L("Chegada ao Clube - Ano", "Club Arrival - Year"), "2 Bytes", "ptrPlayer", ["168"])
    builder.add_record(grp_c_dates, L("Chegada ao Clube - Mês", "Club Arrival - Month"), "Byte", "ptrPlayer", ["16A"])
    builder.add_record(grp_c_dates, L("Chegada ao Clube - Dia", "Club Arrival - Day"), "Byte", "ptrPlayer", ["16B"])
    builder.add_script(grp_c_dates, L("[ 1-CLIQUE ] Destravar Renegociação Imediata", "[ 1-CLICK ] Unlock Instant Renegotiation"), make_lua_button_script("PES10.runPlayerMacro('unlock_renegotiation')"), color=COL_BLUE)

    grp_c_loan = builder.group(grp_contract, L("Empréstimo, Lista de Transferências & Afeição (0-100%)", "Loan Status, Transfer List & Club Affection (0-100%)"), COL_TEAL)
    builder.add_record(grp_c_loan, L("Afeição pelo Clube / Treinador (0-100%)", "Club / Manager Affection (0-100%)"), "Binary", "ptrPlayer", ["13E"], bitstart="2", bitlen="7", color=COL_GREEN)
    builder.add_record(grp_c_loan, L("Está Emprestado (Is On Loan)", "Is Currently On Loan"), "Binary", "ptrPlayer", ["14A"], dropdown=dd_yes_no, bitstart="0", bitlen="1")
    builder.add_record(grp_c_loan, L("Clube Detentor do Passe (RAM)", "Parent Club Handle (Live RAM)"), "4 Bytes", "ptrPlayer", ["164"], is_club_handle=True)
    builder.add_record(grp_c_loan, L("Clube Anterior / Origem (RAM)", "Previous / Origin Club Handle (Live RAM)"), "4 Bytes", "ptrPlayer", ["160"], is_club_handle=True)
    builder.add_record(grp_c_loan, L("Na Lista de Transferências", "Is Transfer Listed"), "Binary", "ptrPlayer", ["14A"], dropdown=dd_yes_no, bitstart="1", bitlen="1")
    builder.add_record(grp_c_loan, L("Na Lista de Empréstimo", "Is Loan Listed"), "Binary", "ptrPlayer", ["14A"], dropdown=dd_yes_no, bitstart="2", bitlen="1")
    builder.add_record(grp_c_loan, L("Jogador Listado", "Is Listed Player"), "Binary", "ptrPlayer", ["13F"], dropdown=dd_yes_no, bitstart="1", bitlen="1")

    # 3.2 1-Click Macros & JSON Backup
    grp_p_macros = builder.group(sec_player, L("2. Macros de 1 Clique, Rejuvenescer & Backup JSON", "2. 1-Click Macros, Rejuvenate & JSON Backup"), COL_BLUE)
    builder.add_script(grp_p_macros, L("[ 1-CLIQUE ] Rejuvenescer (-2 Anos) & Sincronizar Valor de Mercado", "[ 1-CLICK ] Rejuvenate (-2 Yrs) & Sync Market Valuation"), make_lua_button_script("PES10.rejuvenateAndSyncMarket(-2)"), color=COL_GREEN)
    builder.add_script(grp_p_macros, L("[ 1-CLIQUE ] Todas as 13 Posições Jogáveis em Aptidão 'A'", "[ 1-CLICK ] Set All 13 Playable Positions to Grade 'A'"), make_lua_button_script("PES10.runPlayerMacro('all_positions_a')"), color=COL_PURPLE)
    builder.add_script(grp_p_macros, L("[ 1-CLIQUE ] Desbloquear Todas as 41 Habilidades (Skills)", "[ 1-CLICK ] Unlock All 41 Player Skills"), make_lua_button_script("PES10.runPlayerMacro('all_39_skills')"), color=COL_AMBER)
    builder.add_script(grp_p_macros, L("[ 1-CLIQUE ] Pé Ruim 4/4 + Forma 8 + Lesão 3 + Curar", "[ 1-CLICK ] Weak Foot 4/4 + Form 8 + Injury Res 3 + Heal"), make_lua_button_script("PES10.runPlayerMacro('perfect_wf_form_injury')"), color=COL_TEAL)
    builder.add_script(grp_p_macros, L("[ 1-CLIQUE ] Somar +5 em Todos os Atributos (Até 99)", "[ 1-CLICK ] Add +5 to All Attributes (Capped at 99)"), make_lua_button_script("PES10.runPlayerMacro('stats_plus_5')"), color=COL_BLUE)
    builder.add_script(grp_p_macros, L("[ 1-CLIQUE ] Todos os 25 Atributos em 99", "[ 1-CLICK ] Max All 25 Attributes to 99"), make_lua_button_script("PES10.runPlayerMacro('stats_99')"), color=COL_CRIMSON)
    builder.add_script(grp_p_macros, L("[ BACKUP JSON ] Exportar Jogador Selecionado (.json)", "[ JSON BACKUP ] Export Selected Player (.json)"), make_lua_button_script("PES10.exportPlayerBackupJson()"))
    builder.add_script(grp_p_macros, L("[ RESTORE JSON ] Importar Arquivo .json no Jogador Atual", "[ JSON RESTORE ] Import .json into Selected Player"), make_lua_button_script("PES10.importPlayerBackupJson()"))

    # 3.3 Identity & Clubs
    grp_p_info = builder.group(sec_player, L("3. Identidade, Nomes UTF-8 (60B) & Clubes ao Vivo", "3. Identity, UTF-8 Names (60B) & Live Club Handles"), COL_TEAL)
    builder.add_record(grp_p_info, L("ID do Jogador (Player ID)", "Player ID"), "4 Bytes", "ptrPlayer", ["30"])
    builder.add_record(grp_p_info, L("Nome do Jogador (UTF-8 - Até 60 Bytes)", "Player Name (UTF-8 - Up to 60 Bytes)"), "String", "ptrPlayer", ["38"], str_len=60, utf8=True)
    builder.add_record(grp_p_info, L("Nome na Camisa - Clube (UTF-8)", "Shirt Name - Club (UTF-8)"), "String", "ptrPlayer", ["75"], str_len=60, utf8=True)
    builder.add_record(grp_p_info, L("Nome na Camisa - Seleção (UTF-8)", "Shirt Name - National Team (UTF-8)"), "String", "ptrPlayer", ["B2"], str_len=60, utf8=True)
    builder.add_record(grp_p_info, L("Clube Atual (Decodificado ao Vivo na RAM)", "Current Club (Live RAM Decoded)"), "4 Bytes", "ptrPlayer", ["12C"], is_club_handle=True, color=COL_GREEN)
    builder.add_record(grp_p_info, L("Seleção Nacional (Decodificada ao Vivo na RAM)", "National Team (Live RAM Decoded)"), "4 Bytes", "ptrPlayer", ["130"], is_club_handle=True)
    builder.add_record(grp_p_info, L("Nacionalidade / País (221 Países)", "Nationality / Country (221 Countries)"), "2 Bytes", "ptrPlayer", ["144"], dropdown=DROPDOWNS["Nationality"])
    builder.add_record(grp_p_info, L("Idade (15-50)", "Age (15-50)"), "Binary", "ptrPlayer", ["1C"], bitstart="0", bitlen="6")
    builder.add_record(grp_p_info, L("Altura (cm)", "Height (cm)"), "Binary", "ptrPlayer", ["0"], bitstart="0", bitlen="8")
    builder.add_record(grp_p_info, L("Peso (kg)", "Weight (kg)"), "Binary", "ptrPlayer", ["1"], bitstart="0", bitlen="8")
    builder.add_record(grp_p_info, L("Pé Dominante", "Stronger Foot"), "Binary", "ptrPlayer", ["3"], dropdown=dd_foot, bitstart="7", bitlen="1")
    builder.add_record(grp_p_info, L("Comemoração de Gol Principal (+0x02)", "Goal Celebration #1 (+0x02)"), "Byte", "ptrPlayer", ["2"])
    builder.add_record(grp_p_info, L("ID de Narração (Commentary ID)", "Commentary Callname ID"), "4 Bytes", "ptrPlayer", ["34"])

    # 3.4 Fitness, Form Arrow, Weak Foot & Injuries
    grp_p_cond = builder.group(sec_player, L("4. Condição Física, Seta de Forma, Pé Ruim & Lesões", "4. Fitness, Form Arrow, Weak Foot & Injuries"), COL_TEAL)
    builder.add_record(grp_p_cond, L("Barra de Fôlego / Stamina (0-100)", "Fatigue / Stamina Gauge (0-100)"), "Binary", "ptrPlayer", ["146"], bitstart="0", bitlen="7")
    builder.add_record(grp_p_cond, L("Seta de Forma para a Próxima Partida", "Next Match Form Arrow"), "Binary", "ptrPlayer", ["147"], dropdown=dd_form_arrow, bitstart="0", bitlen="3")
    builder.add_record(grp_p_cond, L("Seta Piscando (Em Destaque)", "Flashing Form Arrow"), "Binary", "ptrPlayer", ["146"], dropdown=dd_yes_no, bitstart="7", bitlen="1")
    builder.add_record(grp_p_cond, L("Dias Indisponível por Lesão (0 = Saudável)", "Injury Duration Remaining (0 = Healthy)"), "Byte", "ptrPlayer", ["148"])
    builder.add_record(grp_p_cond, L("Uso do Pé Ruim (1-4)", "Weak Foot Usage (1-4)"), "Binary", "ptrPlayer", ["23"], dropdown=DROPDOWNS["Weak Foot Usage"], bitstart="0", bitlen="2")
    builder.add_record(grp_p_cond, L("Precisão do Pé Ruim (1-4)", "Weak Foot Accuracy (1-4)"), "Binary", "ptrPlayer", ["22"], dropdown=DROPDOWNS["Weak Foot Accuracy"], bitstart="4", bitlen="2")
    builder.add_record(grp_p_cond, L("Forma Física / Regularidade (1-8)", "Conditioning / Form (1-8)"), "Binary", "ptrPlayer", ["17"], dropdown=DROPDOWNS["Form"], bitstart="4", bitlen="3")
    builder.add_record(grp_p_cond, L("Resistência a Lesões (1-3)", "Injury Resistance (1-3)"), "Binary", "ptrPlayer", ["22"], dropdown=DROPDOWNS["Injury Resistance"], bitstart="6", bitlen="2")

    # 3.5..3.8 Positions, 25 Attributes, Playing Style + 41 Skills, COM Styles & Team Role
    grp_p_pos = builder.group(sec_player, L("5. Posição de Origem & 13 Posições Jogáveis (C / B / A)", "5. Registered Position & 13 Playable Positions (C / B / A)"), COL_TEAL)
    builder.add_script(grp_p_pos, L("[ 1-CLIQUE ] Promover Todas as Secundárias ('B' → 'A')", "[ 1-CLICK ] Promote All Secondary Positions ('B' → 'A')"), make_lua_button_script("local p=PES10.getSelectedPlayerOrWarn(); if p~=0 then local n=PES10.promoteSecondaryPositionsToA(p); PES10.toast(string.format(PES10.T('%d secundária(s) promovida(s) p/ A!','%d secondary position(s) promoted to A!'), n)) end"), color=COL_GREEN)
    builder.add_script(grp_p_pos, L("[ +1 OVR POSIÇÃO ] Subir +1 OVR em Posição Específica...", "[ +1 POS OVR ] Boost +1 OVR in Specific Position..."), make_lua_button_script("PES10.promptTargetPositionOVRPlusOne()"), color=COL_BLUE)
    grp_p_attr = builder.group(sec_player, L("6. Todos os 25 Atributos Técnicos, Físicos, Defensivos & GK (40-99)", "6. All 25 Technical, Physical, Defensive & GK Attributes (40-99)"), COL_TEAL)
    grp_p_skills = builder.group(sec_player, L("7. Estilo de Jogo & Todas as 41 Habilidades (Player Skills)", "7. Playing Style & All 41 Player Skills"), COL_TEAL)
    grp_p_com = builder.group(sec_player, L("8. Estilos IA COM & Papel no Time (Team Role)", "8. COM AI Playing Styles & Team Role"), COL_TEAL)
    builder.add_script(grp_p_com, L("[ PRESET ] Tornar LENDA (Nível 3)", "[ PRESET ] Make LEGEND (Level 3)"), make_lua_button_script("local p=PES10.getSelectedPlayerOrWarn(); if p~=0 then PES10.setTeamRole(p,19,3); PES10.toast(PES10.T('Team Role definido: Lenda (Nv.3)!','Team Role set: Legend (Lv.3)!')) end"), color=COL_AMBER)
    builder.add_script(grp_p_com, L("[ PRESET ] Tornar BANDEIRA (Nível 3)", "[ PRESET ] Make BANDIERA (Level 3)"), make_lua_button_script("local p=PES10.getSelectedPlayerOrWarn(); if p~=0 then PES10.setTeamRole(p,18,3); PES10.toast(PES10.T('Team Role definido: Bandeira (Nv.3)!','Team Role set: Bandiera (Lv.3)!')) end"), color=COL_GREEN)
    builder.add_script(grp_p_com, L("[ PRESET ] Tornar MAESTRO (Nível 3)", "[ PRESET ] Make MAESTRO (Level 3)"), make_lua_button_script("local p=PES10.getSelectedPlayerOrWarn(); if p~=0 then PES10.setTeamRole(p,9,3); PES10.toast(PES10.T('Team Role definido: Maestro (Nv.3)!','Team Role set: Maestro (Lv.3)!')) end"), color=COL_BLUE)
    builder.add_script(grp_p_com, L("[ PRESET ] Tornar GENERAL (Nível 3)", "[ PRESET ] Make GENERAL (Level 3)"), make_lua_button_script("local p=PES10.getSelectedPlayerOrWarn(); if p~=0 then PES10.setTeamRole(p,7,3); PES10.toast(PES10.T('Team Role definido: General (Nv.3)!','Team Role set: General (Lv.3)!')) end"), color=COL_PURPLE)

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
        lbl = EN_PLAYER_LABELS.get(d, d) if is_en else PT_PLAYER_LABELS.get(d, d)

        if d == "Registred Position":
            builder.add_record(grp_p_pos, lbl, vt, "ptrPlayer", off, dropdown=dd_reg_pos, bitstart=bs, bitlen=bl)
        elif d in POS_DESCS:
            builder.add_record(grp_p_pos, lbl, vt, "ptrPlayer", off, dropdown=dd_pos_grade, bitstart=bs, bitlen=bl)
        elif 32 <= idx <= 56:
            builder.add_record(grp_p_attr, lbl, vt, "ptrPlayer", off, bitstart=bs, bitlen=bl)
            all_stat_bits.append((int(off[0], 16), int(bs)))
        elif d == "Playing Style":
            builder.add_record(grp_p_skills, lbl, vt, "ptrPlayer", off, dropdown=dd_playing_style, bitstart=bs, bitlen=bl, color=COL_PURPLE)
        elif 57 <= idx <= 97:
            if d == "Heading" and bl == "1":
                sk_label = L("Cabeceio Preciso - Skill (Heading)", "Heading (Player Skill)")
            else:
                sk_label = lbl
            builder.add_record(grp_p_skills, sk_label, vt, "ptrPlayer", off, dropdown=dd_yes_no, bitstart=bs, bitlen=bl)
            all_skill_bits.append((int(off[0], 16), int(bs)))
        elif d in COM_DESCS:
            builder.add_record(grp_p_com, lbl, vt, "ptrPlayer", off, dropdown=dd_yes_no, bitstart=bs, bitlen=bl)
        elif d in ROLE_DESCS:
            dd = DROPDOWNS.get("Team Role") if d == "Team Role" else None
            builder.add_record(grp_p_com, lbl, vt, "ptrPlayer", off, dropdown=dd, bitstart=bs, bitlen=bl)

    grp_p_dbg = builder.group(sec_player, L("9. Diagnóstico Técnico de Ponteiros (Avançado)", "9. Technical Pointer Diagnostics (Advanced)"), None)
    builder.add_record(grp_p_dbg, "ptrPlayer", "8 Bytes", "ptrPlayer", [])
    builder.add_record(grp_p_dbg, "ptrCareer", "8 Bytes", "ptrCareer", [])
    builder.add_record(grp_p_dbg, "ptrPlayerTraining", "8 Bytes", "ptrPlayerTraining", [])
    builder.add_record(grp_p_dbg, "ptrPlayerSquadBonus", "8 Bytes", "ptrPlayerSquadBonus", [])

    # =========================================================================
    # SECTION 4: [ 4. TRAINING (SUB-XP) & BECOME A LEGEND (BAL) ]
    # =========================================================================
    sec_trn_bal = builder.group(
        top_entries,
        L("[ 4. TREINO & ESTRELATO ] Multiplicador Sub-XP, Base & Suite BAL", "[ 4. TRAINING & BAL ] Sub-XP Multiplier, Youth & Become a Legend Suite"),
        COL_PURPLE,
    )

    builder.add_record(
        sec_trn_bal,
        L("Velocidade de Evolução Sub-XP (Clube, Base & BAL)", "Sub-XP Training Multiplier (Your Club, Youth & BAL)"),
        "4 Bytes",
        "cfgTrainingMultMode",
        dropdown=L(
            "0:1x - Evolução Original\n1:2x - Evolução Dobrada\n2:5x - Evolução Rápida\n"
            "3:10x - Super Treino\n4:50x - Turbo Extremo\n5:Instantâneo (10.000 XP - Nível / Posição / Skill em 1 Dia!)",
            "0:1x - Default Progression\n1:2x - Double Speed\n2:5x - Fast Progression\n"
            "3:10x - Super Training\n4:50x - Extreme Turbo\n5:Instant (10,000 XP - Stat / Position / Skill in 1 Day!)",
        ),
        dropdown_only=True,
        color=COL_PURPLE,
    )

    builder.add_script(
        sec_trn_bal,
        L("[ 1-CLIQUE ] Encher 25 Barras de Sub-XP do Atleta Selecionado (9.999 / 10.000)", "[ 1-CLICK ] Fill All 25 Sub-XP Bars of Selected Player (9,999 / 10,000)"),
        make_lua_button_script("PES10.runPlayerMacro('instant_subxp_player')"),
        color=COL_BLUE,
    )

    grp_bal = builder.group(
        sec_trn_bal,
        L("Suite Rumo ao Estrelato (Become a Legend - BAL)", "Become a Legend (BAL) Complete Suite"),
        COL_PURPLE,
        hide_children=False,
    )
    builder.add_script(grp_bal, L("[ BAL 1-CLIQUE ] Carregar Meu Jogador do Estrelato no Editor", "[ BAL 1-CLICK ] Select My Legend Pro into Studio & Editor"), make_lua_button_script("PES10.pointToBALMyPlayer()"), color=COL_BLUE)
    builder.add_record(grp_bal, L("Focus Points Restantes no BAL", "Remaining BAL Focus Points"), "Byte", "ptrCareer", ["016ECBEA"], color=COL_PURPLE)
    builder.add_record(grp_bal, L("Travar Focus Points Sempre em 99", "Lock BAL Focus Points at 99"), "4 Bytes", "cfgLockBALFocusPoints", dropdown=L("0:Desativado\n1:Ativado (99 Pontos Infinitos)", "0:Disabled\n1:Locked at 99 Points"), dropdown_only=True)
    builder.add_script(grp_bal, L("[ BAL 1-CLIQUE ] Colocar 10 Barras no Máximo + 99 Pontos", "[ BAL 1-CLICK ] Max All 10 Training Sliders + 99 Focus Points"), make_lua_button_script("PES10.runPlayerMacro('bal_max_sliders')"), color=COL_GREEN)
    builder.add_record(grp_bal, L("Habilidade (Skill) em Treinamento no BAL", "Active BAL Skill Training Slot"), "4 Bytes", "ptrCareer", ["016ECBEC"], dropdown=dd_bal_skill_ids)
    builder.add_script(grp_bal, L("[ BAL 1-CLIQUE ] Aprender Habilidade em Treino Agora (99,99%)", "[ BAL 1-CLICK ] Instant Learn Active Skill Now (99.99%)"), make_lua_button_script("PES10.runPlayerMacro('bal_instant_skill')"), color=COL_GREEN)
    builder.add_record(grp_bal, L("Posição em Treinamento no BAL", "Active BAL Position Training Slot"), "4 Bytes", "ptrCareer", ["016ECBF0"], dropdown=dd_bal_pos_ids)
    builder.add_script(grp_bal, L("[ BAL 1-CLIQUE ] Aprender Posição em Treino Agora (99,99%)", "[ BAL 1-CLICK ] Instant Learn Active Position Now (99.99%)"), make_lua_button_script("PES10.runPlayerMacro('bal_instant_pos')"), color=COL_GREEN)

    grp_bal_bars = builder.group(sec_trn_bal, L("As 10 Barras Individuais de Treino do BAL", "All 10 Individual BAL Training Sliders"), COL_PURPLE)
    BAL_BARS = [
        ("016ECBE0", L("Barra 1 - Chute (Shooting)", "Slider 1 - Shooting")),
        ("016ECBE1", L("Barra 2 - Passe (Passing)", "Slider 2 - Passing")),
        ("016ECBE2", L("Barra 3 - Drible (Dribbling)", "Slider 3 - Dribbling")),
        ("016ECBE3", L("Barra 4 - Destreza / Ataque (Dexterity)", "Slider 4 - Dexterity")),
        ("016ECBE4", L("Barra 5 - Velocidade / Explosão (Lower Body)", "Slider 5 - Lower Body Strength")),
        ("016ECBE5", L("Barra 6 - Jogo Aéreo / Físico (Aerial)", "Slider 6 - Aerial Strength")),
        ("016ECBE6", L("Barra 7 - Defesa (Defending)", "Slider 7 - Defending")),
        ("016ECBE7", L("Barra 8 - Goleiro (GK)", "Slider 8 - Goalkeeping")),
        ("016ECBE8", L("Barra 9 - Foco em Habilidade (Skill)", "Slider 9 - Skill Training")),
        ("016ECBE9", L("Barra 10 - Foco em Posição (Position)", "Slider 10 - Position Training")),
    ]
    for off_h, bdesc in BAL_BARS:
        builder.add_record(grp_bal_bars, bdesc, "Byte", "ptrCareer", [off_h])

    grp_bal_status = builder.group(sec_trn_bal, L("Status no Elenco & Confiança do Técnico (BAL)", "Squad Status & Manager Trust (BAL)"), COL_PURPLE)
    builder.add_record(grp_bal_status, L("Status no Clube Principal (0=Reserva .. 3=Titular)", "Club Squad Role (0=Reserve .. 3=Key Starter)"), "Byte", "ptrCareer", ["01818429"])
    builder.add_record(grp_bal_status, L("Flags de Moral / Titularidade no Clube", "Club Morale & Starter Flags"), "Byte", "ptrCareer", ["0181842C"])
    builder.add_record(grp_bal_status, L("Status Secundário / Seleção", "National Team / Secondary Status"), "Byte", "ptrCareer", ["0181952D"])

    grp_growth = builder.group(sec_trn_bal, L("Auge (Breakthrough), Pico de Idade & Anti-Declínio 30+", "Breakthrough Surge, Peak Age & Anti-Decline 30+"), COL_PURPLE)
    builder.add_record(grp_growth, L("Curva de Desenvolvimento / Declínio Pós-30", "Growth / Post-30 Decline Curve"), "Binary", "ptrPlayer", ["1B"], dropdown=dd_growth_curve, bitstart="4", bitlen="3")
    builder.add_record(grp_growth, L("Estado de Auge / Breakthrough (2 = Em Explosão!)", "Breakthrough State (2 = Active Surge!)"), "Binary", "ptrPlayerGrowthDesc", ["3"], bitstart="0", bitlen="4")
    builder.add_record(grp_growth, L("Idade de Pico Calculada", "Computed Peak Age"), "Binary", "ptrPlayerGrowthDesc", ["2"], bitstart="1", bitlen="7")

    grp_subxp_stats = builder.group(sec_trn_bal, L("Inspector das 25 Barras de Sub-XP (0..10.000)", "25 Sub-XP Bars Inspector (0..10,000)"), COL_PURPLE)
    SUBXP_FIELDS = [
        (14, "Talento Ofensivo", "Offensive Awareness"), (15, "Talento Defensivo", "Defensive Awareness"),
        (16, "Talento de Goleiro", "GK Awareness"), (17, "Drible", "Dribbling"),
        (18, "Bola Parada", "Place Kicking"), (19, "Finalização", "Finishing"),
        (20, "Passe Rasteiro", "Low Pass"), (21, "Passe Alto", "Lofted Pass"),
        (22, "Cabeceio", "Heading"), (23, "Curva", "Curl"),
        (24, "Resistência", "Stamina"), (25, "Velocidade", "Speed"),
        (26, "Força do Chute", "Kicking Power"), (27, "Impulsão", "Jump"),
        (28, "Alcance GK", "GK Reach"), (29, "Afastar GK", "GK Clearing"),
        (30, "Reflexos GK", "GK Reflexes"), (31, "Firmeza GK", "GK Catching"),
        (33, "Controle de Bola", "Ball Control"), (34, "Contato Físico", "Physical Contact"),
        (35, "Equilíbrio", "Balance"), (36, "Aceleração", "Acceleration"),
        (37, "Desarme", "Ball Winning"), (38, "Condução Firme", "Tight Possession"),
        (39, "Agressividade", "Aggression"),
    ]
    for fid, pt_lbl, en_lbl in SUBXP_FIELDS:
        off_hex = f"{0x16 + fid * 2:X}"
        builder.add_record(grp_subxp_stats, f"Sub-XP: {L(pt_lbl, en_lbl)} (0..10000)", "2 Bytes", "ptrPlayerTraining", [off_hex], signed=True)

    # =========================================================================
    # SECTION 5: [ 5. LIVE MATCH - CLOCK, SCORE & STAMINA ]
    # =========================================================================
    sec_match = builder.group(
        top_entries,
        L("[ 5. PARTIDA AO VIVO ] Relógio Exato, Placar, Pênaltis & Stamina", "[ 5. LIVE MATCH ] Exact Match Clock, Score, Penalties & Stamina"),
        COL_CRIMSON,
    )

    builder.clock_id = builder.add_record(
        sec_match,
        L("[ RELÓGIO ] Tempo da Partida (Duplo-Clique p/ Definir Minuto Exato: 15, 44, 88)", "[ CLOCK ] Live Match Time (Double-Click to Set Exact Minute: 15, 44, 88)"),
        "Float",
        "ptrMatchState",
        ["1174"],
        color=COL_CRIMSON,
    )
    builder.add_record(
        sec_match,
        L("Período da Partida", "Match Period / Half"),
        "4 Bytes",
        "ptrMatchState",
        ["1168"],
        dropdown=L(
            "1:1º Tempo\n3:2º Tempo\n5:1ª Prorrogação\n7:2ª Prorrogação",
            "1:1st Half\n3:2nd Half\n5:1st Extra Time\n7:2nd Extra Time",
        ),
    )
    builder.add_record(
        sec_match,
        L("Congelar Relógio da Partida", "Freeze Live Match Clock"),
        "4 Bytes",
        "cfgFreezeClock",
        dropdown=L(
            "0:Desativado (Relógio Correndo Normal)\n1:Congelado no Segundo Atual",
            "0:Disabled (Normal Clock)\n1:Frozen at Current Second",
        ),
        dropdown_only=True,
    )

    builder.add_script(sec_match, L("[ MINUTO 44' ] Pular para 44:00 (Fim do 1º Tempo)", "[ MINUTE 44' ] Jump to 44:00 (End of 1st Half)"), make_lua_button_script("PES10.setExactMatchMinute(44)"), color=COL_BLUE)
    builder.add_script(sec_match, L("[ MINUTO 88' ] Pular para 88:00 (Minutos Finais)", "[ MINUTE 88' ] Jump to 88:00 (Final Minutes)"), make_lua_button_script("PES10.setExactMatchMinute(88)"), color=COL_BLUE)
    builder.add_script(sec_match, L("[ +10 MIN ] Avançar +10 Minutos no Relógio", "[ +10 MIN ] Advance Clock by +10 Minutes"), make_lua_button_script("PES10.shiftMatchMinutes(10)"))
    builder.add_script(sec_match, L("[ -10 MIN ] Voltar -10 Minutos no Relógio", "[ -10 MIN ] Rewind Clock by -10 Minutes"), make_lua_button_script("PES10.shiftMatchMinutes(-10)"))
    builder.add_script(sec_match, L("[ 00:00 ] Reiniciar Tempo Atual", "[ 00:00 ] Restart Current Half"), make_lua_button_script("PES10.resetCurrentHalfNow()"))
    builder.add_script(sec_match, L("[ APITO ] Encerrar Tempo Atual Agora", "[ WHISTLE ] End Current Half Immediately"), make_lua_button_script("PES10.endCurrentHalfNow()"), color=COL_CRIMSON)
    builder.add_record(sec_match, L("Minutos de Acréscimo no Placar (-1 = Sem Acréscimo)", "Added Injury Time Display (-1 = None)"), "4 Bytes", "ptrMatchHud", ["144"], signed=True)

    grp_m_score = builder.group(sec_match, L("Placar ao Vivo & Disputa de Pênaltis", "Live Match Score & Penalty Shootout"), COL_CRIMSON, hide_children=False)
    home_score_id = builder.add_record(grp_m_score, L("Gols - Time Mandante (Casa) [Sincroniza Motor + Placar]", "Goals - Home Team [Syncs Engine + Scoreboard]"), "Byte", "ptrMatchCore", ["9E"], color=COL_GREEN)
    away_score_id = builder.add_record(grp_m_score, L("Gols - Time Visitante (Fora) [Sincroniza Motor + Placar]", "Goals - Away Team [Syncs Engine + Scoreboard]"), "Byte", "ptrMatchCore", ["9F"], color=COL_GREEN)
    builder.score_sync[home_score_id] = "home"
    builder.score_sync[away_score_id] = "away"
    builder.add_record(grp_m_score, L("Pênaltis Convertidos - Mandante (Casa)", "Penalty Shootout Goals - Home"), "Byte", "ptrMatchCore", ["12C"])
    builder.add_record(grp_m_score, L("Pênaltis Convertidos - Visitante (Fora)", "Penalty Shootout Goals - Away"), "Byte", "ptrMatchCore", ["12D"])

    grp_m_stam = builder.group(sec_match, L("Stamina Seletiva em Campo", "Selective On-Pitch Stamina"), COL_CRIMSON, hide_children=False)
    builder.add_record(
        grp_m_stam,
        L("Modo de Stamina durante a Partida", "Live Match Stamina Mode"),
        "4 Bytes",
        "cfgStaminaMode",
        dropdown=L(
            "0:Desativado (Desgaste Original)\n"
            "1:Stamina 100% - Apenas Time Mandante (Casa)\n"
            "2:Stamina 100% - Apenas Time Visitante (Fora)\n"
            "3:Stamina 100% - Ambos os Times\n"
            "4:Pressão Total Casa (Casa 100% + Drenar Visitante p/ 5%)\n"
            "5:Pressão Total Fora (Fora 100% + Drenar Casa p/ 5%)",
            "0:Disabled (Default Fatigue)\n"
            "1:100% Stamina - Home Team Only\n"
            "2:100% Stamina - Away Team Only\n"
            "3:100% Stamina - Both Teams\n"
            "4:Home Total Press (Home 100% + Drain Away to 5%)\n"
            "5:Away Total Press (Away 100% + Drain Home to 5%)",
        ),
        dropdown_only=True,
        color=COL_GREEN,
    )

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
PES10.COMPUTED_OVR_RECORD_ID = {builder.computed_ovr_id}
PES10.SCORE_SYNC_IDS = {score_map_lua}
"""

    final_lua = (
        lua_template
        .replace("__DEFAULT_LANG__", lang)
        .replace("__BMPES_TEAMS_LUA__", TEAMS_LUA_STR)
        .replace("__BMPES_COMPS_LUA__", COMPS_LUA_STR)
        .replace("__KONAMI_OVR_LUA__", KONAMI_OVR_LUA_STR)
        .replace("-- __PES10_CONFIG_INJECTION__", lua_config_injection)
    )

    ET.SubElement(root, "UserdefinedSymbols")
    lua_el = ET.SubElement(root, "LuaScript")
    lua_el.text = final_lua

    ET.indent(root, space="  ")
    xml_bytes = ET.tostring(root, encoding="utf-8", xml_declaration=True)

    output_ct.parent.mkdir(parents=True, exist_ok=True)
    with open(output_ct, "wb") as f_out:
        f_out.write(xml_bytes)

    return {
        "lang": lang,
        "path": output_ct,
        "entries": builder.next_id - 1,
        "money": len(builder.money_ids),
        "clubs": len(builder.club_handle_ids),
        "skills": len(all_skill_bits),
    }


def main() -> None:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--deploy-dir", default=None)
    args, _ = parser.parse_known_args()

    res_pt = build_edition("pt", OUTPUT_PT)
    res_en = build_edition("en", OUTPUT_EN)

    for r in (res_pt, res_en):
        print(
            f"SUCCESS [{r['lang'].upper()}]: {r['path'].name} "
            f"({r['entries']} entries | {r['money']} money | {r['clubs']} clubs | {r['skills']} skills)"
        )

    deploy_dir = args.deploy_dir
    env_local = REPO_ROOT / ".env.local"
    if not deploy_dir and env_local.exists():
        for line in env_local.read_text(encoding="utf-8").splitlines():
            if line.startswith("DEPLOY_CT_DIR="):
                deploy_dir = line.split("=", 1)[1].strip()
            elif line.startswith("DEPLOY_CT_PATH="):
                p = pathlib.Path(line.split("=", 1)[1].strip())
                deploy_dir = str(p.parent)

    if deploy_dir:
        dd = pathlib.Path(deploy_dir)
        if dd.exists() and dd.is_dir():
            for out_f in (OUTPUT_PT, OUTPUT_EN):
                shutil.copy2(out_f, dd / out_f.name)
                print(f"DEPLOYED: {dd / out_f.name}")
            old_v10 = dd / "PES 2021 - v10.0 Pro Overhaul.CT"
            if old_v10.exists():
                old_v10.unlink()
                print(f"CLEANED OLD EDITION: {old_v10}")


if __name__ == "__main__":
    main()
