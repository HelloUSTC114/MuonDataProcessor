#include "AlignManager.h"

InputFileManager *InputFileManager::Instance()
{
    static InputFileManager *instance = new InputFileManager;
    return instance;
}

ErrorType InputFileManager::OpenFile(std::string sfilename)
{
    if (fInFile && fInFile->IsOpen())
        return FileAlreadyOpen;
    if (fInFile)
        if (!fInFile->IsOpen())
            delete fInFile;

    // Open file and process exception
    fInFile = new TFile(sfilename.c_str());
    if (!fInFile)
        fInFile = NULL;
    if (!fInFile->IsOpen())
    {
        delete fInFile;
        fInFile = NULL;
        return FileNotOpen;
    }

    // Get Trees
    fHGTree = (TTree *)fInFile->Get("HGTree");
    fLGTree = (TTree *)fInFile->Get("LGTree");
    fTDCTree = (TTree *)fInFile->Get("TDCTree");
    if (!fHGTree || !fLGTree || !fTDCTree)
    {
        CloseFile();
        return TreeNotFound;
    }
    fIsOpen = 1;

    // Show Tree Information
    fHGEntries = fHGTree->GetEntries();
    fLGEntries = fLGTree->GetEntries();
    fTDCEntries = fTDCTree->GetEntries();

    // Tree setting
    fHGTree->SetBranchAddress("chHGid", &fHGid);
    fLGTree->SetBranchAddress("chLGid", &fLGid);
    fTDCTree->SetBranchAddress("chTDCid", &fTDCid);

    fHGTree->SetBranchAddress("chHG", fHGamp);
    fLGTree->SetBranchAddress("chLG", fLGamp);
    fTDCTree->SetBranchAddress("chTDC", fTDCTime);

    fHGTree->GetEntry(0);
    fLGTree->GetEntry(0);
    fTDCTree->GetEntry(0);

    fHGLastReadEntry = -1, fLGLastReadEntry = -1, fTDCLastReadEntry = -1;
    return Success;
}

void InputFileManager::CloseFile()
{
    fIsOpen = 0;
    if (fInFile)
    {
        fInFile->Close();
        delete fInFile;
    }
    fInFile = NULL;
    fHGTree = NULL;
    fLGTree = NULL;
    fTDCTree = NULL;
    fHGLastReadEntry = -1, fLGLastReadEntry = -1, fTDCLastReadEntry = -1;
}

Int_t InputFileManager::GetEntry(Long64_t entry, TreeType tree)
{
    if (!IsOpen())
        return -1;
    if (tree == hgTree)
    {
        if (fHGLastReadEntry == entry)
            return AlreadyReadEntry;
        fHGLastReadEntry = entry;
        return fHGTree->GetEntry(entry);
    }
    if (tree == lgTree)
    {
        if (fLGLastReadEntry == entry)
            return AlreadyReadEntry;
        fLGLastReadEntry = entry;
        return fLGTree->GetEntry(entry);
    }
    if (tree == tdcTree)
    {
        if (fTDCLastReadEntry == entry)
            return AlreadyReadEntry;
        fTDCLastReadEntry = entry;
        return fTDCTree->GetEntry(entry);
    }
    return -1;
}

Long64_t InputFileManager::GetEntries(TreeType tree)
{
    if (!IsOpen())
        return -1;
    if (tree == hgTree)
        return fHGEntries;
    if (tree == lgTree)
        return fLGEntries;
    if (tree == tdcTree)
        return fTDCEntries;
    return -1;
}

bool InputFileManager::JudgeEOF(Long64_t entry, TreeType tree)
{
    bool rtn = 0;
    if (tree == hgTree)
        rtn = (entry >= 0) && (entry < fHGEntries);
    if (tree == lgTree)
        rtn = (entry >= 0) && (entry < fLGEntries);
    if (tree == tdcTree)
        rtn = (entry >= 0) && (entry < fTDCEntries);
    return !rtn;
}

bool InputFileManager::JudgeEOF(Long64_t hgentry, Long64_t lgentry, Long64_t tdcentry)
{
    bool eofFlag = gInputFile->JudgeEOF(hgentry, hgTree) || gInputFile->JudgeEOF(lgentry, lgTree) || gInputFile->JudgeEOF(tdcentry, tdcTree);
    return eofFlag;
}

double fgFreq = 433.995; // in unit of MHz, board TDC frequency
double ConvertTDC2Time(uint64_t tdc, double &coarseTime, double &fineTime)
{
    coarseTime = (tdc >> 16) / fgFreq * 1e3;
    fineTime = (tdc & 0xffff) / 65536.0 / fgFreq * 1e3;
    return (coarseTime - fineTime);
}

double ConvertTDC2Time(uint64_t tdc)
{
    double a, b;
    return ConvertTDC2Time(tdc, a, b);
}

AlignOutputFileManager *AlignOutputFileManager::Instance()
{
    static AlignOutputFileManager *instance = new AlignOutputFileManager;
    return instance;
}

