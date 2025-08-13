#include "IEncryptionKeyDAO.h"
#include <mysql.h>
#define SQL_MAX_ 512		// sql语句字符数组最大值


class EncryptionKey : public IEncryptionKey
{
public:
    EncryptionKey(MYSQL* m);
    std::vector<DongleInfo> getDongleInfo() override;

    bool createEncryptionKey(std::string,std::string) override;

    bool updateEncryptionKey(int id,std::string,std::string) override;

    // 交付操作：向encryption_key表和encryption_key_history表插入数据
    bool deliveryOperation(const std::string& clientName,
                          const std::string& shellNumber,
                          const std::string& deviceType,
                          const std::string& deviceNote) override;

    // 入库操作：更新encryption_key_history表中的入库时间和状态
    bool returnOperation(const std::string& clientName,
                        const std::string& shellNumber,
                        const std::string& returnDate) override;

    // 获取可交付的外壳号列表
    std::vector<std::pair<int, std::string>> getAvailableShellNumbers() override;

    // 创建授权操作：向product_authorization和product_authorization_info表插入数据
    bool createAuthorization(const std::string& clientName,
                            const std::string& shellNumber,
                            const std::string& authId,
                            const std::string& authType,
                            const std::string& startDate,
                            const std::string& endDate,
                            const std::string& authNote) override;
    ~EncryptionKey();
private:
    MYSQL* mysql;
    char sql[SQL_MAX_];		// 存储sql语句
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;
};