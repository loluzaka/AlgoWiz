#ifndef OPTYPE_H
#define OPTYPE_H

enum class OpType {
    Compare, Swap, Assign,
    Insert, Delete,
    NodeCreate, NodeDelete,
    EdgeAdd, EdgeRemove,
    Mark, Unmark,
    Highlight,
    Note,
    // ---- appended in the contract amendment (original order untouched) ----
    Access,        // a single read of one entity (search inspected a cell)
    PointerMove,   // a labelled pointer (i, j, mid, HEAD, TOP, FRONT ...) moved
    VisitNode,     // traversal visited a node / vertex
    VisitEdge,     // traversal explored an edge
    Rotate,        // AVL rotation (LL, RR, LR, RL)
    Hash,          // a hash function was evaluated
    Collision,     // a hash slot was already occupied
    Probe,         // one probe of an open-addressing sequence
    Heapify,       // a heapify / sift step
    Partition,     // quick sort / quick select partition boundary
    Merge,         // merge-sort merge of two runs
    Iterate        // start of a new pass / iteration / digit
};

#endif
