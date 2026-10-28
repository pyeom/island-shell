#include "trayparse.h"

#include <QtEndian>

#include <array>
#include <functional>

using namespace Qt::StringLiterals;

namespace island {

TrayAddress parseTrayAddress(const QString& registered, const QString& sender) {
    static const auto kDefaultPath = u"/StatusNotifierItem"_s;
    if (registered.isEmpty())
        return {sender, kDefaultPath};
    if (registered.startsWith(u'/'))
        return {sender, registered};
    const auto slash = registered.indexOf(u'/');
    if (slash > 0)
        return {registered.left(slash), registered.mid(slash)};
    return {registered, kDefaultPath};
}

QImage trayPixmapImage(const QList<TrayPixmap>& pixmaps, int size) {
    const TrayPixmap* best = nullptr;
    for (const auto& p : pixmaps) {
        if (p.width <= 0 || p.height <= 0 || p.data.size() < qsizetype(p.width) * p.height * 4)
            continue;
        if (!best) {
            best = &p;
            continue;
        }
        const bool fits = p.width >= size;
        const bool bestFits = best->width >= size;
        if (fits ? (!bestFits || p.width < best->width) : (!bestFits && p.width > best->width))
            best = &p;
    }
    if (!best)
        return {};
    QImage image(best->width, best->height, QImage::Format_ARGB32);
    const auto* src = reinterpret_cast<const uchar*>(best->data.constData());
    for (int y = 0; y < best->height; ++y) {
        auto* line = reinterpret_cast<quint32*>(image.scanLine(y));
        for (int x = 0; x < best->width; ++x)
            line[x] = qFromBigEndian<quint32>(src + (qsizetype(y) * best->width + x) * 4);
    }
    return image;
}

QString trayDesktopId(const QString& itemId, const QString& title, const QList<DesktopEntry>& entries) {
    QStringList keys;
    for (const auto& k : {itemId, title}) {
        const auto key = k.trimmed().toLower();
        if (!key.isEmpty() && !keys.contains(key))
            keys.append(key);
    }
    if (keys.isEmpty())
        return {};
    const auto fileName = [](const DesktopEntry& e) {
        auto name = e.id.toLower();
        if (name.endsWith(u".desktop"_s))
            name.chop(8);
        return name;
    };
    // Most specific first: "com.discordapp.Discord" ends in "discord".
    const std::array<std::function<bool(const DesktopEntry&, const QString&)>, 4> rules{
        [&](const DesktopEntry& e, const QString& k) { return fileName(e) == k; },
        [&](const DesktopEntry& e, const QString& k) { return fileName(e).endsWith(u'.' + k); },
        [](const DesktopEntry& e, const QString& k) { return e.startupWmClass.toLower() == k; },
        [](const DesktopEntry& e, const QString& k) { return e.name.toLower() == k; },
    };
    for (const auto& rule : rules) {
        for (const auto& key : keys) {
            for (const auto& e : entries) {
                if (!e.hidden && rule(e, key))
                    return e.id;
            }
        }
    }
    return {};
}

QString trayMenuLabel(const QString& label) {
    QString out;
    out.reserve(label.size());
    for (qsizetype i = 0; i < label.size(); ++i) {
        if (label[i] == u'_') {
            if (i + 1 < label.size() && label[i + 1] == u'_')
                out.append(label[++i]);
            continue;
        }
        out.append(label[i]);
    }
    return out;
}

} // namespace island
