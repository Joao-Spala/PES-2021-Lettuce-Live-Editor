#pragma once
// Pastas de dados do aplicativo (ajustes, packs, backups e logs) e migracao das pastas antigas.
//
// Ate a v14.0 os dados ficavam em "Documentos\PES 2021 Workbench" e "%APPDATA%\PES2021Workbench".
// Com o nome Lettuce Live Editor as duas passam a "Lettuce Live Editor". A pasta antiga e movida uma unica vez,
// com os arquivos dentro (packs, backups, logs e settings.ini), para ninguem perder o que ja tinha.

#include <filesystem>

namespace pes::dirs {

inline constexpr const wchar_t* kAppFolder = L"Lettuce Live Editor";
inline constexpr const wchar_t* kLegacyDocsFolder = L"PES 2021 Workbench";   // dentro de Documentos
inline constexpr const wchar_t* kLegacyDataFolder = L"PES2021Workbench";     // dentro de %APPDATA%

// Retorna a pasta a usar dentro de `parent`.
//  - Se a nova ja existe, usa a nova e nao toca na antiga.
//  - Se so a antiga existe, tenta renomea-la para o nome novo. Se o Windows recusar (arquivo em uso, permissao),
//    segue usando a antiga: perder dados e pior do que ficar com o nome velho.
//  - Se nenhuma existe, devolve o caminho novo (o chamador cria quando precisar).
inline std::filesystem::path migrate_dir(const std::filesystem::path& parent, const wchar_t* new_name, const wchar_t* old_name) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path novo = parent / new_name;
    const fs::path antigo = parent / old_name;
    if (fs::exists(novo, ec)) return novo;
    ec.clear();
    if (fs::is_directory(antigo, ec)) {
        ec.clear();
        fs::rename(antigo, novo, ec);
        if (!ec) return novo;
        return antigo;
    }
    return novo;
}

} // namespace pes::dirs
