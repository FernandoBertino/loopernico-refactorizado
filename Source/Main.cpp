/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "MainComponent.h"

namespace
{
    struct SnapshotLaunchRequest
    {
        bool enabled = false;
        juce::String outputPath;
        int focusPedal = -2;
        int delayMs = 1500;
        juce::String vstDisplayText;
        juce::File sessionFile;
        bool showAudioSettings = false;
        bool forceAudioFallback = false;
        int audioBufferSamples = 0;
        int softwareInputMonitoring = -1;
        juce::String vst3Path;
        bool vst3Bypass = false;
        juce::String saveSessionPath;
    };

    SnapshotLaunchRequest parseSnapshotLaunchRequest(const juce::String& commandLine)
    {
        SnapshotLaunchRequest request;
        juce::StringArray arguments;
        arguments.addTokens(commandLine, true);

        for (const auto& rawArgument : arguments)
        {
            const auto argument = rawArgument.unquoted();
            if (argument.equalsIgnoreCase("--snapshot-ui"))
            {
                request.enabled = true;
            }
            else if (argument.startsWithIgnoreCase("--snapshot-ui="))
            {
                request.enabled = true;
                request.outputPath = argument.fromFirstOccurrenceOf("=", false, false).unquoted();
            }
            else if (argument.startsWithIgnoreCase("--snapshot-fx="))
            {
                request.focusPedal = argument.fromFirstOccurrenceOf("=", false, false).getIntValue();
            }
            else if (argument.startsWithIgnoreCase("--snapshot-delay="))
            {
                request.delayMs = juce::jlimit(100, 10000,
                    argument.fromFirstOccurrenceOf("=", false, false).getIntValue());
            }
            else if (argument.startsWithIgnoreCase("--snapshot-vst-text="))
            {
                request.vstDisplayText = argument.fromFirstOccurrenceOf("=", false, false).unquoted();
            }
            else if (argument.equalsIgnoreCase("--snapshot-audio"))
            {
                request.enabled = true;
                request.showAudioSettings = true;
            }
            else if (argument.equalsIgnoreCase("--snapshot-force-fallback"))
            {
                request.forceAudioFallback = true;
            }
            else if (argument.startsWithIgnoreCase("--snapshot-audio-buffer="))
            {
                request.audioBufferSamples = argument.fromFirstOccurrenceOf("=", false, false).getIntValue();
            }
            else if (argument.startsWithIgnoreCase("--snapshot-monitor-input="))
            {
                const auto value = argument.fromFirstOccurrenceOf("=", false, false);
                request.softwareInputMonitoring = value.equalsIgnoreCase("off") ? 0 : 1;
            }
            else if (argument.startsWithIgnoreCase("--snapshot-vst3="))
            {
                request.vst3Path = argument.fromFirstOccurrenceOf("=", false, false).unquoted();
            }
            else if (argument.equalsIgnoreCase("--snapshot-vst3-bypass"))
            {
                request.vst3Bypass = true;
            }
            else if (argument.startsWithIgnoreCase("--snapshot-save-session="))
            {
                request.saveSessionPath = argument.fromFirstOccurrenceOf("=", false, false).unquoted();
            }
            else if (argument.endsWithIgnoreCase(".loopernico"))
            {
                request.sessionFile = juce::File::isAbsolutePath(argument)
                    ? juce::File(argument)
                    : juce::File::getCurrentWorkingDirectory().getChildFile(argument);
            }
        }

        return request;
    }
}

//==============================================================================
class LoopernicoRefactorizadoApplication  : public juce::JUCEApplication
{
public:
    //==============================================================================
    LoopernicoRefactorizadoApplication() {}

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    //==============================================================================
    void initialise (const juce::String& commandLine) override
    {
        // This method is where you should put your application's initialisation code..

        if (commandLine.containsIgnoreCase("--run-latency-tests"))
        {
            juce::String report;
            bool passed = LoopernicoLatency::runDeterministicTests(report);
            juce::String outputPath;
            juce::StringArray arguments;
            arguments.addTokens(commandLine, true);
            for (const auto& rawArgument : arguments)
                if (rawArgument.unquoted().startsWithIgnoreCase("--run-latency-tests="))
                    outputPath = rawArgument.unquoted().fromFirstOccurrenceOf("=", false, false).unquoted();

            if (outputPath.isNotEmpty())
            {
                const juce::File reportFile(outputPath);
                reportFile.getParentDirectory().createDirectory();
                if (!reportFile.replaceWithText(report + "\n"))
                {
                    juce::Logger::writeToLog("No se pudo escribir el reporte de pruebas en: "
                                             + reportFile.getFullPathName());
                    passed = false;
                }
            }

            setApplicationReturnValue(passed ? 0 : 1);
            quit();
            return;
        }

        const auto snapshotRequest = parseSnapshotLaunchRequest(commandLine);
        mainWindow.reset (new MainWindow (getApplicationName(), snapshotRequest));
    }

    void shutdown() override
    {
        // Add your application's shutdown code here..

        mainWindow = nullptr; // (deletes our window)
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
    }

