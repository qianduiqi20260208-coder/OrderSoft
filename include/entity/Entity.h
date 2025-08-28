#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct Model {
    int id;
    std::string ataChapter;
    std::string modelName;
};

struct ModelVersion {
    int id;
    int modelId;
    std::string version;
};

struct ModelLicenseAuth {
    int id;
    int modelVersionId;
    int licenseId;
};

struct User {
    int id;
    int jobNumber; // 工号 userid
    std::string password; // 密码
    std::string fullName; // 姓名
    std::vector<std::string> roleVec; // 用户角色
    std::vector<std::string> responsibleModel; // 关联的模型
    std::string email; // 邮箱
    std::string phone; // 手机号
    std::string createdTime; // 创建时间
    std::string lastLoginTime; // 最后登录时间
    bool isOnline; // 是否在线
    int lastUsedModel; // 最后使用的模型

    //工单流程中的身份
    std::vector<std::string> flowRoleVec;
};

struct TicketExecutor {
    int id;
    int ticketId;
    std::vector<std::string> executor;
    std::vector<std::string> timestamp;
    std::vector<std::string> reason;
};

struct Ticket {
    int id; 
    std::string creatorId; // 创建人ID
    std::string createTime; // 创建时间
    std::string ticketType; // 工单类型
    std::string model; // 关联模型
    std::string modelVersion; // 关联模型版本
    std::string status; // 工单状态
    std::string approverId; // 审批人ID
    std::string priorityHint; // 参考优先级
    std::string distributorId; // 分发人ID
    std::string executorId; // 执行人ID 分发工单时指定的执行人ID
    std::string approvedTime; // 审批时间
    std::string priorityTask; // 任务优先级
    std::string distributedTime; // 分发时间
    std::string completedTime; // 完成时间
    std::string rejectReason; // 拒绝原因
    TicketExecutor executor;//流转工单时对应的执行人们

    std::string dispatchRejectReason; // 分发拒绝原因


    // 多态序列化接口,与前端定义的变量对应
    virtual nlohmann::json to_json() const {
        nlohmann::json j;
		j["orderID"] = (id != 0 ? std::to_string(id) : ""); // 工单ID 增加条件运算符，因为int型变量不能为空，当id为0时，需要返回空字符串
		j["promoterID"] = creatorId; // 发起人ID
		j["startTime"] = createTime; // 发起时间
		j["type"] = ticketType; // 工单类型
		j["modelID"] = model; // 关联模型ID
		j["modelVersionID"] = modelVersion; // 关联模型版本ID
		j["status"] = status; // 工单状态
		j["approverID"] = approverId; // 审批人ID
		j["referencePriority"] = priorityHint; // 参考优先级
		j["distributorID"] = distributorId; // 分发人ID
		j["approveTime"] = approvedTime; // 审批时间
		j["taskPriority"] = priorityTask; // 任务优先级
		j["distributeTime"] = distributedTime; // 分发时间
		j["finishTime"] = completedTime; // 完成时间
        j["rejectReason"] = rejectReason; // 拒绝原因

        // 序列化 executor 流转信息（倒序）
        j["transfers"] = nlohmann::json::array();

        // 安全检查：确保数组不为空且大小一致
        if (!executor.executor.empty() && 
            !executor.timestamp.empty() && 
            !executor.reason.empty()) {
            
            size_t transferCount = std::min({
                executor.executor.size(),
                executor.timestamp.size(), 
                executor.reason.size()
            });

            j["executorID"] = executor.executor[0]; 
            
            // 只有在记录数大于1时才返回流转信息（排除第一条分发记录）
            if (transferCount > 1) {
                // 倒序遍历：从最新的流转记录开始，跳过第一条记录
                for (int i = static_cast<int>(transferCount) - 2; i >= 0; --i) {
                    nlohmann::json transfer;
                    transfer["transferExecutorID"] = executor.executor[i];
                    transfer["transferReason"] = executor.reason[i];
                    transfer["transferTime"] = executor.timestamp[i];
                    j["transfers"].push_back(transfer);
                }
            }
            // 如果只有一条记录，j["transfers"] 保持为空数组
        }

        return j;
    }

