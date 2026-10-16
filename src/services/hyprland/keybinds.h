#pragma once

#include "keybind.h"

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// [keybinds] as Hyprland binds, added while the island runs: each runs
// `islandctl ...` and replaces whatever Hyprland had on the same keys.
// Hyprland drops them when it reloads its config, so they are added again
// after every reload.
class Keybinds : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available CONSTANT)
    // Actions whose keys had a bind in Hyprland's own config, now replaced.
    Q_PROPERTY(QStringList replaced READ replaced NOTIFY changed)
    // Hyprland's answer when it refused binds.
    Q_PROPERTY(QStringList errors READ errors NOTIFY changed)
    // While recording, Hyprland's binds are off (an empty submap; Escape
    // leaves it), so the settings window sees every key.
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)

public:
    static Keybinds* instance();
    static Keybinds* create(QQmlEngine*, QJSEngine*);

    bool available() const;
    QStringList replaced() const { return m_replaced; }
    QStringList errors() const { return m_errors; }
    bool recording() const { return m_recording; }

    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    // "SUPER + R" for a key event (KeyEvent.modifiers, KeyEvent.nativeScanCode);
    // empty while only modifiers are down.
    Q_INVOKABLE QString combo(int modifiers, int nativeScanCode) const;

signals:
    void changed();
    void recordingChanged();

private:
    explicit Keybinds(QObject* parent = nullptr);
    // Binds [keybinds]; unless `force`, only when it changed.
    void apply(bool force);
    void bind(bool lua, const QList<std::pair<QString, hypr::KeyCombo>>& wanted,
              const QList<hypr::KeyCombo>& stale);
    void setSubmap(const QString& name);
    void setRecording(bool on);
    void onEvent(const QString& name, const QString& data);
    static QString islandctl();

    QHash<QString, QString> m_applied; // action → keys
    QStringList m_replaced;
    QStringList m_errors;
    bool m_recording = false;
    bool m_submapReady = false;
    hypr::XkbNames m_layout;
    QTimer m_recordTimeout;
};

} // namespace island
