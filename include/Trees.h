#ifndef TREES_H
#define TREES_H

//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Mon Apr 24 15:21:00 2023 by ROOT version 6.22/02
// from TTree board/board
// found on file: Output-2023-04-24-15-19-49.root
//////////////////////////////////////////////////////////

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>
// Header file for the classes stored in the TTree if any.

namespace ROOTTrees
{
    class board
    {
    public:
        TTree *fChain;  //! pointer to the analyzed TTree or TChain
        Int_t fCurrent; //! current Tree number in a TChain

        // Fixed size dimensions of array or collections stored in the TTree if any.

        // Declaration of leaf types
        UInt_t id;
        UChar_t FiredCount;
        UChar_t FiredCh[24]; //[FiredCount]
        Double_t HGamp[32];
        Double_t LGamp[32];
        Double_t TDCTime[33];

        // List of branches
        TBranch *b_id;         //!
        TBranch *b_FiredCount; //!
        TBranch *b_FiredCh;    //!
        TBranch *b_HGamp;      //!
        TBranch *b_LGamp;      //!
        TBranch *b_TDCTime;    //!

        board(std::string sFileName);
        board(TTree *tree = 0);
        virtual ~board();
        virtual Int_t Cut(Long64_t entry);
        virtual Int_t GetEntry(Long64_t entry);
        virtual Long64_t LoadTree(Long64_t entry);
        virtual void Init(TTree *tree);
        virtual void Loop();
        virtual Bool_t Notify();
        virtual void Show(Long64_t entry = -1);
    };

    // Header file for the classes stored in the TTree if any.

    class tsTree
    {
    public:
        TTree *fChain;  //! pointer to the analyzed TTree or TChain
        Int_t fCurrent; //! current Tree number in a TChain

        // Fixed size dimensions of array or collections stored in the TTree if any.

        // Declaration of leaf types
        UInt_t tsid;
        UChar_t tsBoardCount;
        UChar_t tsBoardStat[6]; //[tsBoardCount]
        Double_t ts[6];
        UChar_t tsFlag[6];

        // List of branches
        TBranch *b_tsid;         //!
        TBranch *b_tsBoardCount; //!
        TBranch *b_tsBoardStat;  //!
        TBranch *b_ts;           //!
        TBranch *b_tsFlag;       //!

        tsTree(std::string sFileName);
        tsTree(TTree *tree = 0);
        virtual ~tsTree();
        virtual Int_t Cut(Long64_t entry);
        virtual Int_t GetEntry(Long64_t entry);
        virtual Long64_t LoadTree(Long64_t entry);
        virtual void Init(TTree *tree);
        virtual void Loop();
        virtual Bool_t Notify();
        virtual void Show(Long64_t entry = -1);
    };
}

#endif
