/*******************************************************
* Name:        Jory Ehman
 * Course:      Program Translation / Compilers
 * Project:     P3
 * File:        parser.h
 * Description:
 *   Declares the parser function responsible for
 *   parsing the input token stream according to
 *   the provided BNF grammar. The parser uses
 *   recursive descent with one token lookahead
 *   (LL(1)) and returns the root of the parse tree
 *   upon success, or generates an error and exits
 *   upon failure.
 *******************************************************/
#ifndef PARSER_H
#define PARSER_H

#include "node.h"
#include "token.h"

node_t* parser(FILE* file);

#endif //PARSER_H