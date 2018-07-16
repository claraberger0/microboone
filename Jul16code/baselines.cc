
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
#include "TF1.h"
#include "TStyle.h"
#include "TPaveStats.h"
#include "TList.h"
#include "TText.h"

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
  TH1I hFirstPreV("hFirstPrev","First sample passing V threshold - first presample ADC; ADC; Frequency",100,-50,50);
  TH1I hFirstPreY("hFirstPrey","First sample passing Y threshold - first presample ADC; ADC; Frequency",400,-200,200);
  TH2I hFirstPre("hFirstPre", "First sample passing threshold - first presample ADC; Channel; ADC", 8256, 0, 8256, 400, -200, 200);
  
  TH1I hFirstPostU("hFirstPostu","First sample passing U threshold - last postsample ADC; ADC; Frequency",400,-200,200);
  TH1I hFirstPostV("hFirstPostv","First sample passing V threshold - last postsample ADC; ADC; Frequency",100,-50,50);
  TH1I hFirstPostY("hFirstPosty","First sample passing Y threshold - last postsample ADC; ADC; Frequency",400,-200,200);
  TH2I hFirstPost("hFirstPost","First sample passing threshold - last postsample ADC; Channel; ADC;", 8256, 0, 8256, 400,-200,200);

  TH1I hFirstAlgoU("hFirstAlgou","First sample passing U threshold - algorithm baseline ADC; ADC; Frequency",400,-200,200);
  TH1I hFirstAlgoV("hFirstAlgov","First sample passing V threshold - algorithm baseline ADC; ADC; Frequency",100,-50,50);
  TH1I hFirstAlgoY("hFirstAlgoy","First sample passing Y threshold - algorithm baseline ADC; ADC; Frequency",400,-200,200);
  TH2I hFirstAlgo("hFirstAlgo","First sample passing threshold - algorithm baseline ADC; Channel; ADC;", 8256, 0, 8256, 400,-200,200);

  // last sample passing the threshold                                              
  TCanvas c5("c_Ul","c1",1200,400);
  c5.Divide(3,1);
  TCanvas c6("c_Vl","c2",1200,400);
  c6.Divide(3,1);
  TCanvas c7("c_Yl","c3",1200,400);
  c7.Divide(3,1);
  TCanvas c8("c_last","c4",600,900);
  c8.Divide(1,3);

  TH1I hLastPreU("hLastPreu","Last sample passing U threshold - first presample ADC; ADC; Frequency",400,-200,200);
  TH1I hLastPreV("hLastPrev","Last sample passing V threshold - first presample ADC; ADC; Frequency",100,-50,50);
  TH1I hLastPreY("hLastPrey","Last sample passing Y threshold - first presample ADC; ADC; Frequency",400,-200,200);
  TH2I hLastPre("hLastPre", "Last sample passing threshold - first presample ADC; Channel; ADC", 8256, 0, 8256, 400, -200, 200);

  TH1I hLastPostU("hLastPostu","Last sample passing U threshold - last postsample ADC; ADC; Frequency",400,-200,200);
  TH1I hLastPostV("hLastPostv","Last sample passing V threshold - last postsample ADC; ADC; Frequency",100,-50,50);
  TH1I hLastPostY("hLastPosty","Last sample passing Y threshold - last postsample ADC; ADC; Frequency",400,-200,200);
  TH2I hLastPost("hLastPost","Last sample passing threshold - last postsample ADC; Channel; ADC;", 8256, 0, 8256, 400,-200,200);

  TH1I hLastAlgoU("hLastAlgou","Last sample passing U threshold - algorithm baseline ADC; ADC; Frequency",400,-200,200);
  TH1I hLastAlgoV("hLastAlgov","Last sample passing V threshold - algorithm baseline ADC; ADC; Frequency",100,-50,50);
  TH1I hLastAlgoY("hLastAlgoy","Last sample passing Y threshold - algorithm baseline ADC; ADC; Frequency",400,-200,200);
  TH2I hLastAlgo("hLastAlgo","Last sample passing threshold - algorithm baseline ADC; Channel; ADC;", 8256, 0, 8256, 400,-200,200);


  //channels in V that have negative first samples passing the threshold
  TCanvas c10("c_neg","c10",900,600);
  TH1I hFirstNegV("hFirstNegV","V Channels where the first sample passing the threshold-last post sample is negative; Channel; Frequency",2400,2401,4800);

  // to find the ratio between the positive and negative peaks of the Vplane threshold
   double counterpos=0;
  double counterneg=0;

  // fit two gaussians to the V planes -- define their functions
  TF1 *g1 = new TF1("gfpre1","gaus",-34,-22);
  TF1 *g2 = new TF1("gfpre2","gaus",8,25);
  TF1 *g3 = new TF1("gfpost1","gaus",-28,-5);
  TF1 *g4 = new TF1("gfpost2","gaus",8,30);
  TF1 *g5 = new TF1("gfalg1","gaus",-40,-10);
  TF1 *g6 = new TF1("gfalg2","gaus",10,25);

  TF1 *g7 = new TF1("glpre1","gaus",-20,-6);
  TF1 *g8 = new TF1("glpre2","gaus",6,20);
  TF1 *g9 = new TF1("glpost1","gaus",-25,0);
  TF1 *g10 = new TF1("glpost2","gaus",10,35);
  TF1 *g11 = new TF1("glalg1","gaus",-25,-5);
  TF1 *g12 = new TF1("glalg2","gaus",8,30);

  
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

   }
 }
 //f_output.cd();
 auto t_end = high_resolution_clock::now();
 duration<double,std::milli> time_total_ms(t_end-t_begin);
 //cout << "\tEvent took " << time_total_ms.count() << " ms to process." << endl;                                                                                   
 evCtr++;
  }// end loop over events
  f_output.cd();
  
  //  double ratio;
  // cout<<counterpos<<' '<<counterneg<<endl;
  //ratio = counterpos/counterneg;
  //cout<<ratio<<endl;

  
  //first passing threshold********************************************************************************                   
  //U plane
  c1.cd(1);
  hFirstPreU.GetYaxis()->SetTitleOffset(2);
  hFirstPreU.Fit("gaus","","",-43,-20);
  gStyle->SetOptFit(1);
  hFirstPreU.SetLineColor(kBlack);
  TLine line(-25,0,-25,18000);
  line.SetLineColor(kRed);
  line.Draw();
  c1.cd(2);
  hFirstPostU.Fit("gaus","","",-35,-11);
  hFirstPostU.GetYaxis()->SetTitleOffset(2);
  gStyle->SetOptFit(1);
  hFirstPostU.SetLineColor(kBlack);
  TLine lineI(-25,0,-25,21000);
  lineI.SetLineColor(kRed);
  lineI.Draw();
  c1.cd(3);
  hFirstAlgoU.Fit("gaus","","",-39,-19);
  gStyle->SetOptFit(1);
  hFirstAlgoU.GetYaxis()->SetTitleOffset(2);
  hFirstAlgoU.SetLineColor(kBlack);
  TLine lineII(-25,0,-25,22000);
  lineII.SetLineColor(kRed);
  lineII.Draw();
  c1.cd();
  c1.Write();
  c1.Print(".png");

  //V plane
  // first sample - presample
  TH1F *hFirstPreV2 = (TH1F*)hFirstPreV.Clone();  // make a copy of the histogram to use for two separate peaks to get two statistics boxes
  hFirstPreV2->SetNameTitle("Positive Peak","hey");
  hFirstPreV.SetNameTitle("Negative Peak","First sample passing V threshold - first presample ADC");

  c2.cd(1);
  // gStyle->SetOptStat(11); // regular stats box only has the name and number of entries -- only turn on for fit because it will affect the other plots
  hFirstPreV.Draw("");
  gStyle->SetOptFit(1);   // statistics box with the info from the fit without errors  
  g1->SetLineColor(kBlue); // make the fit blue to distinguish it
  hFirstPreV.Fit(g1,"R"); // negative peak 
  hFirstPreV.GetYaxis()->SetTitleOffset(2);
  c2.Update();
  TPaveStats *st = (TPaveStats*)hFirstPreV.FindObject( "stats" ); // get that stats box
  if (st){                      // move the statistics box so that you can see both boxes at the same time        
    st->SetX1NDC(0.68);                                                                                                    
    st->SetX2NDC(1.0);                                                                                                   
    st->SetY1NDC(0.6);                                                                                             
    st->SetY2NDC(0.9);                                                                                                  
  }           

  hFirstPreV2->Draw("sames");  // positive peak
  gStyle->SetOptFit(1);
  g2->SetLineColor(kGreen);
  hFirstPreV2->Fit(g2,"R");
  c2.Update();
  TPaveStats *st2 = (TPaveStats*)hFirstPreV2->FindObject( "stats" );
  if (st2){
    st2->SetX1NDC(0.68);
    st2->SetX2NDC(1.0);
    st2->SetY1NDC(0.3);
    st2->SetY2NDC(0.6);
  }
  hFirstPreV2->SetLineColor(kBlack);
  TLine line2(-15,0,-15,27000);  // draw lines showing the thresholds
  line2.SetLineColor(kRed);
  line2.Draw();
  TLine line3(15,0,15,27000);
  line3.SetLineColor(kRed);
  line3.Draw();
  // first sample - postsample
  c2.cd(2);
  TH1F *hFirstPostV2 = (TH1F*)hFirstPostV.Clone();  // make a copy of the histogram to use for two separate peaks to get two statistics boxes                                                                                                         
  hFirstPostV2->SetNameTitle("Positive Peak","hey");
  hFirstPostV.SetNameTitle("Negative Peak","First sample passing V threshold - last postsample ADC");

  // gStyle->SetOptStat(11); // regular stats box only has the name and number of entries  
  hFirstPostV.Draw();
  gStyle->SetOptFit(1);   // statistics box with the info from the fit without errors    
  g3->SetLineColor(kBlue);
  hFirstPostV.Fit(g3,"R");
  hFirstPostV.GetYaxis()->SetTitleOffset(2);
  g3->SetLineColor(kBlue);
  gStyle->SetOptFit(1);
  c2.Update();
  TPaveStats *st3 = (TPaveStats*)hFirstPostV.FindObject( "stats" ); // get that stats box                                  
  c2.Modified();
  if (st3){
    st3->SetX1NDC(0.68);
    st3->SetX2NDC(1.0);
    st3->SetY1NDC(0.6);
    st3->SetY2NDC(0.9);
  }

  hFirstPostV2->Draw("sames");
  gStyle->SetOptFit(1);
  g4->SetLineColor(kGreen);
  hFirstPostV2->Fit(g4,"R");
  c2.Update();
  TPaveStats *st4 = (TPaveStats*)hFirstPostV2->FindObject( "stats" );
  if (st4){
    st4->SetX1NDC(0.68);
    st4->SetX2NDC(1.0);
    st4->SetY1NDC(0.3);
    st4->SetY2NDC(0.6);
  }
  hFirstPostV2->SetLineColor(kBlack);
  TLine line2I(-15,0,-15,20000);
  line2I.SetLineColor(kRed);
  line2I.Draw();
  TLine line3I(15,0,15,20000);
  line3I.SetLineColor(kRed);
  line3I.Draw();

  //first sample-algorithm baseline
  c2.cd(3);
  TH1F *hFirstAlgoV2 = (TH1F*)hFirstAlgoV.Clone();  // make a copy of the histogram to use for two separate peaks to get twostatistics boxes                                                                                                            
  hFirstAlgoV2->SetNameTitle("Positive Peak","hey");
  hFirstAlgoV.SetNameTitle("Negative Peak","First sample passing V threshold - algorithm baseline ADC");
  //gStyle->SetOptStat(11); // regular stats box only has the name and number of entries           
  hFirstAlgoV.Draw();
  gStyle->SetOptFit(1);   // statistics box with the info from the fit without errors            
  g5->SetLineColor(kBlue);
  hFirstAlgoV.Fit(g5,"R");
  hFirstAlgoV.GetYaxis()->SetTitleOffset(2);
  g5->SetLineColor(kBlue);
  gStyle->SetOptFit(1);
  c2.Update();
  TPaveStats *st5 = (TPaveStats*)hFirstAlgoV.FindObject( "stats" ); // get that stats box                              
  if (st5){
   st5->SetX1NDC(0.68);
   st5->SetX2NDC(1.0);
   st5->SetY1NDC(0.6);
   st5->SetY2NDC(0.9);
  }
  hFirstAlgoV2->Draw("sames");
  gStyle->SetOptFit(1);
  g6->SetLineColor(kGreen);
  hFirstAlgoV2->Fit(g6,"R");
  hFirstAlgoV2->GetYaxis()->SetTitleOffset(2);
  c2.Update();
  TPaveStats *st6 = (TPaveStats*)hFirstAlgoV2->FindObject( "stats" );
  if (st6){
    st6->SetX1NDC(0.68);
    st6->SetX2NDC(1.0);
    st6->SetY1NDC(0.3);
    st6->SetY2NDC(0.6);
  }
  hFirstAlgoV2->SetLineColor(kBlack);
  TLine line2II(-15,0,-15,30000);
  line2II.SetLineColor(kRed);
  line2II.Draw();
  TLine line3II(15,0,15,30000);
  line3II.SetLineColor(kRed);
  line3II.Draw();
  c2.cd();
  c2.Write();
  c2.Print(".png");

  //Y plane
  c3.cd(1);
  hFirstPreY.Fit("gaus","","",22,38);
  gStyle->SetOptFit(1);
  hFirstPreY.SetLineColor(kBlack);
  hFirstPreY.GetXaxis()->SetRangeUser(-15,15);
  hFirstPreY.GetYaxis()->SetTitleOffset(2);
  TLine line4(30,0,30,21000);
  line4.SetLineColor(kRed);
  line4.Draw();
  c3.cd(2);
  hFirstPostY.Fit("gaus","","",22,46);
  gStyle->SetOptFit(1);
  hFirstPostY.SetLineColor(kBlack);
  hFirstPostY.GetXaxis()->SetRangeUser(-15,15);
  hFirstPostY.GetYaxis()->SetTitleOffset(2);
  TLine line4I(30,0,30,14000);
  line4I.SetLineColor(kRed);
  line4I.Draw();
  c3.cd(3);
  hFirstAlgoY.Fit("gaus","","",22,39);
  gStyle->SetOptFit(1);
  hFirstAlgoY.SetLineColor(kBlack);
  hFirstAlgoY.GetXaxis()->SetRangeUser(-15,15);
  hFirstAlgoY.GetYaxis()->SetTitleOffset(2);
  line4.Draw();
  c3.cd();
  c3.Write();
  c3.Print(".png");
  
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
  c4.Print(".png");
  
  //last sample passing threshold ***************************************************************************************
  //U plane
  c5.cd(1);
  hLastPreU.Fit("gaus","","",-37,-17);
  gStyle->SetOptFit(1);
  hLastPreU.SetLineColor(kBlack);
  hLastPreU.GetYaxis()->SetTitleOffset(2);
  TLine line16(-25,0,-25,21000);
  line16.SetLineColor(kRed);
  line16.Draw();
  c5.cd(2);
  hLastPostU.Fit("gaus","","",-27,-7);
  gStyle->SetOptFit(1);
  hLastPostU.SetLineColor(kBlack);
  hLastPostU.GetYaxis()->SetTitleOffset(2);
  TLine line6I(-25,0,-25,16000);
  line6I.SetLineColor(kRed);
  line6I.Draw();
  c5.cd(3);
  hLastAlgoU.Fit("gaus","","",-27,-15);
  gStyle->SetOptFit(1);
  hLastAlgoU.SetLineColor(kBlack);
  hLastAlgoU.GetYaxis()->SetTitleOffset(2);
  TLine line6II(-25,0,-25,35000);
  line6II.SetLineColor(kRed);
  line6II.Draw();
  c5.cd();
  c5.Write();
  c5.Print(".png");
  
  //V plane                                                                                      
  // last sample - presample
  TH1F *hLastPreV2 = (TH1F*)hLastPreV.Clone();  // make a copy of the histogram to use for two separate peaks to get two statistics boxes                  
  hLastPreV2->SetNameTitle("Positive Peak","hey");
  hLastPreV.SetNameTitle("Negative Peak","Last sample passing V threshold - first presample ADC");

  c6.cd(1);
  //gStyle->SetOptStat(11); // regular stats box only has the name and number of entries                                                                      
  hLastPreV.Draw("");
  hLastPreV.GetYaxis()->SetTitleOffset(2);
  gStyle->SetOptFit(1);   // statistics box with the info from the fit without errors                                                                    
  g7->SetLineColor(kBlue); // make the fit blue to distinguish it                                                                                           
  hLastPreV.Fit(g7,"R"); // negative peak                                                                                     
  hLastPreV.GetYaxis()->SetTitleOffset(2);
  c6.Update();
  TPaveStats *st7 = (TPaveStats*)hLastPreV.FindObject( "stats" ); // get that stats box                                                                     
  if (st7){                      // move the statistics box so that you can see both boxes at the same time                                                   
    st7->SetX1NDC(0.68);
    st7->SetX2NDC(1.0);
    st7->SetY1NDC(0.6);
    st7->SetY2NDC(0.9);
  }

  hLastPreV2->Draw("sames");  // positive peak                                                                                                              
  gStyle->SetOptFit(1);
  g8->SetLineColor(kGreen);
  hLastPreV2->GetYaxis()->SetTitleOffset(2);
  hLastPreV2->Fit(g8,"R");
  hLastPreV2->GetYaxis()->SetTitleOffset(2);
  c6.Update();
  TPaveStats *st8 = (TPaveStats*)hLastPreV2->FindObject( "stats" );
  if (st8){
    st8->SetX1NDC(0.68);
    st8->SetX2NDC(1.0);
    st8->SetY1NDC(0.3);
    st8->SetY2NDC(0.6);
  }
  hLastPreV2->SetLineColor(kBlack);
  //TLine line2(-15,0,-15,27000);
  line2.SetLineColor(kRed);
  line2.Draw();
  //TLine line3(15,0,15,27000);
  line3.SetLineColor(kRed);
  line3.Draw();
  
  c6.cd(2); //last sample - postsample
  TH1F *hLastPostV2 = (TH1F*)hLastPostV.Clone();  // make a copy of the histogram to use for two separate peaks to get two statistics boxes              
  hLastPostV2->SetNameTitle("Positive Peak","hey");
  hLastPostV.SetNameTitle("Negative Peak","Last sample passing V threshold - last postsample ADC");

  // gStyle->SetOptStat(11); // regular stats box only has the name and number of entries                                                                      
  hLastPostV.Draw("");
  gStyle->SetOptFit(1);   // statistics box with the info from the fit without errors                                                                    
  g9->SetLineColor(kBlue); // make the fit blue to distinguish it                                                                                           
  hLastPostV.Fit(g9,"R"); // negative peak                                                                                
  hLastPostV.GetYaxis()->SetTitleOffset(2);
  c6.Update();
  TPaveStats *st9 = (TPaveStats*)hLastPostV.FindObject( "stats" ); // get that stats box                                                                    
  if (st9){                      // move the statistics box so that you can see both boxes at the same time                                    
    st9->SetX1NDC(0.68);
    st9->SetX2NDC(1.0);
    st9->SetY1NDC(0.6);
    st9->SetY2NDC(0.9);
  }

  hLastPostV2->Draw("sames");  // positive peak                                                                                                             
  gStyle->SetOptFit(1);
  g10->SetLineColor(kGreen);
  hLastPostV2->Fit(g10,"R");
  hLastPostV2->GetYaxis()->SetTitleOffset(2);
  c6.Update();
  TPaveStats *st10 = (TPaveStats*)hLastPostV2->FindObject( "stats" );
  if (st10){
    st10->SetX1NDC(0.68);
    st10->SetX2NDC(1.0);
    st10->SetY1NDC(0.3);
    st10->SetY2NDC(0.6);
  }
  hLastPostV2->SetLineColor(kBlack);
  hLastPostV2->GetYaxis()->SetTitleOffset(2);
  TLine line12I(-15,0,-15,17000);
  line12I.SetLineColor(kRed);
  line12I.Draw();
  TLine line13I(15,0,15,17000);
  line13I.SetLineColor(kRed);
  line13I.Draw();
  
  c6.cd(3); // last sample - algorithm
  TH1F *hLastAlgoV2 = (TH1F*)hLastAlgoV.Clone();  // make a copy of the histogram to use for two separate peaks to get two statistics boxes               
  hLastAlgoV2->SetNameTitle("Positive Peak","hey");
  hLastAlgoV.SetNameTitle("Negative Peak","Last sample passing V threshold - algorithm baseline ADC");

  //  gStyle->SetOptStat(11); // regular stats box only has the name and number of entries                                                                      
  hLastAlgoV.Draw("");
  hLastAlgoV.GetYaxis()->SetTitleOffset(2);
  gStyle->SetOptFit(1);   // statistics box with the info from the fit without errors                                                                    
  g11->SetLineColor(kBlue); // make the fit blue to distinguish it                                                                                          
  hLastAlgoV.Fit(g11,"R"); // negative peak                                                                                                           
  hLastAlgoV.GetYaxis()->SetTitleOffset(2);
  c6.Update();
  TPaveStats *st11 = (TPaveStats*)hLastAlgoV.FindObject( "stats" ); // get that stats box                                                                  
  if (st11){                      // move the statistics box so that you can see both boxes at the same time                              
    st11->SetX1NDC(0.68);
    st11->SetX2NDC(1.0);
    st11->SetY1NDC(0.6);
    st11->SetY2NDC(0.9);
  }

  hLastAlgoV2->Draw("sames");  // positive peak                                                                                                             
  gStyle->SetOptFit(1);
  g12->SetLineColor(kGreen);
  hLastAlgoV2->Fit(g12,"R");
  c6.Update();
  TPaveStats *st12 = (TPaveStats*)hLastAlgoV2->FindObject( "stats" );
  if (st12){
    st12->SetX1NDC(0.68);
    st12->SetX2NDC(1.0);
    st12->SetY1NDC(0.3);
    st12->SetY2NDC(0.6);
  }
  hLastAlgoV2->SetLineColor(kBlack);
  TLine line12II(-15,0,-15,34000);
  line12II.SetLineColor(kRed);
  line12II.Draw();
  TLine line13II(15,0,15,34000);
  line13II.SetLineColor(kRed);
  line13II.Draw();
  c6.cd();
  c6.Write();
  c6.Print(".png");

  //Y plane                                                                                                     
  c7.cd(1);
  hLastPreY.Fit("gaus","","",13,30);
  gStyle->SetOptFit(1);
  hLastPreY.SetLineColor(kBlack);
  hLastPreY.GetYaxis()->SetTitleOffset(2);
  //TLine line4(30,0,30,21000);
  line4.SetLineColor(kRed);
  line4.Draw();
  c7.cd(2);
  hLastPostY.Fit("gaus","","",12,40);
  gStyle->SetOptFit(1);
  hLastPostY.SetLineColor(kBlack);
  hLastPostY.GetYaxis()->SetTitleOffset(2);
  TLine line4Ii(30,0,30,8000);
  line4Ii.SetLineColor(kRed);
  line4Ii.Draw();
  c7.cd(3);
  hLastAlgoY.Fit("gaus","","",16,32);
  gStyle->SetOptFit(1);
  hLastAlgoY.SetLineColor(kBlack);
  hLastAlgoY.GetYaxis()->SetTitleOffset(2);
  line4.Draw();
  c7.cd();
  c7.Write();
  c7.Print(".png");
  
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
  c8.Print(".png");
  
  c10.cd();
  hFirstNegV.Draw("hist ][");
  c10.Print(".png");
  
  //and ... write to file!                                                            
  f_output.Write();
  f_output.Close();

  cout<<"success";
}
