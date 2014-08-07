#include <iostream>

#include "TClass.h"

using namespace std;

int main()
{
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