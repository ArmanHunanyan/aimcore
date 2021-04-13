#include "qlparser.h"

#include <boost/python.hpp>

class Bet
{
public:
    Bet(const boost::python::str& str, bool strict)
        : m_tree()
        , m_strict(strict)
    {
        parse(str);
    }

    Bet(bool strict)
        : m_tree()
        , m_strict(strict)
    {
    }

    Bet(const Bet&) = delete;
    Bet& operator= (const Bet&) = delete;

    void parse(const boost::python::str& str)
    {
        const char* begin = boost::python::extract<const char*>(str);
        const char* end = begin + boost::python::len(str);
        m_tree = ql::parseExpression(begin, end, m_strict);
    }

    void concat(const boost::python::str& str)
    {
        const char* begin = boost::python::extract<const char*>(str);
        const char* end = begin + boost::python::len(str);
        ql::parseAndConcatExpression(m_tree, begin, end, m_strict);
    }

    bool match(const boost::python::dict& dict) const
    {
        bool res = m_tree.evalBool(dict);
        return res;
    }

private:
    ql::ExpressionTree m_tree;
    bool m_strict;
};

void syntaxErrorTransliator(ql::SyntaxError const& x) {
    PyErr_SetString(PyExc_RuntimeError, x.what());
}

BOOST_PYTHON_MODULE(cql)
{
    using boost::python::class_;
    using boost::python::def;
    using boost::python::init;
    using boost::python::register_exception_translator;

    register_exception_translator<
        ql::SyntaxError>(syntaxErrorTransliator);

    class_<Bet, boost::noncopyable>("bet", init<boost::python::str, bool>())
        .def(init<bool>())
        .def("concat", &Bet::concat)
        .def("parse", &Bet::match)
        .def("match", &Bet::match)
        ;
}
