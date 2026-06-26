#pragma once
#include <string>
#include <vector>

struct BomNode {
    int depth;
    std::string partNumber;
    std::string description;
    double quantityPerParent = 1.0;
    std::string baseUnitOfMeasure = "PCS"; // Added for Business Central

    bool isRevision = false;
    std::string oldPartNumberToBlock = "";

    std::vector<BomNode> children;

    bool isSubAssembly() const {
        return !children.empty();
    }
};
