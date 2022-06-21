/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _MATH_H
#define _MATH_H

/* borrowed from Linux */

#define roundup(x, y) (					\
{							\
	typeof(y) __y = y;				\
	(((x) + (__y - 1)) / __y) * __y;		\
}							\
)

#define rounddown(x, y) (				\
{							\
	typeof(x) __x = (x);				\
	__x - (__x % (y));				\
}							\
)

#endif /* _MATH_H */
