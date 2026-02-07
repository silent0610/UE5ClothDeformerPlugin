#pragma once

#include "CoreMinimal.h"

class USkeletalMeshComponent;
class CLOTH_API FInputAdapterBase
{
public:
	virtual  ~FInputAdapterBase() = default;

	virtual void Initialize(USkeletalMeshComponent* inSkelMeshComp) = 0;

	virtual TMap<FString, TArray<float>> ExtractInputs(float deltaTime) = 0;

	virtual void Reset() = 0;
};