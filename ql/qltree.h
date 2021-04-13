#ifndef QL_TREE_H
#define QL_TREE_H

#include <boost/range/iterator_range.hpp>
#include <boost/python/object.hpp>
#include <boost/python/dict.hpp>
#include <boost/variant.hpp>

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>

namespace ql {

class ExpressionNode;
class ExpressionNodeHeap;

typedef ExpressionNode* PExpressionNode;

class ExpressionTree
{
public:
    ExpressionTree();
    ~ExpressionTree();
    ExpressionTree(ExpressionTree&& rhs);

    ExpressionTree& operator=(ExpressionTree&& rhs);

    ExpressionTree(const ExpressionTree& rhs) = delete;
    ExpressionTree& operator=(const ExpressionTree& rhs) = delete;
    std::string toString() const; // For debugging only

    boost::python::object eval(const boost::python::dict& variables) const;
    bool evalBool(const boost::python::dict& variables) const;

public:
    PExpressionNode createOrNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createAndNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createInNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createNotInNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createIsNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createIsNotNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createLessNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createLessEqualNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createGreaterNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createGreaterEqualNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createNotEqualNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createEqualNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createPlusNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createMinusNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createMulNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createDivNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createFloorDivNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createReminderNode(PExpressionNode lhs, PExpressionNode rhs, bool strict);
    PExpressionNode createNotNode(PExpressionNode op, bool strict);
    PExpressionNode createNegateNode(PExpressionNode op, bool strict);
    PExpressionNode createListNode(PExpressionNode first, bool strict);
    PExpressionNode createEmptyList(bool strict);
    void appendListNode(PExpressionNode list, PExpressionNode first);
    PExpressionNode createVariableNode(const boost::iterator_range<const char*>& name, bool strict);
    PExpressionNode createConstBoolNode(bool val, bool strict);
    PExpressionNode createConstIntNode(int val, bool strict);
    PExpressionNode createConstDoubleNode(double val, bool strict);
    PExpressionNode createConstStringNode(const boost::iterator_range<const char*>& pair, bool strict);
    PExpressionNode createNoneNode(bool strict);
    PExpressionNode createPathNode(const boost::iterator_range<const char*>& first, bool strict);
    void appendPathNode(PExpressionNode list, const boost::iterator_range<const char*>& elem);

private:
    friend ExpressionTree parseExpression(const char*, const char*, bool);
    void assignRoot(ExpressionNode* root);

    friend void parseAndConcatExpression(ExpressionTree& res, const char*, const char*, bool);
    void concatRoot(ExpressionNode* root);

    template <typename Node, typename... Args>
    Node* allocNode(Args&&... args);

private:
    ExpressionNode* m_root;
    ExpressionNodeHeap* m_nodeHeap;
};

} // namespace ql

#endif