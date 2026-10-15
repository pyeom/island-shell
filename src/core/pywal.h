#pragma once

#include <QByteArrayView>
#include <QColor>
#include <QString>

#include <array>
#include <optional>

namespace island {

struct PywalColors {
    QColor background;
    QColor foreground;
    std::array<QColor, 16> colors;

    // "color4" → colors[4]; anything else → invalid color.
    [[nodiscard]] QColor named(const QString& name) const;
};

// Parses ~/.cache/wal/colors.json.
std::optional<PywalColors> parsePywal(QByteArrayView json);

} // namespace island
