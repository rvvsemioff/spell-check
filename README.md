# spell-check
Console program which parses and prints UTF-8 text files with coloured recognised words.

# How to run?
Use MinGW-w64 compiler on Windows:
```  w64-mingw32-gcc.exe -Wall  -c codepage.c -o obj\codepage.o
  w64-mingw32-gcc.exe -Wall  -c console.c -o obj\console.o
  w64-mingw32-gcc.exe -Wall  -c import.c -o obj\import.o
  w64-mingw32-gcc.exe -Wall  -c main.c -o obj\main.o
  w64-mingw32-gcc.exe -Wall  -c parse_text.c -o obj\parse_text.o
  w64-mingw32-gcc.exe -Wall  -c sqlite3.c -o obj\sqlite3.o
  w64-mingw32-g++.exe  -o bin\spell-check.exe obj\codepage.o obj\console.o obj\import.o obj\main.o obj\parse_text.o obj\sqlite3.o  -static-libstdc++ -static-libgcc -lpthread -static -s
```
Or open `spell-check.cbp` in Codeblocks

# Command prompt
* `set PROMPT`                        changes language database to `PROMPT` (`en`/`de`/`pl`/`ru`/`ua`)
* `load/ld FILE[.txt]`                loads and parses FILE
* `reload/rl`                         reloads previously loaded FILE
* `add -c/-p/-a WORD[:ORIGIN]`        adds `WORD` with `ORIGIN` (optional) to a selected table (`c`: common / `p`: proper / `a`: abbreviation)
* `count`                             counts number of common words and declensions
* `invert`                            inverts background and text colours and calles 'reload'
* `help`                              list of commands
* `exit`                              terminates the program

# Generating polish language database
1. Download and unzip `odm.txt` from https://sjp.pl/slownik/odmiany/ to `res/` folder.
2. Uncomment `#include "import.h"` and `import();` lines in `main.c`.
3. After first run program will generate polish language database named `lib-pl.db` in project directory.
