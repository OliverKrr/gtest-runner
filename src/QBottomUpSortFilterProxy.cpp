#include "QBottomUpSortFilterProxy.h"



// --------------------------------------------------------------------------------
// 	FUNCTION: QBottomUpSortFilterProxy (public )
// --------------------------------------------------------------------------------
QBottomUpSortFilterProxy::QBottomUpSortFilterProxy(QObject *parent /*= (QObject*)0*/) : QSortFilterProxyModel(parent)
{

}

// --------------------------------------------------------------------------------
// 	FUNCTION: filterAcceptsRow (public )
// --------------------------------------------------------------------------------
bool QBottomUpSortFilterProxy::filterAcceptsRow(const int sourceRow, const QModelIndex &sourceParent) const
{
	return filterAcceptsDescendant(sourceRow, sourceParent) || filterAcceptsAncestor(sourceParent);
}
// --------------------------------------------------------------------------------
// 	FUNCTION: filterAcceptsDescendant (public )
// --------------------------------------------------------------------------------
bool QBottomUpSortFilterProxy::filterAcceptsDescendant(const int sourceRow, const QModelIndex &sourceParent) const
{
	// This function is inclusive of the original row queried in addition to all its descendants.
	const QModelIndex rowToTest = sourceModel()->index(sourceRow, 0, sourceParent);

	/// do bottom to top filtering
	if (sourceModel()->hasChildren(rowToTest))
	{
		for (int i = 0; i < sourceModel()->rowCount(rowToTest); ++i)
		{
			// if the filter accepts the child
			if (filterAcceptsDescendant(i, rowToTest))
				return true;
		}
	}

	return  sourceModel()->data(rowToTest).toString().contains(filterRegExp());
}
// --------------------------------------------------------------------------------
// 	FUNCTION: filterAcceptsAncestor (public )
// --------------------------------------------------------------------------------
bool QBottomUpSortFilterProxy::filterAcceptsAncestor(const QModelIndex &sourceIndex) const
{
	const QModelIndex sourceParentIndex = sourceIndex.parent();

	/// do bottom to top filtering
	if (sourceParentIndex != QModelIndex())
	{
		// if the filter accepts the parent
		if (filterAcceptsAncestor(sourceParentIndex))
			return true;
	}

	return sourceModel()->data(sourceIndex).toString().contains(filterRegExp());
}