#pragma once

#include <QByteArrayView>
#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace island::hypr {

// A key combination as Hyprland binds it: "SUPER + SHIFT + R".
struct KeyCombo {
    QStringList mods; // canonical names (SUPER, CTRL, ALT, SHIFT, ...), in that order
    QString key;      // xkb keysym name ("R", "Print", "space") or "code:N"
    bool operator==(const KeyCombo&) const = default;
};

// "SUPER + SHIFT + R"; case and spacing are free, WIN/META/MOD4 mean SUPER,
// CONTROL means CTRL, MOD1 means ALT. Keys are checked against xkb's keysym
// names, since Hyprland shows an error for a bind it cannot parse.
std::optional<KeyCombo> parseKeyCombo(QStringView text, QString* error = nullptr);
QString comboText(const KeyCombo& combo);
// Hyprland's modmask (as in `j/binds`).
int modMask(const KeyCombo& combo);

// Name to bind a keysym with, empty for modifier keys (Shift_L, Super_R...).
QString keysymName(uint32_t keysym);

// The keyboard layout of Hyprland's main keyboard (`j/devices`).
struct XkbNames {
    QString rules, model, layout, variant, options;
};
XkbNames parseMainKeyboard(QByteArrayView devicesJson);
// What a key binds as, from its xkb keycode, ignoring Shift and friends:
// Hyprland matches SHIFT + 1, not SHIFT + exclam. Empty for modifier keys
// and unknown keycodes.
QString keycodeName(const XkbNames& layout, uint32_t keycode);

struct BindFlags {
    bool repeat = false; // keeps firing while held (volume)
    bool locked = false; // also works on the lock screen (media, volume)
};

// Statements that replace every bind on `combo` with one running `command`.
// Lua config (for `eval`):
QString luaBind(const KeyCombo& combo, const QString& command, const QString& description,
                BindFlags flags = {});
QString luaUnbind(const KeyCombo& combo);
// hyprland.conf (`keyword` arguments): the bind keyword ("bindd", "bindeld"),
// its value, and the "unbind" value.
QString confBindKeyword(BindFlags flags);
QString confBind(const KeyCombo& combo, const QString& command, const QString& description);
QString confUnbind(const KeyCombo& combo);

// A bind from `j/binds`.
struct BoundKey {
    int modmask = 0;
    QString key;
    QString description;
    QString submap;
};
QList<BoundKey> parseBinds(QByteArrayView json);
bool sameKeys(const BoundKey& bound, const KeyCombo& combo);

// The island's actions that [keybinds] can bind, and their islandctl
// arguments.
struct KeyAction {
    std::string_view name;
    std::string_view args;
    BindFlags flags = {};
};
std::span<const KeyAction> keyActions();
std::optional<KeyAction> keyAction(std::string_view name);

} // namespace island::hypr
