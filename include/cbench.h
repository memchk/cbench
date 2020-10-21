#ifndef CBENCH_CBENCH_H
#define CBENCH_CBENCH_H

#include <algorithm>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <iostream>

#include "verilated.h"
#include "verilated_vcd_c.h"

namespace cbench
{

    // Private helper functions.
    namespace
    {   
        template <typename C, typename Arg, typename... Args>
        bool scan_args(C comparand, const Arg &arg, const Args &... rest)
        {
            if constexpr (sizeof...(rest) > 0)
            {
                return (arg == comparand) || scan_args(rest..., comparand);
            }
            else
            {
                return arg == comparand;
            }
        }
    } // namespace

    class Clock
    {

        template<typename Friend>
        friend class TestBench;

        CData &clk_;
        vluint64_t increment_, now_, last_cycle_;
        vluint64_t ticks_;

        bool advance(vluint64_t delta_time)
        {
            // Throw if this would cause a clock to skip.
            assert(delta_time <= increment_);

            now_ += delta_time;

            if (now_ >= last_cycle_ + 2 * increment_)
            {
                // Next positive edge, set positive valued clock.
                last_cycle_ += 2 * increment_;
                ticks_++;
                clk_ = 1;
                return true;
            }
            else if (now_ >= last_cycle_ + increment_)
            {
                // On the negative half-cycle.
                clk_ = 0;
                return false;
            }
            else
            {
                // On the positive half-cycle.
                clk_ = 1;
                return true;
            }
        }

    public:
        Clock(CData &clk, vluint64_t interval, vluint64_t offset) : clk_(clk), increment_(interval / 2), last_cycle_(0), ticks_(0), now_(offset)
        {
            // Start the clock on logic low.
            clk_ = 0;
        };

        // Do not allow copying of clocks.
        Clock &operator=(const Clock &other) = delete;
        Clock(const Clock &) = delete;

        vluint64_t time_to_edge() const
        {
            assert(last_cycle_ <= now_);

            if (now_ < last_cycle_ + increment_)
            {
                // next cycle is a falling edge.
                return (last_cycle_ + increment_) - now_;
            }

            // if (now_ < last_cycle_ + 2*increment_)
            return (last_cycle_ + 2 * increment_) - now_;
        }

        bool rising_edge() const
        {
            return now_ == last_cycle_;
        }

        bool falling_edge() const
        {
            return now_ == last_cycle_ + increment_;
        }

        vluint64_t ticks() const
        {
            return ticks_;
        }
    };

    template <typename M>
    class TestBench final
    {
        std::vector<std::unique_ptr<Clock>> clocks_;
        std::unique_ptr<VerilatedVcdC> trace_;
        vluint64_t global_time_;
        std::unique_ptr<M> core;

    public:
        TestBench() : core(std::make_unique<M>()), global_time_(0) {}

        // Do not allow copying of TestBench.
        TestBench &operator=(const TestBench &other) = delete;
        TestBench(const TestBench &) = delete;

        Clock &assign_clock(CData &clk, vluint64_t interval, vluint64_t offset = 0)
        {
            // Force clocks to be at least 4 time quanta big, prevents issues with trace dumping.
            assert(interval >= 4);

            clocks_.push_back(std::make_unique<Clock>(clk, interval, offset));
            return *clocks_.back();
        }

        void advance()
        {
            auto min_time = (*std::min_element(clocks_.begin(), clocks_.end(), [](auto &a, auto &b) { return a->time_to_edge() < b->time_to_edge(); }))->time_to_edge();

            // Time must advance at least 1 simulation quanta.
            assert(min_time > 0);

            core->eval();

            if (trace_)
            {
                // Add one to the time quanta, so we dump pre and post edge.
                trace_->dump(global_time_ + 1);
            }

            for (auto &clock : clocks_)
            {
                clock->advance(min_time);
            }

            global_time_ += min_time;
            core->eval();

            if (trace_)
            {
                trace_->dump(global_time_);
            }
        }

        void advance(const Clock &pace_clk, vluint64_t ticks)
        {
            auto current_ticks = pace_clk.ticks();
            while (pace_clk.ticks() < (current_ticks + ticks))
            {
                advance();
            }

            assert(pace_clk.rising_edge());
        }

        void open_trace(const std::string &filename, int depth = 99)
        {
            if (!trace_)
            {
                trace_ = std::make_unique<VerilatedVcdC>();
                core->trace(&*trace_, depth);
                trace_->open(filename.c_str());
                if (!trace_->isOpen())
                {
                    trace_.reset();
                }
            }
        }

        void flush()
        {
            if (trace_)
            {
                trace_->flush();
            }
        }

        template <typename C, typename... Args>
        void wait_for(const Clock &clk, C comparand, const Args &... rest)
        {
            do
            {
                do
                {
                    advance();
                } while (!clk.rising_edge());
            } while (!scan_args(comparand, rest...));
        }

        M *operator->()
        {
            return core.get();
        }

        ~TestBench()
        {
            if (trace_)
            {
                trace_->flush();
            }
        }
    };
} // namespace cbench
#endif