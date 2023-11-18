#pragma once

#include "TCPServer.h"
#include <map>

class ChatServer : public TCPServer
{
private:
	// Mapping to the rooms we have and the clients in each one
	std::map<int /* idRoom */, std::map<int /*idUser*/, SOCKET>> m_mapRoomClients;

public:
	ChatServer();
	virtual ~ChatServer();

	bool IsRoomCreated(int idRoom);

	// Go through each room to check the socket
	void GetRoomAndIdUserBySocket(SOCKET& client, int& idRoomOut, int& idUserOut);

	// Add client socket to room id key
	void AddClientToRoom(int idRoom, const int& idUser, SOCKET& client);

	// Remove client socket from room id key
	void RemoveClientFromRoom(int idRoom, const int& idUser);

	// Send the given message to all clients inside the given room
	void BroadcastToRoom(int idRoom, int idUser, const std::string& msg);

	// Get which clients sent message and decide what to do with it based on the message type
	void ExecuteIncommingMsgs();
};