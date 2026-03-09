#include "ClothComponentSource.h"
#include "ClothDeformerComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "SkeletalRenderPublic.h"
#include "GameFramework/Actor.h"

FName UClothComponentSource::Domains::Vertex("Vertex");

FText UClothComponentSource::GetDisplayName() const
{
	return FText::FromString("Cloth Deformer Component");
}

FName UClothComponentSource::GetBindingName() const
{
	return FName("ClothDeformer");
}

TSubclassOf<UActorComponent> UClothComponentSource::GetComponentClass() const
{
	// 绑定你的自定义组件
	return UClothDeformerComponent::StaticClass();
}

TArray<FName> UClothComponentSource::GetExecutionDomains() const
{
	return {Domains::Vertex};
}

// 内部辅助函数：获取同 Actor 上的 SkeletalMeshComponent，以读取渲染数据
static const USkeletalMeshComponent* GetSkeletalMeshFromCloth(const UActorComponent* InComponent)
{
	if (InComponent && InComponent->GetOwner())
	{
		return InComponent->GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	}
	return nullptr;
}

int32 UClothComponentSource::GetLodIndex(const UActorComponent* InComponent) const
{
	const USkeletalMeshComponent* SkeletalMeshComponent = GetSkeletalMeshFromCloth(InComponent);
	const FSkeletalMeshObject* SkeletalMeshObject = SkeletalMeshComponent ? SkeletalMeshComponent->MeshObject : nullptr;
	return SkeletalMeshObject ? SkeletalMeshObject->GetLOD() : 0;
}

uint32 UClothComponentSource::GetDefaultNumInvocations(const UActorComponent* InComponent, int32 InLod) const
{
	const USkeletalMeshComponent* SkinnedMeshComponent = GetSkeletalMeshFromCloth(InComponent);
	if (!SkinnedMeshComponent || !SkinnedMeshComponent->MeshObject)
	{
		return 0;
	}

	FSkeletalMeshRenderData const& SkeletalMeshRenderData = SkinnedMeshComponent->MeshObject->GetSkeletalMeshRenderData();
	FSkeletalMeshLODRenderData const* LodRenderData = &SkeletalMeshRenderData.LODRenderData[InLod];
	return LodRenderData->RenderSections.Num();
}

bool UClothComponentSource::GetComponentElementCountsForExecutionDomain(FName InDomainName, const UActorComponent* InComponent, int32 InLodIndex, TArray<int32>& OutElementCounts) const
{
	if (InDomainName != Domains::Vertex)
	{
		return false;
	}

	const USkeletalMeshComponent* SkinnedMeshComponent = GetSkeletalMeshFromCloth(InComponent);
	if (!SkinnedMeshComponent || !SkinnedMeshComponent->MeshObject)
	{
		return false;
	}

	FSkeletalMeshRenderData const& SkeletalMeshRenderData = SkinnedMeshComponent->MeshObject->GetSkeletalMeshRenderData();
	FSkeletalMeshLODRenderData const* LodRenderData = &SkeletalMeshRenderData.LODRenderData[InLodIndex];

	OutElementCounts.Reset(LodRenderData->RenderSections.Num());
	for (FSkelMeshRenderSection const& RenderSection : LodRenderData->RenderSections)
	{
		// 告诉 Optimus 每个渲染段的具体顶点数量，以便分配正确的 Compute Shader 线程数
		OutElementCounts.Add(RenderSection.NumVertices);
	}
	return true;
}