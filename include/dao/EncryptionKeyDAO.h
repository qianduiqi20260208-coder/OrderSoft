#include "IEncryptionKeyDAO.h"
#include <mysql.h>
#define SQL_MAX 512		// sql语句字符数组最大值


class EncryptionKey : public IEncryptionKey
{
public:
    EncryptionKey(MYSQL* m);
    std::vector<DongleInfo> getDongleInfo() override;

    bool createEncryptionKey(std::string,std::string) override;

    bool updateEncryptionKey(int id,std::string,std::string) override;

    //根据加密狗筛选加密历史记录
    std::vector<std::vector<std::string>> selectAllEncryptionHistoryByEK(std::string) override;

    //根据客户筛选授权信息
    std::vector<std::vector<std::string>> selectAllAuthInfoByClientEK(std::string,std::string) override;


    ~EncryptionKey();
private:
    MYSQL* mysql;
    char sql[SQL_MAX];		// 存储sql语句
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;
};