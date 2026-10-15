#pragma once

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QList>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

namespace island::dbus {

// Values from QtDBus arrive wrapped (QDBusVariant) and complex types arrive as
// QDBusArgument. These unwrap them into plain Qt types.

inline QVariant unwrap(const QVariant& v) {
    if (v.canConvert<QDBusVariant>())
        return v.value<QDBusVariant>().variant();
    return v;
}

template <typename T>
T cast(const QVariant& v) {
    const auto inner = unwrap(v);
    if (inner.canConvert<QDBusArgument>())
        return qdbus_cast<T>(inner.value<QDBusArgument>());
    return inner.value<T>();
}

inline QVariantMap toMap(const QVariant& v) { return cast<QVariantMap>(v); }
inline QStringList toStringList(const QVariant& v) { return cast<QStringList>(v); }

inline QString toPath(const QVariant& v) {
    const auto inner = unwrap(v);
    if (inner.canConvert<QDBusObjectPath>())
        return inner.value<QDBusObjectPath>().path();
    return inner.toString();
}

inline QStringList toPaths(const QVariant& v) {
    QStringList out;
    for (const auto& p : cast<QList<QDBusObjectPath>>(v))
        out.append(p.path());
    return out;
}

} // namespace island::dbus
