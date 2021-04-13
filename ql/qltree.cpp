#include "qltree.h"

#include <boost/mpl/vector.hpp>
#include <boost/mpl/max_element.hpp>
#include <boost/mpl/transform_view.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/sizeof.hpp>
#include <boost/variant.hpp>
#include <boost/python.hpp>
#include <boost/python/operators.hpp>

#include <vector>
#include <deque>
#include <sstream>
#include <unordered_map>

namespace ql {

template <typename T>
void destroy(T* val)
{
    val->~T();
}

class ExpressionNode
{
public:
    virtual ~ExpressionNode() { }

    virtual std::string toString() const = 0; // For debug only
    virtual void append(ExpressionNode* element) { assert(false); }
    virtual void append(const boost::iterator_range<const char*>& element) { assert(false); }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const { assert(false); }
};

class BinOpExpression
    : public ExpressionNode
{
public:
    virtual ~BinOpExpression()
    {
        destroy(m_lhs);
        destroy(m_rhs);
    }

    BinOpExpression(ExpressionNode* lhs, ExpressionNode* rhs)
        : m_lhs(lhs)
        , m_rhs(rhs)
    { }

    virtual std::string opName() const = 0;
    virtual std::string toString() const
    {
        std::string res = this->opName() + "(" + m_lhs->toString() + ", " + m_rhs->toString() + ")";
        return res;
    }

    const ExpressionNode* lhs() const { return m_lhs; }
    const ExpressionNode* rhs() const { return m_rhs; }

private:
    ExpressionNode* m_lhs;
    ExpressionNode* m_rhs;
};

class PlusExpression
    : public BinOpExpression
{
public:
    PlusExpression(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "+"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        lhs()->eval(variables, tmp1);
        boost::python::object tmp2;
        rhs()->eval(variables, tmp2);
        res = boost::python::object(tmp1 + tmp2);
    }
};

class MinusExpression
    : public BinOpExpression
{
public:
    MinusExpression(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "-"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        lhs()->eval(variables, tmp1);
        boost::python::object tmp2;
        rhs()->eval(variables, tmp2);
        res = boost::python::object(tmp1 - tmp2);
    }
};

class MulExpression
    : public BinOpExpression
{
public:
    MulExpression(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "*"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        lhs()->eval(variables, tmp1);
        boost::python::object tmp2;
        rhs()->eval(variables, tmp2);
        res = boost::python::object(tmp1 * tmp2);
    }
};

class DivExpression
    : public BinOpExpression
{
public:
    DivExpression(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "/"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        lhs()->eval(variables, tmp1);
        boost::python::object tmp2;
        rhs()->eval(variables, tmp2);
        res = boost::python::object(boost::python::handle<>(PyNumber_TrueDivide(tmp1.ptr(), tmp2.ptr())));
    }
};

class FloorDivExpression
    : public BinOpExpression
{
public:
    FloorDivExpression(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "//"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        lhs()->eval(variables, tmp1);
        boost::python::object tmp2;
        rhs()->eval(variables, tmp2);
        res = boost::python::object(boost::python::handle<>(PyNumber_FloorDivide(tmp1.ptr(), tmp2.ptr())));
    }
};

class ReminderExpression
    : public BinOpExpression
{
public:
    ReminderExpression(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "%"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        lhs()->eval(variables, tmp1);
        boost::python::object tmp2;
        rhs()->eval(variables, tmp2);
        res = boost::python::object(tmp1 % tmp2);
    }
};

class OrNode
    : public BinOpExpression
{
public:
    OrNode(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "or"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        lhs()->eval(variables, res);
        if (res) {
            return;
        }
        rhs()->eval(variables, res);
    }
};

class AndNode
    : public BinOpExpression
{
public:
    AndNode(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "and"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        lhs()->eval(variables, res);
        if (!res) {
            return;
        }
        rhs()->eval(variables, res);
    }
};

class InNode
    : public BinOpExpression
{
public:
    InNode(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "in"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        lhs()->eval(variables, tmp1);
        boost::python::object tmp2;
        rhs()->eval(variables, tmp2);
        res = boost::python::object(tmp2.contains(tmp1));
    }
};

class NotInNode
    : public BinOpExpression
{
public:
    NotInNode(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "not_in"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        lhs()->eval(variables, tmp1);
        boost::python::object tmp2;
        rhs()->eval(variables, tmp2);
        res = boost::python::object(!tmp2.contains(tmp1));
    }
};

class IsNode
    : public BinOpExpression
{
public:
    IsNode(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "is"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        lhs()->eval(variables, tmp1);
        boost::python::object tmp2;
        rhs()->eval(variables, tmp2);
        res = boost::python::object(tmp1.ptr() == tmp2.ptr());
    }
};

class IsNotNode
    : public BinOpExpression
{
public:
    IsNotNode(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "is_not"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        lhs()->eval(variables, tmp1);
        boost::python::object tmp2;
        rhs()->eval(variables, tmp2);
        res = boost::python::object(tmp1.ptr() != tmp2.ptr());
    }
};

template <typename CmpNode>
class NonStrictCmp
    : public CmpNode
{
public:
    using CmpNode::CmpNode;

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        CmpNode::lhs()->eval(variables, tmp1);
        if (tmp1.is_none()) {
            res = boost::python::object(false);
            return;
        }
        boost::python::object tmp2;
        CmpNode::rhs()->eval(variables, tmp2);
        if (tmp2.is_none()) {
            res = boost::python::object(false);
        } else {
            res = boost::python::object(CmpNode::cmp(tmp1, tmp2));
        }
    }
};

