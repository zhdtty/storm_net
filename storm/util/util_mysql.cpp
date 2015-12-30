#include "util_mysql.h"

#include <assert.h>
#include <stdio.h>

namespace Storm {
MySqlResult::MySqlResult(MYSQL_RES* pRes)
	:m_pRes(pRes)
	,m_iRows(0)
	,m_iFields(0) {

	m_pRes = pRes;
	if (m_pRes == NULL) {
		return;
	}
	m_iRows   = mysql_num_rows(m_pRes);
	m_iFields = mysql_num_fields(m_pRes);

	MYSQL_FIELD* field;
	mysql_field_seek(m_pRes, 0);
	for (uint32_t i = 0; i < m_iFields; ++i) {
		field = mysql_fetch_field(m_pRes);
		m_mField2Index.insert(std::make_pair(field->name, i));
	}
}

MySqlResult::~MySqlResult() {
	if (m_pRes != NULL) {
		mysql_free_result(m_pRes);
	}
}

std::string MySqlResult::get(uint32_t iRow, uint32_t iField) {
	if (iRow >= m_iRows || iField >= m_iFields) {
		return std::string();
	}
	mysql_data_seek(m_pRes, iRow);
	MYSQL_ROW row = mysql_fetch_row(m_pRes);
	if (row == NULL) {
		return std::string();
	}
	if (row[iField] == NULL) {
		return std::string();
	}
	unsigned long* length = mysql_fetch_lengths(m_pRes);
	return std::string(row[iField], length[iField]);
}

std::string MySqlResult::get(uint32_t iRow, const std::string& sFieldName) {
	auto it = m_mField2Index.find(sFieldName);
	//std::map<std::string, uint32_t>::iterator it = m_mField2Index.find(sFieldName);
	if (it == m_mField2Index.end()) {
		return std::string();
	}
	return get(iRow, it->second);
}

MySqlConn::MySqlConn()
	:m_bConnected(false) {
}

MySqlConn::MySqlConn(const DBConfig& config)
	:m_config(config)
	,m_bConnected(false) {
}

MySqlConn::MySqlConn(const std::string& host,
				   	 const std::string& user,
				     const std::string& passwd,
				     const std::string& db,
				     const std::string& charset,
				     uint32_t port)
	:m_bConnected(false) {
	m_config.sHost = host;
	m_config.sUser = user;
	m_config.sPasswd = passwd;
	m_config.sDBName = db;
	m_config.sCharset = charset;
	m_config.iPort = port;
}

MySqlConn::~MySqlConn() {
	disconnect();
}

void MySqlConn::setConfig(const DBConfig& config) {
	disconnect();
	m_config = config;
}

inline void MySqlConn::disconnect() {
	if (m_bConnected) {
		mysql_close(m_mysql);
		m_mysql = NULL;
		m_bConnected = false;
	}
}

inline void MySqlConn::printError() {
	printf("mysql error: %u (%s) %s\n", mysql_errno(m_mysql), mysql_sqlstate(m_mysql),
		    mysql_error(m_mysql));
}

void MySqlConn::connect() {
	if (m_bConnected) {
		return ;
	}
	m_mysql = mysql_init(NULL);
	if (m_mysql == NULL) {
		throw std::runtime_error("mysql_init: " + string(mysql_error(m_mysql)));
	}

	if (mysql_real_connect(m_mysql, m_config.sHost.c_str(), m_config.sUser.c_str(), m_config.sPasswd.c_str(), m_config.sDBName.c_str(),
						   m_config.iPort, NULL, 0) == NULL) {
		mysql_close(m_mysql);
		m_mysql = NULL;
		throw std::runtime_error("mysql_real_connect: " + string(mysql_error(m_mysql)));
	}

	char value = 1;
	mysql_options(m_mysql, MYSQL_OPT_RECONNECT, &value);
	if (mysql_set_character_set(m_mysql, m_config.sCharset.c_str()) != 0) {
		//printError();
		throw std::runtime_error("mysql: " + string(mysql_error(m_mysql)));
		mysql_set_character_set(m_mysql, "utf8");
	}
	m_bConnected = true;
}

void MySqlConn::execute(const std::string& sql) {
	if (!m_bConnected) {
		connect();
	}

	int ret = mysql_real_query(m_mysql, sql.c_str(), sql.length());
	if (ret != 0) {
		//printError();
		throw std::runtime_error("mysql_real_query: " + string(mysql_error(m_mysql)) + ", " + sql);
	}
}

MySqlResult::ptr MySqlConn::query(const std::string& sql) {
	execute(sql);
	MYSQL_RES* result = mysql_store_result(m_mysql);
	if (result == NULL) {
		if (mysql_field_count(m_mysql) != 0) {
			//printError();
			throw std::runtime_error("mysql_store_result: " + string(mysql_error(m_mysql)) + ", " + sql);
		}
	}
	//printResult(result);
	return MySqlResult::ptr(new MySqlResult(result));
}

string MySqlConn::escape(const string &s) {
	if (!m_bConnected) {
		connect();
	}

	string r;
	r.resize(s.size() * 2 + 1);
	mysql_real_escape_string(m_mysql, &r[0], s.c_str(), s.size());
	r.resize(strlen(r.c_str()));
	return r;
}

uint64_t MySqlConn::getInsertId() {
	if (m_mysql != NULL) {
		return mysql_insert_id(m_mysql);
	}
	return 0;
}

uint64_t MySqlConn::getAffectedRows() {
	if (m_mysql != NULL) {
		return mysql_affected_rows(m_mysql);
	}
	return 0;
}

//测试用
void MySqlConn::printResult(MYSQL_RES* result) {
	int iFields = mysql_num_fields(result);

	MYSQL_FIELD* field;
	mysql_field_seek(result, 0);
	for (int i = 0; i < iFields; ++i) {
		field = mysql_fetch_field(result);
		if (i > 0) {
			printf("\t");
		}
		printf("%s", field->name);
	}
	printf("\n");


	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)) != NULL) {
		for (int i = 0; i < iFields; ++i) {
			if (i > 0) {
				printf("\t");
			}
			printf("%s", row[i] != NULL ? row[i] : "NULL");
		}
		printf("\n");
	}
}

}

