#include "UI/JsonViewerWidget.h"
#include "UI/JsonRowWidget.h"
#include "UI/JsonTreeNode.h"
#include "Components/ScrollBox.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"

void UJsonViewerWidget::SetJsonString(const FString& JsonStr)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
        return;
    SetJsonObject(JsonObject);
}

void UJsonViewerWidget::SetJsonObject(const TSharedPtr<FJsonObject>& JsonObject)
{
    if (!JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("JsonViewer: SetJsonObject called with null JsonObject"));
        return;
    }
    if (!RowWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("JsonViewer: RowWidgetClass is not set — assign WBP_JsonRow in Blueprint defaults"));
        return;
    }
    AllNodes.Empty();
    ParseObject(JsonObject, TEXT(""), 0);
    UE_LOG(LogTemp, Log, TEXT("JsonViewer: parsed %d nodes"), AllNodes.Num());
    Rebuild();
}

void UJsonViewerWidget::ToggleNode(UJsonTreeNode* Node)
{
    Node->bIsExpanded = !Node->bIsExpanded;
    Rebuild();
}

void UJsonViewerWidget::ParseObject(const TSharedPtr<FJsonObject>& Obj, const FString& Key, int32 Depth)
{
    if (!Obj.IsValid()) return;

    UJsonTreeNode* Node = NewObject<UJsonTreeNode>(this);
    Node->Key          = Key.IsEmpty() ? TEXT("{...}") : Key;
    Node->Depth        = Depth;
    Node->bIsExpandable = true;
    Node->bIsExpanded  = (Depth < 2);
    int32 Idx = AllNodes.Add(Node);

    for (const auto& Pair : Obj->Values)
        ParseValue(Pair.Value, Pair.Key, Depth + 1);

    Node->SubtreeSize = AllNodes.Num() - Idx - 1;
}

void UJsonViewerWidget::ParseArray(const TArray<TSharedPtr<FJsonValue>>& Arr, const FString& Key, int32 Depth)
{
    UJsonTreeNode* Node = NewObject<UJsonTreeNode>(this);
    Node->Key          = Key.IsEmpty() ? TEXT("[...]") : Key;
    Node->Value        = FString::Printf(TEXT("[%d]"), Arr.Num());
    Node->Depth        = Depth;
    Node->bIsExpandable = Arr.Num() > 0;
    Node->bIsExpanded  = (Depth < 2);
    int32 Idx = AllNodes.Add(Node);

    for (int32 i = 0; i < Arr.Num(); i++)
        ParseValue(Arr[i], FString::Printf(TEXT("[%d]"), i), Depth + 1);

    Node->SubtreeSize = AllNodes.Num() - Idx - 1;
}

void UJsonViewerWidget::ParseValue(const TSharedPtr<FJsonValue>& Val, const FString& Key, int32 Depth)
{
    if (!Val.IsValid()) return;

    switch (Val->Type)
    {
    case EJson::Object:
        ParseObject(Val->AsObject(), Key, Depth);
        return;
    case EJson::Array:
        ParseArray(Val->AsArray(), Key, Depth);
        return;
    default:
        break;
    }

    UJsonTreeNode* Node = NewObject<UJsonTreeNode>(this);
    Node->Key   = Key;
    Node->Depth = Depth;

    switch (Val->Type)
    {
    case EJson::String:
        Node->Value = TEXT("\"") + Val->AsString() + TEXT("\"");
        break;
    case EJson::Number:
    {
        double Num = Val->AsNumber();
        Node->Value = (FMath::Abs(FMath::Frac(Num)) < 1e-9 && FMath::Abs(Num) < 1e15)
            ? FString::Printf(TEXT("%.0f"), Num)
            : FString::Printf(TEXT("%g"), Num);
        break;
    }
    case EJson::Boolean:
        Node->Value = Val->AsBool() ? TEXT("true") : TEXT("false");
        break;
    case EJson::Null:
        Node->Value = TEXT("null");
        break;
    default:
        Node->Value = TEXT("?");
        break;
    }

    AllNodes.Add(Node);
}

void UJsonViewerWidget::Rebuild()
{
    UE_LOG(LogTemp, Log, TEXT("JsonViewer: Rebuild — %d nodes, ScrollBox valid: %d"), AllNodes.Num(), ScrollBox != nullptr);
    ScrollBox->ClearChildren();

    int32 i = 0;
    while (i < AllNodes.Num())
    {
        UJsonTreeNode* Node = AllNodes[i];
        UJsonRowWidget* Row = CreateWidget<UJsonRowWidget>(this, RowWidgetClass);
        Row->InitRow(Node, this);
        ScrollBox->AddChild(Row);

        if (Node->bIsExpandable && !Node->bIsExpanded)
            i += Node->SubtreeSize + 1;
        else
            ++i;
    }
}
