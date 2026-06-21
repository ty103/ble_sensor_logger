#include "platform_time.h"

#include <zephyr/kernel.h>

uint32_t platform_uptime_ms(void)
{
	return (uint32_t)k_uptime_get_32();
}
