#include "registryhelper.h"
#include <QCoreApplication>
#include <QDir>

#ifdef Q_OS_WIN
#include <QSettings>
#include <windows.h>
#endif

RegistryHelper* RegistryHelper::s_instance = nullptr;

const QString RegistryHelper::REGISTRY_KEY = "HKEY_CLASSES_ROOT\\*\\shell\\ZipExtractor";
const QString RegistryHelper::REGISTRY_COMMAND_KEY = "HKEY_CLASSES_ROOT\\*\\shell\\ZipExtractor\\command";
const QString RegistryHelper::MENU_TEXT = "Extract Here";

RegistryHelper::RegistryHelper(QObject *parent)
    : QObject(parent)
{
}

RegistryHelper* RegistryHelper::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    if (!s_instance) {
        s_instance = new RegistryHelper();
    }
    return s_instance;
}

RegistryHelper* RegistryHelper::instance()
{
    if (!s_instance) {
        s_instance = new RegistryHelper();
    }
    return s_instance;
}

bool RegistryHelper::isContextMenuRegistered()
{
#ifdef Q_OS_WIN
    QString command = readRegistryKey(REGISTRY_COMMAND_KEY, "");
    return !command.isEmpty();
#else
    return false;
#endif
}

bool RegistryHelper::isRunningAsAdmin()
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

bool RegistryHelper::requiresElevation()
{
    return false; // Always false now since we handle elevation at startup
}

bool RegistryHelper::registerContextMenu(const QString &appPath)
{
#ifdef Q_OS_WIN
    // Normalize the app path and ensure it's quoted properly
    QString normalizedAppPath = QDir::toNativeSeparators(appPath);
    QString command = QString("\"%1\" \"%2\"").arg(normalizedAppPath).arg("%1");

    // Also register the icon (optional but nice)
    QString iconKey = "HKEY_CLASSES_ROOT\\.zip\\shell\\ZipExtractor";

    bool success = writeRegistryKey(REGISTRY_KEY, "", MENU_TEXT) &&
                   writeRegistryKey(REGISTRY_COMMAND_KEY, "", command);

    // Optionally set an icon
    if (success) {
        writeRegistryKey(iconKey, "Icon", QString("\"%1\",0").arg(normalizedAppPath));
    }

    if (success) {
        emit registrationChanged();
    }

    return success;
#else
    Q_UNUSED(appPath)
    return false;
#endif
}

bool RegistryHelper::unregisterContextMenu()
{
#ifdef Q_OS_WIN
    bool success = deleteRegistryKey("HKEY_CLASSES_ROOT\\.zip\\shell\\ZipExtractor");

    if (success) {
        emit registrationChanged();
    }

    return success;
#else
    return false;
#endif
}

QString RegistryHelper::getCurrentAppPath()
{
    return QCoreApplication::applicationFilePath();
}

bool RegistryHelper::writeRegistryKey(const QString &key, const QString &value, const QString &data)
{
#ifdef Q_OS_WIN
    QSettings settings(key, QSettings::NativeFormat);
    settings.setValue(value.isEmpty() ? "." : value, data);
    settings.sync();

    // Check if write was successful by reading it back
    bool success = (settings.status() == QSettings::NoError);
    if (success && !value.isEmpty()) {
        // Verify the value was actually written
        QString readBack = settings.value(value).toString();
        success = (readBack == data);
    }

    return success;
#else
    Q_UNUSED(key)
    Q_UNUSED(value)
    Q_UNUSED(data)
    return false;
#endif
}

bool RegistryHelper::deleteRegistryKey(const QString &key)
{
#ifdef Q_OS_WIN
    QSettings settings(key, QSettings::NativeFormat);
    settings.clear();
    settings.sync();

    // Verify deletion by checking if the key still exists
    bool success = settings.allKeys().isEmpty();

    return success;
#else
    Q_UNUSED(key)
    return false;
#endif
}

QString RegistryHelper::readRegistryKey(const QString &key, const QString &value)
{
#ifdef Q_OS_WIN
    QSettings settings(key, QSettings::NativeFormat);
    return settings.value(value.isEmpty() ? "." : value).toString();
#else
    Q_UNUSED(key)
    Q_UNUSED(value)
    return QString();
#endif
}
