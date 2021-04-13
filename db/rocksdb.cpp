#include "rocksdb.h"
#include "serializer.h"
#include <rocksdb/db.h>

#include <cassert>
#include <iostream>
#include <chrono>
#include <sstream>

namespace rdb {

    struct DBVersion
    {
        DBVersion() = default;
        DBVersion(unsigned major, unsigned minor, unsigned serializer)
            : major(major)
            , minor(minor)
            , serializer(serializer)
        { }

        void writeTo(rocksdb::DB* db, rocksdb::ColumnFamilyHandle* cf)
        {
            rocksdb::Status status;
            status = db->Put(rocksdb::WriteOptions(), cf, keyMajorVersion, std::to_string(major));
            if (!status.ok()) {
                throw db::Error(std::string("Failed to write version"));
            }
            status = db->Put(rocksdb::WriteOptions(), cf, keyMinorVersion, std::to_string(minor));
            if (!status.ok()) {
                throw db::Error(std::string("Failed to write version"));
            }
            status = db->Put(rocksdb::WriteOptions(), cf, keySerializerVersion, std::to_string(serializer));
            if (!status.ok()) {
                throw db::Error(std::string("Failed to write version"));
            }
        }

        void readFrom(rocksdb::DB* db, rocksdb::ColumnFamilyHandle* cf, const char* path)
        {
            std::string majorVer, minorVer, serializerVer;
            rocksdb::Status status;
            status = db->Get(rocksdb::ReadOptions(), cf, keyMajorVersion, &majorVer);
            if (!status.ok()) {
                throw db::Error(std::string("Failed to open database'") +
                    path + "': Corrupted database");
            }
            status = db->Get(rocksdb::ReadOptions(), cf, keyMinorVersion, &minorVer);
            if (!status.ok()) {
                throw db::Error(std::string("Failed to open database'") +
                    path + "': Corrupted database");
            }
            status = db->Get(rocksdb::ReadOptions(), cf, keySerializerVersion, &serializerVer);
            if (!status.ok()) {
                throw db::Error(std::string("Failed to open database'") +
                    path + "': Corrupted database");
            }

            std::istringstream iss(majorVer);
            iss >> major;
            if (!iss || !iss.eof()) {
                throw db::Error(std::string("Failed to open database'") +
                    path + "': Corrupted database");
            }

            iss.clear();
            iss.str(minorVer);
            iss >> minor;
            if (!iss || !iss.eof()) {
                throw db::Error(std::string("Failed to open database'") +
                    path + "': Corrupted database");
            }

            iss.clear();
            iss.str(serializerVer);
            iss >> serializer;
            if (!iss || !iss.eof()) {
                throw db::Error(std::string("Failed to open database'") +
                    path + "': Corrupted database");
            }
        }

        unsigned major;
        unsigned minor;
        unsigned serializer;

    private:
        static std::string keyMajorVersion;
        static std::string keyMinorVersion;
        static std::string keySerializerVersion;
    };

    std::string DBVersion::keyMajorVersion("major_version");
    std::string DBVersion::keyMinorVersion("minor_version");
    std::string DBVersion::keySerializerVersion("serializer_version");

    std::unique_ptr<db::Database> openRocksDB(const boost::python::str& ppath, const boost::python::str& mode)
    {
        if (mode != boost::python::str("r") && mode != boost::python::str("a")) {
            // Existing database version will be ignored. 
            return std::unique_ptr<db::Database>(new rdbV1::RockDB_Database(ppath, mode));
        }
        // Going to check db version if exists.

        rocksdb::Options options;
        options.create_if_missing = false;
        rocksdb::Status status;

        std::vector<rocksdb::ColumnFamilyHandle*> cf;
        std::vector<rocksdb::ColumnFamilyDescriptor> cfdesc;

        cfdesc.emplace_back(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions());

        const char* path = pyutils::cstr(ppath);

        rocksdb::DB* db;
        status = rocksdb::DB::OpenForReadOnly(options, path, cfdesc, &cf, &db);

        if (status == rocksdb::Status::PathNotFound()) {
            // Create from the scratch
            return std::unique_ptr<db::Database>(new rdbV1::RockDB_Database(ppath, mode));
        }
        else if (!status.ok()) {
            throw db::Error(std::string("Failed to open database '") +
                path + "': " + status.ToString());
        }

        // Get version
        assert(cf.size() == 1);
        DBVersion ver;
        ver.readFrom(db, cf.front(), path);

        db->DestroyColumnFamilyHandle(cf.front());
        delete db;

        // Check versions here
        if (ver.major != 1) {
            throw db::Error(std::string("Failed to open database'") + path + ": version " + std::to_string(ver.major) + " is not supported");
        }
        return std::unique_ptr<db::Database>(new rdbV1::RockDB_Database(ppath, mode));
    }

