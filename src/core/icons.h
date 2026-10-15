#pragma once

#include <QObject>
#include <QQuickImageProvider>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// image://icon/<name> — icons from the current theme, or files by path.
class IconProvider : public QQuickImageProvider {
public:
    IconProvider();
    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;
};

// Turns whatever apps hand us (theme name, absolute path, file:// URL) into
// an Image source:
//   Image { source: Icons.source(app.icon) }
class Icons : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static Icons* create(QQmlEngine*, QJSEngine*);

    Q_INVOKABLE static QString source(const QString& icon, const QString& fallback = {});
    Q_INVOKABLE static bool exists(const QString& icon);

    // Picks the icon theme: config, then the platform's, then kdeglobals and
    // GTK settings, so icons work without a Qt platform theme plugin.
    static void setupTheme(const QString& configured);

private:
    using QObject::QObject;
};

} // namespace island
