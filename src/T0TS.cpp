#include "T0TS.h"

#include <QDir>

#define TS_JUDGE_RANGE (2e8)

namespace T0Process
{
    int MatchT0(std::vector<int> boardArray, std::string txtInPath, std::string sT0FinalROOTPath)
    {
        QDir dir;
        std::string sROOTOutPath = txtInPath + "./T0TS/";
        dir.mkdir(QString::fromStdString(sROOTOutPath));
        gT0Manager->ForceClear();
        gT0Manager->InitBoardArray(boardArray, txtInPath, sROOTOutPath);
        gT0Manager->ConvertAllTS();

        std::string sT0FinalROOTFile = txtInPath + "./TS.root";
        gT0Manager->InitT0Matching(sT0FinalROOTFile);
        gT0Manager->FindBegining();
        int counter = 0;
        for (;;)
        {
            auto flag = gT0Manager->SaveNextTS();
            if (!flag)
                break;
        }
        return counter;
    }

    int ConvertTS2ROOT(std::string sInfile, std::string sOutFile)
    {
        int entries;
        double startT0, endT0;
        return ConvertTS2ROOT(sInfile, sOutFile, entries, startT0, endT0);
    }

    int ConvertTS2ROOT(std::string sInfile, std::string sOutFile, int &entries, double &startT0, double &endT0)
    {
        // Judge whether exists sOutFile
        std::ifstream finTemp(sOutFile.c_str());
        if (finTemp.is_open())
        {
            finTemp.close();
            auto file = new TFile(sOutFile.c_str());
            auto tree = (TTree *)(file->Get("ts"));

            uint32_t tsid = 0;
            double ts;
            tree->SetBranchAddress("tsid", &tsid);
            tree->SetBranchAddress("ts", &ts);
            entries = tree->GetEntries();

            tree->GetEntry(0);
            startT0 = ts;
            tree->GetEntry(entries - 1);
            endT0 = ts;
            // std::cout << std::setprecision(11) << entries << '\t' << startT0 / 1e9 << '\t' << endT0 / 1e9 << std::endl;

            return 0;
        }

        auto file = new TFile(sOutFile.c_str(), "recreate");
        if (!file->IsWritable())
        {
            std::cout << "Error: Cannot openfile: " << sOutFile << std::endl;
            return 0;
        }
        auto tree = new TTree("ts", "T0 time stamp");

        uint32_t tsid = 0;
        double ts;
        tree->Branch("tsid", &tsid, "tsid/i");
        tree->Branch("ts", &ts, "ts/D");

        std::ifstream fin;
        fin.open(sInfile.c_str());
        double preTS = 0;
        int counter = 0;
        for (int row = 0; fin.is_open() && !fin.eof() && fin.good(); row++)
        {
            int boardID;
            int iddev;
            double tsTemp;
            fin >> boardID >> iddev >> tsTemp;

            //  Judge whether time deviation is larger than 1000 ns
            if ((tsTemp - preTS) < TS_JUDGE_RANGE && (row > 0))
                continue;
            if (fin.eof())
                break;

            tsid = boardID;
            ts = tsTemp;
            tree->Fill();
            counter++;
            preTS = tsTemp;
        }

        // Assign statistics variables
        entries = counter;
        tree->GetEntry(0);
        startT0 = ts;
        tree->GetEntry(counter - 1);
        endT0 = ts;

        tree->Write();
        file->Close();
        delete file;
        return counter;
    }
}

BoardT0Manager::~BoardT0Manager()
{
    CloseFile();
}

void BoardT0Manager::ForceClear()
{
    fPathInitFlag = 0;
    fAddBoardCloseFlag = 0;
    CloseFile();
}

BoardT0Manager *BoardT0Manager::Instance()
{
    static auto instance = new BoardT0Manager;
    return instance;
}

bool BoardT0Manager::InitBoardArray(std::vector<int> boardArray, std::string txtInPath, std::string ROOTOutPath)
{
    if (fPathInitFlag)
        return false;
    if (fAddBoardCloseFlag)
        return false;
    fPathInitFlag = true;
    fBoardArray = boardArray;
    fsTXTInPath = txtInPath;
    fsROOTOutPath = ROOTOutPath;
    return true;
}

