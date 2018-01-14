/**
 * \file Logger.h
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

#ifndef MTFS_LOGGER_H
#define MTFS_LOGGER_H

#include <string>
#include <ostream>

class Logger {
public:
	enum level {
		L_ERROR,
		L_WARNING,
		L_INFO,
		L_DEBUG,
	};

	static void init(std::ostream &logStream, const level dLevel = L_ERROR);

	static Logger *getInstance();

	void log(const std::string &key, const std::string &message, const level &level);

private:
	static Logger *instance;

	const level displayLevel;
	std::ostream &outStream;

	Logger(const level dLevel, std::ostream &logStream);

	std::string levelString(const level &l);
};


#endif //MTFS_LOGGER_H
