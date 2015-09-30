#ifndef UTIL_H
#define UTIL_H

#define badd(_d,_i,_lb,_ub)                                    \
do {                                                           \
	*(_d) = min(max(*(_d) + (_i), (_lb)), (_ub));              \
} while (0)

#define wadd(_d,_i,_lb,_ub)                                    \
do {                                                           \
	intmax_t __tmp = *(_d) + (_i);                             \
	*(_d) = ((__tmp > (_ub))                                   \
			? (_lb)                                            \
			: ((__tmp < (_lb))                                 \
				? (_ub)                                        \
				: __tmp));                                     \
} while (0)

#endif