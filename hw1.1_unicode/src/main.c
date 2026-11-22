#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include "tables.h"

static inline bool is_surrogate(uint32_t cp) { /* #define IS_SURROGATE(cp) ((cp) >= 0xD800 && (cp) <= 0xDFFF) */
    return cp >= 0xD800 && cp <= 0xDFFF;
}

static inline bool is_out_of_range(uint32_t cp) {
    return cp > 0x10FFFF;
}

typedef enum {
    ENC_CP1251        = 0,
    ENC_ISO_8859_5    = 1,
    ENC_KOI8R         = 2,
    ENC_COUNT
} encoding_t;

static const uint16_t *const encoding_tables[ENC_COUNT] = {
    [ENC_CP1251]        = cp1251_table,
    [ENC_ISO_8859_5]    = iso8859_5_table,
    [ENC_KOI8R]         = koi8r_table,
};


static int parse_encoding(const char *s, encoding_t *out) {
    if (!strcasecmp(s, "cp1251"))    { *out = ENC_CP1251;     return 0; }
    if (!strcasecmp(s, "iso8859_5")) { *out = ENC_ISO_8859_5; return 0; }
    if (!strcasecmp(s, "koi8r"))     { *out = ENC_KOI8R;      return 0; }
    return -1;
}

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [OPTIONS] INPUT\n"
        "Convert INPUT from a legacy Cyrillic encoding to UTF-8.\n\n"
        "Options:\n"
        "  -e, --encoding NAME   Input encoding (default: cp1251).\n"
        "                        One of: cp1251, iso-8859-5, koi8r.\n"
        "  -o, --output FILE     Output file (default: stdout).\n"
        "  -h, --help            Show this help and exit.\n",
        prog);
}

int main(int argc, char *argv[])
{
    encoding_t enc = ENC_CP1251;
    const char *infile = NULL;
    const char *outfile = "out.txt";

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(argv[0]);
            return 0;
        } else if (!strcmp(a, "-e") || !strcmp(a, "--encoding")) {
            if (++i >= argc) { fprintf(stderr, "error: -e requires a value\n"); return 2; }
            if (parse_encoding(argv[i], &enc) != 0) {
                fprintf(stderr, "error: unknown encoding '%s'\n", argv[i]);
                return 2;
            }
        } else if (!strcmp(a, "-o") || !strcmp(a, "--output")) {
            if (++i >= argc) { fprintf(stderr, "error: -o requires a value\n"); return 2; }
            outfile = argv[i];
            // - после -o — писать в stdout
        } else if (a[0] == '-' && a[1] != '\0') {
            fprintf(stderr, "error: unknown option '%s'\n", a);
            return 2;
            // POSIX Utility Syntax Guidelines (Guideline 13)
            // Если программа ожидает входной файл, - означает stdin (стандартный ввод)
            //   tar -xzf -: извлекает архив, который читается из stdin
            //   cat -     : читает из stdin и выводит в stdout (эквивалентно cat /dev/stdin)
            // Если программа ожидает выходной файл, - означает stdout (стандартный вывод).
            //   wget -O - <URL>: -O задаёт выходной файл. - здесь означает писать в stdout

            // myprog - -o - — конвейер, поэтому возможно: cat in.txt | myprog - -o - > out.txt
        } else {
            // - в позиции infile — читать из stdin
            if (infile) {
                fprintf(stderr, "error: unexpected argument '%s'\n", a);
                return 2;
            }
            infile = a;
        }
    }

    if (!infile) { usage(argv[0]); return 2; }

    if (strcmp(infile, outfile) == 0 && strcmp(infile, "-") != 0) { // Защита от открытия одного и того же файла и на чтение, и на запись
        fprintf(stderr, "error: input and output are the same file: %s\n", infile);
        return 2;
        // 2, не 1 — Это ошибка использования (пользователь неверно вызвал программу),
        // а не ошибка выполнения (не открылся файл, упало чтение).
        // Соглашение: 2 — ошибка аргументов, 1 — ошибка I/O. Сравнение имён — это про аргументы.

// Если нужна полная защита, есть два пути:
// stat обоих путей и сравнение st_ino + st_dev. Совпали — тот же файл (учитывает ./, симлинки, хардлинки, регистр, если ФС так устроена). Не открывайте out, пока не убедились. Минус: stat может не сработать для - (поток), и нужна отдельная ветка; для несуществующего outfile stat вернёт ошибку — это нормально, файла ещё нет, усечения не будет.
// Писать во временный файл + rename в конце. fopen(tmp, "wb"), работа, fclose(tmp), rename(tmp, outfile). rename атомарен, исходный outfile не тронут, пока новый не готов. Защищает от всех алиасов и заодно от падения посреди записи (старый файл остаётся целым). Дороже по коду, но это правильное решение для утилиты, которая может затереть данные.
    }

    FILE *in  = (strcmp(infile,  "-") == 0) ? stdin  : fopen(infile,  "rb"); // rb: бинарный режим, без преобразования окончаний строк.
    // Сообщаем причину ошибки открытия файла.
    if (!in) {
        perror(infile); return 1;
    }

    FILE *out = (strcmp(outfile, "-") == 0) ? stdout : fopen(outfile, "wb"); // усекает или создает новый
    if (!out) {
        perror(outfile);
        if (in != stdin) fclose(in);
        return 1;
    }

    unsigned char buffer[1024];         // Буфер на стеке: 1024 байта.
    unsigned char output_buffer[1024];  // Выходной буфер

    size_t bytes_read;                  // fread возвращает количество успешно прочитанных элементов.

    int rc = 0;

    int current_output_pos = 0;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        for (size_t i = 0; i < bytes_read; ++i) {
            // printf("%02X ", buffer[i]); // Демонстрация: выводим каждый байт в HEX.
            current_output_pos = current_output_pos % sizeof(output_buffer);
            unsigned char current_byte = buffer[i];
            printf("byte in file: %d\n", current_byte);
            // 208 = D0 = 11010000
            uint8_t high = (current_byte & 0xF0) >> 4; // 0x0D = 1101 = D = 13
            uint8_t low  = current_byte & 0x0F;        // 0x00 = 0000 = 0 = 0

            printf("high half byte: %d\n", (high & 0x0F));
            printf("low half byte: %d\n", (low & 0xF0));

            uint32_t unicode_code_point = 0;

            if (current_byte < 0x80) {
                unicode_code_point = current_byte;
            } else {
                int effective_row = high - rows; // поскольку строки таблицы соответствия начинаются не с 0
                int effective_col = low;

                int effective_pos = effective_row * cols + effective_col;
                if (effective_pos > 127) {
                    printf("skip: %u, (0x04%x)", current_byte, current_byte);
                    continue;
                } else {
                    unicode_code_point = koi8r_table[effective_pos];
                    printf("unicode code point U+%04X\n\n", unicode_code_point);
                }
            }

            if (is_surrogate(unicode_code_point)) {
                printf("skip:\n  %u, (0x04%x)", current_byte, current_byte);
                printf("  surrogate unicode code point U+%04X\n\n", unicode_code_point);
                continue;
            } else if (is_out_of_range(unicode_code_point)) {
                printf("skip:\n  %u, (0x04%x)", current_byte, current_byte);
                printf("  out of range unicode code point U+%04X\n\n", unicode_code_point);
                continue;
            }

            // вычислять количество байт, кодирующих code point; или просто принять 2 сразу

            ++current_output_pos;
            output_buffer[current_output_pos] = current_byte;

            if (fwrite(buffer, 1, bytes_read, out) != bytes_read) {
                perror("fwrite");
                rc = 1;
                break;
            }
        }
    }

    if (rc == 0) {
        if (ferror(in)) { perror("fread"); rc = 1; } /* ошибка чтения */
        else if (feof(in)) { /* нормальный конец файла, ничего не делаем */ }
        else               { /* сюда попасть не должны */ }
    }

    if (in != stdin && fclose(in) == EOF) {
        perror("fclose(in)");
        rc = 1;
    }

    if (out == stdout) {
        if (fflush(stdout) == EOF) { perror("fflush(stdout)"); rc = 1; }
    } else if (fclose(out) == EOF) {
        perror("fclose(out)");
        rc = 1;
    }

    return rc;
}

