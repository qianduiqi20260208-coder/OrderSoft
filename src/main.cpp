#ifdef _WIN32
#include <winsock2.h>
#endif

#include <memory>
#include "TicketDAO.h"
#include "TicketService.h"
#include <mysql.h>
#include "ModelDAO.h"
#include "ModelService.h"
#include "UserDAO.h"
#include "UserService.h"
#include "crow.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;

void test();
void testGetModel();
void testGetUser();

int main() {
	// FILE* log = freopen(LOG_PATH,"w",stdout);
	// if(log == nullptr)
	// {
	// 	printf("标准输出重定向失败！\n");
	// 	return 1;
	// }

	//test();
	//testGetModel();
	//testGetUser();

    // crow::SimpleApp app;

    // CROW_ROUTE(app, "/")([](){
    //     return "Hello Crow!";
    // });

    // app.port(18080).run();
	std::cout<<"这是主程序\n";
	return 0;
}

void testGetUser()
{
	MYSQL mysql;
	auto userDAO = std::make_shared<UserDAO>(&mysql);
	auto userSvc = std::make_shared<UserService>(userDAO);
	auto ret1 = userSvc->getUserByJobNumber(111111);

	auto ret2 = userSvc->getUserOrder(888888);

	//查询是否成功的判断
	if(!ret2.empty() && !ret2[0])
	{
		printf("getUserOrder()查询失败!\n");
	}else{
		for(auto ele:ret2)
		{
			printf("user order id: %d\n",ele->id);
			printf("order status: %s\n",ele->status.c_str());
			printf("order type: %s\n",ele->ticketType.c_str());
			// if(ele->ticketType == "问题复现")
			// {
			// 	auto ret = std::static_pointer_cast<TicketReproduce>(ele);
			// 	printf("file: %s\n",ret->attachment.file.c_str());
			// 	printf("fileName: %s\n",ret->attachment.fileName.c_str());

			// }
			// if(ele->ticketType == "版本迭代")
			// {
			// 	auto ret = std::static_pointer_cast<TicketVersion>(ele);
			// 	printf("coordination_id: %s\n",ret->coordinationId.c_str());
			// 	printf("update_content: %s\n",ret->updateNote.c_str());
			// 	printf("packaging_requirements: %s\n",ret->packRequirement.c_str());
			// 	printf("interface_changed: %d\n",ret->interfaceChanged);
			// }
			// if(ele->ticketType == "直接封装+发送")
			// {
			// 	auto ret = std::static_pointer_cast<TicketPackage>(ele);
			// 	printf("coordination_id: %s\n",ret->coordinationId.c_str());
			// 	printf("update_content: %s\n",ret->updateNote.c_str());
			// 	printf("packaging_requirements: %s\n",ret->packRequirement.c_str());
			// 	printf("interface_changed: %s\n",ret->interfaceChanged);
			// 	printf("target_customer: %s\n",ret->targetClient.c_str());
			// 	printf("validated_by_cae: %d\n",ret->validatedByCAE);
			// 	printf("sensitive_info: %s\n",ret->sensitiveInfo.c_str());
			// }
			if(ele->ticketType == "交付发送")
			{
				auto ret = std::static_pointer_cast<TicketDelivery>(ele);
				printf("target_customer: %s\n",ret->targetClient.c_str());
				printf("validated_by_cae: %d\n",ret->validatedByCAE);
				printf("sensitive_info: %s\n",ret->sensitiveInfo.c_str());
			}
			if(ele->ticketType == "功能开发")
			{
				auto ret = std::static_pointer_cast<TicketFeature>(ele);
				printf("description_create: %s\n",ret->featureInit.c_str());

			}
			if(ele->ticketType == "其他")
			{
				auto ret = std::static_pointer_cast<TicketOther>(ele);
				printf("description: %s\n",ret->description.c_str());

			}
		}
	}
}

void testGetModel()
{
	MYSQL mysql;	
	auto modelDAO = std::make_shared<ModelDAO>(&mysql);
	auto modelSvc = std::make_shared<ModelService>(modelDAO);
	auto ret1 = modelSvc->getModel();
	for(auto ele:ret1)
		printf("%s\n",ele.c_str());

	auto ret2 = modelSvc->getModelVersionByModel("ATA04_Aerodynamics");
	for(auto ele:ret2)
		printf("%s\n",ele.c_str());
}

