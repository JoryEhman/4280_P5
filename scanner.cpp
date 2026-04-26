#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <string>

#include "scanner.h"
#include "token.h"

// ====== Internal scanner state ======
static int currentLine = 1;

// Column order (MUST match your driver table):
// 0 WS
// 1 " or #
// 2 Letter
// 3 Digit
// 4 Symbol ($ % & ' ( ) * + , - . /)
// 5 EOF

// Accept codes
static const int ERR  = -1;
static const int ACC_T1  = -2;
static const int ACC_T2  = -3;
static const int ACC_T3  = -4;
static const int ACC_EOF = -5;

// States 0..5, Cols 0..5
static const int FSA[6][6] =
{
    // WS   " #   Ltr  Dig  Sym  EOF
    {  0,    1,   3,  ERR,   5,  ACC_EOF }, // state 0
    { ERR, ERR, ERR,   2,  ERR,    ERR   }, // state 1
    { ACC_T1, ACC_T1, ACC_T1, 2, ACC_T1, ACC_T1 }, // state 2
    { ERR, ERR,   3,   4,  ERR,    ERR   }, // state 3
    { ACC_T2, ACC_T2, ACC_T2, 4, ACC_T2, ACC_T2 }, // state 4
    { ACC_T3, ACC_T3, ACC_T3, ACC_T3, ACC_T3, ACC_T3 }  // state 5
};

// Forward declarations
static int  filterNextChar(FILE* file);
static int  classifyChar(int c);
static void scannerErrorChar(int c);
static void scannerErrorMsg(const std::string& msg);

// ---- Error helpers ----
static void scannerErrorChar(int c)
{
    if (c == EOF)
    {
        std::cerr << "SCANNER ERROR: unexpected EOF at line " << currentLine << std::endl;
    }
    else
    {
        std::cerr << "SCANNER ERROR: " << static_cast<char>(c)
                  << " at line " << currentLine << std::endl;
    }
    exit(EXIT_FAILURE);
}

static void scannerErrorMsg(const std::string& msg)
{
    std::cerr << "SCANNER ERROR: " << msg << " at line " << currentLine << std::endl;
    exit(EXIT_FAILURE);
}

// ---- Preprocessing filter: strips comments, counts lines, validates alphabet ----
// NOTE: whitespace is NOT skipped here because WS is a table column in your FSA.
static int filterNextChar(FILE* file)
{
    int c = fgetc(file);
    while (true)
    {
        if (c == EOF)
        {
            return EOF;
        }

        // Count newline immediately
        if (c == '\n')
        {
            currentLine++;
            return c;   // newline is whitespace
        }
        // Comment handling
        if (c == '!')
        {
            int d = fgetc(file);
            while (true)
            {
                if (d == EOF)
                {
                    std::cerr << "SCANNER ERROR: unterminated comment at line "
                              << currentLine << std::endl;
                    exit(EXIT_FAILURE);
                }

                if (d == '\n')
                {
                    currentLine++;
                }

                if (d == '!')
                {
                    c = fgetc(file);   // resume after closing !
                    break;
                }

                d = fgetc(file);
            }
            continue;   // restart loop with new c
        }
        // Alphabet validation (P2 rules only)
        unsigned char uc = static_cast<unsigned char>(c);

        if (std::isalpha(uc) ||
            std::isdigit(uc) ||
            std::isspace(uc) ||
            uc == '"' ||
            uc == '#' ||
            (uc >= 36 && uc <= 47))
        {
            return c;
        }
        std::cerr << "SCANNER ERROR: "
                  << static_cast<char>(uc)
                  << " at line "
                  << currentLine
                  << std::endl;
        exit(EXIT_FAILURE);
    }
}

// ---- classifyChar: maps a validated character to your table column ----
static int classifyChar(int c)
{
    if (c == EOF)
        return 5;

    unsigned char uc = static_cast<unsigned char>(c);

    if (std::isspace(uc))
        return 0;

    if (uc == '"' || uc == '#')
        return 1;

    if (std::isalpha(uc))
        return 2;

    if (std::isdigit(uc))
        return 3;

    if (uc >= 36 && uc <= 47)
        return 4;

    // Should never happen if filterNextChar() validated correctly
    scannerErrorChar(c);
    return ERR;
}

// ---- scanner: returns one token per call ----
token scanner(FILE* file)
{
    token tk;
    tk.instance.clear();
    tk.line = currentLine;
    tk.id = EOF_tk;

    int state = 0;
    bool tokenLineSet = false;

    while (true)
    {
        int c = filterNextChar(file);
        int col = classifyChar(c);

        int next = FSA[state][col];

        if (next == ERR)
        {
            // We hit an invalid transition; report based on what we have so far.
            // If nothing accumulated, error on the current character.
            if (tk.instance.empty() && c != EOF)
            {
                scannerErrorChar(c);
            }
            else if (!tk.instance.empty())
            {
                scannerErrorMsg(tk.instance);
            }
            else
            {
                scannerErrorMsg("invalid token");
            }
        }

        if (next >= 0)
        {
            // Transition to another state
            // Only start the token instance when we leave state 0 on non-whitespace
            if (!(state == 0 && col == 0))
            {
                if (!tokenLineSet)
                {
                    tk.line = currentLine;
                    tokenLineSet = true;
                }
                if (c != EOF)
                {
                    tk.instance.push_back(static_cast<char>(c));
                }
            }

            state = next;
            continue;
        }

        // Accepting transition
        if (next == ACC_EOF)
        {
            tk.id = EOF_tk;
            tk.instance = "EOF";
            tk.line = currentLine;
            return tk;
        }

        // For t1/t2/t3 accepts: push back the character that triggered accept,
        // unless it was EOF.
        if (c != EOF)
        {
            ungetc(c, file);
            // IMPORTANT: if we pushed back a '\n', line count would be off.
            // Our filter increments line on '\n' when it reads it.
            // Therefore, we should avoid pushing back '\n'.
            // With WS as a column, acceptance often triggers on WS; handle newline carefully:
            if (c == '\n')
            {
                // We already counted it; undo that count since it will be read again.
                currentLine--;
            }
        }

        if (next == ACC_T1)
        {
            tk.id = t1_tk;
            return tk;
        }
        if (next == ACC_T2)
        {
            tk.id = t2_tk;
            return tk;
        }
        if (next == ACC_T3)
        {
            tk.id = t3_tk;
            return tk;
        }

        scannerErrorMsg("unknown accept state");
    }
}