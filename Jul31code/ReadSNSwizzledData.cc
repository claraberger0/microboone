/*************************************************************
 * 
 * ReadSNSwizzledData.cc program
 * 
 * This is a simple demonstration of reading a LArSoft file 
 * and accessing recob::Wire information
 *
 * José I. Crespo-Anadón (jcrespo@nevis.columbia.edu), Jul 10, 2017
 * Based on code by Wesley Ketchum (wketchum@fnal.gov), Aug28, 2016
 * 
 *************************************************************/


//some standard C++ includes
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <chrono>

//some ROOT includes
#include "TInterpreter.h"
#include "TROOT.h"
#include "TH1F.h"
#include "TH2S.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TStyle.h"

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

  TFile f_output("ReadSNSwizzledData_output.root","RECREATE");
  
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



  //ok, now for the event loop! Here's how it works.
  //
  //gallery has these built-in iterator things.
  //
  //You declare an event with a list of file names. Then, you
  //move to the next event by using the "next()" function.
  //Do that until you are "atEnd()".
  //
  //In a for loop, that looks like this:

  size_t _maxEvts = 100;
  size_t evCtr = 0;
  //TCanvas *c1 = new TCanvas("c1","c1",900,700);
  //TCanvas *c2 = new TCanvas("c2","c2",900,700);
  // TCanvas *c3 = new TCanvas("c3","c3",900,700);
  TCanvas *c5 = new TCanvas("c5","c5",1000,600);
  //TCanvas *c4 = new TCanvas("c4","c4",900,600);  
  TH2F* allevent = new TH2F("all","100 events; Channel; Tick",8256,0,8256,6400,0,6400); 
  allevent->GetXaxis()->SetRangeUser(0,8256);
  allevent->GetZaxis()->SetRangeUser(0,120000);
  for (gallery::Event ev(filenames) ; !ev.atEnd(); ev.next()) {
    if(evCtr >= _maxEvts) break;

    auto t_begin = high_resolution_clock::now();

    // int run = ev.eventAuxiliary().run();
    //int event = ev.eventAuxiliary().event();
 
    //to get run and event info, you use this "eventAuxillary()" object.
    //cout << "Processing "
    //	 << "Run " << run << ", "
    //	 << "Event " << event << endl;

    //Now, we want to get a "valid handle" (which is like a pointer to our collection")
    //We use auto, cause it's annoying to write out the fill type. But it's like
    //vector<recob::Wire>* object.
    auto const& wire_handle = ev.getValidHandle< vector<recob::Wire> >(wire_tag);

    //We can now treat this like a pointer, or dereference it to have it be like a vector.
    //I (Wes) for some reason prefer the latter, so I always like to do ...
    auto const& wire_vec(*wire_handle);

    //cout << "\tThere are " << wire_vec.size() << " Wires in this event." << endl;




    // Event display histogram
  
    //TCanvas *c4 = new TCanvas(Form("c4_%d_%d", run ,event),Form("c4_%d_%d", run ,event));
    //c4->Divide(1,3);
  
    //TH2S*  hTickWireu = new TH2S(Form("hTickWireu_run%d_event%d", run ,event), Form("Run %d event %d U; Channel; Tick", run ,event), 8256, 0, 2400, 6400, 0, 6400); 
    //hTickWireu->GetZaxis()->SetRangeUser(1950,2150);
    //TH2S* hTickWirev = new TH2S(Form("hTickWirev_run%d_event%d", run ,event), Form("Run %d event %d V; Channel; Tick", run ,event), 8256, 2400, 4800, 6400, 0, 6400);
    //hTickWirev->GetZaxis()->SetRangeUser(1950,2150);
    //TH2S* hTickWirey = new TH2S(Form("hTickWirey_run%d_event%d", run ,event), Form("Run %d event %d Y; Channel; Tick", run ,event), 8256, 4800, 8256, 6400, 0, 6400);
    //hTickWirey->GetZaxis()->SetRangeUser(380,580);

    //We can loop over the vector to get wire info too!
    //
    //Don't remember what's in a recob::Wire? Me neither. So, we can look it up.
    //The environment variable $LARDATAOBJ_DIR contains the directory of the
    //lardataobj product we set up. Inside that directory we can find what we need:
    // ' ls $LARDATAOBJ_DIR/source/lardataobj/RecoBase/Wire.h '
    //Note: it's the same directory structure as the include, after you go into
    //the 'source' directory. Look at that file and see what you can access.

    cout << "Beginning of loop over wires" << endl;
    
    //We can use a range-based for loop for ease.
    for( auto const& wire : wire_vec){
      //cout << "\nwire.Signal().size() " << wire.Signal().size();
      //cout << "\nwire.NSignal() " << wire.NSignal();
      //cout << "\nwire.View() " << wire.View();
      int channel = wire.Channel();
      //cout << "\nwire.Channel() " << channel << endl;
      // test SignalROI()

      // Loop over the zero-padded vector
      std::vector<float> zeroPaddedWire = wire.Signal();
      for( size_t tick = 0; tick < zeroPaddedWire.size(); tick++ ){
	//to plot all events overlapped
	allevent->Fill(channel, tick, zeroPaddedWire[tick]);
	//if channel number -- to do
	//if (channel <= 2400)
	//{hTickWireu->Fill(channel, tick, zeroPaddedWire[tick]);}
	//else if (channel > 2400 && channel <= 4800)
	// {hTickWirev->Fill(channel, tick, zeroPaddedWire[tick]);}
	//else  
	//{hTickWirey->Fill(channel, tick, zeroPaddedWire[tick]);}
      }
    } //cout << "End of loop over wires" << endl;

    f_output.cd();
        
    //cout << "Drawing u plane" << endl;

    c5->cd();
    allevent->Draw("colz");
    gStyle->SetOptStat(0);
    c5->Print("100events.gif+30");

    // c4->cd(1);
    //hTickWireu->Draw("colz");

    //cout << "Drawing v plane" << endl;

    //c4->cd(2);
    //hTickWirev->Draw("colz");

    //cout << "Drawing y plane" << endl;

    //c4->cd(3);
    //hTickWirey->Draw("colz");

    //hTickWireu->Write();
    //hTickWirev->Write();
    //hTickWirey->Write();

    //c4->cd();
    //allevent->Draw("colz");

    // cout << "Writing canvas to file" << endl;

    //    c4->Print("allevent.png");

    //cout << "Deleting pointers" << endl;

    //delete hTickWireu;
    //delete hTickWirev;
    //delete hTickWirey;
    //delete c4;
    //delete allevent;

    auto t_end = high_resolution_clock::now();
    duration<double,std::milli> time_total_ms(t_end-t_begin);
    //cout << "\tEvent took " << time_total_ms.count() << " ms to process." << endl;
    evCtr++;
  } //end loop over events!

  //  //only for overlapping plot
  f_output.cd();
  //c4->cd();
  //allevent->Draw("colz");
  //gStyle->SetOptStat(0);
  //c4->Print("allevent.png");
  //c4->Write();
  //delete c4;
  //delete allevent;

  c5->Print("100events.gif++");
 //and ... write to file!
  f_output.Write();
  f_output.Close();

}
