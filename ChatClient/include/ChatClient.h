#pragma once

#include "TCPClient.h"

#define CHAT_PORT "8811"

class ChatClient : public TCPClient
{
private:
	int m_idRoom;
	int m_idUser;

public:
	// ctors & dtors
	ChatClient();
	virtual ~ChatClient();

	virtual void Destroy();

	// Check if user is in any room
	bool IsInRoom();

	// Check if user is logged in
	bool IsLogged();

	// Uses our TCP lib to send a chat message to to server
	void SendChatMessage(const std::string& msg);
	
	// Send a request to join the room using the referenced username
	bool JoinRoom(int idRoom, std::string& errorMsgOut);

	// Remove user from room
	void LeaveRoom();

	// Log in to chat
	bool Login(const std::string& email, const std::string& password, std::string& responseMsgOut);

	// New registration in chat
	bool Register(const std::string& email, const std::string& password, std::string& responseMsgOut);

	// Receives all new messages for the room
	std::string ReceiveRoomMsg();
};