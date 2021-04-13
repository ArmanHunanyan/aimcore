#ifndef DB_ERROR_H
#define DB_ERROR_H

namespace db {

class Error
    : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

} // namespace db

#endif // DB_ERROR_H