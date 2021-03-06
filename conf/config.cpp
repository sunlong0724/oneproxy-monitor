/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @FileName: config.cpp
* @Description:
*  	1. 增加构造函数config()中的配置信息
*  	2. 增加print_config中对新加配置的打印信息
*  	3. 修改handle_args函数， 添加新配置的支持。
* All rights Reserved, Designed By huih
* @Company: onexsoft
* @Author: hui
* @Version: V1.0
* @Date: 2016年8月10日
*
*/

#include "config.h"
#include "simpleini.h"
#include "tool.h"
#include "systemapi.h"

#include <getopt.h>

#define IMPLEMENT_CVTBOOL_FUNC(className, funcName) \
void className::funcName(std::string value, ConfigKeyValue& cf) \
{ \
	typedef void (className::*SetBoolValue)(bool); \
	SetBoolValue func = (SetBoolValue)cf.setFunc; \
	if ((strncasecmp_s(value, std::string("true")) == 0) \
			|| (strncasecmp_s(value, std::string("1")) == 0)) {\
		(this->*func)(true);\
	} else { \
		(this->*func)(false); \
	}\
}

#define IMPLEMENT_CVTINT_FUNC(className, funcName) \
void className::funcName(std::string value, ConfigKeyValue& cf) \
{ \
	typedef void (className::*SetIntValue)(int); \
	SetIntValue func = (SetIntValue)cf.setFunc; \
	int v = atoi(value.c_str()); \
	(this->*func)(v); \
}

#define IMPLEMENT_CVTLOGGER_FUNC(className, funcName) \
void className::funcName(std::string value, ConfigKeyValue& cf) \
{ \
	typedef void(className::*SetLoggerValue)(Logger::loggerLevel); \
	SetLoggerValue func = (SetLoggerValue)cf.setFunc; \
	if (strncasecmp_s(value, std::string("debug")) == 0) { \
		(this->*func)(Logger::DEBUG); \
	} else if (strncasecmp_s(value, std::string("info")) == 0) { \
		(this->*func)(Logger::INFO); \
	} else if (strncasecmp_s(value, std::string("warning")) == 0) {\
		(this->*func)(Logger::WARNING); \
	} else if (strncasecmp_s(value, std::string("error")) == 0) { \
		(this->*func)(Logger::ERR); \
	} else if (strncasecmp_s(value, std::string("fatal")) == 0) { \
		(this->*func)(Logger::FATAL); \
	} else { \
		logs(Logger::ERR, "unkown level(%s)", value.c_str()); \
	} \
}

#define IMPLEMENT_CVTSTRING_FUNC(className, funcName) \
void className::funcName(std::string value, ConfigKeyValue& cf) \
{ \
	typedef void (className::*SetStringValue)(std::string); \
	SetStringValue func = (SetStringValue)cf.setFunc; \
	(this->*func)(value); \
}

IMPLEMENT_CVTSTRING_FUNC(DataBase, cvtString)
IMPLEMENT_CVTINT_FUNC(DataBase, cvtInt)

IMPLEMENT_CVTSTRING_FUNC(DataBaseGroup, cvtString)
IMPLEMENT_CVTINT_FUNC(DataBaseGroup, cvtInt)
IMPLEMENT_CVTBOOL_FUNC(DataBaseGroup, cvtBool)

