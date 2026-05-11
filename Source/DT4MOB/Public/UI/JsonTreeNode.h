#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JsonTreeNode.generated.h"

UCLASS(BlueprintType)
class DT4MOB_API UJsonTreeNode : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category="Json") FString Key;
    UPROPERTY(BlueprintReadOnly, Category="Json") FString Value;
    UPROPERTY(BlueprintReadOnly, Category="Json") int32 Depth = 0;
    UPROPERTY(BlueprintReadOnly, Category="Json") bool bIsExpandable = false;
    UPROPERTY(BlueprintReadWrite, Category="Json") bool bIsExpanded = true;
    int32 SubtreeSize = 0;
};
