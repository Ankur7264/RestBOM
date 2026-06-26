#pragma once
#include <string>

// Fetches the OAuth 2.0 Bearer Token from Entra ID
std::string getOAuthToken(const std::string& tenantId, const std::string& clientId, const std::string& clientSecret);
