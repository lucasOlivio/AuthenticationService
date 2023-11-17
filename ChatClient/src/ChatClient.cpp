#include "ChatClient.h"
#include "chat.pb.h"

ChatClient::ChatClient()
{
	this->m_idRoom = -1;
    this->m_idUser = -1;
	this->m_pTCP = nullptr;
    this->m_isInitialized = false;
}

ChatClient::~ChatClient()
{
}

bool ChatClient::Initialize(const char* host, const char* port)
{
    if (this->m_isInitialized)
    {
        // Already initialized
        return true;
    }

    // Initialize WSA and socket
    this->m_pTCP = new TCPClient();
    bool TCPInitialized = this->m_pTCP->Initialize(host, port);
    if (!TCPInitialized)
    {
        printf("Error initializing TCP!");
        return false;
    }

    this->m_isInitialized = true;
    return true;
}

void ChatClient::Destroy()
{
    if (this->m_idRoom > -1)
    {
        // If its in a room, we must exit the room first to clear the username
        this->LeaveRoom();
    }
    this->m_pTCP->Destroy();
    delete this->m_pTCP;
    this->m_isInitialized = false;
    return;
}

bool ChatClient::IsInRoom()
{
    if (this->m_idRoom > -1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void ChatClient::SendChatMessage(const std::string& msg)
{
    // Build msg protobuff
    std::string msgSerialized;
    chat::ChatMessage chatMsg;
    chatMsg.set_userid(this->m_idUser);
    chatMsg.set_roomid(this->m_idRoom);
    chatMsg.set_msg(msg.c_str());

    // Serialize and send request
    chatMsg.SerializeToString(&msgSerialized);

    this->m_pTCP->SendRequest(this->m_pTCP->GetSocket(), "chatmessage", msgSerialized);
    return;
}

bool ChatClient::JoinRoom(int idRoom, std::string& errorMsgOut)
{
    // DEBUG
    //if (this->m_idUser == -1)
    //{
    //    errorMsgOut = "not logged in";
    //    return false;
    //}

    // Build joinroom protobuff
    std::string joinroomSerialized;
    chat::JoinRoom chatJoinRoom;
    chatJoinRoom.set_userid(this->m_idUser);
    chatJoinRoom.set_roomid(idRoom);

    // Serialize and send request
    chatJoinRoom.SerializeToString(&joinroomSerialized);
    this->m_pTCP->SendRequest(this->m_pTCP->GetSocket(), "joinroom", joinroomSerialized);

    // Wait for the server's response if the user was able to join the room
    std::string dataTypeOut;
    std::string dataOut;
    chat::Response chatResponse;
    if (this->m_pTCP->CheckMsgFromServer(3))
    {
        this->m_pTCP->ReceiveRequest(this->m_pTCP->GetSocket(), dataTypeOut, dataOut);
    }
    else
    {
        // No response from server
        errorMsgOut = "timeout";
        return false;
    }

    if (dataTypeOut != "response")
    {
        errorMsgOut = "unexpected response";
        return false;
    }

    bool isDeserialized = chatResponse.ParseFromString(dataOut);

    if (!isDeserialized)
    {
        errorMsgOut = "error parsing response";
        return false;
    }

    if (!chatResponse.success())
    {
        // Error trying to put user in new room
        errorMsgOut = chatResponse.msg();
        return false;
    }

    // User inserted in new room, we can update our current room
    this->m_idRoom = idRoom;
    errorMsgOut = "";

    return true;
}

void ChatClient::LeaveRoom()
{
    // Build leaveroom protobuff
    std::string leaveroomSerialized;
    chat::LeaveRoom chatLeaveRoom;
    chatLeaveRoom.set_userid(this->m_idUser);
    chatLeaveRoom.set_roomid(this->m_idRoom);

    // Serialize and send request
    chatLeaveRoom.SerializeToString(&leaveroomSerialized);
    this->m_pTCP->SendRequest(this->m_pTCP->GetSocket(), "leaveroom", leaveroomSerialized);

    this->m_idRoom = -1;

    return;
}

std::string ChatClient::ReceiveRoomMsg()
{
    bool isNewMsgAvailable = this->m_pTCP->CheckMsgFromServer();
    if (!isNewMsgAvailable)
    {
        // No new messages
        return "";
    }

    std::string dataTypeOut;
    std::string dataOut;
    chat::ChatMessage chatMsg;

    this->m_pTCP->ReceiveRequest(this->m_pTCP->GetSocket(), dataTypeOut, dataOut);

    if (dataTypeOut == "")
    {
        // Server disconnected, so should we
        printf("Server disconnected!\n");
        this->m_idRoom = 0;
        this->Destroy();
        return "";
    }

    bool isDeserialized = chatMsg.ParseFromString(dataOut);
    if (!isDeserialized)
    {
        printf("error parsing chat message");
        return "";
    }

    return chatMsg.msg();
}
