#ifndef ZIPEXTRACTOR_H
#define ZIPEXTRACTOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QQmlEngine>
#include <private/qzipreader_p.h>

class ZipExtractor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(int totalFiles READ totalFiles NOTIFY totalFilesChanged)
    Q_PROPERTY(QString currentFileName READ currentFileName NOTIFY currentFileNameChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString eta READ eta NOTIFY etaChanged)
    Q_PROPERTY(bool isExtracting READ isExtracting NOTIFY isExtractingChanged)

public:
    static ZipExtractor* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static ZipExtractor* instance();

    Q_INVOKABLE void startExtraction(const QString &zipPath, const QString &destPath = "");
    Q_INVOKABLE void cancelExtraction();

    // Property getters
    int currentFile() const { return m_currentFile; }
    int totalFiles() const { return m_totalFiles; }
    QString currentFileName() const { return m_currentFileName; }
    double progress() const { return m_progress; }
    QString eta() const { return m_eta; }
    bool isExtracting() const { return m_isExtracting; }

signals:
    void currentFileChanged();
    void totalFilesChanged();
    void currentFileNameChanged();
    void progressChanged();
    void etaChanged();
    void isExtractingChanged();
    void extractionFinished(bool success, const QString &message);

private slots:
    void processNextFile();
    void updateETA();

private:
    explicit ZipExtractor(QObject *parent = nullptr);
    void resetProgress();

    static ZipExtractor* s_instance;

    int m_currentFile = 0;
    int m_totalFiles = 0;
    QString m_currentFileName;
    double m_progress = 0.0;
    QString m_eta = "Calculating...";
    bool m_isExtracting = false;

    QZipReader *m_zipReader = nullptr;
    QTimer *m_extractTimer;
    QTimer *m_etaTimer;
    QList<QZipReader::FileInfo> m_fileList;
    QString m_destinationPath;
    QElapsedTimer m_elapsedTimer;
};

#endif // ZIPEXTRACTOR_H
