#include <iostream>
#include "DbConnection.h"

DbConnection::DbConnection(DbInfo const &info) : _info(info), _conn(NULL)
{
}

bool DbConnection::hasError() const
{
    if (*mysql_error(_conn))
        return true;
    return false;
}

std::string const& DbConnection::getLastError() const
{
    static std::string str = "";
    str = mysql_error(_conn);
    return str;
}

bool DbConnection::open()
{
    mysql_library_init(0, NULL, NULL);
    mysql_thread_init();

    _conn = mysql_init(NULL);

    if (_conn == NULL)
    {
        std::cerr << "Fail mysql_init" << std::endl;
        return false;
    }

    mysql_real_connect(_conn, _info.host.c_str(), _info.user.c_str(), _info.password.c_str(), _info.database.c_str(), _info.port, NULL, 0);

    if (hasError())
    {
        close();
        return false;
    }
    return true;
}

bool DbConnection::execute(const char *sql)
{
    mysql_query(_conn, sql);

    return !hasError();
}

DbResultPtr DbConnection::query(const char *sql)
{
    if (!execute(sql))
        return DbResultPtr(new DbResult(NULL, NULL, 0, 0));

    MYSQL_RES *result = mysql_store_result(_conn);
    if (hasError())
        return DbResultPtr(new DbResult(NULL, NULL, 0, 0));

    int nbFields = mysql_num_fields(result);
    int nbRows = mysql_affected_rows(_conn);
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    return DbResultPtr(new DbResult(result, fields, nbFields, nbRows));
}

void DbConnection::close()
{
    if (_conn)
    {
        mysql_close(_conn);
        _conn = NULL;
    }
    mysql_library_end();
    mysql_thread_end();
}