    void registerRocksDBEngine()
    {
        db::Database::registerEngine(boost::python::str("ROCKDBCF"), &openRocksDB);
    }

} // namespace rdb

namespace rdbV1 {

    const unsigned RockDB_Database::m_currentMinorVersion = 1;
    const unsigned RockDB_Database::m_currentSerializerVersion = 1;

    class RockDB_Database::MetaComparator
        : public rocksdb::Comparator
    {
    public:
        virtual ~MetaComparator() = default;

        virtual int Compare(const rocksdb::Slice& a, const rocksdb::Slice& b) const override
        {
            unsigned char* dataA = (unsigned char*)a.data();
            size_t szA = a.size();
            assert(szA >= 2);
            unsigned char* dataB = (unsigned char*)b.data();
            size_t szB = b.size();
            assert(szB >= 2);
            // First compare metadata types
            if (*dataA < *dataB) {
                return -1;
            }
            if (*dataA > *dataB) {
                return 1;
            }
            if (szA < szB) {
                return -1;
            }
            if (szA > szB) {
                return 1;
            }
            return memcmp(++dataA, ++dataB, --szA);
        }

        virtual bool Equal(const rocksdb::Slice& a, const rocksdb::Slice& b) const override
        {
            return Compare(a, b) == 0;
        }

        virtual const char* Name() const override {
            return "Metadata Comparator";
        }

        virtual void FindShortestSeparator(std::string* start,
            const rocksdb::Slice& limit) const override
        {  }

        virtual void FindShortSuccessor(std::string* key) const override
        {  }

        virtual const Comparator* GetRootComparator() const override { return this; }
    };

    RockDB_Database::RockDB_Database(const boost::python::str& ppath, const boost::python::str& mode)
        : m_db(nullptr)
        , m_runs()
        , m_defaultCF(nullptr)
        , m_metaComparator(new MetaComparator())
        , m_minorVersion(m_currentMinorVersion)
        , m_serializerVersion(m_currentSerializerVersion)
        , m_serializer(nullptr)
    {
        openDatabase(ppath, mode);
    }

    RockDB_Database::~RockDB_Database()
    {
        closeDatabase();
        delete m_metaComparator;
        m_metaComparator = nullptr;
    }

    void RockDB_Database::openDatabase(const boost::python::str& path, const boost::python::str& mode)
    {
        if (m_db != nullptr) {
            throw db::Error(std::string("Database must be closed before opening new one"));
        }
        if (mode == boost::python::str("r")) {
            openReadOnly(path);
        }
        else if (mode == boost::python::str("a")) {
            openWriteAppend(path);
        }
        else if (mode == boost::python::str("w")) {
            openWriteTruncate(path);
        }
        else if (mode == boost::python::str("x")) {
            openWriteNew(path);
        }
        else {
            throw db::Error(std::string("ValueError: invalid mode: '") + pyutils::cstr(mode) + "'");
        }
    }

    void RockDB_Database::openReadOnly(const boost::python::str& path)
    {

    }

