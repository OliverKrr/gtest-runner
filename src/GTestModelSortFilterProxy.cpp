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
