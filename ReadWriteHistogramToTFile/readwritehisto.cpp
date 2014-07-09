#include <iostream>

#include "TFile.h"
#include "TH1F.h"
#include "TApplication.h"
#include "TSystem.h"

using namespace std;

int main()
{
	auto app = new TApplication("readwritehisto", NULL, NULL);

	gSystem->Setenv("ROOTSYS", "D:\\Code\\ROOTCPPTestProjects\\Release");

	auto rootsys = gSystem->Getenv("ROOTSYS");
	if (rootsys == NULL) {
		cout << "Unable to get proper value for ROOTSYS" << endl;
	}
	else {
		cout << "ROOTSYS is " << rootsys << endl;
	}


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