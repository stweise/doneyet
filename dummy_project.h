#ifndef DUMMY_PROJECT_H_
#define DUMMY_PROJECT_H_

#include <string>
using namespace std;

class DProject {
 public:
  explicit DProject(string name);
  string getDProjectName();
 private:
  string name_;
};

#endif  // DUMMY_PROJECT_H_