    virtual nlohmann::json to_json_order_manage() const {
        nlohmann::json j;
		j["orderID"] = (id != 0 ? std::to_string(id) : ""); // 工单ID 增加条件运算符，因为int型变量不能为空，当id为0时，需要返回空字符串
		j["promoterID"] = creatorId; // 发起人ID
		j["startTime"] = createTime; // 发起时间
		j["type"] = ticketType; // 工单类型
		j["modelID"] = model; // 关联模型ID
		j["modelVersionID"] = modelVersion; // 关联模型版本ID
		j["status"] = status; // 工单状态
		j["approverID"] = approverId; // 审批人ID
		j["referencePriority"] = priorityHint; // 参考优先级
		j["distributorID"] = distributorId; // 分发人ID
		j["approveTime"] = approvedTime; // 审批时间
		j["taskPriority"] = priorityTask; // 任务优先级
		j["distributeTime"] = distributedTime; // 分发时间
		j["finishTime"] = completedTime; // 完成时间
        j["rejectReason"] = rejectReason; // 拒绝原因

        // 序列化 executor 流转信息（倒序）
        j["transfers"] = nlohmann::json::array();

        // 安全检查：确保数组不为空且大小一致
        if (!executor.executor.empty() && 
            !executor.timestamp.empty() && 
            !executor.reason.empty()) {
            
            size_t transferCount = std::min({
                executor.executor.size(),
                executor.timestamp.size(), 
                executor.reason.size()
            });

            j["executorID"] = executor.executor[0];  // 执行人ID从流转结构体中获取，流转结构体中的第一条数据默认存储分发时选择的执行人ID
            
            // 只有在记录数大于1时才返回流转信息（排除第一条分发记录）
            if (transferCount > 1) {
                // 倒序遍历：从最新的流转记录开始，跳过第一条记录
                for (int i = static_cast<int>(transferCount) - 2; i >= 0; --i) {
                    nlohmann::json transfer;
                    transfer["transferExecutorID"] = executor.executor[i];
                    transfer["transferReason"] = executor.reason[i];
                    transfer["transferTime"] = executor.timestamp[i];
                    j["transfers"].push_back(transfer);
                }
            }
            // 如果只有一条记录，j["transfers"] 保持为空数组
        }

        return j;
    }
    virtual ~Ticket() = default;
};

struct Attachment
{
    std::string file; // 文件具体内容
    std::string fileName; // 文件名
};

// 问题复现工单
struct TicketReproduce :public Ticket{
    int id;
    int ticketId;
    std::string coordinationId; // 协调单ID
    std::string content; // 复现内容描述
    Attachment attachment; // 附件信息
    std::string phenomenon; // 复现现象
    std::string remark; // 备注

    // 多态序列化接口
    nlohmann::json to_json() const override {
        nlohmann::json j = Ticket::to_json(); // 先序列化基类字段
        //j["id"] = id; // 如果子类id和基类id不同步，可保留
        //j["ticketId"] = ticketId;
		j["coordinationID"] = coordinationId; // 协调单ID
		j["description"] = content; // 复现内容描述
		j["fileName"] = attachment.fileName; // 附件信息传文件路径
        // 提供文件下载URL而不是直接传输文件内容
        if (!attachment.fileName.empty()) {
            j["fileUrl"] = "/files/ticket/" + std::to_string(Ticket::id) + "/" + attachment.fileName;
            j["hasAttachment"] = true;
        } else {
            j["hasAttachment"] = false;
        }
		j["finishPhenomenon"] = phenomenon; // 复现现象
		j["finishRemark"] = remark; // 备注
        return j;
    }

    // 多态序列化接口order_manage专用
    nlohmann::json to_json_order_manage() const override {
        nlohmann::json j = Ticket::to_json(); // 先序列化基类字段
        //j["id"] = id; // 如果子类id和基类id不同步，可保留
        //j["ticketId"] = ticketId;
		j["coordinationID"] = coordinationId; // 协调单ID
		j["description"] = content; // 复现内容描述
		j["fileName"] = attachment.fileName; // 附件信息传文件路径
        // 提供文件下载URL而不是直接传输文件内容
        if (!attachment.fileName.empty()) {
            j["fileUrl"] = "/files/ticket/" + std::to_string(Ticket::id) + "/" + attachment.fileName;
            j["hasAttachment"] = true;
        } else {
            j["hasAttachment"] = false;
        }
		j["finishPhenomenon"] = phenomenon; // 复现现象
		j["finishRemark"] = remark; // 备注
        return j;
    }
};


