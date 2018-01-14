/**
 * \file S3.cpp
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

#include <aws/core/Aws.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <iostream>
#include <fstream>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <sys/stat.h>
#include <zconf.h>
#include <rapidjson/istreamwrapper.h>
#include <mtfs/structs.h>
#include <mtfs/log/Logging.h>

#include "mtfs/s3/S3.h"

using namespace std;
using namespace Aws::S3;
using namespace rapidjson;

namespace plugin {

	S3::S3() : nextFreeInode(1), nextFreeDirectory(0), nextFreeData(0) {
		this->freeDatas.clear();
		this->freeDirectory.clear();
		this->freeInodes.clear();
	}

	std::string S3::getName() {
		return S3::NAME;
	}

	bool S3::attach(std::map<std::string, std::string> params) {
		if (params.end() == params.find("home") || params.end() == params.find("blockSize") ||
			params.end() == params.find("region") || params.end() == params.find("bucket"))
			return false;

		this->blocksize = static_cast<size_t>(stoi(params["blockSize"]));
		this->bucket = params["bucket"].c_str();
		this->home = params["home"] + this->bucket.c_str() + "/";

		this->options = new Aws::SDKOptions();
		Aws::InitAPI(*this->options);

		Aws::Client::ClientConfiguration configuration;
		configuration.region = params["region"].c_str();
		this->s3Client = new S3Client(configuration);

		if (!dirExists(this->home)) {
			if (mkdir(this->home.c_str(), 0700) != 0) {
				Logger::getInstance()->log("S3", "mkdir error " + this->home + " " + strerror(errno), Logger::L_ERROR);
				return false;
			}
		}

		this->initIds();

		return true;
	}

	bool S3::detach() {
		this->writeMetas();

		Aws::ShutdownAPI(*this->options);

		return true;
	}

	int S3::add(uint64_t *id, const mtfs::blockType &type) {

		mutex *mu = nullptr;
		vector<uint64_t> *ve = nullptr;
		uint64_t *nextId;

		switch (type) {
			case mtfs::INODE:
				mu = &this->inodeMutex;
				ve = &this->freeInodes;
				nextId = &this->nextFreeInode;
				break;
			case mtfs::DIR_BLOCK:
				mu = &this->dirBlockMutex;
				ve = &this->freeDirectory;
				nextId = &this->nextFreeDirectory;
				break;
			case mtfs::DATA_BLOCK:
				mu = &this->blockMutex;
				ve = &this->freeDatas;
				nextId = &this->nextFreeData;
				break;
			default:
				return ENOSYS;
		}

		unique_lock<mutex> lk(*mu);
		if (ve->empty()) {
			*id = *nextId;
			(*nextId)++;
		} else {
			*id = (*ve)[0];
			ve->erase(ve->begin());
		}
		lk.unlock();

		return 0;
	}

	int S3::del(const uint64_t &id, const mtfs::blockType &type) {
		mutex *mu = nullptr;
		vector<uint64_t> *ve = nullptr;
		uint64_t *nextId;

		string key;

		switch (type) {
			case mtfs::INODE:
				mu = &this->inodeMutex;
				ve = &this->freeInodes;
				nextId = &this->nextFreeInode;
				key = INODE_PREFIX;
				break;
			case mtfs::DIR_BLOCK:
				mu = &this->dirBlockMutex;
				ve = &this->freeDirectory;
				nextId = &this->nextFreeDirectory;
				key = DIRECTORY_PREFIX;
				break;
			case mtfs::DATA_BLOCK:
				mu = &this->blockMutex;
				ve = &this->freeDatas;
				nextId = &this->nextFreeData;
				key = DATA_PREFIX;
				break;
			default:
				return ENOSYS;
		}

		unique_lock<mutex> lk(*mu);
		if (id == *nextId - 1) {
			(*nextId)--;
		} else {
			ve->push_back(id);
		}
		lk.unlock();

		key += to_string(id);
		this->delObj(key);

		return 0;
	}

	int S3::get(const uint64_t &id, void *data, const mtfs::blockType &type, bool metas) {
		string filename = this->home, key;
		switch (type) {
			case mtfs::INODE:
				filename += "in";
				key += INODE_PREFIX;
				break;
			case mtfs::DIR_BLOCK:
				filename += "di";
				key += DIRECTORY_PREFIX;
				break;
			case mtfs::DATA_BLOCK:
				filename += "da";
				key += DATA_PREFIX;
				break;
			default:
				return ENOSYS;
		}

		if (metas) {
			filename += "me";
			key = "metas/" + key;
		}

		key += to_string(id);

		if (0 != this->dlObj(filename, key))
			return 1;

		if (mtfs::DATA_BLOCK == type) {
			if (metas) {
				ifstream file(filename);
				if (!file.is_open())
					return errno != 0 ? errno : ENOENT;

				IStreamWrapper wrapper(file);

				Document d;
				d.ParseStream(wrapper);

				((mtfs::blockInfo_t *) data)->fromJson(d);
			} else {
				ifstream file(filename);
				if (!file.is_open())
					return errno != 0 ? errno : ENOENT;

				file.read((char *) data, this->blocksize);
				file.close();
			}
		} else {
			ifstream file(filename);
			if (!file.is_open())
				return errno != 0 ? errno : ENOENT;

			IStreamWrapper wrapper(file);

			Document d;
			d.ParseStream(wrapper);

			switch (type) {
				case mtfs::INODE:
					if (metas)
						((mtfs::blockInfo_t *) data)->fromJson(d);
					else
						((mtfs::inode_t *) data)->fromJson(d);
					break;
				case mtfs::DIR_BLOCK:
					if (metas)
						((mtfs::blockInfo_t *) data)->fromJson(d);
					else
						((mtfs::dirBlock_t *) data)->fromJson(d);
					break;
				case mtfs::SUPERBLOCK:
					break;
				default:
					return ENOSYS;
			}

			file.close();
		}

		unlink(filename.c_str());
		return 0;
	}

	int S3::put(const uint64_t &id, const void *data, const mtfs::blockType &type, bool metas) {
		string filename = this->home, key;
		switch (type) {
			case mtfs::INODE:
				filename += "in";
				key += INODE_PREFIX;
				break;
			case mtfs::DIR_BLOCK:
				filename += "di";
				key += DIRECTORY_PREFIX;
				break;
			case mtfs::DATA_BLOCK:
				filename += "da";
				key += DATA_PREFIX;
				break;
			default:
				return ENOSYS;
		}

		if (metas) {
			filename += "me";
			key = "metas/" + key;
		}

		key += to_string(id);

		if (mtfs::DATA_BLOCK == type) {
			if (metas) {
				Document d;

				((mtfs::blockInfo_t *) data)->toJson(d);

				StringBuffer sb;
				PrettyWriter<StringBuffer> pw(sb);
				d.Accept(pw);

				ofstream file(filename);
				if (!file.is_open())
					return errno != 0 ? errno : ENOENT;
				file << sb.GetString() << endl;
				file.close();
			} else {
				ofstream file(filename);
				if (!file.is_open())
					return errno != 0 ? errno : ENOENT;

				file.write((char *) data, this->blocksize);
				file.close();
			}
		} else {
			Document d;

			switch (type) {
				case mtfs::INODE:
					if (metas)
						((mtfs::blockInfo_t *) data)->toJson(d);
					else
						((mtfs::inode_t *) data)->toJson(d);
					break;
				case mtfs::DIR_BLOCK:
					if (metas)
						((mtfs::blockInfo_t *) data)->toJson(d);
					else
						((mtfs::dirBlock_t *) data)->toJson(d);
					break;
				case mtfs::SUPERBLOCK:
					break;
				default:
					return ENOSYS;
			}

			StringBuffer sb;
			PrettyWriter<StringBuffer> pw(sb);
			d.Accept(pw);

			ofstream file(filename);
			if (!file.is_open())
				return errno != 0 ? errno : ENOENT;
			file << sb.GetString() << endl;
			file.close();
		}

		if (0 != this->upObj(filename, key))
			return 1;

		unlink(filename.c_str());
		return 0;
	}

	bool S3::getSuperblock(mtfs::superblock_t &superblock) {
		return false;
	}

	bool S3::putSuperblock(const mtfs::superblock_t &superblock) {
		return false;
	}

	int S3::writeTmpFile(const string &filename, const void *data, const mtfs::blockType &type) {
		switch (type) {
			case mtfs::INODE:
				return this->writeInode(filename, (mtfs::inode_t *) data);
				break;
			case mtfs::DIR_BLOCK:
				return this->writeDirBlock(filename, (mtfs::dirBlock_t *) data);
				break;
			case mtfs::DATA_BLOCK:
				return this->writeDataBlock(filename, (uint8_t *) data);
				break;
			case mtfs::SUPERBLOCK:
				return this->writeSuperblock(filename, (mtfs::superblock_t *) data);
				break;
			default:
				return ENOSYS;
		}
	}

	int S3::writeInode(const std::string &filename, const mtfs::inode_t *inode) {
		Document d;
		d.SetObject();
		Document::AllocatorType &alloc = d.GetAllocator();

		inode->toJson(d);

		StringBuffer sb;
		PrettyWriter<StringBuffer> pw(sb);
		d.Accept(pw);

		ofstream inodeFile(filename);
		inodeFile << sb.GetString() << endl;
		inodeFile.close();

		return this->SUCCESS;
	}

	int S3::writeDirBlock(const std::string &filename, const mtfs::dirBlock_t *dirBlock) {
		Document d;
		d.SetObject();

		Document::AllocatorType &allocator = d.GetAllocator();

		for (auto &&item: dirBlock->entries) {
			Value r(kArrayType);
			for (auto &&ident: item.second) {
				Value v(kObjectType);
				ident.toJson(v, allocator);
				r.PushBack(v, allocator);
			}
			d.AddMember(StringRef(item.first.c_str()), r, allocator);
		}

		StringBuffer sb;
		PrettyWriter<StringBuffer> pw(sb);
		d.Accept(pw);

		ofstream file(filename);
		if (!file.is_open())
			return errno != 0 ? errno : ENOENT;
		file << sb.GetString() << endl;
		file.close();

		return this->SUCCESS;
	}

	int S3::writeDataBlock(const std::string &filename, const uint8_t *datas) {
		ofstream file(filename);
		if (!file.is_open())
			return errno != 0 ? errno : ENOENT;

		file.write((const char *) datas, this->blocksize);
		file.close();
		return this->SUCCESS;
	}

	int S3::writeSuperblock(const std::string &filename, const mtfs::superblock_t *superblock) {
		ofstream sbFile(filename, ios::binary);
		sbFile.write((const char *) superblock, sizeof(*superblock));
		sbFile.close();
	}

	int S3::dlObj(const std::string &filename, const std::string &key) {
		Model::GetObjectRequest objectRequest;
		objectRequest.WithBucket(this->bucket).WithKey(key.c_str());

		auto getObjectOutcome = this->s3Client->GetObject(objectRequest);

		if (getObjectOutcome.IsSuccess()) {
			Aws::OFStream localFile;
			localFile.open(filename, ios_base::out | ios_base::binary);
			localFile << getObjectOutcome.GetResult().GetBody().rdbuf();
			return 0;
		}

		return 1;
	}

	int S3::upObj(const std::string &filename, const std::string &key) {
		Model::PutObjectRequest objectRequest;
		objectRequest.WithBucket(this->bucket).WithKey(key.c_str());

		auto inputData = Aws::MakeShared<Aws::FStream>("PutObjectInputStream",
													   filename.c_str(), std::ios_base::in);

		objectRequest.SetBody(inputData);

		auto putObjectOutcome = this->s3Client->PutObject(objectRequest);

		if (putObjectOutcome.IsSuccess()) {
//		std::cout << "Done!" << std::endl;
			return 0;
		}
		string message = string("PutObject error: ") + putObjectOutcome.GetError().GetExceptionName().c_str() + " " +
						 putObjectOutcome.GetError().GetMessage().c_str();
		Logger::getInstance()->log("S3", message, Logger::L_ERROR);

		return 1;
	}

	void S3::delObj(const string &key) {
		Aws::S3::Model::DeleteObjectRequest objectRequest;
		objectRequest.WithBucket(this->bucket).WithKey(key.c_str());

		auto deleteObjectOutcome = this->s3Client->DeleteObject(objectRequest);

		if (deleteObjectOutcome.IsSuccess()) {
			Logger::getInstance()->log("S3", "Delete Success", Logger::L_INFO);
		} else {
			string message =
					string("PutObject error: ") + deleteObjectOutcome.GetError().GetExceptionName().c_str() + " " +
					deleteObjectOutcome.GetError().GetMessage().c_str();
			Logger::getInstance()->log("S3", message, Logger::L_ERROR);
		}
	}

	bool S3::dirExists(string path) {
		struct stat info{};

		if (stat(path.c_str(), &info) != 0)
			return false;
		return (info.st_mode & S_IFDIR) != 0;
	}

	void S3::initIds() {
		string keyName = "ids.json";
		string filename = this->home + keyName;

		if (0 != dlObj(filename, keyName)) {
			return;
		}

		ifstream inodeFile(filename);
		if (!inodeFile.is_open())
			return;

		IStreamWrapper isw(inodeFile);

		Document d;
		d.ParseStream(isw);

		assert(d.IsObject());

//	Init inode ids
		assert(d.HasMember("inode"));
		Value o = d["inode"].GetObject();
		assert(o.HasMember("next"));
		this->nextFreeInode = o["next"].GetUint64();
		assert(o.HasMember("frees"));
		for (auto &&id : o["frees"].GetArray()) {
			this->freeInodes.push_back(id.GetUint64());
		}

//	Init Dir block ids
		assert(d.HasMember("directory"));
		o = d["directory"].GetObject();
		assert(o.HasMember("next"));
		this->nextFreeDirectory = o["next"].GetUint64();
		assert(o.HasMember("frees"));
		for (auto &&id : o["frees"].GetArray()) {
			this->freeDirectory.push_back(id.GetUint64());
		}

//	Init data Block Ids
		assert(d.HasMember("data"));
		o = d["data"].GetObject();
		assert(o.HasMember("next"));
		this->nextFreeData = o["next"].GetUint64();
		assert(o.HasMember("frees"));
		for (auto &&id : o["frees"].GetArray()) {
			this->freeDatas.push_back(id.GetUint64());
		}


		unlink(filename.c_str());

		Logger::getInstance()->log("S3", "INIT SUCCESS", Logger::L_INFO);
	}

	void S3::writeMetas() {
		Document d;
		d.SetObject();

		Document::AllocatorType &allocator = d.GetAllocator();

		Value o(kObjectType);
		Value a(kArrayType);
		Value v;
		v.SetUint64(this->nextFreeInode);
		o.AddMember(StringRef("next"), v, allocator);
		for (auto &&id :this->freeInodes) {
			v.SetUint64(id);
			a.PushBack(v, allocator);
		}
		o.AddMember(StringRef("frees"), a, allocator);
		d.AddMember(StringRef("inode"), o, allocator);

		o.SetObject();
		a.SetArray();
		v.SetUint64(this->nextFreeDirectory);
		o.AddMember(StringRef("next"), v, allocator);
		for (auto &&id :this->freeDirectory) {
			v.SetUint64(id);
			a.PushBack(v, allocator);
		}
		o.AddMember(StringRef("frees"), a, allocator);
		d.AddMember(StringRef("directory"), o, allocator);

		o.SetObject();
		a.SetArray();
		v.SetUint64(this->nextFreeData);
		o.AddMember(StringRef("next"), v, allocator);
		for (auto &&id :this->freeDatas) {
			v.SetUint64(id);
			a.PushBack(v, allocator);
		}
		o.AddMember(StringRef("frees"), a, allocator);
		d.AddMember(StringRef("data"), o, allocator);

		StringBuffer strBuff;
		PrettyWriter<StringBuffer> writer(strBuff);
		d.Accept(writer);

		string key = "ids.json";
		string filename = this->home + key;
		ofstream file;
		file.open(filename);
		file << strBuff.GetString() << endl;
		file.close();

		this->upObj(filename, key);

		unlink(filename.c_str());
	}
}

extern "C" plugin::Plugin *createObj() {
	return new plugin::S3();
}

extern "C" void destroyObj(plugin::Plugin *plugin) {
	delete plugin;
}

extern "C" plugin::pluginInfo_t getInfo() {
	vector<string> params;
	params.emplace_back("bucket");
	params.emplace_back("region");

	plugin::pluginInfo_t info = {
			.name = plugin::S3::NAME,
			.params = params,
	};

	return info;
}