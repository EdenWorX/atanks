#ifndef ATANKS_SRC_SATELLITE_H_INCLUDED
#define ATANKS_SRC_SATELLITE_H_INCLUDED 1


#include "environment.h"
#include "globaldata.h"

#define SATELLITE_IMAGE 16

#ifndef BEAM_DEFINE
class BEAM;
#endif // BEAM_DEFINE

class SATELLITE {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	explicit SATELLITE();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void draw() const;
	void move();
	void shoot();


private:
	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	BEAM*   beam   = nullptr;
	int32_t x      = 0;
	int32_t y      = MENUHEIGHT + 5;
	int32_t xv     = -2;
	int32_t prev_x = 0;
};

#endif // ATANKS_SRC_SATELLITE_H_INCLUDED
