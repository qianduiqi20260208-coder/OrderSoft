#include "IEncryptionKeyDAO.h"
#include "BaseDAO.h"
#include <mysql.h>
#define SQL_MAX 99999	

class EncryptionKey : public IEncryptionKey, public BaseDAO
{
public:
    EncryptionKey(); // 使用连接池，不需要MYSQL参数
    EncryptionKey(MYSQL* m); // 兼容旧接口（已废弃）
    std::vector<DongleInfo> getDongleInfo() override;

    bool createEncryptionKey(std::string,std::string) override;

    bool updateEncryptionKey(int id,std::string,std::string) override;

    //根据加密狗筛选加密历史记录
    std::vector<std::vector<std::string>> selectAllEncryptionHistoryByEK(std::string) override;

    //根据客户筛选授权信息
    std::vector<std::vector<std::string>> selectAllAuthInfoByClientEK(std::string,std::string) override;


    // 交付操作：向encryption_key表和encryption_key_history表插入数据
    bool deliveryOperation(const std::string& clientName,
                          const std::string& shellNumber,
                          const std::string& deviceType,
                          const std::string& deviceNote) override;

    // 入库操作：更新encryption_key_history表中的入库时间和状态
    bool returnOperation(const std::string& clientName,
                        const std::string& shellNumber,
                        const std::string& operationType,
                        const std::string& inTime,
                        const std::string& outTime,
                        const std::string& remark) override;

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

    // 批量更新授权截止日期
    bool updateAuthorizationEndDates(const std::string& clientName,
                                     const std::vector<std::tuple<std::string, std::string, std::string>>& changes) override;

    // 更新指定客户的指定外壳号的设备信息
    bool updateShellDeviceInfo(const std::string& clientName,
                               const std::string& shellNumber,
                               const std::string& deviceType,
                               const std::string& deviceNote) override;
    ~EncryptionKey();
private:
    MYSQL* mysql;
    char sql[SQL_MAX];		// 存储sql语句
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;
};