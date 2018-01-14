/**
 * \file s3Tests.cpp
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

#include <gtest/gtest.h>
#include <mtfs/s3/S3.h>
#include <mtfs/structs.h>
//#include <mtfs/structs.h>

#define BLOCK_SIZE 4096

using namespace std;
using namespace plugin;

#define HOME "/home/david/Cours/4eme/Travail_bachelor/Home/Plugins/"

TEST(S3, attachDetach) {
#ifndef DEBUG
	if (setuid(0) != 0)
	cout << "fail setuid" << endl;
#endif

	S3 device;
	map<string, string> params;
	params.insert(make_pair("home", HOME));
	params.insert(make_pair("blockSize", to_string(BLOCK_SIZE)));
	params.emplace("region", "eu-central-1");
	params.emplace("bucket", "mtfs");
	ASSERT_TRUE(device.attach(params));
	ASSERT_TRUE(device.detach());
}

class S3Fixture : public ::testing::Test {
public:
	S3Fixture() {
		rootIdent.poolId = 0;
		rootIdent.volumeId = 0;
		rootIdent.id = 0;
	}

	void SetUp() override {
		map<string, string> params;
		params.emplace("home", HOME);
		params.emplace("blockSize", to_string(BLOCK_SIZE));
		params.emplace("region", "eu-central-1");
		params.emplace("bucket", "mtfs");
		s3.attach(params);
	}

	void TearDown() override {
		s3.detach();
	}

	~S3Fixture() override = default;

	S3 s3;
	mtfs::ident_t rootIdent;

};

TEST_F(S3Fixture, addInode) {
	uint64_t inode = 0;
	ASSERT_EQ(0, s3.add(&inode, mtfs::blockType::INODE));
	ASSERT_NE(0, inode);

	uint64_t inode2 = 0;
	ASSERT_EQ(0, s3.add(&inode2, mtfs::blockType::INODE));
	ASSERT_NE(0, inode2);
	ASSERT_GT(inode2, inode);

	uint64_t inode3 = 0;
	ASSERT_EQ(0, s3.add(&inode3, mtfs::blockType::INODE));
}

TEST_F(S3Fixture, delInode) {
	uint64_t inode;
	s3.add(&inode, mtfs::blockType::INODE);
	ASSERT_EQ(0, s3.del(inode, mtfs::blockType::INODE));

	uint64_t inode2;
	s3.add(&inode2, mtfs::blockType::INODE);
	s3.add(&inode2, mtfs::blockType::INODE);
	s3.add(&inode2, mtfs::blockType::INODE);
	s3.del(inode, mtfs::blockType::INODE);
}

TEST_F(S3Fixture, writeInode) {
	uint64_t inodeId = 0;
	s3.add(&inodeId, mtfs::blockType::INODE);

	mtfs::ident_t oIdent;
	oIdent.id = 42;
	oIdent.volumeId = 1;
	oIdent.poolId = 1;

	mtfs::inode_t ino;
	ino.accesRight = 0666;
	ino.uid = 0;
	ino.gid = 0;
	ino.size = 1024;
	ino.linkCount = 1;
	ino.atime = (unsigned long &&) time(nullptr);

	for (int i = 0; i < 4; ++i) {
		vector<mtfs::ident_t> blocks;

		for (uint j = 0; j < 3; ++j) {
			mtfs::ident_t ident = mtfs::ident_t(1, j, i * 3 + j);

			blocks.push_back(ident);
		}
		ino.dataBlocks.push_back(blocks);
	}

	ASSERT_EQ(0, s3.put(inodeId, &ino, mtfs::blockType::INODE, false));
}

TEST_F(S3Fixture, readInode) {
	mtfs::inode_t original, inode;
	memset(&original, 0, sizeof(mtfs::inode_t));
	memset(&inode, 0, sizeof(mtfs::inode_t));

	original.accesRight = 0644;
	original.uid = 1;
	original.gid = 1;
	original.size = 1024;
	original.linkCount = 1;
	original.atime = (unsigned long &&) time(nullptr);

	for (int i = 0; i < 4; ++i) {
		vector<mtfs::ident_t> blocks;

		for (uint j = 0; j < 3; ++j) {
			mtfs::ident_t ident = mtfs::ident_t(1, j, i * 3 + j);

			blocks.push_back(ident);
		}
		original.dataBlocks.push_back(blocks);
	}

	uint64_t inodeId;
	s3.add(&inodeId, mtfs::blockType::INODE);
	s3.put(inodeId, &original, mtfs::blockType::INODE, false);
	ASSERT_EQ(0, s3.get(inodeId, &inode, mtfs::blockType::INODE, false));

//	cout << original.accesRight << " " << inode.accesRight << endl;
	ASSERT_EQ(original, inode);
	original.size = 2048;
	s3.put(inodeId, &original, mtfs::blockType::INODE, false);
	ASSERT_EQ(0, s3.get(inodeId, &inode, mtfs::blockType::INODE, false));
}

TEST_F(S3Fixture, addBlock) {
	uint64_t block = 0;
	ASSERT_EQ(0, s3.add(&block, mtfs::blockType::DATA_BLOCK));
//	EXPECT_NE(0, block);

	uint64_t block2 = 0;
	ASSERT_EQ(0, s3.add(&block2, mtfs::blockType::DATA_BLOCK));
	ASSERT_NE(0, block2);
	ASSERT_GT(block2, block);

	uint64_t block3 = 0;
	ASSERT_EQ(0, s3.add(&block3, mtfs::blockType::DATA_BLOCK));
}

TEST_F(S3Fixture, delBlock) {
	uint64_t block;
	s3.add(&block, mtfs::blockType::DATA_BLOCK);
	ASSERT_EQ(0, s3.del(block, mtfs::blockType::DATA_BLOCK));

	uint64_t block2;
	s3.add(&block2, mtfs::blockType::DATA_BLOCK);
	s3.add(&block2, mtfs::blockType::DATA_BLOCK);
	s3.add(&block2, mtfs::blockType::DATA_BLOCK);
	s3.del(block, mtfs::blockType::DATA_BLOCK);
}

TEST_F(S3Fixture, writeBlock) {
	uint8_t block[BLOCK_SIZE];
	memset(block, 0, BLOCK_SIZE);
	for (uint8_t i = 0; i < 50; ++i) {
		block[i] = 'a';
	}

	uint64_t blockId = 0;
	s3.add(&blockId, mtfs::blockType::DATA_BLOCK);
	ASSERT_EQ(0, s3.put(blockId, &block, mtfs::blockType::DATA_BLOCK, false));
}

TEST_F(S3Fixture, readBlock) {
	uint8_t block[BLOCK_SIZE];
	memset(block, 0, BLOCK_SIZE);

	uint8_t readBlock[BLOCK_SIZE];
	memset(readBlock, 0, BLOCK_SIZE);

	for (int i = 0; i < 500; ++i) {
		block[i] = (uint8_t) to_string(i)[0];
	}

	uint64_t blockId = 0;
	s3.add(&blockId, mtfs::blockType::DATA_BLOCK);
	s3.put(blockId, &block, mtfs::blockType::DATA_BLOCK, false);

	ASSERT_EQ(0, s3.get(blockId, &readBlock, mtfs::blockType::DATA_BLOCK, false));
	ASSERT_TRUE(0 == memcmp(block, readBlock, BLOCK_SIZE));
}

TEST_F(S3Fixture, rootInode) {
	mtfs::inode_t inode;
	inode.accesRight = 0444;
	inode.uid = 1000;
	inode.gid = 1000;
	inode.size = 1024;
	inode.linkCount = 2;
	inode.atime = (uint64_t) time(nullptr);
	inode.dataBlocks.clear();

	mtfs::inode_t readInode;

	ASSERT_EQ(0, s3.put(0, &inode, mtfs::blockType::INODE, false));
	ASSERT_EQ(0, s3.get(0, &readInode, mtfs::blockType::INODE, false));
	ASSERT_EQ(inode, readInode);
}

TEST_F(S3Fixture, superblock) {
	mtfs::superblock_t superblock;
	superblock.iCacheSz = superblock.dCacheSz = superblock.bCacheSz = superblock.blockSz = 4096;
	superblock.migration = superblock.redundancy = 1;
	superblock.pools.clear();
	for (int i = 0; i < 5; ++i) {
		mtfs::pool_t pool;
		pool.migration = 0;
		pool.rule = nullptr;
		pool.volumes.clear();
		superblock.pools[i] = pool;
	}


	ASSERT_TRUE(s3.putSuperblock(superblock));
}

TEST_F(S3Fixture, putMetas) {
	mtfs::blockInfo_t blockInfo;

	mtfs::ident_t ident1(1, 1, 1);
	mtfs::ident_t ident2(2, 2, 2);

	blockInfo.referenceId.push_back(ident1);
	blockInfo.referenceId.push_back(ident2);
	blockInfo.lastAccess = (uint64_t) time(nullptr);

	ASSERT_EQ(0, s3.put(1, &blockInfo, mtfs::blockType::INODE, true));
	ASSERT_EQ(0, s3.put(1, &blockInfo, mtfs::blockType::DIR_BLOCK, true));
	ASSERT_EQ(0, s3.put(1, &blockInfo, mtfs::blockType::DATA_BLOCK, true));
}

//TEST_F(S3Fixture, getMetas) {
//	mtfs::blockInfo_t blockInfo = mtfs::blockInfo_t(), receiveInfo = mtfs::blockInfo_t();
////	memset(&blockInfo, 0, sizeof(mtfs::blockInfo_t));
////	memset(&receiveInfo, 0, sizeof(mtfs::blockInfo_t));
//
//	mtfs::ident_t ident1(1, 1, 1);
//	mtfs::ident_t ident2(2, 2, 2);
//
//	blockInfo.referenceId.push_back(ident1);
//	blockInfo.referenceId.push_back(ident2);
//	blockInfo.lastAccess = (uint64_t) time(nullptr);
//
//	ASSERT_EQ(0, s3.put(2, &blockInfo, mtfs::blockType::INODE, true));
//	ASSERT_EQ(0, s3.get(2, &receiveInfo, mtfs::blockType::INODE, true));
//	ASSERT_EQ(blockInfo, receiveInfo);
//
//	receiveInfo = mtfs::blockInfo_t();
//	ASSERT_EQ(0, s3.put(2, &blockInfo, mtfs::blockType::DIR_BLOCK, true));
//	ASSERT_EQ(0, s3.get(2, &receiveInfo, mtfs::blockType::DIR_BLOCK, true));
//	ASSERT_EQ(blockInfo, receiveInfo);
//
//	receiveInfo = mtfs::blockInfo_t();
//	ASSERT_EQ(0, s3.put(2, &blockInfo, mtfs::blockType::DATA_BLOCK, true));
//	ASSERT_EQ(0, s3.get(2, &receiveInfo, mtfs::blockType::DATA_BLOCK, true));
//	ASSERT_EQ(blockInfo, receiveInfo);
//}

TEST_F(S3Fixture, putDirBlock) {
	mtfs::ident_t id1, id2;
	id2.poolId = id1.poolId = 1;
	id1.volumeId = 1;
	id1.id = 2;
	id2.volumeId = 2;
	id2.id = 42;

	vector<mtfs::ident_t> ids;
	ids.push_back(id1);
	ids.push_back(id2);

	mtfs::dirBlock_t block = mtfs::dirBlock_t();
	block.entries.clear();
	block.entries.insert(make_pair("baz", ids));
	block.entries.insert(make_pair("test", ids));

	uint64_t dId = 0;

	ASSERT_EQ(0, s3.add(&dId, mtfs::blockType::DIR_BLOCK));
	EXPECT_LE(0, dId);
	ASSERT_EQ(0, s3.put(dId, &block, mtfs::blockType::DIR_BLOCK, false));

	mtfs::dirBlock_t readBlock;
	ASSERT_EQ(0, s3.get(dId, &readBlock, mtfs::blockType::DIR_BLOCK, false));
	ASSERT_EQ(block.entries.size(), readBlock.entries.size());
	for (auto &&item: block.entries) {
		ASSERT_NE(readBlock.entries.end(), readBlock.entries.find(item.first));
		int i = 0;
		for (auto &&ident: item.second) {
			ASSERT_EQ(ident, readBlock.entries[item.first][i]);
			i++;
		}
	}

	ASSERT_EQ(0, s3.del(dId, mtfs::blockType::DIR_BLOCK));
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}