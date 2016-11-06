// **************************************************************************
// File       [ circuit.cpp ]
// Author     [ littleshamoo ]
// Synopsis   [ ]
// Date       [ 2011/07/05 created ]
// **************************************************************************

#include <climits>

#include "circuit.h"

using namespace std;
using namespace IntfNs;
using namespace CoreNs;

// map the circuit to our data structure
bool Circuit::build(Netlist * const nl, const int &nframe,
                    const tfConnectType &type) {
    nframe_ = nframe;
    Cell *top = nl->getTop();
    if (!top) {
        cerr << "**ERROR Circuit::build(): no top module set" << endl;
        return false;
    }
    Techlib *lib = nl->getTechlib();
    if (!lib) {
        cerr << "**ERROR Circuit::build(): no technology library" << endl;
        return false;
    }
    nl_ = nl;
    nframe_ = nframe;
    connType_ = type;

    // levelize
    nl->levelize();
    lib->levelize();

    // map netlist to circuit
    createMap();

    // allocate gate memory
    gates_ = new Gate[ngate_ * nframe];
    fis_ = new int[nnet_ * nframe + nppi_ * (nframe - 1)];
    fos_ = new int[nnet_ * nframe + nppi_ * (nframe - 1)];

    // create gates
    
    createGate();
    connectFrame(); // for multiple time frames
	assignFiMinLvl();
    
    runScoap(); 
    setupCircuitParameter(); 

    return true;
}

