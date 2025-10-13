#pragma once
#include "Entity.h"
#include <vector>

class IEncryptionKey
{
public:
    virtual std::vector<DongleInfo> getDongleInfo() = 0;
    virtual ~IEncryptionKey() = 0;
    virtual bool createEncryptionKey(std::string,std::string) = 0;
    virtual bool updateEncryptionKey(int id,std::string,std::string) = 0;

    virtual std::vector<std::vector<std::string>> selectAllEncryptionHistoryByEK(std::string) = 0;

    virtual std::vector<std::vector<std::string>> selectAllAuthInfoByClientEK(std::string,std::string) = 0;


    // 交付操作：向encryption_key表和encryption_key_history表插入数据
    // 参数：客户名称、外壳号、客户设备类型、客户设备备注
    virtual bool deliveryOperation(const std::string& clientName,
                                  const std::string& shellNumber,
                                  const std::string& deviceType,
                                  const std::string& deviceNote,
                                  const std::string& contractName,
                                  const std::string& contractNumber) = 0;

    // 入库操作：更新encryption_key_history表中的入库时间和状态
    // 参数：客户名称、外壳号、操作类型、入库时间、出库时间、备注
    virtual bool returnOperation(const std::string& clientName,
                                const std::string& shellNumber,
                                const std::string& operationType,
                                const std::string& inTime,
                                const std::string& outTime,
                                const std::string& remark) = 0;

    // 获取可交付的外壳号列表
    // 返回：可交付的外壳号信息列表（id和shell_number）
    virtual std::vector<std::pair<int, std::string>> getAvailableShellNumbers() = 0;

    // 创建授权操作：向product_authorization和product_authorization_info表插入数据
    // 参数：客户名称、外壳号、授权ID、授权类型、授权开始日期、授权结束日期、授权备注
    virtual bool createAuthorization(const std::string& clientName,
                                     const std::string& shellNumber,
                                     const std::string& authId,
                                     const std::string& authType,
                                     const std::string& startDate,
                                     const std::string& endDate,
                                     const std::string& authNote) = 0;

    // 批量更新授权截止日期
    // 参数：客户名称、更新列表（外壳号、授权ID、新的截止日期）
    virtual bool updateAuthorizationEndDates(const std::string& clientName,
                                             const std::vector<std::tuple<std::string, std::string, std::string>>& changes) = 0;

    // 更新指定客户的指定外壳号的设备信息
    // 参数：客户名称、外壳号、设备类型、设备备注
    virtual bool updateShellDeviceInfo(const std::string& clientName,
                                       const std::string& shellNumber,
                                       const std::string& deviceType,
                                       const std::string& deviceNote,
                                       const std::string& contractName,
                                       const std::string& contractNumber) = 0;
};

inline IEncryptionKey::~IEncryptionKey() = default;
