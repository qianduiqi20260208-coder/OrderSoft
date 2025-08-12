#pragma once
#include "Entity.h"
#include <string>

class IModelDAO
{
public:
    virtual std::vector<std::string> getModel(){return {};};
    virtual std::vector<std::string> getModelVersionByModel(std::string modelName){return {};};

    //增加模型
    virtual bool addModelVersion(const std::string& model,const std::string& modelVersion) = 0;

    virtual std::vector<std::vector<std::string>> getModelVersionInfoByModelPaged(const std::string& model,int page,int pageSize) = 0;

    virtual int getModelVersionCount(std::string model) = 0;
    
    virtual ~IModelDAO() = 0;
};

inline IModelDAO::~IModelDAO() =default;