//{{{ void Circuit::createMap()
// **************************************************************************
// Function   [ void Circuit::createMap() ]
// Author     [ littleshamoo ]
// Synopsis   [ map cell's and ports (in verilog netlist) into gates (in the circuit)
//
//              Gate layout
//              frame 1                           frame 2
//              |----- ------ ------ ----- ------|----- ------ ------
//              | PI1 | PPI1 | gate | PO1 | PPO1 | PI2 | PPI2 | gate | ...
//              |----- ------ ------ ----- ------|----- ------ ------
//            ]
// Date       [ ]
// **************************************************************************
void Circuit::createMap() {
    calNgate();
    calNnet();
} //}}}
//{{{ void Circuit::calNgate()
// calculate total number of gates  
void Circuit::calNgate() {
    Cell *top = nl_->getTop();
    Techlib *lib = nl_->getTechlib();

    ngate_ = 0;  // number of gates
    // map PI to pseudo gates
    portToGate_ = new int[top->getNPort()];
    npi_ = 0;
    for (size_t i = 0; i < top->getNPort(); ++i) {
        Port *p = top->getPort(i);
        if (p->type_ != Port::INPUT)
            continue;
		if(!strcmp(p->name_,"CK") || !strcmp(p->name_,"test_si") || !strcmp(p->name_,"test_se")
          || !strcmp(p->name_, "GND") || !strcmp(p->name_, "VDD"))
			continue;
        portToGate_[i] = npi_;
        ngate_++;
        npi_++;
    }

    // map cell to gates
	// a single cell can have more than one gate in it
    nppi_ = 0;
    cellToGate_ = new int[top->getNCell()];
    for (size_t i = 0 ; i < top->getNCell(); ++i) {
        cellToGate_[i] = ngate_;
        if (top->getCell(i)->libc_) { 
            if (lib->hasPmt(top->getCell(i)->libc_->id_, Pmt::DFF)) {
                nppi_++;
                ngate_++;
            }
            else
                ngate_ += top->getCell(i)->libc_->getNCell();
        } 
        else {
            if (strcmp(top->getCell(i)->typeName_, "dff")==0) {
                nppi_++; 
                ngate_++; 
            } 
            else //TODO: XOR gate 
                ngate_++; 
        }
    }

    // calculate combinational gates
    ncomb_ = ngate_ - npi_ - nppi_;

    // map PO to pseudo gates
    npo_ = 0;
    for (size_t i = 0; i < top->getNPort(); ++i) {
        if (top->getPort(i)->type_ == Port::OUTPUT) {
			if(!strcmp(top->getPort(i)->name_,"test_so"))
				continue;
            portToGate_[i] = ngate_;
            ngate_++;
            npo_++;
        }
    }

    // calculate PPO
    ngate_ += nppi_;
} //}}}
//{{{ void Circuit::calNnet()
// calculate number of nets
void Circuit::calNnet() {
    Cell *top = nl_->getTop();
    Techlib *lib = nl_->getTechlib();

    nnet_ = 0;

    for (size_t i = 0; i < top->getNNet(); ++i) {
        NetSet eqvs = top->getEqvNets(i);
        int nports = 0;
        bool smallest = true;
        for (NetSet::iterator it = eqvs.begin(); it != eqvs.end(); ++it) {
            if ((*it)->id_ < (int)i) {
                smallest = false;
                break;
            }
            nports += (*it)->getNPort();
        }
        if (smallest)
            nnet_ += nports - 1;
    }

    // add internal nets
    for (size_t i = 0; i < top->getNCell(); ++i) {
        Cell *c = top->getCell(i);
        if (!c->libc_) continue; 

        if (!lib->hasPmt(c->typeName_, Pmt::DFF))
            nnet_ += c->libc_->getNNet() - c->libc_->getNPort();
    }
}//}}}
// **************************************************************************
// Function   [ createGate ]
// Commentor  [ Bill ]
// Synopsis   [ usage: create gate's PI,PPI,PP,PPO,Comb,initial all kind of gate's data
//              in:    None
//              out:   void
//            ]
// Date       [ Bill Ver. 1.0 started 20130811 ]
// **************************************************************************
//{{{ void Circuit::createGate()
void Circuit::createGate() {
    int nfi = 0;
    int nfo = 0;
    createPi(nfo);
    createPpi(nfo);
    createComb(nfi, nfo);
    createPo(nfi);
    createPpo(nfi);
} //}}}
//{{{ void Circuit::createPi()
void Circuit::createPi(int &nfo) {
    Cell *top = nl_->getTop();

    for (size_t i = 0; i < top->getNPort(); ++i) {
        Port *p = top->getPort(i);
		if(!strcmp(p->name_,"CK") || !strcmp(p->name_,"test_si") || !strcmp(p->name_,"test_se")
          || !strcmp(p->name_, "GND") || !strcmp(p->name_, "VDD"))
			continue;
        int id = portToGate_[i];
        if (p->type_ != Port::INPUT || id < 0)
            continue;
        gates_[id].name_ = p->name_; 
        gates_[id].id_ = id;
        gates_[id].cid_ = i;
        gates_[id].pmtid_ = 0;
        gates_[id].lvl_ = 0;
        gates_[id].type_ = Gate::PI;
        gates_[id].fos_ = &fos_[nfo];
        nfo += top->getNetPorts(p->inNet_->id_).size() - 1;
    }
} //}}}
//{{{ void Circuit::createPpi()
void Circuit::createPpi(int &nfo) {
    Cell *top = nl_->getTop();
    for (int i = 0; i < nppi_; ++i) {
        Cell *c = top->getCell(i);
        int id = cellToGate_[i]; 
        gates_[id].name_ = c->name_; 
        gates_[id].id_ = id;
        gates_[id].cid_ = i;
        gates_[id].pmtid_ = 0;
        gates_[id].lvl_ = 0;
        gates_[id].type_ = Gate::PPI;
        gates_[id].fos_ = &fos_[nfo];
		int qid = 0;
		while(strcmp(c->getPort(qid)->name_,"Q"))++qid;
		nfo += top->getNetPorts(c->getPort(qid)->exNet_->id_).size() - 2;
        if (!c->libc_) // has no serial output to next SDFF 
            nfo++; 
		
		if(nframe_ > 1 && connType_ == SHIFT && i < nppi_-1)++nfo;
		

	}
} //}}}
//{{{ void Circuit::createComb()
// create combinational logic gates
void Circuit::createComb(int &nfi, int &nfo) {
    Cell *top = nl_->getTop();

    for (int i = nppi_; i < (int)top->getNCell(); ++i) {
        Cell *c = top->getCell(i);
        if (c->libc_) { 
            for (int j = 0; j < (int)c->libc_->getNCell(); ++j) {
                int id = cellToGate_[i] + j;
                gates_[id].id_ = id;
                gates_[id].cid_ = c->id_;
                gates_[id].pmtid_ = j;
    
                Pmt *pmt = (Pmt *)c->libc_->getCell(j);
                createPmt(id, c, pmt, nfi, nfo);
                gates_[id].co_i_ = new int[gates_[id].nfi_]; 
                for(int pid=0; pid<gates_[id].nfi_; pid++)
                    gates_[id].co_i_[pid] = 0;   
            }
        }
        else { //TODO: XOR gate 
            int id = cellToGate_[i];
            gates_[id].name_ = c->name_; 
            gates_[id].id_ = id;
            gates_[id].cid_ = c->id_;
    
            createVerilogPmt(id, c, nfi, nfo); 
            gates_[id].co_i_ = new int[gates_[id].nfi_]; 
            for(int pid=0; pid<gates_[id].nfi_; pid++)
                gates_[id].co_i_[pid] = 0;   
        }
    }
    if (top->getNCell() > 0)
        lvl_ = gates_[cellToGate_[top->getNCell() - 1]].lvl_ + 2;
}
//}}}
void Circuit::createVerilogPmt(const int &id, const Cell * const c,
                        int &nfi, int &nfo) {

    detVerilogPmtType(id, c); 

    gates_[id].fis_ = &fis_[nfi]; 
    int maxLvl = -1;
    for (size_t i=0; i<c->getNPort(); ++i) {
        if (c->getPort(i)->type_ != Port::INPUT) 
            continue; 

        Net *nin = c->getPort(i)->exNet_;
        for (size_t j=0; j<nin->getNPort(); j++) {
            Port *p = nin->getPort(j);  
            if (p==c->getPort(i)) 
                continue; 

            int inId = 0; 
            // TODO: XOR gate 
            if (p->type_==Port::OUTPUT && p->top_!=c->top_) 
                inId = cellToGate_[p->top_->id_]; 
            else if (p->type_==Port::INPUT && p->top_==c->top_)
                inId = portToGate_[p->id_];
            else continue; 

            gates_[id].fis_[gates_[id].nfi_] = inId;
            gates_[id].nfi_++;
            nfi++;

            gates_[inId].fos_[gates_[inId].nfo_] = id;
            gates_[inId].nfo_++;

            if (gates_[inId].lvl_ > maxLvl)
                maxLvl = gates_[inId].lvl_;
        }
    }

    gates_[id].lvl_ = maxLvl + 1;

    gates_[id].fos_ = &fos_[nfo];
    Port *outp = NULL;
    for (size_t i = 0; i < c->getNPort() && !outp; ++i) {
        if (c->getPort(i)->type_ == Port::OUTPUT)
            outp = c->getPort(i);
    }
    PortSet ps = c->top_->getNetPorts(outp->exNet_->id_);
    PortSet::iterator it = ps.begin();
    for ( ; it != ps.end(); ++it) {
        if ((*it)->top_ != c->top_ && (*it)->top_ != c) {
            nfo += 1; 
        }
        else if ((*it)->top_ == c->top_) {
            nfo += 1; 
        }
    }
}

