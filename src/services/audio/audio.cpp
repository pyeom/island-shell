#include "audio.h"

#include <QCoreApplication>
#include <QHash>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QProcess>
#include <QRegularExpression>

#include <pipewire/extensions/metadata.h>
#include <pipewire/pipewire.h>
#include <spa/param/audio/raw.h>
#include <spa/param/props.h>
#include <spa/pod/iter.h>
#include <spa/utils/result.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <optional>
#include <utility>

Q_LOGGING_CATEGORY(lcAudio, "island.audio")

namespace island {

namespace {

// One followed default node (output or microphone).
struct Followed {
    Audio::Private* d = nullptr;
    bool isSource = false;
    QString defaultName; // node.name from the "default" metadata
    pw_node* node = nullptr;
    uint32_t id = SPA_ID_INVALID;
    spa_hook listener{};
    bool have = false;
    qreal volume = 0;
    bool muted = false;
    QString description;
};

struct Device {
    QString name;
    QString description;
    bool isSource = false;
};

// A bound output stream, published as an AudioStream.
struct Stream {
    Audio::Private* d = nullptr;
    pw_node* node = nullptr;
    spa_hook listener{};
    AudioStream* object = nullptr;
};

} // namespace

struct Audio::Private {
    Audio* q = nullptr;

    pw_loop* loop = nullptr;
    pw_context* context = nullptr;
    pw_core* core = nullptr;
    pw_registry* registry = nullptr;
    spa_hook coreListener{};
    spa_hook registryListener{};

    pw_metadata* metadata = nullptr;
    spa_hook metadataListener{};

    QHash<uint32_t, Device> devices;
    QHash<uint32_t, Stream*> streams;
    Followed sink;
    Followed source;
    bool connected = false;