int DataBaseGroup::set_dataBaseGroupVec(std::vector<DataBase*>& dbVec,
		const std::string dbGroupName,
		std::vector<DataBase*>& dbGVec)
{
	if (dbGroupName.length() > 0) {
		size_t pos = 0;
		do {
			size_t curPos = dbGroupName.find_first_of(',', pos);
			if (curPos == dbGroupName.npos) {
				curPos = dbGroupName.length();
			}

			std::string dbLabelName = dbGroupName.substr(pos, curPos - pos);
			dbLabelName = Tool::stringTrim(dbLabelName);
			std::vector<DataBase*>::iterator it = dbVec.begin();
			for(; it != dbVec.end(); ++it) {
				if (strncasecmp_s((*it)->get_labelName(), dbLabelName) == 0) {
					dbGVec.push_back((DataBase*)(*it));
					break;
				}
			}
			if (it == dbVec.end()) {
				std::cout << "no find " << dbLabelName.c_str() << " database info, error" << std::endl;
			}

			if (curPos >= dbGroupName.length())
				break;

			pos = curPos + 1;
		} while(1);
	}
	return 0;
}
void Config::update_globalTime() {
	this->m_globalMillisecondTime = SystemApi::system_millisecond();
}
Config::Config()
{
	//add oneproxy default config.
#define add_oneproxyConfig(key, defaultv, cvtf, setf) add_config(this->oneproxyCfg, key, defaultv, (CVTFunc)cvtf, (SetFunc)setf)
	add_oneproxyConfig("logfile", "oneproxy_log.log", &Config::cvtString, &Config::set_logFilePath);
	add_oneproxyConfig("pidfile", "oneproxy_pid.pid", &Config::cvtString, &Config::set_pidFilePath);
	add_oneproxyConfig("listen_addr", "0.0.0.0", &Config::cvtString, &Config::set_oneproxyAddr);
	add_oneproxyConfig("listen_port", "9999", &Config::cvtString, &Config::set_oneproxyPort);
	add_oneproxyConfig("httpserver_addr", "0.0.0.0", &Config::cvtString, &Config::set_httpServerAddr);
	add_oneproxyConfig("httpserver_port", "8080", &Config::cvtInt, &Config::set_httpServerPort);
	add_oneproxyConfig("log_level", "error", &Config::cvtLogger, &Config::set_loggerLevel);
	add_oneproxyConfig("data_dump", "false", &Config::cvtBool, &Config::set_dumpData);
	add_oneproxyConfig("log_sql", "false", &Config::cvtBool, &Config::set_logSql);
	add_oneproxyConfig("logclientsql", "false", &Config::cvtBool, &Config::set_logClientSql);
	add_oneproxyConfig("tryconnservertimes", "3", &Config::cvtInt, &Config::set_tryConnServerTimes);
	add_oneproxyConfig("maxconnectnum", "2000", &Config::cvtInt, &Config::set_maxConnectNum);
	add_oneproxyConfig("keepalive", "false", &Config::cvtBool, &Config::set_keepAlive);
	add_oneproxyConfig("vip_ifname", "", &Config::cvtString, &Config::set_vipIfName);
	add_oneproxyConfig("vip_address", "", &Config::cvtString, &Config::set_vipAddress);
	add_oneproxyConfig("threadnum", "0", &Config::cvtInt, &Config::set_threadNum);
	add_oneproxyConfig("clientusername", "admin", &Config::cvtString, &Config::set_clientUserName);
	add_oneproxyConfig("clientpassword", "0000", &Config::cvtString, &Config::set_clientPassword);
	add_oneproxyConfig("poolconncheckactivetime", "5", &Config::cvtInt, &Config::set_poolConnCheckActiveTime);
	add_oneproxyConfig("poolconntimeoutreleasetime", "60", &Config::cvtInt, &Config::set_poolConnTimeoutReleaseTime);
	add_oneproxyConfig("connecttimeout", "86400", &Config::cvtInt, &Config::set_connectTimeOut);//one day,unit: second.
	add_oneproxyConfig("acceptthreadnum", "1", &Config::cvtInt, &Config::set_acceptThreadNum);
	add_oneproxyConfig("listenbacklog", "1000", &Config::cvtInt, &Config::set_listenBackLog);
	add_oneproxyConfig("usemonitor", "false", &Config::cvtBool, &Config::set_useMonitor);
	add_oneproxyConfig("monitorportclass", "SSProtocol:1433;PGProtocol:5432;", &Config::cvtString, &Config::set_monitorPortClass);
	add_oneproxyConfig("monitordevicename", "", &Config::cvtString, &Config::set_monitorDeviceName);
	add_oneproxyConfig("monitordumpdata", "false", &Config::cvtBool, &Config::set_monitorDumpData);
	add_oneproxyConfig("statsuserdb", "false", &Config::cvtBool, &Config::set_statsUserDB);
	add_oneproxyConfig("monitorthreadnum", "4", &Config::cvtInt, &Config::set_monitorThreadNum);
	add_oneproxyConfig("monitorcaptureaddress", "", &Config::cvtString, &Config::set_monitorCaptureAddress);
	add_oneproxyConfig("monitorautogetdesip", "true", &Config::cvtBool, &Config::set_monitorAutoGetDesIp);
#undef add_oneproxyConfig

#define add_dbConfig(db, key, defaultv, cvtf, setf) add_config(db, key, defaultv, (CVTFunc)cvtf, (SetFunc)setf)
	//增加后端服务器信息
	std::vector<ConfigKeyValue> fakedb;
	add_dbConfig(fakedb, "labelname", "database_fakedb", &DataBase::cvtString, &DataBase::set_labelName);
	add_dbConfig(fakedb, "host", "127.0.0.1", &DataBase::cvtString, &DataBase::set_addr);
	add_dbConfig(fakedb, "port", "1433", &DataBase::cvtInt, &DataBase::set_port);
	add_dbConfig(fakedb, "weightvalue", "0", &DataBase::cvtInt, &DataBase::set_weightValue);
	add_dbConfig(fakedb, "username", "sa", &DataBase::cvtString, &DataBase::set_userName);
	add_dbConfig(fakedb, "password", "0000", &DataBase::cvtString, &DataBase::set_password);
	add_dbConfig(fakedb, "connectnumlimit", "1000", &DataBase::cvtInt, &DataBase::set_connectNumLimit);
	this->dbInfoCfg.push_back(fakedb);

	//前端与后端服务信息进行关联
	std::vector<ConfigKeyValue> dbg;
	add_dbConfig(dbg, "labelname", "group_fake", &DataBaseGroup::cvtString, &DataBaseGroup::set_labelName);
	add_dbConfig(dbg, "dbmastergroup", "database_fakedb", &DataBaseGroup::cvtString, &DataBaseGroup::set_dbMasterGroup);
	add_dbConfig(dbg, "dbslavegroup", "", &DataBase::cvtString, &DataBaseGroup::set_dbSlaveGroup);
	add_dbConfig(dbg, "classname", "FakeProtocol", &DataBaseGroup::cvtString, &DataBaseGroup::set_className);
	add_dbConfig(dbg, "frontport", "0", &DataBaseGroup::cvtInt, &DataBaseGroup::set_frontPort);
	add_dbConfig(dbg, "passwordseparate", "false", &DataBaseGroup::cvtBool, &DataBaseGroup::set_passwordSeparate);
	add_dbConfig(dbg, "readslave", "false", &DataBaseGroup::cvtBool, &DataBaseGroup::set_readSlave);
	add_dbConfig(dbg, "useconnectionpool", "false", &DataBaseGroup::cvtBool, &DataBaseGroup::set_useConnectionPool);
	add_dbConfig(dbg, "clientauthmethod", "md5", &DataBaseGroup::cvtString, &DataBaseGroup::set_clientAuthMethod);
	add_dbConfig(dbg, "clearbackendconnection", "false", &DataBaseGroup::cvtBool, &DataBaseGroup::set_clearBackendConnection);
	add_dbConfig(dbg, "usedatabaseconnecttobalace", "false", &DataBaseGroup::cvtBool, &DataBaseGroup::set_useDatabaseConnectToBalace);
	add_dbConfig(dbg, "usemasterassalve", "false", &DataBaseGroup::cvtBool, &DataBaseGroup::set_useMasterAsSalve);
	this->dbGroupCfg.push_back(dbg);
#undef add_dbConfig

	//init global time
	this->update_globalTime();
}

