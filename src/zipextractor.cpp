#include "zipextractor.h"
#include <QDir>
#include <QFileInfo>

ZipExtractor* ZipExtractor::s_instance = nullptr;

ZipExtractor::ZipExtractor(QObject *parent)
    : QObject(parent)
    , m_extractTimer(new QTimer(this))
    , m_etaTimer(new QTimer(this))
{
    m_extractTimer->setSingleShot(true);
    m_extractTimer->setInterval(1);
    connect(m_extractTimer, &QTimer::timeout, this, &ZipExtractor::processNextFile);

    m_etaTimer->setInterval(1000);
    connect(m_etaTimer, &QTimer::timeout, this, &ZipExtractor::updateETA);
}

ZipExtractor* ZipExtractor::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    if (!s_instance) {
        s_instance = new ZipExtractor();
    }
    return s_instance;
}

ZipExtractor* ZipExtractor::instance()
{
    if (!s_instance) {
        s_instance = new ZipExtractor();
    }
    return s_instance;
}

void ZipExtractor::startExtraction(const QString &zipPath, const QString &destPath)
{
    if (m_isExtracting) return;

    resetProgress();
    m_isExtracting = true;
    emit isExtractingChanged();

    // Store the current zip path
    m_currentZipPath = zipPath;

    // Set destination path
    if (destPath.isEmpty()) {
        QFileInfo zipInfo(zipPath);
        m_destinationPath = zipInfo.absolutePath() + "/" + zipInfo.baseName();
    } else {
        m_destinationPath = destPath;
    }

    // Create destination directory
    QDir().mkpath(m_destinationPath);

    // Open ZIP file
    m_zipReader = new QZipReader(zipPath, QIODevice::ReadOnly);
    if (!m_zipReader->isReadable()) {
        delete m_zipReader;
        m_zipReader = nullptr;
        m_isExtracting = false;
        emit isExtractingChanged();
        emit extractionFinished(false, "Cannot read ZIP file");
        return;
    }

    // Get file list
    m_fileList = m_zipReader->fileInfoList();
    m_totalFiles = m_fileList.size();
    emit totalFilesChanged();

    if (m_totalFiles == 0) {
        delete m_zipReader;
        m_zipReader = nullptr;
        m_isExtracting = false;
        emit isExtractingChanged();
        emit extractionFinished(true, "ZIP file is empty");
        return;
    }

    // Start timers
    m_elapsedTimer.start();
    m_etaTimer->start();

    // Start extraction
    m_extractTimer->start();
}

void ZipExtractor::processNextFile()
{
    if (m_currentFile >= m_fileList.size()) {
        // All files extracted, now handle nested zips
        if (!m_nestedZipsToExtract.isEmpty()) {
            QString nextZip = m_nestedZipsToExtract.takeFirst();
            extractNestedZip(nextZip);
            return;
        }

        // Extraction complete
        m_isExtracting = false;
        m_etaTimer->stop();
        emit isExtractingChanged();
        emit extractionFinished(true, "Extraction completed successfully");
        return;
    }

    const QZipReader::FileInfo &fileInfo = m_fileList[m_currentFile];
    m_currentFileName = fileInfo.filePath;
    emit currentFileNameChanged();

    // Extract current file
    QString fullPath = m_destinationPath + "/" + fileInfo.filePath;

    if (fileInfo.isDir) {
        QDir().mkpath(fullPath);
    } else {
        // Ensure parent directory exists
        QDir().mkpath(QFileInfo(fullPath).absolutePath());

        // Extract file data
        QByteArray data = m_zipReader->fileData(fileInfo.filePath);
        QFile outFile(fullPath);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(data);
        }

        // Check if this is a zip file that should be extracted recursively
        if (fileInfo.filePath.endsWith(".zip", Qt::CaseInsensitive)) {
            if (shouldExtractRecursively(fullPath)) {
                // Handle naming conflict
                QFileInfo extractedZipInfo(fullPath);
                QFileInfo originalZipInfo(m_currentZipPath);  // Use stored path instead of m_zipReader->fileName()

                if (extractedZipInfo.baseName() == originalZipInfo.baseName() && m_fileList.size() > 1) {
                    // Rename the nested zip to avoid conflict
                    QString newName = getUniqueFileName(
                        extractedZipInfo.absolutePath(),
                        extractedZipInfo.baseName(),
                        extractedZipInfo.suffix()
                        );
                    QString newPath = extractedZipInfo.absolutePath() + "/" + newName;
                    QFile::rename(fullPath, newPath);
                    m_nestedZipsToExtract.append(newPath);
                } else {
                    m_nestedZipsToExtract.append(fullPath);
                }
            }
        }
    }

    // Update progress
    m_currentFile++;
    emit currentFileChanged();

    m_progress = (double)m_currentFile / m_totalFiles * 100.0;
    emit progressChanged();

    // Schedule next file
    m_extractTimer->start();
}