// 版本迭代工单
struct TicketVersion :public Ticket{
    int id;
    int ticketId;
    std::string coordinationId; // 协调单ID
    std::string updateNote; // 更新内容
    std::string packRequirement; // 封装要求
    bool interfaceChanged; // 接口是否变化
    std::string newModelVersion; // 升级后模型版本
    std::string baseModelVersion;//模型基版本
    std::string remark; // 备注 

    std::string matlab_version; // 备注 

    // 多态序列化接口
	nlohmann::json to_json() const override {
		nlohmann::json j = Ticket::to_json(); // 先序列化基类字段
		//j["id"] = id; // 如果子类id和基类id不同步，可保留
		//j["ticketId"] = ticketId;
		j["coordinationID"] = coordinationId; // 协调单ID
		j["updateNotes"] = updateNote; // 更新内容
		j["packageRequirement"] = packRequirement; // 封装要求
		j["apiChanged"] = interfaceChanged ? "是" : "否"; // 接口是否变化（是/否）
		j["finishModelVersion"] = newModelVersion; // 升级后模型版本
        j["baseModelVersion"] = baseModelVersion; // 模型父版本
		j["finishRemark"] = remark; // 备注
        j["matlabVersion"] = matlab_version; // MATLAB版本
		return j;
	}

        // 多态序列化接口 order_manage专用
	nlohmann::json to_json_order_manage() const override {
		nlohmann::json j = Ticket::to_json(); // 先序列化基类字段
		//j["id"] = id; // 如果子类id和基类id不同步，可保留
		//j["ticketId"] = ticketId;
		j["coordinationID"] = coordinationId; // 协调单ID
		j["updateNotes"] = updateNote; // 更新内容
		j["packageRequirement"] = packRequirement; // 封装要求
		j["apiChanged"] = interfaceChanged ? "是" : "否"; // 接口是否变化（是/否）
		j["finishModelVersion"] = newModelVersion; // 升级后模型版本
        j["baseModelVersion"] = baseModelVersion; // 模型父版本
		j["completeModelVersion"] = remark; // 备注

        j["matlabVersion"] = matlab_version; // MATLAB版本
		return j;
	}
};


// 直接封装+发送类工单
struct TicketPackage :public Ticket{
    int id;

    int ticketId;
    std::string coordinationId; // 协调单ID
    std::string updateNote; // 更新内容
    std::string packRequirement; // 封装要求
    bool interfaceChanged; // 接口是否变化
    std::string targetClient; // 目标客户
    bool validatedByCAE; // 是否经过CAE检查

    std::string sensitiveInfo; // 是否包含敏感信息

    std::string newModelVersion; // 升级后模型版本
    std::string baseModelVersion;// 模型基版本
    bool encrypted; // 是否加密
    std::vector<std::string> dongle; // 外壳号列表
    std::string license; // 授权ID
    std::string remark; // 备注

    std::string matlab_version; // 备注 
    std::string targetDeliveryTime; // 备注 
    // 多态序列化接口
	nlohmann::json to_json() const override {
		nlohmann::json j = Ticket::to_json(); // 先序列化基类字段
		//j["id"] = id; // 如果子类id和基类id不同步，可保留
		//j["ticketId"] = ticketId;
        j["type"] = "版本迭代+交付发送"; // 工单类型，子类重写
		j["coordinationID"] = coordinationId; // 协调单ID
		j["updateNotes"] = updateNote; // 更新内容
		j["packageRequirement"] = packRequirement; // 封装要求
		j["apiChanged"] = interfaceChanged ? "是" : "否"; // 接口是否变化（是/否）
		j["targetCustomer"] = targetClient; // 目标客户
		j["isCAEChecked"] = validatedByCAE ? "是" : "否"; // 是否经过CAE检查（是/否）
		j["hasSensitiveInfo"] = sensitiveInfo; // 是否包含敏感信息（是/否）
		j["finishModelVersion"] = newModelVersion; // 升级后模型版本ID
        j["baseModelVersion"] = baseModelVersion; // 模型父版本
		j["isEncrypted"] = encrypted ? "是" : "否"; // 是否加密（是/否）
		j["finishShellNo"] = dongle; // 外壳号
		j["finishAuthId"] = license; // 授权ID
		j["finishRemark"] = remark; // 备注
        j["matlabVersion"] = matlab_version; // MATLAB版本
        j["targetDeliveryTime"] = targetDeliveryTime; // 预计发送时间
		return j;
	}

