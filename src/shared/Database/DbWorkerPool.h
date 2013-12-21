#ifndef DBWORKERPOOL_H_
# define DBWORKERPOOL_H_

#include "DbQueue.h"
#include "DbConnection.h"
#include "ThreadPool.hpp"
#include "DbWorker.h"

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <list>

class DbWorkerPool : public ThreadPool
{
public:
    DbWorkerPool();
    bool initialize(DbInfo const &info, unsigned int nbWorker);
    std::string const& getLastError() const;
    void execute(std::string const &sql);
    void execute(const char* str, ...);
    DbResultPtr query(std::string const &sql);
    DbResultPtr query(const char* str, ...);

    void waitAll();
private:
    DbQueue _queue;
    std::list<DbConnection *> _conns;
};

#endif
