#include "TCPBase.h"
#include "packet.pb.h"

void TCPBase::m_SocketError(const char* function, SOCKET socket, bool isFatalError)
{
	printf("%s failed with error %d\n", function, WSAGetLastError());

	if (isFatalError)
	{
		this->Destroy();
	}
	return;
}

void TCPBase::m_ResultError(const char* function, int result, bool isFatalError)
{
	printf("%s failed with error %d\n", function, result);

	if (isFatalError)
	{
		this->Destroy();
	}
	return;
}

uint32 TCPBase::m_SerializePacket(const std::string& dataTypeIn, const std::string& dataIn, Buffer& bufferOut)
{
	// Build protobuffer
	std::string packetSerialized;
	packet::PacketData packetData;
	packetData.set_data(dataIn.c_str());
	packetData.set_datatype(dataTypeIn.c_str());
	packetSerialized = packetData.SerializeAsString();

	// Build prefix header
	bufferOut.WriteUInt32LE((uint32)packetSerialized.size());
	bufferOut.WriteString(packetSerialized);

	return (uint32)bufferOut.vecBufferData.size();
}

bool TCPBase::m_DeserializePacket(const std::string& strPacketIn, std::string& dataTypeOut, std::string& dataOut)
{
	// Build protobuffer
	packet::PacketData packetData;
	bool success = packetData.ParseFromString(strPacketIn);

	if (!success)
	{
		return false;
	}

	dataTypeOut = packetData.datatype();
	dataOut = packetData.data();

	return true;
}

TCPBase::TCPBase()
{
    this->m_serverSocket = INVALID_SOCKET;
    this->m_pInfo = nullptr;
	this->m_isInitialized = false;
	this->m_tv.tv_sec = 1;
	this->m_tv.tv_usec = 0;
}

TCPBase::~TCPBase()
{
}

SOCKET& TCPBase::GetSocket()
{
    return this->m_serverSocket;
}

bool TCPBase::Initialize(const char* host, const char* port)
{
	if (this->m_isInitialized)
	{
		// Already initialized
		return true;
	}

	// Initialize WinSock
	WSADATA wsaData;
	int result;

	// Set version 2.2 with MAKEWORD(2,2)
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		this->m_ResultError("WSAStartup", result, true);
		return false;
	}
	printf("WSAStartup successfully!\n");

	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));	// ensure we don't have garbage data 
	hints.ai_family = AF_INET;			// IPv4
	hints.ai_socktype = SOCK_STREAM;	// Stream
	hints.ai_protocol = IPPROTO_TCP;	// TCP
	hints.ai_flags = AI_PASSIVE;

	// Set all address info 
	result = getaddrinfo(host, port, &hints, &this->m_pInfo);
	if (result != 0) {
		this->m_ResultError("getaddrinfo", result, true);
		return false;
	}
	printf("getaddrinfo successfully!\n");

	// Creates new socket
	this->m_serverSocket = socket(this->m_pInfo->ai_family, this->m_pInfo->ai_socktype, this->m_pInfo->ai_protocol);
	if (this->m_serverSocket == INVALID_SOCKET) {
		this->m_SocketError("socket", this->m_serverSocket, true);
		return false;
	}
	printf("socket created successfully!\n");

	this->m_isInitialized = true;
    return true;
}

void TCPBase::Destroy()
{
	if (this->m_serverSocket != INVALID_SOCKET)
	{
		closesocket(this->m_serverSocket);
	}
	if (this->m_pInfo)
	{
		freeaddrinfo(this->m_pInfo);
	}
	WSACleanup();
	this->m_isInitialized = false;
	return;
}

void TCPBase::SendRequest(SOCKET& destSocket, const std::string& dataTypeIn, const std::string& dataIn)
{
	if (!this->m_isInitialized)
	{
		return;
	}

	// Build string packet to send
	Buffer buffer(BUFFER_SIZE);
	int packetSize = this->m_SerializePacket(dataTypeIn, dataIn, buffer);

	// Send data and validate if succesfull
	int result = send(destSocket, (const char*)(&buffer.vecBufferData[0]), packetSize, 0);
	if (result == SOCKET_ERROR) {
		this->m_SocketError("send", result, false);
		return;
	}
	return;
}

bool TCPBase::ReceiveRequest(SOCKET& origSocket, std::string& dataTypeOut, std::string& dataOut)
{
	if (!this->m_isInitialized)
	{
		return false;
	}

	std::string strPacket = "";
	uint32 packetSize = 0;

	Buffer buffer(BUFFER_SIZE);

	// Get total packet size first to prepare buffer
	buffer.vecBufferData.resize(sizeof(packetSize));
	int result = recv(origSocket, (char*)(&buffer.vecBufferData[0]), sizeof(packetSize), 0);
	if (result == SOCKET_ERROR) {
		this->m_SocketError("recv", result, false);
		return false;
	}
	if (result == 0)
	{
		// User disconnected
		return false;
	}

	// + 1 for the /0 end of string
	packetSize = buffer.ReadUInt32LE() + 1;
	buffer.vecBufferData.resize(packetSize);
	// Now we can get the rest of the message, with the rest total size
	result = recv(origSocket, (char*)(&buffer.vecBufferData[0]), packetSize, 0);
	if (result == SOCKET_ERROR) {
		this->m_SocketError("recv", result, false);
		return false;
	}

	// Transform the data into our readable string
	strPacket = buffer.ReadString(0, packetSize - 1);
	bool isDeserialized = this->m_DeserializePacket(strPacket, dataTypeOut, dataOut);
	if (!isDeserialized)
	{
		return false;
	}

	return true;
}
