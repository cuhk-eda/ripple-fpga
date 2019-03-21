#pragma once

#include "db/db.h"
using namespace db;

class CLBBase {
public:
    int id;

    CLBBase(int i) : id(i) {}
    CLBBase() {
        static int gid = 0;
        id = gid++;
    }
    virtual ~CLBBase(){};
    virtual bool AddInsts(const Group& group) = 0;
    virtual bool IsEmpty() = 0;
    virtual void GetResult(Group& group) = 0;
    virtual void GetFinalResult(Group& group) = 0;
    virtual void Print() const = 0;
};