void test()
{
	//createTicket部分
	MYSQL mysql;	
	auto ticketDAO = std::make_shared<TicketDAO>(&mysql);
	auto ticketSvc = std::make_shared<TicketService>(ticketDAO);
	//问题复现类工单测试
	// TicketReproduce ticket;
	// ticket.creatorId = 666666;
	// ticket.approverId = 888888;
	// ticket.ticketType = "问题复现";
	// ticket.model = "ATA04_Aerodynamics";
	// ticket.modelVersion = "12.2.1";

	// ticket.coordinationId = "这是协调单号";
	// ticket.content = "这是复现内容";
	// ticket.attachment.fileName = "aaa.txt";
	// ticket.attachment.file = "这是复现参考文件内容";

	//版本迭代类工单测试
	// TicketVersion ticket;
	// ticket.creatorId = 666666;
	// ticket.approverId = 888888;
	// ticket.ticketType = "版本迭代";
	// ticket.model = "ATA04_Aerodynamics";
	// ticket.modelVersion = "12.2.1";
	// ticket.coordinationId = "这是协调单号";
	// ticket.updateNote = "这是更新内容";
	// ticket.packRequirement = "这是封装需求";
	// ticket.interfaceChanged = true;

	//交付发送类工单
	// TicketDelivery ticket;
	// ticket.creatorId = 666666;
	// ticket.approverId = 888888;
	// ticket.ticketType = "交付发送";
	// ticket.model = "ATA04_Aerodynamics";
	// ticket.modelVersion = "12.2.1";

	// ticket.targetClient = "张三";
	// ticket.validatedByCAE = true;
	// ticket.sensitiveInfo = "这是敏感信息";

	// TicketPackage ticket;
	// ticket.creatorId = 666666;
	// ticket.approverId = 888888;
	// ticket.ticketType = "直接封装+发送";
	// ticket.model = "ATA04_Aerodynamics";
	// ticket.modelVersion = "12.2.1";
	
	// ticket.coordinationId = "这是协调单ID";
	// ticket.updateNote = "这是更新记录";
	// ticket.packRequirement = "这是封装需求";
	// ticket.interfaceChanged = 0;
	// ticket.targetClient = "张三";
	// ticket.validatedByCAE = true;
	// ticket.sensitiveInfo = "这是敏感信息";
	
	//功能开发类
	// TicketFeature ticket;
	// ticket.creatorId = 666666;
	// ticket.approverId = 888888;
	// ticket.ticketType = "功能开发";
	// ticket.model = "ATA04_Aerodynamics";
	// ticket.modelVersion = "12.2.1";
	
	// ticket.featureInit = "创建时写入的描述";

	//其他类工单
	// TicketOther ticket;
	// ticket.creatorId = 666666;
	// ticket.approverId = 888888;
	// ticket.ticketType = "其他";
	// ticket.model = "ATA04_Aerodynamics";
	// ticket.modelVersion = "12.2.1";
	
	// ticket.description= "这是描述";

	// ticketSvc->createTicket(ticket);

	//审批工单测试
	// TicketReproduce ticket;
	// ticket.Ticket::id =1;
	// ticket.approverId = 888888;
	// ticket.priorityHint = "紧急";
	// ticket.distributorId = 999999;
	// ticketSvc->approveTicket(ticket);

	//分发工单测试
	// TicketReproduce ticket;
	// ticket.Ticket::id =1;
	// ticket.distributorId = 999999;
	// ticket.priorityTask = "紧急";
	// ticket.executorId = 000000;
	// ticketSvc->dispatchTicket(ticket);

	//完成工单测试
	// TicketReproduce ticket;
	// ticket.Ticket::id =1;
	// ticket.ticketType = "问题复现";
	// ticket.executorId = 000000;
	// ticket.phenomenon = "这是复现现象";
	// ticket.remark = "这是问题复现类工单的备注";

	// TicketVersion ticket;
	// ticket.Ticket::id =2;
	// ticket.ticketType = "问题复现";
	// ticket.newModelVersion = "12.2.2";
	// ticket.remark = "这是版本迭代类工单的备注";

	//版本迭代类完成工单
	// TicketVersion ticket;
	// ticket.Ticket::id =3;
	// ticket.ticketType = "版本迭代";
	// ticket.executorId = 000000;
	// ticket.newModelVersion = "12.2.2";
	// ticket.model ="ATA04_Aerodynamics";
	// ticket.remark = "这是问题复现类工单备注";

	
	// 交付发送类工单
	// TicketDelivery ticket;
	// ticket.Ticket::id =4;
	// ticket.ticketType = "交付发送";
	// ticket.encrypted = 1;
	// ticket.dongleId = "qwer1234";
	// ticket.licenseId = "333444555";
	// ticket.remark ="这是交付发送类工单的备注";

	//直接封装+发送类工单
	// TicketPackage ticket;
	// ticket.Ticket::id =5;
	// ticket.ticketType = "直接封装+发送";
	// ticket.model ="ATA04_Aerodynamics";
	// ticket.newModelVersion = "12.2.2";
	// ticket.encrypted = 1;
	// ticket.license = "333444555";
	// ticket.dongle = "qwer1234";
	// ticket.remark ="这是直接封装+发送类工单的备注";

	//功能开发类工单
	// TicketFeature ticket;
	// ticket.Ticket::id =6;
	// ticket.ticketType = "功能开发";
	// ticket.featureFinal = "功能开发最终描述";
	//其他类工单
	TicketOther ticket;
	ticket.Ticket::id =7;
	ticket.ticketType = "其他";
	ticket.remark = "这是其他类工单的备注";

	ticketSvc->completeTicket(ticket);
}

