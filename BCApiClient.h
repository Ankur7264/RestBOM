
#pragma once
#include <string>

//struct MaterialMasterData {
//    std::string itemNo;
//    std::string description;
//    bool isBlocked;
//    bool isSubAssembly; // Tells OData to use Prod. Order or Purchase
//};
struct MaterialMasterData {
	std::string itemNo;
	std::string description;
    std::string baseUnitOfMeasure = "PCS";
	bool isBlocked = false;       // Added default value
	bool isSubAssembly = false;   // Added default value
};

void createBusinessCentralItem(
    const std::string& tenantId, const std::string& envName,
    const std::string& companyName, const std::string& token,
    const MaterialMasterData& data);

void createProductionBOM(
    const std::string& tenantId, const std::string& envName,
    const std::string& companyName, const std::string& token,
    const std::string& bomNo, const std::string& description);

void createProductionBOMLine(
    const std::string& tenantId, const std::string& envName,
    const std::string& companyName, const std::string& token,
    const std::string& bomNo, int lineNo,
    const std::string& childPartNo, double quantity);

bool updateItemLifecycleStatus(
    const std::string& tenantId, const std::string& envName,
    const std::string& companyName, const std::string& token,
    const std::string& itemNo, bool blockStatus);

bool linkBomToItem(
    const std::string& tenantId, const std::string& envName,
    const std::string& companyName, const std::string& token,
    const std::string& itemNo, const std::string& bomNo);
