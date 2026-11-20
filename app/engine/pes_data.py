"""
Static dictionaries, Konami OVR tables, bitfield schemas, positions, skills,
playing styles, nationalities, and team roles for PES 2021 Ultimate Studio.
Loads directly from src/data/*.json (works in both dev mode and PyInstaller _MEIPASS).
"""

import json
import sys
from pathlib import Path
from typing import Dict, List, Any


def get_data_dir() -> Path:
    if getattr(sys, "frozen", False) and hasattr(sys, "_MEIPASS"):
        return Path(sys._MEIPASS) / "data"
    # Dev mode: pes21-workbench/app/engine/pes_data.py -> pes21-workbench/src/data
    return Path(__file__).resolve().parent.parent.parent / "src" / "data"


def _load_json(filename: str) -> Any:
    path = get_data_dir() / filename
    return json.loads(path.read_text(encoding="utf-8"))


OVR_DATA = _load_json("konami_ovr_tables.json")
BMPES_DICTS = _load_json("bmpes_dicts.json")
PLAYER_SCHEMA = _load_json("player_schema.json")

BMPES_TEAMS: Dict[int, str] = {
    int(k): v for k, v in BMPES_DICTS.get("teams", {}).items()
}
BMPES_COMPS: Dict[int, str] = {
    int(k): v for k, v in BMPES_DICTS.get("comps", {}).items()
}


def _parse_dropdown(raw_str: str) -> List[Dict[str, Any]]:
    out = []
    for line in (raw_str or "").splitlines():
        line = line.strip()
        if ":" not in line:
            continue
        k, v = line.split(":", 1)
        try:
            clean = v.strip().strip("'\"")
            out.append({"id": int(k.strip()), "name": clean})
        except ValueError:
            pass
    return out


NATIONALITIES: List[Dict[str, Any]] = _parse_dropdown(
    PLAYER_SCHEMA["dropdowns"].get("Nationality", "")
)
PLAYING_STYLES: List[Dict[str, Any]] = _parse_dropdown(
    PLAYER_SCHEMA["dropdowns"].get("Playing Style", "")
)

# Position metadata (0..12)
POSITIONS = [
    {"id": 0, "pt": "GL", "en": "GK", "name_pt": "Goleiro", "name_en": "Goalkeeper"},
    {"id": 1, "pt": "ZAG", "en": "CB", "name_pt": "Zagueiro", "name_en": "Center Back"},
    {"id": 2, "pt": "LE", "en": "LB", "name_pt": "Lateral Esquerdo", "name_en": "Left Back"},
    {"id": 3, "pt": "LD", "en": "RB", "name_pt": "Lateral Direito", "name_en": "Right Back"},
    {"id": 4, "pt": "VOL", "en": "DMF", "name_pt": "Volante", "name_en": "Defensive Midfielder"},
    {"id": 5, "pt": "MLG", "en": "CMF", "name_pt": "Meia Central", "name_en": "Central Midfielder"},
    {"id": 6, "pt": "MLE", "en": "LMF", "name_pt": "Meia Esquerda", "name_en": "Left Midfielder"},
    {"id": 7, "pt": "MLD", "en": "RMF", "name_pt": "Meia Direita", "name_en": "Right Midfielder"},
    {"id": 8, "pt": "MAT", "en": "AMF", "name_pt": "Meia Atacante", "name_en": "Attacking Midfielder"},
    {"id": 9, "pt": "PTE", "en": "LWF", "name_pt": "Ponta Esquerda", "name_en": "Left Wing Forward"},
    {"id": 10, "pt": "PTD", "en": "RWF", "name_pt": "Ponta Direita", "name_en": "Right Wing Forward"},
    {"id": 11, "pt": "SA", "en": "SS", "name_pt": "Segundo Atacante", "name_en": "Second Striker"},
    {"id": 12, "pt": "CA", "en": "CF", "name_pt": "Centroavante", "name_en": "Center Forward"},
]

