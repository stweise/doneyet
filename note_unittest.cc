#include "gtest/gtest.h"
#include "note.h"
#include "serializer.h"

TEST(NoteTest, OneIsOne) {
  EXPECT_EQ(1, 1);
}

TEST(NoteTest, CreateFromStringAndDestroy) {
  string name = "foo";
  Note* note = new Note(name);
  EXPECT_STREQ(name.c_str(), note->GetText().c_str());
  delete note;
}
	
TEST(NoteTest, SerializedAndUnSerializedNoteMatchInText) {
  string name = "foo2";
  Note* note = new Note(name);
  // serialize it
  string serializedFilename = "/tmp/NoteTest.project";
  Serializer*  s = new Serializer("", serializedFilename);
  note->Serialize(s);
  delete s;

  // clear current instance of note
  delete note;
  note = new Note("otherfoo");

  // unserialze it
  Serializer* s2 = new Serializer(serializedFilename, "");
  note->ReadFromSerializer(s2);
  delete s2;

  EXPECT_STREQ(name.c_str(), note->GetText().c_str());
  delete note;
}

TEST(NoteTest, SerializedAndUnSerializedNoteMatchInTextWithDate) {
  string name = "foo2";
  Note* note = new Note(name);

  // serialize it
  string serializedFilename = "/tmp/NoteTest.project";
  Serializer*  s = new Serializer("", serializedFilename);
  note->Serialize(s);
  string textWithDate = note->Text();
  delete s;

  // clear current instance of note
  delete note;
  note = new Note("otherfoo");

  // unserialze it
  Serializer* s2 = new Serializer(serializedFilename, "");
  note->ReadFromSerializer(s2);
  delete s2;

  EXPECT_STREQ(textWithDate.c_str(), note->Text().c_str());
  delete note;
}