void Circuit::detVerilogPmtType(const int &id, const IntfNs::Cell * const c) {
    if (!strcmp(c->typeName_, "and")) { 
        gates_[id].type_ = Gate::AND;
        if (c->getNPort() == 3)
            gates_[id].type_ = Gate::AND2;
        else if (c->getNPort() == 4)
            gates_[id].type_ = Gate::AND3;
        else if (c->getNPort() == 5)
            gates_[id].type_ = Gate::AND4;
        else { 
            fprintf(stderr, "**WARN Circuit::detVerilogPmtType(): cell ");
            fprintf(stderr, " `%s ", c->name_);
            fprintf(stderr, "has larger fan-in counts than normal\n");
        }
    } 
    else if (!strcmp(c->typeName_, "nand")) { 
        gates_[id].type_ = Gate::NAND;
        if (c->getNPort() == 3)
            gates_[id].type_ = Gate::NAND2;
        else if (c->getNPort() == 4)
            gates_[id].type_ = Gate::NAND3;
        else if (c->getNPort() == 5)
            gates_[id].type_ = Gate::NAND4;
        else { 
            fprintf(stderr, "**WARN Circuit::detVerilogPmtType(): cell ");
            fprintf(stderr, " `%s ", c->name_);
            fprintf(stderr, "has larger fan-in counts than normal\n");
        }
    }
    else if (!strcmp(c->typeName_, "or")) { 
        gates_[id].type_ = Gate::OR;
        if (c->getNPort() == 3)
            gates_[id].type_ = Gate::OR2;
        else if (c->getNPort() == 4)
            gates_[id].type_ = Gate::OR3;
        else if (c->getNPort() == 5)
            gates_[id].type_ = Gate::OR4;
        else { 
            fprintf(stderr, "**WARN Circuit::detVerilogPmtType(): cell ");
            fprintf(stderr, " `%s ", c->name_);
            fprintf(stderr, "has larger fan-in counts than normal\n");
        }
    }
    else if (!strcmp(c->typeName_, "nor")) { 
        gates_[id].type_ = Gate::NOR; 
        if (c->getNPort() == 3)
            gates_[id].type_ = Gate::NOR2;
        else if (c->getNPort() == 4)
            gates_[id].type_ = Gate::NOR3;
        else if (c->getNPort() == 5)
            gates_[id].type_ = Gate::NOR4;
        else { 
            fprintf(stderr, "**WARN Circuit::detVerilogPmtType(): cell ");
            fprintf(stderr, " `%s ", c->name_);
            fprintf(stderr, "has larger fan-in counts than normal\n");
        }
    } 
    else if (!strcmp(c->typeName_, "xor")) 
        gates_[id].type_ = Gate::XOR; // TODO 
    else if (!strcmp(c->typeName_, "xnor")) 
        gates_[id].type_ = Gate::XNOR; // TODO 
    else if (!strcmp(c->typeName_, "not")) 
        gates_[id].type_ = Gate::INV; 
    else if (!strcmp(c->typeName_, "buf")) 
        gates_[id].type_ = Gate::BUF; 
    else { 
        fprintf(stderr, "**ERROR Circuit::createVerilogPmt(): cell type");
        fprintf(stderr, " `%s/ ", c->typeName_);
        fprintf(stderr, "not recognized cell type\n");
        assert(0); 
    }
} 

