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

#include "testsController.h"
#include "gtestModel.h"
#include "utilities.h"
#include <QDomDocument>
#include <QDir>
#include <QMessageBox>


TestsController::TestsController(QObject* parent)
    : QObject(parent), emptyGTestModel_(std::make_shared<GTestModel>())
{
    // nothing
}

void TestsController::addTest(const QString& path)
{
    if (testsData_.find(path) != testsData_.end())
    {
        return;
    }

    auto testData = std::make_shared<TestData>();

    for (const auto& testResultFile : testResultFiles(xmlPath(path), QDir::Time | QDir::Reversed))
    {
        addTestResultData(path, testResultFile, testData);
    }

    // TODO: OVERVIEW: add button to update overview
    // TODO: OVERVIEW: check if useful to always auto-run? -> make global option
    //  --gtest_list_tests

    // TODO: OVERVIEW: No overview -> take last result
    if (!testData->testResults_.empty())
    {
        testData->gtestModel_->updateOverviewDocument(testData->testResults_.back().dom_);
    }

    // Add TestResults to GtestModel
    for (auto& testResult : testData->testResults_)
    {
        testResult.indexInGtestModel_ = testData->gtestModel_->addTestResultFront(testResult.dom_);
    }

    testsData_.emplace(path, std::move(testData));
}

void TestsController::removeTest(const QString& path)
{
    const auto iter = testsData_.find(path);
    if (iter != testsData_.end())
    {
        testsData_.erase(iter);
    }
}

bool TestsController::loadLatestTestResult(const QString& path, int& numberErrors)
{
    numberErrors = 0;
    const auto iter = testsData_.find(path);
    if (iter == testsData_.end())
    {
        return false;
    }
    const TestDataPtr& testData = iter->second;

    // Check for new executed test
    const auto currentResultFiles = testResultFiles(xmlPath(path), QDir::Time);
    if (currentResultFiles.empty())
    {
        // No executed test
        return false;
    }

    if (testData->testResults_.empty() ||
        testData->testResults_.back().testResultFile_ != currentResultFiles.front())
    {
        if (!addTestResultData(path, currentResultFiles.front(), testData))
        {
            return false;
        }

        auto& latestTestResult = testData->testResults_.back();
        // TODO: OVERVIEW: if we don't have overview xml -> need to update model here
        testData->gtestModel_->updateOverviewDocument(latestTestResult.dom_);
        latestTestResult.indexInGtestModel_ = testData->gtestModel_->addTestResultFront(latestTestResult.dom_);

        testData->gtestModel_->updateModel();
    }

    numberErrors = testData->testResults_.back().dom_.elementsByTagName("testsuites").item(0).attributes().
            namedItem("failures").nodeValue().toInt();
    return true;
}

std::shared_ptr<GTestModel> TestsController::gTestModel(const QString& path)
{
    if (path.isEmpty())
    {
        return emptyGTestModel_;
    }

    const auto iter = testsData_.find(path);
    if (iter == testsData_.end())
    {
        return emptyGTestModel_;
    }

    return iter->second->gtestModel_;
}

QString TestsController::latestTestResultFile(const QString& path) const
{
    const auto iter = testsData_.find(path);
    if (iter != testsData_.end() && !iter->second->testResults_.empty())
    {
        return iter->second->testResults_.back().testResultFile_;
    }
    return {};
}

QStringList TestsController::testResultFiles(const QString& basicPath, QDir::SortFlags sortFlag)
{
    return QDir(basicPath).entryList(QDir::Files, sortFlag);
}

bool TestsController::addTestResultData(const QString& path, const QString& testResultFile,
                                        const TestDataPtr& testData)
{
    const QString pathToTestXml = xmlPath(path) + "/" + testResultFile;

    const QFileInfo fileinfo(pathToTestXml);
    if (!fileinfo.exists())
    {
        return false;
    }

    QDomDocument doc(path);
    if (loadTestResultXml(pathToTestXml, doc))
    {
        TestResultData resultData;
        resultData.dom_ = doc;
        testData->testResults_.emplace_back(resultData);
        return true;

        // TODO: need to define a limit and delete old results
        // TODO: add option to make a result "sticky"
    }
    return false;
}

bool TestsController::loadTestResultXml(const QString& pathToTestXml, QDomDocument& doc)
{
    const QFileInfo xmlInfo(pathToTestXml);
    if (!xmlInfo.exists())
    {
        return false;
    }

    QFile file(xmlInfo.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly))
    {
        // TODO: emit and show in mainWindow (set right parent)
        QMessageBox::warning(nullptr, "Error", "Could not open file located at: " + xmlInfo.absoluteFilePath());
        return false;
    }
    if (!doc.setContent(&file))
    {
        file.close();
        return false;
    }
    file.close();
    return true;
}

void TestsController::setAutoRun(const QString& path, bool autoRun)
{
    const auto iter = testsData_.find(path);
    if (iter != testsData_.end())
    {
        iter->second->autoRun_ = autoRun;
    }
}

bool TestsController::autoRun(const QString& path) const
{
    const auto iter = testsData_.find(path);
    if (iter != testsData_.end())
    {
        return iter->second->autoRun_;
    }
    return false;
}


TestsController::TestResultData::TestResultData()
    : indexInGtestModel_(-1)
{
}

TestsController::TestData::TestData()
    : gtestModel_(std::make_shared<GTestModel>()),
      autoRun_(false)
{
}
