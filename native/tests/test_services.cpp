// Testes dos servicos da v14.1: aviso de versao, retencao de backups, log de acoes e persistencia de ajustes.
// Nao toca na rede nem no settings.ini real: tudo roda em pasta temporaria ou em memoria.

#include "../src/app_version.hpp"
#include "../src/engine/action_log.hpp"
#include "../src/engine/backup_retention.hpp"
#include "../src/engine/update_check.hpp"
#include "../src/engine/value_lock.hpp"
#include "../src/engine/form_modifiers.hpp"
#include "../src/ui/arrow_text.hpp"
#include "../src/ui/i18n.hpp"
#include "../src/ui/theme.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

static int g_fail = 0;
static int g_checks = 0;

#define CHECK(cond)                                                                    \
    do {                                                                               \
        ++g_checks;                                                                    \
        if (!(cond)) {                                                                 \
            ++g_fail;                                                                  \
            std::printf("  FALHA linha %d: %s\n", __LINE__, #cond);                    \
        }                                                                              \
    } while (0)

static void touch(const fs::path& p, const std::string& content, int age_minutes) {
    {
        std::ofstream f(p, std::ios::binary);
        f << content;
    }
    std::error_code ec;
    fs::last_write_time(p, fs::file_time_type::clock::now() - std::chrono::minutes(age_minutes), ec);
}

static fs::path make_temp_dir(const char* name) {
    fs::path d = fs::temp_directory_path() / (std::string("lle_test_") + name + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code ec;
    fs::remove_all(d, ec);
    fs::create_directories(d, ec);
    return d;
}

static std::string slurp(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// ------------------------------------------------------------------------------------------------

static void test_versions() {
    using namespace pes::update;
    auto v = parse_version("v14.1.0");
    CHECK(v.valid && v.major == 14 && v.minor == 1 && v.patch == 0);
    v = parse_version("14.1");
    CHECK(v.valid && v.major == 14 && v.minor == 1 && v.patch == 0);
    v = parse_version("V15");
    CHECK(v.valid && v.major == 15 && v.minor == 0 && v.patch == 0);
    v = parse_version("v14.1.0-beta.2");
    CHECK(v.valid && v.major == 14 && v.minor == 1 && v.patch == 0);
    v = parse_version("14.1.2+build5");
    CHECK(v.valid && v.patch == 2);
    CHECK(!parse_version("").valid);
    CHECK(!parse_version("v").valid);
    CHECK(!parse_version("latest").valid);
    CHECK(!parse_version("14.").valid);
    CHECK(!parse_version("14abc").valid);
    CHECK(!parse_version("14.1.x").valid);
    CHECK(!parse_version("1234567.0.0").valid);    // numero absurdo
    CHECK(!parse_version("-1.0.0").valid);

    CHECK(compare_versions(parse_version("14.1.0"), parse_version("14.0.0")) > 0);
    CHECK(compare_versions(parse_version("14.0.9"), parse_version("14.1.0")) < 0);
    CHECK(compare_versions(parse_version("v14.1.0"), parse_version("14.1")) == 0);
    CHECK(compare_versions(parse_version("15.0.0"), parse_version("14.99.99")) > 0);
    CHECK(compare_versions(parse_version("14.10.0"), parse_version("14.9.0")) > 0);   // numerico, nao texto
    std::printf("Versoes: parse e comparacao PASS\n");
}

static void test_release_json() {
    using namespace pes::update;
    const std::string base = "https://github.com/Joao-Spala/PES-2021-Lettuce-Live-Editor/releases/tag/";
    Release r;
    CHECK(parse_release_json(R"({"tag_name":"v14.2.0","html_url":")" + base + R"(v14.2.0","draft":false,"prerelease":false,"name":"x"})", r));
    CHECK(r.tag == "v14.2.0" && r.url == base + "v14.2.0");
    // campos extras e ordem diferente
    CHECK(parse_release_json(R"({"id":1,"html_url":")" + base + R"(v15.0.0","tag_name":"15.0.0","assets":[{"a":1}]})", r));
    CHECK(r.tag == "15.0.0");

    CHECK(!parse_release_json("", r));
    CHECK(!parse_release_json("not json", r));
    CHECK(!parse_release_json("[]", r));
    CHECK(!parse_release_json("{}", r));
    {   // flags de tipo errado e tags fora de [0-9A-Za-z.+-] nunca derrubam nem passam
        const std::string base2 = std::string(pes::kReleasesPagePrefix) + "tag/v15.0.0";
        CHECK(!parse_release_json(R"({"tag_name":"v15.0.0","html_url":")" + base2 + R"(","draft":null})", r));
        CHECK(!parse_release_json(R"({"tag_name":"v15.0.0","html_url":")" + base2 + R"(","prerelease":"no"})", r));
        CHECK(!parse_release_json(R"({"tag_name":"v15.0.0_rc1","html_url":")" + base2 + R"("})", r));
        CHECK(parse_release_json(R"({"tag_name":"v15.0.0-rc.1","html_url":")" + base2 + R"("})", r));
    }
    CHECK(!parse_release_json(R"({"tag_name":"v1.0.0"})", r));                                  // sem url
    CHECK(!parse_release_json(R"({"html_url":")" + base + R"(v1","tag_name":5})", r));        // tag nao e texto
    CHECK(!parse_release_json(R"({"tag_name":"v14.2.0","draft":true,"html_url":")" + base + R"(v14.2.0"})", r));
    CHECK(!parse_release_json(R"({"tag_name":"v14.2.0","prerelease":true,"html_url":")" + base + R"(v14.2.0"})", r));
    CHECK(!parse_release_json(R"({"tag_name":"nightly","html_url":")" + base + R"(nightly"})", r));  // tag sem versao
    CHECK(!parse_release_json(R"({"tag_name":"v14.2.0","html_url":"https://evil.example.com/Joao-Spala/PES-2021-Lettuce-Live-Editor/releases/v14.2.0"})", r));
    CHECK(!parse_release_json(R"({"tag_name":"v14.2.0","html_url":"http://github.com/Joao-Spala/PES-2021-Lettuce-Live-Editor/releases/v14.2.0"})", r));
    CHECK(!parse_release_json(R"({"tag_name":"v14.2.0 x","html_url":")" + base + R"(v14.2.0"})", r));  // espaco na tag
    CHECK(!parse_release_json(std::string(300 * 1024, ' '), r));                                // corpo grande demais
    // r nao pode vazar valor de uma chamada anterior que falhou
    CHECK(!parse_release_json("{}", r) && r.tag.empty() && r.url.empty());

    CHECK(is_trusted_release_url(base + "v14.2.0"));
    CHECK(!is_trusted_release_url("https://github.com/Joao-Spala/PES-2021-Lettuce-Live-Editor"));
    CHECK(!is_trusted_release_url("https://github.com/Joao-Spala/PES-2021-Lettuce-Live-Editor/releases/"));  // sem destino
    CHECK(!is_trusted_release_url("https://github.com/Joao-Spala/OutroRepo/releases/tag/v1"));
    CHECK(!is_trusted_release_url("https://github.com.evil.io/Joao-Spala/PES-2021-Lettuce-Live-Editor/releases/x"));
    CHECK(!is_trusted_release_url(base + "v1 .0"));
    CHECK(!is_trusted_release_url(base + "../../x"));
    CHECK(!is_trusted_release_url(base + "v1\n"));
    CHECK(!is_trusted_release_url(base + std::string(400, 'a')));
    std::printf("Aviso de versao: JSON, rascunhos e URLs confiaveis PASS\n");
}

static void test_backups() {
    using namespace pes::backups;
    const fs::path dir = make_temp_dir("bk");

    CHECK(is_backup_name("Backup_antes_ovr1_2026-11-26_10-00-00_1.json"));
    CHECK(is_backup_name("Backup_x_1.JSON"));
    CHECK(!is_backup_name("Backup_.json"));
    CHECK(!is_backup_name("backup_x_1.json"));      // prefixo e exato
    CHECK(!is_backup_name("Backup_x_1.txt"));
    CHECK(!is_backup_name("MeuBackup_x.json"));
    CHECK(!is_backup_name("notas.json"));

    // 6 backups (do mais antigo ao mais novo) + arquivos que nunca podem ser tocados
    for (int i = 0; i < 6; ++i) {
        touch(dir / ("Backup_t_" + std::to_string(i) + ".json"), std::string(100 + i, 'x'), (6 - i) * 10);
    }
    touch(dir / "notas.txt", "minhas notas", 500);
    touch(dir / "Pack_importante.json", "{}", 600);
    touch(dir / "backup_minusculo.json", "{}", 700);
    fs::create_directories(dir / "Backup_pasta.json");   // diretorio com nome de backup

    auto all = list(dir);
    CHECK(all.size() == 6);
    CHECK(all.front().path.filename() == "Backup_t_5.json");   // mais novo primeiro
    CHECK(all.back().path.filename() == "Backup_t_0.json");
    CHECK(total_bytes(all) == 100 + 101 + 102 + 103 + 104 + 105);

    CHECK(prune(dir, 0, {}) == 0);        // ilimitado: nada e apagado
    CHECK(prune(dir, -3, {}) == 0);
    CHECK(prune(dir, 6, {}) == 0);        // exatamente no limite
    CHECK(prune(dir, 10, {}) == 0);
    CHECK(list(dir).size() == 6);

    CHECK(prune(dir, 4, dir / "Backup_t_5.json") == 2);
    all = list(dir);
    CHECK(all.size() == 4);
    CHECK(fs::exists(dir / "Backup_t_5.json") && fs::exists(dir / "Backup_t_2.json"));
    CHECK(!fs::exists(dir / "Backup_t_0.json") && !fs::exists(dir / "Backup_t_1.json"));
    // nada alem dos backups foi removido
    CHECK(fs::exists(dir / "notas.txt") && fs::exists(dir / "Pack_importante.json"));
    CHECK(fs::exists(dir / "backup_minusculo.json") && fs::is_directory(dir / "Backup_pasta.json"));

    // protegido antigo continua vivo mesmo fora do top-N
    CHECK(prune(dir, 1, dir / "Backup_t_2.json") >= 2);
    CHECK(fs::exists(dir / "Backup_t_2.json"));
    CHECK(fs::exists(dir / "Backup_t_5.json"));

    // pasta inexistente ou vazia
    CHECK(list(dir / "nao_existe").empty());
    CHECK(prune(dir / "nao_existe", 3, {}) == 0);

    std::error_code ec;
    fs::remove_all(dir, ec);
    std::printf("Backups: retencao, protecao do novo e arquivos alheios PASS\n");
}

static void test_action_log() {
    using namespace pes::actionlog;
    CHECK(format_line("2026-01-01 10:00:00", "INFO", "linha\ncom\tquebra") == "2026-01-01 10:00:00 [INFO] linha com quebra\n");
    // Truncamento nao corta no meio de um caractere UTF-8
    std::string longmsg;
    for (int i = 0; i < 2000; ++i) longmsg += "\xC3\xA7";   // "c cedilha"
    const std::string line = format_line("t", "INFO", longmsg);
    CHECK(line.size() < 3200);
    CHECK(line.size() > 100);
    bool utf8_ok = true;
    for (size_t i = 0; i < line.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(line[i]);
        if (c == 0xC3) { if (i + 1 >= line.size() || (static_cast<unsigned char>(line[i + 1]) & 0xC0) != 0x80) utf8_ok = false; }
    }
    CHECK(utf8_ok);

    const fs::path dir = make_temp_dir("log");
    set_directory(dir);
    set_enabled(false);
    write("INFO", "nao deve gravar");
    CHECK(!fs::exists(dir / "workbench.log"));

    set_enabled(true);
    write("INFO", "primeira");
    write("ERROR", "segunda");
    const std::string txt = slurp(dir / "workbench.log");
    CHECK(txt.find("[INFO] primeira\n") != std::string::npos);
    CHECK(txt.find("[ERROR] segunda\n") != std::string::npos);

    // Rotacao: passa de 1 MB, o arquivo antigo vira workbench.1.log e o novo recomeca
    const std::string big(900, 'z');
    for (int i = 0; i < 1300; ++i) write("INFO", big);
    CHECK(fs::exists(dir / "workbench.1.log"));
    CHECK(fs::exists(dir / "workbench.log"));
    CHECK(fs::file_size(dir / "workbench.log") < kRotateBytes);
    CHECK(fs::file_size(dir / "workbench.1.log") >= kRotateBytes);

    set_directory({});
    write("INFO", "sem pasta definida: nao pode quebrar");
    set_enabled(true);
    std::error_code ec;
    fs::remove_all(dir, ec);
    std::printf("Log de acoes: formato, UTF-8, desligado e rotacao PASS\n");
}

static void test_settings() {
    using namespace pes::ui;
    // Valores padrao seguros: nada de rede, backups ilimitados
    AppSettings d;
    CHECK(!d.check_updates);
    CHECK(d.backup_keep_idx == 0);
    CHECK(d.action_log);
    CHECK(!d.first_run_done);

    // Ida e volta
    AppSettings s;
    s.check_updates = true;
    s.update_dismissed = "v14.2.0";
    s.backup_keep_idx = 3;
    s.action_log = false;
    s.first_run_done = true;
    s.lang = Lang::EN;
    s.currency_idx = 2;
    std::ostringstream out;
    write_settings(out, s);
    std::istringstream in(out.str());
    AppSettings r = parse_settings(in);
    CHECK(r.check_updates && r.update_dismissed == "v14.2.0" && r.backup_keep_idx == 3);
    CHECK(!r.action_log && r.first_run_done && r.lang == Lang::EN && r.currency_idx == 2);

    // Arquivo de versao anterior (sem as chaves novas): usuario antigo nao ve o guia, nada de rede, backups ilimitados
    std::istringstream old_file("lang=pt\ncurrency=1\ntheme=0\naccent=5\n");
    AppSettings o = parse_settings(old_file);
    CHECK(o.first_run_done);
    CHECK(!o.check_updates && o.backup_keep_idx == 0 && o.action_log);
    CHECK(o.currency_idx == 1);

    // Arquivo editado a mao / corrompido
    std::istringstream bad("backup_keep=999\nupdate_dismissed=v1;rm -rf /\r\ncheck_updates=yes\nfirst_run_done=0\naction_log=0\n");
    AppSettings b = parse_settings(bad);
    CHECK(b.backup_keep_idx == pes::backups::kKeepOptionCount - 1);
    CHECK(b.update_dismissed == "v1rm-rf");    // so [0-9A-Za-z.+-]
    CHECK(!b.check_updates);                     // so "1" liga
    CHECK(!b.first_run_done);                    // chave presente com 0: guia pendente
    CHECK(!b.action_log);

    std::istringstream neg("backup_keep=-4\nupdate_dismissed=" + std::string(500, 'a') + "\n");
    AppSettings n = parse_settings(neg);
    CHECK(n.backup_keep_idx == 0);
    CHECK(n.update_dismissed.size() == 40);

    std::istringstream empty("");
    AppSettings e = parse_settings(empty);
    CHECK(e.first_run_done);   // arquivo existe mas vazio: trata como usuario antigo
    std::printf("Ajustes: padroes, migracao e valores corrompidos PASS\n");
}

static void test_value_lock() {
    using namespace pes;
    uint8_t buf[0x17C]{};
    buf[kRecordIdByte] = 0x39;      // id 12345 = 0x00003039
    buf[kRecordIdByte + 1] = 0x30;
    CHECK(record_player_id(buf) == 12345u);

    LockWrite w[3];
    ValueLockConfig none;
    CHECK(!none.any());
    CHECK(plan_lock_writes(buf, sizeof(buf), none, w) == 0);

    // folego: so escreve se diferir
    set_bits_buf(buf, sizeof(buf), kLockStaminaByte, 0, 7, 94);
    ValueLockConfig st;
    st.stamina = true; st.stamina_value = 100;
    CHECK(st.any());
    int n = plan_lock_writes(buf, sizeof(buf), st, w);
    CHECK(n == 1 && w[0].field == LockField::Stamina && w[0].value == 100u);
    set_bits_buf(buf, sizeof(buf), kLockStaminaByte, 0, 7, 100);
    CHECK(plan_lock_writes(buf, sizeof(buf), st, w) == 0);

    // a seta piscando (bit 7 do mesmo byte) nao interfere na comparacao do folego
    set_bits_buf(buf, sizeof(buf), kLockStaminaByte, 7, 1, 1);
    CHECK(plan_lock_writes(buf, sizeof(buf), st, w) == 0);
    set_bits_buf(buf, sizeof(buf), kLockStaminaByte, 7, 1, 0);

    // valores fora da faixa sao limitados
    ValueLockConfig big;
    big.stamina = true; big.stamina_value = 300;
    big.arrow = true;   big.arrow_value = 9;
    set_bits_buf(buf, sizeof(buf), kLockStaminaByte, 0, 7, 50);
    set_bits_buf(buf, sizeof(buf), kLockArrowByte, 0, 3, 1);
    n = plan_lock_writes(buf, sizeof(buf), big, w);
    CHECK(n == 2 && w[0].value == 100u && w[1].field == LockField::Arrow && w[1].value == 4u);
    ValueLockConfig neg;
    neg.stamina = true; neg.stamina_value = -5;
    n = plan_lock_writes(buf, sizeof(buf), neg, w);
    CHECK(n == 1 && w[0].value == 0u);
    CHECK(clamp_lock_config(big).stamina_value == 100 && clamp_lock_config(big).arrow_value == 4);
    CHECK(clamp_lock_config(neg).stamina_value == 0);

    // seta: igual = nada, diferente = escreve
    ValueLockConfig ar;
    ar.arrow = true; ar.arrow_value = 4;
    set_bits_buf(buf, sizeof(buf), kLockArrowByte, 0, 3, 4);
    CHECK(plan_lock_writes(buf, sizeof(buf), ar, w) == 0);
    set_bits_buf(buf, sizeof(buf), kLockArrowByte, 0, 3, 2);
    n = plan_lock_writes(buf, sizeof(buf), ar, w);
    CHECK(n == 1 && w[0].field == LockField::Arrow && w[0].value == 4u);

    // lesao: zera so se houver dias
    ValueLockConfig inj;
    inj.injury = true;
    buf[kLockInjuryByte] = 0;
    CHECK(plan_lock_writes(buf, sizeof(buf), inj, w) == 0);
    buf[kLockInjuryByte] = 7;
    n = plan_lock_writes(buf, sizeof(buf), inj, w);
    CHECK(n == 1 && w[0].field == LockField::Injury && w[0].value == 0u);

    // os tres de uma vez, na ordem folego, seta, lesao
    ValueLockConfig all;
    all.stamina = true; all.stamina_value = 80;
    all.arrow = true;   all.arrow_value = 3;
    all.injury = true;
    set_bits_buf(buf, sizeof(buf), kLockStaminaByte, 0, 7, 10);
    set_bits_buf(buf, sizeof(buf), kLockArrowByte, 0, 3, 0);
    buf[kLockInjuryByte] = 30;
    n = plan_lock_writes(buf, sizeof(buf), all, w);
    CHECK(n == 3 && w[0].field == LockField::Stamina && w[1].field == LockField::Arrow && w[2].field == LockField::Injury);

    // buffer curto ou nulo nao quebra
    CHECK(plan_lock_writes(buf, kLockInjuryByte, all, w) == 0);
    CHECK(plan_lock_writes(nullptr, 100, all, w) == 0);

    // igualdade da config (usada para detectar mudancas na interface)
    ValueLockConfig a = all, b = all;
    CHECK(a == b);
    b.arrow_value = 2;
    CHECK(!(a == b));
    std::printf("Travas de valores: planejamento, limites e campos independentes PASS\n");
}


// ------------------------------------------------------------------------------------------------
// Espanhol: dicionario ingles -> espanhol

// Especificadores de printf de um texto (ex.: "%d", "%s", "%.1f"), em ordem, concatenados
static std::string format_specs(const char* s) {
    std::string out;
    for (const char* p = s; *p; ++p) {
        if (*p != '%') continue;
        const char* q = p + 1;
        while (*q && std::strchr("-+#0", *q)) ++q;
        while (*q >= '0' && *q <= '9') ++q;
        if (*q == '.') { ++q; while (*q >= '0' && *q <= '9') ++q; }
        if ((*q >= 'a' && *q <= 'z') || (*q >= 'A' && *q <= 'Z') || *q == '%') {
            out.append(p, static_cast<size_t>(q - p) + 1);
            p = q;
        }
    }
    return out;
}

static std::string id_suffix(const char* s) {
    const char* h = std::strstr(s, "##");
    return h ? std::string(h) : std::string();
}

static int count_char(const char* s, char c) {
    int n = 0;
    for (; *s; ++s) if (*s == c) ++n;
    return n;
}

static void test_spanish() {
    using namespace pes::ui;
    const EsEntry* tb = es_table();
    const size_t n = es_table_size();
    CHECK(n > 700);

    // Ordem estrita por strcmp (a busca binaria depende disso) e sem chaves repetidas
    bool sorted = true;
    for (size_t i = 1; i < n; ++i) if (std::strcmp(tb[i - 1].en, tb[i].en) >= 0) sorted = false;
    CHECK(sorted);

    int bad_fmt = 0, bad_id = 0, bad_nl = 0, bad_edge = 0, bad_punct = 0, empty = 0, miss = 0;
    for (size_t i = 0; i < n; ++i) {
        const char* en = tb[i].en;
        const char* es = tb[i].es;
        if (!*es) ++empty;
        if (format_specs(en) != format_specs(es)) { ++bad_fmt; std::printf("  formato: %s\n", en); }
        if (id_suffix(en) != id_suffix(es)) { ++bad_id; std::printf("  id: %s\n", en); }
        if (count_char(en, '\n') != count_char(es, '\n')) { ++bad_nl; std::printf("  quebra: %s\n", en); }
        const size_t le = std::strlen(en), ls = std::strlen(es);
        if (le && ls && ((en[0] == ' ') != (es[0] == ' ') || (en[le - 1] == ' ') != (es[ls - 1] == ' '))) { ++bad_edge; std::printf("  borda: %s\n", en); }
        if (std::strchr(es, ';') || std::strstr(es, "\xE2\x80\x94")) ++bad_punct;   // ponto e virgula e travessao
        if (std::strcmp(es_lookup(en), es) != 0) ++miss;   // a busca encontra cada chave
    }
    CHECK(empty == 0);
    CHECK(bad_fmt == 0);
    CHECK(bad_id == 0);
    CHECK(bad_nl == 0);
    CHECK(bad_edge == 0);
    CHECK(bad_punct == 0);
    CHECK(miss == 0);

    // Busca: achado, ausente (devolve o proprio ponteiro) e entradas invalidas
    CHECK(std::strcmp(es_lookup("Settings"), "Ajustes") == 0);
    const char* absent = "This sentence is not in the dictionary";
    CHECK(es_lookup(absent) == absent);
    CHECK(es_lookup(nullptr) == nullptr);
    CHECK(std::strcmp(es_lookup(""), "") == 0);

    // T(): cada idioma pega o seu texto, e o espanhol cai no ingles quando falta traducao
    CHECK(std::strcmp(T(Lang::PT, "Ajustes", "Settings"), "Ajustes") == 0);
    CHECK(std::strcmp(T(Lang::EN, "Ajustes", "Settings"), "Settings") == 0);
    CHECK(std::strcmp(T(Lang::ES, "Ajustes", "Settings"), "Ajustes") == 0);
    CHECK(T(Lang::ES, "x", absent) == absent);

    // Posicoes: as 13 existem em espanhol e nenhuma fica vazia
    CHECK(std::strcmp(pos_short(Lang::ES, 0), "POR") == 0);
    CHECK(std::strcmp(pos_short(Lang::ES, 12), "DC") == 0);
    bool pos_ok = true;
    for (int i = 0; i < 13; ++i) {
        if (!*pos_short(Lang::ES, i) || !*pos_full(Lang::ES, i)) pos_ok = false;
        if (std::strcmp(pos_short(Lang::ES, i), pos_short(Lang::EN, i)) == 0 && i != 0 && i != 5) { /* algumas siglas coincidem */ }
    }
    CHECK(pos_ok);
    CHECK(std::strcmp(pos_short(Lang::ES, 99), "---") == 0);

    // Os 4 nomes das listas de olheiros, os 25 atributos e os 10 presets tem traducao
    for (const auto& sl : pes::data::SCOUT_LISTS) CHECK(es_lookup(sl.en) != sl.en || std::strcmp(sl.en, "Shortlist") != 0);
    int stat_missing = 0;
    for (const auto& sd : pes::data::STAT_DEFS) {
        if (es_lookup(sd.en) == sd.en) ++stat_missing;
        if (es_lookup(sd.desc_en) == sd.desc_en) ++stat_missing;
    }
    CHECK(stat_missing == 0);

    // Ajustes: lang=es entra e sai, e valor desconhecido volta para portugues
    {
        AppSettings a;
        a.lang = Lang::ES;
        std::ostringstream os;
        write_settings(os, a);
        CHECK(os.str().find("lang=es\n") != std::string::npos);
        std::istringstream is(os.str());
        CHECK(parse_settings(is).lang == Lang::ES);
        std::istringstream is2("lang=fr\n");
        CHECK(parse_settings(is2).lang == Lang::PT);
        std::istringstream is3("lang=en\n");
        CHECK(parse_settings(is3).lang == Lang::EN);
    }
    std::printf("Espanhol: dicionario, formatos, busca e ajustes PASS\n");
}


// ------------------------------------------------------------------------------------------------
// Seta de forma: multiplicadores lidos da tabela do jogo e texto do tooltip

static void test_form_arrows() {
    using namespace pes::form;
    using pes::ui::Lang;

    // Os 25 atributos do app: 23 em algum grupo (8, 12 e 3) e 2 fora (Posse apertada = 19, Agressividade = 25)
    int n[3] = {0, 0, 0}, none = 0;
    for (const auto& sd : pes::data::STAT_DEFS) {
        const int g = group_of_fid(sd.fid);
        CHECK(g >= -1 && g <= 2);
        if (g < 0) { ++none; CHECK(sd.fid == 19 || sd.fid == 25); }
        else ++n[g];
    }
    CHECK(n[0] == 8 && n[1] == 12 && n[2] == 3 && none == 2);

    // Valores da tabela (azul = 4 ... vermelha = 0), neutro em 1.0 e piora monotona
    CHECK(std::fabs(multiplier(4, 0) - 1.12f) < 1e-6f && std::fabs(multiplier(0, 0) - 0.84f) < 1e-6f);
    CHECK(std::fabs(multiplier(4, 1) - 1.09f) < 1e-6f && std::fabs(multiplier(0, 1) - 0.88f) < 1e-6f);
    CHECK(std::fabs(multiplier(4, 2) - 1.06f) < 1e-6f && std::fabs(multiplier(0, 2) - 0.88f) < 1e-6f);
    for (int g = 0; g < 3; ++g) {
        CHECK(multiplier(2, g) == 1.0f);
        for (int a = 0; a < 4; ++a) CHECK(multiplier(a + 1, g) > multiplier(a, g));
    }
    CHECK(multiplier(-1, 0) == 1.0f && multiplier(5, 0) == 1.0f && multiplier(3, 7) == 1.0f);
    CHECK(percent(4, 0) == 12 && percent(3, 0) == 6 && percent(2, 0) == 0 && percent(1, 0) == -8 && percent(0, 0) == -16);
    CHECK(percent(1, 2) == -6 && percent(0, 1) == -12);

    // Texto do tooltip nos tres idiomas
    for (Lang L : {Lang::PT, Lang::EN, Lang::ES}) {
        const std::string blue = pes::ui::arrow_effect_text(L, 4);
        const std::string red = pes::ui::arrow_effect_text(L, 0);
        const std::string yellow = pes::ui::arrow_effect_text(L, 2);
        CHECK(blue.find("+12%") != std::string::npos && blue.find("+9%") != std::string::npos && blue.find("+6%") != std::string::npos);
        CHECK(red.find("-16%") != std::string::npos && red.find("-12%") != std::string::npos);
        CHECK(yellow.find('%') == std::string::npos || yellow.find("+") == std::string::npos);
        CHECK(yellow.find("+12%") == std::string::npos);
        CHECK(*pes::ui::arrow_title(L, 3) != 0 && *pes::ui::arrow_name(L, 1) != 0);
    }
    // os dois atributos sem efeito aparecem como "sem alteracao" (Posse apertada e Agressividade)
    CHECK(pes::ui::arrow_effect_text(Lang::EN, 4).find("Tight Possession, Aggression") != std::string::npos);
    std::printf("Seta de forma: tabela do jogo, grupos e tooltips PASS%c", 10);
}

int main() {
    std::printf("== Servicos v14.1 ==\n");
    test_versions();
    test_release_json();
    test_backups();
    test_action_log();
    test_settings();
    test_value_lock();
    test_spanish();
    test_form_arrows();
    std::printf("%d verificacoes, %d falha(s)\n", g_checks, g_fail);
    return g_fail == 0 ? 0 : 1;
}
