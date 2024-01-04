#include "Trees.h"

#include <iostream>

#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TGraph.h>

using namespace ROOTTrees;
board::board(std::string sFileName)
{
    TTree *tree = NULL;
    TFile *f = (TFile *)gROOT->GetListOfFiles()->FindObject(sFileName.c_str());
    if (!f || !f->IsOpen())
    {
        f = new TFile(sFileName.c_str());
    }
    f->GetObject("board", tree);
    Init(tree);
}
board::board(TTree *tree) : fChain(0)
{
    // if parameter tree is not specified (or zero), connect the file
    // used to generate this class and read the Tree.
    if (tree == 0)
    {
        TFile *f = (TFile *)gROOT->GetListOfFiles()->FindObject("3/Output4.root");
        if (!f || !f->IsOpen())
        {
            f = new TFile("3/Output4.root");
        }
        f->GetObject("board", tree);
    }
    Init(tree);
}

board::~board()
{
    if (!fChain)
        return;
    delete fChain->GetCurrentFile();
}

Int_t board::GetEntry(Long64_t entry)
{
    // Read contents of entry.
    if (!fChain)
        return 0;
    return fChain->GetEntry(entry);
}
Long64_t board::LoadTree(Long64_t entry)
{
    // Set the environment to read one entry
    if (!fChain)
        return -5;
    Long64_t centry = fChain->LoadTree(entry);
    if (centry < 0)
        return centry;
    if (fChain->GetTreeNumber() != fCurrent)
    {
        fCurrent = fChain->GetTreeNumber();
        Notify();
    }
    return centry;
}

void board::Init(TTree *tree)
{
    // The Init() function is called when the selector needs to initialize
    // a new tree or chain. Typically here the branch addresses and branch
    // pointers of the tree will be set.
    // It is normally not necessary to make changes to the generated
    // code, but the routine can be extended by the user if needed.
    // Init() will be called many times when running on PROOF
    // (once per file to be processed).

    // Set branch addresses and branch pointers
    if (!tree)
        return;
    fChain = tree;
    fCurrent = -1;
    fChain->SetMakeClass(1);

    fChain->SetBranchAddress("id", &id, &b_id);
    fChain->SetBranchAddress("FiredCount", &FiredCount, &b_FiredCount);
    fChain->SetBranchAddress("FiredCh", FiredCh, &b_FiredCh);
    fChain->SetBranchAddress("HGamp", HGamp, &b_HGamp);
    fChain->SetBranchAddress("LGamp", LGamp, &b_LGamp);
    fChain->SetBranchAddress("TDCTime", TDCTime, &b_TDCTime);
    Notify();
}

Bool_t board::Notify()
{
    // The Notify() function is called when a new file is opened. This
    // can be either for a new TTree in a TChain or when when a new TTree
    // is started when using PROOF. It is normally not necessary to make changes
    // to the generated code, but the routine can be extended by the
    // user if needed. The return value is currently not used.

    return kTRUE;
}

void board::Show(Long64_t entry)
{
    // Print contents of entry.
    // If entry is not specified, print current entry
    if (!fChain)
        return;
    fChain->Show(entry);
}
Int_t board::Cut(Long64_t entry)
{
    // This function may be called from Loop.
    // returns  1 if entry is accepted.
    // returns -1 otherwise.
    return 1;
}

void board::Loop()
{
    //   In a ROOT session, you can do:
    //      root> .L board.C
    //      root> board t
    //      root> t.GetEntry(12); // Fill t data members with entry number 12
    //      root> t.Show();       // Show values of entry 12
    //      root> t.Show(16);     // Read and show values of entry 16
    //      root> t.Loop();       // Loop on all entries
    //

    //     This is the loop skeleton where:
    //    jentry is the global entry number in the chain
    //    ientry is the entry number in the current Tree
    //  Note that the argument to GetEntry must be:
    //    jentry for TChain::GetEntry
    //    ientry for TTree::GetEntry and TBranch::GetEntry
    //
    //       To read only selected branches, Insert statements like:
    // METHOD1:
    //    fChain->SetBranchStatus("*",0);  // disable all branches
    //    fChain->SetBranchStatus("branchname",1);  // activate branchname
    // METHOD2: replace line
    //    fChain->GetEntry(jentry);       //read all branches
    // by  b_branchname->GetEntry(ientry); //read only this branch

    auto c = new TCanvas("c", "c", 1);
    auto h = new TH1D("h", "h", 2e2, -1e2, 1e2);
    auto h2 = new TH1D("h2", "h2", 2e2, -4e2, 4e2);

    auto tg = new TGraph();
    int tgPointCounter = 0;

    if (fChain == 0)
        return;

    Long64_t nentries = fChain->GetEntriesFast();

    Long64_t nbytes = 0, nb = 0;

    double time1 = -1, time2 = -1;
    int oddCounter = 0;
    for (Long64_t jentry = 0; jentry < nentries; jentry++)
    {
        Long64_t ientry = LoadTree(jentry);
        if (ientry < 0)
            break;
        nb = fChain->GetEntry(jentry);
        nbytes += nb;
        // std::cout << jentry << std::endl;
        // if (Cut(ientry) < 0) continue;
        h->Fill(TDCTime[FiredCh[0]] - TDCTime[FiredCh[1]]);

        if (TDCTime[32] != 0)
        {
            if (time1 < 0)
                time1 = TDCTime[32];
            else
            {
                double dev = ((time2 - time1) - 1e9 + 347527);
                h2->Fill(dev);
                time1 = time2;
                time2 = TDCTime[32];
                // std::cout << ((time2 - time1) - 1e9 + 347527) << std::endl;
                if (dev > -4e2 && dev < 4e2)
                {
                    tg->SetPoint(tgPointCounter, tgPointCounter, dev);
                    tgPointCounter++;
                }
                if (TMath::Abs(time2 - time1) > 1.1e9)
                {
                    std::cout << oddCounter++ << '\t' << time1 << '\t' << time2 << '\t' << time2 - time1 << '\t' << std::endl;
                }
            }
        }
    }
    h->Draw();
    c->SaveAs("test.pdf");
    h2->Draw();
    c->SaveAs("test2.pdf");
    tg->SetTitle(";Time/s;T_{meas}-T_{0}/ns");
    tg->Draw("AZL");
    c->SaveAs("test3.pdf");
}

