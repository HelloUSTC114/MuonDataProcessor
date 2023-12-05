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
    Success = 0,
    // File Manager
    FileNotOpen = -1,
    TreeNotFound = -2,
    FileAlreadyOpen = -3,

    // Align Output file manager
    EndOfInputFile = -4,
    InputFileNotOpen = -5,

    // Tree Read Entry
    AlreadyReadEntry = -6,
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

    Long64_t GetEntries(TreeType tree);

    /// @brief Judge whether this entry is out of range
    /// @param entry
    /// @param tree
    /// @return 0 means safe, 1 means out of range
    bool JudgeEOF(Long64_t entry, TreeType tree);
    bool JudgeEOF(Long64_t hgentry, Long64_t lgentry, Long64_t tdcentry);

    // Get arbitrary object from input file
    TObject *Get(const char *namecycle);

private:
    // Read tree
    TFile *fInFile = NULL;
    TTree *fHGTree = NULL, *fLGTree = NULL, *fTDCTree = NULL;
    Long64_t fHGEntries = 0, fLGEntries = 0, fTDCEntries = 0;
    int fHGStartid = 0, fLGStartid = 0, fTDCStartid = 0;
    volatile bool fIsOpen = 0;
    // tree setting
    double fHGamp[N_BOARD_CHANNELS];
    double fLGamp[N_BOARD_CHANNELS];
    uint64_t fTDCTime[N_BOARD_CHANNELS + 1];
    uint32_t fHGid = 0, fLGid = 0, fTDCid = 0;
    // Read Control
    Long64_t fHGLastReadEntry = -1, fLGLastReadEntry = -1, fTDCLastReadEntry = -1;
};
#define gInputFile (InputFileManager::Instance())

double ConvertTDC2Time(uint64_t tdc, double &coarseTime, double &fineTime);
double ConvertTDC2Time(uint64_t tdc);

class AlignOutputFileManager
{
public:
    static AlignOutputFileManager *Instance();

    bool IsOpen() { return fOpenFlag; };
    ErrorType OpenFile(std::string sfilename);
    void CloseFile();

    /// @brief
    /// @return Number of largest skipped entries. If come across some errors, return error code
    int AlignOneEntry(bool verbose = 0);
    int AlignAllEntries();

private:
    volatile bool fOpenFlag = 0;
    int fAlignedEntries = 0;
    int fHGCurrentEntry = 0;
    int fLGCurrentEntry = 0;
    int fTDCCurrentEntry = 0;

    TFile *fOutFile = NULL;
    TTree *fOutTree = NULL;
    uint32_t fTriggerID;
    UChar_t fFiredCount;
    UChar_t fFiredChannel[N_BOARD_CHANNELS + 1]; //! fFiredChannel[fFiredCount]
    double fHGamp[N_BOARD_CHANNELS];
    double fLGamp[N_BOARD_CHANNELS];
    double fTDCTime[N_BOARD_CHANNELS + 1];
};
#define gAlignOutput (AlignOutputFileManager::Instance())

#endif // ALIGNMANAGER_H