#include "hooks.hpp"

#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace pes {

namespace {

int32_t check_rel32(int64_t rel) {
    if (rel < -0x80000000LL || rel > 0x7FFFFFFFLL) {
        throw std::overflow_error("rel32 out of range");
    }
    return static_cast<int32_t>(rel);
}

class CodeBuf {
public:
    explicit CodeBuf(uint64_t base_addr) : base(base_addr) { buf.reserve(160); }

    uint64_t cur_addr() const { return base + buf.size(); }

    void emit(std::initializer_list<uint8_t> bytes) {
        buf.insert(buf.end(), bytes.begin(), bytes.end());
    }

    void label(const char* name) {
        labels[name] = buf.size();
    }

    void emit_rip(std::initializer_list<uint8_t> prefix, uint64_t target_abs, std::initializer_list<uint8_t> imm_tail = {}) {
        buf.insert(buf.end(), prefix.begin(), prefix.end());
        uint64_t next_rip = cur_addr() + 4 + imm_tail.size();
        int32_t rel = check_rel32(static_cast<int64_t>(target_abs) - static_cast<int64_t>(next_rip));
        uint8_t raw[4];
        std::memcpy(raw, &rel, 4);
        buf.insert(buf.end(), raw, raw + 4);
        buf.insert(buf.end(), imm_tail.begin(), imm_tail.end());
    }

    void jmp_abs(uint64_t target_abs) {
        buf.push_back(0xE9);
        int32_t rel = check_rel32(static_cast<int64_t>(target_abs) - static_cast<int64_t>(cur_addr() + 4));
        uint8_t raw[4];
        std::memcpy(raw, &rel, 4);
        buf.insert(buf.end(), raw, raw + 4);
    }

    void jcc8(uint8_t opcode, const char* lbl) {
        buf.push_back(opcode);
        fixups_rel8.emplace_back(buf.size(), lbl);
        buf.push_back(0x00);
    }

    void jcc32(std::initializer_list<uint8_t> opcodes, const char* lbl) {
        buf.insert(buf.end(), opcodes.begin(), opcodes.end());
        fixups_rel32.emplace_back(buf.size(), lbl);
        for (int i = 0; i < 4; ++i) buf.push_back(0x00);
    }

    std::vector<uint8_t> finalize() {
        for (const auto& [pos, name] : fixups_rel8) {
            int64_t rel = static_cast<int64_t>(labels.at(name)) - static_cast<int64_t>(pos + 1);
            if (rel < -128 || rel > 127) throw std::overflow_error("rel8 overflow");
            buf[pos] = static_cast<uint8_t>(rel & 0xFF);
        }
        for (const auto& [pos, name] : fixups_rel32) {
            int32_t rel = check_rel32(static_cast<int64_t>(labels.at(name)) - static_cast<int64_t>(pos + 4));
            std::memcpy(&buf[pos], &rel, 4);
        }
        return buf;
    }

private:
    uint64_t base;
    std::vector<uint8_t> buf;
    std::unordered_map<std::string, size_t> labels;
    std::vector<std::pair<size_t, std::string>> fixups_rel8;
    std::vector<std::pair<size_t, std::string>> fixups_rel32;
};

struct StaleEntry {
    uint32_t rva;
    int32_t check_off;
    uint8_t check_byte;
    uint8_t orig[10];
    uint8_t orig_len;
    int16_t stale_byte; // -1 means 0xE9
};

} // namespace

bool HookManager::init_state() {
    if (state_base && mem.attached()) return true;
    if (!mem.attached()) return false;
    sigs.reset();             // processo novo: as assinaturas precisam ser procuradas de novo
    recover_stale_hooks();    // ganchos que uma instancia anterior deixou no jogo
    uint64_t ptr = mem.alloc_near(mem.module_base, 0x2000);
    if (!ptr) return false;
    std::vector<uint8_t> zeros(0x2000, 0);
    if (!mem.write_from(ptr, zeros.data(), zeros.size())) {
        mem.free_mem(ptr);
        return false;
    }
    state_base = ptr;
    cave_cursor = ptr + 0x500;
    patches.clear();
    active_hooks.fill(false);
    return true;
}

