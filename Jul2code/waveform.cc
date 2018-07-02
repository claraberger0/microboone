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

  TFile f_output("waveform_output.root","RECREATE");

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

  size_t _maxEvts = 2;
  size_t evCtr = 0;
  
  // TCanvas c("c","c",900,500);
  //TH1D horig("roi_original", "roi_original;Tick;ADC",21,5458,5478);
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

    //We can now treat this like a pointer, or dereference it to have it be like a vector.                                   
    //I (Wes) for some reason prefer the latter, so I always like to do ...                                                  
    auto const& wire_vec(*wire_handle);

    cout << "\tThere are " << wire_vec.size() << " Wires in this event." << endl;
    
    for (unsigned int i=0; i<wire_vec.size();i++){
      auto zsROIs = wire_vec[i].SignalROI();
      int channel = wire_vec[i].Channel();
      
      for (auto iROI = zsROIs.begin_range(); iROI != zsROIs.end_range(); ++iROI) {
	auto ROI = *iROI;
	const int firstTick = ROI.begin_index();
	const size_t endTick = ROI.end_index();
       

	//	TH1D horig("roi_original", "roi_original;Tick;ADC", endTick + 1 - firstTick, firstTick, endTick + 1);
	//horig.SetLineColor(kBlack);
     

	//for (size_t iTick = ROI.begin_index(); iTick <= ROI.end_index(); iTick++ ){  
	//horig.Fill((int)iTick,ROI[iTick]);}
	
	double firstpost;
	if(ROI[endTick]>1){   // not counting when the last tick is 0 or very small                                        
          firstpost = ROI[firstTick+7]-ROI[endTick];
	  TH1D horig("roi_original", "roi_original;Tick;ADC", endTick + 1 - firstTick, firstTick, endTick + 1);
	  horig.SetLineColor(kBlack);
          for (size_t iTick = ROI.begin_index(); iTick <= ROI.end_index(); iTick++ ){  //fill up to endTick                
	    horig.Fill((int)iTick,ROI[iTick]);}
	  if (channel >3400 && channel <=3420 && firstpost<0){
	    TCanvas c(Form("c_%d_%d_V_neg",channel,firstTick),Form("c%d_Y",channel),900,500);
	    horig.Draw("hist ]");
	    c.Modified();
	    c.Update();
	    cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<" to "<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick]<<"="<<firstpost<<"\n";
	    c.Print(".png");
	  }
	  if (channel >3400 && channel <=3410 && firstpost>0){
	    TCanvas c(Form("c_%d_%d_V_pos",channel,firstTick),Form("c%d_Y",channel),900,500);
	    horig.Draw("hist ]");
	    c.Modified();
	    c.Update();
	    cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<" to "<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick]<<"="<<firstpost<<"\n";
	    c.Print(".png");
	  }
	}

        else                                                                                                              
          {firstpost = ROI[firstTick+7]-ROI[endTick-1];
	   TH1D horig("roi_original", "roi_original;Tick;ADC", endTick  - firstTick, firstTick, endTick);
	   horig.SetLineColor(kBlack);
	  for (size_t iTick = ROI.begin_index(); iTick < ROI.end_index(); iTick++ ){  // fill up to endTick-1            
	      horig.Fill((int)iTick,ROI[iTick]);}
	  if (channel >3400 && channel <=3420 && firstpost<0){
	    TCanvas c(Form("c_%d_%d_V_neg",channel,firstTick),Form("c%d_Y",channel),900,500);
	    horig.Draw("hist ]");
	    c.Modified();
	    c.Update();
	    cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<" to "<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick-1]<<"="<<firstpost<<"\n";
	    c.Print(".png");
          }
	  if (channel >3400 && channel <=3410 && firstpost>0){
	    TCanvas c(Form("c_%d_%d_V_pos",channel,firstTick),Form("c%d_Y",channel),900,500);
	    horig.Draw("hist ]");
	    c.Modified();
	    c.Update();
	    cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<" to "<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick-1]<<"="<<firstpost<<"\n";
	    c.Print(".png");
	  }
	  }

	// first sample passing the threshold - last post sample when  the difference is negative in the V plane    	
	//if (channel >3400 && channel <=3420 && firstpost<0){
	// TCanvas c(Form("c_%d_%d_V_neg",channel,firstTick),Form("c%d_Y",channel),900,500);
	//horig.Draw("hist ]");
	//c.Modified();
	//c.Update();
	//cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<" to "<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick]<<"="<<firstpost<<"\n";
	//c.Print(".png");
	//}	
	//if (channel >3400 && channel <=3410 && firstpost>0){
	//TCanvas c(Form("c_%d_%d_V_pos",channel,firstTick),Form("c%d_Y",channel),900,500);
	//horig.Draw("hist ]");
	// c.Modified();
	//c.Update();
	//cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<" to "<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick]<<"="<<firstpost<<"\n";
	//c.Print(".png");
        //}

	// look at waveforms where the difference between the first passing sample and the last post sample is 0
	//double firstpost; //fist sample passing threshold - the last post sample 
	//if(ROI[endTick]>1){   // not counting when the last tick is 0 or very small
	  //firstpost = ROI[firstTick+7]-ROI[endTick];
	  //for (size_t iTick = ROI.begin_index(); iTick <= ROI.end_index(); iTick++ ){  //fill up to endTick
	    //  horig.Fill((int)iTick,ROI[iTick]);}}
	//else
	  //{firstpost = ROI[firstTick+7]-ROI[endTick-1];
	    //for (size_t iTick = ROI.begin_index(); iTick < ROI.end_index(); iTick++ ){  // fill up to endTick-1 
	      //    horig.Fill((int)iTick,ROI[iTick]);}}
      	
	//if(firstpost<1 && firstpost>=0){    // when this differece is 0,
	//  cout<<"event:"<<event<<' '<<"channel:"<<channel<<' '<<firstTick<<' to  '<<endTick<<' '<<ROI[firstTick+7]<<"-"<<ROI[endTick]<<"="<<firstpost<<"\n";
	// TCanvas c(Form("c_%d_%d_Y",channel,firstTick),Form("c%d_Y",channel),900,500);                             
	// horig.Draw("hist ]");
	// c.Modified();
	// c.Update();
	// c.Print(".png"); // print that waveform
	//}


	//	if(channel==5644)
	//{TCanvas c(Form("c_%d_%d_Y",channel,firstTick),Form("c%d_Y",channel),900,500);
	    //	    cout<<ROI[endTick]<<;
	//  horig.Draw("hist ]");
	//  c.Modified();
	//  c.Update();
	    // c.Print(".png");
	//}

	//        if ( channel >= 1010 && channel <= 1020)// just a random sample of U plane to look at
	//{ TCanvas c(Form("c_%d_U",channel),Form("c%d_U",channel),900,500);
	// horig.Draw("hist ][");
	//c.Modified();
	//c.Update();
	//f_output.cd();
	//c.Write();
	  //	  cout<<ROI[endTick]<<' '<< channel <<'\n';
	//}
        //else if (channel >=3400 && channel<=3410){
	//TCanvas c(Form("c_%d_V",channel),Form("c%d_V",channel),900,500);
	// horig.Draw("hist ][");
	//c.Modified();
	//c.Update();
	//f_output.cd();
	//c.Write();
	 //cout<<ROI[endTick]<<' '<<channel<<'\n';
	//}
	//else if (channel >= 6800 && channel <= 6810)
	   //{ TCanvas c(Form("c_%d_Y",channel),Form("c%d_Y",channel),900,500);
	//horig.Draw("hist ]");
	//c.Modified();
	//c.Update();
	//f_output.cd();
	//c.Write();
	  //cout<<ROI[endTick]<<' '<<channel<<'\n';
	//}
	
      }
    }
    // f_output.cd();
 
    auto t_end = high_resolution_clock::now();
    duration<double,std::milli> time_total_ms(t_end-t_begin);
    cout << "\tEvent took " << time_total_ms.count() << " ms to process." << endl;
    evCtr++;
  } //end loop over events!

  //  horig.Draw("hist ][");                             
  // c.Write();
  
  //and ... write to file!                                                                                                                                   
  f_output.Write();
  f_output.Close();

}
