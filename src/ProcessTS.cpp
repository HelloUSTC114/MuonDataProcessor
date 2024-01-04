#include <iostream>
#include <iomanip>
#include <fstream>
#include <TFile.h>
#include <TTree.h>
#define TS_JUDGE_RANGE (2e8)

#ifdef TESTT0_INSIDE

void ConvertTS2ROOT(int board = 0)
{
    auto file = new TFile(Form("Board%dTS.root", board), "recreate");
    auto tree = new TTree(Form("ts%d", board), Form("Time Stamp for board %d", board));

    uint32_t tsid = 0;
    double ts;
    tree->Branch("tsid", &tsid, "tsid/i");
    tree->Branch("ts", &ts, "ts/D");

    std::ifstream fin;
    fin.open(Form("Board%dTS.txt", board));
    double preTS = 0;
    for (int row = 0; fin.is_open() && !fin.eof() && fin.good(); row++)
    {
        int boardID;
        int iddev;
        double tsTemp;
        fin >> boardID >> iddev >> tsTemp;

        //  Judge whether time deviation is larger than 1000 ns
        if ((tsTemp - preTS) < TS_JUDGE_RANGE && (row > 0))
            continue;

        tsid = boardID;
        ts = tsTemp;
        tree->Fill();
        preTS = tsTemp;
    }
    tree->Write();
    file->Close();
    delete file;
}

#define MAX_BOARD_COUNTS 256
#include <vector>
#include <map>

// Range of Time stamp judgement, in unti of ns

class tempClass
{
public:
    // tempClass();
    ~tempClass();
    bool Init(std::string sFileName = "TS.root", std::vector<int> boardArray = {0, 1, 2, 3, 4, 5});
    void ConvertAllTS();
    bool FindBegining();
    bool SaveNextTS();

private:
    TFile *fOutFile = NULL;
    TTree *fBNTree = NULL;
    TTree *fTree = NULL;
    std::vector<int> fBoardArray;
    int fBoardCount = 0;

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
};

tempClass::~tempClass()
{
    CloseFile();
}

void tempClass::ConvertAllTS()
{
    for (int i = 0; i < fBoardArray.size(); i++)
    {
        ConvertTS2ROOT(fBoardArray[i]);
    }
}

bool tempClass::Init(std::string sFileName, std::vector<int> boardArray)
{
    CloseFile();
    fOutFile = new TFile(sFileName.c_str(), "recreate");

    // Save board number information
    fBNTree = new TTree("bnTree", "Tree save all board number"); // Board number tree
    uint32_t boardNo;
    fBNTree->Branch("bn", &boardNo, "bn/i");
    fBoardArray = boardArray;
    fBoardCount = fBoardArray.size();
    for (int i = 0; i < fBoardCount; i++)
    {
        boardNo = fBoardArray[i];
        fBNTree->Fill();
    }

    // Generate TS data tree in output file
    fTree = new TTree("tsTree", "Time stamp for all boards");
    int NTotalBoards = fBoardCount;
    fTSid = 0;
    fTree->Branch("tsid", &fTSid, "tsid/i");
    fTree->Branch("tsBoardCount", &fTSBoardCount, "tsBoardCount/b");
    fTree->Branch("tsBoardStat", fTSBoardStat, "tsBoardStat[tsBoardCount]/b");
    fTree->Branch("ts", fTS, Form("ts[%d]/D", NTotalBoards));
    fTree->Branch("tsFlag", fTSFlag, Form("tsFlag[%d]/b", NTotalBoards));

    // Read Input files
    for (int i = 0; i < fBoardCount; i++)
    {
        fInFiles[i] = new TFile(Form("Board%dTS.root", fBoardArray[i]));
        if (!fInFiles[i]->IsOpen())
        {
            std::cout << "No files: " << i << std::endl;
            CloseFile();
            return false;
        }
        fInTrees[i] = (TTree *)(fInFiles[i]->Get(Form("ts%d", boardArray[i])));
        if (!fInTrees[i])
        {
            std::cout << "No trees: " << i << std::endl;
            CloseFile();
            return false;
        }
        fInTrees[i]->SetBranchAddress("ts", &fInTS[i]);
    }

    return true;
}