TEAM_ROLES = [
    {"id": 0, "pt": "Nenhum (-)", "en": "None (-)"},
    {"id": 1, "pt": "Jovem Promessa", "en": "Youth Prospect"},
    {"id": 2, "pt": "Pupilo", "en": "Protégé"},
    {"id": 3, "pt": "Jogador de Equipe", "en": "Team Player"},
    {"id": 4, "pt": "Armador", "en": "Playmaker"},
    {"id": 5, "pt": "Estrela", "en": "Star Player"},
    {"id": 6, "pt": "Líder", "en": "Leader"},
    {"id": 7, "pt": "General", "en": "General"},
    {"id": 8, "pt": "Criador", "en": "Creator"},
    {"id": 9, "pt": "Maestro", "en": "Maestro"},
    {"id": 10, "pt": "Incansável", "en": "Workhorse"},
    {"id": 11, "pt": "Jogador Inteligente", "en": "Smart Player"},
    {"id": 12, "pt": "Guerreiro", "en": "Fighter"},
    {"id": 13, "pt": "Jogador Chave", "en": "Key Player"},
    {"id": 14, "pt": "Superestrela", "en": "Superstar"},
    {"id": 15, "pt": "Herói", "en": "Hero"},
    {"id": 16, "pt": "Virtuoso", "en": "Virtuoso"},
    {"id": 17, "pt": "Regente", "en": "Conductor"},
    {"id": 18, "pt": "Bandeira", "en": "Bandiera"},
    {"id": 19, "pt": "Lenda", "en": "Legend"},
    {"id": 20, "pt": "Estrela em Ascensão", "en": "Rising Star"},
    {"id": 21, "pt": "Garoto Rebelde", "en": "Bad Boy"},
    {"id": 22, "pt": "Ousado", "en": "Risk-Taker"},
]

SCOUT_EMPTY_SENTINEL = 0x0000FFFF
SCOUT_LISTS = [
    {"slot": 1, "base": 0x01676054, "cap": 30, "pt": "Olheiro 1", "en": "Scout 1"},
    {"slot": 2, "base": 0x01676144, "cap": 30, "pt": "Olheiro 2", "en": "Scout 2"},
    {"slot": 3, "base": 0x01676234, "cap": 30, "pt": "Olheiro 3", "en": "Scout 3"},
    {"slot": 4, "base": 0x01675D34, "cap": 100, "pt": "Favoritos (Shortlist)", "en": "Shortlist"},
]

# Konami OVR Engine field locations: fid -> (dword_offset, bit_start)
FID_LOC = {
    14: (0x03, 0),
    15: (0x04, 0),
    16: (0x04, 7),
    17: (0x04, 14),
    18: (0x04, 21),
    19: (0x08, 0),
    20: (0x08, 7),
    21: (0x08, 14),
    22: (0x08, 21),
    23: (0x0C, 0),
    24: (0x0C, 7),
    25: (0x0C, 14),
    26: (0x0C, 21),
    27: (0x10, 0),
    28: (0x10, 7),
    29: (0x10, 14),
    30: (0x10, 21),
    31: (0x14, 0),
    33: (0x14, 7),
    34: (0x14, 14),
    35: (0x14, 21),
    36: (0x18, 0),
    37: (0x18, 7),
    38: (0x18, 14),
    39: (0x18, 21),
}

OUTFIELD_FIDS = [14, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 33, 34, 35, 36, 37, 38, 39]
GK_FIDS = [16, 28, 29, 30, 31]
POS_REMAP = {0: 12, 1: 9, 2: 10, 3: 11, 4: 5, 5: 6, 6: 7, 7: 8, 8: 4, 9: 2, 10: 3, 11: 1, 12: 0}

# Position aptitude bit locations: pos_idx -> (byte_offset, bit_start)
POS_APT_LOC = {
    0: (0x22, 2),
    1: (0x21, 4),
    2: (0x21, 6),
    3: (0x22, 0),
    4: (0x20, 4),
    5: (0x20, 6),
    6: (0x21, 0),
    7: (0x21, 2),
    8: (0x20, 2),
    9: (0x1F, 5),
    10: (0x20, 0),
    11: (0x1F, 3),
    12: (0x1F, 1),
}

