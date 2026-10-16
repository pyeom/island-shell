#include "keybinds.h"

#include "config.h"
#include "hyprland.h"
#include "hyprlandparse.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJSEngine>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcKeys, "island.keybinds")

using namespace Qt::StringLiterals;

namespace island {

namespace {
constexpr auto kRecordSubmap = "island-shell-record";
constexpr auto kDescriptionPrefix = "island-shell: ";
} // namespace

Keybinds* Keybinds::instance() {
    static auto* keys = new Keybinds(QCoreApplication::instance());
    return keys;
}

Keybinds* Keybinds::create(QQmlEngine*, QJSEngine*) {
    auto* keys = instance();
    QJSEngine::setObjectOwnership(keys, QJSEngine::CppOwnership);
    return keys;
}

Keybinds::Keybinds(QObject* parent) : QObject(parent) {
    m_recordTimeout.setSingleShot(true);
    m_recordTimeout.setInterval(20 * 1000);
    connect(&m_recordTimeout, &QTimer::timeout, this, &Keybinds::stopRecording);
    if (!available())
        return;

    auto* hypr = Hyprland::instance();
    connect(hypr, &Hyprland::event, this, &Keybinds::onEvent);
    connect(Config::instance(), &Config::changed, this, [this] { apply(false); });
    apply(true);
    // A previous island may have died while recording.
    hypr->request("submap", [this](const QByteArray& reply) {
        if (reply.trimmed() == kRecordSubmap)
            setSubmap(u"reset"_s);
    });
}

bool Keybinds::available() const {
    return Hyprland::instance()->available();
}

QString Keybinds::islandctl() {
    // Next to island-shell when installed; else whatever is on PATH.
    const auto sibling = QCoreApplication::applicationDirPath() + u"/islandctl"_s;
    return QFileInfo(sibling).isExecutable() ? sibling : u"islandctl"_s;
}

void Keybinds::onEvent(const QString& name, const QString& data) {
    if (name == u"configreloaded") {
        m_submapReady = false;
        apply(true);
    } else if (name == u"submap" && m_recording && data != QLatin1String(kRecordSubmap)) {
        setRecording(false); // Escape, or something else changed it
    }
}

void Keybinds::apply(bool force) {
    QList<std::pair<QString, hypr::KeyCombo>> wanted;
    QHash<QString, QString> keys;
    for (const auto& kb : Config::instance()->data().keybinds) {
        const auto action = QString::fromStdString(kb.action);
        if (const auto combo = hypr::parseKeyCombo(QString::fromStdString(kb.keys))) {
            wanted.append({action, *combo});
            keys.insert(action, hypr::comboText(*combo));
        }
    }
    if (!force && keys == m_applied)
        return;

    QList<hypr::KeyCombo> stale;
    for (const auto& old : std::as_const(m_applied)) {
        const auto combo = hypr::parseKeyCombo(old);
        if (combo && !keys.values().contains(old))
            stale.append(*combo);
    }
    const auto previous = std::exchange(m_applied, keys);
    const auto previouslyReplaced = m_replaced;

    auto* hypr = Hyprland::instance();
    // Which keys Hyprland's config already uses, before they are replaced.
    hypr->request("j/binds", [=, this](const QByteArray& json) {
        const auto bound = hypr::parseBinds(json);
        QStringList replaced;
        for (const auto& [action, combo] : wanted) {
            const bool other = std::ranges::any_of(bound, [&](const hypr::BoundKey& b) {
                return hypr::sameKeys(b, combo) && !b.description.startsWith(QLatin1String(kDescriptionPrefix));
            });
            // Unchanged binds were applied before: what they replaced is gone.
            const bool before = previous.value(action) == hypr::comboText(combo) && previouslyReplaced.contains(action);
            if (other || before)
                replaced.append(action);
        }
        if (replaced != m_replaced) {
            m_replaced = replaced;
            emit changed();
        }
        Hyprland::instance()->configIsLua([=, this](bool lua) { bind(lua, wanted, stale); });
    });
}

void Keybinds::bind(bool lua, const QList<std::pair<QString, hypr::KeyCombo>>& wanted,
                    const QList<hypr::KeyCombo>& stale) {
    auto* hypr = Hyprland::instance();
    const auto ctl = islandctl();
    const auto commandFor = [&](const QString& action) {
        return ctl + u' ' + QString::fromUtf8(hypr::keyAction(action.toStdString())->args);
    };
    const auto setErrors = [this](QStringList errors) {
        if (errors != m_errors) {
            m_errors = std::move(errors);
            emit changed();
        }
    };
    setErrors({});
    qCInfo(lcKeys) << "binding" << wanted.size() << "keys," << stale.size() << "released";

    if (lua) {
        QStringList script;
        for (const auto& combo : stale)
            script.append(hypr::luaUnbind(combo));
        for (const auto& [action, combo] : wanted)
            script.append(hypr::luaBind(combo, commandFor(action), QLatin1String(kDescriptionPrefix) + action));
        if (script.isEmpty())
            return;
        hypr->request("eval " + script.join(u'\n').toUtf8(), [setErrors](const QByteArray& reply) {
            if (reply.trimmed() != "ok") {
                qCWarning(lcKeys).noquote() << "eval ->" << reply.trimmed();
                setErrors({QString::fromUtf8(reply.trimmed())});
            }
        });
        return;
    }

    const auto keyword = [this, hypr](const QString& args) {
        hypr->request("keyword " + args.toUtf8(), [this, args](const QByteArray& reply) {
            if (reply.trimmed() == "ok")
                return;
            qCWarning(lcKeys).noquote() << "keyword" << args << "->" << reply.trimmed();
            m_errors.append(QString::fromUtf8(reply.trimmed()));
            emit changed();
        });
    };
    for (const auto& combo : stale)
        keyword(u"unbind "_s + hypr::confUnbind(combo));
    for (const auto& [action, combo] : wanted) {
        keyword(u"unbind "_s + hypr::confUnbind(combo));
        keyword(u"bindd "_s + hypr::confBind(combo, commandFor(action), QLatin1String(kDescriptionPrefix) + action));
    }
}

void Keybinds::setSubmap(const QString& name) {
    Hyprland::instance()->configIsLua([name](bool lua) {
        Hyprland::instance()->dispatch(lua ? u"hl.dsp.submap("_s + hypr::luaString(name) + u')' : u"submap "_s + name);
    });
}

void Keybinds::startRecording() {
    if (m_recording)
        return;
    setRecording(true);
    m_recordTimeout.start();
    if (!available())
        return;
    auto* hypr = Hyprland::instance();
    hypr->request("j/devices", [this](const QByteArray& json) { m_layout = hypr::parseMainKeyboard(json); });
    hypr->configIsLua([this, hypr](bool lua) {
        if (!m_submapReady) {
            // An otherwise empty submap: no binds but the way out.
            if (lua)
                hypr->request(u"eval hl.define_submap(%1, function() hl.bind(\"Escape\", hl.dsp.submap(\"reset\")) end)"_s
                                  .arg(hypr::luaString(QString::fromLatin1(kRecordSubmap)))
                                  .toUtf8());
            else
                hypr->request(u"[[BATCH]]keyword submap %1;keyword bind ,Escape,submap,reset;keyword submap reset"_s
                                  .arg(QLatin1String(kRecordSubmap))
                                  .toUtf8());
            m_submapReady = true;
        }
        if (m_recording)
            setSubmap(QLatin1String(kRecordSubmap));
    });
}

void Keybinds::stopRecording() {
    if (!m_recording)
        return;
    setRecording(false);
    if (available())
        setSubmap(u"reset"_s);
}

void Keybinds::setRecording(bool on) {
    if (on == m_recording)
        return;
    m_recording = on;
    if (!on)
        m_recordTimeout.stop();
    emit recordingChanged();
}

QString Keybinds::combo(int modifiers, int nativeScanCode) const {
    hypr::KeyCombo c;
    c.key = hypr::keycodeName(m_layout, static_cast<uint32_t>(nativeScanCode));
    if (c.key.isEmpty())
        return {};
    if (modifiers & Qt::MetaModifier)
        c.mods.append(u"SUPER"_s);
    if (modifiers & Qt::ControlModifier)
        c.mods.append(u"CTRL"_s);
    if (modifiers & Qt::AltModifier)
        c.mods.append(u"ALT"_s);
    if (modifiers & Qt::ShiftModifier)
        c.mods.append(u"SHIFT"_s);
    return hypr::comboText(c);
}

} // namespace island
