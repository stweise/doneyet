#include "task.h"
#include <assert.h>
#include <algorithm>
#include <string>
#include "file-versions.h"
#include "note.h"
#include "serializer.h"
#include "utils.h"

using std::string;

Task::Task(const string& title, const string& description)
    : parent_(NULL),
      status_(CREATED),
      title_(title),
      description_(description) {
  creation_date_.SetToNow();
  start_date_.SetToEmptyTime();
  completion_date_.SetToEmptyTime();
}

Task::~Task() {
  for (int i = 0; i < subtasks_.size(); ++i) {
    delete subtasks_[i];
  }
}

Task* Task::NewTaskFromSerializer(Serializer* s) {
  string title = s->ReadString();
  string description = s->ReadString();
  Task* t = new Task(title, description);
  t->UnSerializeFromSerializer(s);
  return t;
}

void Task::AddNote(const string& note) { notes_.push_back(new Note(note)); }

bool Task::HasNotes() { return !notes_.empty(); }

void Task::DeleteNote(const string& note) {
  std::vector<Note*>::iterator delete_it;
  bool found = false;

  for (vector<Note*>::iterator it = notes_.begin(); it != notes_.end(); it++) {
    if ((*it)->GetText().compare(note) == 0) {
      found = true;
      delete_it = it;
    }
  }
  if (found) {
    notes_.erase(delete_it);
  }
}

vector<string> Task::Notes() {
  vector<string> notes;
  for (int i = 0; i < notes_.size(); ++i) {
    notes.push_back(notes_[i]->GetText());
  }
  return notes;
}

map<string, string> Task::MappedNotes() {
  map<string, string> mappedNotes;
  for (int i = 0; i < notes_.size(); ++i) {
    mappedNotes[notes_[i]->Text()] = notes_[i]->GetText();
  }
  return mappedNotes;
}

void Task::ApplyFilter(FilterPredicate<Task>* filter) {
  // It's important that we filter ourselves after our children because often
  // filters have an OrPredicate of "Has any filtered children" which wouldn't
  // if we filtered ourselves before our children.
  for (int i = 0; i < NumChildren(); ++i) {
    Child(i)->ApplyFilter(filter);
  }
  filtered_tasks_ = filter->FilterVector(subtasks_);
}

void Task::AddSubTask(Task* subtask) {
  subtask->SetParent(this);
  subtasks_.push_back(subtask);
}

void Task::RemoveSubtaskFromList(Task* t) {
  for (int i = 0; i < subtasks_.size(); ++i) {
    if (subtasks_[i] == t) {
      subtasks_.erase(subtasks_.begin() + i);
      return;
    }
  }
}

void Task::DeleteTask(Task* t) {
  // Check if we're supposed to delete ourselves:
  if (this == t) {
    Delete();
  } else {
    for (int i = 0; i < NumChildren(); ++i) {
      if (Child(i) == t) {
        Child(i)->Delete();
        return;
      } else {
        Child(i)->DeleteTask(t);
      }
    }
  }
}

void Task::Delete() {
  for (int i = 0; i < NumChildren(); ++i) {
    Child(i)->Delete();
  }
  if (Parent() != NULL) {
    Parent()->RemoveSubtaskFromList(this);
  }
  delete this;
}

void Task::SwapTasks(Task* a, Task* b) {
  if (a == b) {
    return;
  }

  // Find the indices of a and b.
  vector<Task*>::iterator ait = find(subtasks_.begin(), subtasks_.end(), a);
  vector<Task*>::iterator bit = find(subtasks_.begin(), subtasks_.end(), b);

  assert(ait != subtasks_.end() || bit != subtasks_.end());

  int ai = ait - subtasks_.begin();
  int bi = bit - subtasks_.begin();

  Task* tmp = a;
  subtasks_[ai] = b;
  subtasks_[bi] = tmp;
  return;
}

void Task::MoveTaskUp(Task* t) {
  // We can't move up the first task.
  if (t == filtered_tasks_[0]) {
    return;
  }

  // Find the task in our filtered list:
  vector<Task*>::iterator it =
      find(filtered_tasks_.begin(), filtered_tasks_.end(), t);
  --it;
  SwapTasks(*it, t);
}

void Task::MoveTaskDown(Task* t) {
  // We can't move down the last task.
  if (t == filtered_tasks_[filtered_tasks_.size() - 1]) {
    return;
  }

  // Find the task in our filtered list:
  vector<Task*>::iterator it =
      find(filtered_tasks_.begin(), filtered_tasks_.end(), t);
  ++it;
  SwapTasks(t, *it);
}

void Task::MoveUp() {
  if (Parent()) {
    Parent()->MoveTaskUp(this);
  }
}

