/**
 * \file structs.h
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

#ifndef TRAVAIL_BACHELOR_STRUCTS_H
#define TRAVAIL_BACHELOR_STRUCTS_H

#include <map>
#include <rapidjson/document.h>

#define IN_MODE "mode"
#define IN_UID "uid"
#define IN_GID "gid"
#define IN_SIZE "size"
#define IN_LINKS "linkCount"
#define IN_ACCESS "atime"
#define IN_BLOCKS "dataBlocks"

#define ID_POOL "poolId"
#define ID_VOLUME "volumeId"
#define ID_ID "id"

#define RU_MIGRATION "migration"

#define PO_POOLS "pools"

#define SB_INODE_CACHE "inodeCacheSize"
#define SB_DIR_CACHE "directoryCacheSize"
#define SB_BLOCK_CACHE "blockCacheSize"
#define SB_BLOCK_SIZE_ST "blockSize"
#define SB_REDUNDANCY "redundancy"

#define BI_REFF "referenceId"
#define BI_ACCESS "lastAccess"

namespace mtfs {
	class Rule;

	enum blockType {
		INODE,
		DIR_BLOCK,
		DATA_BLOCK,
		SUPERBLOCK
	};

	typedef struct ruleInfo_st {
		uid_t uid;
		gid_t gid;
		uint64_t lastAccess;

		ruleInfo_st() : ruleInfo_st(0, 0, 0) {}

		ruleInfo_st(uid_t uid, gid_t gid, uint64_t atime) : uid(uid), gid(gid), lastAccess(atime) {}
	} ruleInfo_t;

	typedef struct ident_st {
		std::uint32_t poolId;
		std::uint32_t volumeId;
		std::uint64_t id;

		explicit ident_st(std::uint64_t id = 0, std::uint32_t vid = 0, std::uint32_t pid = 0) : id(id),
																								volumeId(vid),
																								poolId(pid) {}

		~ident_st() = default;

		ident_st &operator=(const ident_st &id) {
			this->poolId = id.poolId;
			this->volumeId = id.volumeId;
			this->id = id.id;

			return *this;
		}

		bool operator==(const ident_st &i) const {
			return (poolId == i.poolId && volumeId == i.volumeId && id == i.id);
		}

		bool operator!=(const ident_st &i) const {
			return (poolId != i.poolId || volumeId != i.volumeId || id != i.id);
		}

		bool operator<(const ident_st &i) const {
			if (poolId < i.poolId)
				return true;

			if (volumeId < i.volumeId)
				return true;

			return id < i.id;
		}

		void toJson(rapidjson::Value &dest, rapidjson::Document::AllocatorType &alloc) const {
			rapidjson::Value v;

			v.SetUint(this->poolId);
			dest.AddMember(rapidjson::StringRef(ID_POOL), v, alloc);

			v.SetUint(this->volumeId);
			dest.AddMember(rapidjson::StringRef(ID_VOLUME), v, alloc);

			v.SetUint64(this->id);
			dest.AddMember(rapidjson::StringRef(ID_ID), v, alloc);
		}

		void fromJson(rapidjson::Value &src) {
			assert(src.IsObject());
			assert(src.HasMember(ID_POOL));
			assert(src.HasMember(ID_VOLUME));
			assert(src.HasMember(ID_ID));

			this->poolId = src[ID_POOL].GetUint();
			this->volumeId = src[ID_VOLUME].GetUint();
			this->id = src[ID_ID].GetUint64();
		}

		std::string toString() {
			return "p:" + std::to_string(this->poolId) + " v:" + std::to_string(this->volumeId) + " i:" +
				   std::to_string(this->id);
		}

	} ident_t;

	typedef struct inode_st {
		mode_t accesRight{0};
		uid_t uid{0};
		gid_t gid{0};
		uint64_t size{0};
		uint32_t linkCount{1};
		std::uint64_t atime;
		std::vector<std::vector<ident_t>> dataBlocks;

		inode_st() : atime((uint64_t) time(nullptr)) {
			this->dataBlocks.clear();
		}

		bool operator==(const inode_st &rhs) const {
			if (accesRight != rhs.accesRight)
				return false;
			if (uid != rhs.uid)
				return false;
			if (gid != rhs.gid)
				return false;
			if (size != rhs.size)
				return false;
			if (linkCount != rhs.linkCount)
				return false;
			if (atime != rhs.atime)
				return false;

			if (dataBlocks.size() != rhs.dataBlocks.size()) {
//				std::cout << "size differ " << dataBlocks.size() << " " << rhs.dataBlocks.size() << std::endl;
				return false;
			}
			for (auto lIter = dataBlocks.begin(), rIter = rhs.dataBlocks.begin();
				 lIter != dataBlocks.end(); lIter++, rIter++) {
				std::vector<ident_t> red = *lIter, rhsRed = *rIter;

				if (red.size() != rhsRed.size())
					return false;
				for (auto lb = red.begin(), rb = rhsRed.begin();
					 lb != red.end(); lb++, rb++) {
					ident_t li = *lb, ri = *rb;

					if (li != ri)
						return false;
				}
			}

			return true;
		}

		void toJson(rapidjson::Document &dest) const {
			dest.SetObject();

			rapidjson::Value v;
			rapidjson::Document::AllocatorType &alloc = dest.GetAllocator();

			v.SetInt(this->accesRight);
			dest.AddMember(rapidjson::StringRef(IN_MODE), v, alloc);

			v.SetInt(this->uid);
			dest.AddMember(rapidjson::StringRef(IN_UID), v, alloc);

			v.SetInt(this->gid);
			dest.AddMember(rapidjson::StringRef(IN_GID), v, alloc);

			v.SetUint64(this->size);
			dest.AddMember(rapidjson::StringRef(IN_SIZE), v, alloc);

			v.SetUint(this->linkCount);
			dest.AddMember(rapidjson::StringRef(IN_LINKS), v, alloc);

			v.SetUint64(this->atime);
			dest.AddMember(rapidjson::StringRef(IN_ACCESS), v, alloc);

			rapidjson::Value a(rapidjson::kArrayType);
			for (auto &&blocks : dataBlocks) {
				rapidjson::Value red(rapidjson::kArrayType);

				for (auto &&block : blocks) {
					rapidjson::Value ident(rapidjson::kObjectType);

					block.toJson(ident, alloc);

					red.PushBack(ident, alloc);
				}

				a.PushBack(red, alloc);
			}
			dest.AddMember(rapidjson::StringRef(IN_BLOCKS), a, alloc);
		}

		void fromJson(rapidjson::Document &d) {
			assert(d.IsObject());
			assert(d.HasMember(IN_MODE));
			assert(d.HasMember(IN_UID));
			assert(d.HasMember(IN_GID));
			assert(d.HasMember(IN_SIZE));
			assert(d.HasMember(IN_LINKS));
			assert(d.HasMember(IN_ACCESS));
			assert(d.HasMember(IN_BLOCKS));

			this->accesRight = (uint16_t) d[IN_MODE].GetUint();
			this->uid = (uint16_t) d[IN_UID].GetUint();
			this->gid = (uint16_t) d[IN_GID].GetUint();
			this->size = d[IN_SIZE].GetUint64();
			this->linkCount = (uint8_t) d[IN_LINKS].GetUint();
			this->atime = d[IN_ACCESS].GetUint64();

			const rapidjson::Value &dataArray = d[IN_BLOCKS];
			assert(dataArray.IsArray());
			this->dataBlocks.clear();
			for (auto &a : dataArray.GetArray()) {
				std::vector<ident_t> redundancy;

				assert(a.IsArray());
				for (auto &v : a.GetArray()) {
					ident_t ident;

					assert(v.IsObject());
					assert(v.HasMember(ID_POOL));
					assert(v.HasMember(ID_VOLUME));
					assert(v.HasMember(ID_ID));

					ident.poolId = (uint16_t) v[ID_POOL].GetUint();
					ident.volumeId = (uint16_t) v[ID_VOLUME].GetUint();
					ident.id = v[ID_ID].GetUint64();

					redundancy.push_back(ident);
				}

				this->dataBlocks.push_back(redundancy);
			}
		}
	} inode_t;

	typedef struct dirBlock_st {
		std::map<std::string, std::vector<mtfs::ident_t>> entries;

		void fromJson(rapidjson::Document &d) {
			assert(d.IsObject());

			for (auto &&item: d.GetObject()) {
				assert(item.value.IsArray());

				std::vector<ident_t> ids;

				for (auto &&ident: item.value.GetArray()) {
					ident_t i;
					i.fromJson(ident);
					ids.push_back(i);
				}
				this->entries.emplace(item.name.GetString(), ids);
			}
		}

		void toJson(rapidjson::Document &d) {
			d.SetObject();

			rapidjson::Document::AllocatorType &allocator = d.GetAllocator();

			for (auto &&item: this->entries) {
				rapidjson::Value r(rapidjson::kArrayType);
				for (auto &&ident: item.second) {
					rapidjson::Value v(rapidjson::kObjectType);
					ident.toJson(v, allocator);
					r.PushBack(v, allocator);
				}
				d.AddMember(rapidjson::StringRef(item.first.c_str()), r, allocator);
			}
		}

	} dirBlock_t;

	typedef struct volume_st {
		std::string pluginName;
		Rule *rule;
		std::map<std::string, std::string> params;
	} volume_t;

	typedef struct pool_st {
		int migration;
		Rule *rule;
		std::map<uint32_t, volume_t> volumes;
	} pool_t;

	typedef struct superblock_st {
		size_t iCacheSz;
		size_t dCacheSz;
		size_t bCacheSz;
		size_t blockSz;
		size_t redundancy;
		int migration;
		std::map<uint32_t, pool_t> pools;
		std::vector<ident_t> rootInodes;
	} superblock_t;

	typedef struct blockInfo_st {
		ident_t id;
		std::vector<ident_t> referenceId;
		uint64_t lastAccess{0};

		bool operator==(const blockInfo_st &other) const {
			if (other.id != this->id)
				return false;

			if (!(other.referenceId == this->referenceId))
				return false;


			return (other.lastAccess == this->lastAccess);
		}

		blockInfo_st() : id(ident_t()) {};

		void toJson(rapidjson::Document &d) {
			d.SetObject();
			rapidjson::Document::AllocatorType &allocator = d.GetAllocator();

			rapidjson::Value v;

			rapidjson::Value a(rapidjson::kArrayType);
			for (auto &&id : this->referenceId) {
				v.SetObject();

				v.AddMember(rapidjson::StringRef(ID_POOL), rapidjson::Value(id.poolId), allocator);
				v.AddMember(rapidjson::StringRef(ID_VOLUME), rapidjson::Value(id.volumeId), allocator);
				v.AddMember(rapidjson::StringRef(ID_ID), rapidjson::Value(id.id), allocator);

				a.PushBack(v, allocator);
			}
			d.AddMember(rapidjson::StringRef(BI_REFF), a, allocator);

			v.SetUint64(this->lastAccess);
			d.AddMember(rapidjson::StringRef(BI_ACCESS), v, allocator);
		}

		void fromJson(rapidjson::Document &d) {
			assert(d.IsObject());
			assert(d.HasMember(BI_REFF));
			assert(d[BI_REFF].IsArray());
			assert(d.HasMember(BI_ACCESS));

			for (auto &&id :d[BI_REFF].GetArray()) {
				assert(id.IsObject());
				assert(id.HasMember(ID_POOL));
				assert(id.HasMember(ID_VOLUME));
				assert(id.HasMember(ID_ID));

				uint32_t poolId = id[ID_POOL].GetUint();
				uint32_t volumeId = id[ID_VOLUME].GetUint();
				uint64_t i = id[ID_ID].GetUint64();

				this->referenceId.push_back(ident_st(i, volumeId, poolId));
			}

			this->lastAccess = d[BI_ACCESS].GetUint64();
		}

	} blockInfo_t;


}  // namespace mtFS

#endif //TRAVAIL_BACHELOR_STRUCTS_H