template <typename CmpNode>
class StrictCmp
    : public CmpNode
{
public:
    using CmpNode::CmpNode;

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        CmpNode::lhs()->eval(variables, tmp1);
        boost::python::object tmp2;
        CmpNode::rhs()->eval(variables, tmp2);
        res = boost::python::object(CmpNode::cmp(tmp1, tmp2));
    }
};

class LessNode
    : public BinOpExpression
{
public:
    LessNode(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "<"; }

    bool cmp(const boost::python::object& obj1, const boost::python::object& obj2) const
    {
        return obj1 < obj2;
    }
};

class LessEqualNode
    : public BinOpExpression
{
public:
    LessEqualNode(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "<="; }

    bool cmp(const boost::python::object& obj1, const boost::python::object& obj2) const
    {
        return obj1 <= obj2;
    }
};

class GreaterNode
    : public BinOpExpression
{
public:
    GreaterNode(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return ">"; }

    bool cmp(const boost::python::object& obj1, const boost::python::object& obj2) const
    {
        return obj1 > obj2;
    }
};

class GreaterEqualNode
    : public BinOpExpression
{
public:
    GreaterEqualNode(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return ">="; }

    bool cmp(const boost::python::object& obj1, const boost::python::object& obj2) const
    {
        return obj1 >= obj2;
    }
};

class EqualNode
    : public BinOpExpression
{
public:
    EqualNode(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "=="; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        lhs()->eval(variables, tmp1);
        boost::python::object tmp2;
        rhs()->eval(variables, tmp2);
        res = boost::python::object(tmp1 == tmp2);
    }
};

class NotEqualNode
    : public BinOpExpression
{
public:
    NotEqualNode(ExpressionNode* lhs, ExpressionNode* rhs)
        : BinOpExpression(lhs, rhs)
    { }

    virtual std::string opName() const { return "!="; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::object tmp1;
        lhs()->eval(variables, tmp1);
        boost::python::object tmp2;
        rhs()->eval(variables, tmp2);
        res = boost::python::object(tmp1 != tmp2);
    }
};

class UnOpNodeBase
    : public ExpressionNode
{
public:
    virtual ~UnOpNodeBase()
    {
        destroy(m_operand);
    }

    UnOpNodeBase(ExpressionNode* op)
        : m_operand(op)
    { }

    virtual std::string opName() const = 0;
    virtual std::string toString() const
    {
        std::string res = this->opName() + "(" + m_operand->toString() + ")";
        return res;
    }

    ExpressionNode* operand()
    {
        return m_operand;
    }

    const ExpressionNode* operand() const
    {
        return m_operand;
    }

private:
    ExpressionNode* m_operand;
};

class NegateNode
    : public UnOpNodeBase
{
public:
    NegateNode(ExpressionNode* op)
        : UnOpNodeBase(op)
    { }

private:
    virtual std::string opName() const { return "-"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        operand()->eval(variables, res);
        res = boost::python::object(boost::python::handle<>(PyNumber_Negative(res.ptr())));
    }
};

class NotNode
    : public UnOpNodeBase
{
public:
    NotNode(ExpressionNode* op)
        : UnOpNodeBase(op)
    { }

    virtual std::string opName() const { return "not"; }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        operand()->eval(variables, res);
        res = boost::python::object(!res);
    }
};

