#pragma once

#include <QHash>
#include <QObject>
#include <QString>

namespace island {

// What changes with the power profile ([power_modes.<profile>]): Hyprland's
// animations, blur and shadows, the island's own animations, and commands.
// Hyprland options are changed at runtime and put back to what they were
// (the value read before the first change) when the profile no longer
// wants them, when Hyprland reloads its config, and when the island quits.
class PowerModes : public QObject {
    Q_OBJECT

public:
    static PowerModes* instance();

private:
    explicit PowerModes(QObject* parent = nullptr);
    void update();
    void apply(bool entering);
    void setOption(const QString& option, bool value);
    void restoreAll();

    QString m_profile;
    // Hyprland option -> its value before the island changed it.
    QHash<QString, bool> m_originals;
};

} // namespace island
