#pragma once
#include "Entity.h"
#include <string>

class IModelDAO
{
public:
    virtual std::vector<std::string> getModel(){return {};};
    virtual std::vector<std::string> getModelVersionByModel(std::string modelName){return {};};

    virtual ~IModelDAO() = 0;
};

inline IModelDAO::~IModelDAO() =default;