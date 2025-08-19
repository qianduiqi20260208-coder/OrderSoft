#include "IOperationLog.h"

#include <mysql.h> // mysql文件
#include <stdio.h>
#include <iostream>
#include <string>

#define SQL_MAX 1024		// sql语句字符数组最大值

bool generateOperationLog(std::string, const Log&);