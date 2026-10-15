#include "useractivities.h"

#include "activity.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlError>
#include <QStandardPaths>
#include <QUrl>

Q_LOGGING_CATEGORY(lcActivities, "island.activities")

namespace island {

namespace {
// "5:3: Cannot assign to ..." — the file name is shown separately.
QString describe(const QQmlError& e) {
    if (e.line() > 0)
        return QStringLiteral("%1:%2: %3").arg(e.line()).arg(e.column()).arg(e.description());
    return e.description();
}
} // namespace

UserActivities* UserActivities::instance() {
    static auto* ua = new UserActivities(QCoreApplication::instance());
    return ua;
}

UserActivities* UserActivities::create(QQmlEngine*, QJSEngine*) {
    auto* ua = instance();
    QJSEngine::setObjectOwnership(ua, QJSEngine::CppOwnership);
    return ua;
}

UserActivities::UserActivities(QObject* parent) : QObject(parent) {
    const auto env = qEnvironmentVariable("ISLAND_SHELL_ACTIVITIES");
    m_directory = !env.isEmpty()
                      ? env
                      : QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                            QLatin1String("/island-shell/activities");

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(200);
    connect(&m_debounce, &QTimer::timeout, this, &UserActivities::reload);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_debounce, qOverload<>(&QTimer::start));
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_debounce,
            qOverload<>(&QTimer::start));
}

void UserActivities::setEngine(QQmlEngine* engine) {
    if (m_engine)
        disconnect(m_engine, nullptr, this, nullptr);
    m_engine = engine;
    if (m_engine)
        connect(m_engine, &QQmlEngine::warnings, this, &UserActivities::onEngineWarnings);
}

QStringList UserActivities::loaded() const {
    return m_files;
}

void UserActivities::reload() {
    for (const auto& obj : std::as_const(m_objects))
        delete obj.data();
    m_objects.clear();
    m_files.clear();
    m_errors.clear();

    if (!m_engine)
        return;
    // Edited files must be recompiled, not served from the cache.
    m_engine->clearComponentCache();

    QDir dir(m_directory);
    const auto entries = dir.entryInfoList({QStringLiteral("*.qml")}, QDir::Files, QDir::Name);
    for (const auto& info : entries) {
        const auto url = QUrl::fromLocalFile(info.absoluteFilePath());
        QQmlComponent component(m_engine, url, QQmlComponent::PreferSynchronous);
        if (component.isError()) {
            for (const auto& e : component.errors())
                addError(info.fileName(), describe(e));
            continue;
        }
        QObject* obj = component.create();
        if (!obj) {
            for (const auto& e : component.errors())
                addError(info.fileName(), describe(e));
            continue;
        }
        if (!qobject_cast<Activity*>(obj)) {
            addError(info.fileName(), QStringLiteral("root object must be an Activity"));
            delete obj;
            continue;
        }
        QJSEngine::setObjectOwnership(obj, QJSEngine::CppOwnership);
        m_objects.append(obj);
        m_files.append(info.fileName());
    }
    qCInfo(lcActivities).noquote() << "loaded" << m_files.size() << "user activities from"
                                   << m_directory;
    watch();
    emit changed();
}

void UserActivities::watch() {
    if (!m_watcher.files().isEmpty())
        m_watcher.removePaths(m_watcher.files());
    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    if (!QDir(m_directory).exists())
        return;
    QStringList paths{m_directory};
    QDirIterator it(m_directory, {QStringLiteral("*.qml"), QStringLiteral("*.js")}, QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext())
        paths.append(it.next());
    QDirIterator dirs(m_directory, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dirs.hasNext())
        paths.append(dirs.next());
    m_watcher.addPaths(paths);
}

void UserActivities::addError(const QString& file, const QString& message) {
    // A broken binding re-reports on every evaluation; keep each message once.
    constexpr qsizetype kMaxErrors = 50;
    for (const auto& e : std::as_const(m_errors))
        if (e.toMap().value(QStringLiteral("message")) == message)
            return;
    if (m_errors.size() >= kMaxErrors)
        return;
    qCWarning(lcActivities).noquote() << file << message;
    m_errors.append(QVariantMap{{QStringLiteral("file"), file}, {QStringLiteral("message"), message}});
}

void UserActivities::onEngineWarnings(const QList<QQmlError>& warnings) {
    // Runtime errors (bad bindings, exceptions) in user files.
    const auto prefix = QUrl::fromLocalFile(m_directory).toString();
    bool any = false;
    for (const auto& w : warnings) {
        const auto url = w.url().toString();
        if (!url.startsWith(prefix))
            continue;
        addError(url.mid(prefix.size() + 1), describe(w));
        any = true;
    }
    if (any)
        emit changed();
}

} // namespace island