uint64_t HookManager::alloc_cave(const std::vector<uint8_t>& code) {
    uint64_t addr = (cave_cursor + 15ULL) & ~15ULL;
    if (addr + code.size() > state_base + 0x2000ULL) return 0;
    if (!mem.write_from(addr, code.data(), code.size())) return 0;
    mem.flush_icache(addr, code.size());
    cave_cursor = addr + code.size();
    return addr;
}

bool HookManager::install_jmp(int hook_id, uint64_t site, uint64_t cave_addr, size_t orig_len) {
    if (!cave_addr || orig_len < 5) return false;
    std::vector<uint8_t> orig(orig_len);
    if (!mem.read_into(site, orig.data(), orig_len)) return false;
    // Anti-double-hook guard: never overwrite an existing JMP and corrupt original backup
    if (orig[0] == 0xE9) return false;

    int64_t rel = static_cast<int64_t>(cave_addr) - static_cast<int64_t>(site + 5);
    if (rel < -0x80000000LL || rel > 0x7FFFFFFFLL) return false;

    std::vector<uint8_t> patch(orig_len, 0x90);
    patch[0] = 0xE9;
    int32_t rel32 = static_cast<int32_t>(rel);
    std::memcpy(&patch[1], &rel32, 4);

    if (!mem.write_code_bytes(site, patch.data(), patch.size())) return false;
    patches[hook_id].push_back({site, std::move(orig), patch});
    write_journal();
    return true;
}

uint64_t HookManager::site_of(const SigDef& def) {
    const uint64_t a = sigs.find(def);
    sigs.release_code();   // o codigo lido (dezenas de MB) nao precisa ficar na memoria do app
    if (!a) return 0;
    // O resultado pode estar em cache de ha tempo: confere os bytes AO VIVO antes de deixar alguem gravar ali
    Pattern p;
    if (!parse_pattern(def.pattern, p)) return 0;
    std::vector<uint8_t> cur(p.bytes.size());
    if (!mem.read_into(a, cur.data(), cur.size())) return 0;
    for (size_t i = 0; i < cur.size(); ++i) {
        if (p.mask[i] == 0xFF && cur[i] != p.bytes[i]) return 0;
    }
    return a;
}

void HookManager::resolve_all() {
    size_t found = 0;
    for (const SigDef* d : sig::kAll) {
        if (sigs.find(*d)) ++found;
    }
    // Nenhuma nas secoes executaveis normais: o codigo pode estar numa secao enorme. Plano B, uma vez so.
    if (found == 0) sigs.resolve_bulk(sig::kAll, sizeof(sig::kAll) / sizeof(sig::kAll[0]));
    sigs.release_code();
}

bool HookManager::is_reference_build() {
    const KnownBuild* b = identify_build(sigs.image());
    return b && b->reference_rvas;
}

namespace {
template <class Ok>
uint32_t feature_mask(Ok ok) {
    uint32_t m = 0;
    if (!(ok(sig::kPlayerA) || ok(sig::kPlayerB) || ok(sig::kPlayerC) || ok(sig::kPlayerD))) m |= kFeatPlayerCapture;
    if (!(ok(sig::kCareerGetter) && ok(sig::kMatchMgr))) m |= kFeatCareerPointers;
    if (!ok(sig::kTraining)) m |= kFeatTraining;
    if (!ok(sig::kBalCap)) m |= kFeatBalCap;
    if (!(ok(sig::kMatchHud) && ok(sig::kMatchClock))) m |= kFeatMatchClock;
    if (!ok(sig::kStamina)) m |= kFeatMatchStamina;
    return m;
}
} // namespace

uint32_t HookManager::missing_features() {
    resolve_all();
    const uint32_t m = feature_mask([&](const SigDef& d) { return sigs.find(d) != 0; });
    sigs.release_code();
    return m;
}

uint32_t HookManager::known_missing() const {
    return feature_mask([&](const SigDef& d) {
        auto it = sigs.results().find(d.id);
        return it != sigs.results().end() && it->second.unique();
    });
}

std::string HookManager::compat_report() {
    resolve_all();
    std::string r = sigs.describe();
    const uint32_t m = missing_features();
    r += "legacy orphan-hook table: ";
    r += is_reference_build() ? "applies (reference build)\r\n" : "skipped (different build)\r\n";
    if (m == 0) {
        r += "features: all available\r\n";
    } else {
        r += "features missing:";
        if (m & kFeatPlayerCapture) r += " player-capture";
        if (m & kFeatCareerPointers) r += " career-pointers";
        if (m & kFeatTraining) r += " training-xp";
        if (m & kFeatBalCap) r += " bal-focus-cap";
        if (m & kFeatMatchClock) r += " match-clock";
        if (m & kFeatMatchStamina) r += " match-stamina";
        r += "\r\n";
    }
    return r;
}