    void RockDB_Database::openWriteAppend(const boost::python::str& ppath)
    {
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::Status status;

        std::vector<std::string> cfname;
        std::vector<rocksdb::ColumnFamilyDescriptor> cfdesc;

        const char* path = pyutils::cstr(ppath);
        unsigned currentMajor = 1;

        status = rocksdb::DB::ListColumnFamilies(options, path, &cfname);

        if (status == rocksdb::Status::PathNotFound()) {
            // Create from the scratch
            status = rocksdb::DB::Open(options,
                path, &m_db);
            if (!status.ok()) {
                throw db::Error(std::string("Failed to open database '") +
                    path + "': " + status.ToString());
            }
            m_defaultCF = m_db->DefaultColumnFamily();
            m_minorVersion = m_currentMinorVersion;
            m_serializerVersion = m_currentSerializerVersion;
            rdb::DBVersion version(currentMajor, m_minorVersion, m_serializerVersion);
            version.writeTo(m_db, m_defaultCF);
            m_cf.push_back(m_defaultCF);
            m_isNew = true;
        }
        else if (!status.ok()) {
            throw db::Error(std::string("Failed to open database '") +
                path + "': " + status.ToString());
        }
        else {
            rocksdb::ColumnFamilyOptions metaOptions;
            metaOptions.comparator = m_metaComparator;
            for (const auto& name : cfname) {
                if (parsRunName(name).second == CFNameType::META) {
                    cfdesc.emplace_back(name, metaOptions);
                }
                else {
                    cfdesc.emplace_back(name, rocksdb::ColumnFamilyOptions());
                }
            }
            status = rocksdb::DB::Open(options,
                path, cfdesc, &m_cf, &m_db);
            if (!status.ok()) {
                throw db::Error(std::string("Failed to open database '") +
                    path + "': " + status.ToString());
            }
            initRuns(cfname.begin(), cfname.end(), m_cf.begin(), m_cf.end());
            assert(m_defaultCF != nullptr);
            rdb::DBVersion version;
            version.readFrom(m_db, m_defaultCF, path);
            assert(version.major == currentMajor);
            m_minorVersion = version.minor;
            m_serializerVersion = version.serializer;
            m_isNew = false;
        }

        m_serializer = db::Serializer::create(m_serializerVersion).release();
        if (m_serializer == nullptr) {
            throw db::Error(std::string("Failed to open database'") + path + ": serializer version " + std::to_string(m_serializerVersion) + " is not supported");
        }
    }

    void RockDB_Database::openWriteTruncate(const boost::python::str& path)
    {

    }

    void RockDB_Database::openWriteNew(const boost::python::str& path)
    {

    }

    template <typename NameI, typename HandleI>
    void RockDB_Database::initRuns(NameI firstName, NameI lastName, HandleI firstHandle, HandleI lastHandle)
    {
        rocksdb::ColumnFamilyHandle* def = nullptr;
        NameI nameIt = firstName;
        HandleI handleIt = firstHandle;
        for (; (nameIt != lastName) && (handleIt != lastHandle); ++nameIt, ++handleIt) {
            if ((*handleIt)->GetID() == m_db->DefaultColumnFamily()->GetID()) {
                assert(def == nullptr);
                // Its important to not directly assign to m_defaultCF. See cycle below that destroys 
                // all CFs if unexpected column found. If m_defaultCF will be assigned here close() will double delete default column
                def = *handleIt;
                continue;
            }
            auto runName = parsRunName(*nameIt);
            if (runName.second == CFNameType::UNKNOWN) {
                for (; firstHandle != lastHandle; ++firstHandle) {
                    auto status = m_db->DestroyColumnFamilyHandle(*firstHandle);
                    assert(status.ok());
                    //
                }
                m_runs.clear();
                closeDatabase();
                throw db::Error(std::string("Failed to open database: Database corrupted"));
            }
            auto inserted = m_runs.emplace(runName.first, RockDB_Run());
            if (runName.second == CFNameType::META) {
                inserted.first->second.meta = *handleIt;
            }
            else {
                assert(runName.second == CFNameType::DATA);
                inserted.first->second.data = *handleIt;
            }
        }
        assert(nameIt == lastName);
        assert(handleIt == lastHandle);

        assert(def != nullptr);
        m_defaultCF = def;

        for (const auto& run : m_runs) {
            if (run.second.data == nullptr || run.second.meta == nullptr) {
                throw db::Error(std::string("Failed to open database: Database corrupted"));
            }
        }
    }

    std::pair<std::string, RockDB_Database::CFNameType> RockDB_Database::parsRunName(const std::string& name)
    {
        auto pos = name.find_last_of('.');
        if (pos == std::string::npos) {
            return std::make_pair(name, CFNameType::UNKNOWN);
        }
        std::string resName = name.substr(0, pos);
        if (name.substr(pos + 1) == "meta") {
            return std::make_pair(resName, CFNameType::META);
        }
        if (name.substr(pos + 1) == "data") {
            return std::make_pair(resName, CFNameType::DATA);
        }
        return std::make_pair(resName, CFNameType::UNKNOWN);
    }

    std::string RockDB_Database::metaCFName(const boost::python::str& runName)
    {
        return pyutils::cstr(runName) + std::string(".meta");
    }

    std::string RockDB_Database::dataCFName(const boost::python::str& runName)
    {
        return pyutils::cstr(runName) + std::string(".data");
    }

