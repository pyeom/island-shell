#include "audio.h"

#include <QCoreApplication>
#include <QHash>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

#include <pipewire/extensions/metadata.h>
#include <pipewire/pipewire.h>
#include <spa/param/audio/raw.h>
#include <spa/param/props.h>
#include <spa/pod/iter.h>
#include <spa/utils/result.h>

#include <cmath>
#include <cstring>
#include <optional>

Q_LOGGING_CATEGORY(lcAudio, "island.audio")

namespace island {

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

    struct SinkInfo {
        QString name;
        QString description;
    };
    QHash<uint32_t, SinkInfo> sinks;
    QString defaultSinkName;

    pw_node* node = nullptr;
    uint32_t nodeId = SPA_ID_INVALID;
    spa_hook nodeListener{};

    bool connected = false;
    bool haveState = false;
    qreal volume = 0.0;
    bool muted = false;
    QString description;

    void bindDefaultSink();
    void unbindNode();
    void onParam(const spa_pod* param);
};

namespace {

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
    if (id == SPA_PARAM_Props)
        static_cast<Audio::Private*>(data)->onParam(param);
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
    if (subject != PW_ID_CORE || !key || std::strcmp(key, "default.audio.sink") != 0)
        return 0;
    // value: {"name":"alsa_output..."}
    const auto name = value ? QJsonDocument::fromJson(value).object()[u"name"].toString()
                            : QString();
    if (name != d->defaultSinkName) {
        d->defaultSinkName = name;
        d->bindDefaultSink();
    }
    return 0;
}

const pw_metadata_events kMetadataEvents = [] {
    pw_metadata_events e{};
    e.version = PW_VERSION_METADATA_EVENTS;
    e.property = onMetadataProperty;
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
        if (!mediaClass || std::strcmp(mediaClass, "Audio/Sink") != 0)
            return;
        const char* name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
        const char* desc = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
        d->sinks.insert(id, {QString::fromUtf8(name ? name : ""),
                             QString::fromUtf8(desc ? desc : (name ? name : ""))});
        d->bindDefaultSink();
    }
}

void onGlobalRemove(void* data, uint32_t id) {
    auto* d = static_cast<Audio::Private*>(data);
    d->sinks.remove(id);
    if (id == d->nodeId)
        d->unbindNode();
}

const pw_registry_events kRegistryEvents = [] {
    pw_registry_events e{};
    e.version = PW_VERSION_REGISTRY_EVENTS;
    e.global = onGlobal;
    e.global_remove = onGlobalRemove;
    return e;
}();

} // namespace

void Audio::Private::unbindNode() {
    if (!node)
        return;
    spa_hook_remove(&nodeListener);
    pw_proxy_destroy(reinterpret_cast<pw_proxy*>(node));
    node = nullptr;
    nodeId = SPA_ID_INVALID;
}

void Audio::Private::bindDefaultSink() {
    for (auto it = sinks.cbegin(); it != sinks.cend(); ++it) {
        if (it->name != defaultSinkName)
            continue;
        if (it.key() == nodeId)
            return;
        unbindNode();
        nodeId = it.key();
        description = it->description;
        node = static_cast<pw_node*>(
            pw_registry_bind(registry, nodeId, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0));
        spa_zero(nodeListener);
        pw_node_add_listener(node, &nodeListener, &kNodeEvents, this);
        uint32_t ids[] = {SPA_PARAM_Props};
        pw_node_subscribe_params(node, ids, 1);
        qCInfo(lcAudio).noquote() << "default sink:" << description;
        return;
    }
}

void Audio::Private::onParam(const spa_pod* param) {
    if (!param || !spa_pod_is_object(param))
        return;
    const auto* obj = reinterpret_cast<const spa_pod_object*>(param);
    std::optional<qreal> newVolume;
    std::optional<bool> newMuted;
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
                newVolume = std::cbrt(static_cast<double>(max));
            }
        } else if (prop->key == SPA_PROP_mute) {
            bool m = false;
            if (spa_pod_get_bool(&prop->value, &m) == 0)
                newMuted = m;
        }
    }
    if (!newVolume && !newMuted)
        return;
    const qreal v = newVolume.value_or(volume);
    const bool m = newMuted.value_or(muted);
    const bool differs = std::abs(v - volume) > 0.004 || m != muted;
    const bool first = !haveState;
    volume = v;
    muted = m;
    haveState = true;
    if (differs || first)
        emit q->stateChanged();
    if (differs && !first)
        emit q->changed();
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
    d->unbindNode();
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

bool Audio::available() const { return d->connected; }
qreal Audio::volume() const { return d->volume; }
bool Audio::muted() const { return d->muted; }
QString Audio::sinkDescription() const { return d->description; }

} // namespace island
