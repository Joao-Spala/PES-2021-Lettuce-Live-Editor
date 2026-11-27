#pragma once
// Espanhol da interface: dicionario ingles -> espanhol consultado por T(). Quando falta uma entrada, o
// texto em ingles e usado (nunca um texto inventado). A tabela vem de native/i18n_es.json.

#include <cstddef>

namespace pes::ui {

struct EsEntry {
    const char* en;
    const char* es;
};

// Traducao para espanhol de `en`, ou o proprio `en` se nao houver (ou se for nulo).
const char* es_lookup(const char* en);

// Tabela ordenada por strcmp na chave `en` (usada pelos testes)
const EsEntry* es_table();
size_t es_table_size();

} // namespace pes::ui
