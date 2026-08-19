#include "Vst3PluginCatalog.h"

namespace
{
    juce::String normalisedPath(const juce::File& file)
    {
        return file.getFullPathName().trim().replaceCharacter('/', '\\');
    }

    void addUniquePath(juce::StringArray& paths, const juce::File& file)
    {
        const auto path = normalisedPath(file);
        if (path.isEmpty())
            return;

        for (const auto& existing : paths)
            if (existing.equalsIgnoreCase(path))
                return;

        paths.add(path);
    }
}

Vst3PluginCatalog::Vst3PluginCatalog()
    : juce::Thread("Loopernico VST3 scanner")
{
    load();
}

Vst3PluginCatalog::~Vst3PluginCatalog()
{
    signalThreadShouldExit();
    stopThread(5000);
}

juce::StringArray Vst3PluginCatalog::defaultSearchPaths()
{
    juce::StringArray paths;
    addUniquePath(paths, juce::File::getSpecialLocation(juce::File::globalApplicationsDirectory)
                              .getChildFile("Common Files").getChildFile("VST3"));

    const auto programFilesX86 = juce::SystemStats::getEnvironmentVariable("ProgramFiles(x86)", {});
    if (programFilesX86.isNotEmpty())
        addUniquePath(paths, juce::File(programFilesX86).getChildFile("Common Files").getChildFile("VST3"));

    addUniquePath(paths, juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                              .getChildFile("Downloads"));
    addUniquePath(paths, juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                              .getParentDirectory().getChildFile("Plugins"));
    return paths;
}

juce::StringArray Vst3PluginCatalog::getSearchPaths() const
{
    const juce::ScopedLock lock(stateLock);
    return searchPaths;
}

juce::Array<juce::File> Vst3PluginCatalog::getPlugins() const
{
    const juce::ScopedLock lock(stateLock);
    return plugins;
}

juce::String Vst3PluginCatalog::getStatus() const
{
    const juce::ScopedLock lock(stateLock);
    return status;
}

void Vst3PluginCatalog::addSearchPath(const juce::File& directory)
{
    if (!directory.isDirectory())
        return;

    {
        const juce::ScopedLock lock(stateLock);
        addUniquePath(searchPaths, directory);
        status = "Carpeta agregada. Pulsa ESCANEAR para actualizar el catalogo.";
    }
    save();
    sendChangeMessage();
}

void Vst3PluginCatalog::removeSearchPath(int index)
{
    {
        const juce::ScopedLock lock(stateLock);
        if (!juce::isPositiveAndBelow(index, searchPaths.size()))
            return;
        searchPaths.remove(index);
        status = "Carpeta retirada. Pulsa ESCANEAR para actualizar el catalogo.";
    }
    save();
    sendChangeMessage();
}

void Vst3PluginCatalog::restoreDefaultPaths()
{
    {
        const juce::ScopedLock lock(stateLock);
        searchPaths = defaultSearchPaths();
        status = "Rutas habituales restauradas.";
    }
    save();
    sendChangeMessage();
}

void Vst3PluginCatalog::startScan()
{
    if (isThreadRunning())
        return;

    {
        const juce::ScopedLock lock(stateLock);
        status = "Escaneando archivos VST3...";
    }
    scanning.store(true, std::memory_order_release);
    sendSynchronousChangeMessage();
    startThread(juce::Thread::Priority::low);
}

void Vst3PluginCatalog::run()
{
    const auto paths = getSearchPaths();
    juce::StringArray foundPaths;

    std::function<void(const juce::File&, int)> scanDirectory;
    scanDirectory = [this, &foundPaths, &scanDirectory](const juce::File& directory, int depth)
    {
        if (threadShouldExit() || depth > 20 || !directory.isDirectory())
            return;

        for (const auto& entry : juce::RangedDirectoryIterator(
                 directory, false, "*", juce::File::findFilesAndDirectories,
                 juce::File::FollowSymlinks::no))
        {
            if (threadShouldExit())
                return;

            const auto file = entry.getFile();
            if (file.hasFileExtension("vst3"))
            {
                addUniquePath(foundPaths, file);
                continue;
            }

            if (file.isDirectory())
                scanDirectory(file, depth + 1);
        }
    };

    for (const auto& path : paths)
    {
        if (threadShouldExit())
            break;

        const juce::File root(path);
        if (!root.isDirectory())
            continue;

        scanDirectory(root, 0);
    }

    foundPaths.sortNatural();
    juce::Array<juce::File> found;
    for (const auto& path : foundPaths)
        found.add(juce::File(path));

    {
        const juce::ScopedLock lock(stateLock);
        if (!threadShouldExit())
        {
            plugins = std::move(found);
            status = juce::String(plugins.size()) + " plugin"
                   + (plugins.size() == 1 ? " encontrado." : "s encontrados.");
        }
        else
            status = "Escaneo cancelado.";
    }

    scanning.store(false, std::memory_order_release);
    if (!threadShouldExit())
        save();
    sendChangeMessage();
}

juce::File Vst3PluginCatalog::settingsFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Loopernico").getChildFile("vst3-catalog.xml");
}

void Vst3PluginCatalog::load()
{
    const auto file = settingsFile();
    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));

    if (xml != nullptr && xml->hasTagName("LOOPERNICO_VST3_CATALOG"))
    {
        for (auto* child : xml->getChildIterator())
        {
            if (child->hasTagName("PATH"))
                addUniquePath(searchPaths, juce::File(child->getStringAttribute("value")));
            else if (child->hasTagName("PLUGIN"))
            {
                const juce::File plugin(child->getStringAttribute("value"));
                if (plugin.exists())
                    plugins.add(plugin);
            }
        }
    }

    if (searchPaths.isEmpty())
        searchPaths = defaultSearchPaths();
    status = plugins.isEmpty() ? "Pulsa ESCANEAR para buscar plugins VST3."
                               : juce::String(plugins.size()) + " plugins en el catalogo.";
}

void Vst3PluginCatalog::save() const
{
    juce::StringArray pathsCopy;
    juce::Array<juce::File> pluginsCopy;
    {
        const juce::ScopedLock lock(stateLock);
        pathsCopy = searchPaths;
        pluginsCopy = plugins;
    }

    juce::XmlElement xml("LOOPERNICO_VST3_CATALOG");
    for (const auto& path : pathsCopy)
    {
        auto* child = xml.createNewChildElement("PATH");
        child->setAttribute("value", path);
    }
    for (const auto& plugin : pluginsCopy)
    {
        auto* child = xml.createNewChildElement("PLUGIN");
        child->setAttribute("value", plugin.getFullPathName());
    }

    const auto file = settingsFile();
    file.getParentDirectory().createDirectory();
    xml.writeTo(file);
}
