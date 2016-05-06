/*
 * lm27966.h - TI LM27966 LEDs Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __LM27966_H__
#define __LM27966_H__

struct device;

struct lm27966_platform_data {
	struct device *fbdev;
	unsigned int max_value;
	unsigned int def_value;
};

#endif