//{{{ void Circuit::createPmt()
// primitive is from Mentor .mdt
// cell--> primitive  --> gate
// but primitive is not actually in our data structure
void Circuit::createPmt(const int &id, const Cell * const c,
                        const Pmt * const pmt, int &nfi, int &nfo) {
    // determine gate type
    detGateType(id, c, pmt);

    // determine fanin and level
    gates_[id].fis_ = &fis_[nfi];
    int maxLvl = -1;
    for (size_t i = 0; i < pmt->getNPort(); ++i) {
        if (pmt->getPort(i)->type_ != Port::INPUT)
            continue;
        Net *nin = pmt->getPort(i)->exNet_;
        for (size_t j = 0; j < nin->getNPort(); ++j) {
            Port *p = nin->getPort(j);
            if (p == pmt->getPort(i))
                continue;

            int inId = 0;
            // internal connection
            if (p->type_ == Port::OUTPUT && p->top_ != c->libc_)
                inId = cellToGate_[c->id_] + p->top_->id_;

            // external connection
            else if (p->type_ == Port::INPUT && p->top_ == c->libc_) {
                Net *nex = c->getPort(p->id_)->exNet_;
                PortSet ps = c->top_->getNetPorts(nex->id_);
                PortSet::iterator it = ps.begin();
                for ( ; it != ps.end(); ++it) {
                    Cell *cin = (*it)->top_;
                    if ((*it)->type_ == Port::OUTPUT && cin != c->top_) {
                        CellSet cs = cin->libc_->getPortCells((*it)->id_);
                        inId = cellToGate_[cin->id_];
						if (nl_->getTechlib()->hasPmt(cin->libc_->id_, Pmt::DFF))//NE
							break;
                        inId += (*cs.begin())->id_;
                        break;
                    }
                    else if ((*it)->type_ == Port::INPUT && cin == c->top_) {
                        inId = portToGate_[(*it)->id_];
                        break;
                    }
                }
            }


            gates_[id].fis_[gates_[id].nfi_] = inId;
            gates_[id].nfi_++;
            nfi++;

            gates_[inId].fos_[gates_[inId].nfo_] = id;
            gates_[inId].nfo_++;

            if (gates_[inId].lvl_ > maxLvl)
                maxLvl = gates_[inId].lvl_;
        }
    }
    gates_[id].lvl_ = maxLvl + 1;

    // determine fanout
    
    gates_[id].fos_ = &fos_[nfo];
    Port *outp = NULL;
    for (size_t i = 0; i < pmt->getNPort() && !outp; ++i) {
        if (pmt->getPort(i)->type_ == Port::OUTPUT)
            outp = pmt->getPort(i);
    }
    PortSet ps = c->libc_->getNetPorts(outp->exNet_->id_);
    PortSet::iterator it = ps.begin();
    for ( ; it != ps.end(); ++it) {
        if ((*it)->top_ != c->libc_ && (*it)->top_ != pmt) {
            nfo += 1;
        }
        else if ((*it)->top_ == c->libc_) {
            int nid = c->getPort((*it)->id_)->exNet_->id_;
            nfo += c->top_->getNetPorts(nid).size() - 1;
        }
    }
} //}}}
//{{{ void Circuit::detGateType()
void Circuit::detGateType(const int &id, const Cell * const c,
                          const Pmt * const pmt) {
    switch (pmt->type_) {
        case Pmt::BUF:
            gates_[id].type_ = Gate::BUF;
            break;
        case Pmt::INV:
        case Pmt::INVF:
            gates_[id].type_ = Gate::INV;
            break;
        case Pmt::MUX:
            gates_[id].type_ = Gate::MUX;
            break;
        case Pmt::AND:
            if (c->getNPort() == 3)
                gates_[id].type_ = Gate::AND2;
            else if (c->getNPort() == 4)
                gates_[id].type_ = Gate::AND3;
            else if (c->getNPort() == 5)
                gates_[id].type_ = Gate::AND4;
            else
                gates_[id].type_ = Gate::AND;
            break;
        case Pmt::NAND:
            if (c->getNPort() == 3)
                gates_[id].type_ = Gate::NAND2;
            else if (c->getNPort() == 4)
                gates_[id].type_ = Gate::NAND3;
            else if (c->getNPort() == 5)
                gates_[id].type_ = Gate::NAND4;
            else
                gates_[id].type_ = Gate::NAND;
            break;
        case Pmt::OR:
            if (c->getNPort() == 3)
                gates_[id].type_ = Gate::OR2;
            else if (c->getNPort() == 4)
                gates_[id].type_ = Gate::OR3;
            else if (c->getNPort() == 5)
                gates_[id].type_ = Gate::OR4;
            else
                gates_[id].type_ = Gate::OR;
            break;
        case Pmt::NOR:
            if (c->getNPort() == 3)
                gates_[id].type_ = Gate::NOR2;
            else if (c->getNPort() == 4)
                gates_[id].type_ = Gate::NOR3;
            else if (c->getNPort() == 5)
                gates_[id].type_ = Gate::NOR4;
            else
                gates_[id].type_ = Gate::NOR;
            break;
        case Pmt::XOR:
            if (c->getNPort() == 3)
                gates_[id].type_ = Gate::XOR2;
            else if (c->getNPort() == 4)
                gates_[id].type_ = Gate::XOR3;
            else
                gates_[id].type_ = Gate::XOR;
            break;
        case Pmt::XNOR:
            if (c->getNPort() == 3)
                gates_[id].type_ = Gate::XNOR2;
            else if (c->getNPort() == 4)
                gates_[id].type_ = Gate::XNOR3;
            else
                gates_[id].type_ = Gate::XNOR;
            break;
        case Pmt::TIE1:
            gates_[id].type_ = Gate::TIE1;
            break;
        case Pmt::TIE0:
            gates_[id].type_ = Gate::TIE0;
            break;
        case Pmt::TIEX:
            gates_[id].type_ = Gate::TIE0;
            break;
        case Pmt::TIEZ:
            gates_[id].type_ = Gate::TIEZ;
            break;
        default:
            gates_[id].type_ = Gate::NA;
    }
} //}}}
//{{{ void Circuit::createPo()
void Circuit::createPo(int &nfi) {
    Cell *top = nl_->getTop();

    for (size_t i = 0; i < top->getNPort(); ++i) {
        Port *p = top->getPort(i);
		if(!strcmp(p->name_,"test_so"))
			continue;
        int id = portToGate_[i];
        if (p->type_ != Port::OUTPUT || id < 0)
            continue;
        gates_[id].name_ = p->name_; 
        gates_[id].id_ = id;
        gates_[id].cid_ = i;
        gates_[id].pmtid_ = 0;
        gates_[id].lvl_ = lvl_ - 1;
        gates_[id].type_ = Gate::PO;
        gates_[id].fis_ = &fis_[nfi];
        PortSet ps = top->getNetPorts(p->inNet_->id_);
        PortSet::iterator it = ps.begin();
        for ( ; it != ps.end(); ++it) {
            if ((*it) == p)
                continue;
            int inId = 0;
            if ((*it)->top_ == top && (*it)->type_ == Port::INPUT)
                inId = portToGate_[(*it)->id_];
            else if ((*it)->top_ != top && (*it)->type_ == Port::OUTPUT) {
                inId = cellToGate_[(*it)->top_->id_];
                Cell *libc = (*it)->top_->libc_;
                if (libc) 
				    if (!nl_->getTechlib()->hasPmt(libc->id_, Pmt::DFF))//NE
					    inId += (*libc->getPortCells((*it)->id_).begin())->id_;
            }
			else continue;

            gates_[id].fis_[gates_[id].nfi_] = inId;
            gates_[id].nfi_++;

            gates_[inId].fos_[gates_[inId].nfo_] = id;
            gates_[inId].nfo_++;

            nfi++;
        }
        gates_[id].co_i_ = new int[gates_[id].nfi_]; 
        for(int pid=0; pid<gates_[id].nfi_; pid++)
            gates_[id].co_i_[pid] = 0;   
    }
} //}}}
//{{{ void Circuit::createPpo()
void Circuit::createPpo(int &nfi) {
    Cell *top = nl_->getTop();
    for (int i = 0; i < nppi_; ++i) {
        Cell *c = top->getCell(i);
        int id = ngate_ - nppi_ + i ;
        gates_[id].name_ = c->name_; 
        gates_[id].id_ = id;
        gates_[id].cid_ = i;
        gates_[id].pmtid_ = 0;
        gates_[id].lvl_ = lvl_ - 1;
        gates_[id].type_ = Gate::PPO;
        gates_[id].fis_ = &fis_[nfi];
		int did = 0;
		while(strcmp(c->getPort(did)->name_,"D"))++did;
		Port *inp = c->getPort(did);

        PortSet ps = top->getNetPorts(c->getPort(inp->id_)->exNet_->id_);
        PortSet::iterator it = ps.begin();
        for ( ; it != ps.end(); ++it) {
            if ((*it) == inp)
                continue;
            int inId = 0;
            if ((*it)->top_ == top && (*it)->type_ == Port::INPUT)
                inId = portToGate_[(*it)->id_];
            else if ((*it)->top_ != top && (*it)->type_ == Port::OUTPUT) {
                inId = cellToGate_[(*it)->top_->id_];
                Cell *libc = (*it)->top_->libc_;
                if (libc) 
				    if (!nl_->getTechlib()->hasPmt(libc->id_, Pmt::DFF))//NE
					    inId += (*libc->getPortCells((*it)->id_).begin())->id_;
            }
			else continue;

            gates_[id].fis_[gates_[id].nfi_] = inId;
            gates_[id].nfi_++;

            gates_[inId].fos_[gates_[inId].nfo_] = id;
            gates_[inId].nfo_++;

            nfi++;
        }
        gates_[id].co_i_ = new int[gates_[id].nfi_]; 
        for(int pid=0; pid<gates_[id].nfi_; pid++)
            gates_[id].co_i_[pid] = 0;   
    }
} //}}}

