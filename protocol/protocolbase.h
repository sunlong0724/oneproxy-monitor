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
* @FileName: protocolbase.h
* @Description: 此类是所有数据库或者其他数据协议类的基础类，必须继承此类，同时需要实现如下的接口：
* 必须实现的接口：
* 1. is_currentDatabase: 这个类是用来选择数据库类的，当connection第一次接收到数据包时，通过此类来判断当前连接是否属于这个数据库的连接，如果是则返回true。
* 否则返回false。如果为true，则框架将从配置中读取数据库地址和端口，连接连接，同时转发数据到这个连接上面。
* 2. createInstance: 框架通过调用这个函数来创建对应的实例，建议不要使用单例模式来创建，因为在这个实例中有prepared的相关信息，不同的连接可能有相同的句柄。
* 3. destoryInstance: 当连接完成时，会调用此函数来释放相关的内存等。
* 4. get_packetType:根据包的数据返回包的类型，由于不同数据库包的格式不同，类型的位置和长度不同，故需要不同的数据库协议类自己实现此接口。
* 可选实现的接口：
* 1. protocol_front: 前端接收到的数据包，都会传递给这个函数，协议类可以通过重新实现这个函数来改变默认的处理方式。框架在调用这个函数后，就直接把数据包转发到服务端了。
* 2. protocol_backend: 后端接收到数据包，都会传递给这个函数，协议类可以通过重新实现这个函数改变默认的处理方式。框架在调用这个函数后，就直接把数据包转发到客户端了。
* 3. prehandle_frontPacket:默认实现是没有处理数据包的任何内容。如果需要针对接收到的前端数据包进行处理，则需要实现此函数。比如：当客户端的数据包特别大时，被客户 端分成
* 了多个数据包发送到服务端，在进行统计前，需要针对这些数据包进行合并，那么就需要在此包中进行合并的逻辑。
* 4. prehandle_backendPacket:与prehandle_frontPacket相同。不同的点是，这个函数针对接收到的后端数据包
* 5. protocol_initFrontStatPacket: 默认实现只是把bufpointer的指针指向接收到的数据包。如果在进行执行处理函数前，需要针对数据进行修改，那么需要实现此函数。
* 6. protocol_initBackendStatPacket: 与protocol_initFrontStatPacket函数功能相同，不同点是，这个针对后端数据包。
* 7. protocol_endFrontStatPacket:当执行完协议类注册的处理函数后，执行此函数。默认实现是空的。
* 8. protocol_endBackendStatPacket:功能与protocol_endFrontStatPacket相同，不同的是这个函数针对后端数据包的处理。
* All rights Reserved, Designed By huih
* @Company: onexsoft
* @Author: hui
* @Version: V1.0
* @Date: 2016年9月12日 上午9:22:04
*
*/

#ifndef PROTOCOL_PROTOCOLBASE_H_
#define PROTOCOL_PROTOCOLBASE_H_

#include "protocoldynamiccls.h"
#include "connection.h"
#include "tcpclient.h"
#include "sqlparser.h"
#include "systemapi.h"
#include "record.h"
#include "tool.h"

#include <sstream>
#include <iostream>

typedef std::map<unsigned int, void*> IntPointerMap;
/***
 * 结构体PreparedParamT和PreparedDataInfoT用于保存prepared中sql语句和sql语句参数的信息
 **/
