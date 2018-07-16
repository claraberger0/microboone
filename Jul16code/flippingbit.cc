
//***************************
//    analysis of SN readout waveforms
//    Clara Berger 6/20/18
//***************************


//some standard C++ includes
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <chrono>
#include <math.h>

//some ROOT includes
#include "TInterpreter.h"
#include "TROOT.h"
#include "TH1F.h"
#include "TH2S.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TMath.h"
#include "TLine.h"
#include "TStyle.h"
#include "TExec.h"
#include "TLegend.h"

//"art" includes (canvas, and gallery)
#include "canvas/Utilities/InputTag.h"
#include "gallery/Event.h"
#include "gallery/ValidHandle.h"
#include "canvas/Persistency/Common/FindMany.h"
#include "canvas/Persistency/Common/FindOne.h"

//"larsoft" object includes
#include "lardataobj/RecoBase/Wire.h"
//#include "larevt/CalibrationDBI/Interface/ChannelStatusService.h"
//#include "larevt/CalibrationDBI/Interface/ChannelStatusProvider.h"

//convenient for us! let's not bother with art and std namespaces!
using namespace art;
using namespace std;

using namespace std::chrono;

int main(int argc, char** argv) {

   TFile f_output("flippingbit_output.root","RECREATE");
  
  //We specify our files in a list of file names!
  //Note: multiple files allowed. Just separate by comma.
  vector<string> filenames { argv[1] };

  
  //Check the contents of your file by setting up a version of uboonecode, and
  //running an event dump:
  //  'lar -c eventdump.fcl -s MyInputFile_1.root -n 1'
  //  InputTag wire_tag { "sndaq", "SupernovaAssembler" }; // does not work
  InputTag wire_tag { "sndaq", "", "SupernovaAssembler" }; // before deconvolution
  // InputTag wire_tag { "sndeco", "", "CalDataSN" }; // after deconvolution


  size_t _maxEvts = 100;
  size_t evCtr = 0;
 

  // difference to interpolation
  TCanvas c7("c7","c7",900,600);
  TH2F hIntFlipped("hIntFlipped", "ROI Integral Flipped Bit vs. No Flipped Bit; Channel; ROI integral (ADC)", 8256, 0, 8256, 8192, -4096, 4096);
  TH2F hIntNot("hIntNot", "ROI Integral Flipped Bit vs. No Flipped Bit; Channel; ROI integral (ADC)", 8256, 0, 8256, 8192, -4096, 4096);
  
  TCanvas c8("c8","c8",900,600);
  // 
  for (gallery::Event ev(filenames) ; !ev.atEnd(); ev.next()) {
    if(evCtr >= _maxEvts) break;

    auto t_begin = high_resolution_clock::now();
    
    // int run = ev.eventAuxiliary().run();
    //int event = ev.eventAuxiliary().event();
 
    //to get run and event info, you use this "eventAuxillary()" object.
    //    cout << "Processing "
    //	 << "Run " << run << ", "
    //	 << "Event " << event << endl;

    //get vector of bad channels
    //SimpleChannelStatus::SimpleChannelStatus(fhicl::ParameterSet const& pset)
    //: fMaxChannel(raw::InvalidChannelID)
    //, fMaxPresentChannel(raw::InvalidChannelID)
    //{
    //using chan_vector = std::vector<raw::ChannelID_t>;
    //chan_vect_vect BadChannels = pset.get<chan_vect>("BadChannels",chan_vect());
    //}
    
    //Now, we want to get a "valid handle" (which is like a pointer to our collection")
    //We use auto, cause it's annoying to write out the fill type. But it's like
    //vector<recob::Wire>* object.
    auto const& wire_handle = ev.getValidHandle< vector<recob::Wire> >(wire_tag);

    //We can now treat this like a pointer, or dereference it to have it be like a vector.
    //I (Wes) for some reason prefer the latter, so I always like to do ...
    auto const& wire_vec(*wire_handle);

    //cout << "\tThere are " << wire_vec.size() << " Wires in this event." << endl;
    // Event display histogram


    for (unsigned int i=0; i<wire_vec.size();i++){
      auto zsROIs = wire_vec[i].SignalROI();
      int channel = wire_vec[i].Channel();
     
      //Int nROI = wire_vec[i].SignalROI().n_ranges(); // how many ROIs in a channel
 
      for (auto iROI = zsROIs.begin_range(); iROI != zsROIs.end_range(); ++iROI) {
        auto ROI = *iROI;
        const size_t firstTick = ROI.begin_index();
	const size_t endTick = ROI.end_index();
		

	// define vector of 0s and 1s that will say whether there is a flipped bit or not 
	vector<int> flippedROI;

	// Difference to interpolation
	double difftoint;
	for (size_t iTick = 1+ROI.begin_index(); iTick < ROI.end_index(); iTick++ ){   ///loop through values in the ROI
	  difftoint = ROI[iTick]-(( ROI[iTick+1] + ROI[iTick-1] )/2);
	  
	  if(difftoint > 32){              // our metric of flipped bit is a difference of more than 32
	    flippedROI.push_back(1);}    // add a 1 if there is a flipped bit
	  else
	    {flippedROI.push_back(0);}    // add a 0 if there isnt a flipped bit
	}
      
	//cout<<flippedROI.size();
	//for(int i:flippedROI){
	//cout<<' '<<i<<' ';}
	
	// check if every value in flippedROI is 0 which would mean there is no flipped bit
	if( std::all_of(flippedROI.begin(), flippedROI.end(), [](int x){return x==0;})){   
	  double integral;                                                                                                                                  
	  TH1D horig("roi_original", "roi_original;Tick;ADC", endTick + 1 - firstTick, firstTick, endTick + 1); // new hist of the waveform
	  for (size_t iTick = ROI.begin_index(); iTick <= ROI.end_index(); iTick++ ){                                                                   
	    horig.Fill((int)iTick,ROI[iTick]-ROI[firstTick]);}                    // fill that hist with the waveform              
	  integral = horig.Integral();                                            // take the integral of that waveform              
	  hIntNot.Fill(channel,integral); // fill the histogram of no flipped bits integrals
	  }

	else {  // if there is at least one 1 there is a flipped bit
	  double integral;
          TH1D horig("roi_original", "roi_original;Tick;ADC", endTick + 1 - firstTick, firstTick, endTick + 1);
          for (size_t iTick = ROI.begin_index(); iTick <= ROI.end_index(); iTick++ ){
            horig.Fill((int)iTick,ROI[iTick]-ROI[firstTick]);}
	  TCanvas c1(Form("c1_%d",channel),"c1",900,600);
	  //cout<<ROI[endTick]<<"\n";
	  //c1.cd();
	  //horig.Draw();
	  //  c1.Print(".png");
	  integral = horig.Integral();                                                                                                                     
          hIntFlipped.Fill(channel,integral);   // fill the histogram of at least one flipped bit integrals
	}
       
	
	
      } //end loop over ROIs  

    }//end loop over wires

    f_output.cd();
    auto t_end = high_resolution_clock::now();
    duration<double,std::milli> time_total_ms(t_end-t_begin);
    //cout << "\tEvent took " << time_total_ms.count() << " ms to process." << endl;
    evCtr++;

  }
  //end loop over events!
  f_output.cd();
  
  // executables to be able to set two different color palettes
  TExec *ex1 = new TExec("ex1","gStyle->SetPalette(57)");
  TExec *ex2 = new TExec("ex2","gStyle->SetPalette(80)");
  
  //diff to interpolation
  c7.cd();
  hIntNot.Draw("col");
  ex1->Draw();
  hIntNot.Draw("colz same");
  hIntNot.SetStats(0);
  ex2->Draw();
  hIntFlipped.Draw("colz same");
  hIntFlipped.SetStats(0);
  
  c7.Print(".png");

  delete ex1;
  delete ex2;
//and ... write to file!
  f_output.Write();
  f_output.Close();  
  cout<<endl;
}
