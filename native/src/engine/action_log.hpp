#pragma once
// Registro local das acoes do app (uma linha por aviso mostrado ao usuario), para reproduzir problemas.
// Fica em Documentos\Lettuce Live Editor\Logs, nunca sai do computador, gira em 1 MB e pode ser desligado.

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace pes::actionlog {

// Define a pasta dos arquivos de log (chamar uma vez na partida, antes de write)
void set_directory(const std::filesystem::path& dir);
void set_enabled(bool enabled);
bool enabled();
std::filesystem::path directory();

// level: "INFO", "ERROR" etc. Seguro entre threads. Quebras de linha viram espacos.
void write(std::string_view level, std::string_view message);

// Formata uma linha de log (sem gravar): usado pelo write e pelos testes
std::string format_line(std::string_view timestamp, std::string_view level, std::string_view message);

inline constexpr uint64_t kRotateBytes = 1024ULL * 1024ULL;

} // namespace pes::actionlog
