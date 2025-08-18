#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <memory>
#include <mysql.h>

#include "EncryptionKeyDAO.h"
#include "EncryptionKeyService.h"


MYSQL* mysql;
auto encryptionKeyDAO = std::make_shared<EncryptionKey>(mysql);

auto encryptionKeySvc = std::make_shared<EncryptionKeyService>(encryptionKeyDAO);

TEST_CASE("getDongleInfo") {

   auto ret = encryptionKeySvc->getDongleInfo();

}

TEST_CASE("createEncryptionKey") {
    auto ret = encryptionKeySvc->createEncryptionKey("asfsfs","fsdfdsfsdf");

}

TEST_CASE("updateEncryptionKey") {
    auto ret = encryptionKeySvc->updateEncryptionKey(1, "qwer1234","fsd");
}

TEST_CASE("send record") {
    auto ret = encryptionKeySvc->getEncryptionKeyHistoryWithAuthRecordByEK("qwer1234");
}
