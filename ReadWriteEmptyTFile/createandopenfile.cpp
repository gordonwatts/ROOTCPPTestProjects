#include <iostream>

#include "TFile.h"

using namespace std;

int main()
{
	cout << "Creating empty file." << endl;
	auto f1 = TFile::Open("junk.root", "RECREATE");
	delete f1;

	cout << "Opening for reading the empty file" << endl;
	auto f2 = TFile::Open("junk.root", "READ");
	f2->ls();
	delete f2;
	return 0;
}