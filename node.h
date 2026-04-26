/*******************************************************
* Name:        Jory Ehman
 * Course:      Program Translation / Compilers
 * Project:     P3
 * File:        node.h
 * Description:
 *   Defines the node_t structure used to implement
 *   the parse tree. Each node stores:
 *     - A label (nonterminal name or token instance)
 *     - A vector of child node pointers
 *
 *   This structure represents the core data model
 *   for the parse tree used throughout the program.
 *******************************************************/
#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>

struct node_t
{
    std::string label;
    std::vector<node_t*> children;

    node_t(const std::string& lbl)
    {
        label = lbl;
    }
};

#endif //NODE_H