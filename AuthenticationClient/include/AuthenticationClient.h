#pragma once

#include "TCPClient.h"

#define AUTH_PORT "8812"

class AuthenticationClient : public TCPClient
{
public:
	// ctors & dtors
	AuthenticationClient();
	virtual ~AuthenticationClient();

	// Uses our TCP lib to send a create user request to the server
	void SendCreateUserRequest(int requestId, const std::string& email, const std::string& password);

	// Receives all new messages from the server returning if new msg arrived
	bool ReceiveServerMsg(int& requestId, int& userId, bool& success, int& errorReason);
};