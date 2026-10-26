#include "powermodes.h"

#include "apps.h"
#include "config.h"
#include "hyprland.h"
#include "hyprlandparse.h"
#include "powerprofiles.h"

#include <QCoreApplication>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcPowerModes, "island.powermodes")

namespace island {

namespace {

struct HyprOption {
    const char* name;
    std::string PowerModeConfig::*setting;
};

constexpr HyprOption kHyprOptions[] = {
    {"animations:enabled", &PowerModeConfig::hyprAnimations},
    {"decoration:blur:enabled", &PowerModeConfig::hyprBlur},
    {"decoration:shadow:enabled", &PowerModeConfig::hyprShadows},
};

PowerModeConfig modeFor(const QString& profile) {
    const auto& modes = Config::instance()->data().powerModes;
    const auto it = modes.find(profile.toStdString());
    return it == modes.end() ? PowerModeConfig{} : it->second;
}

} // namespace

PowerModes* PowerModes::instance() {
    static auto* p = new PowerModes(QCoreApplication::instance());
    return p;
}

PowerModes::PowerModes(QObject* parent) : QObject(parent) {
    auto* profiles = PowerProfiles::create(nullptr, nullptr);
    connect(profiles, &PowerProfiles::changed, this, &PowerModes::update);
    connect(Config::instance(), &Config::changed, this, [this] { apply(false); });
    connect(Hyprland::instance(), &Hyprland::event, this, [this](const QString& name, const QString&) {
        if (name != u"configreloaded")
            return;
        // The reload put every option back to the config: those are the
        // values to restore now, and the mode's changes are gone.
        m_originals.clear();
        apply(false);
    });
    connect(qApp, &QCoreApplication::aboutToQuit, this, &PowerModes::restoreAll);
    update();
}

void PowerModes::update() {
    const auto profile = PowerProfiles::create(nullptr, nullptr)->active();
    if (profile.isEmpty() || profile == m_profile)
        return;
    qCInfo(lcPowerModes).noquote() << "power profile:" << profile;
    m_profile = profile;
    apply(true);
}

void PowerModes::apply(bool entering) {
    if (m_profile.isEmpty())
        return;
    const auto mode = modeFor(m_profile);

    Config::instance()->setAnimationsOverride(
        mode.islandAnimations == "keep" ? QString() : QString::fromStdString(mode.islandAnimations));

    for (const auto& option : kHyprOptions) {
        const QString name = QLatin1String(option.name);
        const auto& want = mode.*option.setting;
        if (want == "keep") {
            if (m_originals.contains(name))
                setOption(name, m_originals.take(name));
            continue;
        }
        const bool value = want == "on";
        if (m_originals.contains(name)) {
            setOption(name, value);
            continue;
        }
        // Remember what the config has before the first change.
        Hyprland::instance()->request("j/getoption " + name.toUtf8(), [this, name, value](const QByteArray& reply) {
            const auto original = hypr::parseBoolOption(reply);
            if (!original) {
                qCWarning(lcPowerModes).noquote() << "cannot read" << name << "->" << reply.trimmed();
                return;
            }
            if (!m_originals.contains(name))
                m_originals.insert(name, *original);
            setOption(name, value);
        });
    }

    if (entering) {
        for (const auto& command : mode.commands)
            Apps::instance()->run(QString::fromStdString(command));
    }
}

void PowerModes::setOption(const QString& name, bool value) {
    auto* h = Hyprland::instance();
    h->configIsLua([h, name, value](bool lua) {
        const auto command = lua ? hypr::luaSetOption(name, value) : hypr::confSetOption(name, value);
        h->request(command, [command](const QByteArray& reply) {
            if (reply.trimmed() != "ok")
                qCWarning(lcPowerModes).noquote() << command << "->" << reply.trimmed();
        });
    });
}

void PowerModes::restoreAll() {
    // The event loop is stopping: blocking requests, or nothing would be sent.
    const auto lua = Hyprland::instance()->knownLua();
    if (!lua)
        return;
    for (auto it = m_originals.cbegin(); it != m_originals.cend(); ++it)
        Hyprland::instance()->requestNow(*lua ? hypr::luaSetOption(it.key(), it.value())
                                              : hypr::confSetOption(it.key(), it.value()));
    m_originals.clear();
}

} // namespace island