Config::~Config() {
	{
		std::vector<DataBaseGroup*>::iterator it =  dbGroupVector.begin();
		for(; it != dbGroupVector.end();) {
			DataBaseGroup* pit = *it;
			++it;
			if (pit) {
				delete pit;
			}
		}
	}

	{
		std::vector<DataBase*>::iterator it =  dbVector.begin();
		for(; it != dbVector.end();) {
			DataBase* pit = *it;
			++it;
			if (pit) {
				delete pit;
			}
		}
	}
}

void Config::add_config(std::vector<ConfigKeyValue>& db, std::string key, std::string dvalue, CVTFunc cf, SetFunc sf)
{
	db.push_back(ConfigKeyValue(key, dvalue, cf, sf));
}

void Config::default_oneproxyConfig()
{
	std::vector<ConfigKeyValue>::iterator it = this->oneproxyCfg.begin();
	for(; it != this->oneproxyCfg.end(); ++it) {
		CVTFunc func = (*it).cvtFunc;
		(this->*func)((*it).defaultValue, (*it));
	}
}

void Config::default_database()
{
	std::list<std::vector<ConfigKeyValue> >::iterator it = this->dbInfoCfg.begin();
	for (; it != this->dbInfoCfg.end(); ++it) {
		DataBase* db = new DataBase();

		std::vector<ConfigKeyValue>& dbcfg = (*it);
		std::vector<ConfigKeyValue>::iterator dbit = dbcfg.begin();
		for (; dbit != dbcfg.end(); ++dbit) {
			CVTFunc func = (*dbit).cvtFunc;
			(db->*func)((*dbit).defaultValue, (*dbit));
		}
		this->set_database(db);
	}
}

void Config::default_databaseGroup()
{
	std::list<std::vector<ConfigKeyValue> >::iterator it = this->dbGroupCfg.begin();
	for (; it != this->dbGroupCfg.end(); ++it) {
		DataBaseGroup* dbg = new DataBaseGroup();

		std::vector<ConfigKeyValue>& dbcfg = (*it);
		std::vector<ConfigKeyValue>::iterator dbit = dbcfg.begin();
		for (; dbit != dbcfg.end(); ++dbit) {
			CVTFunc func = (*dbit).cvtFunc;
			(dbg->*func)((*dbit).defaultValue, (*dbit));
		}
		this->set_dataBaseGroup(dbg);
	}
}

void Config::default_config()
{
	config()->default_oneproxyConfig();
	config()->default_database();
	config()->default_databaseGroup();
}

