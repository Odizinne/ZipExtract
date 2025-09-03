#include "ZipExtractor.h"
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

    // Set destination path
    if (destPath.isEmpty()) {
        QFileInfo zipInfo(zipPath);
        m_destinationPath = zipInfo.absolutePath() + "/" + zipInfo.baseName();
    } else {
        m_destinationPath = destPath;
    }

    // Create destination directory
    QDir().mkpath(m_destinationPath);

    // Open ZIP file - Fixed constructor call
    m_zipReader = new QZipReader(zipPath, QIODevice::ReadOnly);
    if (!m_zipReader->isReadable()) {
        delete m_zipReader; // Use delete instead of deleteLater
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
    }

    // Update progress
    m_currentFile++;
    emit currentFileChanged();

    m_progress = (double)m_currentFile / m_totalFiles * 100.0;
    emit progressChanged();

    // Schedule next file
    m_extractTimer->start();
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

    if (m_zipReader) {
        delete m_zipReader; // Use delete instead of deleteLater
        m_zipReader = nullptr;
    }

    emit currentFileChanged();
    emit totalFilesChanged();
    emit currentFileNameChanged();
    emit progressChanged();
    emit etaChanged();
}
