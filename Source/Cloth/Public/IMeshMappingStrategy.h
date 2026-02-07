#pragma once

#include "CoreMinimal.h"
#include "UObject/NameTypes.h" // For FString
#include "Widgets/SWidget.h" // 策略需要返回 UI 组件

class USkeletalMesh;
class UMeshMappingAsset;

struct FSparseMappingMatrix;

namespace UE::Geometry
{
    class FDynamicMesh3;
}

class IMeshMappingStrategy
{
public:
    virtual ~IMeshMappingStrategy() = default;
    virtual FSparseMappingMatrix BuildMappingMatrix(const UE::Geometry::FDynamicMesh3* HighPolyMesh, const UE::Geometry::FDynamicMesh3* LowPolyMesh) = 0;


    virtual bool GenerateMapping(
        const USkeletalMesh* LowPoly,
        const USkeletalMesh* HighPoly,
        UMeshMappingAsset* OutAsset
    ) = 0;

    virtual TSharedRef<SWidget> CreateSettingsWidget() = 0;

    virtual FString GetStrategyName() const = 0;
};