// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Widgets/SFlowHistory.h"
#include "Asset/FlowAssetEditor.h"
#include "FlowEditorStyle.h"
#include "FlowSubsystem.h"
#include "FlowAsset.h"
#include "Nodes/Route/FlowNode_SubGraph.h"
#include "Graph/FlowGraphEditor.h"

#include "Widgets/Input/STextComboBox.h"
#include "DetailLayoutBuilder.h"

struct FHistoryTreeEntry : public TSharedFromThis<FHistoryTreeEntry>
{
	int Index = 0;
	FFlowHistoryEntry FlowHistoryEntry;

	FHistoryTreeEntry() = default;
	FHistoryTreeEntry(int index, const FFlowHistoryEntry& flowHistoryEntry)
		:Index(index)
		, FlowHistoryEntry(flowHistoryEntry)
	{}
};

void SFlowHistory::Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditor)
{
	FlowAssetEditor = InFlowAssetEditor;
	ensure(FlowAssetEditor.IsValid());
	FlowAsset = FlowAssetEditor.Pin()->GetFlowAsset();
	ensure(FlowAsset);

	UFlowSubsystem::OnFlowHistoryAdded.AddRaw(this, &SFlowHistory::HistoryEntryAdded);

	InstancesHistory.Empty();
	const auto& AllInstacesHistory = UFlowSubsystem::GetInstancedFlowHistory(FlowAsset->AssetGuid);
	for (const auto& InstanceHistory : AllInstacesHistory)
	{
		InstancesHistory.Add(InstanceHistory.InstanceName, InstanceHistory);
	}

	GatherInstanceNames();

	HeaderRowWidget =
		SNew(SHeaderRow)
		.Visibility(EVisibility::Visible)
		.CanSelectGeneratedColumn(true);

	TArray<FString> HeaderColumms = { "Index", "Type", "Name", "GUID" };

	for (const auto& Columm : HeaderColumms)
	{
		const auto ColumnArgs = SHeaderRow::Column(FName(*Columm))
			.DefaultLabel(FText::FromString(Columm))
			.FillWidth(1.0f);

		HeaderRowWidget->AddColumn(ColumnArgs);
	}

	this->ChildSlot
		[
			SNew(SBorder)
			.Padding(2.0f)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(3.0f)
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Instance history: "))
						.Font(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 10, "Bold"))
						
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SAssignNew(InstanceComboBox, STextComboBox)
						.OptionsSource(&InstancesNames)
						.OnSelectionChanged(this, &SFlowHistory::InstancesSelectionChanged)
					]
				]
				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SScrollBox)
					+SScrollBox::Slot()
					[
						SAssignNew(HistoryTreeView, STreeView<FHistoryTreeItemPtr>)
						// Determined by the mode
						.SelectionMode(ESelectionMode::Single)

						// Point the tree to our array of root-level items.  Whenever this changes, we'll call RequestTreeRefresh()
						.TreeItemsSource(&RootTreeItems)

						// Generates the actual widget for a tree item
						.OnGenerateRow(this, &SFlowHistory::OnGenerateRow)

						// Called to child items for any given parent item
						.OnGetChildren(this, &SFlowHistory::OnGetChildren)

						// Called when the user double-clicks with LMB on an item in the list
						.OnMouseButtonDoubleClick(this, &SFlowHistory::OnHistoryTreeDoubleClick)

						// Header for the tree
						.HeaderRow(HeaderRowWidget)
					]
				]
			]
		];

	InstanceComboBox->SetSelectedItem(InstancesNames[0]);
}

SFlowHistory::~SFlowHistory()
{
	UFlowSubsystem::OnFlowHistoryAdded.RemoveAll(this);
	RootTreeItems.Empty();
}

void SFlowHistory::ClearHistory()
{
	RootTreeItems.Empty();
	HistoryTreeView->RequestTreeRefresh();
}

TSharedRef<ITableRow> SFlowHistory::OnGenerateRow(FHistoryTreeItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	FText NodeName = GetNodeDisplayNameFromGUID(Item->FlowHistoryEntry.NodeGUID);

	return
		SNew(STableRow<FHistoryTreeItemPtr>, OwnerTable)
		.ToolTipText(NodeName)
		[
			SNew(SFlowHistoryRow)
			.Index(Item->Index)
			.FlowHistoryEntry(Item->FlowHistoryEntry)
			.EntryNodeName(NodeName)
		];
}

