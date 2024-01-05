#ifndef T0TS_H
#define T0TS_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <TFile.h>
#include <TTree.h>

#define MAX_BOARD_COUNTS 32
#define gT0Manager (BoardT0Manager::Instance())

namespace ROOTTrees
{
    class tsTree;
    class board;
}

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
    ~BoardT0Manager();

    void ForceClear();
    void CloseT0ROOTFile() { CloseFile(); };

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
    BoardT0Manager() = default;

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

#define gDataMatchManager (DataMatchManager::Instance())
class DataMatchManager
{
public:
    static DataMatchManager *Instance();

    bool InitBoardArray(std::vector<int> boardArray);
    void CloseFile();

    static bool JudgeBoardFlag(bool *flags, int boardNumber);
    /// @brief Find closet entry to start Entry, in T0 TS file
    /// @param startEntry [IN]
    /// @param stampArray [OUT]
    /// @param devFromStart [OUT] counter of how many events since startEntry
    /// @return
    bool FindStamp(int startEntry, double *stampArray, int *devFromStart);

    /// @brief Initiate T0 TS file
    bool InitiateT0TS(std::string sT0TSFile = "TS.root");

    /// @brief Updage interval, start with start entry in T0 TS file
    /// @param startEntry
    /// @return
    bool UpdateInterval(int startEntry);

    /// @brief Get segpoint closet to start Entry in T0 TS file
    /// @param startEntry
    /// @return
    bool UpdateSegPoint(int startEntry);

    /// @brief Update segpoint & interval
    /// @param startEntry
    /// @return
    bool UpdateSeg(int startEntry) { return (UpdateInterval(startEntry) && UpdateSegPoint(startEntry)); };

    /// @brief Initiate Board trees
    bool GenerateBoardMap(std::string sBoardDataFolder = "./");

    /// @brief Initiate Board Matching file
    bool InitMatchFile(std::string sOutputFile = "./MatchEntries.root");

    /// @brief judge whether event time is inside interval
    /// @param eventTime
    /// @param boardNo
    /// @return
    int JudgeEventTime(double eventTime, int boardNo);

    /// @brief find all evnets in the segment interval
    /// @param segEntry
    /// @param startEntries
    /// @param endEntries
    /// @return
    bool FindAllEventsInSeg(int segEntry, int *startEntries, int *endEntries);

    /// @brief Match events inside
    /// @param startEntries
    /// @param endEntries
    /// @return
    bool MatchEventsInSeg(int *startEntries, int *endEntries);

    bool ReadyForMatch() { return fT0Flag && fBoardFlag && fOutFlag; };
    void DoInitiate(std::string sT0File = "./TS.root", std::string sBoardFolder = "../Processed/", std::string sOutputFile = "../Processed/MatchEntries.root");
    int DoMatch();
    int MatchBoards(std::string sT0File = "./TS.root", std::string sBoardFolder = "../Processed/", std::string sOutputFile = "../Processed/MatchEntries.root");

private:
    bool fBoardArrayInitFlag = 0;

    uint64_t fTSReadingEntry = 0;
    double fTSInterval[MAX_BOARD_COUNTS];

    // Matched information
    double fLastSeg[MAX_BOARD_COUNTS];  // segment time into pieces, length is about 1 s
    double fNextSeg[MAX_BOARD_COUNTS];  // Same use with gLastTS
    double fLastSeg2[MAX_BOARD_COUNTS]; // segment time into pieces, length is about 1 s
    double fNextSeg2[MAX_BOARD_COUNTS]; // Same use with gLastTS

    // Initate Segment
    double fT0TSInterval2[MAX_BOARD_COUNTS];
    double fT0TSStart2[MAX_BOARD_COUNTS];
    ROOTTrees::tsTree *fTS = NULL;
    std::string fsT0TSFile = "";
    bool fT0Flag = 0;

    // Used for calculating interval and segpoint
    double fCurrentValidInterval[MAX_BOARD_COUNTS]; // Update valid interval real time
    double fCurrentValidSegPoint[MAX_BOARD_COUNTS]; // Update valid seg point real time

    // Each Board information
    int fBoardCount = 0;
    std::vector<int> fBoardArray;
    ROOTTrees::board *fBoard[MAX_BOARD_COUNTS];
    std::string fsBoardDataFolder = "";
    bool fBoardFlag = 0;

    // Match Information
    TFile *fMatchFile = NULL;
    TTree *fMatchTree = NULL;
    TTree *fBNTree = NULL;
    bool fOutFlag = 0;

    int fMatchCounter;                       // restore match counter
    int fMatchedBoard[MAX_BOARD_COUNTS];     // restore matched channel
    int fMatchFlag[MAX_BOARD_COUNTS];        // restore whether this board has matched index
    ULong64_t fMatchEntry[MAX_BOARD_COUNTS]; // restore entries
    double fMatchTime[MAX_BOARD_COUNTS];     // restore matched time

    uint64_t fCurrentEntries[MAX_BOARD_COUNTS]{0}; // Current reading entries
    double fT0Delay[MAX_BOARD_COUNTS];             // Restore T0Delay for each board, set board 0 at 0
    bool fMatchedFlag = 0;

    std::vector<double> fEventTime[MAX_BOARD_COUNTS];
    std::vector<int> fEventEntry[MAX_BOARD_COUNTS];

    DataMatchManager() = default;
};

#endif