typedef enum _prepared_param_type_t{
	PARAM_TYPE_STRING,//是string类型的值，需要作为字符串处理
	PARAM_TYPE_OTHER,//直接替换带原来的参数即可。
}PreparedParamType;
typedef struct _prepared_param_t{
	std::string paramName;
	PreparedParamType paramType;
	unsigned int paramTypeI;
	unsigned int paramStartPos;
	unsigned int paramEndPos;
	_prepared_param_t() {
		this->paramType = PARAM_TYPE_OTHER;
		this->paramEndPos = 0;
		this->paramStartPos = 0;
		this->paramTypeI = 0;
	}
}PreparedParamT;
typedef struct _Prepared_data_info_t{
	std::string sql;
	unsigned int paramNum;//sql语句中的参数的数量
#define MAX_PARAM_NUM 100 //假设最多100个参数
	PreparedParamT param[MAX_PARAM_NUM];

	std::vector<std::string> paramValueVec; //参数的值,位置应该与param中的位置一致

	static _Prepared_data_info_t* create_instance() {
		return new _Prepared_data_info_t();
	}

	void destory_instance() {
		delete this;
	}
	void clear() {
		this->sql.clear();
		this->paramNum = 0;
		this->paramValueVec.clear();
	}
}PreparedDataInfoT;

typedef enum _protocol_handle_return_value_t{
	HANDLE_RETURN_ERROR, //表示当前函数处理出现错误,框架会直接把数据转发到服务端的。
	HANDLE_RETURN_SUCCESS, //表示当前函数成功处理，框架会调用下一个注册函数处理剩下的数据
	HANDLE_RETURN_SEND_DIRECT,//表示当前处理成功，但是框架不会处理剩下的数据，直接把接收到的数据直接转发到对端
	HANDLE_RETURN_SEND_TO_CLIENT,//表示把前端缓存中的数据，直接转发给客户端。这种情况是针对在处理过程中修改了数据包，这个数据包是发送到前端的数据。目前保留
	HANDLE_RETURN_SEND_TO_SERVER,
	HANDLE_RETURN_FAILED_CLOSE_CONN,//表示当前的连接已经无法处理，需要把连接关闭掉
	HANDLE_RETURN_NORMAL_CLOSE_CONN,//前端正常关闭连接
} ProtocolHandleRetVal;

declare_clsfunc_pointer(ProtocolHandleRetVal, ProtocolBase, BaseFunc, Connection&, StringBuf&)
#define declare_protocol_handle_func(funcName) ProtocolHandleRetVal funcName(Connection& conn, StringBuf& packet);
#define implement_protocol_handle_func(className, funcName) ProtocolHandleRetVal className::funcName(Connection& conn, StringBuf& packet)
#define regester_front_handle_func(type, classFuncPointer) regester_frontHandleFunc((int)type, (BaseFunc)classFuncPointer);
#define regester_backend_handle_func(type, classFuncPointer) regester_backendHandleFunc((int)type, (BaseFunc)classFuncPointer);

class ProtocolBase {
public:
	ProtocolBase(){};
	virtual ~ProtocolBase(){};
	virtual ProtocolHandleRetVal protocol_front(Connection& conn);
	virtual ProtocolHandleRetVal protocol_backend(Connection& conn);
	virtual bool is_currentDatabase(Connection& conn) = 0;
	virtual int protocol_init(Connection& conn){return 0;}
	virtual int protocol_getBackendConnect(Connection& conn);
	int protocol_releaseBackendConnect(Connection& conn, ConnFinishType type);

	static void* createInstance();
	virtual void destoryInstance() = 0;

private:
	SqlParser sqlParser;

	//定义函数处理map
	declare_type_alias(BaseHandleFuncMap, std::map<int, BaseFunc>)
	BaseHandleFuncMap frontHandleFunc;
	BaseHandleFuncMap backendHandleFunc;
	TcpClient tcpClient;

protected:
	virtual int protocol_initBackendConnect(Connection& conn) {return 0;}
	virtual int protocol_chooseDatabase(Connection& conn);
	virtual int protocol_createBackendConnect(Connection& conn);

	//调用此函数，返回包的类型，及注册函数的key值，由于每种数据库包类型长度不同，位置不同，故需要协议部分实现
	virtual int get_packetType(StringBuf& packet) = 0;
	virtual bool is_frontPacket(int type);

	//注册处理函数句柄
	void regester_frontHandleFunc(int type, BaseFunc func);
	void regester_backendHandleFunc(int type, BaseFunc func);
	virtual declare_protocol_handle_func(handle_defaultPacket)

