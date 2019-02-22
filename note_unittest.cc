#include "gtest/gtest.h"
#include "note.h"

TEST(NoteTest, OneIsOne) {
  EXPECT_EQ(1, 1);
}

TEST(NoteTest, CreateFromStringAndDestroy) {
  string name = "foo";
  Note* note = new Note(name);
  EXPECT_STREQ(name.c_str(), note->GetText().c_str());
  delete note;
}
	
TEST(NoteTest, SerializeAndUnSerializeMatch) {
  string name = "foo";
  Note* note = new Note(name);
  // serialize it
  // unserialze it
  EXPECT_STREQ(name.c_str(), note->GetText().c_str());
  delete note;
}
