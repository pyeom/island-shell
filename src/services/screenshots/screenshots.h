#pragma once

#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Screenshots with grim (slurp to select a region or window): saved to
// [screenshots] directory and copied to the clipboard. The island hides while capturing and then offers the
// result (screenshot activity).
class Screenshots : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool capturing READ capturing NOTIFY capturingChanged)
    // Path of the last screenshot, empty once deleted.
    Q_PROPERTY(QString lastPath READ lastPath NOTIFY lastChanged)
    Q_PROPERTY(bool canEdit READ canEdit NOTIFY lastChanged)

public:
    static Screenshots* instance();
    static Screenshots* create(QQmlEngine*, QJSEngine*);

    bool capturing() const { return m_process != nullptr; }
    QString lastPath() const { return m_lastPath; }
    bool canEdit() const { return !editor().isEmpty(); }

    // "region" | "window" | "output" (focused monitor) | "active" (focused window)
    Q_INVOKABLE bool take(const QString& mode);
    Q_INVOKABLE void open();
    Q_INVOKABLE void copy();
    Q_INVOKABLE void edit();
    Q_INVOKABLE void remove();
    Q_INVOKABLE void dismiss();

signals:
    void capturingChanged();
    void lastChanged();
    // A new screenshot was saved.
    void taken(const QString& path);

private:
    explicit Screenshots(QObject* parent = nullptr);
    QString directory() const;
    QString editor() const;

    QPointer<QProcess> m_process;
    QString m_lastPath;
};

} // namespace island
