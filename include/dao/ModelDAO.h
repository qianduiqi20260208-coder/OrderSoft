#include "IModelDAO.h"
#include "BaseDAO.h"
#include <mysql.h>
#define SQL_MAX 99999		// sql语句字符数组最大值

class ModelDAO : public IModelDAO, public BaseDAO
{
public:
    ModelDAO(); // 使用连接池，不需要MYSQL参数
    ModelDAO(MYSQL* ms); // 兼容旧接口（已废弃）
    std::vector<std::string> getModel()override;
    std::vector<std::string> getModelVersionByModel(std::string modelName)override;
    

    bool addModelVersion(const std::string& model,const std::string& modelVersion) override;

    std::vector<std::vector<std::string>> getModelVersionInfoByModelPaged(const std::string& model,int page,int pageSize) override;

    int getModelVersionCount(std::string model) override;

    ~ModelDAO();
private:
    MYSQL* mysql;
    char sql[SQL_MAX];		// 存储sql语句
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;
};
