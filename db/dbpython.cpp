#include "database.h"
#include "rocksdb.h"

namespace db {

    void syntaxErrorTransliator(db::Error const& x) {
        PyErr_SetString(PyExc_RuntimeError, x.what());
    }

} // namespace db

BOOST_PYTHON_MODULE(aimdb)
{
    using boost::python::class_;
    using boost::python::def;
    using boost::python::make_function;
    using boost::python::register_exception_translator;

    register_exception_translator<
        db::Error>(db::syntaxErrorTransliator);

    def("open", db::Database::openPy);

    class_<db::RunWriter, boost::noncopyable>("dbrwifc", boost::python::no_init)
        .def("addMetric", &db::RunWriter::addMetricPy)
        .def("update", &db::RunWriter::update)
        .def("close", &db::RunWriter::close)
        .def("dumpMeta", &db::RunWriter::dumpMeta)
        ;

    class_<db::Database, boost::noncopyable>("dbifc", boost::python::no_init)
        .def("addRunWriter", &db::Database::addRunWriterPy)
        .def("close", &db::Database::close)
        ;

    rdb::registerRocksDBEngine();
}

void db::initPyEmbedding()
{
#if PY_MAJOR_VERSION >= 3
    PyImport_AppendInittab("aimdb", &PyInit_aimdb);
#else
    PyImport_AppendInittab("aimdb", &initaimdb);
#endif
}