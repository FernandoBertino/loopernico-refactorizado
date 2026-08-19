#pragma once

#include <JuceHeader.h>

#if JUCE_WINDOWS
#include <audioclient.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#endif

#include <atomic>
#include <vector>

// Captura el mix del dispositivo de reproduccion predeterminado de Windows.
// WASAPI se drena en un temporizador de alta resolucion independiente de la UI;
// el callback de audio solo consume un ring buffer preasignado y no realiza
// COM, esperas ni memoria.
class SystemAudioCapture final : private juce::HighResolutionTimer
{
public:
    struct Status
    {
        bool starting{ false };
        bool active{ false };
        juce::String deviceName;
        juce::String error;
        double sampleRate{ 0.0 };
    };

    SystemAudioCapture()
#if JUCE_WINDOWS
        : leftRing(kRingCapacity, 0.0f), rightRing(kRingCapacity, 0.0f)
#endif
    {
    }

    ~SystemAudioCapture() override { stop(); }

    void start()
    {
#if !JUCE_WINDOWS
        statusState.store(3, std::memory_order_release);
#else
        stop();
        statusState.store(1, std::memory_order_release);

        const HRESULT comResult = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        comInitialisedByUs = SUCCEEDED(comResult);
        if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE)
        {
            failStart();
            return;
        }

        HRESULT hr = ::CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                        __uuidof(IMMDeviceEnumerator), (void**) enumerator.put());
        if (SUCCEEDED(hr))
            hr = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, device.put());
        if (SUCCEEDED(hr))
            hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                  (void**) client.put());
        if (SUCCEEDED(hr))
            hr = client->GetMixFormat(&mixFormat);
        if (FAILED(hr) || mixFormat == nullptr)
        {
            failStart();
            return;
        }

        floatingPoint = mixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT;
        bool pcm = mixFormat->wFormatTag == WAVE_FORMAT_PCM;
        if (mixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE
            && mixFormat->cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
        {
            const auto* extensible = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(mixFormat);
            floatingPoint = ::IsEqualGUID(extensible->SubFormat,
                                          KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) != FALSE;
            pcm = ::IsEqualGUID(extensible->SubFormat, KSDATAFORMAT_SUBTYPE_PCM) != FALSE;
        }

        if ((!floatingPoint && !pcm) || mixFormat->nChannels == 0
            || (mixFormat->wBitsPerSample != 8 && mixFormat->wBitsPerSample != 16
                && mixFormat->wBitsPerSample != 24 && mixFormat->wBitsPerSample != 32))
        {
            failStart();
            return;
        }

        constexpr REFERENCE_TIME bufferDuration100ns = 200000; // 20 ms
        hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
                                bufferDuration100ns, 0, mixFormat, nullptr);
        if (SUCCEEDED(hr))
            hr = client->GetService(__uuidof(IAudioCaptureClient), (void**) capture.put());
        if (SUCCEEDED(hr))
            hr = client->Start();
        if (FAILED(hr))
        {
            failStart();
            return;
        }

        writeFrame.store(0, std::memory_order_release);
        peakSinceStart.store(0.0f, std::memory_order_relaxed);
        generation.fetch_add(1, std::memory_order_acq_rel);
        sourceSampleRate.store((double) mixFormat->nSamplesPerSec, std::memory_order_release);
        statusState.store(2, std::memory_order_release);
        // El drenaje no puede depender del hilo de mensajes: una repintada o un
        // dialogo lento producirian huecos audibles en la grabacion.
        startTimer(2);
