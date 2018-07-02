//some standard C++ includes                                                                                                  
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


void PressEnterToContinue()
{
  cout << "Press Enter to continue" << flush ;
  cin.ignore(numeric_limits<streamsize>::max(),'\n');
}

TCanvas* obj;


void  DrawCanvasData()
{
  gStyle->SetPalette(55);
  TFile *F1 = TFile::Open("ReadSNSwizzledData_output_500.root");
  
  TIter next(F1->GetListOfKeys());
  TKey *key;
  while ((key = (TKey*)next()))
    {
      obj  =(TCanvas*) F1->Get(key->GetName());
      obj->Draw();
      gPad->WaitPrimitive();
      PressEnterToContinue();
    }
}
