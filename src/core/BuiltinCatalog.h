#ifndef BUILTINCATALOG_H
#define BUILTINCATALOG_H

// One place that knows every built-in module. Adding a new algorithm family means
// adding one include and one registerXxx() call here.

#include "AlgorithmCatalog.h"
#include "graphs/Graph.h"
#include "hashing/HashTable.h"
#include "heaps/Heap.h"
#include "linear/Queue.h"
#include "linear/Stack.h"
#include "lists/ArrayStructure.h"
#include "lists/LinkedList.h"
#include "searching/Searches.h"
#include "sorting/DistributionSorts.h"
#include "strings/StringSorts.h"
#include "trees/AVLTree.h"
#include "trees/BST.h"
#include "trees/BinaryTree.h"

// Category order == order of the home menu (items 1-10).
inline const vector<string>& menuCategories() {
    static const vector<string> c = {"Arrays", "Searching", "Sorting", "Linked Lists", "Stack", "Queue", "Hashing", "Trees", "Heaps", "Graphs"};
    return c;
}

inline AlgorithmCatalog buildBuiltinCatalog() {
    AlgorithmCatalog cat;
    registerArrays(cat);
    registerSearching(cat);
    registerSorting(cat);
    registerStrings(cat);
    registerLists(cat);
    registerStack(cat);
    registerQueue(cat);
    registerHashing(cat);
    registerBinaryTree(cat);
    registerBst(cat);
    registerAvl(cat);
    registerHeaps(cat);
    registerGraphs(cat);
    return cat;
}

#endif
