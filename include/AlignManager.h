#ifndef ALIGNMANAGER_H
#define ALIGNMANAGER_H

#include <TFile.h>
#include <TTree.h>
#define N_BOARD_CHANNELS 32

// STL
#include <string>

class FileManager;

enum ErrorType
{
    Success = 1,
    FileNotOpen = -1,
    TreeNotFound = -2,
    FileAlreadyOpen = -3,
};
enum TreeType
{
    hgTree = 0,
    lgTree = 1,
    tdcTree = 2,
};

class InputFileManager
{
public:
    static InputFileManager *Instance();

    ErrorType OpenFile(std::string sfilename);
    void CloseFile();
    bool IsOpen() { return fIsOpen; };

    const double *HGamp() { return fHGamp; };
    const double *LGamp() { return fLGamp; };
    const uint64_t *TDCTime() { return fTDCTime; };
    uint32_t HGid() { return fHGid; };
    uint32_t LGid() { return fLGid; };
    uint32_t TDCid() { return fTDCid; };

    /// @brief Get entry for 3 trees
    /// @param entry
    /// @param tree which tree to read
    /// @return -1 if failed
    Int_t GetEntry(Long64_t entry, TreeType tree);

    Int_t GetEntries(TreeType tree);

private:
    // Read tree
    TFile *fInFile = NULL;
    TTree *fHGTree = NULL, *fLGTree = NULL, *fTDCTree = NULL;
    int fHGEntries = 0, fLGEntries = 0, fTDCEntries = 0;
    int fHGStartid = 0, fLGStartid = 0, fTDCStartid = 0;
    volatile bool fIsOpen = 0;
    // tree setting
    double fHGamp[N_BOARD_CHANNELS];
    double fLGamp[N_BOARD_CHANNELS];
    uint64_t fTDCTime[N_BOARD_CHANNELS + 1];
    uint32_t fHGid = 0, fLGid = 0, fTDCid = 0;
};
#define gInputFile (InputFileManager::Instance())

#endif // ALIGNMANAGER_H