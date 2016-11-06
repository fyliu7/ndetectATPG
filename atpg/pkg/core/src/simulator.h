// **************************************************************************
// File       [ simulator.h ]
// Author     [ littleshamoo ]
// Synopsis   [ it supports two modes: pp means parallel pattern;
//                                     pf means parallel fault
// Date       [ 2011/09/14 created ]
// **************************************************************************

#ifndef _CORE_SIMULATOR_H_
#define _CORE_SIMULATOR_H_

#include <stack>

#include "pattern.h"
#include "fault.h"

namespace CoreNs {

class Simulator {
public:
         Simulator(Circuit *cir);
         ~Simulator();

    // used by both parallel pattern and parallel fault
    void setNdet(const int &ndet);  // this for n-detect 
    void eventFaultSim();
    void goodSim();
    void goodSimCopyToFault();
    void goodEval(const int &i);
    void faultEval(const int &i);
    void assignPatternToPi(const Pattern *const p); 

    // parallel fault
    void pfFaultSim(PatternProcessor *pcoll, FaultListExtract *fListExtract);
    int  pfFaultSim(const Pattern * const p, FaultList &remain);
    int  pfFaultSim(FaultList &remain);
    int  pfFaultSim(const Pattern * const p, 
                    FaultVec &detect, 
                    FaultList &remain);
    int  pfFaultSim(FaultVec &detect, FaultList &remain);

    // parallel pattern
    void ppGoodSim(PatternProcessor *pcoll);
    void ppFaultSim(PatternProcessor *pcoll, FaultListExtract *fListExtract);
    void ppFaultSim(FaultList &remain);

protected:
    // used by both parallel pattern and parallel fault
    Circuit         *cir_;
    int             ndet_; // for n-detect
    std::stack<int> *events_;
    bool            *processed_; // array of processed flags.  TRUE means this gate is processed
    int             *recover_; // array of gates to be recovered from the last fault injection
    int             nrecover_; // number of recovers needed 
	//  this is to inject fault into the circuit
	//  faultInjectL_ =1 faultInjectH_ =0 inject a stuck-at zero fault
    ParaValue       (*faultInjectL_)[5];  
    ParaValue       (*faultInjectH_)[5];

    // parallel fault
    void            pfReset();
    bool            pfCheckActivation(const Fault * const f);
    void            pfInject(const Fault * const f, const size_t &i);
    int             pfCheckDetection(FaultList &remain);
    int             pfCheckDetection(FaultVec &detect, FaultList &remain);
    FaultListIter   injected_[WORD_SIZE];
    int             ninjected_;

