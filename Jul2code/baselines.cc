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


//convenient for us! let's not bother with art and std namespaces!                        
using namespace art;
using namespace std;

using namespace std::chrono;

int main(int argc, char** argv) {

  TFile f_output("baselines_output.root","RECREATE");

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

  // first sample passing the threshold                                             
  TCanvas c1("c_U","c1",1100,400);
  c1.Divide(3,1);
  TCanvas c2("c_V","c2",1100,400);
  c2.Divide(3,1);
  TCanvas c3("c_Y","c3",1100,400);
  c3.Divide(3,1);
  TCanvas c4("c_first","c4",600,900);
  c4.Divide(1,3);

  
  TH1I hFirstPreU("hFirstPreu","First sample passing U threshold - first presample ADC; ADC; Frequency",400,-200,200);
  TH1I hFirstPreV("hFirstPrev","First sample passing V threshold - first presample ADC; ADC; Frequency",400,-200,200);
  TH1I hFirstPreY("hFirstPrey","First sample passing Y threshold - first presample ADC; ADC; Frequency",400,-200,200);
  TH2I hFirstPre("hFirstPre", "First sample passing threshold - first presample ADC; Channel; ADC", 8256, 0, 8256, 400, -200, 200);
  
  TH1I hFirstPostU("hFirstPostu","First sample passing U threshold - last postsample ADC; ADC; Frequency",400,-200,200);
  TH1I hFirstPostV("hFirstPostv","First sample passing V threshold - last postsample ADC; ADC; Frequency",400,-200,200);
  TH1I hFirstPostY("hFirstPosty","First sample passing Y threshold - last postsample ADC; ADC; Frequency",400,-200,200);
  TH2I hFirstPost("hFirstPost","First sample passing threshold - last postsample ADC; Channel; ADC;", 8256, 0, 8256, 400,-200,200);

  TH1I hFirstAlgoU("hFirstAlgou","First sample passing U threshold - algorithm baseline ADC; ADC; Frequency",400,-200,200);
  TH1I hFirstAlgoV("hFirstAlgov","First sample passing V threshold - algorithm baseline ADC; ADC; Frequency",400,-200,200);
  TH1I hFirstAlgoY("hFirstAlgoy","First sample passing Y threshold - algorithm baseline ADC; ADC; Frequency",400,-200,200);
  TH2I hFirstAlgo("hFirstAlgo","First sample passing threshold - algorithm baseline ADC; Channel; ADC;", 8256, 0, 8256, 400,-200,200);

  // last sample passing the threshold                                              
  TCanvas c5("c_Ul","c1",1100,400);
  c5.Divide(3,1);
  TCanvas c6("c_Vl","c2",1100,400);
  c6.Divide(3,1);
  TCanvas c7("c_Yl","c3",1100,400);
  c7.Divide(3,1);
  TCanvas c8("c_last","c4",600,900);
  c8.Divide(1,3);

  TH1I hLastPreU("hLastPreu","Last sample passing U threshold - first presample ADC; ADC; Frequency",400,-200,200);
  TH1I hLastPreV("hLastPrev","Last sample passing V threshold - first presample ADC; ADC; Frequency",400,-200,200);
  TH1I hLastPreY("hLastPrey","Last sample passing Y threshold - first presample ADC; ADC; Frequency",400,-200,200);
  TH2I hLastPre("hLastPre", "Last sample passing threshold - first presample ADC; Channel; ADC", 8256, 0, 8256, 400, -200, 200);

  TH1I hLastPostU("hLastPostu","Last sample passing U threshold - last postsample ADC; ADC; Frequency",400,-200,200);
  TH1I hLastPostV("hLastPostv","Last sample passing V threshold - last postsample ADC; ADC; Frequency",400,-200,200);
  TH1I hLastPostY("hLastPosty","Last sample passing Y threshold - last postsample ADC; ADC; Frequency",400,-200,200);
  TH2I hLastPost("hLastPost","Last sample passing threshold - last postsample ADC; Channel; ADC;", 8256, 0, 8256, 400,-200,200);

  TH1I hLastAlgoU("hLastAlgou","Last sample passing U threshold - algorithm baseline ADC; ADC; Frequency",400,-200,200);
  TH1I hLastAlgoV("hLastAlgov","Last sample passing V threshold - algorithm baseline ADC; ADC; Frequency",400,-200,200);
  TH1I hLastAlgoY("hLastAlgoy","Last sample passing Y threshold - algorithm baseline ADC; ADC; Frequency",400,-200,200);
  TH2I hLastAlgo("hLastAlgo","Last sample passing threshold - algorithm baseline ADC; Channel; ADC;", 8256, 0, 8256, 400,-200,200);


  //channels in V that have negative first samples passing the threshold
  TCanvas c10("c_neg","c10",900,600);
  TH1I hFirstNegV("hFirstNegV","V Channels where the first sample passing the threshold-last post sample is negative; Channel; Frequency",2400,2401,4800);

  // to find the ratio between the positive and negative peaks of the Vplane threshold
  double counterpos=0;
  double counterneg=0;

  size_t  fZSPresamples = 7;
  size_t  fZSPostsamples = 8;

  for (gallery::Event ev(filenames) ; !ev.atEnd(); ev.next()) {
    if(evCtr >= _maxEvts) break;

    auto t_begin = high_resolution_clock::now();

    // int run = ev.eventAuxiliary().run();                                         
    //int event = ev.eventAuxiliary().event();

    //to get run and event info, you use this "eventAuxillary()" object.            
    //    cout << "Processing "                                                     
    //   << "Run " << run << ", "                                                   
    //   << "Event " << event << endl;                                              

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

   //const float maxADCInterpolDiff = 32; // Maximum ADC difference to the interpolation using nearest neigbors to be considered non-flipped bits.
   //size_t ctrROI = 0;

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
			       //		       fDiffPresamplesMedianHist->Fill(channel, ROI[ROI.begin_index() + isample]- medianPresample);
       if(fabs(ROI[ROI.begin_index() + isample]- medianPresample) < medianCut){
	 pretick = ROI.begin_index() + isample;
	 prebaseline = ROI[pretick];
	 //    std::cout << "Prebaseline at tick " << isample << " = " << prebaseline << std::endl;                                                                 
	 break;
       }
     }
     for(size_t isample = 0; isample < fZSPostsamples; isample++){
       //if(fOutputHistograms) if(ROI[ROI.begin_index() + isample] > 1)
       //		       fDiffPostsamplesMedianHist->Fill(channel, ROI[ROI.begin_index() + isample]- medianPostsample);
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
     
     // first sample passing the threshold                                       
     double firstpre;
     firstpre = ROI[firstTick+7]-ROI[firstTick];
     hFirstPre.Fill(channel,firstpre);
     if(channel <= 2400){
       hFirstPreU.Fill(firstpre);}
     else if(channel >2400 && channel <=4800){
       hFirstPreV.Fill(firstpre);
       if (firstpre>=0){counterpos += 1;}
       if (firstpre<0){counterneg += 1;}}
     else
       {hFirstPreY.Fill(firstpre);}

     double firstpost;
     if(ROI[endTick]>1){
       firstpost = ROI[firstTick+7]-ROI[endTick];}
     else 
       {firstpost = ROI[firstTick+7]-ROI[endTick-1];}
     hFirstPost.Fill(channel,firstpost);
     if(channel <= 2400){
       hFirstPostU.Fill(firstpost);}
     else if(channel >2400 && channel <=4800){
       hFirstPostV.Fill(firstpost);
       if(firstpost<0){hFirstNegV.Fill(channel);}
	 //cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<' '<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick]<<"="<<firstpost<<"\n";}
     }
     else
       {hFirstPostY.Fill(firstpost);
	 //if(firstpost<1 && firstpost>=0){ 
	   //  cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<' '<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick]<<"="<<firstpost<<"\n";}
       }
     double firstalgo;
     firstalgo = ROI[firstTick+7]-(slope*(firstTick+7)+intercept);
     hFirstAlgo.Fill(channel,firstalgo);
     if(channel <= 2400){
       hFirstAlgoU.Fill(firstalgo);}
     else if(channel >2400 && channel <=4800){
       hFirstAlgoV.Fill(firstalgo);}
     else
       {hFirstAlgoY.Fill(firstalgo);}


     // last sample passing the threshold                                         
     double lastpre;
     lastpre = ROI[endTick-8]-ROI[firstTick];
     hLastPre.Fill(channel,lastpre);
     if(channel <= 2400){
       hLastPreU.Fill(lastpre);}
     else if(channel >2400 && channel <=4800){
       hLastPreV.Fill(lastpre);}
     else
       {hLastPreY.Fill(lastpre);}

     double lastpost;
     if(ROI[endTick]>0){
       lastpost = ROI[endTick-8]-ROI[endTick];}
     else
       {lastpost = ROI[endTick-8]-ROI[endTick-1];}
     hLastPost.Fill(channel,lastpost);
     if(channel <= 2400){
       hLastPostU.Fill(lastpost);}
     else if(channel >2400 && channel <=4800){
       hLastPostV.Fill(lastpost);}
     else
       {hLastPostY.Fill(lastpost);}

     double lastalgo;
     lastalgo = ROI[endTick-8]-(slope*(endTick-8)+intercept);
     hLastAlgo.Fill(channel,lastalgo);
     if(channel <= 2400){
       hLastAlgoU.Fill(lastalgo);}
     else if(channel >2400 && channel <=4800){
       hLastAlgoV.Fill(lastalgo);}
     else
       {hLastAlgoY.Fill(lastalgo);}



     //  double lastsample;
     //lastsample = ROI[endTick-8]-ROI[endTick];
     // if(channel <= 2400){
     // hLastSamplePassingThresholdu.Fill(lastsample);}
     // else if(channel >2400 && channel <=4800){
     // hLastSamplePassingThresholdv.Fill(lastsample);}
     //else
     //{hLastSamplePassingThresholdy.Fill(lastsample);}

   }
 }
 //f_output.cd();
 auto t_end = high_resolution_clock::now();
 duration<double,std::milli> time_total_ms(t_end-t_begin);
 //cout << "\tEvent took " << time_total_ms.count() << " ms to process." << endl;                                                                                   
 evCtr++;
  }// end loop over events
  f_output.cd();
  
  double ratio;
  cout<<counterpos<<' '<<counterneg<<endl;
  ratio = counterpos/counterneg;
  cout<<ratio<<endl;

  
  //first passing threshold                                                         
  //U plane
  c1.cd(1);
  hFirstPreU.GetYaxis()->SetTitleOffset(2.4);
  hFirstPreU.Draw("hist ][");
  hFirstPreU.SetLineColor(kBlack);
  TLine line(-25,0,-25,18000);
  line.SetLineColor(kRed);
  line.Draw();
  c1.cd(2);
  hFirstPostU.Draw("hist ][");
  hFirstPostU.SetLineColor(kBlack);
  TLine lineI(-25,0,-25,14000);
  lineI.SetLineColor(kRed);
  lineI.Draw();
  c1.cd(3);
  hFirstAlgoU.Draw("hist ][");
  hFirstAlgoU.SetLineColor(kBlack);
  TLine lineII(-25,0,-25,22000);
  lineII.SetLineColor(kRed);
  lineII.Draw();
  c1.cd();
  c1.Write();

  //V plane
  c2.cd(1);
  hFirstPreV.Draw("hist ][");
  hFirstPreV.SetLineColor(kBlack);
  TLine line2(-15,0,-15,27000);
  line2.SetLineColor(kRed);
  line2.Draw();
  TLine line3(15,0,15,27000);
  line3.SetLineColor(kRed);
  line3.Draw();
  c2.cd(2);
  hFirstPostV.Draw("hist ][");
  hFirstPostV.SetLineColor(kBlack);
  TLine line2I(-15,0,-15,11000);
  line2I.SetLineColor(kRed);
  line2I.Draw();
  TLine line3I(15,0,15,11000);
  line3I.SetLineColor(kRed);
  line3I.Draw();
  c2.cd(3);
  hFirstAlgoV.Draw("hist ][");
  hFirstAlgoV.SetLineColor(kBlack);
  TLine line2II(-15,0,-15,30000);
  line2II.SetLineColor(kRed);
  line2II.Draw();
  TLine line3II(15,0,15,30000);
  line3II.SetLineColor(kRed);
  line3II.Draw();
  c2.cd();
  c2.Write();

  //Y plane
  c3.cd(1);
  hFirstPreY.Draw("hist ][");
  hFirstPreY.SetLineColor(kBlack);
  TLine line4(30,0,30,21000);
  line4.SetLineColor(kRed);
  line4.Draw();
  c3.cd(2);
  hFirstPostY.Draw("hist ][");
  hFirstPostY.SetLineColor(kBlack);
  TLine line4I(30,0,30,8000);
  line4I.SetLineColor(kRed);
  line4I.Draw();
  c3.cd(3);
  hFirstAlgoY.Draw("hist ][");
  hFirstAlgoY.SetLineColor(kBlack);
  line4.Draw();
  c3.cd();
  c3.Write();

  //2D
  c4.cd(1);
  hFirstPre.Draw("colz");
  TLine line5(0,-25,2400,-25);
  line5.SetLineColor(kRed);
  line5.Draw();
  TLine line6(2400,15,4800,15);
  line6.SetLineColor(kRed);
  line6.Draw();
  TLine line7(2400,-15,4800,-15);
  line7.SetLineColor(kRed);
  line7.Draw();
  TLine line8(4800,30,8256,30);
  line8.SetLineColor(kRed);
  line8.Draw();
  c4.cd(2);
  hFirstPost.Draw("colz");
  line5.Draw();
  line6.Draw();
  line7.Draw();
  line8.Draw();
  c4.cd(3);
  hFirstAlgo.Draw("colz");
  line5.Draw();
  line6.Draw();
  line7.Draw();
  line8.Draw();
  c4.cd();
  c4.Write();

  //last sample passing threshold
  //U plane
  c5.cd(1);
  hLastPreU.Draw("hist ][");
  hLastPreU.SetLineColor(kBlack);
  TLine line16(-25,0,-25,21000);
  line16.SetLineColor(kRed);
  line16.Draw();
  c5.cd(2);
  hLastPostU.Draw("hist ][");
  hLastPostU.SetLineColor(kBlack);
  TLine line6I(-25,0,-25,16000);
  line6I.SetLineColor(kRed);
  line6I.Draw();
  c5.cd(3);
  hLastAlgoU.Draw("hist ][");
  hLastAlgoU.SetLineColor(kBlack);
  TLine line6II(-25,0,-25,35000);
  line6II.SetLineColor(kRed);
  line6II.Draw();
  c5.cd();
  c5.Write();

  //V plane                                                                                                  
  c6.cd(1);
  hLastPreV.Draw("hist ][");
  hLastPreV.SetLineColor(kBlack);
  //TLine line2(-15,0,-15,27000);
  line2.SetLineColor(kRed);
  line2.Draw();
  //TLine line3(15,0,15,27000);
  line3.SetLineColor(kRed);
  line3.Draw();
  c6.cd(2);
  hLastPostV.Draw("hist ][");
  hLastPostV.SetLineColor(kBlack);
  TLine line12I(-15,0,-15,17000);
  line12I.SetLineColor(kRed);
  line12I.Draw();
  TLine line13I(15,0,15,17000);
  line13I.SetLineColor(kRed);
  line13I.Draw();
  c6.cd(3);
  hLastAlgoV.Draw("hist ][");
  hLastAlgoV.SetLineColor(kBlack);
  TLine line12II(-15,0,-15,34000);
  line12II.SetLineColor(kRed);
  line12II.Draw();
  TLine line13II(15,0,15,34000);
  line13II.SetLineColor(kRed);
  line13II.Draw();
  c6.cd();
  c6.Write();

  //Y plane                                                                                                     
  c7.cd(1);
  hLastPreY.Draw("hist ][");
  hLastPreY.SetLineColor(kBlack);
  //TLine line4(30,0,30,21000);
  line4.SetLineColor(kRed);
  line4.Draw();
  c7.cd(2);
  hLastPostY.Draw("hist ][");
  hLastPostY.SetLineColor(kBlack);
  //TLine line4I(30,0,30,8000);
  line4I.SetLineColor(kRed);
  line4I.Draw();
  c7.cd(3);
  hLastAlgoY.Draw("hist ][");
  hLastAlgoY.SetLineColor(kBlack);
  line4.Draw();
  c7.cd();
  c7.Write();

  //2D                                                                                                    
  c8.cd(1);
  hLastPre.Draw("colz");
  //TLine line5(0,-25,8256,-25);
  line5.SetLineColor(kRed);
  line5.Draw();
  //TLine line6(0,15,8256,15);
  line6.SetLineColor(kRed);
  line6.Draw();
  //TLine line7(0,-15,8256,-15);
  line7.SetLineColor(kRed);
  line7.Draw();
  //TLine line8(0,30,8256,30);
  line8.SetLineColor(kRed);
  line8.Draw();
  c8.cd(2);
  hLastPost.Draw("colz");
  line5.Draw();
  line6.Draw();
  line7.Draw();
  line8.Draw();
  c8.cd(3);
  hLastAlgo.Draw("colz");
  line5.Draw();
  line6.Draw();
  line7.Draw();
  line8.Draw();
  c8.cd();
  c8.Write();

  c10.cd();
  hFirstNegV.Draw("hist ][");
  c10.Print(".png");
  
  //and ... write to file!                                                            
  f_output.Write();
  f_output.Close();

  cout<<"success";
}
