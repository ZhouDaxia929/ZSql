#include "connectionPool.h"

#include <cstdio>

ConnectionPool* ConnectionPool::getConnnectionPool() {
    static ConnectionPool pool;
    return &pool;
}

ConnectionPool::ConnectionPool() {
    if(!loadConfigFile()){
        return;
    }

    for(int i = 0; i < _initSize; ++i){
        Connection *p = new Connection();
        p->setReconnect(_reconnect);
        p->setConnectTimeout(_connectionTimeout);
        p->connect(_ip, _port, _username, _password, _dbname);
        _ConnectionQueue.push(p);
        p->refreshAlivetime();
        ++_connectionCnt;
    }

    //启动一个新的线程，作为连接的生产者
    thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
    produce.detach();

    //启动一个新的线程，扫描超过maxidletime时间的空闲连接，进行队列的回收
    thread scanner(std::bind(&ConnectionPool::scanConnectionTask, this));
    scanner.detach();
}

bool ConnectionPool::loadConfigFile() {
    FILE *pf = fopen("mysql.ini", "r");
    if(pf == nullptr){
        LOG("mysql.ini file is not exist!");
        return false;
    }

    while(!feof(pf)){
        char line[1024] = {0};
        fgets(line, 1024, pf);
        string str = line;
        size_t idx = str.find('=', 0);
        if(idx == -1){ //无效的配置项
            continue;
        }
        size_t endidx = str.find('\n', idx);

        string key = str.substr(0, idx);
        string value = str.substr(idx + 1, endidx - idx - 1);

        if(key == "ip"){
            _ip = value;
        }
        else if(key == "port"){
            _port = atoi(value.c_str());
        }
        else if(key == "username"){
            _username = value;
        }
        else if(key == "password"){
            _password = value;
        }
        else if(key == "dbname"){
            _dbname = value;
        }
        else if(key == "initSize"){
            _initSize = atoi(value.c_str());
        }
        else if(key == "maxSize"){
            _maxSize = atoi(value.c_str());
        }
        else if(key == "maxIdleTime"){
            _maxIdleTime = atoi(value.c_str());
        }
        else if(key == "maxconnectionTimeout"){
            _connectionTimeout = atoi(value.c_str());
        }
        else if(key == "reconnect"){
            if(value == "true"){
                _reconnect = true;
            }
            else{
                _reconnect = false;
            }
        }
    }

    return true;
}

void ConnectionPool::produceConnectionTask() {
    for(;;){
        unique_lock<mutex> lock(_queueMutex);
        while(!_ConnectionQueue.empty()){
            cv.wait(lock); //如果队列不为空，条件变量处于阻塞状态
        }
        //如果队列为空，就得创建新的连接
        if(_connectionCnt < _maxSize){
            Connection *conn = new Connection();
            conn->connect(_ip, _port, _username, _password, _dbname);
            conn->refreshAlivetime();
            _ConnectionQueue.push(conn); //将新连接入队
            ++_connectionCnt;
        }
        cv.notify_all(); //通知消费者线程可以获取新连接了
    }
}

void ConnectionPool::scanConnectionTask() {
    for(;;){
        //通过sleep来模拟定时效果
        this_thread::sleep_for(chrono::seconds(_maxIdleTime));

        //扫描整个队列，释放空闲连接
        unique_lock<mutex> lock(_queueMutex);
        while(_connectionCnt > _initSize){
            Connection *p = _ConnectionQueue.front();
            //如果说连接空闲时间超过了最大空闲时间，就将连接出队，然后delete。
            if(p->getAliveTime() > (1000 * _maxIdleTime)){ //如果大于最大空闲时间就得出队
                _ConnectionQueue.pop();
                --_connectionCnt;
                delete p; //调用~Connection()将资源释放即可
            }
            else{
                break; //如果说队头元素未超时，则其他元素也未超时
            }
        }
    }
}

shared_ptr<Connection> ConnectionPool::getConnection() {
    unique_lock<mutex> lock(_queueMutex); //给queue加锁
    while(_ConnectionQueue.empty()){
        if(cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout))){
            if(_ConnectionQueue.empty()){
                LOG("get free connections timeout ... get connection fail!");
                return nullptr;
            }
        }
    }
    //对于shared_ptr而言，出作用域后自动析构是将连接直接销毁，而我们要达到的目的是
    //要将连接归还，所以要自定义删除器,使用lambda表达式来定义删除器
    shared_ptr<Connection> sp(_ConnectionQueue.front(),
            [&](Connection *pcon){
                unique_lock<mutex> lock(_queueMutex);
                pcon->refreshAlivetime();
                _ConnectionQueue.push(pcon);
    });

    _ConnectionQueue.pop();
    cv.notify_all(); //通知消费者线程检查一下队列是否为空;
    return sp;
}