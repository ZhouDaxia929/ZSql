#ifndef ZSQL_ZSQL_H
#define ZSQL_ZSQL_H

#include "D:/MySoftware/MySQL/mysql-8.0.15-winx64/include/mysql.h"

//#include "D:/MySoftware/MySQL/mysql-8.0.15-winx64/include/mysql/udf_registration_types.h"
#include "ResultData.h"

#include <vector>
#include <map>

namespace ZSql{
    class ZSql{
    public:
        virtual ~ZSql();

        //初始化操作
        bool init();

        //连接数据库
        bool connect(const char *host, const char *user, const char *pswd,
                     const char *db, unsigned int port, unsigned int flag = 0);

        //设置连接超时
        bool setConnectTimeout(int seconds);

        //设置是否超时后重新尝试连接
        bool setReconnect(bool r);


        //发起一次查询请求
        bool query(const char *sql, size_t sqlLen = 0);

        //释放当前的结果
        bool freeResult();

        //存储结果
        bool storeResult();

        //使用结果
        bool useResult();



        //获取一行结果
        std::vector<ResultData> getOneRow();



        //插入操作
        bool insert(const std::map<std::string, ResultData> &resultData,
                    const std::string &tableName);

        //整体插入，加快操作的速度
        bool insertBin(const std::map<std::string, ResultData> &resultData,
                       const std::string &tableName);



        //更新操作
        int update(const std::map<std::string, ResultData> &resultData,
                   const std::string &tableName,
                   const std::string &where);

        //整体更新，加快操作的速度
        int updateBin(const std::map<std::string, ResultData> &resultData,
                      const std::string &tableName,
                      const std::string &where);

        //开始一个事务
        bool startTransaction();

        //结束一个事务
        bool stopTransaction();

        //提交事务
        bool commit();

        //回滚操作
        bool rollBack();

        //获取全部结果
        std::vector<std::vector<ResultData>> getResults(const std::string &sql);

    private:
        bool option(mysql_option option, const void *arg);

        //生成一条查询SQL语句
        std::string getInsertSql(const std::map<std::string, ResultData> &resultData,
                                 const std::string &tableName);

        //生成一条更新SQL语句
        std::string getUpdateSql(const std::map<std::string, ResultData> &resultData,
                                 const std::string &tableName,
                                 const std::string &where);

    protected:
        MYSQL *mysql = nullptr;
        MYSQL_RES *result = nullptr;
    };
}

#endif //ZSQL_ZSQL_H
