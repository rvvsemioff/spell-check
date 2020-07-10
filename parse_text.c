#include "parse_text.h"

//Loading and parsing utf-8 text files

//Function loads file, initiates work variables and calls count_words and parse_text_loop
//If file wasn't found, appends ".txt" to filename and tries again.
//Returns 1 on success and 0 on failure.
byte load_text_file(char filename[], sqlite3 *db){
    int count[2]={0, 0};
    FILE *fp=fopen(filename, "r");
    if(fp==NULL){
        strcat(filename, ".txt");
        fp=fopen(filename, "r");
        if(fp==NULL) return 1;
    }
    count_words(db);
    parse_text_loop(fp, count, db);
    printf("\n\nParsed %d words, recognized %d (%d%%) words.\n\n", count[0]+count[1], count[0], (int)round(100*(float)count[0]/(float)(count[0]+count[1])));
    return 0;
}

//Function counts quantity of words in every table (excluding "saying" and "other").
//Has callback function count_words_callback which writes quantity to _database_size array.
void count_words(sqlite3 *db){
    char sql[100];
    for(_table_id=0; _table_id<4; ++_table_id){
        sprintf(sql, SQL_COUNT, _table_name[_table_id]);
        sqlite3_exec(db, sql, count_words_callback, NULL, NULL);
    }
}

int count_words_callback(void *null_data, int argc, char **argv, char **argcol){
    _database_size[_table_id]=atoi(argv[0]);
    return 0;
}

//Function parses text, character by character.
//Loads stings of letters for check in latter functions.
void parse_text_loop(FILE *fp, int count[], sqlite3 *db){
    byte overflow=0, is_fetched, is_first_wchar_fetched=0, is_sentence_started=1, word_i=0;
    wchar_t c_buffer[5];
    fseek(fp, 3, SEEK_SET);
    while(1){
        if(decode_utf_8(fp, c_buffer)==EOF){
            //EOF_block - if end of file reached - break the loop.
            if(is_fetched) print_word(&word_i, count, &is_sentence_started, db);
            SetConsoleTextAttribute(_h, _colour_standard);
            break;
        }
        //Fetch block.
        is_fetched=0;
        if(check_letter(c_buffer)) fetch_block(c_buffer, &overflow, &word_i, &is_fetched, &is_first_wchar_fetched);
        //Otherwise, if overflow previously occurred - reset console colour and overflow variable.
        else{
            is_first_wchar_fetched=0;

            if(overflow){
                SetConsoleTextAttribute(_h, _colour_standard);
                overflow=0;
            }
            //If no EOF or letter fetched or overflow occurred - print previously fetched word.
            else if(word_i>0){
                print_word(&word_i, count, &is_sentence_started, db);
                if(check_ending_letter(c_buffer[0])) is_sentence_started=1;
            }
        }

        //If character wasn't fetched to word buffer - print it.
        if(!is_fetched) wprintf(L"%ls", c_buffer);
    }
}

int decode_utf_8(FILE *fp, wchar_t c_buffer[]){
    wchar_t load_buffer[4];
    fscanf(fp, "%lc", &load_buffer[0]);
    //BUG - Should be fwscanf but crashes
    if(feof(fp)) return EOF;
    if(load_buffer[0]<=127) swprintf(c_buffer, 2, L"%c%c", load_buffer[0], L'\0');
    else if(load_buffer[0]>=192&&load_buffer[0]<=223){
        fscanf(fp, "%lc", &load_buffer[1]);
        swprintf(c_buffer, 3, L"%c%c%lc", load_buffer[0], load_buffer[1], L'\0');
    }
    else if(load_buffer[0]<=239){
        fscanf(fp, "%lc%lc", &load_buffer[1], &load_buffer[2]);
        swprintf(c_buffer, 4, L"%lc%lc%lc%lc", load_buffer[0], load_buffer[1], load_buffer[2], L'\0');
    }
    else if(load_buffer[0]<=247){
        fscanf(fp, "%lc%lc%lc", &load_buffer[1], &load_buffer[2], &load_buffer[3]);
        swprintf(c_buffer, 5, L"%lc%lc%lc%lc%lc", load_buffer[0], load_buffer[1], load_buffer[2], load_buffer[3], L'\0');
    }
    return 0;
}

//Function is a part of parse_text_loop.
//Fetches a character to c_buffer and checks EOF condition.
//In the case of EOF:
//  If no overflow occurred - calls print_word.
//  Then resets console colour.
//  Returns 0.
//Otherwise returns 1.
void EOF_block(byte overflow, byte *word_i, int count[], sqlite3 *db){
    if(!overflow) print_word(word_i, count, 0, db);
    SetConsoleTextAttribute(_h, _colour_standard);
}