void Config::print_config()
{
	logs(Logger::INFO, "maxConnectNum: %d", (int)this->m_maxConnectNum);
	logs(Logger::INFO, "loggerLevel: %d", this->m_loggerLevel);
	logs(Logger::INFO, "tryConnServerTimes: %d", this->m_tryConnServerTimes);
	logs(Logger::INFO, "oneproxyAddr: %s", this->m_oneproxyAddr.c_str());
	logs(Logger::INFO, "oneproxyPort: %s", this->m_oneproxyPort.c_str());
	{
		std::set<unsigned int>::iterator it = this->m_oneproxyPortSet.begin();
		for (; it != this->m_oneproxyPortSet.end(); ++it) {
			logs(Logger::INFO, "port: %u", *it);
		}
	}
	logs(Logger::INFO, "dumpData: %d", this->m_dumpData);
	logs(Logger::INFO, "logSql: %d", this->m_logSql);
	logs(Logger::INFO, "logclientsql: %d", this->m_logClientSql);
	logs(Logger::INFO, "httpServerAddr: %s", this->m_httpServerAddr.c_str());
	logs(Logger::INFO, "httpServerPort: %d", this->m_httpServerPort);
	logs(Logger::INFO, "logFilePath: %s", this->m_logFilePath.c_str());
	logs(Logger::INFO, "pidFilePath: %s", this->m_pidFilePath.c_str());
	logs(Logger::INFO, "keepAlive: %d", this->m_keepAlive);
	logs(Logger::INFO, "vipIfName: %s", this->m_vipIfName.c_str());
	logs(Logger::INFO, "vipAddress: %s", this->m_vipAddress.c_str());
	logs(Logger::INFO, "threadNum: %d", this->m_threadNum);
	logs(Logger::INFO, "clientUserName: %s", this->m_clientUserName.c_str());
	logs(Logger::INFO, "clientPassword: %s", this->m_clientPassword.c_str());
	logs(Logger::INFO, "poolConnCheckActiveTime: %d", this->m_poolConnCheckActiveTime);
	logs(Logger::INFO, "poolConnTimeoutReleaseTime: %d", this->m_poolConnTimeoutReleaseTime);
	logs(Logger::INFO, "acceptThreadNum: %d", this->m_acceptThreadNum);
	logs(Logger::INFO, "listenbacklog: %d", this->m_listenBackLog);
	logs(Logger::INFO, "useMonitor: %d", this->m_useMonitor);
	logs(Logger::INFO, "monitorPortClass:%s", this->m_monitorPortClass.c_str());
	logs(Logger::INFO, "monitorDeviceName:%s", this->m_monitorDeviceName.c_str());
	logs(Logger::INFO, "monitordumpdata: %d", this->m_monitorDumpData);
	logs(Logger::INFO, "statsuserdb:%d", this->m_statsUserDB);
	logs(Logger::INFO, "monitorthreadnum:%d", this->m_monitorThreadNum);
	logs(Logger::INFO, "monitorcaptureaddress:%s", this->m_monitorCaptureAddress.c_str());
	logs(Logger::INFO, "monitorautogetdesip: %d", this->m_monitorAutoGetDesIp);

	std::vector<DataBase*>::iterator pit = dbVector.begin();
	for (; pit != dbVector.end(); ++pit) {
		DataBase* it = (DataBase*)(*pit);
		logs(Logger::INFO, "database: labelName: %s, addr: %s, port: %d, weightValue: %d, "
				"connectNum: %d, userName: %s, password: %s, connectNumLimit: %d",
				it->get_labelName().c_str(), it->get_addr().c_str(), it->get_port(),
				it->get_weightValue(), it->get_connectNum(), it->get_userName().c_str(),
				it->get_password().c_str(), it->get_connectNumLimit());
	}

	std::vector<DataBaseGroup*>::iterator pitg = dbGroupVector.begin();
	for (; pitg != dbGroupVector.end(); ++pitg) {
		DataBaseGroup* itg = (DataBaseGroup*)(*pitg);
		logs(Logger::INFO, "dbGroup: labelName: %s, className: %s, master: %s, slave: %s, "
				"frontPort: %d, passwordSeparate: %d, readSlave: %d, useConnectionPool: %d,"
				"clearBackendConnection: %d, usedatabaseconnecttobalace: %d, useMasterAsSalve: %d",
				itg->get_labelName().c_str(), itg->get_className().c_str(),
				itg->get_dbMasterGroup().c_str(), itg->get_dbSlaveGroup().c_str(), itg->get_frontPort(),
				itg->get_passwordSeparate(), itg->get_readSlave(), itg->get_useConnectionPool(),
				itg->get_clearBackendConnection(), itg->get_useDatabaseConnectToBalace(),
				itg->get_useMasterAsSalve());
	}
}

void Config::handle_ports()
{
	if (this->m_oneproxyPort.size() <= 0)
		return;

	size_t pos = 0;
	do {
		size_t curPos = this->m_oneproxyPort.find_first_of(',', pos);
		if (curPos == this->m_oneproxyPort.npos) {
			curPos = this->m_oneproxyPort.length();
		}

		unsigned int port = (unsigned int)atoi(this->m_oneproxyPort.substr(pos, curPos - pos).c_str());
		if (port > 0) {
			this->m_oneproxyPortSet.insert(port);
		}

		if (curPos >= this->m_oneproxyPort.length())
			break;

		pos = curPos + 1;
	} while(1);
}

