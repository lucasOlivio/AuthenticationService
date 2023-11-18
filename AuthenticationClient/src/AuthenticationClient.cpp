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

bool AuthenticationClient::ReceiveServerMsg(int& requestId, int& userId, bool& success, std::string& response)
{
    std::string dataTypeOut;
    std::string dataOut;
    requestId = -1;
    userId = -1;

    bool isNewMsg = this->ReceiveRequest(this->GetSocket(), dataTypeOut, dataOut);

    if (!isNewMsg || (isNewMsg && dataTypeOut == ""))
    {
        // No new messages
        return false;
    }
    if (dataTypeOut == "")
    {
        printf("Server disconnected!\n");
        this->Destroy();
        return false;
    }

    if (dataTypeOut == "createaccountwebsuccess")
    {
        // Success on account creation
        authentication::CreateAccountWebSuccess webSuccess;
        bool isDeserialized = webSuccess.ParseFromString(dataOut);
        if (!isDeserialized)
        {
            printf("error desserializing\n");
            return true;
        }

        requestId = (int)webSuccess.requestid();
        userId = (int)webSuccess.userid();
        success = true;
        response = "Authentication successful, account created on " + webSuccess.timecreation();
    }
    else if (dataTypeOut == "createaccountwebfailure")
    {
        // Failure on account creation
        authentication::CreateAccountWebFailure webFailure;
        webFailure.ParseFromString(dataOut);

        requestId = (int)webFailure.requestid();
        success = false;

        // Reason of failure as string msg
        if (webFailure.reason() == authentication::CreateAccountWebFailure_Reason_ACCOUNT_ALREADY_EXISTS)
        {
            response = "ACCOUNT ALREADY EXISTS";
        }
        else if (webFailure.reason() == authentication::CreateAccountWebFailure_Reason_INTERNAL_SERVER_ERROR)
        {
            response = "INTERNAL SERVER ERROR";
        }
    }

    return true;
}
