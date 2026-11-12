#pragma once

// The node type each recursive call of the kernel passes, logged for the test
// that pins it. Only the test library defines BASILISK_CHILD_LOG; in every
// other build the hooks are empty and the kernel is unchanged.
//
// Before each call the caller names its site and its own type; the callee,
// on entry, records the type it received under that site. A negamax entry no
// site named is counted apart, so a new call site cannot go unpinned.

#ifdef BASILISK_CHILD_LOG

#include <cstdint>

enum class ChildSite {
    Root,             // the driver's call of the root
    NullMove,         // the null move's child
    NullVerify,       // the null move's verification at the node's own ply
    ProbCutQs,        // ProbCut's quiescence search
    ProbCutSearch,    // ProbCut's reduced search
    Exclusion,        // the singular search with the TT move excluded
    FirstMove,        // the first move's child
    Reduced,          // a later move's reduced search
    ReducedResearch,  // the full-depth re-search of a reduced move that failed high
    ZeroWindow,       // a later move's unreduced null-window search
    PvResearch,       // the full-window re-search at a PV node
    Count
};

struct ChildLog {
    // calls[site][parent_pv][parent_cut][child_pv][child_cut]
    uint64_t calls[int(ChildSite::Count)][2][2][2][2] = {};
    uint64_t unnamed = 0;
    bool pending = false;
    ChildSite site = ChildSite::Root;
    bool parent_pv = false;
    bool parent_cut = false;

    void note(ChildSite s, bool pv, bool cut) {
        pending = true;
        site = s;
        parent_pv = pv;
        parent_cut = cut;
    }
    // A negamax entry: every one is named.
    void received(bool pv, bool cut) {
        if (!pending) {
            ++unnamed;
            return;
        }
        pending = false;
        ++calls[int(site)][parent_pv][parent_cut][pv][cut];
    }
    // A quiescence entry: named only where the main search calls it as a
    // site of its own; it has no cut flag.
    void received_qs(bool pv) {
        if (!pending)
            return;
        pending = false;
        ++calls[int(site)][parent_pv][parent_cut][pv][0];
    }
};

// This thread's log.
ChildLog& child_log();

#define CHILD_SITE(s, pv, cut) child_log().note(ChildSite::s, (pv), (cut))
#define CHILD_RECEIVED(pv, cut) child_log().received((pv), (cut))
#define CHILD_RECEIVED_QS(pv) child_log().received_qs(pv)

#else

#define CHILD_SITE(s, pv, cut) ((void)0)
#define CHILD_RECEIVED(pv, cut) ((void)0)
#define CHILD_RECEIVED_QS(pv) ((void)0)

#endif
