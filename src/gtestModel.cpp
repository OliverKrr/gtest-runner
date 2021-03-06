#include "domitem.h"
#include "gtestModel.h"

#include <QIcon>
#include <QtXml>

GTestModel::GTestModel(QDomDocument overviewDocuement, QObject* parent)
    : QAbstractTableModel(parent), overviewModel_(initModel(overviewDocuement, overviewDocuement)),
    grayIcon(":/images/gray"), greenIcon(":/images/green"),
    yellowIcon(":/images/yellow"), redIcon(":/images/red")
{

}

void GTestModel::addTestResult(int index, QDomDocument document)
{
    if (index < 0 || index > testResults_.size())
    {
        return;
    }

    auto pos = testResults_.begin() + index;
    ModelPtr model = initModel(document, overviewModel_->document_);
    testResults_.insert(pos, model);
}

void GTestModel::removeTestResult(const int index)
{
    if (index < 0 || index >= testResults_.size())
    {
        return;
    }
    auto pos = testResults_.begin() + index;
    testResults_.erase(pos);
}


int GTestModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 2 + testResults_.size();
}

QVariant GTestModel::data(const QModelIndex& index, int role) const
{
    FlatDomeItemPtr item = itemForIndex(index);
    if (!item)
    {
        return QVariant();
    }

    const QDomNode& node = item->node();
    QDomNamedNodeMap attributeMap = node.attributes();

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
            return QVariant();
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
            QString failures = attributeMap.namedItem("failures").nodeValue();
            QString tests = attributeMap.namedItem("tests").nodeValue();
            if (!failures.isEmpty() || !tests.isEmpty())
            {
                result += failures + "/" + tests + reverseIndent(item->level());
            }
            result += QString::number(attributeMap.namedItem("time").nodeValue().toDouble(), 'f', 3);
            return result;
        }
        default:
            return QVariant();
        }
        break;
    case Qt::DecorationRole:
        if (index.column() >= 2)
        {
            if (attributeMap.namedItem("status").nodeValue().contains("notrun"))
                return grayIcon;
            if (!attributeMap.namedItem("failures").isNull())
            {
                if (attributeMap.namedItem("failures").nodeValue().toInt() > 0)
                    return redIcon;
                else
                    return greenIcon;
            }
            else
            {
                if (node.childNodes().count())
                    return redIcon;
                else
                    return greenIcon;
            }
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
        break;
    case FailureRole:
    {
        if (attributeMap.namedItem("failures").isNull())
            return node.childNodes().count();
        else
            return attributeMap.namedItem("failures").nodeValue();
    }
    default:
        return QVariant();
    }

    return QVariant();
}

QVariant GTestModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (sectionForColumn(section))
        {
        case TestNumber:
            return tr("Test #");
        case Name:
            return tr("Name");
        case ResultAndTime:
        {
            return timestampForColumn(section);
            break;
        }
        default:
            return QVariant();
        }
    }
    return QVariant();
}

int GTestModel::rowCount(const QModelIndex& parent) const
{
    return overviewModel_->domeItemHandler_.numberItems();
}

GTestModel::ModelPtr GTestModel::modelForColumn(const int column) const
{
    if (column == 0 || column == 1)
    {
        return overviewModel_;
    }
    else if (column - 2 < testResults_.size())
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
    ModelPtr model = modelForColumn(index.column());
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
    else if (column == 1)
    {
        return Name;
    }
    return ResultAndTime;
}

QString GTestModel::timestampForColumn(const int column) const
{
    ModelPtr model = modelForColumn(column);
    if (model)
    {
        FlatDomeItemPtr item = model->domeItemHandler_.item(0);
        if (item)
        {
            QString timestamp = item->node().attributes().namedItem("timestamp").nodeValue();
            int splitPos = timestamp.indexOf("T");
            return timestamp.left(splitPos) + " " + timestamp.right(timestamp.length() - splitPos - 1);
        }
    }
    return QString();
}

QString GTestModel::indent(const int level) const
{
    QString result;
    for (int i = 0; i < level; ++i)
    {
        result += "    ";
    }
    return result;
}

QString GTestModel::reverseIndent(const int level) const
{
    QString result;
    for (int i = level; i < 2; ++i)
    {
        result += "    ";
    }
    return result;
}


GTestModel::ModelPtr GTestModel::initModel(QDomDocument& document, const QDomDocument& overviewDocuement) const
{
    removeComments(document);
    FlatDomeItemHandler domeItemHandler = FlatDomeItemHandler(document, overviewDocuement, std::bind(&GTestModel::isFailure, this, std::placeholders::_1));
    return std::make_shared<Model>(document, domeItemHandler);
}

void GTestModel::removeComments(QDomNode& node) const
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

bool GTestModel::isFailure(const QDomNode& node) const
{
    // don't show failure/skipped nodes in the test model. They'll go in a separate model.
    return node.nodeName() != "failure" && node.nodeName() != "skipped";
}