ErrorType AlignOutputFileManager::OpenFile(std::string sfilename)
{
    if (fOutFile && fOutFile->IsOpen())
        return FileAlreadyOpen;
    if (fOutFile)
        if (!fOutFile->IsOpen())
            delete fOutFile;

    fAlignedEntries = 0;
    fHGCurrentEntry = 0;
    fLGCurrentEntry = 0;
    fTDCCurrentEntry = 0;

    // Open file and process exception
    fOutFile = new TFile(sfilename.c_str(), "recreate");
    if (!fOutFile)
        fOutFile = NULL;
    if (!fOutFile->IsOpen())
    {
        delete fOutFile;
        fOutFile = NULL;
        return FileNotOpen;
    }
    fOpenFlag = 1;

    // Get Trees
    fOutTree = new TTree("board", "board");
    fOutTree->Branch("id", &fTriggerID, "id/i");
    fOutTree->Branch("FiredCount", &fFiredCount, "FiredCount/b");
    fOutTree->Branch("FiredCh", fFiredChannel, "FiredCh[FiredCount]/b");

    fOutTree->Branch("HGamp", fHGamp, "HGamp[32]/D");
    fOutTree->Branch("LGamp", fLGamp, "LGamp[32]/D");
    fOutTree->Branch("TDCTime", fTDCTime, "TDCTime[33]/D");

    return Success;
}

void AlignOutputFileManager::CloseFile()
{
    fOpenFlag = 0;
    if (fOutTree && fOutFile && fOutFile->IsWritable())
    {
        fOutFile->cd();
        fOutTree->Write();
    }
    if (fOutFile)
    {
        fOutFile->Close();
        delete fOutFile;
    }
    fOutFile = NULL;
    fOutTree = NULL;

    fAlignedEntries = 0;
    fHGCurrentEntry = 0;
    fLGCurrentEntry = 0;
    fTDCCurrentEntry = 0;
}

#include <vector>
#include <iostream>
int AlignOutputFileManager::AlignOneEntry(bool verbose)
{
    if (!IsOpen())
    {
        return FileNotOpen;
    }
    if (!gInputFile->IsOpen())
    {
        return InputFileNotOpen;
    }

    // Find Aligned entries
    bool alignFlag = 0;
    int loopCounter = 0;
    for (loopCounter = 0; !alignFlag; loopCounter++)
    {
        auto eof = gInputFile->JudgeEOF(fHGCurrentEntry, fLGCurrentEntry, fTDCCurrentEntry);
        if (eof)
            return EndOfInputFile;

        gInputFile->GetEntry(fHGCurrentEntry, hgTree);
        gInputFile->GetEntry(fLGCurrentEntry, lgTree);
        gInputFile->GetEntry(fTDCCurrentEntry, tdcTree);

        auto hgid = gInputFile->HGid();
        auto lgid = gInputFile->LGid();
        auto tdcid = gInputFile->TDCid();
        if ((int32_t)(hgid) == -1)
        {
            hgid = 0;
        }
        if ((int32_t)(lgid) == -1)
        {
            lgid = 0;
        }
        if ((int32_t)(tdcid) == -1)
        {
            tdcid = 0;
        }

        auto maxid = TMath::Max(TMath::Max(hgid, lgid), tdcid);
        bool idEqual = (hgid == maxid) && (lgid == maxid) && (tdcid == maxid);
        if (!idEqual)
        {
            if (verbose)
                std::cout << "Finding: " << hgid << '\t' << lgid << '\t' << tdcid << '\t' << maxid << std::endl;
            if (hgid < maxid)
                fHGCurrentEntry++;
            if (lgid < maxid)
                fLGCurrentEntry++;
            if (tdcid < maxid)
                fTDCCurrentEntry++;
            continue;
        }
        if (verbose)
            std::cout << "success: " << hgid << '\t' << lgid << '\t' << tdcid << '\t' << maxid << std::endl;
        fTriggerID = maxid;
        alignFlag = 1;
    }
    memcpy(fHGamp, gInputFile->HGamp(), N_BOARD_CHANNELS * sizeof(double));
    memcpy(fLGamp, gInputFile->LGamp(), N_BOARD_CHANNELS * sizeof(double));

    fFiredCount = 0;
    for (int i = 0; i < N_BOARD_CHANNELS + 1; i++)
    {
        fTDCTime[i] = ConvertTDC2Time(gInputFile->TDCTime()[i]);
        if (fTDCTime[i] != 0)
        {
            fFiredChannel[fFiredCount++] = i;
            if (verbose)
                std::cout << i << '\t' << (uint64_t)fTDCTime[i] << std::endl;
        }
    }
    fOutTree->Fill();
    if (verbose)
    {
        std::cout << "Totally fired channel: " << (int)fFiredCount << std::endl;
        for (int i = 0; i < fFiredCount; i++)
        {
            std::cout << "Channel: " << (int)fFiredChannel[i] << std::endl;
        }
    }

    fHGCurrentEntry++, fLGCurrentEntry++, fTDCCurrentEntry++;
    return loopCounter;
}

#include "mainwindow.h"
int AlignOutputFileManager::AlignAllEntries()
{
    int entries = 0;
    for (entries = 0; AlignOneEntry() >= 0; entries++)
        ;
    std::cout << "Totally aligned " << entries << " entries" << std::endl;
    gAlignWin->DisplayAlignedEntries(entries);
    return entries;
}
