#pragma once
// Retencao dos backups JSON do elenco (Packs\Backups). So mexe em arquivos Backup_*.json dessa pasta.
// A poda e opt-in: com keep <= 0 nada e apagado, e o backup recem criado nunca entra na poda.

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace pes::backups {

struct Entry {
    std::filesystem::path path;
    uint64_t size = 0;
    std::filesystem::file_time_type mtime{};
};

// Opcoes do seletor de retencao (0 = ilimitado)
inline constexpr int kKeepOptions[5] = {0, 10, 20, 50, 100};
inline constexpr int kKeepOptionCount = 5;

// Nome no formato Backup_<tag>_<data>_<n>.json (case-insensitive na extensao)
bool is_backup_name(const std::filesystem::path& file_name);

// Backups da pasta (so arquivos regulares que casam com o nome), do mais novo para o mais antigo
std::vector<Entry> list(const std::filesystem::path& dir);

uint64_t total_bytes(const std::vector<Entry>& entries);

// Remove os mais antigos ate sobrar `keep`. O arquivo `protect` nunca e removido.
// Retorna quantos foram removidos. keep <= 0 nao remove nada.
int prune(const std::filesystem::path& dir, int keep, const std::filesystem::path& protect = {});

} // namespace pes::backups
