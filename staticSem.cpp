/*******************************************************
 * Name:        Jory Ehman
 * Course:      Program Translation / Compilers
 * Project:     P5
 * File:        staticSem.cpp
 * Description:
 *   Implements static semantic analysis via a preorder
 *   traversal of the parse tree. Uses a vector as the
 *   symbol table, storing each declared variable name.
 *
 *   Two semantic rules are enforced:
 *     1. A variable (t1 token) used outside a $ declaration
 *        must already exist in the symbol table.
 *        Error type: "undefined variable"
 *     2. A variable being declared with $ must not already
 *        exist in the symbol table.
 *        Error type: "variable redefined"
 *
 *   The symbol table is exposed via getSymbolTable() so
 *   that code generation can look up variable mappings.
 *
 *   The program exits on the first error found.
 *   On success, the symbol table is printed.
 *******************************************************/
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>

#include "staticSem.h"
#include "node.h"

/* Symbol table: stores declared variable names in
 * order of declaration. Shared with code generation. */
static std::vector<std::string> symbolTable;

/* Forward declaration of recursive traversal helper */
static void traverse(node_t* node, bool isDecl);

/*
 * insert:
 * Inserts a variable name into the symbol table.
 * Exits with an error if the name already exists.
 */
static void insert(const std::string& name)
{
    for (size_t i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i] == name)
        {
            std::cerr << "SEMANTIC ERROR: variable redefined: "
                      << name << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    symbolTable.push_back(name);
}

/*
 * verify:
 * Returns true if the variable name exists in the
 * symbol table, false otherwise.
 */
static bool verify(const std::string& name)
{
    for (size_t i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i] == name)
            return true;
    }
    return false;
}

/*
 * isT1:
 * Returns true if the given label is a t1 token.
 * t1 tokens begin with " or # followed by one or
 * more digit characters (0-9).
 */
static bool isT1(const std::string& label)
{
    if (label.size() < 2)
        return false;

    char first = label[0];
    if (first != '"' && first != '#')
        return false;

    for (size_t i = 1; i < label.size(); i++)
    {
        if (label[i] < '0' || label[i] > '9')
            return false;
    }

    return true;
}

/*
 * traverse:
 * Recursively walks the parse tree in preorder.
 * B nodes are handled specially to distinguish
 * declarations ($) from uses (*).
 */
static void traverse(node_t* node, bool isDecl)
{
    if (node == NULL)
        return;

    /* Leaf node: check t1 variables against symbol table */
    if (node->children.empty())
    {
        if (isT1(node->label))
        {
            if (isDecl)
                insert(node->label);
            else if (!verify(node->label))
            {
                std::cerr << "SEMANTIC ERROR: undefined variable: "
                          << node->label << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        return;
    }

    /* B node: children[0] is "$" (declare) or "*" (use) */
    if (node->label == "B")
    {
        bool declFlag = (!node->children.empty() &&
                         node->children[0]->label == "$");

        traverse(node->children[0], false);

        if (node->children.size() > 1)
            traverse(node->children[1], declFlag);

        return;
    }

    /* All other nodes: recurse normally */
    for (size_t i = 0; i < node->children.size(); i++)
        traverse(node->children[i], false);
}

/*
 * getSymbolTable:
 * Returns a const reference to the symbol table.
 * Called by code generation to resolve variable names.
 */
const std::vector<std::string>& getSymbolTable()
{
    return symbolTable;
}

/*
 * staticSem:
 * Public entry point. Clears and rebuilds the symbol
 * table, then prints it on success.
 */
void staticSem(node_t* root)
{
    symbolTable.clear();
    traverse(root, false);

    std::cout << "Symbol Table:" << std::endl;

    for (size_t i = 0; i < symbolTable.size(); i++)
    {
        if (i > 0)
            std::cout << " ";
        std::cout << symbolTable[i];
    }
    std::cout << std::endl;
}
