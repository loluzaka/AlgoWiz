#ifndef ROLE_H
#define ROLE_H

enum class Role {
    Default, Compared, Sorted, Pivot, Found,
    Active, Current, Candidate,
    Visited, Queued, Frontier,
    Path, Error
};

#endif 