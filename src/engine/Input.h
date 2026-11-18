#ifndef INPUT_H
#define INPUT_H

#include <string>
#include <vector>
#include <cstddef>

using namespace std;

// One user-visible operation on a data structure ("insert_end 40", "add_edge 0 1 5" ...).
struct Command {
    string op;
    int    a = 0, b = 0, c = 0;
    string s;
};

enum class StringMode { Lexicographic, LexicographicReverse, Length, LengthReverse };

// Everything an algorithm may need. Fields that a given algorithm does not use are ignored.
struct Input {
    vector<int>     values;               // array contents / initial structure contents
    vector<string>  words;                // string sorting
    bool            hasTarget = false;
    int             target = 0;           // search target
    bool            descending = false;   // sorting direction
    StringMode      stringMode = StringMode::Lexicographic;
    bool            caseInsensitive = false;
    int             capacity = 8;         // stack / queue slots
    int             tableSize = 7;        // hash table slots
    bool            directed = false;     // graphs
    int             vertexCount = 0;      // graphs: initial vertices 0..n-1
    vector<Command> commands;             // history of structure operations
    size_t          visibleFrom = 0;      // commands before this index run silently (they only build state)
};

#endif