void tempClass::CloseFile()
{
    if (fOutFile && fOutFile->IsWritable())
    {
        fOutFile->cd();
        fTree->Write();
    }
    delete fOutFile;
    fOutFile = NULL;
    fTree = NULL;
    fBNTree = NULL;

    for (int i = 0; i < fBoardCount; i++)
    {
        delete fInFiles[i];
        fInFiles[i] = NULL;
        fInTrees[i] = NULL;
        fPreEntry[i] = 0;
        fPreTS[i] = 0;
    }
}

bool tempClass::SaveNextTS()
{
    static double fTSDev[MAX_BOARD_COUNTS]; // Save calculated time stamp deviation, only for find minimum deviation

    // Find Minimum deviation
    double minDev = 0;
    bool minDevInitFlag = 0;
    for (int board = 0; board < fBoardCount; board++)
    {
        if (fPreEntry[board] + 1 >= fInTrees[board]->GetEntries())
            return false;
        fInTrees[board]->GetEntry(fPreEntry[board] + 1);
        fTSDev[board] = fInTS[board] - fPreTS[board] - fUnmatchedCum[board];
        // if (minDevInitFlag == 0 && TMath::Abs(fTSDev[board] - 1e9) < TS_JUDGE_RANGE)
        if (minDevInitFlag == 0)
        {
            minDev = fTSDev[board];
            minDevInitFlag = 1;
        }
        if (minDevInitFlag && minDev > fTSDev[board])
            minDev = fTSDev[board];
    }
    // std::cout << fTSid << '\t' << minDev << std::endl;
    static int sgErrorCount = 0;
    static int sgCountDown = 0;
    if (minDev / 1e9 > 5)
    {
        // std::cout << "Error count: " << sgErrorCount << std::endl;
        std::cout << std::setprecision(5) << minDev / 1e9;
        for (int board = 0; board < fBoardCount; board++)
        {
            std::cout << '\t' << fTSDev[board] / 1e9 << '\t';
        }
        std::cout << std::endl;
        sgErrorCount++;
    }
    if (sgErrorCount > 200)
    {
        sgCountDown++;
        if (sgCountDown > 100)
            return false;
    }

    fTSBoardCount = 0;
    static int gInsideEntry = 0;
    gInsideEntry++;
    for (int board = 0; board < fBoardCount; board++)
    {
        // std::cout << board << '\t' << fPreEntry[board] << '\t' << (bool)fTSFlag[board] << '\t' << minDev << '\t' << fInTS[board] / 1e9 << '\t' << fPreTS[board] / 1e9 << '\t' << fTSDev[board] / 1e9 << '\t' << fUnmatchedCum[board] / 1e9 << '\t' << (TMath::Abs(fTSDev[board] - minDev) < TS_JUDGE_RANGE) << std::endl;

        // If TS matched
        if (TMath::Abs(fTSDev[board] - minDev) < TS_JUDGE_RANGE)
        // if (1)
        {
            fTSBoardStat[fTSBoardCount++] = board;
            fTS[board] = fInTS[board];
            fTSFlag[board] = 1;

            fPreEntry[board]++;
            fUnmatchedCum[board] = 0;
            fPreTS[board] = fTS[board];
        }
        else // if not matched
        {
            fUnmatchedCum[board] += minDev;
            fTSFlag[board] = 0;
            fTS[board] = fPreTS[board] + fUnmatchedCum[board];
            // std::cout << std::setprecision(10) << gInsideEntry << '\t' << std::setprecision(1) << board << std::setprecision(10) << '\t' << fPreEntry[board] << '\t' << (bool)fTSFlag[board] << '\t' << "N:" << fInTS[board] / 1e9 << '\t' << fPreTS[board] / 1e9 << '\t' << fTSDev[board] / 1e9 << '\t' << fUnmatchedCum[board] / 1e9 << '\t' << minDev / 1e9 << '\t' << (TMath::Abs(fTSDev[board] - minDev) < TS_JUDGE_RANGE) << std::endl;
            std::cout << std::setprecision(10) << gInsideEntry << '\t' << std::setprecision(1) << board << std::setprecision(10) << '\t' << fPreEntry[board] << '\t' << (bool)fTSFlag[board] << '\t' << "N:" << fInTS[board] / 1e9 << '\t' << fPreTS[board] / 1e9 << '\t' << fUnmatchedCum[board] / 1e9 << '\t' << fTSDev[board] / 1e9 << '\t' << minDev / 1e9 << '\t' << (fTSDev[board] - minDev) / 1e9 << std::endl;
        }
        // std::cout << board << '\t' << minDev << '\t' << fTSDev[board] << '\t' << fTSDev[board] - minDev - fUnmatchedCum[board] << std::endl;
    }
    // std::cout << std::endl;
    fTree->Fill();
    fTSid++;

    return true;
}