#endif
    }

    void stop()
    {
        stopTimer();
#if JUCE_WINDOWS
        cleanupResources(true);
#endif
        sourceSampleRate.store(0.0, std::memory_order_release);
        statusState.store(0, std::memory_order_release);
    }

    Status getStatus() const
    {
        Status result;
        const int state = statusState.load(std::memory_order_acquire);
        result.starting = state == 1;
        result.active = state == 2;
        if (result.active)
            result.deviceName = "Salida predeterminada de Windows";
        if (state == 3)
            result.error = "No se pudo abrir la salida predeterminada de Windows.";
        result.sampleRate = sourceSampleRate.load(std::memory_order_acquire);
        return result;
    }

    float getPeakSinceStart() const noexcept
    {
#if JUCE_WINDOWS
        return peakSinceStart.load(std::memory_order_relaxed);
#else
        return 0.0f;
#endif
    }

    bool read(juce::AudioBuffer<float>& destination, int startSample,
              int numSamples, double destinationSampleRate) noexcept
    {
        destination.clear(startSample, numSamples);
#if !JUCE_WINDOWS
        juce::ignoreUnused(destinationSampleRate);
        return false;
#else
        const double captureRate = sourceSampleRate.load(std::memory_order_acquire);
        const auto publishedWrite = writeFrame.load(std::memory_order_acquire);
        if (captureRate <= 0.0 || destinationSampleRate <= 0.0 || publishedWrite < 2)
            return false;

        const auto currentGeneration = generation.load(std::memory_order_acquire);
        constexpr double targetDelaySeconds = 0.12;
        const double desiredDelay = juce::jlimit(256.0, captureRate * 0.20,
                                                 captureRate * targetDelaySeconds);
        if (consumerGeneration != currentGeneration)
        {
            consumerGeneration = currentGeneration;
            consumerPosition = 0.0;
            consumerResumeWrite = publishedWrite;
            consumerStarted = false;
        }

        // Esperar un pequeno colchon antes de arrancar evita que el consumidor
        // alcance al capturador durante los primeros callbacks.
        if (!consumerStarted)
        {
            if ((double) (publishedWrite - consumerResumeWrite) < desiredDelay + 2.0)
                return false;
            consumerPosition = (double) publishedWrite - desiredDelay;
            consumerStarted = true;
        }

        const double backlog = (double) publishedWrite - consumerPosition;
        if (backlog > captureRate * 0.25 || backlog > (double) kRingCapacity - 4.0)
            consumerPosition = (double) publishedWrite - desiredDelay;

        const double nominalRatio = captureRate / destinationSampleRate;
        // WASAPI y el dispositivo ASIO pueden tener relojes fisicos levemente
        // distintos. Esta correccion muy suave mantiene constante el colchon
        // sin modificar perceptiblemente el tono ni la velocidad.
        const double delayError = (backlog - desiredDelay) / desiredDelay;
        const double ratio = nominalRatio * juce::jlimit(0.997, 1.003,
                                                         1.0 + delayError * 0.001);
        auto* left = destination.getWritePointer(0, startSample);
        auto* right = destination.getNumChannels() > 1
            ? destination.getWritePointer(1, startSample) : nullptr;
        int rendered = 0;
        uint64_t lastAvailableWrite = publishedWrite;
        for (int outputFrame = 0; outputFrame < numSamples; ++outputFrame)
        {
            const auto frame0 = (uint64_t) consumerPosition;
            const auto availableWrite = writeFrame.load(std::memory_order_acquire);
            lastAvailableWrite = availableWrite;
            if (frame0 + 1 < availableWrite)
            {
                const float fraction = (float) (consumerPosition - (double) frame0);
                const auto i0 = (size_t) (frame0 & kRingMask);
                const auto i1 = (size_t) ((frame0 + 1) & kRingMask);
                left[outputFrame] = leftRing[i0] + fraction * (leftRing[i1] - leftRing[i0]);
                if (right != nullptr)
                    right[outputFrame] = rightRing[i0]
                                       + fraction * (rightRing[i1] - rightRing[i0]);
                ++rendered;
            }

            // Incluso ante un underrun mantenemos el reloj real. Esperar aqui
            // las muestras atrasadas era lo que ralentizaba y entrecortaba el loop.
            consumerPosition += ratio;
        }

        // Si el endpoint deja de entregar paquetes (por ejemplo durante una
        // pausa de YouTube), rearmar el colchon con audio nuevo. Asi no se
        // releen muestras antiguas ni queda el cursor esperando en el futuro.
        if (rendered < numSamples)
        {
            consumerStarted = false;
            consumerResumeWrite = lastAvailableWrite;
        }
        return rendered > 0;
#endif
    }

