#ifndef ATANKS_RANDOM_H
#define ATANKS_RANDOM_H 1
//
// Created by sed on 25.07.23.
//

#include <cstdint>


int32_t get_rand();

// A few often needed randomization shortcuts
#define RAND_AI_0P ( get_rand() % ai_level )
#define RAND_AI_0N ( 0 == ( get_rand() % ai_level ) )
#define RAND_AI_1P ( get_rand() % ( ai_level + 1 ) )
#define RAND_AI_1N ( 0 == ( get_rand() % ( ai_level + 1 ) ) )


#endif // ATANKS_RANDOM_H
