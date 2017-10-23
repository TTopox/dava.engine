#include "Classes/SceneTree/Private/SceneTreeView.h"
#include "Classes/SceneTree/Private/SceneTreeItemDelegateV2.h"

#include "Classes/Application/RESettings.h"
#include "Classes/SceneManager/SceneData.h"

#include <TArc/Utils/ScopedValueGuard.h>
#include <TArc/Core/ContextAccessor.h>

#include <Debug/DVAssert.h>
#include <Logger/Logger.h>

#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>

SceneTreeView::SceneTreeView(const Params& params, DAVA::TArc::ContextAccessor* accessor, DAVA::Reflection model, QWidget* parent)
    : ControlProxyImpl<QTreeView>(params, DAVA::TArc::ControlDescriptor(params.fields), accessor, model, parent)
    , defaultSelectionModel(new QItemSelectionModel(nullptr, this))
{
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setExpandsOnDoubleClick(false);
    setUniformRowHeights(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDragEnabled(true);
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);

    setItemDelegate(new SceneTreeItemDelegateV2(this));

    connections.AddConnection(this, &QTreeView::expanded, DAVA::MakeFunction(this, &SceneTreeView::OnItemExpanded));
    connections.AddConnection(this, &QTreeView::collapsed, DAVA::MakeFunction(this, &SceneTreeView::OnItemCollapsed));
    connections.AddConnection(this, &QTreeView::doubleClicked, DAVA::MakeFunction(this, &SceneTreeView::OnDoubleClicked));
}

void SceneTreeView::AddAction(QAction* action)
{
    addAction(action);
}

void SceneTreeView::UpdateControl(const DAVA::TArc::ControlDescriptor& descriptor)
{
    if (descriptor.IsChanged(Fields::DataModel) == true)
    {
        setModel(GetFieldValue<QAbstractItemModel*>(Fields::DataModel, nullptr));
    }

    if (descriptor.IsChanged(Fields::ExpandedIndexList) == true)
    {
        DAVA::TArc::ScopedValueGuard<bool> guard(inExpandingSync, true);
        expandedIndexList = GetFieldValue(Fields::ExpandedIndexList, DAVA::Set<QPersistentModelIndex>());
        collapseAll();
        for (const QPersistentModelIndex& index : expandedIndexList)
        {
            if (index.isValid() == true)
            {
                DVASSERT(static_cast<QAbstractItemView*>(this)->model() == index.model());
                expand(index);
            }
        }
    }

    if (descriptor.IsChanged(Fields::SelectionModel) == true)
    {
        QItemSelectionModel* selectionModel = GetFieldValue<QItemSelectionModel*>(Fields::SelectionModel, nullptr);
        if (selectionModel != nullptr)
        {
            setSelectionModel(selectionModel);
        }
    }
}

void SceneTreeView::OnItemExpanded(const QModelIndex& index)
{
    SCOPED_VALUE_GUARD(bool, inExpandingSync, true, void());
    expandedIndexList.insert(QPersistentModelIndex(index));
    wrapper.SetFieldValue(GetFieldName(Fields::ExpandedIndexList), expandedIndexList);
}

void SceneTreeView::OnItemCollapsed(const QModelIndex& index)
{
    SCOPED_VALUE_GUARD(bool, inExpandingSync, true, void());
    expandedIndexList.erase(QPersistentModelIndex(index));
    wrapper.SetFieldValue(GetFieldName(Fields::ExpandedIndexList), expandedIndexList);
}

void SceneTreeView::OnDoubleClicked(const QModelIndex& index)
{
    DAVA::FastName doubleClickedName = GetFieldName(Fields::DoubleClicked);
    if (doubleClickedName.IsValid() == true)
    {
        DAVA::AnyFn fn = model.GetMethod(doubleClickedName.c_str());
        DVASSERT(fn.IsValid() == true);
        fn.Invoke(index);
    }
}

void SceneTreeView::contextMenuEvent(QContextMenuEvent* e)
{
    DAVA::FastName contextMenuName = GetFieldName(Fields::ContextMenuRequested);
    if (contextMenuName.IsValid() == true)
    {
        DAVA::AnyFn fn = model.GetMethod(contextMenuName.c_str());
        DVASSERT(fn.IsValid() == true);

        QModelIndex itemIndex = indexAt(e->pos());
        if (itemIndex.isValid())
        {
            fn.Invoke(itemIndex, e->globalPos());
        }
    }
}

void SceneTreeView::dragEnterEvent(QDragEnterEvent* e)
{
    QTreeView::dragEnterEvent(e);

    if (e->source() == this && e->isAccepted() == false)
    {
        e->setDropAction(Qt::IgnoreAction);
        e->accept();
    }
}

void SceneTreeView::dragLeaveEvent(QDragLeaveEvent* e)
{
    QTreeView::dragLeaveEvent(e);
}

void SceneTreeView::dragMoveEvent(QDragMoveEvent* e)
{
    GlobalSceneSettings* settings = controlParams.accessor->GetGlobalContext()->GetData<GlobalSceneSettings>();
    if (settings->dragAndDropWithShift == true && ((e->keyboardModifiers() & Qt::SHIFT) != Qt::SHIFT))
    {
        e->setDropAction(Qt::IgnoreAction);
        e->accept();
        return;
    }

    QTreeView::dragMoveEvent(e);
}

void SceneTreeView::dropEvent(QDropEvent* e)
{
    QTreeView::dropEvent(e);

    if (e->isAccepted())
    {
        QPersistentModelIndex index = indexAt(e->pos());

        switch (dropIndicatorPosition())
        {
        case QAbstractItemView::AboveItem:
        case QAbstractItemView::BelowItem:
            index = index.parent();
            break;
        case QAbstractItemView::OnItem:
        case QAbstractItemView::OnViewport:
            break;
        }

        DAVA::FastName dropExecuted = GetFieldName(Fields::DropExecuted);
        if (dropExecuted.IsValid() == true)
        {
            DAVA::AnyFn fn = model.GetMethod(dropExecuted.c_str());
            DVASSERT(fn.IsValid() == true);
            fn.Invoke();
        }

        executor.DelayedExecute([this, index]() {
            expand(index);
        });
    }

    e->setDropAction(Qt::IgnoreAction);
    e->accept();
}
