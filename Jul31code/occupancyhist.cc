//***************************************
//  channel occupancy histogram
//***************************************

//some standard C++ includes
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>

//some ROOT includes
#include "TInterpreter.h"
#include "TROOT.h"
#include "TH1F.h"
#include "TH2S.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TVectorD.h"
#include "TGraph.h"

//"art" includes (canvas, and gallery)
#include "canvas/Utilities/InputTag.h"
#include "gallery/Event.h"
#include "gallery/ValidHandle.h"
#include "canvas/Persistency/Common/FindMany.h"
#include "canvas/Persistency/Common/FindOne.h"

//"larsoft" object includes
#include "lardataobj/RecoBase/Wire.h"

//convenient for us! let's not bother with art and std namespaces!
using namespace art;
using namespace std;

using namespace std::chrono;



int main(int argc, char** argv) {

  TFile f_output("occupancyhist_output.root","RECREATE");
  
  //We specify our files in a list of file names!
  //Note: multiple files allowed. Just separate by comma.
  vector<string> filenames { argv[1] };

  //We need to specify the "input tag" for our collection of optical flashes.
  //This is like the module label, except it can also include process name
  //and an instance label. Format is like this:
  //InputTag mytag{ "module_label","instance_label","process_name"};
  //You can ignore instance label if there isn't one. If multiple processes
  //used the same module label, the most recent one should be used by default.
  //
  //Check the contents of your file by setting up a version of uboonecode, and
  //running an event dump:
  //  'lar -c eventdump.fcl -s MyInputFile_1.root -n 1'
  //  InputTag wire_tag { "sndaq", "SupernovaAssembler" }; // does not work
  InputTag wire_tag { "sndaq", "", "SupernovaAssembler" }; // before deconvolution
  // InputTag wire_tag { "sndeco", "", "CalDataSN" }; // after deconvolution


  size_t _maxEvts = 200;
  size_t evCtr = 0;

  TCanvas *c1 = new TCanvas("wirehist200","c1",1000,700);
  
  // Create two arrays that will eventually be used to fill the plot, one of the total hits per each wire and one of just the channel numbers
  double totalhits[8256];
  double x[8256];

  for (gallery::Event ev(filenames) ; !ev.atEnd(); ev.next()) {
    if(evCtr >= _maxEvts) break;

    auto t_begin = high_resolution_clock::now();

    int run = ev.eventAuxiliary().run();
    int event = ev.eventAuxiliary().event();
 
    //to get run and event info, you use this "eventAuxillary()" object.
    cout << "Processing "
	 << "Run " << run << ", "
	 << "Event " << event << endl;

    //Now, we want to get a "valid handle" (which is like a pointer to our collection")
    //We use auto, cause it's annoying to write out the fill type. But it's like
    //vector<recob::Wire>* object.
    auto const& wire_handle = ev.getValidHandle< vector<recob::Wire> >(wire_tag);
   
    auto const& wire_vec(*wire_handle);
    
    cout << "\tThere are " << wire_vec.size() << " Wires in this event." << endl;
   
    // cout << "Beginning of loop over wires" << endl;

    // create a vector with the number of hits per wire of one event
    std::vector<int> wirehits(8256);
    
    //Fill the totalhits array by adding up the number of hits in each wire over every event
    for (unsigned int i=0;i<wire_vec.size();i++ )
      { auto const numhits = wire_vec[i].SignalROI().n_ranges(); // n_ranges give number of events
	int channel = wire_vec[i].Channel();//get channel number of each wire 
	//cout<< channel<<' ';
	wirehits[channel]= numhits;
	totalhits[channel]=totalhits[channel]+wirehits[channel];//add to previous event by channel in total hits  
    	      }
    
    

    //We can use a range-based for loop for ease.
    //for( auto const& wire : wire_vec){
      //cout << "\nwire.Signal().size() " << wire.Signal().size();
      //cout << "\nwire.NSignal() " << wire.NSignal();
      //cout << "\nwire.View() " << wire.View();
      // int channel = wire.Channel();
      //cout << "\nwire.Channel() " << channel  << endl;
      // test SignalROI()
      // Loop over the zero-padded vector
      
      
      //auto const numhits = wire_vec[i].SignalROI().n_ranges();
      //wirehits[i]= numhits;
      //totalhits[i]=totalhits[i]+wirehits[i];
    

      //std::vector<float> zeroPaddedWire = wire.Signal();
      //auto const numhits = wire.SignalROI().n_ranges();
      //cout<<zeroPaddedWire.size();
      
      //for( size_t tick = 0; tick < zeroPaddedWire.size(); tick++ ){
      //cout << tick << ' ' << zeroPaddedWire[tick]<< '\n'; }
      //}
	
       
      // }

    auto t_end = high_resolution_clock::now();
    duration<double,std::milli> time_total_ms(t_end-t_begin);
    cout << "\tEvent took " << time_total_ms.count() << " ms to process." << endl;
    evCtr++;
  } 
  f_output.cd();
  // just make the double array of all channel numbers
   for(int n=0; n<8256;n++)
    {x[n]=n;}
   
   // make a bar graph using totalhits as the height of all 8256 bars each bar is a different channel 
   // this will essentially be a histogram with number of hits per wire on the y axis
   TGraph* wireevents  = new TGraph(8256,x,totalhits);
   wireevents->SetTitle("Wire Occupancy");
   wireevents->GetXaxis()->SetTitle("Wire");
   wireevents->GetXaxis()->SetRangeUser(0,8256);
   wireevents->GetYaxis()->SetTitle("Number of Hits per Wire");
   wireevents->GetYaxis()->SetTitleOffset(1.2);
   c1->cd();
   wireevents->SetFillColor(1);
   wireevents->Draw("AB");

   c1->Print(".png");
  
  delete c1;
  delete wireevents;
 
 
  f_output.Write();
  f_output.Close();

}
