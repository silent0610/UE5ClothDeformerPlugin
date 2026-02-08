#pragma once

#include "CoreMinimal.h"
#include "IMeshMappingStrategy.h" // 必须包含我们所继承的接口

class FSurfaceProjection : public IMeshMappingStrategy
{
    virtual FSparseMappingMatrix BuildMappingMatrix(
        const UE::Geometry::FDynamicMesh3 *HighPolyMesh,
        const UE::Geometry::FDynamicMesh3 *LowPolyMesh) override;
    virtual FString GetStrategyName() const override;
    virtual bool GenerateMapping(
        const USkeletalMesh* LowPoly,
        const USkeletalMesh* HighPoly,
        UMeshMappingAsset* OutAsset
    ) override;

    virtual TSharedRef<SWidget> CreateSettingsWidget() override;
};