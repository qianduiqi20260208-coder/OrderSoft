#pragma once
#include "IEncryptionKeyService.h"
#include "IEncryptionKeyDAO.h"
#include <memory>

class EncryptionKeyService : public IEncryptionKeyService
{
public:
    EncryptionKeyService(std::shared_ptr<IEncryptionKey> dao);
    std::vector<DongleInfo> getDongleInfo() override;
    bool createEncryptionKey(std::string,std::string) override;
    bool updateEncryptionKey(int id,std::string,std::string) override;

    // 交付操作：向encryption_key表和encryption_key_history表插入数据
    bool deliveryOperation(const std::string& clientName,
                          const std::string& shellNumber,
                          const std::string& deviceType,
                          const std::string& deviceNote) override;
    // 归还操作：更新encryption_key_history表中的归还时间和状态
    bool returnOperation(const std::string& clientName,
                         const std::string& shellNumber,
                         const std::string& returnDate) override;

    // 获取可交付的外壳号列表
    std::vector<std::pair<int, std::string>> getAvailableShellNumbers() override;

    // 创建授权操作
    bool createAuthorization(const std::string& clientName,
                            const std::string& shellNumber,
                            const std::string& authId,
                            const std::string& authType,
                            const std::string& startDate,
                            const std::string& endDate,
                            const std::string& authNote) override;

    // 批量更新授权截止日期
    bool updateAuthorizationEndDates(const std::string& clientName,
                                     const std::vector<std::tuple<std::string, std::string, std::string>>& changes) override;

    // 更新指定客户的指定外壳号的设备信息
    bool updateShellDeviceInfo(const std::string& clientName,
                               const std::string& shellNumber,
                               const std::string& deviceType,
                               const std::string& deviceNote) override;

    ~EncryptionKeyService();
private:
    std::shared_ptr<IEncryptionKey> encryptionKeyDAO_;
};