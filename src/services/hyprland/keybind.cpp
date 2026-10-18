#include "keybind.h"

#include "hyprlandparse.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <xkbcommon/xkbcommon.h>

#include <array>
#include <memory>

using namespace Qt::StringLiterals;

namespace island::hypr {

namespace {

struct Modifier {
    QStringView name;
    int mask;
};

// Canonical order and Hyprland's masks.
constexpr std::array kModifiers{
    Modifier{u"SUPER", 64}, Modifier{u"CTRL", 4}, Modifier{u"ALT", 8}, Modifier{u"SHIFT", 1},
    Modifier{u"CAPS", 2},   Modifier{u"MOD2", 16}, Modifier{u"MOD3", 32}, Modifier{u"MOD5", 128},
};

QString canonicalModifier(const QString& upper) {
    if (upper == u"WIN" || upper == u"LOGO" || upper == u"META" || upper == u"MOD4")
        return u"SUPER"_s;
    if (upper == u"CONTROL")
        return u"CTRL"_s;
    if (upper == u"MOD1")
        return u"ALT"_s;
    for (const auto& m : kModifiers) {
        if (m.name == upper)
            return upper;
    }
    return {};
}

int modifierOrder(const QString& name) {
    for (size_t i = 0; i < kModifiers.size(); ++i) {
        if (kModifiers[i].name == name)
            return static_cast<int>(i);
    }
    return static_cast<int>(kModifiers.size());
}

bool isModifierKeysym(xkb_keysym_t sym) {
    return (sym >= XKB_KEY_Shift_L && sym <= XKB_KEY_Hyper_R) ||
           (sym >= XKB_KEY_ISO_Lock && sym <= XKB_KEY_ISO_Level5_Lock) || sym == XKB_KEY_Num_Lock ||
           sym == XKB_KEY_Mode_switch;
}

} // namespace

QString keysymName(uint32_t keysym) {
    if (keysym == XKB_KEY_NoSymbol || isModifierKeysym(keysym))
        return {};
    std::array<char, 64> buf{};
    if (xkb_keysym_get_name(keysym, buf.data(), buf.size()) <= 0)
        return {};
    auto name = QString::fromUtf8(buf.data());
    // Letters read better upper case; Hyprland matches either.
    if (name.size() == 1)
        name = name.toUpper();
    return name;
}

std::optional<KeyCombo> parseKeyCombo(QStringView text, QString* error) {
    const auto fail = [&](const QString& message) -> std::optional<KeyCombo> {
        if (error)
            *error = message;
        return std::nullopt;
    };
    const auto parts = text.split(u'+');
    KeyCombo combo;
    for (qsizetype i = 0; i < parts.size(); ++i) {
        const auto part = parts[i].trimmed().toString();
        if (part.isEmpty())
            return fail(text.trimmed().isEmpty() ? u"no key"_s : u"empty part in \"%1\""_s.arg(text));
        if (i + 1 < parts.size()) {
            const auto mod = canonicalModifier(part.toUpper());
            if (mod.isEmpty())
                return fail(u"unknown modifier \"%1\""_s.arg(part));
            if (!combo.mods.contains(mod))
                combo.mods.append(mod);
            continue;
        }
        if (part.startsWith(u"code:"_s, Qt::CaseInsensitive)) {
            bool ok = false;
            const int code = part.mid(5).toInt(&ok);
            if (!ok || code <= 0)
                return fail(u"bad key code \"%1\""_s.arg(part));
            combo.key = u"code:"_s + QString::number(code);
            continue;
        }
        const auto sym = xkb_keysym_from_name(part.toUtf8().constData(), XKB_KEYSYM_CASE_INSENSITIVE);
        if (sym == XKB_KEY_NoSymbol)
            return fail(u"unknown key \"%1\""_s.arg(part));
        // A modifier on its own ("SUPER + Super_L") is fine; keysymName() drops it.
        const auto name = keysymName(sym);
        combo.key = name.isEmpty() ? part : name;
    }
    std::ranges::stable_sort(combo.mods, {}, modifierOrder);
    return combo;
}

QString comboText(const KeyCombo& combo) {
    auto parts = combo.mods;
    parts.append(combo.key);
    return parts.join(u" + "_s);
}

int modMask(const KeyCombo& combo) {
    int mask = 0;
    for (const auto& m : kModifiers) {
        if (combo.mods.contains(m.name))
            mask |= m.mask;
    }
    return mask;
}

XkbNames parseMainKeyboard(QByteArrayView devicesJson) {
    const auto keyboards = QJsonDocument::fromJson(devicesJson.toByteArray()).object().value(u"keyboards"_s).toArray();
    QJsonObject chosen;
    for (const auto& kb : keyboards) {
        const auto o = kb.toObject();
        if (chosen.isEmpty() || o.value(u"main"_s).toBool())
            chosen = o;
        if (o.value(u"main"_s).toBool())
            break;
    }
    return {chosen.value(u"rules"_s).toString(), chosen.value(u"model"_s).toString(),
            chosen.value(u"layout"_s).toString(), chosen.value(u"variant"_s).toString(),
            chosen.value(u"options"_s).toString()};
}

QString keycodeName(const XkbNames& layout, uint32_t keycode) {
    struct KeymapDeleter {
        void operator()(xkb_keymap* k) const { xkb_keymap_unref(k); }
    };
    struct ContextDeleter {
        void operator()(xkb_context* c) const { xkb_context_unref(c); }
    };
    // Compiling a keymap takes a few milliseconds: keep the last one.
    static std::unique_ptr<xkb_context, ContextDeleter> context(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
    static std::unique_ptr<xkb_keymap, KeymapDeleter> keymap;
    static QString keymapFor;
    if (!context)
        return {};
    const auto id = QStringList{layout.rules, layout.model, layout.layout, layout.variant, layout.options}.join(u'\n');
    if (!keymap || keymapFor != id) {
        const auto rules = layout.rules.toUtf8(), model = layout.model.toUtf8(), lay = layout.layout.toUtf8(),
                   variant = layout.variant.toUtf8(), options = layout.options.toUtf8();
        const auto orNull = [](const QByteArray& b) { return b.isEmpty() ? nullptr : b.constData(); };
        const xkb_rule_names names{orNull(rules), orNull(model), orNull(lay), orNull(variant), orNull(options)};
        keymap.reset(xkb_keymap_new_from_names(context.get(), &names, XKB_KEYMAP_COMPILE_NO_FLAGS));
        keymapFor = id;
    }
    if (!keymap)
        return {};
    const xkb_keysym_t* syms = nullptr;
    if (xkb_keymap_key_get_syms_by_level(keymap.get(), keycode, 0, 0, &syms) < 1)
        return {};
    return keysymName(syms[0]);
}

QString luaBind(const KeyCombo& combo, const QString& command, const QString& description,
                BindFlags flags) {
    QString options = u"description = "_s + luaString(description);
    if (flags.repeat)
        options += u", repeating = true"_s;
    if (flags.locked)
        options += u", locked = true"_s;
    return luaUnbind(combo) + u"\nhl.bind("_s + luaString(comboText(combo)) + u", hl.dsp.exec_cmd("_s +
           luaString(command) + u"), { "_s + options + u" })"_s;
}

QString luaUnbind(const KeyCombo& combo) {
    return u"hl.unbind("_s + luaString(comboText(combo)) + u')';
}

QString confBindKeyword(BindFlags flags) {
    return u"bind"_s + (flags.repeat ? u"e"_s : QString()) + (flags.locked ? u"l"_s : QString()) + u'd';
}

QString confBind(const KeyCombo& combo, const QString& command, const QString& description) {
    auto desc = description;
    desc.replace(u',', u' '); // fields are comma separated; the command is the last one
    return confUnbind(combo) + u',' + desc + u",exec,"_s + command;
}

QString confUnbind(const KeyCombo& combo) {
    return combo.mods.join(u' ') + u',' + combo.key;
}

QList<BoundKey> parseBinds(QByteArrayView json) {
    QList<BoundKey> out;
    for (const auto& v : QJsonDocument::fromJson(json.toByteArray()).array()) {
        const auto o = v.toObject();
        out.append({o.value(u"modmask"_s).toInt(), o.value(u"key"_s).toString(),
                    o.value(u"description"_s).toString(), o.value(u"submap"_s).toString()});
    }
    return out;
}

bool sameKeys(const BoundKey& bound, const KeyCombo& combo) {
    return bound.submap.isEmpty() && bound.modmask == modMask(combo) &&
           bound.key.compare(combo.key, Qt::CaseInsensitive) == 0;
}

std::span<const KeyAction> keyActions() {
    static constexpr std::array kActions{
        KeyAction{"launcher", "launcher toggle"},
        KeyAction{"power", "power"},
        KeyAction{"quick_settings", "quick"},
        KeyAction{"settings", "settings"},
        KeyAction{"expand", "expand"},
        KeyAction{"collapse", "collapse"},
        KeyAction{"media", "expand media"},
        KeyAction{"wallpaper", "wallpaper"},
        KeyAction{"wallpaper_random", "wallpaper random"},
        KeyAction{"screenshot_region", "screenshot region"},
        KeyAction{"screenshot_window", "screenshot window"},
        KeyAction{"screenshot_output", "screenshot output"},
        KeyAction{"dnd", "dnd toggle"},
        KeyAction{"notifications_clear", "notifications clear"},
        KeyAction{"reload", "reload"},
        KeyAction{"media_play_pause", "media play-pause", {.locked = true}},
        KeyAction{"media_next", "media next", {.locked = true}},
        KeyAction{"media_previous", "media previous", {.locked = true}},
        KeyAction{"volume_up", "volume up", {.repeat = true, .locked = true}},
        KeyAction{"volume_down", "volume down", {.repeat = true, .locked = true}},
        KeyAction{"volume_mute", "volume mute", {.locked = true}},
        KeyAction{"mic_mute", "volume mic-mute", {.locked = true}},
    };
    return kActions;
}

std::optional<KeyAction> keyAction(std::string_view name) {
    for (const auto& a : keyActions()) {
        if (a.name == name)
            return a;
    }
    return std::nullopt;
}

} // namespace island::hypr