private:
#if JUCE_WINDOWS
    template <typename Type>
    struct ComObject
    {
        ~ComObject() { reset(); }
        void reset() noexcept { if (value != nullptr) { value->Release(); value = nullptr; } }
        Type** put() noexcept { return &value; }
        Type* operator->() const noexcept { return value; }
        Type* value{ nullptr };
    };

    static float decodeSample(const BYTE* frame, int channel,
                              const WAVEFORMATEX& format, bool isFloat) noexcept
    {
        const int bytesPerSample = format.wBitsPerSample / 8;
        const BYTE* sample = frame + channel * bytesPerSample;
        if (isFloat && format.wBitsPerSample == 32)
        {
            float value = 0.0f;
            std::memcpy(&value, sample, sizeof(value));
            return juce::jlimit(-1.0f, 1.0f, value);
        }
        if (format.wBitsPerSample == 16)
        {
            int16_t value = 0;
            std::memcpy(&value, sample, sizeof(value));
            return (float) value / 32768.0f;
        }
        if (format.wBitsPerSample == 24)
        {
            int32_t value = (int32_t) sample[0] | ((int32_t) sample[1] << 8)
                          | ((int32_t) sample[2] << 16);
            if ((value & 0x00800000) != 0)
                value |= (int32_t) 0xff000000;
            return (float) value / 8388608.0f;
        }
        if (format.wBitsPerSample == 32)
        {
            int32_t value = 0;
            std::memcpy(&value, sample, sizeof(value));
            return (float) ((double) value / 2147483648.0);
        }
        return format.wBitsPerSample == 8 ? ((float) sample[0] - 128.0f) / 128.0f : 0.0f;
    }

    void publishPacket(const BYTE* data, UINT32 frames, DWORD flags) noexcept
    {
        auto write = writeFrame.load(std::memory_order_relaxed);
        const int channels = juce::jmax(1, (int) mixFormat->nChannels);
        float packetPeak = 0.0f;
        for (UINT32 frameIndex = 0; frameIndex < frames; ++frameIndex)
        {
            float left = 0.0f, right = 0.0f;
            if ((flags & AUDCLNT_BUFFERFLAGS_SILENT) == 0 && data != nullptr)
            {
                const BYTE* frame = data + (size_t) frameIndex * mixFormat->nBlockAlign;
                left = decodeSample(frame, 0, *mixFormat, floatingPoint);
                right = channels > 1 ? decodeSample(frame, 1, *mixFormat, floatingPoint) : left;
            }
            packetPeak = juce::jmax(packetPeak, juce::jmax(std::abs(left), std::abs(right)));
            const auto index = (size_t) (write & kRingMask);
            leftRing[index] = left;
            rightRing[index] = right;
            ++write;
        }
        writeFrame.store(write, std::memory_order_release);
        float previousPeak = peakSinceStart.load(std::memory_order_relaxed);
        while (packetPeak > previousPeak
               && !peakSinceStart.compare_exchange_weak(previousPeak, packetPeak,
                                                         std::memory_order_relaxed)) {}
    }

    void hiResTimerCallback() override
    {
        if (capture.value == nullptr || statusState.load(std::memory_order_acquire) != 2)
            return;

        HRESULT hr = S_OK;
        UINT32 packetFrames = 0;
        while (SUCCEEDED(hr = capture->GetNextPacketSize(&packetFrames)) && packetFrames > 0)
        {
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            hr = capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
            if (FAILED(hr))
                break;
            publishPacket(data, frames, flags);
            capture->ReleaseBuffer(frames);
        }

        if (FAILED(hr))
        {
            stopTimer();
            sourceSampleRate.store(0.0, std::memory_order_release);
            statusState.store(3, std::memory_order_release);
        }
    }

    void failStart()
    {
        cleanupResources(false);
        sourceSampleRate.store(0.0, std::memory_order_release);
        statusState.store(3, std::memory_order_release);
    }

    void cleanupResources(bool stopClient)
    {
        if (stopClient && client.value != nullptr)
            client->Stop();
        capture.reset();
        client.reset();
        device.reset();
        enumerator.reset();
        if (mixFormat != nullptr)
        {
            ::CoTaskMemFree(mixFormat);
            mixFormat = nullptr;
        }
        if (comInitialisedByUs)
        {
            ::CoUninitialize();
            comInitialisedByUs = false;
        }
    }

    static constexpr uint64_t kRingCapacity = 1u << 20;
    static constexpr uint64_t kRingMask = kRingCapacity - 1;
    std::vector<float> leftRing, rightRing;
    std::atomic<uint64_t> writeFrame{ 0 }, generation{ 0 };
    std::atomic<float> peakSinceStart{ 0.0f };
    uint64_t consumerGeneration{ 0 };
    uint64_t consumerResumeWrite{ 0 };
    double consumerPosition{ 0.0 };
    bool consumerStarted{ false };
    ComObject<IMMDeviceEnumerator> enumerator;
    ComObject<IMMDevice> device;
    ComObject<IAudioClient> client;
    ComObject<IAudioCaptureClient> capture;
    WAVEFORMATEX* mixFormat{ nullptr };
    bool floatingPoint{ false };
    bool comInitialisedByUs{ false };
#else
    void hiResTimerCallback() override {}
#endif

    std::atomic<double> sourceSampleRate{ 0.0 };
    std::atomic<int> statusState{ 0 }; // 0 detenido, 1 iniciando, 2 activo, 3 error

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SystemAudioCapture)
};
