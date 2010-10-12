/*
 * \brief  Utility functions
 * \author Norman Feske
 * \date   2009-03-09
 */

/*
 * Copyright (C) 2009 Norman Feske
 * Genode Labs, Feske & Helmuth Systementwicklung GbR
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef __UTIL_H
#define __UTIL_H

#include <dopelib.h>
#include <stdlib.h>

/**
 * Request long attribute from widget
 *
 * This is a convenience function for an often used special case of
 * calling 'dope_req'.
 */
static inline long dope_req_l(int app_id, char *cmd)
{
	char buf[16];
	dope_req(app_id, buf, 16, cmd);
	return atol(buf);
}


#define HEADER_PIXEL(data,pixel) {\
  pixel[0] = (((data[0] - 33) << 2) | ((data[1] - 33) >> 4)); \
  pixel[1] = ((((data[1] - 33) & 0xF) << 4) | ((data[2] - 33) >> 2)); \
  pixel[2] = ((((data[2] - 33) & 0x3) << 6) | ((data[3] - 33))); \
  data += 4; \
}

#endif /* __UTIL_H */
