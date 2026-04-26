/*******************************************************
 * Name:        Jory Ehman
 * Course:      Program Translation / Compilers
 * Project:     P5
 * File:        codeGen.cpp
 * Description:
 *   Implements code generation by traversing the parse
 *   tree left to right and emitting VM accumulator
 *   assembly instructions to the output file.
 *
 *   Variable mapping:
 *     t1 tokens (e.g. #361, "580) are mapped to valid
 *     VM names by replacing the leading " or # with
 *     a letter prefix: # -> a, " -> b.
 *     e.g. #361 -> a361, "580 -> b580
 *
 *   t2 tokens are integer literals:
 *     One leading letter = positive (k23 -> 23)
 *     Multiple leading letters = negative (mg46 -> -46)
 *
 *   Temporaries use the prefix x (x0, x1, x2, ...)
 *   as suggested by the spec to avoid collisions.
 *
 *   Grammar semantics:
 *     B -> $ t1   : READ vmName(t1)
 *     B -> * t1   : WRITE vmName(t1)
 *     C -> - t1 J : evaluate J, STORE vmName(t1)
 *     J -> ' H D  : load H into ACC, apply D ops
 *     D -> E H D  : apply operator E to H, continue
 *     E -> (      : ADD next operand
 *     E -> )      : SUB next operand
 *     G H F J ' S : conditional (%) or loop (&)
 *       Evaluates J (arg2), stores in tmp,
 *       evaluates H (arg1), SUBs tmp,
 *       branches on false condition past body S.
 *       Loop adds BR back to condition label.
 *
 *   Comparison branch logic (ACC = arg1 - arg2):
 *     , (GT: arg1 > arg2): skip on ACC <= 0 -> BRZNEG
 *     . (LT: arg1 < arg2): skip on ACC >= 0 -> BRZPOS
 *     + (EQ: arg1 == arg2): skip on ACC != 0 -> BRNEG + BRPOS
 *******************************************************/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "codeGen.h"
#include "staticSem.h"
#include "node.h"

/* Counter for generating unique temporary variable names */
static int tempCount = 0;

/* Counter for generating unique label names */
static int labelCount = 0;

/* Temporary variable names collected during traversal */
static std::vector<std::string> tempVars;

/* Output file handle */
static FILE* out = NULL;

/* Forward declarations of recursive node handlers */
static void genS(node_t* node);
static void genA(node_t* node);
static void genB(node_t* node);
static void genC(node_t* node);
static void genD(node_t* node);
static void genJ(node_t* node);
static void genK(node_t* node);
static std::string evalH(node_t* node);

/*
 * newTemp:
 * Allocates a new unique temporary variable name
 * using the x prefix (x0, x1, x2, ...).
 * Records the name for later storage directive emission.
 */
static std::string newTemp()
{
    char buf[16];
    snprintf(buf, sizeof(buf), "x%d", tempCount++);
    tempVars.push_back(std::string(buf));
    return std::string(buf);
}

/*
 * newLabel:
 * Generates a unique label string (L0, L1, L2, ...).
 */
static std::string newLabel()
{
    char buf[16];
    snprintf(buf, sizeof(buf), "L%d", labelCount++);
    return std::string(buf);
}

/*
 * vmName:
 * Converts a t1 token instance to a valid VM variable name.
 * # prefix -> a prefix  (e.g. #361 -> a361)
 * " prefix -> b prefix  (e.g. "580 -> b580)
 */
static std::string vmName(const std::string& t1)
{
    if (t1.empty())
        return t1;

    std::string result = t1.substr(1); // strip " or #

    if (t1[0] == '#')
        result = "a" + result;
    else
        result = "b" + result;

    return result;
}

/*
 * t2Value:
 * Converts a t2 token instance to its integer value string.
 * One leading letter = positive: k23 -> "23"
 * Multiple leading letters = negative: mg46 -> "-46"
 */
static std::string t2Value(const std::string& t2)
{
    size_t i = 0;

    /* Count leading letters */
    while (i < t2.size() && std::isalpha((unsigned char)t2[i]))
        i++;

    std::string digits = t2.substr(i);

    /* Multiple leading letters means negative */
    if (i > 1)
        return "-" + digits;

    return digits;
}

/*
 * isT1:
 * Returns true if label is a t1 token (starts with " or #).
 */
