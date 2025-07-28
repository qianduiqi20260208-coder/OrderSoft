#pragma once
#include "IModelService.h"
#include "IModelDAO.h"
#include <memory>

class ModelService :public IModelService
{
public:
    ModelService(std::shared_ptr<IModelDAO> sp);
    std::vector<std::string> getModel() override;
    std::vector<std::string> getModelVersionByModel(std::string modelName)override;
private:
    std::shared_ptr<IModelDAO> modelDAO_;
};