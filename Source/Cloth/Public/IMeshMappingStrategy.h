#pragma once

#include "CoreMinimal.h"
#include "UObject/NameTypes.h" // For FString

struct FSparseMappingMatrix;

namespace UE::Geometry
{
    class FDynamicMesh3;
}

class IMeshMappingStrategy
{
public:
    virtual ~IMeshMappingStrategy() {};
    virtual FSparseMappingMatrix BuildMappingMatrix(const UE::Geometry::FDynamicMesh3* HighPolyMesh, const UE::Geometry::FDynamicMesh3* LowPolyMesh) = 0;
    virtual FString GetStrategyName() const = 0;
};