void Config::handle_monitorPortClass()
{
	//for example:SSProtocol:1433;PGProtocol:5432,5433;FakeProtocol:-1

	if (this->m_monitorPortClass.length() <= 0)
		return;

	size_t pos = 0;
	do{
		size_t curPos = this->m_monitorPortClass.find_first_of(';', pos);
		if (curPos == this->m_monitorPortClass.npos) {
			curPos = this->m_monitorPortClass.length();
		}

		size_t nPos = this->m_monitorPortClass.find_first_of(':', pos);
		if (nPos >= curPos) {
			//error
			logs(Logger::ERR,
					"monitorPortClass(%s) format error, normal format exmaple: className:port1,port2;className:port;...",
					this->m_monitorPortClass.c_str());
			return;
		}
		std::string className = this->m_monitorPortClass.substr(pos, nPos - pos);
		if (className.length() <= 0) {
			logs(Logger::ERR, "class name is empty error, monitorPortClass(%s)", this->m_monitorPortClass.c_str());
			return;
		}

		size_t pPos = nPos + 1;
		while(pPos < curPos) {
			size_t cur_pPos = this->m_monitorPortClass.find_first_of(',', pPos);
			unsigned int port = (unsigned int)atoi(this->m_monitorPortClass.substr(pPos, cur_pPos - pPos).c_str());
			if (port == 0) {
				logs(Logger::ERR, "className(%s) 'port is 0 error,  monitorPortClass(%s)",
						className.c_str(), this->m_monitorPortClass.c_str());
				return;
			}
			this->monitorPortClass[port] = className;
			pPos = cur_pPos + 1;
		}
		pos = curPos + 1;
	}while(pos < this->m_monitorPortClass.length());
}

int Config::handle_config()
{
	unsigned int cpuNum = SystemApi::system_cpus();
	if (this->get_threadNum() <= 0 && cpuNum > 0) {
		this->set_threadNum(cpuNum);
	}

	if (this->get_acceptThreadNum() <= 0 || cpuNum <= 1) {
		this->set_acceptThreadNum(1);
	}

	if (this->get_listenBackLog() <= 128) {
		this->set_listenBackLog(128);
	}

	if (this->get_tryConnServerTimes() <= 0) {
		this->set_tryConnServerTimes(1);
	}

	if (this->get_maxConnectNum() <= 0) {
		this->set_maxConnectNum(1);
	}

	if (this->get_poolConnCheckActiveTime() <= 0) {
		this->set_poolConnCheckActiveTime(1);
	}
	if (this->get_poolConnTimeoutReleaseTime() <= 0) {
		this->set_poolConnTimeoutReleaseTime(1);
	}
	if (this->get_connectTimeOut() <= 0) {
		this->set_connectTimeOut(1);
	}


	//处理多个端口号的情况。
	this->handle_ports();

	//转换数据库组中的配置信息
	this->handle_dataBaseGroup();

	//判断是否有数据库信息
	if (this->get_dataBaseGroupSize() <= 0) {
		logs(Logger::ERR, "no database group error");
		return -1;
	}

	//forbit PGProtocol, FakeProtocol use passwordSeparate.
	//std::string pgp = std::string("PGProtocol");
	std::string fp = std::string("FakeProtocol");
	std::vector<DataBaseGroup*>::iterator pit = dbGroupVector.begin();
	for(; pit != dbGroupVector.end(); ++pit) {
		DataBaseGroup* it = (DataBaseGroup*)(*pit);
		if (this->get_useMonitor()) {//use monitor, close all function.
			it->set_passwordSeparate(false);
			it->set_readSlave(false);
			it->set_useConnectionPool(false);
		}

		if (it->get_className().size() && strncmp_s(it->get_className(), fp) == 0) {
			it->set_passwordSeparate(false);
			it->set_readSlave(false);
			it->set_useConnectionPool(false);
		}

		//if passwordSeparate is false, forbit use readSlave and use connectionPool
		if (!it->get_passwordSeparate()) {
			it->set_readSlave(false);
			it->set_useConnectionPool(false);
		}
	}

	//use monitor
	this->handle_monitorPortClass();

	return 0;
}

int Config::handle_dataBaseGroup()
{
	//根据DatabaseGroup中的dbMasterGroup设置dbMasterGroupVec
	//根据DatabaseGroup中的dbSlaveGroup设置dbSlaveGroupVec
	std::vector<DataBaseGroup*>::iterator pit = dbGroupVector.begin();
	for (; pit != dbGroupVector.end(); ++pit) {
		DataBaseGroup* it = (*pit);
		DataBaseGroup::set_dataBaseGroupVec(this->dbVector, it->get_dbMasterGroup(), it->get_dbMasterGroupVec());
		DataBaseGroup::set_dataBaseGroupVec(this->dbVector, it->get_dbSlaveGroup(), it->get_dbSlaveGroupVec());

		//把主数据库和salve数据的再保存一份到dbGroupVec中
		std::vector<DataBase*>& dbGVec = it->get_dbGroupVec();
		std::vector<DataBase*>::iterator tit = it->get_dbMasterGroupVec().begin();
		for(; tit != it->get_dbMasterGroupVec().end(); ++tit) {
			dbGVec.push_back((*tit));
		}

		tit = it->get_dbSlaveGroupVec().begin();
		for(; tit != it->get_dbSlaveGroupVec().end(); ++tit) {
			dbGVec.push_back((*tit));
		}
	}

	//去掉没有主数据库信息的数据组信息
	pit = dbGroupVector.begin();
	for (; pit != dbGroupVector.end();) {
		DataBaseGroup* it = (DataBaseGroup*)(*pit);
		if (it->get_dbMasterGroupVec().size() <= 0) {
			logs(Logger::ERR, "group lable[%s] no master database and remove it error", it->get_labelName().c_str());
			pit = dbGroupVector.erase(pit);
		} else {
			++pit;
		}
	}
	return 0;
}

