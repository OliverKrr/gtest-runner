#include "qexecutablemodel.h"

#include <QByteArray>
#include <QDebug>
#include <QIcon>
#include <QFileInfo>
#include <QMimeData>

//--------------------------------------------------------------------------------------------------
//	CLASS: QExecutableModelPrivate
//--------------------------------------------------------------------------------------------------
/// @brief		Private data of QExecutable Model
/// @details	
//--------------------------------------------------------------------------------------------------
class QExecutableModelPrivate
{
public:
    QIcon grayIcon;
    QIcon greenIcon;
    QIcon yellowIcon;
    QIcon redIcon;

    explicit QExecutableModelPrivate() : grayIcon(QIcon(":images/gray")),
                                         greenIcon(QIcon(":images/green")),
                                         yellowIcon(QIcon(":images/yellow")),
                                         redIcon(QIcon(":images/red"))
    {
    };

    QHash<QString, QModelIndex> indexCache; // used to speed up finding indices.
}; // CLASS: QExecutableModelPrivate


//--------------------------------------------------------------------------------------------------
//	FUNCTION: QExecutableModel
//--------------------------------------------------------------------------------------------------
QExecutableModel::QExecutableModel(QObject* parent /*= nullptr*/) : QTreeModel<ExecutableData>(parent),
                                                                    d_ptr(new QExecutableModelPrivate)
{
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: columnCount
//--------------------------------------------------------------------------------------------------
Q_INVOKABLE int QExecutableModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
    return 3;
}

