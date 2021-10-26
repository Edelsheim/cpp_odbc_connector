#pragma once

#include <string>
#include <vector>
#include <map>

// sql
#include <Windows.h>
#include <sqlext.h>

class ODBC {
public:
	/// <summary>
	/// Connect SQL server string builder
	/// </summary>
	std::string connectBuild;

	/// <summary>
	/// Empty constructor. Need Call Connect functions
	/// </summary>
	ODBC() {
		this->Close();
		this->sqlSuccess = false;
		this->sqlHDBC = SQL_NULL_HDBC;
		this->sqlHSTMT = SQL_NULL_HSTMT;
	};

	/// <summary>
	/// ODBC constructor
	/// </summary>
	/// <param name="driver">SQL Driver name</param>
	/// <param name="serverIP">SQL Server ip</param>
	/// <param name="serverPort">SQL Server port</param>
	/// <param name="uid">SQL Server user id</param>
	/// <param name="pwd">SQL Server password</param>
	/// <param name="databaseName">SQL Server database name</param>
	ODBC(std::string driver, std::string serverIP, std::string serverPort, std::string uid, std::string pwd, std::string databaseName) {
		this->sqlSuccess = false;
		this->sqlHDBC = SQL_NULL_HDBC;
		this->sqlHSTMT = SQL_NULL_HSTMT;

		// connect build string
		this->connectBuild = "DRIVER={" + driver + "};";
		this->connectBuild += "SERVER=" + serverIP + ", " + serverPort + ";";
		this->connectBuild += "UID=" + uid + ";";
		this->connectBuild += "PWD=" + pwd + ";";
		this->connectBuild += "DATABASE=" + databaseName;

		this->Connect(this->connectBuild);
	};

	~ODBC() {
		this->Close();
	};

	/// <summary>
	/// Connect SQL Server at local class variable connectBuild
	/// </summary>
	/// <returns>connected : true, other : false</returns>
	bool Connect() {
		return this->Connect(this->connectBuild);
	}

	/// <summary>
	/// Connect SQL Server
	/// </summary>
	/// <param name="driver">SQL Driver name</param>
	/// <param name="serverIP">SQL Server ip</param>
	/// <param name="serverPort">SQL Server port</param>
	/// <param name="uid">SQL Server user id</param>
	/// <param name="pwd">SQL Server password</param>
	/// <param name="databaseName">SQL Server database name</param>
	/// <returns>connected : true, other : false</returns>
	bool Connect(std::string driver, std::string serverIP, std::string serverPort, std::string uid, std::string pwd, std::string databaseName) {
		// connect build string
		this->connectBuild = "DRIVER={" + driver + "};";
		this->connectBuild += "SERVER=" + serverIP + ", " + serverPort + ";";
		this->connectBuild += "UID=" + uid + ";";
		this->connectBuild += "PWD=" + pwd + ";";
		this->connectBuild += "DATABASE=" + databaseName;
		return this->Connect();
	}