    void RockDB_Database::closeDatabase()
    {
        if (m_db != nullptr) {
            rocksdb::Status status;

            if (m_isNew) {
                // Only destroy newely created cfs
                for (size_t idx = 1; idx < m_cf.size(); ++idx) {
                    status = m_db->DestroyColumnFamilyHandle(m_cf[idx]);
                    assert(status.ok());
                }
            } else {
                // Default must not be dropped
                for (size_t idx = 1; idx < m_cf.size(); ++idx) {
                    status = m_db->DropColumnFamily(m_cf[idx]);
                    assert(status.ok());
                }

                for (size_t idx = 0; idx < m_cf.size(); ++idx) {
                    status = m_db->DestroyColumnFamilyHandle(m_cf[idx]);
                    assert(status.ok());
                }
            }

            m_runs.clear();
            m_cf.clear();

            delete m_db;
            m_db = nullptr;
        }
    }

    std::unique_ptr<db::RunWriter> RockDB_Database::addRunWriter(const boost::python::str& hash,
        const boost::python::dict& runMetrics,
        const boost::python::dict& runFields,
        const boost::python::dict& runParameters)
    {
        if (m_db == nullptr) {
            throw db::Error(std::string("Attempt to create run writer on closed database"));
        }
        auto pos = m_runs.emplace(hash, RockDB_Run());
        if (pos.second) {
            // create new run
            rocksdb::Status status;
            rocksdb::ColumnFamilyOptions metaOptions;
            metaOptions.comparator = m_metaComparator;
            status = m_db->CreateColumnFamily(
                rocksdb::ColumnFamilyOptions(),
                dataCFName(hash), &pos.first->second.data);
            if (!status.ok()) {
                throw db::Error(std::string("Error while creating run: '") +
                    pyutils::cstr(hash) + "': " + status.ToString());
            }

            m_cf.push_back(pos.first->second.data);

            status = m_db->CreateColumnFamily(
                metaOptions,
                metaCFName(hash), &pos.first->second.meta);

            if (!status.ok()) {
                throw db::Error(std::string("Error while creating run: '") +
                    pyutils::cstr(hash) + "': " + status.ToString());
            }

            m_cf.push_back(pos.first->second.meta);
        }

        std::unique_ptr<db::RunWriter> res(new RockDB_RunWriter(&(pos.first->second), this,
            runMetrics, runFields, runParameters));
        return res;
    }

    void RockDB_Database::close()
    {
        closeDatabase();
    }

    enum class MetaType
    {
        Metric = 0,
        Field = 2,
        Param = 4,
        Internal = 6
    };

    RockDB_RunWriter::RockDB_RunWriter(RockDB_Run* run,
        RockDB_Database* db,
        const boost::python::dict& runMetrics,
        const boost::python::dict& runFields,
        const boost::python::dict& runParameters)
        : m_run(run)
        , m_database(db)
    {
        update(runMetrics, runFields, runParameters);
    }

    rocksdb::Slice slice(const db::Buffer& buff)
    {
        return rocksdb::Slice((const char*)buff.ptr(), buff.size());
    }

    void RockDB_RunWriter::setMeta(unsigned char type, const boost::python::object& key, const boost::python::object& val)
    {
        db::Buffer keyBuff, valBuff;
        keyBuff.putByte(type);
        m_database->serializer()->serializeWType(keyBuff, key);
        m_database->serializer()->serializeWType(valBuff, val);
        rocksdb::Status status;
        status = m_database->db()->Put(rocksdb::WriteOptions(), m_run->meta, slice(keyBuff), slice(valBuff));
        assert(status.ok());
    }

    void RockDB_RunWriter::close()
    {
        m_run = nullptr;
        m_database = nullptr;
    }

    void RockDB_RunWriter::update(const boost::python::dict& runMetrics,
        const boost::python::dict& runFields,
        const boost::python::dict& runParameters)
    {
        if (m_run == nullptr) {
            throw db::Error(std::string("Attempt to access closed handle"));
        }
        pyutils::iterDict(runMetrics, [this](auto obj) { this->setMeta((unsigned char)MetaType::Metric, obj[0], obj[1]); });
        pyutils::iterDict(runFields, [this](auto obj) { this->setMeta((unsigned char)MetaType::Field, obj[0], obj[1]); });
        pyutils::iterDict(runParameters, [this](auto obj) { this->setMeta((unsigned char)MetaType::Param, obj[0], obj[1]); });
    }

