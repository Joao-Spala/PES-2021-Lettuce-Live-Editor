#include "i18n_es.hpp"

#include <cstring>

namespace pes::ui {

namespace {

constexpr EsEntry kTable[] = {
#include "i18n_es_table.inc"
};
constexpr size_t kCount = sizeof(kTable) / sizeof(kTable[0]);

} // namespace

const EsEntry* es_table() { return kTable; }
size_t es_table_size() { return kCount; }

const char* es_lookup(const char* en) {
    if (!en || !*en) return en;
    size_t lo = 0, hi = kCount;
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        const int c = std::strcmp(kTable[mid].en, en);
        if (c == 0) return kTable[mid].es;
        if (c < 0) lo = mid + 1;
        else hi = mid;
    }
    return en;
}

} // namespace pes::ui
