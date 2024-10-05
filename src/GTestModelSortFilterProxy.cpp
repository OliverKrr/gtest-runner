#include "GTestModelSortFilterProxy.h"
#include "gtestModel.h"


// --------------------------------------------------------------------------------
// 	FUNCTION: QBottomUpSortFilterProxy (public )
// --------------------------------------------------------------------------------
GTestModelSortFilterProxy::GTestModelSortFilterProxy(QObject* parent /*= (QObject*)0*/) : QSortFilterProxyModel(parent)
{
}

// --------------------------------------------------------------------------------
// 	FUNCTION: filterAcceptsRow (public )
// --------------------------------------------------------------------------------
bool GTestModelSortFilterProxy::filterAcceptsRow(const int sourceRow, const QModelIndex& sourceParent) const
{
    return filterAcceptsDescendant(sourceRow) || filterAcceptsAncestor(sourceRow);
}

// --------------------------------------------------------------------------------
// 	FUNCTION: filterAcceptsDescendant (public )
// --------------------------------------------------------------------------------
bool GTestModelSortFilterProxy::filterAcceptsDescendant(const int sourceRow) const
{
    // This function is inclusive of the original row queried in addition to all its descendants.
    const QModelIndex nameRowToTest = sourceModel()->index(sourceRow, GTestModel::Name);
    const FlatDomeItemPtr nameItem = static_cast<GTestModel *>(sourceModel())->itemForIndex(nameRowToTest);

    // do bottom to top filtering
    if (nameItem)
    {
        const QModelIndex resultRowToTest = sourceModel()->index(sourceRow, GTestModel::ResultAndTime);
        const FlatDomeItemPtr resultItem = static_cast<GTestModel *>(sourceModel())->itemForIndex(resultRowToTest);
        if (!checkShowItem(resultItem, resultRowToTest, nameItem->level()))
        {
            return false;
        }

        if (nameItem->level() < 2)
        {
            for (const int childIndex : nameItem->childrenIndex())
            {
                // if the filter accepts the child
                if (filterAcceptsDescendant(childIndex))
                {
                    return true;
                }
            }
        }
    }

    return sourceModel()->data(nameRowToTest).toString().contains(filterRegExp());
}

// --------------------------------------------------------------------------------
// 	FUNCTION: filterAcceptsAncestor (public )
// --------------------------------------------------------------------------------
bool GTestModelSortFilterProxy::filterAcceptsAncestor(const int sourceRow) const
{
    const QModelIndex nameRowToTest = sourceModel()->index(sourceRow, GTestModel::Name);
    const FlatDomeItemPtr nameItem = static_cast<GTestModel *>(sourceModel())->itemForIndex(nameRowToTest);

    // do bottom to top filtering
    if (nameItem)
    {
        const QModelIndex resultRowToTest = sourceModel()->index(sourceRow, GTestModel::ResultAndTime);
        const FlatDomeItemPtr resultItem = static_cast<GTestModel *>(sourceModel())->itemForIndex(resultRowToTest);
        if (!checkShowItem(resultItem, resultRowToTest, nameItem->level()))
        {
            return false;
        }

        // if the filter accepts the parent
        if (nameItem->level() > 0 &&
            filterAcceptsAncestor(nameItem->parentIndex()))
        {
            return true;
        }
    }

    return sourceModel()->data(nameRowToTest).toString().contains(filterRegExp());
}

bool GTestModelSortFilterProxy::checkShowItem(const FlatDomeItemPtr& resultItem, const QModelIndex& resultRowToTest,
                                              const int level) const
{
    // Always show "All Test" even when nothing failed/ignored
    if (level == 0)
    {
        return true;
    }
    // Nothing to filter?
    if (showNotExecuted_ &&
        showPassed_ &&
        showIgnored_)
    {
        return true;
    }

    if (!resultItem)
    {
        return showNotExecuted_;
    }

    const bool isIgnored = sourceModel()->data(resultRowToTest, GTestModel::IgnoredRole).toBool();
    if (isIgnored &&
        !showIgnored_)
    {
        return false;
    }

    if (!isIgnored)
    {
        const bool isPassed = sourceModel()->data(resultRowToTest, GTestModel::FailureRole).toInt() == 0;
        if (isPassed &&
            !showPassed_)
        {
            return false;
        }
    }

    return true;
}

bool GTestModelSortFilterProxy::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    // First check if left or right is the top-level AllTest -> always first
    const QModelIndex leftName = sourceModel()->index(left.row(), GTestModel::Name);
    const FlatDomeItemPtr leftNameItem = static_cast<GTestModel *>(sourceModel())->itemForIndex(leftName);
    if (leftNameItem->level() == 0)
    {
        return sortOrder() == Qt::AscendingOrder;
    }

    const QModelIndex rightName = sourceModel()->index(right.row(), GTestModel::Name);
    const FlatDomeItemPtr rightNameItem = static_cast<GTestModel *>(sourceModel())->itemForIndex(rightName);
    if (rightNameItem->level() == 0)
    {
        return sortOrder() != Qt::AscendingOrder;
    }

    // Check if one is the child of the other -> parent (TestSuite) always before child (Test)
    if (leftNameItem->level() != rightNameItem->level())
    {
        if (leftNameItem->level() == 2 && leftNameItem->parentIndex() == right.row())
        {
            return sortOrder() != Qt::AscendingOrder;
        }
        if (rightNameItem->level() == 2 && rightNameItem->parentIndex() == left.row())
        {
            return sortOrder() == Qt::AscendingOrder;
        }
    }

    const int leftParentIndex = leftNameItem->level() == 2 ? leftNameItem->parentIndex() : left.row();
    const int rightParentIndex = rightNameItem->level() == 2 ? rightNameItem->parentIndex() : right.row();

    int leftRow;
    int rightRow;
    if (leftParentIndex != rightParentIndex)
    {
        // Sort by parent index (TestSuite) when Tests have not the same parent
        leftRow = leftParentIndex;
        rightRow = rightParentIndex;
    }
    else
    {
        // Sort by child index (Test)
        leftRow = left.row();
        rightRow = right.row();
    }

    if (left.column() == GTestModel::TestNumber)
    {
        return leftRow < rightRow;
    }
    if (left.column() == GTestModel::Name)
    {
        return QSortFilterProxyModel::lessThan(sourceModel()->index(leftRow, GTestModel::Name),
                                               sourceModel()->index(rightRow, GTestModel::Name));
    }
    // Sort by ResultTime
    const FlatDomeItemPtr leftResultItem = static_cast<GTestModel *>(sourceModel())->itemForIndex(
        sourceModel()->index(leftRow, left.column()));
    const FlatDomeItemPtr rightResultItem = static_cast<GTestModel *>(sourceModel())->itemForIndex(
        sourceModel()->index(rightRow, right.column()));
    if (leftResultItem && rightResultItem)
    {
        return leftResultItem->node().attributes().namedItem("time").nodeValue().toDouble() <
               rightResultItem->node().attributes().namedItem("time").nodeValue().toDouble();
    }
    if (leftResultItem)
    {
        return true;
    }
    if (rightResultItem)
    {
        return false;
    }
    // Keep same order by default
    return leftRow < rightRow;
}
