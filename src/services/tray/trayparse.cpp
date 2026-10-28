#include "trayparse.h"

#include <QtEndian>

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
