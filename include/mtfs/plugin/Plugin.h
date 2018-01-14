/**
 * \file Plugin.h
 * \brief
 * \author David Wittwer
 * \version 0.0.1
 * \copyright GNU Publis License V3
 *
 * This file is part of MTFS.

    MTFS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PLUGINSYSTEM_PLUGIN_H
#define PLUGINSYSTEM_PLUGIN_H

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <cassert>

#include "../structs.h"
#include <map>

namespace plugin {
	typedef struct {
		std::string name;
		std::vector<std::string> params;
	} pluginInfo_t;

	class Plugin {
	public:
		static constexpr const char *TYPE = "plName";
		static constexpr const char *PARAMS = "params";

		enum statusCode {
			SUCCESS,
		};

		/**
		 * Get the name of plugin
		 *
		 * @return name of plugin
		 */
		virtual std::string getName()=0;

		virtual bool attach(std::map<std::string, std::string> params)=0;

		virtual bool detach()=0;

		virtual int add(uint64_t *id, const mtfs::blockType &type)=0;

		virtual int del(const uint64_t &id, const mtfs::blockType &type)=0;

		virtual int get(const uint64_t &id, void *data, const mtfs::blockType &type, bool metas)=0;

		virtual int put(const uint64_t &id, const void *data, const mtfs::blockType &type, bool metas)=0;

		virtual bool getSuperblock(mtfs::superblock_t &superblock)=0;

		virtual bool putSuperblock(const mtfs::superblock_t &superblock)=0;

	};

}  // namespace Plugin
#endif
