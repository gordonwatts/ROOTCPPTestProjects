#include "TSystem.h"
#include "TApplication.h"
#include "TString.h"
#include "TInterpreter.h"

#include <iostream>
#include <fstream>

using namespace std;

//
// WARNING:
// You must have all the VC defines done before running this, or the compiler will totally fail to run.
// So, be very careful! :-)
//

int main()
{
	// You can't do this without initializing TCint. The easiest way to do this is
	// by creating the TApplication object.

	int args = 0;
	char **argv = 0;
	auto t = new TApplication("hi there", &args, argv);

	// Add ROOTSYS\bin to the PATH, because otherwise nothing will work here!
	gSystem->Setenv("PATH", TString(gSystem->Getenv("PATH")) + ";" + gSystem->Getenv("ROOTSYS") + "\\bin");

	// If cl.exe isn't in the path, then we need to bomb. ACLiC can't find cl.exe on its own.

	auto cl_location = TString(gSystem->FindFile(gSystem->Getenv("PATH"), TString("cl.exe")));
	if (cl_location.Length() == 0) {
		cout << "Unable to find the location of the C++ compiler, cl.exe. This test will fail." << endl;
		cout << "Make sure you are running this from the VS command line tools prompt, or have run the vc env vars bat script!" << endl;
		cout << "Aborting" << endl;
		return 1;
	}

	cout << endl;
	cout << "Writing out the C++ file" << endl;
	ofstream output("junk.C");
	output << "#include <iostream>" << endl;
	output << "using namespace std;" << endl;
	output << "void junk() {" << endl;
	output << "  cout << \"hi\" << endl;" << endl;
	output << "}" << endl;
	output.close();

	cout << endl;
	cout << "Compiling and Loading macro" << endl;
	cout << "========================" << endl;
	gSystem->CompileMacro("junk.C");

	// Now, can we run it?
	cout << endl;
	cout << "Running macro" << endl;
	cout << "========================" << endl;
	int error;
	gInterpreter->Execute("junk", "", &error);
	cout << "Error from running: " << error << endl;
}