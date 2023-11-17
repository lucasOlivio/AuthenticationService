#include "ChatServer.h"
#include "chat.pb.h"

ChatServer::ChatServer()
{
    this->m_mapRoomClients = {};

    this->m_pTCP = nullptr;
    this->m_isInitialized = false;
}

ChatServer::~ChatServer()
{
}

bool ChatServer::Initialize(const char* host, const char* port)
{
    if (this->m_isInitialized)
    {
        // Already initialized
        return true;
    }

    // Initialize WSA and socket
    this->m_pTCP = new TCPServer();
    bool TCPInitialized = this->m_pTCP->Initialize(host, port);
    if (!TCPInitialized)
    {
        printf("Error initializing TCP!");
        return false;
    }

    this->m_isInitialized = true;
    return true;
}

void ChatServer::Destroy()
{
    this->m_pTCP->Destroy();
    delete this->m_pTCP;
    this->m_isInitialized = false;
}

bool ChatServer::IsRoomCreated(int idRoom)
{
    if (this->m_mapRoomClients.find(idRoom) == this->m_mapRoomClients.end())
    {
        return false;
    }
    return true;
}

void ChatServer::GetRoomAndIdUserBySocket(SOCKET& client, int& idRoomOut, int& idUserOut)
{
    // We go through each room
    for (std::pair<int /* idRoom */, std::map<int /*idUser*/, SOCKET>> room : this->m_mapRoomClients)
    {
        // We go through each user
        for (std::pair<int, SOCKET> user : room.second)
        {
            if (user.second == client)
            {
                // Found the user!
                idRoomOut = room.first;
                idUserOut = user.first;
                return;
            }
        }
    }
}

void ChatServer::AddClientToRoom(int idRoom, const int& idUser, SOCKET& client)
{
    if (!this->m_isInitialized)
    {
        return;
    }

    if (idRoom == 0)
    {
        // Rooms should start from 1
        return;
    }

    if (!this->IsRoomCreated(idRoom))
    {
        // If room doesn't exist yet, create one empty
        this->m_mapRoomClients[idRoom] = {};
    }

    chat::Response chatResponse;
    std::string responseSerialized;
    chatResponse.set_success(true);
    chatResponse.set_msg("ok");

    this->m_mapRoomClients[idRoom][idUser] = client;

    // Succesfully added the new user to the room
    chatResponse.SerializeToString(&responseSerialized);
    this->m_pTCP->SendRequest(client, "response", responseSerialized);

    std::string joinedMsg = "has joined the room";
    this->BroadcastToRoom(idRoom, idUser, joinedMsg);

    printf("%d %s %d\n", (int)client, joinedMsg.c_str(), idRoom);

    return;
}

void ChatServer::RemoveClientFromRoom(int idRoom, const int& idUser)
{
    if (!this->m_isInitialized)
    {
        return;
    }

    // Try to find user in room by his username
    std::map<int, SOCKET>::iterator it = this->m_mapRoomClients[idRoom].find(idUser);
    if (it == this->m_mapRoomClients[idRoom].end())
    {
        // User not in room
        return;
    }

    // Found user in room so we can remove him
    std::string leftMsg = "has left the room";
    printf("%d %s %d\n", (int)it->second, leftMsg.c_str(), idRoom);
    this->m_mapRoomClients[idRoom].erase(idUser);

    // Succesfully removed user from room, notify everyone else in the room
    this->BroadcastToRoom(idRoom, idUser, leftMsg);

    return;
}

void ChatServer::BroadcastToRoom(int idRoom, int idUser, const std::string& msg)
{
    if (!this->m_isInitialized)
    {
        return;
    }

    if (!this->IsRoomCreated(idRoom))
    {
        // If room doesn't exist yet, there is no one to send
        return;
    }

    std::string formatedMsg = "[" + std::to_string(idUser) + "] " + msg;
    std::string msgSerialized;
    chat::ChatMessage chatMsg;

    chatMsg.set_userid(idUser);
    chatMsg.set_roomid(idRoom);
    chatMsg.set_msg(formatedMsg.c_str());
    chatMsg.SerializeToString(&msgSerialized);

    for (std::pair<int, SOCKET> user : this->m_mapRoomClients[idRoom])
    {
        this->m_pTCP->SendRequest(user.second, "chatmessage", msgSerialized);
    }

    return;
}

void ChatServer::ExecuteIncommingMsgs()
{
    std::map<SOCKET, myTcp::sPacketData> mapNewMsgs;
    this->m_pTCP->ReadNewMsgs(mapNewMsgs);

    for (std::pair<SOCKET, myTcp::sPacketData> newMsg : mapNewMsgs)
    {
        SOCKET clientSocket = newMsg.first;
        myTcp::sPacketData msgPacket = newMsg.second;

        if (msgPacket.dataType == "")
        {
            // Client disconnected, so we have to make sure he left the room first
            int idRoom = -1;
            int idUser = -1;
            this->GetRoomAndIdUserBySocket(clientSocket, idRoom, idUser);

            if (idRoom == -1)
            {
                continue;
            }

            // Client didn't leave the room, so we must remove him
            this->RemoveClientFromRoom(idRoom, idUser);
        }
        
        // Find which action to take
        if (msgPacket.dataType == "joinroom")
        {
            chat::JoinRoom chatJoinRoom;
            bool isDeserialized = chatJoinRoom.ParseFromString(msgPacket.data);
            if (!isDeserialized)
            {
                continue;
            }

            this->AddClientToRoom((int)chatJoinRoom.roomid(), (int)chatJoinRoom.userid(), clientSocket);
            continue;
        }
        else if (msgPacket.dataType == "leaveroom")
        {
            chat::LeaveRoom chatLeaveRoom;
            bool isDeserialized = chatLeaveRoom.ParseFromString(msgPacket.data);
            if (!isDeserialized)
            {
                continue;
            }

            this->RemoveClientFromRoom((int)chatLeaveRoom.roomid(), (int)chatLeaveRoom.userid());
            continue;
        }
        if (msgPacket.dataType == "chatmessage")
        {
            chat::ChatMessage chatMsg;
            chatMsg.ParseFromString(msgPacket.data);

            // Just a chat msg, so we send to everyone in the room
            this->BroadcastToRoom((int)chatMsg.roomid(), (int)chatMsg.userid(), chatMsg.msg());

            continue;
        }
    }
}
