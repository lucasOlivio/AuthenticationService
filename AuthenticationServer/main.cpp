#include "AuthenticationServer.h"
#include <conio.h>

// TODO: This data should be stored in a file not versioned
#define DB_HOST "127.0.0.1:3306"
#define DB_USER "admin"
#define DB_PASS "admin123"
#define DB_SCHEMA "authentication_db"

int main()
{
	AuthenticationServer server = AuthenticationServer();

	bool initialized = server.Initialize(NULL, DEFAULT_PORT, DB_HOST, DB_USER, DB_PASS, DB_SCHEMA);

	if (!initialized)
	{
		printf("Error initializing server!");
		system("pause");
		exit(EXIT_FAILURE);
	}

	printf("Press ESC to close\n");
	while (true)
	{
		// server.ExecuteIncommingMsgs();

		if (_kbhit())
		{
			int key = _getch();
			if (key == 27) /*ESC*/
			{
				// Leave program
				break;
			}
		}
	}

	server.Destroy();

	return 0;
}