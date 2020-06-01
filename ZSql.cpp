#include "ZSql.h"
#include <iostream>
#include <assert.h>

using std::cout;
using std::cerr;

const char endl = '\n';

namespace ZSql{
    bool ZSql::init() {
        this->mysql = mysql_init(nullptr);
        if(!mysql){
            cerr << "mysql_init error: " << mysql_error(this->mysql) << endl;
            return false;
        }
        return true;
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

    bool ZSql::connect(const char *host,
                       const char *user,
                       const char *pswd,
                       const char *db,
                       unsigned int port,
                       unsigned int flag) {
        if(!this->mysql){
            bool f = this->init();
            if(!f){
                cerr << "connect init error" << endl;
                return f;
            }
        }
        else{
            if(!mysql_real_connect(this->mysql, host, user, pswd, db, port, 0, flag)){
                cerr << "real connect error" << endl;
                return false;
            }
            else{
                return true;
            }
        }
    }

    bool ZSql::option(mysql_option option, const void *arg) {
        int res = mysql_options(this->mysql, option, arg);
        if(res != 0){
            cerr << "mysql option error: " << mysql_error(this->mysql) << endl;
            return false;
        }
        return true;
    }

    bool ZSql::setConnectTimeout(int seconds) {
        return this->option(MYSQL_OPT_CONNECT_TIMEOUT, &seconds);
    }

    bool ZSql::setReconnect(bool r) {
        return this->option(MYSQL_OPT_RECONNECT, &r);
    }

    bool ZSql::query(const char *sql, size_t sqlLen) {
        if(!sql){
            cerr << "sql is null" << endl;
            return false;
        }
        if(!sqlLen){
            sqlLen = strlen(sql);
            assert(sqlLen > 0);
        }

        int res = mysql_real_query(this->mysql, sql, sqlLen);
        if(res){
            cerr << "mysql query error: " << mysql_error(this->mysql) << endl;
            return false;
        }
        return true;
    }

    bool ZSql::freeResult() {
        if(result != nullptr){
            mysql_free_result(this->result);
            this->result = nullptr;
            return true;
        }
        return false;
    }

    bool ZSql::storeResult() {
        if(!this->mysql){
            cerr << "store result failed: mysql is null" << endl;
            return false;
        }
        this->freeResult();
        this->result = mysql_store_result(this->mysql);
        if(!result){
            cerr << "store result failed: mysql don't have result" << endl;
            return false;
        }
        return true;
    }

    bool ZSql::useResult() {
        if(!this->mysql){
            cerr << "use result failed: mysql is null" << endl;
            return false;
        }
        this->freeResult();
        this->result = mysql_use_result(this->mysql);
        if(!result){
            cerr << "use result failed: " << mysql_error(this->mysql) << endl;
            return false;
        }
        return true;
    }

    std::vector<ResultData> ZSql::getOneRow() {
        std::vector<ResultData> res;
        if(!this->result){
            return res;
        }
        else{
            MYSQL_ROW row = mysql_fetch_row(result);
            if(!row){
                return res;
            }
            else{
                unsigned long *lengths = mysql_fetch_lengths(result);
                int num = mysql_num_fields(result);
                for(int i = 0; i < num; ++i){
                    auto field = mysql_fetch_field_direct(this->result, i);
                    ResultData resultData(lengths[i], field->type, row[i]);
                    resultData.setType(field->type);
                    res.push_back(resultData);
                }
            }
        }
        return res;
    }

    std::string ZSql::getInsertSql(const std::map<std::string, ResultData> &resultData,
                                   const std::string &tableName) {
        using std::string;
        string re;
        if(resultData.empty() || tableName.empty()){
            return re;
        }

        re = "insert into `";
        re += tableName;
        re += '`';

        string key;
        string value;
        for(const auto &e: resultData){
            key += "`";
            key += e.first;
            key += "`";
            key += ",";

            value += "`";
            value += e.second.getData();
            value += "`";
            value += ",";
        }

        key[key.size() - 1] = ' ';
        value[value.size() - 1] = ' ';

        re += "(";
        re += key;
        re += ")values(";
        re += value;
        re += ")";

        return re;
    }

    bool ZSql::insert(const std::map<std::string, ResultData> &resultData,
                      const std::string &tableName) {
        if(!this->mysql || resultData.empty() || tableName.empty()){
            return false;
        }
        else{
            std::string re;
            if((re = this->getInsertSql(resultData, tableName)).empty()){
                return false;
            }

            bool f = this->query(re.data());
            if(!f){
                return false;
            }
            else{
                size_t num = mysql_affected_rows(this->mysql);
                return num > 0;
            }
        }
    }

    bool ZSql::insertBin(const std::map<std::string, ResultData> &resultData,
                         const std::string &tableName) {
        using std::string;
        if(!this->mysql || resultData.empty() || tableName.empty()){
            return false;
        }

        string re;
        re = "insert into `";
        re += tableName;
        re += "`";
        MYSQL_BIND mysqlBind[256];

        string key;
        string value;

        int i = 0;
        for(const auto &e: resultData){
            key += "`";
            key += e.first;
            key += "`";
            key += ",";

            value += "?.";

            mysqlBind[i].buffer = (char *)e.second.getData();
            mysqlBind[i].buffer_length = static_cast<unsigned long>(e.second.getSize());
            mysqlBind[i].buffer_type = static_cast<enum_field_types>(e.second.getType());
            ++i;
        }

        key[key.size() - 1] = ' ';
        value[value.size() - 1] = ' ';

        re += "(";
        re += key;
        re += ")values(";
        re += value;
        re += ")";

        MYSQL_STMT *stmt = mysql_stmt_init(this->mysql);
        if(!stmt){
            cerr << "mysql_stmt_init error: " << mysql_error(this->mysql) << endl;
            return false;
        }
        if(mysql_stmt_prepare(stmt, re.data(), static_cast<unsigned long>(re.size()))){
            mysql_stmt_close(stmt);
            cerr << "mysql_stmt_prepare error: " << mysql_error(this->mysql) << endl;
            return false;
        }
        if(mysql_stmt_bind_param(stmt, mysqlBind) != 0){
            mysql_stmt_close(stmt);
            cerr << "mysql_stmt_bind_param error: " << mysql_error(this->mysql) << endl;
            return false;
        }
        if(mysql_stmt_execute(stmt) != 0){
            mysql_stmt_close(stmt);
            cerr << "mysql_stmt_execute error: " << mysql_error(this->mysql) << endl;
            return false;
        }

        mysql_stmt_close(stmt);
        return true;
    }

    std::string ZSql::getUpdateSql(const std::map<std::string, ResultData> &resultData,
                                   const std::string &tableName,
                                   const std::string &where) {
        using std::string;
        string sql;
        if(!this->mysql || tableName.empty() || resultData.empty()){
            return sql;
        }

        sql = "update `";
        sql += tableName;
        sql += "` set";
        for(const auto &e: resultData){
            sql += "`";
            sql += e.first;
            sql += "`='";
            if(e.second.getType() == MYSQL_TYPE_LONG){
                sql += std::to_string(*((int *)e.second.getData()));
            }
            else{
                sql += e.second.getData();
            }
            sql += "', ";
        }
        sql[sql.size() - 1] = ' ';
        sql += " where ";
        sql += where;

        return sql;
    }

    int ZSql::update(const std::map<std::string, ResultData> &resultData,
                     const std::string &tableName,
                     const std::string &where) {
        if(!this->mysql || tableName.empty() || resultData.empty()){
            return -1;
        }
        std::string sql = getUpdateSql(resultData, tableName, where);
        if(sql.empty()){
            return -1;
        }
        if(!query(sql.data())){
            return -1;
        }
        return (int)mysql_affected_rows(this->mysql);
    }

    int ZSql::updateBin(const std::map<std::string, ResultData> &resultData,
                        const std::string &tableName,
                        const std::string &where) {
        using std::string;
        string sql;
        if (!this->mysql || tableName.empty() || resultData.empty()) {
            return -1;
        }

        sql = "update `";
        sql += tableName;
        sql += "` set ";
        MYSQL_BIND mysqlBind[256];

        int i = 0;
        for (const auto &e:resultData) {
            sql += "`";
            sql += e.first;
            sql += "`= ?";
            sql += ",";

            mysqlBind[i].buffer = (char *) e.second.getData();
            mysqlBind[i].buffer_length = static_cast<unsigned long>(e.second.getSize());
            mysqlBind[i].buffer_type = static_cast<enum_field_types>(e.second.getType());

            ++i;
        }

        sql[sql.size() - 1] = ' ';
        sql += " where ";
        sql += where;

        MYSQL_STMT *stmt = mysql_stmt_init(this->mysql);
        if (!stmt) {
            cerr << "mysql_stmt_init error: " << mysql_error(this->mysql) << endl;
            return false;
        }
        if (mysql_stmt_prepare(stmt, sql.data(), static_cast<unsigned long>(sql.size()))) {
            mysql_stmt_close(stmt);
            cerr << "mysql_stmt_prepare error: " << mysql_error(this->mysql) << endl;
            return false;
        }
        if (mysql_stmt_bind_param(stmt, mysqlBind) != 0) {
            mysql_stmt_close(stmt);
            cerr << "mysql_stmt_bind_param error: " << mysql_error(this->mysql) << endl;
            return false;
        }
        if (mysql_stmt_execute(stmt) != 0) {
            mysql_stmt_close(stmt);
            cerr << "mysql_stmt_execute error: " << mysql_error(this->mysql) << endl;
            return false;
        }

        int r = (int)mysql_affected_rows(this->mysql);
        mysql_stmt_close(stmt);
        return r;
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
        auto res = std::vector<std::vector<ResultData>>();
        if(!this->query(sql.data())){
            return res;
        }
        if(!this->storeResult()){
            return res;
        }
        while(true){
            auto r = this->getOneRow();
            if(r.empty()){
                break;
            }
            else{
                res.push_back(r);
            }
        }
        return res;
    }
}

