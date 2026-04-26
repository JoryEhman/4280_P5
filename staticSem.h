/*******************************************************
 * Name:        Jory Ehman
 * Course:      Program Translation / Compilers
 * Project:     P5
 * File:        staticSem.h
 * Description:
 *   Declares the staticSem function and the symbol
 *   table accessor used by code generation.
 *
 *   The symbol table is built during static semantic
 *   analysis and shared with the code generator so
 *   that variable name mappings are available when
 *   emitting assembly instructions.
 *
 *   Enforces two rules:
 *     1. Variables must be declared before first use
 *     2. A variable name may only be declared once
 *******************************************************/
#ifndef STATICSEM_H
#define STATICSEM_H

#include <vector>
#include <string>
#include "node.h"

/* staticSem: performs static semantic analysis on the
 * parse tree. Populates the shared symbol table.
 * Prints error and exits on first violation found. */
void staticSem(node_t* root);

/* getSymbolTable: returns a reference to the symbol
 * table built during static semantic analysis.
 * Used by code generation to map t1 tokens to VM names. */
const std::vector<std::string>& getSymbolTable();

#endif //STATICSEM_H
