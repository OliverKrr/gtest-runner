#include "gtestModel.h"

#include <QIcon>
#include <QtXml>

GTestModel::GTestModel(QObject* parent)
    : QAbstractTableModel(parent), isRealOverview_(false),
      grayIcon(":/images/gray"), greenIcon(":/images/green"),
      yellowIcon(":/images/yellow"), redIcon(":/images/red")
{
}

void GTestModel::updateModel()
{
    const QModelIndex topLeft = index(0, 0);
    const QModelIndex bottomRight = index(rowCount(), columnCount());
    emit dataChanged(topLeft, bottomRight);
}

void GTestModel::updateOverviewDocument(QDomDocument overviewDocument, const bool isRealOverview)
{
    isRealOverview_ = isRealOverview;
    overviewModel_ = initModel(overviewDocument, overviewDocument);
    for (ModelPtr& testResult : testResults_)
    {
        const ModelPtr updatedResult = initModel(testResult->document_, overviewModel_->document_);
        testResult = updatedResult;
    }
}

void GTestModel::addTestResultFront(QDomDocument document)
{
    const ModelPtr model = initModel(document, overviewModel_ ? overviewModel_->document_ : QDomDocument());
    testResults_.emplace_front(model);
}

void GTestModel::removeTestResultBack()
{
    if (testResults_.empty())
    {
        return;
    }
    testResults_.pop_back();
}

int GTestModel::columnCount(const QModelIndex& /*parent*/) const
{
    return static_cast<int>(testResults_.size() + 2);
}

QVariant GTestModel::data(const QModelIndex& index, const int role) const
{
    const FlatDomeItemPtr item = itemForIndex(index);
    if (!item)
    {
        return {};
    }

    const QDomNode& node = item->node();
    const QDomNamedNodeMap attributeMap = node.attributes();

    switch (role)
    {
    case Qt::DisplayRole:
        switch (sectionForColumn(index.column()))
        {
        case TestNumber:
            if (item->row() > 0)
            {
                return item->row();
            }
            return {};
        case Name:
        {
            QString name = attributeMap.namedItem("name").nodeValue();
            if (attributeMap.contains("value_param"))
            {
                name += " (" + attributeMap.namedItem("value_param").nodeValue() + ")";
            }
            return indent(item->level()) + name;
        }
        case ResultAndTime:
        {
            QString result;
            const QString time = QString::number(attributeMap.namedItem("time").nodeValue().toDouble(), 'f', 3);

            const QString failures = attributeMap.namedItem("failures").nodeValue();
            const QString tests = attributeMap.namedItem("tests").nodeValue();
            if (!failures.isEmpty() || !tests.isEmpty())
            {
                result += indent(item->level()) + failures + "/" + tests;
                // fill middle between result and time with spaces
                const QString timestamp = timestampForColumn(index.column());
                // TODO: doesn't work like this
                result += QString(timestamp.size() - result.size() - time.size(), ' ');
            }
            result += time;
            return result;
        }
        default:
            return {};
        }
    case Qt::DecorationRole:
        if (index.column() >= 2)
        {
            if (attributeMap.namedItem("status").nodeValue().contains("notrun") ||
                attributeMap.namedItem("result").nodeValue().contains("skipped"))
                return QColor(Qt::gray);
            if (!attributeMap.namedItem("failures").isNull())
            {
                if (attributeMap.namedItem("failures").nodeValue().toInt() > 0)
                    return QColor(Qt::darkRed);
                return QColor(Qt::darkGreen);
            }
            if (node.childNodes().count())
                return QColor(Qt::darkRed);
            return QColor(Qt::darkGreen);
        }
        break;
    case Qt::TextAlignmentRole:
        switch (sectionForColumn(index.column()))
        {
        case TestNumber:
            return Qt::AlignCenter;
        case Name:
            return Qt::AlignLeft;
        case ResultAndTime:
            return Qt::AlignRight;
        default:
            return Qt::AlignLeft;
        }
    case FailureRole:
    {
        if (attributeMap.namedItem("failures").isNull())
            return node.childNodes().count();
        return attributeMap.namedItem("failures").nodeValue();
    }
    case IgnoredRole:
    {
        if (attributeMap.namedItem("status").nodeValue().contains("notrun") ||
            attributeMap.namedItem("result").nodeValue().contains("skipped"))
            return true;
        return false;
    }
    default:
        return {};
    }

    return {};
}