template <typename Type>
struct TypeChar;

template <>
struct TypeChar<std::string>
{
    static const char result = 's';
};

template <>
struct TypeChar<int>
{
    static const char result = 'i';
};

template <>
struct TypeChar<bool>
{
    static const char result = 'b';
};

template <>
struct TypeChar<double>
{
    static const char result = 'd';
};

// This class is template for debugging and testing purposes. 
// It uses template Type argument only in toString() function. See TypeChar
template <typename Type>
class ConstValueNode
    : public ExpressionNode
{
public:
    virtual ~ConstValueNode()
    {
    }

    ConstValueNode(Type val)
        : m_val(val)
    {
    }

    // Constructor for Type = std::string
    template <typename It>
    ConstValueNode(const boost::iterator_range<It>& pair)
        : m_val(std::string(pair.begin() + 1, pair.end() - 1))
    { }

    virtual std::string toString() const
    {
        return TypeChar<Type>::result + std::string(":") + boost::python::extract<std::string>(boost::python::str(m_val))();
    }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        res = m_val;
    }

private:
    static std::string to_string_impl(const std::string& s)
    {
        return "s:" + s;
    }

    static std::string to_string_impl(int v)
    {
        return "i:" + std::to_string(v);
    }

    static std::string to_string_impl(bool v)
    {
        return v ? "true" : "false";
    }

    static std::string to_string_impl(double v)
    {
        return "d:" + std::to_string(v);
    }

    template <typename T>
    static std::string to_string_impl(T val)
    {
        assert(false);
        return std::to_string(0);
    }

private:
    boost::python::object m_val;
};

class NoneNode
    : public ExpressionNode
{
public:
    virtual ~NoneNode()
    {
    }

    NoneNode()
    { }

    virtual std::string toString() const
    {
        return "n:None";
    }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        res = boost::python::object();
    }
};

class ListNode
    : public ExpressionNode
{
public:
    virtual ~ListNode()
    {
        for (auto expr : m_list)
        {
            destroy(expr);
        }
    }

    ListNode(ExpressionNode* first)
        : m_list()
    {
        m_list.push_back(first);
    }

    ListNode()
        : m_list()
    {
    }

    virtual void append(ExpressionNode* expr)
    {
        m_list.push_back(expr);
    }

    virtual std::string toString() const
    {
        if (m_list.empty()) {
            return "l:[]";
        }
        std::ostringstream oss;

        oss << "l:[" << m_list[0]->toString();
        for (size_t idx = 1, size = m_list.size(); idx < size; ++idx) {
            oss << ", " << m_list[idx]->toString();
        }
        oss << "]";
        return oss.str();
    }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        boost::python::list tmp;
        for (const auto* expr : m_list) {
            boost::python::object exprRes;
            expr->eval(variables, exprRes);
            tmp.append(exprRes);
        }
        res = tmp;
    }

private:
    std::vector<ExpressionNode*> m_list;
};

class PathNode
    : public ExpressionNode
{
public:
    virtual ~PathNode()
    {
    }

    PathNode(const boost::iterator_range<const char*>& pair)
        : ExpressionNode()
        , m_ids()
    {
        m_ids.emplace_back(pair.begin(), pair.end());
    }

    virtual void append(const boost::iterator_range<const char*>& pair)
    {
        m_ids.emplace_back(pair.begin(), pair.end());
    }

    virtual std::string toString() const
    {
        if (m_ids.empty()) {
            return "";
        }
        std::ostringstream oss;

        oss << m_ids[0];
        for (size_t idx = 1, size = m_ids.size(); idx < size; ++idx) {
            oss << "." << m_ids[idx];
        }
        return "p:" + oss.str();
    }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        assert(m_ids.size() != 0);
        evalImpl(variables, res, 0);
    }

    virtual void evalImpl(const boost::python::dict& variables, boost::python::object& res, size_t idx) const
    {
        if (idx == m_ids.size() - 1) {
            res = variables.get(m_ids[idx]);
        } else {
            boost::python::object ob = variables.get(m_ids[idx]);
            if (ob.is_none()) {
                res = boost::python::object();
            } else {
                boost::python::extract<boost::python::dict> extractDict(ob);
                if (!extractDict.check()) {
                    res = boost::python::object();
                } else {
                    evalImpl(extractDict(), res, idx + 1);
                }
            }
        }
    }

private:
    std::vector<std::string> m_ids;
};

