// **************************************************************************
// File       [ main.cpp ]
// Author     [ littleshamoo ]
// Synopsis   [ ]
// Date       [ 2011/07/05 created ]
// **************************************************************************

#include <cstdlib>

#include "common/sys_cmd.h"
#include "setup_cmd.h"
#include "atpg_cmd.h"
#include "misc_cmd.h"

using namespace std;
using namespace CommonNs;
using namespace IntfNs;
using namespace CoreNs;
using namespace FanNs;

void printWelcome();
void initOpt(OptMgr &mgr);
void initCmd(CmdMgr &cmdMgr, FanMgr &fanMgr);
void printGoodbye(TmUsage &tmusg);

int main(int argc, char **argv) {

    // start calculating resource usage
    TmUsage tmusg;
    tmusg.totalStart();

    // initialize option manager
    OptMgr optMgr;
    initOpt(optMgr);

    optMgr.parse(argc, argv);
    if (optMgr.isFlagSet("h")) {
        optMgr.usage();
        exit(0);
    }

    // initialize command manager and FAN manager
    FanMgr fanMgr;
    CmdMgr cmdMgr;
    initCmd(cmdMgr, fanMgr);
    CmdMgr::Result res = CmdMgr::SUCCESS;

    // welcome message
    printWelcome();

    // run startup commands
    if (optMgr.isFlagSet("f")) {
        string cmd = "source " + optMgr.getFlagVar("f");
        res = cmdMgr.exec(cmd);
    }

    // read techlib
    if (optMgr.isFlagSet("l")) {
        string cmd = "read_lib " + optMgr.getFlagVar("l");
        res = cmdMgr.exec(cmd);
    }

    // enter user interface
    while (res != CmdMgr::EXIT) {
        res = cmdMgr.read();
        if (res == CmdMgr::NOT_EXIST) {
            cerr << "**ERROR main(): command `" << cmdMgr.getErrorStr();
            cerr << "' not found" << endl;
            continue;
        }
    }

    // goodbye
    printGoodbye(tmusg);
    return 0;
}

void printWelcome() {
    cout << "#  LaDS ATPG-SYSTEM v1.0a                      ";
    cout << "                               Feb 2016" << endl;
    cout << "#                     Copyright(c) LaDS";
    cout << "(I) GIEE NTU TAIWAN "                    << endl;
    cout << "#"                                       << endl;
    cout << "#                              All Righ";
    cout << "ts Reserved."                            << endl;
    cout << "#"                                       << endl;

    // system information
    // OS kernel
    FILE *sysout;
    sysout = popen("uname -s 2> /dev/null", "r");
    char buf[128];
    cout << "#  Kernel:   ";
    if (!sysout)
        cout << "UNKNOWN" << endl;
    else {
        if (fgets(buf, sizeof(buf), sysout))
            cout << buf;
        else
            cout << "UNKNOWN" << endl;
        pclose(sysout);
    }

    // platform
    sysout = popen("uname -i 2> /dev/null", "r");
    cout << "#  Platform: ";
    if (!sysout)
        cout << "UNKNOWN" << endl;
    else {
        if (fgets(buf, sizeof(buf), sysout))
            cout << buf;
        else
            cout << "UNKNOWN" << endl;
        pclose(sysout);
    }

    // memory
    FILE *meminfo = fopen("/proc/meminfo", "r");
    cout << "#  Memory:   ";
    if (!meminfo)
        cout << "UNKNOWN" << endl;
    else {
        while (fgets(buf, 128, meminfo)) {
            char *ch;
            if ((ch = strstr(buf, "MemTotal:"))) {
                cout << (double)atol(ch + 9) / 1024.0 << " MB" << endl;
                break;
            }
        }
        fclose(meminfo);
    }

    // cout << "#  Note:     ";
    // cout << "UI, including COMMON & INTERFACE package, is" << endl; 
    // cout << "#            in the courtesy of LaDS(II)" << endl; 

    cout << "#" << endl;
}

void printGoodbye(TmUsage &tmusg) {
    TmStat stat;
    tmusg.getTotalUsage(stat);
    cout << "#  Goodbye" << endl;
    cout << "#  Runtime        ";
    cout << "real " << (double)stat.rTime / 1000000.0 << " s        ";
    cout << "user " << (double)stat.uTime / 1000000.0 << " s        ";
    cout << "sys "  << (double)stat.sTime / 1000000.0 << " s" << endl;
    cout << "#  Memory         ";
    cout << "peak " << (double)stat.vmPeak / 1024.0 << " MB" << endl;
}

