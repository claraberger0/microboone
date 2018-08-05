********************************
// plot event display after display to look through with more convenience 
*********************************


//some standard C++ includes          (from ReadSNSwizzledData.cc)                                                                                           
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <chrono>
#include <limits>


//some ROOT includes                                                                                                          
#include "TInterpreter.h"
#include "TROOT.h"
#include "TH1F.h"
#include "TH2S.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TIterator.h"
#include "TKey.h"

//convenient for us! let's not bother with art and std namespaces!                                                           
 
using namespace art;
using namespace std;

using namespace std::chrono;

// define a control that shows "press enter to continue" after each iteration and then takes an enter input
void PressEnterToContinue()
{
  cout << "Press Enter to continue" << flush ;
  cin.ignore(numeric_limits<streamsize>::max(),'\n');
}

TCanvas* obj;


void  DrawCanvasData()
{
  gStyle->SetPalette(55);
  TFile *F1 = TFile::Open("ReadSNSwizzledData_output_500.root");  // specifically uses output data from 500 events in run 14662
  
  TIter next(F1->GetListOfKeys());
  TKey *key;
  while ((key = (TKey*)next()))
    {
      obj  =(TCanvas*) F1->Get(key->GetName());
      obj->Draw();               // draw the event display 
      gPad->WaitPrimitive();     // pause to look at the event display
      PressEnterToContinue();    // go to the next one
    }
}
