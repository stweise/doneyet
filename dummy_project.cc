#include "dummy_project.h"

DProject::DProject(string name) : name_(name) { }

string DProject::getDProjectName(){
	return name_;
}
