// **************************************************************************
// File       [ rcextrace_file.h ]
// Author     [ fyliu ]
// Synopsis   [ ]
// Date       [ 2016/11/01 created ]
// **************************************************************************

#ifndef _INTF_RCEXTRACE_FILE_H_
#define _INTF_RCEXTRACE_FILE_H_

#include "global.h"

namespace IntfNs {

struct RCExtraceNetNames {
    char     name[NAME_LEN];
    RCExtraceNetNames *next;
    RCExtraceNetNames *head;
};

class RCExtraceFile {
public:
                 RCExtraceFile();
    virtual      ~RCExtraceFile();

    virtual bool read(const char * const fname, const bool &verbose = false);
    virtual void setPiOrder(const RCExtraceNetNames * const pis);
    virtual void setPoOrder(const RCExtraceNetNames * const pos);
//    virtual void setPatternNum(const int &num);
    virtual void addPattern(const char * const pi1, const char * const pi2,
                            const char * const ppi, const char * const si,
                            const char * const po1, const char * const po2,
                            const char * const ppo);

protected:
    bool success_;
    bool verbose_;
};

inline RCExtraceFile::RCExtraceFile() {
    success_ = true;
    verbose_ = false;
}
inline RCExtraceFile::~RCExtraceFile() {}

};

#endif