	/// <summary>
	/// Using string builder to connect SQL Server
	/// </summary>
	/// <param name="connectBuild"></param>
	/// <returns>connected : true, other : false</returns>
	bool Connect(std::string connectBuild) {
		bool isConnect = false;
		if (SQL_ERROR != SQLAllocEnv(&this->sqlHENV)) {
			SQLSetEnvAttr(this->sqlHENV, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
			if (SQL_ERROR != SQLAllocConnect(this->sqlHENV, &this->sqlHDBC)) {
				SQLSMALLINT short_result;
				SQLRETURN ret = SQLDriverConnect(this->sqlHDBC, NULL, (SQLCHAR*)connectBuild.c_str(), SQL_NTS, NULL, 0, &short_result, SQL_DRIVER_NOPROMPT);
				if (SQL_SUCCEEDED(ret)) {
					ret = SQLAllocStmt(this->sqlHDBC, &this->sqlHSTMT);
					if (SQL_SUCCEEDED(ret)) {
						isConnect = true;
						this->sqlSuccess = true;
					}
					else {
						throw "SQL Alloc STMT error";
					}
				}
				else {
					throw "SQL Driver Connect error";
				}
			}
			else {
				throw "SQL Alloc Connect error";
			}
		}
		else {
			throw "SQL Alloc Env error";
		}
		if (!isConnect)
			this->Close();

		return isConnect;
	}

	/// <summary>
	/// Run query.
	/// </summary>
	/// <param name="query">SQL query</param>
	/// <returns>success : true, fail : false</returns>
	bool ExecQuery(std::string query) {
		if (this->sqlHSTMT != SQL_NULL_HSTMT) SQLFreeStmt(this->sqlHSTMT, SQL_DROP);
		SQLRETURN ret = SQLAllocStmt(this->sqlHDBC, &this->sqlHSTMT);

		return this->ExecDirectSQL(this->sqlHSTMT, query);
	}

	/// <summary>
	/// Get query result
	/// </summary>
	/// <param name="query">SQL query</param>
	/// <returns>[rows][column]</returns>
	std::vector<std::vector<std::string>> GetData(std::string query) {
		std::vector<std::vector<std::string>> result;
		result.clear();

		if (this->sqlHSTMT != SQL_NULL_HSTMT) SQLFreeStmt(this->sqlHSTMT, SQL_DROP);
		SQLRETURN ret = SQLAllocStmt(this->sqlHDBC, &this->sqlHSTMT);

		if (SQL_SUCCEEDED(ret)) {
			if (this->ExecDirectSQL(this->sqlHSTMT, query)) {
				SQLRETURN ret = SQLFetch(this->sqlHSTMT);
				while (true) {
					if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
						break;
					}

					std::vector<std::string> columnData = this->GetColumnData(this->sqlHSTMT);
					result.push_back(columnData);
					ret = SQLFetch(this->sqlHSTMT);
				}
			}
		}
		return result;
	}

	/// <summary>
	/// Get query result
	/// </summary>
	/// <param name="query">SQL query</param>
	/// <returns>[rows]{first = field, secord = record}</returns>
	std::vector<std::map<std::string, std::string>> GetDataMap(std::string query) {
		std::vector<std::map<std::string, std::string>> result;
		result.clear();

		if (this->sqlHSTMT != SQL_NULL_HSTMT) SQLFreeStmt(this->sqlHSTMT, SQL_DROP);
		SQLRETURN ret = SQLAllocStmt(this->sqlHDBC, &this->sqlHSTMT);
		if (SQL_SUCCEEDED(ret)) {
			if (this->ExecDirectSQL(this->sqlHSTMT, query)) {
				SQLRETURN ret = SQLFetch(this->sqlHSTMT);
				while (true) {
					if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
						break;
					}

					std::map<std::string, std::string> columnDataMap = this->GetColumnDataMap(this->sqlHSTMT);
					result.push_back(columnDataMap);
					ret = SQLFetch(this->sqlHSTMT);
				}
			}
		}
		return result;
	}

	/// <summary>
	/// Return SQL Server connect
	/// </summary>
	/// <returns>connected : true, other : fail</returns>
	bool isOpen() {
		return sqlSuccess;
	}