    // 多态序列化接口 order_manage专用
    nlohmann::json to_json_order_manage() const override {
		nlohmann::json j = Ticket::to_json(); // 先序列化基类字段
		//j["id"] = id; // 如果子类id和基类id不同步，可保留
		//j["ticketId"] = ticketId;
        j["type"] = "版本迭代+交付发送"; // 工单类型，子类重写
		j["coordinationID"] = coordinationId; // 协调单ID
		j["updateNotes"] = updateNote; // 更新内容
		j["packageRequirement"] = packRequirement; // 封装要求
		j["apiChanged"] = interfaceChanged ? "是" : "否"; // 接口是否变化（是/否）
		j["targetCustomer"] = targetClient; // 目标客户
		j["isCAEChecked"] = validatedByCAE ? "是" : "否"; // 是否经过CAE检查（是/否）
		j["hasSensitiveInfo"] = sensitiveInfo; // 是否包含敏感信息（是/否）
		j["finishModelVersion"] = newModelVersion; // 升级后模型版本ID
        j["baseModelVersion"] = baseModelVersion; // 模型父版本
		j["isEncrypted"] = encrypted ? "是" : "否"; // 是否加密（是/否）
		j["finishShellNo"] = dongle; // 外壳号
		j["finishAuthId"] = license; // 授权ID
		j["completeModelVersion"] = remark; // 备注
        j["matlabVersion"] = matlab_version; // MATLAB版本
        j["targetDeliveryTime"] = targetDeliveryTime; // 预计发送时间
		return j;
	}
};


// 交付发送类工单
struct TicketDelivery :public Ticket{
    int id;
    std::string targetClient; // 目标客户

    bool validatedByCAE; // 是否经过CAE检查
    std::string sensitiveInfo; // 是否包含敏感信息
    bool encrypted; // 是否加密
    std::string dongleId; // 外壳号
    std::string licenseId; // 授权ID
    std::string remark; // 备注
    std::string targetDeliveryTime; // 备注

    // 多态序列化接口
	nlohmann::json to_json() const override {
		nlohmann::json j = Ticket::to_json(); // 先序列化基类字段
		//j["id"] = id; // 如果子类id和基类id不同步，可保留
		//j["ticketId"] = ticketId;
		j["targetCustomer"] = targetClient;  // 目标客户
		j["isCAEChecked"] = validatedByCAE ? "是" : "否"; // 是否经过CAE检查（是/否）
		j["hasSensitiveInfo"] = sensitiveInfo; // 是否包含敏感信息（是/否）
		j["isEncrypted"] = encrypted ? "是" : "否"; // 是否加密（是/否）
		j["finishShellNo"] = dongleId; // 外壳号
		j["finishAuthId"] = licenseId; // 授权ID
		j["finishRemark"] = remark; // 备注
        j["targetDeliveryTime"] = targetDeliveryTime; // 预计发送时间
		return j;
	}

        // 多态序列化接口order manage专用
	nlohmann::json to_json_order_manage() const override {
		nlohmann::json j = Ticket::to_json(); // 先序列化基类字段
		//j["id"] = id; // 如果子类id和基类id不同步，可保留
		//j["ticketId"] = ticketId;
		j["targetCustomer"] = targetClient;  // 目标客户
		j["isCAEChecked"] = validatedByCAE ? "是" : "否"; // 是否经过CAE检查（是/否）
		j["hasSensitiveInfo"] = sensitiveInfo; // 是否包含敏感信息（是/否）
		j["isEncrypted"] = encrypted ? "是" : "否"; // 是否加密（是/否）
		j["finishShellNo"] = dongleId; // 外壳号
		j["finishAuthId"] = licenseId; // 授权ID
		j["finishRemark"] = remark; // 备注
        j["targetDeliveryTime"] = targetDeliveryTime; // 预计发送时间
		return j;
	}
};


// 功能开发类工单
struct TicketFeature :public Ticket{
    int id;
    int ticketId;
    std::string featureInit; // 功能描述
    std::string newModelVersion; // 完成后模型版本ID
    std::string baseModelVersion;//模型基版本
    std::string featureFinal; // 完成后功能描述

