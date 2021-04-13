#include "database.h"

#include <map>

namespace db {

class Database::EngineRegister
{
public:
    EngineRegister() = default;
    ~EngineRegister() = default;
    EngineRegister(const EngineRegister&) = delete;
    EngineRegister& operator= (const EngineRegister&) = delete;

    void add(const boost::python::str& name, Database::OpenProc proc)
    {
        auto res = m_map.emplace(name, proc);
        if (!res.second) {
            throw db::Error(std::string("Creator already registered for '") + (const char*)boost::python::extract<const char*>(name) + "'");
        }
    }

    Database::OpenProc openProc(const boost::python::str& name)
    {
        auto res = m_map.find(name);
        if (res == m_map.end()) {
            throw db::Error(std::string("Wrong database class '") + (const char*)boost::python::extract<const char*>(name) + "'");
        }
        return res->second;
    }

private:
    std::map<boost::python::str, Database::OpenProc> m_map;
};

Database::EngineRegister& Database::engineRegister()
{
    static EngineRegister inst;
    return inst;
}

std::unique_ptr<Database> Database::open(const boost::python::str& engineName, const boost::python::str& path, const boost::python::str& mode)
{
    return engineRegister().openProc(engineName)(path, mode);
}

void Database::registerEngine(const boost::python::str& name, OpenProc proc)
{
    engineRegister().add(name, proc);
}

} // namespace db