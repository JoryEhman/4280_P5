/*******************************************************
 * Name:        Jory Ehman
 * Course:      Program Translation / Compilers
 * Project:     P5
 * File:        main.cpp
 * Description:
 *   Entry point of the program. Handles command-line
 *   argument validation, opens input file or reads
 *   from standard input, invokes the parser, then
 *   static semantic analysis, then code generation.
 *
 *   Output assembly file naming:
 *     P5 filename.txt  ->  filename.asm
 *     P5 (stdin)       ->  out.asm
 *
 *   Invocation:
 *     P5 [filename]
 *******************************************************/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "parser.h"
#include "staticSem.h"
#include "codeGen.h"

int main(int argc, char* argv[])
{
    FILE* inFile  = NULL;
    FILE* outFile = NULL;
    std::string outName;

    if (argc == 1)
    {
        inFile  = stdin;
        outName = "out.asm";
    }
    else if (argc == 2)
    {
        inFile = fopen(argv[1], "r");

        if (inFile == NULL)
        {
            fprintf(stderr, "Fatal: Cannot open file\n");
            exit(EXIT_FAILURE);
        }

        /* Derive output filename: strip extension, add .asm */
        std::string base = argv[1];
        size_t dot = base.rfind('.');

        if (dot != std::string::npos)
            base = base.substr(0, dot);

        outName = base + ".asm";
    }
    else
    {
        fprintf(stderr, "Fatal: Improper usage\n");
        fprintf(stderr, "Usage: P5 [filename]\n");
        exit(EXIT_FAILURE);
    }

    /* Build parse tree */
    node_t* root = parser(inFile);

    if (inFile != stdin)
        fclose(inFile);

    /* Run static semantic analysis */
    staticSem(root);

    /* Open output assembly file */
    outFile = fopen(outName.c_str(), "w");

    if (outFile == NULL)
    {
        fprintf(stderr, "Fatal: Cannot create output file %s\n",
                outName.c_str());
        exit(EXIT_FAILURE);
    }

    /* Generate assembly code */
    codeGen(root, outFile);

    fclose(outFile);

    fprintf(stdout, "Assembly written to %s\n", outName.c_str());

    return 0;
}
