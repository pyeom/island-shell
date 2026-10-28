#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace island {

// A tray item's menu (com.canonical.dbusmenu), as nested entries:
//   { id, label, enabled, separator, toggle: "checkmark" | "radio" | "",
//     checked, icon, submenu, children: [...] }
// Hidden entries are left out.
//
//   TrayMenu { service: model.service; path: model.menuPath }
class TrayMenu : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString service READ service WRITE setService NOTIFY sourceChanged)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY sourceChanged)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
    explicit TrayMenu(QObject* parent = nullptr);
    ~TrayMenu() override;

    QString service() const { return m_service; }
    void setService(const QString& service);
    QString path() const { return m_path; }
    void setPath(const QString& path);
    QVariantList entries() const { return m_entries; }
    bool loading() const { return m_loading; }

    // Before showing a submenu (0: the menu itself): apps fill some lazily.
    Q_INVOKABLE void aboutToShow(int id);
    // An entry was clicked.
    Q_INVOKABLE void activate(int id);

signals:
    void sourceChanged();
    void entriesChanged();
    void loadingChanged();

private slots:
    void onLayoutUpdated();

private:
    void connectSource();
    void disconnectSource();
    void fetch();
    void setLoading(bool loading);

    QString m_service;
    QString m_path;
    QVariantList m_entries;
    QStringList m_images; // image://tray/ sources of icon-data
    QTimer m_fetch;
    bool m_connected = false;
    bool m_loading = false;
};

} // namespace island