MutexLock Config::mutexLock;
Config* Config::get_config()
{
	static Config* config = NULL;
	if (config == NULL) {
		Config::mutexLock.set_name("config_lock");
		Config::mutexLock.lock();
		if (config == NULL) {
			config = new Config();
		}
		Config::mutexLock.unlock();
	}
	return config;
}

int Config::handle_args(int argc, char* argv[])
{
	int long_idx = 0;
	int c;

	static const char usage_str[] =
		"Usage: %s [OPTION]\n"
		"  -q, --quiet            Run quietly\n"
		"  -v, --verbose          Increase verbosity\n"
		"  -V, --version          Show version\n"
		"  -h, --help             Show this help screen and exit\n"
		"  -f, --file             Config file path\n"
		"  -S, --shownic          Show network interface control infomation\n"
		"--oneproxy_address       Oneproxy listen address(default:127.0.0.1)\n"
		"--oneproxy_port          Oneproxy listen port(default:9999); when have many ports, use comma separate\n"
		"--httpserver_address     Http server listen address(default:127.0.0.1)\n"
		"--httpserver_port        Http server listen port(default:8080)\n"
		"--clientusername         Client use the name to log in\n"
		"--clientpassword         Client use the password to log in\n"
		"--database_host          Database listen address(default:127.0.0.1)\n"
		"--database_port          Database listen port(default:sqlserver 1433)\n"
		"--database_classname     ClassName of handle database protocol(default:FakeProtocol)\n"
		"--database_username      Database's user name\n"
		"--database_password      Database's password \n"
		"--maxconnectnum          The number of oneproxy connection to database(default:2000)\n"
		"--keepalive              keep the process alive\n"
		"--vip_ifname             the vip network adapter name, for example: eth0:0\n"
		"--vip_address            the vip address\n"
		"--threadnum              the number of worker threads\n"
		"--passwordseparate       the client and middle software use different password(default: true)\n"
		"--readslave              when not in trans. read from slave database(default: true).\n"
		"--useconnectionpool      when is true use connection pool, else not use connection pool(default: true)\n"
		"Notice: If you need config database, you must use database_host, database_classname,\n"
		"database_username, database_password at the same time.\n"
		;

	static const struct option long_options[] = {
//			{"oneproxy_address", required_argument, NULL, 'a'},
//			{"httpserver_address", required_argument, NULL, 'A'},
//			{"database_host", required_argument, NULL, 'b'},
//			{"database_port", required_argument, NULL, 'c'},
//			{"daemon", no_argument, NULL, 'd'},
//			{"dump_data", no_argument, NULL, 'D'},
//			{"database_classname", required_argument, NULL, 'e'},
			{"file", required_argument, NULL, 'f'},
//			{"vip_ifname", required_argument, NULL, 'g'},
//			{"vip_address", required_argument, NULL, 'G'},
			{"help", no_argument, NULL, 'h'},
//			{"passwordseparate", required_argument, NULL, 'L'},
//			{"readsalve", required_argument, NULL, 'l'},
//			{"keepalive", no_argument, NULL, 'k'},
//			{"maxconnectnum", required_argument, NULL, 'm'},
//			{"useconnectionpool", required_argument, NULL, 'o'},
//			{"oneproxy_port", required_argument, NULL, 'p'},
//			{"httpserver_port", required_argument, NULL, 'P'},
			{"quiet", no_argument, NULL, 'q'},
//			{"log_sql", no_argument, NULL, 's'},
//			{"threadnum", required_argument, NULL, 't'},
//			{"clientusername", required_argument, NULL, 'u'},
//			{"database_username", required_argument, NULL, 'U'},
//			{"clientpassword", required_argument, NULL, 'w'},
//			{"database_password", required_argument, NULL, 'W'},
			{"verbose", no_argument, NULL, 'v'},
			{"version", no_argument, NULL, 'V'},
			{"shownic", no_argument, NULL, 'S'},
			{NULL, 0, NULL, 0},
	};

	//先载入默认配置，再根据其他配置针对默认配置的修改
	config()->default_oneproxyConfig();

	if (argc > 1) {
//		DataBase db;
//		DataBaseGroup dbg;
		while ((c = getopt_long(argc, argv, "qvhdVSf:", long_options, &long_idx)) != -1) {
			switch (c) {
			case 'v':
				config()->set_loggerLevel(Logger::INFO);
				break;
			case 'V':
				printf("%s\n", oneproxy_version());
				exit(0);
				break;
//			case 'd':
//				break;
			case 'q':
				config()->set_loggerLevel(Logger::ERR);
				break;
			case 'f':
				config()->loadConfig(optarg);
				break;
//			case 'g':
//				config()->set_vipIfName(std::string(optarg));
//				break;
//			case 'G':
//				config()->set_vipAddress(std::string(optarg));
//				break;
//			case 'D':
//				config()->set_dumpData(true);
//				break;
//			case 's':
//				config()->set_logSql(true);
//				break;
//			case 'a':
//				config()->set_oneproxyAddr(std::string(optarg));
//				break;
//			case 'p':
//				config()->set_oneproxyPort(std::string(optarg));
//				break;
//			case 'A':
//				config()->set_httpServerAddr(std::string(optarg));
//				break;
//			case 'o':
//				if (strncmp(optarg, "true", 4)) {
//					dbg.set_useConnectionPool(true);
//				} else {
//					dbg.set_useConnectionPool(false);
//				}
//				break;
//			case 'P':
//				config()->set_httpServerPort(atoi(optarg));
//				break;
//			case 'b':
//				db.set_addr(std::string(optarg));
//				break;
//			case 'c':
//				db.set_port(atoi(optarg));
//				break;
//			case 'e':
//				dbg.set_className(std::string(optarg));
//				break;
//			case 'L':
//				if (strncmp(optarg, "true", 4)) {
//					dbg.set_passwordSeparate(true);
//				} else {
//					dbg.set_passwordSeparate(false);
//				}
//				break;
//			case 'l':
//				if (strncmp(optarg, "true", 4)){
//					dbg.set_readSlave(true);
//				} else {
//					dbg.set_readSlave(false);
//				}
//				break;
//			case 'm':
//				config()->set_maxConnectNum(atoi(optarg));
//				break;
//			case 'k':
//				config()->set_keepAlive(true);
//				break;
//			case 't':
//				config()->set_threadNum(atoi(optarg));
//				break;
//			case 'u':
//				config()->set_clientUserName(std::string(optarg));
//				break;
//			case 'U':
//			{
//				std::string un = std::string(optarg);
//				db.set_userName(un);
//			}
//				break;
//			case 'w':
//				config()->set_clientPassword(std::string(optarg));
//				break;
//			case 'W':
//			{
//				std::string passwd = std::string(optarg);
//				db.set_password(passwd);
//			}
//				break;
			case 'h':
				printf("%s\n", usage_str);
				exit(0);
				break;
			case 'S':
				SystemApi::system_showNetworkInterfInfo();
				exit(0);
				break;
			default:
				printf("Please use config file to set application parameters\n");
				printf("%s\n", usage_str);
				exit(1);
			}
		}

//		const std::string argDataBaseLabelName = std::string("database_args");
//		dbg.set_dbMasterGroup(argDataBaseLabelName);//给参数设置的数据库取一个名称
//		db.set_labelName(argDataBaseLabelName);
//		if (db.is_valid() && dbg.is_valid()) {
//			config()->set_database(db);
//			config()->set_dataBaseGroup(dbg);
//		} else {
			if (config()->get_databaseSize() <= 0) {
				config()->default_database();//set default database
			}
			if (config()->get_dataBaseGroupSize() <= 0) {
				config()->default_databaseGroup();
			}
//		}
	} else {
		// 寻找配置文件
		do {
			std::string configFilePath;
#ifdef _WIN32
			configFilePath = Tool::search_oneFile(std::string(".\\"), std::string("*.ini"));
#else
			configFilePath = Tool::search_oneFile(std::string("./"), std::string("*.ini"));
#endif
			if (configFilePath.length() <= 0) {
				logs(Logger::INFO, "use default config");
				config()->default_database();
				config()->default_databaseGroup();
				break;
			}

			//载入配置文件
			if (config()->loadConfig(configFilePath)) {
				logs(Logger::FATAL, "load config file error, exit.");
				return -1;
			}

			//如果配置文件中没有配置数据库，则使用默认数据库
			if(config()->get_databaseSize() <= 0)
				config()->default_database();
			if (config()->get_dataBaseGroupSize() <= 0)
				config()->default_databaseGroup();
		} while(0);
	}

	if (this->handle_config()) {
		logs(Logger::ERR, "verify config error");
		return -1;
	}

	//set logger level
	logs_setLevel(config()->get_loggerLevel());
	logs_setDumpData(config()->get_dumpData());
	logs_setLogSql(config()->get_logSql());
	logs_setLogFile(config()->get_logFilePath());

	config()->print_config();

	return 0;
}

