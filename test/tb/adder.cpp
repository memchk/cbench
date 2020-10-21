#include "cbench.h"
#include "Vadder.h"

int main(int argc, char const *argv[])
{   
    Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);

    cbench::TestBench<Vadder> tb;
    auto &clk = tb.assign_clock(tb->clk, 1000);
    auto &clk_offset = tb.assign_clock(tb->clk_offset, 1000, 500);
    tb.open_trace("adder.vcd");

    for (auto a = 0; a < 256; a++)
    {
        for (auto b = 0; b < 256; b++)
        {
            tb->a = a;
            tb->b = b;
            tb.advance(clk, 1);
        }
    }
    

    tb.advance(clk, 1);
    return 0;
}
