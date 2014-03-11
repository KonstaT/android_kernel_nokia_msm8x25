#undef TRACE_SYSTEM
#define TRACE_SYSTEM cpufreq

#if !defined(_TRACE_EVENT_CPUFREQ_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_EVENT_CPUFREQ_H

#include <linux/tracepoint.h>

TRACE_EVENT(cpufreq_chg,
	TP_PROTO(unsigned int cpu, unsigned int freq),

	TP_ARGS(cpu, freq),

	TP_STRUCT__entry(
		__field(	unsigned int,	cpu)
		__field(	unsigned int,	freq)
	),

	TP_fast_assign(
		__entry->cpu	= cpu;
		__entry->freq	= freq;
	),

	TP_printk("cpu %d is at %d HZ", __entry->cpu, __entry->freq)
);

#endif

/* This part must be outside protection */
#include <trace/define_trace.h>

