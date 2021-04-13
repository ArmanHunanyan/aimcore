
#include "qlparser.h"
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <cassert>
#include <string>
#include <functional>

namespace ql {

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace spirit = boost::spirit;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createOrNodePh, std::bind(&ExpressionTree::createOrNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createAndNodePh, std::bind(&ExpressionTree::createAndNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createInNodePh, std::bind(&ExpressionTree::createInNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createNotInNodePh, std::bind(&ExpressionTree::createNotInNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createIsNodePh, std::bind(&ExpressionTree::createIsNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createIsNotNodePh, std::bind(&ExpressionTree::createIsNotNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createLessNodePh, std::bind(&ExpressionTree::createLessNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createLessEqualNodePh, std::bind(&ExpressionTree::createLessEqualNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createGreaterNodePh, std::bind(&ExpressionTree::createGreaterNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createGreaterEqualNodePh, std::bind(&ExpressionTree::createGreaterEqualNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createNotEqualNodePh, std::bind(&ExpressionTree::createNotEqualNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createEqualNodePh, std::bind(&ExpressionTree::createEqualNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createPlusNodePh, std::bind(&ExpressionTree::createPlusNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createMinusNodePh, std::bind(&ExpressionTree::createMinusNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createMulNodePh, std::bind(&ExpressionTree::createMulNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createDivNodePh, std::bind(&ExpressionTree::createDivNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createFloorDivNodePh, std::bind(&ExpressionTree::createFloorDivNode, _1, _2, _3, _4), 4);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createReminderNodePh, std::bind(&ExpressionTree::createReminderNode, _1, _2, _3, _4), 4);

BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createNotNodePh, std::bind(&ExpressionTree::createNotNode, _1, _2, _3), 3);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createNegateNodePh, std::bind(&ExpressionTree::createNegateNode, _1, _2, _3), 3);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createListNodePh, std::bind(&ExpressionTree::createListNode, _1, _2, _3), 3);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createEmptyListPh, std::bind(&ExpressionTree::createEmptyList, _1, _2), 2);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createPathNodePh, std::bind(&ExpressionTree::createPathNode, _1, _2, _3), 3);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createVariableNodePh, std::bind(&ExpressionTree::createVariableNode, _1, _2, _3), 3);

BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createConstBoolNodePh, std::bind(&ExpressionTree::createConstBoolNode, _1, _2, _3), 3);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createConstIntNodePh, std::bind(&ExpressionTree::createConstIntNode, _1, _2, _3), 3);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createConstDoubleNodePh, std::bind(&ExpressionTree::createConstDoubleNode, _1, _2, _3), 3);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createConstStringNodePh, std::bind(&ExpressionTree::createConstStringNode, _1, _2, _3), 3);

BOOST_PHOENIX_ADAPT_FUNCTION(void, appendListNodePh, std::bind(&ExpressionTree::appendListNode, _1, _2, _3), 3);
BOOST_PHOENIX_ADAPT_FUNCTION(void, appendPathNodePh, std::bind(&ExpressionTree::appendPathNode, _1, _2, _3), 3);
BOOST_PHOENIX_ADAPT_FUNCTION(PExpressionNode, createNoneNodePh, std::bind(&ExpressionTree::createNoneNode, _1, _2), 2);

class SyntaxErrorImpl
    : public SyntaxError
{
public:
    template <typename CharIter>
    SyntaxErrorImpl(const std::string& expected, CharIter first, CharIter pos, CharIter end)
        : SyntaxError(generateMsg(expected, pos, end))
        , m_errorPos(pos - first)
    { }

    template <typename CharIter>
    SyntaxErrorImpl(CharIter first, CharIter pos, CharIter end)
        : SyntaxError(generateMsg(pos, end))
        , m_errorPos(pos - first)
    { }

private:
    template <typename CharIter>
    std::string generateMsg(const std::string& expected, CharIter pos, CharIter end)
    {
        std::ostringstream oss;
        oss << "SyntaxError: Expecting '" << expected << "' but got '" << std::string(pos, end) << "'";
        return oss.str();
    }

    template <typename CharIter>
    std::string generateMsg(CharIter pos, CharIter end)
    {
        std::ostringstream oss;
        oss << "SyntaxError: Extra symbols found at: '" << std::string(pos, end) << "'";
        return oss.str();
    }

    virtual size_t errorPos() const
    {
        return m_errorPos;
    }

private:
    size_t m_errorPos;
};

template <typename Exception>
struct ThrowSyntaxError {
    template <typename, typename = void, typename = void>
    struct result { typedef void type; };

    template <typename ErrorInfo, typename Iterator>
    void operator() (ErrorInfo const& what, Iterator first, Iterator errPos, Iterator last) const
    {
        std::stringstream ss;
        ss << what;
        throw Exception(ss.str(), first, errPos, last);
    }
};

boost::phoenix::function<ThrowSyntaxError<SyntaxErrorImpl>> throwError;

template <typename Iterator>
struct ExpressionGrammar : qi::grammar<Iterator, ascii::space_type, PExpressionNode()>
{
    ExpressionGrammar(ExpressionTree * tree, bool strict) : ExpressionGrammar::base_type(expr)
    {
        using qi::int_;
        using qi::double_;
        using qi::char_;
        using qi::_val;
        using qi::_1;
        using qi::_2;
        using qi::_3;
        using qi::_4;
        using qi::on_error;
        using qi::fail;
        using qi::lit;
        using qi::alpha;
        using qi::alnum;
        using qi::lexeme;
        using qi::eps;
        using boost::phoenix::new_;
        using boost::phoenix::construct;
        using boost::phoenix::push_back;
        using boost::phoenix::val;
        using boost::phoenix::ref;

        expr =
            logical_and_expression[_val = _1]
            >> *("or" >> logical_and_expression[_val = createOrNodePh(val(tree), _val, _1, strict)])
            ;

        logical_and_expression =
            logical_not_expression[_val = _1]
            >> *("and" >> logical_not_expression[_val = createAndNodePh(val(tree), _val, _1, strict)])
            ;

        logical_not_expression =
            ("not" >> comparison_expr[_val = createNotNodePh(val(tree), _1, strict)])
            | comparison_expr[_val = _1];

        comparison_expr =
            additive_expression[_val = _1]
            >> *(("in" >> additive_expression[_val = createInNodePh(val(tree), _val, _1, strict)])
                | ("not" >> lit("in") >> additive_expression[_val = createNotInNodePh(val(tree), _val, _1, strict)])
                | ("is" >> lit("not") >> additive_expression[_val = createIsNotNodePh(val(tree), _val, _1, strict)])
                | ("is" >> additive_expression[_val = createIsNodePh(val(tree), _val, _1, strict)])
                | ("<" >> additive_expression[_val = createLessNodePh(val(tree), _val, _1, strict)])
                | ("<=" >> additive_expression[_val = createLessEqualNodePh(val(tree), _val, _1, strict)])
                | (">" >> additive_expression[_val = createGreaterNodePh(val(tree), _val, _1, strict)])
                | (">=" >> additive_expression[_val = createGreaterEqualNodePh(val(tree), _val, _1, strict)])
                | ("!=" >> additive_expression[_val = createNotEqualNodePh(val(tree), _val, _1, strict)])
                | ("==" >> additive_expression[_val = createEqualNodePh(val(tree), _val, _1, strict)])
                )
            ;

        additive_expression =
            term[_val = _1]
            >> *(('+' >> term[_val = createPlusNodePh(val(tree), _val, _1, strict)])
                | ('-' >> term[_val = createMinusNodePh(val(tree), _val, _1, strict)])
                )
            ;

        term =
            factor[_val = _1]
            >> *(('*' >> factor[_val = createMulNodePh(val(tree), _val, _1, strict)])
                | ('/' >> factor[_val = createDivNodePh(val(tree), _val, _1, strict)])
                | ("//" >> factor[_val = createFloorDivNodePh(val(tree), _val, _1, strict)])
                | ('%' >> factor[_val = createReminderNodePh(val(tree), _val, _1, strict)])
                )
            ;

        factor =
            pyatom[_val = _1]
            | '-' > factor[_val = createNegateNodePh(val(tree), _1, strict)]
            | '+' > factor[_val = _1];

        pyatom =
            pyliteral[_val = _1]
            | path[_val = _1]
            | pyidentifier[_val = _1]
            | pyenclosure[_val = _1];

        pyenclosure =
            pyparenth_form[_val = _1]
            | list_display[_val = _1];

        pyparenth_form =
            '(' >> expr[_val = _1] >> ')';

        expr_comma_list =
            expr[_val = createListNodePh(val(tree), _1, strict)] >> *(',' >> expr[appendListNodePh(val(tree), _val, _1)]) >> -char_(',');

        list_display =
            char_('[') >> char_(']') >> eps[_val = createEmptyListPh(val(tree), strict)]
            | char_('(') >> char_(')') >> eps[_val = createEmptyListPh(val(tree), strict)]
            | '[' > expr_comma_list[_val = _1] >> ']'
            | '(' > expr_comma_list[_val = _1] > ')';

        pyidentifier_raw =
            qi::raw[lexeme[(alpha | char_('_')) >> *(alnum | char_('_'))]][_val = _1]; // raw makes attribute to be an iterator pair


        pyidentifier =
            pyidentifier_raw[_val = createVariableNodePh(val(tree), _1, strict)];

        pyliteral =
            pystringliteral[_val = _1]
            | (pyfloatnumber[_val = _1]
            | pyinteger[_val = _1])
            | lit("True")[_val = createConstBoolNodePh(val(tree),true, strict)]
            | lit("False")[_val = createConstBoolNodePh(val(tree),false, strict)]
            | lit("None")[_val = createNoneNodePh(val(tree), strict)];

        pystringliteral =
            qi::raw[lexeme['"' > *(char_ - '"') > '"']][_val = createConstStringNodePh(val(tree), _1, strict)];

        pyinteger =
            int_[_val = createConstIntNodePh(val(tree), _1, strict)];

        static qi::real_parser<double, qi::strict_real_policies<double>> const _strict_double;
        pyfloatnumber =
            _strict_double[_val = createConstDoubleNodePh(val(tree), _1, strict)];

        path =
            pyidentifier_raw[_val = createPathNodePh(val(tree), _1, strict)]
            >> +('.' > pyidentifier_raw[appendPathNodePh(val(tree), _val, _1)]);

        BOOST_SPIRIT_DEBUG_NODE(expr);
        BOOST_SPIRIT_DEBUG_NODE(logical_and_expression);
        BOOST_SPIRIT_DEBUG_NODE(logical_not_expression);
        BOOST_SPIRIT_DEBUG_NODE(comparison_expr);
        BOOST_SPIRIT_DEBUG_NODE(additive_expression);
        BOOST_SPIRIT_DEBUG_NODE(term);
        BOOST_SPIRIT_DEBUG_NODE(factor);
        BOOST_SPIRIT_DEBUG_NODE(operand);
        BOOST_SPIRIT_DEBUG_NODE(pyatom);
        BOOST_SPIRIT_DEBUG_NODE(pyliteral);
        BOOST_SPIRIT_DEBUG_NODE(pyenclosure);
        BOOST_SPIRIT_DEBUG_NODE(pyparenth_form);
        BOOST_SPIRIT_DEBUG_NODE(list_display);
        BOOST_SPIRIT_DEBUG_NODE(expr_comma_list);
        BOOST_SPIRIT_DEBUG_NODE(pyidentifier);
        BOOST_SPIRIT_DEBUG_NODE(path);
        BOOST_SPIRIT_DEBUG_NODE(pyidentifier_raw);
        BOOST_SPIRIT_DEBUG_NODE(pystringliteral);
        BOOST_SPIRIT_DEBUG_NODE(pyinteger);
        BOOST_SPIRIT_DEBUG_NODE(pyfloatnumber);

        expr.name("expression");
        logical_and_expression.name("and-expression");
        logical_not_expression.name("or-expression");
        comparison_expr.name("cmp-expression");
        additive_expression.name("add-expression");
        term.name("term-expression");
        factor.name("factor-expression");
        operand.name("operand-expression");
        pyatom.name("atom-expression");
        pyliteral.name("literal-expression");
        pyenclosure.name("enclosure-expression");
        pyparenth_form.name("parenth-expression");
        list_display.name("list-expression");
        expr_comma_list.name("comma-separated-list");
        pyidentifier.name("identifier");
        path.name("path");
        pyidentifier_raw.name("identifier");
        pystringliteral.name("literal");
        pyinteger.name("integer");
        pyfloatnumber.name("float");

        on_error<fail>
            (
                expr
                , throwError(_4, _1, _3, _2)
            );
    }

    // rules that start with py are matching to rules from Python3x's BNF 
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> expr;

    qi::rule<Iterator, ascii::space_type, PExpressionNode()> logical_and_expression;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> logical_not_expression;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> comparison_expr;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> additive_expression;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> term;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> factor;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> operand;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> pyatom;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> pyliteral;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> pyenclosure;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> pyparenth_form;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> list_display;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> expr_comma_list;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> pyidentifier;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> path;
    qi::rule<Iterator, ascii::space_type, boost::iterator_range<Iterator>> pyidentifier_raw;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> pystringliteral;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> pyinteger;
    qi::rule<Iterator, ascii::space_type, PExpressionNode()> pyfloatnumber;
};

PExpressionNode parseExpressionNode(ExpressionTree& res, const char* first, const char* last, bool strict)
{
    using boost::spirit::ascii::space;
    using boost::spirit::qi::phrase_parse;
    typedef ExpressionGrammar<const char*> Grammar;

    const char* firstStore = first; // Keep for error position calculation

    Grammar grammar(&res, strict);

    PExpressionNode root = 0;
    bool r = phrase_parse(first, last, grammar, space, root);


    if (first != last) { // Not a full match
        throw SyntaxErrorImpl(firstStore, first, last);
    }

    return root;
}

ExpressionTree parseExpression(const char* first, const char* last, bool strict)
{
    ExpressionTree res;
    res.assignRoot(parseExpressionNode(res, first, last, strict));
    return res;
}

void parseAndConcatExpression(ExpressionTree& res, const char* first, const char* last, bool strict)
{
    res.concatRoot(parseExpressionNode(res, first, last, strict));
}

} // namespace ql
