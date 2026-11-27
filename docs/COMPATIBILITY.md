# Compatibility

English | [Português](COMPATIBILITY.pt-BR.md)

How the app finds things inside the game, what has been verified, and how to check or add a build.

## Principles

1. **No fixed addresses.** Every hook site and global pointer is found by a byte signature (with wildcards) searched in the executable sections of the running process. The same code works on any executable built from the PES 2021 engine.
2. **Unique or refuse.** A signature must match exactly once. Zero or several matches turn the dependent feature off and the connection line at the top of each view shows a warning. The app never guesses an address.
3. **Read again before writing.** The bytes at a site are compared with the signature right before the patch, so a stale result can never be written over something that changed.
4. **Layout guards.** The two pointer signatures contain the offset of a career structure field (`E0 CB 6E 01`). If the structure moves, they stop matching and the app refuses to edit.
5. **Everything a hook assumes is inside its signature.** The stamina hook rebuilds the function epilogue by hand, so its signature runs up to the `RET`.
6. **Safety net.** Each hook is recorded in a journal inside the helper area the app allocates in the game. If the app dies, the next instance reverts the jumps and frees the area, but only when nothing still jumps into it and only when the owner process (PID and creation time) is gone. A live instance is never touched, even if its file has another name.

## Signatures

All eleven are unique in the executable sections of both builds below.

| Id | Used by | PES 2021 RVA | FL_2027 RVA |
| --- | --- | --- | --- |
| `player_capture_1` | Highlighted player capture (4 points) | `0xC6ED2D` | `0xC6D75D` |
| `player_capture_2` | | `0x8E24D7` | `0x8E1E47` |
| `player_capture_3` | | `0xDDCA5C` | `0xDDA8DC` |
| `player_capture_4` | | `0xF27462` | `0xF1B6B2` |
| `training_xp` | Training XP multiplier | `0x12E14A0` | `0x12DB3A0` |
| `bal_focus_cap` | BAL focus points cap | `0xBBFE71` | `0xBB35C1` |
| `match_hud` | Match HUD pointers | `0x20F9FD6` | `0x20F1786` |
| `match_clock` | Match clock | `0xA3C787` | `0xA3C157` |
| `match_stamina` | Selective stamina in match | `0xA58EAC` | `0xA587EC` |
| `career_owner` | Career pointer (via the getter call) | `0xBBFEF3` | `0xBB3643` |
| `match_manager` | Match manager pointer | `0x4242AE` | `0x423A0E` |

The career global is at RVA `0x3714240` in PES 2021 and `0x3705E10` in FL_2027, the match manager at `0x3702208` and `0x36F3DC8`. Both lie in a data section. Earlier versions trusted the first value and used it blindly, which is wrong on any other build.

## Verified builds

| Build | PE timestamp | Image size | Status |
| --- | --- | --- | --- |
| PES 2021 Steam 1.7.2.0 | `0x62787C6A` | `0x1B48A000` | 11 of 11 unique. Tested live. |
| `FL_2027.exe` | `0x5F5ED4BF` | `0x1BBA6000` | 11 of 11 unique. Career layout identical. **Checked offline only**, not run live. |

Section names are scrambled per build. The main code section is `.text` in PES 2021 and `.trace` in FL_2027, and `.trace` is a small data section in PES 2021. The app never looks for code by name, it uses the `IMAGE_SCN_MEM_EXECUTE` flag.

## Checking an executable offline

```powershell
python native/check_exe_signatures.py "path\to\game.exe"
```

The script reads the signatures from `native/src/engine/signatures.hpp`, searches the file without running it and counts the references to the career fields (player count, transfer budget, salary cap, wage overrides, year, user club, record size). A compatible build prints `compatible by signatures and by structure` and exits with 0. The file must not be encrypted on disk.

## Diagnostics

`Settings > Services > Diagnostics > Copy report` includes the build and whether it is in the verified list, every signature with its address, how much of the executable sections was read, the squad scan (count field, bytes read, records of your club, rejected records) and where the captured player sits in the player table. Paste it when asking for help.

## Limits

- Only 64-bit PE images. Only the career layout of PES 2021 is known. A build with a different layout makes the pointer signatures fail, which is the intended result.
- Executable sections above 128 MB are not searched, except once as a last resort when no signature at all was found.
- `FL_2027.exe` has not been run live with this app. Squad and finance editing there depends on the player table, which can only be confirmed with the report from a real save.

## Adding a build

1. Run `check_exe_signatures.py` on the executable.
2. If the verdict is compatible, add a line to `kKnown` in `native/src/engine/image_scan.cpp` with `reference_rvas` set to `false`.
3. Test live and send the diagnostics report if anything looks off.
