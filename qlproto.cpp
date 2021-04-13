#include <iostream>
#include <cassert>
#include <boost/python.hpp>

#include "ql/qlparser.h"
#include "db/rocksdb.h"
#include "db/serializer.h"

#include <limits>

void test();
void testSerializerWTypeWSizeInt();
using namespace ql;

int main()
{
    test();
    return 0;
    std::string str;
    while (std::getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        ExpressionTree exp;

        try {
            exp = parseExpression(str.c_str(), str.c_str() + str.size(), true);
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded: " << exp.toString() << "\n";
            boost::python::dict variables;
            variables["a"] = 5;
            variables["b"] = "test";
            boost::python::list tmplist;
            tmplist.append(12);
            tmplist.append(13);
            variables["c"] = tmplist;
            boost::python::dict tmp;
            tmp["cc"] = 500;
            boost::python::dict tmp2;
            tmp["bb"] = tmp;
            variables["aa"] = tmp2;
            std::cout << "Result           : " << boost::python::extract<std::string>(exp.eval(variables))() << "\n";
            std::cout << "-------------------------\n";
        }
        catch (const std::exception& err) {
            std::cerr << "Error: " << err.what() << std::endl;
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}

template <typename T>
struct PrintHelper
{
    static std::string print(const T& a)
    {
        std::ostringstream oss;
        oss << a;
        return oss.str();
    }
};

template <typename T>
struct PrintHelper<boost::python::extract<T>>
{
    static std::string print(const boost::python::extract<T>& a)
    {
        return PrintHelper<T>::print(a());
    }
};

template <>
struct PrintHelper<boost::python::list>
{
    static std::string print(const boost::python::list& a)
    {
        return boost::python::extract<std::string>(boost::python::str(a));
    }
};

template <>
struct PrintHelper<boost::python::object>
{
    static std::string print(const boost::python::object& a)
    {
        return boost::python::extract<std::string>(boost::python::str(a));
    }
};

template <>
struct PrintHelper<boost::python::long_>
{
    static std::string print(const boost::python::long_& a)
    {
        return boost::python::extract<std::string>(boost::python::str(a));
    }
};

template <typename T>
struct EvalHelper
{
    static T eval(const T& a) { return a; }
};

template <unsigned int n>
struct EvalHelper<char[n]>
{
    static std::string eval(const char * a) { return std::string(a); }
};

template <typename T>
struct EvalHelper<boost::python::extract<T>>
{
    static T eval(const boost::python::extract<T>& a) { return a(); }
};

template <typename T1, typename T2>
bool testEqual(const T1& a, const T2& b)
{
    if (EvalHelper<T1>::eval(a) != EvalHelper<T2>::eval(b)) {
        auto str1 = PrintHelper<T1>::print(a);
        auto str2 = PrintHelper<T2>::print(b);
        std::cerr << "Expected \'" << str1 << "\' but got \'" << str2 << "\'" << std::endl;
        return false;
    }
    return true;
}

#define TEST_EQUAL(a, b) try { if (!testEqual(a, b))  { assert(false); } } catch (boost::python::error_already_set) { PyErr_Print(); assert(false);} catch (...) { assert(false); }

#define TEST_THROW(exp) \
    try { \
        exp; \
        std::cerr << "Expected to throw but finished successfully" << std::endl; \
        assert(false); \
    } catch(boost::python::error_already_set) { \
        PyErr_Clear(); \
    } catch(const std::exception& err) { \
        std::cerr << "Unexpected exception is thrown: " << err.what() << std::endl; \
        assert(false); \
    } catch (...) { \
            std::cerr << "Unexpected exception is thrown: Unknown error" << std::endl; \
        assert(false); \
    }

#define TEST_NOTHROW(exp) \
    try { \
        exp; \
    } catch(boost::python::error_already_set) { \
        std::cerr << "Unexpected exception is thrown: "; \
        PyErr_Print(); \
        std::cerr << std::endl; \
        assert(false); \
    } catch(const std::exception& err) { \
        std::cerr << "Unexpected exception is thrown: " << err.what() << std::endl; \
        assert(false); \
    } catch (...) { \
        std::cerr << "Unexpected exception is thrown: Unknown error" << std::endl; \
        assert(false); \
    }

const bool StrictCmp = true;
const bool NonStrictCmp = false;

void testConstantsStrict()
{
    ExpressionTree exp = parseExpressionStr("5", StrictCmp);
    TEST_EQUAL("i:5", exp.toString());
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("-5", StrictCmp);
    TEST_EQUAL("i:-5", exp.toString());
    TEST_EQUAL(-5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True", StrictCmp);
    TEST_EQUAL("b:True", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0", StrictCmp);
    TEST_EQUAL("d:5.0", exp.toString());
    TEST_EQUAL(5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("-5.0", StrictCmp);
    TEST_EQUAL("d:-5.0", exp.toString());
    TEST_EQUAL(-5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"", StrictCmp);
    TEST_EQUAL("s:teststr", exp.toString());
    TEST_EQUAL(std::string("teststr"), boost::python::extract<std::string>(exp.eval(boost::python::dict())));
}

void testNoneStrict()
{
    ExpressionTree exp = parseExpressionStr("None", StrictCmp);
    TEST_EQUAL("n:None", exp.toString());
    TEST_EQUAL(true, exp.eval(boost::python::dict()).is_none());
}

void testVarsAndPathsStrict()
{
    ExpressionTree exp = parseExpressionStr("foo", StrictCmp);
    TEST_EQUAL("v:foo", exp.toString());
    TEST_EQUAL(true, exp.eval(boost::python::dict()).is_none());

    boost::python::dict vars;
    vars["foo"] = 5;
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(vars)));

    exp = parseExpressionStr("foo.bar", StrictCmp);
    TEST_EQUAL("p:foo.bar", exp.toString());
    TEST_EQUAL(true, exp.eval(boost::python::dict()).is_none());

    boost::python::dict foo;
    foo["bar"] = 500;
    vars["foo"] = foo;
    TEST_EQUAL(500, boost::python::extract<int>(exp.eval(vars)));

    foo["bar"] = std::string("tests");
    vars["foo"] = foo;
    TEST_EQUAL(std::string("tests"), boost::python::extract<std::string>(exp.eval(vars)));

    vars["foo"] = boost::python::object();
    TEST_EQUAL(true, exp.eval(vars).is_none());
}

template <typename T>
boost::python::list pyList(const std::initializer_list<T>& list)
{
    boost::python::list res;
    for (const auto elem : list) {
        res.append(elem);
    }
    return res;
}

template <typename T>
boost::python::list& pyListHelper(boost::python::list& res, const T& a)
{
    res.append(a);
    return res;
}

template <typename T, typename... Args>
boost::python::list& pyListHelper(boost::python::list& res, const T& a, Args... args)
{
    res.append(a);
    return pyListHelper(res, args...);
}

template <typename... Args>
boost::python::list pyList(Args... args)
{
    boost::python::list res;
    return pyListHelper(res, args...);
}

void testList1Strict()
{
    ExpressionTree exp = parseExpressionStr("[5]", StrictCmp);
    TEST_EQUAL("l:[i:5]", exp.toString());
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5,]", StrictCmp);
    TEST_EQUAL("l:[i:5]", exp.toString());
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[-5]", StrictCmp);
    TEST_EQUAL("l:[i:-5]", exp.toString());
    TEST_EQUAL(pyList({ -5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[-5,]", StrictCmp);
    TEST_EQUAL("l:[i:-5]", exp.toString());
    TEST_EQUAL(pyList({ -5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[False]", StrictCmp);
    TEST_EQUAL("l:[b:False]", exp.toString());
    TEST_EQUAL(pyList({ false }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[False,]", StrictCmp);
    TEST_EQUAL("l:[b:False]", exp.toString());
    TEST_EQUAL(pyList({ false }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5.0]", StrictCmp);
    TEST_EQUAL("l:[d:5.0]", exp.toString());
    TEST_EQUAL(pyList({ 5.0 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5.0,]", StrictCmp);
    TEST_EQUAL("l:[d:5.0]", exp.toString());
    TEST_EQUAL(pyList({ 5.0 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[-5.0]", StrictCmp);
    TEST_EQUAL("l:[d:-5.0]", exp.toString());
    TEST_EQUAL(pyList({ -5.0 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[-5.0,]", StrictCmp);
    TEST_EQUAL("l:[d:-5.0]", exp.toString());
    TEST_EQUAL(pyList({ -5.0 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[\"teststr\"]", StrictCmp);
    TEST_EQUAL("l:[s:teststr]", exp.toString());
    TEST_EQUAL(pyList({ std::string("teststr") }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[\"teststr\",]", StrictCmp);
    TEST_EQUAL("l:[s:teststr]", exp.toString());
    TEST_EQUAL(pyList({ std::string("teststr") }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[None]", StrictCmp);
    TEST_EQUAL("l:[n:None]", exp.toString());
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[None,]", StrictCmp);
    TEST_EQUAL("l:[n:None]", exp.toString());
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
}

void testList1TupleStrict()
{
    ExpressionTree exp = parseExpressionStr("(5,)", StrictCmp);
    TEST_EQUAL("l:[i:5]", exp.toString());
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(-5,)", StrictCmp);
    TEST_EQUAL("l:[i:-5]", exp.toString());
    TEST_EQUAL(pyList({ -5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(False,)", StrictCmp);
    TEST_EQUAL("l:[b:False]", exp.toString());
    TEST_EQUAL(pyList({ false }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(5.0,)", StrictCmp);
    TEST_EQUAL("l:[d:5.0]", exp.toString());
    TEST_EQUAL(pyList({ 5.0 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(-5.0,)", StrictCmp);
    TEST_EQUAL("l:[d:-5.0]", exp.toString());
    TEST_EQUAL(pyList({ -5.0 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(\"teststr\",)", StrictCmp);
    TEST_EQUAL("l:[s:teststr]", exp.toString());
    TEST_EQUAL(pyList({ std::string("teststr") }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(None,)", StrictCmp);
    TEST_EQUAL("l:[n:None]", exp.toString());
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
}

template <typename It, typename T>
boost::python::object makePathHelper(It first, It last, const T& val)
{
    if (first == last) {
        return boost::python::object(val);
    }
    boost::python::dict res;
    std::string key = *first++;
    res[key] = makePathHelper(first, last, val);
    return res;
}

template <typename It, typename T>
void addPathHelper(boost::python::dict& res, It first, It last, const T& val)
{
    if (first == last) {
        assert(false);
    }
    std::string key = *first++;
    res[key] = makePathHelper(first, last, val);
}


template <typename T>
boost::python::dict makePath(std::initializer_list<const char*> path, const T& val)
{
    return boost::python::extract<boost::python::dict>(makePathHelper(path.begin(), path.end(), val))();
}

template <typename T>
void addPath(boost::python::dict& res, std::initializer_list<const char*> path, const T& val)
{
    addPathHelper(res, path.begin(), path.end(), val);
}

void testList1VarStrict()
{
    ExpressionTree exp = parseExpressionStr("[foo]", StrictCmp);
    TEST_EQUAL("l:[v:foo]", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars["foo"] = 5;
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("[foo,]", StrictCmp);
    TEST_EQUAL("l:[v:foo]", exp.toString());
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("[foo.bar]", StrictCmp);
    TEST_EQUAL("l:[p:foo.bar]", exp.toString());
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
    TEST_EQUAL(pyList({ 7 }), boost::python::extract<boost::python::list>(exp.eval(makePath({ "foo", "bar" }, 7))));

    exp = parseExpressionStr("[foo.bar,]", StrictCmp);
    TEST_EQUAL("l:[p:foo.bar]", exp.toString());
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
    TEST_EQUAL(pyList({ 7 }), boost::python::extract<boost::python::list>(exp.eval(makePath({ "foo", "bar" }, 7))));
}

void testList1VarTupleStrict()
{
    ExpressionTree exp = parseExpressionStr("(foo,)", StrictCmp);
    TEST_EQUAL("l:[v:foo]", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars["foo"] = 5;
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("(foo.bar,)", StrictCmp);
    TEST_EQUAL("l:[p:foo.bar]", exp.toString());
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
    TEST_EQUAL(pyList({ 7 }), boost::python::extract<boost::python::list>(exp.eval(makePath({ "foo", "bar" }, 7))));
}

void testList2Strict()
{
    ExpressionTree exp = parseExpressionStr("[5, True]", StrictCmp);
    TEST_EQUAL("l:[i:5, b:True]", exp.toString());
    TEST_EQUAL(pyList(5, true), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[-5, None]", StrictCmp);
    TEST_EQUAL("l:[i:-5, n:None]", exp.toString());
    TEST_EQUAL(pyList(-5, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[False, 5]", StrictCmp);
    TEST_EQUAL("l:[b:False, i:5]", exp.toString());
    TEST_EQUAL(pyList(false, 5), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5.0, 7]", StrictCmp);
    TEST_EQUAL("l:[d:5.0, i:7]", exp.toString());
    TEST_EQUAL(pyList(5.0, 7), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[-5.0, None]", StrictCmp);
    TEST_EQUAL("l:[d:-5.0, n:None]", exp.toString());
    TEST_EQUAL(pyList(-5.0, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[\"teststr\", 5]", StrictCmp);
    TEST_EQUAL("l:[s:teststr, i:5]", exp.toString());
    TEST_EQUAL(pyList(std::string("teststr"), 5), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[None, False]", StrictCmp);
    TEST_EQUAL("l:[n:None, b:False]", exp.toString());
    TEST_EQUAL(pyList(boost::python::object(), false), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
}

void testList2TupleStrict()
{
    ExpressionTree exp = parseExpressionStr("(5, True)", StrictCmp);
    TEST_EQUAL("l:[i:5, b:True]", exp.toString());
    TEST_EQUAL(pyList(5, true), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(-5, None)", StrictCmp);
    TEST_EQUAL("l:[i:-5, n:None]", exp.toString());
    TEST_EQUAL(pyList(-5, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(False, 5)", StrictCmp);
    TEST_EQUAL("l:[b:False, i:5]", exp.toString());
    TEST_EQUAL(pyList(false, 5), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(5.0, 7)", StrictCmp);
    TEST_EQUAL("l:[d:5.0, i:7]", exp.toString());
    TEST_EQUAL(pyList(5.0, 7), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(-5.0, None)", StrictCmp);
    TEST_EQUAL("l:[d:-5.0, n:None]", exp.toString());
    TEST_EQUAL(pyList(-5.0, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(\"teststr\", 5)", StrictCmp);
    TEST_EQUAL("l:[s:teststr, i:5]", exp.toString());
    TEST_EQUAL(pyList(std::string("teststr"), 5), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(None, False)", StrictCmp);
    TEST_EQUAL("l:[n:None, b:False]", exp.toString());
    TEST_EQUAL(pyList(boost::python::object(), false), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
}

void testList2VarStrict()
{
    ExpressionTree exp = parseExpressionStr("[foo, bar]", StrictCmp);
    TEST_EQUAL("l:[v:foo, v:bar]", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(pyList({ boost::python::object(), boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars["foo"] = 5;
    TEST_EQUAL(pyList(5, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars["bar"] = true;
    TEST_EQUAL(pyList(5, true), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("[foo.r, bar.r]", StrictCmp);
    TEST_EQUAL("l:[p:foo.r, p:bar.r]", exp.toString());
    TEST_EQUAL(pyList(boost::python::object(), boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars = makePath({ "foo", "r" }, 5);
    TEST_EQUAL(pyList(5, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(vars)));
    addPath(vars, { "bar", "r" }, true);
    TEST_EQUAL(pyList(5, true), boost::python::extract<boost::python::list>(exp.eval(vars)));
}

void testList2VarTupleStrict()
{
    ExpressionTree exp = parseExpressionStr("(foo, bar)", StrictCmp);
    TEST_EQUAL("l:[v:foo, v:bar]", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(pyList(boost::python::object(), boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars["foo"] = 5;
    TEST_EQUAL(pyList(5, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars["bar"] = true;
    TEST_EQUAL(pyList(5, true), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("(foo.r, bar.r)", StrictCmp);
    TEST_EQUAL("l:[p:foo.r, p:bar.r]", exp.toString());
    TEST_EQUAL(pyList(boost::python::object(), boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
    vars = makePath({ "foo", "r" }, 5);
    TEST_EQUAL(pyList(5, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(vars)));
    addPath(vars, { "bar", "r" }, true);
    TEST_EQUAL(pyList(5, true), boost::python::extract<boost::python::list>(exp.eval(vars)));
}

void testNotWithConstantsStrict()
{
    ExpressionTree exp = parseExpressionStr("not 5", StrictCmp);
    TEST_EQUAL("not(i:5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not -5", StrictCmp);
    TEST_EQUAL("not(i:-5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not True", StrictCmp);
    TEST_EQUAL("not(b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not False", StrictCmp);
    TEST_EQUAL("not(b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not 5.0", StrictCmp);
    TEST_EQUAL("not(d:5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not -5.0", StrictCmp);
    TEST_EQUAL("not(d:-5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not \"teststr\"", StrictCmp);
    TEST_EQUAL("not(s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not [5]", StrictCmp);
    TEST_EQUAL("not(l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not (5,)", StrictCmp);
    TEST_EQUAL("not(l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not None", StrictCmp);
    TEST_EQUAL("not(n:None)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));
}

void testNotWithVarsStrict()
{
    ExpressionTree exp = parseExpressionStr("not foo", StrictCmp);
    TEST_EQUAL("not(v:foo)", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    
    vars["foo"] = 5;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["foo"] = 5.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    vars["foo"] = "teststr";
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
}

void testNegateWithConstantsStrict()
{
    ExpressionTree exp = parseExpressionStr("--5", StrictCmp);
    TEST_EQUAL("-(i:-5)", exp.toString());
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("-True", StrictCmp);
    TEST_EQUAL("-(b:True)", exp.toString());
    TEST_EQUAL(-1, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("-False", StrictCmp);
    TEST_EQUAL("-(b:False)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("--5.0", StrictCmp);
    TEST_EQUAL("-(d:-5.0)", exp.toString());
    TEST_EQUAL(5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("-\"teststr\"", StrictCmp);
    TEST_EQUAL("-(s:teststr)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
   
    exp = parseExpressionStr("-[5]", StrictCmp);
    TEST_EQUAL("-(l:[i:5])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("-(5,)", StrictCmp);
    TEST_EQUAL("-(l:[i:5])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("-None", StrictCmp);
    TEST_EQUAL("-(n:None)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testNegateWithVarsStrict()
{
    ExpressionTree exp = parseExpressionStr("-foo", StrictCmp);
    TEST_EQUAL("-(v:foo)", exp.toString());
    boost::python::dict vars;
    TEST_THROW(exp.eval(vars));

    vars["foo"] = 5;
    TEST_EQUAL(-5, boost::python::extract<int>(exp.eval(vars)));

    vars["foo"] = 5.0;
    TEST_EQUAL(-5.0, boost::python::extract<double>(exp.eval(vars)));

    vars["foo"] = true;
    TEST_EQUAL(-1, boost::python::extract<int>(exp.eval(vars)));

    vars["foo"] = false;
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(vars)));

    vars["foo"] = "teststr";
    TEST_THROW(exp.eval(vars));

    vars["foo"] = pyList({ 5 });
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("-foo.bar", StrictCmp);
    TEST_EQUAL("-(p:foo.bar)", exp.toString());
    TEST_THROW(exp.eval(vars));

    vars = makePath({ "foo", "bar" }, 5);
    TEST_EQUAL(-5, boost::python::extract<int>(exp.eval(vars)));

    vars = makePath({ "foo", "bar" }, 5.0);
    TEST_EQUAL(-5.0, boost::python::extract<double>(exp.eval(vars)));

    vars = makePath({ "foo", "bar" }, true);
    TEST_EQUAL(-1, boost::python::extract<int>(exp.eval(vars)));

    vars = makePath({ "foo", "bar" }, false);
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(vars)));

    vars = makePath({ "foo", "bar" }, "teststr");
    TEST_THROW(exp.eval(vars));

    vars = makePath({ "foo", "bar" }, pyList({ 5 }));
    TEST_THROW(exp.eval(vars));
}

void testEqualWithConstantsStrict()
{
    ExpressionTree exp = parseExpressionStr("5==5", StrictCmp);
    TEST_EQUAL("==(i:5, i:5)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5==6", StrictCmp);
    TEST_EQUAL("==(i:5, i:6)", exp.toString());
    TEST_EQUAL(false , boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True==True", StrictCmp);
    TEST_EQUAL("==(b:True, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True==(not False)", StrictCmp);
    TEST_EQUAL("==(b:True, not(b:False))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True==False", StrictCmp);
    TEST_EQUAL("==(b:True, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0==5.0", StrictCmp);
    TEST_EQUAL("==(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0==6.0", StrictCmp);
    TEST_EQUAL("==(d:5.0, d:6.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"==\"teststr\"", StrictCmp);
    TEST_EQUAL("==(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"==\"teststr1\"", StrictCmp);
    TEST_EQUAL("==(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]==(5,)", StrictCmp);
    TEST_EQUAL("==(l:[i:5], l:[i:5])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]==(5.0,)", StrictCmp);
    TEST_EQUAL("==(l:[i:5], l:[d:5.0])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]==(6,)", StrictCmp);
    TEST_EQUAL("==(l:[i:5], l:[i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]==[5, 6]", StrictCmp);
    TEST_EQUAL("==(l:[i:5], l:[i:5, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None==None", StrictCmp);
    TEST_EQUAL("==(n:None, n:None)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));


    exp = parseExpressionStr("1==True", StrictCmp);
    TEST_EQUAL("==(i:1, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2==True", StrictCmp);
    TEST_EQUAL("==(i:2, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0==True", StrictCmp);
    TEST_EQUAL("==(d:1.0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2.0==True", StrictCmp);
    TEST_EQUAL("==(d:2.0, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1==1.0", StrictCmp);
    TEST_EQUAL("==(i:1, d:1.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2==1.0", StrictCmp);
    TEST_EQUAL("==(i:2, d:1.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));
}

void testEqualWithVarsStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("1==foo", StrictCmp);
    TEST_EQUAL("==(i:1, v:foo)", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("False==foo", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(b:False, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("\"teststr\"==foo", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(s:teststr, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("[0]==foo", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(l:[i:0], v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    // lhs is variable
    exp = parseExpressionStr("foo==1", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(v:foo, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo==False", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(v:foo, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo==\"teststr\"", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(v:foo, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo==[0]", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(v:foo, l:[i:0])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    // both operands are variables
    exp = parseExpressionStr("foo==bar", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(v:foo, v:bar)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = false;
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = std::string("teststr");
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = pyList({0});
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
}

void testEqualWithNotStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("(not 1)==(not foo)", StrictCmp);
    TEST_EQUAL("==(not(i:1), not(v:foo))", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not False)==(not foo)", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(not(b:False), not(v:foo))", exp.toString());
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not [0])== (not foo)", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(not(l:[i:0]), not(v:foo))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    
    // lhs is variable
    exp = parseExpressionStr("(not foo)==(not 1)", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(not(v:foo), not(i:1))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not foo)==(not \"teststr\")", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(not(v:foo), not(s:teststr))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
   
    exp = parseExpressionStr("(not foo)==(not [0])", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(not(v:foo), not(l:[i:0]))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
   
    // both operands are variables
    exp = parseExpressionStr("(not foo)==(not bar)", StrictCmp);
    vars.clear();
    TEST_EQUAL("==(not(v:foo), not(v:bar))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    vars["bar"] = pyList({ 6 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = boost::python::list();
    vars["bar"] = pyList({ 6 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("str1");
    vars["bar"] = std::string("str2");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("");
    vars["bar"] = std::string("str2");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
}

void testNotEqualWithConstantsStrict()
{
    ExpressionTree exp = parseExpressionStr("5!=5", StrictCmp);
    TEST_EQUAL("!=(i:5, i:5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5!=6", StrictCmp);
    TEST_EQUAL("!=(i:5, i:6)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True!=True", StrictCmp);
    TEST_EQUAL("!=(b:True, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True!=False", StrictCmp);
    TEST_EQUAL("!=(b:True, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0!=5.0", StrictCmp);
    TEST_EQUAL("!=(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0!=6.0", StrictCmp);
    TEST_EQUAL("!=(d:5.0, d:6.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"!=\"teststr\"", StrictCmp);
    TEST_EQUAL("!=(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"!=\"teststr1\"", StrictCmp);
    TEST_EQUAL("!=(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]!=(5,)", StrictCmp);
    TEST_EQUAL("!=(l:[i:5], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]!=(6,)", StrictCmp);
    TEST_EQUAL("!=(l:[i:5], l:[i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]!=[5, 6]", StrictCmp);
    TEST_EQUAL("!=(l:[i:5], l:[i:5, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None!=None", StrictCmp);
    TEST_EQUAL("!=(n:None, n:None)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));


    exp = parseExpressionStr("1!=True", StrictCmp);
    TEST_EQUAL("!=(i:1, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2!=True", StrictCmp);
    TEST_EQUAL("!=(i:2, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0!=True", StrictCmp);
    TEST_EQUAL("!=(d:1.0, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2.0!=True", StrictCmp);
    TEST_EQUAL("!=(d:2.0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1!=1.0", StrictCmp);
    TEST_EQUAL("!=(i:1, d:1.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2!=1.0", StrictCmp);
    TEST_EQUAL("!=(i:2, d:1.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));
}

void testNotEqualWithVarsStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("1!=foo", StrictCmp);
    TEST_EQUAL("!=(i:1, v:foo)", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("False!=foo", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(b:False, v:foo)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("\"teststr\"!=foo", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(s:teststr, v:foo)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("[0]!=foo", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(l:[i:0], v:foo)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    // lhs is variable
    exp = parseExpressionStr("foo!=1", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(v:foo, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo!=False", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(v:foo, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo!=\"teststr\"", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(v:foo, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo!=[0]", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(v:foo, l:[i:0])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    // both operands are variables
    exp = parseExpressionStr("foo!=bar", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(v:foo, v:bar)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = false;
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = std::string("teststr");
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = pyList({ 0 });
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
}

void testNotEqualWithNotStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("(not 1)!=(not foo)", StrictCmp);
    TEST_EQUAL("!=(not(i:1), not(v:foo))", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not False)!=(not foo)", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(not(b:False), not(v:foo))", exp.toString());
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not [0])!= (not foo)", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(not(l:[i:0]), not(v:foo))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    // lhs is variable
    exp = parseExpressionStr("(not foo)!=(not 1)", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(not(v:foo), not(i:1))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not foo)!=(not \"teststr\")", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(not(v:foo), not(s:teststr))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not foo)!=(not [0])", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(not(v:foo), not(l:[i:0]))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    // both operands are variables
    exp = parseExpressionStr("(not foo)!=(not bar)", StrictCmp);
    vars.clear();
    TEST_EQUAL("!=(not(v:foo), not(v:bar))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    vars["bar"] = pyList({ 6 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = boost::python::list();
    vars["bar"] = pyList({ 6 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("str1");
    vars["bar"] = std::string("str2");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("");
    vars["bar"] = std::string("str2");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
}

void testLessWithConstantsStrict()
{
    ExpressionTree exp = parseExpressionStr("5<5", StrictCmp);
    TEST_EQUAL("<(i:5, i:5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5<6", StrictCmp);
    TEST_EQUAL("<(i:5, i:6)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True<True", StrictCmp);
    TEST_EQUAL("<(b:True, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False<True", StrictCmp);
    TEST_EQUAL("<(b:False, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0<5.0", StrictCmp);
    TEST_EQUAL("<(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0<6.0", StrictCmp);
    TEST_EQUAL("<(d:5.0, d:6.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"<\"teststr\"", StrictCmp);
    TEST_EQUAL("<(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"<\"teststr1\"", StrictCmp);
    TEST_EQUAL("<(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<(5,)", StrictCmp);
    TEST_EQUAL("<(l:[i:5], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<(5.0, 6, False)", StrictCmp);
    TEST_EQUAL("<(l:[i:5], l:[d:5.0, i:6, b:False])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<(6,)", StrictCmp);
    TEST_EQUAL("<(l:[i:5], l:[i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[False]<[1, 6]", StrictCmp);
    TEST_EQUAL("<(l:[b:False], l:[i:1, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None<None", StrictCmp);
    TEST_EQUAL("<(n:None, n:None)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));


    exp = parseExpressionStr("1<True", StrictCmp);
    TEST_EQUAL("<(i:1, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0<True", StrictCmp);
    TEST_EQUAL("<(i:0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0<True", StrictCmp);
    TEST_EQUAL("<(d:1.0, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0.0<True", StrictCmp);
    TEST_EQUAL("<(d:0.0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<1.0", StrictCmp);
    TEST_EQUAL("<(i:1, d:1.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<1.1", StrictCmp);
    TEST_EQUAL("<(i:1, d:1.1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<[1.1]", StrictCmp);
    TEST_EQUAL("<(i:1, l:[d:1.1])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testLessWithVarsStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("1<foo", StrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("<(i:1, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("False<foo", StrictCmp);
    vars.clear();
    TEST_EQUAL("<(b:False, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.2;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("\"teststr\"<foo", StrictCmp);
    vars.clear();
    TEST_EQUAL("<(s:teststr, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststs");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("[0]<foo", StrictCmp);
    vars.clear();
    TEST_EQUAL("<(l:[i:0], v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // lhs is variable
    exp = parseExpressionStr("foo<1", StrictCmp);
    vars.clear();
    TEST_EQUAL("<(v:foo, i:1)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.9;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo<False", StrictCmp);
    vars.clear();
    TEST_EQUAL("<(v:foo, b:False)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -0.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo<\"teststr\"", StrictCmp);
    vars.clear();
    TEST_EQUAL("<(v:foo, s:teststr)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststq");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo<[0]", StrictCmp);
    vars.clear();
    TEST_EQUAL("<(v:foo, l:[i:0])", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // both operands are variables
    exp = parseExpressionStr("foo<bar", StrictCmp);
    vars.clear();
    TEST_EQUAL("<(v:foo, v:bar)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.5;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = false;
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -0.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = std::string("teststr");
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("testst");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr1");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = pyList({ 0 });
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = boost::python::list();
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));
}

void testLessEqWithConstantsStrict()
{
    ExpressionTree exp = parseExpressionStr("5<=4", StrictCmp);
    TEST_EQUAL("<=(i:5, i:4)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5<=5", StrictCmp);
    TEST_EQUAL("<=(i:5, i:5)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5<=6", StrictCmp);
    TEST_EQUAL("<=(i:5, i:6)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True<=False", StrictCmp);
    TEST_EQUAL("<=(b:True, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True<=True", StrictCmp);
    TEST_EQUAL("<=(b:True, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False<=True", StrictCmp);
    TEST_EQUAL("<=(b:False, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0<=4.0", StrictCmp);
    TEST_EQUAL("<=(d:5.0, d:4.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0<=5.0", StrictCmp);
    TEST_EQUAL("<=(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0<=6.0", StrictCmp);
    TEST_EQUAL("<=(d:5.0, d:6.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr1\"<=\"teststr\"", StrictCmp);
    TEST_EQUAL("<=(s:teststr1, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"<=\"teststr\"", StrictCmp);
    TEST_EQUAL("<=(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"<=\"teststr1\"", StrictCmp);
    TEST_EQUAL("<=(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<=(4,)", StrictCmp);
    TEST_EQUAL("<=(l:[i:5], l:[i:4])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5, 1]<=(5,)", StrictCmp);
    TEST_EQUAL("<=(l:[i:5, i:1], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<=(4,)", StrictCmp);
    TEST_EQUAL("<=(l:[i:5], l:[i:4])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<=(5.0, 6, False)", StrictCmp);
    TEST_EQUAL("<=(l:[i:5], l:[d:5.0, i:6, b:False])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<=(6,)", StrictCmp);
    TEST_EQUAL("<=(l:[i:5], l:[i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[False]<=[1, 6]", StrictCmp);
    TEST_EQUAL("<=(l:[b:False], l:[i:1, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None<=None", StrictCmp);
    TEST_EQUAL("<=(n:None, n:None)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));


    exp = parseExpressionStr("1<=True", StrictCmp);
    TEST_EQUAL("<=(i:1, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<=False", StrictCmp);
    TEST_EQUAL("<=(i:1, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0<=True", StrictCmp);
    TEST_EQUAL("<=(i:0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0<=False", StrictCmp);
    TEST_EQUAL("<=(i:0, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0<=True", StrictCmp);
    TEST_EQUAL("<=(d:1.0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0.0<=True", StrictCmp);
    TEST_EQUAL("<=(d:0.0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<=1.0", StrictCmp);
    TEST_EQUAL("<=(i:1, d:1.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<=1.1", StrictCmp);
    TEST_EQUAL("<=(i:1, d:1.1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<=[1.1]", StrictCmp);
    TEST_EQUAL("<=(i:1, l:[d:1.1])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testLessEqWithVarsStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("1<=foo", StrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("<=(i:1, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("False<=foo", StrictCmp);
    vars.clear();
    TEST_EQUAL("<=(b:False, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.2;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("\"teststr\"<=foo", StrictCmp);
    vars.clear();
    TEST_EQUAL("<=(s:teststr, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststq");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("[0]<=foo", StrictCmp);
    vars.clear();
    TEST_EQUAL("<=(l:[i:0], v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = boost::python::list();
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // lhs is variable
    exp = parseExpressionStr("foo<=1", StrictCmp);
    vars.clear();
    TEST_EQUAL("<=(v:foo, i:1)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 2;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.9;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo<=False", StrictCmp);
    vars.clear();
    TEST_EQUAL("<=(v:foo, b:False)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -0.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo<=\"teststr\"", StrictCmp);
    vars.clear();
    TEST_EQUAL("<=(v:foo, s:teststr)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststs");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo<=[0]", StrictCmp);
    vars.clear();
    TEST_EQUAL("<=(v:foo, l:[i:0])", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // both operands are variables
    exp = parseExpressionStr("foo<=bar", StrictCmp);
    vars.clear();
    TEST_EQUAL("<=(v:foo, v:bar)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.5;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = false;
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = std::string("teststr");
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr1");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr1");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = pyList({ 0 });
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = boost::python::list();
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));
}

void testGreaterWithConstantsStrict()
{
    ExpressionTree exp = parseExpressionStr("5>5", StrictCmp);
    TEST_EQUAL(">(i:5, i:5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("6>5", StrictCmp);
    TEST_EQUAL(">(i:6, i:5)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>True", StrictCmp);
    TEST_EQUAL(">(b:True, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>False", StrictCmp);
    TEST_EQUAL(">(b:True, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0>5.0", StrictCmp);
    TEST_EQUAL(">(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("6.0>5.0", StrictCmp);
    TEST_EQUAL(">(d:6.0, d:5.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\">\"teststr\"", StrictCmp);
    TEST_EQUAL(">(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr1\">\"teststr\"", StrictCmp);
    TEST_EQUAL(">(s:teststr1, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]>(5,)", StrictCmp);
    TEST_EQUAL(">(l:[i:5], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(5.0, 6, False)>[5]", StrictCmp);
    TEST_EQUAL(">(l:[d:5.0, i:6, b:False], l:[i:5])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[6]>(5,)", StrictCmp);
    TEST_EQUAL(">(l:[i:6], l:[i:5])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[1, 6]>[False]", StrictCmp);
    TEST_EQUAL(">(l:[i:1, i:6], l:[b:False])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None>None", StrictCmp);
    TEST_EQUAL(">(n:None, n:None)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));


    exp = parseExpressionStr("True>1", StrictCmp);
    TEST_EQUAL(">(b:True, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>0", StrictCmp);
    TEST_EQUAL(">(b:True, i:0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>1.0", StrictCmp);
    TEST_EQUAL(">(b:True, d:1.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>0.0", StrictCmp);
    TEST_EQUAL(">(b:True, d:0.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0>1", StrictCmp);
    TEST_EQUAL(">(d:1.0, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.1>1", StrictCmp);
    TEST_EQUAL(">(d:1.1, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[1.1]>1", StrictCmp);
    TEST_EQUAL(">(l:[d:1.1], i:1)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testGreaterWithVarsStrict()
{
    // lhs is variable
    ExpressionTree exp = parseExpressionStr("foo>1", StrictCmp);
    boost::python::dict vars;
    TEST_EQUAL(">(v:foo, i:1)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo>False", StrictCmp);
    vars.clear();
    TEST_EQUAL(">(v:foo, b:False)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.2;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo>\"teststr\"", StrictCmp);
    vars.clear();
    TEST_EQUAL(">(v:foo, s:teststr)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststs");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo>[0]", StrictCmp);
    vars.clear();
    TEST_EQUAL(">(v:foo, l:[i:0])", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // rhs is variable
    exp = parseExpressionStr("1>foo", StrictCmp);
    vars.clear();
    TEST_EQUAL(">(i:1, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.9;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("False>foo", StrictCmp);
    vars.clear();
    TEST_EQUAL(">(b:False, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -0.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("\"teststr\">foo", StrictCmp);
    vars.clear();
    TEST_EQUAL(">(s:teststr, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststq");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("[0]>foo", StrictCmp);
    vars.clear();
    TEST_EQUAL(">(l:[i:0], v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // both operands are variables
    exp = parseExpressionStr("bar>foo", StrictCmp);
    vars.clear();
    TEST_EQUAL(">(v:bar, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.5;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = false;
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -0.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = std::string("teststr");
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("testst");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr1");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = pyList({ 0 });
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = boost::python::list();
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));
}

void testGreaterEqWithConstantsStrict()
{
    ExpressionTree exp = parseExpressionStr("4>=5", StrictCmp);
    TEST_EQUAL(">=(i:4, i:5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5>=5", StrictCmp);
    TEST_EQUAL(">=(i:5, i:5)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("6>=5", StrictCmp);
    TEST_EQUAL(">=(i:6, i:5)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False>=True", StrictCmp);
    TEST_EQUAL(">=(b:False, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>=True", StrictCmp);
    TEST_EQUAL(">=(b:True, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>=False", StrictCmp);
    TEST_EQUAL(">=(b:True, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("4.0>=5.0", StrictCmp);
    TEST_EQUAL(">=(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0>=5.0", StrictCmp);
    TEST_EQUAL(">=(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("6.0>=5.0", StrictCmp);
    TEST_EQUAL(">=(d:6.0, d:5.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\">=\"teststr1\"", StrictCmp);
    TEST_EQUAL(">=(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\">=\"teststr\"", StrictCmp);
    TEST_EQUAL(">=(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr1\">=\"teststr\"", StrictCmp);
    TEST_EQUAL(">=(s:teststr1, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(4,)>=(5,)", StrictCmp);
    TEST_EQUAL(">=(l:[i:4], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(5,)>=[5, 1]", StrictCmp);
    TEST_EQUAL(">=(l:[i:5], l:[i:5, i:1])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(4,)>=[5]", StrictCmp);
    TEST_EQUAL(">=(l:[i:4], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(5.0, 6, False)>=[5]", StrictCmp);
    TEST_EQUAL(">=(l:[d:5.0, i:6, b:False], l:[i:5])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(6,)>=[5]", StrictCmp);
    TEST_EQUAL(">=(l:[i:6], l:[i:5])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[1, 6]>=[False]", StrictCmp);
    TEST_EQUAL(">=(l:[i:1, i:6], l:[b:False])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None>=None", StrictCmp);
    TEST_EQUAL(">=(n:None, n:None)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));


    exp = parseExpressionStr("True>=1", StrictCmp);
    TEST_EQUAL(">=(b:True, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False>=1", StrictCmp);
    TEST_EQUAL(">=(b:False, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>=0", StrictCmp);
    TEST_EQUAL(">=(b:True, i:0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False>=0", StrictCmp);
    TEST_EQUAL(">=(b:False, i:0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>=1.0", StrictCmp);
    TEST_EQUAL(">=(b:True, d:1.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>=0.0", StrictCmp);
    TEST_EQUAL(">=(b:True, d:0.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0>=1", StrictCmp);
    TEST_EQUAL(">=(d:1.0, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.1>=1", StrictCmp);
    TEST_EQUAL(">=(d:1.1, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[1.1]>=1", StrictCmp);
    TEST_EQUAL(">=(l:[d:1.1], i:1)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testGreaterEqWithVarsStrict()
{
    // lhs is variable
    ExpressionTree exp = parseExpressionStr("foo>=1", StrictCmp);
    boost::python::dict vars;
    TEST_EQUAL(">=(v:foo, i:1)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo>=False", StrictCmp);
    vars.clear();
    TEST_EQUAL(">=(v:foo, b:False)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.2;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo>=\"teststr\"", StrictCmp);
    vars.clear();
    TEST_EQUAL(">=(v:foo, s:teststr)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststq");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo>=[0]", StrictCmp);
    vars.clear();
    TEST_EQUAL(">=(v:foo, l:[i:0])", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = boost::python::list();
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // rhs is variable
    exp = parseExpressionStr("1>=foo", StrictCmp);
    vars.clear();
    TEST_EQUAL(">=(i:1, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 2;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.9;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("False>=foo", StrictCmp);
    vars.clear();
    TEST_EQUAL(">=(b:False, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -0.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("\"teststr\">=foo", StrictCmp);
    vars.clear();
    TEST_EQUAL(">=(s:teststr, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststs");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("[0]>=foo", StrictCmp);
    vars.clear();
    TEST_EQUAL(">=(l:[i:0], v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // both operands are variables
    exp = parseExpressionStr("bar>=foo", StrictCmp);
    vars.clear();
    TEST_EQUAL(">=(v:bar, v:foo)", exp.toString());
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.5;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = false;
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = std::string("teststr");
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr1");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr1");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = pyList({ 0 });
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = boost::python::list();
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));
}

void testIsWithConstantsStrict()
{
    ExpressionTree exp = parseExpressionStr("5 is 5", StrictCmp);
    TEST_EQUAL("is(i:5, i:5)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 is 6", StrictCmp);
    TEST_EQUAL("is(i:5, i:6)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True is True", StrictCmp);
    TEST_EQUAL("is(b:True, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True is False", StrictCmp);
    TEST_EQUAL("is(b:True, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0 is 5.0", StrictCmp);
    TEST_EQUAL("is(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0 is 6.0", StrictCmp);
    TEST_EQUAL("is(d:5.0, d:6.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\" is \"teststr\"", StrictCmp);
    TEST_EQUAL("is(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\" is \"teststr1\"", StrictCmp);
    TEST_EQUAL("is(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]is(5,)", StrictCmp);
    TEST_EQUAL("is(l:[i:5], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None is None", StrictCmp);
    TEST_EQUAL("is(n:None, n:None)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None is 1", StrictCmp);
    TEST_EQUAL("is(n:None, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1 is True", StrictCmp);
    TEST_EQUAL("is(i:1, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0 is True", StrictCmp);
    TEST_EQUAL("is(d:1.0, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1 is 1.0", StrictCmp);
    TEST_EQUAL("is(i:1, d:1.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));
}

void testIsWithVarsStrict()
{
    ExpressionTree exp = parseExpressionStr("foo is bar", StrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("is(v:foo, v:bar)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    vars["bar"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 2;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
}

void testIsWithNotStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("(not 1) is (not foo)", StrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("is(not(i:1), not(v:foo))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("test");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    // both operands are variables
    exp = parseExpressionStr("(not foo)is(not bar)", StrictCmp);
    vars.clear();
    TEST_EQUAL("is(not(v:foo), not(v:bar))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    vars["bar"] = boost::python::list();
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
}

void testIsNotWithConstantsStrict()
{
    ExpressionTree exp = parseExpressionStr("5 is not 5", StrictCmp);
    TEST_EQUAL("is_not(i:5, i:5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 is not 6", StrictCmp);
    TEST_EQUAL("is_not(i:5, i:6)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True is not True", StrictCmp);
    TEST_EQUAL("is_not(b:True, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True is not False", StrictCmp);
    TEST_EQUAL("is_not(b:True, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0 is not 5.0", StrictCmp);
    TEST_EQUAL("is_not(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0 is not 6.0", StrictCmp);
    TEST_EQUAL("is_not(d:5.0, d:6.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\" is not \"teststr\"", StrictCmp);
    TEST_EQUAL("is_not(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\" is not \"teststr1\"", StrictCmp);
    TEST_EQUAL("is_not(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]is not(5,)", StrictCmp);
    TEST_EQUAL("is_not(l:[i:5], l:[i:5])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None is not None", StrictCmp);
    TEST_EQUAL("is_not(n:None, n:None)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None is not 1", StrictCmp);
    TEST_EQUAL("is_not(n:None, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1 is not True", StrictCmp);
    TEST_EQUAL("is_not(i:1, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0 is not True", StrictCmp);
    TEST_EQUAL("is_not(d:1.0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1 is not 1.0", StrictCmp);
    TEST_EQUAL("is_not(i:1, d:1.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));
}

void testIsNotWithVarsStrict()
{
    ExpressionTree exp = parseExpressionStr("foo is not bar", StrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("is_not(v:foo, v:bar)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 2;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
}

void testIsNotWithNotStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("(not 1) is not (not foo)", StrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("is_not(not(i:1), not(v:foo))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("test");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    // both operands are variables
    exp = parseExpressionStr("(not foo)is not(not bar)", StrictCmp);
    vars.clear();
    TEST_EQUAL("is_not(not(v:foo), not(v:bar))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    vars["bar"] = boost::python::list();
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
}

void testInStrict()
{
    ExpressionTree exp = parseExpressionStr("5 in [3, 4, 5, 6]", StrictCmp);
    TEST_EQUAL("in(i:5, l:[i:3, i:4, i:5, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2 in [3, 4, 5, 6]", StrictCmp);
    TEST_EQUAL("in(i:2, l:[i:3, i:4, i:5, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5] in [3, 4, [5], 6]", StrictCmp);
    TEST_EQUAL("in(l:[i:5], l:[i:3, i:4, l:[i:5], i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5] in [3, 4, 5, 6]", StrictCmp);
    TEST_EQUAL("in(l:[i:5], l:[i:3, i:4, i:5, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 in [3, 4, 5.0, 6]", StrictCmp);
    TEST_EQUAL("in(i:5, l:[i:3, i:4, d:5.0, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True in [3, 4, 1, 6]", StrictCmp);
    TEST_EQUAL("in(b:True, l:[i:3, i:4, i:1, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"Ar\" in [3, 4, 1, 6]", StrictCmp);
    TEST_EQUAL("in(s:Ar, l:[i:3, i:4, i:1, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"Ar\" in \"Arman\"", StrictCmp);
    TEST_EQUAL("in(s:Ar, s:Arman)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"rm\" in \"Arman\"", StrictCmp);
    TEST_EQUAL("in(s:rm, s:Arman)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 in \"Arman\"", StrictCmp);
    TEST_EQUAL("in(i:5, s:Arman)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("5 in 5", StrictCmp);
    TEST_EQUAL("in(i:5, i:5)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testNotInStrict()
{
    ExpressionTree exp = parseExpressionStr("5 not in [3, 4, 5, 6]", StrictCmp);
    TEST_EQUAL("not_in(i:5, l:[i:3, i:4, i:5, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2 not in [3, 4, 5, 6]", StrictCmp);
    TEST_EQUAL("not_in(i:2, l:[i:3, i:4, i:5, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5] not in [3, 4, [5], 6]", StrictCmp);
    TEST_EQUAL("not_in(l:[i:5], l:[i:3, i:4, l:[i:5], i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[2] not in [3, 4, [5], 6]", StrictCmp);
    TEST_EQUAL("not_in(l:[i:2], l:[i:3, i:4, l:[i:5], i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5] not in [3, 4, 5, 6]", StrictCmp);
    TEST_EQUAL("not_in(l:[i:5], l:[i:3, i:4, i:5, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 not in [3, 4, 5.0, 6]", StrictCmp);
    TEST_EQUAL("not_in(i:5, l:[i:3, i:4, d:5.0, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True not in [3, 4, 1, 6]", StrictCmp);
    TEST_EQUAL("not_in(b:True, l:[i:3, i:4, i:1, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"Ar\" not in [3, 4, 1, 6]", StrictCmp);
    TEST_EQUAL("not_in(s:Ar, l:[i:3, i:4, i:1, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"Ar\" not in \"Arman\"", StrictCmp);
    TEST_EQUAL("not_in(s:Ar, s:Arman)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"rm\" not in \"Arman\"", StrictCmp);
    TEST_EQUAL("not_in(s:rm, s:Arman)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 not in \"Arman\"", StrictCmp);
    TEST_EQUAL("not_in(i:5, s:Arman)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("5 not in 5", StrictCmp);
    TEST_EQUAL("not_in(i:5, i:5)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testAndStrict()
{
    ExpressionTree exp = parseExpressionStr("5 and 4", StrictCmp);
    TEST_EQUAL("and(i:5, i:4)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 and 0", StrictCmp);
    TEST_EQUAL("and(i:5, i:0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0 and 1", StrictCmp);
    TEST_EQUAL("and(i:0, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True and False", StrictCmp);
    TEST_EQUAL("and(b:True, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False and True", StrictCmp);
    TEST_EQUAL("and(b:False, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True and True", StrictCmp);
    TEST_EQUAL("and(b:True, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0 and 5.0", StrictCmp);
    TEST_EQUAL("and(d:1.0, d:5.0)", exp.toString());
    TEST_EQUAL(5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0 and 0.0", StrictCmp);
    TEST_EQUAL("and(d:1.0, d:0.0)", exp.toString());
    TEST_EQUAL(0.0 , boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0 and False", StrictCmp);
    TEST_EQUAL("and(d:1.0, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True and \"teststr\"", StrictCmp);
    TEST_EQUAL("and(b:True, s:teststr)", exp.toString());
    TEST_EQUAL("teststr", boost::python::extract<std::string>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"\" and True", StrictCmp);
    TEST_EQUAL("and(s:, b:True)", exp.toString());
    TEST_EQUAL("", boost::python::extract<std::string>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True and [5]", StrictCmp);
    TEST_EQUAL("and(b:True, l:[i:5])", exp.toString());
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[] and True", StrictCmp);
    TEST_EQUAL("and(l:[], b:True)", exp.toString());
    TEST_EQUAL(boost::python::list(), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None and True", StrictCmp);
    TEST_EQUAL("and(n:None, b:True)", exp.toString());
    TEST_EQUAL(true, exp.eval(boost::python::dict()).is_none());

    //'(a == 10 and (10 == 3 or "1" in "1234"))', 'b != 1', { 'a': 10, 'b' : 2 }
    exp = parseExpressionStr("(a == 10 and (10 == 3 or \"1\" in \"1234\"))", StrictCmp);
    boost::python::dict vars;
    parseAndConcatExpressionStr(exp, "b != 1", StrictCmp);
    vars["a"] = 10;
    vars["b"] = 2;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
}

void testOrStrict()
{
    ExpressionTree exp = parseExpressionStr("5 or 4", StrictCmp);
    TEST_EQUAL("or(i:5, i:4)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 or 0", StrictCmp);
    TEST_EQUAL("or(i:5, i:0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0 or 1", StrictCmp);
    TEST_EQUAL("or(i:0, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0 or 0", StrictCmp);
    TEST_EQUAL("or(i:0, i:0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False or False", StrictCmp);
    TEST_EQUAL("or(b:False, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True or False", StrictCmp);
    TEST_EQUAL("or(b:True, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False or True", StrictCmp);
    TEST_EQUAL("or(b:False, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True or True", StrictCmp);
    TEST_EQUAL("or(b:True, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0 or 5.0", StrictCmp);
    TEST_EQUAL("or(d:1.0, d:5.0)", exp.toString());
    TEST_EQUAL(1.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0.0 or 0.0", StrictCmp);
    TEST_EQUAL("or(d:0.0, d:0.0)", exp.toString());
    TEST_EQUAL(0.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0.0 or False", StrictCmp);
    TEST_EQUAL("or(d:0.0, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False or \"teststr\"", StrictCmp);
    TEST_EQUAL("or(b:False, s:teststr)", exp.toString());
    TEST_EQUAL("teststr", boost::python::extract<std::string>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"\" or False", StrictCmp);
    TEST_EQUAL("or(s:, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False or [5]", StrictCmp);
    TEST_EQUAL("or(b:False, l:[i:5])", exp.toString());
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[] or False", StrictCmp);
    TEST_EQUAL("or(l:[], b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None or False", StrictCmp);
    TEST_EQUAL("or(n:None, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False or None", StrictCmp);
    TEST_EQUAL("or(b:False, n:None)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));
}

void testNumericPlusStrict()
{
    ExpressionTree exp = parseExpressionStr("5+6", StrictCmp);
    TEST_EQUAL("+(i:5, i:6)", exp.toString());
    TEST_EQUAL(11, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True+True", StrictCmp);
    TEST_EQUAL("+(b:True, b:True)", exp.toString());
    TEST_EQUAL(2, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True+False", StrictCmp);
    TEST_EQUAL("+(b:True, b:False)", exp.toString());
    TEST_EQUAL(1, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("4.0+5.0", StrictCmp);
    TEST_EQUAL("+(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(9.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5+True", StrictCmp);
    TEST_EQUAL("+(i:5, b:True)", exp.toString());
    TEST_EQUAL(6, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False+5", StrictCmp);
    TEST_EQUAL("+(b:False, i:5)", exp.toString());
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5+7.0", StrictCmp);
    TEST_EQUAL("+(i:5, d:7.0)", exp.toString());
    TEST_EQUAL(12.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("7.0+5", StrictCmp);
    TEST_EQUAL("+(d:7.0, i:5)", exp.toString());
    TEST_EQUAL(12.0, boost::python::extract<double>(exp.eval(boost::python::dict())));
}

void testStringPlusStrict()
{
    ExpressionTree exp = parseExpressionStr("\"Te\"+\"st\"", StrictCmp);
    TEST_EQUAL("+(s:Te, s:st)", exp.toString());
    TEST_EQUAL("Test", boost::python::extract<std::string>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"Te\"+\"\"", StrictCmp);
    TEST_EQUAL("+(s:Te, s:)", exp.toString());
    TEST_EQUAL("Te", boost::python::extract<std::string>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"\"+\"st\"", StrictCmp);
    TEST_EQUAL("+(s:, s:st)", exp.toString());
    TEST_EQUAL("st", boost::python::extract<std::string>(exp.eval(boost::python::dict())));
}

void testListPlusStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] + [3, 4]", StrictCmp);
    TEST_EQUAL("+(l:[i:1, i:2], l:[i:3, i:4])", exp.toString());
    TEST_EQUAL(pyList({1, 2, 3, 4}), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[1, 2] + []", StrictCmp);
    TEST_EQUAL("+(l:[i:1, i:2], l:[])", exp.toString());
    TEST_EQUAL(pyList({ 1, 2}), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[] + [3, 4]", StrictCmp);
    TEST_EQUAL("+(l:[], l:[i:3, i:4])", exp.toString());
    TEST_EQUAL(pyList({3, 4 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[] + []", StrictCmp);
    TEST_EQUAL("+(l:[], l:[])", exp.toString());
    TEST_EQUAL(boost::python::list(), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
}

void testPlusErrorsStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] + \"test\"", StrictCmp);
    TEST_EQUAL("+(l:[i:1, i:2], s:test)", exp.toString());
    TEST_THROW(boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1 + \"test\"", StrictCmp);
    TEST_EQUAL("+(i:1, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1.0 + \"test\"", StrictCmp);
    TEST_EQUAL("+(d:1.0, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True + \"test\"", StrictCmp);
    TEST_EQUAL("+(b:True, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testListPlusVarsStrict()
{
    ExpressionTree exp = parseExpressionStr("foo + bar", StrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("+(v:foo, v:bar)", exp.toString());
    TEST_THROW(exp.eval(vars));

    vars["foo"] = 5;
    TEST_THROW(exp.eval(vars));

    vars["bar"] = 6;
    TEST_EQUAL(11, boost::python::extract<int>(exp.eval(vars)));

    vars["foo"] = pyList({1, 2});
    vars["bar"] = pyList({3, 4});
    TEST_EQUAL(pyList({1, 2, 3, 4}), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("foo + foo", StrictCmp);
    vars.clear();
    vars["foo"] = pyList({ 1, 2 });
    TEST_EQUAL(pyList({ 1, 2, 1, 2 }), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("foo + bar", StrictCmp);
    vars.clear();
    TEST_EQUAL("+(v:foo, v:bar)", exp.toString());
    vars["foo"] = std::string("te");
    vars["bar"] = std::string("st");
    TEST_EQUAL(std::string("test"), boost::python::extract<std::string>(exp.eval(vars)));

    exp = parseExpressionStr("[1, 2, foo] + [foo, 3]", StrictCmp);
    vars.clear();
    TEST_EQUAL("+(l:[i:1, i:2, v:foo], l:[v:foo, i:3])", exp.toString());
    vars["foo"] = std::string("te");
    TEST_EQUAL(pyList(1, 2, std::string("te"), std::string("te"), 3), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("[1, 2, foo] + [bar, 3]", StrictCmp);
    vars.clear();
    TEST_EQUAL("+(l:[i:1, i:2, v:foo], l:[v:bar, i:3])", exp.toString());
    vars["foo"] = std::string("te");
    vars["bar"] = std::string("st");
    TEST_EQUAL(pyList(1, 2, std::string("te"), std::string("st"), 3), boost::python::extract<boost::python::list>(exp.eval(vars)));

    TEST_EQUAL("+(l:[i:1, i:2, v:foo], l:[v:bar, i:3])", exp.toString());
    vars["foo"] = pyList({ 3, 4 });
    vars["bar"] = pyList({ 5, 6 });
    boost::python::list res;
    res.append(1);
    res.append(2);
    res.append(pyList({ 3, 4 }));
    res.append(pyList({ 5, 6 }));
    res.append(3);
    TEST_EQUAL(res, boost::python::extract<boost::python::list>(exp.eval(vars)));
}

void testNumericMinusStrict()
{
    ExpressionTree exp = parseExpressionStr("5-6", StrictCmp);
    TEST_EQUAL("-(i:5, i:6)", exp.toString());
    TEST_EQUAL(-1, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True-True", StrictCmp);
    TEST_EQUAL("-(b:True, b:True)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True-False", StrictCmp);
    TEST_EQUAL("-(b:True, b:False)", exp.toString());
    TEST_EQUAL(1, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("4.0-5.0", StrictCmp);
    TEST_EQUAL("-(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(-1.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5-True", StrictCmp);
    TEST_EQUAL("-(i:5, b:True)", exp.toString());
    TEST_EQUAL(4, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False-5", StrictCmp);
    TEST_EQUAL("-(b:False, i:5)", exp.toString());
    TEST_EQUAL(-4, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5-7.0", StrictCmp);
    TEST_EQUAL("-(i:5, d:7.0)", exp.toString());
    TEST_EQUAL(-2.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("7.0-5", StrictCmp);
    TEST_EQUAL("-(d:7.0, i:5)", exp.toString());
    TEST_EQUAL(2.0, boost::python::extract<double>(exp.eval(boost::python::dict())));
}

void testMinusErrorsStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] - \"test\"", StrictCmp);
    TEST_EQUAL("-(l:[i:1, i:2], s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1 - \"test\"", StrictCmp);
    TEST_EQUAL("-(i:1, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1.0 - \"test\"", StrictCmp);
    TEST_EQUAL("-(d:1.0, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True - \"test\"", StrictCmp);
    TEST_EQUAL("-(b:True, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("\"test\" - \"test\"", StrictCmp);
    TEST_EQUAL("-(s:test, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("[1, 2] - [1, 2]", StrictCmp);
    TEST_EQUAL("-(l:[i:1, i:2], l:[i:1, i:2])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testNumericMulStrict()
{
    ExpressionTree exp = parseExpressionStr("5*6", StrictCmp);
    TEST_EQUAL("*(i:5, i:6)", exp.toString());
    TEST_EQUAL(30, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True*True", StrictCmp);
    TEST_EQUAL("*(b:True, b:True)", exp.toString());
    TEST_EQUAL(1, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True*False", StrictCmp);
    TEST_EQUAL("*(b:True, b:False)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("4.0*5.0", StrictCmp);
    TEST_EQUAL("*(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(20.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5*True", StrictCmp);
    TEST_EQUAL("*(i:5, b:True)", exp.toString());
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5*[3]", StrictCmp);
    TEST_EQUAL("*(i:5, l:[i:3])", exp.toString());
    TEST_EQUAL(pyList({ 3, 3, 3, 3, 3 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False*5", StrictCmp);
    TEST_EQUAL("*(b:False, i:5)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5*7.0", StrictCmp);
    TEST_EQUAL("*(i:5, d:7.0)", exp.toString());
    TEST_EQUAL(35, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("7.0*5", StrictCmp);
    TEST_EQUAL("*(d:7.0, i:5)", exp.toString());
    TEST_EQUAL(35, boost::python::extract<double>(exp.eval(boost::python::dict())));
}

void testMulErrorsStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] * \"test\"", StrictCmp);
    TEST_EQUAL("*(l:[i:1, i:2], s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1.0 * \"test\"", StrictCmp);
    TEST_EQUAL("*(d:1.0, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("\"test\" * \"test\"", StrictCmp);
    TEST_EQUAL("*(s:test, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("[1, 2] * [1, 2]", StrictCmp);
    TEST_EQUAL("*(l:[i:1, i:2], l:[i:1, i:2])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testNumericDivStrict()
{
    ExpressionTree exp = parseExpressionStr("5/6", StrictCmp);
    TEST_EQUAL("/(i:5, i:6)", exp.toString());
    TEST_EQUAL(double(5)/double(6), boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5/0", StrictCmp);
    TEST_EQUAL("/(i:5, i:0)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True/True", StrictCmp);
    TEST_EQUAL("/(b:True, b:True)", exp.toString());
    TEST_EQUAL(1.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True/False", StrictCmp);
    TEST_EQUAL("/(b:True, b:False)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("4.0/5.0", StrictCmp);
    TEST_EQUAL("/(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(4.0/5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5/True", StrictCmp);
    TEST_EQUAL("/(i:5, b:True)", exp.toString());
    TEST_EQUAL(5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False/5", StrictCmp);
    TEST_EQUAL("/(b:False, i:5)", exp.toString());
    TEST_EQUAL(0.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5/7.0", StrictCmp);
    TEST_EQUAL("/(i:5, d:7.0)", exp.toString());
    TEST_EQUAL(5 / 7.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("7.0/5", StrictCmp);
    TEST_EQUAL("/(d:7.0, i:5)", exp.toString());
    TEST_EQUAL(7.0 / 5, boost::python::extract<double>(exp.eval(boost::python::dict())));
}

void testDivErrorsStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] / \"test\"", StrictCmp);
    TEST_EQUAL("/(l:[i:1, i:2], s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1 / \"test\"", StrictCmp);
    TEST_EQUAL("/(i:1, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1.0 / \"test\"", StrictCmp);
    TEST_EQUAL("/(d:1.0, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True / \"test\"", StrictCmp);
    TEST_EQUAL("/(b:True, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("\"test\" / \"test\"", StrictCmp);
    TEST_EQUAL("/(s:test, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("[1, 2] / [1, 2]", StrictCmp);
    TEST_EQUAL("/(l:[i:1, i:2], l:[i:1, i:2])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testNumericFloorStrict()
{
    ExpressionTree exp = parseExpressionStr("5//6", StrictCmp);
    TEST_EQUAL("//(i:5, i:6)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5//0", StrictCmp);
    TEST_EQUAL("//(i:5, i:0)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True//True", StrictCmp);
    TEST_EQUAL("//(b:True, b:True)", exp.toString());
    TEST_EQUAL(1, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True//False", StrictCmp);
    TEST_EQUAL("//(b:True, b:False)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("4.0//5.0", StrictCmp);
    TEST_EQUAL("//(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(0.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5//True", StrictCmp);
    TEST_EQUAL("//(i:5, b:True)", exp.toString());
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False//5", StrictCmp);
    TEST_EQUAL("//(b:False, i:5)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5//7.0", StrictCmp);
    TEST_EQUAL("//(i:5, d:7.0)", exp.toString());
    TEST_EQUAL(0.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("7.0//5", StrictCmp);
    TEST_EQUAL("//(d:7.0, i:5)", exp.toString());
    TEST_EQUAL(1.0, boost::python::extract<double>(exp.eval(boost::python::dict())));
}

void testFloorErrorsStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] // \"test\"", StrictCmp);
    TEST_EQUAL("//(l:[i:1, i:2], s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1 // \"test\"", StrictCmp);
    TEST_EQUAL("//(i:1, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1.0 // \"test\"", StrictCmp);
    TEST_EQUAL("//(d:1.0, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True // \"test\"", StrictCmp);
    TEST_EQUAL("//(b:True, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("\"test\" // \"test\"", StrictCmp);
    TEST_EQUAL("//(s:test, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("[1, 2] // [1, 2]", StrictCmp);
    TEST_EQUAL("//(l:[i:1, i:2], l:[i:1, i:2])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testNumericRmStrict()
{
    ExpressionTree exp = parseExpressionStr("5%6", StrictCmp);
    TEST_EQUAL("%(i:5, i:6)", exp.toString());
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5%0", StrictCmp);
    TEST_EQUAL("%(i:5, i:0)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True%True", StrictCmp);
    TEST_EQUAL("%(b:True, b:True)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True%False", StrictCmp);
    TEST_EQUAL("%(b:True, b:False)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("4.0%5.0", StrictCmp);
    TEST_EQUAL("%(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(4.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5%True", StrictCmp);
    TEST_EQUAL("%(i:5, b:True)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False%5", StrictCmp);
    TEST_EQUAL("%(b:False, i:5)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5%7.0", StrictCmp);
    TEST_EQUAL("%(i:5, d:7.0)", exp.toString());
    TEST_EQUAL(5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("7.0%5", StrictCmp);
    TEST_EQUAL("%(d:7.0, i:5)", exp.toString());
    TEST_EQUAL(2.0, boost::python::extract<double>(exp.eval(boost::python::dict())));
}

void testRmErrorsStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] % \"test\"", StrictCmp);
    TEST_EQUAL("%(l:[i:1, i:2], s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1 % \"test\"", StrictCmp);
    TEST_EQUAL("%(i:1, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1.0 % \"test\"", StrictCmp);
    TEST_EQUAL("%(d:1.0, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True % \"test\"", StrictCmp);
    TEST_EQUAL("%(b:True, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("\"test\" % \"test\"", StrictCmp);
    TEST_EQUAL("%(s:test, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("[1, 2] % [1, 2]", StrictCmp);
    TEST_EQUAL("%(l:[i:1, i:2], l:[i:1, i:2])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testConstantsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5", NonStrictCmp);
    TEST_EQUAL("i:5", exp.toString());
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("-5", NonStrictCmp);
    TEST_EQUAL("i:-5", exp.toString());
    TEST_EQUAL(-5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True", NonStrictCmp);
    TEST_EQUAL("b:True", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0", NonStrictCmp);
    TEST_EQUAL("d:5.0", exp.toString());
    TEST_EQUAL(5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("-5.0", NonStrictCmp);
    TEST_EQUAL("d:-5.0", exp.toString());
    TEST_EQUAL(-5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"", NonStrictCmp);
    TEST_EQUAL("s:teststr", exp.toString());
    TEST_EQUAL(std::string("teststr"), boost::python::extract<std::string>(exp.eval(boost::python::dict())));
}

void testNoneNonStrict()
{
    ExpressionTree exp = parseExpressionStr("None", NonStrictCmp);
    TEST_EQUAL("n:None", exp.toString());
    TEST_EQUAL(true, exp.eval(boost::python::dict()).is_none());
}

void testVarsAndPathsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("foo", NonStrictCmp);
    TEST_EQUAL("v:foo", exp.toString());
    TEST_EQUAL(true, exp.eval(boost::python::dict()).is_none());

    boost::python::dict vars;
    vars["foo"] = 5;
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(vars)));

    exp = parseExpressionStr("foo.bar", NonStrictCmp);
    TEST_EQUAL("p:foo.bar", exp.toString());
    TEST_EQUAL(true, exp.eval(boost::python::dict()).is_none());

    boost::python::dict foo;
    foo["bar"] = 500;
    vars["foo"] = foo;
    TEST_EQUAL(500, boost::python::extract<int>(exp.eval(vars)));

    foo["bar"] = std::string("tests");
    vars["foo"] = foo;
    TEST_EQUAL(std::string("tests"), boost::python::extract<std::string>(exp.eval(vars)));

    vars["foo"] = boost::python::object();
    TEST_EQUAL(true, exp.eval(vars).is_none());
}

void testList1NonStrict()
{
    ExpressionTree exp = parseExpressionStr("[5]", NonStrictCmp);
    TEST_EQUAL("l:[i:5]", exp.toString());
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5,]", NonStrictCmp);
    TEST_EQUAL("l:[i:5]", exp.toString());
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[-5]", NonStrictCmp);
    TEST_EQUAL("l:[i:-5]", exp.toString());
    TEST_EQUAL(pyList({ -5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[-5,]", NonStrictCmp);
    TEST_EQUAL("l:[i:-5]", exp.toString());
    TEST_EQUAL(pyList({ -5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[False]", NonStrictCmp);
    TEST_EQUAL("l:[b:False]", exp.toString());
    TEST_EQUAL(pyList({ false }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[False,]", NonStrictCmp);
    TEST_EQUAL("l:[b:False]", exp.toString());
    TEST_EQUAL(pyList({ false }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5.0]", NonStrictCmp);
    TEST_EQUAL("l:[d:5.0]", exp.toString());
    TEST_EQUAL(pyList({ 5.0 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5.0,]", NonStrictCmp);
    TEST_EQUAL("l:[d:5.0]", exp.toString());
    TEST_EQUAL(pyList({ 5.0 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[-5.0]", NonStrictCmp);
    TEST_EQUAL("l:[d:-5.0]", exp.toString());
    TEST_EQUAL(pyList({ -5.0 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[-5.0,]", NonStrictCmp);
    TEST_EQUAL("l:[d:-5.0]", exp.toString());
    TEST_EQUAL(pyList({ -5.0 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[\"teststr\"]", NonStrictCmp);
    TEST_EQUAL("l:[s:teststr]", exp.toString());
    TEST_EQUAL(pyList({ std::string("teststr") }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[\"teststr\",]", NonStrictCmp);
    TEST_EQUAL("l:[s:teststr]", exp.toString());
    TEST_EQUAL(pyList({ std::string("teststr") }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[None]", NonStrictCmp);
    TEST_EQUAL("l:[n:None]", exp.toString());
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[None,]", NonStrictCmp);
    TEST_EQUAL("l:[n:None]", exp.toString());
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
}

void testList1TupleNonStrict()
{
    ExpressionTree exp = parseExpressionStr("(5,)", NonStrictCmp);
    TEST_EQUAL("l:[i:5]", exp.toString());
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(-5,)", NonStrictCmp);
    TEST_EQUAL("l:[i:-5]", exp.toString());
    TEST_EQUAL(pyList({ -5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(False,)", NonStrictCmp);
    TEST_EQUAL("l:[b:False]", exp.toString());
    TEST_EQUAL(pyList({ false }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(5.0,)", NonStrictCmp);
    TEST_EQUAL("l:[d:5.0]", exp.toString());
    TEST_EQUAL(pyList({ 5.0 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(-5.0,)", NonStrictCmp);
    TEST_EQUAL("l:[d:-5.0]", exp.toString());
    TEST_EQUAL(pyList({ -5.0 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(\"teststr\",)", NonStrictCmp);
    TEST_EQUAL("l:[s:teststr]", exp.toString());
    TEST_EQUAL(pyList({ std::string("teststr") }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(None,)", NonStrictCmp);
    TEST_EQUAL("l:[n:None]", exp.toString());
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
}

void testList1VarNonStrict()
{
    ExpressionTree exp = parseExpressionStr("[foo]", NonStrictCmp);
    TEST_EQUAL("l:[v:foo]", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars["foo"] = 5;
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("[foo,]", NonStrictCmp);
    TEST_EQUAL("l:[v:foo]", exp.toString());
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("[foo.bar]", NonStrictCmp);
    TEST_EQUAL("l:[p:foo.bar]", exp.toString());
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
    TEST_EQUAL(pyList({ 7 }), boost::python::extract<boost::python::list>(exp.eval(makePath({ "foo", "bar" }, 7))));

    exp = parseExpressionStr("[foo.bar,]", NonStrictCmp);
    TEST_EQUAL("l:[p:foo.bar]", exp.toString());
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
    TEST_EQUAL(pyList({ 7 }), boost::python::extract<boost::python::list>(exp.eval(makePath({ "foo", "bar" }, 7))));
}

void testList1VarTupleNonStrict()
{
    ExpressionTree exp = parseExpressionStr("(foo,)", NonStrictCmp);
    TEST_EQUAL("l:[v:foo]", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars["foo"] = 5;
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("(foo.bar,)", NonStrictCmp);
    TEST_EQUAL("l:[p:foo.bar]", exp.toString());
    TEST_EQUAL(pyList({ boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
    TEST_EQUAL(pyList({ 7 }), boost::python::extract<boost::python::list>(exp.eval(makePath({ "foo", "bar" }, 7))));
}

void testList2NonStrict()
{
    ExpressionTree exp = parseExpressionStr("[5, True]", NonStrictCmp);
    TEST_EQUAL("l:[i:5, b:True]", exp.toString());
    TEST_EQUAL(pyList(5, true), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[-5, None]", NonStrictCmp);
    TEST_EQUAL("l:[i:-5, n:None]", exp.toString());
    TEST_EQUAL(pyList(-5, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[False, 5]", NonStrictCmp);
    TEST_EQUAL("l:[b:False, i:5]", exp.toString());
    TEST_EQUAL(pyList(false, 5), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5.0, 7]", NonStrictCmp);
    TEST_EQUAL("l:[d:5.0, i:7]", exp.toString());
    TEST_EQUAL(pyList(5.0, 7), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[-5.0, None]", NonStrictCmp);
    TEST_EQUAL("l:[d:-5.0, n:None]", exp.toString());
    TEST_EQUAL(pyList(-5.0, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[\"teststr\", 5]", NonStrictCmp);
    TEST_EQUAL("l:[s:teststr, i:5]", exp.toString());
    TEST_EQUAL(pyList(std::string("teststr"), 5), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[None, False]", NonStrictCmp);
    TEST_EQUAL("l:[n:None, b:False]", exp.toString());
    TEST_EQUAL(pyList(boost::python::object(), false), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
}

void testList2TupleNonStrict()
{
    ExpressionTree exp = parseExpressionStr("(5, True)", NonStrictCmp);
    TEST_EQUAL("l:[i:5, b:True]", exp.toString());
    TEST_EQUAL(pyList(5, true), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(-5, None)", NonStrictCmp);
    TEST_EQUAL("l:[i:-5, n:None]", exp.toString());
    TEST_EQUAL(pyList(-5, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(False, 5)", NonStrictCmp);
    TEST_EQUAL("l:[b:False, i:5]", exp.toString());
    TEST_EQUAL(pyList(false, 5), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(5.0, 7)", NonStrictCmp);
    TEST_EQUAL("l:[d:5.0, i:7]", exp.toString());
    TEST_EQUAL(pyList(5.0, 7), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(-5.0, None)", NonStrictCmp);
    TEST_EQUAL("l:[d:-5.0, n:None]", exp.toString());
    TEST_EQUAL(pyList(-5.0, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(\"teststr\", 5)", NonStrictCmp);
    TEST_EQUAL("l:[s:teststr, i:5]", exp.toString());
    TEST_EQUAL(pyList(std::string("teststr"), 5), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(None, False)", NonStrictCmp);
    TEST_EQUAL("l:[n:None, b:False]", exp.toString());
    TEST_EQUAL(pyList(boost::python::object(), false), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
}

void testList2VarNonStrict()
{
    ExpressionTree exp = parseExpressionStr("[foo, bar]", NonStrictCmp);
    TEST_EQUAL("l:[v:foo, v:bar]", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(pyList({ boost::python::object(), boost::python::object() }), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars["foo"] = 5;
    TEST_EQUAL(pyList(5, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars["bar"] = true;
    TEST_EQUAL(pyList(5, true), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("[foo.r, bar.r]", NonStrictCmp);
    TEST_EQUAL("l:[p:foo.r, p:bar.r]", exp.toString());
    TEST_EQUAL(pyList(boost::python::object(), boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars = makePath({ "foo", "r" }, 5);
    TEST_EQUAL(pyList(5, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(vars)));
    addPath(vars, { "bar", "r" }, true);
    TEST_EQUAL(pyList(5, true), boost::python::extract<boost::python::list>(exp.eval(vars)));
}

void testList2VarTupleNonStrict()
{
    ExpressionTree exp = parseExpressionStr("(foo, bar)", NonStrictCmp);
    TEST_EQUAL("l:[v:foo, v:bar]", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(pyList(boost::python::object(), boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars["foo"] = 5;
    TEST_EQUAL(pyList(5, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(vars)));
    vars["bar"] = true;
    TEST_EQUAL(pyList(5, true), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("(foo.r, bar.r)", NonStrictCmp);
    TEST_EQUAL("l:[p:foo.r, p:bar.r]", exp.toString());
    TEST_EQUAL(pyList(boost::python::object(), boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
    vars = makePath({ "foo", "r" }, 5);
    TEST_EQUAL(pyList(5, boost::python::object()), boost::python::extract<boost::python::list>(exp.eval(vars)));
    addPath(vars, { "bar", "r" }, true);
    TEST_EQUAL(pyList(5, true), boost::python::extract<boost::python::list>(exp.eval(vars)));
}

void testNotWithConstantsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("not 5", NonStrictCmp);
    TEST_EQUAL("not(i:5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not -5", NonStrictCmp);
    TEST_EQUAL("not(i:-5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not True", NonStrictCmp);
    TEST_EQUAL("not(b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not False", NonStrictCmp);
    TEST_EQUAL("not(b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not 5.0", NonStrictCmp);
    TEST_EQUAL("not(d:5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not -5.0", NonStrictCmp);
    TEST_EQUAL("not(d:-5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not \"teststr\"", NonStrictCmp);
    TEST_EQUAL("not(s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not [5]", NonStrictCmp);
    TEST_EQUAL("not(l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not (5,)", NonStrictCmp);
    TEST_EQUAL("not(l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("not None", NonStrictCmp);
    TEST_EQUAL("not(n:None)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));
}

void testNotWithVarsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("not foo", NonStrictCmp);
    TEST_EQUAL("not(v:foo)", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    vars["foo"] = 5;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["foo"] = 5.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    vars["foo"] = "teststr";
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
}

void testNegateWithConstantsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("--5", NonStrictCmp);
    TEST_EQUAL("-(i:-5)", exp.toString());
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("-True", NonStrictCmp);
    TEST_EQUAL("-(b:True)", exp.toString());
    TEST_EQUAL(-1, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("-False", NonStrictCmp);
    TEST_EQUAL("-(b:False)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("--5.0", NonStrictCmp);
    TEST_EQUAL("-(d:-5.0)", exp.toString());
    TEST_EQUAL(5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("-\"teststr\"", NonStrictCmp);
    TEST_EQUAL("-(s:teststr)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("-[5]", NonStrictCmp);
    TEST_EQUAL("-(l:[i:5])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("-(5,)", NonStrictCmp);
    TEST_EQUAL("-(l:[i:5])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("-None", NonStrictCmp);
    TEST_EQUAL("-(n:None)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testNegateWithVarsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("-foo", NonStrictCmp);
    TEST_EQUAL("-(v:foo)", exp.toString());
    boost::python::dict vars;
    TEST_THROW(exp.eval(vars));

    vars["foo"] = 5;
    TEST_EQUAL(-5, boost::python::extract<int>(exp.eval(vars)));

    vars["foo"] = 5.0;
    TEST_EQUAL(-5.0, boost::python::extract<double>(exp.eval(vars)));

    vars["foo"] = true;
    TEST_EQUAL(-1, boost::python::extract<int>(exp.eval(vars)));

    vars["foo"] = false;
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(vars)));

    vars["foo"] = "teststr";
    TEST_THROW(exp.eval(vars));

    vars["foo"] = pyList({ 5 });
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("-foo.bar", NonStrictCmp);
    TEST_EQUAL("-(p:foo.bar)", exp.toString());
    TEST_THROW(exp.eval(vars));

    vars = makePath({ "foo", "bar" }, 5);
    TEST_EQUAL(-5, boost::python::extract<int>(exp.eval(vars)));

    vars = makePath({ "foo", "bar" }, 5.0);
    TEST_EQUAL(-5.0, boost::python::extract<double>(exp.eval(vars)));

    vars = makePath({ "foo", "bar" }, true);
    TEST_EQUAL(-1, boost::python::extract<int>(exp.eval(vars)));

    vars = makePath({ "foo", "bar" }, false);
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(vars)));

    vars = makePath({ "foo", "bar" }, "teststr");
    TEST_THROW(exp.eval(vars));

    vars = makePath({ "foo", "bar" }, pyList({ 5 }));
    TEST_THROW(exp.eval(vars));
}

void testEqualWithConstantsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5==5", NonStrictCmp);
    TEST_EQUAL("==(i:5, i:5)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5==6", NonStrictCmp);
    TEST_EQUAL("==(i:5, i:6)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True==True", NonStrictCmp);
    TEST_EQUAL("==(b:True, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True==(not False)", NonStrictCmp);
    TEST_EQUAL("==(b:True, not(b:False))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True==False", NonStrictCmp);
    TEST_EQUAL("==(b:True, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0==5.0", NonStrictCmp);
    TEST_EQUAL("==(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0==6.0", NonStrictCmp);
    TEST_EQUAL("==(d:5.0, d:6.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"==\"teststr\"", NonStrictCmp);
    TEST_EQUAL("==(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"==\"teststr1\"", NonStrictCmp);
    TEST_EQUAL("==(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]==(5,)", NonStrictCmp);
    TEST_EQUAL("==(l:[i:5], l:[i:5])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]==(5.0,)", NonStrictCmp);
    TEST_EQUAL("==(l:[i:5], l:[d:5.0])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]==(6,)", NonStrictCmp);
    TEST_EQUAL("==(l:[i:5], l:[i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]==[5, 6]", NonStrictCmp);
    TEST_EQUAL("==(l:[i:5], l:[i:5, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None==None", NonStrictCmp);
    TEST_EQUAL("==(n:None, n:None)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));


    exp = parseExpressionStr("1==True", NonStrictCmp);
    TEST_EQUAL("==(i:1, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2==True", NonStrictCmp);
    TEST_EQUAL("==(i:2, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0==True", NonStrictCmp);
    TEST_EQUAL("==(d:1.0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2.0==True", NonStrictCmp);
    TEST_EQUAL("==(d:2.0, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1==1.0", NonStrictCmp);
    TEST_EQUAL("==(i:1, d:1.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2==1.0", NonStrictCmp);
    TEST_EQUAL("==(i:2, d:1.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));
}

void testEqualWithVarsNonStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("1==foo", NonStrictCmp);
    TEST_EQUAL("==(i:1, v:foo)", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("False==foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(b:False, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("\"teststr\"==foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(s:teststr, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("[0]==foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(l:[i:0], v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    // lhs is variable
    exp = parseExpressionStr("foo==1", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(v:foo, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo==False", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(v:foo, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo==\"teststr\"", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(v:foo, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo==[0]", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(v:foo, l:[i:0])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    // both operands are variables
    exp = parseExpressionStr("foo==bar", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(v:foo, v:bar)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = false;
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = std::string("teststr");
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = pyList({ 0 });
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
}

void testEqualWithNotNonStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("(not 1)==(not foo)", NonStrictCmp);
    TEST_EQUAL("==(not(i:1), not(v:foo))", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not False)==(not foo)", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(not(b:False), not(v:foo))", exp.toString());
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not [0])== (not foo)", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(not(l:[i:0]), not(v:foo))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    // lhs is variable
    exp = parseExpressionStr("(not foo)==(not 1)", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(not(v:foo), not(i:1))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not foo)==(not \"teststr\")", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(not(v:foo), not(s:teststr))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not foo)==(not [0])", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(not(v:foo), not(l:[i:0]))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    // both operands are variables
    exp = parseExpressionStr("(not foo)==(not bar)", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("==(not(v:foo), not(v:bar))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    vars["bar"] = pyList({ 6 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = boost::python::list();
    vars["bar"] = pyList({ 6 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("str1");
    vars["bar"] = std::string("str2");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("");
    vars["bar"] = std::string("str2");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
}

void testNotEqualWithConstantsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5!=5", NonStrictCmp);
    TEST_EQUAL("!=(i:5, i:5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5!=6", NonStrictCmp);
    TEST_EQUAL("!=(i:5, i:6)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True!=True", NonStrictCmp);
    TEST_EQUAL("!=(b:True, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True!=False", NonStrictCmp);
    TEST_EQUAL("!=(b:True, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0!=5.0", NonStrictCmp);
    TEST_EQUAL("!=(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0!=6.0", NonStrictCmp);
    TEST_EQUAL("!=(d:5.0, d:6.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"!=\"teststr\"", NonStrictCmp);
    TEST_EQUAL("!=(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"!=\"teststr1\"", NonStrictCmp);
    TEST_EQUAL("!=(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]!=(5,)", NonStrictCmp);
    TEST_EQUAL("!=(l:[i:5], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]!=(6,)", NonStrictCmp);
    TEST_EQUAL("!=(l:[i:5], l:[i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]!=[5, 6]", NonStrictCmp);
    TEST_EQUAL("!=(l:[i:5], l:[i:5, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None!=None", NonStrictCmp);
    TEST_EQUAL("!=(n:None, n:None)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));


    exp = parseExpressionStr("1!=True", NonStrictCmp);
    TEST_EQUAL("!=(i:1, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2!=True", NonStrictCmp);
    TEST_EQUAL("!=(i:2, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0!=True", NonStrictCmp);
    TEST_EQUAL("!=(d:1.0, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2.0!=True", NonStrictCmp);
    TEST_EQUAL("!=(d:2.0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1!=1.0", NonStrictCmp);
    TEST_EQUAL("!=(i:1, d:1.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2!=1.0", NonStrictCmp);
    TEST_EQUAL("!=(i:2, d:1.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));
}

void testNotEqualWithVarsNonStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("1!=foo", NonStrictCmp);
    TEST_EQUAL("!=(i:1, v:foo)", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("False!=foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(b:False, v:foo)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("\"teststr\"!=foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(s:teststr, v:foo)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("[0]!=foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(l:[i:0], v:foo)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    // lhs is variable
    exp = parseExpressionStr("foo!=1", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(v:foo, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo!=False", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(v:foo, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo!=\"teststr\"", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(v:foo, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo!=[0]", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(v:foo, l:[i:0])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    // both operands are variables
    exp = parseExpressionStr("foo!=bar", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(v:foo, v:bar)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = false;
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = std::string("teststr");
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = pyList({ 0 });
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
}

void testNotEqualWithNotNonStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("(not 1)!=(not foo)", NonStrictCmp);
    TEST_EQUAL("!=(not(i:1), not(v:foo))", exp.toString());
    boost::python::dict vars;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not False)!=(not foo)", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(not(b:False), not(v:foo))", exp.toString());
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not [0])!= (not foo)", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(not(l:[i:0]), not(v:foo))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    // lhs is variable
    exp = parseExpressionStr("(not foo)!=(not 1)", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(not(v:foo), not(i:1))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not foo)!=(not \"teststr\")", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(not(v:foo), not(s:teststr))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("(not foo)!=(not [0])", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(not(v:foo), not(l:[i:0]))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    // both operands are variables
    exp = parseExpressionStr("(not foo)!=(not bar)", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("!=(not(v:foo), not(v:bar))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    vars["bar"] = pyList({ 6 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = boost::python::list();
    vars["bar"] = pyList({ 6 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("str1");
    vars["bar"] = std::string("str2");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("");
    vars["bar"] = std::string("str2");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
}

void testLessWithConstantsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5<5", NonStrictCmp);
    TEST_EQUAL("<(i:5, i:5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5<6", NonStrictCmp);
    TEST_EQUAL("<(i:5, i:6)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True<True", NonStrictCmp);
    TEST_EQUAL("<(b:True, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False<True", NonStrictCmp);
    TEST_EQUAL("<(b:False, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0<5.0", NonStrictCmp);
    TEST_EQUAL("<(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0<6.0", NonStrictCmp);
    TEST_EQUAL("<(d:5.0, d:6.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"<\"teststr\"", NonStrictCmp);
    TEST_EQUAL("<(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"<\"teststr1\"", NonStrictCmp);
    TEST_EQUAL("<(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<(5,)", NonStrictCmp);
    TEST_EQUAL("<(l:[i:5], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<(5.0, 6, False)", NonStrictCmp);
    TEST_EQUAL("<(l:[i:5], l:[d:5.0, i:6, b:False])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<(6,)", NonStrictCmp);
    TEST_EQUAL("<(l:[i:5], l:[i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[False]<[1, 6]", NonStrictCmp);
    TEST_EQUAL("<(l:[b:False], l:[i:1, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None<None", NonStrictCmp);
    TEST_EQUAL("<(n:None, n:None)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<True", NonStrictCmp);
    TEST_EQUAL("<(i:1, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0<True", NonStrictCmp);
    TEST_EQUAL("<(i:0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0<True", NonStrictCmp);
    TEST_EQUAL("<(d:1.0, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0.0<True", NonStrictCmp);
    TEST_EQUAL("<(d:0.0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<1.0", NonStrictCmp);
    TEST_EQUAL("<(i:1, d:1.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<1.1", NonStrictCmp);
    TEST_EQUAL("<(i:1, d:1.1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<[1.1]", NonStrictCmp);
    TEST_EQUAL("<(i:1, l:[d:1.1])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testLessWithVarsNonStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("1<foo", NonStrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("<(i:1, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("False<foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<(b:False, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.2;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("\"teststr\"<foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<(s:teststr, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststs");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("[0]<foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<(l:[i:0], v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // lhs is variable
    exp = parseExpressionStr("foo<1", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<(v:foo, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.9;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo<False", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<(v:foo, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -0.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo<\"teststr\"", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<(v:foo, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststq");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo<[0]", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<(v:foo, l:[i:0])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // both operands are variables
    exp = parseExpressionStr("foo<bar", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<(v:foo, v:bar)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.5;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = false;
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -0.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = std::string("teststr");
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("testst");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr1");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = pyList({ 0 });
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = boost::python::list();
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));
}

void testLessEqWithConstantsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5<=4", NonStrictCmp);
    TEST_EQUAL("<=(i:5, i:4)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5<=5", NonStrictCmp);
    TEST_EQUAL("<=(i:5, i:5)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5<=6", NonStrictCmp);
    TEST_EQUAL("<=(i:5, i:6)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True<=False", NonStrictCmp);
    TEST_EQUAL("<=(b:True, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True<=True", NonStrictCmp);
    TEST_EQUAL("<=(b:True, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False<=True", NonStrictCmp);
    TEST_EQUAL("<=(b:False, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0<=4.0", NonStrictCmp);
    TEST_EQUAL("<=(d:5.0, d:4.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0<=5.0", NonStrictCmp);
    TEST_EQUAL("<=(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0<=6.0", NonStrictCmp);
    TEST_EQUAL("<=(d:5.0, d:6.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr1\"<=\"teststr\"", NonStrictCmp);
    TEST_EQUAL("<=(s:teststr1, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"<=\"teststr\"", NonStrictCmp);
    TEST_EQUAL("<=(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\"<=\"teststr1\"", NonStrictCmp);
    TEST_EQUAL("<=(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<=(4,)", NonStrictCmp);
    TEST_EQUAL("<=(l:[i:5], l:[i:4])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5, 1]<=(5,)", NonStrictCmp);
    TEST_EQUAL("<=(l:[i:5, i:1], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<=(4,)", NonStrictCmp);
    TEST_EQUAL("<=(l:[i:5], l:[i:4])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<=(5.0, 6, False)", NonStrictCmp);
    TEST_EQUAL("<=(l:[i:5], l:[d:5.0, i:6, b:False])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]<=(6,)", NonStrictCmp);
    TEST_EQUAL("<=(l:[i:5], l:[i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[False]<=[1, 6]", NonStrictCmp);
    TEST_EQUAL("<=(l:[b:False], l:[i:1, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None<=None", NonStrictCmp);
    TEST_EQUAL("<=(n:None, n:None)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<=True", NonStrictCmp);
    TEST_EQUAL("<=(i:1, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<=False", NonStrictCmp);
    TEST_EQUAL("<=(i:1, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0<=True", NonStrictCmp);
    TEST_EQUAL("<=(i:0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0<=False", NonStrictCmp);
    TEST_EQUAL("<=(i:0, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0<=True", NonStrictCmp);
    TEST_EQUAL("<=(d:1.0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0.0<=True", NonStrictCmp);
    TEST_EQUAL("<=(d:0.0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<=1.0", NonStrictCmp);
    TEST_EQUAL("<=(i:1, d:1.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<=1.1", NonStrictCmp);
    TEST_EQUAL("<=(i:1, d:1.1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1<=[1.1]", NonStrictCmp);
    TEST_EQUAL("<=(i:1, l:[d:1.1])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testLessEqWithVarsNonStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("1<=foo", NonStrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("<=(i:1, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("False<=foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<=(b:False, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.2;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("\"teststr\"<=foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststq");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("[0]<=foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<=(l:[i:0], v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = boost::python::list();
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // lhs is variable
    exp = parseExpressionStr("foo<=1", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<=(v:foo, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 2;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.9;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo<=False", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<=(v:foo, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -0.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo<=\"teststr\"", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<=(v:foo, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststs");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo<=[0]", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<=(v:foo, l:[i:0])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // both operands are variables
    exp = parseExpressionStr("foo<=bar", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("<=(v:foo, v:bar)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.5;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = false;
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = std::string("teststr");
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr1");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr1");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = pyList({ 0 });
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = boost::python::list();
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));
}

void testGreaterWithConstantsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5>5", NonStrictCmp);
    TEST_EQUAL(">(i:5, i:5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("6>5", NonStrictCmp);
    TEST_EQUAL(">(i:6, i:5)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>True", NonStrictCmp);
    TEST_EQUAL(">(b:True, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>False", NonStrictCmp);
    TEST_EQUAL(">(b:True, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0>5.0", NonStrictCmp);
    TEST_EQUAL(">(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("6.0>5.0", NonStrictCmp);
    TEST_EQUAL(">(d:6.0, d:5.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\">\"teststr\"", NonStrictCmp);
    TEST_EQUAL(">(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr1\">\"teststr\"", NonStrictCmp);
    TEST_EQUAL(">(s:teststr1, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]>(5,)", NonStrictCmp);
    TEST_EQUAL(">(l:[i:5], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(5.0, 6, False)>[5]", NonStrictCmp);
    TEST_EQUAL(">(l:[d:5.0, i:6, b:False], l:[i:5])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[6]>(5,)", NonStrictCmp);
    TEST_EQUAL(">(l:[i:6], l:[i:5])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[1, 6]>[False]", NonStrictCmp);
    TEST_EQUAL(">(l:[i:1, i:6], l:[b:False])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None>None", NonStrictCmp);
    TEST_EQUAL(">(n:None, n:None)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>1", NonStrictCmp);
    TEST_EQUAL(">(b:True, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>0", NonStrictCmp);
    TEST_EQUAL(">(b:True, i:0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>1.0", NonStrictCmp);
    TEST_EQUAL(">(b:True, d:1.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>0.0", NonStrictCmp);
    TEST_EQUAL(">(b:True, d:0.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0>1", NonStrictCmp);
    TEST_EQUAL(">(d:1.0, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.1>1", NonStrictCmp);
    TEST_EQUAL(">(d:1.1, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[1.1]>1", NonStrictCmp);
    TEST_EQUAL(">(l:[d:1.1], i:1)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testGreaterWithVarsNonStrict()
{
    // lhs is variable
    ExpressionTree exp = parseExpressionStr("foo>1", NonStrictCmp);
    boost::python::dict vars;
    TEST_EQUAL(">(v:foo, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo>False", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">(v:foo, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.2;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo>\"teststr\"", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">(v:foo, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststs");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo>[0]", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">(v:foo, l:[i:0])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // rhs is variable
    exp = parseExpressionStr("1>foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">(i:1, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.9;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("False>foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">(b:False, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -0.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("\"teststr\">foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">(s:teststr, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststq");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("[0]>foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">(l:[i:0], v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // both operands are variables
    exp = parseExpressionStr("bar>foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">(v:bar, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.5;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = false;
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -0.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = std::string("teststr");
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("testst");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr1");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = pyList({ 0 });
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = boost::python::list();
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));
}

void testGreaterEqWithConstantsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("4>=5", NonStrictCmp);
    TEST_EQUAL(">=(i:4, i:5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5>=5", NonStrictCmp);
    TEST_EQUAL(">=(i:5, i:5)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("6>=5", NonStrictCmp);
    TEST_EQUAL(">=(i:6, i:5)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False>=True", NonStrictCmp);
    TEST_EQUAL(">=(b:False, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>=True", NonStrictCmp);
    TEST_EQUAL(">=(b:True, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>=False", NonStrictCmp);
    TEST_EQUAL(">=(b:True, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("4.0>=5.0", NonStrictCmp);
    TEST_EQUAL(">=(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0>=5.0", NonStrictCmp);
    TEST_EQUAL(">=(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("6.0>=5.0", NonStrictCmp);
    TEST_EQUAL(">=(d:6.0, d:5.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\">=\"teststr1\"", NonStrictCmp);
    TEST_EQUAL(">=(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\">=\"teststr\"", NonStrictCmp);
    TEST_EQUAL(">=(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr1\">=\"teststr\"", NonStrictCmp);
    TEST_EQUAL(">=(s:teststr1, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(4,)>=(5,)", NonStrictCmp);
    TEST_EQUAL(">=(l:[i:4], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(5,)>=[5, 1]", NonStrictCmp);
    TEST_EQUAL(">=(l:[i:5], l:[i:5, i:1])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(4,)>=[5]", NonStrictCmp);
    TEST_EQUAL(">=(l:[i:4], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(5.0, 6, False)>=[5]", NonStrictCmp);
    TEST_EQUAL(">=(l:[d:5.0, i:6, b:False], l:[i:5])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("(6,)>=[5]", NonStrictCmp);
    TEST_EQUAL(">=(l:[i:6], l:[i:5])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[1, 6]>=[False]", NonStrictCmp);
    TEST_EQUAL(">=(l:[i:1, i:6], l:[b:False])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None>=None", NonStrictCmp);
    TEST_EQUAL(">=(n:None, n:None)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>=1", NonStrictCmp);
    TEST_EQUAL(">=(b:True, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False>=1", NonStrictCmp);
    TEST_EQUAL(">=(b:False, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>=0", NonStrictCmp);
    TEST_EQUAL(">=(b:True, i:0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False>=0", NonStrictCmp);
    TEST_EQUAL(">=(b:False, i:0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>=1.0", NonStrictCmp);
    TEST_EQUAL(">=(b:True, d:1.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True>=0.0", NonStrictCmp);
    TEST_EQUAL(">=(b:True, d:0.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0>=1", NonStrictCmp);
    TEST_EQUAL(">=(d:1.0, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.1>=1", NonStrictCmp);
    TEST_EQUAL(">=(d:1.1, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[1.1]>=1", NonStrictCmp);
    TEST_EQUAL(">=(l:[d:1.1], i:1)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testGreaterEqWithVarsNonStrict()
{
    // lhs is variable
    ExpressionTree exp = parseExpressionStr("foo>=1", NonStrictCmp);
    boost::python::dict vars;
    TEST_EQUAL(">=(v:foo, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo>=False", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">=(v:foo, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.2;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("foo>=\"teststr\"", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">=(v:foo, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststq");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("foo>=[0]", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">=(v:foo, l:[i:0])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = boost::python::list();
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // rhs is variable
    exp = parseExpressionStr("1>=foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">=(i:1, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 2;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.9;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("False>=foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">=(b:False, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = -0.1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    exp = parseExpressionStr("\"teststr\">=foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">=(s:teststr, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststs");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    exp = parseExpressionStr("[0]>=foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">=(l:[i:0], v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    // both operands are variables
    exp = parseExpressionStr("bar>=foo", NonStrictCmp);
    vars.clear();
    TEST_EQUAL(">=(v:bar, v:foo)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.5;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = false;
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = false;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 0.1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));

    vars["bar"] = std::string("teststr");
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = pyList({ 0 });
    TEST_THROW(exp.eval(vars));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr1");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr1");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    vars["bar"] = pyList({ 0 });
    vars["foo"] = 6;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = false;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = 0.0;
    TEST_THROW(exp.eval(vars));
    vars["foo"] = boost::python::list();
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_THROW(exp.eval(vars));
}

void testIsWithConstantsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5 is 5", NonStrictCmp);
    TEST_EQUAL("is(i:5, i:5)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 is 6", NonStrictCmp);
    TEST_EQUAL("is(i:5, i:6)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True is True", NonStrictCmp);
    TEST_EQUAL("is(b:True, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True is False", NonStrictCmp);
    TEST_EQUAL("is(b:True, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0 is 5.0", NonStrictCmp);
    TEST_EQUAL("is(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0 is 6.0", NonStrictCmp);
    TEST_EQUAL("is(d:5.0, d:6.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\" is \"teststr\"", NonStrictCmp);
    TEST_EQUAL("is(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\" is \"teststr1\"", NonStrictCmp);
    TEST_EQUAL("is(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]is(5,)", NonStrictCmp);
    TEST_EQUAL("is(l:[i:5], l:[i:5])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None is None", NonStrictCmp);
    TEST_EQUAL("is(n:None, n:None)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None is 1", NonStrictCmp);
    TEST_EQUAL("is(n:None, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1 is True", NonStrictCmp);
    TEST_EQUAL("is(i:1, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0 is True", NonStrictCmp);
    TEST_EQUAL("is(d:1.0, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1 is 1.0", NonStrictCmp);
    TEST_EQUAL("is(i:1, d:1.0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));
}

void testIsWithVarsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("foo is bar", NonStrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("is(v:foo, v:bar)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    vars["bar"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 2;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
}

void testIsWithNotNonStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("(not 1) is (not foo)", NonStrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("is(not(i:1), not(v:foo))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("test");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));

    // both operands are variables
    exp = parseExpressionStr("(not foo)is(not bar)", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("is(not(v:foo), not(v:bar))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    vars["bar"] = boost::python::list();
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
}

void testIsNotWithConstantsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5 is not 5", NonStrictCmp);
    TEST_EQUAL("is_not(i:5, i:5)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 is not 6", NonStrictCmp);
    TEST_EQUAL("is_not(i:5, i:6)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True is not True", NonStrictCmp);
    TEST_EQUAL("is_not(b:True, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True is not False", NonStrictCmp);
    TEST_EQUAL("is_not(b:True, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0 is not 5.0", NonStrictCmp);
    TEST_EQUAL("is_not(d:5.0, d:5.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5.0 is not 6.0", NonStrictCmp);
    TEST_EQUAL("is_not(d:5.0, d:6.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\" is not \"teststr\"", NonStrictCmp);
    TEST_EQUAL("is_not(s:teststr, s:teststr)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"teststr\" is not \"teststr1\"", NonStrictCmp);
    TEST_EQUAL("is_not(s:teststr, s:teststr1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5]is not(5,)", NonStrictCmp);
    TEST_EQUAL("is_not(l:[i:5], l:[i:5])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None is not None", NonStrictCmp);
    TEST_EQUAL("is_not(n:None, n:None)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None is not 1", NonStrictCmp);
    TEST_EQUAL("is_not(n:None, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1 is not True", NonStrictCmp);
    TEST_EQUAL("is_not(i:1, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0 is not True", NonStrictCmp);
    TEST_EQUAL("is_not(d:1.0, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1 is not 1.0", NonStrictCmp);
    TEST_EQUAL("is_not(i:1, d:1.0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));
}

void testIsNotWithVarsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("foo is not bar", NonStrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("is_not(v:foo, v:bar)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 2;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = true;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1.0;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 1 });
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("teststr");
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
}

void testIsNotWithNotNonStrict()
{
    // rhs is variable
    ExpressionTree exp = parseExpressionStr("(not 1) is not (not foo)", NonStrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("is_not(not(i:1), not(v:foo))", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = std::string("test");
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));

    // both operands are variables
    exp = parseExpressionStr("(not foo)is not(not bar)", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("is_not(not(v:foo), not(v:bar))", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = 6;
    vars["bar"] = 1;
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(vars)));
    vars["foo"] = pyList({ 0 });
    vars["bar"] = boost::python::list();
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
}

void testInNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5 in [3, 4, 5, 6]", NonStrictCmp);
    TEST_EQUAL("in(i:5, l:[i:3, i:4, i:5, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2 in [3, 4, 5, 6]", NonStrictCmp);
    TEST_EQUAL("in(i:2, l:[i:3, i:4, i:5, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5] in [3, 4, [5], 6]", NonStrictCmp);
    TEST_EQUAL("in(l:[i:5], l:[i:3, i:4, l:[i:5], i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5] in [3, 4, 5, 6]", NonStrictCmp);
    TEST_EQUAL("in(l:[i:5], l:[i:3, i:4, i:5, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 in [3, 4, 5.0, 6]", NonStrictCmp);
    TEST_EQUAL("in(i:5, l:[i:3, i:4, d:5.0, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True in [3, 4, 1, 6]", NonStrictCmp);
    TEST_EQUAL("in(b:True, l:[i:3, i:4, i:1, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"Ar\" in [3, 4, 1, 6]", NonStrictCmp);
    TEST_EQUAL("in(s:Ar, l:[i:3, i:4, i:1, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"Ar\" in \"Arman\"", NonStrictCmp);
    TEST_EQUAL("in(s:Ar, s:Arman)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"rm\" in \"Arman\"", NonStrictCmp);
    TEST_EQUAL("in(s:rm, s:Arman)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 in \"Arman\"", NonStrictCmp);
    TEST_EQUAL("in(i:5, s:Arman)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("5 in 5", NonStrictCmp);
    TEST_EQUAL("in(i:5, i:5)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testNotInNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5 not in [3, 4, 5, 6]", NonStrictCmp);
    TEST_EQUAL("not_in(i:5, l:[i:3, i:4, i:5, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("2 not in [3, 4, 5, 6]", NonStrictCmp);
    TEST_EQUAL("not_in(i:2, l:[i:3, i:4, i:5, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5] not in [3, 4, [5], 6]", NonStrictCmp);
    TEST_EQUAL("not_in(l:[i:5], l:[i:3, i:4, l:[i:5], i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[2] not in [3, 4, [5], 6]", NonStrictCmp);
    TEST_EQUAL("not_in(l:[i:2], l:[i:3, i:4, l:[i:5], i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[5] not in [3, 4, 5, 6]", NonStrictCmp);
    TEST_EQUAL("not_in(l:[i:5], l:[i:3, i:4, i:5, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 not in [3, 4, 5.0, 6]", NonStrictCmp);
    TEST_EQUAL("not_in(i:5, l:[i:3, i:4, d:5.0, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True not in [3, 4, 1, 6]", NonStrictCmp);
    TEST_EQUAL("not_in(b:True, l:[i:3, i:4, i:1, i:6])", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"Ar\" not in [3, 4, 1, 6]", NonStrictCmp);
    TEST_EQUAL("not_in(s:Ar, l:[i:3, i:4, i:1, i:6])", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"Ar\" not in \"Arman\"", NonStrictCmp);
    TEST_EQUAL("not_in(s:Ar, s:Arman)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"rm\" not in \"Arman\"", NonStrictCmp);
    TEST_EQUAL("not_in(s:rm, s:Arman)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 not in \"Arman\"", NonStrictCmp);
    TEST_EQUAL("not_in(i:5, s:Arman)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("5 not in 5", NonStrictCmp);
    TEST_EQUAL("not_in(i:5, i:5)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testAndNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5 and 4", NonStrictCmp);
    TEST_EQUAL("and(i:5, i:4)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 and 0", NonStrictCmp);
    TEST_EQUAL("and(i:5, i:0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0 and 1", NonStrictCmp);
    TEST_EQUAL("and(i:0, i:1)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True and False", NonStrictCmp);
    TEST_EQUAL("and(b:True, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False and True", NonStrictCmp);
    TEST_EQUAL("and(b:False, b:True)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True and True", NonStrictCmp);
    TEST_EQUAL("and(b:True, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0 and 5.0", NonStrictCmp);
    TEST_EQUAL("and(d:1.0, d:5.0)", exp.toString());
    TEST_EQUAL(5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0 and 0.0", NonStrictCmp);
    TEST_EQUAL("and(d:1.0, d:0.0)", exp.toString());
    TEST_EQUAL(0.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0 and False", NonStrictCmp);
    TEST_EQUAL("and(d:1.0, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True and \"teststr\"", NonStrictCmp);
    TEST_EQUAL("and(b:True, s:teststr)", exp.toString());
    TEST_EQUAL("teststr", boost::python::extract<std::string>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"\" and True", NonStrictCmp);
    TEST_EQUAL("and(s:, b:True)", exp.toString());
    TEST_EQUAL("", boost::python::extract<std::string>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True and [5]", NonStrictCmp);
    TEST_EQUAL("and(b:True, l:[i:5])", exp.toString());
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[] and True", NonStrictCmp);
    TEST_EQUAL("and(l:[], b:True)", exp.toString());
    TEST_EQUAL(boost::python::list(), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None and True", NonStrictCmp);
    TEST_EQUAL("and(n:None, b:True)", exp.toString());
    TEST_EQUAL(true, exp.eval(boost::python::dict()).is_none());

    //'(a == 10 and (10 == 3 or "1" in "1234"))', 'b != 1', { 'a': 10, 'b' : 2 }
    exp = parseExpressionStr("(a == 10 and (10 == 3 or \"1\" in \"1234\"))", NonStrictCmp);
    boost::python::dict vars;
    parseAndConcatExpressionStr(exp, "b != 1", NonStrictCmp);
    vars["a"] = 10;
    vars["b"] = 2;
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(vars)));
}

void testOrNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5 or 4", NonStrictCmp);
    TEST_EQUAL("or(i:5, i:4)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5 or 0", NonStrictCmp);
    TEST_EQUAL("or(i:5, i:0)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0 or 1", NonStrictCmp);
    TEST_EQUAL("or(i:0, i:1)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0 or 0", NonStrictCmp);
    TEST_EQUAL("or(i:0, i:0)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False or False", NonStrictCmp);
    TEST_EQUAL("or(b:False, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True or False", NonStrictCmp);
    TEST_EQUAL("or(b:True, b:False)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False or True", NonStrictCmp);
    TEST_EQUAL("or(b:False, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True or True", NonStrictCmp);
    TEST_EQUAL("or(b:True, b:True)", exp.toString());
    TEST_EQUAL(true, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1.0 or 5.0", NonStrictCmp);
    TEST_EQUAL("or(d:1.0, d:5.0)", exp.toString());
    TEST_EQUAL(1.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0.0 or 0.0", NonStrictCmp);
    TEST_EQUAL("or(d:0.0, d:0.0)", exp.toString());
    TEST_EQUAL(0.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("0.0 or False", NonStrictCmp);
    TEST_EQUAL("or(d:0.0, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False or \"teststr\"", NonStrictCmp);
    TEST_EQUAL("or(b:False, s:teststr)", exp.toString());
    TEST_EQUAL("teststr", boost::python::extract<std::string>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"\" or False", NonStrictCmp);
    TEST_EQUAL("or(s:, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False or [5]", NonStrictCmp);
    TEST_EQUAL("or(b:False, l:[i:5])", exp.toString());
    TEST_EQUAL(pyList({ 5 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[] or False", NonStrictCmp);
    TEST_EQUAL("or(l:[], b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("None or False", NonStrictCmp);
    TEST_EQUAL("or(n:None, b:False)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False or None", NonStrictCmp);
    TEST_EQUAL("or(b:False, n:None)", exp.toString());
    TEST_EQUAL(false, boost::python::extract<bool>(exp.eval(boost::python::dict())));
}

void testNumericPlusNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5+6", NonStrictCmp);
    TEST_EQUAL("+(i:5, i:6)", exp.toString());
    TEST_EQUAL(11, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True+True", NonStrictCmp);
    TEST_EQUAL("+(b:True, b:True)", exp.toString());
    TEST_EQUAL(2, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True+False", NonStrictCmp);
    TEST_EQUAL("+(b:True, b:False)", exp.toString());
    TEST_EQUAL(1, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("4.0+5.0", NonStrictCmp);
    TEST_EQUAL("+(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(9.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5+True", NonStrictCmp);
    TEST_EQUAL("+(i:5, b:True)", exp.toString());
    TEST_EQUAL(6, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False+5", NonStrictCmp);
    TEST_EQUAL("+(b:False, i:5)", exp.toString());
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5+7.0", NonStrictCmp);
    TEST_EQUAL("+(i:5, d:7.0)", exp.toString());
    TEST_EQUAL(12.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("7.0+5", NonStrictCmp);
    TEST_EQUAL("+(d:7.0, i:5)", exp.toString());
    TEST_EQUAL(12.0, boost::python::extract<double>(exp.eval(boost::python::dict())));
}

void testStringPlusNonStrict()
{
    ExpressionTree exp = parseExpressionStr("\"Te\"+\"st\"", NonStrictCmp);
    TEST_EQUAL("+(s:Te, s:st)", exp.toString());
    TEST_EQUAL("Test", boost::python::extract<std::string>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"Te\"+\"\"", NonStrictCmp);
    TEST_EQUAL("+(s:Te, s:)", exp.toString());
    TEST_EQUAL("Te", boost::python::extract<std::string>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("\"\"+\"st\"", NonStrictCmp);
    TEST_EQUAL("+(s:, s:st)", exp.toString());
    TEST_EQUAL("st", boost::python::extract<std::string>(exp.eval(boost::python::dict())));
}

void testListPlusNonStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] + [3, 4]", NonStrictCmp);
    TEST_EQUAL("+(l:[i:1, i:2], l:[i:3, i:4])", exp.toString());
    TEST_EQUAL(pyList({ 1, 2, 3, 4 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[1, 2] + []", NonStrictCmp);
    TEST_EQUAL("+(l:[i:1, i:2], l:[])", exp.toString());
    TEST_EQUAL(pyList({ 1, 2 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[] + [3, 4]", NonStrictCmp);
    TEST_EQUAL("+(l:[], l:[i:3, i:4])", exp.toString());
    TEST_EQUAL(pyList({ 3, 4 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("[] + []", NonStrictCmp);
    TEST_EQUAL("+(l:[], l:[])", exp.toString());
    TEST_EQUAL(boost::python::list(), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));
}

void testPlusErrorsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] + \"test\"", NonStrictCmp);
    TEST_EQUAL("+(l:[i:1, i:2], s:test)", exp.toString());
    TEST_THROW(boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("1 + \"test\"", NonStrictCmp);
    TEST_EQUAL("+(i:1, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1.0 + \"test\"", NonStrictCmp);
    TEST_EQUAL("+(d:1.0, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True + \"test\"", NonStrictCmp);
    TEST_EQUAL("+(b:True, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testListPlusVarsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("foo + bar", NonStrictCmp);
    boost::python::dict vars;
    TEST_EQUAL("+(v:foo, v:bar)", exp.toString());
    TEST_THROW(exp.eval(vars));

    vars["foo"] = 5;
    TEST_THROW(exp.eval(vars));

    vars["bar"] = 6;
    TEST_EQUAL(11, boost::python::extract<int>(exp.eval(vars)));

    vars["foo"] = pyList({ 1, 2 });
    vars["bar"] = pyList({ 3, 4 });
    TEST_EQUAL(pyList({ 1, 2, 3, 4 }), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("foo + foo", NonStrictCmp);
    vars.clear();
    vars["foo"] = pyList({ 1, 2 });
    TEST_EQUAL(pyList({ 1, 2, 1, 2 }), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("foo + bar", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("+(v:foo, v:bar)", exp.toString());
    vars["foo"] = std::string("te");
    vars["bar"] = std::string("st");
    TEST_EQUAL(std::string("test"), boost::python::extract<std::string>(exp.eval(vars)));

    exp = parseExpressionStr("[1, 2, foo] + [foo, 3]", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("+(l:[i:1, i:2, v:foo], l:[v:foo, i:3])", exp.toString());
    vars["foo"] = std::string("te");
    TEST_EQUAL(pyList(1, 2, std::string("te"), std::string("te"), 3), boost::python::extract<boost::python::list>(exp.eval(vars)));

    exp = parseExpressionStr("[1, 2, foo] + [bar, 3]", NonStrictCmp);
    vars.clear();
    TEST_EQUAL("+(l:[i:1, i:2, v:foo], l:[v:bar, i:3])", exp.toString());
    vars["foo"] = std::string("te");
    vars["bar"] = std::string("st");
    TEST_EQUAL(pyList(1, 2, std::string("te"), std::string("st"), 3), boost::python::extract<boost::python::list>(exp.eval(vars)));

    TEST_EQUAL("+(l:[i:1, i:2, v:foo], l:[v:bar, i:3])", exp.toString());
    vars["foo"] = pyList({ 3, 4 });
    vars["bar"] = pyList({ 5, 6 });
    boost::python::list res;
    res.append(1);
    res.append(2);
    res.append(pyList({ 3, 4 }));
    res.append(pyList({ 5, 6 }));
    res.append(3);
    TEST_EQUAL(res, boost::python::extract<boost::python::list>(exp.eval(vars)));
}

void testNumericMinusNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5-6", NonStrictCmp);
    TEST_EQUAL("-(i:5, i:6)", exp.toString());
    TEST_EQUAL(-1, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True-True", NonStrictCmp);
    TEST_EQUAL("-(b:True, b:True)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True-False", NonStrictCmp);
    TEST_EQUAL("-(b:True, b:False)", exp.toString());
    TEST_EQUAL(1, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("4.0-5.0", NonStrictCmp);
    TEST_EQUAL("-(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(-1.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5-True", NonStrictCmp);
    TEST_EQUAL("-(i:5, b:True)", exp.toString());
    TEST_EQUAL(4, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False-5", NonStrictCmp);
    TEST_EQUAL("-(b:False, i:5)", exp.toString());
    TEST_EQUAL(-4, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5-7.0", NonStrictCmp);
    TEST_EQUAL("-(i:5, d:7.0)", exp.toString());
    TEST_EQUAL(-2.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("7.0-5", NonStrictCmp);
    TEST_EQUAL("-(d:7.0, i:5)", exp.toString());
    TEST_EQUAL(2.0, boost::python::extract<double>(exp.eval(boost::python::dict())));
}

void testMinusErrorsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] - \"test\"", NonStrictCmp);
    TEST_EQUAL("-(l:[i:1, i:2], s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1 - \"test\"", NonStrictCmp);
    TEST_EQUAL("-(i:1, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1.0 - \"test\"", NonStrictCmp);
    TEST_EQUAL("-(d:1.0, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True - \"test\"", NonStrictCmp);
    TEST_EQUAL("-(b:True, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("\"test\" - \"test\"", NonStrictCmp);
    TEST_EQUAL("-(s:test, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("[1, 2] - [1, 2]", NonStrictCmp);
    TEST_EQUAL("-(l:[i:1, i:2], l:[i:1, i:2])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testNumericMulNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5*6", NonStrictCmp);
    TEST_EQUAL("*(i:5, i:6)", exp.toString());
    TEST_EQUAL(30, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True*True", NonStrictCmp);
    TEST_EQUAL("*(b:True, b:True)", exp.toString());
    TEST_EQUAL(1, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True*False", NonStrictCmp);
    TEST_EQUAL("*(b:True, b:False)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("4.0*5.0", NonStrictCmp);
    TEST_EQUAL("*(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(20.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5*True", NonStrictCmp);
    TEST_EQUAL("*(i:5, b:True)", exp.toString());
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5*[3]", NonStrictCmp);
    TEST_EQUAL("*(i:5, l:[i:3])", exp.toString());
    TEST_EQUAL(pyList({ 3, 3, 3, 3, 3 }), boost::python::extract<boost::python::list>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False*5", NonStrictCmp);
    TEST_EQUAL("*(b:False, i:5)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5*7.0", NonStrictCmp);
    TEST_EQUAL("*(i:5, d:7.0)", exp.toString());
    TEST_EQUAL(35, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("7.0*5", NonStrictCmp);
    TEST_EQUAL("*(d:7.0, i:5)", exp.toString());
    TEST_EQUAL(35, boost::python::extract<double>(exp.eval(boost::python::dict())));
}

void testMulErrorsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] * \"test\"", NonStrictCmp);
    TEST_EQUAL("*(l:[i:1, i:2], s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1.0 * \"test\"", NonStrictCmp);
    TEST_EQUAL("*(d:1.0, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("\"test\" * \"test\"", NonStrictCmp);
    TEST_EQUAL("*(s:test, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("[1, 2] * [1, 2]", NonStrictCmp);
    TEST_EQUAL("*(l:[i:1, i:2], l:[i:1, i:2])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testNumericDivNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5/6", NonStrictCmp);
    TEST_EQUAL("/(i:5, i:6)", exp.toString());
    TEST_EQUAL(double(5) / double(6), boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5/0", NonStrictCmp);
    TEST_EQUAL("/(i:5, i:0)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True/True", NonStrictCmp);
    TEST_EQUAL("/(b:True, b:True)", exp.toString());
    TEST_EQUAL(1.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True/False", NonStrictCmp);
    TEST_EQUAL("/(b:True, b:False)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("4.0/5.0", NonStrictCmp);
    TEST_EQUAL("/(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(4.0 / 5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5/True", NonStrictCmp);
    TEST_EQUAL("/(i:5, b:True)", exp.toString());
    TEST_EQUAL(5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False/5", NonStrictCmp);
    TEST_EQUAL("/(b:False, i:5)", exp.toString());
    TEST_EQUAL(0.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5/7.0", NonStrictCmp);
    TEST_EQUAL("/(i:5, d:7.0)", exp.toString());
    TEST_EQUAL(5 / 7.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("7.0/5", NonStrictCmp);
    TEST_EQUAL("/(d:7.0, i:5)", exp.toString());
    TEST_EQUAL(7.0 / 5, boost::python::extract<double>(exp.eval(boost::python::dict())));
}

void testDivErrorsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] / \"test\"", NonStrictCmp);
    TEST_EQUAL("/(l:[i:1, i:2], s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1 / \"test\"", NonStrictCmp);
    TEST_EQUAL("/(i:1, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1.0 / \"test\"", NonStrictCmp);
    TEST_EQUAL("/(d:1.0, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True / \"test\"", NonStrictCmp);
    TEST_EQUAL("/(b:True, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("\"test\" / \"test\"", NonStrictCmp);
    TEST_EQUAL("/(s:test, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("[1, 2] / [1, 2]", NonStrictCmp);
    TEST_EQUAL("/(l:[i:1, i:2], l:[i:1, i:2])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testNumericFloorNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5//6", NonStrictCmp);
    TEST_EQUAL("//(i:5, i:6)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5//0", NonStrictCmp);
    TEST_EQUAL("//(i:5, i:0)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True//True", NonStrictCmp);
    TEST_EQUAL("//(b:True, b:True)", exp.toString());
    TEST_EQUAL(1, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True//False", NonStrictCmp);
    TEST_EQUAL("//(b:True, b:False)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("4.0//5.0", NonStrictCmp);
    TEST_EQUAL("//(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(0.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5//True", NonStrictCmp);
    TEST_EQUAL("//(i:5, b:True)", exp.toString());
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False//5", NonStrictCmp);
    TEST_EQUAL("//(b:False, i:5)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5//7.0", NonStrictCmp);
    TEST_EQUAL("//(i:5, d:7.0)", exp.toString());
    TEST_EQUAL(0.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("7.0//5", NonStrictCmp);
    TEST_EQUAL("//(d:7.0, i:5)", exp.toString());
    TEST_EQUAL(1.0, boost::python::extract<double>(exp.eval(boost::python::dict())));
}

void testFloorErrorsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] // \"test\"", NonStrictCmp);
    TEST_EQUAL("//(l:[i:1, i:2], s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1 // \"test\"", NonStrictCmp);
    TEST_EQUAL("//(i:1, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1.0 // \"test\"", NonStrictCmp);
    TEST_EQUAL("//(d:1.0, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True // \"test\"", NonStrictCmp);
    TEST_EQUAL("//(b:True, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("\"test\" // \"test\"", NonStrictCmp);
    TEST_EQUAL("//(s:test, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("[1, 2] // [1, 2]", NonStrictCmp);
    TEST_EQUAL("//(l:[i:1, i:2], l:[i:1, i:2])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void testNumericRmNonStrict()
{
    ExpressionTree exp = parseExpressionStr("5%6", NonStrictCmp);
    TEST_EQUAL("%(i:5, i:6)", exp.toString());
    TEST_EQUAL(5, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5%0", NonStrictCmp);
    TEST_EQUAL("%(i:5, i:0)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True%True", NonStrictCmp);
    TEST_EQUAL("%(b:True, b:True)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("True%False", NonStrictCmp);
    TEST_EQUAL("%(b:True, b:False)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("4.0%5.0", NonStrictCmp);
    TEST_EQUAL("%(d:4.0, d:5.0)", exp.toString());
    TEST_EQUAL(4.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5%True", NonStrictCmp);
    TEST_EQUAL("%(i:5, b:True)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("False%5", NonStrictCmp);
    TEST_EQUAL("%(b:False, i:5)", exp.toString());
    TEST_EQUAL(0, boost::python::extract<int>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("5%7.0", NonStrictCmp);
    TEST_EQUAL("%(i:5, d:7.0)", exp.toString());
    TEST_EQUAL(5.0, boost::python::extract<double>(exp.eval(boost::python::dict())));

    exp = parseExpressionStr("7.0%5", NonStrictCmp);
    TEST_EQUAL("%(d:7.0, i:5)", exp.toString());
    TEST_EQUAL(2.0, boost::python::extract<double>(exp.eval(boost::python::dict())));
}

void testRmErrorsNonStrict()
{
    ExpressionTree exp = parseExpressionStr("[1, 2] % \"test\"", NonStrictCmp);
    TEST_EQUAL("%(l:[i:1, i:2], s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1 % \"test\"", NonStrictCmp);
    TEST_EQUAL("%(i:1, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("1.0 % \"test\"", NonStrictCmp);
    TEST_EQUAL("%(d:1.0, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("True % \"test\"", NonStrictCmp);
    TEST_EQUAL("%(b:True, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("\"test\" % \"test\"", NonStrictCmp);
    TEST_EQUAL("%(s:test, s:test)", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));

    exp = parseExpressionStr("[1, 2] % [1, 2]", NonStrictCmp);
    TEST_EQUAL("%(l:[i:1, i:2], l:[i:1, i:2])", exp.toString());
    TEST_THROW(exp.eval(boost::python::dict()));
}

void databaseTest();

void test()
{
    try {
        databaseTest();
        return;
        testConstantsStrict();
        testNoneStrict();
        testVarsAndPathsStrict();
        testList1Strict();
        testList1TupleStrict();
        testList2Strict();
        testList2TupleStrict();
        testList1VarStrict();
        testList1VarTupleStrict();
        testList2VarStrict();
        testList2VarTupleStrict();
        testNotWithConstantsStrict();
        testNotWithVarsStrict();
        testNegateWithConstantsStrict();
        testNegateWithVarsStrict();
        testEqualWithConstantsStrict();
        testEqualWithVarsStrict();
        testEqualWithNotStrict();
        testNotEqualWithConstantsStrict();
        testNotEqualWithVarsStrict();
        testNotEqualWithNotStrict();

        testLessWithConstantsStrict();
        testLessWithVarsStrict();
        testLessEqWithConstantsStrict();
        testLessEqWithVarsStrict();

        testGreaterWithConstantsStrict();
        testGreaterWithVarsStrict();
        testGreaterEqWithConstantsStrict();
        testGreaterEqWithVarsStrict();

        testIsWithConstantsStrict();
        testIsWithVarsStrict();
        testIsWithNotStrict();
        testIsNotWithConstantsStrict();
        testIsNotWithVarsStrict();
        testIsNotWithNotStrict();

        testInStrict();
        testNotInStrict();

        testAndStrict();
        testOrStrict();

        testNumericPlusStrict();
        testStringPlusStrict();
        testListPlusStrict();
        testListPlusVarsStrict();
        testPlusErrorsStrict();

        testNumericPlusStrict();
        testMinusErrorsStrict();

        testNumericMulStrict();
        testMulErrorsStrict();

        testNumericDivStrict();
        testDivErrorsStrict();

        testNumericFloorStrict();
        testFloorErrorsStrict();

        testNumericRmStrict();
        testRmErrorsStrict();

        testConstantsNonStrict();
        testNoneNonStrict();
        testVarsAndPathsNonStrict();
        testList1NonStrict();
        testList1TupleNonStrict();
        testList2NonStrict();
        testList2TupleNonStrict();
        testList1VarNonStrict();
        testList1VarTupleNonStrict();
        testList2VarNonStrict();
        testList2VarTupleNonStrict();
        testNotWithConstantsNonStrict();
        testNotWithVarsNonStrict();
        testNegateWithConstantsNonStrict();
        testNegateWithVarsNonStrict();
        testEqualWithConstantsNonStrict();
        testEqualWithVarsNonStrict();
        testEqualWithNotNonStrict();
        testNotEqualWithConstantsNonStrict();
        testNotEqualWithVarsNonStrict();
        testNotEqualWithNotNonStrict();

        testLessWithConstantsNonStrict();
        testLessWithVarsNonStrict();
        testLessEqWithConstantsNonStrict();
        testLessEqWithVarsNonStrict();

        testGreaterWithConstantsNonStrict();
        testGreaterWithVarsNonStrict();
        testGreaterEqWithConstantsNonStrict();
        testGreaterEqWithVarsNonStrict();

        testIsWithConstantsNonStrict();
        testIsWithVarsNonStrict();
        testIsWithNotNonStrict();
        testIsNotWithConstantsNonStrict();
        testIsNotWithVarsNonStrict();
        testIsNotWithNotNonStrict();

        testInNonStrict();
        testNotInNonStrict();

        testAndNonStrict();
        testOrNonStrict();

        testNumericPlusNonStrict();
        testStringPlusNonStrict();
        testListPlusNonStrict();
        testListPlusVarsNonStrict();
        testPlusErrorsNonStrict();

        testNumericPlusNonStrict();
        testMinusErrorsNonStrict();

        testNumericMulNonStrict();
        testMulErrorsNonStrict();

        testNumericDivNonStrict();
        testDivErrorsNonStrict();

        testNumericFloorNonStrict();
        testFloorErrorsNonStrict();

        testNumericRmNonStrict();
        testRmErrorsNonStrict();
        std::cout << "++++++PASSED" << std::endl;
    }
    catch (boost::python::error_already_set) {
        PyErr_Print();
    }
    catch (const std::exception& e) {
        std::cerr << "Unexpected Exception:" << e.what() << std::endl;
    }
}

const unsigned int SERIALIZER_VER = 1;

void testSerializerWTypeWSizeNone()
{
    std::unique_ptr<db::Serializer> sz = db::Serializer::create(SERIALIZER_VER);

    boost::python::object res;
    db::Buffer buff;

    sz->serializeWTypeWSize(buff, boost::python::object(0));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::object(0));
}

void testSerializerWTypeWSizeInt()
{
    std::unique_ptr<db::Serializer> sz = db::Serializer::create(SERIALIZER_VER);

    boost::python::object res;
    db::Buffer buff;

    sz->serializeWTypeWSize(buff, boost::python::long_(0));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_(0));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_(1));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_(1));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_(-1));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_(-1));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_((long)std::numeric_limits<int8_t>::max()));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int8_t>::max()));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_((long)std::numeric_limits<int8_t>::min()));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int8_t>::min()));
    
    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_((long)std::numeric_limits<int8_t>::max() + 1));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int8_t>::max() + 1));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_((long)std::numeric_limits<int8_t>::min() - 1));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int8_t>::min() - 1));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_((long)std::numeric_limits<int16_t>::max()));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int16_t>::max()));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_((long)std::numeric_limits<int16_t>::min()));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int16_t>::min()));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_((long)std::numeric_limits<int16_t>::max() +1));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int16_t>::max() +1));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_((long)std::numeric_limits<int16_t>::min() - 1));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int16_t>::min() - 1));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_((long)std::numeric_limits<int32_t>::max()));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int32_t>::max()));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_((long)std::numeric_limits<int32_t>::min()));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int32_t>::min()));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_((long)std::numeric_limits<int32_t>::max()) + 1);
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int32_t>::max()) + 1);

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::long_((long)std::numeric_limits<int32_t>::min()) - 1);
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int32_t>::min()) - 1);

    for (size_t ord = 1; ord < 100; ++ord) {
        boost::python::object pnum = (boost::python::long_(1) << ord);
        boost::python::object nnum = boost::python::long_(0) -pnum;

        buff.clear();
        sz->serializeWTypeWSize(buff, pnum);
        TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
        TEST_EQUAL(res, pnum);

        buff.clear();
        sz->serializeWTypeWSize(buff, nnum);
        TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
        TEST_EQUAL(res, nnum);
    }
}

void testSerializerWTypeWSizeBool()
{
    std::unique_ptr<db::Serializer> sz = db::Serializer::create(SERIALIZER_VER);

    boost::python::object res;
    db::Buffer buff;

    sz->serializeWTypeWSize(buff, boost::python::object(true));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::object(true));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::object(false));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::object(false));
}

void testSerializerWTypeWSizeDouble()
{
    std::unique_ptr<db::Serializer> sz = db::Serializer::create(SERIALIZER_VER);

    boost::python::object res;
    db::Buffer buff;

    sz->serializeWTypeWSize(buff, boost::python::object(0.0));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::object(0.0));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::object(1.0));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::object(1.0));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::object(-1.0));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::object(-1.0));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::object(-1.5));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::object(-1.5));

    buff.clear();
    sz->serializeWTypeWSize(buff, boost::python::object(10.1));
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, boost::python::object(10.1));
}

void testSerializerWTypeWSizeStr()
{
    std::unique_ptr<db::Serializer> sz = db::Serializer::create(SERIALIZER_VER);

    boost::python::object res;
    db::Buffer buff;

    boost::python::str val = boost::python::str("Value1");
    sz->serializeWTypeWSize(buff, val);
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, val);

    buff.clear();
    val = boost::python::str("");
    sz->serializeWTypeWSize(buff, val);
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, val);

    buff.clear();
    val = boost::python::str(std::wstring(L"\x043a\x043e\x0448\x043a\x0430 \x65e5\x672c\x56fd"));
    sz->serializeWTypeWSize(buff, val);
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, val);

    buff.clear();
    val = boost::python::str("Value").replace("a", '\0');
    sz->serializeWTypeWSize(buff, val);
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, val);

    buff.clear();
    val = boost::python::str(std::wstring(L"\x043a\x043e\x000\x043a\x0430 \x65e5\x672c\x56fd"));
    sz->serializeWTypeWSize(buff, val);
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, val);
}

void testSerializerWTypeWSizeWPickle()
{
    std::unique_ptr<db::Serializer> sz = db::Serializer::create(SERIALIZER_VER);

    boost::python::object res;
    db::Buffer buff;

    boost::python::tuple val;
    sz->serializeWTypeWSize(buff, val);
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, val);

    buff.clear();
    val = boost::python::tuple(("", ""));
    sz->serializeWTypeWSize(buff, val);
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, val);

    buff.clear();
    val = boost::python::tuple(("Val1", "Val2"));
    sz->serializeWTypeWSize(buff, val);
    TEST_NOTHROW(res = sz->deserializeWTypeWSize(buff));
    TEST_EQUAL(res, val);
}

void testSerializerWTypeWSize()
{
    testSerializerWTypeWSizeNone();
    testSerializerWTypeWSizeInt();
    testSerializerWTypeWSizeBool();
    testSerializerWTypeWSizeDouble();
    testSerializerWTypeWSizeStr();
    testSerializerWTypeWSizeWPickle();
}

void testSerializerWTypeNone()
{
    std::unique_ptr<db::Serializer> sz = db::Serializer::create(SERIALIZER_VER);

    boost::python::object res;
    db::Buffer buff;

    sz->serializeWType(buff, boost::python::object(0));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::object(0));
}

void testSerializerWTypeInt()
{
    std::unique_ptr<db::Serializer> sz = db::Serializer::create(SERIALIZER_VER);

    boost::python::object res;
    db::Buffer buff;

    sz->serializeWType(buff, boost::python::long_(0));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_(0));

    buff.clear();
    sz->serializeWType(buff, boost::python::long_(1));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_(1));

    buff.clear();
    sz->serializeWType(buff, boost::python::long_(-1));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_(-1));

    buff.clear();
    sz->serializeWType(buff, boost::python::long_((long)std::numeric_limits<int8_t>::max()));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int8_t>::max()));

    buff.clear();
    sz->serializeWType(buff, boost::python::long_((long)std::numeric_limits<int8_t>::min()));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int8_t>::min()));

    buff.clear();
    sz->serializeWType(buff, boost::python::long_((long)std::numeric_limits<int8_t>::max() + 1));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int8_t>::max() + 1));

    buff.clear();
    sz->serializeWType(buff, boost::python::long_((long)std::numeric_limits<int8_t>::min() - 1));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int8_t>::min() - 1));

    buff.clear();
    sz->serializeWType(buff, boost::python::long_((long)std::numeric_limits<int16_t>::max()));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int16_t>::max()));

    buff.clear();
    sz->serializeWType(buff, boost::python::long_((long)std::numeric_limits<int16_t>::min()));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int16_t>::min()));

    buff.clear();
    sz->serializeWType(buff, boost::python::long_((long)std::numeric_limits<int16_t>::max() + 1));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int16_t>::max() + 1));

    buff.clear();
    sz->serializeWType(buff, boost::python::long_((long)std::numeric_limits<int16_t>::min() - 1));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int16_t>::min() - 1));

    buff.clear();
    sz->serializeWType(buff, boost::python::long_((long)std::numeric_limits<int32_t>::max()));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int32_t>::max()));

    buff.clear();
    sz->serializeWType(buff, boost::python::long_((long)std::numeric_limits<int32_t>::min()));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int32_t>::min()));

    buff.clear();
    sz->serializeWType(buff, boost::python::long_((long)std::numeric_limits<int32_t>::max()) + 1);
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int32_t>::max()) + 1);

    buff.clear();
    sz->serializeWType(buff, boost::python::long_((long)std::numeric_limits<int32_t>::min()) - 1);
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::long_((long)std::numeric_limits<int32_t>::min()) - 1);

    for (size_t ord = 1; ord < 100; ++ord) {
        boost::python::object pnum = (boost::python::long_(1) << ord);
        boost::python::object nnum = boost::python::long_(0) - pnum;

        buff.clear();
        sz->serializeWType(buff, pnum);
        TEST_NOTHROW(res = sz->deserializeWType(buff));
        TEST_EQUAL(res, pnum);

        buff.clear();
        sz->serializeWType(buff, nnum);
        TEST_NOTHROW(res = sz->deserializeWType(buff));
        TEST_EQUAL(res, nnum);
    }
}

void testSerializerWTypeBool()
{
    std::unique_ptr<db::Serializer> sz = db::Serializer::create(SERIALIZER_VER);

    boost::python::object res;
    db::Buffer buff;

    sz->serializeWType(buff, boost::python::object(true));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::object(true));

    buff.clear();
    sz->serializeWType(buff, boost::python::object(false));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::object(false));
}

void testSerializerWTypeDouble()
{
    std::unique_ptr<db::Serializer> sz = db::Serializer::create(SERIALIZER_VER);

    boost::python::object res;
    db::Buffer buff;

    sz->serializeWType(buff, boost::python::object(0.0));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::object(0.0));

    buff.clear();
    sz->serializeWType(buff, boost::python::object(1.0));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::object(1.0));

    buff.clear();
    sz->serializeWType(buff, boost::python::object(-1.0));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::object(-1.0));

    buff.clear();
    sz->serializeWType(buff, boost::python::object(-1.5));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::object(-1.5));

    buff.clear();
    sz->serializeWType(buff, boost::python::object(10.1));
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, boost::python::object(10.1));
}

void testSerializerWTypeStr()
{
    std::unique_ptr<db::Serializer> sz = db::Serializer::create(SERIALIZER_VER);

    boost::python::object res;
    db::Buffer buff;

    boost::python::str val = boost::python::str("Value1");
    sz->serializeWType(buff, val);
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, val);

    buff.clear();
    val = boost::python::str("");
    sz->serializeWType(buff, val);
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, val);

    buff.clear();
    val = boost::python::str(std::wstring(L"\x043a\x043e\x0448\x043a\x0430 \x65e5\x672c\x56fd"));
    sz->serializeWType(buff, val);
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, val);

    buff.clear();
    val = boost::python::str("Value").replace("a", '\0');
    sz->serializeWType(buff, val);
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, val);

    buff.clear();
    val = boost::python::str(std::wstring(L"\x043a\x043e\x000\x043a\x0430 \x65e5\x672c\x56fd"));
    sz->serializeWType(buff, val);
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, val);
}

void testSerializerWTypeWPickle()
{
    std::unique_ptr<db::Serializer> sz = db::Serializer::create(SERIALIZER_VER);

    boost::python::object res;
    db::Buffer buff;

    boost::python::tuple val;
    sz->serializeWType(buff, val);
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, val);

    buff.clear();
    val = boost::python::tuple(("", ""));
    sz->serializeWType(buff, val);
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, val);

    buff.clear();
    val = boost::python::tuple(("Val1", "Val2"));
    sz->serializeWType(buff, val);
    TEST_NOTHROW(res = sz->deserializeWType(buff));
    TEST_EQUAL(res, val);
}

void testSerializerWType()
{
    testSerializerWTypeNone();
    testSerializerWTypeInt();
    testSerializerWTypeBool();
    testSerializerWTypeDouble();
    testSerializerWTypeStr();
    testSerializerWTypeWPickle();
}

using namespace boost::python;

void testOpenAppend(object main_namespace)
{
    std::string pyCode = "db = aimdb.open('ROCKDBCF', 'testdbs\\\\test1.db', 'a')\n"
        "rw = db.addRunWriter('run1', {'metric':'m1'}, {'field':'f1'}, {'param':'p1'})\n"
        "rw.dumpMeta()\n"
        "rw=None\n"
        "db=None\n";
    handle<> ignored((PyRun_String(pyCode.c_str(),
        Py_file_input,
        main_namespace.ptr(),
        main_namespace.ptr())));
    boost::python::object obj(ignored);
}

void databaseTest()
{
    try {
        db::initPyEmbedding();
        Py_Initialize();
        testSerializerWTypeWSize();
        testSerializerWType();
        object main_module((
            handle<>(borrowed(PyImport_AddModule("__main__")))));
        object main_namespace = main_module.attr("__dict__");
        object cpp_module((handle<>(PyImport_ImportModule("aimdb"))));
        main_namespace["aimdb"] = cpp_module;
        for (int i = 0; i < 50; ++i) {
            std::cerr << i << std::endl;
            testOpenAppend(main_namespace);
        }
    }
    catch (error_already_set) {
        PyErr_Print();
    }
}