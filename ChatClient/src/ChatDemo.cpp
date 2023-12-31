#include "ChatDemo.h"
#include <iostream>
#include <conio.h>

const uint32 MIN_ROOM_ID = 1;
const uint32 MAX_ROOM_ID = 255;

ChatDemo::ChatDemo()
{
	this->m_pChat = nullptr;
	this->m_roomMessages = {};
	this->m_isRunning = false;
}

ChatDemo::~ChatDemo()
{
}

bool ChatDemo::Initialize()
{
	this->m_pChat = new ChatClient();
	bool initialized = m_pChat->Initialize(LOCALHOST, CHAT_PORT);

	if (!initialized)
	{
		printf("Error connecting to server!\n\n");
		system("pause");
		return false;
	}
	return true;
}

void ChatDemo::Destroy()
{
	this->m_roomMessages.clear();
	this->m_pChat->Destroy();
	delete this->m_pChat;

	return;
}

void ChatDemo::RunChat()
{
	this->m_isRunning = true;
	while (this->m_isRunning)
	{
		if (!this->m_pChat->IsLogged())
		{
			LoginScene();
		}
		else if (this->m_pChat->IsInRoom())
		{
			InRoomScene();
		}
		else
		{
			OutRoomScene();
		}
	}

	this->Destroy();

	return;
}

void ChatDemo::LoginScene()
{
	int option;
	std::string email = "";
	std::string password = "";
	std::string responseMsg = "";

	system("cls");
	printf("\n\n0 - Exit\n1 - Login\n2 - Register\n\noption: ");
	std::cin >> option;
	if (option == 0)
	{
		// Exiting program;
		this->m_isRunning = false;
		return;
	}
	
	printf("Enter email: ");
	std::cin >> email;
	printf("Enter password: ");
	std::cin >> password;

	if (option == 1)
	{
		m_pChat->Login(email, password, responseMsg);
	}
	else if (option == 2)
	{
		m_pChat->Register(email, password, responseMsg);
	}

	printf(("\n" + responseMsg + "\n\n").c_str());
	system("pause");

	return;
}

void ChatDemo::OutRoomScene()
{
	uint32 idRoom;
	std::string errorMsg = "";

	system("cls");
	printf("Join room id (Between %d and %d, or 0 to exit): ", MIN_ROOM_ID, MAX_ROOM_ID);
	std::cin >> idRoom;
	if (idRoom == 0)
	{
		// Exiting program;
		this->m_isRunning = false;
		return;
	}
	if (idRoom < MIN_ROOM_ID || idRoom > MAX_ROOM_ID)
	{
		printf("Room id not in range! Try again\n");
		return;
	}

	// Try to enter room with this username
	if (!this->m_pChat->JoinRoom(idRoom, errorMsg))
	{
		// couldn't join
		printf("\nCould not join room: %s\n\n", errorMsg.c_str());
		return;
	}

	// Entered room, so clear msgs vector
	this->m_roomMessages.clear();

	return;
}

void ChatDemo::PrintAllMsgs()
{
	for (std::string msg : this->m_roomMessages)
	{
		printf("%s\n", msg.c_str());
	}
	return;
}

void ChatDemo::RedrawInRoomScene()
{
	system("cls");
	this->PrintAllMsgs();

	printf("\n\n");

	printf("Send message (':q' to leave room, ESC to exit the program): \n");
	printf("%s", this->m_myMsg.c_str());

	return;
}

void ChatDemo::InRoomScene()
{
	std::string newMsg = this->m_pChat->ReceiveRoomMsg();

	if (newMsg != "")
	{
		// There is new msg, add to cache and print
		this->m_roomMessages.push_back(newMsg);
		this->RedrawInRoomScene();
	}

	if (_kbhit())
	{
		int key = _getch();
		if (key == 27) /*ESC*/
		{ 
			// Leave program
			this->m_isRunning = false;
			return;
		}
		if (key == 13) /*ENTER*/
		{ 
			if (this->m_myMsg == ":q")
			{
				// Leave room and clear msg cache and scene
				this->m_pChat->LeaveRoom();
				this->m_roomMessages.clear();
				system("cls");
			} 
			else
			{
				// New message sent
				this->m_pChat->SendChatMessage(this->m_myMsg);
			}

			this->m_myMsg = "";
			return;
		}

		if (key == 8) /*BACKSPACE*/
		{
			if (!this->m_myMsg.empty()) {
				this->m_myMsg.pop_back(); // Remove last char
			}
		}
		else
		{
			// Anything else we add to the msg been typed by the user
			this->m_myMsg += static_cast<char>(key);
		}

		this->RedrawInRoomScene();
	}

	return;
}
