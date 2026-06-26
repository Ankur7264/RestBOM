
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include "BCAuth.h"
#include "BCApiClient.h"
#include "BomTree.h"
#include "BomProcessor.h"

using json = nlohmann::json;

// Recursive function to build the Multi-Level tree from JSON 
// and simultaneously extract a flattened list for the BomProcessor engine
BomNode buildNodeFromJson(const json& j, int currentDepth, std::vector<BomNode>& flatList) {
    BomNode node;
    node.partNumber = j.value("PartNumber", "");
    node.description = j.value("Description", "");
    node.baseUnitOfMeasure = j.value("BaseUnitOfMeasure", "PCS"); // Extracted from JSON, defaults to PCS
    node.depth = currentDepth;

    // --- DEFENSIVE PARSING FOR QUANTITY ---
    double quantity = 1.0; // Default fallback
    if (j.contains("Quantity") && !j["Quantity"].is_null()) {
        if (j["Quantity"].is_string()) {
            std::string qStr = j["Quantity"].get<std::string>();
            if (!qStr.empty()) {
                try {
                    quantity = std::stod(qStr);
                }
                catch (const std::invalid_argument&) {
                    // If the string is text instead of a number, fallback to 1.0
                    quantity = 1.0;
                }
            }
        }
        else if (j["Quantity"].is_number()) {
            quantity = j["Quantity"].get<double>();
        }
    }
    node.quantityPerParent = quantity;
    // --------------------------------------

    if (j.contains("Children") && j["Children"].is_array()) {
        for (const auto& childJson : j["Children"]) {
            node.children.push_back(buildNodeFromJson(childJson, currentDepth + 1, flatList));
        }
    }

    // Push the constructed node into our flat collection for easy OData syncing
    flatList.push_back(node);

    return node;
}

int main() {
    std::ifstream configFile("config.json");
    if (!configFile.is_open()) {
        std::cerr << "[ERROR] Could not open config.json!" << std::endl;
        std::cin.get(); // Pause so user can read error
        return 1;
    }
    json config;
    configFile >> config;

    std::string tenantId = config.value("tenantId", "");
    std::string clientId = config.value("clientId", "");
    std::string clientSecret = config.value("clientSecret", "");
    std::string envName = config.value("envName", "Production");
    std::string companyName = config.value("companyName", "My Company");

    std::string token = getOAuthToken(tenantId, clientId, clientSecret);
    if (token.empty()) {
        std::cerr << "[ERROR] Failed to fetch OAuth token!" << std::endl;
        std::cin.get();
        return 1;
    }

    std::cout << "\n======================================================" << std::endl;
    std::cout << " READING NESTED BOM FROM JSON                         " << std::endl;
    std::cout << "======================================================\n" << std::endl;

    std::ifstream bomFile("bom_data.json");
    if (!bomFile.is_open()) {
        std::cerr << "[ERROR] Could not open bom_data.json! Make sure it is in the project folder." << std::endl;
        std::cin.get();
        return 1;
    }

    std::vector<BomNode> allNodesFlat;
    BomNode topLevelAssembly;

    // --- SAFELY PARSE JSON ---
    try {
        json bomJson;
        bomFile >> bomJson;
        topLevelAssembly = buildNodeFromJson(bomJson, 0, allNodesFlat);
    }
    catch (const json::exception& e) {
        std::cerr << "[JSON ERROR] Failed to parse bom_data.json: " << e.what() << std::endl;
        std::cin.get();
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "[FATAL ERROR] " << e.what() << std::endl;
        std::cin.get();
        return 1;
    }

    std::cout << "[SUCCESS] Tree built in memory! Top Assembly: " << topLevelAssembly.partNumber << std::endl;

    std::cout << "\n======================================================" << std::endl;
    std::cout << " EXECUTING MASSIVE MULTI-LEVEL SYNC TO BC             " << std::endl;
    std::cout << "======================================================\n" << std::endl;

    // Pass the flatList to the processor. It handles the bottom-up depth sorting automatically!
    BomProcessor::syncMultiLevelBom(allNodesFlat, tenantId, envName, companyName, token);

    std::cout << "\n[PROCESS COMPLETE]! Press Enter to exit." << std::endl;
    std::string exitPause;
    std::getline(std::cin, exitPause);

    return 0;
}