    // 多态序列化接口
	nlohmann::json to_json() const override {
		nlohmann::json j = Ticket::to_json(); // 先序列化基类字段
		//j["id"] = id; // 如果子类id和基类id不同步，可保留
		//j["ticketId"] = ticketId;
		j["featureDesc"] = featureInit; // 功能描述
		j["finishModelVersionId"] = newModelVersion; // 完成后模型版本ID
        j["baseModelVersion"] = baseModelVersion; // 模型父版本
		j["finishFeatureDesc"] = featureFinal; // 完成功能描述
		return j;
	}

    // 多态序列化接口order manage专用
	nlohmann::json to_json_order_manage() const override {
		nlohmann::json j = Ticket::to_json(); // 先序列化基类字段
		//j["id"] = id; // 如果子类id和基类id不同步，可保留
		//j["ticketId"] = ticketId;
		j["featureDesc"] = featureInit; // 功能描述
		j["finishModelVersionId"] = newModelVersion; // 完成后模型版本ID
        j["baseModelVersion"] = baseModelVersion; // 模型父版本
		j["completeModelVersion"] = featureFinal; // 完成功能描述
		return j;
	}
};


// 其他类工单
struct TicketOther :public Ticket{
    int id;
    int ticketId;
    std::string description; // 内容描述
    std::string remark; // 备注

    // 多态序列化接口
	nlohmann::json to_json() const override {
		nlohmann::json j = Ticket::to_json(); // 先序列化基类字段
		//j["id"] = id; // 如果子类id和基类id不同步，可保留
		//j["ticketId"] = ticketId;
		j["contentDesc"] = description; // 内容描述
		j["finishRemarkOther"] = remark; // 备注
		return j;
	}

    // 多态序列化接口order manage专用
	nlohmann::json to_json_order_manage() const override {
		nlohmann::json j = Ticket::to_json(); // 先序列化基类字段
		//j["id"] = id; // 如果子类id和基类id不同步，可保留
		//j["ticketId"] = ticketId;
		j["contentDesc"] = description; // 内容描述
		j["finishRemarkOther"] = remark; // 备注
		return j;
	}
};

struct DongleInfo {
    std::string dongleId;           // 加密狗ID
    std::string shellCode;          // 外壳号
    std::string shellSerial;        // 外壳序列号
    std::string dongleRemark;       // 加密狗备注
    std::string dongleStatus;       // 加密狗状态
    std::string clientName;         // 客户名称
    std::string clientDeviceType;   // 客户设备类型（lab/IPT/FTD/FFS）
    std::string clientNote;         // 客户电脑备注
    std::string inTime;             // 入库时间（建议使用 std::chrono 或转成时间戳类型）
    std::string outTime;            // 出库时间（建议使用 std::chrono 或转成时间戳类型）

    // 序列化接口
    nlohmann::json to_json() const {
        nlohmann::json j;
        j["dongleId"] = dongleId; // 加密狗ID
        j["shellCode"] = shellCode; // 外壳号
        j["shellSerial"] = shellSerial; // 外壳序列号
        j["dongleRemark"] = dongleRemark; // 加密狗备注
        j["dongleStatus"] = dongleStatus; // 加密狗状态
        j["clientName"] = clientName; // 客户名称
        j["clientDeviceType"] = clientDeviceType; // 客户设备类型
        j["clientNote"] = clientNote; // 客户电脑备注
        j["inTime"] = inTime; // 入库时间
        j["outTime"] = outTime; // 出库时间
        return j;
    }
};


struct Log {
    int id;
    int userId;
    std::string opType;
    std::string targetTable;
    int recordId;
    std::string before;
    std::string after;
    std::string createdTime;
};


struct ProductLicense {
    int id;
    int dongleId;
    std::string licenseCode;
    std::string type;
    std::string startDate;
    std::string endDate;
    std::string status;
    std::string remark;
};

struct TicketReproduceAttachment
{
    int id;
    int ticketId;
    std::string filePath;
    std::string fileName;
    std::string uploadTime;
};


//
struct Authorization
{
    std::string authId;
    std::string startDate;
    std::string endDate;
    std::string authType;
    std::string authNote;
    // std::string status; 暂不定义
};
//
struct ShellNumber
{

   std::string shellNumber;
   std::string deviceType;
   std::string deviceNote;
   std::string outTime;
   int authCount;
   std::vector<Authorization> authorizationList;
};
//
struct Client
{
   int id;
   std::string clientName;
   int dongleCount;
   std::vector<ShellNumber> shellNumbers;
};
