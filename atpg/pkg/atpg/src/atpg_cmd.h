// **************************************************************************
// File       [ atpg_cmd.h ]
// Author     [ littleshamoo ]
// Synopsis   [ ]
// Date       [ 2011/09/28 created ]
// **************************************************************************

#ifndef _FAN_ATPG_CMD_H_
#define _FAN_ATPG_CMD_H_

#include "common/cmd.h"

#include "fan_mgr.h"

namespace FanNs {

// ATPG commands
class ReadPatCmd : public CommonNs::Cmd {
public:
         ReadPatCmd(const std::string &name, FanMgr *fanMgr);
         ~ReadPatCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    FanMgr *fanMgr_;
};

class ReportPatCmd : public CommonNs::Cmd {
public:
         ReportPatCmd(const std::string &name, FanMgr *fanMgr);
         ~ReportPatCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    FanMgr *fanMgr_;
};

// TODO read rc extraction 
class ReadRCExtraceCmd : public CommonNs::Cmd{
public:
         ReadRCExtraceCmd(const std::string &name, FanMgr *fanMgr);
         ~ReadRCExtraceCmd();
   bool exec(const std::vector<std::string> &argv);

private:
   FanMgr *fanMgr_;
};
//------------------------------

// TODO report read rc extraction
class ReportRCExtraceCmd : public CommonNs::Cmd{
public:
         ReportRCExtraceCmd(const std::string &name, FanMgr *fanMgr);
         ~ReportRCExtraceCmd();
   bool exec(const std::vector<std::string> &argv);

private:
   FanMgr *fanMgr_;
};
//-------------------------------

class AddFaultCmd : public CommonNs::Cmd {
public:
         AddFaultCmd(const std::string &name, FanMgr *fanMgr);
         ~AddFaultCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    void   addAllFault();
    bool   addPinFault(const std::string &type, const std::string &pin);
    bool   addCellFault(const std::string &type, const std::string &cell,
                        const std::string &pin);
    FanMgr *fanMgr_;
};

class ReportFaultCmd : public CommonNs::Cmd {
public:
         ReportFaultCmd(const std::string &name, FanMgr *fanMgr);
         ~ReportFaultCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    FanMgr *fanMgr_;
};

class ReportCircuitCmd : public CommonNs::Cmd {
public:
         ReportCircuitCmd(const std::string &name, FanMgr *fanMgr);
         ~ReportCircuitCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    FanMgr *fanMgr_;
};

class ReportGateCmd : public CommonNs::Cmd {
public:
         ReportGateCmd(const std::string &name, FanMgr *fanMgr);
         ~ReportGateCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    void   reportGate(const int &i) const;
    FanMgr *fanMgr_;
};

class ReportValueCmd : public CommonNs::Cmd {
public:
         ReportValueCmd(const std::string &name, FanMgr *fanMgr);
         ~ReportValueCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    void   reportValue(const int &i) const;
    FanMgr *fanMgr_;
};

class ReportStatsCmd : public CommonNs::Cmd {
public:
         ReportStatsCmd(const std::string &name, FanMgr *fanMgr);
         ~ReportStatsCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    FanMgr *fanMgr_;
};

class AddPinConsCmd : public CommonNs::Cmd {
public:
         AddPinConsCmd(const std::string &name, FanMgr *fanMgr);
         ~AddPinConsCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    FanMgr *fanMgr_;
};

class RunLogicSimCmd : public CommonNs::Cmd {
public:
         RunLogicSimCmd(const std::string &name, FanMgr *fanMgr);
         ~RunLogicSimCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    FanMgr *fanMgr_;
};

class RunFaultSimCmd : public CommonNs::Cmd {
public:
         RunFaultSimCmd(const std::string &name, FanMgr *fanMgr);
         ~RunFaultSimCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    FanMgr *fanMgr_;
};

class RunStaticLearnCmd : public CommonNs::Cmd { 
public: 
        RunStaticLearnCmd(const std::string &name, FanMgr *fanMgr);
        ~RunStaticLearnCmd(); 

    bool exec(const std::vector<std::string> &argv);

private: 
    FanMgr *fanMgr_;
}; 

class RunAtpgCmd : public CommonNs::Cmd {
public:
         RunAtpgCmd(const std::string &name, FanMgr *fanMgr);
         ~RunAtpgCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    FanMgr *fanMgr_;
};

class WriteFaultCmd : public CommonNs::Cmd { 
public: 
         WriteFaultCmd(const std::string &name, FanMgr *fanMgr); 
         ~WriteFaultCmd(); 

    bool exec(const std::vector<std::string> &argv);

private:
    FanMgr *fanMgr_;
}; 

class WritePatCmd : public CommonNs::Cmd {
public:
         WritePatCmd(const std::string &name, FanMgr *fanMgr);
         ~WritePatCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    FanMgr *fanMgr_;
};

//Ne
class AddScanChainsCmd : public CommonNs::Cmd {
public:
         AddScanChainsCmd(const std::string &name, FanMgr *fanMgr);
         ~AddScanChainsCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    FanMgr *fanMgr_;
};
//Ne
class WriteProcCmd : public CommonNs::Cmd {
public:
         WriteProcCmd(const std::string &name, FanMgr *fanMgr);
         ~WriteProcCmd();

    bool exec(const std::vector<std::string> &argv);

private:
    FanMgr *fanMgr_;
};

class ReportScoapCmd : public CommonNs::Cmd {    
public: 
         ReportScoapCmd(const std::string &name, FanMgr *fanMgr); 
         ~ReportScoapCmd(); 

    bool exec(const std::vector<std::string> &argv); 
private: 
    FanMgr *fanMgr_;  

    enum Type { NA = -1, 
                CC1, CC0, CO }; 

    void reportScoap(Type type); 
    void reportCC1(); 
    void reportCC0(); 
    void reportCO(); 
}; 

};

#endif


