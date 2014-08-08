#include <iostream>
#include <string>
#include <functional>
#include <map>

#include "TRandom.h"
#include "TString.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TApplication.h"

using namespace std;

function<pair<double, double>(void)> generate_from_2d_hist(auto_ptr<TFile> &f, const string &hname);
pair<double, double> calc_lxy(const pair<double, double> &pt, const pair<double, double> &ctau, const double &mass);
function<double(pair<double, double>)> get_2d_lookup_function(auto_ptr<TFile> &f, const string &hname);
double get_hist_entries(auto_ptr<TFile> &f, const string &hnamne);
double get_hist_integral(auto_ptr<TFile> &f, const string &hnamne);

template <class T>
T *make_clone(T *original, const string &search, const string &replace);

// Helper for extracting and copying binning
class axis_binning {
public:
	int nbins;
	double lowx;
	double highx;

	axis_binning(int nb, double lx, double hx)
		: nbins(nb), lowx(lx), highx(hx)
	{}
	axis_binning(TH1F *h)
	{
		PopulateFromAxis(h->GetXaxis());
	}
	axis_binning(TH2F *h, int axis)
	{
		if (axis == 1) {
			PopulateFromAxis(h->GetXaxis());
		}
		else {
			PopulateFromAxis(h->GetYaxis());
		}
	}
private:
	void PopulateFromAxis(TAxis *a)
	{
		nbins = a->GetNbins();
		lowx = a->GetBinLowEdge(1);
		highx = a->GetBinUpEdge(nbins);
	}
};

pair<axis_binning, axis_binning> get_binning_from_hist(auto_ptr<TFile> &f, const string &hname);

// Plots 2d plots, and 2 1d plots.
class plot_2d {
public:
	plot_2d(const string &name, const string &title,
		const string &name_1, const string &title_1, int nbinsX, double lowx, double highx,
		const string &name_2, const string &title_2, int nbinxY, double lowy, double highy) {
		Book(name, title, name_1, title_1, nbinsX, lowx, highx, name_2, title_2, nbinxY, lowy, highy);
	}
	plot_2d(const string &name, const string &title,
		const string &name_1, const string &title_1, const axis_binning &a1,
		const string &name_2, const string &title_2, const axis_binning &a2)
	{
		Book(name, title, name_1, title_1, a1.nbins, a1.lowx, a1.highx, name_2, title_2, a2.nbins, a2.lowx, a2.highx);
	}

	void Fill(pair<double, double> p, double weight = 1.0) {
		_axis1->Fill(p.first, weight);
		_axis2->Fill(p.second, weight);
		_map->Fill(p.first, p.second, weight);
	}

	plot_2d Clone(const string &search, const string &replace) {
		auto newNameX = TString(_axis1->GetName()).ReplaceAll(search.c_str(), replace.c_str());
		auto newNameY = TString(_axis2->GetName()).ReplaceAll(search.c_str(), replace.c_str());
		auto newName = TString(_map->GetName()).ReplaceAll(search.c_str(), replace.c_str());

		auto n_axis1 = (TH1F*)_axis1->Clone(newNameX);
		auto n_axis2 = (TH1F*)_axis1->Clone(newNameY);
		auto n_map = (TH2F*)_map->Clone(newName);

		n_axis1->SetTitle(TString(n_axis1->GetTitle()).ReplaceAll(search.c_str(), replace.c_str()));
		n_axis2->SetTitle(TString(n_axis2->GetTitle()).ReplaceAll(search.c_str(), replace.c_str()));
		n_map->SetTitle(TString(n_map->GetTitle()).ReplaceAll(search.c_str(), replace.c_str()));

		return plot_2d(n_axis1, n_axis2, n_map);
	}

private:
	TH1F *_axis1, *_axis2;
	TH2F *_map;

	plot_2d(TH1F *a1, TH1F *a2, TH2F *m) {
		_axis1 = a1;
		_axis2 = a2;
		_map = m;
	}

	void Book(const string &name, const string &title,
		const string &name_1, const string &title_1, int nbinsX, double lowx, double highx,
		const string &name_2, const string &title_2, int nbinxY, double lowy, double highy) {
		_axis1 = new TH1F(name_1.c_str(), title_1.c_str(), nbinsX, lowx, highx);
		_axis2 = new TH1F(name_2.c_str(), title_2.c_str(), nbinsX, lowy, highy);
		_map = new TH2F(name.c_str(), title.c_str(), nbinsX, lowx, highx, nbinxY, lowy, highy);
	}
};

#define sample_100_25