static int loadConfigKeyValue(const CSimpleIniA& ini, const char* section, const char* key,
		bool useDefaultValue, std::vector<ConfigKeyValue>& cfg, ConfigBase* obj)
{
	std::vector<ConfigKeyValue>::iterator it = cfg.begin();
	for (; it != cfg.end(); ++it) {
		if (strncmp_c(key, it->key.c_str()) == 0) {
			break;
		}
	}
	if (it == cfg.end()) {
		logs(Logger::ERR, "unkown key(%s), the key will be igore", key);
		return 0;
	}
	char* value = NULL;
	if (useDefaultValue) {
		value = (char*)ini.GetValue(section, key, it->defaultValue.c_str());
	} else {
		value = (char*)ini.GetValue(section, key, "");
	}
	if (value == NULL) {
		logs(Logger::ERR, "get key(%s)'s value error", key);
		return -1;
	}

	CVTFunc func = (*it).cvtFunc;
	(obj->*func)(std::string(value), (*it));
	return 0;
}

int Config::loadConfig(std::string filePath)
{
	CSimpleIniA ini;
	ini.SetUnicode();
	if (ini.LoadFile(filePath.c_str()) != SI_OK) {
		logs(Logger::ERR, "load file error");
		return -1;
	}

	CSimpleIniA::TNamesDepend keyList;
	CSimpleIniA::TNamesDepend sections;
	ini.GetAllSections(sections);
	CSimpleIniA::TNamesDepend::iterator sit = sections.begin();
	for (; sit != sections.end(); ++sit) {
		if(!ini.GetAllKeys(sit->pItem, keyList)) {
			logs(Logger::ERR, "get section[%s] keys error", sit->pItem);
			continue;
		}
		CSimpleIniA::TNamesDepend::iterator kit = keyList.begin();

		if (strncmp_c(sit->pItem, "oneproxy") == 0) {
			//load oneproxy section
			for (; kit != keyList.end(); ++kit) {
				loadConfigKeyValue(ini, sit->pItem, kit->pItem, true, this->oneproxyCfg, this);
			}
		} else if (strncmp_c(sit->pItem, "database") == 0) {//读取数据库信息
			if (this->dbInfoCfg.size() <= 0) //必须有默认配置，至少一个
				continue;
			DataBase* db = new DataBase();
			std::list<std::vector<ConfigKeyValue> >::iterator dbiter = this->dbInfoCfg.begin();
			for (; kit != keyList.end(); ++kit) {
				loadConfigKeyValue(ini, sit->pItem, kit->pItem, false, *dbiter, db);
			}
			if (!db->is_valid()) {//无效
				continue;
			}
			if (db->get_labelName().length() <= 0)
				db->set_labelName(std::string(sit->pItem));
			this->set_database(db);
		} else {//读取数据库组信息
			if (this->dbGroupCfg.size() <= 0) //必须有默认配置，至少一个
				continue;
			DataBaseGroup *dbg = new DataBaseGroup();
			std::list<std::vector<ConfigKeyValue> >::iterator dbgiter = this->dbGroupCfg.begin();
			for (; kit != keyList.end(); ++kit) {
				loadConfigKeyValue(ini, sit->pItem, kit->pItem, false, *dbgiter, dbg);
			}
			if (!dbg->is_valid()) {//无效
				continue;
			}
			if (dbg->get_labelName().length() <= 0)
				dbg->set_labelName(std::string(sit->pItem));
			this->set_dataBaseGroup(dbg);
		}
	}
	return 0;
}

