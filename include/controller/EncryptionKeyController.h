#pragma once
#include "IEncryptionKeyService.h"
#include "util/ticket.h"
#include "entity/Entity.h"
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include <crow.h>
#include "crow/middlewares/cors.h"

class EncryptionKeyController
{
public:
    EncryptionKeyController(std::shared_ptr<IEncryptionKeyService> service);

    MultipartResult parseMultipartForm(const std::string& content_type, const std::string& body,TicketReproduce& ticketreproduce);
    
    // 注册路由
    void registerRoutes(crow::App<crow::CORSHandler>& app);
    
private:
    std::shared_ptr<IEncryptionKeyService> encryptionKeyService_;
};