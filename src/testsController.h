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
// Copyright (c) 2021 Oliver Karrenbauer
//
//--------------------------------------------------------------------------------------------------

#pragma once

#include <QObject>
#include <QDomDocument>
#include <QString>
#include <map>
#include <vector>
#include <memory>
#include <qdir.h>

class GTestModel;


class TestsController final : public QObject
{
    Q_OBJECT

public:
    explicit TestsController(QObject* parent = nullptr);

    // Make non-copyable
    TestsController(const TestsController&) = delete;

    TestsController& operator=(const TestsController&) = delete;

    /** \brief add test and search test result dir for testResults
    */
    void addTest(const QString& path);

    void removeTest(const QString& path);

    /** \brief Loads the latest test result that was set via setCurrentTestResultDir
    *
    * \param numberErrors if latest test results could be loaded, numberErrors will contain the number of failures
    * \return true if there are any previous test results
    */
    bool loadLatestTestResult(const QString& path, int& numberErrors);

    /** \brief return GTestModel for given \a path
    *
    * If given \a path is empty, return an empty GTestModel
    * The ownership of the GTestModel* is still with the TestsController
    */
    std::shared_ptr<GTestModel> gTestModel(const QString& path);

    /** \brief Get path to latest (current) test result for given \a path
    */
    QString latestTestResultFile(const QString& path) const;

    void setAutoRun(const QString& path, bool autoRun);

    bool autoRun(const QString& path) const;

private:
    struct TestResultData
    {
        QString testResultFile_;
        QDomDocument dom_;
    };

    struct TestData
    {
        /* Results should be sorted by time with the oldest first. */
        std::vector<TestResultData> testResults_;
        std::shared_ptr<GTestModel> gtestModel_;
        bool autoRun_;

        TestData();

        // Make non-copyable
        TestData(const TestData&) = delete;

        TestData& operator=(const TestData&) = delete;
    };

    using TestDataPtr = std::shared_ptr<TestData>;


    std::map<QString, TestDataPtr> testsData_;
    std::shared_ptr<GTestModel> emptyGTestModel_;


    static QStringList testResultFiles(const QString& basicPath, QDir::SortFlags sortFlag);

    static bool addTestResultData(const QString& path, const QString& testResultFile, const TestDataPtr& testData);

    static bool loadTestResultXml(const QString& pathToTestXml, QDomDocument& doc);

    static void removeOldTests(const TestDataPtr& testData);
};
