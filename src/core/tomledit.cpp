#include "tomledit.h"

#include <QRegularExpression>

namespace island {

QString tomlLiteral(const QVariant& value) {
    switch (value.typeId()) {
    case QMetaType::Bool:
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    case QMetaType::Int:
    case QMetaType::LongLong:
    case QMetaType::UInt:
    case QMetaType::ULongLong:
        return QString::number(value.toLongLong());
    case QMetaType::Double:
    case QMetaType::Float: {
        const double d = value.toDouble();
        if (d == static_cast<double>(static_cast<long long>(d)))
            return QString::number(static_cast<long long>(d)) + QStringLiteral(".0");
        return QString::number(d, 'g', 15);
    }
    case QMetaType::QStringList:
    case QMetaType::QVariantList: {
        QStringList items;
        for (const auto& v : value.toList())
            items.append(tomlLiteral(v));
        return u'[' + items.join(QStringLiteral(", ")) + u']';
    }
    default: {
        QString s = value.toString();
        s.replace(u'\\', QStringLiteral("\\\\"));
        s.replace(u'"', QStringLiteral("\\\""));
        s.replace(u'\n', QStringLiteral("\\n"));
        return u'"' + s + u'"';
    }
    }
}

namespace {

// Position of a '#' that starts a comment (outside strings), or -1.
qsizetype commentStart(const QString& line) {
    QChar quote;
    for (qsizetype i = 0; i < line.size(); ++i) {
        const QChar c = line[i];
        if (!quote.isNull()) {
            if (c == u'\\' && quote == u'"')
                ++i;
            else if (c == quote)
                quote = QChar();
        } else if (c == u'"' || c == u'\'') {
            quote = c;
        } else if (c == u'#') {
            return i;
        }
    }
    return -1;
}

QString headerName(const QString& line) {
    static const QRegularExpression header(QStringLiteral("^\\s*\\[\\s*([^\\[\\]]+?)\\s*\\]\\s*(#.*)?$"));
    const auto m = header.match(line);
    return m.hasMatch() ? m.captured(1) : QString();
}

bool isArrayHeader(const QString& line) {
    return line.trimmed().startsWith(QLatin1String("[["));
}

} // namespace

QString tomlSet(const QString& document, const QString& section, const QString& key,
                const QVariant& value) {
    QStringList lines = document.split(u'\n');
    const QString literal = tomlLiteral(value);
    const QRegularExpression keyLine(QStringLiteral("^(\\s*") + QRegularExpression::escape(key) +
                                     QStringLiteral("\\s*=\\s*)(.*)$"));

    // Find the section's line range.
    qsizetype begin = -1;
    qsizetype end = lines.size();
    for (qsizetype i = 0; i < lines.size(); ++i) {
        const auto name = isArrayHeader(lines[i]) ? QStringLiteral("[[array]]") : headerName(lines[i]);
        if (name.isEmpty())
            continue;
        if (begin >= 0) {
            end = i;
            break;
        }
        if (name == section)
            begin = i + 1;
    }

    if (begin < 0) {
        // New section at the end.
        while (!lines.isEmpty() && lines.last().trimmed().isEmpty())
            lines.removeLast();
        lines << QString() << u'[' + section + u']' << key + QStringLiteral(" = ") + literal << QString();
        return lines.join(u'\n');
    }

    for (qsizetype i = begin; i < end; ++i) {
        const auto m = keyLine.match(lines[i]);
        if (!m.hasMatch())
            continue;
        // Keep an inline comment after the value.
        const auto rest = m.captured(2);
        const auto hash = commentStart(rest);
        const QString comment = hash >= 0 ? QStringLiteral("  ") + rest.mid(hash).trimmed() : QString();
        lines[i] = m.captured(1) + literal + comment;
        return lines.join(u'\n');
    }

    // Missing key: after the section's last non-blank line.
    qsizetype insertAt = begin;
    for (qsizetype i = begin; i < end; ++i)
        if (!lines[i].trimmed().isEmpty())
            insertAt = i + 1;
    lines.insert(insertAt, key + QStringLiteral(" = ") + literal);
    return lines.join(u'\n');
}

} // namespace island
