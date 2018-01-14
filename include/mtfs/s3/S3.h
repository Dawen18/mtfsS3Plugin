/**
 * \file S3.h
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

#ifndef MTFS_S3_H
#define MTFS_S3_H

#include "../plugin/Plugin.h"
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>

namespace plugin {
	class S3 : public Plugin {
	public:

		static constexpr const char *NAME = "s3";

		S3();

		std::string getName() override;

		bool attach(std::map<std::string, std::string> params) override;

		bool detach() override;

		int add(uint64_t *id, const mtfs::blockType &type) override;

		int del(const uint64_t &id, const mtfs::blockType &type) override;

		int get(const uint64_t &id, void *data, const mtfs::blockType &type, bool metas) override;

		int put(const uint64_t &id, const void *data, const mtfs::blockType &type, bool metas) override;

		bool getSuperblock(mtfs::superblock_t &superblock) override;

		bool putSuperblock(const mtfs::superblock_t &superblock) override;

	private:
		static constexpr const char *INODE_PREFIX = "inodes/";
		static constexpr const char *DIRECTORY_PREFIX = "directories/";
		static constexpr const char *DATA_PREFIX = "datas/";

		Aws::S3::S3Client *s3Client;
		Aws::SDKOptions *options;
		std::string home;
		size_t blocksize;
		Aws::String bucket;

		std::mutex inodeMutex;
		std::vector<uint64_t> freeInodes;
		uint64_t nextFreeInode;

		std::mutex dirBlockMutex;
		std::vector<uint64_t> freeDirectory;
		uint64_t nextFreeDirectory;

		std::mutex blockMutex;
		std::vector<uint64_t> freeDatas;
		uint64_t nextFreeData;

		int writeTmpFile(const std::string &filename, const void *data, const mtfs::blockType &type);

		int writeInode(const std::string &filename, const mtfs::inode_t *inode);

		int writeDirBlock(const std::string &filename, const mtfs::dirBlock_t *dirBlock);

		int writeDataBlock(const std::string &filename, const uint8_t *datas);

		int writeSuperblock(const std::string &filename, const mtfs::superblock_t *superblock);

		int dlObj(const std::string &filename, const std::string &key);

		int upObj(const std::string &filename, const std::string &key);

		bool dirExists(std::string path);

		void initIds();

		void writeMetas();

		void delObj(const std::string &key);
	};
}

#endif //MTFS_S3_H