class VariableNode
    : public ExpressionNode
{
public:
    virtual ~VariableNode()
    {
    }

    VariableNode(std::string&& name)
        : ExpressionNode()
        , m_name(name)
    {
    }

    virtual std::string toString() const
    {
        return "v:" + m_name;
    }

    virtual void eval(const boost::python::dict& variables, boost::python::object& res) const
    {
        res = variables.get(m_name);
    }

private:
    std::string m_name;
};

namespace mpl = boost::mpl;

class ExpressionNodeHeap
{
public:
    void* allocNode(size_t sz)
    {
        assert(sz <= MaxSizeType::value);
        m_nodes.emplace_back();
        return &m_nodes.back();
    }

private:
    typedef boost::mpl::joint_view < // Using join view because of vector's 10 type hard limit
        mpl::vector <
        AndNode,
        InNode,
        NotInNode,
        IsNode,
        IsNotNode,
        StrictCmp<LessNode>,
        StrictCmp<LessEqualNode>,
        StrictCmp<GreaterNode>,
        StrictCmp<GreaterEqualNode>,
        NonStrictCmp<LessNode>,
        NonStrictCmp<LessEqualNode>,
        NonStrictCmp<GreaterNode>,
        NonStrictCmp<GreaterEqualNode>,
        NotEqualNode,
        EqualNode,
        PlusExpression,
        MinusExpression,
        MulExpression,
        DivExpression,
        FloorDivExpression>,
        mpl::vector <
        ReminderExpression,
        NotNode,
        NegateNode,
        ListNode,
        VariableNode,
        ConstValueNode<bool>,
        ConstValueNode<int>,
        ConstValueNode<double>,
        ConstValueNode<std::string>,
        NoneNode,
        PathNode >> AllNodes;

    typedef typename mpl::deref<
        typename mpl::max_element<
        mpl::transform_view<AllNodes, mpl::sizeof_<mpl::_1> >
        >::type
    >::type MaxSizeType;

    struct NodeMem
    {
        unsigned char bytes[MaxSizeType::value];
    };

    std::deque<NodeMem> m_nodes;
};

PExpressionNode ExpressionTree::createOrNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<OrNode>(lhs, rhs);
}

PExpressionNode ExpressionTree::createAndNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<AndNode>(lhs, rhs);
}

PExpressionNode ExpressionTree::createInNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<InNode>(lhs, rhs);
}

PExpressionNode ExpressionTree::createNotInNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<NotInNode>(lhs, rhs);
}

PExpressionNode ExpressionTree::createIsNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<IsNode>(lhs, rhs);
}

PExpressionNode ExpressionTree::createIsNotNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<IsNotNode>(lhs, rhs);
}

PExpressionNode ExpressionTree::createLessNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    if (strict) {
        return allocNode<StrictCmp<LessNode>>(lhs, rhs);
    } else {
        return allocNode<NonStrictCmp<LessNode>>(lhs, rhs);
    }
    
}

PExpressionNode ExpressionTree::createLessEqualNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    if (strict) {
        return allocNode<StrictCmp<LessEqualNode>>(lhs, rhs);
    }  else {
        return allocNode<NonStrictCmp<LessEqualNode>>(lhs, rhs);
    }
}

PExpressionNode ExpressionTree::createGreaterNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    if (strict) {
        return allocNode<StrictCmp<GreaterNode>>(lhs, rhs);
    } else {
        return allocNode<NonStrictCmp<GreaterNode>>(lhs, rhs);
    }
}

PExpressionNode ExpressionTree::createGreaterEqualNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    if (strict) {
        return allocNode<StrictCmp<GreaterEqualNode>>(lhs, rhs);
    } else {
        return allocNode<NonStrictCmp<GreaterEqualNode>>(lhs, rhs);
    }
}

PExpressionNode ExpressionTree::createNotEqualNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<NotEqualNode>(lhs, rhs);
}

PExpressionNode ExpressionTree::createEqualNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<EqualNode>(lhs, rhs);
}

PExpressionNode ExpressionTree::createPlusNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<PlusExpression>(lhs, rhs);
}

PExpressionNode ExpressionTree::createMinusNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<MinusExpression>(lhs, rhs);
}

PExpressionNode ExpressionTree::createMulNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<MulExpression>(lhs, rhs);
}

PExpressionNode ExpressionTree::createDivNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<DivExpression>(lhs, rhs);
}

PExpressionNode ExpressionTree::createFloorDivNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<FloorDivExpression>(lhs, rhs);
}

