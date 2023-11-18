#include "AuthenticationServer.h"
#include "myutils.h"
#include "authentication.pb.h"
#include <ctime>

const char* DEFAULT_DB_DATETIME = "%Y-%m-%d %H:%M:%S";
const int DEFAULT_SALT = 16;

AuthenticationServer::AuthenticationServer() : m_pMysql(nullptr), m_PreparedStatements({})
{
}

AuthenticationServer::~AuthenticationServer()
{
	if (m_pMysql != nullptr)
		delete m_pMysql;

    for (std::pair<int, sql::PreparedStatement*> pairSts : m_PreparedStatements)
    {
        delete pairSts.second;
    }
    m_PreparedStatements.clear();
}

bool AuthenticationServer::Initialize(const char* authHost, const char* authPort,
									  const char* host, const char* username,
									  const char* password, const char* schema)
{
	TCPServer::Initialize(authHost, authPort);

	m_pMysql = new MySQLUtil();
	bool isConnDB = m_pMysql->ConnectToDatabase(host, username, password, schema);
	if (!isConnDB)
	{
		return false;
	}

    this->LoadPrepStatements();

    return true;
}

void AuthenticationServer::Destroy()
{
	TCPServer::Destroy();

	m_pMysql->Disconnect();
}

void AuthenticationServer::LoadPrepStatements()
{
    // table web_auth
    m_PreparedStatements[(int)StatementType::CreateWebAuth] = m_pMysql->PrepareStatement(
        "INSERT INTO web_auth (email, salt, hashed_password, userId) VALUES (?, ?, ?, ?);"
    );
    m_PreparedStatements[(int)StatementType::SelectWebAuth] = m_pMysql->PrepareStatement(
        "SELECT email, salt, hashed_password, userId FROM web_auth WHERE email = ?;"
    );
    m_PreparedStatements[(int)StatementType::UpdateUser] = m_pMysql->PrepareStatement(
        "UPDATE web_auth SET email = ?, salt = ?, hashed_password = ? WHERE userId = ?;"
    );

    // table user
    m_PreparedStatements[(int)StatementType::CreateUser] = m_pMysql->PrepareStatement(
        "INSERT INTO user (last_login, creation_date) VALUES (?, ?);"
    );
    m_PreparedStatements[(int)StatementType::SelectUser] = m_pMysql->PrepareStatement(
        "SELECT id, last_login, creation_date FROM user WHERE id = ?;"
    );
    m_PreparedStatements[(int)StatementType::UpdateUser] = m_pMysql->PrepareStatement(
        "UPDATE user SET last_login = ?  WHERE id = ?;"
    );

    // TODO: Multiple queries to get the id in the same transaction the insert occurred
    // common
    m_PreparedStatements[(int)StatementType::GetLastId] = m_pMysql->PrepareStatement(
        "SELECT LAST_INSERT_ID() as id;"
    );
}

void AuthenticationServer::CreateNewAccount(SOCKET& client, std::string packetDataIn, 
                                            std::string& responseTypeOut, std::string& responseDataOut)
{
    // Deserialize packet
    authentication::CreateAccountWeb createAccount;
    bool isDeserialized = createAccount.ParseFromString(packetDataIn);

    // Prepare responses
    authentication::CreateAccountWebSuccess webSuccess;
    webSuccess.set_requestid(createAccount.requestid());
    authentication::CreateAccountWebFailure webFailure;
    webFailure.set_requestid(createAccount.requestid());
    if (!isDeserialized)
    {
        responseTypeOut = "createaccountwebfailure";
        webFailure.set_reason(authentication::CreateAccountWebFailure_Reason_INTERNAL_SERVER_ERROR);
        webFailure.SerializeToString(&responseDataOut);
        printf("error deserializing message\n");
        return;
    }

    // Check if email is already in db
    sql::PreparedStatement* pStmt = m_PreparedStatements[(int)StatementType::SelectWebAuth];
    pStmt->setString(1, createAccount.email());
    sql::ResultSet* response = pStmt->executeQuery();
    if (response->rowsCount() > 0)
    {
        responseTypeOut = "createaccountwebfailure";
        webFailure.set_reason(authentication::CreateAccountWebFailure_Reason_ACCOUNT_ALREADY_EXISTS);
        responseDataOut = webFailure.SerializeAsString();
        return;
    }

    // Create new id for user
    std::string timeNow = TimeNow(DEFAULT_DB_DATETIME);
    pStmt = m_PreparedStatements[(int)StatementType::CreateUser];
    pStmt->setDateTime(1, timeNow);
    pStmt->setDateTime(2, timeNow);

    pStmt->execute();

    // Retrieve the last inserted ID
    pStmt = m_PreparedStatements[(int)StatementType::GetLastId];
    response = pStmt->executeQuery();

    int32_t userId = 0;
    while (response->next())
    {
        userId = response->getInt("id");
    }
    if (userId == 0)
    {
        responseTypeOut = "createaccountwebfailure";
        webFailure.set_reason(authentication::CreateAccountWebFailure_Reason_INTERNAL_SERVER_ERROR);
        webFailure.SerializeToString(&responseDataOut);
        printf("failed to retrieve userId\n");
        return;
    }

    // Hash password
    std::string genSalt = GenerateRandomSalt(DEFAULT_SALT);
    std::string hashedPass = HashPassword(createAccount.plaintextpassword(), genSalt);

    // Create user web auth
    pStmt = m_PreparedStatements[(int)StatementType::CreateWebAuth];
    pStmt->setString(1, createAccount.email());
    pStmt->setString(2, genSalt);
    pStmt->setString(3, hashedPass);
    pStmt->setInt(4, userId);

    pStmt->execute();

    webSuccess.set_userid(userId);
    webSuccess.set_timecreation(timeNow);
    responseDataOut = webSuccess.SerializeAsString();
    responseTypeOut = "createaccountwebsuccess";

    return;
}

