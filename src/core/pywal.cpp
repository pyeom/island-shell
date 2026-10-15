#include "pywal.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace island {

QColor PywalColors::named(const QString& name) const {
    if (!name.startsWith(QLatin1String("color")))
        return {};
    bool ok = false;
    const int index = name.mid(5).toInt(&ok);
    if (!ok || index < 0 || index >= static_cast<int>(colors.size()))
        return {};
    return colors[index];
}

std::optional<PywalColors> parsePywal(QByteArrayView json) {
    const auto doc = QJsonDocument::fromJson(json.toByteArray());
    if (!doc.isObject())
        return std::nullopt;
    const auto special = doc[QLatin1String("special")].toObject();
    const auto colors = doc[QLatin1String("colors")].toObject();

    PywalColors out;
    out.background = QColor::fromString(special[QLatin1String("background")].toString());
    out.foreground = QColor::fromString(special[QLatin1String("foreground")].toString());
    for (int i = 0; i < static_cast<int>(out.colors.size()); ++i)
        out.colors[i] = QColor::fromString(colors[QStringLiteral("color%1").arg(i)].toString());

    if (!out.background.isValid() || !out.foreground.isValid())
        return std::nullopt;
    return out;
}

} // namespace island
