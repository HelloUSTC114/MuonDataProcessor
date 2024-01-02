#ifndef T0TS_H
#define T0TS_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <TFile.h>
#include <TTree.h>

#define MAX_BOARD_COUNTS 256
#define gT0Manager (BoardT0Manager::Instance())

namespace T0Process
{
    // An example of how to use BoardT0Manager
    int MatchT0(std::vector<int> boardArray, std::string txtInPath, std::string sT0FinalROOTPath);
    int ConvertTS2ROOT(std::string sInfile, std::string sOutFile);
    int ConvertTS2ROOT(std::string sInfile, std::string sOutFile, int &entries, double &startT0, double &endT0);
};

class BoardT0Manager
{
public:
    BoardT0Manager() = default;
    ~BoardT0Manager();

    void ForceClear();

    // static int MatchT0(std::vector<int> boardArray, std::string txtInPath, std::string sT0FinalROOTPath);
    static BoardT0Manager *Instance();

    bool InitBoardArray(std::vector<int> boardArray, std::string txtInPath, std::string ROOTOutPath);
    bool AddBoard(int board);
    bool UpdateTXTROOTPath(std::string txtInPath, std::string ROOTOutPath);

    bool InitT0Matching(std::string sFileName = "TS.root");
    void ConvertAllTS();
    int ConvertTS(int idx, int &entries, double &startT0, double &endT0);
    bool FindBegining();
    bool SaveNextTS();
    std::vector<int> &GetInsideMatchedCounter() { return fMatchedCounter; };

private:
    TFile *fOutFile = NULL;
    TTree *fBNTree = NULL;
    TTree *fTree = NULL;
    std::vector<int> fBoardArray;
    int fBoardCount = 0;

    // TXT to ROOT
    bool fPathInitFlag = 0;
    bool fAddBoardCloseFlag = 0; // Set as 1 after fOutFile initiated, stop root file creating & reading
    std::string fsTXTInPath = "";
    std::string fsROOTOutPath = "";

    // Save Data
    uint32_t fTSid;                         // Time stamp ID
    uint8_t fTSBoardCount;                  // How many boards has this time stamp
    uint8_t fTSBoardStat[MAX_BOARD_COUNTS]; // which boards has this time stamp, no map, save array index only
    double fTS[MAX_BOARD_COUNTS];           // Time stamp for each board
    uint8_t fTSFlag[MAX_BOARD_COUNTS];      // Whether this board has time stamp

    // Open input file
    TFile *fInFiles[MAX_BOARD_COUNTS]{0};
    TTree *fInTrees[MAX_BOARD_COUNTS]{0};
    double fInTS[MAX_BOARD_COUNTS];

    // para for match
    int fPreEntry[MAX_BOARD_COUNTS];        // Save previous entry
    double fPreTS[MAX_BOARD_COUNTS];        // Save previous time stamp, infer its value if  the TS is missing
    double fUnmatchedCum[MAX_BOARD_COUNTS]; // Save accumulated unmatched time deviation

    void CloseFile();
    bool Try_FindBegining(double AbsTimeStamp);

    // Counter inside Manager
    std::vector<int> fMatchedCounter;
};

#endif