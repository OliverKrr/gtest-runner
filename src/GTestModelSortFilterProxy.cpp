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
    const QModelIndex rowToTest = sourceModel()->index(sourceRow, GTestModel::Name);

    const FlatDomeItemPtr item = static_cast<GTestModel *>(sourceModel())->itemForIndex(rowToTest);

    // do bottom to top filtering
    if (item)
    {
        if (!checkShowItem(rowToTest, item))
        {
            return false;
        }

        if (item->level() < 2)
        {
            for (const int childIndex : item->childrenIndex())
            {
                // if the filter accepts the child
                if (filterAcceptsDescendant(childIndex))
                {
                    return true;
                }
            }
        }
    }

    return sourceModel()->data(rowToTest).toString().contains(filterRegExp());
}

// --------------------------------------------------------------------------------
// 	FUNCTION: filterAcceptsAncestor (public )
// --------------------------------------------------------------------------------
bool GTestModelSortFilterProxy::filterAcceptsAncestor(const int sourceRow) const
{
    const QModelIndex rowToTest = sourceModel()->index(sourceRow, GTestModel::Name);

    const FlatDomeItemPtr item = static_cast<GTestModel *>(sourceModel())->itemForIndex(rowToTest);

    // do bottom to top filtering
    if (item)
    {
        if (!checkShowItem(rowToTest, item))
        {
            return false;
        }

        // if the filter accepts the parent
        if (item->level() > 0 &&
            filterAcceptsAncestor(item->parentIndex()))
        {
            return true;
        }
    }

    return sourceModel()->data(rowToTest).toString().contains(filterRegExp());
}

bool GTestModelSortFilterProxy::checkShowItem(const QModelIndex& rowToTest, const FlatDomeItemPtr& item) const
{
    // Always show "All Test" even when nothing failed/ignored
    if (item->level() == 0)
    {
        return true;
    }
    // Nothing to filter?
    if (showPassed_ &&
        showIgnored_)
    {
        return true;
    }

    const bool isIgnored = sourceModel()->data(rowToTest, GTestModel::IgnoredRole).toBool();
    if (isIgnored &&
        !showIgnored_)
    {
        return false;
    }

    if (!isIgnored)
    {
        const bool isPassed = sourceModel()->data(rowToTest, GTestModel::FailureRole).toInt() == 0;
        if (isPassed &&
            !showPassed_)
        {
            return false;
        }
    }

    return true;
}
