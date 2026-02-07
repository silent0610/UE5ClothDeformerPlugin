#pragma once

#include "CoreMinimal.h"
#include "IMeshMappingStrategy.h"

class FKnnMappingStrategy : public IMeshMappingStrategy
{
public:
    FKnnMappingStrategy(int32 k = 3);

    virtual FSparseMappingMatrix BuildMappingMatrix(const UE::Geometry::FDynamicMesh3 *HighPloyMesh, const UE::Geometry::FDynamicMesh3 *LowPolyMesh) override;

    virtual bool GenerateMapping(
        const USkeletalMesh* LowPoly,
        const USkeletalMesh* HighPoly,
        UMeshMappingAsset* OutAsset
    ) override;

    virtual TSharedRef<SWidget> CreateSettingsWidget() override;

    virtual FString GetStrategyName() const override;

private:
    int32 k_;
    const float kEpsilon_;
};