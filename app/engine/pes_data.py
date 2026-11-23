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

# Playing Styles (0..21) with PT-BR / EN names, active positions, and concise mechanics
PLAYING_STYLES: List[Dict[str, Any]] = [
    {"id": 0, "pt": "Nenhum", "en": "None", "pos_pt": "Todas", "pos_en": "All", "desc_pt": "Movimentação neutra sem comportamento tático pré-definido.", "desc_en": "Neutral movement with no preset tactical behavior."},
    {"id": 1, "pt": "Artilheiro", "en": "Goal Poacher", "pos_pt": "CA, SA", "pos_en": "CF, SS", "desc_pt": "Ataca o espaço nas costas do último zagueiro buscando bolas enfiadas.", "desc_en": "Plays on the shoulder of the last defender to burst in behind."},
    {"id": 2, "pt": "Isca", "en": "Dummy Runner", "pos_pt": "CA, SA, MAT", "pos_en": "CF, SS, AMF", "desc_pt": "Faz corridas para atrair a marcação e abrir espaço para os companheiros.", "desc_en": "Makes decoy runs to pull defenders away and open space for teammates."},
    {"id": 3, "pt": "Homem de Área", "en": "Fox in the Box", "pos_pt": "CA", "pos_en": "CF", "desc_pt": "Fica posicionado dentro da área esperando o passe ou o rebote para finalizar.", "desc_en": "Lurks inside the penalty box waiting for passes and rebounds to finish."},
    {"id": 4, "pt": "Ala Produtivo", "en": "Prolific Winger", "pos_pt": "PTE, PTD", "pos_en": "LWF, RWF", "desc_pt": "Abre bem na ponta para receber e corta em diagonal para o gol quando surge espaço.", "desc_en": "Stays wide to stretch the defense and cuts inside when space opens."},
    {"id": 5, "pt": "Clássico nº 10", "en": "Classic No. 10", "pos_pt": "SA, MAT, MLG", "pos_en": "SS, AMF, CMF", "desc_pt": "Armador clássico mais estático que organiza com passes curtos e toques refinados.", "desc_en": "Traditional static playmaker who dictates play through deft touches and passes."},
    {"id": 6, "pt": "Jogador de Infiltração", "en": "Hole Player", "pos_pt": "SA, MAT, MLE, MLD, MLG", "pos_en": "SS, AMF, LMF, RMF, CMF", "desc_pt": "Infiltra com frequência na área adversária vindo de trás durante o ataque.", "desc_en": "Makes frequent late runs from midfield into the opponent's penalty area."},
    {"id": 7, "pt": "Meia Versátil (Box-to-Box)", "en": "Box-to-Box", "pos_pt": "MAT, MLE, MLD, MLG, VOL", "pos_en": "AMF, LMF, RMF, CMF, DMF", "desc_pt": "Cobre o campo inteiro o jogo todo, apoiando tanto na defesa quanto no ataque.", "desc_en": "Covers the entire pitch nonstop, contributing to both defense and attack."},
    {"id": 8, "pt": "Primeiro Volante", "en": "Anchor Man", "pos_pt": "VOL", "pos_en": "DMF", "desc_pt": "Volante fixo à frente da zaga que protege o setor e raramente sobe ao ataque.", "desc_en": "Deep-sitting midfielder who shields the backline and rarely pushes forward."},
    {"id": 9, "pt": "Destruidor", "en": "The Destroyer", "pos_pt": "MLG, VOL, ZAG", "pos_en": "CMF, DMF, CB", "desc_pt": "Marca com pressão agressiva e desarmes firmes para cortar ataques adversários.", "desc_en": "Aggressively presses and tackles hard to break up opposition attacks."},
    {"id": 10, "pt": "Atacante Surpresa", "en": "Extra Frontman", "pos_pt": "ZAG", "pos_en": "CB", "desc_pt": "Zagueiro que avança ao ataque sempre que o time pressiona no campo adversário.", "desc_en": "Center back who steps out of defense to join sustained attacks."},
    {"id": 11, "pt": "Lateral Ofensivo", "en": "Offensive Full-back", "pos_pt": "LE, LD", "pos_en": "LB, RB", "desc_pt": "Sobe com frequência pela linha lateral para apoiar e cruzar na linha de fundo.", "desc_en": "Overlaps frequently down the touchline to support attacks and cross."},
    {"id": 12, "pt": "Lateral Defensivo", "en": "Defensive Full-back", "pos_pt": "LE, LD", "pos_en": "LB, RB", "desc_pt": "Prioriza a cobertura defensiva e evita avançar muito ao campo de ataque.", "desc_en": "Stays back to maintain defensive solidity instead of pushing high."},
    {"id": 13, "pt": "Pivô", "en": "Target Man", "pos_pt": "CA", "pos_en": "CF", "desc_pt": "Faz o paredão de costas para o gol para segurar a bola e acionar quem chega.", "desc_en": "Holds up the ball with his back to goal to bring teammates into play."},
    {"id": 14, "pt": "Armador Criativo", "en": "Creative Playmaker", "pos_pt": "SA, PTE, PTD, MAT, MLE, MLD", "pos_en": "SS, LWF, RWF, AMF, LMF, RMF", "desc_pt": "Recua entre linhas para buscar a bola, criar jogadas e dar assistências.", "desc_en": "Drops into pockets between the lines to create chances and assists."},
    {"id": 15, "pt": "Defensor Criativo", "en": "Build Up", "pos_pt": "ZAG", "pos_en": "CB", "desc_pt": "Recua na saída de bola para iniciar a construção desde a defesa com calma.", "desc_en": "Drops deeper to receive the ball and initiate build-up play from the back."},
    {"id": 16, "pt": "Goleiro Ofensivo", "en": "Offensive Goalkeeper", "pos_pt": "GL", "pos_en": "GK", "desc_pt": "Joga adiantado para interceptar lançamentos longos nas costas da zaga.", "desc_en": "Acts as a sweeper keeper, rushing out to clear balls behind the defense."},
    {"id": 17, "pt": "Goleiro Defensivo", "en": "Defensive Goalkeeper", "pos_pt": "GL", "pos_en": "GK", "desc_pt": "Permanece mais próximo da linha do gol priorizando defesas embaixo das traves.", "desc_en": "Stays closer to the goal line to focus on shot-stopping."},
    {"id": 18, "pt": "Ala Móvel", "en": "Roaming Flank", "pos_pt": "PTE, PTD, MLE, MLD", "pos_en": "LWF, RWF, LMF, RMF", "desc_pt": "Corta cedo da ponta para o meio para receber entre linhas e finalizar.", "desc_en": "Drifts inside early from the flank into central half-spaces to receive and shoot."},
    {"id": 19, "pt": "Especialista em Cruzamentos", "en": "Cross Specialist", "pos_pt": "PTE, PTD, MLE, MLD", "pos_en": "LWF, RWF, LMF, RMF", "desc_pt": "Fica colado na linha lateral esperando espaço para alçar bolas na área.", "desc_en": "Hugs the touchline waiting for opportunities to whip crosses into the box."},
    {"id": 20, "pt": "Orquestrador", "en": "Orchestrator", "pos_pt": "MLG, VOL", "pos_en": "CMF, DMF", "desc_pt": "Posiciona-se mais recuado no meio-campo para ditar o ritmo e distribuir passes.", "desc_en": "Sits deeper in midfield to control tempo and launch attacks with passing."},
    {"id": 21, "pt": "Lateral Atacante", "en": "Full-back Finisher", "pos_pt": "LE, LD", "pos_en": "LB, RB", "desc_pt": "Ataca por dentro entrando em zonas centrais como um meia ou elemento surpresa.", "desc_en": "Tucks inside into high central areas when attacking rather than staying wide."},
]

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
    {"id": 0, "pt": "Nenhum", "en": "None", "desc_pt": "Sem função especial na Master League.", "desc_en": "No special Master League squad role."},
    {"id": 1, "pt": "Jovem Promessa", "en": "Youth Prospect", "desc_pt": "Ganha bônus de XP individual em treinos e jogos para acelerar a evolução.", "desc_en": "Gains extra individual EXP in training and matches to develop faster."},
    {"id": 2, "pt": "Pupilo", "en": "Protégé", "desc_pt": "Recebe grande bônus de evolução quando atua ao lado de veteranos do clube.", "desc_en": "Receives a strong growth boost when learning alongside senior teammates."},
    {"id": 3, "pt": "Jogador de Equipe", "en": "Team Player", "desc_pt": "Papel equilibrado que ajuda a manter a harmonia do vestiário sem efeitos extremos.", "desc_en": "Balanced squad role that keeps locker room chemistry steady."},
    {"id": 4, "pt": "Armador", "en": "Playmaker", "desc_pt": "Aumenta o Espírito de Equipe e ajuda a desenvolver meias da mesma função.", "desc_en": "Boosts Team Spirit and supports training growth for fellow playmakers."},
    {"id": 5, "pt": "Estrela", "en": "Star Player", "desc_pt": "Atrai torcedores e aumenta a receita de bilheteria e produtos do clube.", "desc_en": "Boosts club fan base, matchday attendance, and merchandise revenue."},
    {"id": 6, "pt": "Líder", "en": "Leader", "desc_pt": "Eleva o Espírito de Equipe e reduz a queda de moral após derrotas.", "desc_en": "Raises Team Spirit and softens morale drops during poor runs."},
    {"id": 7, "pt": "General", "en": "General", "desc_pt": "Liderança forte no elenco com bônus médio de XP em treino para a mesma posição.", "desc_en": "Strong squad leadership with a medium training EXP boost for same-position peers."},
    {"id": 8, "pt": "Criador", "en": "Creator", "desc_pt": "Bônus de XP em treino para jogadores ofensivos e criativos do elenco.", "desc_en": "Grants a training EXP boost to attacking and creative teammates."},
    {"id": 9, "pt": "Maestro", "en": "Maestro", "desc_pt": "Referência técnica de elite com alto bônus de XP em treino para o setor.", "desc_en": "Elite technical mentor granting a strong training EXP boost to nearby positions."},
    {"id": 10, "pt": "Incansável", "en": "Workhorse", "desc_pt": "Ajuda a manter a intensidade e a consistência do elenco ao longo da temporada.", "desc_en": "Helps sustain squad intensity and consistency over a long season."},
    {"id": 11, "pt": "Jogador Inteligente", "en": "Smart Player", "desc_pt": "Concede bônus médio de XP fora de campo para atletas da mesma posição.", "desc_en": "Grants a medium off-match training EXP boost to same-position players."},
    {"id": 12, "pt": "Guerreiro", "en": "Fighter", "desc_pt": "Fortalece a competitividade do elenco e ajuda o desenvolvimento defensivo.", "desc_en": "Raises squad grit and supports defensive teammates' development."},
    {"id": 13, "pt": "Jogador Chave", "en": "Key Player", "desc_pt": "Grande bônus de XP para colegas da posição e aumento expressivo de receitas.", "desc_en": "Large training EXP boost for positional peers plus a major revenue lift."},
    {"id": 14, "pt": "Superestrela", "en": "Superstar", "desc_pt": "Ícone global que dispara vendas de camisas, público e atratividade do clube.", "desc_en": "Global icon that dramatically increases merchandise, fans, and club appeal."},
    {"id": 15, "pt": "Herói", "en": "Hero", "desc_pt": "Protege a moral em jogos decisivos e dá bônus de treino ao elenco.", "desc_en": "Protects squad morale in big matches and gives a training boost."},
    {"id": 16, "pt": "Virtuoso", "en": "Virtuoso", "desc_pt": "Acelera a evolução técnica e de drible de jovens atacantes do elenco.", "desc_en": "Accelerates technical and dribbling growth for younger attacking teammates."},
    {"id": 17, "pt": "Regente", "en": "Conductor", "desc_pt": "Aumenta a familiaridade tática do time e concede bônus de XP no meio-campo.", "desc_en": "Speeds up tactical familiarity and gives a training EXP boost in midfield."},
    {"id": 18, "pt": "Bandeira", "en": "Bandiera", "desc_pt": "Ídolo máximo do clube. Maximiza Espírito de Equipe, treino e facilita renovações.", "desc_en": "Ultimate club icon. Maximizes Team Spirit, training growth, and contract talks."},
    {"id": 19, "pt": "Lenda", "en": "Legend", "desc_pt": "Status supremo mundial. Multiplica receitas do clube e XP de treino dos jovens.", "desc_en": "Highest global status. Massively multiplies club income and youth training EXP."},
    {"id": 20, "pt": "Estrela em Ascensão", "en": "Rising Star", "desc_pt": "Jovem em destaque com evolução acelerada e ganho inicial de popularidade.", "desc_en": "Breakout youngster with accelerated progression and growing fan appeal."},
    {"id": 21, "pt": "Garoto Rebelde", "en": "Bad Boy", "desc_pt": "Perfil explosivo que gera mídia e intensidade, mas pode agitar o vestiário.", "desc_en": "Fiery personality that draws attention and edge, but can unsettle morale."},
    {"id": 22, "pt": "Ousado", "en": "Risk-Taker", "desc_pt": "Estilo arrojado que rende bônus médio de XP em treino para o grupo.", "desc_en": "Audacious profile that provides a medium training EXP boost to teammates."},
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
    {"fid": 14, "group": "attack", "pt": "Talento Ofensivo", "en": "Offensive Awareness", "desc_pt": "Velocidade de reação e inteligência para se desmarcar e atacar espaços no ataque.", "desc_en": "Reaction speed and intelligence when making runs into attacking spaces."},
    {"fid": 20, "group": "attack", "pt": "Finalização", "en": "Finishing", "desc_pt": "Precisão e frieza ao chutar a gol de dentro ou perto da área.", "desc_en": "Accuracy and composure when shooting on goal."},
    {"fid": 23, "group": "attack", "pt": "Cabeceio", "en": "Heading", "desc_pt": "Precisão e força nos cabeceios ofensivos e defensivos.", "desc_en": "Accuracy and power when heading the ball in attack and defense."},
    {"fid": 26, "group": "attack", "pt": "Bola Parada", "en": "Place Kicking", "desc_pt": "Precisão em cobranças de falta, escanteios e pênaltis.", "desc_en": "Accuracy on free kicks, corners, and penalties."},
    {"fid": 27, "group": "attack", "pt": "Curva", "en": "Curl", "desc_pt": "Quantidade de efeito lateral aplicado em chutes colocados, faltas e cruzamentos.", "desc_en": "Amount of bend applied to finesse shots, free kicks, and crosses."},
    # Technique
    {"fid": 18, "group": "tech", "pt": "Controle de Bola", "en": "Ball Control", "desc_pt": "Qualidade do domínio ao receber passes fortes, altos ou difíceis.", "desc_en": "First-touch quality when trapping fast or difficult passes."},
    {"fid": 17, "group": "tech", "pt": "Drible", "en": "Dribbling", "desc_pt": "Controle e velocidade ao conduzir a bola em arrancada.", "desc_en": "Control and speed when carrying the ball at pace."},
    {"fid": 19, "group": "tech", "pt": "Condução Firme", "en": "Tight Possession", "desc_pt": "Agilidade para girar e proteger a bola em espaços curtos sob marcação.", "desc_en": "Agility when turning and shielding the ball in tight spaces."},
    {"fid": 21, "group": "tech", "pt": "Passe Rasteiro", "en": "Low Pass", "desc_pt": "Precisão de passes curtos, tabelas e enfiadas rasteiras.", "desc_en": "Accuracy of short ground passes and low through balls."},
    {"fid": 22, "group": "tech", "pt": "Passe Alto", "en": "Lofted Pass", "desc_pt": "Precisão em lançamentos longos, inversões de jogo e cruzamentos.", "desc_en": "Accuracy of long balls, switches of play, and crosses."},
    # Physical
    {"fid": 33, "group": "phys", "pt": "Velocidade", "en": "Speed", "desc_pt": "Velocidade máxima de corrida em campo aberto.", "desc_en": "Top sprinting speed over open ground."},
    {"fid": 37, "group": "phys", "pt": "Aceleração", "en": "Acceleration", "desc_pt": "Explosão nos primeiros metros para atingir a velocidade máxima.", "desc_en": "Burst over the first few yards to reach top speed."},
    {"fid": 36, "group": "phys", "pt": "Força do Chute", "en": "Kicking Power", "desc_pt": "Potência e velocidade da bola em chutes e lançamentos fortes.", "desc_en": "Shot velocity and power on strikes and driven passes."},
    {"fid": 38, "group": "phys", "pt": "Impulsão", "en": "Jump", "desc_pt": "Altura alcançada nos saltos para disputas aéreas.", "desc_en": "Vertical leap height in aerial duels."},
    {"fid": 34, "group": "phys", "pt": "Contato Físico", "en": "Physical Contact", "desc_pt": "Força corporal para vencer divididas e proteger a bola no corpo a corpo.", "desc_en": "Body strength to win shoulder-to-shoulder duels and shield the ball."},
    {"fid": 35, "group": "phys", "pt": "Equilíbrio", "en": "Balance", "desc_pt": "Capacidade de manter a postura e evitar tropeços após contatos físicos.", "desc_en": "Ability to stay on his feet and keep control after physical challenges."},
    {"fid": 39, "group": "phys", "pt": "Resistência", "en": "Stamina", "desc_pt": "Duração do fôlego durante a partida antes de perder desempenho.", "desc_en": "How long the player sustains energy before fatigue reduces performance."},
    # Defense
    {"fid": 15, "group": "def", "pt": "Talento Defensivo", "en": "Defensive Awareness", "desc_pt": "Posicionamento e leitura de jogo para fechar linhas de passe e acompanhar atacantes.", "desc_en": "Positioning and reading of the game to cut passing lanes and track runners."},
    {"fid": 24, "group": "def", "pt": "Desarme", "en": "Ball Winning", "desc_pt": "Eficiência ao roubar a bola em botes e carrinhos sem cometer falta.", "desc_en": "Effectiveness at winning the ball cleanly in standing and sliding tackles."},
    {"fid": 25, "group": "def", "pt": "Agressividade", "en": "Aggression", "desc_pt": "Intensidade com que o jogador pressiona e vai para cima do portador da bola.", "desc_en": "Intensity when pressing and challenging the ball carrier."},
    # Goalkeeper
    {"fid": 16, "group": "gk", "pt": "Talento de Goleiro", "en": "GK Awareness", "desc_pt": "Posicionamento no gol e velocidade de reação a desvios e jogadas rápidas.", "desc_en": "Goalkeeping positioning and reaction speed to threats."},
    {"fid": 28, "group": "gk", "pt": "Firmeza (GL)", "en": "GK Catching", "desc_pt": "Segurança para encaixar chutes fortes sem dar rebote.", "desc_en": "Ability to catch powerful shots cleanly without spilling rebounds."},
    {"fid": 29, "group": "gk", "pt": "Afastamento (GL)", "en": "GK Clearing", "desc_pt": "Precisão ao espalmar a bola para zonas seguras longe dos atacantes.", "desc_en": "Ability to parry or punch shots safely away from danger."},
    {"fid": 30, "group": "gk", "pt": "Reflexos (GL)", "en": "GK Reflexes", "desc_pt": "Velocidade de resposta em finalizações à queima-roupa.", "desc_en": "Reaction speed against close-range strikes."},
    {"fid": 31, "group": "gk", "pt": "Alcance (GL)", "en": "GK Reach", "desc_pt": "Cobertura de área do gol em defesas difíceis nos cantos.", "desc_en": "Coverage across the goal frame when diving for corner shots."},
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
    # Dribbling & Ball Control (9)
    "Scissors Feint": {
        "cat": "dribble", "pt": "Pedalada Simples", "en": "Scissors Feint",
        "desc_pt": "Executa pedaladas em alta velocidade para desequilibrar o marcador.",
        "desc_en": "Executes the Scissors Feint at high speed to unbalance defenders.",
    },
    "Double Touch": {
        "cat": "dribble", "pt": "Toque Duplo", "en": "Double Touch",
        "desc_pt": "Realiza o drible de dois toques (La Croqueta) com mais rapidez e precisão.",
        "desc_en": "Performs the quick Double Touch (La Croqueta) cleanly past markers.",
    },
    "Flip Flap": {
        "cat": "dribble", "pt": "Elástico", "en": "Flip Flap",
        "desc_pt": "Libera o drible elástico rápido para cortar o defensor em curto espaço.",
        "desc_en": "Unlocks fast Flip Flap (Elastico) feints to beat defenders in tight space.",
    },
    "Marseille Turn": {
        "cat": "dribble", "pt": "Giro de 360°", "en": "Marseille Turn",
        "desc_pt": "Executa a roleta de 360° com fluidez para escapar da marcação.",
        "desc_en": "Performs the 360-degree Marseille Turn smoothly to evade pressure.",
    },
    "Sombrero": {
        "cat": "dribble", "pt": "Chapéu", "en": "Sombrero",
        "desc_pt": "Permite dar chapéus e executar o giro com bola no alto sobre o adversário.",
        "desc_en": "Enables Sombrero flicks, Boomerang Trap, and Crescent Turn moves.",
    },
    "Cross Over Turn": {
        "cat": "dribble", "pt": "Corte de Calcanhar", "en": "Cross Over Turn",
        "desc_pt": "Faz o corte rápido passando a bola por trás da perna de apoio.",
        "desc_en": "Quickly executes the Cross Over Turn to change direction sharply.",
    },
    "Cut Behind & Turn": {
        "cat": "dribble", "pt": "Corte por Trás", "en": "Cut Behind & Turn",
        "desc_pt": "Puxa a bola por trás do corpo em movimento para inverter a direção.",
        "desc_en": "Pulls the ball behind the standing leg to cut back cleanly.",
    },
    "Scotch Move": {
        "cat": "dribble", "pt": "Finta de Corpo", "en": "Scotch Move",
        "desc_pt": "Finta rápida levando a bola para o lado oposto com o pé dominante.",
        "desc_en": "Executes the quick Scotch Move feint to open a lane past the defender.",
    },
    "Step On Skill control": {
        "cat": "dribble", "pt": "Controle de Sola", "en": "Step On Skill Control",
        "desc_pt": "Usa a sola do pé com mais agilidade em giros, domínios e fintas curtas.",
        "desc_en": "Uses the sole of the foot for sharper turns, traps, and close-space feints.",
    },
    # Shooting & Finishing (10)
    "Heading": {
        "cat": "shoot", "pt": "Cabeceio", "en": "Heading",
        "desc_pt": "Aumenta a precisão das cabeçadas e a frequência de cabeceios para o chão.",
        "desc_en": "Improves header accuracy and triggers downward headers more often.",
    },
    "Long Range Drive": {
        "cat": "shoot", "pt": "Chute com Curva", "en": "Long Range Drive",
        "desc_pt": "Melhora muito a precisão e a curva dos chutes colocados de fora da área.",
        "desc_en": "Greatly improves accuracy and bend on curling finesse shots from distance.",
    },
    "Chip shot control": {
        "cat": "shoot", "pt": "Cavadinha", "en": "Chip Shot Control",
        "desc_pt": "Torna as finalizações por cobertura mais precisas e controladas.",
        "desc_en": "Makes lofted chip shots over the goalkeeper much more accurate.",
    },
    "Long Range Shooting": {
        "cat": "shoot", "pt": "Chute de Longa Dist.", "en": "Long Range Shooting",
        "desc_pt": "Aumenta a precisão geral dos chutes fortes de longa distância.",
        "desc_en": "Increases overall shot accuracy when striking from long range.",
    },
    "Knuckle Shot": {
        "cat": "shoot", "pt": "Folha Seca", "en": "Knuckle Shot",
        "desc_pt": "Facilita chutes e faltas sem rotação com trajetória imprevisível no ar.",
        "desc_en": "Makes erratic, dipping knuckle shots easier in open play and free kicks.",
    },
    "Dipping Shot": {
        "cat": "shoot", "pt": "Chute com Queda", "en": "Dipping Shot",
        "desc_pt": "Libera chutes com efeito para baixo que caem rápido e quicam na frente do goleiro.",
        "desc_en": "Enables topspin strikes that dip sharply and bounce awkwardly before the keeper.",
    },
    "Rising Shots": {
        "cat": "shoot", "pt": "Chute Ascendente", "en": "Rising Shots",
        "desc_pt": "Libera bombas rasantes que ganham altura rapidamente rumo ao ângulo.",
        "desc_en": "Enables low-trajectory strikes that rise sharply toward the top corner.",
    },
    "Acrobatic Finishing": {
        "cat": "shoot", "pt": "Finaliz. Acrobática", "en": "Acrobatic Finishing",
        "desc_pt": "Permite finalizar com voleios, bicicletas e chutes desequilibrados com eficiência.",
        "desc_en": "Enables effective volleys, overhead kicks, and off-balance finishes.",
    },
    "Heel Trick": {
        "cat": "shoot", "pt": "Toque de Calcanhar", "en": "Heel Trick",
        "desc_pt": "Permite passes e finalizações de calcanhar mesmo fora de posição ideal.",
        "desc_en": "Allows accurate backheel passes and shots even from awkward angles.",
    },
    "First-time Shot": {
        "cat": "shoot", "pt": "Chute de Primeira", "en": "First-time Shot",
        "desc_pt": "Aumenta a precisão e a velocidade de resposta ao finalizar sem dominar.",
        "desc_en": "Boosts accuracy and responsiveness when shooting first-time or on the turn.",
    },
    # Passing & Crossing (8)
    "One-touch Pass": {
        "cat": "pass", "pt": "Passe de Primeira", "en": "One-touch Pass",
        "desc_pt": "Deixa os passes de primeira muito mais rápidos e precisos sob pressão.",
        "desc_en": "Improves accuracy and tempo on instant one-touch passes under pressure.",
    },
    "Through Passing": {
        "cat": "pass", "pt": "Passe em Profundidade", "en": "Through Passing",
        "desc_pt": "Aumenta a precisão dos passes enfiados no ponto futuro para atacantes.",
        "desc_en": "Increases the accuracy and timing of through balls into space.",
    },
    "Weighted Pass": {
        "cat": "pass", "pt": "Passe Medido", "en": "Weighted Pass",
        "desc_pt": "Aplica efeito de recuo em lançamentos e enfiadas altas, facilitando o domínio.",
        "desc_en": "Adds backspin to lofted passes and through balls so teammates control them easily.",
    },
    "Pinpoint Crossing": {
        "cat": "pass", "pt": "Cruzamento Preciso", "en": "Pinpoint Crossing",
        "desc_pt": "Melhora a curva e a exatidão dos cruzamentos quando há espaço pelos lados.",
        "desc_en": "Delivers sharper, more accurate curling crosses from wide areas.",
    },
    "Outside Curler": {
        "cat": "pass", "pt": "Trivela", "en": "Outside Curler",
        "desc_pt": "Usa a parte externa do pé bom para passar e chutar em vez de usar o pé ruim.",
        "desc_en": "Uses the outside of the stronger foot for passes and shots from tight angles.",
    },
    "Rabona": {
        "cat": "pass", "pt": "Letra (Rabona)", "en": "Rabona",
        "desc_pt": "Executa passes e chutes de letra para acionar o pé dominante em ângulos fechados.",
        "desc_en": "Executes Rabona kicks to pass or shoot with the stronger foot from awkward angles.",
    },
    "No Look Pass": {
        "cat": "pass", "pt": "Passe sem Olhar", "en": "No Look Pass",
        "desc_pt": "Engana a marcação olhando para um lado e tocando na direção oposta.",
        "desc_en": "Deceives defenders by passing in a different direction than where he is looking.",
    },
    "Low Lofted Pass": {
        "cat": "pass", "pt": "Passe Alto Rasteiro", "en": "Low Lofted Pass",
        "desc_pt": "Faz lançamentos longos com trajetória mais baixa, rápida e precisa.",
        "desc_en": "Hits long lofted passes on a flatter, faster, and more accurate trajectory.",
    },
    # Defense & Physical (5)
    "Man Marking": {
        "cat": "def", "pt": "Marcação Individual", "en": "Man Marking",
        "desc_pt": "Gruda no atacante adversário e acompanha sua movimentação de perto.",
        "desc_en": "Sticks tight to opposing attackers and tracks their runs closely.",
    },
    "Track Back": {
        "cat": "def", "pt": "Voltar para Marcar", "en": "Track Back",
        "desc_pt": "Faz jogadores ofensivos pressionarem ativamente a saída de bola adversária.",
        "desc_en": "Makes attacking players actively press and chase down opponents on defense.",
    },
    "Interception": {
        "cat": "def", "pt": "Interceptação", "en": "Interception",
        "desc_pt": "Aumenta o alcance e o reflexo para cortar passes e bolas divididas.",
        "desc_en": "Improves positioning and reach to intercept passes and loose balls.",
    },
    "Acrobatic Clear": {
        "cat": "def", "pt": "Afast. Acrobático", "en": "Acrobatic Clear",
        "desc_pt": "Permite afastar bolas perigosas com chutes acrobáticos mesmo desequilibrado.",
        "desc_en": "Allows desperate acrobatic clearances from difficult or off-balance positions.",
    },
    "Long Throw": {
        "cat": "def", "pt": "Lateral Longo", "en": "Long Throw",
        "desc_pt": "Aumenta consideravelmente a distância alcançada nas cobranças de lateral.",
        "desc_en": "Significantly extends throw-in distance to reach directly into the box.",
    },
    # Goalkeeper, Set Pieces & Mental (9)
    "GK Low Punt": {
        "cat": "gk_mental", "pt": "Reposição Baixa (GL)", "en": "GK Low Punt",
        "desc_pt": "Chutão rasteiro e preciso do goleiro para armar contra-ataques rápidos.",
        "desc_en": "Enables flat, accurate goalkeeper punts to launch fast counterattacks.",
    },
    "GK High Punt": {
        "cat": "gk_mental", "pt": "Reposição Alta (GL)", "en": "GK High Punt",
        "desc_pt": "Reposição longa e alta do goleiro levando a bola fundo no campo adversário.",
        "desc_en": "Enables deep, high-arcing punts straight into opposition territory.",
    },
    "GK Long Throw": {
        "cat": "gk_mental", "pt": "Lançamento Longo (GL)", "en": "GK Long Throw",
        "desc_pt": "Aumenta bastante o alcance da reposição com as mãos pelo goleiro.",
        "desc_en": "Extends the range of accurate goalkeeper throws to start counters.",
    },
    "Penalty Specialist": {
        "cat": "gk_mental", "pt": "Especialista Pênalti", "en": "Penalty Specialist",
        "desc_pt": "Aumenta a precisão e reduz a variação da mira nas cobranças de pênalti.",
        "desc_en": "Increases accuracy and composure when taking penalty kicks.",
    },
    "GK Penalty Saver": {
        "cat": "gk_mental", "pt": "Pegador de Pênaltis", "en": "GK Penalty Saver",
        "desc_pt": "Melhora os reflexos e o alcance do goleiro em cobranças de pênalti.",
        "desc_en": "Enhances goalkeeper reactions and reach when facing penalty kicks.",
    },
    "Gamesmanship": {
        "cat": "gk_mental", "pt": "Malícia", "en": "Gamesmanship",
        "desc_pt": "Aumenta a chance de sofrer faltas em contatos apertados perto da área.",
        "desc_en": "Increases the ability to draw free kicks in tight challenges.",
    },
    "Captaincy": {
        "cat": "gk_mental", "pt": "Liderança", "en": "Captaincy",
        "desc_pt": "Quando capitão, reduz os efeitos da fadiga em todos os companheiros de time.",
        "desc_en": "When set as captain, reduces the impact of fatigue across the entire team.",
    },
    "Super-Sub": {
        "cat": "gk_mental", "pt": "Super Substituto", "en": "Super-Sub",
        "desc_pt": "Recebe bônus real de desempenho e forma ao entrar vindo do banco no 2º tempo.",
        "desc_en": "Receives a noticeable performance and form boost when subbed on after halftime.",
    },
    "Fighting Spirit": {
        "cat": "gk_mental", "pt": "Espírito Guerreiro", "en": "Fighting Spirit",
        "desc_pt": "Mantém o desempenho alto sob pressão intensa, cansaço ou jogos adversos.",
        "desc_en": "Sustains high performance under heavy pressure, fatigue, and adverse situations.",
    },
    # COM Playing Styles (AI) (7)
    "Trickster": {
        "cat": "com_ai", "pt": "Mágico", "en": "Trickster",
        "desc_pt": "Quando controlado pela IA, usa fintas e dribles plásticos com frequência.",
        "desc_en": "When AI-controlled, frequently uses flair skill moves to beat markers.",
    },
    "Mazing Run": {
        "cat": "com_ai", "pt": "Serpenteante", "en": "Mazing Run",
        "desc_pt": "Quando controlado pela IA, avança fundo na defesa com mudanças curtas de direção.",
        "desc_en": "When AI-controlled, drives deep into defense using sharp turns and dribbles.",
    },
    "Speeding Bullet": {
        "cat": "com_ai", "pt": "Bala", "en": "Speeding Bullet",
        "desc_pt": "Quando controlado pela IA, acelera em velocidade máxima buscando profundidade.",
        "desc_en": "When AI-controlled, sprints aggressively forward to exploit space with pace.",
    },
    "Incisive Run": {
        "cat": "com_ai", "pt": "Arrancada Incisiva", "en": "Incisive Run",
        "desc_pt": "Quando controlado pela IA, corta da ponta para dentro buscando finalização.",
        "desc_en": "When AI-controlled, cuts inside from the wings looking for goal chances.",
    },
    "Long Ball Expert": {
        "cat": "com_ai", "pt": "Mestre Bola Longa", "en": "Long Ball Expert",
        "desc_pt": "Quando controlado pela IA, aciona lançamentos longos e viradas com frequência.",
        "desc_en": "When AI-controlled, frequently plays long passes and switches of play.",
    },
    "Early Cross": {
        "cat": "com_ai", "pt": "Cruzamento Antecipado", "en": "Early Cross",
        "desc_pt": "Quando controlado pela IA, cruza cedo para a área assim que vê espaço.",
        "desc_en": "When AI-controlled, hits early crosses into the box whenever space opens.",
    },
    "Long Ranger": {
        "cat": "com_ai", "pt": "Chutador de Longe", "en": "Long Ranger",
        "desc_pt": "Quando controlado pela IA, arrisca finalizações de fora da área com frequência.",
        "desc_en": "When AI-controlled, regularly takes shots on goal from outside the box.",
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