void ZipExtractor::extractNestedZip(const QString &zipPath)
{
    QFileInfo zipInfo(zipPath);
    QString nestedDestPath = zipInfo.absolutePath() + "/" + zipInfo.baseName();

    // Update current file name to show nested extraction
    m_currentFileName = "Extracting nested: " + zipInfo.fileName();
    emit currentFileNameChanged();

    // Create nested destination
    QDir().mkpath(nestedDestPath);

    // Open nested ZIP
    QZipReader nestedReader(zipPath, QIODevice::ReadOnly);
    if (!nestedReader.isReadable()) {
        // Continue with remaining nested zips
        if (!m_nestedZipsToExtract.isEmpty()) {
            QString nextZip = m_nestedZipsToExtract.takeFirst();
            extractNestedZip(nextZip);
        } else {
            m_isExtracting = false;
            m_etaTimer->stop();
            emit isExtractingChanged();
            emit extractionFinished(true, "Extraction completed with some nested ZIP errors");
        }
        return;
    }

    // Extract all files from nested ZIP
    QList<QZipReader::FileInfo> nestedFileList = nestedReader.fileInfoList();

    for (const auto &fileInfo : nestedFileList) {
        QString fullPath = nestedDestPath + "/" + fileInfo.filePath;

        if (fileInfo.isDir) {
            QDir().mkpath(fullPath);
        } else {
            QDir().mkpath(QFileInfo(fullPath).absolutePath());
            QByteArray data = nestedReader.fileData(fileInfo.filePath);
            QFile outFile(fullPath);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(data);
            }

            // Check for more nested zips
            if (fileInfo.filePath.endsWith(".zip", Qt::CaseInsensitive)) {
                if (shouldExtractRecursively(fullPath)) {
                    m_nestedZipsToExtract.append(fullPath);
                }
            }
        }
    }

    // Remove the extracted nested zip file
    QFile::remove(zipPath);

    // Continue with next nested zip or finish
    if (!m_nestedZipsToExtract.isEmpty()) {
        QString nextZip = m_nestedZipsToExtract.takeFirst();
        extractNestedZip(nextZip);
    } else {
        m_isExtracting = false;
        m_etaTimer->stop();
        emit isExtractingChanged();
        emit extractionFinished(true, "Extraction completed successfully");
    }
}

bool ZipExtractor::shouldExtractRecursively(const QString &zipPath) const
{
    // Open the zip to check if it contains only another zip or has multiple files
    QZipReader reader(zipPath, QIODevice::ReadOnly);
    if (!reader.isReadable()) {
        return false;
    }

    QList<QZipReader::FileInfo> fileList = reader.fileInfoList();

    // Extract if it contains only one zip file, or if it's a zip among other files
    return !fileList.isEmpty();
}

QString ZipExtractor::getUniqueFileName(const QString &directory, const QString &baseName, const QString &extension) const
{
    QString fileName;
    int counter = 1;

    do {
        fileName = QString("%1 (%2).%3").arg(baseName).arg(counter).arg(extension);
        counter++;
    } while (QFile::exists(directory + "/" + fileName));

    return fileName;
}

void ZipExtractor::updateETA()
{
    if (m_currentFile == 0) {
        m_eta = "Calculating...";
        emit etaChanged();
        return;
    }

    qint64 elapsed = m_elapsedTimer.elapsed();
    double filesPerMs = (double)m_currentFile / elapsed;
    int remainingFiles = m_totalFiles - m_currentFile;

    if (filesPerMs > 0) {
        qint64 remainingMs = remainingFiles / filesPerMs;
        int remainingSeconds = remainingMs / 1000;

        if (remainingSeconds < 60) {
            m_eta = QString("%1 seconds").arg(remainingSeconds);
        } else {
            int minutes = remainingSeconds / 60;
            int seconds = remainingSeconds % 60;
            m_eta = QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
        }
    } else {
        m_eta = "Unknown";
    }

    emit etaChanged();
}

void ZipExtractor::cancelExtraction()
{
    if (m_isExtracting) {
        m_extractTimer->stop();
        m_etaTimer->stop();
        m_isExtracting = false;
        m_nestedZipsToExtract.clear();
        emit isExtractingChanged();
        emit extractionFinished(false, "Extraction cancelled by user");
    }
}

void ZipExtractor::resetProgress()
{
    m_currentFile = 0;
    m_totalFiles = 0;
    m_currentFileName = "";
    m_progress = 0.0;
    m_eta = "Calculating...";
    m_currentZipPath = "";  // Clear the stored path
    m_nestedZipsToExtract.clear();

    if (m_zipReader) {
        delete m_zipReader;
        m_zipReader = nullptr;
    }

    emit currentFileChanged();
    emit totalFilesChanged();
    emit currentFileNameChanged();
    emit progressChanged();
    emit etaChanged();
}
