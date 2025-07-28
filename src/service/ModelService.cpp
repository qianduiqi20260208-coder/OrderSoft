#include "ModelService.h"

ModelService::ModelService(std::shared_ptr<IModelDAO> sp):modelDAO_(sp)
{

}

std::vector<std::string> ModelService::getModel()
{
    return modelDAO_->getModel();

}

std::vector<std::string> ModelService::getModelVersionByModel(std::string modelName)
{
    return modelDAO_->getModelVersionByModel(modelName);
}
