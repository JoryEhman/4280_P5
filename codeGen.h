/*******************************************************
 * Name:        Jory Ehman
 * Course:      Program Translation / Compilers
 * Project:     P5
 * File:        codeGen.h
 * Description:
 *   Declares the codeGen function which traverses the
 *   parse tree and emits VM accumulator assembly to
 *   the specified output file.
 *
 *   The generated assembly is runnable by virtMach.
 *   Instructions are emitted first, followed by STOP,
 *   followed by all storage directives (declared
 *   variables and temporaries).
 *******************************************************/
#ifndef CODEGEN_H
#define CODEGEN_H

#include <cstdio>
#include "node.h"

/* codeGen: traverses the parse tree left to right and
 * writes VM assembly instructions to outFile.
 * Emits STOP and all storage directives at the end. */
void codeGen(node_t* root, FILE* outFile);

#endif //CODEGEN_H