// ---- Diario dos ganchos dentro da area do app no jogo: permite recuperar ganchos orfaos em qualquer versao ----
namespace {
constexpr uint64_t kJournalOff = 0x200;
constexpr size_t kJournalMax = 15;
struct JournalHeader {
    char magic[8];         // "LLEHOOK2"
    uint32_t owner_pid;    // processo do app dono desta area
    uint32_t count;
    uint64_t owner_start;  // FILETIME de criacao do dono: distingue o dono de outro processo que herdou o mesmo PID
};
struct JournalEntry {
    uint64_t site;
    uint8_t len;
    uint8_t pad[7];
    uint8_t orig[16];
    uint8_t patched[16];
};
static_assert(sizeof(JournalHeader) == 24);
static_assert(sizeof(JournalEntry) == 48);
static_assert(kJournalOff + sizeof(JournalHeader) + kJournalMax * sizeof(JournalEntry) <= 0x500);

uint64_t process_start_time(HANDLE h) {
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetProcessTimes(h, &created, &exited, &kernel, &user)) return 0;
    return (static_cast<uint64_t>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
}

// O app dono ainda esta rodando? Compara o PID e o horario de criacao (o nome do arquivo pode ser outro: duas copias
// do app com nomes diferentes podem estar abertas ao mesmo tempo). Em duvida considera vivo: melhor nao mexer.
bool owner_alive(uint32_t pid, uint64_t start) {
    if (pid == 0) return false;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return GetLastError() == ERROR_ACCESS_DENIED;
    DWORD code = 0;
    bool alive = GetExitCodeProcess(h, &code) && code == STILL_ACTIVE;
    if (alive && start != 0) {
        const uint64_t now_start = process_start_time(h);
        alive = (now_start == 0) || (now_start == start);   // outro horario = PID reaproveitado por outro programa
    }
    CloseHandle(h);
    return alive;
}
} // namespace

void HookManager::write_journal() {
    if (!state_base || !mem.attached()) return;
    std::vector<uint8_t> buf(sizeof(JournalHeader) + kJournalMax * sizeof(JournalEntry), 0);
    JournalHeader hdr{};
    std::memcpy(hdr.magic, "LLEHOOK2", 8);
    hdr.owner_pid = GetCurrentProcessId();
    hdr.owner_start = process_start_time(GetCurrentProcess());
    uint32_t n = 0;
    for (const auto& [hid, list] : patches) {
        for (const auto& rec : list) {
            if (n >= kJournalMax) break;
            if (rec.orig.empty() || rec.orig.size() > 16 || rec.patched.size() != rec.orig.size()) continue;
            JournalEntry e{};
            e.site = rec.site;
            e.len = static_cast<uint8_t>(rec.orig.size());
            std::memcpy(e.orig, rec.orig.data(), rec.orig.size());
            std::memcpy(e.patched, rec.patched.data(), rec.patched.size());
            std::memcpy(buf.data() + sizeof(JournalHeader) + n * sizeof(JournalEntry), &e, sizeof(e));
            ++n;
        }
    }
    hdr.count = n;
    std::memcpy(buf.data(), &hdr, sizeof(hdr));
    mem.write_from(state_base + kJournalOff, buf.data(), buf.size());
}

