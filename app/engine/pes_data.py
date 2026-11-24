"""
Static dictionaries, Konami OVR tables, bitfield schemas, positions, skills,
playing styles, nationalities, and team roles for PES 2021 Workbench.
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

# Playing Styles (0..21) - Exact official PES 2021 PT-BR (dt20_bra_x64.cpk) & EN names + mechanics
PLAYING_STYLES: List[Dict[str, Any]] = [
    {"id": 0, "pt": "Nenhum", "en": "None", "pos_pt": "Todas", "pos_en": "All", "desc_pt": "Movimentação neutra sem comportamento tático pré-definido.", "desc_en": "Neutral movement with no preset tactical behavior."},
    {"id": 1, "pt": "Artilheiro", "en": "Goal Poacher", "pos_pt": "CA, SA", "pos_en": "CF, SS", "desc_pt": "Atacante voraz que joga na linha do último zagueiro buscando arrancadas nas costas da defesa.", "desc_en": "Predatory striker who plays off the shoulder of the last defender to burst in behind."},
    {"id": 2, "pt": "Puxa marcação", "en": "Dummy Runner", "pos_pt": "CA, SA, MAT", "pos_en": "CF, SS, AMF", "desc_pt": "Faz corridas para atrair os defensores e abrir espaços no meio ou na área para os companheiros.", "desc_en": "Makes decoy runs to pull defenders out of position and open space for teammates."},
    {"id": 3, "pt": "Homem de área", "en": "Fox in the Box", "pos_pt": "CA", "pos_en": "CF", "desc_pt": "Fica posicionado dentro da grande área esperando o passe, o cruzamento ou o rebote para finalizar.", "desc_en": "Lurks inside the penalty box waiting for passes, crosses, and rebounds to finish."},
    {"id": 4, "pt": "Ala produtivo", "en": "Prolific Winger", "pos_pt": "PTE, PTD", "pos_en": "LWF, RWF", "desc_pt": "Abre bem na ponta para receber passes e corta em diagonal para dentro quando surge espaço para finalizar.", "desc_en": "Positions himself wide to stretch the defense and cuts inside when space opens."},
    {"id": 5, "pt": "Clássica nº 10", "en": "Classic No. 10", "pos_pt": "SA, MAT, MLG", "pos_en": "SS, AMF, CMF", "desc_pt": "Armador estático tradicional que evita arrancadas longas e controla o jogo com toques curtos e passes refinados.", "desc_en": "Old-style static playmaker who dictates play through deft touches and passes rather than constant running."},
    {"id": 6, "pt": "Jog. de infiltração", "en": "Hole Player", "pos_pt": "SA, MAT, MLE, MLD, MLG", "pos_en": "SS, AMF, LMF, RMF, CMF", "desc_pt": "Infiltra com frequência na área adversária vindo de trás sempre que o time ataca.", "desc_en": "Makes frequent late runs from deeper areas into the opponent's penalty box during attacks."},
    {"id": 7, "pt": "Meia versátil", "en": "Box-to-Box", "pos_pt": "MAT, MLE, MLD, MLG, VOL", "pos_en": "AMF, LMF, RMF, CMF, DMF", "desc_pt": "Cobre o campo inteiro durante os 90 minutos, participando ativamente tanto da marcação quanto do ataque.", "desc_en": "Tirelessly covers every blade of grass for the full 90 minutes, helping both defense and attack."},
    {"id": 8, "pt": "Primeiro volante", "en": "Anchor Man", "pos_pt": "VOL", "pos_en": "DMF", "desc_pt": "Volante fixo à frente dos zagueiros que protege a defesa, fecha linhas de passe e raramente sobe ao ataque.", "desc_en": "Deep-sitting midfielder who shields the backline and rarely pushes forward."},
    {"id": 9, "pt": "O destruidor", "en": "The Destroyer", "pos_pt": "MLG, VOL, ZC", "pos_en": "CMF, DMF, CB", "desc_pt": "Marca com pressão agressiva e desarmes firmes para interromper os ataques adversários logo no início.", "desc_en": "Tenacious battler who breaks up opposition attacks with hard tackling and relentless pressing."},
    {"id": 10, "pt": "Atacante surpresa", "en": "Extra Frontman", "pos_pt": "ZC", "pos_en": "CB", "desc_pt": "Zagueiro que gosta de sair da linha defensiva e avançar ao ataque quando o time pressiona no campo adversário.", "desc_en": "Defender who likes to join the attack and lay siege on the opposition goal when given the chance."},
    {"id": 11, "pt": "Zagueiro ofensivo", "en": "Offensive Full-back", "pos_pt": "LE, LD", "pos_en": "LB, RB", "desc_pt": "Lateral de mentalidade ofensiva que sobe com frequência pelo corredor para apoiar e cruzar da linha de fundo.", "desc_en": "Attacking-minded full-back who overlaps frequently down the flank whenever the opportunity arises."},
    {"id": 12, "pt": "Zagueiro defensivo", "en": "Defensive Full-back", "pos_pt": "LE, LD", "pos_en": "LB, RB", "desc_pt": "Lateral sólido que prefere ficar mais recuado protegendo o setor defensivo em vez de subir ao ataque.", "desc_en": "Solid full-back who prefers staying back to prioritize defensive duties."},
    {"id": 13, "pt": "Pivô", "en": "Target Man", "pos_pt": "CA", "pos_en": "CF", "desc_pt": "Referência avançada que faz o paredão de costas para o gol para segurar a bola e acionar quem chega.", "desc_en": "Focal point of the attack who holds up the ball in an advanced position to bring teammates into play."},
    {"id": 14, "pt": "Armador criativo", "en": "Creative Playmaker", "pos_pt": "SA, PTE, PTD, MAT, MLE, MLD", "pos_en": "SS, LWF, RWF, AMF, LMF, RMF", "desc_pt": "Recua entre linhas para buscar a bola e aproveita qualquer brecha na defesa para armar jogadas e assistências.", "desc_en": "Drops into pockets between the lines to exploit defensive openings and create chances."},
    {"id": 15, "pt": "Provocador", "en": "Build Up", "pos_pt": "ZC", "pos_en": "CB", "desc_pt": "Zagueiro que recua na saída de bola para receber com calma e iniciar a construção dos ataques desde a defesa.", "desc_en": "Defender who drops deeper to receive the ball and launch attacks calmly from the back."},
    {"id": 16, "pt": "Goleiro ofensivo", "en": "Offensive Goalkeeper", "pos_pt": "GO", "pos_en": "GK", "desc_pt": "Goleiro adiantado que sai do gol com frequência para cortar lançamentos nas costas da zaga.", "desc_en": "Sweeper keeper who often comes off his line to clear balls played behind the defense."},
    {"id": 17, "pt": "Goleiro defensivo", "en": "Defensive Goalkeeper", "pos_pt": "GO", "pos_en": "GK", "desc_pt": "Goleiro firme que prefere ficar próximo da linha do gol priorizando defesas embaixo das traves.", "desc_en": "Solid goalkeeper who prefers staying close to the goal line to focus on shot-stopping."},
    {"id": 18, "pt": "Lateral móvel", "en": "Roaming Flank", "pos_pt": "PTE, PTD, MLE, MLD", "pos_en": "LWF, RWF, LMF, RMF", "desc_pt": "Corta cedo da ponta para dentro do campo para receber entre linhas e buscar finalizações por dentro.", "desc_en": "Tends to cut inside early from wide areas into central spaces to receive passes and shoot."},
    {"id": 19, "pt": "Especialista em cruz.", "en": "Cross Specialist", "pos_pt": "PTE, PTD, MLE, MLD", "pos_en": "LWF, RWF, LMF, RMF", "desc_pt": "Fica colado na linha lateral esperando espaço para alçar cruzamentos precisos na área.", "desc_en": "Hugs the touchline waiting for opportunities to whip accurate crosses into the box."},
    {"id": 20, "pt": "Orquestrador", "en": "Orchestrator", "pos_pt": "MLG, VOL", "pos_en": "CMF, DMF", "desc_pt": "Posiciona-se mais recuado no meio-campo para ditar o ritmo e iniciar ataques com passes de qualidade.", "desc_en": "Lurks in deeper midfield positions ready to control the tempo and initiate attacks."},
    {"id": 21, "pt": "Lateral atacante", "en": "Full-back Finisher", "pos_pt": "LE, LD", "pos_en": "LB, RB", "desc_pt": "Lateral ofensivo que ataca por dentro entrando em zonas centrais como meia ou elemento surpresa na área.", "desc_en": "Attacking full-back who cuts into high central areas during attacks instead of staying wide."},
]

# Position metadata (0..12) - Exact official PES 2021 PT-BR & EN labels
POSITIONS = [
    {"id": 0, "pt": "GO", "en": "GK", "name_pt": "Goleiro", "name_en": "Goalkeeper"},
    {"id": 1, "pt": "ZC", "en": "CB", "name_pt": "Zagueiro central", "name_en": "Centre Back"},
    {"id": 2, "pt": "LE", "en": "LB", "name_pt": "Lateral esquerdo", "name_en": "Left Back"},
    {"id": 3, "pt": "LD", "en": "RB", "name_pt": "Lateral direito", "name_en": "Right Back"},
    {"id": 4, "pt": "VOL", "en": "DMF", "name_pt": "Volante", "name_en": "Defensive Midfielder"},
    {"id": 5, "pt": "MLG", "en": "CMF", "name_pt": "Meia de ligação", "name_en": "Centre Midfielder"},
    {"id": 6, "pt": "MLE", "en": "LMF", "name_pt": "Meia esquerda", "name_en": "Left Midfielder"},
    {"id": 7, "pt": "MLD", "en": "RMF", "name_pt": "Meia direita", "name_en": "Right Midfielder"},
    {"id": 8, "pt": "MAT", "en": "AMF", "name_pt": "Meia atacante", "name_en": "Attacking Midfielder"},
    {"id": 9, "pt": "PTE", "en": "LWF", "name_pt": "Ponta esquerda", "name_en": "Left Wing Forward"},
    {"id": 10, "pt": "PTD", "en": "RWF", "name_pt": "Ponta direita", "name_en": "Right Wing Forward"},
    {"id": 11, "pt": "SA", "en": "SS", "name_pt": "Segundo atacante", "name_en": "Second Striker"},
    {"id": 12, "pt": "CA", "en": "CF", "name_pt": "Centroavante", "name_en": "Centre Forward"},
]

# Team Roles (0..22) - Exact official PES 2021 PT-BR (dt20_bra_x64.cpk) & EN names + Captain Effects
TEAM_ROLES = [
    {
        "id": 0, "pt": "Nenhum", "en": "None",
        "captain": False, "tags": [],
        "desc_pt": "Sem função especial ativa na Master League. O atleta atua normalmente sem gerar bônus extras de treino, condição, receita ou capitania.",
        "desc_en": "No active Master League squad role. The player performs normally with no extra training, condition, revenue, or captain perks.",
        "captain_pt": "", "captain_en": "",
    },
    {
        "id": 1, "pt": "Jovem promessa", "en": "Youth Prospect",
        "captain": False, "tags": ["xp"],
        "desc_pt": "Atleta jovem promovido em fase inicial de evolução. Ganha mais efeitos de treinamento e domina novas habilidades de jogador mais rápido.",
        "desc_en": "Young talent in early development. Gains extra training effects and masters player skills faster.",
        "captain_pt": "", "captain_en": "",
    },
    {
        "id": 2, "pt": "Protegido", "en": "Protégé",
        "captain": False, "tags": ["xp"],
        "desc_pt": "Jovem apadrinhado pelo técnico que evolui muito rápido jogando. Ganha mais pontos de experiência por partida e aprende habilidades mais depressa.",
        "desc_en": "Mentored youngster who develops rapidly through game time. Earns more experience points per match and masters player skills faster.",
        "captain_pt": "", "captain_en": "",
    },
    {
        "id": 3, "pt": "Jogador fiel", "en": "Team Player",
        "captain": False, "tags": ["spirit"],
        "desc_pt": "Peça coletiva e altruísta que une o elenco. Aumenta o Espírito de equipe (P) sempre que entra em campo.",
        "desc_en": "Selfless squad member who strengthens team cohesion. Raises Team Spirit (S) whenever he plays.",
        "captain_pt": "", "captain_en": "",
    },
    {
        "id": 4, "pt": "Organizador de jogo", "en": "Playmaker",
        "captain": True, "tags": ["fin", "spirit", "cap"],
        "desc_pt": "Referência tática respeitada pela torcida e pelo elenco. Gera bônus de receita de torcedores (M) e aumenta o Espírito de equipe (M) quando joga.",
        "desc_en": "Tactical leader popular with fans and teammates. Adds a Fan-Favorite revenue bonus (M) and raises Team Spirit (M) when playing.",
        "captain_pt": "Eleva ainda mais o Espírito de equipe e aumenta a taxa de vitória ao pular ou simular partidas (P).",
        "captain_en": "Further boosts Team Spirit and increases win rate when skipping matches (S).",
    },
    {
        "id": 5, "pt": "Astro", "en": "Star Player",
        "captain": False, "tags": ["fin"],
        "desc_pt": "Jogador carismático com forte apelo junto à torcida. Concede bônus de receita de torcedores e produtos do clube (P).",
        "desc_en": "Charismatic player with strong supporter appeal. Grants a Fan-Favorite revenue and merchandise bonus (S).",
        "captain_pt": "", "captain_en": "",
    },
    {
        "id": 6, "pt": "Líder", "en": "Leader",
        "captain": False, "tags": ["cond"],
        "desc_pt": "Presença motivadora que puxa a entrega do grupo antes de jogos grandes. Melhora a condição de alguns companheiros em partidas importantes (P).",
        "desc_en": "Passionate presence whose dedication lifts the dressing room. Boosts the condition of selected teammates for key matches (S).",
        "captain_pt": "", "captain_en": "",
    },
    {
        "id": 7, "pt": "General", "en": "General",
        "captain": True, "tags": ["cond", "xp", "cap"],
        "desc_pt": "Comandante respeitado em momentos difíceis. Melhora a condição de colegas em jogos importantes (M) e aumenta o treino (M) para todos da mesma posição registrada.",
        "desc_en": "Commanding anchor in tough situations. Boosts teammate condition for key matches (M) and increases training effects (M) for all peers in the same registered position.",
        "captain_pt": "Reduz o desgaste e os efeitos do cansaço em todo o time durante as partidas (P).",
        "captain_en": "Reduces fatigue effects across the entire team during matches (S).",
    },
    {
        "id": 8, "pt": "Criador", "en": "Creator",
        "captain": False, "tags": ["xp"],
        "desc_pt": "Mentor técnico focado em evolução. Aumenta os efeitos de treino (P) para todos os companheiros com a mesma posição registrada, mesmo fora de campo.",
        "desc_en": "Creative technical mentor. Boosts training effects (S) for all teammates sharing the same registered position, even off the pitch.",
        "captain_pt": "", "captain_en": "",
    },
    {
        "id": 9, "pt": "Mestre", "en": "Maestro",
        "captain": True, "tags": ["fin", "xp", "cap"],
        "desc_pt": "Combinação de carisma e técnica que inspira o elenco. Gera bônus de receita de torcedores (M) e turbina o treino (M) de todos da mesma posição registrada.",
        "desc_en": "Blend of charisma and technique that elevates peers. Adds a Fan-Favorite bonus (M) and boosts training effects (M) for all players in the same registered position.",
        "captain_pt": "Faz os companheiros em campo ganharem mais pontos de experiência por partida (P).",
        "captain_en": "Teammates on the pitch earn extra experience points from matches (S).",
    },
    {
        "id": 10, "pt": "Motorzinho", "en": "Workhorse",
        "captain": True, "tags": ["cond", "spirit", "cap"],
        "desc_pt": "Dedicação incansável cobrindo cada centímetro do campo. Melhora a condição de colegas em jogos importantes (M) e sobe o Espírito de equipe (M) quando joga.",
        "desc_en": "Tireless team engine who sets the physical standard. Boosts teammate condition for key matches (M) and increases Team Spirit (M) when playing.",
        "captain_pt": "Reduz o desgaste e os efeitos do cansaço em todo o time durante as partidas (P).",
        "captain_en": "Reduces fatigue effects across the entire team during matches (S).",
    },
    {
        "id": 11, "pt": "Inteligente", "en": "Smart Player",
        "captain": True, "tags": ["xp", "spirit", "cap"],
        "desc_pt": "Sexto sentido tático que faz o time render mais. Aumenta o treino (M) para todos da mesma posição registrada e eleva o Espírito de equipe (M) quando joga.",
        "desc_en": "Tactically sharp player who improves squad cohesion. Boosts training effects (M) for same-position peers and raises Team Spirit (M) when playing.",
        "captain_pt": "Eleva ainda mais o Espírito de equipe e aumenta a taxa de vitória ao pular ou simular partidas (P).",
        "captain_en": "Further boosts Team Spirit and increases win rate when skipping matches (S).",
    },
    {
        "id": 12, "pt": "Batalhador", "en": "Fighter",
        "captain": True, "tags": ["fin", "cond", "cap"],
        "desc_pt": "Entrega máxima em cada dividida e muito querido pela arquibancada. Gera bônus de receita de torcedores (M) e melhora a condição de colegas em jogos importantes (M).",
        "desc_en": "Fierce battler adored by supporters. Grants a Fan-Favorite revenue bonus (M) and lifts teammate condition in key matches (M).",
        "captain_pt": "Faz os companheiros em campo ganharem mais pontos de experiência por partida (P).",
        "captain_en": "Teammates on the pitch earn extra experience points from matches (S).",
    },
    {
        "id": 13, "pt": "Homem de referência", "en": "Key Player",
        "captain": True, "tags": ["fin", "cond", "xp", "spirit", "cap"],
        "desc_pt": "Pilar completo do time em todas as frentes. Gera receita de torcedores (M), melhora a condição de colegas em todos os jogos (M), eleva o treino na mesma posição (M) e sobe o Espírito de equipe (M).",
        "desc_en": "All-around pillar of the squad. Adds Fan-Favorite revenue (M), boosts teammate condition for all matches (M), increases same-position training (M), and raises Team Spirit (M).",
        "captain_pt": "Eleva ainda mais o Espírito de equipe e aumenta a taxa de vitória ao pular ou simular partidas (M).",
        "captain_en": "Further boosts Team Spirit and increases win rate when skipping matches (M).",
    },
    {
        "id": 14, "pt": "Superastro", "en": "Superstar",
        "captain": True, "tags": ["fin", "cond", "xp", "spirit", "cap"],
        "desc_pt": "Fenômeno mundial que dispara bilheteria e receita de torcedores (G), melhora condição em todos os jogos (P), treino na posição (M) e Espírito de equipe (P). Facilita contratar outras posições, mas dificulta trazer concorrentes da mesma posição.",
        "desc_en": "Global superstar who massively boosts gate receipts and Fan-Favorite income (L), plus condition (S), same-position training (M), and Team Spirit (S). Makes signing other positions easier, but same-position signings harder.",
        "captain_pt": "Reduz fortemente os efeitos do cansaço em todo o time durante as partidas (G).",
        "captain_en": "Heavily reduces fatigue effects across the entire team during matches (L).",
    },
    {
        "id": 15, "pt": "Herói", "en": "Hero",
        "captain": True, "tags": ["fin", "cond", "xp", "spirit", "cap"],
        "desc_pt": "Raça contagiante que carrega o grupo. Gera receita (M), melhora fortemente a condição de colegas em todos os jogos (G), ajuda no treino da posição (P) e sobe Espírito de equipe (P). Sua condição em campo afeta diretamente a dos companheiros.",
        "desc_en": "Clutch club hero who inspires the squad. Adds revenue (M), heavily lifts teammate condition for all matches (L), boosts same-position training (S), and raises Team Spirit (S). His own form influences teammates on the pitch.",
        "captain_pt": "Reduz fortemente os efeitos do cansaço em todo o time durante as partidas (G).",
        "captain_en": "Heavily reduces fatigue effects across the entire team during matches (L).",
    },
    {
        "id": 16, "pt": "Virtuoso", "en": "Virtuoso",
        "captain": True, "tags": ["fin", "cond", "xp", "spirit", "cap"],
        "desc_pt": "Pura magia com a bola nos pés e referência para formar craques. Gera receita (P), condição em todos os jogos (P), Espírito de equipe (M) e enorme bônus de treino (G) na mesma posição. A experiência dos colegas varia conforme a nota dele no jogo.",
        "desc_en": "Elite technical mentor for developing stars. Grants revenue (S), all-match condition (S), Team Spirit (M), and a huge training boost (L) for same-position peers. Teammate match EXP also scales with his match rating.",
        "captain_pt": "Faz os companheiros em campo ganharem muito mais pontos de experiência por partida (G).",
        "captain_en": "Teammates on the pitch earn a large bonus to match experience points (L).",
    },
    {
        "id": 17, "pt": "Maestro", "en": "Conductor",
        "captain": True, "tags": ["fin", "cond", "xp", "spirit", "cap"],
        "desc_pt": "A figura mais próxima de um técnico em campo, regendo o time como uma orquestra. Gera receita (P), condição em todos os jogos (M), treino na posição (P) e o maior impacto base em Espírito de equipe (G), subindo mais quando o time está em boa fase.",
        "desc_en": "The manager's extension on the pitch, directing the team like an orchestra. Grants revenue (S), all-match condition (M), same-position training (S), and a massive Team Spirit boost (L) that grows when players are in good form.",
        "captain_pt": "Eleva ainda mais o Espírito de equipe, acelerando o entrosamento tático máximo.",
        "captain_en": "Further amplifies Team Spirit, helping the squad reach peak tactical chemistry faster.",
    },
    {
        "id": 18, "pt": "Ídolo", "en": "Bandiera",
        "captain": True, "tags": ["fin", "cond", "xp", "spirit", "cap"],
        "desc_pt": "Símbolo máximo de lealdade ao clube (nível máximo em tudo). Bônus grande (G) em receita de torcedores, condição em todos os jogos, treino na mesma posição e Espírito de equipe. Também faz os companheiros aprenderem habilidades mais rápido.",
        "desc_en": "Ultimate one-club icon (top tier across all categories). Large (L) boosts to Fan-Favorite income, all-match condition, same-position training, and Team Spirit. Also helps teammates master player skills faster.",
        "captain_pt": "Eleva ainda mais o Espírito de equipe, aumenta a taxa de vitória ao pular jogos (M) e reduz o cansaço de todo o time (P).",
        "captain_en": "Further boosts Team Spirit, raises win rate when skipping matches (M), and reduces whole-team fatigue (S).",
    },
    {
        "id": 19, "pt": "Lenda", "en": "Legend",
        "captain": True, "tags": ["fin", "cond", "xp", "spirit", "cap"],
        "desc_pt": "Status supremo respeitado no mundo inteiro (nível máximo em tudo). Bônus grande (G) em receita, condição em todos os jogos, treino para a mesma posição e Espírito de equipe. Também acelera o aprendizado de habilidades dos colegas.",
        "desc_en": "Supreme global icon (top tier across all categories). Large (L) boosts to revenue, all-match condition, same-position training, and Team Spirit. Also helps teammates master player skills faster.",
        "captain_pt": "Eleva ainda mais o Espírito de equipe, aumenta a taxa de vitória ao pular jogos (M) e faz os colegas ganharem mais experiência nas partidas (M).",
        "captain_en": "Further boosts Team Spirit, raises win rate when skipping matches (M), and grants teammates bonus match EXP (M).",
    },
    {
        "id": 20, "pt": "Astro em ascensão", "en": "Rising Star",
        "captain": False, "tags": ["xp", "fin"],
        "desc_pt": "Jovem em constante boa fase que puxa o entusiasmo do grupo. Ganha mais XP por jogo, aprende habilidades mais rápido, tem mais chance de grande salto de evolução conforme a nota em campo e valoriza ofertas de venda.",
        "desc_en": "Breakout youngster on a hot streak. Earns extra match EXP, masters skills faster, gets higher breakthrough odds based on match ratings, and attracts larger transfer offers.",
        "captain_pt": "", "captain_en": "",
    },
    {
        "id": 21, "pt": "Bad boy", "en": "Bad Boy",
        "captain": False, "tags": ["xp", "cond"],
        "desc_pt": "Ginga e técnica, mas nem sempre anda na linha. Ganha mais XP por jogo, aprende habilidades mais rápido e tem a própria condição elevada, mas pode baixar a condição dos colegas após atuações ruins e pegar suspensões maiores em expulsões.",
        "desc_en": "Skilful but rebellious wildcard. Earns extra match EXP, learns skills faster, and gets a personal condition boost, but can lower teammates' condition after poor displays and face longer red-card bans.",
        "captain_pt": "", "captain_en": "",
    },
    {
        "id": 22, "pt": "Arriscado", "en": "Risk-Taker",
        "captain": False, "tags": ["cond", "xp", "spirit"],
        "desc_pt": "Personalidade indomável e pouco cautelosa que muda jogos. Melhora a condição de colegas em jogos importantes (M) e turbina o treino da mesma posição (M). Aumenta o Espírito de equipe se o time está em boa forma, mas derruba se estiver mal.",
        "desc_en": "Fearless, high-variance maverick. Boosts teammate condition in key matches (M) and same-position training (M). Raises Team Spirit when players are in good form, or drops it in poor form.",
        "captain_pt": "", "captain_en": "",
    },
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

# Full 25 core stats - Exact official PES 2021 PT-BR (dt20_bra_x64.cpk) & EN names (verified bit-for-bit)
STAT_DEFS = [
    # Attack
    {"fid": 14, "group": "attack", "pt": "Talento ofensivo", "en": "Offensive Awareness", "desc_pt": "Velocidade de reação e inteligência para se desmarcar e atacar espaços livres no ataque.", "desc_en": "Reaction speed and intelligence when making runs into attacking spaces."},
    {"fid": 20, "group": "attack", "pt": "Finalização", "en": "Finishing", "desc_pt": "Precisão e frieza ao chutar a gol de dentro ou perto da área.", "desc_en": "Accuracy and composure when shooting on goal."},
    {"fid": 23, "group": "attack", "pt": "Cabeceio", "en": "Heading", "desc_pt": "Precisão e direcionamento nos cabeceios ofensivos e defensivos.", "desc_en": "Accuracy and direction when heading the ball in attack and defense."},
    {"fid": 26, "group": "attack", "pt": "Chute colocado", "en": "Place Kicking", "desc_pt": "Precisão em bolas paradas, incluindo cobranças de falta, escanteios e pênaltis.", "desc_en": "Accuracy on set pieces, including free kicks, corners, and penalties."},
    {"fid": 27, "group": "attack", "pt": "Curva", "en": "Curl", "desc_pt": "Quantidade de efeito lateral aplicado em chutes colocados, faltas e cruzamentos.", "desc_en": "Amount of bend applied to finesse shots, free kicks, and crosses."},
    # Technique
    {"fid": 18, "group": "tech", "pt": "Controle de bola", "en": "Ball Control", "desc_pt": "Qualidade do domínio ao receber passes fortes ou difíceis e eficiência em fintas.", "desc_en": "Overall ball control, affecting first-touch traps and feints."},
    {"fid": 17, "group": "tech", "pt": "Drible", "en": "Dribbling", "desc_pt": "Controle e velocidade ao conduzir a bola em arrancada.", "desc_en": "Control and speed when carrying the ball at pace."},
    {"fid": 19, "group": "tech", "pt": "Condução firme", "en": "Tight Possession", "desc_pt": "Agilidade para girar e mudar de direção em baixa velocidade sob marcação apertada.", "desc_en": "Agility when turning and keeping control while dribbling at low speed."},
    {"fid": 21, "group": "tech", "pt": "Passe rasteiro", "en": "Low Pass", "desc_pt": "Precisão de passes curtos, tabelas e enfiadas rasteiras.", "desc_en": "Accuracy of short ground passes and low through balls."},
    {"fid": 22, "group": "tech", "pt": "Passe alto", "en": "Lofted Pass", "desc_pt": "Precisão em lançamentos longos, inversões de jogo e cruzamentos.", "desc_en": "Accuracy of long balls, switches of play, and crosses."},
    # Physical
    {"fid": 33, "group": "phys", "pt": "Velocidade", "en": "Speed", "desc_pt": "Velocidade máxima de corrida sem a bola em campo aberto.", "desc_en": "Top sprinting speed off the ball over open ground."},
    {"fid": 37, "group": "phys", "pt": "Aceleração", "en": "Acceleration", "desc_pt": "Rapidez nos primeiros metros para atingir a velocidade máxima.", "desc_en": "How quickly the player reaches top sprinting speed."},
    {"fid": 36, "group": "phys", "pt": "Força do chute", "en": "Kicking Power", "desc_pt": "Potência e velocidade da bola em chutes e lançamentos fortes.", "desc_en": "Shot velocity and power generated on strikes and driven passes."},
    {"fid": 38, "group": "phys", "pt": "Impulsão", "en": "Jump", "desc_pt": "Altura alcançada nos saltos para vencer disputas aéreas.", "desc_en": "Vertical leap height in aerial duels."},
    {"fid": 34, "group": "phys", "pt": "Contato físico", "en": "Physical Contact", "desc_pt": "Força corporal para afastar marcadores, vencer divididas e proteger a bola.", "desc_en": "Body strength to fend off opponents and win shoulder-to-shoulder duels."},
    {"fid": 35, "group": "phys", "pt": "Equilíbrio", "en": "Balance", "desc_pt": "Capacidade de evitar desarmes e se manter de pé após contatos físicos.", "desc_en": "Ability to evade tackles and stay on his feet after physical contact."},
    {"fid": 39, "group": "phys", "pt": "Resistência", "en": "Stamina", "desc_pt": "Nível de preparo físico. Valores altos reduzem o desgaste ao longo dos 90 minutos.", "desc_en": "Fitness level. Higher values reduce stamina drain across the match."},
    # Defense
    {"fid": 15, "group": "def", "pt": "Talento defensivo", "en": "Defensive Awareness", "desc_pt": "Posicionamento e leitura defensiva para fechar espaços e acompanhar atacantes.", "desc_en": "Defensive positioning and reading of the game to track attackers."},
    {"fid": 24, "group": "def", "pt": "Desarme", "en": "Ball Winning", "desc_pt": "Eficiência ao roubar a bola em botes e carrinhos sem cometer falta.", "desc_en": "Proficiency at winning the ball cleanly from opponents."},
    {"fid": 25, "group": "def", "pt": "Agressividade", "en": "Aggression", "desc_pt": "Intensidade com que o jogador pressiona e dá o bote no portador da bola.", "desc_en": "Intensity when pressing and challenging the ball carrier."},
    # Goalkeeper
    {"fid": 16, "group": "gk", "pt": "Talento de GO", "en": "GK Awareness", "desc_pt": "Velocidade de reação e posicionamento do goleiro ao defender a meta.", "desc_en": "Goalkeeper reaction speed and positioning when defending the goal."},
    {"fid": 28, "group": "gk", "pt": "Firmeza do GO", "en": "GK Catching", "desc_pt": "Segurança para agarrar chutes fortes sem dar rebote na área.", "desc_en": "Ability to catch powerful shots cleanly without spilling rebounds."},
    {"fid": 29, "group": "gk", "pt": "Afast. de bola do GO", "en": "GK Clearing", "desc_pt": "Precisão ao espalmar ou socar a bola para zonas seguras longe dos atacantes.", "desc_en": "Ability to parry or punch shots safely away from danger."},
    {"fid": 30, "group": "gk", "pt": "Reflexos do GO", "en": "GK Reflexes", "desc_pt": "Velocidade de resposta em finalizações à queima-roupa.", "desc_en": "Reaction speed against close-range strikes."},
    {"fid": 31, "group": "gk", "pt": "Alcance do GO", "en": "GK Reach", "desc_pt": "Área do gol coberta pelo goleiro em saltos para buscar bolas nos cantos.", "desc_en": "Coverage across the goal frame when diving for corner shots."},
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

SKILL_META = {
    # Dribbling & Ball Control (9) - Exact official PES 2021 PT-BR (dt20_bra_x64.cpk) & EN names
    "Scissors Feint": {
        "cat": "dribble", "pt": "Pedalada simples", "en": "Scissors Feint",
        "desc_pt": "Executa pedaladas em alta velocidade sem perder ritmo na corrida, facilitando passar pelo marcador no 1 contra 1.",
        "desc_en": "Executes the Scissors Feint at full pace without losing momentum, making 1v1 take-ons much sharper.",
    },
    "Double Touch": {
        "cat": "dribble", "pt": "Toque duplo", "en": "Double Touch",
        "desc_pt": "Realiza o drible de dois toques (La Croqueta) com animação mais rápida e curta, ideal para limpar o marcador e abrir espaço para chutar ou passar.",
        "desc_en": "Performs a quicker, tighter Double Touch (La Croqueta), ideal for shifting past a defender to open a shooting or passing lane.",
    },
    "Flip Flap": {
        "cat": "dribble", "pt": "Elástico", "en": "Flip Flap",
        "desc_pt": "Libera o drible elástico rápido para mudar de direção em espaço reduzido. Combinado com Toque duplo e Controle de domínio, ativa a animação especial mais rápida do jogo.",
        "desc_en": "Unlocks fast Flip Flap (Elastico) feints in tight spaces. Combined with Double Touch and Step On Skill control, triggers the fastest ball-roll animation.",
    },
    "Marseille Turn": {
        "cat": "dribble", "pt": "360 graus", "en": "Marseille Turn",
        "desc_pt": "Executa o drible 360 graus (roleta) com mais agilidade e proteção de bola para escapar de marcadores chegando de frente ou de lado.",
        "desc_en": "Performs the 360-degree Marseille Turn more smoothly, protecting the ball to escape oncoming tacklers.",
    },
    "Sombrero": {
        "cat": "dribble", "pt": "Chapéu", "en": "Sombrero",
        "desc_pt": "Permite dar chapéus limpos sobre o defensor, além de liberar domínios plásticos no ar como o drible da vaca invertido e o chapéu de letra.",
        "desc_en": "Enables clean Sombrero flicks over defenders, plus stylish aerial controls like the Boomerang Trap and Crescent Turn.",
    },
    "Cross Over Turn": {
        "cat": "dribble", "pt": "Corte de calcanhar", "en": "Cross Over Turn",
        "desc_pt": "Executa o corte rápido passando a bola por trás da perna de apoio (estilo Cristiano Ronaldo) para quebrar a cintura do marcador em velocidade.",
        "desc_en": "Quickly executes the Ronaldo Chop behind the standing leg to sharply change direction while sprinting.",
    },
    "Cut Behind & Turn": {
        "cat": "dribble", "pt": "Puxada de letra", "en": "Cut Behind & Turn",
        "desc_pt": "Puxa a bola por trás do corpo durante a condução ou finta de chute, invertendo o sentido com precisão para deixar o zagueiro passar direto.",
        "desc_en": "Pulls the ball cleanly behind the standing leg on fake shots or turns, reversing direction while sending defenders the wrong way.",
    },
    "Scotch Move": {
        "cat": "dribble", "pt": "Finta de letra", "en": "Scotch Move",
        "desc_pt": "Finta rápida levando a bola para o lado oposto com o pé dominante para abrir corredor lateral ou enganar o marcador na entrada da área.",
        "desc_en": "Executes the quick Scotch Move feint with the stronger foot to open a wide lane or shift past markers near the box.",
    },
    "Step On Skill control": {
        "cat": "dribble", "pt": "Controle de domínio", "en": "Step On Skill control",
        "desc_pt": "Usa a sola do pé com muito mais fluidez para dominar, girar curto e emendar fintas sob pressão apertada sem perder a bola.",
        "desc_en": "Uses the sole of the foot with extra agility for tight traps, quick pivots, and chained feints under close pressure.",
    },
    # Shooting & Finishing (10)
    "Heading": {
        "cat": "shoot", "pt": "Cabeçada", "en": "Heading",
        "desc_pt": "Aumenta a precisão das cabeçadas e faz o atleta direcionar mais cabeçadas picadas para o chão, muito mais difíceis para o goleiro defender.",
        "desc_en": "Improves header accuracy and triggers downward headers more often, making them much harder for goalkeepers to reach.",
    },
    "Long Range Drive": {
        "cat": "shoot", "pt": "Chute de longe", "en": "Long Range Drive",
        "desc_pt": "Aumenta bastante a precisão, o efeito e o alcance dos chutes colocados em curva (R2/RB) de média e longa distância buscando o ângulo.",
        "desc_en": "Greatly enhances accuracy, bend, and effective range on curling finesse shots (R2/RB) from outside the box.",
    },
    "Chip shot control": {
        "cat": "shoot", "pt": "Controle da cavadinha", "en": "Chip shot control",
        "desc_pt": "Deixa as finalizações por cobertura (L1/LB) muito mais precisas e controladas quando o goleiro sai do gol no 1 contra 1.",
        "desc_en": "Makes lofted chip shots (L1/LB) far more accurate and controlled when the goalkeeper rushes out 1v1.",
    },
    "Long Range Shooting": {
        "cat": "shoot", "pt": "Precisão à distância", "en": "Long Range Shooting",
        "desc_pt": "Aumenta a precisão e reduz o erro de pontaria em chutes fortes de fora da área, fazendo a bola ir mais vezes na direção do gol.",
        "desc_en": "Boosts shooting accuracy and reduces shot scatter on powerful strikes taken from outside the penalty box.",
    },
    "Knuckle Shot": {
        "cat": "shoot", "pt": "Chute com o peito do pé", "en": "Knuckle Shot",
        "desc_pt": "Facilita acertar bombas sem rotação (em bola rolando e em faltas) com trajetória imprevisível no ar que complica a defesa do goleiro.",
        "desc_en": "Makes it easier to trigger zero-spin knuckle shots in open play and free kicks, causing erratic mid-air movement.",
    },
    "Dipping Shot": {
        "cat": "shoot", "pt": "Folha seca", "en": "Dipping Shot",
        "desc_pt": "Libera chutes com forte efeito para baixo que caem bruscamente no fim da trajetória e quicam de forma imprevisível na frente do goleiro.",
        "desc_en": "Unlocks heavy topspin strikes that dip sharply late in flight and bounce awkwardly right in front of the goalkeeper.",
    },
    "Rising Shots": {
        "cat": "shoot", "pt": "Chute ascendente", "en": "Rising Shots",
        "desc_pt": "Libera chutes rasantes e violentos que começam baixos e decolam rápido com força em direção ao travessão, dificultando a reação do goleiro.",
        "desc_en": "Unlocks fierce low-trajectory strikes that rise rapidly toward the top of the net, giving keepers little reaction time.",
    },
    "Acrobatic Finishing": {
        "cat": "shoot", "pt": "Finaliz. acrobática", "en": "Acrobatic Finishing",
        "desc_pt": "Permite concluir bolas difíceis com voleios, bicicletas, puxetas e chutes desequilibrados com muito mais precisão e menos finalizações mascadas.",
        "desc_en": "Allows clean volleys, overhead kicks, and off-balance finishes from awkward body positions with far fewer mishits.",
    },
    "Heel Trick": {
        "cat": "shoot", "pt": "Toque de calcanhar", "en": "Heel Trick",
        "desc_pt": "Libera passes e finalizações precisas de calcanhar mesmo de costas para o alvo ou sem equilíbrio, agilizando jogadas dentro da área.",
        "desc_en": "Enables accurate backheel passes and finishes even with his back to the target or off balance.",
    },
    "First-time Shot": {
        "cat": "shoot", "pt": "Chute de primeira", "en": "First-time Shot",
        "desc_pt": "Aumenta muito a técnica e a precisão ao finalizar de primeira ou girando rápido, essencial para centroavantes e meias que infiltram na área.",
        "desc_en": "Greatly improves shot accuracy and release speed on one-touch strikes and shots on the turn, crucial for strikers and hole players.",
    },
    # Passing & Crossing (8)
    "One-touch Pass": {
        "cat": "pass", "pt": "Passe de primeira", "en": "One-touch Pass",
        "desc_pt": "Reduz drasticamente erros em passes de primeira e tabelas rápidas, mantendo precisão e velocidade mesmo com o corpo mal posicionado ou sob pressão.",
        "desc_en": "Drastically cuts errors on instant one-touch passes and quick wall passes, preserving accuracy even off-balance or under pressure.",
    },
    "Through Passing": {
        "cat": "pass", "pt": "Passe em profundidade", "en": "Through Passing",
        "desc_pt": "Aumenta a precisão e o peso dos passes enfiados (triângulo/Y), encontrando melhor o espaço nas costas da zaga para deixar o atacante na cara do gol.",
        "desc_en": "Improves accuracy and weighting on through balls, slicing passes cleanly into space behind the defensive line.",
    },
    "Weighted Pass": {
        "cat": "pass", "pt": "Passe na medida", "en": "Weighted Pass",
        "desc_pt": "Aplica giro para trás (backspin) em passes altos e lançamentos enfiados, fazendo a bola frear ao quicar para o companheiro dominar sem esforço.",
        "desc_en": "Applies backspin to lofted passes and chipped through balls so the ball slows on bounce for an easy first touch.",
    },
    "Pinpoint Crossing": {
        "cat": "pass", "pt": "Cruzamento preciso", "en": "Pinpoint Crossing",
        "desc_pt": "Aumenta a curva, a velocidade e a exatidão dos cruzamentos da ponta para a área, buscando a cabeça ou o pé do atacante com muito mais consistência.",
        "desc_en": "Adds whip, bend, and precision to wide crosses, consistently finding attackers' heads or feet inside the box.",
    },
    "Outside Curler": {
        "cat": "pass", "pt": "Curva para fora", "en": "Outside Curler",
        "desc_pt": "Permite bater de trivela com a parte externa do pé bom em passes, cruzamentos e chutes, compensando ângulos ruins sem precisar usar o pior pé.",
        "desc_en": "Uses the outside of the stronger foot for passes, crosses, and shots, avoiding awkward angles and weaker-foot errors.",
    },
    "Rabona": {
        "cat": "pass", "pt": "De letra", "en": "Rabona",
        "desc_pt": "Permite cruzar, passar ou finalizar de letra para acionar o pé dominante em ângulos fechados, pegando os adversários de surpresa.",
        "desc_en": "Triggers Rabona passes, crosses, or shots to stay on the stronger foot from tight angles and catch defenders off guard.",
    },
    "No Look Pass": {
        "cat": "pass", "pt": "Passe sem olhar", "en": "No Look Pass",
        "desc_pt": "Engana marcadores próximos olhando para um lado e tocando para o outro, atrasando a reação defensiva em passes curtos.",
        "desc_en": "Deceives nearby defenders by looking one way and passing another, briefly delaying their defensive reaction.",
    },
    "Low Lofted Pass": {
        "cat": "pass", "pt": "Passe aéreo baixo", "en": "Low Lofted Pass",
        "desc_pt": "Dispara lançamentos longos e viradas de jogo em trajetória mais baixa e tensa, chegando bem mais rápido ao destino (não ativa em cruzamentos).",
        "desc_en": "Drives long lofted passes and switches of play on a lower, faster, flatter arc so they arrive much sooner (does not apply to crosses).",
    },
    # Defense & Physical (5)
    "Man Marking": {
        "cat": "def", "pt": "Marcação individual", "en": "Man Marking",
        "desc_pt": "Faz o defensor grudar no atacante adversário e acompanhar suas movimentações insistentemente sem dar espaço para girar ou receber livre.",
        "desc_en": "Makes defenders stick tight to opposing attackers and shadow their runs relentlessly, denying room to turn or receive.",
    },
    "Track Back": {
        "cat": "def", "pt": "Volta para marcar", "en": "Track Back",
        "desc_pt": "Faz atacantes e meias ofensivos pressionarem ativamente o adversário com a bola assim que o time perde a posse, ajudando na marcação alta.",
        "desc_en": "Makes forwards and attacking midfielders actively chase and press opponents with the ball to win back possession.",
    },
    "Interception": {
        "cat": "def", "pt": "Interceptação", "en": "Interception",
        "desc_pt": "Melhora o posicionamento e o reflexo para esticar a perna e cortar passes ou enfiadas adversárias automaticamente quando a bola passa perto.",
        "desc_en": "Sharpens positioning and automatic leg reach to cut out opposition passes and through balls passing nearby.",
    },
    "Acrobatic Clear": {
        "cat": "def", "pt": "Afastamento acrobático", "en": "Acrobatic Clear",
        "desc_pt": "Permite afastar bolas perigosas usando os pés com voleios, carrinhos aéreos e chutes acrobáticos mesmo em posições estranhas ou sem equilíbrio.",
        "desc_en": "Enables desperate overhead, sliding, or off-balance clearances with the feet to rescue dangerous balls out of normal reach.",
    },
    "Long Throw": {
        "cat": "def", "pt": "Arrem. lateral longo", "en": "Long Throw",
        "desc_pt": "Aumenta muito o alcance das cobranças de lateral, permitindo jogar a bola direto dentro da área adversária como um escanteio curto.",
        "desc_en": "Greatly extends throw-in distance, letting the player launch balls straight into the penalty box like a set piece.",
    },
    # Goalkeeper, Set Pieces & Mental (9)
    "GK Low Punt": {
        "cat": "gk_mental", "pt": "Repos. baixa do GO", "en": "GK Low Punt",
        "desc_pt": "Habilita o chutão rasteiro, longo e preciso do goleiro direto nos pés dos atacantes ou pontas para puxar contra-ataques rápidos.",
        "desc_en": "Enables flat, driven, highly accurate goalkeeper punts straight to forwards or wingers to spark fast counterattacks.",
    },
    "GK High Punt": {
        "cat": "gk_mental", "pt": "Reposição alta do GO", "en": "GK High Punt",
        "desc_pt": "Habilita a reposição longa e alta do goleiro mandando a bola fundo no campo adversário para disputas aéreas com o centroavante.",
        "desc_en": "Enables deep, towering punts far into opposition territory to target tall strikers in aerial duels.",
    },
    "GK Long Throw": {
        "cat": "gk_mental", "pt": "Arrem. longo do GO", "en": "GK Long Throw",
        "desc_pt": "Aumenta consideravelmente o alcance e a força da reposição com as mãos, passando do meio-campo para iniciar contra-ataques limpos.",
        "desc_en": "Significantly extends the range and power of goalkeeper hand throws past midfield to launch clean counterattacks.",
    },
    "Penalty Specialist": {
        "cat": "gk_mental", "pt": "Especialista em pênalti", "en": "Penalty Specialist",
        "desc_pt": "Reduz o tremor e a abertura do alvo nas cobranças de pênalti, permitindo bater bem nos cantos com mais precisão e menos risco de errar.",
        "desc_en": "Tightens the aiming circle and boosts composure on penalty kicks, making corner placements much safer.",
    },
    "GK Penalty Saver": {
        "cat": "gk_mental", "pt": "Pegador de pênaltis", "en": "GK Penalty Saver",
        "desc_pt": "Melhora a velocidade de reação e o alcance do goleiro ao defender pênaltis, alcançando cobranças mais próximas das traves.",
        "desc_en": "Improves goalkeeper reaction speed and diving reach during penalty kicks to parry corner shots.",
    },
    "Gamesmanship": {
        "cat": "gk_mental", "pt": "Malícia", "en": "Gamesmanship",
        "desc_pt": "Aumenta a chance de cavar faltas a favor do atleta em divididas apertadas, contatos corporais e chegadas atrasadas perto da área.",
        "desc_en": "Increases the likelihood of winning free kicks from tight body challenges and late tackles near the box.",
    },
    "Captaincy": {
        "cat": "gk_mental", "pt": "Liderança", "en": "Captaincy",
        "desc_pt": "Torna o jogador a inspiração do time em campo. Quando escolhido como capitão da partida, reduz os efeitos do cansaço em todos os companheiros.",
        "desc_en": "Makes the player a rallying point on the pitch. When selected as match captain, reduces fatigue effects across the entire team.",
    },
    "Super-Sub": {
        "cat": "gk_mental", "pt": "Super substituto", "en": "Super-Sub",
        "desc_pt": "Ao entrar vindo do banco no 2º tempo, sobe automaticamente 1 nível na seta de forma (ex: Amarela vira Verde, Verde vira Azul), elevando os atributos em campo.",
        "desc_en": "When subbed on in the second half, automatically boosts match form arrow by 1 tier (e.g. Yellow to Green, Green to Blue), lifting live stats.",
    },
    "Fighting Spirit": {
        "cat": "gk_mental", "pt": "Espírito guerreiro", "en": "Fighting Spirit",
        "desc_pt": "Evita quedas bruscas de precisão, controle e finalização quando o atleta está cansado, marcado sob pressão forte ou enfrentando adversidades.",
        "desc_en": "Prevents sharp drops in passing, control, and finishing when fatigued, heavily pressed, or facing adverse match conditions.",
    },
    # COM Playing Styles (AI) (7) - Exact official PES 2021 PT-BR (dt20_bra_x64.cpk) & EN names
    "Trickster": {
        "cat": "com_ai", "pt": "Malandro", "en": "Trickster",
        "desc_pt": "Quando controlado pela IA (COMP ou modo SIM), tenta fintas, pedaladas e dribles plásticos com alta frequência para superar a marcação.",
        "desc_en": "When AI-controlled (COM or SIM mode), frequently attempts flair feints and skill moves to beat defenders.",
    },
    "Mazing Run": {
        "cat": "com_ai", "pt": "Drible veloz", "en": "Mazing Run",
        "desc_pt": "Quando controlado pela IA, carrega a bola para dentro da defesa adversária costurando marcadores com giros curtos e mudanças rápidas de direção.",
        "desc_en": "When AI-controlled, drives deep into the defense by weaving past markers with sharp turns and close dribbles.",
    },
    "Speeding Bullet": {
        "cat": "com_ai", "pt": "Rápido como uma bala", "en": "Speeding Bullet",
        "desc_pt": "Quando controlado pela IA, prioriza arrancadas em velocidade máxima pelo corredor assim que encontra campo aberto.",
        "desc_en": "When AI-controlled, prioritizes full-speed sprints down the flank or into open space whenever a lane opens.",
    },
    "Incisive Run": {
        "cat": "com_ai", "pt": "Corrida com gás", "en": "Incisive Run",
        "desc_pt": "Quando controlado pela IA, corta em diagonal da ponta para o centro da área buscando o chute ou o passe decisivo.",
        "desc_en": "When AI-controlled, cuts diagonally inside from wide positions toward the box to shoot or create chances.",
    },
    "Long Ball Expert": {
        "cat": "com_ai", "pt": "Perito em bola longa", "en": "Long Ball Expert",
        "desc_pt": "Quando controlado pela IA, procura com frequência lançamentos longos e viradas de jogo nas costas da defesa adversária.",
        "desc_en": "When AI-controlled, regularly looks for long balls and switches of play behind the opposition defense.",
    },
    "Early Cross": {
        "cat": "com_ai", "pt": "Cruz. antecip.", "en": "Early Cross",
        "desc_pt": "Quando controlado pela IA, levanta a bola na área mais cedo de zonas intermediárias sem precisar ir até a linha de fundo.",
        "desc_en": "When AI-controlled, whips early crosses into the box from deep wide areas instead of running to the byline.",
    },
    "Long Ranger": {
        "cat": "com_ai", "pt": "Batedor Pro", "en": "Long Ranger",
        "desc_pt": "Quando controlado pela IA, arrisca finalizações de média e longa distância sempre que encontra uma brecha fora da área.",
        "desc_en": "When AI-controlled, frequently shoots from distance whenever space opens up outside the penalty area.",
    },
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
    meta = SKILL_META.get(desc, {})
    item = {
        "key": desc,
        "en": meta.get("en", desc),
        "pt": meta.get("pt", desc),
        "cat": meta.get("cat", "gk_mental"),
        "desc_pt": meta.get("desc_pt", ""),
        "desc_en": meta.get("desc_en", ""),
        "offset": off_int,
        "bit": int(bs),
    }
    if desc in PLAYER_SKILL_NAMES:
        PLAYER_SKILLS.append(item)
    elif desc in COM_AI_NAMES:
        COM_AI_STYLES.append(item)
