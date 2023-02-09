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
    fLGEntries = fHGTree->GetEntries();
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
}

Int_t InputFileManager::GetEntry(Long64_t entry, TreeType tree)
{
    if (!IsOpen())
        return -1;
    if (tree == hgTree)
        return fHGTree->GetEntry(entry);
    if (tree == lgTree)
        return fLGTree->GetEntry(entry);
    if (tree == tdcTree)
        return fTDCTree->GetEntry(entry);
    return -1;
}

Int_t InputFileManager::GetEntries(TreeType tree)
{
    if (!IsOpen())
        return -1;
    if (tree == hgTree)
        return fHGTree->GetEntries();
    if (tree == lgTree)
        return fLGTree->GetEntries();
    if (tree == tdcTree)
        return fTDCTree->GetEntries();
    return -1;
}
