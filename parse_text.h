#ifndef PARSE_TEXT_H_INCLUDED
#define PARSE_TEXT_H_INCLUDED

#include "codepage.h"

#define WORD_SIZE 401
#define WORD_SIZE_OVERFLOW 400

byte load_text_file(char filename[], sqlite3 *db);

void count_words(sqlite3 *db);
int count_words_callback(void *null_data, int argc, char **argv, char **argcol);

void parse_text_loop(FILE *fp, int count[], sqlite3 *db);
int decode_utf_8(FILE *fp, wchar_t c_buffer[]);
void EOF_block(byte overflow, byte *word_i, int count[], sqlite3 *db);
void fetch_block(wchar_t c_buffer[], byte *overflow, byte *word_i, byte *is_fetched, byte *is_first_wchar_fetched);

void print_word(byte *word_i, int count[], byte *is_sentence_started, sqlite3 *db);
void handle_leftover(byte word_i, sbyte leftover, wchar_t leftover_char);

sbyte check_word(sbyte *leftover, wchar_t *leftover_char, wchar_t leftover_string[], int count[], byte *is_sentence_started, sqlite3 *db);
byte check_leftover(sbyte *leftover, wchar_t *leftover_char, wchar_t leftover_string[]);

byte find_word(int count[], sqlite3 *db, byte buffer_type);
int find_word_callback(void *null_data, int argc, char **argv, char **argcol);

HANDLE _h;
byte _table_id, _find_word_query_result, _colour_standard, _colour_green, _colour_cyan, _colour_red;
char _database_id[3], _table_name[3][21];
wchar_t _original_word_buffer[WORD_SIZE], _word_buffer[WORD_SIZE], _upper_word_buffer[WORD_SIZE], _full_upper_word_buffer[WORD_SIZE];
int _database_size[6];

#endif
