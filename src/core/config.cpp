#include "config.h"

#include <QCoreApplication>
#include <QFile>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(lcConfig, "island.config")

namespace island {

namespace {

QString configPath() {
    const auto env = qEnvironmentVariable("ISLAND_SHELL_CONFIG");
    if (!env.isEmpty())
        return env;
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
           QLatin1String("/island-shell/config.toml");
}

QVariant toVariant(const toml::node& node) {
    if (const auto* v = node.as_string())
        return QString::fromStdString(v->get());
    if (const auto* v = node.as_integer())
        return QVariant::fromValue<qint64>(v->get());
    if (const auto* v = node.as_floating_point())
        return v->get();
    if (const auto* v = node.as_boolean())
        return v->get();
    if (const auto* arr = node.as_array()) {
        QVariantList list;
        for (const auto& item : *arr)
            list.append(toVariant(item));
        return list;
    }
    if (const auto* table = node.as_table()) {
        QVariantMap map;
        for (const auto& [key, value] : *table)
            map.insert(QString::fromUtf8(key.str().data(), qsizetype(key.str().size())),
                       toVariant(value));
        return map;
    }
    return {};
}

} // namespace

Config* Config::instance() {
    static auto* config = new Config(QCoreApplication::instance());
    return config;
}

Config* Config::create(QQmlEngine*, QJSEngine*) {
    auto* config = instance();
    QJSEngine::setObjectOwnership(config, QJSEngine::CppOwnership);
    return config;
}

Config::Config(QObject* parent) : QObject(parent) {
    m_watch.setPath(configPath());
    connect(&m_watch, &FileWatch::changed, this, &Config::reload);
    reload();
}

QString Config::animations() const {
    switch (m_data.animations) {
    case AnimationMode::Reduced:
        return QStringLiteral("reduced");
    case AnimationMode::Off:
        return QStringLiteral("off");
    case AnimationMode::Full:
        break;
    }
    return QStringLiteral("full");
}

QVariant Config::get(const QString& path, const QVariant& fallback) const {
    const auto node = m_data.raw.at_path(path.toStdString());
    if (!node)
        return fallback;
    auto value = toVariant(*node.node());
    return value.isValid() ? value : fallback;
}

void Config::reload() {
    std::string text;
    QFile file(path());
    if (file.open(QIODevice::ReadOnly))
        text = file.readAll().toStdString();

    std::vector<std::string> errors;
    m_data = parseConfig(text, errors);
    m_errors.clear();
    for (const auto& e : errors) {
        m_errors.append(QString::fromStdString(e));
        qCWarning(lcConfig).noquote() << path() << e.c_str();
    }
    qCInfo(lcConfig).noquote() << (file.isOpen() ? "loaded" : "using defaults, no file at")
                               << path();
    emit changed();
}

} // namespace island
