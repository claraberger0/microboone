/*************************************************************                                                                                     
 * ReadSNSwizzledData.cc program                                                                                             
 * This is a simple demonstration of reading a LArSoft file                                                                  
 * and accessing recob::Wire information                                                                   
 * José I. Crespo-Anadón (jcrespo@nevis.columbia.edu), Jul 10, 2017                                                          
 * Based on code by Wesley Ketcum
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

  // TFile f_output("waveform_output.root","RECREATE");

  //We specify our files in a list of file names!                                                                            
  //Note: multiple files allowed. Just separate by comma.                                                                    
  vector<string> filenames { argv[1] };

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


  // how many of the repeating bits do we see in the induction vs collection channels
  int uevents = 0;
  int vevents = 0;
  int yevents = 0;

  //last sample - second to last sample
  TCanvas c1("c_secondlast","c1",1100,400);
  c1.Divide(3,1);

  TH1F hSecondLastU("hSecondlastu", "Last Sample - Second Last Sample ADC U; Last Sample - (Last-1) Sample; Frequency", 200, -100, 100);
  TH1F hSecondLastV("hSecondlastv", "Last Sample - Second Last Sample ADC V; Last Sample - (Last-1) Sample; Frequency", 200, -100, 100);
  TH1F hSecondLastY("hSecondlasty", "Last Sample - Second Last Sample ADC Y; Last Sample - (Last-1) Sample; Frequency", 200, -100, 100);


  size_t _maxEvts = 200;
  size_t evCtr = 0;

  for (gallery::Event ev(filenames) ; !ev.atEnd(); ev.next()) {
    if(evCtr >= _maxEvts) break;

    auto t_begin = high_resolution_clock::now();
    //int run = ev.eventAuxiliary().run();
    int event = ev.eventAuxiliary().event();

    cout<<event<<endl;
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
    
    for (unsigned int i=0; i<wire_vec.size();i++){
      auto zsROIs = wire_vec[i].SignalROI();
      int channel = wire_vec[i].Channel();
      

      for (auto iROI = zsROIs.begin_range(); iROI != zsROIs.end_range(); ++iROI) {
	auto ROI = *iROI;
	const int firstTick = ROI.begin_index();
	const size_t endTick = ROI.end_index();
       
       

	// look at waveforms where the difference between the first passing sample and the last post sample is 0
	double firstpost; //fist sample passing threshold - the last post sample 
    
	//last sample - second to last sample
	double secondlast;
  
	
        if(ROI[endTick]>1){   // not counting when the last tick is 0 or very small                                 
          firstpost = ROI[firstTick+7]-ROI[endTick];
	  secondlast = ROI[endTick]-ROI[endTick-1];
	  
	  //histogram to show waveforms
          TH1D horig("roi_original", "ROI where first sample passing threshold - last postsample is 0;Tick;ADC", endTick + 1 - firstTick, firstTick, endTick + 1);
          horig.SetLineColor(kBlack);

          for (size_t iTick = ROI.begin_index(); iTick <= ROI.end_index(); iTick++ ){  //fill up to endTick            
	    horig.Fill((int)iTick,ROI[iTick]);}
          
	  if (channel >2400 && channel <=4800 && firstpost>=0 && firstpost<1){
            TCanvas c(Form("c_%d_%d_V",event,channel),Form("c%d_Y",channel),900,500);
            horig.Draw("hist ]");
            //cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<" to "<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick]<<"="<<firstpost<<"\n";
	    if ( (endTick-firstTick)>17 ){
	      vevents += 1;
	      if (firstTick != 1600 && firstTick != 4800){hSecondLastV.Fill(secondlast);}}
	    //Cout<<uevents<<' '<<vevents<<' '<<yevents<<endl;
	    //	      c.Print(".png");
          }
	  if ( channel <=2400 && firstpost>=0 && firstpost<1){
            TCanvas c(Form("c_%d_%d_U",event,channel),Form("c%d_Y",channel),900,500);
            horig.Draw("hist ]");
            //cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<" to "<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick]<<"="<<firstpost<<"\n";
	    if ( (endTick-firstTick)>17){
	      uevents += 1;
	      if(secondlast==0){
	    //cout<<uevents<<' '<<vevents<<' '<<yevents<<endl;
            //c.Print(".png");
		if (firstTick != 1600 && firstTick != 4800){ hSecondLastU.Fill(secondlast);
		  c.Print(".png");}}}
	  }
	  if (channel >4800 && channel <=8256 && firstpost>=0 && firstpost<1){
            TCanvas c(Form("c_%d_%d_Y",event,channel),Form("c%d_Y",channel),900,500);
            horig.Draw("hist ]");
            //cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<" to "<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick]<<"="<<firstpost<<"\n";
	    if ( (endTick-firstTick)>17){
              yevents += 1;
	    //cout<<uevents<<' '<<vevents<<' '<<yevents<<endl;
            //c.Print(".png";
	      if (firstTick != 1600 && firstTick != 4800){hSecondLastY.Fill(secondlast);}}
          }
	}

        else
          {firstpost = ROI[firstTick+7]-ROI[endTick-1];
	   secondlast = ROI[endTick-1]-ROI[endTick-2];

	   TH1D horig("roi_original", "ROI where first sample passing threshold - last postsample is 0;Tick;ADC", endTick  - firstTick, firstTick, endTick);
	   horig.SetLineColor(kBlack);
	   
	   for (size_t iTick = ROI.begin_index(); iTick < ROI.end_index(); iTick++ ){  // fill up to endTick-1       
	     horig.Fill((int)iTick,ROI[iTick]);}
	   
	   if (channel >2400 && channel <=4800 && firstpost>=0 && firstpost<1){
	     TCanvas c(Form("c_%d_%d_V",event,channel),Form("c%d_Y",channel),900,500);
	     horig.Draw("hist ]");
	     //cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<" to "<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick-1]<<"="<<firstpost<<"\n";
	     if ( (endTick-firstTick)>17){
	       vevents += 1;
	     //cout<<uevents<<' '<<vevents<<' '<<yevents<<endl;
	     //c.Print(".png");
	       if (firstTick != 1600 && firstTick != 4800){hSecondLastV.Fill(secondlast);}}
	    }
	   if (channel <=2400 && firstpost>=0 && firstpost<1){
	     TCanvas c(Form("c_%d_%d_U",event,channel),Form("c%d_Y",channel),900,500);
	     horig.Draw("hist ]");
	     //cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<" to "<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick-1]<<"="<<firstpost<<"\n";
	     if ( (endTick-firstTick)>17){
	       uevents += 1;
	     //cout<<uevents<<' '<<vevents<<' '<<yevents<<endl;
	     //c.Print(".png");
	       if(secondlast==0){
		 if (firstTick != 1600 && firstTick != 4800){
		   hSecondLastU.Fill(secondlast);
		   c.Print(".png");}}}
	    }
	   if (channel >4800 && channel <=8256 && firstpost>=0 && firstpost<1){
	     TCanvas c(Form("c_%d_%d_Y",event,channel),Form("c%d_Y",channel),900,500);
	     horig.Draw("hist ]");
	     //cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<" to "<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick-1]<<"="<<firstpost<<"\n";
	     if ( (endTick-firstTick)>17){
	       yevents += 1;
	     //cout<<uevents<<' '<<vevents<<' '<<yevents<<endl;
	     //c.Print(".png");
	       if (firstTick != 1600 && firstTick != 4800){hSecondLastY.Fill(secondlast);}}
	   }
          } // end else
	
      } //end loop over ROIs
    } //end loop over wires

    // f_output.cd();
 
    auto t_end = high_resolution_clock::now();
    duration<double,std::milli> time_total_ms(t_end-t_begin);
    //cout << "\tEvent took " << time_total_ms.count() << " ms to process." << endl;
    evCtr++;
  } //end loop over events!

  c1.cd(1);
  hSecondLastU.Draw();
  TLine lineI(-25,0,-25,22);
  lineI.SetLineColor(kRed);
  lineI.Draw();
  c1.cd(2);
  hSecondLastV.Draw();
  TLine line2(-15,0,-15,62);
  line2.SetLineColor(kRed);
  line2.Draw();
  TLine line3(15,0,15,62);
  line3.SetLineColor(kRed);
  line3.Draw();
  c1.cd(3);
  hSecondLastY.Draw();
  TLine line4(30,0,30,32);
  line4.SetLineColor(kRed);
  line4.Draw();

  c1.cd();
  c1.Print(".png");
  
  cout<<"induction plane events: "<<uevents+vevents<<"(U: "<< uevents<< " V: "<<vevents<<" )   collection plane events: "<<yevents<<endl;
  //and ... write to file!                                                                                                                                   
  // f_output.Write();
  //f_output.Close();

}
