#include "domitem.h"
#include "GTestFailureModel.h"

#include <QtXml>

GTestFailureModel::GTestFailureModel(const DomItem* root, QObject *parent)
	: QAbstractItemModel(parent), rootItem(nullptr), failIcon(":/images/fail")
{
	if(root)
	{
		rootItem = new DomItem(root->node(), 0);
	}
}

GTestFailureModel::~GTestFailureModel()
{
	delete rootItem;
}

int GTestFailureModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 9;
}

QVariant GTestFailureModel::data(const QModelIndex &index, const int role) const
{
	if (!index.isValid())
		return {};

	const auto item = static_cast<DomItem*>(index.internalPointer());
	QString message = item->node().attributes().namedItem("message").nodeValue();

	static QRegularExpression filerx("(.*)[:]([0-9]+)");
	static QRegularExpression valueofrx("[Vv]alue of: ([^\n]*)|[Dd]eath test: ([^\n]*)|[Tt]o be equal to: ([^\n]*)");
	static QRegularExpression actualrx("[Aa]ctual[:][ ]([^\n]*)|[Rr]esult[:][ ]([^\n]*)|(Failed)|[Tt]o be equal to: .*?[\n]\\s*Which is: ([^\n]*)", QRegularExpression::MultilineOption);
	static QRegularExpression expectedrx("[Ee]xpected[:][ ](.*?)([,][ ]actual|$)|[Ee]rror msg[:]\n(.*)", QRegularExpression::MultilineOption);
	static QRegularExpression whichisrx("[Ww]hich is: ([^\n]*)");
	static QRegularExpression nearrx("The difference between (.*) and (.*) is (.*), which exceeds (.*), where\n(.*) evaluates to(.*),\n(.*) evaluates to(.*), and\n(.*) evaluates to(.*).");
	static QRegularExpression predrx("\n(.*) evaluates to (.*), where\n(.*)");
	static QRegularExpression sehrx("(.*)\n(.*) with (code|description) (.*) thrown in the test body");

	QRegularExpressionMatch fileMatch;
	QRegularExpressionMatch valueofMatch;
	QRegularExpressionMatch actualMatch;
	QRegularExpressionMatch expectedMatch;
	QRegularExpressionMatch whichisMatch;
	QRegularExpressionMatch nearMatch;
	QRegularExpressionMatch predMatch;
	QRegularExpressionMatch sehMatch;

	QString filename;

	switch (role)
	{
	case Qt::DisplayRole:
		switch (index.column())
		{
		case 0:
			fileMatch = filerx.match(message);
			filename = QFileInfo(fileMatch.captured(1)).fileName();
			if (!filename.isEmpty()) return filename;
			sehMatch = sehrx.match(message);
			return sehMatch.captured(1);
		case 1:
			fileMatch = filerx.match(message);
			return fileMatch.captured(2);
		case 2:
			valueofMatch = valueofrx.match(message);
			for (int i = 1; i <= valueofMatch.lastCapturedIndex(); ++i)
				if (!valueofMatch.captured(i).isEmpty()) return valueofMatch.captured(i);
			nearMatch = nearrx.match(message);
			if (!nearMatch.captured(7).isEmpty()) return nearMatch.captured(7);
			predMatch = predrx.match(message);
			if (!predMatch.captured(1).isEmpty())  return predMatch.captured(1);
			sehMatch = sehrx.match(message);
			return sehMatch.captured(2);
		case 3:
			actualMatch = actualrx.match(message);
			for (int i = 1; i <= actualMatch.lastCapturedIndex(); ++i)
				if (!actualMatch.captured(i).isEmpty()) return actualMatch.captured(i);
			nearMatch = nearrx.match(message);
			if (!nearMatch.captured(8).isEmpty()) return nearMatch.captured(8);
			predMatch = predrx.match(message);
			if (!predMatch.captured(2).isEmpty()) return predMatch.captured(2);
			sehMatch = sehrx.match(message);
			return sehMatch.captured(4);
		case 4:
			expectedMatch = expectedrx.match(message);
			for (int i = 1; i <= expectedMatch.lastCapturedIndex(); ++i)
				if (!expectedMatch.captured(i).isEmpty()) return expectedMatch.captured(i);
			nearMatch = nearrx.match(message);
			if (!nearMatch.captured(5).isEmpty()) return nearMatch.captured(5);
			predMatch = predrx.match(message);
			if (!predMatch.captured(1).isEmpty()) return "true";
			return {};
		case 5:
			whichisMatch = whichisrx.match(message);
			for (int i = 1; i <= whichisMatch.lastCapturedIndex(); ++i)
				if (!whichisMatch.captured(i).isEmpty()) return whichisMatch.captured(i);
			nearMatch = nearrx.match(message);
			if (!nearMatch.captured(6).isEmpty()) return nearMatch.captured(6);
			predMatch = predrx.match(message);
			return predMatch.captured(3);
		case 6:
			nearMatch = nearrx.match(message);
			return nearMatch.captured(3);
		case 7:
			nearMatch = nearrx.match(message);
			return nearMatch.captured(10);
                case 8:
                    return message;
		default:
			return {};
		}
	case Qt::DecorationRole:
		if (index.column() == 0) return failIcon;
		return {};
	case Qt::TextAlignmentRole:
		switch (index.column())
		{
		case 1:
			return (int)(Qt::AlignHCenter | Qt::AlignVCenter);
		default:
			return (int)(Qt::AlignLeft | Qt::AlignVCenter);
		}
	case Qt::ToolTipRole:
	case MessageRole:
		return message;
	case PathRole:
		fileMatch = filerx.match(message);
		return QFileInfo(fileMatch.captured(1)).canonicalFilePath();
	case LineRole:
		fileMatch = filerx.match(message);
		return fileMatch.captured(2);
	default:
		return {};
	}
}

Qt::ItemFlags GTestFailureModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return nullptr;

	return QAbstractItemModel::flags(index);
}

QVariant GTestFailureModel::headerData(const int section, const Qt::Orientation orientation,
                                       const int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		switch (section) {
		case 0:
			return tr("File Name");
		case 1:
			return tr("Line");
		case 2:
			return tr("Value of");
		case 3:
			return tr("Actual");
		case 4:
			return tr("Expected");
		case 5:
			return tr("Which is");
		case 6:
			return tr("Difference");
		case 7:
			return tr("Tolerance");
                case 8:
                        return tr("Message");
		default:
			return {};
		}
	}

	return {};
}

QModelIndex GTestFailureModel::index(const int row, const int column, const QModelIndex &parent)
const
{
	if (!hasIndex(row, column, parent))
		return {};

	DomItem *parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<DomItem*>(parent.internalPointer());

	DomItem *childItem = parentItem->child(row);
	if (childItem)
	{
		return createIndex(row, column, childItem);
	}
	return {};
}

QModelIndex GTestFailureModel::parent(const QModelIndex &child) const
{
	if (!child.isValid())
		return {};

	const auto* childItem = static_cast<DomItem*>(child.internalPointer());
	DomItem *parentItem = childItem->parent();

	if (!parentItem || parentItem == rootItem)
		return {};

	return createIndex(parentItem->row(), 0, parentItem);
}

int GTestFailureModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0)
		return 0;

	DomItem *parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<DomItem*>(parent.internalPointer());

	if (parentItem == rootItem && rootItem)
	{
		int count = 0;
		const QDomNodeList failures = parentItem->node().childNodes();
		for (auto i = 0; i < failures.count(); ++i)
		{
			if (failures.at(i).nodeName() == "failure") ++count;
		}
		return count;
	}
	return 0;
}