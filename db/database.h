#ifndef DATABASE_H
#define DATABASE_H

#include "pyutils.h"
#include "dberror.h"

#include <boost/python.hpp>
#include <stdexcept>

namespace db {

class RunWriter
{
public:
    RunWriter() = default;
    virtual ~RunWriter() = default;
    RunWriter(const RunWriter&) = delete;
    RunWriter& operator=(const RunWriter&) = delete;

    virtual void close() = 0;

    virtual void update(const boost::python::dict& runMetrics,
                        const boost::python::dict& runFields,
                        const boost::python::dict& runParameters) = 0;

    class MetricContainer
    {
    public:
        virtual void appendMetric(
            const boost::python::object& value,
            const boost::python::long_& epoch,
            const boost::python::long_& iteration,
            const boost::python::long_& timestamp
        ) = 0;
    };

    class Trace
        : public MetricContainer
    {};

    class Metric
        : public MetricContainer
    {
    public:
        virtual std::unique_ptr<Trace> addTrace(
            const boost::python::object& metric,
            const boost::python::dict& trace
        ) = 0;


        boost::python::object addTracePy(
            const boost::python::object& metric,
            const boost::python::dict& trace
        )
        {
            return pyutils::transferToPython(addTrace(metric, trace));
        }
    };

    virtual std::unique_ptr<Metric> addMetric(
        const boost::python::str& name
    ) = 0;

public:
    boost::python::object addMetricPy(
        const boost::python::str& name
    )
    {
        return pyutils::transferToPython(addMetric(name));
    }

public:
    // Optional debug interface
    virtual void dumpMeta() = 0;
};

class Database
{
protected:
    Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

public:
    virtual ~Database() = default;

    virtual std::unique_ptr<RunWriter> addRunWriter(const boost::python::str& hash,
                                                    const boost::python::dict& runMetrics,
                                                    const boost::python::dict& runFields,
                                                    const boost::python::dict& runParameters) = 0;

    virtual void close() = 0;

    boost::python::object addRunWriterPy(const boost::python::str& hash,
        const boost::python::dict& runMetrics,
        const boost::python::dict& runFields,
        const boost::python::dict& runParameters)
    {
        return pyutils::transferToPython(addRunWriter(hash, runMetrics, runFields, runParameters));
    }

    static std::unique_ptr<Database> open(const boost::python::str& engineName, const boost::python::str& path, const boost::python::str& mode);
    static boost::python::object openPy(const boost::python::str& engineName, const boost::python::str& path, const boost::python::str& mode)
    {
        return pyutils::transferToPython(open(engineName, path, mode));
    }

public:
    typedef std::unique_ptr<Database> (* OpenProc) (const boost::python::str& path, const boost::python::str& mode);
    static void registerEngine(const boost::python::str& name, OpenProc proc);

private:
    class EngineRegister;
    static EngineRegister& engineRegister();
};

void initPyEmbedding();

} // namespace db

#endif