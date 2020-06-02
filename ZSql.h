#ifndef ZSQL_ZSQL_H
#define ZSQL_ZSQL_H

#include "mysql.h"
#include "ResultData.h"
#include "connectionPool.h"

#include <vector>
#include <map>
#include <string>

namespace ZSql{
    class ZSql{
    public:
        virtual ~ZSql();

        //初始化操作
        bool init();

        //得到一个连接
        shared_ptr<Connection> getConnection();

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
        //发起一次查询请求，但是不处理结果，只是返回是否操作成功
        bool query(const string &sql);

    protected:
        MYSQL *mysql = nullptr;
        MYSQL_RES *result = nullptr;

    private:
        ConnectionPool *p;
    };
}

#endif //ZSQL_ZSQL_H
