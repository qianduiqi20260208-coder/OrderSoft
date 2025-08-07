#pragma once
#include "IModelService.h"
#include "IModelDAO.h"
#include <memory>
#include "ITicketDAO.h"


class ModelService :public IModelService
{
public:
    ModelService(std::shared_ptr<IModelDAO>,std::shared_ptr<ITicketDAO>);
    std::vector<std::string> getModel() override;
    std::vector<std::string> getModelVersionByModel(std::string modelName)override;

    std::vector<std::pair<std::vector<std::string>,std::vector<std::shared_ptr<Ticket>>>> getModelVersionWithOrdersByModelPaged(const std::string& model,int page,int pageSize) override;

private:
    std::shared_ptr<IModelDAO> modelDAO_;
    std::shared_ptr<ITicketDAO> ticketDAO_;
};