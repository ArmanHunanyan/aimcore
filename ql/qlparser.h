#ifndef QL_PARSER_H
#define QL_PARSER_H

#include "qltree.h"

#include <string>
#include <stdexcept>

namespace ql {

class SyntaxError
    : public std::runtime_error
{
public:
    SyntaxError(const std::string& msg)
        : std::runtime_error(msg)
    { }

    virtual size_t errorPos() const = 0;
};

ExpressionTree parseExpression(const char* first, const char* last, bool strict);

inline ExpressionTree parseExpressionStr(const std::string& s, bool strict)
{
    return parseExpression(s.c_str(), s.c_str() + s.size(), strict);
}

void parseAndConcatExpression(ExpressionTree& tree, const char* first, const char* last, bool strict);

inline void parseAndConcatExpressionStr(ExpressionTree& tree, const std::string& s, bool strict)
{
    parseAndConcatExpression(tree, s.c_str(), s.c_str() + s.size(), strict);
}

} // namespace ql

#endif