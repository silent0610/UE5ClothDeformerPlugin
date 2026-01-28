#include "ClothDeformerComponent.h"
#include "OnnxModelInstance.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformFilemanager.h"

// 包含ONNX Runtime头文件用于测试
#if PLATFORM_WINDOWS && PLATFORM_64BITS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <psapi.h> // 用于检测加载的模块
#endif
#include "onnxruntime_cxx_api.h"
#if PLATFORM_WINDOWS && PLATFORM_64BITS
#include "Windows/HideWindowsPlatformTypes.h"
#endif
#include "DynamicMesh/DynamicMesh3.h"
#include "MeshDescriptionToDynamicMesh.h"
#include "IMeshMappingStrategy.h"
#include "KnnMeshMapping.h"
#include "SurfaceProjection.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "AssetToolsModule.h"
#include "Factories/DataAssetFactory.h"
#include "MeshMappingAsset.h"
// The UClothDeformerComponent must define its destructor in the .cpp file.
// This is required because the header file only has a forward declaration of FOnnxModelInstance.
// The TUniquePtr<FOnnxModelInstance> member needs the full definition of FOnnxModelInstance to be able to call its destructor.
// By defining the UClothDeformerComponent destructor here, we ensure that the compiler sees the full definition of FOnnxModelInstance
// (included from OnnxModelInstance.h) before it tries to generate the code to destroy the modelInstance_ member.
UClothDeformerComponent::~UClothDeformerComponent()
{
    // Intentionally empty. The key is that this definition exists here in the .cpp file.
}

UClothDeformerComponent::UClothDeformerComponent()
{
    // 启用Tick，以便每帧运行
    PrimaryComponentTick.bCanEverTick = true;
}
void UClothDeformerComponent::BeginPlay()
{
    Super::BeginPlay();

    // 尝试初始化ONNX模型
    UE_LOG(LogTemp, Log, TEXT("Cloth Deformer Component BeginPlay - attempting to initialize model..."));

    if (Initialize())
    {
        UE_LOG(LogTemp, Log, TEXT("Cloth Deformer Component initialized successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Cloth Deformer Component initialization failed - no model asset specified"));
    }
}
void UClothDeformerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 清理资源
    modelInstance_.Reset();

    Super::EndPlay(EndPlayReason);
}
bool UClothDeformerComponent::Initialize()
{
    try
    {
        if (modelAsset_)
        {
            modelInstance_ = MakeUnique<FOnnxModelInstance>(modelAsset_);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Cloth Deformer Component initialization failed - no model asset specified"));
            modelInstance_.Reset();
            bIsInitialized = false;
            return false;
        }

        if (modelInstance_ && modelInstance_->IsInitialized())
        {
            UE_LOG(LogTemp, Log, TEXT("ONNX Model Instance created successfully"));
            bIsInitialized = true;
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to initialize ONNX Model Instance"));
            modelInstance_.Reset();
            bIsInitialized = false;
            return false;
        }
    }
    catch (const std::exception &e)
    {
        UE_LOG(LogTemp, Error, TEXT("Exception creating ONNX Model Instance: %s"), UTF8_TO_TCHAR(e.what()));
        modelInstance_.Reset();
        bIsInitialized = false;
        return false;
    }
}

bool UClothDeformerComponent::RunInference(const TArray<float> &InputData, TArray<float> &OutputData)
{
    if (!IsInitialized())
    {
        UE_LOG(LogTemp, Warning, TEXT("RunInference called but component is not initialized"));
        return false;
    }
    
    if (!modelInstance_) return false;
    return modelInstance_->Run(InputData, OutputData);
}

bool UClothDeformerComponent::IsInitialized() const
{
    return bIsInitialized && modelInstance_ && modelInstance_->IsInitialized();
}

void UClothDeformerComponent::Reset()
{
    modelInstance_.Reset();
    bIsInitialized = false;
    UE_LOG(LogTemp, Log, TEXT("ONNX Component reset"));
}


// Editor-only includes
#if WITH_EDITOR


// Helper function to convert UStaticMesh to FDynamicMesh3
bool ConvertStaticToDynamicMesh(UStaticMesh *InStaticMesh, UE::Geometry::FDynamicMesh3 &OutDynamicMesh)
{
    if (!InStaticMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("ConvertStaticToDynamicMesh: Input Static Mesh is null."));
        return false;
    }

    // Ensure we have a valid Mesh Description
    FMeshDescription *MeshDescription = InStaticMesh->GetMeshDescription(0);
    if (!MeshDescription)
    {
        UE_LOG(LogTemp, Error, TEXT("ConvertStaticToDynamicMesh: Failed to get Mesh Description for %s."), *InStaticMesh->GetName());
        return false;
    }

    FMeshDescriptionToDynamicMesh Converter;
    Converter.Convert(MeshDescription, OutDynamicMesh);
    return true;
}