//Function is a part of parse_text_loop.
//If overflow occurred:
//  Sets colour to _colour_red
//  Prints contents of _original_word_buffer
//  Resets word_i and overflow variables
//If no overflow occurred - loads character to _original_word_buffer and lower case form to _word_buffer.
//P.S. Maybe overflow check should be extracted to upper function
void fetch_block(wchar_t c_buffer[], byte *overflow, byte *word_i, byte *is_fetched, byte *is_first_wchar_fetched){
    if(*word_i==WORD_SIZE_OVERFLOW){
        _original_word_buffer[*word_i]='\0';
        SetConsoleTextAttribute(_h, _colour_red);
        wprintf(L"%ls", _original_word_buffer);
        *word_i=0;
        wcscpy(_original_word_buffer, L"\0");
        wcscpy(_word_buffer, L"\0");
        wcscpy(_upper_word_buffer, L"\0");
        *overflow=1;
    }
    else if(!*overflow){
        if(!*word_i){
            wcscpy(_original_word_buffer, c_buffer);
            tolower_utf_8(c_buffer);
            wcscpy(_word_buffer, c_buffer);
            toupper_utf_8(c_buffer);
            wcscpy(_upper_word_buffer, c_buffer);
            wcscpy(_full_upper_word_buffer, c_buffer);
        }
        else{
            wcscat(_original_word_buffer, c_buffer);
            tolower_utf_8(c_buffer);
            wcscat(_word_buffer, c_buffer);
            wcscat(_upper_word_buffer, c_buffer);
            toupper_utf_8(c_buffer);
            wcscat(_full_upper_word_buffer, c_buffer);
        }
        *is_fetched=1;
        ++*word_i;
    }
}

//Function prints received word in one of several ways.
void print_word(byte *word_i, int count[], byte *is_sentence_started, sqlite3 *db){
    wchar_t leftover_char, leftover_string[WORD_SIZE];
    //leftover variable - -1 if no leftover occurred, otherwise shows index where leftover starts.
    sbyte leftover=0, last_check;
    leftover_string[0]=L'\0';
    while(leftover!=-1){
        //Call check_word and set console colour to _colour_green on 1 or _colour_red on 0.
        last_check=check_word(&leftover, &leftover_char, leftover_string, count, is_sentence_started, db);
        if(last_check) SetConsoleTextAttribute(_h, _colour_green);
        else SetConsoleTextAttribute(_h, _colour_red);
        //If leftover occurred - call handle_leftover.
        if(leftover!=-1){
            if(leftover>0&&!last_check) ++count[1];
            handle_leftover(*word_i, leftover, leftover_char);
        }
        else{
            wprintf(L"%ls", _original_word_buffer);
            SetConsoleTextAttribute(_h, _colour_standard);
            if(!last_check) ++count[1];
        }
    }
    for(byte i=0; i<wcslen(leftover_string); ++i){
        if(check_ending_letter(leftover_string[i])) *is_sentence_started=1;
        break;
    }
    wprintf(L"%ls", leftover_string);
    *word_i=0;
}

//Function handles word buffers if leftover occurred.
void handle_leftover(byte word_i, sbyte leftover, wchar_t leftover_char){
    byte j;
    wchar_t c_buffer[3];
    //Load current character to leftover_char and append null character to _original_word_buffer.
    //Print word, reset console colour and print first leftover character.
    //printf("word_i = %hhu, lo = %hhu\n", word_i, leftover);

    //Write left leftover part to word buffers.
    if(!leftover){
        SetConsoleTextAttribute(_h, _colour_standard);
        wprintf(L"%lc", leftover_char);
        j=1;
    }
    else{
        wprintf(L"%ls", _original_word_buffer);
        _original_word_buffer[leftover]=leftover_char;
        _word_buffer[leftover]=leftover_char;
        _upper_word_buffer[leftover]=leftover_char;
        _full_upper_word_buffer[leftover]=leftover_char;
        j=leftover;
    }
    for(byte i=0; ; ++i){
        _original_word_buffer[i]=_original_word_buffer[j];
        _word_buffer[i]=_word_buffer[j];
        _full_upper_word_buffer[i]=_full_upper_word_buffer[j];
        if(_original_word_buffer[j]=='\0') break;
        ++j;
    }
    wcscpy(_upper_word_buffer, _word_buffer);
    if(_upper_word_buffer[0]<=127) _upper_word_buffer[0]=toupper(_upper_word_buffer[0]);
    else if(_upper_word_buffer[0]>=192&&_upper_word_buffer[0]<=223){
        c_buffer[0]=_upper_word_buffer[0];
        c_buffer[1]=_upper_word_buffer[1];
        c_buffer[2]=L'\0';
        toupper_utf_8(c_buffer);
        _upper_word_buffer[0]=c_buffer[0];
        _upper_word_buffer[1]=c_buffer[1];
    }
}

