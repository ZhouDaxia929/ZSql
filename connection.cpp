#include "connection.h"

Connection::Connection() {
    _conn = mysql_init(nullptr);
}

Connection::~Connection() {
    if(_conn != nullptr){
        mysql_close(_conn);
    }
}

bool Connection::connect(string ip,
                         unsigned short port,
                         string user,
                         string password,
                         string dbname) {
    MYSQL *p = mysql_real_connect(_conn,
                                  ip.c_str(),
                                  user.c_str(),
                                  password.c_str(),
                                  dbname.c_str(),
                                  port,
                                  nullptr, 0);
    return p != nullptr;
}


void Connection::refreshAlivetime() {
    _alivetime = clock();
}

clock_t Connection::getAliveTime() {
    return clock() - _alivetime;
}

bool Connection::option(mysql_option option, const void *arg) {
    int res = mysql_options(_conn, option, arg);
    if(res != 0){
        cerr << "mysql option error: " << mysql_error(_conn) << endl;
        return false;
    }
    return true;
}

bool Connection::setConnectTimeout(int seconds) {
    return this->option(MYSQL_OPT_CONNECT_TIMEOUT, &seconds);
}

bool Connection::setReconnect(bool r) {
    return this->option(MYSQL_OPT_RECONNECT, &r);
}

bool Connection::query(const string &sql) {
    if(sql.empty()){
        cerr << "sql is null" << endl;
        return false;
    }

    int res = mysql_real_query(_conn, sql.c_str(), sql.size());
    if(res){
        cerr << "mysql query error: " << mysql_error(_conn) << endl;
        return false;
    }
    return true;
}

bool Connection::freeResult() {
    if(result != nullptr){
        mysql_free_result(this->result);
        result = nullptr;
        return true;
    }
    return false;
}

bool Connection::storeResult() {
    if(!_conn){
        cerr << "store result failed: mysql is null" << endl;
        return false;
    }
    this->freeResult();
    result = mysql_store_result(_conn);
    if(!result){
        cerr << "store result failed: mysql don't have result" << endl;
        return false;
    }
    return true;
}

bool Connection::useResult() {
    if(!_conn){
        cerr << "use result failed: mysql is null" << endl;
        return false;
    }
    this->freeResult();
    result = mysql_use_result(_conn);
    if(!result){
        cerr << "use result failed: " << mysql_error(_conn) << endl;
        return false;
    }
    return true;
}

vector<ResultData> Connection::getOneRow() {
    std::vector<ResultData> res;
    if(!_conn){
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

string Connection::getInsertSql(const map<string, ResultData> &resultData,
                                const string &tableName) {
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

string Connection::getUpdateSql(const map<string, ResultData> &resultData,
                                const string &tableName,
                                const string &where) {
    string sql;
    if(!_conn || tableName.empty() || resultData.empty()){
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

bool Connection::insert(const map<string, ResultData> &resultData, const string &tableName) {
    if(!_conn || resultData.empty() || tableName.empty()){
        return false;
    }
    else{
        string re;
        if((re = this->getInsertSql(resultData, tableName)).empty()){
            return false;
        }

        bool f = this->query(re);
        if(!f){
            return false;
        }
        else{
            size_t num = mysql_affected_rows(_conn);
            return num > 0;
        }
    }
}

bool Connection::insertBin(const map<string, ResultData> &resultData,
                           const string &tableName) {
    if(!_conn || resultData.empty() || tableName.empty()){
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

    MYSQL_STMT *stmt = mysql_stmt_init(_conn);
    if(!stmt){
        cerr << "mysql_stmt_init error: " << mysql_error(_conn) << endl;
        return false;
    }
    if(mysql_stmt_prepare(stmt, re.data(), static_cast<unsigned long>(re.size()))){
        mysql_stmt_close(stmt);
        cerr << "mysql_stmt_prepare error: " << mysql_error(_conn) << endl;
        return false;
    }
    if(mysql_stmt_bind_param(stmt, mysqlBind) != 0){
        mysql_stmt_close(stmt);
        cerr << "mysql_stmt_bind_param error: " << mysql_error(_conn) << endl;
        return false;
    }
    if(mysql_stmt_execute(stmt) != 0){
        mysql_stmt_close(stmt);
        cerr << "mysql_stmt_execute error: " << mysql_error(_conn) << endl;
        return false;
    }

    mysql_stmt_close(stmt);
    return true;
}

int Connection::update(const map<string, ResultData> &resultData,
                       const string &tableName,
                       const string &where) {
    if(!_conn || tableName.empty() || resultData.empty()){
        return -1;
    }
    string sql = getUpdateSql(resultData, tableName, where);
    if(sql.empty()){
        return -1;
    }
    if(!query(sql)){
        return -1;
    }
    return (int)mysql_affected_rows(_conn);
}

int Connection::updateBin(const map<string, ResultData> &resultData,
                          const string &tableName,
                          const string &where) {
    string sql;
    if (!_conn || tableName.empty() || resultData.empty()) {
        return -1;
    }

    sql = "update `";
    sql += tableName;
    sql += "` set ";
    MYSQL_BIND mysqlBind[256];

    int i = 0;
    for (const auto &e: resultData) {
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

    MYSQL_STMT *stmt = mysql_stmt_init(_conn);
    if (!stmt) {
        cerr << "mysql_stmt_init error: " << mysql_error(_conn) << endl;
        return -1;
    }
    if (mysql_stmt_prepare(stmt, sql.data(), static_cast<unsigned long>(sql.size()))) {
        mysql_stmt_close(stmt);
        cerr << "mysql_stmt_prepare error: " << mysql_error(_conn) << endl;
        return -1;
    }
    if (mysql_stmt_bind_param(stmt, mysqlBind) != 0) {
        mysql_stmt_close(stmt);
        cerr << "mysql_stmt_bind_param error: " << mysql_error(_conn) << endl;
        return -1;
    }
    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        cerr << "mysql_stmt_execute error: " << mysql_error(_conn) << endl;
        return -1;
    }

    int r = (int)mysql_affected_rows(_conn);
    mysql_stmt_close(stmt);
    return r;
}

vector<vector<ResultData>> Connection::getResults(const string &sql) {
    auto res = vector<vector<ResultData>>();
    if(!this->query(sql)){
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