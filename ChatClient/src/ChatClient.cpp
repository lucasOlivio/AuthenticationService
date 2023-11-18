#include "ChatClient.h"
#include "chat.pb.h"

ChatClient::ChatClient()
{
	this->m_idRoom = -1;
    this->m_idUser = -1;
}

ChatClient::~ChatClient()
{
}

void ChatClient::Destroy()
{
    if (this->m_idRoom > -1)
    {
        // If its in a room, we must exit the room first to clear the username
        this->LeaveRoom();
    }

    this->TCPClient::Destroy();

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

    this->SendRequest(this->GetSocket(), "chatmessage", msgSerialized);
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
    this->SendRequest(this->GetSocket(), "joinroom", joinroomSerialized);

    // Wait for the server's response if the user was able to join the room
    std::string dataTypeOut;
    std::string dataOut;
    chat::Response chatResponse;
    bool haveResponse = this->CheckMsgFromServer(3);
    if (haveResponse)
    {
        this->ReceiveRequest(this->GetSocket(), dataTypeOut, dataOut);
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
    this->SendRequest(this->GetSocket(), "leaveroom", leaveroomSerialized);

    this->m_idRoom = -1;

    return;
}

std::string ChatClient::ReceiveRoomMsg()
{
    std::string dataTypeOut;
    std::string dataOut;
    chat::ChatMessage chatMsg;

    bool isNewMsg = this->ReceiveRequest(this->GetSocket(), dataTypeOut, dataOut);

    if (isNewMsg && dataTypeOut == "")
    {
        // No new messages
        return "";
    }
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
