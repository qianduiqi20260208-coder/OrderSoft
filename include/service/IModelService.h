#pragma once
#include <vector>
#include <string>
#include <memory>
#include "Entity.h"

class IModelService
{
public:
    virtual std::vector<std::string> getModel(){return {};};
    virtual std::vector<std::string> getModelVersionByModel(std::string modelName){return {};};

    virtual std::vector<std::pair<std::vector<std::string>,std::vector<std::shared_ptr<Ticket>>>> getModelVersionWithOrdersByModelPaged(const std::string& model,int page,int pageSize) = 0;
    
    virtual ~IModelService() =0;
};

inline IModelService::~IModelService() = default;