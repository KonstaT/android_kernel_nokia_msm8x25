#undef TRACE_SYSTEM
#define TRACE_SYSTEM cpu_hotplug

#if !defined(_TRACE_EVENT_CPU_HOTPLUG_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_EVENT_CPU_HOTPLUG_H

#include <linux/tracepoint.h>

TRACE_EVENT(cpu_online,
	TP_PROTO(unsigned int on),

	TP_ARGS(on),

	TP_STRUCT__entry(
		__field(	unsigned int,	on)
	),

	TP_fast_assign(
		__entry->on	= on;
	),

	TP_printk("turn cpu %s", __entry->on ? "on" : "off")
);

#endif

/* This part must be outside protection */
#include <trace/define_trace.h>