int main()
{
	int args = 0;
	char **argv = 0;
	auto t = new TApplication("hi there", &args, argv);

	// Config parameters (shoudl be loaded from a file).
	string dataset = "159221_100_25_plots_integral_10m_1500GeV_new";
	double generated_ctau = 1.25; // meters (from the note appendix)
	double vpion_mass = 25; // GeV.

	double lxy_min = 0;
	double lxy_max = 10000 / 1000.;

	// Open the file so we can have it for later use.
	auto input_fname = TString("") + dataset + ".root";
	auto input_file = auto_ptr<TFile>(TFile::Open(input_fname, "READ"));

	// Functions we can use to generate various things
	double ctau = generated_ctau;
	function<pair<double, double>(void) > generate_lifetime_dual_exp = [ctau]() { return make_pair(gRandom->Exp(ctau), gRandom->Exp(ctau)); };

	function<pair<double, double>(void)> generate_pt_from_final = generate_from_2d_hist(input_file, "Final_events/ana_vpi_pt1_pt2");
	function<pair<double, double>(void)> generate_pt_from_initial = generate_from_2d_hist(input_file, "MC_Truth/genVpiPt1Pt2");
	auto pt_axis_binning = get_binning_from_hist(input_file, "MC_Truth/genVpiPt1Pt2");

	// Choose what we will use to do the generation.
	function<pair<double, double>(void)> generate_lifetime = generate_lifetime_dual_exp;
	function<pair<double, double>(void)> generate_pt = generate_pt_from_initial;

	// The weighting function to use
	auto weight_lxy = get_2d_lookup_function(input_file, "Final_events/effi_Lxy1_Lxy2");

	// Output file
	auto fname = TString("../results_") + dataset + ".root";
	auto hist_output = auto_ptr<TFile>(TFile::Open(fname, "RECREATE"));

	// Histograms to save.
	auto h_ctau1 = new TH1F("ctau_1", "ctau of vpion 1", 5000, 0.0, 30.0);
	auto h_ctau2 = make_clone(h_ctau1, "1", "2");

	auto h_pt = plot_2d("raw_pt1_pt2", "p_T_1 vs p_T_2 raw distributions",
		"raw_pt1", "p_T_1 raw", pt_axis_binning.first,
		"raw_pt2", "p_T_2 raw", pt_axis_binning.second);

	auto h_lxy1 = new TH1F("lxy1", "lxy of vpion 1", 20 * 20, 0.0, 20.0);
	auto h_lxy2 = make_clone(h_lxy1, "1", "2");

	auto h_lxy1_maxmin = make_clone(h_lxy1, "lxy", "bad min bmax vpion's lxy for");
	h_lxy1_maxmin->SetName("lxy1_minmax");
	auto h_lxy2_maxmin = make_clone(h_lxy1_maxmin, "1", "2");

	auto h_lxy1_badeff = make_clone(h_lxy1, "lxy", "bad eff lxy for");
	h_lxy1_badeff->SetName("lxy1_badeff");
	auto h_lxy2_badeff = make_clone(h_lxy1_badeff, "1", "2");

	auto h_lxy1_weight = make_clone(h_lxy1, "lxy", "weighted lxy");
	h_lxy1_weight->SetName("lxy1_weighted");
	auto h_lxy2_weight = make_clone(h_lxy1_badeff, "1", "2");

	auto h_event_weight = new TH1F("event_weight", "Event weights", 100, 0.0, 1.0);

	auto h_lxy_weight_map = new TH2F("event_weight_map", "Map of efficiencies", 20 * 10, 0.0, 10.0, 20 * 10, 0.0, 10.0);
	auto h_lxy_map = new TH2F("event_map", "Map of lxy pairs", 20 * 10, 0.0, 10.0, 20 * 10, 0.0, 10.0);
	auto h_pt_weight = h_pt.Clone("raw", "weighted");

	// Run the toy
	const int toy_runs = 10000000;
	double eff_sum = 0.0;
	for (int i_toy = 0; i_toy < toy_runs; i_toy++) {
		// We need the pT and the lifetimes of the objects.
		auto lt = generate_lifetime();
		h_ctau1->Fill(lt.first);
		h_ctau2->Fill(lt.second);

		auto pt = generate_pt();
		h_pt.Fill(pt);

		// Straight conversion into the actual decay length.
		auto lxy = calc_lxy(pt, lt, vpion_mass);
		h_lxy1->Fill(lxy.first);
		h_lxy2->Fill(lxy.second);

		// Calc the weighting for this event. We have some x-checks to make sure everything is "in bounds" here.
		if (lxy.first < lxy_min || lxy.first > lxy_max
			|| lxy.second < lxy_min || lxy.second > lxy_max) {
			h_lxy1_maxmin->Fill(lxy.first);
			h_lxy2_maxmin->Fill(lxy.second);
		}
		else {
			double event_eff = weight_lxy(lxy);
			if (event_eff < 0) {
				h_lxy1_badeff->Fill(lxy.first);
				h_lxy2_badeff->Fill(lxy.second);
			}
			else {
				// Everything is good! Fill some "interesting" plots.
				h_event_weight->Fill(event_eff);
				h_lxy1_weight->Fill(lxy.first, event_eff);
				h_lxy1_weight->Fill(lxy.second, event_eff);
				h_lxy_weight_map->Fill(lxy.first, lxy.second, event_eff);
				h_lxy_map->Fill(lxy.first, lxy.second);
				h_pt_weight.Fill(pt, event_eff);
				eff_sum += event_eff;
			}
		}
	}

	// Calculate the final efficiency

	double sample_eff = eff_sum / toy_runs;
	cout << "Sample efficiency is " << sample_eff << endl;

	auto total_events_in_sample = get_hist_integral(input_file, "MC_Truth/genVpiPt1Pt2");
	cout << "Total events in sample " << total_events_in_sample << endl;
	double expected_events = total_events_in_sample * sample_eff;
	cout << "Sample expected passing events is " << expected_events << endl;

	double actual_events = get_hist_integral(input_file, "Final_events/ana_vpi_Lxy1_Lxy2");
	double delta = (expected_events - actual_events) / actual_events;
	cout << "Actual events passing is " << actual_events << "(" << delta << " difference)" << endl;
	cout << "Actual sample efficiency is " << actual_events / total_events_in_sample << endl;

	// Make sure everything is correctly saved.
	hist_output->Write();
	hist_output->Close();
	return 0;
}