int HookManager::recover_stale_hooks() {
    other_instance_live = false;
    if (!mem.attached() || !mem.handle || !mem.module_base) return 0;
    int restored_regions = 0;
    const uint64_t own_start = process_start_time(GetCurrentProcess());
    const uint64_t lo = mem.module_base > 0x80000000ULL ? mem.module_base - 0x80000000ULL : 0x10000ULL;
    const uint64_t hi = mem.module_base + 0x80000000ULL;
    uint64_t addr = lo;
    MEMORY_BASIC_INFORMATION mbi{};
    while (addr < hi && VirtualQueryEx(mem.handle, reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)) == sizeof(mbi)) {
        const uint64_t base = reinterpret_cast<uint64_t>(mbi.BaseAddress);
        const uint64_t next = base + mbi.RegionSize;
        // A area do app e sempre privada, executavel e de 0x2000 bytes
        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE && mbi.RegionSize == 0x2000 &&
            mbi.Protect == PAGE_EXECUTE_READWRITE && base != state_base) {
            std::vector<uint8_t> buf(sizeof(JournalHeader) + kJournalMax * sizeof(JournalEntry));
            JournalHeader hdr{};
            if (mem.read_into(base + kJournalOff, buf.data(), buf.size())) {
                std::memcpy(&hdr, buf.data(), sizeof(hdr));
                if (std::memcmp(hdr.magic, "LLEHOOK2", 8) == 0 && hdr.count <= kJournalMax) {
                    // Area desta mesma instancia (PID e horario de criacao iguais): nao e orfa
                    const bool mine = hdr.owner_pid == GetCurrentProcessId() && (hdr.owner_start == 0 || hdr.owner_start == own_start);
                    if (!mine && owner_alive(hdr.owner_pid, hdr.owner_start)) {
                        other_instance_live = true;   // outro app vivo: nao mexe e a limpeza legada tambem nao pode
                    } else if (!mine) {
                        bool safe = true;   // so libera a area se nada do jogo ainda salta para ela
                        for (uint32_t i = 0; i < hdr.count; ++i) {
                            JournalEntry e{};
                            std::memcpy(&e, buf.data() + sizeof(JournalHeader) + i * sizeof(JournalEntry), sizeof(e));
                            if (e.len == 0 || e.len > 16 || e.site <= 0x10000ULL) continue;
                            uint8_t cur[16]{};
                            if (!mem.read_into(e.site, cur, e.len)) {
                                safe = false;
                                continue;
                            }
                            bool ours = std::memcmp(cur, e.patched, e.len) == 0;
                            if (!ours && e.len >= 5 && cur[0] == 0xE9) {
                                // salto para esta area mesmo com a cauda diferente do que o gancho gravou
                                int32_t rel = 0;
                                std::memcpy(&rel, cur + 1, 4);
                                const uint64_t target = static_cast<uint64_t>(static_cast<int64_t>(e.site + 5) + rel);
                                ours = target >= base && target < base + 0x2000ULL;
                            }
                            if (!ours) continue;   // ja esta original, ou e de outro: nao e nosso
                            uint8_t chk[16]{};
                            if (!mem.write_code_bytes(e.site, e.orig, e.len) || !mem.read_into(e.site, chk, e.len) ||
                                std::memcmp(chk, e.orig, e.len) != 0) {
                                safe = false;
                            }
                        }
                        if (safe) {
                            Sleep(80);   // uma thread do jogo pode estar dentro do codigo do gancho
                            mem.free_mem(base);
                            ++restored_regions;
                        }
                    }
                }
            }
        }
        if (next <= addr) break;
        addr = next;
    }
    return restored_regions;
}

bool HookManager::restore_site(const PatchRecord& rec) {
    const size_t n = rec.orig.size();
    if (n == 0 || n > 16) return true;
    uint8_t cur[16]{};
    if (!mem.read_into(rec.site, cur, n)) return false;   // ilegivel: nao da para afirmar que voltou ao original
    if (std::memcmp(cur, rec.orig.data(), n) == 0) return true;
    bool ours = rec.patched.size() == n && std::memcmp(cur, rec.patched.data(), n) == 0;
    if (!ours && n >= 5 && cur[0] == 0xE9 && state_base) {
        int32_t rel = 0;
        std::memcpy(&rel, cur + 1, 4);
        const uint64_t target = static_cast<uint64_t>(static_cast<int64_t>(rec.site + 5) + rel);
        ours = target >= state_base && target < state_base + 0x2000ULL;   // salto para a nossa area
    }
    if (!ours) return true;   // bytes de outro (outra instancia ou outro programa): nao e nosso para desfazer
    if (!mem.write_code_bytes(rec.site, rec.orig.data(), n)) return false;
    uint8_t chk[16]{};
    return mem.read_into(rec.site, chk, n) && std::memcmp(chk, rec.orig.data(), n) == 0;
}

bool HookManager::restore_hook_sites(int hook_id) {
    auto it = patches.find(hook_id);
    if (it == patches.end()) return true;
    bool ok = true;
    for (const auto& rec : it->second) {
        if (!restore_site(rec)) ok = false;
    }
    return ok;
}

