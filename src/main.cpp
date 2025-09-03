#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "registryhelper.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

bool isRunningAsAdmin()
{
#ifdef Q_OS_WIN
    BOOL isAdmin = FALSE;
    PSID administratorsGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                 &administratorsGroup)) {
        CheckTokenMembership(NULL, administratorsGroup, &isAdmin);
        FreeSid(administratorsGroup);
    }

    return isAdmin;
#else
    return false;
#endif
}

bool requestElevation()
{
#ifdef Q_OS_WIN
    QString program = QCoreApplication::applicationFilePath();
    std::wstring wProgram = program.toStdWString();

    SHELLEXECUTEINFO sei = { 0 };
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"runas";
    sei.lpFile = wProgram.c_str();
    sei.hwnd = NULL;
    sei.nShow = SW_NORMAL;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    return ShellExecuteEx(&sei);
#else
    return false;
#endif
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QString zipFilePath;
    if (argc > 1) {
        zipFilePath = QString::fromLocal8Bit(argv[1]);
    }

    // If no zip file provided, we're opening the main UI for context menu management
    if (zipFilePath.isEmpty()) {
        // Check if we need admin privileges for registry operations
        if (!isRunningAsAdmin()) {
            // Request elevation and exit current instance
            if (requestElevation()) {
                return 0; // Exit gracefully, elevated instance will start
            } else {
                // User cancelled UAC or elevation failed
                return 1;
            }
        }
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("initialZipPath", zipFilePath);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    if (zipFilePath.isEmpty()) {
        engine.loadFromModule("Odizinne.ZipExtract", "Main");
    } else {
        engine.loadFromModule("Odizinne.ZipExtract", "Extractor");
    }

    return app.exec();
}
