#pragma once

#include "CoreMinimal.h"
#include "OptimusComponentSource.h"
#include "ClothComponentSource.generated.h"

UCLASS(meta=(DisplayName="Cloth Deformer Component"))
class CLOTH_API UClothComponentSource : public UOptimusComponentSource
{
	GENERATED_BODY()
public:
	struct Domains
	{
		static FName Vertex;
	};

	// UOptimusComponentSource 接口重写
	virtual FText GetDisplayName() const override;
	virtual FName GetBindingName() const override;
	virtual TSubclassOf<UActorComponent> GetComponentClass() const override;
	virtual TArray<FName> GetExecutionDomains() const override;
	virtual int32 GetLodIndex(const UActorComponent* InComponent) const override;
	virtual uint32 GetDefaultNumInvocations(const UActorComponent* InComponent, int32 InLod) const override;
	virtual bool GetComponentElementCountsForExecutionDomain(FName InDomainName, const UActorComponent* InComponent, int32 InLodIndex, TArray<int32>& OutElementCounts) const override;
};