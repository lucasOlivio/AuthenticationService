#include "AuthenticationClient.h"
#include "authentication.pb.h"

AuthenticationClient::AuthenticationClient()
{
}

AuthenticationClient::~AuthenticationClient()
{
}

void AuthenticationClient::SendCreateUserRequest(int requestId, const std::string& email, const std::string& password)
{
    // Build msg protobuff
    std::string msgSerialized;
    authentication::CreateAccountWeb createAccount;
    createAccount.set_email(email);
    createAccount.set_plaintextpassword(password);
    createAccount.set_requestid(requestId);

    // Serialize and send request
    createAccount.SerializeToString(&msgSerialized);
    this->SendRequest(this->GetSocket(), "createaccountweb", msgSerialized);
    
    return;
}

bool AuthenticationClient::ReceiveServerMsg(int& requestId, int& userId, bool& success, int& errorReason)
{
    std::string dataTypeOut;
    std::string dataOut;

    bool isNewMsg = this->ReceiveRequest(this->GetSocket(), dataTypeOut, dataOut);

    if (!isNewMsg || (isNewMsg && dataTypeOut == ""))
    {
        // No new messages
        return false;
    }
    if (dataTypeOut == "")
    {
        printf("Server disconnected!\n");
        return false;
    }

    if (dataTypeOut == "createaccountwebsuccess")
    {
        authentication::CreateAccountWebSuccess webSuccess;
        bool isDeserialized = webSuccess.ParseFromString(dataOut);
        if (!isDeserialized)
        {
            printf("error desserializing\n");
            return false;
        }

        requestId = (int)webSuccess.requestid();
        userId = (int)webSuccess.userid();
        success = true;
    }
    else if (dataTypeOut == "createaccountwebfailure")
    {
        authentication::CreateAccountWebFailure webFailure;
        bool isDeserialized = webFailure.ParseFromString(dataOut);
        if (!isDeserialized)
        {
            printf("error desserializing\n");
            return false;
        }

        requestId = (int)webFailure.requestid();
        errorReason = (int)webFailure.reason();
        success = false;
    }

    return true;
}
