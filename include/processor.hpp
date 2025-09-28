#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

class Processor {
    public:
        float Utilization();
    private:
        long prev_total_{0};
        long prev_idle_{0};
};

#endif