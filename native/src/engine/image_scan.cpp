#include "image_scan.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace pes {

namespace {

constexpr uint32_t kMaxCachedSection = 0x8000000;    // 128 MB: secoes de codigo maiores so entram na busca "bulk"
constexpr uint64_t kMaxCachedTotal = 0x10000000ULL;  // 256 MB lidos no total
constexpr uint64_t kMaxBulkScanned = 0x40000000ULL;  // 1 GB no maximo na busca "bulk"
constexpr size_t kReadPiece = 0x800000;              // 8 MB por leitura

// Versoes conferidas a mao (PE da imagem em memoria)
const KnownBuild kKnown[] = {
    {"PES 2021 Steam 1.7.2.0 (DP 7.0)", 0x62787C6Au, 0x1B48A000u, true},
    // Conferido offline no arquivo: as 11 assinaturas sao unicas e as referencias aos campos da carreira (contador,
    // orcamento, teto salarial, salarios, ano e clube) aparecem o mesmo numero de vezes que na referencia.
    {"Football Life 2027 (FL_2027.exe)", 0x5F5ED4BFu, 0x1BBA6000u, false},
};

int hex_nib(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string hex(uint64_t v) {
    char b[32];
    std::snprintf(b, sizeof(b), "0x%llX", static_cast<unsigned long long>(v));
    return b;
}

} // namespace

bool read_image_info(const Win32Memory& mem, ImageInfo& out) {
    out = ImageInfo{};
    if (!mem.module_base) return false;
    uint8_t dos[0x40]{};
    if (!mem.read_into(mem.module_base, dos, sizeof(dos))) return false;
    if (dos[0] != 'M' || dos[1] != 'Z') return false;
    uint32_t nt_off = 0;
    std::memcpy(&nt_off, dos + 0x3C, 4);
    if (nt_off < 0x40 || nt_off > 0x1000) return false;
    uint8_t nt[0x18 + 0xF0]{};
    if (!mem.read_into(mem.module_base + nt_off, nt, sizeof(nt))) return false;
    if (nt[0] != 'P' || nt[1] != 'E' || nt[2] != 0 || nt[3] != 0) return false;
    uint16_t nsec = 0, optsz = 0, magic = 0;
    std::memcpy(&out.machine, nt + 4, 2);
    std::memcpy(&nsec, nt + 6, 2);
    std::memcpy(&out.timestamp, nt + 8, 4);
    std::memcpy(&optsz, nt + 20, 2);
    const uint8_t* opt = nt + 0x18;
    std::memcpy(&magic, opt, 2);
    if (magic != 0x20B || nsec == 0 || nsec > 96 || optsz < 0x40 || optsz > 0xF0) return false;   // so PE32+ (64 bits)
    std::memcpy(&out.entry_rva, opt + 16, 4);
    std::memcpy(&out.size_of_image, opt + 56, 4);
    std::memcpy(&out.checksum, opt + 64, 4);
    out.base = mem.module_base;

    std::vector<uint8_t> sec(static_cast<size_t>(nsec) * 40);
    if (!mem.read_into(mem.module_base + nt_off + 0x18 + optsz, sec.data(), sec.size())) return false;
    for (uint16_t i = 0; i < nsec; ++i) {
        const uint8_t* s = sec.data() + static_cast<size_t>(i) * 40;
        ImageSection is;
        char nm[9]{};
        std::memcpy(nm, s, 8);
        is.name = nm;
        std::memcpy(&is.vsize, s + 8, 4);
        std::memcpy(&is.rva, s + 12, 4);
        std::memcpy(&is.characteristics, s + 36, 4);
        out.sections.push_back(std::move(is));
    }
    out.valid = true;
    return true;
}

const KnownBuild* known_builds(size_t& count) {
    count = sizeof(kKnown) / sizeof(kKnown[0]);
    return kKnown;
}

const KnownBuild* identify_build(const ImageInfo& img) {
    if (!img.valid) return nullptr;
    for (const auto& b : kKnown) {
        if (b.timestamp == img.timestamp && b.size_of_image == img.size_of_image) return &b;
    }
    return nullptr;
}

bool parse_pattern(std::string_view text, Pattern& out) {
    out = Pattern{};
    size_t pos = 0;
    while (pos < text.size()) {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
        if (pos >= text.size()) break;
        size_t end = pos;
        while (end < text.size() && !std::isspace(static_cast<unsigned char>(text[end]))) ++end;
        const std::string_view tok = text.substr(pos, end - pos);
        if (tok == "?" || tok == "??") {
            out.bytes.push_back(0);
            out.mask.push_back(0x00);
        } else if (tok.size() == 2 && hex_nib(tok[0]) >= 0 && hex_nib(tok[1]) >= 0) {
            out.bytes.push_back(static_cast<uint8_t>((hex_nib(tok[0]) << 4) | hex_nib(tok[1])));
            out.mask.push_back(0xFF);
        } else {
            out = Pattern{};
            return false;
        }
        pos = end;
    }
    // Precisa de ao menos 4 bytes fixos: padroes quase todo curinga casariam em qualquer lugar
    size_t fixed = 0;
    for (uint8_t m : out.mask) fixed += (m == 0xFF) ? 1 : 0;
    if (fixed < 4) {
        out = Pattern{};
        return false;
    }
    return true;
}

size_t find_pattern(const uint8_t* data, size_t len, const Pattern& p, size_t* out_pos, size_t max_out, size_t count_cap) {
    const size_t n = p.bytes.size();
    if (!data || n == 0 || len < n) return 0;
    // Ancora: maior trecho seguido de bytes fixos
    size_t best_start = 0, best_len = 0, cur_start = 0, cur_len = 0;
    for (size_t i = 0; i < n; ++i) {
        if (p.mask[i] == 0xFF) {
            if (cur_len == 0) cur_start = i;
            ++cur_len;
            if (cur_len > best_len) { best_len = cur_len; best_start = cur_start; }
        } else {
            cur_len = 0;
        }
    }
    if (best_len == 0) return 0;
    const uint8_t first = p.bytes[best_start];
    size_t found = 0;
    const size_t last = len - n;   // ultima posicao inicial possivel
    size_t i = 0;
    while (i <= last) {
        // procura o primeiro byte da ancora a partir de i + best_start
        const uint8_t* hit = static_cast<const uint8_t*>(std::memchr(data + i + best_start, first, last - i + 1));
        if (!hit) break;
        const size_t pos = static_cast<size_t>(hit - data) - best_start;
        bool ok = true;
        for (size_t k = 0; k < n; ++k) {
            if (p.mask[k] == 0xFF && data[pos + k] != p.bytes[k]) { ok = false; break; }
        }
        if (ok) {
            if (found < max_out && out_pos) out_pos[found] = pos;
            ++found;
            if (found >= count_cap) break;
        }
        i = pos + 1;
    }
    return found;
}

// ---------------------------------------------------------------------------------------------------------------

void SigResolver::reset() {
    img_ = ImageInfo{};
    img_read_ = false;
    loaded_ = false;
    code_.clear();
    code_.shrink_to_fit();
    results_.clear();
    retry_after_.clear();
    fail_count_.clear();
    cov_ = Coverage{};
    bulk_done_ = false;
}

void SigResolver::release_code() {
    loaded_ = false;
    code_.clear();
    code_.shrink_to_fit();
}

const ImageInfo& SigResolver::image() {
    if (!img_read_) {
        img_read_ = read_image_info(mem_, img_);
        if (!img_read_) img_ = ImageInfo{};
    }
    return img_;
}

bool SigResolver::ensure_loaded() {
    if (loaded_) return true;
    const ImageInfo& im = image();
    if (!im.valid) return false;
    code_.clear();
    cov_ = Coverage{};
    uint64_t total = 0;
    for (const auto& s : im.sections) {
        if (!s.is_exec() || s.vsize == 0) continue;
        // Secoes enormes (blocos de dados marcados como executaveis) ficam fora da busca normal
        if (s.vsize > kMaxCachedSection || total + s.vsize > kMaxCachedTotal) {
            cov_.skipped += s.vsize;
            continue;
        }
        Chunk c;
        c.rva = s.rva;
        c.bytes.assign(s.vsize, 0);
        bool any = false;
        for (size_t off = 0; off < s.vsize; off += kReadPiece) {
            const size_t n = std::min<size_t>(kReadPiece, s.vsize - off);
            if (mem_.read_into(mem_.module_base + s.rva + off, c.bytes.data() + off, n)) {
                cov_.scanned += n;
                any = true;
                continue;
            }
            // Um bloco falhou: tenta pagina a pagina, para uma pagina ruim nao tirar 8 MB da busca
            for (size_t o = 0; o < n; o += 0x10000) {
                const size_t m = std::min<size_t>(0x10000, n - o);
                if (mem_.read_into(mem_.module_base + s.rva + off + o, c.bytes.data() + off + o, m)) {
                    cov_.scanned += m;
                    any = true;
                } else {
                    cov_.unreadable += m;
                }
            }
        }
        if (any) {
            total += s.vsize;
            code_.push_back(std::move(c));
        }
    }
    loaded_ = !code_.empty();
    return loaded_;
}

size_t SigResolver::scan_code(const Pattern& p, uint64_t* out_addr, size_t max_out) {
    size_t total = 0;
    for (const auto& c : code_) {
        size_t pos[8];
        const size_t cap = std::min<size_t>(max_out, 8);
        const size_t n = find_pattern(c.bytes.data(), c.bytes.size(), p, pos, cap, 8);
        for (size_t k = 0; k < n && total + k < max_out; ++k) {
            out_addr[total + k] = mem_.module_base + c.rva + pos[k];
        }
        total += n;
        if (total >= 8) break;
    }
    return total;
}

void SigResolver::schedule_retry(const std::string& id) {
    const int f = ++fail_count_[id];
    const uint64_t delay = std::min<uint64_t>(60000ULL, 3000ULL << std::min(f - 1, 4));
    retry_after_[id] = GetTickCount64() + delay;
}

void SigResolver::resolve_bulk(const SigDef* const* defs, size_t n) {
    if (bulk_done_) return;
    bulk_done_ = true;
    const ImageInfo& im = image();
    if (!im.valid) return;
    struct Pending {
        const SigDef* def;
        Pattern pat;
        std::vector<uint64_t> addrs;
    };
    std::vector<Pending> pend;
    size_t max_overlap = 0;
    for (size_t i = 0; i < n; ++i) {
        auto it = results_.find(defs[i]->id);
        if (it != results_.end() && it->second.matches != 0) continue;   // achada (ou ambigua) no codigo normal: nao mexe
        Pending p;
        p.def = defs[i];
        if (!parse_pattern(defs[i]->pattern, p.pat)) continue;
        max_overlap = std::max(max_overlap, p.pat.bytes.size() - 1);
        pend.push_back(std::move(p));
    }
    if (pend.empty()) return;
    constexpr size_t kBlock = 0x1000000;   // 16 MB
    uint64_t scanned = 0;
    std::vector<uint8_t> buf;
    for (const auto& s : im.sections) {
        if (!s.is_exec() || s.vsize <= kMaxCachedSection) continue;
        for (uint64_t off = 0; off < s.vsize && scanned < kMaxBulkScanned; off += kBlock) {
            const size_t want = static_cast<size_t>(std::min<uint64_t>(kBlock + max_overlap, s.vsize - off));
            buf.assign(want, 0);
            if (!mem_.read_into(mem_.module_base + s.rva + off, buf.data(), want)) continue;
            scanned += want;
            for (auto& pd : pend) {
                size_t pos[4];
                const size_t c = find_pattern(buf.data(), buf.size(), pd.pat, pos, 4, 4);
                for (size_t k = 0; k < c; ++k) {
                    const uint64_t a = mem_.module_base + s.rva + off + pos[k];
                    // o trecho de sobreposicao entre blocos pode repetir uma ocorrencia
                    if (std::find(pd.addrs.begin(), pd.addrs.end(), a) == pd.addrs.end() && pd.addrs.size() < 4) pd.addrs.push_back(a);
                }
            }
        }
    }
    for (auto& pd : pend) {
        if (pd.addrs.empty()) continue;
        SigResult r;
        r.id = pd.def->id;
        r.hint_rva = pd.def->hint_rva;
        r.matches = static_cast<int>(pd.addrs.size());
        r.addr = (pd.addrs.size() == 1) ? pd.addrs[0] : 0;
        results_[r.id] = r;
    }
}

uint64_t SigResolver::find(const SigDef& d) {
    auto it = results_.find(d.id);
    if (it != results_.end()) {
        if (it->second.unique()) return it->second.addr;
        auto ra = retry_after_.find(d.id);
        if (ra != retry_after_.end() && GetTickCount64() < ra->second) return 0;
    }
    SigResult r;
    r.id = d.id;
    r.hint_rva = d.hint_rva;
    Pattern p;
    if (!parse_pattern(d.pattern, p)) {
        results_[d.id] = r;
        retry_after_[d.id] = GetTickCount64() + 60000ULL;
        return 0;
    }
    if (!ensure_loaded()) {
        results_[d.id] = r;
        schedule_retry(d.id);
        return 0;
    }
    uint64_t addrs[8]{};
    const size_t n = scan_code(p, addrs, 8);
    r.matches = static_cast<int>(n);
    r.addr = (n == 1) ? addrs[0] : 0;
    results_[d.id] = r;
    if (!r.unique()) schedule_retry(d.id);
    return r.addr;
}

std::string SigResolver::describe() {
    std::string out;
    const ImageInfo& im = image();
    if (!im.valid) return "image: PE header not readable\r\n";
    char b[320];
    const KnownBuild* kb = identify_build(im);
    std::time_t ts = static_cast<std::time_t>(im.timestamp);
    std::tm tmv{};
    gmtime_s(&tmv, &ts);
    char date[24]{};
    std::strftime(date, sizeof(date), "%Y-%m-%d", &tmv);
    std::snprintf(b, sizeof(b), "build: %s\r\n", kb ? kb->name : "UNKNOWN (not in the verified list, signatures decide)");
    out += b;
    std::snprintf(b, sizeof(b), "image: timestamp 0x%08X (%s), size 0x%08X, entry 0x%08X, %zu sections\r\n", im.timestamp, date, im.size_of_image, im.entry_rva, im.sections.size());
    out += b;
    out += "code sections:";
    for (const auto& s : im.sections) {
        if (!s.is_code()) continue;
        std::snprintf(b, sizeof(b), " %s(0x%X+0x%X)", s.name.c_str(), s.rva, s.vsize);
        out += b;
    }
    out += "\r\n";
    std::snprintf(b, sizeof(b), "scan coverage: %llu MB read from executable sections, %llu KB unreadable, %llu MB skipped as oversized\r\n",
                  static_cast<unsigned long long>(cov_.scanned >> 20), static_cast<unsigned long long>(cov_.unreadable >> 10),
                  static_cast<unsigned long long>(cov_.skipped >> 20));
    out += b;
    for (const auto& [id, r] : results_) {
        std::string line = "sig " + id + ": ";
        if (r.unique()) {
            const uint32_t rva = static_cast<uint32_t>(r.addr - im.base);
            line += "unique at RVA " + hex(rva);
            if (r.hint_rva != 0) line += (rva == r.hint_rva) ? " (as in the reference build)" : " (reference build has " + hex(r.hint_rva) + ")";
        } else if (r.matches == 0) {
            line += "NOT FOUND";
        } else {
            line += "AMBIGUOUS, " + std::to_string(r.matches) + " matches";
        }
        out += line + "\r\n";
    }
    return out;
}

} // namespace pes