PExpressionNode ExpressionTree::createReminderNode(PExpressionNode lhs, PExpressionNode rhs, bool strict)
{
    return allocNode<ReminderExpression>(lhs, rhs);
}

PExpressionNode ExpressionTree::createNotNode(PExpressionNode op, bool strict)
{
    return allocNode<NotNode>(op);
}

PExpressionNode ExpressionTree::createNegateNode(PExpressionNode op, bool strict)
{
    return allocNode<NegateNode>(op);
}

PExpressionNode ExpressionTree::createListNode(PExpressionNode first, bool strict)
{
    return allocNode<ListNode>(first);
}

PExpressionNode ExpressionTree::createEmptyList(bool strict)
{
    return allocNode<ListNode>();
}

void ExpressionTree::appendListNode(PExpressionNode list, PExpressionNode first)
{
    list->append(first);
}

PExpressionNode ExpressionTree::createVariableNode(const boost::iterator_range<const char*>& range, bool strict)
{
    std::string name(range.begin(), range.end());
    return allocNode<VariableNode>(std::move(name));
}

PExpressionNode ExpressionTree::createConstBoolNode(bool val, bool strict)
{
    return allocNode<ConstValueNode<bool>>(val);
}

PExpressionNode ExpressionTree::createConstIntNode(int val, bool strict)
{
    return allocNode<ConstValueNode<int>>(val);
}

PExpressionNode ExpressionTree::createConstDoubleNode(double val, bool strict)
{
    return allocNode<ConstValueNode<double>>(val);
}

PExpressionNode ExpressionTree::createConstStringNode(const boost::iterator_range<const char*>& pair, bool strict)
{
    return allocNode<ConstValueNode<std::string>>(pair);
}

PExpressionNode ExpressionTree::createNoneNode(bool strict)
{
    return allocNode<NoneNode>();
}

PExpressionNode ExpressionTree::createPathNode(const boost::iterator_range<const char*>& first, bool strict)
{
    return allocNode<PathNode>(first);
}

void ExpressionTree::appendPathNode(PExpressionNode list, const boost::iterator_range<const char*>& elem)
{
    list->append(elem);
}

ExpressionTree::ExpressionTree()
    : m_root(0)
    , m_nodeHeap(new ExpressionNodeHeap())
{
}

ExpressionTree::~ExpressionTree()
{
    if (nullptr != m_root) {
        destroy(m_root);
    }
    delete m_nodeHeap;
}

ExpressionTree::ExpressionTree(ExpressionTree&& rhs)
    : m_root(rhs.m_root)
    , m_nodeHeap(rhs.m_nodeHeap)
{
    rhs.m_root = nullptr;
    rhs.m_nodeHeap = nullptr;
}

ExpressionTree& ExpressionTree::operator=(ExpressionTree&& rhs)
{
    if (&rhs != this) {
        if (nullptr != m_root) {
            destroy(m_root);
        }
        delete m_nodeHeap;

        m_root = rhs.m_root;
        m_nodeHeap = rhs.m_nodeHeap;
        rhs.m_root = nullptr;
        rhs.m_nodeHeap = nullptr;
    }
    return *this;
}

std::string ExpressionTree::toString() const
{
    assert(0 != m_root);
    return m_root->toString();
}

boost::python::object ExpressionTree::eval(const boost::python::dict& variables) const
{
    boost::python::object res;
    m_root->eval(variables, res);
    return res;
}

bool  ExpressionTree::evalBool(const boost::python::dict& variables) const
{
    boost::python::object res;
    m_root->eval(variables, res);
    return boost::python::extract<bool>(res);
}

void ExpressionTree::assignRoot(ExpressionNode* root)
{
    // root should have to be allocated from m_nodeHeap
    assert(m_root == nullptr); // making assignRoot callable only once to be safe from having root and m_nodeHeap non-compatible.
    if (nullptr != m_root) {
        destroy(m_root);
    }
    m_root = root;
}

void ExpressionTree::concatRoot(ExpressionNode* root)
{
    assert(m_root != nullptr);
    ExpressionNode* newRoot = createAndNode(m_root, root, true /* Make no sense since both args will be convertable to bool*/); 
    m_root = newRoot;
}

template <typename Node, typename... Args>
Node* ExpressionTree::allocNode(Args&&... args)
{
    return new (m_nodeHeap->allocNode(sizeof(Node))) Node(std::forward<Args>(args)...);
}

} // namespace ql
