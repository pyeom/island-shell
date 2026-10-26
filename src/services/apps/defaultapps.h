#pragma once

#include <QObject>
#include <QHash>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Default applications (browser, file manager...) as xdg-open picks them:
// read with `xdg-mime query default`, set with `xdg-mime default`, which
// writes ~/.config/mimeapps.list.
class DefaultApps : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // xdg-mime installed (xdg-utils).
    Q_PROPERTY(bool available READ available CONSTANT)
    // [{ key, current, candidates: [{ id, name, icon }] }]; keys: browser,
    // files, mail, text, images, video, music, pdf. `current` is a desktop
    // file ID, empty when none is set.
    Q_PROPERTY(QVariantList categories READ categories NOTIFY changed)

public:
    static DefaultApps* create(QQmlEngine*, QJSEngine*);

    bool available() const;
    QVariantList categories() const;

    // Reads the current defaults again (the settings window calls it when
    // it opens; other tools may have changed them).
    Q_INVOKABLE void refresh();
    // Makes `desktopId` handle every MIME type of the category.
    Q_INVOKABLE void setDefault(const QString& key, const QString& desktopId);

signals:
    void changed();

private:
    explicit DefaultApps(QObject* parent = nullptr);
    void query(const QString& key);

    QHash<QString, QString> m_current;
};

} // namespace island
