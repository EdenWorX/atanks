//
// Created by sed on 25.07.23.
//

#include "random.h"

#include <random>

/// @brief substitute for get_rand()
int32_t get_rand() {
	static thread_local std::random_device rand_dev{};
	static thread_local std::minstd_rand   rand_gen{ rand_dev() };

	return static_cast< int32_t >( rand_gen() );
}
