#include "privacy.h"

#include "hyprland.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QJSEngine>
#include <QSet>
#include <QTimer>

#include <chrono>

#include <pipewire/pipewire.h>

#include <sys/inotify.h>
#include <unistd.h>

#include <cstring>

namespace island {


namespace {

struct Capture {
    Privacy::Private* d = nullptr;
    uint32_t id = 0;
    pw_node* node = nullptr;
    spa_hook listener{};
    bool monitor = false; // capturing a sink, not a microphone
    bool running = false;
    bool counted = false; // a real microphone capture, running now
    QString app;
};

} // namespace

struct Privacy::Private {
    Privacy* q = nullptr;
    pw_loop* loop = nullptr;
    pw_context* context = nullptr;
    pw_core* core = nullptr;
    pw_registry* registry = nullptr;
    spa_hook registryListener{};
    // Capture streams, bound so their full props are visible: the registry
    // only carries a few keys, not stream.capture.sink or the binary.
    QHash<uint32_t, Capture*> captures;

    void publish() {
        QSet<QString> unique;
        for (const auto* c : std::as_const(captures))
            if (c->counted)
                unique.insert(c->app);
        QStringList apps(unique.begin(), unique.end());
        apps.sort(Qt::CaseInsensitive);
        q->setMicrophoneApps(apps);
    }

    void scheduleRecheck(uint32_t id) {
        QTimer::singleShot(std::chrono::seconds(5), q, [this, id] {
            Capture* c = captures.value(id);
            if (c && c->counted && !(c->running && !c->monitor)) {
                c->counted = false;
                publish();
            }
        });
    }

