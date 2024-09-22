#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
using namespace std;

class Redis
{
private:
    redisContext* _publishContext;

    redisContext* _subscribeContext;

    function<void(int, string)> _notifyMessageHandler;


public:
    Redis();
    ~Redis();

    bool connect();

    bool publish(int channel, string message);

    bool subscribe(int channel);

    bool unsubscribe(int channel);

    void observerChannelMessage();

    void initNotifyHandler(function<void(int, string)> fn);

};




#endif