    void follow(Followed& f);
    void unbind(Followed& f);
    void onParam(Followed& f, const spa_pod* param);
    void dropStream(uint32_t id);
};

namespace {

// Volume (as pactl shows it) and mute from a Props param.
void readProps(const spa_pod* param, std::optional<qreal>& volume, std::optional<bool>& muted) {
    if (!param || !spa_pod_is_object(param))
        return;
    const auto* obj = reinterpret_cast<const spa_pod_object*>(param);
    const spa_pod_prop* prop = nullptr;
    SPA_POD_OBJECT_FOREACH(obj, prop) {
        if (prop->key == SPA_PROP_channelVolumes) {
            float values[SPA_AUDIO_MAX_CHANNELS];
            const uint32_t n = spa_pod_copy_array(&prop->value, SPA_TYPE_Float, values,
                                                  SPA_AUDIO_MAX_CHANNELS);
            if (n > 0) {
                float max = 0.0F;
                for (uint32_t i = 0; i < n; ++i)
                    max = std::max(max, values[i]);
                // Props hold linear volume; pactl and most UIs show the cube root.
                volume = std::cbrt(static_cast<double>(max));
            }
        } else if (prop->key == SPA_PROP_mute) {
            bool m = false;
            if (spa_pod_get_bool(&prop->value, &m) == 0)
                muted = m;
        }
    }
}

void onCoreError(void* data, uint32_t id, int /*seq*/, int res, const char* message) {
    auto* d = static_cast<Audio::Private*>(data);
    qCWarning(lcAudio) << "pipewire error" << id << spa_strerror(res) << message;
    if (id == PW_ID_CORE && res == -EPIPE) {
        d->connected = false;
        emit d->q->availableChanged();
    }
}

const pw_core_events kCoreEvents = [] {
    pw_core_events e{};
    e.version = PW_VERSION_CORE_EVENTS;
    e.error = onCoreError;
    return e;
}();

void onNodeParam(void* data, int /*seq*/, uint32_t id, uint32_t /*index*/, uint32_t /*next*/,
                 const spa_pod* param) {
    auto* f = static_cast<Followed*>(data);
    if (id == SPA_PARAM_Props)
        f->d->onParam(*f, param);
}

const pw_node_events kNodeEvents = [] {
    pw_node_events e{};
    e.version = PW_VERSION_NODE_EVENTS;
    e.param = onNodeParam;
    return e;
}();

int onMetadataProperty(void* data, uint32_t subject, const char* key, const char* /*type*/,
                       const char* value) {
    auto* d = static_cast<Audio::Private*>(data);
    if (subject != PW_ID_CORE || !key)
        return 0;
    Followed* f = std::strcmp(key, "default.audio.sink") == 0     ? &d->sink
                  : std::strcmp(key, "default.audio.source") == 0 ? &d->source
                                                                   : nullptr;
    if (!f)
        return 0;
    // value: {"name":"alsa_output..."}
    const auto name = value ? QJsonDocument::fromJson(value).object()[u"name"].toString()
                            : QString();
    if (name != f->defaultName) {
        f->defaultName = name;
        d->follow(*f);
        emit d->q->devicesChanged();
    }
    return 0;
}

const pw_metadata_events kMetadataEvents = [] {
    pw_metadata_events e{};
    e.version = PW_VERSION_METADATA_EVENTS;
    e.property = onMetadataProperty;
    return e;
}();

void onStreamInfo(void* data, const pw_node_info* info) {
    auto* st = static_cast<Stream*>(data);
    // Later events carry an empty dict unless the props actually changed.
    if (!(info->change_mask & PW_NODE_CHANGE_MASK_PROPS) || !info->props)
        return;
    const auto prop = [info](const char* key) {
        return QString::fromUtf8(spa_dict_lookup(info->props, key));
    };
    const auto binary = prop(PW_KEY_APP_PROCESS_BINARY);
    auto name = prop(PW_KEY_APP_NAME);
    // Audio libraries name themselves, not the app: the binary ("Discord")
    // says more than "WEBRTC VoiceEngine".
    static const QStringList libraries = {QStringLiteral("WEBRTC VoiceEngine"), QStringLiteral("Chromium"),
                                          QStringLiteral("ALSA plug-in"), QStringLiteral("PipeWire ALSA")};
    if (name.isEmpty() || std::ranges::any_of(libraries, [&](const QString& l) { return name.startsWith(l); }))
        name = binary.isEmpty() ? name : binary;
    if (name.isEmpty())
        name = prop(PW_KEY_NODE_DESCRIPTION);
    if (name.isEmpty())
        name = prop(PW_KEY_NODE_NAME);
    auto title = prop(PW_KEY_MEDIA_NAME);
    // Generic stream names say nothing ("Playback", "audio stream", the app name).
    if (title.compare(name, Qt::CaseInsensitive) == 0 ||
        title.contains(QLatin1String("stream"), Qt::CaseInsensitive) ||
        title.compare(QLatin1String("Playback"), Qt::CaseInsensitive) == 0)
        title.clear();
    auto icon = prop(PW_KEY_APP_ICON_NAME);
    if (icon.isEmpty())
        icon = binary.isEmpty() ? name.toLower() : binary;
    st->object->setInfo(name, title, icon);
}

void onStreamParam(void* data, int /*seq*/, uint32_t id, uint32_t /*index*/, uint32_t /*next*/,
                   const spa_pod* param) {
    auto* st = static_cast<Stream*>(data);
    if (id != SPA_PARAM_Props)
        return;
    std::optional<qreal> volume;
    std::optional<bool> muted;
    readProps(param, volume, muted);
    if (volume || muted)
        st->object->setVolume(volume.value_or(st->object->volume()),
                              muted.value_or(st->object->muted()));
}

const pw_node_events kStreamEvents = [] {
    pw_node_events e{};
    e.version = PW_VERSION_NODE_EVENTS;
    e.info = onStreamInfo;
    e.param = onStreamParam;
    return e;
}();

void onGlobal(void* data, uint32_t id, uint32_t /*permissions*/, const char* type,
              uint32_t /*version*/, const spa_dict* props) {
    auto* d = static_cast<Audio::Private*>(data);
    if (!props)
        return;
    if (std::strcmp(type, PW_TYPE_INTERFACE_Metadata) == 0) {
        const char* name = spa_dict_lookup(props, PW_KEY_METADATA_NAME);
        if (d->metadata || !name || std::strcmp(name, "default") != 0)
            return;
        d->metadata = static_cast<pw_metadata*>(
            pw_registry_bind(d->registry, id, type, PW_VERSION_METADATA, 0));
        pw_metadata_add_listener(d->metadata, &d->metadataListener, &kMetadataEvents, d);
    } else if (std::strcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
        const char* mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
        if (!mediaClass)
            return;
        if (std::strcmp(mediaClass, "Stream/Output/Audio") == 0) {
            auto* st = new Stream;
            st->d = d;
            st->object = new AudioStream(static_cast<int>(id), d->q);
            st->node = static_cast<pw_node*>(
                pw_registry_bind(d->registry, id, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0));
            pw_node_add_listener(st->node, &st->listener, &kStreamEvents, st);
            uint32_t ids[] = {SPA_PARAM_Props};
            pw_node_subscribe_params(st->node, ids, 1);
            d->streams.insert(id, st);
            // Listed once it has a name, so the mixer never shows a blank row.
            QObject::connect(st->object, &AudioStream::changed, d->q,
                             [q = d->q, o = st->object, listed = false]() mutable {
                if (!o->name().isEmpty() && !std::exchange(listed, true))
                    emit q->streamsChanged();
            });
            return;
        }
        const bool sink = std::strcmp(mediaClass, "Audio/Sink") == 0;
        const bool source = std::strcmp(mediaClass, "Audio/Source") == 0;
        if (!sink && !source)
            return;
        const char* name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
        const char* desc = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
        d->devices.insert(id, {QString::fromUtf8(name ? name : ""),
                               QString::fromUtf8(desc ? desc : (name ? name : "")), source});
        d->follow(source ? d->source : d->sink);
        emit d->q->devicesChanged();
    }
}

void onGlobalRemove(void* data, uint32_t id) {
    auto* d = static_cast<Audio::Private*>(data);
    if (d->streams.contains(id)) {
        d->dropStream(id);
        emit d->q->streamsChanged();
        return;
    }
    if (d->devices.remove(id) == 0)
        return;
    for (Followed* f : {&d->sink, &d->source}) {
        if (f->id == id) {
            d->unbind(*f);
            f->have = false;
            emit d->q->stateChanged();
        }
    }
    emit d->q->devicesChanged();
}

const pw_registry_events kRegistryEvents = [] {
    pw_registry_events e{};
    e.version = PW_VERSION_REGISTRY_EVENTS;
    e.global = onGlobal;
    e.global_remove = onGlobalRemove;
    return e;
}();

} // namespace

void Audio::Private::unbind(Followed& f) {
    if (!f.node)
        return;
    spa_hook_remove(&f.listener);
    pw_proxy_destroy(reinterpret_cast<pw_proxy*>(f.node));
    f.node = nullptr;
    f.id = SPA_ID_INVALID;
}

void Audio::Private::dropStream(uint32_t id) {
    Stream* st = streams.take(id);
    if (!st)
        return;
    spa_hook_remove(&st->listener);
    pw_proxy_destroy(reinterpret_cast<pw_proxy*>(st->node));
    st->object->deleteLater();
    delete st;
}

void Audio::Private::follow(Followed& f) {
    for (auto it = devices.cbegin(); it != devices.cend(); ++it) {
        if (it->isSource != f.isSource || it->name != f.defaultName)
            continue;
        if (it.key() == f.id)
            return;
        unbind(f);
        f.id = it.key();
        f.description = it->description;
        f.node = static_cast<pw_node*>(
            pw_registry_bind(registry, f.id, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0));
        spa_zero(f.listener);
        pw_node_add_listener(f.node, &f.listener, &kNodeEvents, &f);
        uint32_t ids[] = {SPA_PARAM_Props};
        pw_node_subscribe_params(f.node, ids, 1);
        qCInfo(lcAudio).noquote() << (f.isSource ? "default source:" : "default sink:")
                                  << f.description;
        return;
    }
}

void Audio::Private::onParam(Followed& f, const spa_pod* param) {
    std::optional<qreal> newVolume;
    std::optional<bool> newMuted;
    readProps(param, newVolume, newMuted);
    if (!newVolume && !newMuted)
        return;
    const qreal v = newVolume.value_or(f.volume);
    const bool m = newMuted.value_or(f.muted);
    const bool volumeDiffers = std::abs(v - f.volume) > 0.004;
    const bool muteDiffers = m != f.muted;
    const bool first = !f.have;
    f.volume = v;
    f.muted = m;
    f.have = true;
    if (volumeDiffers || muteDiffers || first)
        emit q->stateChanged();
    if (first)
        return;
    if (!f.isSource && (volumeDiffers || muteDiffers))
        emit q->changed();
    if (f.isSource && muteDiffers)
        emit q->micMuteChanged();
}

Audio* Audio::instance() {
    static auto* audio = new Audio(QCoreApplication::instance());
    return audio;
}

Audio* Audio::create(QQmlEngine*, QJSEngine*) {
    auto* audio = instance();
    QJSEngine::setObjectOwnership(audio, QJSEngine::CppOwnership);
    return audio;
}

Audio::Audio(QObject* parent) : QObject(parent), d(std::make_unique<Private>()) {
    d->q = this;
    d->sink.d = d.get();
    d->source.d = d.get();
    d->source.isSource = true;
    m_volumeThrottle.setSingleShot(true);
    m_volumeThrottle.setInterval(40);
    connect(&m_volumeThrottle, &QTimer::timeout, this, &Audio::flushVolumes);

    pw_init(nullptr, nullptr);
    d->loop = pw_loop_new(nullptr);
    if (!d->loop)
        return;
    d->context = pw_context_new(d->loop, nullptr, 0);
    d->core = d->context ? pw_context_connect(d->context, nullptr, 0) : nullptr;
    if (!d->core) {
        qCWarning(lcAudio) << "could not connect to PipeWire; volume OSD disabled";
        return;
    }
    pw_core_add_listener(d->core, &d->coreListener, &kCoreEvents, d.get());
    d->registry = pw_core_get_registry(d->core, PW_VERSION_REGISTRY, 0);
    pw_registry_add_listener(d->registry, &d->registryListener, &kRegistryEvents, d.get());
    d->connected = true;

    pw_loop_enter(d->loop);
    m_notifier = new QSocketNotifier(pw_loop_get_fd(d->loop), QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this,
            [this] { pw_loop_iterate(d->loop, 0); });
    pw_loop_iterate(d->loop, 0);
}

Audio::~Audio() {
    if (m_notifier)
        m_notifier->setEnabled(false);
    d->unbind(d->sink);
    d->unbind(d->source);
    for (const auto id : d->streams.keys())
        d->dropStream(id);
    if (d->metadata) {
        spa_hook_remove(&d->metadataListener);
        pw_proxy_destroy(reinterpret_cast<pw_proxy*>(d->metadata));
    }
    if (d->registry) {
        spa_hook_remove(&d->registryListener);
        pw_proxy_destroy(reinterpret_cast<pw_proxy*>(d->registry));
    }
    if (d->core)
        pw_core_disconnect(d->core);
    if (d->context)
        pw_context_destroy(d->context);
    if (d->loop) {
        if (m_notifier)
            pw_loop_leave(d->loop);
        pw_loop_destroy(d->loop);
    }
}

namespace {
void wpctl(const QStringList& args) {
    QProcess::startDetached(QStringLiteral("wpctl"), args);
}
} // namespace

void Audio::setVolume(qreal volume) {
    m_pendingVolume = std::clamp(volume, 0.0, 1.5);
    if (!m_volumeThrottle.isActive())
        m_volumeThrottle.start();
}

void Audio::setMicVolume(qreal volume) {
    m_pendingMicVolume = std::clamp(volume, 0.0, 1.5);
    if (!m_volumeThrottle.isActive())
        m_volumeThrottle.start();
}

void Audio::setStreamVolume(int id, qreal volume) {
    if (!d->streams.contains(static_cast<uint32_t>(id)))
        return;
    m_pendingStreamVolumes.insert(id, std::clamp(volume, 0.0, 1.5));
    if (!m_volumeThrottle.isActive())
        m_volumeThrottle.start();
}

void Audio::setStreamMuted(int id, bool muted) {
    if (!d->streams.contains(static_cast<uint32_t>(id)))
        return;
    wpctl({QStringLiteral("set-mute"), QString::number(id),
           muted ? QStringLiteral("1") : QStringLiteral("0")});
}

void Audio::flushVolumes() {
    if (m_pendingVolume >= 0)
        wpctl({QStringLiteral("set-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@"),
               QString::number(m_pendingVolume, 'f', 3)});
    if (m_pendingMicVolume >= 0)
        wpctl({QStringLiteral("set-volume"), QStringLiteral("@DEFAULT_AUDIO_SOURCE@"),
               QString::number(m_pendingMicVolume, 'f', 3)});
    for (auto it = m_pendingStreamVolumes.cbegin(); it != m_pendingStreamVolumes.cend(); ++it)
        wpctl({QStringLiteral("set-volume"), QString::number(it.key()),
               QString::number(it.value(), 'f', 3)});
    m_pendingStreamVolumes.clear();
    m_pendingVolume = m_pendingMicVolume = -1;
}

void Audio::setMuted(bool muted) {
    wpctl({QStringLiteral("set-mute"), QStringLiteral("@DEFAULT_AUDIO_SINK@"),
           muted ? QStringLiteral("1") : QStringLiteral("0")});
}

void Audio::setMicMuted(bool muted) {
    wpctl({QStringLiteral("set-mute"), QStringLiteral("@DEFAULT_AUDIO_SOURCE@"),
           muted ? QStringLiteral("1") : QStringLiteral("0")});
}

void Audio::setDefault(int id) {
    if (!d->devices.contains(static_cast<uint32_t>(id)))
        return;
    wpctl({QStringLiteral("set-default"), QString::number(id)});
}

bool Audio::available() const { return d->connected; }
qreal Audio::volume() const { return d->sink.volume; }
bool Audio::muted() const { return d->sink.muted; }
QString Audio::sinkDescription() const { return d->sink.description; }
bool Audio::hasMicrophone() const { return d->source.have; }
qreal Audio::micVolume() const { return d->source.volume; }
bool Audio::micMuted() const { return d->source.muted; }
QString Audio::sourceDescription() const { return d->source.description; }

namespace {
QVariantList deviceList(const QHash<uint32_t, Device>& devices, bool sources,
                        const QString& defaultName) {
    QVariantList out;
    for (auto it = devices.cbegin(); it != devices.cend(); ++it) {
        if (it->isSource != sources)
            continue;
        // "Ellesmere HDMI Audio [Radeon RX 470/480] Digital Stereo (HDMI 3)":
        // the bracketed chip name is noise in a menu.
        static const QRegularExpression brackets(QStringLiteral("\\s*\\[[^\\]]*\\]"));
        out.append(QVariantMap{
            {QStringLiteral("id"), it.key()},
            {QStringLiteral("name"), it->name},
            {QStringLiteral("description"), QString(it->description).remove(brackets).simplified()},
            {QStringLiteral("isDefault"), it->name == defaultName},
        });
    }
    std::ranges::sort(out, [](const QVariant& a, const QVariant& b) {
        return QString::localeAwareCompare(a.toMap().value(QStringLiteral("description")).toString(),
                                           b.toMap().value(QStringLiteral("description")).toString()) < 0;
    });
    return out;
}
} // namespace

QList<AudioStream*> Audio::streams() const {
    QList<AudioStream*> out;
    for (const auto* st : std::as_const(d->streams)) {
        const auto name = st->object->name();
        // speech-dispatcher's dummy output never plays anything.
        if (!name.isEmpty() && name != QLatin1String("speech-dispatcher-dummy"))
            out.append(st->object);
    }
    std::ranges::sort(out, [](const AudioStream* a, const AudioStream* b) {
        return QString::localeAwareCompare(a->name(), b->name()) < 0;
    });
    return out;
}

void AudioStream::setInfo(const QString& name, const QString& title, const QString& iconName) {
    if (name == m_name && title == m_title && iconName == m_iconName)
        return;
    m_name = name;
    m_title = title;
    m_iconName = iconName;
    emit changed();
}

void AudioStream::setVolume(qreal volume, bool muted) {
    if (std::abs(volume - m_volume) <= 0.004 && muted == m_muted)
        return;
    m_volume = volume;
    m_muted = muted;
    emit changed();
}

QVariantList Audio::sinks() const { return deviceList(d->devices, false, d->sink.defaultName); }
QVariantList Audio::sources() const { return deviceList(d->devices, true, d->source.defaultName); }

} // namespace island
