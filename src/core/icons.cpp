#include "icons.h"

#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QJSEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

namespace island {

namespace {
// Themes disagree on symbolic variants; try "x-symbolic" and "x" either way.
QIcon themeIcon(const QString& name) {
    QIcon icon = QIcon::fromTheme(name);
    if (!icon.isNull())
        return icon;
    static const QString suffix = QStringLiteral("-symbolic");
    const auto alt = name.endsWith(suffix) ? name.chopped(suffix.size()) : name + suffix;
    return QIcon::fromTheme(alt);
}
} // namespace

IconProvider::IconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

QPixmap IconProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) {
    const QSize want = requestedSize.isValid() && !requestedSize.isEmpty() ? requestedSize
                                                                          : QSize(64, 64);
    const auto name = QUrl::fromPercentEncoding(id.toUtf8());
    QIcon icon;
    if (name.startsWith(u'/'))
        icon = QIcon(name);
    else
        icon = themeIcon(name);
    if (icon.isNull())
        icon = QIcon::fromTheme(QStringLiteral("application-x-executable"));
    const QPixmap pm = icon.pixmap(want);
    if (size)
        *size = pm.size();
    return pm;
}

Icons* Icons::create(QQmlEngine*, QJSEngine*) {
    static auto* icons = new Icons();
    QJSEngine::setObjectOwnership(icons, QJSEngine::CppOwnership);
    return icons;
}

QString Icons::source(const QString& icon, const QString& fallback) {
    if (icon.isEmpty())
        return fallback.isEmpty() ? QString() : source(fallback);
    if (icon.startsWith(QLatin1String("file://")) || icon.startsWith(QLatin1String("http")) ||
        icon.startsWith(QLatin1String("image://")) || icon.startsWith(QLatin1String("qrc:")))
        return icon;
    if (icon.startsWith(u'/') && !icon.endsWith(QLatin1String(".svg")) &&
        !icon.endsWith(QLatin1String(".svgz")))
        return QUrl::fromLocalFile(icon).toString();
    if (!fallback.isEmpty() && !exists(icon))
        return source(fallback);
    return QStringLiteral("image://icon/") + QString::fromUtf8(QUrl::toPercentEncoding(icon));
}

bool Icons::exists(const QString& icon) {
    if (icon.startsWith(u'/'))
        return QFileInfo::exists(icon);
    return !themeIcon(icon).isNull();
}

void Icons::setupTheme(const QString& configured) {
    if (!configured.isEmpty()) {
        QIcon::setThemeName(configured);
        return;
    }
    const auto current = QIcon::themeName();
    if (!current.isEmpty() && current != u"hicolor")
        return;
    const auto config = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    const QSettings kde(config + QLatin1String("/kdeglobals"), QSettings::IniFormat);
    auto name = kde.value(QStringLiteral("Icons/Theme")).toString();
    if (name.isEmpty()) {
        const QSettings gtk(config + QLatin1String("/gtk-3.0/settings.ini"), QSettings::IniFormat);
        name = gtk.value(QStringLiteral("Settings/gtk-icon-theme-name")).toString();
    }
    if (!name.isEmpty())
        QIcon::setThemeName(name);
}

} // namespace island
