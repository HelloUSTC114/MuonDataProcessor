#include <iostream>
#include <iomanip>
#include <fstream>
#include <TFile.h>
#include <TTree.h>
#define TS_JUDGE_RANGE (2e8)

#ifdef TEST_INSIDE

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