bool tempClass::FindBegining()
{
    if (fBoardCount == 0)
        return false;

    for (int entry = 0;; entry++)
    {
        fInTrees[0]->GetEntry(entry);
        if (fInTS[0] / 1e9 > 30)
            return false;
        fTSBoardCount = 0;
        if (Try_FindBegining(fInTS[0]))
        {
            fTSid = 0;
            fTree->Fill();
            fTSid++;
            return true;
        }
    }
    return true;
}

bool tempClass::Try_FindBegining(double AbsTimeStamp)
{
    for (int board = 0; board < fBoardCount; board++)
    {
        bool flag = 0;
        for (int entry = 0;; entry++)
        {
            fInTrees[board]->GetEntry(entry);
            // time deviation less than 0.2s
            // std::cout << entry << '\t' << board << '\t' << AbsTimeStamp / 1e9 << '\t' << (fInTS[board] - AbsTimeStamp) / 1e9 << std::endl;
            if (TMath::Abs(fInTS[board] - AbsTimeStamp) < 0.1e9)
            {
                flag = 1;
                fPreEntry[board] = entry;
                fPreTS[board] = fInTS[board];
                fUnmatchedCum[board] = 0;

                fTSBoardStat[fTSBoardCount++] = board;
                fTS[board] = fInTS[board];
                fTSFlag[board] = 1;

                break;
            }
            if (fInTS[board] - AbsTimeStamp > 1e9)
            {
                flag = 0;
                break;
            }
        }
        if (flag == 0)
            return false;
    }
    return true;
}

void ProcessTS()
{
    for (int board = 0; board < 6; board++)
    {
        ConvertTS2ROOT(board);
    }
    tempClass a;
    a.Init();
    std::cout << a.FindBegining() << std::endl;
    std::cout << a.SaveNextTS() << std::endl;
    // return;
    int counter = 0;
    for (;; counter++)
    {
        auto flag = a.SaveNextTS();
        if (!flag)
            break;
        // std::cout << flag << std::endl;
        if (counter > 100000)
            break;
    }
    std::cout << counter << std::endl;
    return;
}
#endif

#ifdef TESTDATAMATCHINSIDE

#include <vector>
#include <map>
#include <string>
#include <TFile.h>
#include <TTree.h>
#include <iostream>

#include "board.C"
#include "tsTree.C"

uint64_t gTSReadingEntry = 0;
double gCurrentTS[6];
double gTSInterval[6];

// Matched information
double gLastSeg[6];  // segment time into pieces, length is about 1 s
double gNextSeg[6];  // Same use with gLastTS
double gLastSeg2[6]; // segment time into pieces, length is about 1 s
double gNextSeg2[6]; // Same use with gLastTS

// Initate Segment
double gT0TSInterval2[6];
double gT0TSStart2[6];

tsTree *gTS = NULL;

bool JudgeBoardFlag(bool *flags, int boardNumber)
{
    for (int i = 0; i < boardNumber; i++)
    {
        if (!flags[i])
            return false;
    }
    return true;
}

bool FindStamp(int startEntry, double *stampArray, int *devFromStart)
{
    gTS->fChain->GetEntry(startEntry);

    bool findFlag[6]{0};

    for (int board = 0; board < 6; board++)
    {
        findFlag[board] = 0;
        devFromStart[board] = 0;
    }
    for (int entry = startEntry; !JudgeBoardFlag(findFlag, 6); entry++)
    {
        if (entry >= gTS->fChain->GetEntries())
            return false;
        gTS->GetEntry(entry);
        for (int board = 0; board < 6; board++)
        {
            if (findFlag[board])
                continue;

            if (gTS->tsFlag[board])
            {
                findFlag[board] = true;
                stampArray[board] = gTS->ts[board];
            }
            else
            {
                devFromStart[board]++;
            }
        }
    }
    gTS->fChain->GetEntry(startEntry);
    return true;
}

