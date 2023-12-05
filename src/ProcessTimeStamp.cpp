#include "ProcessTimeStamp.h"

#include <TFile.h>
#include <TTree.h>
TimeStampManager *TimeStampManager::Instance()
{
    static TimeStampManager *instance = new TimeStampManager();
    return instance;
}

bool TimeStampManager::Init(std::string sOutputFile)
{
    fOutFile = new TFile(sOutputFile.c_str(), "recreate");
    if (!fOutFile->IsOpen() || !fOutFile->IsWritable())
        return false;

    return true;
}

bool TimeStampManager::GenerateBranchForBoard(int BoardNo)
{
    //! TODO: Add Tree and branch
    fOutFile->cd();
    fOutTree = new TTree("tsTree", "Time Stamp Tree");
    // fOutTree->Branch()
    return true;
}
