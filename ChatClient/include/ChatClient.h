#pragma once

#include "TCPClient.h"

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

	// Uses our TCP lib to send a chat message to to server
	void SendChatMessage(const std::string& msg);
	
	// Send a request to join the room using the referenced username
	bool JoinRoom(int idRoom, std::string& errorMsgOut);

	// Remove user from room
	void LeaveRoom();

	// Receives all new messages for the room
	std::string ReceiveRoomMsg();
};