#ifndef CBENCH_CBENCH_H
#define CBENCH_CBENCH_H

#include <algorithm>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <iostream>

#include "verilated.h"
#ifdef TRACE_FST
#include "verilated_fst_c.h"
#else
#include "verilated_vcd_c.h"
#endif
// Internal Macros
#define CBENCH_UNCOPYABLE(X) X &operator=(const X &other) = delete; \
                             X(const X &) = delete;

namespace cbench
{   
    // Private helper functions.
    namespace
    {
        template <typename EventNotifier, typename... Args>
        bool or_notifier(EventNotifier &&arg, Args&&... rest)
        {
            if constexpr (sizeof...(rest) > 0)
            {
                return std::forward<EventNotifier>(arg)() || or_notifier(std::forward<Args>(rest)...);
            }
            else
            {
                return std::forward<EventNotifier>(arg)();
            }
        }
    } // namespace

    class Peripherial
    {
        template <typename Friend>
        friend class TestBench;

    private:
        virtual vluint64_t time_to_event() const = 0;
        virtual bool advance(vluint64_t delta_time) = 0;
    };

    class Clock final : public Peripherial
    {

        CData &clk_;
        vluint64_t increment_, now_, last_cycle_;
        vluint64_t ticks_;

        bool advance(vluint64_t delta_time) override
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

        vluint64_t time_to_event() const override
        {
            assert(last_cycle_ <= now_);

            if (now_ < last_cycle_ + increment_)
            {
                // next cycle is a falling edge.
                return (last_cycle_ + increment_) - now_;
            }
            return (last_cycle_ + 2 * increment_) - now_;
        }

    public:
        Clock(CData &clk, vluint64_t interval, vluint64_t offset) : clk_(clk), increment_(interval / 2), now_(offset), last_cycle_(0), ticks_(0) {}
        CBENCH_UNCOPYABLE(Clock);

        bool rising_edge() const
        {
            return now_ == last_cycle_;
        }

        bool falling_edge() const
        {
            return now_ == last_cycle_ + increment_;
        }

        auto rising_edges(vluint64_t n) const
        {      
            auto goal = last_cycle_ + (2* n * increment_);
            return [this, goal]() {
                return now_ >= goal;
            };
        }

        auto falling_edges(vluint64_t n) const
        {
            auto goal = last_cycle_ + increment_ + (2* n * increment_);
            return [this, goal]() {
                return now_ >= goal;
            };
        }
    };

    class SPIMaster final : public Peripherial
    {
        CData &sclk_, &miso_, &mosi_, &cs_;
    };

    template <typename M>
    class TestBench final
    {
        std::vector<Peripherial *> peripherials_;
        std::unique_ptr<VerilatedVcdC> trace_;
        vluint64_t global_time_;
        std::unique_ptr<M> core;

    public:
        TestBench() : core(std::make_unique<M>()), global_time_(0) {}

        // Do not allow copying of TestBench.
        CBENCH_UNCOPYABLE(TestBench);

        template<typename P>
        void attach(P &p)
        {
            peripherials_.push_back(&p);
        }

        void advance()
        {   
            // Find the minimum time between any registered periperials to advance time.
            auto min_time = (*std::min_element(peripherials_.begin(), peripherials_.end(), [](auto &a, auto &b) { return a->time_to_event() < b->time_to_event(); }))->time_to_event();

            // Time must advance at least 1 simulation quanta.
            assert(min_time > 0);;
            std::cout << "MT: " << min_time << std::endl;
            core->eval();

            if (trace_)
            {
                // Add one to the time quanta, so we dump pre and post edge.
                trace_->dump(global_time_ + 1);
            }

            for (auto &p : peripherials_)
            {
                p->advance(min_time);
            }

            global_time_ += min_time;
            core->eval();

            if (trace_)
            {
                trace_->dump(global_time_);
            }
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

    template<typename M, typename EventNotifier>
    TestBench<M>& operator>>(TestBench<M> &tb, EventNotifier notif)
    {
        while(!notif())
        {
            tb.advance();
        }

        return tb;
    }


    template <typename... Args>
    auto any(Args&&... rest)
    {   
        return [&]()
        {
            return or_notifier(std::forward<Args>(rest)...);
        };
    }

    template <typename Pin>
    auto value(Pin &pin, Pin value)
    {
        return [&]()
        {
            return pin == value;
        };
    }

    template <typename Pin>
    auto high(Pin &pin)
    {
        return [&]()
        {
            return pin > 0;
        };
    }

    template <typename Pin>
    auto low(Pin &pin)
    {
        return [&]()
        {
            return pin == 0;
        };
    }

} // namespace cbench


#undef CBENCH_UNCOPYABLE
#endif