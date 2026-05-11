#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Dom/JsonObject.h"
#include "JsonViewerWidget.generated.h"

class UScrollBox;
class UJsonRowWidget;
class UJsonTreeNode;

UCLASS()
class DT4MOB_API UJsonViewerWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(meta=(BindWidget))
    UScrollBox* ScrollBox;

    UPROPERTY(EditDefaultsOnly, Category="Json")
    TSubclassOf<UJsonRowWidget> RowWidgetClass;

    UFUNCTION(BlueprintCallable, Category="Json")
    void SetJsonString(const FString& JsonStr);

    void SetJsonObject(const TSharedPtr<FJsonObject>& JsonObject);

    void ToggleNode(UJsonTreeNode* Node);

private:
    UPROPERTY()
    TArray<UJsonTreeNode*> AllNodes;

    void ParseObject(const TSharedPtr<FJsonObject>& Obj, const FString& Key, int32 Depth);
    void ParseArray(const TArray<TSharedPtr<FJsonValue>>& Arr, const FString& Key, int32 Depth);
    void ParseValue(const TSharedPtr<FJsonValue>& Val, const FString& Key, int32 Depth);
    void Rebuild();
};
