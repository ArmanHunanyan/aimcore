#ifndef ROCKS_DB_H
#define ROCKS_DB_H

#include "database.h"

#include <map>

namespace rocksdb {
    class DB;
    class ColumnFamilyHandle;
}

namespace db {
    class Serializer;
}

namespace rdbV1 {

    struct RockDB_Run
    {
        RockDB_Run() = default;
        rocksdb::ColumnFamilyHandle* data;
        rocksdb::ColumnFamilyHandle* meta;
    };

    class RockDB_Database;

    class RockDB_RunWriter
        : public db::RunWriter
    {
    public:
        RockDB_RunWriter(RockDB_Run* run,
            RockDB_Database* db,
            const boost::python::dict& runMetrics,
            const boost::python::dict& runFields,
            const boost::python::dict& runParameters);
        ~RockDB_RunWriter() = default;
        virtual void close()  override;

        virtual void update(const boost::python::dict& runMetrics,
            const boost::python::dict& runFields,
            const boost::python::dict& runParameters) override;

        class RockDB_Trace
            : public db::RunWriter::Trace
        {
            virtual void appendMetric(
                const boost::python::object& value,
                const boost::python::long_& epoch,
                const boost::python::long_& iteration,
                const boost::python::long_& timestamp
            ) override;
        };

        class RockDB_Metric
            : public db::RunWriter::Metric
        {
        public:
            virtual std::unique_ptr<db::RunWriter::Trace> addTrace(
                const boost::python::object& metric,
                const boost::python::dict& trace
            ) override;

            virtual void appendMetric(
                const boost::python::object& value,
                const boost::python::long_& epoch,
                const boost::python::long_& iteration,
                const boost::python::long_& timestamp
            ) override;
        };

        virtual std::unique_ptr<db::RunWriter::Metric> addMetric(
            const boost::python::str& name
        ) override {
            return 0;
        }

        virtual void dumpMeta() override;

    private:
        void setMeta(unsigned char, const boost::python::object& key, const boost::python::object& val);

    private:
        RockDB_Run* m_run;
        RockDB_Database* m_database;
    };

    class RockDB_Database
        : public db::Database
    {
    public:
        RockDB_Database(const boost::python::str& ppath, const boost::python::str& mode);
        ~RockDB_Database() override;

        virtual std::unique_ptr<db::RunWriter> addRunWriter(const boost::python::str& hash,
            const boost::python::dict& runMetrics,
            const boost::python::dict& runFields,
            const boost::python::dict& runParameters) override;

        virtual void close() override;

    private:
        void openDatabase(const boost::python::str& path, const boost::python::str& mode);
        void closeDatabase();
        void openReadOnly(const boost::python::str& path);
        void openWriteAppend(const boost::python::str& path);
        void openWriteTruncate(const boost::python::str& path);
        void openWriteNew(const boost::python::str& path);

    private:
        template <typename NameI, typename HandleI>
        void initRuns(NameI firstName, NameI lastName, HandleI firstHandle, HandleI lastHandle);

        enum class CFNameType {
            META,
            DATA,
            UNKNOWN
        };

        std::pair<std::string, CFNameType> parsRunName(const std::string& name);
        std::string metaCFName(const boost::python::str& runName);
        std::string dataCFName(const boost::python::str& runName);

    private:
        rocksdb::DB* m_db;

        typedef std::map<boost::python::str, RockDB_Run> RunMap;
        RunMap m_runs;

        rocksdb::ColumnFamilyHandle* m_defaultCF;

        class MetaComparator;

        MetaComparator* m_metaComparator;

        static const unsigned m_currentMinorVersion;
        unsigned m_minorVersion;
        static const unsigned m_currentSerializerVersion;
        unsigned m_serializerVersion;
        db::Serializer* m_serializer;

        friend class RockDB_RunWriter;
        const db::Serializer* serializer() const { return m_serializer; }
        rocksdb::DB* db() { return m_db; }

        std::vector<rocksdb::ColumnFamilyHandle*> m_cf;
        bool m_isNew;
    };

} // namespace dbimpl

namespace rdb {
    void registerRocksDBEngine();
}

#endif
