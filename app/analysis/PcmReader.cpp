#include "PcmReader.h"

#include "core/Paths.h"
#include "net/ProxySettings.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryFile>

#include <mpv/client.h>

#include <cstring>

namespace {

QString temporaryPattern()
{
    const QString directory = core::paths::cacheDir() + QStringLiteral("/analysis");
    QDir().mkpath(directory);
    return directory + QStringLiteral("/pcm-XXXXXX.f32");
}

bool setOption(mpv_handle *handle, const char *name, const QByteArray &value)
{
    return mpv_set_option_string(handle, name, value.constData()) >= 0;
}

}

namespace analysis {

std::optional<PcmReader::Result> PcmReader::read(const player::Stream &stream,
                                                 const std::atomic_bool &cancelled)
{
    if (!stream.valid() || cancelled.load())
        return std::nullopt;

    QTemporaryFile output(temporaryPattern());
    output.setAutoRemove(true);
    if (!output.open())
        return std::nullopt;
    const QString path = output.fileName();
    output.close();

    mpv_handle *handle = mpv_create();
    if (!handle)
        return std::nullopt;

    const bool configured = setOption(handle, "vid", "no")
        && setOption(handle, "audio-display", "no") && setOption(handle, "ao", "pcm")
        && setOption(handle, "ao-pcm-file", path.toUtf8())
        && setOption(handle, "ao-pcm-waveheader", "no")
        && setOption(handle, "audio-samplerate", QByteArray::number(kSampleRate))
        && setOption(handle, "audio-channels", "mono") && setOption(handle, "audio-format", "float")
        && setOption(handle, "cache", "yes");
    const QString proxy = net::ProxySettings::instance().effectiveMpvProxy();
    if (!proxy.isEmpty())
        setOption(handle, "http-proxy", proxy.toUtf8());
    if (!configured || mpv_initialize(handle) < 0) {
        mpv_terminate_destroy(handle);
        return std::nullopt;
    }

    if (!stream.userAgent.isEmpty())
        mpv_set_property_string(handle, "user-agent", stream.userAgent.toUtf8().constData());
    const char *clr[] = {"change-list", "http-header-fields", "clr", "", nullptr};
    mpv_command(handle, clr);

    const QString host = stream.url.host();
    if (host.contains(QLatin1String("googlevideo.com")) || host.contains(QLatin1String("youtube.com"))) {
        const char *origin[] = {"change-list", "http-header-fields", "append",
                                "Origin: https://music.youtube.com", nullptr};
        mpv_command(handle, origin);
        const char *referer[] = {"change-list", "http-header-fields", "append",
                                 "Referer: https://music.youtube.com/", nullptr};
        mpv_command(handle, referer);
    }
    if (!stream.cookie.isEmpty()) {
        const QByteArray cookieHeader = (QStringLiteral("Cookie: ") + stream.cookie).toUtf8();
        const char *cookie[] = {"change-list", "http-header-fields", "append",
                                cookieHeader.constData(), nullptr};
        mpv_command(handle, cookie);
    }

    const QByteArray target = stream.url.toString().toUtf8();
    const char *arguments[] = {"loadfile", target.constData(), "replace", nullptr};
    if (mpv_command(handle, arguments) < 0) {
        mpv_terminate_destroy(handle);
        return std::nullopt;
    }

    QElapsedTimer timer;
    timer.start();
    bool complete = false;
    while (!cancelled.load() && !complete) {
        mpv_event *event = mpv_wait_event(handle, 0.1);
        if (event->event_id != MPV_EVENT_END_FILE)
            continue;
        const auto *end = static_cast<mpv_event_end_file *>(event->data);
        complete = end->reason == MPV_END_FILE_REASON_EOF;
        break;
    }

    if (cancelled.load()) {
        const char *stop[] = {"stop", nullptr};
        mpv_command(handle, stop);
    }
    mpv_terminate_destroy(handle);
    if (!complete || cancelled.load())
        return std::nullopt;

    QFile input(path);
    if (!input.open(QIODevice::ReadOnly))
        return std::nullopt;
    const QByteArray bytes = input.readAll();
    if (bytes.isEmpty() || bytes.size() % int(sizeof(float)) != 0)
        return std::nullopt;

    Result result;
    result.samples.resize(bytes.size() / int(sizeof(float)));
    memcpy(result.samples.data(), bytes.constData(), size_t(bytes.size()));
    result.durationMs = qint64(result.samples.size()) * 1000 / kSampleRate;
    result.elapsedMs = timer.elapsed();
    return result;
}

}