bool BoardT0Manager::AddBoard(int board)
{
    if (!fPathInitFlag)
        return false;
    if (fAddBoardCloseFlag)
        return false;
    if (std::find(fBoardArray.begin(), fBoardArray.end(), board) != fBoardArray.end())
        return false;
    std::ifstream fin;
    fin.open(Form("%s/Board%dTS.txt", board));
    if (!fin.is_open())
        return false;

    fin.close();
    fBoardArray.push_back(board);
    return true;
}

bool BoardT0Manager::UpdateTXTROOTPath(std::string txtInPath, std::string ROOTOutPath)
{
    if (fAddBoardCloseFlag)
        return false;
    fsTXTInPath = txtInPath;
    fsROOTOutPath = ROOTOutPath;
    return true;
}

void BoardT0Manager::ConvertAllTS()
{
    fAddBoardCloseFlag = 1;
    int entries;
    double startT0, endT0;
    for (int i = 0; i < fBoardArray.size(); i++)
    {
        ConvertTS(i, entries, startT0, endT0);
    }
}

int BoardT0Manager::ConvertTS(int idx, int &entries, double &startT0, double &endT0)
{
    if (idx < 0 || idx >= fBoardArray.size())
        return 0;
    std::string sTxtFile = fsTXTInPath + "/" + Form("Board%dTS.txt", fBoardArray[idx]);
    std::string sROOTFile = fsROOTOutPath + "/" + Form("Board%dTS.root", fBoardArray[idx]);
    return T0Process::ConvertTS2ROOT(sTxtFile, sROOTFile, entries, startT0, endT0);
}

bool BoardT0Manager::InitT0Matching(std::string sFileName)
{
    if (!fAddBoardCloseFlag)
        return false;

    CloseFile();
    fOutFile = new TFile(sFileName.c_str(), "recreate");

    // Save board number information
    fBNTree = new TTree("bnTree", "Tree save all board number"); // Board number tree
    uint32_t boardNo;
    fBNTree->Branch("bn", &boardNo, "bn/i");
    // fBoardArray = boardArray;
    fBoardCount = fBoardArray.size();
    for (int i = 0; i < fBoardCount; i++)
    {
        boardNo = fBoardArray[i];
        fBNTree->Fill();
        fMatchedCounter.push_back(0);
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
        fInFiles[i] = new TFile(Form("%s/Board%dTS.root", fsROOTOutPath.c_str(), fBoardArray[i]));
        if (!fInFiles[i]->IsOpen())
        {
            std::cout << "No files: " << i << std::endl;
            CloseFile();
            return false;
        }
        fInTrees[i] = (TTree *)(fInFiles[i]->Get("ts"));
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

void BoardT0Manager::CloseFile()
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
    fMatchedCounter.clear();
}

bool BoardT0Manager::SaveNextTS()
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
#ifdef DEBUG_T0DEV
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
#endif

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
            fMatchedCounter[board]++;

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
            // std::cout << std::setprecision(10) << gInsideEntry << '\t' << std::setprecision(1) << board << std::setprecision(10) << '\t' << fPreEntry[board] << '\t' << (bool)fTSFlag[board] << '\t' << "N:" << fInTS[board] / 1e9 << '\t' << fPreTS[board] / 1e9 << '\t' << fUnmatchedCum[board] / 1e9 << '\t' << fTSDev[board] / 1e9 << '\t' << minDev / 1e9 << '\t' << (fTSDev[board] - minDev) / 1e9 << std::endl;
            std::cout << std::setprecision(1) << board << std::setprecision(10) << '\t' << fPreEntry[board] << '\t' << (bool)fTSFlag[board] << '\t' << "N:" << fInTS[board] / 1e9 << '\t' << fPreTS[board] / 1e9 << '\t' << fUnmatchedCum[board] / 1e9 << '\t' << fTSDev[board] / 1e9 << '\t' << minDev / 1e9 << '\t' << (fTSDev[board] - minDev) / 1e9 << std::endl;
        }
        // std::cout << board << '\t' << minDev << '\t' << fTSDev[board] << '\t' << fTSDev[board] - minDev - fUnmatchedCum[board] << std::endl;
    }
    // std::cout << std::endl;
    fTree->Fill();
    fTSid++;

    return true;
}

