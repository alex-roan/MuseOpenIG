//#******************************************************************************
//#*
//#*      Copyright (C) 2015  Compro Computer Services
//#*      http://openig.compro.net
//#*
//#*      Source available at: https://github.com/CCSI-CSSI/MuseOpenIG
//#*
//#*      This software is released under the LGPL.
//#*
//#*   This software is free software; you can redistribute it and/or modify
//#*   it under the terms of the GNU Lesser General Public License as published
//#*   by the Free Software Foundation; either version 2.1 of the License, or
//#*   (at your option) any later version.
//#*
//#*   This software is distributed in the hope that it will be useful,
//#*   but WITHOUT ANY WARRANTY; without even the implied warranty of
//#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
//#*   the GNU Lesser General Public License for more details.
//#*
//#*   You should have received a copy of the GNU Lesser General Public License
//#*   along with this library; if not, write to the Free Software
//#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//#*
//#*    Please direct any questions or comments to the OpenIG Forums
//#*    Email address: openig@compro.net
//#*
//#*
//#*    Please direct any questions or comments to the OpenIG Forums
//#*    Email address: openig@compro.net
//#*
//#*
//#*****************************************************************************

#include "PluginHost.h"

#include <boost/filesystem.hpp>

#include <osgDB/FileNameUtils>
#include <osgDB/DynamicLibrary>
#include <osgDB/XmlParser>

using namespace OpenIG::PluginBase;

typedef Plugin* (createPluginFunction)();
typedef void (deletePluginFunction)(Plugin*);

PluginHost::PluginHost()
{
}

PluginHost::~PluginHost()
{
    unloadPlugins();
}

void PluginHost::loadPlugins(const std::string& path, const std::string& configFileName)
{
    typedef std::map< std::string, int >            PluginOrder;
    typedef std::map< std::string, int >::iterator  PluginOrderIterator;

    PluginOrder pluginOrder;

    if (!configFileName.empty())
    {

        osg::ref_ptr<osgDB::XmlNode> root = osgDB::readXmlFile(configFileName);
        if (root.valid() && root->children.size())
        {
			root = root->children.at(0);
			if (!root.valid()) return;

            for (size_t i = 0; i<root->children.size(); ++i)
            {
                osg::ref_ptr<osgDB::XmlNode> config = root->children.at(i);
                if (config->name != "ImageGenerator-Plugins-Config") continue;

                osgDB::XmlNode::Children::iterator itr = config->children.begin();
                for ( ; itr != config->children.end(); ++itr)
                {
                    osg::ref_ptr<osgDB::XmlNode> child = *itr;
                    if (child->name != "Plugin") continue;
                    if (child->children.size() != 2) continue;

                    osg::ref_ptr<osgDB::XmlNode> orderNumber = child->children.at(0);
                    osg::ref_ptr<osgDB::XmlNode> name = child->children.at(1);

                    if (!orderNumber.valid() || orderNumber->name != "Order-Number") continue;
                    if (!name.valid() || name->name != "Name") continue;

                    pluginOrder[name->contents] = atoi(orderNumber->contents.c_str());

                }
            }
        }
    }

    boost::filesystem::path dir(path);

    std::vector<boost::filesystem::path> plugins;

    if ( boost::filesystem::exists(dir) && boost::filesystem::is_directory(dir))
    {
        boost::filesystem::directory_iterator dirIter(dir);
        boost::filesystem::directory_iterator endIter;

        for( ; dirIter != endIter ; ++dirIter)
        {
            if (boost::filesystem::is_regular_file(dirIter->status() ) &&
               isPlugin(dirIter->path().filename().string()))
            {
                plugins.push_back(*dirIter);
            }
        }
    }

    std::vector<boost::filesystem::path>::iterator itr = plugins.begin();
    for ( ; itr != plugins.end(); ++itr )
    {
        std::string pluginFileName = itr->string();

        osg::ref_ptr<osgDB::DynamicLibrary> pluginLibrary = osgDB::DynamicLibrary::loadLibrary(pluginFileName);
        if (pluginLibrary.valid())
        {

            osgDB::DynamicLibrary::PROC_ADDRESS proc = pluginLibrary->getProcAddress("CreatePlugin");
            if (proc)
            {
                createPluginFunction* fn = (createPluginFunction*)proc;
                osg::ref_ptr<Plugin> plugin = fn();
                if (plugin.valid())
                {
                    PluginOrderIterator pitr = pluginOrder.find(plugin->getName());
                    if (pitr == pluginOrder.end()) continue;

                    _plugins[pitr->second] = plugin;
                    _pluginLibraries[plugin->getName()] = pluginLibrary;

                    plugin->setLibrary(pluginFileName);
                    plugin->setOrderNumber(pitr->second);
                }
            }
        }
		else
			osg::notify(osg::ALWAYS) << "PluginCore: -- FAILED to load plugin: " << pluginFileName << "!!!!!!!!" << std::endl;
    }
}
void PluginHost::unloadPlugins()
{
    PluginLibrariesMapIterator itr = _pluginLibraries.begin();
    while ( itr != _pluginLibraries.end() )
    {
        std::string pluginName = itr->first;
        osgDB::DynamicLibrary* pluginLibrary = itr->second;

        osgDB::DynamicLibrary::PROC_ADDRESS proc = pluginLibrary->getProcAddress("DeletePlugin");
        if (proc)
        {
            deletePluginFunction* fn = (deletePluginFunction*)proc;

            Plugin* plugin = 0;
            PluginsMapIterator pitr = _plugins.begin();
            for ( ; pitr != _plugins.end(); ++pitr)
            {
                if (pitr->second.valid() && pitr->second->getName()==pluginName)
                {
                    plugin = pitr->second.release();
                    break;
                }
            }

            if (pitr != _plugins.end() && plugin)
            {
                osg::notify(osg::NOTICE) << "PluginCore: Unloading plugin: " << plugin->getName() << std::endl;

                _plugins.erase(pitr);
                fn(plugin);

                _pluginLibraries.erase(itr);
                itr = _pluginLibraries.begin();
                continue;
            }
        }
        ++itr;
    }

    _pluginLibraries.clear();
    _plugins.clear();
}

bool PluginHost::isPlugin(const std::string& fileName) const
{
#if     defined (__linux)
    if (osgDB::getFileExtension(fileName).compare(0,2,"so")==0 && fileName.substr(0,16).compare(0,16,"libOpenIG-Plugin")==0)
        return true;
    else
        return false;
#elif   defined (_WIN32)
    if (osgDB::getFileExtension(fileName).compare(0,3,"dll")==0 && fileName.substr(0,13).compare(0,13,"OpenIG-Plugin")==0)
        return true;
    else
        return false;
#elif   defined (__APPLE__)
    if (osgDB::getFileExtension(fileName).compare(0,5,"dylib")==0 && fileName.substr(0,16).compare(0,16,"libOpenIG-Plugin")==0)
        return true;
    else
        return false;
#endif
    return false;
}

void PluginHost::applyPluginOperation(OpenIG::PluginBase::PluginOperation* operation)
{
    if (!operation)
        return;

    PluginsMapIterator itr = _plugins.begin();
    for ( ; itr != _plugins.end(); ++itr )
    {
        if (itr->second.valid())
        {
            operation->apply(itr->second.get());
        }
    }
}
