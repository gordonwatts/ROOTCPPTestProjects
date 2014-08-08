#include <iostream>

#include "TClass.h"
#include "TApplication.h"
#include "TInterpreter.h"

using namespace std;

int main()
{
	// You can't do this without initializing TCint. The easiest way to do this is
	// by creating the TApplication object.

	int args = 0;
	char **argv = 0;
	auto t = new TApplication("hi there", &args, argv);

	// Get the class pointer and then the list of objects.
	
	auto a = TClass::GetClass("TApplication");
	if (a == nullptr) {
		cout << "Unable to get the TApplication class pointer!" << endl;
		return 1;
	}
	auto slist = a->GetSharedLibs();
	if (slist == nullptr) {
		cout << "Unable to get the TApplication list of shared libraries!" << endl;
		return 1;
	}
	cout << "Shared libraries: " << slist << endl;
	return 0;
}