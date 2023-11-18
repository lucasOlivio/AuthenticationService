#include "myutils.h"
#include <iomanip>
#include <ctime>
#include <sstream>
#include <random>
#pragma warning(disable : 4996)
#include <openssl/sha.h>

std::string TimeNow(const char* format)
{
    struct tm newtime;
    time_t now = time(0);
    auto tm = localtime_s(&newtime, &now);
    
    std::string formattedTime = "";
    if (tm == 0) {
        // Conversion successful
        char buffer[80]; // Adjust the buffer size as needed
        strftime(buffer, sizeof(buffer), format, &newtime);
       formattedTime = buffer;
    }

    return formattedTime;
}

std::string GenerateRandomSalt(const int saltLength)
{
    const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, sizeof(charset) - 2);

    std::string salt;
    salt.reserve(saltLength);

    for (int i = 0; i < saltLength; ++i) {
        salt += charset[distribution(generator)];
    }

    return salt;
}

std::string HashPassword(const std::string& password, const std::string& salt) 
{
    std::string combined = salt + password;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, combined.c_str(), combined.length());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}
