#include "cbench.h"
#include "Vadder.h"

int main(int argc, char const *argv[])
{   
    Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);

    cbench::TestBench<Vadder> tb;
    cbench::Clock clk(tb->clk, 1000, 0);
    cbench::Clock clk_offset(tb->clk_offset, 500, 0);

    tb.attach(clk);
    tb.attach(clk_offset);

    tb.open_trace("adder.vcd");

    for (auto a = 0; a < 255; a++)
    {
        for (auto b = 0; b < 255; b++)
        {
            tb->a = a;
            tb->b = b;
            tb >> clk.rising_edges(1) >> clk_offset.rising_edges(1);
        }
    }

    return 0;
}
