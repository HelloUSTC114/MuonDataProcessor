#include "T0TS.h"

#include <QDir>

#define TS_JUDGE_RANGE (2e8)

namespace T0Process
{
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

    void ProcessTS()
    {
        for (int board = 0; board < 6; board++)
        {
            T0Process::ConvertTS2ROOT(board);
        }
        BoardT0Manager a;
        a.InitT0Matching();
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
            std::cout << std::setprecision(10) << gInsideEntry << '\t' << std::setprecision(1) << board << std::setprecision(10) << '\t' << fPreEntry[board] << '\t' << (bool)fTSFlag[board] << '\t' << "N:" << fInTS[board] / 1e9 << '\t' << fPreTS[board] / 1e9 << '\t' << fUnmatchedCum[board] / 1e9 << '\t' << fTSDev[board] / 1e9 << '\t' << minDev / 1e9 << '\t' << (fTSDev[board] - minDev) / 1e9 << std::endl;
        }
        // std::cout << board << '\t' << minDev << '\t' << fTSDev[board] << '\t' << fTSDev[board] - minDev - fUnmatchedCum[board] << std::endl;
    }
    // std::cout << std::endl;
    fTree->Fill();
    fTSid++;

    return true;
}

int T0Process::ConvertTS2ROOT(std::string sInfile, std::string sOutFile)
{
    int entries;
    double startT0, endT0;
    return ConvertTS2ROOT(sInfile, sOutFile, entries, startT0, endT0);
}

int T0Process::ConvertTS2ROOT(std::string sInfile, std::string sOutFile, int &entries, double &startT0, double &endT0)
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
