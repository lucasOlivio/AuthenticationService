#pragma once
#include <string>

// Get the time now in string format
// https://stackoverflow.com/a/16358264/10510337
std::string TimeNow(const char* format);

// Function to generate a random salt
std::string GenerateRandomSalt(const int saltLength);

// Function to hash the combination of password and salt using SHA256
std::string HashPassword(const std::string& password, const std::string& salt);