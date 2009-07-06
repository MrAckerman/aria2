#include "BtBitfieldMessage.h"

#include <cstring>

#include <cppunit/extensions/HelperMacros.h>

#include "PeerMessageUtil.h"
#include "Util.h"
#include "Peer.h"
#include "MockPieceStorage.h"
#include "DlAbortEx.h"

namespace aria2 {

class BtBitfieldMessageTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(BtBitfieldMessageTest);
  CPPUNIT_TEST(testCreate);
  CPPUNIT_TEST(testGetMessage);
  CPPUNIT_TEST(testDoReceivedAction);
  CPPUNIT_TEST(testDoReceivedAction_goodByeSeeder);
  CPPUNIT_TEST(testToString);
  CPPUNIT_TEST_SUITE_END();
private:

public:
  void setUp() {
  }

  void testCreate();
  void testGetMessage();
  void testDoReceivedAction();
  void testDoReceivedAction_goodByeSeeder();
  void testToString();
};


CPPUNIT_TEST_SUITE_REGISTRATION(BtBitfieldMessageTest);

void BtBitfieldMessageTest::testCreate() {
  unsigned char msg[5+2];
  PeerMessageUtil::createPeerMessageString(msg, sizeof(msg), 3, 5);
  unsigned char bitfield[2];
  memset(bitfield, 0xff, sizeof(bitfield));
  memcpy(&msg[5], bitfield, sizeof(bitfield));
  SharedHandle<BtBitfieldMessage> pm = BtBitfieldMessage::create(&msg[4], 3);
  CPPUNIT_ASSERT_EQUAL((uint8_t)5, pm->getId());
  CPPUNIT_ASSERT(memcmp(bitfield, pm->getBitfield(), sizeof(bitfield)) == 0);
  CPPUNIT_ASSERT_EQUAL((size_t)2, pm->getBitfieldLength());
  // case: payload size is wrong
  try {
    unsigned char msg[5];
    PeerMessageUtil::createPeerMessageString(msg, sizeof(msg), 1, 5);
    BtBitfieldMessage::create(&msg[4], 1);
    CPPUNIT_FAIL("exception must be thrown.");
  } catch(...) {
  }
  // case: id is wrong
  try {
    unsigned char msg[5+2];
    PeerMessageUtil::createPeerMessageString(msg, sizeof(msg), 3, 6);
    BtBitfieldMessage::create(&msg[4], 3);
    CPPUNIT_FAIL("exception must be thrown.");
  } catch(...) {
  }
}

void BtBitfieldMessageTest::testGetMessage() {
  BtBitfieldMessage msg;
  unsigned char bitfield[2];
  memset(bitfield, 0xff, sizeof(bitfield));
  msg.setBitfield(bitfield, sizeof(bitfield));
  unsigned char data[5+2];
  PeerMessageUtil::createPeerMessageString(data, sizeof(data), 3, 5);
  memcpy(&data[5], bitfield, sizeof(bitfield));
  CPPUNIT_ASSERT(memcmp(msg.getMessage(), data, 7) == 0);
}

void BtBitfieldMessageTest::testDoReceivedAction() {
  SharedHandle<Peer> peer(new Peer("host1", 6969));
  peer->allocateSessionResource(16*1024, 16*16*1024);
  BtBitfieldMessage msg;
  msg.setPeer(peer);
  SharedHandle<MockPieceStorage> pieceStorage(new MockPieceStorage());
  msg.setPieceStorage(pieceStorage);
  unsigned char bitfield[] = { 0xff, 0xff };
  msg.setBitfield(bitfield, sizeof(bitfield));
  
  CPPUNIT_ASSERT_EQUAL(std::string("0000"), Util::toHex(peer->getBitfield(),
							peer->getBitfieldLength()));
  msg.doReceivedAction();
  CPPUNIT_ASSERT_EQUAL(std::string("ffff"), Util::toHex(peer->getBitfield(),
							peer->getBitfieldLength()));
}

void BtBitfieldMessageTest::testDoReceivedAction_goodByeSeeder()
{
  SharedHandle<Peer> peer(new Peer("ip", 6000));
  peer->allocateSessionResource(1024, 1024);
  BtBitfieldMessage msg;
  msg.setPeer(peer);
  SharedHandle<MockPieceStorage> pieceStorage(new MockPieceStorage());
  msg.setPieceStorage(pieceStorage);
  unsigned char bitfield[] = { 0x00 };
  msg.setBitfield(bitfield, sizeof(bitfield));

  // peer is not seeder and client have not completed download
  msg.doReceivedAction();

  pieceStorage->setDownloadFinished(true);

  // client completed download, but peer is not seeder
  msg.doReceivedAction();

  pieceStorage->setDownloadFinished(false);
  bitfield[0] = 0x80;
  msg.setBitfield(bitfield, sizeof(bitfield));

  // peer is seeder, but client have not completed download
  msg.doReceivedAction();

  pieceStorage->setDownloadFinished(true);
  try {
    msg.doReceivedAction();
    CPPUNIT_FAIL("exception must be thrown.");
  } catch(DlAbortEx& e) {
    // success
  }
}

void BtBitfieldMessageTest::testToString() {
  BtBitfieldMessage msg;
  unsigned char bitfield[] = { 0xff, 0xff };
  msg.setBitfield(bitfield, sizeof(bitfield));

  CPPUNIT_ASSERT_EQUAL(std::string("bitfield ffff"), msg.toString());
}

} // namespace aria2
