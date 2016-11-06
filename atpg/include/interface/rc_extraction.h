// **************************************************************************
// File       [ rc_extraction.h ]
// Author     [ fyliu ]
// Synopsis   [ define the neighborhoods by rc extrace ]
// Date       [ 2016/10/30 created ]
// **************************************************************************

#ifndef _INTF_RCEXTRACE_H_
#define _INTF_RCEXTRACE_H_

#include "cell.h"
#include "techlib.h"

namespace IntfNs {

class RCExtrace {

public:

            RCExtrace();
            ~RCExtrace();

    // info
    bool    check(const bool &verbose = false) const;
    bool    setTechlib(Techlib * const lib);
    Techlib *getTechlib() const;

    // top module
    Cell    *getTop() const;
    bool    setTop(const size_t &i);
    bool    setTop(const char * const name);

    // modules
    bool    addModule(Cell *m);
    size_t  getNModule() const;
    Cell    *getModule(const char * const name) const;
    Cell    *getModule(const size_t &i) const;

    // operations
    bool    removeFloatingNets();
    bool    levelize();

private:
    Cell    *top_;
    CellVec modules_;
    CellMap nameToModule_;
    Techlib *lib_;
    int     lvl_;
};


// inline methods
inline RCExtrace::RCExtrace() {
    top_ = NULL;
    lib_ = NULL;
    lvl_ = -1;
}

inline RCExtrace::~RCExtrace() {
}

inline bool RCExtrace::setTechlib(Techlib * const lib) {
    lib_ = lib;
    return true;
}

inline Techlib *RCExtrace::getTechlib() const {
    return lib_;
}

inline Cell *RCExtrace::getTop() const {
    return top_;
}

inline bool RCExtrace::setTop(const char * const name) {
    CellMap::const_iterator it = nameToModule_.find(name);
    if (it != nameToModule_.end()) {
        top_ = it->second;
        lvl_ = -1;
    }
    return it != nameToModule_.end();
}

inline bool RCExtrace::setTop(const size_t &i) {
    if (i < modules_.size()) {
        top_ = modules_[i];
        lvl_ = -1;
    }
    return i < modules_.size();
}

inline bool RCExtrace::addModule(Cell *m) {
    CellMap::iterator it = nameToModule_.find(m->name_);
    if (it != nameToModule_.end())
        return false;
    modules_.push_back(m);
    nameToModule_[m->name_] = m;
    return true;
}

inline size_t RCExtrace::getNModule() const {
    return modules_.size();
}

inline Cell *RCExtrace::getModule(const char * const name) const {
    CellMap::const_iterator it = nameToModule_.find(name);
    return it == nameToModule_.end() ? NULL : it->second;
}

inline Cell *RCExtrace::getModule(const size_t &i) const {
    return i < modules_.size() ? modules_[i] : NULL;
}

};

#endif

