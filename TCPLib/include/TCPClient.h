#pragma once

#include "TCPBase.h"

class TCPClient : public TCPBase
{
public:
	// ctors & dtors
	TCPClient();
	virtual ~TCPClient();

	bool Initialize(const char* host, const char* port);
	void Destroy();

	bool Connect();

	// Set the socket to be blocking or not
	bool SetBlocking(u_long mode);

	// Wait "tv_sec" for msg from server
	bool CheckMsgFromServer(int tv_sec);
};