bool BoardT0Manager::FindBegining()
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
            for (int i = 0; i < fMatchedCounter.size(); i++)
                fMatchedCounter[i]++;
            return true;
        }
    }
    return true;
}

bool BoardT0Manager::Try_FindBegining(double AbsTimeStamp)
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

#include "Trees.h"

DataMatchManager *DataMatchManager::Instance()
{
    static auto instance = new DataMatchManager();
    return instance;
}

bool DataMatchManager::InitBoardArray(std::vector<int> boardArray)
{
    if (fBoardArrayInitFlag)
        return false;

    fBoardArray = boardArray;
    if (boardArray.size() > MAX_BOARD_COUNTS)
        fBoardArray.resize(MAX_BOARD_COUNTS);
    fBoardCount = fBoardArray.size();
    return true;
}

void DataMatchManager::CloseFile()
{
    if (fMatchFile)
    {
        if (fMatchTree)
        {
            fMatchTree->Write();
            fMatchTree = NULL;
        }
        if (fBNTree)
        {
            fBNTree->Write();
            fBNTree = NULL;
        }
        delete fMatchFile;
        fMatchFile = NULL;
    }
    for (int board = 0; board < fBoardCount; board++)
        if (fBoard[board])
        {
            delete fBoard[board];
            fBoard[board] = NULL;
        }
    if (fTS)
    {
        delete fTS;
        fTS = NULL;
    }

    fsBoardDataFolder = "";
    fsT0TSFile = "";
    fBoardArray.clear();
    fBoardCount = 0;
    fBoardArrayInitFlag = 0;
    fT0Flag = 0;
    fBoardFlag = 0;
}

bool DataMatchManager::JudgeBoardFlag(bool *flags, int boardNumber)
{
    for (int i = 0; i < boardNumber; i++)
    {
        if (!flags[i])
            return false;
    }
    return true;
}

bool DataMatchManager::FindStamp(int startEntry, double *stampArray, int *devFromStart)
{
    fTS->fChain->GetEntry(startEntry);

    bool findFlag[MAX_BOARD_COUNTS]{0};

    for (int board = 0; board < fBoardCount; board++)
    {
        findFlag[board] = 0;
        devFromStart[board] = 0;
    }
    for (int entry = startEntry; !JudgeBoardFlag(findFlag, fBoardCount); entry++)
    {
        if (entry >= fTS->fChain->GetEntries())
            return false;
        fTS->GetEntry(entry);
        for (int board = 0; board < fBoardCount; board++)
        {
            if (findFlag[board])
                continue;

            if (fTS->tsFlag[board])
            {
                findFlag[board] = true;
                stampArray[board] = fTS->ts[board];
            }
            else
            {
                devFromStart[board]++;
            }
        }
    }
    fTS->fChain->GetEntry(startEntry);
    return true;
}

