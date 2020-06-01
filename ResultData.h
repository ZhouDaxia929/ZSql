#ifndef ZSQL_RESULTDATA_H
#define ZSQL_RESULTDATA_H

#include <cstring>
#include <cstdio>
#include "D:/MySoftware/MySQL/mysql-8.0.15-winx64/include/binary_log_types.h"

class ResultData{
public:
    ResultData() = delete;

    ResultData(const char *data): size(strlen(data)){
        if(data){
            this->data = new char[this->size + 1];
            strcpy(this->data, data);
        }
    }

    ResultData(const int data): type(MYSQL_TYPE_LONG), size(sizeof(int)){
        this->size = sizeof(int);
        sprintf(this->data, "%d", data);
    }

    ResultData(size_t size, int type, char *data);

    ResultData(const ResultData &resultData){
        this->size = resultData.size;
        this->type = resultData.type;
        if(resultData.type != MYSQL_TYPE_LONG){
            this->data = new char[this->size + 1];
            strcpy(this->data, resultData.data);
        }
        else{
            this->data = resultData.data;
        }
    }

    ResultData &operator=(const ResultData &resultData){
        if(this == &resultData){
            return *this;
        }
        if(data != nullptr){
            delete [] data;
        }
        this->size = resultData.size;
        this->type = resultData.type;
        if(resultData.type != MYSQL_TYPE_LONG){
            this->data = new char[this->size + 1];
            strcpy(this->data, resultData.data);
        }
        else{
            this->data = resultData.data;
        }
        return *this;
    }

    void setType(int type){
        this->type = type;
    }

    const char *getData() const{
        return this->data;
    }

    const size_t getSize() const{
        return this->size;
    }

    const size_t getType() const{
        return this->type;
    }

    virtual ~ResultData();

    bool loadFile(const char *filename);
    bool saveFile(const char *filename);

private:
    size_t size = 0;
    //默认字段类型为字符串类型
    int type = MYSQL_TYPE_STRING;
    char *data = nullptr;
};

#endif //ZSQL_RESULTDATA_H