# Full 25 core stats (100% verified bit-for-bit against player_schema.json & FID_LOC)
STAT_DEFS = [
    # Attack
    {"fid": 14, "group": "attack", "pt": "Talento Ofensivo", "en": "Offensive Awareness"},   # 0x03:0
    {"fid": 20, "group": "attack", "pt": "Finalização", "en": "Finishing"},                  # 0x08:7
    {"fid": 23, "group": "attack", "pt": "Cabeceio", "en": "Heading"},                       # 0x0C:0
    {"fid": 26, "group": "attack", "pt": "Bola Parada", "en": "Place Kicking"},              # 0x0C:21
    {"fid": 27, "group": "attack", "pt": "Curva", "en": "Curl"},                             # 0x10:0
    # Technique
    {"fid": 18, "group": "tech", "pt": "Controle de Bola", "en": "Ball Control"},            # 0x04:21
    {"fid": 17, "group": "tech", "pt": "Drible", "en": "Dribbling"},                         # 0x04:14
    {"fid": 19, "group": "tech", "pt": "Condução Firme", "en": "Tight Possession"},          # 0x08:0
    {"fid": 21, "group": "tech", "pt": "Passe Rasteiro", "en": "Low Pass"},                  # 0x08:14
    {"fid": 22, "group": "tech", "pt": "Passe Alto", "en": "Lofted Pass"},                   # 0x08:21
    # Physical
    {"fid": 33, "group": "phys", "pt": "Velocidade", "en": "Speed"},                         # 0x14:7
    {"fid": 37, "group": "phys", "pt": "Aceleração", "en": "Acceleration"},                  # 0x18:7
    {"fid": 36, "group": "phys", "pt": "Força do Chute", "en": "Kicking Power"},             # 0x18:0
    {"fid": 38, "group": "phys", "pt": "Impulsão", "en": "Jump"},                            # 0x18:14
    {"fid": 34, "group": "phys", "pt": "Contato Físico", "en": "Physical Contact"},          # 0x14:14
    {"fid": 35, "group": "phys", "pt": "Equilíbrio", "en": "Balance"},                       # 0x14:21
    {"fid": 39, "group": "phys", "pt": "Resistência", "en": "Stamina"},                      # 0x18:21
    # Defense
    {"fid": 15, "group": "def", "pt": "Talento Defensivo", "en": "Defensive Awareness"},     # 0x04:0
    {"fid": 24, "group": "def", "pt": "Desarme", "en": "Ball Winning"},                      # 0x0C:7
    {"fid": 25, "group": "def", "pt": "Agressividade", "en": "Aggression"},                  # 0x0C:14
    # Goalkeeper
    {"fid": 16, "group": "gk", "pt": "Talento de Goleiro", "en": "GK Awareness"},            # 0x04:7
    {"fid": 28, "group": "gk", "pt": "Firmeza (GL)", "en": "GK Catching"},                   # 0x10:7
    {"fid": 29, "group": "gk", "pt": "Afastamento (GL)", "en": "GK Clearing"},               # 0x10:14
    {"fid": 30, "group": "gk", "pt": "Reflexos (GL)", "en": "GK Reflexes"},                  # 0x10:21
    {"fid": 31, "group": "gk", "pt": "Alcance (GL)", "en": "GK Reach"},                      # 0x14:0
]

# Extract exact skill & COM AI bitfields directly from player_schema.json
PLAYER_SKILL_NAMES = {
    "Scissors Feint", "Double Touch", "Flip Flap", "Marseille Turn", "Sombrero",
    "Cross Over Turn", "Cut Behind & Turn", "Scotch Move", "Step On Skill control",
    "Heading", "Long Range Drive", "Chip shot control", "Long Range Shooting",
    "Knuckle Shot", "Dipping Shot", "Rising Shots", "Acrobatic Finishing",
    "Heel Trick", "First-time Shot", "One-touch Pass", "Through Passing",
    "Weighted Pass", "Pinpoint Crossing", "Outside Curler", "Rabona",
    "No Look Pass", "Low Lofted Pass", "GK Low Punt", "GK High Punt",
    "Long Throw", "GK Long Throw", "Penalty Specialist", "GK Penalty Saver",
    "Gamesmanship", "Man Marking", "Track Back", "Interception",
    "Acrobatic Clear", "Captaincy", "Super-Sub", "Fighting Spirit",
}