void Task::MoveDown() {
  if (Parent()) {
    Parent()->MoveTaskDown(this);
  }
}

void Task::Serialize(Serializer* s) {
  // Initially we store a unique identifier to ourselves that will help with
  // reading in the tasks and assembling the tree.
  s->WriteUint64((uint64)this);

  // Data about this task.
  s->WriteString(title_);
  s->WriteString(description_);
  s->WriteInt32(static_cast<int32>(status_));

  // Various dates.
  creation_date_.Serialize(s);
  start_date_.Serialize(s);
  completion_date_.Serialize(s);

  // The notes associated with this task.
  if (s->Version() >= NOTES_VERSION) {
    s->WriteInt32(notes_.size());
    for (int i = 0; i < notes_.size(); ++i) {
      notes_[i]->Serialize(s);
    }
  }

  // Task status changes.
  if (s->Version() >= TASK_STATUS_VERSION) {
    s->WriteInt32(status_changes_.size());
    for (int i = 0; i < status_changes_.size(); ++i) {
      status_changes_[i].date.Serialize(s);
      s->WriteInt32(status_changes_[i].status);
    }
  }

  // Finally our parent pointer and then we move onto the children.
  s->WriteUint64((uint64)parent_);
  for (int i = 0; i < subtasks_.size(); ++i) {
    subtasks_[i]->Serialize(s);
  }
}

void Task::UnSerializeFromSerializer(Serializer* s) {
  status_ = static_cast<TaskStatus>(s->ReadInt32());
  creation_date_.ReadFromSerializer(s);
  start_date_.ReadFromSerializer(s);
  completion_date_.ReadFromSerializer(s);

  if (s->Version() >= NOTES_VERSION) {
    int num_notes = s->ReadInt32();
    for (int i = 0; i < num_notes; ++i) {
      Note* n = new Note("");
      n->ReadFromSerializer(s);
      notes_.push_back(n);
    }
  }

  if (s->Version() >= TASK_STATUS_VERSION) {
    int num_status_changes = s->ReadInt32();
    for (int i = 0; i < num_status_changes; ++i) {
      Date d;
      d.ReadFromSerializer(s);
      int status = s->ReadInt32();
      status_changes_.push_back(StatusChange(d, status));
    }
  }
}

void Task::SetStatus(TaskStatus t) {
  if (status_ == CREATED && t == IN_PROGRESS) {
    // We were set to in progress for the first time.
    start_date_.SetToNow();
  } else if (t == COMPLETED && status_ != COMPLETED) {
    completion_date_.SetToNow();
  } else if (t == PAUSED && status_ != PAUSED) {
    completion_date_.SetToEmptyTime();
  }

  if (t == IN_PROGRESS) {
    completion_date_.SetToEmptyTime();
  }

  status_ = t;

  // Update the status record for this task.
  status_changes_.push_back(StatusChange(Date(), status_));
}

int Task::NumOffspring() {
  int sum_from_children = 0;
  for (int i = 0; i < subtasks_.size(); ++i) {
    sum_from_children += 1 + subtasks_[i]->NumOffspring();
  }

  return sum_from_children;
}

int Task::NumFilteredOffspring() {
  int sum_from_children = 0;
  for (int i = 0; i < filtered_tasks_.size(); ++i) {
    sum_from_children += 1 + filtered_tasks_[i]->NumFilteredOffspring();
  }
  return sum_from_children;
}

int Task::ListColor() {
  int c = 0;
  if (status_ == CREATED) {
    c |= COLOR_PAIR(0);
  } else if (status_ == IN_PROGRESS) {
    c |= COLOR_PAIR(2);  // green
  } else if (status_ == COMPLETED) {
    c |= COLOR_PAIR(4);  // blue
  } else if (status_ == PAUSED) {
    c |= COLOR_PAIR(1);  // red
  }

  return c;
}

int Task::NumFilteredChildren() { return filtered_tasks_.size(); }

Task* Task::FilteredChild(int c) { return filtered_tasks_[c]; }

void Task::ToStream(ostream& out, int depth) {
  const string marker = "- ";
  for (int i = 0; i < depth; ++i) {
    out << " ";
  }
  vector<string> words;
  StrUtils::SplitStringUsing(" ", title_, &words);

  out << marker;
  int line_length = depth + marker.length();
  for (int i = 0; i < words.size(); ++i) {
    if (line_length + words[i].length() > 80) {
      out << std::endl;
      for (int s = 0; s < depth + marker.length(); ++s) {
        out << " ";
      }
      line_length = depth + marker.length();
    }
    out << words[i];
    out << " ";
    line_length += words[i].length() + 1;
  }
  out << std::endl;

  for (int i = 0; i < NumFilteredChildren(); ++i) {
    FilteredChild(i)->ToStream(out, depth + 2);
  }
}