tsTree::tsTree(std::string sFileName)
{
    TTree *tree = NULL;
    TFile *f = (TFile *)gROOT->GetListOfFiles()->FindObject(sFileName.c_str());
    if (!f || !f->IsOpen())
    {
        f = new TFile(sFileName.c_str());
    }
    f->GetObject("tsTree", tree);
    Init(tree);
}
tsTree::tsTree(TTree *tree) : fChain(0)
{
    // if parameter tree is not specified (or zero), connect the file
    // used to generate this class and read the Tree.
    if (tree == 0)
    {
        TFile *f = (TFile *)gROOT->GetListOfFiles()->FindObject("TS.root");
        if (!f || !f->IsOpen())
        {
            f = new TFile("TS.root");
        }
        f->GetObject("tsTree", tree);
    }
    Init(tree);
}

tsTree::~tsTree()
{
    if (!fChain)
        return;
    delete fChain->GetCurrentFile();
}

Int_t tsTree::GetEntry(Long64_t entry)
{
    // Read contents of entry.
    if (!fChain)
        return 0;
    return fChain->GetEntry(entry);
}
Long64_t tsTree::LoadTree(Long64_t entry)
{
    // Set the environment to read one entry
    if (!fChain)
        return -5;
    Long64_t centry = fChain->LoadTree(entry);
    if (centry < 0)
        return centry;
    if (fChain->GetTreeNumber() != fCurrent)
    {
        fCurrent = fChain->GetTreeNumber();
        Notify();
    }
    return centry;
}

void tsTree::Init(TTree *tree)
{
    // The Init() function is called when the selector needs to initialize
    // a new tree or chain. Typically here the branch addresses and branch
    // pointers of the tree will be set.
    // It is normally not necessary to make changes to the generated
    // code, but the routine can be extended by the user if needed.
    // Init() will be called many times when running on PROOF
    // (once per file to be processed).

    // Set branch addresses and branch pointers
    if (!tree)
        return;
    fChain = tree;
    fCurrent = -1;
    fChain->SetMakeClass(1);

    fChain->SetBranchAddress("tsid", &tsid, &b_tsid);
    fChain->SetBranchAddress("tsBoardCount", &tsBoardCount, &b_tsBoardCount);
    fChain->SetBranchAddress("tsBoardStat", tsBoardStat, &b_tsBoardStat);
    fChain->SetBranchAddress("ts", ts, &b_ts);
    fChain->SetBranchAddress("tsFlag", tsFlag, &b_tsFlag);
    Notify();
}

Bool_t tsTree::Notify()
{
    // The Notify() function is called when a new file is opened. This
    // can be either for a new TTree in a TChain or when when a new TTree
    // is started when using PROOF. It is normally not necessary to make changes
    // to the generated code, but the routine can be extended by the
    // user if needed. The return value is currently not used.

    return kTRUE;
}

void tsTree::Show(Long64_t entry)
{
    // Print contents of entry.
    // If entry is not specified, print current entry
    if (!fChain)
        return;
    fChain->Show(entry);
}
Int_t tsTree::Cut(Long64_t entry)
{
    // This function may be called from Loop.
    // returns  1 if entry is accepted.
    // returns -1 otherwise.
    return 1;
}

void tsTree::Loop()
{
    //   In a ROOT session, you can do:
    //      root> .L tsTree.C
    //      root> tsTree t
    //      root> t.GetEntry(12); // Fill t data members with entry number 12
    //      root> t.Show();       // Show values of entry 12
    //      root> t.Show(16);     // Read and show values of entry 16
    //      root> t.Loop();       // Loop on all entries
    //

    //     This is the loop skeleton where:
    //    jentry is the global entry number in the chain
    //    ientry is the entry number in the current Tree
    //  Note that the argument to GetEntry must be:
    //    jentry for TChain::GetEntry
    //    ientry for TTree::GetEntry and TBranch::GetEntry
    //
    //       To read only selected branches, Insert statements like:
    // METHOD1:
    //    fChain->SetBranchStatus("*",0);  // disable all branches
    //    fChain->SetBranchStatus("branchname",1);  // activate branchname
    // METHOD2: replace line
    //    fChain->GetEntry(jentry);       //read all branches
    // by  b_branchname->GetEntry(ientry); //read only this branch
    if (fChain == 0)
        return;

    Long64_t nentries = fChain->GetEntriesFast();

    Long64_t nbytes = 0, nb = 0;
    for (Long64_t jentry = 0; jentry < nentries; jentry++)
    {
        Long64_t ientry = LoadTree(jentry);
        if (ientry < 0)
            break;
        nb = fChain->GetEntry(jentry);
        nbytes += nb;
        // if (Cut(ientry) < 0) continue;
    }
}
