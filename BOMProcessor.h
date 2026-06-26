#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_set> 
#include "BomTree.h"
#include "BCApiClient.h"

class BomProcessor {
public:
    static void syncMultiLevelBom(
        std::vector<BomNode>& nodes,
        const std::string& tenantId,
        const std::string& envName, const std::string& companyName,
        const std::string& token)
    {
        // 1. Sort bottom-up so children are processed before parents
        std::sort(nodes.begin(), nodes.end(), [](const BomNode& a, const BomNode& b) {
            return a.depth > b.depth;
            });

        // 2. Registry to track created Items to prevent 400 Bad Request
        std::unordered_set<std::string> createdItems;

        for (auto& node : nodes) {
            std::cout << "[PROCESSING] " << node.partNumber << " (Level " << node.depth << ")" << std::endl;

            // Only create the Item if we haven't processed it in this run
            if (createdItems.find(node.partNumber) == createdItems.end()) {

                MaterialMasterData partData;
                partData.itemNo = node.partNumber;
                partData.description = node.description;
                // Ensure this matches the UoM code in your Business Central instance (e.g., "PCS", "EA")
                partData.baseUnitOfMeasure = node.baseUnitOfMeasure.empty() ? "PCS" : node.baseUnitOfMeasure;
                partData.isSubAssembly = !node.children.empty();
                partData.isBlocked = false;

                createBusinessCentralItem(tenantId, envName, companyName, token, partData);

                // Mark as created so we skip it if it appears in another branch
                createdItems.insert(node.partNumber);
            }
            else {
                std::cout << "[INFO] Skipping duplicate item creation: " << node.partNumber << std::endl;
            }

            // 3. Always create the BOM structure and link lines
            if (!node.children.empty()) {
                std::string bomNo = ("BOM-" + node.partNumber).substr(0, 20);

                createProductionBOM(tenantId, envName, companyName, token, bomNo, node.description);

                int lineNo = 10000;
                for (const auto& child : node.children) {
                    createProductionBOMLine(tenantId, envName, companyName, token, bomNo, lineNo, child.partNumber, child.quantityPerParent);
                    lineNo += 10000;
                }

                linkBomToItem(tenantId, envName, companyName, token, node.partNumber, bomNo);
            }
        }
    }
};