void HookManager::forget_process() {
    patches.clear();
    active_hooks.fill(false);
    state_base = 0;
    cave_cursor = 0;
    other_instance_live = false;
    sigs.reset();
}

void HookManager::restore_stale_hook_bytes(int hook_num) {
    // Tabela de RVAs so vale no executavel de referencia: em qualquer outro ela gravaria em endereco errado
    if (!mem.attached() || !mem.module_base || !is_reference_build()) return;
    if (other_instance_live) return;   // outra instancia viva do app tem ganchos aqui: reverter seria brigar com ela

    static const std::unordered_map<int, std::vector<StaleEntry>> kStale = {
        {1, {
            {0xC6ED40, -19, 0xCB, {0x0F, 0x10, 0x02, 0x0F, 0x11, 0x01}, 6, -1},
            {0x8E24D7,  -7, 0x48, {0x48, 0x8B, 0x40, 0x2C, 0x48, 0x89, 0x02}, 7, -1},
            {0xDDCA5C,   7, 0x89, {0x0F, 0xB6, 0x80, 0x50, 0x01, 0x00, 0x00}, 7, -1},
            {0xF27462,   7, 0x89, {0x0F, 0xB6, 0x80, 0x50, 0x01, 0x00, 0x00}, 7, -1},
        }},
        {3, {
            {0x12E14A0,  6, 0x0F, {0x45, 0x8B, 0xC8, 0x83, 0xFA, 0x6C}, 6, -1},
        }},
        {4, {
            {0xBBFE79,  -8, 0x0F, {0x15}, 1, 0x7F},
        }},
        {5, {
            {0x20F9FD6,  7, 0x88, {0x49, 0x8B, 0xF8, 0x44, 0x0F, 0xB6, 0xE2}, 7, -1},
            {0xA3C787,   8, 0xF3, {0x8B, 0x44, 0x24, 0x40, 0x89, 0x44, 0x24, 0x40}, 8, -1},
        }},
        {6, {
            {0xA58EAC,   9, 0x48, {0x6B, 0x45, 0x10, 0x64, 0x48, 0x8B, 0x5C, 0x24, 0x50}, 9, -1},
        }},
    };

    auto it = kStale.find(hook_num);
    if (it == kStale.end()) return;
    for (const auto& e : it->second) {
        uint64_t addr = mem.module_base + e.rva;
        uint8_t b0 = mem.read_u8(addr);
        bool is_stale = (e.stale_byte >= 0) ? (b0 == static_cast<uint8_t>(e.stale_byte)) : (b0 == 0xE9);
        if (is_stale && mem.read_u8(addr + e.check_off) == e.check_byte) {
            mem.write_code_bytes(addr, e.orig, e.orig_len);
        }
    }
}

bool HookManager::verify_hook_alive(int hook_num) const {
    if (!active_hooks[hook_num] || !state_base || !mem.attached()) return false;
    auto it = patches.find(hook_num);
    if (it == patches.end() || it->second.empty()) return false;
    for (const auto& rec : it->second) {
        if (rec.orig.size() < 5) continue;
        uint8_t cur[5]{};
        if (!mem.read_into(rec.site, cur, 5)) return false;
        if (cur[0] != 0xE9) return false;
        int32_t rel = 0;
        std::memcpy(&rel, cur + 1, 4);
        uint64_t target = static_cast<uint64_t>(static_cast<int64_t>(rec.site + 5) + rel);
        if (target < state_base || target >= state_base + 0x2000ULL) return false;
    }
    return true;
}

