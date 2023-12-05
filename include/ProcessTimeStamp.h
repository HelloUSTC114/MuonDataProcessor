#ifndef PROCESSTIMESTAMP
#define PROCESSTIMESTAMP
#include <string>

class TFile;
class TTree;

#define MAX_BOARD_SAMETIME (256)
class TimeStampManager
{
public:
    static TimeStampManager *Instance();

    bool Init(std::string sOutputFile);

private:
    TFile *fOutFile = nullptr;
    TTree *fOutTree = nullptr;

    // Related to tree
    uint32_t fTotalBoards;
    uint32_t fCurrentT0ID;
    uint32_t fCalculatedT0ID;
    double fTimeStamp[MAX_BOARD_SAMETIME]; // only save fTotalBoards data

    bool GenerateBranchForBoard(int BoardNo);
};

#define gTSManager (TimeStampManager::Instance())

#endif