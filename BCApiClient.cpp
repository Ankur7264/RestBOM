#include "BCApiClient.h"
#include "HttpUtils.h"
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string urlEncode(CURL* curl, const std::string& str) {
    char* escaped = curl_easy_escape(curl, str.c_str(), 0);
    std::string result(escaped);
    curl_free(escaped);
    return result;
}

// 1. Create Item (OData)
void createBusinessCentralItem(
    const std::string& tenantId, const std::string& envName,
    const std::string& companyName, const std::string& token,
    const MaterialMasterData& data)
{
    CURL* curl = curl_easy_init();
    if (!curl) return;

    std::string safeCompanyName = urlEncode(curl, companyName);
    std::string url = "https://api.businesscentral.dynamics.com/v2.0/" + tenantId + "/" + envName +
        "/ODataV4/Company('" + safeCompanyName + "')/ItemOData";

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    json body = {
         {"No", data.itemNo},
         {"Description", data.description},
         {"Base_Unit_of_Measure", data.baseUnitOfMeasure},
         {"Replenishment_System", data.isSubAssembly ? "Prod. Order" : "Purchase"},
         {"Blocked", data.isBlocked}
    };

    std::string jsonStr = body.dump();
    std::string responseString;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

    long responseCode = 0;
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

    if (responseCode == 200 || responseCode == 201) {
        std::cout << "[SUCCESS] Created Item: " << data.itemNo << std::endl;
    }
    else {
        std::cerr << "[ERROR] Item failed: " << data.itemNo << ". Code: " << responseCode << std::endl;
        std::cerr << "Details: " << responseString << std::endl;
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

// 2. Create Production BOM Header (OData)
void createProductionBOM(
    const std::string& tenantId, const std::string& envName,
    const std::string& companyName, const std::string& token,
    const std::string& bomNo, const std::string& description)
{
    CURL* curl = curl_easy_init();
    if (!curl) return;

    std::string safeCompanyName = urlEncode(curl, companyName);
    std::string url = "https://api.businesscentral.dynamics.com/v2.0/" + tenantId + "/" + envName +
        "/ODataV4/Company('" + safeCompanyName + "')/ProductionBOMs";

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // FIXED: Added Unit_of_Measure_Code back so it doesn't crash searching for a blank UoM!
    json body = {
        {"No", bomNo},
        {"Description", description},
        {"Unit_of_Measure_Code", "PCS"},
        {"Status", "New"}
    };

    std::string jsonStr = body.dump();
    std::string responseString;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

    long responseCode = 0;
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

    if (responseCode == 200 || responseCode == 201) {
        std::cout << "[SUCCESS] Created BOM Header: " << bomNo << std::endl;
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

// 3. Create Production BOM Line Items (OData)
void createProductionBOMLine(
    const std::string& tenantId, const std::string& envName,
    const std::string& companyName, const std::string& token,
    const std::string& bomNo, int lineNo,
    const std::string& childPartNo, double quantity)
{
    CURL* curl = curl_easy_init();
    if (!curl) return;

    std::string safeCompanyName = urlEncode(curl, companyName);
    std::string url = "https://api.businesscentral.dynamics.com/v2.0/" + tenantId + "/" + envName +
        "/ODataV4/Company('" + safeCompanyName + "')/ProductionBOMLines";

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    json body = {
        {"Production_BOM_No", bomNo},
        {"Line_No", lineNo},
        {"Type", "Item"},
        {"No", childPartNo},
        {"Quantity_per", quantity}
    };

    std::string jsonStr = body.dump();
    std::string responseString;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

    long responseCode = 0;
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

    if (responseCode == 200 || responseCode == 201) {
        std::cout << "[SUCCESS] Attached Line " << childPartNo << " to " << bomNo << std::endl;
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

// 4. Update Lifecycle Status 
bool updateItemLifecycleStatus(
    const std::string& tenantId, const std::string& envName,
    const std::string& companyName, const std::string& token,
    const std::string& itemNo, bool blockStatus)
{
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string safeCompanyName = urlEncode(curl, companyName);
    std::string url = "https://api.businesscentral.dynamics.com/v2.0/" + tenantId + "/" + envName +
        "/ODataV4/Company('" + safeCompanyName + "')/ItemOData('" + itemNo + "')";

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "If-Match: *");

    json body = { {"Blocked", blockStatus} };
    std::string jsonStr = body.dump();
    std::string responseString;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

    long responseCode = 0;
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

    bool status = (responseCode == 200 || responseCode == 204);
    if (status) std::cout << "[SUCCESS] Obsoleted Item: " << itemNo << std::endl;

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return status;
}

// 5. Link BOM to Item
bool linkBomToItem(
    const std::string& tenantId, const std::string& envName,
    const std::string& companyName, const std::string& token,
    const std::string& itemNo, const std::string& bomNo)
{
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string safeCompanyName = urlEncode(curl, companyName);
    std::string url = "https://api.businesscentral.dynamics.com/v2.0/" + tenantId + "/" + envName +
        "/ODataV4/Company('" + safeCompanyName + "')/ItemOData('" + itemNo + "')";

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "If-Match: *");

    json body = {
        {"Production_BOM_No", bomNo}
    };

    std::string jsonStr = body.dump();
    std::string responseString;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

    long responseCode = 0;
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

    bool success = false;
    if (responseCode == 200 || responseCode == 204) {
        std::cout << "[SUCCESS] Linked BOM " << bomNo << " to Item " << itemNo << std::endl;
        success = true;
    }
    else {
        std::cerr << "[ERROR] Failed to link BOM. Code: " << responseCode << std::endl;
        std::cerr << "Details: " << responseString << std::endl;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return success;
}