double gCurrentValidInterval[6]; // Update valid interval real time
double gCurrentValidSegPoint[6]; // Update valid seg point real time
bool UpdateInterval(int startEntry)
{
    double tsPre[6];
    int devCount[6];
    bool tsIntervalFlag[6]{0};

    for (int entry = startEntry; !JudgeBoardFlag(tsIntervalFlag, 6); entry++)
    {
        if (entry >= gTS->fChain->GetEntries())
            return false;
        gTS->GetEntry(entry);
#ifdef DEBUG_INTERVAL
        std::cout << entry << '\t';
        std::cout << "Calculating first time stamp interval" << std::endl;
#endif
        for (int board = 0; board < 6; board++)
        {
            if (gTS->tsFlag[board])
            {
                double tsDev = gTS->ts[board] - tsPre[board];
                tsDev /= devCount[board] + 1;
                if (TMath::Abs(tsDev - 1e9) < 0.01e9)
                {
                    gTSInterval[board] = tsDev;
                    tsIntervalFlag[board] = 1;
                }
#ifdef DEBUG_INTERVAL
                std::cout << gTS->ts[board] / 1e9 << '\t' << tsDev / 1e9 - 1 << '\t';
#endif
                devCount[board] = 0;
                tsPre[board] = gTS->ts[board];
                if (TMath::Abs(gTSInterval[board] / gT0TSInterval2[board] - 1) * 1e6 < 1)
                {
                    gCurrentValidInterval[board] = gTSInterval[board];
                    gCurrentValidSegPoint[board] = gTS->ts[board];
                }
            }
            else
            {
                devCount[board]++;
#ifdef DEBUG_INTERVAL
                std::cout << "Void \t"
                          << "Void \t";
#endif
            }
        }
#ifdef DEBUG_INTERVAL
        std::cout
            << std::endl;
#endif
    }
    return true;
}

void InitiateTS()
{
    gTS = new tsTree("TS.root");
    UpdateInterval(0);

    gTS->GetEntry(0);
    std::cout << "calculated initiated time interval: " << std::endl;
    gTSReadingEntry = 0;
    for (int board = 0; board < 6; board++)
    {
        gLastSeg[board] = gTS->ts[board];
        gCurrentTS[board] = gTS->ts[board];
        std::cout << "Board index: " << board << '\t' << "First Time stamp(s): " << '\t' << gCurrentTS[board] / 1e9 << '\t' << " Time Interval(s): " << gTSInterval[board] / 1e9 << '\t' << std::endl;

        int startIdx = 30;
        int endIdx = gTS->fChain->GetEntries() - 100;
        // int endIdx = 100;
        // int endIdx = 1020;
        double ts1, ts2;
        for (; startIdx < gTS->fChain->GetEntries(); startIdx++)
        {
            gTS->fChain->GetEntry(startIdx);
            if (gTS->tsFlag[board])
                break;
        }
        ts1 = gTS->ts[board];

        for (; endIdx > 0; endIdx--)
        {
            gTS->fChain->GetEntry(endIdx);
            if (gTS->tsFlag[board])
                break;
        }
        ts2 = gTS->ts[board];
        int clockCount = 0;
        double tsPre, tsNow;
        gTS->GetEntry(startIdx);
        tsPre = gTS->ts[board];
        for (int idx = startIdx + 1; idx <= endIdx; idx++)
        {
            gTS->GetEntry(idx);
            tsNow = gTS->ts[board];
            double dev = tsNow - tsPre;
            clockCount += round(dev / 1e9);
            // std::cout << dev << '\t' << round(dev / 1e9) << std::endl;
            tsPre = tsNow;
        }
        // gT0TSInterval2[board] = (ts2 - ts1) / (endIdx - startIdx);
        gT0TSInterval2[board] = (ts2 - ts1) / clockCount;
        gT0TSStart2[board] = (ts1 - startIdx * gT0TSInterval2[board]);
        gCurrentValidInterval[board] = gT0TSInterval2[board];
        gCurrentValidSegPoint[board] = gT0TSStart2[board];
        std::cout << "startIdx: " << startIdx << " endIdx: " << endIdx << " count: " << clockCount << " interval: " << gT0TSInterval2[board] << std::endl
                  << std::endl;
    }
}