    //==============================================================================
    /*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
    class MainWindow    : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name, const SnapshotLaunchRequest& snapshotRequest)
            : DocumentWindow (name,
                              juce::Desktop::getInstance().getDefaultLookAndFeel()
                                                          .findColour (juce::ResizableWindow::backgroundColourId),
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(snapshotRequest.forceAudioFallback), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen (true);
           #else
            setResizable (true, true);
            centreWithSize (getWidth(), getHeight());
           #endif

            setVisible (true);

            if (auto* main = dynamic_cast<MainComponent*> (getContentComponent()))
            {
                if (snapshotRequest.audioBufferSamples > 0)
                    main->setAudioBufferForSnapshot(snapshotRequest.audioBufferSamples);
                if (snapshotRequest.softwareInputMonitoring >= 0)
                    main->setSoftwareInputMonitoringForSnapshot(snapshotRequest.softwareInputMonitoring != 0);
                if (snapshotRequest.vst3Path.isNotEmpty())
                    main->loadExternalVst3ForSnapshot(juce::File(snapshotRequest.vst3Path),
                                                      !snapshotRequest.vst3Bypass);
            }

            if (snapshotRequest.sessionFile != juce::File())
            {
                juce::Timer::callAfterDelay(250,
                    [safeWindow = juce::Component::SafePointer<MainWindow>(this),
                     sessionFile = snapshotRequest.sessionFile]
                    {
                        if (auto* window = safeWindow.getComponent())
                            if (auto* main = dynamic_cast<MainComponent*> (window->getContentComponent()))
                                main->openSessionFile(sessionFile);
                    });
            }

            if (snapshotRequest.saveSessionPath.isNotEmpty())
            {
                juce::Timer::callAfterDelay(1250,
                    [safeWindow = juce::Component::SafePointer<MainWindow>(this),
                     outputFile = juce::File(snapshotRequest.saveSessionPath)]
                    {
                        if (auto* window = safeWindow.getComponent())
                            if (auto* main = dynamic_cast<MainComponent*> (window->getContentComponent()))
                            {
                                juce::String error;
                                main->saveSessionFileForSnapshot(outputFile, error);
                            }
                    });
            }

            if (snapshotRequest.enabled && snapshotRequest.focusPedal >= -1)
                if (auto* main = dynamic_cast<MainComponent*> (getContentComponent()))
                    main->focusFxPedalForSnapshot(snapshotRequest.focusPedal);

            if (snapshotRequest.enabled && snapshotRequest.vstDisplayText.isNotEmpty())
                if (auto* main = dynamic_cast<MainComponent*> (getContentComponent()))
                    main->setVst3DisplayTextForSnapshot(snapshotRequest.vstDisplayText);

            if (snapshotRequest.enabled && snapshotRequest.showAudioSettings)
                if (auto* main = dynamic_cast<MainComponent*> (getContentComponent()))
                    main->showAudioSettingsForSnapshot();

           #if ! JUCE_IOS && ! JUCE_ANDROID
            // With JUCE's native Windows peer this maps to SW_SHOWMAXIMIZED: it uses
            // the desktop work area while retaining the title bar, taskbar and normal
            // window controls. Snapshot mode keeps its fixed visual-reference size.
            if (! snapshotRequest.enabled)
                setFullScreen (true);
           #endif

            if (snapshotRequest.enabled)
                scheduleUiSnapshot(snapshotRequest.outputPath, true, snapshotRequest.delayMs);
        }

        void closeButtonPressed() override
        {
            // This is called when the user tries to close this window. Here, we'll just
            // ask the app to quit when this happens, but you can change this to do
            // whatever you need.
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        bool keyPressed(const juce::KeyPress& key) override
        {
            if (key.getKeyCode() == juce::KeyPress::F12Key)
            {
                scheduleUiSnapshot({}, false, 100);
                return true;
            }

            return DocumentWindow::keyPressed(key);
        }

        /* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */

    private:
        static juce::String snapshotFileName()
        {
            const auto now = juce::Time::getCurrentTime();
            return "loopernico-ui-" + now.formatted("%Y%m%d-%H%M%S")
                 + "-" + juce::String(now.toMilliseconds()) + ".png";
        }

        static juce::File resolveSnapshotFile(juce::String requestedPath)
        {
            requestedPath = requestedPath.trim().unquoted();
            if (requestedPath.isEmpty())
            {
                return juce::File::getSpecialLocation(juce::File::userPicturesDirectory)
                    .getChildFile("Loopernico")
                    .getChildFile(snapshotFileName());
            }

            auto output = juce::File::isAbsolutePath(requestedPath)
                ? juce::File(requestedPath)
                : juce::File::getCurrentWorkingDirectory().getChildFile(requestedPath);

            if (output.isDirectory())
                output = output.getChildFile(snapshotFileName());
            else if (!output.hasFileExtension("png"))
                output = output.withFileExtension("png");

            return output;
        }

        bool writeUiSnapshot(const juce::String& requestedPath)
        {
            auto* content = getContentComponent();
            if (content == nullptr)
                return false;

            const auto output = resolveSnapshotFile(requestedPath);
            if (output.getParentDirectory().createDirectory().failed())
                return false;

            output.deleteFile();
            juce::FileOutputStream stream(output);
            if (!stream.openedOk())
                return false;

            const auto image = content->createComponentSnapshot(content->getLocalBounds());
            juce::PNGImageFormat png;
            const bool written = png.writeImageToStream(image, stream);
            stream.flush();

            if (written)
                juce::Logger::writeToLog("Captura de interfaz guardada en: " + output.getFullPathName());

            return written;
        }

        void scheduleUiSnapshot(const juce::String& requestedPath, bool quitAfterCapture, int delayMs)
        {
            juce::Timer::callAfterDelay(delayMs,
                [safeWindow = juce::Component::SafePointer<MainWindow>(this), requestedPath, quitAfterCapture]
                {
                    if (auto* window = safeWindow.getComponent())
                        window->writeUiSnapshot(requestedPath);

                    if (quitAfterCapture)
                        if (auto* app = juce::JUCEApplication::getInstance())
                            app->systemRequestedQuit();
                });
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (LoopernicoRefactorizadoApplication)
