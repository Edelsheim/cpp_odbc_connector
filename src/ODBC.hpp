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
	ODBC(const std::string driver, const std::string serverIP, const std::string serverPort, const std::string uid, const std::string pwd, const std::string databaseName) {
		this->sqlSuccess = false;
		this->sqlHDBC = SQL_NULL_HDBC;

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
	bool Connect(const std::string driver, const std::string serverIP, const std::string serverPort, const std::string uid, const std::string pwd, const std::string databaseName) {
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
	bool Connect(const std::string connectBuild) {
		bool isConnect = false;

		SQLSetEnvAttr(NULL, SQL_ATTR_CONNECTION_POOLING, SQL_CP_ONE_PER_DRIVER, SQL_IS_INTEGER);

		if (SQL_ERROR != SQLAllocHandle(SQL_HANDLE_ENV, NULL, &this->sqlHENV)) {
			SQLSetEnvAttr(this->sqlHENV, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER);
			if (SQL_ERROR != SQLAllocHandle(SQL_HANDLE_DBC, this->sqlHENV, &this->sqlHDBC)) {
				SQLSMALLINT short_result;
				SQLRETURN ret = SQLDriverConnect(this->sqlHDBC, NULL, (SQLCHAR*)connectBuild.c_str(), SQL_NTS, NULL, 0, &short_result, SQL_DRIVER_NOPROMPT);
				if (SQL_SUCCEEDED(ret)) {
					SQLHSTMT sqlHSTMT = SQL_NULL_HSTMT;
					ret = SQLAllocHandle(SQL_HANDLE_STMT, this->sqlHDBC, &sqlHSTMT);
					if (SQL_SUCCEEDED(ret)) {
						SQLFreeHandle(SQL_HANDLE_STMT, sqlHSTMT);
						isConnect = true;
						this->sqlSuccess = true;
					}
					else {
						throw "SQL Alloc STMT error";
					}
					sqlHSTMT = SQL_NULL_HSTMT;
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
	bool ExecQuery(const std::string query) {
		bool result = false;

		SQLHDBC sqlHDBC = SQL_NULL_HDBC;
		if (SQL_ERROR != SQLAllocHandle(SQL_HANDLE_DBC, this->sqlHENV, &sqlHDBC)) {
			SQLSMALLINT short_result;
			SQLRETURN ret = SQLDriverConnect(sqlHDBC, NULL, (SQLCHAR*)this->connectBuild.c_str(), SQL_NTS, NULL, 0, &short_result, SQL_DRIVER_NOPROMPT);
			if (SQL_SUCCEEDED(ret)) {
				SQLHSTMT sqlHSTMT = SQL_NULL_HSTMT;
				if (SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, sqlHDBC, &sqlHSTMT))) {
					result = this->ExecDirectSQL(sqlHSTMT, query);
					SQLFreeHandle(SQL_HANDLE_STMT, sqlHSTMT);
					sqlHSTMT = SQL_NULL_HSTMT;
				}
			}
		}

		if (sqlHDBC != SQL_NULL_HDBC)
			if (SQL_SUCCEEDED(SQLDisconnect(sqlHDBC)))
				SQLFreeHandle(SQL_HANDLE_DBC, sqlHDBC);
		sqlHDBC = SQL_NULL_HDBC;

		return result;
	}

	/// <summary>
	/// Get query result
	/// </summary>
	/// <param name="query">SQL query</param>
	/// <returns>[rows][column]</returns>
	std::vector<std::vector<std::string>> GetData(const std::string query) {
		std::vector<std::vector<std::string>> result;
		result.clear();

		SQLHDBC sqlHDBC = SQL_NULL_HDBC;
		if (SQL_ERROR != SQLAllocHandle(SQL_HANDLE_DBC, this->sqlHENV, &sqlHDBC)) {
			SQLSMALLINT short_result;
			SQLRETURN ret = SQLDriverConnect(sqlHDBC, NULL, (SQLCHAR*)this->connectBuild.c_str(), SQL_NTS, NULL, 0, &short_result, SQL_DRIVER_NOPROMPT);
			if (SQL_SUCCEEDED(ret)) {
				SQLHSTMT sqlHSTMT = SQL_NULL_HSTMT;
				if (SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, sqlHDBC, &sqlHSTMT))) {
					if (this->ExecDirectSQL(sqlHSTMT, query)) {
						SQLRETURN ret = SQLFetch(this->sqlHSTMT);
						while (1) {
							if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
								break;
							}

							std::vector<std::string> columnData = this->GetColumnData(this->sqlHSTMT);
							result.push_back(columnData);
							ret = SQLFetch(this->sqlHSTMT);
						}
					}
					SQLFreeHandle(SQL_HANDLE_STMT, sqlHSTMT);
				}
				sqlHSTMT = SQL_NULL_HSTMT;
			}
		}

		if (sqlHDBC != SQL_NULL_HDBC)
			if (SQL_SUCCEEDED(SQLDisconnect(sqlHDBC)))
				SQLFreeHandle(SQL_HANDLE_DBC, sqlHDBC);
		sqlHDBC = SQL_NULL_HDBC;
		return result;
	}

	/// <summary>
	/// Get query result
	/// </summary>
	/// <param name="query">SQL query</param>
	/// <returns>[rows]{first = field, secord = record}</returns>
	std::vector<std::map<std::string, std::string>> GetDataMap(const std::string query) {
		std::vector<std::map<std::string, std::string>> result;
		result.clear();

		SQLHDBC sqlHDBC = SQL_NULL_HDBC;
		if (SQL_ERROR != SQLAllocHandle(SQL_HANDLE_DBC, this->sqlHENV, &sqlHDBC)) {
			SQLSMALLINT short_result;
			SQLRETURN ret = SQLDriverConnect(sqlHDBC, NULL, (SQLCHAR*)this->connectBuild.c_str(), SQL_NTS, NULL, 0, &short_result, SQL_DRIVER_NOPROMPT);
			if (SQL_SUCCEEDED(ret)) {
				SQLHSTMT sqlHSTMT = SQL_NULL_HSTMT;
				if (SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, this->sqlHDBC, &sqlHSTMT))) {
					if (this->ExecDirectSQL(this->sqlHSTMT, query)) {
						SQLRETURN ret = SQLFetch(this->sqlHSTMT);
						while (1) {
							if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
								break;
							}

							std::map<std::string, std::string> columnDataMap = this->GetColumnDataMap(this->sqlHSTMT);
							result.push_back(columnDataMap);
							ret = SQLFetch(this->sqlHSTMT);
						}
					}
					SQLFreeHandle(SQL_HANDLE_STMT, sqlHSTMT);
				}
				sqlHSTMT = SQL_NULL_HSTMT;
			}
		}

		if (sqlHDBC != SQL_NULL_HDBC)
			if (SQL_SUCCEEDED(SQLDisconnect(sqlHDBC)))
				SQLFreeHandle(SQL_HANDLE_DBC, sqlHDBC);
		sqlHDBC = SQL_NULL_HDBC;

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
	SQLHENV sqlHENV;
	bool sqlSuccess;

	// Exec SQL
	// return Exec success or fail
	bool ExecDirectSQL(SQLHSTMT& hstmt, const std::string query) {
		if (hstmt == SQL_NULL_HSTMT) {
			return false;
		}

		SQLRETURN ret = SQLExecDirect(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);
		if (SQL_SUCCEEDED(ret)) {
			return true;
		}
		else {
			return false;
		}
	}

	bool ExecDirectSQL(SQLHSTMT& hstmt, const std::wstring query) {
		if (hstmt == SQL_NULL_HSTMT) {
			return false;
		}

		SQLRETURN ret = SQLExecDirectW(hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
		if (SQL_SUCCEEDED(ret)) {
			return true;
		}
		else {
			return false;
		}
	}

	// Get column count(field count)
	SQLSMALLINT GetColumnCount(SQLHSTMT& hstmt) {
		SQLSMALLINT cols;
		if (SQL_SUCCEEDED(SQLNumResultCols(hstmt, &cols))) {
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
	std::string GetFieldName(SQLHSTMT& hstmt, const SQLSMALLINT index = 1) {
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
	std::string GetFieldValue(SQLHSTMT& hstmt, const SQLSMALLINT index = 1) {
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

		if (this->sqlHDBC != SQL_NULL_HDBC) {
			if (SQL_SUCCEEDED(SQLDisconnect(this->sqlHDBC)))
				SQLFreeHandle(SQL_HANDLE_DBC, this->sqlHDBC);
		}
		this->sqlHDBC = SQL_NULL_HDBC;

		if (this->sqlHENV != SQL_NULL_HENV)
			SQLFreeHandle(SQL_HANDLE_ENV, this->sqlHENV);
		this->sqlHENV = SQL_NULL_HENV;
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