bool HookManager::enable_hook_1_player() {
    if (active_hooks[1] && verify_hook_alive(1)) return true;
    active_hooks[1] = false;
    // Um dos 4 pontos pode ter sido pisado por fora enquanto os outros seguem apontando para as nossas cavernas:
    // devolve o original de tudo o que ainda e nosso antes de refazer, senao esses saltos ficariam sem registro
    if (!restore_hook_sites(1)) return false;
    patches.erase(1);
    write_journal();
    if (!init_state()) return false;
    restore_stale_hook_bytes(1);
    // Refazer o gancho gasta caverna nova. Estourado o limite so da para recomecar se nenhum outro gancho usa a area.
    if (cave_cursor > state_base + 0x1600ULL) {
        if (active_hooks[3] || active_hooks[5] || active_hooks[6]) return false;
        cave_cursor = state_base + 0x500ULL;
    }
    const uint64_t ptr_player = sym_addr(HookSym::ptrPlayer);
    const uint64_t hits = sym_addr(HookSym::hitsCapture);   // contador por ponto (diagnostico: de qual tela veio a captura)
    int installed = 0;

    try {
        if (uint64_t s1 = site_of(sig::kPlayerA)) {
            uint64_t site = s1 + 0x13;
            uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
            CodeBuf cb(cave_addr);
            cb.emit_rip({0xFF, 0x05}, hits + 0);   // inc dword [hits[0]]
            cb.emit({0x83, 0xF8, 0x02});
            cb.jcc8(0x75, "code");
            cb.emit_rip({0x48, 0x89, 0x15}, ptr_player);
            cb.label("code");
            cb.emit({0x0F, 0x10, 0x02, 0x0F, 0x11, 0x01});
            cb.jmp_abs(site + 6);
            if (install_jmp(1, site, alloc_cave(cb.finalize()), 6)) ++installed;
        }
        if (uint64_t s2 = site_of(sig::kPlayerB)) {
            uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
            CodeBuf cb(cave_addr);
            cb.emit_rip({0xFF, 0x05}, hits + 4);   // inc dword [hits[1]]
            cb.emit_rip({0x48, 0x89, 0x05}, ptr_player);
            cb.emit({0x48, 0x8B, 0x40, 0x2C, 0x48, 0x89, 0x02});
            cb.jmp_abs(s2 + 7);
            if (install_jmp(1, s2, alloc_cave(cb.finalize()), 7)) ++installed;
        }
        if (uint64_t s3 = site_of(sig::kPlayerC)) {
            uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
            CodeBuf cb(cave_addr);
            cb.emit_rip({0xFF, 0x05}, hits + 8);   // inc dword [hits[2]]
            cb.emit({0x48, 0x85, 0xC0});
            cb.jcc8(0x74, "code");
            cb.emit_rip({0x48, 0x89, 0x05}, ptr_player);
            cb.label("code");
            cb.emit({0x0F, 0xB6, 0x80, 0x50, 0x01, 0x00, 0x00});
            cb.jmp_abs(s3 + 7);
            if (install_jmp(1, s3, alloc_cave(cb.finalize()), 7)) ++installed;
        }
        if (uint64_t s4 = site_of(sig::kPlayerD)) {
            uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
            CodeBuf cb(cave_addr);
            cb.emit_rip({0xFF, 0x05}, hits + 12);  // inc dword [hits[3]]
            cb.emit({0x48, 0x85, 0xC0});
            cb.jcc8(0x74, "code");
            cb.emit_rip({0x48, 0x89, 0x05}, ptr_player);
            cb.label("code");
            cb.emit({0x0F, 0xB6, 0x80, 0x50, 0x01, 0x00, 0x00});
            cb.jmp_abs(s4 + 7);
            if (install_jmp(1, s4, alloc_cave(cb.finalize()), 7)) ++installed;
        }
    } catch (...) {
        return false;
    }

    active_hooks[1] = installed > 0;
    return active_hooks[1];
}

