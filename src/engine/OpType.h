#ifndef OPTYPE_H
#define OPTYPE_H

enum class OpType {
    Compare, Swap, Assign,
    Insert, Delete,                 
    NodeCreate, NodeDelete,        
    EdgeAdd, EdgeRemove,            
    Mark, Unmark,                   
    Highlight,                      
    Note                           
};

#endif 