void SFlowHistory::OnGetChildren(FHistoryTreeItemPtr InItem, TArray<FHistoryTreeItemPtr>& OutChildren)
{
}

void SFlowHistory::OnHistoryTreeDoubleClick(FHistoryTreeItemPtr TreeItem)
{
	if (auto flowGraph = FlowAssetEditor.Pin()->GetFlowGraph())
	{
		if (auto flowNode = FlowAsset->GetNode(TreeItem->FlowHistoryEntry.NodeGUID))
		{
			flowGraph->JumpToNode(flowNode->GetGraphNode());
		}
	}
}

void SFlowHistory::InstancesSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	ClearHistory();

	if (NewSelection.IsValid())
	{
		if (auto Instance = InstancesHistory.Find(FName(*NewSelection)))
		{
			const auto& HistoryEntries = Instance->History.HistoryEntries;

			for (int i = 0; i < HistoryEntries.Num(); i++)
			{
				FHistoryTreeEntry Entry = FHistoryTreeEntry(i, HistoryEntries[i]);
				RootTreeItems.Add(MakeShared<FHistoryTreeEntry>(Entry));
			}

			HistoryTreeView->RequestTreeRefresh();
		}
	}
}

void SFlowHistory::HistoryEntryAdded(const FGuid & FlowAssetGuid, const FFlowInstanceHistory & InstanceHistory)
{
	if (FlowAsset && FlowAsset->AssetGuid == FlowAssetGuid)
	{	
		InstancesHistory.Add(InstanceHistory.InstanceName, InstanceHistory);
		GatherInstanceNames();
	}
}

void SFlowHistory::GatherInstanceNames()
{
	InstancesNames.Empty();

	for (const auto& history : InstancesHistory)
	{
		InstancesNames.Add(MakeShareable(new FString(history.Key.ToString())));
	}

	if (InstancesNames.IsEmpty())
	{
		InstancesNames.Add(MakeShareable(new FString(TEXT("Root"))));
	}

	if (InstanceComboBox.IsValid())
	{
		InstanceComboBox->ClearSelection();
		InstanceComboBox->RefreshOptions();

		InstanceComboBox->SetSelectedItem(InstancesNames[0]);
	}
}

FText SFlowHistory::GetNodeDisplayNameFromGUID(const FGuid& guid) const
{
	FText nodeName = FText::FromString(guid.ToString());

	if (FlowAsset)
	{
		if (auto graphNode = FlowAsset->GetNode(guid))
		{
			nodeName = graphNode->GetNodeTitle();
		}
	}

	return nodeName;
}

void SFlowHistoryRow::Construct(const FArguments& InArgs)
{
	TypeSlateBrush = FFlowEditorStyle::GetBrush("FlowGraph.Arrow");

	const auto& triggerType = InArgs._FlowHistoryEntry.Type;
	FText triggerTypeText = FText();
	FLinearColor triggerTypeColor = FLinearColor();

	switch (triggerType)
	{
		case EFlowNodeTriggerType::Input:
		{
			triggerTypeText = FText::FromString(FString::Printf(TEXT("In(%d)"), InArgs._FlowHistoryEntry.PinIndex));
			triggerTypeColor = FLinearColor::Green;
			break;
		}
		case EFlowNodeTriggerType::Output:
		{
			triggerTypeText = FText::FromString(FString::Printf(TEXT("Out(%d)"), InArgs._FlowHistoryEntry.PinIndex));
			triggerTypeColor = FLinearColor::Gray;
			break;
		}
		default:
		{
			break;
		}
	}

	this->ChildSlot
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::AsNumber(InArgs._Index))
				]

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					.Padding(3.0f)
					[
						SNew(SImage)
						.Image(TypeSlateBrush)
						.ColorAndOpacity(triggerTypeColor)
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(triggerTypeText)
					]
				]

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(InArgs._EntryNodeName)
				]

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(InArgs._FlowHistoryEntry.NodeGUID.ToString()))
				]
		];
}
