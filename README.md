# C++ ODBC Connector

Windows C++ ODBC Connector

-----------------------

## Test & Build env

OS : Windows 10
Platform : Visual Studio 2019 (v142)
Windows SDK Version : 10.0
C : /std:C11
C++ : /std:C++14
Character Set : None

-----------------------

## Usage

```text
#include <memory>
#include <vector>
#include <map>
#include <string>

#include "ODBC.h"

// MS SQL Server

std::string driver = "SQL Server";
std::string ip = "127.0.0.1";
std::string port = "1433";
std::string uid = "sa";
std::string pwd = "MSSQLPassword123!";
std::string db = "dbName";

std::unique_ptr<ODBC> odbc = std::make_unique<ODBC>(driver, ip, port, uid, pwd, db);
if (odbc->isOpen()) {
    std::string query = "SELECT * FROM table";

    // return no field name vector<record>
    std::vector<std::vector<std::string>> data = odbc->GetData(query);
    for (int i = 0; i != data.size(); i++) {
        for (int j = 0; j != data[i].size(); j++) {
            printf("%s ", data[i][j].c_str());
        }
        printf("\n");
    }

    // return map<field, record> return
    std::vector<std::map<std::string, std::string>> data_map = odbc->GetDataMap(query);
    for (int i = 0; i != data_map.size(); i++) {
        std::map<std::string, std::string>::iterator iter = data_map[i].begin();
        std::map<std::string, std::string>::iterator end = data_map[i].end();
        
        while (iter != end) {
            printf("%s : %s\n", iter->first.c_str(), iter->second.c_str());
            iter++;
        }
        printf("----------\n");
    }
}
```
