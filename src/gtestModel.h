//--------------------------------------------------------------------------------------------------
// 
///	@PROJECT	project
/// @BRIEF		brief
///	@DETAILS	details
//
//--------------------------------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
// and associated documentation files (the "Software"), to deal in the Software without 
// restriction, including without limitation the rights to use, copy, modify, merge, publish, 
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or 
// substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//--------------------------------------------------------------------------------------------------
//
// ATTRIBUTION:
// Parts of this work have been adapted from: 
//
//--------------------------------------------------------------------------------------------------
// 
// Copyright (c) 2016 Nic Holthaus
// Copyright (c) 2020 Oliver Karrenbauer
// 
//--------------------------------------------------------------------------------------------------

#ifndef gtestModel_h__
#define gtestModel_h__

//------------------------------
//	INCLUDE
//------------------------------

#include <QAbstractTableModel>
#include <QDomDocument>
#include <QIcon>
#include <QModelIndex>

#include "flatDomeItem.h"

class GTestModel : public QAbstractTableModel
{
    Q_OBJECT

public:

    enum Roles
    {
        FailureRole = Qt::UserRole,
    };

    enum Sections
    {
        TestNumber = 0,
        Name,
        ResultAndTime
    };


    explicit GTestModel(QDomDocument overviewDocuement, QObject* parent = 0);

    void addTestResult(int index, QDomDocument document);
    void removeTestResult(const int index);
    FlatDomeItemPtr itemForIndex(const QModelIndex& index) const;

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

private:
    struct Model
    {
        QDomDocument document_;
        FlatDomeItemHandler domeItemHandler_;

        explicit Model(const QDomDocument document, const FlatDomeItemHandler domeItemHandler)
            : document_(document), domeItemHandler_(domeItemHandler)
        {
        }
    };
    using ModelPtr = std::shared_ptr<Model>;

    ModelPtr modelForColumn(int column) const;
    Sections sectionForColumn(int column) const;
    QString timestampForColumn(int column) const;
    QString indent(int level) const;
    QString reverseIndent(int level) const;

    ModelPtr initModel(QDomDocument& document, const QDomDocument& overviewDocuement) const;
    void removeComments(QDomNode& node) const;
    bool isFailure(const QDomNode& node) const;


    ModelPtr overviewModel_;
    std::vector<ModelPtr> testResults_;

    QIcon grayIcon;
    QIcon greenIcon;
    QIcon yellowIcon;
    QIcon redIcon;
};

#endif // gtestModel_h__
