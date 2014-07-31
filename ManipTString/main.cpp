// Make sure the interface to TString works properly in debug and relese mode.

#include <iostream>
#include <string>

#include <TString.h>

using namespace std;

int main()
{
	TString s ("hi");
	s = s + " there";

	cout << "the result is '" << (const char*)s << "'" << endl;

	TString s2("hi");
	string b(" there");
	auto r = s2 + b;
	cout << "the second result is '" << (const char*)r << "'" << endl;

	return 0;
}