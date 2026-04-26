/*******************************************************
 * Name:        Jory Ehman
 * Course:      Program Translation / Compilers
 * Project:     P3
 * File:        parser.cpp
 * Description:
 *   Implements a recursive descent LL(1) parser for
 *   the provided BNF grammar. Uses one token lookahead
 *   to predict productions. Each nonterminal builds
 *   and returns a subtree. Parser generates an error
 *   with line number and token involved, or returns
 *   the parse tree root on success.
 *
 *   All nonterminal functions are static to restrict
 *   linkage to this translation unit.
 *******************************************************/
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "parser.h"
#include "scanner.h"
#include "token.h"

// ====== Internal parser state ======
static token tk;
static FILE* srcFile;

// ====== Static nonterminal function prototypes ======
static node_t* S();
static node_t* A();
static node_t* B();
static node_t* C();
static node_t* D();
static node_t* E();
static node_t* F();
static node_t* G();
static node_t* H();
static node_t* J();
static node_t* K();

// ====== Helper functions ======

/*
 * nextToken:
 * Advances to the next token by calling the scanner.
 */
static void nextToken()
{
    tk = scanner(srcFile);
}

/*
 * parserError:
 * Prints an error message with the current token
 * and line number, then exits the program.
 */
static void parserError(const std::string& expected)
{
    std::cerr << "PARSER ERROR: Expected " << expected
              << " but found \"" << tk.instance
              << "\" at line " << tk.line << std::endl;
    exit(EXIT_FAILURE);
}

/*
 * consumeToken:
 * Captures current instance, verifies it matches
 * expected, advances to next token, returns leaf node.
 */
static node_t* consumeToken(const std::string& expected)
{
    if (tk.instance != expected)
    {
        parserError(expected);
    }
    node_t* leaf = new node_t(tk.instance);
    nextToken();
    return leaf;
}

/*
 * consumeType:
 * Captures current instance, verifies it matches
 * expected type, advances to next token, returns leaf node.
 */
static node_t* consumeType(tokenID expectedType)
{
    if (tk.id != expectedType)
    {
        if (expectedType == t1_tk)
            parserError("t1 token");
        else if (expectedType == t2_tk)
            parserError("t2 token");
        else
            parserError("expected token type");
    }
    node_t* leaf = new node_t(tk.instance);
    nextToken();
    return leaf;
}

// ====== Nonterminal implementations ======

/*
 * S -> A K '
 */
static node_t* S()
{
    node_t* node = new node_t("S");

    node->children.push_back(A());
    node->children.push_back(K());
    node->children.push_back(consumeToken("'"));

    return node;
}

/*
 * A -> B | C | G H F J ' S
 * FIRST(B) = {$, *}
 * FIRST(C) = {-}
 * FIRST(G) = {%, &}
 */
static node_t* A()
{
    node_t* node = new node_t("A");

    if (tk.instance == "$" || tk.instance == "*")
    {
        node->children.push_back(B());
    }
    else if (tk.instance == "-")
    {
        node->children.push_back(C());
    }
    else if (tk.instance == "%" || tk.instance == "&")
    {
        node->children.push_back(G());
        node->children.push_back(H());
        node->children.push_back(F());
        node->children.push_back(J());
        node->children.push_back(consumeToken("'"));
        node->children.push_back(S());
    }
    else
    {
        parserError("$, *, -, %, or &");
    }

    return node;
}

/*
 * B -> $ t1 | * t1
 * FIRST(B) = {$, *}
 */
static node_t* B()
{
    node_t* node = new node_t("B");

    if (tk.instance == "$")
    {
        node->children.push_back(consumeToken("$"));
        node->children.push_back(consumeType(t1_tk));
    }
    else if (tk.instance == "*")
    {
        node->children.push_back(consumeToken("*"));
        node->children.push_back(consumeType(t1_tk));
    }
    else
    {
        parserError("$ or *");
    }

    return node;
}

/*
 * C -> - t1 J
 */
static node_t* C()
{
    node_t* node = new node_t("C");

    node->children.push_back(consumeToken("-"));
    node->children.push_back(consumeType(t1_tk));
    node->children.push_back(J());

    return node;
}

/*
 * D -> E H D | ε
 * FIRST(E) = {(, )}
 * FOLLOW(D) = {$, *, -, %, &, '}
 */
static node_t* D()
{
    node_t* node = new node_t("D");

    if (tk.instance == "(" || tk.instance == ")")
    {
        node->children.push_back(E());
        node->children.push_back(H());
        node->children.push_back(D());
    }
    else
    {
        // epsilon production
        node->children.push_back(new node_t("ε"));
    }

    return node;
}

/*
 * E -> ( | )
 */
static node_t* E()
{
    node_t* node = new node_t("E");

    if (tk.instance == "(")
    {
        node->children.push_back(consumeToken("("));
    }
    else if (tk.instance == ")")
    {
        node->children.push_back(consumeToken(")"));
    }
    else
    {
        parserError("( or )");
    }

    return node;
}

/*
 * F -> , | . | +
 */
static node_t* F()
{
    node_t* node = new node_t("F");

    if (tk.instance == ",")
    {
        node->children.push_back(consumeToken(","));
    }
    else if (tk.instance == ".")
    {
        node->children.push_back(consumeToken("."));
    }
    else if (tk.instance == "+")
    {
        node->children.push_back(consumeToken("+"));
    }
    else
    {
        parserError(", or . or +");
    }

    return node;
}

/*
 * G -> % | &
 */
static node_t* G()
{
    node_t* node = new node_t("G");

    if (tk.instance == "%")
    {
        node->children.push_back(consumeToken("%"));
    }
    else if (tk.instance == "&")
    {
        node->children.push_back(consumeToken("&"));
    }
    else
    {
        parserError("% or &");
    }

    return node;
}

/*
 * H -> t1 | t2
 */
static node_t* H()
{
    node_t* node = new node_t("H");

    if (tk.id == t1_tk)
    {
        node->children.push_back(consumeType(t1_tk));
    }
    else if (tk.id == t2_tk)
    {
        node->children.push_back(consumeType(t2_tk));
    }
    else
    {
        parserError("t1 or t2 token");
    }

    return node;
}

/*
 * J -> ' H D
 */
static node_t* J()
{
    node_t* node = new node_t("J");

    node->children.push_back(consumeToken("'"));
    node->children.push_back(H());
    node->children.push_back(D());

    return node;
}

/*
 * K -> S K | ε
 * FIRST(S) = {$, *, -, %, &}
 * FOLLOW(K) = {'}
 */
static node_t* K()
{
    node_t* node = new node_t("K");

    if (tk.instance == "$" || tk.instance == "*" ||
        tk.instance == "-" || tk.instance == "%" ||
        tk.instance == "&")
    {
        node->children.push_back(S());
        node->children.push_back(K());
    }
    else
    {
        // epsilon production
        node->children.push_back(new node_t("ε"));
    }

    return node;
}

// ====== Public parser function ======

/*
 * parser:
 * Entry point. Sets up scanner state, primes the
 * first token, and begins recursive descent from S.
 * Returns root of parse tree on success.
 */
node_t* parser(FILE* file)
{
    srcFile = file;
    nextToken();

    node_t* root = S();

    if (tk.id != EOF_tk)
    {
        parserError("EOF");
    }

    return root;
}