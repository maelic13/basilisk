#pragma once

// Without arguments, the UCI loop on stdin. With arguments, they are one
// command, run to its end; the return value is the exit status.
int run_engine(int argc, char* argv[]);