COM_AI_NAMES = {
    "Trickster", "Mazing Run", "Speeding Bullet", "Incisive Run",
    "Long Ball Expert", "Early Cross", "Long Ranger",
}

SKILL_PT_MAP = {
    "Scissors Feint": "Pedalada Simples",
    "Double Touch": "Toque Duplo",
    "Flip Flap": "Elástico",
    "Marseille Turn": "Giro de 360°",
    "Sombrero": "Chapéu",
    "Cross Over Turn": "Corte de Calcanhar",
    "Cut Behind & Turn": "Corte por Trás",
    "Scotch Move": "Finta de Corpo",
    "Step On Skill control": "Controle de Sola",
    "Heading": "Cabeceio",
    "Long Range Drive": "Chute com Curva",
    "Chip shot control": "Cavadinha",
    "Long Range Shooting": "Chute de Longa Dist.",
    "Knuckle Shot": "Folha Seca",
    "Dipping Shot": "Chute com Queda",
    "Rising Shots": "Chute Ascendente",
    "Acrobatic Finishing": "Finaliz. Acrobática",
    "Heel Trick": "Toque de Calcanhar",
    "First-time Shot": "Chute de Primeira",
    "One-touch Pass": "Passe de Primeira",
    "Through Passing": "Passe em Profundidade",
    "Weighted Pass": "Passe Medido",
    "Pinpoint Crossing": "Cruzamento Preciso",
    "Outside Curler": "Trivela",
    "Rabona": "Letra (Rabona)",
    "No Look Pass": "Passe sem Olhar",
    "Low Lofted Pass": "Passe Alto Rasteiro",
    "GK Low Punt": "Reposição Baixa (GL)",
    "GK High Punt": "Reposição Alta (GL)",
    "Long Throw": "Lateral Longo",
    "GK Long Throw": "Lançamento Longo (GL)",
    "Penalty Specialist": "Especialista Pênalti",
    "GK Penalty Saver": "Pegador de Pênaltis",
    "Gamesmanship": "Malícia",
    "Man Marking": "Marcação Individual",
    "Track Back": "Voltar para Marcar",
    "Interception": "Interceptação",
    "Acrobatic Clear": "Afast. Acrobático",
    "Captaincy": "Liderança",
    "Super-Sub": "Super Substituto",
    "Fighting Spirit": "Espírito Guerreiro",
    "Trickster": "Mágico",
    "Mazing Run": "Serpenteante",
    "Speeding Bullet": "Bala",
    "Incisive Run": "Arrancada Incisiva",
    "Long Ball Expert": "Mestre Bola Longa",
    "Early Cross": "Cruzamento Antecipado",
    "Long Ranger": "Chutador de Longe",
}

PLAYER_SKILLS: List[Dict[str, Any]] = []
COM_AI_STYLES: List[Dict[str, Any]] = []

for _e in PLAYER_SCHEMA.get("player_entries", []):
    desc = _e.get("desc", "")
    offs = _e.get("offs") or []
    bs = _e.get("bitstart")
    bl = _e.get("bitlen")
    if not offs or bs is None or bl is None or int(bl) != 1:
        continue
    off_int = int(offs[-1], 16)
    item = {
        "key": desc,
        "en": desc,
        "pt": SKILL_PT_MAP.get(desc, desc),
        "offset": off_int,
        "bit": int(bs),
    }
    if desc in PLAYER_SKILL_NAMES:
        PLAYER_SKILLS.append(item)
    elif desc in COM_AI_NAMES:
        COM_AI_STYLES.append(item)
