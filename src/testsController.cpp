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
        const bool isOverview = testResultFile.endsWith(GTEST_LIST_NAME);
        addTestResultData(path, testResultFile, testData, isOverview);
    }

    if (!testData->testOverview_.lastModified_.isNull())
    {
        testData->gtestModel_->updateOverviewDocument(testData->testOverview_.dom_, true);
    }
    else if (!testData->testResults_.empty())
    {
        // No overview -> use latest as fallback
        testData->gtestModel_->updateOverviewDocument(testData->testResults_.back().dom_, false);
    }

    // Add TestResults to GtestModel
    for (const auto& testResult : testData->testResults_)
    {
        testData->gtestModel_->addTestResultFront(testResult.dom_);
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

bool TestsController::loadLatestTestResult(const QString& path, int& numberErrors, bool& newTestResult)
{
    numberErrors = 0;
    newTestResult = false;
    const auto iter = testsData_.find(path);
    if (iter == testsData_.end())
    {
        return false;
    }
    const TestDataPtr& testData = iter->second;

    // Check for new executed test
    auto currentResultFiles = testResultFiles(xmlPath(path), QDir::Time);
    for (auto resultIter = currentResultFiles.begin(); resultIter != currentResultFiles.end(); ++resultIter)
    {
        if (resultIter->endsWith(GTEST_LIST_NAME))
        {
            if (addTestResultData(path, *resultIter, testData, true))
            {
                testData->gtestModel_->updateOverviewDocument(testData->testOverview_.dom_, true);
            }
            currentResultFiles.erase(resultIter);
            break;
        }
    }

    if (currentResultFiles.empty())
    {
        // No executed test
        return false;
    }

    if (testData->testResults_.empty() ||
        !testData->testResults_.back().testResultFile_.endsWith(currentResultFiles.front()))
    {
        if (!addTestResultData(path, currentResultFiles.front(), testData, false))
        {
            return false;
        }

        newTestResult = true;
        const auto& latestTestResult = testData->testResults_.back();
        if (testData->testOverview_.lastModified_.isNull())
        {
            // if we don't have overview xml -> use latest
            testData->gtestModel_->updateOverviewDocument(latestTestResult.dom_, false);
        }
        testData->gtestModel_->addTestResultFront(latestTestResult.dom_);
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
                                        const TestDataPtr& testData, const bool isOverview)
{
    const QString pathToTestXml = xmlPath(path) + "/" + testResultFile;

    const QFileInfo fileinfo(pathToTestXml);
    if (!fileinfo.exists())
    {
        return false;
    }

    const auto lastModified = QFileInfo(pathToTestXml).lastModified();
    if (isOverview &&
        !testData->testOverview_.lastModified_.isNull() &&
        testData->testOverview_.lastModified_ >= lastModified)
    {
        // nothing to update
        return false;
    }

    QDomDocument doc(path);
    if (loadTestResultXml(pathToTestXml, doc))
    {
        TestResultData resultData;
        resultData.testResultFile_ = pathToTestXml;
        resultData.lastModified_ = lastModified;
        resultData.dom_ = doc;
        if (isOverview)
        {
            testData->testOverview_ = resultData;
        }
        else
        {
            testData->testResults_.emplace_back(resultData);
            removeOldTests(testData);
        }
        return true;
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

void TestsController::removeOldTests(const TestDataPtr& testData)
{
    // Remove the oldest results
    // TODO: add option to make a result "sticky"
    while (testData->testResults_.size() > MAX_HISTORY_TEST_RESULTS)
    {
        const auto toRemove = testData->testResults_.begin();
        QFile xmlFile(toRemove->testResultFile_);
        if (xmlFile.exists())
        {
            xmlFile.remove();
        }
        testData->gtestModel_->removeTestResultBack();
        testData->testResults_.erase(toRemove);
    }
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

bool TestsController::hasOverview(const QString& path) const
{
    const auto iter = testsData_.find(path);
    if (iter != testsData_.end())
    {
        return !iter->second->testOverview_.lastModified_.isNull();
    }
    return false;
}

TestsController::TestData::TestData()
    : gtestModel_(std::make_shared<GTestModel>()),
      autoRun_(false)
{
}