//Function checks if any form of that word exists in one of tables.
//Returns 1 if word was found anywhere, 0 if not found.
sbyte check_word(sbyte *leftover, wchar_t *leftover_char, wchar_t leftover_string[], int count[], byte *is_sentence_started, sqlite3 *db){
    byte c=1, diff=0;
    *leftover=-1;
    while(c){
        if(_original_word_buffer[0]==L'\0') return -1;

        if(!strcmp(_database_id, "de")&&!*is_sentence_started){
            if(!wcscmp(_word_buffer, _original_word_buffer)) diff='l';
            else if(!wcscmp(_upper_word_buffer, _original_word_buffer)) diff='u';
        }

        //Check lower case, upper case and original form in "common" table.
        _table_id=0;
        if((find_word(count, db, 0)&&diff!='u')||(find_word(count, db, 1)&&diff!='l')||find_word(count, db, 3)){
            ++count[0];
            *is_sentence_started=0;
            return 1;
        }
        //Check lower case, upper case and original form in "abbreviation" table.
        _table_id=2;
        if((find_word(count, db, 0)&&diff!='u')||(find_word(count, db, 1)&&diff!='l')||find_word(count, db, 2)||find_word(count, db, 3)){
            ++count[0];
            *is_sentence_started=0;
            return 1;
        }
        //Check lower case, upper case and original form in "proper" table.
        _table_id=1;
        if((find_word(count, db, 0)&&diff!='u')||(find_word(count, db, 1)&&diff!='l')||find_word(count, db, 3)){
            ++count[0];
            *is_sentence_started=0;
            return 1;
        }
        //At first loop run, check leftover.
        //If leftover found, check_leftover cuts it of and loop starts over.
        //If word wasn't recognised at second run, break the loop.
        //If no leftover was found at first place, break the loop.
        c=0;
        if(*leftover==-1) c=check_leftover(leftover, leftover_char, leftover_string);
        //printf("\nc = %hhu\n", c);

    }
    *is_sentence_started=1;
    return 0;
}

//Function checks if word has leftover part.
//If leftover was found:
//  OUTDATED Writes start position in leftover variable if leftover position is 0;
//  Cuts every word buffer by inserting null character in that position;
//  Lowers first letter of _word_buffer;
//  Returns 1.
//Otherwise returns 0.
//P.S. Leftover part - part of unrecognised word, which starts by special character (', . or -).
byte check_leftover(sbyte *leftover, wchar_t *leftover_char, wchar_t leftover_string[]){
    short i, j;
    for(i=0; _original_word_buffer[i]!=L'\0'; ++i);
    if(_original_word_buffer[i-1]==L'\''||_original_word_buffer[i-1]==L'-'||_original_word_buffer[i-1]==L'.'){
        for(j=0; leftover_string[j]!=L'\0'; ++j);
        leftover_string[j]=_original_word_buffer[i-1];
        leftover_string[j+1]=L'\0';
        _original_word_buffer[i-1]=L'\0';
        _word_buffer[i-1]=L'\0';
        _upper_word_buffer[i-1]=L'\0';
        _full_upper_word_buffer[i-1]=L'\0';
        return 1;
    }
    for(byte i=0; _original_word_buffer[i]!=L'\0'; ++i){
        if(_original_word_buffer[i]==L'\''||_original_word_buffer[i]==L'-'||_original_word_buffer[i]==L'.'){
            *leftover_char=_original_word_buffer[i];
            _original_word_buffer[i]=L'\0';
            _word_buffer[i]=L'\0';
            _upper_word_buffer[i]=L'\0';
            _full_upper_word_buffer[i]=L'\0';
            *leftover=i;
            if(!i) return 0;
            return 1;
        }
    }
    return 0;
}

//Function executes SQL query which tries to fetch word data identified by word in table pointed by _table_id.
//Has callback function find_word_callback
byte find_word(int count[], sqlite3 *db, byte buffer_type){
    char sql[450]/*not verified*/;
    wchar_t query_buffer[WORD_SIZE], modified_query_buffer[WORD_SIZE*2];//disgusting warning
    byte j=0;
    switch(buffer_type){
    case 0:
        wcscpy(query_buffer, _word_buffer);
        break;
    case 1:
        wcscpy(query_buffer, _upper_word_buffer);
        break;
    case 2:
        wcscpy(query_buffer, _full_upper_word_buffer);
        break;
    case 3:
        wcscpy(query_buffer, _original_word_buffer);
    }
    for(byte i=0; i<=wcslen(query_buffer); ++i){
        modified_query_buffer[j]=query_buffer[i];
        if(query_buffer[i]==L'\'') modified_query_buffer[++j]=L'\'';
        ++j;
    }
    sprintf(sql, SQL_FIND, _table_name[_table_id], modified_query_buffer);
    _find_word_query_result=0;
    sqlite3_exec(db, sql, find_word_callback, NULL, NULL);
    if(_find_word_query_result) return 1;
    return 0;
}

//Callback function copies query result to _find_word_query_result.
int find_word_callback(void *null_data, int argc, char **argv, char **argcol){
    _find_word_query_result=1;
    return 0;
}
