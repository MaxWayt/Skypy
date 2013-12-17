#include <iostream>
#include "DbConnection.h"
#include "DbWorkerPool.h"
#include "Skypy.h"
#include "Thread.hpp"
#include "Utils.hpp"

#define SERVER_SLEEP_TIME 50

class SkypyRunnable
{
public:
    void run()
    {
        sSkypy->onStartup();

        uint32 prevTime = Utils::getMSTime();
        uint32 currTime = 0;
        uint32 prevSleep = 0;
        while (!sSkypy->getStopEvent())
        {
            currTime = Utils::getMSTime();
            uint32 diff = Utils::getMSTimeDiff(prevTime, currTime);

            sSkypy->update(diff);
            prevTime = currTime;

            if (diff <= SERVER_SLEEP_TIME + prevSleep)
            {
                prevSleep = SERVER_SLEEP_TIME + prevSleep - diff;
                Thread::Sleep(prevSleep);
            }
            else
                prevSleep = 0;
        }

        sSkypy->onShutdown();
    }
};

int main(int ac, char** av)
{
    SkypyRunnable runn;
    Thread t(&runn);
    t.join();
    return 0;
}
