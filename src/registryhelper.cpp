#include "registryhelper.h"
#include <QCoreApplication>
#include <QDir>

#ifdef Q_OS_WIN
#include <QSettings>
#include <windows.h>
#endif

RegistryHelper* RegistryHelper::s_instance = nullptr;

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
    QString zipAssoc = readRegistryKey("HKEY_CLASSES_ROOT\\.zip", "");
    if (zipAssoc.isEmpty())
        return false;

    QString commandKey = QString("HKEY_CLASSES_ROOT\\%1\\shell\\ZipExtractor\\command").arg(zipAssoc);
    QString command = readRegistryKey(commandKey, "");

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
    return false; // Assuming elevation is handled externally
}

bool RegistryHelper::registerContextMenu(const QString &appPath)
{
#ifdef Q_OS_WIN
    QString zipAssoc = readRegistryKey("HKEY_CLASSES_ROOT\\.zip", "");
    if (zipAssoc.isEmpty())
        zipAssoc = ".zip"; // fallback

    QString baseKey = QString("HKEY_CLASSES_ROOT\\%1\\shell\\ZipExtractor").arg(zipAssoc);
    QString commandKey = baseKey + "\\command";

    QString normalizedAppPath = QDir::toNativeSeparators(appPath);
    QString command = QString("\"%1\" \"%%1\"").arg(normalizedAppPath); // %%1 escapes %1 for registry

    bool success = writeRegistryKey(baseKey, "", MENU_TEXT) &&
                   writeRegistryKey(baseKey, "Icon", QString("\"%1\",0").arg(normalizedAppPath)) &&
                   writeRegistryKey(commandKey, "", command);

    if (success)
        emit registrationChanged();

    return success;
#else
    Q_UNUSED(appPath)
    return false;
#endif
}

bool RegistryHelper::unregisterContextMenu()
{
#ifdef Q_OS_WIN
    QString zipAssoc = readRegistryKey("HKEY_CLASSES_ROOT\\.zip", "");
    if (zipAssoc.isEmpty())
        zipAssoc = ".zip"; // fallback

    QString keyToDelete = QString("HKEY_CLASSES_ROOT\\%1\\shell\\ZipExtractor").arg(zipAssoc);
    bool success = deleteRegistryKey(keyToDelete);

    if (success)
        emit registrationChanged();

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

    bool success = (settings.status() == QSettings::NoError);
    if (success && !value.isEmpty()) {
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

    return settings.allKeys().isEmpty();
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
