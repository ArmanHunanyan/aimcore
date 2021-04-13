#ifndef PYUTILS_H
#define PYUTILS_H

#include <boost/python.hpp>

namespace pyutils {

template <typename T>
inline boost::python::object transferToPython(std::unique_ptr<T>&& ptr)
{
    typename boost::python::manage_new_object::apply<T*>::type converter;

    boost::python::handle<> handle(converter(*ptr));
    ptr.release();

    return boost::python::object(handle);
}

inline const char* cstr(const boost::python::str& str)
{
    return (const char*)boost::python::extract<const char*>(str);
}

inline const char* cstr(const boost::python::object& str)
{
    return (const char*)boost::python::extract<const char*>(str);
}

inline boost::python::object dictItemsView(const boost::python::dict& dict)
{
#if PY_MAJOR_VERSION >= 3
    return dict.items();
#else
    return dict.iteritems();
#endif
}

template <typename Func>
inline void iterDict(const boost::python::dict& dict, Func func)
{
    using boost::python::stl_input_iterator;
    using boost::python::tuple;
    auto items = dictItemsView(dict);
    for (auto it = stl_input_iterator<tuple>(items); it != stl_input_iterator<tuple>(); ++it) {
        func(*it);
    }
}

namespace detail {

template <typename Int>
struct CreateIntObjectHelper
{
    static boost::python::object run(Int val)
    {
        BOOST_STATIC_ASSERT(sizeof(Int) <= sizeof(long));
        return boost::python::object(boost::python::handle<>(PyLong_FromLong((long)val)));
    }
};

template <>
struct CreateIntObjectHelper<int64_t>
{
    static boost::python::object run(int64_t val)
    {
        return boost::python::object(boost::python::handle<>(PyLong_FromLongLong(val)));
    }
};

} // namespace detail

template <typename Int>
boost::python::object createIntObject(Int val)
{
    BOOST_STATIC_ASSERT(std::numeric_limits<Int>::is_signed); // Not implemented for unsigneds
    return detail::CreateIntObjectHelper<Int>::run(val);
}

} // namespace pyutils

#endif // PYUTILS_H