/*
 * KOI8-R (Russian)
 *
 * Lookup table:
 *   - Row    = high 4 bits of the byte.
 *   - Column = low 4 bits of the byte.
 *   - Cell   = Unicode code point of the corresponding character.
 *
 * Examples:
 *   0x80 -> U+2500  (─)
 *   0x8A -> U+253C  (┼)
 *   0xC0 -> U+044E  (ю)
 *   0xE1 -> U+0410  (А)
 *
 * To find a character:
 *   1. Split the byte into two hexadecimal digits.
 *   2. Use the first digit to select the row.
 *   3. Use the second digit to select the column.
 *
 * Example: byte 0xC4
 *   Row    = C
 *   Column = 4
 *   Result = U+0434 (д)
 *
 *          .0      .1      .2      .3      .4      .5      .6      .7      .8      .9      .A        .B      .C      .D      .E      .F
 * 0 - 7    ascii---------------------------------------------------------------------------------------------------------------------table
 * 8.       ─ 2500  │ 2502  ┌ 250C  ┐ 2510  └ 2514  ┘ 2518  ├ 251C  ┤ 2524  ┬ 252C  ┴ 2534  ┼ 253C    ▀ 2580  ▄ 2584  █ 2588  ▌ 258C  ▐ 2590
 * 9.       ░ 2591  ▒ 2592  ▓ 2593  ⌠ 2320  ■ 25A0  ∙ 2219  √ 221A  ≈ 2248  ≤ 2264  ≥ 2265  NBSP 00A0 ⌡ 2321  ° 00B0  ² 00B2  · 00B7  ÷ 00F7
 * A.       ═ 2550  ║ 2551  ╒ 2552  ё 0451  ╓ 2553  ╔ 2554  ╕ 2555  ╖ 2556  ╗ 2557  ╘ 2558  ╙ 2559    ╚ 255A  ╛ 255B  ╜ 255C  ╝ 255D  ╞ 255E
 * B.       ╟ 255F  ╠ 2560  ╡ 2561  Ё 0401  ╢ 2562  ╣ 2563  ╤ 2564  ╥ 2565  ╦ 2566  ╧ 2567  ╨ 2568    ╩ 2569  ╪ 256A  ╫ 256B  ╬ 256C  © 00A9
 * C.       ю 044E  а 0430  б 0431  ц 0446  д 0434  е 0435  ф 0444  г 0433  х 0445  и 0438  й 0439    к 043A  л 043B  м 043C  н 043D  о 043E
 * D.       п 043F  я 044F  р 0440  с 0441  т 0442  у 0443  ж 0436  в 0432  ь 044C  ы 044B  з 0437    ш 0448  э 044D  щ 0449  ч 0447  ъ 044A
 * E.       Ю 042E  А 0410  Б 0411  Ц 0426  Д 0414  Е 0415  Ф 0424  Г 0413  Х 0425  И 0418  Й 0419    К 041A  Л 041B  М 041C  Н 041D  О 041E
 * F.       П 041F  Я 042F  Р 0420  С 0421  Т 0422  У 0423  Ж 0416  В 0412  Ь 042C  Ы 042B  З 0417    Ш 0428  Э 042D  Щ 0429  Ч 0427  Ъ 042A
 */