private:
	SQLHDBC sqlHDBC;
	SQLHENV sqlHENV;
	SQLHSTMT sqlHSTMT;
	bool sqlSuccess;

	// Exec SQL
	// return Exec success or fail
	bool ExecDirectSQL(SQLHSTMT& hstmt, std::string query) {
		if (hstmt == SQL_NULL_HSTMT) {
			return false;
		}

		SQLRETURN ret = SQLExecDirect(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);
		if (SQL_SUCCEEDED(ret)) {
			return true;
		}
		return false;
	}

	bool ExecDirectSQL(SQLHSTMT& hstmt, std::wstring query) {
		if (hstmt == SQL_NULL_HSTMT) {
			return false;
		}

		SQLRETURN ret = SQLExecDirectW(hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
		if (SQL_SUCCEEDED(ret)) {
			return true;
		}
		return false;
	}

	// Get column count(field count)
	SQLSMALLINT GetColumnCount(SQLHSTMT& hstmt) {
		SQLSMALLINT cols;
		SQLRETURN ret = SQLNumResultCols(hstmt, &cols);
		if (SQL_SUCCEEDED(ret)) {
			return cols;
		}
		return 0;
	}

	// Get column vector data
	std::vector<std::string> GetColumnData(SQLHSTMT& hstmt) {
		std::vector<std::string> columnData;
		columnData.clear();

		SQLSMALLINT cols = this->GetColumnCount(hstmt);
		if (cols == 0) {
			return columnData;
		}

		for (int i = 1; i <= cols; i++) {
			std::string data = this->GetFieldValue(hstmt, i);
			data = this->TrimLeft(data, ' ');
			data = this->TrimRight(data, ' ');
			columnData.push_back(data);
		}
		return columnData;
	}

	// Get column map data
	// map<field name, field data>
	std::map<std::string, std::string> GetColumnDataMap(SQLHSTMT& hstmt) {
		std::map<std::string, std::string> columnDataMap;
		columnDataMap.clear();

		SQLSMALLINT cols = this->GetColumnCount(hstmt);
		if (cols == 0) {
			return columnDataMap;
		}

		for (int i = 1; i <= cols; i++) {
			std::string columnName = this->GetFieldName(hstmt, i);
			std::string columnData = this->GetFieldValue(hstmt, i);
			columnData = this->TrimLeft(columnData, ' ');
			columnData = this->TrimRight(columnData, ' ');
			columnDataMap.insert(std::make_pair(columnName, columnData));
		}

		return columnDataMap;
	}

	// get field name
	std::string GetFieldName(SQLHSTMT& hstmt, SQLSMALLINT index = 1) {
		std::string columnName;

		SQLCHAR columnName_c[1024] = { 0, };
		SQLSMALLINT columnNameLen;
		SQLSMALLINT columnType;
		SQLULEN columnSize;
		SQLSMALLINT columnScale;
		SQLSMALLINT columnNullable;
		SQLRETURN ret = SQLDescribeCol(hstmt, index, columnName_c, 1024, &columnNameLen, &columnType, &columnSize, &columnScale, &columnNullable);
		if (SQL_SUCCEEDED(ret)) {
			columnName = std::string(reinterpret_cast<char const*>(columnName_c));
		}
		else {
			columnName = "";
		}

		return columnName.c_str();
	}

	// get field value
	std::string GetFieldValue(SQLHSTMT& hstmt, SQLSMALLINT index = 1) {
		std::string data_str;

		char data_c[1024] = { 0, };
		SQLLEN outLen;
		SQLRETURN ret = SQLGetData(hstmt, index, SQL_CHAR, data_c, 1024, &outLen);
		if (SQL_SUCCEEDED(ret)) {
			data_str = std::string(data_c);
		}
		else {
			data_str = "";
		}

		return data_str;
	}

	void Close() {
		this->sqlSuccess = false;

		if (this->sqlHSTMT != SQL_NULL_HSTMT) SQLFreeStmt(this->sqlHSTMT, SQL_DROP);
		this->sqlHSTMT = SQL_NULL_HSTMT;

		if (this->sqlHENV != SQL_NULL_HENV) SQLFreeEnv(this->sqlHENV);
		this->sqlHENV = SQL_NULL_HENV;

		if (this->sqlHDBC != SQL_NULL_HDBC) SQLFreeConnect(this->sqlHDBC);
		this->sqlHDBC = SQL_NULL_HDBC;
	}

#pragma region Utils
	// trim right
	std::string TrimRight(std::string str, const char trim) {
		return str.erase(str.find_last_not_of(trim) + 1);
	}
	std::wstring TrimRight(std::wstring str, const wchar_t trim) {
		return str.erase(str.find_last_not_of(trim) + 1);
	}

	// trim left
	std::string TrimLeft(std::string str, const char trim) {
		return str.erase(0, str.find_first_not_of(trim));
	}
	std::wstring TrimLeft(std::wstring str, const wchar_t trim) {
		return str.erase(0, str.find_first_not_of(trim));
	}
#pragma endregion
};