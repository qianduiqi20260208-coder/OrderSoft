#include "ModelService.h"

ModelService::ModelService(std::shared_ptr<IModelDAO> sp1,std::shared_ptr<ITicketDAO> sp2):modelDAO_(sp1),ticketDAO_(sp2)
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

//最外面的vector存放的是所有的模型版本，pair.first存放的是模型版本信息（包括所属模型、模型版本、更新时间）,pair.second存放的是模型版本对应的所有工单
std::vector<std::pair<std::vector<std::string>, std::vector<std::shared_ptr<Ticket>>>> ModelService::getModelVersionWithOrdersByModelPaged(const std::string &model, int page, int pageSize)
{
    std::vector<std::pair<std::vector<std::string>, std::vector<std::shared_ptr<Ticket>>>> retVec;
    int offset = page * pageSize;
    int count = pageSize;
    const std::vector<std::vector<std::string>>& modelVersionVec = modelDAO_->getModelVersionInfoByModelPaged(model,offset,pageSize);
    for(const auto& vec : modelVersionVec)
    {
        std::pair<std::vector<std::string>, std::vector<std::shared_ptr<Ticket>>> tmp;
        //将模型以及模型版本信息放入结果集中
        tmp.first.push_back(model);
        tmp.first.push_back(vec[0]);
        tmp.first.push_back(vec[2]);
        //根据model_version_id来倒叙查询工单 不同类型的工单分开处理。
        std::map<std::string,std::string> ruleMap;
        ruleMap["model_version_id"] = vec[1];
        ruleMap["type"] = "问题复现";
        tmp.second = std::move(ticketDAO_->selectOrderByCondition_(ruleMap,0,INT_MAX));
        ruleMap["type"] = "交付发送";
        const auto& ret1 = ticketDAO_->selectOrderByCondition_(ruleMap,0,INT_MAX);
        for(const auto& ele:ret1)
            tmp.second.push_back(ele);
        ruleMap["type"] = "其他";
        const auto& ret2 = ticketDAO_->selectOrderByCondition_(ruleMap,0,INT_MAX);
        for(const auto& ele:ret2)
            tmp.second.push_back(ele);

        //查找所有目标版本为给定值的工单
        const auto& ret3 = findOrdersByTargetVersion(vec[0]);
        for(const auto& ele:ret3)
            tmp.second.push_back(ele);

        retVec.push_back(tmp);
    }

    return retVec;
}

std::vector<std::shared_ptr<Ticket>> ModelService::findOrdersByTargetVersion(std::string targetModelVersion)
{
    std::vector<std::shared_ptr<Ticket>> retVec;
    //查询版本迭代类工单
    std::map<std::string,std::string> ruleMap;
    ruleMap["type"] = "版本迭代";
    const auto& ret1 = ticketDAO_->selectOrderByCondition_(ruleMap,0,INT_MAX);
    for(const auto& ele:ret1)
    {
        const auto& transfered = std::static_pointer_cast<const TicketVersion>(ele);
        if(transfered->newModelVersion == targetModelVersion)
        {
            retVec.push_back(ele);
        }
    }

    //查询直接封装+发送类工单
    ruleMap["type"] = "直接封装+发送";
    const auto& ret2 = ticketDAO_->selectOrderByCondition_(ruleMap,0,INT_MAX);
    for(const auto& ele:ret2)
    {
        const auto& transfered = std::static_pointer_cast<const TicketPackage>(ele);
        if(transfered->newModelVersion == targetModelVersion)
        {
            retVec.push_back(ele);
        }
    }
    //查询功能开发类工单
    ruleMap["type"] = "功能开发";
    const auto& ret3 = ticketDAO_->selectOrderByCondition_(ruleMap,0,INT_MAX);
    for(const auto& ele:ret3)
    {
        const auto& transfered = std::static_pointer_cast<const TicketFeature>(ele);
        if(transfered->newModelVersion == targetModelVersion)
        {
            retVec.push_back(ele);
        }   
    }

    return retVec;
}

int ModelService::getModelVersionCount(std::string model)
{
    return modelDAO_->getModelVersionCount(model);
}