#include "IModelDAO.h"
#include <mysql.h>
#define SQL_MAX 512		// sql语句字符数组最大值

class ModelDAO :public IModelDAO
{
public:
    ModelDAO(MYSQL* ms);
    std::vector<std::string> getModel()override;
    std::vector<std::string> getModelVersionByModel(std::string modelName)override;

    bool addModelVersion(const std::string& model,const std::string& modelVersion) override;

    ~ModelDAO();
private:
    MYSQL* mysql;
    char sql[SQL_MAX];		// 存储sql语句
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;
};
