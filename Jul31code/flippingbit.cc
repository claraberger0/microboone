
//***************************
//    flipped integral analysis
//    Clara Berger 7/16/18
//***************************


//some standard C++ includes
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <chrono>
#include <math.h>
#include <cmath>

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

   TFile f_output("flippingbit_output.root","RECREATE");
  
  //We specify our files in a list of file names!
  //Note: multiple files allowed. Just separate by comma.
  vector<string> filenames { argv[1] };

  
  //Check the contents of your file by setting up a version of uboonecode, and
  //running an event dump:
  //  'lar -c eventdump.fcl -s MyInputFile_1.root -n 1'
  //  InputTag wire_tag { "sndaq", "SupernovaAssembler" }; // does not work
  InputTag wire_tag { "sndaq", "", "SupernovaAssembler" }; // before deconvolution
  InputTag wire_tag_d { "sndeco", "", "CalDataSN" }; // after deconvolution


  size_t _maxEvts = 65;
  size_t evCtr = 0;
 

  // difference to interpolation
  TCanvas c7("notflippedint","c7",900,600);
  TH2F hIntFlipped("hIntFlipped", "ROI Integral with Flipped Bit; Channel; ROI integral (ADC)", 8256, 0, 8256, 8000, 0, 8000);
  TH2F hIntNot("hIntNot", "ROI Integral No Flipped Bit; Channel; ROI integral (ADC)", 8256, 0, 8256, 8000, 0, 8000);
  TCanvas c8("flippedint","c8",900,600);
  

  TCanvas c4("notflippedint_decon","c4",900,600);
  TH2F hIntFlipped_d("hIntFlipped_decon", "Deconvoluted ROI Integral with Flipped Bit; Channel; ROI integral (ADC)", 8256, 0, 8256, 4000, 0, 4000);
  TH2F hIntNot_d("hIntNot_decon", " Deconvoluted ROI Integral No Flipped Bit; Channel; ROI integral (ADC)", 8256, 0, 8256, 4000, 0, 4000);
  TCanvas c5("flippedint_decon","c5",900,600);


  // how many times and ROI in a channel had a flipped bit 
  int numflipped[8256];
  int x[8256];
  TCanvas c1("flippedchannel","c1",900,600);
  // TH1F hNumFlipped("hNumFlipped", "Frequency of Flipped Bits by Channel; Channel; # of times there was a flipped bit", 8256, 0, 8256);

  //how long are the ROIs in the channels
  TCanvas c2("roilength","c2",900,600);
  TH2F hRoiLen("hRoiLen", "Lengths of ROIs by Channel; Channel; Length of ROI (Ticks)", 8256, 0, 8256, 2000, 0, 2000);

  // integral vs. length of ROI
  TCanvas c3("lengthintegral","c3",900,600);
  TH2F hIntLen("hIntLen", "ROI Integral; Length of ROI (Ticks); ROI Integral (ADC)", 400, 0, 400, 10000, 0, 10000);



  size_t  fZSPresamples = 7;
  size_t  fZSPostsamples = 8;

  for (gallery::Event ev(filenames) ; !ev.atEnd(); ev.next()) {
    if(evCtr >= _maxEvts) break;

    auto t_begin = high_resolution_clock::now();     
    // int run = ev.eventAuxiliary().run();
    int event = ev.eventAuxiliary().event();
 
    //to get run and event info, you use this "eventAuxillary()" object.
    //    cout << "Processing "
    //	 << "Run " << run << ", "
    //	 << "Event " << event << endl;

   
    //Now, we want to get a "valid handle" (which is like a pointer to our collection")
    //We use auto, cause it's annoying to write out the fill type. But it's like
    //vector<recob::Wire>* object.
    auto const& wire_handle = ev.getValidHandle< vector<recob::Wire> >(wire_tag);
    auto const& wire_handle_d = ev.getValidHandle< vector<recob::Wire> >(wire_tag_d);


    //We can now treat this like a pointer, or dereference it to have it be like a vector.
    //I (Wes) for some reason prefer the latter, so I always like to do ...
    auto const& wire_vec(*wire_handle);
    auto const& wire_vec_d(*wire_handle_d);

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
		
	//********************************************** begin baseline algorithm***********************************                                            
	// Find the median of presamples/postsamples                                                                                                            
	std::vector<float> presamples; presamples.reserve(fZSPresamples);
	std::vector<float> postsamples; postsamples.reserve(fZSPostsamples);
	for(size_t isample = 0; isample < fZSPresamples; isample++){
	  if(ROI[ROI.begin_index() + isample] > 1){ // Get rid of 1e-44 and similar swizzler errors(?)                                                          
	    presamples.push_back(ROI[ROI.begin_index() + isample]);
	    //            std::cout << "Presample " << isample << " = " << presamples.back() << std::endl;                                                      
	  }
	}
	for(size_t isample = 0; isample < fZSPostsamples; isample++){
	  if(ROI[ROI.end_index() - fZSPostsamples + 1 + isample] > 1){
	    postsamples.push_back(ROI[ROI.end_index() - fZSPostsamples + 1 + isample]);
	    //            std::cout << "Postsample " << isample << " = " << postsamples.back() << std::endl;                                                    
	  }
	}

	std::sort(presamples.begin(), presamples.end());
	std::sort(postsamples.begin(), postsamples.end());

	const size_t goodPresamples = presamples.size();
	const size_t goodPostsamples = postsamples.size();

	const float medianPresample = ((goodPresamples % 2) == 0)?
	  (presamples[goodPresamples/2 - 1] + presamples[goodPresamples/2])/2. : presamples[goodPresamples/2];
	//            std::cout << "Median presample " << medianPresample << std::endl;                                                                         
	const float medianPostsample = ((goodPostsamples % 2) == 0)?
	  (postsamples[goodPostsamples/2 - 1] + postsamples[goodPostsamples/2])/2. : postsamples[goodPostsamples/2];
	//            std::cout << "Median postsample " << medianPostsample << std::endl;                                                                       
	// Get the earliest presample and last postsample which are close to the medians                                                                       
	float prebaseline = -4095; // baseline estimated from presamples                                                                                     
	float postbaseline = -4095; // baseline estimated from postsamples                                                                                   
	size_t pretick = -1; // tick with baseline estimated from presamples                                                                                

	size_t postick = -1; // tick with baseline estimated from postsamples                                                                                
	const float medianCut = 15; // Maximum absolute difference between a sample and the median to be considered as baseline // Add to FHiCL file?      
	  for(size_t isample = 0; isample < fZSPresamples; isample++){
	    //if(fOutputHistograms) if(ROI[ROI.begin_index() + isample] > 1)                                                                                      
	    //                      fDiffPresamplesMedianHist->Fill(channel, ROI[ROI.begin_index() + isample]- medianPresample);          
	    if(fabs(ROI[ROI.begin_index() + isample]- medianPresample) < medianCut){
	      pretick = ROI.begin_index() + isample;
	      prebaseline = ROI[pretick];
	      //    std::cout << "Prebaseline at tick " << isample << " = " << prebaseline << std::endl;                                                    
	      break;
	    }
	  }
	  for(size_t isample = 0; isample < fZSPostsamples; isample++){
	    //if(fOutputHistograms) if(ROI[ROI.begin_index() + isample] > 1)                                                                                      
	    //                      fDiffPostsamplesMedianHist->Fill(channel, ROI[ROI.begin_index() + isample]- medianPostsample);                                
	    if(fabs(ROI[ROI.end_index() -  isample] - medianPostsample) < medianCut){
	      postick = ROI.end_index() - isample;
	      postbaseline = ROI[postick];
	      //            std::cout << "Postbaseline at tick -" << isample << " = " << postbaseline << std::endl;                                         
	      break;
	    }
	  }

	  // Linear interpolation for baseline                                                                                                                    
	  const float slope = (postbaseline - prebaseline)/(postick - pretick);
	  //const float intercept = (postick*prebaseline - postbaseline*pretick)/(postick - pretick);                                                             
	  const float intercept = prebaseline - slope*pretick;

	  //*******************************************end baseline algorightm*************************************************      


	// define vector of 0s and 1s that will say whether there is a flipped bit or not 
	vector<int> flippedROI;

	// Difference to interpolation
	double difftoint;
	for (size_t iTick = 1+ROI.begin_index(); iTick < ROI.end_index()-1; iTick++ ){   ///loop through values in the ROI excluding the second to last tick as well
	  difftoint = ROI[iTick] - (( ROI[iTick+1] + ROI[iTick-1] )/2);
	  
	  if(difftoint > 32){              // our metric of flipped bit is a difference of more than 32
	    flippedROI.push_back(1);}    // add a 1 if there is a flipped bit
	  else
	    {flippedROI.push_back(0);}    // add a 0 if there isnt a flipped bit
	}
      
	//cout<<flippedROI.size();
	//for(int i:flippedROI){
	//cout<<' '<<i<<' ';}
	

	double roilength;  // find the length of each ROI
        roilength = endTick-firstTick;
        hRoiLen.Fill(channel,roilength);

	// check if every value in flippedROI is 0 which would mean there is no flipped bit
	if( std::all_of(flippedROI.begin(), flippedROI.end(), [](int x){return x==0;})){   
	  double integral;                                                                                                                                  
	  TH1D horig("roi_original", "roi_original;Tick;ADC", endTick - firstTick, firstTick, endTick); // new hist of the waveform
	  for (size_t iTick = ROI.begin_index(); iTick < ROI.end_index(); iTick++ ){    //not including last sample                   
	    double absvalue;
	    absvalue = abs(ROI[iTick]-(slope*(iTick)+intercept));  // subtract the algorithm baseline and then take the absolue value
	    horig.Fill((int)iTick,absvalue);}                    // fill that hist with the waveform              
	  integral = horig.Integral();                                            // take the integral of that waveform              
	  hIntNot.Fill(channel,integral); // fill the histogram of no flipped bits integrals
	  TCanvas c1(Form("c1_%d_%d",event,channel),"c1",900,600);
	  horig.Draw("hist ]");
	  // c1.Print(".png");

	}

	else {  // if there is at least one 1 there is a flipped bit
	  double integral;
          TH1D horig("roi_original", "roi_original;Tick;ADC", endTick - firstTick, firstTick, endTick);
	  for (size_t iTick = ROI.begin_index(); iTick < ROI.end_index(); iTick++ ){
	    double absvalue;
	    absvalue = abs(ROI[iTick]-(slope*(iTick)+intercept));
	    horig.Fill((int)iTick,absvalue);}
	  TCanvas c1(Form("c1_%d_%d_f",event,channel),"c1",900,600);
	  //cout<<"channel: "<<channel<<' '<< abs(ROI[endTick]-(slope*(endTick)+intercept))<<"\n";
	  horig.Draw("hist ]");
	  // c1.Print(".png");
	  integral = horig.Integral();                                                                                                               
          hIntFlipped.Fill(channel,integral);   // fill the histogram of at least one flipped bit integrals

	  numflipped[channel]+= 1;   // if there is a flipped bit add 1 to the number of occurances for the channel
	
	  hIntLen.Fill(roilength,integral);  
	}

	auto zsROIs_d = wire_vec_d[i].SignalROI();
	int channel_d = wire_vec_d[i].Channel();

	//auto ROI_d = zsROIs_d[ROI];
	for (auto iROI_d = zsROIs_d.begin_range(); iROI_d != zsROIs_d.end_range(); ++iROI_d) {
	  auto ROI_d = *iROI_d;
	 
	  if( std::all_of(flippedROI.begin(), flippedROI.end(), [](int x){return x==0;})){
	    double integral;
	    TH1D horig("roi_original", "roi_original;Tick;ADC", endTick - firstTick, firstTick, endTick); // new hist of the waveform                       
	    for (size_t iTick = ROI_d.begin_index(); iTick < ROI_d.end_index(); iTick++ ){    //not including last sample                                   
	      horig.Fill((int)iTick,ROI_d[iTick]);}                    
	    integral = horig.Integral();                                            // take the integral of that waveform                                 
	    hIntNot_d.Fill(channel,integral); // fill the histogram of no flipped bits integrals                                                            
	    
	    TCanvas c1(Form("c1_%d_%d_d",event,channel_d),"c1",900,600);
	    horig.Draw("hist ]");
	    //c1.Print(".png");
	  }
	  else {  // if there is at least one 1 there is a flipped bit                                                                                         
	    double integral;
	    TH1D horig("roi_original", "roi_original;Tick;ADC", endTick - firstTick, firstTick, endTick);
	    for (size_t iTick = ROI_d.begin_index(); iTick < ROI_d.end_index(); iTick++ ){    //not including last sample                              
              horig.Fill((int)iTick,ROI_d[iTick]);}
	    integral = horig.Integral();
            hIntFlipped_d.Fill(channel,integral);   // fill the histogram of at least one flipped bit integrals  
	    
	    TCanvas c1(Form("c1_%d_%d_df",event,channel),"c1",900,600);
	    horig.Draw("hist ]");
	    //c1.Print(".png");                                            
	  }
      }// end loop over deconvoluted 
	

	
      } //End loop over ROIs  

    }//end loop over wires


    f_output.cd();
    auto t_end = high_resolution_clock::now();
    duration<double,std::milli> time_total_ms(t_end-t_begin);
    //cout << "\tEvent took " << time_total_ms.count() << " ms to process." << endl;
    evCtr++;

  }
  //end loop over events!
  f_output.cd();
  
  
  //diff to interpolation
  c7.cd();
  hIntNot.Draw("colz");
  gStyle->SetPalette(60);
  // hIntNot.SetStats(0);
  c7.Print(".png");

  c8.cd();
  hIntFlipped.Draw("colz");
  gStyle->SetPalette(60);
  //hIntFlipped.SetStats(0);
  c8.Print(".png");

  
  c4.cd();
  hIntNot_d.Draw("colz");
  hIntNot_d.GetZaxis()->SetRangeUser(0,60);
  gStyle->SetPalette(60);
  //hIntNot_d.SetStats(0);
  c4.Print(".png");

  c5.cd();
  hIntFlipped_d.Draw("colz");
  gStyle->SetPalette(60);
  hIntFlipped_d.GetZaxis()->SetRangeUser(0,7);
  //hIntFlipped_d.SetStats(0);
  c5.Print(".png");


  // Just make the double array of all channel numbers                                                                                                      
  for(int n=0; n<8256;n++)
    {x[n]=n;}

  // make a bar graph using totalhits as the height of all 8256 bars each bar is a different channel                                                        
  // this will essentially be a histogram with number of hits per wire on the y axis                                                                        
  TGraph* channelbits  = new TGraph(8256,x,numflipped);
  channelbits->SetTitle("Frequency of Flipped Bits by Channel");
  channelbits->GetXaxis()->SetTitle("Channel");
  channelbits->GetXaxis()->SetRangeUser(0,8256);
  channelbits->GetYaxis()->SetTitle("# of times there was an ROI with a flipped bit");
  channelbits->GetYaxis()->SetTitleOffset(1.4);
  c1.cd();
  channelbits->SetFillColor(1);
  channelbits->Draw("AB");
  //c1.Print(".png");
  //c1.Write();
  delete channelbits;
  
  // roi length
  c2.cd();
  hRoiLen.Draw("colz");
  //c2.Print(".png");

  // length vs. integral
  c3.cd();
  hIntLen.Draw("colz");
  //c3.Print(".png");
  

//and ... write to file!
  f_output.Write();
  f_output.Close();  
  cout<<endl;
}