void AuthenticationServer::AuthenticateUser(SOCKET& client, std::string packetDataIn, 
                                            std::string& responseTypeOut, std::string& responseDataOut)
{
    using namespace std;

    // Deserialize packet
    authentication::AuthenticateWeb authAccount;
    bool isDeserialized = authAccount.ParseFromString(packetDataIn);

    // Prepare responses
    authentication::AuthenticateWebSuccess webSuccess;
    webSuccess.set_requestid(authAccount.requestid());
    authentication::AuthenticateWebFailure webFailure;
    webFailure.set_requestid(authAccount.requestid());
    if (!isDeserialized)
    {
        responseTypeOut = "authenticatewebfailure";
        webFailure.set_reason(authentication::AuthenticateWebFailure_Reason_INTERNAL_SERVER_ERROR);
        webFailure.SerializeToString(&responseDataOut);
        printf("error deserializing message\n");
        return;
    }

    // Check if email exist in db
    sql::PreparedStatement* pStmt = m_PreparedStatements[(int)StatementType::SelectWebAuth];
    pStmt->setString(1, authAccount.email());
    sql::ResultSet* response = pStmt->executeQuery();
    if (response->rowsCount() == 0)
    {
        responseTypeOut = "authenticatewebfailure";
        webFailure.set_reason(authentication::AuthenticateWebFailure_Reason_INVALID_CREDENTIALS);
        responseDataOut = webFailure.SerializeAsString();
        return;
    }

    // Check if password is the same
    response->next();
    string salt = response->getString("salt");
    string dbPassHash = response->getString("hashed_password");
    int userId = response->getInt("userId");

    string authPassHash = HashPassword(authAccount.plaintextpassword(), salt);

    if (dbPassHash == authPassHash)
    {
        // Auth success

        // Get user info
        pStmt = m_PreparedStatements[(int)StatementType::SelectUser];
        pStmt->setInt(1, userId);
        sql::ResultSet* responseUser = pStmt->executeQuery();
        responseUser->next();
        string creationDate = responseUser->getString("creation_date");

        // Build auth success response string
        responseTypeOut = "authenticatewebsuccess";
        webSuccess.set_userid(userId);
        webSuccess.set_creationdate(creationDate);

        responseDataOut = webSuccess.SerializeAsString();

        // Update last login
        std::string timeNow = TimeNow(DEFAULT_DB_DATETIME);
        pStmt = m_PreparedStatements[(int)StatementType::UpdateUser];
        pStmt->setString(1, timeNow);
        pStmt->setInt(2, userId);
        pStmt->execute();

        return;
    }
    else
    {
        responseTypeOut = "authenticatewebfailure";
        webFailure.set_reason(authentication::AuthenticateWebFailure_Reason_INVALID_CREDENTIALS);
        responseDataOut = webFailure.SerializeAsString();
        return;
    }
}

void AuthenticationServer::ExecuteIncommingMsgs()
{
    std::map<SOCKET, myTcp::sPacketData> mapNewMsgs;
    ReadNewMsgs(mapNewMsgs);

    for (std::pair<SOCKET, myTcp::sPacketData> newMsg : mapNewMsgs)
    {
        std::string responseData = "";
        std::string responseType = "";
        SOCKET clientSocket = newMsg.first;
        myTcp::sPacketData msgPacket = newMsg.second;

        // Find which action to take
        if (msgPacket.dataType == "createaccountweb")
        {
            CreateNewAccount(clientSocket, msgPacket.data, responseType, responseData);
        }
        else if (msgPacket.dataType == "authenticateweb")
        {
            AuthenticateUser(clientSocket, msgPacket.data, responseType, responseData);
        }
        
        // Return to client the result of the action
        SendRequest(clientSocket, responseType, responseData);
    }
}