// **************************************************************************
// Function   [ connectFrame]
// Commentor  [ LingYunHsu]
// Synopsis   [ usage: connect gates in different time rames, 
//              this is for multiple time frame ATPG
//              in:    void
//              out:   void
//            ]
// Date       [ LingYunHsu Ver. 1.0 started 20130818 ]
// **************************************************************************

//{{{ void Circuit::connectFrame()
void Circuit::connectFrame() { 
    int nfo = nnet_;
    int nfi = nnet_;
    for (int i = 1; i < nframe_; ++i) {
        int offset = ngate_ * i;
		if(connType_ == CAPTURE)
			for (int j = 0; j < nppi_; ++j){ ///give nfo and fos before nppi_
				int id = offset - nppi_ + j;
				gates_[id].nfo_ = 1;
				gates_[id].fos_ = &fos_[nfo];
				nfo += gates_[id].nfo_;
			}
        for (int j = 0; j < ngate_; ++j) { ///read pattern
            int id = offset + j;
            gates_[id].id_ = id;
            gates_[id].cid_ = gates_[j].cid_;
            gates_[id].lvl_ = lvl_ * i + gates_[j].lvl_;
            gates_[id].type_ = gates_[j].type_;
            gates_[id].frame_ = i; ///record id, cid, level, type and frame
			if(gates_[id].type_ != Gate::PPI && gates_[id].type_ != Gate::PPO){
				gates_[id].nfo_ = gates_[j].nfo_;
				gates_[id].fos_ = &fos_[nfo];
				for (int k = 0; k < gates_[j].nfo_; ++k)
					gates_[id].fos_[k] = gates_[j].fos_[k] + offset;
				nfo += gates_[id].nfo_;
				gates_[id].nfi_ = gates_[j].nfi_;
				gates_[id].fis_ = &fis_[nfi];
				for (int k = 0; k < gates_[j].nfi_; ++k)
					gates_[id].fis_[k] = gates_[j].fis_[k] + offset;
				nfi += gates_[id].nfi_;
			} ///if not ppi or ppo
			else if (gates_[id].type_ == Gate::PPI){
				gates_[id].nfo_ = gates_[j].nfo_;
				//if(connType_ == SHIFT && i == nframe_-1 && (j!= npi_+nppi_-1))--gates_[id].nfo_;
				gates_[id].fos_ = &fos_[nfo];
				for (int k = 0; k < gates_[j].nfo_; ++k)
					gates_[id].fos_[k] = gates_[j].fos_[k] + offset;
				nfo += gates_[id].nfo_;
				gates_[id].nfi_ = 1;
				gates_[id].fis_ = &fis_[nfi];
				if(connType_ == CAPTURE){
					gates_[id].fis_[0] = id - npi_ - nppi_ ;
					gates_[id].type_ = Gate::BUF;
					gates_[id - npi_ - nppi_].fos_[0] = id ;
					//gates_[id - npi_ - nppi_].type_ = Gate::BUF ;
				} ///do CAPTURE
				else if(id != (offset + npi_)){
					gates_[id].fis_[0] = id - ngate_ - 1 ;
					gates_[id].type_ = Gate::BUF;
					gates_[id - ngate_ - 1].fos_[gates_[id - ngate_ - 1].nfo_] = id ;
					++gates_[id - ngate_ - 1].nfo_ ;
				} ///do SHIFT
				else
					gates_[id].nfi_ = 0;
				//gates_[id - npi_ - nppi_].fos_[0] = id ;
				nfi += gates_[id].nfi_;
			} ///if ppi
			else{
				gates_[id].nfo_ = 0;
				gates_[id].fos_ = &fos_[nfo];
				gates_[id].nfi_ = gates_[j].nfi_;
				gates_[id].fis_ = &fis_[nfi];
				for (int k = 0; k < gates_[j].nfi_; ++k)
					gates_[id].fis_[k] = gates_[j].fis_[k] + offset;
				nfi += gates_[id].nfi_;
			}///if ppo
        }
		if(connType_ == CAPTURE)
			for (int j = 0; j < nppi_; ++j){
				int id = offset - nppi_ + j;
				gates_[id].type_ = Gate::BUF;
			} //reset after for loop
    }
    tlvl_ = lvl_ * nframe_;
    tgate_ = ngate_ * nframe_;
} //}}}