#include <TMath.h>
bool UpdateSegPoint(int startEntry)
{
    double stamp[6];
    int dev[6];
    bool flag;
    flag = FindStamp(startEntry, stamp, dev);
    if (!flag)
        return false;
    for (int board = 0; board < 6; board++)
    {
        gLastSeg[board] = stamp[board] - dev[board] * gTSInterval[board];
    }
    flag = FindStamp(startEntry + 1, stamp, dev);
    if (flag)
        for (int board = 0; board < 6; board++)
            gNextSeg[board] = stamp[board] - dev[board] * gTSInterval[board];
    else
        for (int board = 0; board < 6; board++)
            gNextSeg[board] = gLastSeg[board] + gTSInterval[board];

    for (int board = 0; board < 6; board++)
    {
        gLastSeg2[board] = round((gLastSeg[board] - gCurrentValidSegPoint[board]) / gCurrentValidInterval[board]) * gCurrentValidInterval[board] + gCurrentValidSegPoint[board];
        gNextSeg2[board] = round((gNextSeg[board] - gCurrentValidSegPoint[board]) / gCurrentValidInterval[board]) * gCurrentValidInterval[board] + gCurrentValidSegPoint[board];
    }

    // std::cout << "Segment: " << startEntry << '\t';
    // for (int board = 0; board < 6; board++)
    //     std::cout << gLastSeg[board] / 1e9 << '\t' << gNextSeg[board] / 1e9 << '\t';
    // std::cout << std::endl;
    return true;
}

bool UpdateSeg(int startEntry)
{
    return (UpdateInterval(startEntry) && UpdateSegPoint(startEntry));
}

// Each Board information
int gBoardNo[6];
const int gBoardCount = 6;
board *gBoard[6];

void GenerateBoardMap()
{
    for (int i = 0; i < 6; i++)
    {
        gBoardNo[i] = i;
        gBoard[i] = new board(Form("Board%d-Aligned.root", i));
    }
}

TFile *gMatchFile = NULL;
TTree *gMatchTree = NULL;

int gMatchCounter;        // restore match counter
int gMatchedBoard[256];   // restore matched channel
int gMatchFlag[6];        // restore whether this board has matched index
ULong64_t gMatchEntry[6]; // restore entries
double gMatchTime[6];     // restore matched time

uint64_t gCurrentEntries[6]{0}; // Current reading entries
double gT0Delay[6];             // Restore T0Delay for each board, set board 0 at 0

std::vector<double> gEventTime[6];
std::vector<int> gEventEntry[6];

void InitMatchFile()
{
    gMatchFile = new TFile("MatchEntries.root", "recreate");
    gMatchTree = new TTree("match", "matched entries");
    gMatchTree->Branch("counter", &gMatchCounter, "counter/I");
    gMatchTree->Branch("matchedBoard", gMatchedBoard, "matchedBoard[counter]/I");
    gMatchTree->Branch("matchFlag", gMatchFlag, "matchFlag[6]/I");
    gMatchTree->Branch("matchEntry", gMatchEntry, "matchEntry[6]/l");
    gMatchTree->Branch("matchTime", gMatchTime, "matchTime[6]/D");
    gMatchTree->Branch("lastSeg", gLastSeg, "lastSeg[6]/D");
    gMatchTree->Branch("nextSeg", gNextSeg, "nextSeg[6]/D");
    gMatchTree->Branch("lastSeg2", gLastSeg2, "lastSeg2[6]/D");
    gMatchTree->Branch("nextSeg2", gNextSeg2, "nextSeg2[6]/D");
    gMatchTree->Branch("interval", gTSInterval, "interval[6]/D");
    gMatchTree->Branch("interval2", gT0TSInterval2, "interval2[6]/D");
    gMatchTree->Branch("currentInterval", gCurrentValidInterval, "currentInterval[6]/D");
    gMatchTree->Branch("currentSegPoint", gCurrentValidSegPoint, "currentSegPoint[6]/D");
    gMatchTree->AutoSave();
}

