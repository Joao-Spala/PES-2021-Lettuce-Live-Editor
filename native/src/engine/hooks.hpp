#pragma once
// Native x64 Code-Cave Hook Manager for PES 2021 / SP Football Life.
// Exact byte-for-byte parity with verified PES2021.exe v1.7.2.0 hooks:
//   Hook 1: Automatic Player Capture (4 UI sites -> ptrPlayer)
//   Hook 3: Universal Training Sub-XP Multiplier (1x/2x/5x/10x/50x/Instant)
//   Hook 4: BAL Focus Points 21-Cap Unlock (0x15 -> 0x7F)
//   Hook 5: Match HUD & Clock Freeze Tap
//   Hook 6: Selective Match Stamina (6 modes)

#include "image_scan.hpp"
#include "signatures.hpp"
#include "win32_mem.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pes {

// Shared state block layout inside state_base (0x000..0x0A0)
struct HookStateBlock {
    uint64_t ptrCareer;             // 0x000
    uint64_t ptrPlayer;             // 0x008
    uint64_t ptrMatchMgr;           // 0x010
    uint64_t ptrMatchState;         // 0x018
    uint64_t ptrMatchHud;           // 0x020
    uint64_t ptrMatchCore;          // 0x028
    uint32_t hitsCapture[4];        // 0x030..0x03F: quantas vezes cada um dos 4 pontos de captura de atleta disparou
    uint8_t  _pad1[0x34];           // 0x040..0x073
    uint32_t cfgTrainingMultMode;   // 0x074
    uint32_t cfgStaminaMode;        // 0x078
    uint32_t cfgFreezeClock;        // 0x07C
    float    valProtectedMatchTicks;// 0x080
    float    valFrozenTotalTicks;   // 0x084
    uint8_t  _pad2[0x08];           // 0x088..0x08F
    uint32_t cfgLockBALFocusPoints; // 0x090
    uint8_t  _pad3[0x0C];           // 0x094..0x09F
};
static_assert(offsetof(HookStateBlock, ptrCareer) == 0x000);
static_assert(offsetof(HookStateBlock, ptrPlayer) == 0x008);
static_assert(offsetof(HookStateBlock, ptrMatchMgr) == 0x010);
static_assert(offsetof(HookStateBlock, ptrMatchState) == 0x018);
static_assert(offsetof(HookStateBlock, ptrMatchHud) == 0x020);
static_assert(offsetof(HookStateBlock, ptrMatchCore) == 0x028);
static_assert(offsetof(HookStateBlock, hitsCapture) == 0x030);
static_assert(offsetof(HookStateBlock, cfgTrainingMultMode) == 0x074);
static_assert(offsetof(HookStateBlock, cfgStaminaMode) == 0x078);
static_assert(offsetof(HookStateBlock, cfgFreezeClock) == 0x07C);
static_assert(offsetof(HookStateBlock, valProtectedMatchTicks) == 0x080);
static_assert(offsetof(HookStateBlock, valFrozenTotalTicks) == 0x084);
static_assert(offsetof(HookStateBlock, cfgLockBALFocusPoints) == 0x090);

enum class HookSym : uint32_t {
    ptrCareer = 0x000,
    ptrPlayer = 0x008,
    ptrMatchMgr = 0x010,
    ptrMatchState = 0x018,
    ptrMatchHud = 0x020,
    ptrMatchCore = 0x028,
    hitsCapture = 0x030,
    cfgTrainingMultMode = 0x074,
    cfgStaminaMode = 0x078,
    cfgFreezeClock = 0x07C,
    valProtectedMatchTicks = 0x080,
    valFrozenTotalTicks = 0x084,
    cfgLockBALFocusPoints = 0x090,
};

struct PatchRecord {
    uint64_t site;
    std::vector<uint8_t> orig;
    std::vector<uint8_t> patched;   // bytes que o gancho gravou (usado para recuperar ganchos orfaos)
};

// Recursos do app que dependem de uma assinatura, para o relatorio e para a interface avisarem o que falta
enum FeatureBit : uint32_t {
    kFeatPlayerCapture = 1u << 0,   // ao menos um dos 4 pontos de captura de atleta
    kFeatCareerPointers = 1u << 1,  // ponteiros de carreira e de partida
    kFeatTraining = 1u << 2,
    kFeatBalCap = 1u << 3,
    kFeatMatchClock = 1u << 4,
    kFeatMatchStamina = 1u << 5,
};

class HookManager {
public:
    explicit HookManager(Win32Memory& m) : mem(m), sigs(m) {}

    Win32Memory& mem;
    SigResolver sigs;   // assinaturas de bytes: acham todo gancho e ponteiro, sem enderecos fixos
    uint64_t state_base = 0;
    uint64_t cave_cursor = 0;
    std::unordered_map<int, std::vector<PatchRecord>> patches;
    std::array<bool, 8> active_hooks{};
    // Verdadeiro quando outra instancia viva do app tem ganchos neste jogo: a tabela legada de limpeza nao pode mexer neles
    bool other_instance_live = false;

    uint64_t sym_addr(HookSym s) const {
        if (!state_base) return 0;
        return state_base + static_cast<uint32_t>(s);
    }

    bool read_state_block(HookStateBlock& out) const {
        if (!state_base) return false;
        return mem.read_into(state_base, &out, sizeof(HookStateBlock));
    }

    bool init_state();
    void restore_stale_hook_bytes(int hook_num);

    // Procura todas as assinaturas de uma vez (o relatorio mostra cada uma)
    void resolve_all();
    // Bits de FeatureBit cujas assinaturas NAO foram achadas de forma unica neste executavel (refaz a busca do que faltou)
    uint32_t missing_features();
    // Mesmo calculo so com o que ja esta em cache: barato, pode ser chamado a cada ciclo
    uint32_t known_missing() const;
    // Executavel identico ao de referencia (PES 2021 Steam 1.7.2.0)?
    bool is_reference_build();
    // Texto de diagnostico: versao do jogo, assinaturas, ganchos e o que falta
    std::string compat_report();
    // Procura, na memoria do jogo, areas de uma instancia anterior do app que ficou sem desligar os ganchos
    // (travou ou foi encerrada) e devolve o codigo original ao jogo. Nao mexe nas de um app que ainda esta vivo.
    int recover_stale_hooks();
    bool verify_hook_alive(int hook_num) const;

    bool enable_hook_1_player();
    bool enable_hook_3_training();
    bool enable_hook_4_bal_cap();
    bool enable_hook_5_match();
    bool enable_hook_6_stamina();

    void disable_all();
    // Esquece o processo atual SEM tocar na memoria dele (o processo acabou ou trocou): zera estado, ganchos e assinaturas
    void forget_process();

private:
    bool restore_site(const PatchRecord& rec);
    bool restore_hook_sites(int hook_id);
    uint64_t alloc_cave(const std::vector<uint8_t>& code);
    bool install_jmp(int hook_id, uint64_t site, uint64_t cave_addr, size_t orig_len);
    uint64_t site_of(const SigDef& def);
    void write_journal();
};

} // namespace pes
