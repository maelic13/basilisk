// Origin: this file descends from BBC (Bit Board Chess) by Maksim Korzh, https://github.com/maksimKorzh/bbc,
// released under the GNU General Public License v3; the project started from BBC in 2024 and has since rewritten
// and extended much of it. Triumviratus is distributed under the GNU GPL v3 (see COPYING).
#pragma once
#ifndef PERFT_H
#define PERFT_H

#include "defs.h"
#include "movegen.h"
#include "misc.h"

extern U64 nodes;

extern void perft_driver(int depth);
extern void perft_test(int depth);

#endif