int JudgeEventTime(double eventTime, int boardNo)
{
    if (eventTime < gLastSeg[boardNo])
        return -1;
    else if (eventTime < gNextSeg[boardNo])
        return 0;
    else
        return 1;
};
bool FindAllEventsInSeg(int segEntry, int *startEntries, int *endEntries)
{
    if (!UpdateSeg(segEntry))
        return false;
    for (int board = 0; board < 6; board++)
    {
        int idx = gCurrentEntries[board];
        gBoard[board]->GetEntry(idx);
        auto eventTime = gBoard[board]->TDCTime[gBoard[board]->FiredCh[0]];
        if (JudgeEventTime(eventTime, board) < 0)
        {
            for (; JudgeEventTime(eventTime, board) < 0; idx++)
            {
                if (idx < 0 || idx >= gBoard[board]->fChain->GetEntries())
                    break;
                gBoard[board]->GetEntry(idx);
                eventTime = gBoard[board]->TDCTime[gBoard[board]->FiredCh[0]];
            }
            startEntries[board] = idx;
            for (; JudgeEventTime(eventTime, board) == 0; idx++)
            {
                if (idx < 0 || idx >= gBoard[board]->fChain->GetEntries())
                    break;
                gBoard[board]->GetEntry(idx);
                eventTime = gBoard[board]->TDCTime[gBoard[board]->FiredCh[0]];
            }
            endEntries[board] = idx - 1;
        }
        else if (JudgeEventTime(eventTime, board) > 0)
        {
            for (; JudgeEventTime(eventTime, board) <= 0; idx--)
            {
                if (idx < 0 || idx >= gBoard[board]->fChain->GetEntries())
                    break;
                gBoard[board]->GetEntry(idx);
                eventTime = gBoard[board]->TDCTime[gBoard[board]->FiredCh[0]];
            }
            endEntries[board] = idx;
            for (; JudgeEventTime(eventTime, board) == 0; idx--)
            {
                if (idx < 0 || idx >= gBoard[board]->fChain->GetEntries())
                    break;
                gBoard[board]->GetEntry(idx);
                eventTime = gBoard[board]->TDCTime[gBoard[board]->FiredCh[0]];
            }
            startEntries[board] = idx + 1;
        }
        else if (JudgeEventTime(eventTime, board) == 0)
        {
            for (idx = gCurrentEntries[board]; JudgeEventTime(eventTime, board) == 0; idx--)
            {
                if (idx < 0 || idx >= gBoard[board]->fChain->GetEntries())
                    break;
                gBoard[board]->GetEntry(idx);
                eventTime = gBoard[board]->TDCTime[gBoard[board]->FiredCh[0]];
            }
            startEntries[board] = idx + 1;
            for (idx = gCurrentEntries[board]; JudgeEventTime(eventTime, board) == 0; idx++)
            {
                if (idx < 0 || idx >= gBoard[board]->fChain->GetEntries())
                    break;
                gBoard[board]->GetEntry(idx);
                eventTime = gBoard[board]->TDCTime[gBoard[board]->FiredCh[0]];
            }
            startEntries[board] = idx - 1;
        }

        gCurrentEntries[board] = startEntries[board];
    }
    // Match: find how many
    // for (int)
    return true;
}

