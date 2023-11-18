#pragma once

#include "TCPServer.h"
#include "mysqlutil.h"

#define AUTH_PORT "8812"

class AuthenticationServer : TCPServer
{
	enum class StatementType
	{
		CreateWebAuth = 0,
		SelectWebAuth = 1,
		UpdateWebAuth = 2,
		CreateUser = 3,
		SelectUser = 4,
		UpdateUser = 5,
		GetLastId = 6
};
private:
	MySQLUtil* m_pMysql;

	// Make sure to cleanup (delete these pointers) when done.
	std::map<int, sql::PreparedStatement*> m_PreparedStatements;

public:
	AuthenticationServer();
	~AuthenticationServer();

	virtual bool Initialize(const char* authHost, const char* authPort,
							const char* host, const char* username,
							const char* password, const char* schema);

	virtual void Destroy();

	// Load prepared statements into cache
	void LoadPrepStatements();

	// Creates new user auth and send response back to client
	void CreateNewAccount(SOCKET& client, std::string packetData, std::string responseTypeOut, std::string responseDataOut);

	// Get which clients sent message and decide what to do with it based on the message type
	void ExecuteIncommingMsgs();


};