QVariant GTestModel::headerData(const int section, const Qt::Orientation orientation,
                                const int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (sectionForColumn(section))
        {
        case TestNumber:
            return tr("Test #");
        case Name:
            if (isRealOverview_)
            {
                return tr("Name");
            }
            return tr("Name (*)");
        case ResultAndTime:
        {
            return timestampForColumn(section);
        }
        default:
            return {};
        }
    }
    return {};
}

int GTestModel::rowCount(const QModelIndex& /*parent*/) const
{
    if (!overviewModel_)
    {
        return 0;
    }
    return overviewModel_->domeItemHandler_.numberItems();
}

GTestModel::ModelPtr GTestModel::modelForColumn(const int column) const
{
    if (column == 0 || column == 1)
    {
        return overviewModel_;
    }
    if (column - 2 < static_cast<int>(testResults_.size()))
    {
        return testResults_[column - 2];
    }
    return nullptr;
}

FlatDomeItemPtr GTestModel::itemForIndex(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return nullptr;
    }
    const ModelPtr model = modelForColumn(index.column());
    if (!model)
    {
        return nullptr;
    }
    return model->domeItemHandler_.item(index.row());
}

GTestModel::Sections GTestModel::sectionForColumn(const int column) const
{
    if (column == 0)
    {
        return TestNumber;
    }
    if (column == 1)
    {
        return Name;
    }
    return ResultAndTime;
}

QString GTestModel::timestampForColumn(const int column) const
{
    const ModelPtr model = modelForColumn(column);
    if (model)
    {
        const FlatDomeItemPtr item = model->domeItemHandler_.item(0);
        if (item)
        {
            const QString timestamp = item->node().attributes().namedItem("timestamp").nodeValue();
            const int splitPos = timestamp.indexOf("T");
            return timestamp.left(splitPos) + " " + timestamp.right(timestamp.length() - splitPos - 1);
        }
    }
    return {};
}

QString GTestModel::indent(const int level)
{
    QString result;
    for (int i = 0; i < level; ++i)
    {
        result += "    ";
    }
    return result;
}

QString GTestModel::reverseIndent(const int level)
{
    QString result;
    for (int i = level; i < 2; ++i)
    {
        result += "    ";
    }
    return result;
}


GTestModel::ModelPtr GTestModel::initModel(QDomDocument& document, const QDomDocument& overviewDocument) const
{
    removeComments(document);
    FlatDomeItemHandler domeItemHandler(document, overviewDocument, [](const QDomNode& node)
    {
        return isFailure(node);
    });
    return std::make_shared<Model>(document, domeItemHandler);
}

void GTestModel::removeComments(const QDomNode& node)
{
    if (node.hasChildNodes())
    {
        // call remove comments recursively on all the child nodes
        // iterate backwards because once a node is removed, the
        // remaining nodes shift down in index, meaning iterating
        // forward would skip over some.
        for (int i = node.childNodes().count() - 1; i >= 0; i--)
        {
            QDomNode child = node.childNodes().at(i);
            // Uh-oh, recursion!!
            removeComments(child);
        }
    }
    else
    {
        // if the node has no children, check if it's a comment
        if (node.nodeType() == QDomNode::ProcessingInstructionNode ||
            node.nodeType() == QDomNode::CommentNode)
        {
            // if so, get rid of it.
            node.parentNode().removeChild(node);
        }
    }
}

bool GTestModel::isFailure(const QDomNode& node)
{
    // don't show failure/skipped nodes in the test model. They'll go in a separate model.
    return node.nodeName() != "failure" && node.nodeName() != "skipped";
}