void UClothDeformerComponent::BuildMappingMatrix()
{
    UE_LOG(LogTemp, Log, TEXT("Starting to build mapping matrix..."));

    // 1. Validate input meshes
    if (!testHighMesh_ || !testLowMesh_)
    {
        UE_LOG(LogTemp, Error, TEXT("BuildMappingMatrix: High-poly or Low-poly mesh not set."));
        return;
    }

    // 2. Convert to DynamicMesh
    UE::Geometry::FDynamicMesh3 HighPolyMesh, LowPolyMesh;
    if (!ConvertStaticToDynamicMesh(testHighMesh_, HighPolyMesh) || !ConvertStaticToDynamicMesh(testLowMesh_, LowPolyMesh))
    {
        return; // Errors logged in helper
    }

    // 3. Create Strategy based on enum
    TUniquePtr<IMeshMappingStrategy> Strategy{nullptr};
    switch (MappingStrategy)
    {
    case EMappingStrategyType::KNN:
        Strategy = MakeUnique<FKnnMappingStrategy>();
        break;
    case EMappingStrategyType::SurfaceProjection:
        Strategy = MakeUnique<FSurfaceProjection>();
        break;
    default:
        UE_LOG(LogTemp, Error, TEXT("BuildMappingMatrix: Unknown mapping strategy selected."));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("Using strategy: %s"), *Strategy->GetStrategyName());

    // 4. Build Matrix
    BuiltMappingMatrix = Strategy->BuildMappingMatrix(&HighPolyMesh, &LowPolyMesh);

    // Log result
    FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Successfully built mapping matrix (%d x %d, %d non-zero)."), BuiltMappingMatrix.NumRow, BuiltMappingMatrix.NumCol, BuiltMappingMatrix.Value.Num())));
    Info.ExpireDuration = 5.0f;
    FSlateNotificationManager::Get().AddNotification(Info);

    UE_LOG(LogTemp, Log, TEXT("BuildMappingMatrix successful. Rows: %d, Cols: %d, NNZ: %d"), BuiltMappingMatrix.NumRow, BuiltMappingMatrix.NumCol, BuiltMappingMatrix.Value.Num());
}
void UClothDeformerComponent::SaveMappingToAsset()
{
    // 1. Validate built data
    if (BuiltMappingMatrix.Value.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("SaveMappingToAsset: No mapping matrix has been built. Please run BuildMappingMatrix first."));
        return;
    }

    // 2. Prepare for asset creation
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

    // Default path and name
    // 使用简单的字符串拼接，避免除号运算符重载带来的歧义
    FString DefaultPath = TEXT("/Game/");
    FString DefaultName = FString::Printf(TEXT("MM_%s_to_%s"), *testLowMesh_->GetName(), *testHighMesh_->GetName());
    FString BasePackageName = DefaultPath + DefaultName;

    FString PackagePath;
    FString AssetName;

    // Create unique asset name
    AssetToolsModule.Get().CreateUniqueAssetName(BasePackageName, TEXT(""), PackagePath, AssetName);

    // 3. Create Asset 
    // 关键修改：Factory 传 nullptr。AssetTools 会自动处理简单的 DataAsset 创建。
    UObject* NewAsset = AssetToolsModule.Get().CreateAsset(
        AssetName,
        FPackageName::GetLongPackagePath(PackagePath),
        UMeshMappingAsset::StaticClass(),
        nullptr
    );

    if (UMeshMappingAsset* MappingAssetRef = Cast<UMeshMappingAsset>(NewAsset))
    {
        // 4. Copy data
        MappingAssetRef->MappingData = BuiltMappingMatrix;

        // 标记资产为"脏"，这样编辑器会提示用户保存，且图标上会出现小星号
        MappingAssetRef->MarkPackageDirty();

        // 5. Automatically assign it back to the component
        this->MappingAsset = MappingAssetRef;

        UE_LOG(LogTemp, Log, TEXT("Successfully saved mapping matrix to asset: %s"), *MappingAssetRef->GetName());

        // 显示编辑器右下角的通知
        FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Saved asset %s"), *MappingAssetRef->GetName())));
        Info.ExpireDuration = 5.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SaveMappingToAsset: Failed to create new UMeshMappingAsset."));
    }
}
void UClothDeformerComponent::GenerateAndSaveMappingAsset()
{
    BuildMappingMatrix();
    if (BuiltMappingMatrix.Value.Num() > 0)
    {
        SaveMappingToAsset();
    }
}

void UClothDeformerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 确保已初始化且有测试数据
	if (IsInitialized() && TestInputArray.Num() > 0)
	{
		if (RunInference(TestInputArray, TestOutputArray))
		{
			// For debugging, log the first element of the output array
			if (TestOutputArray.Num() > 0)
			{
				UE_LOG(LogTemp, Log, TEXT("Inference successful. First output element: %f"), TestOutputArray[0]);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("RunInference failed in TickComponent."));
		}
	}
}


#endif // WITH_EDITOR