// **************************************************************************
// Function   [ Circuit::assignFiMinLvl() ]
// Commentor  [ Jun-Han,Pan ]
// Synopsis   [ usage: Find the minimum from every gate's fan-ins' level and assign
//                     it to the gate->fiMinLvl.
//                     used for headLine justification (atpg.h)
//              in:    gate array.
//              out:   g->fiMinLvl_ is updated
//            ]
// Date       [ Jun-Han,Pan Ver. 1.0 at 2013/08/18 ]
// **************************************************************************

void Circuit::assignFiMinLvl(){
	for(int i = 0; i < tgate_ ; ++i){
		Gate* g = &gates_[i];
		int minLvl = g->lvl_;
		for(int j = 0; j < g->nfi_; ++j)
			if(gates_[g->fis_[j]].lvl_ < minLvl){
				minLvl = gates_[g->fis_[j]].lvl_;
				g->fiMinLvl_ = minLvl;
//          find the minimum among g->fis_[j] and replace g->fiMinLvl with it.
//                                *********
//                       g->nfi_  *       *
//          ************          *   g   *
//          *g->fis_[j]* -------- *       *
//          ************          *********
			}
	}
}


void Circuit::runScoap() {
    //calculate cc 
    for(int i=0; i<tgate_; ++i) {
        Gate& g = gates_[i]; 
        switch(g.type_) {
        case Gate::PI: 
        case Gate::PPI: 
            g.cc0_=g.cc1_=1; 
            break; 
        case Gate::PO: 
        case Gate::PPO: 
            g.cc0_=gates_[g.fis_[0]].cc0_; 
            g.cc1_=gates_[g.fis_[0]].cc1_; 
            break; 
        case Gate::INV: 
        case Gate::BUF: 
        {
            Gate fi = gates_[g.fis_[0]]; 
            g.cc0_ = (!g.isInverse())?fi.cc0_:fi.cc1_; 
            g.cc1_ = (!g.isInverse())?fi.cc1_:fi.cc0_; 

            g.cc0_+=1; 
            g.cc1_+=1; 
            break; 
        }
        case Gate::AND: 
        case Gate::AND2: 
        case Gate::AND3: 
        case Gate::AND4: 
        case Gate::NAND: 
        case Gate::NAND2: 
        case Gate::NAND3: 
        case Gate::NAND4: 
        { 
            int cc1_sum = 0; 
            int cc0_min = INT_MAX; 
            for(int i=0; i<g.nfi_; i++) { 
                Gate fi = gates_[g.fis_[i]]; 
                cc1_sum+=fi.cc1_; 
                if(fi.cc0_ < cc0_min) 
                    cc0_min = fi.cc0_; 
            }

            g.cc0_ = (!g.isInverse())?cc0_min:cc1_sum; 
            g.cc1_ = (!g.isInverse())?cc1_sum:cc0_min; 

            g.cc0_+=1; 
            g.cc1_+=1; 
            break; 
        }
        case Gate::OR: 
        case Gate::OR2: 
        case Gate::OR3: 
        case Gate::OR4: 
        case Gate::NOR: 
        case Gate::NOR2: 
        case Gate::NOR3: 
        case Gate::NOR4: 
        { 
            int cc0_sum = 0; 
            int cc1_min = INT_MAX; 
            for(int i=0; i<g.nfi_; i++) { 
                Gate fi = gates_[g.fis_[i]]; 
                cc0_sum+=fi.cc0_; 
                if(fi.cc1_ < cc1_min) 
                    cc1_min = fi.cc1_; 
            }

            g.cc0_ = (!g.isInverse())?cc0_sum:cc1_min; 
            g.cc1_ = (!g.isInverse())?cc1_min:cc0_sum; 

            g.cc0_+=1; 
            g.cc1_+=1; 
            break; 
        }
        default: 
            cout << "***WARNNING: Circuit::runScoap(): gate type "; 
            cout << g.type_; 
            cout << " currently not supported...\n"; 
            break; 
        }
    }

    //calculate co 
    for(int n=tgate_-1; n>=0; --n) {
        Gate *g = &gates_[n]; 
        //set co_o_
        bool co_o_set = false; 
        int co_o_min = INT_MAX; 
        for(int i=0; i<g->nfo_; ++i) {
            Gate *fo = &gates_[g->fos_[i]]; 
            for(int j=0; j<fo->nfi_; ++j) { 
                if(fo->fis_[j]==g->id_ && 
                     fo->co_i_[j]<co_o_min) { 
                        co_o_min = fo->co_i_[j]; 
                        co_o_set = true; 
                }
            }
        }
        g->co_o_ = (!co_o_set)?0:co_o_min; 

        //set co_i_[]; 
        switch(g->type_) { 
        case Gate::PI: 
        case Gate::PPI: 
            break; 
        case Gate::PO: 
        case Gate::PPO: 
            g->co_i_[0] = 0; 
            break; 
        case Gate::INV: 
        case Gate::BUF: 
            g->co_i_[0] = g->co_o_+1; 
            break; 
        case Gate::AND: 
        case Gate::AND2: 
        case Gate::AND3: 
        case Gate::AND4: 
        case Gate::NAND: 
        case Gate::NAND2: 
        case Gate::NAND3: 
        case Gate::NAND4: 
        case Gate::OR: 
        case Gate::OR2: 
        case Gate::OR3: 
        case Gate::OR4: 
        case Gate::NOR:  
        case Gate::NOR2: 
        case Gate::NOR3: 
        case Gate::NOR4: 
        {
            for(int i=0; i<g->nfi_; ++i) { 
                g->co_i_[i] = g->co_o_ + 1; 
                for(int j=0; j<g->nfi_; ++j) { 
                    if(j==i) continue; 
                    int cc = (g->getInputNonCtrlValue()==L)?gates_[g->fis_[j]].cc0_:gates_[g->fis_[j]].cc1_;  
                    g->co_i_[i]+=cc; 
                }
            }
            break; 
        }
        default: 
            cout << "***WARNNING: Circuit::runScoap(): gate type "; 
            cout << g->type_; 
            cout << " currently not supported...\n"; 
            break; 
        }
    }
}

