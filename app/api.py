"""
pywebview JS <-> Python API Bridge for PES 2021 Ultimate Studio.
Every public method on StudioAPI is directly callable from JavaScript via window.pywebview.api.<method>().
"""

from typing import Any, Dict, Optional
from engine.pes_engine import PESEngine
from engine.pes_data import (
    POSITIONS,
    TEAM_ROLES,
    STAT_DEFS,
    PLAYER_SKILLS,
    COM_AI_STYLES,
    NATIONALITIES,
    PLAYING_STYLES,
    SCOUT_LISTS,
)


class StudioAPI:
    def __init__(self, engine: PESEngine) -> None:
        self.engine = engine

    def get_metadata(self) -> Dict[str, Any]:
        return {
            "version": "12.0 Ultimate Studio",
            "positions": POSITIONS,
            "team_roles": TEAM_ROLES,
            "stats": STAT_DEFS,
            "skills": PLAYER_SKILLS,
            "com_ai": COM_AI_STYLES,
            "nationalities": NATIONALITIES,
            "playing_styles": PLAYING_STYLES,
            "scout_lists": [
                {"slot": s["slot"], "pt": s["pt"], "en": s["en"], "cap": s["cap"]}
                for s in SCOUT_LISTS
            ],
        }

    # Live status & connection
    def get_status(self) -> Dict[str, Any]:
        return self.engine.get_status()

    def try_attach(self) -> Dict[str, Any]:
        ok = self.engine.ensure_attached()
        st = self.engine.get_status()
        st["attach_ok"] = ok
        return st

    # Player Studio
    def select_player(self, addr: int) -> Dict[str, Any]:
        ok = self.engine.select_player(int(addr))
        if not ok:
            return {"ok": False}
        return self.engine.get_player_details(int(addr))

    def get_player_details(self, addr: Optional[int] = None) -> Dict[str, Any]:
        return self.engine.get_player_details(int(addr) if addr else None)

    def set_quick_condition(
        self, stamina: Optional[int] = None, arrow: Optional[int] = None, heal: bool = False
    ) -> Dict[str, Any]:
        return self.engine.set_player_quick_condition(stamina=stamina, arrow=arrow, heal=bool(heal))

    def update_player_bio(self, payload: Dict[str, Any]) -> Dict[str, Any]:
        return self.engine.update_player_bio(payload)

    def set_player_stat(self, fid: int, val: int) -> Dict[str, Any]:
        return self.engine.set_player_stat(int(fid), int(val))

    def set_player_stats_batch(self, stats_map: Dict[str, int]) -> Dict[str, Any]:
        return self.engine.set_player_stats_batch(stats_map)

    def set_player_skill(self, skill_key: str, enabled: bool) -> Dict[str, Any]:
        return self.engine.set_player_skill(str(skill_key), bool(enabled))

    def set_playing_style(self, style_id: int) -> Dict[str, Any]:
        return self.engine.set_playing_style(int(style_id))

    def set_team_role(self, role_id: int, level: int) -> Dict[str, Any]:
        return self.engine.set_team_role(int(role_id), int(level))

    # Position Lab
    def set_position_aptitude(self, pos_idx: int, grade: int) -> Dict[str, Any]:
        return self.engine.set_position_aptitude(int(pos_idx), int(grade))

    def set_registered_position(self, pos_idx: int) -> Dict[str, Any]:
        return self.engine.set_registered_position(int(pos_idx))

    def step_position_ovr(self, pos_idx: int, delta: int) -> Dict[str, Any]:
        return self.engine.step_position_ovr(int(pos_idx), int(delta))

    def match_position_to_primary(self, pos_idx: int) -> Dict[str, Any]:
        return self.engine.match_position_to_primary(int(pos_idx))

    def undo_last(self) -> Dict[str, Any]:
        return self.engine.undo_last()

    # Squad
    def get_squad_roster(self) -> Dict[str, Any]:
        return self.engine.get_squad_roster()

    def run_squad_commander(self, mode: str) -> Dict[str, Any]:
        return self.engine.run_squad_commander(str(mode))

    # Search 30K+
    def search_players(
        self,
        query: str = "",
        pos_filter: int = -1,
        min_ovr: int = 0,
        max_age: int = 99,
        free_only: bool = False,
    ) -> Dict[str, Any]:
        return self.engine.search_players(
            query=str(query or ""),
            pos_filter=int(pos_filter),
            min_ovr=int(min_ovr),
            max_age=int(max_age),
            free_only=bool(free_only),
        )

    # Scouts & Shortlist
    def get_scout_lists(self) -> Dict[str, Any]:
        return self.engine.get_scout_lists()

    def add_to_scout_list(self, slot_num: int, player_addr: Optional[int] = None) -> Dict[str, Any]:
        return self.engine.add_to_scout_list(int(slot_num), int(player_addr) if player_addr else None)

    def remove_from_scout_list(self, slot_num: int, handle_str: str) -> Dict[str, Any]:
        return self.engine.remove_from_scout_list(int(slot_num), str(handle_str))

    # Finances
    def get_finances(self) -> Dict[str, Any]:
        return self.engine.get_finances()

    def set_finances(self, transfer_str: str, salary_avail_str: str) -> Dict[str, Any]:
        return self.engine.set_finances(transfer_str, salary_avail_str)

    def add_finance_delta(self, kind: str, raw_delta: int) -> Dict[str, Any]:
        return self.engine.add_finance_delta(str(kind), int(raw_delta))

    # Training, Match, BAL
    def set_training_mode(self, mode: int) -> Dict[str, Any]:
        return self.engine.set_training_mode(int(mode))

    def set_stamina_mode(self, mode: int) -> Dict[str, Any]:
        return self.engine.set_stamina_mode(int(mode))

    def set_bal_focus(self, points: int, lock_enabled: bool) -> Dict[str, Any]:
        return self.engine.set_bal_focus(int(points), bool(lock_enabled))

    def set_freeze_clock(self, freeze: bool) -> Dict[str, Any]:
        return self.engine.set_freeze_clock(bool(freeze))

    # JSON Backup
    def export_player_json(self) -> Dict[str, Any]:
        return self.engine.export_player_json()

    def import_player_json(self, json_str: str) -> Dict[str, Any]:
        return self.engine.import_player_json(str(json_str))