// --------------------------------------------------------------------------------------------------
// 	FUNCTION: data
// --------------------------------------------------------------------------------------------------
Q_INVOKABLE QVariant QExecutableModel::data(const QModelIndex& index, const int role /*= Qt::DisplayRole*/) const
{
    if (!index.isValid())
        return {};

    Q_D(const QExecutableModel);

    const auto itr = indexToIterator(index);
    if (itr == Tree<ExecutableData>::end())
        return {};

    switch (role)
    {
    case Qt::DecorationRole:
        if (index.column() == NameColumn)
        {
            switch (data(index, StateRole).toInt())
            {
            case ExecutableData::NOT_RUNNING:
                return d->grayIcon;
            case ExecutableData::RUNNING:
                return d->yellowIcon;
            case ExecutableData::PASSED:
                return d->greenIcon;
            case ExecutableData::FAILED:
                return d->redIcon;
            default:
                return QIcon();
            }
        }
    case Qt::DisplayRole:
        switch (index.column())
        {
        case NameColumn:
            return data(index, NameRole);
        default:
            return {};
        }
    case Qt::CheckStateRole:
        if (index.column() == NameColumn)
        {
            if (itr->autorun)
                return Qt::Checked;
            return Qt::Unchecked;
        }
        return {};
    case AutorunRole:
        return itr->autorun;
    case PathRole:
        return itr->path;
    case NameRole:
        return itr->name;
    case TestDriverRole:
        return itr->testDriver;
    case StateRole:
        return itr->state;
    case LastModifiedRole:
        return itr->lastModified;
    case ProgressRole:
        return itr->progress;
    case FilterRole:
        return itr->filter;
    case RepeatTestsRole:
        return itr->repeat;
    case RunDisabledTestsRole:
        return itr->runDisabled;
    case BreakOnFailureRole:
        return itr->breakOnFailure;
    case FailFastRole:
        return itr->failFast;
    case ShuffleRole:
        return itr->shuffle;
    case RandomSeedRole:
        return itr->randomSeed;
    case ArgsRole:
        return itr->otherArgs;
    default:
        return {};
    }
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: setData
//--------------------------------------------------------------------------------------------------
Q_INVOKABLE bool QExecutableModel::setData(const QModelIndex& index, const QVariant& value,
                                           const int role /*= Qt::EditRole*/)
{
    const auto itr = indexToIterator(index);
    switch (role)
    {
    case Qt::EditRole || Qt::DisplayRole || QExecutableModel::PathRole:
        itr->path = value.toString();
        break;
    case NameRole:
        itr->name = value.toString();
        break;
    case TestDriverRole:
        itr->testDriver = value.toString();
        break;
    case Qt::CheckStateRole:
        itr->autorun = value.toBool();
    case AutorunRole:
        itr->autorun = value.toBool();
        break;
    case StateRole:
        itr->state = static_cast<ExecutableData::States>(value.toInt());
        break;
    case LastModifiedRole:
        itr->lastModified = value.toDateTime();
        break;
    case ProgressRole:
        itr->progress = value.toDouble();
        break;
    case FilterRole:
        itr->filter = value.toString();
        break;
    case RepeatTestsRole:
        itr->repeat = value.toInt();
        break;
    case RunDisabledTestsRole:
        itr->runDisabled = static_cast<Qt::CheckState>(value.toInt());
        break;
    case BreakOnFailureRole:
        itr->breakOnFailure = static_cast<Qt::CheckState>(value.toInt());
        break;
    case FailFastRole:
        itr->failFast = static_cast<Qt::CheckState>(value.toInt());
        break;
    case ShuffleRole:
        itr->shuffle = static_cast<Qt::CheckState>(value.toInt());
        break;
    case RandomSeedRole:
        itr->randomSeed = value.toInt();
        break;
    case ArgsRole:
        itr->otherArgs = value.toString();
        break;
    default:
        return false;
    }

    // signal that the whole row has changed
    const QModelIndex right = index.sibling(index.row(), columnCount());
    emit dataChanged(index, right);
    return true;
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: index
//--------------------------------------------------------------------------------------------------
QModelIndex QExecutableModel::index(const QString& path) const
{
    // 	if(d_ptr->indexCache.contains(path))
    // 	{
    // 		QModelIndex index = d_ptr->indexCache[path];
    //
    // 		// check the cache to see if we know the index
    // 		if (index.isValid())
    // 		{
    // 			// if it hasn't changed since last time
    // 			if (index.data(QExecutableModel::PathRole).toString() == path)
    // 			{
    // 				return index;
    // 			}
    // 		}
    // 	}

    const auto itr = std::find(begin(), end(), path);
    const QModelIndex index = iteratorToIndex(itr);

    // cache for later use
    d_ptr->indexCache[path] = index;

    return index;
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: index
//--------------------------------------------------------------------------------------------------
QModelIndex QExecutableModel::index(const int row, const int column,
                                    const QModelIndex& parent /*= QModelIndex()*/) const
{
    return QTreeModel::index(row, column, parent);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: mimeTypes
//--------------------------------------------------------------------------------------------------
QStringList QExecutableModel::mimeTypes() const
{
    QStringList types;
    types << "application/x.text.executableData.list";
    return types;
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: mimeData
//--------------------------------------------------------------------------------------------------
QMimeData* QExecutableModel::mimeData(const QModelIndexList& indexes) const
{
    auto* mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    foreach(const QModelIndex &index, indexes)
    {
        if (index.isValid() && index.column() == 0)
        {
            QVariant PathRoleText = data(index, PathRole);
            QVariant NameRoleText = data(index, NameRole);
            QVariant TestDriverText = data(index, TestDriverRole);
            QVariant StateRoleText = data(index, StateRole);
            QVariant LastModifiedRoleText = data(index, LastModifiedRole);
            QVariant ProgressRoleText = data(index, ProgressRole);
            QVariant FilterRoleText = data(index, FilterRole);
            QVariant RepeatTestsRoleText = data(index, RepeatTestsRole);
            QVariant RunDisabledTestsRoleText = data(index, RunDisabledTestsRole);
            QVariant breakOnFailureText = data(index, BreakOnFailureRole);
            QVariant failFastText = data(index, FailFastRole);
            QVariant ShuffleRoleText = data(index, ShuffleRole);
            QVariant RandomSeedRoleText = data(index, RandomSeedRole);
            QVariant ArgsRoleText = data(index, ArgsRole);
            QVariant AutorunRoleText = data(index, AutorunRole);
            stream << PathRoleText << NameRoleText << TestDriverText << StateRoleText << LastModifiedRoleText <<
                    ProgressRoleText << FilterRoleText << RepeatTestsRoleText <<
                    RunDisabledTestsRoleText << breakOnFailureText << failFastText << ShuffleRoleText << RandomSeedRoleText << ArgsRoleText <<
                    AutorunRoleText;
        }
    }

    mimeData->setData("application/x.text.executableData.list", encodedData);
    return mimeData;
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: dropMimeData
//--------------------------------------------------------------------------------------------------
bool QExecutableModel::dropMimeData(const QMimeData* data, const Qt::DropAction action, int row, int column,
                                    const QModelIndex& parent)
{
    if (action == Qt::IgnoreAction)
        return true;

    if (!(supportedDragActions() & action))
        return false;

    if (data->hasFormat("application/x.text.executableData.list"))
    {
        QByteArray encodedData = data->data("application/x.text.executableData.list");
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        QList<QMap<int, QVariant>> newItems;
        int count = 0;

        while (!stream.atEnd())
        {
            QMap<int, QVariant> itemData;
            stream >> itemData[PathRole];
            stream >> itemData[NameRole];
            stream >> itemData[TestDriverRole];
            stream >> itemData[StateRole];
            stream >> itemData[LastModifiedRole];
            stream >> itemData[ProgressRole]; // doesn't seem to be there
            stream >> itemData[FilterRole];
            stream >> itemData[RepeatTestsRole];
            stream >> itemData[RunDisabledTestsRole];
            stream >> itemData[BreakOnFailureRole];
            stream >> itemData[FailFastRole];
            stream >> itemData[ShuffleRole];
            stream >> itemData[RandomSeedRole];
            stream >> itemData[ArgsRole];
            stream >> itemData[AutorunRole];
            newItems.push_back(itemData);
            ++count;
        }

        if (row < 0)
            row = rowCount(parent);

        insertRows(row, count, parent);

        for (int i = 0; i < count; ++i)
        {
            setItemData(index(row + i, 0, parent), newItems[i]);
        }

        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: itemData
//--------------------------------------------------------------------------------------------------
QMap<int, QVariant> QExecutableModel::itemData(const QModelIndex& index) const
{
    const auto itr = indexToIterator(index);
    QMap<int, QVariant> ret;
    ret[PathRole] = itr->path;
    ret[NameRole] = itr->name;
    ret[TestDriverRole] = itr->testDriver;
    ret[StateRole] = itr->state;
    ret[LastModifiedRole] = itr->lastModified;
    ret[ProgressRole] = itr->progress;
    ret[FilterRole] = itr->filter;
    ret[RepeatTestsRole] = itr->repeat;
    ret[RunDisabledTestsRole] = itr->runDisabled;
    ret[BreakOnFailureRole] = itr->breakOnFailure;
    ret[FailFastRole] = itr->failFast;
    ret[ShuffleRole] = itr->shuffle;
    ret[RandomSeedRole] = itr->randomSeed;
    ret[ArgsRole] = itr->otherArgs;
    ret[AutorunRole] = itr->autorun;
    return ret;
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: setItemData
//--------------------------------------------------------------------------------------------------
bool QExecutableModel::setItemData(const QModelIndex& index, const QMap<int, QVariant>& roles)
{
    Q_D(QExecutableModel);
    // invalidate the cache on a remove
    d->indexCache.clear();

    const auto itr = indexToIterator(index);
    itr->path = roles[PathRole].toString();
    itr->name = roles[NameRole].toString();
    itr->testDriver = roles[TestDriverRole].toString();
    itr->state = static_cast<ExecutableData::States>(roles[StateRole].toInt());
    itr->lastModified = roles[LastModifiedRole].toDateTime();
    itr->progress = roles[ProgressRole].toInt();
    itr->filter = roles[FilterRole].toString();
    itr->repeat = roles[RepeatTestsRole].toInt();
    itr->runDisabled = static_cast<Qt::CheckState>(roles[RunDisabledTestsRole].toInt());
    itr->breakOnFailure = static_cast<Qt::CheckState>(roles[BreakOnFailureRole].toInt());
    itr->failFast = static_cast<Qt::CheckState>(roles[FailFastRole].toInt());
    itr->shuffle = static_cast<Qt::CheckState>(roles[ShuffleRole].toInt());
    itr->randomSeed = roles[RandomSeedRole].toInt();
    itr->otherArgs = roles[ArgsRole].toString();
    itr->autorun = roles[AutorunRole].toBool();
    return true;
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: moveRows
//--------------------------------------------------------------------------------------------------
bool QExecutableModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                                const QModelIndex& destinationParent, int destinationChild)
{
    throw std::logic_error("The method or operation is not implemented.");
}

// --------------------------------------------------------------------------------------------------
// 	FUNCTION: supportedDragActions
// --------------------------------------------------------------------------------------------------
Qt::DropActions QExecutableModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

// --------------------------------------------------------------------------------------------------
// 	FUNCTION: supportedDropActions
// --------------------------------------------------------------------------------------------------
Q_INVOKABLE Qt::DropActions QExecutableModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: flags
//--------------------------------------------------------------------------------------------------
Qt::ItemFlags QExecutableModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f;

    if (index.isValid())
        f = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
    else
        f = Qt::ItemIsDropEnabled;

    switch (index.column())
    {
    case NameColumn:
        f |= Qt::ItemIsUserCheckable;
        break;
    default:
        break;
    }

    return f;
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: insertRows
//--------------------------------------------------------------------------------------------------
bool QExecutableModel::insertRows(const int row, const int count, const QModelIndex& parent /*= QModelIndex()*/)
{
    Q_D(QExecutableModel);
    // invalidate the cache on a remove
    d->indexCache.clear();
    return QTreeModel::insertRows(row, count, parent);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: removeRows
//--------------------------------------------------------------------------------------------------
bool QExecutableModel::removeRows(const int row, const int count, const QModelIndex& parent /*= QModelIndex()*/)
{
    Q_D(QExecutableModel);
    // invalidate the cache on a remove
    d->indexCache.clear();
    return QTreeModel::removeRows(row, count, parent);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: removeRow
//--------------------------------------------------------------------------------------------------
QModelIndex QExecutableModel::removeRow(const int row, const QModelIndex& parent /*= QModelIndex()*/)
{
    Q_D(QExecutableModel);
    // invalidate the cache on a remove
    d->indexCache.clear();
    return QTreeModel::removeRow(row, parent);
}