bool MatchEventsInSeg(int *startEntries, int *endEntries)
{
    int eventsCount[6];
    int judgeCounter[6]; // how many judge left
    int idxCount[6];     // current reading time
    int maxEventsCount = 0;
    int maxBoard = 0;
    for (int board = 0; board < 6; board++)
    {
        eventsCount[board] = endEntries[board] - startEntries[board] + 1;
        if (eventsCount[board] > maxEventsCount)
        {
            maxEventsCount = eventsCount[board];
            maxBoard = board;
        }

        gEventEntry[board].clear();
        gEventTime[board].clear();
        for (int entry = startEntries[board]; entry <= endEntries[board]; entry++)
        {
            gBoard[board]->GetEntry(entry);
            gEventEntry[board].push_back(entry);
            gEventTime[board].push_back(gBoard[board]->TDCTime[gBoard[board]->FiredCh[0]]);
        }
    }
    for (int board = 0; board < 6; board++)
    {
        judgeCounter[board] = maxEventsCount - eventsCount[board];
        if (judgeCounter[board] < 0)
            judgeCounter[board] = 0;
        idxCount[board] = 0;
    }

    for (int idx0 = 0; idx0 < maxEventsCount; idx0++)
    {
        gMatchCounter = 0;
        for (int board = 0; board < 6; board++)
        {
            int &idx = idxCount[board];
            double maxBoardTime = gEventTime[maxBoard][idx];
            bool skipFlag = 0;
            double timeNow;
            double ctimeNow;
            double cmaxBoardTime;
            if (idx >= gEventEntry[board].size())
            {
                skipFlag = 1;
            }
            else
            {
                timeNow = gEventTime[board][idx];
                ctimeNow = (timeNow - gLastSeg[board]) / gTSInterval[board];
                cmaxBoardTime = (maxBoardTime - gLastSeg[maxBoard]) / gTSInterval[maxBoard];
                skipFlag = !(TMath::Abs(ctimeNow - cmaxBoardTime) < 1000e3);
            }
            // Judge range: 2000 ns
            // skipFlag = !(TMath::Abs((ctimeNow - cmaxBoardTime) * 1e9) < 2000);
            if (judgeCounter[board] > 0)
            {
                if (skipFlag)
                {
                    gMatchFlag[board] = 0;
                    judgeCounter[board]--;
                }
                else
                {
                    gMatchedBoard[gMatchCounter++] = board;
                    gMatchEntry[board] = gEventEntry[board][idx];
                    gMatchFlag[board] = 1;
                    gMatchTime[board] = gEventTime[board][idx];
                    idx++;
                }
            }
            else
            {
                gMatchedBoard[gMatchCounter++] = board;
                gMatchEntry[board] = gEventEntry[board][idx];
                gMatchFlag[board] = !skipFlag;
                gMatchTime[board] = gEventTime[board][idx];
                idx++;
            }
        }
        if (gMatchTree)
        {
            gMatchTree->Fill();
        }
    }
    return true;
}

void GenerateMatchKeyTree()
{
    InitiateTS();
    GenerateBoardMap();
    InitMatchFile();
    // UpdateSeg(1000);
    int startEntries[6], endEntries[6];
    double preTime[6];
    for (int entry = 0; entry < 1000000; entry++)
    // for (int entry = 10; entry < 11; entry++)
    {
        auto flag = FindAllEventsInSeg(entry, startEntries, endEntries);
        if (!flag)
            break;
        if (entry % 100 == 0)
            std::cout << entry << std::endl;
        MatchEventsInSeg(startEntries, endEntries);
        // for (int board = 0; board < 6; board++)
        // {
        //     // std::cout << (endEntries[board] - startEntries[board] + 1) << '\t';
        //     // std::cout << startEntries[board] << '-' << endEntries[board] << '\t' << (endEntries[board] - startEntries[board] + 1) << '\t';
        //     continue;
        //     std::cout << board << std::endl;
        //     std::cout << gLastSeg[board] / 1e9 << '\t' << gNextSeg[board] / 1e9 << '\t' << gTSInterval[board] / 1e9 << '\t' << (gNextSeg[board] - gLastSeg[board]) / (gTSInterval[board]) - 1 << std::endl;
        //     std::cout << startEntries[board] << '\t' << endEntries[board] << std::endl;
        //     // for (int entries = startEntries[board]; entries <= endEntries[board]; entries++)
        //     // {
        //     //     gBoard[board]->GetEntry(entries);
        //     //     auto eventTime = gBoard[board]->TDCTime[gBoard[board]->FiredCh[0]];
        //     //     // std::cout << eventTime / 1e9 << '\t';
        //     //     // std::cout << (eventTime - gLastSeg[board]) / gTSInterval[board] << '\t';
        //     //     double correctedTime = (eventTime - gLastSeg[board]) / gTSInterval[board] * 1e9;
        //     //     std::cout << Form("%.1f", correctedTime - preTime[board]) << '\t';

        //     //     preTime[board] = correctedTime;
        //     // }
        //     // std::cout << std::endl;
        //     // std::cout << std::endl;
        // }
        // std::cout << std::endl;
    }
    gMatchTree->Write();
    gMatchFile->Close();
}

void MatchBoards()
{
    GenerateMatchKeyTree();
    // InitiateTS();
}

#endif