
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

   TFile f_output("waveanalysis_output.root","RECREATE");
  
  //We specify our files in a list of file names!
  //Note: multiple files allowed. Just separate by comma.
  vector<string> filenames { argv[1] };

  
  //Check the contents of your file by setting up a version of uboonecode, and
  //running an event dump:
  //  'lar -c eventdump.fcl -s MyInputFile_1.root -n 1'
  //  InputTag wire_tag { "sndaq", "SupernovaAssembler" }; // does not work
  InputTag wire_tag { "sndaq", "", "SupernovaAssembler" }; // before deconvolution
  // InputTag wire_tag { "sndeco", "", "CalDataSN" }; // after deconvolution


  size_t _maxEvts = 1;
  size_t evCtr = 0;
 

  //histograms in 3 plans for first-last plots    
  TCanvas c1("c1","c1",900,400);
  c1.Divide(3,1);
  TH1I hDiffFirstLastSampleU("hDiffFirstLastSampleu", "First - last ADC U; First - last (ADC); Frequency", 8192, -4096, 4096);
  TH1I hDiffFirstLastSampleV("hDiffFirstLastSamplev", "First - last ADC V; First - last (ADC); Frequency", 8192, -4096, 4096);
  TH1I hDiffFirstLastSampleY("hDiffFirstLastSampley", "First - last ADC Y; First - last (ADC); Frequency", 8192, -4096, 4096);

  // histogram of means
  TCanvas c2("c2","c2",900,600);
  TH2F hMean("hMean", "Mean; Channel; Mean (ADC)", 8256, 0, 8256, 4096, 0, 4096); 

  //histogram of variance
  TCanvas c3("c3","c3",900,600);
  TH2F hVariance("hVariance", "FPGA-like variance; Channel; Variance (ADC^{2})", 8256, 0, 8256, 4096, 0, 4096);       
  
  //histogram of the integral of the signal
  TCanvas c4("c4","c4",900,600);
  TH2I hInt("hInt", "ROI integral (baseline subtracted using 1st sample); Channel; ROI integral (ADC)", 8256, 0, 8256, 8192, -4096, 4096);

  // first sample passing the threshold
  TCanvas c5("c5","c5",1100,500);
  c5.Divide(3,1);
  TH1I hFirstSamplePassingThresholdu("hFirstSamplePassingThresholdu","First sample passing U threshold - first presample ADC; ADC; Frequency",400,-200,200);
  TH1I hFirstSamplePassingThresholdv("hFirstSamplePassingThresholdv","First sample passing V threshold - first presample ADC; ADC; Frequency",400,-200,200);
  TH1I hFirstSamplePassingThresholdy("hFirstSamplePassingThresholdy","First sample passing Y threshold - first presample ADC; ADC; Frequency",400,-200,200);

  // last sample passing the threshold 
  TCanvas c6("c6","c6",1100,500);
  c6.Divide(3,1);
  TH1I  hLastSamplePassingThresholdu("hLastSamplePassingThresholdu", "Last sample passing threshold - last postsample ADC; Channel; ADC", 400, -200, 200);
  TH1I  hLastSamplePassingThresholdv("hLastSamplePassingThresholdv", "Last sample passing threshold - last postsample ADC; Channel; ADC", 400, -200, 200);
  TH1I  hLastSamplePassingThresholdy("hLastSamplePassingThresholdy", "Last sample passing threshold - last postsample ADC; Channel; ADC", 400,-200, 200);

  // difference to interpolation
  TCanvas c7("c7","c7",900,600);
  TH2F hDiffToInterpol("hDiffToInterpol", "Difference to interpolation; Channel; ADC_{i} - (ADC_{i+1} + ADC_{i-1})/2 (ADC)", 8256, 0, 8256, 4096, 0, 4096);
  
  // cumulative length of ROIs per frame
  TCanvas c8("c8","c8",900,600);
  TH2I hLengthFrame("hLengthFrame", "Cumulative length of ROIs per frame; Channel; Cumulative length/frame (TDC)", 8256, 0, 8256, 3200, 0, 3200);

  // first presample
  TCanvas c9("c9","c9",700,900);
  c9.Divide(1,2);
  TH2I hBaselineFirstSample("hBaselineFirstSample", "First presample ADC; Channel; First sample (ADC)", 8256, 0, 8256, 4096, 0, 4096);
  TH2I hBaselineLastSample("hBaselineLastSample", "Last postsample ADC; Channel; Last sample (ADC)", 8256, 0, 8256, 4096, 0, 4096);

  // distribution of tick values
  TCanvas c10("c10","c10",900,600);
  TH2F hTick("hTick", "Tick value of first ROI sample; Channel; Tick", 8256, 0, 8256, 3200, 0, 3200);


  // 
  for (gallery::Event ev(filenames) ; !ev.atEnd(); ev.next()) {
    if(evCtr >= _maxEvts) break;

    auto t_begin = high_resolution_clock::now();
    
    // int run = ev.eventAuxiliary().run();
    int event = ev.eventAuxiliary().event();
 
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

    TH1I hInterpolU(Form("hinterpolu_event%d",event),Form("Event %d Difference to interpolation U; ADC_{i} - (ADC_{i+1} + ADC_{i-1})/2 (ADC); Frequency",event), 3200, 0, 3200);//changed from 4096 to zoom in
    TH1I hInterpolV(Form("hinterpolv_event%d",event),Form("Event %d Difference to interpolation V; ADC_{i} - (ADC_{i+1} + ADC_{i-1})/2 (ADC); Frequency",event ), 3200, 0, 3200);
    TH1I hInterpolY(Form("hinterpoly_event%d",event),Form("Event %d Difference to interpolation Y; ADC_{i} - (ADC_{i+1} + ADC_{i-1})/2 (ADC); Frequency",event), 3200, 0, 3200);

    TCanvas c11(Form("c_event%d",event),Form("c_event%d",event),900,400);
    c11.Divide(3,1);

    for (unsigned int i=0; i<wire_vec.size();i++){
      auto zsROIs = wire_vec[i].SignalROI();
      int channel = wire_vec[i].Channel();
      
      //cumulative length of ROIs 
      double lengthperframe=0;
      
      for (auto iROI = zsROIs.begin_range(); iROI != zsROIs.end_range(); ++iROI) {
        auto ROI = *iROI;
        const size_t firstTick = ROI.begin_index();
        const size_t endTick = ROI.end_index();
	

	// histogram of first - last ROI signal for each of the 3 planes
	double diff;
	if (ROI[endTick]>1){    // to make sure we're not getting any of the zeros
	  diff = ROI[firstTick]-ROI[endTick];}
	if(channel <= 2400){
	  hDiffFirstLastSampleU.Fill(diff);} 
	else if(channel>2400 && channel <=4800){
	  hDiffFirstLastSampleV.Fill(diff);}
	else 
	  {hDiffFirstLastSampleY.Fill(diff);}

	
	// mean
	double mean;
	mean=TMath::Mean(ROI.begin(),ROI.end());
	hMean.Fill(channel,mean);
      

	// variance
	double stdev,variance;
	stdev = TMath::StdDev(ROI.begin(),ROI.end());
        variance = pow(stdev,2);
	hVariance.Fill(channel,variance);
	

	// integral
	double integral;
	TH1D horig("roi_original", "roi_original;Tick;ADC", endTick + 1 - firstTick, firstTick, endTick + 1);
        for (size_t iTick = ROI.begin_index(); iTick <= ROI.end_index(); iTick++ ){
	  horig.Fill((int)iTick,ROI[iTick]-ROI[firstTick]);}
	integral = horig.Integral();
	hInt.Fill(channel,integral);

	
	// first sample passing the threshold
	double firstsample;
	firstsample = ROI[firstTick+7]-ROI[firstTick];
	if(channel <= 2400){
	  hFirstSamplePassingThresholdu.Fill(firstsample);}
	else if(channel >2400 && channel <=4800){
	  hFirstSamplePassingThresholdv.Fill(firstsample);}
	else
	    {hFirstSamplePassingThresholdy.Fill(firstsample);}

	
	// last sample passing the threshold
	double lastsample;
	lastsample = ROI[endTick-8]-ROI[endTick];
        if(channel <= 2400){
          hLastSamplePassingThresholdu.Fill(lastsample);}
        else if(channel >2400 && channel <=4800){
          hLastSamplePassingThresholdv.Fill(lastsample);}
        else
	  {hLastSamplePassingThresholdy.Fill(lastsample);}


	// difference to interpolation
	double difftoint;
	for (size_t iTick = 1+ROI.begin_index(); iTick < ROI.end_index(); iTick++ ){
	  difftoint = ROI[iTick]-(( ROI[iTick+1] + ROI[iTick-1] )/2);
	  hDiffToInterpol.Fill(channel,difftoint);
	  if(channel <= 2400){
	    hInterpolU.Fill(difftoint);}
	  else if(channel >2400 && channel <=4800){
	    hInterpolV.Fill(difftoint);}
	  else
	    {hInterpolY.Fill(difftoint);}
	}	

	// length per frame                                                                                                 
	lengthperframe += endTick-firstTick;
	

	// first presample
	hBaselineFirstSample.Fill(channel,ROI[firstTick]);
	
	
	// last postsample
	hBaselineLastSample.Fill(channel,ROI[endTick]);
	if(channel > 4800 && ROI[endTick]>1500){
	  cout<<"channel: "<<channel<<"ADC: "<<ROI[endTick]<<"\n";}
	

	// tick value of first sample
	hTick.Fill(channel,firstTick);


      } //end loop over ROI  
      
      hLengthFrame.Fill(channel,lengthperframe);
    }//end loop over wires

    f_output.cd();
    auto t_end = high_resolution_clock::now();
    duration<double,std::milli> time_total_ms(t_end-t_begin);
    //cout << "\tEvent took " << time_total_ms.count() << " ms to process." << endl;
    evCtr++;

    // diff to interpolation in 3 plots
    c11.cd(1);
    hInterpolU.Draw("hist ][");
    hInterpolU.GetYaxis()->SetRangeUser(0,1000);
    c11.cd(2);
    hInterpolV.Draw("hist ][");
    hInterpolV.GetYaxis()->SetRangeUser(0,1000);
    c11.cd(3);
    hInterpolY.Draw("hist ][");
    hInterpolY.GetYaxis()->SetRangeUser(0,1000);
    c11.cd();
    c11.Write();
  }
  //end loop over events!
  f_output.cd();

  //diff
  c1.cd(1);
  hDiffFirstLastSampleU.Draw("hist ][");
  hDiffFirstLastSampleU.SetLineColor(kBlack);
  // hDiffFirstLastSampleU.GetXaxis()->SetRangeUser(-100,100);    //if you wanted to zoom in to better see the width of the plots  
  c1.cd(2);
  hDiffFirstLastSampleV.Draw("hist ][");
  hDiffFirstLastSampleV.SetLineColor(kBlack);
  //hDiffFirstLastSampleV.GetXaxis()->SetRangeUser(-100,100);                  
  c1.cd(3);
  hDiffFirstLastSampleY.Draw("hist ][");
  hDiffFirstLastSampleY.SetLineColor(kBlack);
  //hDiffFirstLastSampleY.GetXaxis()->SetRangeUser(-100,100);
  c1.cd();
  c1.Write();
  //  c1.Print("diffzoom.png");

  //mean
  c2.cd();
  hMean.Draw("colz");
  c2.Write();
  
  //variance
  c3.cd();
  hVariance.Draw("colz");
  hVariance.SetLineColor(kBlack);
  c3.Write();

  //integral
  c4.cd();
  hInt.Draw("colz");
  c4.Write();
 
  //first passing threshold
  c5.cd(1);
  hFirstSamplePassingThresholdu.Draw("hist ][");
  hFirstSamplePassingThresholdu.SetLineColor(kBlack);               
  TLine line(-25,0,-25,18000);
  line.SetLineColor(kRed);
  line.Draw();
  c5.cd(2);
  hFirstSamplePassingThresholdv.Draw("hist ][");
  hFirstSamplePassingThresholdv.SetLineColor(kBlack);
  TLine line2(-15,0,-15,27000);
  line2.SetLineColor(kRed);
  line2.Draw();
  TLine line3(15,0,15,27000);
  line3.SetLineColor(kRed);
  line3.Draw();
  c5.cd(3);
  hFirstSamplePassingThresholdy.Draw("hist ][");
  hFirstSamplePassingThresholdy.SetLineColor(kBlack);
  TLine line4(30,0,30,21000);
  line4.SetLineColor(kRed);
  line4.Draw();
  c5.cd();
  c5.Write();
 
  //last passing threshold
  c6.cd(1);
  hLastSamplePassingThresholdu.Draw("hist ][");
  hLastSamplePassingThresholdu.SetLineColor(kBlack);
  c6.cd(2);
  hLastSamplePassingThresholdv.Draw("hist ][");
  hLastSamplePassingThresholdv.SetLineColor(kBlack);
  c6.cd(3);
  hLastSamplePassingThresholdy.Draw("hist ][");
  hLastSamplePassingThresholdy.SetLineColor(kBlack);
  c6.cd();
  c6.Write();

  //diff to interpolation
  c7.cd();
  hDiffToInterpol.Draw("colz");
  for(int i=0;i<14;i++)
    {TLine *lines = new TLine();
      lines->SetLineColor(13);
      lines->DrawLine(0,pow(2,i),8256,pow(2,i));
      delete lines;}
  c7.Write();

  //ROI/frame
  c8.cd();
  hLengthFrame.Draw("colz");
  c8.Write(); 

  //first baseline
  c9.cd(1);
  hBaselineFirstSample.Draw("colz");
  c9.cd(2);
  hBaselineLastSample.Draw("colz");
  c9.cd();
  c9.Write();

  //first tick value of sample
  c10.cd();
  hTick.Draw("colz");
  c10.Write();

//and ... write to file!
  f_output.Write();
  f_output.Close();

  cout<<"success";  
}