void Config::set_database(DataBase* db) {
	this->dbVector.push_back(db);
}

DataBase * Config::get_database() {
	if (this->dbVector.size()) {
		return this->dbVector.at(0);
	}
	return NULL;
}

unsigned int Config::get_databaseSize() {
	return this->dbVector.size();
}

DataBase* Config::get_database(unsigned int index) {
	if (index >= this->get_databaseSize()) {
		return NULL;
	}
	return this->dbVector[index];
}

void Config::set_dataBaseGroup(DataBaseGroup* dataBaseGroup)
{
	this->dbGroupVector.push_back(dataBaseGroup);
}

DataBaseGroup* Config::get_dataBaseGroup(unsigned int index)
{
	if (index >= this->dbGroupVector.size()) {
		return NULL;
	}
	return this->dbGroupVector[index];
}

unsigned int Config::get_dataBaseGroupSize()
{
	return this->dbGroupVector.size();
}

u_uint64 Config::get_globalSecondTime()
{
	return this->m_globalMillisecondTime/1000;
}

u_uint64 Config::get_globalMillisecondTime()
{
	return this->m_globalMillisecondTime;
}

std::map<int, std::string>& Config::get_monitorPortClassMap()
{
	return this->monitorPortClass;
}

IMPLEMENT_CVTSTRING_FUNC(Config, cvtString)
IMPLEMENT_CVTINT_FUNC(Config, cvtInt)
IMPLEMENT_CVTLOGGER_FUNC(Config, cvtLogger)
IMPLEMENT_CVTBOOL_FUNC(Config, cvtBool)