bool HookManager::enable_hook_3_training() {
    if (active_hooks[3]) return true;
    if (!init_state()) return false;
    restore_stale_hook_bytes(3);
    uint64_t s = site_of(sig::kTraining);
    if (!s) return false;
    try {
        uint64_t cfg = sym_addr(HookSym::cfgTrainingMultMode);
        uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
        CodeBuf cb(cave_addr);
        cb.emit_rip({0x44, 0x8B, 0x0D}, cfg);
        cb.emit({0x45, 0x85, 0xC9});
        cb.jcc8(0x74, "orig");
        cb.emit({0x45, 0x85, 0xC0});
        cb.jcc8(0x7E, "orig");

        cb.emit({0x41, 0x83, 0xF9, 0x01});
        cb.jcc8(0x75, "m2");
        cb.emit({0x45, 0x6B, 0xC0, 0x02});
        cb.jcc8(0xEB, "clamp");

        cb.label("m2");
        cb.emit({0x41, 0x83, 0xF9, 0x02});
        cb.jcc8(0x75, "m3");
        cb.emit({0x45, 0x6B, 0xC0, 0x05});
        cb.jcc8(0xEB, "clamp");

        cb.label("m3");
        cb.emit({0x41, 0x83, 0xF9, 0x03});
        cb.jcc8(0x75, "m4");
        cb.emit({0x45, 0x6B, 0xC0, 0x0A});
        cb.jcc8(0xEB, "clamp");

        cb.label("m4");
        cb.emit({0x41, 0x83, 0xF9, 0x04});
        cb.jcc8(0x75, "m5");
        cb.emit({0x45, 0x6B, 0xC0, 0x32});
        cb.jcc8(0xEB, "clamp");

        cb.label("m5");
        cb.emit({0x41, 0xB8, 0x10, 0x27, 0x00, 0x00});

        cb.label("clamp");
        cb.emit({0x41, 0x81, 0xF8, 0x10, 0x27, 0x00, 0x00});
        cb.jcc8(0x76, "orig"); // jbe (unsigned <= 10000)
        cb.emit({0x41, 0xB8, 0x10, 0x27, 0x00, 0x00});

        cb.label("orig");
        cb.emit({0x45, 0x8B, 0xC8, 0x83, 0xFA, 0x6C});
        cb.jmp_abs(s + 6);

        bool ok = install_jmp(3, s, alloc_cave(cb.finalize()), 6);
        active_hooks[3] = ok;
        return ok;
    } catch (...) {
        return false;
    }
}

bool HookManager::enable_hook_4_bal_cap() {
    if (active_hooks[4]) return true;
    if (!init_state()) return false;
    restore_stale_hook_bytes(4);
    uint64_t s = site_of(sig::kBalCap);
    if (!s) return false;
    uint64_t site = s + 8;
    uint8_t orig = 0;
    if (!mem.read_into(site, &orig, 1)) return false;
    uint8_t patched = 0x7F;
    if (!mem.write_code_bytes(site, &patched, 1)) return false;
    patches[4].push_back({site, {orig}, {patched}});
    write_journal();
    active_hooks[4] = true;
    return true;
}

bool HookManager::enable_hook_5_match() {
    if (active_hooks[5]) return true;
    if (!init_state()) return false;
    restore_stale_hook_bytes(5);
    bool ok_clk = false;

    try {
        if (uint64_t s_hud = site_of(sig::kMatchHud)) {
            uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
            CodeBuf cb(cave_addr);
            cb.emit_rip({0x48, 0x89, 0x0D}, sym_addr(HookSym::ptrMatchHud));
            cb.emit({0x50, 0x48, 0x8B, 0x41, 0x38});
            cb.emit_rip({0x48, 0x89, 0x05}, sym_addr(HookSym::ptrMatchCore));
            cb.emit({0x58, 0x49, 0x8B, 0xF8, 0x44, 0x0F, 0xB6, 0xE2});
            cb.jmp_abs(s_hud + 7);
            install_jmp(5, s_hud, alloc_cave(cb.finalize()), 7);
        }

        if (uint64_t s_clk = site_of(sig::kMatchClock)) {
            uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
            CodeBuf cb(cave_addr);
            cb.emit({0x8B, 0x44, 0x24, 0x40, 0x51});
            cb.emit_rip({0x48, 0x8B, 0x0D}, sym_addr(HookSym::ptrMatchState));
            cb.emit({0x48, 0x85, 0xC9});
            cb.jcc8(0x74, "done");
            cb.emit({0x48, 0x8D, 0x89, 0x68, 0x11, 0x00, 0x00, 0x48, 0x39, 0xCB});
            cb.jcc8(0x75, "done");
            cb.emit_rip({0x89, 0x05}, sym_addr(HookSym::valProtectedMatchTicks));
            cb.emit_rip({0x83, 0x3D}, sym_addr(HookSym::cfgFreezeClock), {0x01});
            cb.jcc8(0x75, "done");
            cb.emit_rip({0xF3, 0x0F, 0x10, 0x05}, sym_addr(HookSym::valFrozenTotalTicks));
            cb.emit_rip({0xF3, 0x0F, 0x5C, 0x05}, sym_addr(HookSym::valProtectedMatchTicks));
            cb.emit({0xF3, 0x0F, 0x11, 0x43, 0x0C});
            cb.label("done");
            cb.emit({0x59, 0x89, 0x44, 0x24, 0x40});
            cb.jmp_abs(s_clk + 8);
            ok_clk = install_jmp(5, s_clk, alloc_cave(cb.finalize()), 8);
        }
    } catch (...) {
        ok_clk = false;
    }

    if (!ok_clk) {
        auto it = patches.find(5);
        if (it != patches.end()) {
            for (const auto& rec : it->second) {
                mem.write_code_bytes(rec.site, rec.orig.data(), rec.orig.size());
            }
            patches.erase(it);
        }
    }

    active_hooks[5] = ok_clk;
    write_journal();
    return ok_clk;
}