void initOpt(OptMgr &mgr) {
    // set program information
    mgr.setName("fan");
    mgr.setShortDes("FAN based ATPG");
    mgr.setDes("FAN based ATPG");

    // register options
    Opt *opt = new Opt(Opt::BOOL, "print this usage", "");
    opt->addFlag("h");
    opt->addFlag("help");
    mgr.regOpt(opt);

    opt = new Opt(Opt::STR_REQ, "execute command file at startup", "file");
    opt->addFlag("f");
    mgr.regOpt(opt);

    opt = new Opt(Opt::STR_REQ, "read technology library", "techlib");
    opt->addFlag("l");
    opt->addFlag("lib");
    mgr.regOpt(opt);
}

void initCmd(CmdMgr &cmdMgr, FanMgr &fanMgr) {
    // system commands
    Cmd *listCmd   = new SysListCmd("ls");
    Cmd *cdCmd     = new SysCdCmd("cd");
    Cmd *catCmd    = new SysCatCmd("cat");
    Cmd *pwdCmd    = new SysPwdCmd("pwd");
    Cmd *setCmd    = new SysSetCmd("set", &cmdMgr);
    Cmd *exitCmd   = new SysExitCmd("exit", &cmdMgr);
    Cmd *quitCmd   = new SysExitCmd("quit", &cmdMgr);
    Cmd *sourceCmd = new SysSourceCmd("source", &cmdMgr);
    Cmd *helpCmd   = new SysHelpCmd("help", &cmdMgr);
    cmdMgr.regCmd("SYSTEM", listCmd);
    cmdMgr.regCmd("SYSTEM", cdCmd);
    cmdMgr.regCmd("SYSTEM", catCmd);
    cmdMgr.regCmd("SYSTEM", pwdCmd);
    cmdMgr.regCmd("SYSTEM", setCmd);
    cmdMgr.regCmd("SYSTEM", exitCmd);
    cmdMgr.regCmd("SYSTEM", quitCmd);
    cmdMgr.regCmd("SYSTEM", sourceCmd);
    cmdMgr.regCmd("SYSTEM", helpCmd);

    // setup commands
    Cmd *readLibCmd      = new ReadLibCmd("read_lib", &fanMgr);
    Cmd *readNlCmd       = new ReadNlCmd("read_netlist", &fanMgr);
	
	//TODO new fault type fault tuple
    Cmd *setFaultTypeCmd = new SetFaultTypeCmd("set_fault_type", &fanMgr);
	//---------------------------------------------------------------------------------
	
    Cmd *buildCirCmd     = new BuildCircuitCmd("build_circuit", &fanMgr);
    Cmd *reportNlCmd     = new ReportNetlistCmd("report_netlist", &fanMgr);
    Cmd *reportCellCmd   = new ReportCellCmd("report_cell", &fanMgr);
    Cmd *reportLibCmd    = new ReportLibCmd("report_lib", &fanMgr);
    Cmd *setPatternTypeCmd = new SetPatternTypeCmd("set_pattern_type", &fanMgr);
    Cmd *setStaticCompressionCmd = new SetStaticCompressionCmd("set_static_compression", &fanMgr);
    Cmd *setDynamicCompressionCmd = new SetDynamicCompressionCmd("set_dynamic_compression", &fanMgr);
    Cmd *setXFillCmd = new SetXFillCmd("set_X-Fill", &fanMgr);
    Cmd *setDFSCmd = new SetDFSCmd("set_dfs_atpg", &fanMgr); 
    Cmd *setObjOptimCmd = new SetObjOptimCmd("set_oo", &fanMgr); 
    cmdMgr.regCmd("SETUP", readLibCmd);
    cmdMgr.regCmd("SETUP", readNlCmd);
    cmdMgr.regCmd("SETUP", setFaultTypeCmd);
    cmdMgr.regCmd("SETUP", buildCirCmd);
    cmdMgr.regCmd("SETUP", reportNlCmd);
    cmdMgr.regCmd("SETUP", reportCellCmd);
    cmdMgr.regCmd("SETUP", reportLibCmd);
    cmdMgr.regCmd("SETUP", setPatternTypeCmd);
    cmdMgr.regCmd("SETUP", setStaticCompressionCmd);
    cmdMgr.regCmd("SETUP", setDynamicCompressionCmd);
    cmdMgr.regCmd("SETUP", setXFillCmd);
    cmdMgr.regCmd("SETUP", setDFSCmd); 
    cmdMgr.regCmd("SETUP", setObjOptimCmd); 

    // ATPG commands
    Cmd *readPatCmd       = new ReadPatCmd("read_pattern", &fanMgr);
    Cmd *reportPatCmd     = new ReportPatCmd("report_pattern", &fanMgr);
	
    // TODO read rc extraction file
    Cmd *readRCExtraceCmd = new ReadRCExtraceCmd("read_rc_extrace", &fanMgr);
    //----------------------------
    
    // TODO report read rc extraction file
    Cmd *reportRCExtraceCmd = new ReportRCExtraceCmd("report_rc_extrace", &fanMgr);
    //----------------------------
	
	// TODO new fault type fault tuple
    Cmd *addFaultCmd      = new AddFaultCmd("add_fault", &fanMgr);
	//--------------------------------------
	
    Cmd *reportFaultCmd   = new ReportFaultCmd("report_fault", &fanMgr);
    Cmd *addPinConsCmd    = new AddPinConsCmd("add_pin_constraint", &fanMgr);
    Cmd *runLogicSimCmd   = new RunLogicSimCmd("run_logic_sim", &fanMgr);
    Cmd *runFaultSimCmd   = new RunFaultSimCmd("run_fault_sim", &fanMgr);
    Cmd *runStaticLearnCmd= new RunStaticLearnCmd("run_static_learn", &fanMgr); 
    Cmd *runAtpgCmd       = new RunAtpgCmd("run_atpg", &fanMgr);
    Cmd *reportCircuitCmd = new ReportCircuitCmd("report_circuit", &fanMgr);
    Cmd *reportGateCmd    = new ReportGateCmd("report_gate", &fanMgr);
    Cmd *reportValueCmd   = new ReportValueCmd("report_value", &fanMgr);
    Cmd *reportStatsCmd   = new ReportStatsCmd("report_statistics", &fanMgr);
    Cmd *writeFaultCmd    = new WriteFaultCmd("write_fault", &fanMgr); 
    Cmd *writePatCmd      = new WritePatCmd("write_pattern", &fanMgr);
    Cmd *writeProcCmd     = new WriteProcCmd("write_test_procedure_file", &fanMgr);
    Cmd *addScanChainsCmd = new AddScanChainsCmd("add_scan_chains", &fanMgr);
    Cmd *reportScoapCmd   = new ReportScoapCmd("report_scoap", &fanMgr); 
    cmdMgr.regCmd("ATPG", readPatCmd);
    cmdMgr.regCmd("ATPG", reportPatCmd);
	
	// TODO read rc extraction file
    cmdMgr.regCmd("ATPG", readRCExtraceCmd);
    // ------------------------------------
    // TODO report rc extraction file
    cmdMgr.regCmd("ATPG", reportRCExtraceCmd);
    // -----------------------------------
	
    cmdMgr.regCmd("ATPG", addFaultCmd);
    cmdMgr.regCmd("ATPG", reportFaultCmd);
    cmdMgr.regCmd("ATPG", addPinConsCmd);
    cmdMgr.regCmd("ATPG", runLogicSimCmd);
    cmdMgr.regCmd("ATPG", runFaultSimCmd);
    cmdMgr.regCmd("ATPG", runStaticLearnCmd); 
    cmdMgr.regCmd("ATPG", runAtpgCmd);
    cmdMgr.regCmd("ATPG", reportCircuitCmd);
    cmdMgr.regCmd("ATPG", reportGateCmd);
    cmdMgr.regCmd("ATPG", reportValueCmd);
    cmdMgr.regCmd("ATPG", reportStatsCmd);
    cmdMgr.regCmd("ATPG", writeFaultCmd); 
    cmdMgr.regCmd("ATPG", writePatCmd);
    cmdMgr.regCmd("ATPG", writeProcCmd);
    cmdMgr.regCmd("ATPG", addScanChainsCmd);
    cmdMgr.regCmd("ATPG", reportScoapCmd); 

    // misc commands
    Cmd *reportPatFormatCmd = new ReportPatFormatCmd("report_pattern_format");
    Cmd *reportMemUsgCmd = new ReportMemUsgCmd("report_memory_usage");
    cmdMgr.regCmd("MISC", reportPatFormatCmd);
    cmdMgr.regCmd("MISC", reportMemUsgCmd);

    // user interface
    cmdMgr.setComment('#');
    cmdMgr.setPrompt("atpg> ");
    cmdMgr.setColor(CmdMgr::CYAN);
}

