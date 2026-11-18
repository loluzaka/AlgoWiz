#ifndef ROLE_H
#define ROLE_H

enum class Role {
    Default, Compared, Sorted, Pivot, Found,
    Active, Current, Candidate,
    Visited, Queued, Frontier,
    Path, Error,
    // ---- appended in the contract amendment (original order untouched) ----
    Moved,         // value just swapped / shifted / written (badge [SWP])
    Collision,     // hash slot that caused a collision
    Eliminated,    // part of the search range that was discarded
    Range,         // part of the live search range / partition boundary
    NotFound,      // a search that failed
    GroupA,        // left run / left partition / first merge group
    GroupB         // right run / right partition / second merge group
};

#endif
