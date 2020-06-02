#ifndef ZSQL_CONNECTIONPOOL_H
#define ZSQL_CONNECTIONPOOL_H

#include "connection.h"

#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>
#include <condition_variable>
#include <functional>

class ConnectionPool{
public:
    //获取连接池对象实例
    static ConnectionPool *getConnnectionPool();

    //用户获得连接的接口
    shared_ptr<Connection> getConnection();

private:
    ConnectionPool(); //单例模式，将构造函数私有化

    //从配置文件中加载配置项
    bool loadConfigFile();

    //单独运行一个线程，用于生产连接
    void produceConnectionTask();

    //单独运行一个线程，扫描队列中超过maxidletime连接
    void scanConnectionTask();

    string _ip;             //MySQL登录IP地址
    unsigned short _port;   //MySQL端口号3306
    string _username;       //MySQL登录用户名
    string _password;       //MySQL登录密码
    string _dbname;         //连接的数据库名称

    int _initSize;          //连接池连接数的初始值
    int _maxSize;           //连接池的最大连接量
    int _maxIdleTime;       //连接池的最大空闲时间
    int _connectionTimeout; //最大超时时间
    bool _reconnect = false;//是否重新连接

    queue<Connection *> _ConnectionQueue; //存储MySQL连接的队列
    mutex _queueMutex;                    //维护连接队列的线程安全互斥锁
    atomic_int _connectionCnt;            //记录所创建的connection连接的总个数
    condition_variable cv;                //用于生产者线程和消费者线程之间的通信

};

#endif //ZSQL_CONNECTIONPOOL_H
