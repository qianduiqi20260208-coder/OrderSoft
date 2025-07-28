#pragma once
#include <vector>
#include <string>

class IModelService
{
public:
    virtual std::vector<std::string> getModel(){return {};};
    virtual std::vector<std::string> getModelVersionByModel(std::string modelName){return {};};
    virtual ~IModelService() =0;
};

inline IModelService::~IModelService() = default;