	//每次接收到数据包时，需要先调用此函数进行预处理。
	virtual ProtocolHandleRetVal prehandle_frontPacket(Connection& conn);
	virtual ProtocolHandleRetVal prehandle_backendPacket(Connection& conn);

	//下面的两个函数是调用用户注册的处理函数前需要调用的，目的是让用户操作需要处理的数据，比如:sql server中需要跳过头部等。
	virtual int protocol_initFrontStatPacket(Connection& conn);
	virtual int protocol_initBackendStatPacket(Connection& conn);
	virtual int protocol_endFrontStatPacket(Connection& conn);
	virtual int protocol_endBackendStatPacket(Connection& conn);

	//返回sqlParser对象
	SqlParser& get_sqlParser();
	//返回当前正在执行的sql的hashcode。如果没有，则返回0.
	unsigned int get_currentSqlHashCode(Connection& conn);
	//返回当前正在执行的sql语句
	std::string& get_currentSqlText(Connection& conn);
	//此函数中调用parse对象解析sql语句，并且把信息保存到conn.record.sqlinfo中
	virtual int parse_sql(Connection& conn, std::string sqlText);

	//统计读取到前端的数据
	virtual void stat_readFrontData(Connection& conn);
	//统计从后端返回的数据
	virtual void stat_readBackendData(Connection& conn);

	//开始事务
	virtual void stat_startTrans(Connection& conn);
	//结束事务
	virtual void stat_endTrans(Connection& conn, bool isRollBack = false);

	//把解析后的sql语句记录到缓存中
	virtual void stat_saveSql(Connection& conn, std::string& sqlText);
	//记录当前执行的sql，这个sql是经过解析器改写后的sql语句，及把值替换后的sql语句
	virtual void stat_executeSql(Connection& conn, std::string& sqlText);
	//记录当前执行的sql，只进行parse操作
	virtual int stat_parseSql(Connection& conn, std::string& sqlText);

	//根据connection中的hashcode来统计执行的sql语句
	virtual void stat_executeSql(Connection& conn);
	//记录当前执行的sql，传入的sql语句是原始的sql语句及没有经过sql解析器处理 的sql，再此函数中将会调用
	//sql解析器先解析sql，在调用stat_executeSql来统计sql
	virtual int stat_parseAndStatSql(Connection& conn, std::string sqlText);
	//根据prepreadHandle来查找sql语句，并且针对这个语句进行统计
	virtual int stat_preparedSql(Connection& conn, unsigned int preparedHandle);

	//统计查询结果
	virtual void stat_recvOneRow(Connection& conn);//接收到一行数据
	//接收完成，其中rows时结束包中提供的总数据行数，如果没有，可以不用填写。但是必须调用此函数
	virtual void stat_recvFinishedRow(Connection& conn, unsigned int rows = 0);
	//sql语句执行错误，这个函数应该与stat_recvFinishedRow互斥
	virtual void stat_executeErr(Connection& conn);
	//统计登录情况
	virtual void stat_login(Connection& conn);

	//检测socket是否active
	static bool check_socketActive(NetworkSocket* ns) {return true;}
	//把连接增加到pool中
	virtual void add_socketToPool(NetworkSocket* ns);
	//把已经使用过的后端连接保存到pool中
	virtual int set_oldSocketToPool(NetworkSocket* ns, ConnFinishType type);
	//根据hashcode查找sql语句
	virtual std::string get_sqlText(unsigned int hashCode);

	//根据参数名称，标记参数在sql语句中的位置
	virtual void mark_sqlParamPosition(PreparedDataInfoT& preparedDataInfot);
	//把参数的值填充到原始sql语句中，并输出
	virtual void fill_originSqlValue(const PreparedDataInfoT& preparedDataInfot, Connection& conn);
	//输出回填值后的sql语句,用于sql审计功能
	virtual void output_originSql(const std::string& sql, Connection& conn);
};

#endif /* PROTOCOL_PROTOCOLBASE_H_ */