static bool isT1(const std::string& label)
{
    if (label.size() < 2)
        return false;
    return (label[0] == '"' || label[0] == '#');
}

/*
 * isT2:
 * Returns true if label is a t2 token (starts with a letter,
 * contains digits, and does not start with " or #).
 */
static bool isT2(const std::string& label)
{
    if (label.empty())
        return false;
    if (!std::isalpha((unsigned char)label[0]))
        return false;
    /* Must contain at least one digit */
    for (size_t i = 0; i < label.size(); i++)
    {
        if (std::isdigit((unsigned char)label[i]))
            return true;
    }
    return false;
}

/*
 * evalH:
 * Returns the operand string for an H node.
 * H -> t1: returns the VM variable name
 * H -> t2: returns the integer value string
 * Result is ready to use directly as a VM instruction argument.
 */
static std::string evalH(node_t* node)
{
    /* H has exactly one child: the t1 or t2 leaf */
    if (node == NULL || node->children.empty())
        return "0";

    node_t* child = node->children[0];

    if (isT1(child->label))
        return vmName(child->label);

    if (isT2(child->label))
        return t2Value(child->label);

    return "0";
}

/*
 * genD:
 * Generates code for D -> E H D | epsilon.
 * Assumes ACC already holds the left-hand value.
 * Applies ADD or SUB based on E, then continues with D.
 */
static void genD(node_t* node)
{
    if (node == NULL)
        return;

    /* Epsilon production: nothing to do */
    if (node->children.size() == 1 &&
        node->children[0]->label == "ε")
        return;

    /* D -> E H D: children are E, H, D */
    if (node->children.size() < 3)
        return;

    node_t* eNode = node->children[0]; /* E: ( or ) */
    node_t* hNode = node->children[1]; /* H: operand */
    node_t* dNode = node->children[2]; /* D: continuation */

    std::string operand = evalH(hNode);
    std::string op = eNode->children[0]->label;

    if (op == "(")
        fprintf(out, "ADD %s\n", operand.c_str());
    else if (op == ")")
        fprintf(out, "SUB %s\n", operand.c_str());

    genD(dNode);
}

/*
 * genJ:
 * Generates code for J -> ' H D.
 * Loads H into ACC, then applies D operations.
 * Result remains in ACC after execution.
 */
static void genJ(node_t* node)
{
    if (node == NULL || node->children.size() < 3)
        return;

    /* children: ' (index 0), H (index 1), D (index 2) */
    node_t* hNode = node->children[1];
    node_t* dNode = node->children[2];

    std::string operand = evalH(hNode);
    fprintf(out, "LOAD %s\n", operand.c_str());

    genD(dNode);
}

/*
 * genB:
 * Generates code for B -> $ t1 | * t1.
 * $ t1 : READ vmName (declare and read from user)
 * * t1 : WRITE vmName (print to screen)
 */
static void genB(node_t* node)
{
    if (node == NULL || node->children.size() < 2)
        return;

    std::string op   = node->children[0]->label;
    std::string name = vmName(node->children[1]->label);

    if (op == "$")
        fprintf(out, "READ %s\n", name.c_str());
    else if (op == "*")
        fprintf(out, "WRITE %s\n", name.c_str());
}

/*
 * genC:
 * Generates code for C -> - t1 J.
 * Evaluates J (leaves result in ACC), then stores
 * the result into the t1 variable.
 */
static void genC(node_t* node)
{
    if (node == NULL || node->children.size() < 3)
        return;

    /* children: - (0), t1 (1), J (2) */
    std::string name = vmName(node->children[1]->label);
    node_t* jNode    = node->children[2];

    genJ(jNode);
    fprintf(out, "STORE %s\n", name.c_str());
}

/*
 * genS:
 * Generates code for S -> A K '
 * Delegates to A and K handlers.
 */
static void genS(node_t* node);

/*
 * genK:
 * Generates code for K -> S K | epsilon.
 * Recursively processes each statement in sequence.
 */
static void genK(node_t* node)
{
    if (node == NULL)
        return;

    if (node->children.size() == 1 &&
        node->children[0]->label == "ε")
        return;

    /* K -> S K */
    if (node->children.size() >= 2)
    {
        genS(node->children[0]);
        genK(node->children[1]);
    }
}

