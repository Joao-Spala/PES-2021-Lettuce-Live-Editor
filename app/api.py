"""
pywebview JS <-> Python API Bridge for PES 2021 Ultimate Studio.
All inputs are safely coerced so malformed JS arguments never raise uncaught exceptions.
"""

from typing import Any, Dict, Optional
from engine.pes_engine import PESEngine, _safe_int
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

    def get_status(self) -> Dict[str, Any]:
        return self.engine.get_status()

    def try_attach(self) -> Dict[str, Any]:
        ok = self.engine.ensure_attached()
        st = self.engine.get_status()
        st["attach_ok"] = ok
        return st

    def select_player(self, addr: Any) -> Dict[str, Any]:
        a = _safe_int(addr, 0) or 0
        if not self.engine.select_player(a):
            return {"ok": False, "code": "no_player"}
        return self.engine.get_player_details(a)

    def get_player_details(self, addr: Any = None) -> Dict[str, Any]:
        return self.engine.get_player_details(_safe_int(addr))

    def set_quick_condition(
        self, stamina: Any = None, arrow: Any = None, heal: Any = False, addr: Any = None
    ) -> Dict[str, Any]:
        return self.engine.set_player_quick_condition(
            stamina=_safe_int(stamina),
            arrow=_safe_int(arrow),
            heal=bool(heal),
            addr=_safe_int(addr),
        )

    def step_player_ovr(self, delta: Any) -> Dict[str, Any]:
        return self.engine.step_player_ovr(_safe_int(delta, 0) or 0)

    def update_player_bio(self, payload: Any) -> Dict[str, Any]:
        return self.engine.update_player_bio(payload if isinstance(payload, dict) else {})

    def set_player_stat(self, fid: Any, val: Any) -> Dict[str, Any]:
        return self.engine.set_player_stat(_safe_int(fid, -1) or -1, _safe_int(val, 40) or 40)

    def set_player_stats_batch(self, stats_map: Any) -> Dict[str, Any]:
        return self.engine.set_player_stats_batch(stats_map if isinstance(stats_map, dict) else {})

    def set_player_skill(self, skill_key: Any, enabled: Any) -> Dict[str, Any]:
        return self.engine.set_player_skill(str(skill_key or ""), bool(enabled))

    def set_playing_style(self, style_id: Any) -> Dict[str, Any]:
        return self.engine.set_playing_style(_safe_int(style_id, 0) or 0)

    def set_team_role(self, role_id: Any, level: Any) -> Dict[str, Any]:
        return self.engine.set_team_role(_safe_int(role_id, 0) or 0, _safe_int(level, 0) or 0)

    def set_position_aptitude(self, pos_idx: Any, grade: Any) -> Dict[str, Any]:
        return self.engine.set_position_aptitude(_safe_int(pos_idx, -1), _safe_int(grade, -1))

    def set_registered_position(self, pos_idx: Any) -> Dict[str, Any]:
        return self.engine.set_registered_position(_safe_int(pos_idx, -1))

    def step_position_ovr(self, pos_idx: Any, delta: Any) -> Dict[str, Any]:
        return self.engine.step_position_ovr(_safe_int(pos_idx, -1), _safe_int(delta, 0) or 0)

    def match_position_to_primary(self, pos_idx: Any) -> Dict[str, Any]:
        return self.engine.match_position_to_primary(_safe_int(pos_idx, -1))

    def undo_last(self) -> Dict[str, Any]:
        return self.engine.undo_last()

    def get_squad_roster(self) -> Dict[str, Any]:
        return self.engine.get_squad_roster()

    def run_squad_commander(self, mode: Any) -> Dict[str, Any]:
        return self.engine.run_squad_commander(str(mode or ""))

    def search_players(
        self,
        query: Any = "",
        pos_filter: Any = -1,
        min_ovr: Any = 0,
        max_age: Any = 99,
        free_only: Any = False,
    ) -> Dict[str, Any]:
        return self.engine.search_players(
            query=str(query or ""),
            pos_filter=_safe_int(pos_filter, -1) if _safe_int(pos_filter) is not None else -1,
            min_ovr=_safe_int(min_ovr, 0) or 0,
            max_age=_safe_int(max_age, 99) or 99,
            free_only=bool(free_only),
        )

    def get_scout_lists(self) -> Dict[str, Any]:
        return self.engine.get_scout_lists()

    def add_to_scout_list(self, slot_num: Any, player_addr: Any = None) -> Dict[str, Any]:
        return self.engine.add_to_scout_list(_safe_int(slot_num, 4) or 4, _safe_int(player_addr))

    def remove_from_scout_list(self, slot_num: Any, handle_str: Any) -> Dict[str, Any]:
        return self.engine.remove_from_scout_list(_safe_int(slot_num, 4) or 4, str(handle_str or ""))

    def clear_scout_list(self, slot_num: Any) -> Dict[str, Any]:
        return self.engine.clear_scout_list(_safe_int(slot_num, 4) or 4)

    def get_finances(self) -> Dict[str, Any]:
        return self.engine.get_finances()

    def set_finances(self, transfer_str: Any, salary_avail_str: Any) -> Dict[str, Any]:
        return self.engine.set_finances(str(transfer_str or ""), str(salary_avail_str or ""))

    def add_finance_delta(self, kind: Any, raw_delta: Any) -> Dict[str, Any]:
        return self.engine.add_finance_delta(str(kind or "transfer"), _safe_int(raw_delta, 0) or 0)

    def set_training_mode(self, mode: Any) -> Dict[str, Any]:
        return self.engine.set_training_mode(_safe_int(mode, 0) or 0)

    def set_stamina_mode(self, mode: Any) -> Dict[str, Any]:
        return self.engine.set_stamina_mode(_safe_int(mode, 0) or 0)

    def set_bal_focus(self, points: Any, lock_enabled: Any) -> Dict[str, Any]:
        return self.engine.set_bal_focus(_safe_int(points, 21) or 0, bool(lock_enabled))

    def set_freeze_clock(self, freeze: Any) -> Dict[str, Any]:
        return self.engine.set_freeze_clock(bool(freeze))

    def export_player_json(self) -> Dict[str, Any]:
        return self.engine.export_player_json()

    def import_player_json(self, json_str: Any) -> Dict[str, Any]:
        return self.engine.import_player_json(str(json_str or ""))