    // parallel pattern
    void            ppReset();
    bool            ppCheckActivation(const Fault * const f);
    void            ppInject(const Fault * const f);
    void            ppCheckDetection(Fault * const f);
    void            ppSetPattern(PatternProcessor *pcoll, const int &i);
    ParaValue       activated_;

};

inline Simulator::Simulator(Circuit *cir) {
    // used by both parallel pattern and parallel fault
    cir_       = cir;
    ndet_      = 1;
    events_    = new std::stack<int>[cir_->tlvl_];
    processed_ = new bool[cir_->tgate_];
    recover_   = new int[cir_->tgate_];
    nrecover_  = 0;
    faultInjectL_     = new ParaValue[cir_->tgate_][5];
    faultInjectH_     = new ParaValue[cir_->tgate_][5];

    memset(processed_, 0, cir_->tgate_ * sizeof(bool));
    memset(faultInjectL_, 0, cir_->tgate_ * 5 * sizeof(ParaValue));
    memset(faultInjectH_, 0, cir_->tgate_ * 5 * sizeof(ParaValue));

    // parallel fault
    ninjected_ = 0;

    // parallel pattern
    activated_ = PARA_L;
}

inline Simulator::~Simulator() {
    delete [] events_;
    delete [] processed_;
    delete [] recover_;
    delete [] faultInjectL_;
    delete [] faultInjectH_;
}

// **************************************************************************
// Function   [ Simulator::assignPatternToPi ]	
// Commentor  [ HKY CYW ]
// Synopsis   [ usage: assign test pattern to circuit PI & PPI
//					   for further fault simulation
//              in:    Pattern*
//              out:   void
//            ]
// Date       [ HKY Ver. 1.1 started 2014/09/01 ]
// **************************************************************************
inline void Simulator::assignPatternToPi(const Pattern *const pat)
{ 
        // set pattern; apply pattern to PI
        for (int j = 0; j < cir_->npi_; ++j) {
            cir_->gates_[j].gl_ = PARA_L;
            cir_->gates_[j].gh_ = PARA_L;
            if (pat->pi1_) {
                if (pat->pi1_[j] == L)
                    cir_->gates_[j].gl_ = PARA_H;
                else if (pat->pi1_[j] == H)
                    cir_->gates_[j].gh_ = PARA_H;
            }
            if (cir_->nframe_ > 1) {
                cir_->gates_[j + cir_->ngate_].gl_ = PARA_L;
                cir_->gates_[j + cir_->ngate_].gh_ = PARA_L;
                if (pat->pi2_) {
                    if (pat->pi2_[j] == L)
                        cir_->gates_[j + cir_->ngate_].gl_ = PARA_H;
                    else if (pat->pi2_[j] == H)
                        cir_->gates_[j + cir_->ngate_].gh_ = PARA_H;
                }
            }
        }

        // set pattern; apply pattern to PPI
        for (int j = cir_->npi_; j < cir_->npi_+ cir_->nppi_; ++j) {
            cir_->gates_[j].gl_ = PARA_L;
            cir_->gates_[j].gh_ = PARA_L;
            if (pat->ppi_) {
                if (pat->ppi_[j - cir_->npi_] == L)
                    cir_->gates_[j].gl_ = PARA_H;
                else if (pat->ppi_[j - cir_->npi_] == H)
                    cir_->gates_[j].gh_ = PARA_H;
            }
            if (cir_->connType_ == Circuit::SHIFT && cir_->nframe_ > 1) {
				for(int k = 1 ; k < cir_->nframe_; ++k){
					cir_->gates_[j + cir_->ngate_*k].gl_ = PARA_L;
					cir_->gates_[j + cir_->ngate_*k].gh_ = PARA_L;
					if (j == cir_->npi_){
						if (pat->si_) {
						if (pat->si_[k-1] == L)
							cir_->gates_[j + cir_->ngate_*k].gl_ = PARA_H;
						else if (pat->si_[k-1] == H)
							cir_->gates_[j + cir_->ngate_*k].gh_ = PARA_H;
						}
					}
					else{
					}
				}
            }
        }

}
// **************************************************************************
// Function   [ Simulator::setNdet ]	
// Commentor  [ CJY CBH ]
// Synopsis   [ usage: set number of detection (default = 1)
//              in:    #detection
//              out:   void
//            ]
// Date       [ CJY Ver. 1.0 started 2013/08/18 ]
// **************************************************************************
inline void Simulator::setNdet(const int &ndet) {
    ndet_ = ndet;
}

// **************************************************************************
// Function   [ Simulator::goodSim ]	
// Commentor  [ CJY CBH ]
// Synopsis   [ usage: sim good value of every gate; call goodEval function for each gate
//              in:    void
//              out:   void
//            ]
// Date       [ CJY Ver. 1.0 started 2013/08/18 ]
// **************************************************************************
inline void Simulator::goodSim() {
    for (int i = 0; i < cir_->tgate_; ++i)
        goodEval(i);
}

// **************************************************************************
// Function   [ Simulator::goodSimCopyToFault ]	
// Commentor  [ CJY CBH]
// Synopsis   [ usage: call goodEval function for each gate
//                     and copy goodsim result to faultsim variable
//              in:    void
//              out:   void
//            ]
// Date       [ CBH Ver. 1.0 started 2031/08/18 ]
// **************************************************************************
inline void Simulator::goodSimCopyToFault() {
    for (int i = 0; i < cir_->tgate_; ++i) {
        goodEval(i);
        cir_->gates_[i].fl_ = cir_->gates_[i].gl_;
        cir_->gates_[i].fh_ = cir_->gates_[i].gh_;
    }
}

// **************************************************************************
// Function   [ Simulator::pfReset ]	
// Commentor  [ CJY CBH]
// Synopsis   [ usage: reset faulty value of the fault gate to good value
//                     reset mask to zero
//              in:    void
//              out:   void
//            ]
// Date       [ CBH Ver. 1.0 started 2031/08/18 ]
// **************************************************************************
inline void Simulator::pfReset() {
    for (int i = 0; i < nrecover_; ++i) {
        cir_->gates_[recover_[i]].fl_ = cir_->gates_[recover_[i]].gl_;
        cir_->gates_[recover_[i]].fh_ = cir_->gates_[recover_[i]].gh_;
    }
    nrecover_ = 0;
    memset(processed_, 0, cir_->tgate_ * sizeof(bool));
    memset(faultInjectL_, 0, cir_->tgate_ * 5 * sizeof(ParaValue));
    memset(faultInjectH_, 0, cir_->tgate_ * 5 * sizeof(ParaValue));

    ninjected_ = 0;
}

// **************************************************************************
// Function   [ ppReset ]
// Commentor  [ Bill ]
// Synopsis   [ usage: reset all circuit gates ,faulty low & faulty high to good low & goood high
//              in:    none
//              out:   void
//            ]
// Date       [ Bill Ver. 1.0 started 20130811 ]
// **************************************************************************
inline void Simulator::ppReset() {
    for (int i = 0; i < nrecover_; ++i) {
        cir_->gates_[recover_[i]].fl_ = cir_->gates_[recover_[i]].gl_;
        cir_->gates_[recover_[i]].fh_ = cir_->gates_[recover_[i]].gh_;
    }
    nrecover_ = 0;
    memset(processed_, 0, cir_->tgate_ * sizeof(bool));
    memset(faultInjectL_, 0, cir_->tgate_ * 5 * sizeof(ParaValue));
    memset(faultInjectH_, 0, cir_->tgate_ * 5 * sizeof(ParaValue));

    activated_ = PARA_L;
}

// **************************************************************************
// Function   [ Simulator::goodEval ]	
// Commentor  [ CJY CBH ]
// Synopsis   [ usage: assign good value from fanin value
//                     to output of gate(gl and gh)
//                       PS:  case1:  gl = 1, gh = 0 => 0
//                            case2:  gl = 0, gh = 1 => 1
//                            case3:  gl = 1, gh = 1 => X
//              in:    gate's ID
//              out:   void
//            ]
// Date       [ CJY Ver. 1.0 started 2013/08/18 ]
// **************************************************************************
//{{{ inline void Simulator::goodEval(const int &)
inline void Simulator::goodEval(const int &i) {
    //find number of fanin 
    const int fi1 = cir_->gates_[i].nfi_ > 0 ? cir_->gates_[i].fis_[0] : 0;
    const int fi2 = cir_->gates_[i].nfi_ > 1 ? cir_->gates_[i].fis_[1] : 0;
    const int fi3 = cir_->gates_[i].nfi_ > 2 ? cir_->gates_[i].fis_[2] : 0;
    const int fi4 = cir_->gates_[i].nfi_ > 3 ? cir_->gates_[i].fis_[3] : 0;
    //read value of fanin
    const ParaValue &l1 = cir_->gates_[fi1].gl_;
    const ParaValue &h1 = cir_->gates_[fi1].gh_;
    const ParaValue &l2 = cir_->gates_[fi2].gl_;
    const ParaValue &h2 = cir_->gates_[fi2].gh_;
    const ParaValue &l3 = cir_->gates_[fi3].gl_;
    const ParaValue &h3 = cir_->gates_[fi3].gh_;
    const ParaValue &l4 = cir_->gates_[fi4].gl_;
    const ParaValue &h4 = cir_->gates_[fi4].gh_;
    //evaluate good value of gate's output
    switch (cir_->gates_[i].type_) {
        case Gate::INV:
            cir_->gates_[i].gl_ = h1;
            cir_->gates_[i].gh_ = l1;
            break;
        case Gate::PO:
        case Gate::PPO:
        case Gate::BUF:
            cir_->gates_[i].gl_ = l1;
            cir_->gates_[i].gh_ = h1;
            break;
        case Gate::AND2:
            cir_->gates_[i].gl_ = l1 | l2;
            cir_->gates_[i].gh_ = h1 & h2;
            break;
        case Gate::AND3:
            cir_->gates_[i].gl_ = l1 | l2 | l3;
            cir_->gates_[i].gh_ = h1 & h2 & h3;
            break;
        case Gate::AND4:
            cir_->gates_[i].gl_ = l1 | l2 | l3 | l4;
            cir_->gates_[i].gh_ = h1 & h2 & h3 & h4;
            break;
        case Gate::NAND2:
            cir_->gates_[i].gl_ = h1 & h2;
            cir_->gates_[i].gh_ = l1 | l2;
            break;
        case Gate::NAND3:
            cir_->gates_[i].gl_ = h1 & h2 & h3;
            cir_->gates_[i].gh_ = l1 | l2 | l3;
            break;
        case Gate::NAND4:
            cir_->gates_[i].gl_ = h1 & h2 & h3 & h4;
            cir_->gates_[i].gh_ = l1 | l2 | l3 | l4;
            break;
        case Gate::OR2:
            cir_->gates_[i].gl_ = l1 & l2;
            cir_->gates_[i].gh_ = h1 | h2;
            break;
        case Gate::OR3:
            cir_->gates_[i].gl_ = l1 & l2 & l3;
            cir_->gates_[i].gh_ = h1 | h2 | h3;
            break;
        case Gate::OR4:
            cir_->gates_[i].gl_ = l1 & l2 & l3 & l4;
            cir_->gates_[i].gh_ = h1 | h2 | h3 | h4;
            break;
        case Gate::NOR2:
            cir_->gates_[i].gl_ = h1 | h2;
            cir_->gates_[i].gh_ = l1 & l2;
            break;
        case Gate::NOR3:
            cir_->gates_[i].gl_ = h1 | h2 | h3;
            cir_->gates_[i].gh_ = l1 & l2 & l3;
            break;
        case Gate::NOR4:
            cir_->gates_[i].gl_ = h1 | h2 | h3 | h4;
            cir_->gates_[i].gh_ = l1 & l2 & l3 & l4;
            break;
        case Gate::XOR2:
            cir_->gates_[i].gl_ = (l1 & l2) | (h1 & h2);
            cir_->gates_[i].gh_ = (l1 & h2) | (h1 & l2);
            break;
        case Gate::XOR3:
            cir_->gates_[i].gl_ = (l1 & l2) | (h1 & h2);
            cir_->gates_[i].gl_ = (cir_->gates_[i].gl_ & l3)
                                 | (cir_->gates_[i].gh_ & h3);
            cir_->gates_[i].gh_ = (l1 & h2) | (h1 & l2);
            cir_->gates_[i].gh_ = (cir_->gates_[i].gl_ & h3)
                                 | (cir_->gates_[i].gh_ & l3);
            break;
        case Gate::XNOR2:
            cir_->gates_[i].gl_ = (l1 & h2) | (h1 & l2);
            cir_->gates_[i].gh_ = (l1 & l2) | (h1 & h2);
            break;
        case Gate::XNOR3:
            cir_->gates_[i].gl_ = (l1 & l2) | (h1 & h2);
            cir_->gates_[i].gl_ = (cir_->gates_[i].gl_ & h3)
                                 | (cir_->gates_[i].gh_ & l3);
            cir_->gates_[i].gh_ = (l1 & h2) | (h1 & l2);
            cir_->gates_[i].gh_ = (cir_->gates_[i].gl_ & l3)
                                 | (cir_->gates_[i].gh_ & h3);
            break;
        case Gate::TIE1:
            cir_->gates_[i].gl_ = PARA_L;
            cir_->gates_[i].gh_ = PARA_H;
            break;
        case Gate::TIE0:
            cir_->gates_[i].gl_ = PARA_H;
            cir_->gates_[i].gh_ = PARA_L;
            break;
		case Gate::PPI:
			if(cir_->connType_ == Circuit::CAPTURE && cir_->gates_[i].frame_ > 0){
				cir_->gates_[i].gl_ = l1;
				cir_->gates_[i].gh_ = h1;
			}
            break;
        default:
            break;
    }
} //}}}

// **************************************************************************
// Function   [ Simulator::faultEval ]
// Commentor  [ CJY CBH ]
// Synopsis   [ usage: assign faulty value from fanin value
//                     to output of gate(fl and gh)
//              in:    gate's ID
//              out:   void
//            ]
// Date       [ CJY Ver. 1.0 started 2013/08/14 ]
// **************************************************************************
//{{{ inline void Simulator::faultEval(const int &)
inline void Simulator::faultEval(const int &i) {
    //find number of fanin
    const int fi1 = cir_->gates_[i].nfi_ > 0 ? cir_->gates_[i].fis_[0] : 0;
    const int fi2 = cir_->gates_[i].nfi_ > 1 ? cir_->gates_[i].fis_[1] : 0;
    const int fi3 = cir_->gates_[i].nfi_ > 2 ? cir_->gates_[i].fis_[2] : 0;
    const int fi4 = cir_->gates_[i].nfi_ > 3 ? cir_->gates_[i].fis_[3] : 0;
    //read value of fanin with fault masking
    const ParaValue l1 = (cir_->gates_[fi1].fl_ & ~faultInjectH_[i][1])
                        | faultInjectL_[i][1];
    const ParaValue h1 = (cir_->gates_[fi1].fh_ & ~faultInjectL_[i][1])
                        | faultInjectH_[i][1];
    const ParaValue l2 = (cir_->gates_[fi2].fl_ & ~faultInjectH_[i][2])
                        | faultInjectL_[i][2];
    const ParaValue h2 = (cir_->gates_[fi2].fh_ & ~faultInjectL_[i][2])
                        | faultInjectH_[i][2];
    const ParaValue l3 = (cir_->gates_[fi3].fl_ & ~faultInjectH_[i][3])
                        | faultInjectL_[i][3];
    const ParaValue h3 = (cir_->gates_[fi3].fh_ & ~faultInjectL_[i][3])
                        | faultInjectH_[i][3];
    const ParaValue l4 = (cir_->gates_[fi4].fl_ & ~faultInjectH_[i][4])
                        | faultInjectL_[i][4];
    const ParaValue h4 = (cir_->gates_[fi4].fh_ & ~faultInjectL_[i][4])
                        | faultInjectH_[i][4];
    //evaluate faulty value of gate's output
    switch (cir_->gates_[i].type_) {
        case Gate::INV:
            cir_->gates_[i].fl_ = h1;
            cir_->gates_[i].fh_ = l1;
            break;
        case Gate::PO:
        case Gate::PPO:
        case Gate::BUF:
            cir_->gates_[i].fl_ = l1;
            cir_->gates_[i].fh_ = h1;
            break;
        case Gate::AND2:
            cir_->gates_[i].fl_ = l1 | l2;
            cir_->gates_[i].fh_ = h1 & h2;
            break;
        case Gate::AND3:
            cir_->gates_[i].fl_ = l1 | l2 | l3;
            cir_->gates_[i].fh_ = h1 & h2 & h3;
            break;
        case Gate::AND4:
            cir_->gates_[i].fl_ = l1 | l2 | l3 | l4;
            cir_->gates_[i].fh_ = h1 & h2 & h3 & h4;
            break;
        case Gate::NAND2:
            cir_->gates_[i].fl_ = h1 & h2;
            cir_->gates_[i].fh_ = l1 | l2;
            break;
        case Gate::NAND3:
            cir_->gates_[i].fl_ = h1 & h2 & h3;
            cir_->gates_[i].fh_ = l1 | l2 | l3;
            break;
        case Gate::NAND4:
            cir_->gates_[i].fl_ = h1 & h2 & h3 & h4;
            cir_->gates_[i].fh_ = l1 | l2 | l3 | l4;
            break;
        case Gate::OR2:
            cir_->gates_[i].fl_ = l1 & l2;
            cir_->gates_[i].fh_ = h1 | h2;
            break;
        case Gate::OR3:
            cir_->gates_[i].fl_ = l1 & l2 & l3;
            cir_->gates_[i].fh_ = h1 | h2 | h3;
            break;
        case Gate::OR4:
            cir_->gates_[i].fl_ = l1 & l2 & l3 & l4;
            cir_->gates_[i].fh_ = h1 | h2 | h3 | h4;
            break;
        case Gate::NOR2:
            cir_->gates_[i].fl_ = h1 | h2;
            cir_->gates_[i].fh_ = l1 & l2;
            break;
        case Gate::NOR3:
            cir_->gates_[i].fl_ = h1 | h2 | h3;
            cir_->gates_[i].fh_ = l1 & l2 & l3;
            break;
        case Gate::NOR4:
            cir_->gates_[i].fl_ = h1 | h2 | h3 | h4;
            cir_->gates_[i].fh_ = l1 & l2 & l3 & l4;
            break;
        case Gate::XOR2:
            cir_->gates_[i].fl_ = (l1 & l2) | (h1 & h2);
            cir_->gates_[i].fh_ = (l1 & h2) | (h1 & l2);
            break;
        case Gate::XOR3:
            cir_->gates_[i].fl_ = (l1 & l2) | (h1 & h2);
            cir_->gates_[i].fl_ = (cir_->gates_[i].fl_ & l3)
                                 | (cir_->gates_[i].fh_ & h3);
            cir_->gates_[i].fh_ = (l1 & h2) | (h1 & l2);
            cir_->gates_[i].fh_ = (cir_->gates_[i].fl_ & h3)
                                 | (cir_->gates_[i].fh_ & l3);
            break;
        case Gate::XNOR2:
            cir_->gates_[i].fl_ = (l1 & h2) | (h1 & l2);
            cir_->gates_[i].fh_ = (l1 & l2) | (h1 & h2);
            break;
        case Gate::XNOR3:
            cir_->gates_[i].fl_ = (l1 & l2) | (h1 & h2);
            cir_->gates_[i].fl_ = (cir_->gates_[i].fl_ & h3)
                                 | (cir_->gates_[i].fh_ & l3);
            cir_->gates_[i].fh_ = (l1 & h2) | (h1 & l2);
            cir_->gates_[i].fh_ = (cir_->gates_[i].fl_ & l3)
                                 | (cir_->gates_[i].fh_ & h3);
            break;
        case Gate::TIE1:
            cir_->gates_[i].fl_ = PARA_L;
            cir_->gates_[i].fh_ = PARA_H;
            break;
        case Gate::TIE0:
            cir_->gates_[i].fl_ = PARA_H;
            cir_->gates_[i].fh_ = PARA_L;
            break;
		case Gate::PPI:
			if(cir_->connType_ == Circuit::CAPTURE && cir_->gates_[i].frame_ > 0){
				cir_->gates_[i].fl_ = l1;
				cir_->gates_[i].fh_ = h1;
			}
            break;
        default:
            break;
    }
    cir_->gates_[i].fl_ = (cir_->gates_[i].fl_ & ~faultInjectH_[i][0])
                         | faultInjectL_[i][0];
    cir_->gates_[i].fh_ = (cir_->gates_[i].fh_ & ~faultInjectL_[i][0])
                         | faultInjectH_[i][0];
} //}}}

};

#endif


