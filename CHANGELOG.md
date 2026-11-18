# Changelog

All notable changes to this project are documented in this file.

## [10.1.0] — 2026-11-18

### Added / Adicionado
- **Bilingual Dual-Edition Release (`PT-BR` & `EN`)**: Full native **Português (PT-BR)** and **English (EN)** `.CT` editions compiled deterministically from source, plus a live **`[ PT | EN ]`** segmented switcher inside the Studio GUI header.
- **Konami Exact `OVR` Engine & Proportional DNA Solver**: Extracted the exact rating weight matrices (`23 × 13`) and non-linear lookup curve from `PES2021.exe` (`fcn.1414e7a50` + `fcn.141e640a0`), adding **`+1 OVR DNA`**, **`-1 OVR DNA`**, **`Target OVR (40–99)`**, double-clickable **`Current Overall`** row in Cheat Engine, **`Squad +1 OVR DNA`**, **`Rejuvenate (-2 Yrs) & Sync Market Value`**, and the **`13-Position & DNA X-Ray`** tactical advisor.
- **Zero-Click Auto-Arm Master Hooks**: Automatically activates all 6 Master Hooks (`900ms` settle delay) upon attaching to `PES2021.exe`.
- **Stateful Active Mode Badges (`● ATIVO` / `● ACTIVE`)**: Persistent toggle cards in the Studio GUI (*Training 10x / Instant / 1x*, *Freeze Clock*, *Home / Away 100% Stamina*) now glow with an active border and status chip, with click-to-toggle reset on Stamina cards.

### Changed / Aprimorado
- **Offscreen Double-Buffered `Canvas` Studio GUI (`940 × 636` logical `ClientWidth × ClientHeight`)**: Replaced nested LCL `TPanel`/`TLabel` controls with a custom offscreen-buffered `TBitmap.Canvas` renderer, eliminating titlebar/border clipping, text overflow, flicker, and vertical/horizontal spacing asymmetries across `100%`, `125%`, and `150%` DPI.
- **Streamlined 5-Section Cheat Table Tree**: Consolidated root sections from 7 to 5 clean categories mirroring the 4 Studio GUI tabs, with `[ ★ OPEN PRO STUDIO GUI ]` at Row #1 and instant Player OVR controls at the top of `Edit Player`.

---

## [10.0.0] — 2026-11-18

### Added / Adicionado
- Initial **v10.0 Pro Overhaul**: Dark Studio 4-tab GUI, Euro (`€`) human financial formatter/parser (`50M`, `500k`, `1.5B`), 64-slot Exact Release Clause Override (`Hook 2`), User-Only Sub-XP Multiplier (`Hook 3`), BAL 99 Focus Points unlock (`Hook 4`), Live RAM Club Decoder (`+0xADF4BC`), Global 30,000-Player Search, JSON Player Backup/Restore, FNV-1a crash-free Match Clock & Sider-safe Score/Stamina hooks (`Hook 5` & `Hook 6`).
