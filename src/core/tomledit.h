#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>

namespace island {

// TOML literal for a scalar or a list of strings: "text", 42, 0.5, true,
// ["a", "b"].
QString tomlLiteral(const QVariant& value);

// Sets `key` in `[section]` of a TOML document, editing only that line:
// comments, other keys, order and blank lines are kept. A missing key is
// added at the end of its section; a missing section at the end of the file.
// Commented-out keys ("# key = ...") are left alone.
QString tomlSet(const QString& document, const QString& section, const QString& key,
                const QVariant& value);

} // namespace island
