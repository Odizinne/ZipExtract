#ifndef REGISTRYHELPER_H
#define REGISTRYHELPER_H

#include <QObject>
#include <QQmlEngine>

class RegistryHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool isRegistered READ isContextMenuRegistered NOTIFY registrationChanged)

public:
    static RegistryHelper* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static RegistryHelper* instance();

    Q_INVOKABLE bool isContextMenuRegistered();
    Q_INVOKABLE bool registerContextMenu(const QString &appPath);
    Q_INVOKABLE bool unregisterContextMenu();
    Q_INVOKABLE QString getCurrentAppPath();

signals:
    void registrationChanged();

private:
    explicit RegistryHelper(QObject *parent = nullptr);

    bool writeRegistryKey(const QString &key, const QString &value, const QString &data);
    bool deleteRegistryKey(const QString &key);
    QString readRegistryKey(const QString &key, const QString &value);

    static RegistryHelper* s_instance;
    static const QString REGISTRY_KEY;
    static const QString REGISTRY_COMMAND_KEY;
    static const QString MENU_TEXT;
};

#endif // REGISTRYHELPER_H