// Helper function for making duplicate identical histograms.
template<class T>
T *make_clone(T *original, const string &search, const string &replace)
{
	TString newName(original->GetName());
	TString newTitle(original->GetTitle());

	newName.ReplaceAll(search.c_str(), replace.c_str());
	newTitle.ReplaceAll(search.c_str(), replace.c_str());

	T *result = (T*)original->Clone(newName);
	result->SetTitle(newTitle);
	return result;
}

// Get a 2D histo from a file and use it to generate a random 2D distribution.
function<pair<double, double>(void)> generate_from_2d_hist(auto_ptr<TFile> &f, const string &hname)
{
	auto h = (TH2F*)f->Get(hname.c_str());
	if (h == nullptr) {
		throw runtime_error(string("Unable to load histogram") + hname);
	}
	return[h]() {double p1, p2; h->GetRandom2(p1, p2); return make_pair(p1, p2); };
}

// Given pt and ctau, find out where the thing decayed.
pair<double, double> calc_lxy(const pair<double, double> &pt, const pair<double, double> &ctau, const double &mass)
{
	return make_pair(pt.first / mass * ctau.first, pt.second / mass * ctau.second);
}

// Get a bin number for the bin we need to look at. Return a -1 if we over flow.
function<double(pair<double, double>)> get_2d_lookup_function(auto_ptr<TFile> &f, const string &hname)
{
	auto h = (TH2F*)f->Get(hname.c_str());
	if (h == nullptr) {
		throw runtime_error(string("Unable to load histogram") + hname);
	}
	return [h](pair<double, double> l) {
		int i_bin_1 = h->GetXaxis()->FindBin(l.first);
		int i_bin_2 = h->GetXaxis()->FindBin(l.second);

		// If overflow, bail!
		if (i_bin_1 > h->GetXaxis()->GetNbins())
			return -1.0;
		if (i_bin_2 > h->GetYaxis()->GetNbins())
			return -1.0;
		if (i_bin_1 < 0 || i_bin_2 < 0)
			return -1.0;

		return h->GetBinContent(i_bin_1, i_bin_2);
	};
}

// Return the number of entries in this file
double get_hist_entries(auto_ptr<TFile> &f, const string &hname)
{
	auto h = (TH1*)f->Get(hname.c_str());
	if (h == nullptr) {
		throw runtime_error(string("Unable to load histogram") + hname);
	}
	return h->GetEntries();
}

// Calculate the integral for a histogram
double get_hist_integral(auto_ptr<TFile> &f, const string &hname)
{
	auto h = (TH1*)f->Get(hname.c_str());
	if (h == nullptr) {
		throw runtime_error(string("Unable to load histogram") + hname);
	}
	return h->Integral();
}

// Return the axis binning from a 2D histo.
pair<axis_binning, axis_binning> get_binning_from_hist(auto_ptr<TFile> &f, const string &hname)
{
	auto h = (TH2F*)f->Get(hname.c_str());
	if (h == nullptr) {
		throw runtime_error(string("Unable to load histogram") + hname);
	}

	return make_pair(axis_binning(h, 1), axis_binning(h, 2));
}