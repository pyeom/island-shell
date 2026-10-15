#include "theme.h"

#include "config.h"

#include <QCoreApplication>
#include <QFile>
#include <QJSEngine>
#include <QStandardPaths>

namespace island {

Theme* Theme::instance() {
    static auto* theme = new Theme(QCoreApplication::instance());
    return theme;
}

Theme* Theme::create(QQmlEngine*, QJSEngine*) {
    auto* theme = instance();
    QJSEngine::setObjectOwnership(theme, QJSEngine::CppOwnership);
    return theme;
}

Theme::Theme(QObject* parent) : QObject(parent) {
    m_pywalWatch.setPath(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) +
                         QLatin1String("/wal/colors.json"));
    connect(&m_pywalWatch, &FileWatch::changed, this, &Theme::reload);
    connect(Config::instance(), &Config::changed, this, &Theme::apply);
    reload();
}

void Theme::reload() {
    QFile file(m_pywalWatch.path());
    m_pywal = file.open(QIODevice::ReadOnly) ? parsePywal(file.readAll()) : std::nullopt;
    apply();
}

void Theme::apply() {
    const auto& cfg = Config::instance()->data().theme;
    const bool usePywal = cfg.source == "pywal" && m_pywal.has_value();

    m_blur = cfg.style == "blur";
    m_foreground = Qt::white;
    m_background = Qt::black;
    if (m_blur) {
        // Blur needs a translucent fill; the layerrule does the actual blur.
        if (usePywal) {
            m_background = m_pywal->background;
            m_foreground = m_pywal->foreground;
        }
        m_background.setAlphaF(static_cast<float>(cfg.blurOpacity));
    }
    m_secondary = m_foreground;
    m_secondary.setAlphaF(0.6F);

    m_accent = QColor::fromString(QString::fromStdString(cfg.accent));
    if (usePywal) {
        const auto pywalAccent = m_pywal->named(QString::fromStdString(cfg.pywalAccent));
        if (pywalAccent.isValid())
            m_accent = pywalAccent;
    }
    if (!m_accent.isValid())
        m_accent = QColor(0x0a, 0x84, 0xff);

    emit changed();
}

} // namespace island