/*
 * genA:
 * Generates code for A -> B | C | G H F J ' S.
 * Dispatches to the appropriate handler based on
 * which production was used.
 *
 * For G H F J ' S (if/while):
 *   Evaluates J (arg2) into a temp.
 *   Loads H (arg1), SUBs temp.
 *   Branches past body on false condition.
 *   For & (while): adds back-branch to condition label.
 */
static void genA(node_t* node)
{
    if (node == NULL || node->children.empty())
        return;

    node_t* first = node->children[0];

    if (first->label == "B")
    {
        genB(first);
        return;
    }

    if (first->label == "C")
    {
        genC(first);
        return;
    }

    /* G H F J ' S production: conditional or loop */
    /* children of A: G(0) H(1) F(2) J(3) '(4) S(5) */
    if (first->label == "G" && node->children.size() >= 6)
    {
        node_t* gNode = node->children[0]; /* G: % or & */
        node_t* hNode = node->children[1]; /* H: arg1   */
        node_t* fNode = node->children[2]; /* F: comparator */
        node_t* jNode = node->children[3]; /* J: arg2 expression */
        node_t* sNode = node->children[5]; /* S: body  */

        std::string gOp = gNode->children[0]->label;
        std::string fOp = fNode->children[0]->label;

        /* Allocate out label and temp for comparison.
         * Loop label is only needed for while (&). */
        std::string outLabel  = newLabel();
        std::string loopLabel = "";
        std::string tmp       = newTemp();

        /* For while (&): allocate and place loop label before condition */
        if (gOp == "&")
        {
            loopLabel = newLabel();
            fprintf(out, "%s: ", loopLabel.c_str());
        }

        /* Evaluate arg2 (from J) and store in temp */
        genJ(jNode);
        fprintf(out, "STORE %s\n", tmp.c_str());

        /* Load arg1 (H) and subtract arg2 temp */
        std::string arg1 = evalH(hNode);
        fprintf(out, "LOAD %s\n", arg1.c_str());
        fprintf(out, "SUB %s\n", tmp.c_str());

        /* Branch past body on false condition */
        /* ACC = arg1 - arg2 */
        if (fOp == ",")
        {
            /* GT (arg1 > arg2): false when ACC <= 0 */
            fprintf(out, "BRZNEG %s\n", outLabel.c_str());
        }
        else if (fOp == ".")
        {
            /* LT (arg1 < arg2): false when ACC >= 0 */
            fprintf(out, "BRZPOS %s\n", outLabel.c_str());
        }
        else if (fOp == "+")
        {
            /* EQ (arg1 == arg2): false when ACC != 0 */
            fprintf(out, "BRNEG %s\n", outLabel.c_str());
            fprintf(out, "BRPOS %s\n", outLabel.c_str());
        }

        /* Generate body */
        genS(sNode);

        /* For while (&): branch back to condition */
        if (gOp == "&")
            fprintf(out, "BR %s\n", loopLabel.c_str());

        /* Out label lands here after body (or on false) */
        fprintf(out, "%s: NOOP\n", outLabel.c_str());
    }
}

/*
 * genS:
 * Generates code for S -> A K '
 * The ' is structural only and produces no code.
 */
static void genS(node_t* node)
{
    if (node == NULL || node->children.size() < 3)
        return;

    /* children: A(0) K(1) '(2) */
    genA(node->children[0]);
    genK(node->children[1]);
}

/*
 * codeGen:
 * Public entry point. Traverses the parse tree,
 * emits all instructions, then emits STOP followed
 * by storage directives for all variables and temps.
 */
void codeGen(node_t* root, FILE* outFile)
{
    out       = outFile;
    tempCount = 0;
    labelCount = 0;
    tempVars.clear();

    /* Generate all instructions */
    genS(root);

    /* STOP marks end of executable instructions */
    fprintf(out, "STOP\n");

    /* Emit storage directives for declared variables */
    const std::vector<std::string>& symTable = getSymbolTable();

    for (size_t i = 0; i < symTable.size(); i++)
        fprintf(out, "%s 0\n", vmName(symTable[i]).c_str());

    /* Emit storage directives for temporaries */
    for (size_t i = 0; i < tempVars.size(); i++)
        fprintf(out, "%s 0\n", tempVars[i].c_str());
}