void Circuit::setupCircuitParameter() {
    identifyLineParameter(); 
}

void Circuit::identifyLineParameter() {
    nHeadLine_ = 0;// number of head line

	// go through all the gates
    for(int i = 0 ; i < tgate_ ; i++ ){

		// get ith gate from gate array in cir_
        Gate &g = gates_[i];

		// assign as FREE_LINE first
        g.ltype_ = Gate::FREE_LINE;

		// check it is BOUND_LINE or not
        if (g.type_ != Gate::PI && g.type_ != Gate::PPI)
            for (int j = 0 ; j < g.nfi_ ; j++ )//g.nfi_  number of fanin

				// if one of fanin is not FREE_LINE, then set lineType as BOUND_LINE
                if ( g.ltype_!= Gate::FREE_LINE ) {
                    g.ltype_ = Gate::BOUND_LINE;
                    break;
                }
		// check it is HEAD_LINE or not(rule 1)
        if( g.ltype_== Gate::FREE_LINE && g.nfo_ != 1){//g.nfo_  number of fanout
            g.ltype_ = Gate::HEAD_LINE;
            nHeadLine_++;//number of head line + 1
        }

		// check it is HEAD_LINE or not(rule 2)
        if( g.ltype_ == Gate::BOUND_LINE )
            for ( int j = 0 ; j < g.nfi_ ; j++ )//g.nfi_  number of fanin
                if ( g.ltype_== Gate::FREE_LINE ){
                    g.ltype_ = Gate::HEAD_LINE;
                    nHeadLine_++;
                }
    }

	// store all head lines to array  headLines_
    headLines_ = new int[nHeadLine_];
    int inHead = 0;
    for(int i = 0 ; i < tgate_ ; i++ ) { 
        Gate &g = gates_[i];
        if( g.ltype_ == Gate::HEAD_LINE){
            headLines_[inHead++] = g.id_;
            if( inHead == nHeadLine_ )
                break;
        }
    }
}
