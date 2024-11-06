// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Widgets/SCompoundWidget.h"
#include <Widgets/Views/STreeView.h>
#include "FlowTypes.h"

class SHeaderRow;
class FFlowAssetEditor;
class UFlowAsset;

struct FHistoryTreeEntry;
struct FFlowHistoryEntry;

typedef TSharedPtr<FHistoryTreeEntry> FHistoryTreeItemPtr;

/** Widget that represents a row in the history's tree.  Generates widgets for each column on demand. */
class FLOWEDITOR_API SFlowHistoryRow : public SCompoundWidget
{
protected:
	int Index = 0;
	FText EntryNodeName;
	FFlowHistoryEntry FlowHistoryEntry;
	const FSlateBrush* TypeSlateBrush;

public:
	SLATE_BEGIN_ARGS(SFlowHistoryRow) 
		{}
		SLATE_ARGUMENT(uint32, Index)
		SLATE_ARGUMENT(FFlowHistoryEntry, FlowHistoryEntry)
		SLATE_ARGUMENT(FText, EntryNodeName)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};

class FLOWEDITOR_API SFlowHistory : public SCompoundWidget
{
private:
	/** The header row of the history outliner */
	TSharedPtr<SHeaderRow> HeaderRowWidget;

	/** Our tree view */
	TSharedPtr<STreeView<FHistoryTreeItemPtr>> HistoryTreeView;

	/** Root level tree items */
	TArray<FHistoryTreeItemPtr> RootTreeItems;

	TArray<TSharedPtr<FString>> InstancesNames;
	TSharedPtr<STextComboBox> InstanceComboBox;

	TMap<FName, FFlowInstanceHistory> InstancesHistory;

protected:
	TWeakPtr<FFlowAssetEditor> FlowAssetEditor;
	UFlowAsset* FlowAsset;

public:
	SLATE_BEGIN_ARGS(SFlowHistory) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditor);
	virtual ~SFlowHistory() override;

	void ClearHistory();

protected:
	/** Tree view event bindings */

	/** Called by STreeView to generate a table row for the specified item */
	TSharedRef<ITableRow> OnGenerateRow(FHistoryTreeItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);

	// Get the children of an item
	void OnGetChildren(FHistoryTreeItemPtr InItem, TArray<FHistoryTreeItemPtr>& OutChildren);

	/** Called by STreeView when the user double-clicks on an item in the tree */
	void OnHistoryTreeDoubleClick(FHistoryTreeItemPtr TreeItem);

	void InstancesSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	/** */
	void HistoryEntryAdded(const FGuid & FlowAssetGuid, const FFlowInstanceHistory & InstanceHistory);

private:

	void GatherInstanceNames();
	FText GetNodeDisplayNameFromGUID(const FGuid& guid) const;
};
