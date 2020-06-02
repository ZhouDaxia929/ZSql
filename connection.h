#ifndef ZSQL_CONNECTION_H
#define ZSQL_CONNECTION_H

#include "public.h"
#include "ResultData.h"
#include <mysql.h>
#include <ctime>
#include <vector>
#include <map>
#include <string>


using namespace std;

//数据库操作类
class Connection{
public:
    //初始化数据库连接
    Connection();
    //释放数据库连接资源
    ~Connection();

    //连接数据库
    bool connect(string ip,
                 unsigned short port,
                 string user,
                 string password,
                 string dbname);

    //更新操作：insert、delete、update
    //bool update(string sql);

    bool query(const string &sql);

    //插入操作
    bool insert(const map<string, ResultData> &resultData,
                const string &tableName);

    //整体插入，加快操作的速度
    bool insertBin(const map<string, ResultData> &resultData,
                   const string &tableName);

    //更新操作
    int update(const map<string, ResultData> &resultData,
               const string &tableName,
               const string &where);

    //整体更新，加快操作的速度
    int updateBin(const map<string, ResultData> &resultData,
                  const string &tableName,
                  const string &where);

    //获取全部结果
    vector<vector<ResultData>> getResults(const string &sql);

    //刷新连接的起始空闲时间
    void refreshAlivetime();

    //返回该连接的存活时间
    clock_t getAliveTime();

    //设置连接超时
    bool setConnectTimeout(int seconds);

    //设置是否超时后重新尝试连接
    bool setReconnect(bool r);



private:
    bool option(mysql_option option, const void *arg);

    //释放当前结果集
    bool freeResult();

    bool storeResult();

    bool useResult();

    //获取一行结果
    vector<ResultData> getOneRow();

    //生成一条查询SQL语句
    string getInsertSql(const map<string, ResultData> &resultData,
                             const string &tableName);

    //生成一条更新SQL语句
    string getUpdateSql(const map<string, ResultData> &resultData,
                             const string &tableName,
                             const string &where);

    MYSQL *_conn = nullptr;         //表示和MySQL的一个连接
    MYSQL_RES *result = nullptr;    //表示一次查询的结果集
    clock_t _alivetime; //该连接被激活时的时间戳
};

#endif //ZSQL_CONNECTION_H
