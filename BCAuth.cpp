#include "BCAuth.h"
#include "HttpUtils.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

std::string getOAuthToken(const std::string& tenantId, const std::string& clientId, const std::string& clientSecret) {
	CURL* curl = curl_easy_init();
	std::string readBuffer;
	std::string accessToken = "";

	if (curl) {
		std::string url = "https://login.microsoftonline.com/" + tenantId + "/oauth2/v2.0/token";

		std::string postFields = "grant_type=client_credentials"
			"&client_id=" + clientId +
			"&client_secret=" + clientSecret +
			"&scope=https://api.businesscentral.dynamics.com/.default";

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		CURLcode res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			std::cerr << "OAuth Request Failed: " << curl_easy_strerror(res) << std::endl;
		}
		else {
			try {
				auto jsonResponse = json::parse(readBuffer);
				if (jsonResponse.contains("access_token")) {
					accessToken = jsonResponse["access_token"];
				}
				else {
					std::cerr << "Auth failed. Response: " << readBuffer << std::endl;
				}
			}
			catch (json::parse_error& e) {
				std::cerr << "OAuth JSON parse error: " << e.what() << std::endl;
			}
		}
		curl_easy_cleanup(curl);
	}
	return accessToken;
}