bool DataMatchManager::UpdateInterval(int startEntry)
{
    double tsPre[MAX_BOARD_COUNTS];
    int devCount[MAX_BOARD_COUNTS];
    bool tsIntervalFlag[MAX_BOARD_COUNTS]{0};

    for (int entry = startEntry; !JudgeBoardFlag(tsIntervalFlag, fBoardCount); entry++)
    {
        if (entry >= fTS->fChain->GetEntries())
            return false;
        fTS->GetEntry(entry);
#ifdef DEBUG_INTERVAL
        std::cout << entry << '\t';
        std::cout << "Calculating first time stamp interval" << std::endl;
#endif
        for (int board = 0; board < fBoardCount; board++)
        {
            if (fTS->tsFlag[board])
            {
                double tsDev = fTS->ts[board] - tsPre[board];
                tsDev /= devCount[board] + 1;
                if (TMath::Abs(tsDev - 1e9) < 0.01e9)
                {
                    fTSInterval[board] = tsDev;
                    tsIntervalFlag[board] = 1;
                }
#ifdef DEBUG_INTERVAL
                std::cout << fTS->ts[board] / 1e9 << '\t' << tsDev / 1e9 - 1 << '\t';
#endif
                devCount[board] = 0;
                tsPre[board] = fTS->ts[board];
                if (TMath::Abs(fTSInterval[board] / fT0TSInterval2[board] - 1) * 1e6 < 1)
                {
                    fCurrentValidInterval[board] = fTSInterval[board];
                    fCurrentValidSegPoint[board] = fTS->ts[board];
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

bool DataMatchManager::InitiateT0TS(std::string sT0TSFile)
{
    if (fT0Flag)
        return false;
    fTS = new ROOTTrees::tsTree(sT0TSFile.c_str());
    fT0Flag = 1;

    UpdateInterval(0);

    fTS->GetEntry(0);
    std::cout << "calculated initiated time interval: " << std::endl;
    fTSReadingEntry = 0;
    for (int board = 0; board < fBoardCount; board++)
    {
        fLastSeg[board] = fTS->ts[board];
        std::cout << "Board index: " << board << '\t' << "First Time stamp(s): " << '\t' << fLastSeg[board] / 1e9 << '\t' << " Time Interval(s): " << fTSInterval[board] / 1e9 << '\t' << std::endl;

        int startIdx = 30;
        int endIdx = fTS->fChain->GetEntries() - 100;
        // int endIdx = 100;
        // int endIdx = 1020;
        double ts1, ts2;
        for (; startIdx < fTS->fChain->GetEntries(); startIdx++)
        {
            fTS->fChain->GetEntry(startIdx);
            if (fTS->tsFlag[board])
                break;
        }
        ts1 = fTS->ts[board];

        for (; endIdx > 0; endIdx--)
        {
            fTS->fChain->GetEntry(endIdx);
            if (fTS->tsFlag[board])
                break;
        }
        ts2 = fTS->ts[board];
        int clockCount = 0;
        double tsPre, tsNow;
        fTS->GetEntry(startIdx);
        tsPre = fTS->ts[board];
        for (int idx = startIdx + 1; idx <= endIdx; idx++)
        {
            fTS->GetEntry(idx);
            tsNow = fTS->ts[board];
            double dev = tsNow - tsPre;
            clockCount += round(dev / 1e9);
            // std::cout << dev << '\t' << round(dev / 1e9) << std::endl;
            tsPre = tsNow;
        }
        // fT0TSInterval2[board] = (ts2 - ts1) / (endIdx - startIdx);
        fT0TSInterval2[board] = (ts2 - ts1) / clockCount;
        fT0TSStart2[board] = (ts1 - startIdx * fT0TSInterval2[board]);
        fCurrentValidInterval[board] = fT0TSInterval2[board];
        fCurrentValidSegPoint[board] = fT0TSStart2[board];
        std::cout << "startIdx: " << startIdx << " endIdx: " << endIdx << " count: " << clockCount << " interval: " << fT0TSInterval2[board] << std::endl
                  << std::endl;
    }
    return true;
}

bool DataMatchManager::UpdateSegPoint(int startEntry)
{
    double stamp[MAX_BOARD_COUNTS];
    int dev[MAX_BOARD_COUNTS];
    bool flag;
    flag = FindStamp(startEntry, stamp, dev);
    if (!flag)
        return false;
    for (int board = 0; board < fBoardCount; board++)
    {
        fLastSeg[board] = stamp[board] - dev[board] * fTSInterval[board];
    }
    flag = FindStamp(startEntry + 1, stamp, dev);
    if (flag)
        for (int board = 0; board < fBoardCount; board++)
            fNextSeg[board] = stamp[board] - dev[board] * fTSInterval[board];
    else
        for (int board = 0; board < fBoardCount; board++)
            fNextSeg[board] = fLastSeg[board] + fTSInterval[board];

    for (int board = 0; board < fBoardCount; board++)
    {
        fLastSeg2[board] = round((fLastSeg[board] - fCurrentValidSegPoint[board]) / fCurrentValidInterval[board]) * fCurrentValidInterval[board] + fCurrentValidSegPoint[board];
        fNextSeg2[board] = round((fNextSeg[board] - fCurrentValidSegPoint[board]) / fCurrentValidInterval[board]) * fCurrentValidInterval[board] + fCurrentValidSegPoint[board];
    }

    // std::cout << "Segment: " << startEntry << '\t';
    // for (int board = 0; board < fBoardCount; board++)
    //     std::cout << fLastSeg[board] / 1e9 << '\t' << fNextSeg[board] / 1e9 << '\t';
    // std::cout << std::endl;
    return true;
}

bool DataMatchManager::GenerateBoardMap(std::string sBoardDataFolder)
{
    if (fBoardFlag)
        return false;
    fsBoardDataFolder = sBoardDataFolder;
    for (int i = 0; i < fBoardCount; i++)
        fBoard[i] = new ROOTTrees::board(Form("%s/Board%d-Aligned.root", fsBoardDataFolder.c_str(), fBoardArray[i]));

    fBoardFlag = 1;
    return true;
}

void DataMatchManager::InitMatchFile()
{
    fMatchFile = new TFile("MatchEntries.root", "recreate");
    fMatchTree = new TTree("match", "matched entries");
    fMatchTree->Branch("counter", &fMatchCounter, "counter/I");
    fMatchTree->Branch("matchedBoard", fMatchedBoard, "matchedBoard[counter]/I");
    fMatchTree->Branch("matchFlag", fMatchFlag, Form("matchFlag[%d]/I", fBoardCount));
    fMatchTree->Branch("matchEntry", fMatchEntry, Form("matchEntry[%d]/l", fBoardCount));
    fMatchTree->Branch("matchTime", fMatchTime, Form("matchTime[%d]/D", fBoardCount));
    fMatchTree->Branch("lastSeg", fLastSeg, Form("lastSeg[%d]/D", fBoardCount));
    fMatchTree->Branch("nextSeg", fNextSeg, Form("nextSeg[%d]/D", fBoardCount));
    fMatchTree->Branch("lastSeg2", fLastSeg2, Form("lastSeg2[%d]/D", fBoardCount));
    fMatchTree->Branch("nextSeg2", fNextSeg2, Form("nextSeg2[%d]/D", fBoardCount));
    fMatchTree->Branch("interval", fTSInterval, Form("interval[%d]/D", fBoardCount));
    fMatchTree->Branch("interval2", fT0TSInterval2, Form("interval2[%d]/D", fBoardCount));
    fMatchTree->Branch("currentInterval", fCurrentValidInterval, Form("currentInterval[%d]/D", fBoardCount));
    fMatchTree->Branch("currentSegPoint", fCurrentValidSegPoint, Form("currentSegPoint[%d]/D", fBoardCount));
    fMatchTree->AutoSave();

    // Save board number information
    fBNTree = new TTree("bnTree", "Tree save all board number"); // Board number tree
    uint32_t boardNo;
    fBNTree->Branch("bn", &boardNo, "bn/i");
    for (int i = 0; i < fBoardCount; i++)
    {
        boardNo = fBoardArray[i];
        fBNTree->Fill();
    }
}

int DataMatchManager::JudgeEventTime(double eventTime, int boardNo)
{
    if (eventTime < fLastSeg[boardNo])
        return -1;
    else if (eventTime < fNextSeg[boardNo])
        return 0;
    else
        return 1;
}

bool DataMatchManager::FindAllEventsInSeg(int segEntry, int *startEntries, int *endEntries)
{
    if (!UpdateSeg(segEntry))
        return false;
    for (int board = 0; board < fBoardCount; board++)
    {
        int idx = fCurrentEntries[board];
        fBoard[board]->GetEntry(idx);
        auto eventTime = fBoard[board]->TDCTime[fBoard[board]->FiredCh[0]];
        if (JudgeEventTime(eventTime, board) < 0)
        {
            for (; JudgeEventTime(eventTime, board) < 0; idx++)
            {
                if (idx < 0 || idx >= fBoard[board]->fChain->GetEntries())
                    break;
                fBoard[board]->GetEntry(idx);
                eventTime = fBoard[board]->TDCTime[fBoard[board]->FiredCh[0]];
            }
            startEntries[board] = idx;
            for (; JudgeEventTime(eventTime, board) == 0; idx++)
            {
                if (idx < 0 || idx >= fBoard[board]->fChain->GetEntries())
                    break;
                fBoard[board]->GetEntry(idx);
                eventTime = fBoard[board]->TDCTime[fBoard[board]->FiredCh[0]];
            }
            endEntries[board] = idx - 1;
        }
        else if (JudgeEventTime(eventTime, board) > 0)
        {
            for (; JudgeEventTime(eventTime, board) <= 0; idx--)
            {
                if (idx < 0 || idx >= fBoard[board]->fChain->GetEntries())
                    break;
                fBoard[board]->GetEntry(idx);
                eventTime = fBoard[board]->TDCTime[fBoard[board]->FiredCh[0]];
            }
            endEntries[board] = idx;
            for (; JudgeEventTime(eventTime, board) == 0; idx--)
            {
                if (idx < 0 || idx >= fBoard[board]->fChain->GetEntries())
                    break;
                fBoard[board]->GetEntry(idx);
                eventTime = fBoard[board]->TDCTime[fBoard[board]->FiredCh[0]];
            }
            startEntries[board] = idx + 1;
        }
        else if (JudgeEventTime(eventTime, board) == 0)
        {
            for (idx = fCurrentEntries[board]; JudgeEventTime(eventTime, board) == 0; idx--)
            {
                if (idx < 0 || idx >= fBoard[board]->fChain->GetEntries())
                    break;
                fBoard[board]->GetEntry(idx);
                eventTime = fBoard[board]->TDCTime[fBoard[board]->FiredCh[0]];
            }
            startEntries[board] = idx + 1;
            for (idx = fCurrentEntries[board]; JudgeEventTime(eventTime, board) == 0; idx++)
            {
                if (idx < 0 || idx >= fBoard[board]->fChain->GetEntries())
                    break;
                fBoard[board]->GetEntry(idx);
                eventTime = fBoard[board]->TDCTime[fBoard[board]->FiredCh[0]];
            }
            startEntries[board] = idx - 1;
        }

        fCurrentEntries[board] = startEntries[board];
    }
    // Match: find how many
    // for (int)
    return true;
}

bool DataMatchManager::MatchEventsInSeg(int *startEntries, int *endEntries)
{
    int eventsCount[MAX_BOARD_COUNTS];
    int judgeCounter[MAX_BOARD_COUNTS]; // how many judge left
    int idxCount[MAX_BOARD_COUNTS];     // current reading time
    int maxEventsCount = 0;
    int maxBoard = 0;
    for (int board = 0; board < fBoardCount; board++)
    {
        eventsCount[board] = endEntries[board] - startEntries[board] + 1;
        if (eventsCount[board] > maxEventsCount)
        {
            maxEventsCount = eventsCount[board];
            maxBoard = board;
        }

        fEventEntry[board].clear();
        fEventTime[board].clear();
        for (int entry = startEntries[board]; entry <= endEntries[board]; entry++)
        {
            fBoard[board]->GetEntry(entry);
            fEventEntry[board].push_back(entry);
            fEventTime[board].push_back(fBoard[board]->TDCTime[fBoard[board]->FiredCh[0]]);
        }
    }
    for (int board = 0; board < fBoardCount; board++)
    {
        judgeCounter[board] = maxEventsCount - eventsCount[board];
        if (judgeCounter[board] < 0)
            judgeCounter[board] = 0;
        idxCount[board] = 0;
    }

    for (int idx0 = 0; idx0 < maxEventsCount; idx0++)
    {
        fMatchCounter = 0;
        for (int board = 0; board < fBoardCount; board++)
        {
            int &idx = idxCount[board];
            double maxBoardTime = fEventTime[maxBoard][idx];
            bool skipFlag = 0;
            double timeNow;
            double ctimeNow;
            double cmaxBoardTime;
            if (idx >= fEventEntry[board].size())
            {
                skipFlag = 1;
            }
            else
            {
                timeNow = fEventTime[board][idx];
                ctimeNow = (timeNow - fLastSeg[board]) / fTSInterval[board];
                cmaxBoardTime = (maxBoardTime - fLastSeg[maxBoard]) / fTSInterval[maxBoard];
                skipFlag = !(TMath::Abs(ctimeNow - cmaxBoardTime) < 1000e3);
            }
            // Judge range: 2000 ns
            // skipFlag = !(TMath::Abs((ctimeNow - cmaxBoardTime) * 1e9) < 2000);
            if (judgeCounter[board] > 0)
            {
                if (skipFlag)
                {
                    fMatchFlag[board] = 0;
                    judgeCounter[board]--;
                }
                else
                {
                    fMatchedBoard[fMatchCounter++] = board;
                    fMatchEntry[board] = fEventEntry[board][idx];
                    fMatchFlag[board] = 1;
                    fMatchTime[board] = fEventTime[board][idx];
                    idx++;
                }
            }
            else
            {
                fMatchedBoard[fMatchCounter++] = board;
                fMatchEntry[board] = fEventEntry[board][idx];
                fMatchFlag[board] = !skipFlag;
                fMatchTime[board] = fEventTime[board][idx];
                idx++;
            }
        }
        if (fMatchTree)
        {
            fMatchTree->Fill();
        }
    }
    return true;
}

void DataMatchManager::MatchBoards()
{
    InitiateT0TS();
    GenerateBoardMap();
    InitMatchFile();
    // UpdateSeg(1000);
    int startEntries[MAX_BOARD_COUNTS], endEntries[MAX_BOARD_COUNTS];
    double preTime[MAX_BOARD_COUNTS];
    for (int entry = 0; entry < 1000000; entry++)
    // for (int entry = 10; entry < 11; entry++)
    {
        auto flag = FindAllEventsInSeg(entry, startEntries, endEntries);
        if (!flag)
            break;
        if (entry % 100 == 0)
            std::cout << entry << std::endl;
        MatchEventsInSeg(startEntries, endEntries);
        // for (int board = 0; board < fBoardCount; board++)
        // {
        //     // std::cout << (endEntries[board] - startEntries[board] + 1) << '\t';
        //     // std::cout << startEntries[board] << '-' << endEntries[board] << '\t' << (endEntries[board] - startEntries[board] + 1) << '\t';
        //     continue;
        //     std::cout << board << std::endl;
        //     std::cout << fLastSeg[board] / 1e9 << '\t' << fNextSeg[board] / 1e9 << '\t' << fTSInterval[board] / 1e9 << '\t' << (fNextSeg[board] - fLastSeg[board]) / (fTSInterval[board]) - 1 << std::endl;
        //     std::cout << startEntries[board] << '\t' << endEntries[board] << std::endl;
        //     // for (int entries = startEntries[board]; entries <= endEntries[board]; entries++)
        //     // {
        //     //     fBoard[board]->GetEntry(entries);
        //     //     auto eventTime = fBoard[board]->TDCTime[fBoard[board]->FiredCh[0]];
        //     //     // std::cout << eventTime / 1e9 << '\t';
        //     //     // std::cout << (eventTime - fLastSeg[board]) / fTSInterval[board] << '\t';
        //     //     double correctedTime = (eventTime - fLastSeg[board]) / fTSInterval[board] * 1e9;
        //     //     std::cout << Form("%.1f", correctedTime - preTime[board]) << '\t';

        //     //     preTime[board] = correctedTime;
        //     // }
        //     // std::cout << std::endl;
        //     // std::cout << std::endl;
        // }
        // std::cout << std::endl;
    }
    CloseFile();
}
