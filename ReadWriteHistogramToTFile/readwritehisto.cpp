#include <iostream>

#include "TFile.h"
#include "TH1F.h"

using namespace std;

int main()
{
	cout << "Creating empty file." << endl;
	auto f1 = TFile::Open("junkWithHisto.root", "RECREATE");

	// Creating a histo
	cout << "Creating histo" << endl;
	auto h1 = new TH1F("hi", "there", 10, 0.0, 10.0);
	h1->Fill(4.0);
	f1->Write();
	f1->Close();
	delete f1;

	cout << "Opening for reading the empty file" << endl;
	auto f2 = TFile::Open("junkWithHisto.root", "READ");
	f2->ls();
	delete f2;
	return 0;
}