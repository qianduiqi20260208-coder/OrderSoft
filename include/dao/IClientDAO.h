#pragma once
#include <vector>
#include "Entity.h"
#include <memory>

class IClientDAO
{
public:

    virtual ~IClientDAO() = 0; 
};

inline IClientDAO::~IClientDAO() =default;