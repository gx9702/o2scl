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
/* siman/siman.c
 * 
 * Copyright (C) 2007 Brian Gough
 * Copyright (C) 1996, 1997, 1998, 1999, 2000 Mark Galassi
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 
 * 02110-1301, USA.
 */
#ifndef O2SCL_ANNEAL_GSL_H
#define O2SCL_ANNEAL_GSL_H

/** \file anneal_gsl.h
    \brief File defining \ref o2scl::anneal_gsl
*/

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>

#include <o2scl/anneal.h>

#ifndef DOXYGEN_NO_O2NS
namespace o2scl {
#endif
  
  /** \brief Multidimensional minimization by simulated annealing (GSL)
      
      This class is a modification of simulated annealing as
      implemented in GSL in the function \c gsl_siman_solve(). It acts
      as a generic multidimensional minimizer for any function given a
      generic temperature schedule specified by the user.

      There are a large variety of strategies for choosing the
      temperature evolution. To offer the user the largest possible
      flexibility, the temperature evolution is controlled by a 
      the virtual functions start() and next() which can be freely
      changed by creating a child class which overwrites these 
      functions. 

      The simulated annealing algorithm proposes a displacement of one 
      coordinate of the previous point by
      \f[
      x_{i,\mathrm{new}} = \mathrm{step\_size}_i 
      (2 u_i - 1) + x_{i,\mathrm{old}}
      \f]
      where the \f$u_i\f$ are random numbers between 0 and 1. The
      displacement is accepted or rejected based on the Metropolis
      method. The random number generator is set in the parent,
      anneal.

      The default behavior is as follows: Initially, the step sizes
      are chosen to be 1.0 (or whatever was recently specified in \ref
      set_step() ) and the temperature to be \ref T_start (default
      1.0). Each iteration decreases the temperature by a factor of
      \ref T_dec (default 1.5) for each step, and the minimizer is
      finished when the next decrease would bring the temperature
      below multi_min::tol_abs. If none of the multi_min::ntrial steps in
      a particular iteration changes the value of the minimum, and the
      step sizes are greater than \ref min_step_ratio (default 100)
      times multi_min::tol_abs, then the step sizes are decreased by a
      factor of \ref step_dec (default 1.5) for the next iteration.

      If \ref mmin_base::verbose is greater than zero, then \ref
      mmin() will print out information and/or request a keypress
      after the function iterations for each temperature.
      
      An example demonstrating the usage of this class is given in
      <tt>examples/ex_anneal.cpp</tt> and in the \ref ex_anneal_sect .

      The form of the user-specified function is as in \ref
      multi_funct has a "function value" which is the value of the
      function (given in the third argument as a number of type \c
      double), and a "return value" (the integer return value). The
      initial function evaluation which is performed at the
      user-specified initial guess must give 0 as the return value. If
      subsequent function evaluations have a non-zero return value,
      then the resulting point is ignored and a new point is selected.

      This class thus can sometimes handle constrained minimization
      problems. If the user ensures that the function's return value
      is non-zero when the function is evaluated outside the allowed
      region, the minimizer will not accept any steps which take the
      minimizer outside the allowed region. Note that this should be
      done with care, however, as this approach may cause convergence
      problems with sufficiently difficult functions or constraints.

      See also a multi-threaded version of this class in \ref
      anneal_mt.

      \future There's x0, old_x, new_x, best_x, and x? There's probably
      some duplication here which could be avoided.

      \future 
      - Implement a more general simulated annealing routine
      which would allow the solution of discrete problems like the
      Traveling Salesman problem.
      \comment
      This could probably be done just by creating a parent abstract 
      base class which doesn't have any reference to step() and
      step_vec.
      \endcomment
      
  */
#ifdef O2SCL_CPP11
  template<class func_t=multi_funct11,
    class vec_t=boost::numeric::ublas::vector<double>,
    class rng_t=std::mt19937,
    class rng_dist_t=std::uniform_real_distribution<double> > 
    class anneal_gsl : public anneal_base<func_t,vec_t,rng_t,rng_dist_t>
#else
    template<class func_t=multi_funct<>, 
    class vec_t=boost::numeric::ublas::vector<double>,
    class rng_t=int,
    class rng_dist_t=rng_gsl > class anneal_gsl :
    public anneal_base<func_t,vec_t,rng_t,rng_dist_t>
#endif
    {

  public:
  
  typedef boost::numeric::ublas::vector<double> ubvector;
  
  anneal_gsl() {
    boltz=1.0;
    step_vec.resize(1);
    step_vec[0]=1.0;
    T_start=1.0;
    T_dec=1.5;
    step_dec=1.5;
    min_step_ratio=100.0;
  }

  virtual ~anneal_gsl() {
  }
      
  /** \brief Calculate the minimum \c fmin of \c func w.r.t the 
      array \c x0 of size \c nvar.
  */
  virtual int mmin(size_t nvar, vec_t &x0, double &fmin, 
		   func_t &func) {
    
    if (nvar==0) {
      O2SCL_ERR2_RET("Tried to minimize over zero variables ",
		     " in anneal_gsl::mmin().",exc_einval);
    }
    
    fmin=0.0;
    
    allocate(nvar);
    
    double E, new_E, best_E, T, old_E;
    int i, iter=0;
    size_t j;

    for(j=0;j<nvar;j++) {
      x[j]=x0[j];
      best_x[j]=x0[j];
    }
    
    E=func(nvar,x);
    best_E=E;

    // Setup initial temperature and step sizes
    start(nvar,T);

    bool done=false;

    while (!done) {

      // Copy old value of x for next() function
      for(j=0;j<nvar;j++) old_x[j]=x[j];
      old_E=E;
	  
      size_t nmoves=0;

      for (i=0;i<this->ntrial;++i) {
	for (j=0;j<nvar;j++) new_x[j]=x[j];
      
	step(new_x,nvar);
	new_E=func(nvar,new_x);
	
	// Store best value obtained so far
	if(new_E<=best_E){
	  for(j=0;j<nvar;j++) best_x[j]=new_x[j];
	  best_E=new_E;
	}
	
	// Take the crucial step: see if the new point is accepted
	// or not, as determined by the Boltzmann probability
	if (new_E<E) {
	  for(j=0;j<nvar;j++) x[j]=new_x[j];
	  E=new_E;
	  nmoves++;
	} else {
	  double r=this->rng_dist(this->rng);
	  if (r < exp(-(new_E-E)/(boltz*T))) {
	    for(j=0;j<nvar;j++) x[j]=new_x[j];
	    E=new_E;
	    nmoves++;
	  }
	}

      }
	  
      if (this->verbose>0) {
	this->print_iter(nvar,best_x,best_E,iter,T,"anneal_gsl");
	iter++;
      }
	  
      // See if we're finished and proceed to the next step
      next(nvar,old_x,old_E,x,E,T,nmoves,done);
      
    }
  
    for(j=0;j<nvar;j++) x0[j]=best_x[j];
    fmin=best_E;

    return 0;
  }
      
  /// Return string denoting type ("anneal_gsl")
  virtual const char *type() { return "anneal_gsl"; }

  /** \brief Boltzmann factor (default 1.0).
   */
  double boltz;
      
  /// Set the step sizes 
  template<class vec2_t> int set_step(size_t nv, vec2_t &stepv) {
    if (nv>0) {
      step_vec.resize(nv);
      for(size_t i=0;i<nv;i++) step_vec[i]=stepv[i];
    }
    return 0;
  }

  /// Initial temperature (default 1.0)
  double T_start;

  /// Factor to decrease temperature by (default 1.5)
  double T_dec;

  /// Factor to decrease step size by (default 1.5)
  double step_dec;

  /// Ratio between minimum step size and \ref tol_abs (default 100.0)
  double min_step_ratio;

#ifndef DOXYGEN_INTERNAL
      
  protected:
      
  /// \name Storage for present, next, and best vectors
  //@{
  ubvector x, new_x, best_x, old_x;
  //@}

  /// Vector of step sizes
  ubvector step_vec;
      
  /// Determine how to change the minimization for the next iteration
  virtual int next(size_t nvar, vec_t &x_old, double min_old, 
		   vec_t &x_new, double min_new, double &T, 
		   size_t n_moves, bool &finished) {
	
    if (T/T_dec<this->tol_abs) {
      finished=true;
      return success;
    }
    if (n_moves==0) {
      for(size_t i=0;i<nvar;i++) {
	if (i<step_vec.size() && step_vec[i]>this->tol_abs*min_step_ratio) {
	  step_vec[i]/=step_dec;
	}
      }
    }
    T/=T_dec;
    return success;
  }

  /// Setup initial temperature and stepsize
  virtual int start(size_t nvar, double &T) {
    T=T_start;
    return success;
  }

  /** \brief Allocate memory for a minimizer over \c n dimensions
      with stepsize \c step and Boltzmann factor \c boltz_factor 
  */
  virtual int allocate(size_t n, double boltz_factor=1.0) {
    x.resize(n);
    old_x.resize(n);
    new_x.resize(n);
    best_x.resize(n);
    boltz=boltz_factor;
    return 0;
  }

  /** \brief Make a step to a new attempted minimum
   */
  virtual int step(vec_t &sx, int nvar) {
    size_t nstep=step_vec.size();
    for(int i=0;i<nvar;i++) {
      double u=this->rng_dist(this->rng);
      sx[i]=(2.0*u-1.0)*step_vec[i%nstep]+sx[i];
    }
    return 0;
  }
  
#endif

  };

#ifndef DOXYGEN_NO_O2NS
}
#endif

#endif
