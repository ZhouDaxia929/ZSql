#include "ZSql.h"
#include <iostream>
#include <assert.h>

using std::cout;
using std::cerr;
using std::endl;

namespace ZSql{
    bool ZSql::init() {
        p = ConnectionPool::getConnnectionPool();
    }

    ZSql::~ZSql(){
        if(this->result != nullptr){
            mysql_free_result(this->result);
            this->result = nullptr;
        }

        if(this->mysql != nullptr){
            this->mysql = nullptr;
            mysql_close(this->mysql);
        }
    }

    shared_ptr<Connection> ZSql::getConnection() {
        return p->getConnection();
    }

    bool ZSql::query(const string &sql) {
        p->getConnection()->query(sql);
    }


    bool ZSql::insert(const std::map<std::string, ResultData> &resultData,
                      const std::string &tableName) {
        return p->getConnection()->insert(resultData, tableName);
    }

    bool ZSql::insertBin(const std::map<std::string, ResultData> &resultData,
                         const std::string &tableName) {
        return p->getConnection()->insert(resultData, tableName);
    }



    int ZSql::update(const std::map<std::string, ResultData> &resultData,
                     const std::string &tableName,
                     const std::string &where) {
        return p->getConnection()->update(resultData, tableName, where);
    }

    int ZSql::updateBin(const std::map<std::string, ResultData> &resultData,
                        const std::string &tableName,
                        const std::string &where) {
        return p->getConnection()->updateBin(resultData, tableName, where);
    }

    bool ZSql::startTransaction() {
        return this->query("set autocommit = 0");
    }

    bool ZSql::stopTransaction() {
        return this->query("set autocommit = 1");
    }

    bool ZSql::commit() {
        return this->query("commit");
    }

    bool ZSql::rollBack() {
        return this->query("rollback ");
    }

    std::vector<std::vector<ResultData>> ZSql::getResults(const std::string &sql) {
        return p->getConnection()->getResults(sql);
    }
}

