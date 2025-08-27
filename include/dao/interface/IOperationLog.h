#pragma once
#include "Entity.h"

class IOperationLog
{
public:
    virtual bool generateOperationLog(std::string, const Log&) = 0;
    virtual ~IOperationLog() = 0;
};