    void RockDB_RunWriter::dumpMeta()
    {
        return;
        rocksdb::Iterator* it = m_database->db()->NewIterator(rocksdb::ReadOptions(), m_run->meta);
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            rocksdb::Slice keySlice = it->key();
            unsigned char* keyBuff = (unsigned char*)keySlice.data();
            size_t keySize = keySlice.size();

            rocksdb::Slice dataSlice = it->value();
            unsigned char* dataBuff = (unsigned char*)dataSlice.data();
            size_t dataSize = dataSlice.size();

            assert(keySize >= 2);
            assert(dataSize >= 1);

            switch ((MetaType)*keyBuff) {
            case MetaType::Param:
                std::cerr << "param.";
                break;
            case MetaType::Internal:
                std::cerr << "internal.";
                break;
            case MetaType::Metric:
                std::cerr << "metric.";
                break;
            case MetaType::Field:
                std::cerr << "field.";
                break;
            }

            ++keyBuff;
            --keySize;

            //std::cout << it->key().ToString() << ": " << it->value().ToString() << std::endl;
        }
        assert(it->status().ok()); // Check for any errors found during the scan
        delete it;
    }

} // namespace dbimpl

/*
std::string dbPath = "C:\\Users\\hunanyan\\testdb";

int iterationTest();

int main111()
{
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status;

    std::vector<std::string> cfname;
    std::vector<rocksdb::ColumnFamilyHandle*> cf;

    DestroyDB(dbPath, options);


    status = rocksdb::DB::Open(options, dbPath, &db);
    assert(status.ok());
    {
        ScopeTimer timer("Creating " + std::to_string(runCount) + " column families (a.k.a runs)");
        for (unsigned int idx = 0; idx < runCount; ++idx) {
            cf.emplace_back();
            status = db->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), "new_cf" + std::to_string(idx), &cf.back());
        }
    }

    //Insertion begin
    unsigned step = 0;
    unsigned long long bytesStored = 0;
    for (auto handle : cf) {
        ScopeTimer timer("Inserting " + std::to_string(rowCount) + " steps " + std::to_string(rowSize) + "bytes in each ");
        for (unsigned int idx = 0; idx < rowCount; ++idx) {
            static std::vector<char> buff(rowSize, 0);

            rocksdb::Slice key((char*)&idx, sizeof(idx));
            rocksdb::Slice value(&buff[0], buff.size());

            bytesStored += buff.size();
            status = db->Put(rocksdb::WriteOptions(), handle, key, value);
            assert(s.ok());
        }
        std::cout << "Step " << ++step << ": Stored = " << (double(bytesStored / 1024) / (1024 * 1024)) << "Gb ";
    }
    //Insertion end

    for (auto handle : cf) {
        status = db->DestroyColumnFamilyHandle(handle);
        assert(status.ok());
    }

    delete db;
    std::cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
    iterationTest();
}

int iterationTest()
{
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = false;
    rocksdb::Status status;

    std::vector<std::string> cfname;
    std::vector<rocksdb::ColumnFamilyHandle*> cf;

    rocksdb::DB::ListColumnFamilies(options, dbPath, &cfname);

    assert(cfname.size() == runCount);
    std::vector<rocksdb::ColumnFamilyDescriptor> cfdesc;
    for (const auto& name : cfname) {
        cfdesc.emplace_back(name, rocksdb::ColumnFamilyOptions());
    }
    status = rocksdb::DB::Open(options, dbPath, cfdesc, &cf, &db);
    assert(status.ok());
    assert(cf.size() == runCount);

    //Insertion begin
    unsigned step = 0;
    unsigned long long bytesIterated = 0;
    for (auto handle : cf) {
        if (handle->GetID() == db->DefaultColumnFamily()->GetID()) {
            continue;
        }
        ScopeTimer timer("Iterating " + std::to_string(rowCount) + " steps " + std::to_string(rowSize) + "bytes in each ");
        rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions(), handle);
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            bytesIterated += it->value().size();
        }
        assert(it->status().ok()); // Check for any errors found during the scan
        delete it;
        std::cout << "Iteration " << ++step << ": Read = " << (double(bytesIterated / 1024) / (1024 * 1024)) << "Gb ";
    }
    //Insertion end

    for (auto handle : cf) {
        status = db->DestroyColumnFamilyHandle(handle);
        assert(status.ok());
    }

    delete db;

}*/