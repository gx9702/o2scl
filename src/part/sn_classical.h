/*
  -------------------------------------------------------------------
  
  Copyright (C) 2006-2014, Andrew W. Steiner
  
  This file is part of O2scl.
  
  O2scl is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.
  
  O2scl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with O2scl. If not, see <http://www.gnu.org/licenses/>.

  -------------------------------------------------------------------
*/
#ifndef O2SCL_SN_CLASSICAL_H
#define O2SCL_SN_CLASSICAL_H

#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <o2scl/constants.h>

#include <o2scl/part_deriv.h>

#ifndef DOXYGEN_NO_O2NS
namespace o2scl {
#endif

  /** \brief Equation of state for a classical particle with derivatives

      \todo This does not work with inc_rest_mass=true
  */
  class sn_classical {

  public:

    sn_classical();

    virtual ~sn_classical();
    
    /** \brief Compute the properties of particle \c p at temperature 
	\c temper from its chemical potential
    */
    virtual void calc_mu(part_deriv &p, double temper);
    
    /** \brief Compute the properties of particle \c p at temperature 
	\c temper from its density
    */
    virtual void calc_density(part_deriv &p, double temper);

    /// Return string denoting type ("sn_classical")
    virtual const char *type() { return "sn_classical"; };

  };

#ifndef DOXYGEN_NO_O2NS
}
#endif

#endif
