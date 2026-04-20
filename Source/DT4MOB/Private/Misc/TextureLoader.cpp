// // PortugalTextureLoader.cpp
// #include "PortugalTextureLoader.h"
// #include "IImageWrapper.h"
// #include "IImageWrapperModule.h"
// #include "Modules/ModuleManager.h"
// #include "Engine/Texture2DDynamic.h"
// #include "RenderUtils.h"

// void APortugalTextureLoader::LoadTextureFromFile(const FString& FilePath)
// {
//     // 1. Read raw bytes from disk
//     TArray<uint8> FileData;
//     if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
//     {
//         UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FilePath);
//         return;
//     }

//     // 2. Detect format & decode
//     IImageWrapperModule& ImageWrapperModule = 
//         FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

//     EImageFormat Format = ImageWrapperModule.DetectImageFormat(FileData.GetData(), FileData.Num());
//     TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(Format);

//     if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num()))
//     {
//         UE_LOG(LogTemp, Error, TEXT("Invalid image file."));
//         return;
//     }

//     TArray<uint8> RawData;
//     ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData);

//     // 3. Create dynamic texture
//     UTexture2DDynamic* DynTexture = UTexture2DDynamic::Create(
//         ImageWrapper->GetWidth(), ImageWrapper->GetHeight()
//     );
//     DynTexture->SRGB = true;
//     DynTexture->UpdateResource();

//     // Upload pixel data
//     ENQUEUE_RENDER_COMMAND(UpdateTextureData)(
//         [DynTexture, RawData](FRHICommandListImmediate& RHICmdList)
//         {
//             FTexture2DDynamicResource* Resource = 
//                 static_cast<FTexture2DDynamicResource*>(DynTexture->GetResource());
//             if (Resource)
//             {
//                 uint32 Stride = 0;
//                 void* TextureData = RHILockTexture2D(
//                     Resource->GetTexture2DRHI(), 0, RLM_WriteOnly, Stride, false
//                 );
//                 FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
//                 RHIUnlockTexture2D(Resource->GetTexture2DRHI(), 0, false);
//             }
//         }
//     );

//     // 4. Swap the material parameter
//     if (!DynMaterial)
//     {
//         DynMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
//         MeshComponent->SetMaterial(0, DynMaterial);
//     }
//     DynMaterial->SetTextureParameterValue(FName("DynamicTexture"), DynTexture);
// }