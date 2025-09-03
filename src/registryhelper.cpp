#include "registryhelper.h"
#include <QCoreApplication>
#include <QDir>

#ifdef Q_OS_WIN
#include <QSettings>
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

bool RegistryHelper::registerContextMenu(const QString &appPath)
{
#ifdef Q_OS_WIN
    QString command = QString("\"%1\" \"%2\"").arg(appPath, "%1");

    bool success = writeRegistryKey(REGISTRY_KEY, "", MENU_TEXT) &&
                   writeRegistryKey(REGISTRY_COMMAND_KEY, "", command);

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
    bool success = deleteRegistryKey("HKEY_CLASSES_ROOT\\*\\shell\\ZipExtractor");

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
    settings.setValue(value, data);
    return settings.status() == QSettings::NoError;
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
    return settings.status() == QSettings::NoError;
#else
    Q_UNUSED(key)
    return false;
#endif
}

QString RegistryHelper::readRegistryKey(const QString &key, const QString &value)
{
#ifdef Q_OS_WIN
    QSettings settings(key, QSettings::NativeFormat);
    return settings.value(value).toString();
#else
    Q_UNUSED(key)
    Q_UNUSED(value)
    return QString();
#endif
}
