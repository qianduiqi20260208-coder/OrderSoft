#include "IModelDAO.h"
#include <mysql.h>
#define SQL_MAX 1024		// sql语句字符数组最大值

class ModelDAO :public IModelDAO
{
public:
    ModelDAO(MYSQL* ms);
    std::vector<std::string> getModel()override;
    std::vector<std::string> getModelVersionByModel(std::string modelName)override;
    

    bool addModelVersion(const std::string& model,const std::string& modelVersion) override;

    std::vector<std::vector<std::string>> getModelVersionInfoByModelPaged(const std::string& model,int page,int pageSize) override;

    int getModelVersionCount(std::string model) override;

    std::vector<std::pair<std::string,std::string>> selectModelUpdateNotesByModelName(std::string) override;

    ~ModelDAO();
private:
    MYSQL* mysql;
    char sql[SQL_MAX];		// 存储sql语句
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;
};
