#ifndef STEP_H
#define STEP_H

#include <vector>
#include <string>
#include <cstdint>

#include "OpType.h"
#include "Role.h"

using namespace std;


struct Delta {
    enum class Kind { ValueChange, NodeCreate, NodeDelete, EdgeAdd, EdgeRemove, RoleChange };
    Kind kind;
    int  target_id;      
    int  new_value;      
    int  edge_to = -1;   
};

struct Step {
    uint64_t            seq;      
    OpType              op;        
    vector<Delta>       deltas;    
    vector<Role>        roles;     
    string              note;      
    float               progress;  
};

struct Timeline {
    string              algorithm; 
    vector<Step>        steps;
};

#endif