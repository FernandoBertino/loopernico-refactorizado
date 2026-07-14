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
            setContentOwned (new MainComponent(), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen (true);
           #else
            setResizable (true, true);
            centreWithSize (getWidth(), getHeight());
           #endif

            setVisible (true);

           #if ! JUCE_IOS && ! JUCE_ANDROID
            // With JUCE's native Windows peer this maps to SW_SHOWMAXIMIZED: it uses
            // the desktop work area while retaining the title bar, taskbar and normal
            // window controls. Snapshot mode keeps its fixed visual-reference size.
            if (! snapshotRequest.enabled)
                setFullScreen (true);
           #endif

            if (snapshotRequest.enabled)
                scheduleUiSnapshot(snapshotRequest.outputPath, true, 1500);
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