    void drop(uint32_t id) {
        Capture* c = captures.take(id);
        if (!c)
            return;
        spa_hook_remove(&c->listener);
        pw_proxy_destroy(reinterpret_cast<pw_proxy*>(c->node));
        delete c;
    }
};

namespace {

bool isTrue(const char* v) {
    return v && (std::strcmp(v, "true") == 0 || std::strcmp(v, "1") == 0);
}

void onNodeInfo(void* data, const pw_node_info* info) {
    auto* c = static_cast<Capture*>(data);
    // Later events carry an empty dict unless the props actually changed.
    if ((info->change_mask & PW_NODE_CHANGE_MASK_PROPS) && info->props) {
        const spa_dict* props = info->props;
        const char* appId = spa_dict_lookup(props, PW_KEY_APP_ID);
        const char* target = spa_dict_lookup(props, PW_KEY_TARGET_OBJECT);
        // Recording what the speakers play (visualizers, recorders of desktop
        // audio) is not the microphone; neither are level meters.
        c->monitor = isTrue(spa_dict_lookup(props, PW_KEY_STREAM_CAPTURE_SINK)) ||
                     isTrue(spa_dict_lookup(props, PW_KEY_STREAM_MONITOR)) ||
                     (target && std::strstr(target, ".monitor")) ||
                     (appId && std::strcmp(appId, "org.PulseAudio.pavucontrol") == 0);
        // The binary ("Discord") says more than many app names ("WEBRTC VoiceEngine").
        const char* app = spa_dict_lookup(props, PW_KEY_APP_PROCESS_BINARY);
        if (!app)
            app = spa_dict_lookup(props, PW_KEY_APP_NAME);
        if (!app)
            app = spa_dict_lookup(props, PW_KEY_NODE_NAME);
        c->app = QString::fromUtf8(app ? app : "?");
    }
    if (info->change_mask & PW_NODE_CHANGE_MASK_STATE)
        c->running = info->state == PW_NODE_STATE_RUNNING;
    const bool counted = c->running && !c->monitor;
    if (counted) {
        c->counted = true;
        c->d->publish();
    } else if (c->counted) {
        // Voice chat suspends and resumes capture with voice activity; keep
        // the indicator steady and only drop it after a few quiet seconds.
        c->d->scheduleRecheck(c->id);
    }
}

const pw_node_events kNodeEvents = [] {
    pw_node_events e{};
    e.version = PW_VERSION_NODE_EVENTS;
    e.info = onNodeInfo;
    return e;
}();

void onGlobal(void* data, uint32_t id, uint32_t /*permissions*/, const char* type,
              uint32_t /*version*/, const spa_dict* props) {
    auto* d = static_cast<Privacy::Private*>(data);
    if (!props || std::strcmp(type, PW_TYPE_INTERFACE_Node) != 0)
        return;
    const char* mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    if (!mediaClass || std::strcmp(mediaClass, "Stream/Input/Audio") != 0)
        return;
    auto* c = new Capture;
    c->d = d;
    c->id = id;
    c->node = static_cast<pw_node*>(
        pw_registry_bind(d->registry, id, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0));
    pw_node_add_listener(c->node, &c->listener, &kNodeEvents, c);
    d->captures.insert(id, c);
}

void onGlobalRemove(void* data, uint32_t id) {
    auto* d = static_cast<Privacy::Private*>(data);
    if (d->captures.contains(id)) {
        d->drop(id);
        d->publish();
    }
}

const pw_registry_events kRegistryEvents = [] {
    pw_registry_events e{};
    e.version = PW_VERSION_REGISTRY_EVENTS;
    e.global = onGlobal;
    e.global_remove = onGlobalRemove;
    return e;
}();

} // namespace

Privacy* Privacy::instance() {
    static auto* privacy = new Privacy(QCoreApplication::instance());
    return privacy;
}

Privacy* Privacy::create(QQmlEngine*, QJSEngine*) {
    auto* privacy = instance();
    QJSEngine::setObjectOwnership(privacy, QJSEngine::CppOwnership);
    return privacy;
}

Privacy::Privacy(QObject* parent) : QObject(parent), d(std::make_unique<Private>()) {
    d->q = this;
    setupPipeWire();
    setupCamera();

    // "screencast>>1,0": state, owner (0 monitor, 1 window).
    connect(Hyprland::instance(), &Hyprland::event, this,
            [this](const QString& name, const QString& data) {
                if (name != u"screencast")
                    return;
                const bool sharing = data.section(u',', 0, 0) == u"1";
                if (sharing != m_screen) {
                    m_screen = sharing;
                    emit changed();
                }
            });
}

Privacy::~Privacy() {
    for (const auto id : d->captures.keys())
        d->drop(id);
    if (m_inotify >= 0)
        ::close(m_inotify);
    if (d->registry) {
        spa_hook_remove(&d->registryListener);
        pw_proxy_destroy(reinterpret_cast<pw_proxy*>(d->registry));
    }
    if (d->core)
        pw_core_disconnect(d->core);
    if (d->context)
        pw_context_destroy(d->context);
    if (d->loop) {
        if (m_pwNotifier)
            pw_loop_leave(d->loop);
        pw_loop_destroy(d->loop);
    }
}

void Privacy::setMicrophoneApps(QStringList apps) {
    if (apps == m_micApps)
        return;
    m_micApps = std::move(apps);
    emit changed();
}

void Privacy::setupPipeWire() {
    pw_init(nullptr, nullptr);
    d->loop = pw_loop_new(nullptr);
    if (!d->loop)
        return;
    d->context = pw_context_new(d->loop, nullptr, 0);
    d->core = d->context ? pw_context_connect(d->context, nullptr, 0) : nullptr;
    if (!d->core)
        return;
    d->registry = pw_core_get_registry(d->core, PW_VERSION_REGISTRY, 0);
    pw_registry_add_listener(d->registry, &d->registryListener, &kRegistryEvents, d.get());
    pw_loop_enter(d->loop);
    m_pwNotifier = new QSocketNotifier(pw_loop_get_fd(d->loop), QSocketNotifier::Read, this);
    connect(m_pwNotifier, &QSocketNotifier::activated, this,
            [this] { pw_loop_iterate(d->loop, 0); });
    pw_loop_iterate(d->loop, 0);
}

void Privacy::setupCamera() {
    const auto devices = QDir(QStringLiteral("/dev")).entryList({QStringLiteral("video*")},
                                                                QDir::System);
    if (devices.isEmpty())
        return;
    m_inotify = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (m_inotify < 0)
        return;
    for (const auto& dev : devices)
        inotify_add_watch(m_inotify, QFile::encodeName(QStringLiteral("/dev/") + dev).constData(),
                          IN_OPEN | IN_CLOSE);

    m_cameraScan.setSingleShot(true);
    m_cameraScan.setInterval(300);
    connect(&m_cameraScan, &QTimer::timeout, this, &Privacy::scanCamera);
    m_inotifyNotifier = new QSocketNotifier(m_inotify, QSocketNotifier::Read, this);
    connect(m_inotifyNotifier, &QSocketNotifier::activated, this, [this] {
        alignas(inotify_event) char buf[4096];
        ssize_t n;
        while ((n = ::read(m_inotify, buf, sizeof(buf))) > 0) {
            for (ssize_t i = 0; i < n;) {
                const auto* ev = reinterpret_cast<const inotify_event*>(buf + i);
                if (ev->mask & IN_Q_OVERFLOW)
                    m_cameraActivity.overflowed();
                if (ev->mask & IN_OPEN)
                    m_cameraActivity.opened();
                if (ev->mask & IN_CLOSE)
                    m_cameraActivity.closed();
                i += static_cast<ssize_t>(sizeof(inotify_event)) + ev->len;
            }
        }
        m_cameraScan.start();
    });
    scanCamera();
}

void Privacy::scanCamera() {
    // Only after open/close events, and not for a probe that opened and
    // closed a camera nobody was using.
    if (!m_cameraActivity.needsScan(!m_cameraApps.isEmpty()) && m_cameraScanned)
        return;
    m_cameraActivity.scanned();
    m_cameraScanned = true;
    const auto list = cameraUsers(QStringLiteral("/proc"), QCoreApplication::applicationPid());
    if (list != m_cameraApps) {
        m_cameraApps = list;
        emit changed();
    }
}

} // namespace island
