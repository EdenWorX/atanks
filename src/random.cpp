//
// Created by sed on 25.07.23.
//

#include "random.h"

#include <random>


/** @brief get a random number between 0 and @arg u
 *
 * Noon-extreme values are preferred, based on a simple cubic function.
 *
 * @param[in] u The upper limit
 * @return a random number between 0 and @arg u
**/
double central_rand( double u ) {
	double const x = static_cast< double >( get_rand() ) / static_cast< double >( INT32_MAX ) - 0.5; // [-.5,+.5]
	return u * ( 0.5 - ( x * x * x ) * 4.0 );
}


/** @brief get a full range random number, substituting srand()/rand()
 *
 * @return a random number between 0 and INT_MAX
**/
int32_t get_rand() {
	static thread_local std::random_device rand_dev{};
	static thread_local std::minstd_rand   rand_gen{ rand_dev() };

	return static_cast< int32_t >( rand_gen() );
}
