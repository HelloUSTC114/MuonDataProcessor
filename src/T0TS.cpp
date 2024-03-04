#include "T0TS.h"

#include <QDir>
#include <QDebug>

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
            delete file;

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
        int sgMultiReadCount = 0;
        double sgMultiReadTS[5];
        uint32_t sgCurrentID = 0;
        int sgCurrentDev;
        int boardID;
        int iddev;
        double tsTemp;

        for (int row = 0; fin.is_open() && !fin.eof() && fin.good(); row++)
        {
            fin >> boardID >> iddev >> tsTemp;
            if (boardID == 0)
                continue;
            // if iddev > 1, judge whether it is a multi read
            if (iddev > 1)
            {
                if (sgMultiReadCount == 0)
                {
                    sgCurrentID = boardID;
                    sgCurrentDev = iddev;
                }
                // Judge whether it's an old multi read
                if (sgCurrentID == boardID)
                {
                    sgMultiReadTS[sgMultiReadCount] = tsTemp;
                    sgMultiReadCount++;
                    continue;
                }
            }
            // Process old multi read
            if (sgMultiReadCount > 0)
            {
                for (int idx = 0; idx < sgMultiReadCount; idx++)
                {
                    tsid = sgCurrentID - (sgMultiReadCount - 1) + idx;
                    ts = sgMultiReadTS[idx];

                    double sgTSDev = TMath::Abs(sgMultiReadTS[idx] - sgMultiReadTS[idx + 1]);
                    if (idx < sgMultiReadCount - 1 && sgTSDev < 0.8e9)
                        continue;

                    tree->Fill();
                    counter++;
                    preTS = ts;
                }
                preTS = sgMultiReadTS[sgMultiReadCount - 1];
                sgMultiReadCount = 0;

                // Process a new multi read
                if (iddev > 1)
                {
                    sgCurrentID = boardID;
                    sgCurrentDev = iddev;
                    sgMultiReadTS[sgMultiReadCount] = tsTemp;
                    sgMultiReadCount++;
                    continue;
                }
            }

            //  Judge whether time deviation is larger than 1000 ns

            if (fin.eof())
                break;

            if (row < 5 && tsTemp > 10e9)
                continue;
            if ((tsTemp - preTS) < TS_JUDGE_RANGE && (row > 0))
                continue;

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

void BoardT0Manager::ConvertAllTS(std::map<int, std::string> sBoardMap)
{
    fAddBoardCloseFlag = 1;
    int entries;
    double startT0, endT0;
    for (int i = 0; i < fBoardArray.size(); i++)
    {
        ConvertTS(i, sBoardMap, entries, startT0, endT0);
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

int BoardT0Manager::ConvertTS(int idx, std::map<int, std::string> sBoardMap, int &entries, double &startT0, double &endT0)
{
    if (idx < 0 || idx >= fBoardArray.size())
        return 0;
    std::string sTxtFile = sBoardMap[fBoardArray[idx]];
    std::string sROOTFile = fsROOTOutPath + "/" + Form("Board%dTS.root", fBoardArray[idx]);
    auto rtn = T0Process::ConvertTS2ROOT(sTxtFile, sROOTFile, entries, startT0, endT0);
    return rtn;
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
    fInsideTSidStart.resize(fBoardCount);
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
        fInTrees[i]->SetBranchAddress("tsid", &fInTSid[i]);
        fInTrees[i]->GetEntry(0);
        fInsideTSidStart[i] = fInTSid[i];

        fPreEntry[i] = 0;
        fPreTS[i] = 0;
        fUnmatchedCum[i] = 0;
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
            std::cout << "Unmatched: " << std::setprecision(1) << board << '\t' << std::setprecision(10) << fPreEntry[board] << '\t' << (bool)fTSFlag[board] << '\t' << "N:" << fInTS[board] / 1e9 << '\t' << fPreTS[board] / 1e9 << '\t' << fUnmatchedCum[board] / 1e9 << '\t' << fTSDev[board] / 1e9 << '\t' << minDev / 1e9 << '\t' << (fTSDev[board] - minDev) / 1e9 << std::endl;
        }
        // std::cout << board << '\t' << minDev << '\t' << fTSDev[board] << '\t' << fTSDev[board] - minDev - fUnmatchedCum[board] << std::endl;
    }
    // std::cout << std::endl;
    fTree->Fill();
    fTSid++;

    return true;
}

int BoardT0Manager::MatchByID()
{
    std::for_each(fMatchedCounter.begin(), fMatchedCounter.end(), [](int &i)
                  { i = 0; });
    for (int boardNo = 0; boardNo < fBoardCount; boardNo++)
    {
        fInEntries[boardNo] = fInTrees[boardNo]->GetEntries();
        fCurrentEntry[boardNo] = 0;
    }
    for (int loopCounter = 0; MatchByIDOnce() >= 0; loopCounter++)
        std::for_each(fMatchedCounter.begin(), fMatchedCounter.end(), [](int &i)
                      { i++; });
    return 0;
}

#include <algorithm>
int BoardT0Manager::MatchByIDOnce()
{
    int loopCounter = 0;
    for (loopCounter = 0;; loopCounter++)
    {
        for (int boardNo = 0; boardNo < fBoardCount; boardNo++)
        {
            if (fCurrentEntry[boardNo] >= fInEntries[boardNo])
                return -1;
            fInTrees[boardNo]->GetEntry(fCurrentEntry[boardNo]);
            if (fInTS[boardNo] < 0)
                return -2;
        }

        // Find the maximum time stamp id
        for (int idx = 0; idx < fBoardCount; idx++)
            fInTSid[idx] -= fInsideTSidStart[idx];
        auto maxid_idx = std::max_element(fInTSid, fInTSid + fBoardCount);
        auto maxid = *maxid_idx;

        // Judge whether all time stamp id are equal
        bool idEqualFlag = 1;
        for (int boardNo = 0; boardNo < fBoardCount; boardNo++)
        {
            if (fInTSid[boardNo] < maxid)
            {
                idEqualFlag = 0;
                fCurrentEntry[boardNo]++;
            }
        }

        if (idEqualFlag)
            break;
    }

    // Fill trees

    fTSBoardCount = fBoardCount;
    for (int boardNo = 0; boardNo < fBoardCount; boardNo++)
    {
        fTSBoardStat[boardNo] = boardNo;
        fTS[boardNo] = fInTS[boardNo];
        fTSFlag[boardNo] = 1;
        // qDebug() << boardNo << fInTSid[boardNo] << fTS[boardNo] - fTS[0];
    }
    // qDebug() << endl;
    fTree->Fill();
    fTSid++;
    for (int boardNo = 0; boardNo < fBoardCount; boardNo++)
        fCurrentEntry[boardNo]++;

    return loopCounter;
}

void BoardT0Manager::SetTSidStartVector(int board, std::vector<uint32_t> TSidStart)
{
    if (board < 0 || board >= fBoardCount)
        return;
    fInsideTSidStart = TSidStart;
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
    // Easy to get infinite loop here, because if not find at first, then it will compare all elements for all boards
    for (int board = 0; board < fBoardCount; board++)
    {
        bool flag = 0;
        for (int entry = 0;; entry++)
        {
            fInTrees[board]->GetEntry(entry);
            // time deviation less than 0.2s
            // std::cout << entry << '\t' << board << '\t' << AbsTimeStamp / 1e9 << '\t' << (fInTS[board] - AbsTimeStamp) / 1e9 << std::endl;
            if (TMath::Abs(fInTS[board] - AbsTimeStamp) < 0.9e9)
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
    fOutFlag = 0;
    fMatchedFlag = 0;
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
    if (!fTS->fChain)
    {
        fT0Flag = 0;
        return false;
    }
    fT0Flag = 1;

    UpdateInterval(0);

    fTS->GetEntry(0);
    std::cout << "calculated initiated time interval: " << std::endl;
    fTSReadingEntry = 0;
    for (int board = 0; board < fBoardCount; board++)
    {
        fLastSeg[board] = fTS->ts[board];
        std::cout << "Board index: " << board << '\t' << "First Time stamp(s): " << '\t' << fLastSeg[board] / 1e9 << '\t' << " Time Interval(s): " << fTSInterval[board] / 1e9 << '\t' << std::endl;
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

    for (int i = 0; i < fBoardCount; i++)
        if (!fBoard[i]->fChain)
        {
            fBoardFlag = 0;
            return false;
        }

    fBoardFlag = 1;
    return true;
}

bool DataMatchManager::InitMatchFile(std::string sOutputFile)
{
    if (fOutFlag)
        return false;
    fMatchFile = new TFile(sOutputFile.c_str(), "recreate");
    if (!fMatchFile->IsWritable())
    {
        fOutFlag = 0;
        return false;
    }
    fMatchTree = new TTree("match", "matched entries");
    fMatchTree->Branch("counter", &fMatchCounter, "counter/I");
    fMatchTree->Branch("matchedBoard", fMatchedBoard, "matchedBoard[counter]/I");
    fMatchTree->Branch("matchFlag", fMatchFlag, Form("matchFlag[%d]/I", fBoardCount));
    fMatchTree->Branch("matchEntry", fMatchEntry, Form("matchEntry[%d]/l", fBoardCount));
    fMatchTree->Branch("matchTime", fMatchTime, Form("matchTime[%d]/D", fBoardCount));
    fMatchTree->Branch("lastSeg", fLastSeg, Form("lastSeg[%d]/D", fBoardCount));
    fMatchTree->Branch("nextSeg", fNextSeg, Form("nextSeg[%d]/D", fBoardCount));
    fMatchTree->Branch("interval", fTSInterval, Form("interval[%d]/D", fBoardCount));
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
    fOutFlag = 1;
    return true;
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
        // std::cout << "John Debug: " << startEntries[board] << '\t' << endEntries[board] << std::endl;
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

int DataMatchManager::DoMatch()
{
    if (!ReadyForMatch())
        return 0;
    if (fMatchedFlag)
        return 0;

    int startEntries[MAX_BOARD_COUNTS], endEntries[MAX_BOARD_COUNTS];
    double preTime[MAX_BOARD_COUNTS];
    for (int entry = 0; entry < fTS->fChain->GetEntries(); entry++)
    // for (int entry = 10; entry < 11; entry++)
    {
        auto flag = FindAllEventsInSeg(entry, startEntries, endEntries);
        if (!flag)
            break;
        if (entry % 100 == 0)
            std::cout << entry << std::endl;
        MatchEventsInSeg(startEntries, endEntries);
    }
    fMatchedFlag = 1;
    return fMatchTree->GetEntries();
}

void DataMatchManager::DoInitiate(std::vector<int> vBoard, std::string sT0File, std::string sBoardFolder, std::string sOutputFile)
{
    CloseFile();
    InitBoardArray(vBoard);
    InitiateT0TS(sT0File);
    GenerateBoardMap(sBoardFolder);
    InitMatchFile(sOutputFile);
}

int DataMatchManager::MatchBoards(std::vector<int> vBoard, std::string sT0File, std::string sBoardFolder, std::string sOutputFile)
{
    DoInitiate(vBoard, sT0File, sBoardFolder, sOutputFile);
    auto entries = DoMatch();
    CloseFile();
    return entries;
}
