#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum tokenID{
    t1_tk,
    t2_tk,
    t3_tk,
    EOF_tk
  };

struct token {
    tokenID id;
    std::string instance;
    int line;
};

static const std::string tokenNames[] = {
    "t1 token",
    "t2 token",
    "t3 token",
    "EOFTk"
 };

#endif