bool HookManager::enable_hook_6_stamina() {
    if (active_hooks[6]) return true;
    if (!init_state()) return false;
    restore_stale_hook_bytes(6);
    uint64_t s = site_of(sig::kStamina);
    if (!s) return false;

    try {
        uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
        CodeBuf cb(cave_addr);
        uint64_t cfg = sym_addr(HookSym::cfgStaminaMode);
        uint64_t mgr = sym_addr(HookSym::ptrMatchMgr);

        cb.emit_rip({0x83, 0x3D}, cfg, {0x00});
        cb.jcc32({0x0F, 0x84}, "orig");
        cb.emit_rip({0x48, 0x8B, 0x0D}, mgr);
        cb.emit({0x48, 0x85, 0xC9});
        cb.jcc32({0x0F, 0x84}, "orig");
        cb.emit({0x31, 0xD2});

        cb.label("loop");
        cb.emit({0x48, 0x89, 0xD0, 0x48, 0x05, 0xF3, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x04, 0x80, 0x48, 0x39, 0x1C, 0xC1});
        cb.jcc8(0x74, "found");
        cb.emit({0xFF, 0xC2, 0x83, 0xFA, 0x16});
        cb.jcc8(0x72, "loop");
        cb.jcc32({0xE9}, "orig");

        cb.label("found");
        cb.emit_rip({0x8B, 0x05}, cfg);
        cb.emit({0x83, 0xFA, 0x0B});
        cb.jcc8(0x73, "away");

        cb.emit({0x83, 0xF8, 0x01});
        cb.jcc8(0x74, "give100");
        cb.emit({0x83, 0xF8, 0x03});
        cb.jcc8(0x74, "give100");
        cb.emit({0x83, 0xF8, 0x04});
        cb.jcc8(0x74, "give100");
        cb.emit({0x83, 0xF8, 0x05});
        cb.jcc8(0x74, "drain");
        cb.jcc8(0xEB, "orig");

        cb.label("away");
        cb.emit({0x83, 0xF8, 0x02});
        cb.jcc8(0x74, "give100");
        cb.emit({0x83, 0xF8, 0x03});
        cb.jcc8(0x74, "give100");
        cb.emit({0x83, 0xF8, 0x05});
        cb.jcc8(0x74, "give100");
        cb.emit({0x83, 0xF8, 0x04});
        cb.jcc8(0x74, "drain");
        cb.jcc8(0xEB, "orig");

        cb.label("give100");
        cb.emit({0x48, 0x8B, 0x5C, 0x24, 0x50, 0x48, 0x8B, 0x7C, 0x24, 0x58, 0xB8, 0x64, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x30, 0x5D, 0xC3});

        cb.label("drain");
        cb.emit({0x48, 0x8B, 0x5C, 0x24, 0x50, 0x48, 0x8B, 0x7C, 0x24, 0x58, 0xB8, 0x05, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x30, 0x5D, 0xC3});

        cb.label("orig");
        cb.emit({0x6B, 0x45, 0x10, 0x64, 0x48, 0x8B, 0x5C, 0x24, 0x50});
        cb.jmp_abs(s + 9);

        bool ok = install_jmp(6, s, alloc_cave(cb.finalize()), 9);
        active_hooks[6] = ok;
        return ok;
    } catch (...) {
        return false;
    }
}

void HookManager::disable_all() {
    if (mem.attached()) {
        bool all_restored = true;
        for (const auto& [hid, list] : patches) {
            (void)list;
            if (!restore_hook_sites(hid)) all_restored = false;
        }
        if (state_base && all_restored) {
            // Allow any in-flight game thread inside a cave to return before freeing
            Sleep(80);
            mem.free_mem(state_base);
        }
    }
    forget_process();
}

} // namespace pes
