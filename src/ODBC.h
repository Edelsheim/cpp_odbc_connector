#pragma once

#include <string>
#include <vector>
#include <map>

// sql
#include <Windows.h>
#include <sqlext.h>

class ODBC {
public:
	std::string connectBuild;

	ODBC() {
		this->Close();
		this->sqlSuccess = false;
		this->sqlHDBC = SQL_NULL_HDBC;
		this->sqlHSTMT = SQL_NULL_HSTMT;
	};

	ODBC(std::string driver, std::string serverIP, std::string serverPort, std::string uid, std::string pwd, std::string databaseName) {
		bool isClose = true;
		this->sqlSuccess = false;
		this->sqlHDBC = SQL_NULL_HDBC;
		this->sqlHSTMT = SQL_NULL_HSTMT;

		// connect build string
		connectBuild = "DRIVER={" + driver + "};";
		connectBuild += "SERVER=" + serverIP + ", " + serverPort + ";";
		connectBuild += "UID=" + uid + ";";
		connectBuild += "PWD=" + pwd + ";";
		connectBuild += "DATABASE=" + databaseName;

		if (SQL_ERROR != SQLAllocEnv(&this->sqlHENV)) {
			SQLSetEnvAttr(this->sqlHENV, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
			if (SQL_ERROR != SQLAllocConnect(this->sqlHENV, &this->sqlHDBC)) {
				SQLSMALLINT short_result;
				SQLRETURN ret = SQLDriverConnect(this->sqlHDBC, NULL, (SQLCHAR*)connectBuild.c_str(), SQL_NTS, NULL, 0, &short_result, SQL_DRIVER_NOPROMPT);
				if (SQL_SUCCEEDED(ret)) {
					ret = SQLAllocStmt(this->sqlHDBC, &this->sqlHSTMT);
					if (SQL_SUCCEEDED(ret)) {
						isClose = false;
						this->sqlSuccess = true;
					}
				}
			}
		}
		if (isClose) {
			this->Close();
		}
	};

	~ODBC() {
		this->Close();
	};

	void ExecDirectSQL(std::string sql) {
		if (this->sqlHSTMT != SQL_NULL_HSTMT) SQLFreeStmt(this->sqlHSTMT, SQL_DROP);
		SQLRETURN ret = SQLAllocStmt(this->sqlHDBC, &this->sqlHSTMT);

		this->ExecDirectSQL(this->sqlHSTMT, sql);
	}

	std::vector<std::vector<std::string>> GetData(std::string sql) {
		std::vector<std::vector<std::string>> result;
		result.clear();

		if (this->sqlHSTMT != SQL_NULL_HSTMT) SQLFreeStmt(this->sqlHSTMT, SQL_DROP);
		SQLRETURN ret = SQLAllocStmt(this->sqlHDBC, &this->sqlHSTMT);

		if (SQL_SUCCEEDED(ret)) {
			if (this->ExecDirectSQL(this->sqlHSTMT, sql)) {
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

	std::vector<std::map<std::string, std::string>> GetDataMap(std::string sql) {
		std::vector<std::map<std::string, std::string>> result;
		result.clear();

		if (this->sqlHSTMT != SQL_NULL_HSTMT) SQLFreeStmt(this->sqlHSTMT, SQL_DROP);
		SQLRETURN ret = SQLAllocStmt(this->sqlHDBC, &this->sqlHSTMT);
		if (SQL_SUCCEEDED(ret)) {
			if (this->ExecDirectSQL(this->sqlHSTMT, sql)) {
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
		}
		return result;
	}

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
	bool ExecDirectSQL(SQLHSTMT& hstmt, std::string sql) {
		if (hstmt == SQL_NULL_HSTMT) {
			return false;
		}

		SQLRETURN ret = SQLExecDirect(hstmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
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

	// trim left
	std::string TrimLeft(std::string str, const char trim) {
		return str.erase(0, str.find_first_not_of(trim));
	}
#pragma endregion
};