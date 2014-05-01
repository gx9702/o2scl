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
#ifndef O2SCL_HADRONIC_EOS_H
#define O2SCL_HADRONIC_EOS_H

#include <iostream>
#include <string>

#include <boost/numeric/ublas/vector.hpp>

#include <o2scl/deriv_gsl.h>
#include <o2scl/mroot.h>
#include <o2scl/mroot_hybrids.h>
#include <o2scl/mm_funct.h>
#include <o2scl/eos_base.h>
#include <o2scl/fermion_eff.h>
#include <o2scl/part.h>

#ifndef DOXYGEN_NO_O2NS
namespace o2scl {
#endif

  /** \brief Hadronic equation of state [abstract base]

      Denote the number density of neutrons as \f$ n_n \f$, the number
      density of protons as \f$ n_p \f$, the total baryon density \f$
      n_B = n_n + n_p \f$, the asymmetry \f$ \alpha \equiv
      (n_n-n_p)/n_B \f$, the nuclear saturation density as \f$ n_0
      \approx 0.16~\mathrm{fm}^{-3} \f$, and the quantity \f$ \epsilon
      \equiv (n_B-n_0)/3n_0 \f$. (Note that some authors define
      \f$ \alpha \f$ as \f$ n_n - n_p \f$, which is not the same as
      the definition above.) Then the energy per baryon of nucleonic
      matter can be written as an expansion around 
      \f$ \epsilon =\alpha = 0 \f$
      \f[
      E(n_B,\alpha) = -B + \frac{\tilde{K}}{2!} {\epsilon}^2 + 
      \frac{Q}{3!} {\epsilon}^3 + \alpha^2 \left(S + L \epsilon + 
      \frac{K_{\mathrm{sym}}}{2!} {\epsilon}^2
      + \frac{Q_{\mathrm{sym}}}{3!} \epsilon^3 \right) + 
      E_4(n_B,\alpha) + {\cal O}(\alpha^6)
      \qquad \left(\mathrm{Eq.}~1\right)
      \f]
      where \f$ E_4 \f$ represents the quartic terms
      \f[
      E_4(n_B,\alpha) 
      = \alpha^4 \left(S_4 + L_4 \epsilon + \frac{K_4}{2!} {\epsilon}^2
      + \frac{Q_4}{3!} \epsilon^3 \right) \qquad
      \left(\mathrm{Eq.}~2\right)
      \f]
      (Adapted slightly from \ref Piekarewicz09). From this, one can
      compute the energy density of nuclear matter \f$
      \varepsilon(n_B,\alpha) = n_B E(n_B,\alpha) \f$, the chemical
      potentials \f$ \mu_i \equiv (\partial \varepsilon) / (\partial
      n_i ) \f$ and the pressure \f$ P = -\varepsilon + \mu_n n_n +
      \mu_p n_p \f$. This expansion motivates the definition of
      several separate terms. The binding energy \f$ B \f$ of
      symmetric nuclear matter (\f$ \alpha = 0 \f$) is around 16 MeV.

      The compression modulus is usually defined by \f$ \chi = -1/V
      (dV/dP) = 1/n (dP/dn)^{-1} \f$ . In nuclear physics it has
      become common to use the incompressibility (or bulk) modulus
      with an extra factor of 9 (motivated by the 3 in the denominator
      in the definition of \f$ \epsilon \f$), \f$ K=9/(n \chi) \f$ and
      refer to \f$ K \f$ simply as the incompressibility. Here, we
      define the function
      \f[
      K(n_B,\alpha) \equiv 9 \left( \frac{\partial P}{\partial n_B} 
      \right) = 9 n_B \left(\frac{\partial^2 \varepsilon}
      {\partial n_B^2}\right)
      \f]
      This quantity is computed by the function \ref fcomp() by
      computing the first derivative of the pressure, which is more
      numerically stable than the second derivative of the energy
      density (since most \o2 EOSs compute the pressure exactly). This
      function is typically evaluated at the point \f$
      (n_B=n_0,\alpha=0) \f$ and is stored in \ref comp by the
      function \ref saturation(). This quantity is not always the same
      as \f$ \tilde{K} \f$, defined here as
      \f[
      \tilde{K}(n_B,\alpha) = 9 n_B^2 \left(\frac{\partial^2 E}{\partial 
      n_B^2}\right) = K(n_B,\alpha) - \frac{1}{n_B} 18 P(n_B,\alpha)
      \f]
      We denote \f$ K \equiv K(n_B=n_0,\alpha=0) \f$ and similarly for
      \f$ \tilde{K} \f$, the quantity in Eq. 1 above. In nuclear
      matter at saturation, the pressure is zero and \f$ K = \tilde{K}
      \f$. See \ref Chabanat97 for further discussion of the distinction
      between \f$ K \f$ and \f$ \tilde{K} \f$.

      The symmetry energy, \f$ S(n_B,\alpha), \f$ can be defined as
      \f[
      S(n_B,\alpha) \equiv \frac{1}{2 n_B}\frac{\partial^2 \varepsilon}
      {\partial \alpha^2}
      \f]
      and the parameter \f$ S \f$ in Eq. 1 is just \f$ S(n_0,0) \f$. 
      Using
      \f[
      \left(\frac{\partial \varepsilon}{\partial \alpha}\right)_{n_B} = 
      \frac{\partial \varepsilon}{\partial n_n} 
      \left(\frac{\partial n_n}{\partial \alpha}\right)_{n_B} +
      \frac{\partial \varepsilon}{\partial n_p} 
      \left(\frac{\partial n_p}{\partial \alpha}\right)_{n_B} 
      = \frac{n_B}{2} \left(\mu_n - \mu_p \right)
      \f]
      this can be rewritten 
      \f[
      S(n_B,\alpha) = \frac{1}{4} \frac{\partial}{\partial \alpha}
      \left(\mu_n - \mu_p\right)
      \f]
      where the dependence of the chemical potentials on \f$ n_B \f$
      and \f$ \alpha \f$ is not written explicitly. This quantity is
      computed by function \ref fesym(). Note that many of the
      functions in this class are written in terms of the proton
      fraction \f$ x_p = (1-\alpha)/2 \f$ denoted as <tt>'pf'</tt>
      instead of as functions of \f$ \alpha \f$. Frequently, \f$
      S(n_B,\alpha) \f$ is evaluated at \f$ \alpha=0 \f$ to give a
      univariate function of the baryon density. It is sometimes also
      evaluated at the point \f$ (n_B=n_0, \alpha=0) \f$, and this
      value is denoted by \f$ S \f$ above and is typically stored in
      \ref esym. Alternatively, one can define the symmetry energy by
      \f[
      \tilde{S}(n_B) \approx E(n_B,\alpha=1)-E(n_B,\alpha=0)
      \f]
      which is computed by function \ref fesym_diff() . The
      functions \f$ S(n_B,\alpha=0) \f$ and \f$ \tilde{S}(n_B) \f$
      are equal when \f$ {\cal O}(\alpha^4) \f$ terms are zero. 
      In this case, \f$ \mu_n - \mu_p \f$ is proportional to 
      \f$ \alpha \f$ and so 
      \f[
      S(n_B) = \tilde{S}(n_B) = \frac{1}{4} 
      \frac{(\mu_n-\mu_p)}{\alpha} \, .
      \f]
      
      The symmetry energy slope parameter \f$ L \f$, can be defined
      by 
      \f[
      L(n_B,\alpha) \equiv 3 n_B \frac{\partial S(n_B,\alpha)}
      {\partial n_B} = 3 n_B \frac{\partial}{\partial n_B} \left[ 
      \frac{1}{2 n_B} \frac{\partial^2 \varepsilon}{\partial \alpha^2}
      \right]
      \f]
      This can be rewritten as 
      \f[
      L(n_B,\alpha) = \frac{3 n_B}{4} \frac{\partial}{\partial n_B}
      \frac{\partial}{\partial \alpha} \left(\mu_n - \mu_p\right)
      \f]
      (where the derivatives can be evaluated in either order) 
      and this is the method used to compute this function 
      in \ref fesym_slope(). Alternatively, using 
      \f[
      \left(\frac{\partial \varepsilon}{\partial n_B}\right)_{\alpha} = 
      \frac{\partial \varepsilon}{\partial n_n} 
      \left(\frac{\partial n_n}{\partial n_B}\right)_{\alpha} +
      \frac{\partial \varepsilon}{\partial n_p} 
      \left(\frac{\partial n_p}{\partial n_B}\right)_{\alpha} 
      = \frac{1}{2} \left(\mu_n + \mu_p \right)
      \f]
      \f$ L \f$ can be rewritten
      \f{eqnarray*}
      L(n_B,\alpha) &=& 3 n_B \left[\frac{-1}{2 n_B^2} 
      \frac{\partial^2 \varepsilon}{\partial \alpha^2} + 
      \frac{1}{4 n_B} \frac{\partial^2}{\partial \alpha^2} 
      \left(\mu_n + \mu_p\right)\right] \\
      &=& \frac{3}{4}\frac{\partial^2}{\partial \alpha^2} 
      \left(\mu_n + \mu_p\right) - 3 S(n_B,\alpha) \, .
      \f}

      The third derivative with respect to the baryon density is
      sometimes called the skewness. Here, we define
      \f[
      Q(n_B,\alpha) = 27 n_B^3 \frac{\partial^3}{\partial n_B^3} 
      \left(\frac{\varepsilon}{n_B}\right) = 
      27 n_B^3 \frac{\partial^2}{\partial n_B^2} 
      \left(\frac{P}{n_B^2}\right)
      \f]
      and this function is computed in \ref fkprime() by computing
      the second derivative of the pressure.

      The second derivative of the symmetry energy with respect
      to the baryon density is
      \f[
      K_{\mathrm{sym}}(n_B,\alpha) = 9 n_B^2 
      \frac{\partial^2}{\partial n_B^2} S(n_B,\alpha)
      \f]
      and this function is computed in \ref fesym_curve(). 

      The third derivative of the symmetry energy with respect
      to the baryon density is
      \f[
      Q_{\mathrm{sym}}(n_B,\alpha) = 27 n_B^3 
      \frac{\partial^3}{\partial n_B^3} S(n_B,\alpha)
      \f]
      and this function is computed in \ref fesym_skew(). Note that
      the numerical evaluation of higher derivatives can make \ref
      eos_had_base::fesym_curve() and \ref eos_had_base::fesym_skew()
      inaccurate.

      Note that assuming terms of order \f$ \epsilon^3 \f$ and higher
      are zero and solving for the baryon density for which \f$ P=0 \f$
      gives, to order \f$ \alpha^2 \f$ (\ref Piekarewicz09),
      \f[
      n_{B,\mathrm{sat}} = n_0 \left[ 1 - \frac{3 L \alpha^2}{K} \right]
      \f]
      this implies a new 'incompressibility' around the saturation
      point, i.e. 
      \f[
      K(n_B=n_{B,\mathrm{sat}},\alpha)=
      K+\alpha^2 \left( K_{\mathrm{sym}}-6 L- \frac{L Q}{K} \right)
      + {\cal O}\left(\alpha^4\right)
      \f]
      The quantity in parenthesis is referred to by some authors as
      \f$ K_{\tau} \f$. Note that, because one is evaluating this at
      \f$ n_B=n_{B,\mathrm{sat}} \f$, this is distinct from
      \f[
      \tilde{K}_{\tau} \equiv \frac{1}{2} \frac{\partial^2 K(n_B,\alpha)}
      {\partial \alpha^2}
      \f]
      which is equal to \f$ K_{\mathrm{sym}} + 6 L \f$ to lowest order
      in \f$ \alpha \f$ at \f$ n_B = n_0 \f$.

      The quartic symmetry energy \f$ S_4(n_B,\alpha) \f$ can be defined as
      \f[
      S_4(n_B,\alpha) \equiv \frac{1}{24 n_B}\frac{\partial^4 \varepsilon}
      {\partial \alpha^4}
      \f]
      However, fourth derivatives are difficult numerically, and so an
      alternative quantity is preferable. Instead, one can evaluate
      the extent to which \f$ {\cal O}(\alpha^4) \f$ terms are
      important from
      \f[
      \eta(n_B) \equiv \frac{E(n_B,1)-E(n_B,1/2)}
      {3 \left[E(n_B,1/2)-E(n_B,0)\right]}
      \f]
      as described in \ref Steiner06 . This function can be expressed
      either in terms of \f$ \tilde{S} \f$ or \f$ S_4 \f$
      \f[
      \eta(n_B) = \frac{5 \tilde{S}(n_B) - S(n_B,0)}
      {\tilde{S}(n_B) + 3 S(n_B,0)} =
      \frac{5 S_4(n_B,0) + 4 S(n_B,0)}
      {S_4(n_B,0) + 4 S(n_B,0)} 
      \f]

      Evaluating this function at the saturation density gives
      \f[
      \eta(n_0) = \frac{4 S + 5 S_4}{4 S + S_4}
      \f]
      (Note that \f$ S_4 \f$ is referred to as \f$ Q \f$ in 
      \ref Steiner06). Sometimes it is useful to separate out
      the kinetic and potential parts of the energy density when
      computing \f$ \eta(n_B) \f$, and the class \ref eos_had_sym4_base_base
      is useful for this purpose. 

      \future Could write a function to compute the "symmetry free energy"
      or the "symmetry entropy"
  */
  class eos_had_base : public eos_base {

  public:
    
    typedef boost::numeric::ublas::vector<double> ubvector;
    typedef boost::numeric::ublas::vector<int> ubvector_int;
    typedef boost::numeric::ublas::matrix<double> ubmatrix;
    typedef boost::numeric::ublas::matrix<int> ubmatrix_int;

    eos_had_base();

    virtual ~eos_had_base() {};

    /// Binding energy
    double eoa;

    /// Compression modulus
    double comp;

    /// Symmetry energy
    double esym;
  
    /// Saturation density
    double n0;

    /// Effective mass (neutron)
    double msom;

    /// Skewness
    double kprime;
    
    /// \name Equation of state
    //@{
    /** \brief Equation of state as a function of the chemical potentials
    */
    virtual int calc_p(fermion &n, fermion &p, thermo &th)=0;

    /** \brief Equation of state as a function of density
    */
    virtual int calc_e(fermion &n, fermion &p, thermo &th)=0;
    //@}

    /// \name EOS properties
    //@{
    /** \brief Calculate the incompressibility in \f$ \mathrm{fm}^{-1} \f$ 
	using calc_e()

	This function computes \f$ K (n_B,\alpha) = 9 n_B \partial^2
	\varepsilon /(\partial n_B^2) = 9 \partial P / (\partial n_B)
	\f$ . The value of \f$ K(n_0,0) \f$, often referred to as the
	"compressibility", is stored in \ref comp by \ref saturation() 
	and is about 240 MeV at saturation density.
    */
    virtual double fcomp(double nb, const double &alpha=0.0);

    /** \brief Compute the incompressibility and its uncertainty

	This function works like \ref fcomp(), except that it also
	returns the uncertainty in \c unc. 
     */
    virtual double fcomp_err(double nb, double alpha, double &unc);

    /** \brief Calculate the energy per baryon in \f$ \mathrm{fm}^{-1} \f$ 
	using calc_e()

	This function computes the energy per baryon of matter 
	without the nucleon rest masses at the specified baryon
	density, \c nb, and isospin asymmetry \c alpha. 
    */
    virtual double feoa(double nb, const double &alpha=0.0);

    /** \brief Calculate symmetry energy of matter in 
	\f$ \mathrm{fm}^{-1} \f$ using \ref calc_dmu_alpha() .

	This function computes the symmetry energy,
	\f[
	\left(\frac{1}{2 n_B}\frac{d^2 \varepsilon}{d \alpha^2}
	\right) = \frac{1}{4} \frac{\partial}{\partial \alpha}
	\left(\mu_n - \mu_p \right)
	\f]
	at the value of \f$ n_B \f$ given in \c nb and \f$ \alpha \f$
	given in \c alpha. The symmetry energy at \f$ \alpha=0 \f$ at
	the saturation density and is stored in \ref esym by 
	\ref saturation().
    */
    virtual double fesym(double nb, const double &alpha=0.0);

    /** \brief Calculate symmetry energy of matter and its
	uncertainty

	This estimates the uncertainty due to the numerical
	differentiation, assuming that difference betwen the neutron
	and proton chemical potentials is computed exactly by \ref
	calc_dmu_alpha() .
    */
    virtual double fesym_err(double nb, double alpha, 
			     double &unc);
    
    /** \brief The symmetry energy slope parameter
	
	This returns the value of the "slope parameter" of the
	symmetry energy as a function of baryon density \c nb and
	isospin asymmetry \c alpha. This ranges between about zero and
	200 MeV for most equations of state.
    */
    virtual double fesym_slope(double nb, const double &alpha=0.0);

    /** \brief The curvature of the symmetry energy
     */
    virtual double fesym_curve(double nb, const double &alpha=0.0);

    /** \brief The skewness of the symmetry energy
     */
    virtual double fesym_skew(double nb, const double &alpha=0.0);

    /** \brief Calculate symmetry energy of matter as energy of 
	neutron matter minus the energy of nuclear matter

	This function returns the energy per baryon of neutron matter
	minus the energy per baryon of nuclear matter. This will
	deviate significantly from the results from fesym() only if
	the dependence of the symmetry energy on \f$ \delta \f$ is not
	quadratic.
    */
    virtual double fesym_diff(double nb);

    /** \brief The strength parameter for quartic terms in the 
	symmetry energy
    */
    virtual double feta(double nb);

    /** \brief Calculate skewness of nuclear matter using calc_e()

	The skewness is defined to be 
	\f$ 27 n^3 d^3 (\varepsilon/n)/(d n^3) = 
	27 n^3 d^2 (P/n^2) / (d n^2) \f$ 
	and is denoted 'kprime'. This definition seems to be ambiguous
	for densities other than the saturation density and is not
	quite analogous to the compression modulus.
    */
    virtual double fkprime(double nb, const double &alpha=0.0);

    /** \brief Calculate reduced neutron effective mass using calc_e()

	Neutron effective mass (as stored in <tt>part::ms</tt>)
	divided by vacuum mass (as stored in <tt>part::m</tt>) in
	nuclear matter at saturation density. Note that this simply
	uses the value of n.ms from calc_e(), so that this effective
	mass could be either the Landau or Dirac mass depending on the
	context. Note that this may not be equal to the reduced proton
	effective mass.
    */
    virtual double fmsom(double nb, const double &alpha=0.0);

    /** \brief Calculate saturation density using calc_e()

	This function finds the baryon density for which the pressure 
	vanishes. 
    */
    virtual double fn0(double alpha, double &leoa);

    /** \brief Calculates some of the EOS properties at the saturation 
	density
	
	This computes the saturation density, and the
	incompressibility, the symmetry energy, the binding energy,
	the reduced neutron effective mass at the saturation density,
	and the skewness in isospin-symmetric matter. The results are
	stored in \ref n0, \ref comp, \ref esym, \ref eoa, \ref msom,
	and \ref kprime, respectively.

	\future It would be great to provide numerical uncertainties
	in the saturation properties.
    */
    virtual void saturation();
    //@}

    /** \brief Calculate coefficients for \gradient \part of Hamiltonian

	\note This is still somewhat experimental.

	We want the \gradient \part of the Hamiltonian in the form
	\f[
	{\cal H}_{\mathrm{grad}} = \frac{1}{2} \sum_{i=n,p}
	\sum_{j=n,p} Q_{ij}
	\vec{\nabla} n_i \cdot \vec{\nabla} n_j
	\f]

	The expression for the \gradient terms from \ref Pethick95 is 
	\f{eqnarray*}
	{\cal H}_{\mathrm{grad}}&=&-\frac{1}{4}
	\left(2 P_1 + P_{1;f}-P_{2;f}\right) 
	\nonumber \\
	&& +\frac{1}{2} \left( Q_1+Q_2 \right) 
	\left(n_n \nabla^2 n_n+n_p \nabla^2 n_p\right) \nonumber \\
	&& + \frac{1}{4}\left( Q_1-Q_2 \right) 
	\left[\left(\nabla n_n\right)^2+\left(\nabla n_p\right)^2
	\right] \nonumber \\
	&& + \frac{1}{2} \frac{d Q_2}{d n} 
	\left( n_n \nabla n_n + n_p \nabla n_p \right) \cdot \nabla n
	\f}
	This can be rewritten
	\f{eqnarray*}
	{\cal H}_{\mathrm{grad}}&=&\frac{1}{2}\left(\nabla n\right)^2
	\left[ \frac{3}{2} P_1+n \frac{d P_1}{d n}\right] \nonumber \\
	&& - \frac{3}{4} \left[ \left( \nabla n_n\right)^2 + 
	\left( \nabla n_p \right)^2 \right] \nonumber \\
	&& -\frac{1}{2} \left[ \right] \cdot \nabla n \frac{d Q_1}{d n}
	\nonumber \\ && - \frac{1}{4} \left( \nabla n\right)^2 P_2
	- \frac{1}{4} \left[ \left( \nabla n_n\right)^2 +
	\left( \nabla n_p \right)^2 \right] Q_2
	\f}
	or
	\f{eqnarray*}
	{\cal H}_{\mathrm{grad}}&=&\frac{1}{4} \left( \nabla n\right)^2
	\left[3 P_1 + 2 n \frac{d P_1}{d n}-P_2\right] \nonumber \\
	&& - \frac{1}{4} \left( 3 Q_1+Q_2 \right)
	\left[ \left( \nabla n_n\right)^2 + 
	\left( \nabla n_p \right)^2 \right] \nonumber \\
	&& - \frac{1}{2} \frac{d Q_1}{d n}
	\left[ n_n \nabla n_n + n_p \nabla n_p \right]
	\cdot \nabla n 
	\f}
	or
	\f{eqnarray*}
	{\cal H}_{\mathrm{grad}}&=&\frac{1}{4} \left( \nabla n\right)^2
	\left[3 P_1 + 2 n \frac{d P_1}{d n}-P_2\right] \nonumber \\
	&& - \frac{1}{4} \left( 3 Q_1+Q_2 \right)
	\left[ \left( \nabla n_n\right)^2 + 
	\left( \nabla n_p \right)^2 \right] \nonumber \\
	&& - \frac{1}{2} \frac{d Q_1}{d n}
	\left[ n_n \left( \nabla n_n \right)^2 +
	n_p \left( \nabla n_p \right)^2 + n \nabla n_n \cdot
	\nabla n_p \right]
	\f}

	Generally, for Skyrme-like interactions
	\f{eqnarray*}
	P_i &=& \frac{1}{4} t_i \left(1+\frac{1}{2} x_i \right) \nonumber \\
	Q_i &=& \frac{1}{4} t_i \left(\frac{1}{2} + x_i \right) \, .
	\f}
	for \f$ i=1,2 \f$ .

	This function uses the assumption \f$ x_1=x_2=0 \f$ to 
	calculate \f$ t_1 \f$ and \f$ t_2 \f$ from the neutron
	and proton effective masses assuming the Skyrme form. The
	values of \f$ Q_{ij} \f$ and their derivatives are then computed.

	The functions set_n_and_p() and set_thermo() will be called by
	gradient_qij(), to facilitate the use of the \c n, \c p, and
	\c th parameters.
       
    */
    void gradient_qij(fermion &n, fermion &p, thermo &th, 
		     double &qnn, double &qnp, double &qpp, 
		     double &dqnndnn, double &dqnndnp,
		     double &dqnpdnn, double &dqnpdnp,
		     double &dqppdnn, double &dqppdnp);
    
    /// \name Functions for calculating physical properties
    //@{

    /** \brief Compute the difference between neutron and proton chemical
	potentials as a function of the isospin asymmetry

	This uses \ref neutron, \ref proton, \ref eos_base::eos_thermo,
	and \ref calc_e() .
    */
    double calc_dmu_alpha(double alpha, const double &nb);
    
    /** \brief Compute the sum of the neutron and proton chemical
	potentials as a function of the isospin asymmetry

	This uses \ref neutron, \ref proton, \ref eos_base::eos_thermo,
	and \ref calc_e() .
    */
    double calc_musum_alpha(double alpha, const double &nb);

    /** \brief Compute the pressure as a function of baryon density
	at fixed isospin asymmetry

        Used by fcomp().
    */
    double calc_pressure_nb(double nb, const double &alpha=0.0);

    /** \brief Compute the energy density as a function of baryon density
	at fixed isospin asymmetry

	This uses \ref neutron, \ref proton, \ref eos_base::eos_thermo,
	and \ref calc_e() .
    */
    double calc_edensity_nb(double nb, const double &alpha=0.0);

    /** \brief Compute derivatives at constant proton fraction
     */
    void const_pf_derivs(double nb, double pf, 
			 double &dednb_pf, double &dPdnb_pf);

    /** \brief Calculate pressure / baryon density squared in nuclear
	matter as a function of baryon density at fixed isospin asymmetry
	
	Used by fkprime().

	This uses \ref neutron, \ref proton, \ref eos_base::eos_thermo,
	and \ref calc_e() .
    */
    double calc_press_over_den2(double nb, const double &alpha=0.0);

    /** \brief Calculate energy density as a function of the
	isospin asymmetry at fixed baryon density

        Used by fesym().

	This function calls \ref eos_had_base::calc_e() with the
	internally stored neutron and proton objects.
    */
    double calc_edensity_alpha(double alpha, const double &nb);
    //@}

    /// \name Other functions
    //@{
    /** \brief Solve for the chemical potentials given the densities

	The neutron and proton chemical potentials should be stored in
	<tt>x[0]</tt> and <tt>x[1]</tt> and the neutron and proton
	densities should be stored in <tt>pa[0]</tt> and
	<tt>pa[1]</tt>.

	Because this function is designed to be used in a solver,
	it returns \ref exc_efailed without calling the error 
	handler if the densities are not finite.

	This function is used by \ref eos_had_base_pres::calc_e().
    */
    int nuc_matter_p(size_t nv, const ubvector &x, ubvector &y, 
		     double *&pa);
    
    /** \brief Solve for the densities given the chemical potentials

	The neutron and proton densities should be stored in
	<tt>x[0]</tt> and <tt>x[1]</tt> and the neutron and proton
	chemical potentials should be stored in <tt>pa[0]</tt> and
	<tt>pa[1]</tt>.

	Because this function is designed to be used in a solver,
	it returns \ref exc_efailed without calling the error 
	handler if the chemical potentials are not finite.

	This function is used by \ref eos_had_base_eden::calc_p().
    */
    int nuc_matter_e(size_t nv, const ubvector &x, ubvector &y, 
		     double *&pa);
    //@}

    /// \name Set auxilliary objects
    //@{
    /** \brief Set class mroot object for use in calculating chemical
	potentials from densities 

	\note While in principle this allows one to use any \ref mroot
	object, in practice some of the current EOSs require \ref
	mroot_hybrids because it automatically avoids regions
	where the equations are undefined.
    */
    virtual void set_mroot(mroot<mm_funct<>,
			  boost::numeric::ublas::vector<double>, 
			  jac_funct<> > &mr);
    
    /** \brief Set class mroot object for use calculating saturation density

	\note While in principle this allows one to use any \ref mroot
	object, in practice some of the current EOSs require \ref
	mroot_hybrids because it automatically avoids regions
	where the equations are undefined.
     */
    virtual void set_sat_root(root<funct > &mr);
    
    /// Set \ref deriv_base object to use to find saturation properties
    virtual void set_sat_deriv(deriv_base<funct > &de);

    /** \brief Set the second \ref deriv_base object to use to find
	saturation properties

	Computing the slope of the symmetry energy at the saturation
	density requires two derivative objects, because it has to
	take an isospin derivative and a density derivative. Thus this
	second \ref deriv_base object is used in the function
	fesym_slope().
    */
    virtual void set_sat_deriv2(deriv_base<funct> &de);
    
    /// Set neutron and proton 
    virtual void set_n_and_p(fermion &n, fermion &p);
    //@}

    /** \brief The defaut neutron

	By default this has a spin degeneracy of 2 and a mass of \ref
	o2scl_mks::mass_neutron . Also the value of 
	<tt>part::non_interacting</tt> is set to <tt>false</tt>.
     */
    fermion def_neutron;

    /** \brief The defaut proton

	By default this has a spin degeneracy of 2 and a mass of \ref
	o2scl_mks::mass_proton . Also the value of 
	<tt>part::non_interacting</tt> is set to <tt>false</tt>.
     */
    fermion def_proton;

    /// \name Default solvers and derivative classes
    //@{
    /** \brief The default object for derivatives
	
	The value of deriv_gsl::h is set to \f$ 10^{-3} \f$ in 
	the eos_had_base constructor.
    */
    deriv_gsl<funct > def_deriv;
    
    /** \brief The second default object for derivatives
	
	The value of deriv_gsl::h is set to \f$ 10^{-3} \f$ in 
	the eos_had_base constructor.
    */
    deriv_gsl<funct> def_deriv2;

    /** \brief The default solver

	Used by calc_e() to solve nuc_matter_p() (2 variables) and by
	calc_p() to solve nuc_matter_e() (2 variables).
    */
    mroot_hybrids<mm_funct<>,
      boost::numeric::ublas::vector<double>, 
      boost::numeric::ublas::matrix<double>,
      jac_funct<> > def_mroot;
    
    /** \brief The default solver for calculating the saturation 
	density

	Used by fn0() (which is called by saturation()) to solve
	saturation_matter_e() (1 variable).
    */
    root_cern<funct > def_sat_root;
    //@}
    
    /// Return string denoting type ("eos_had_base")
    virtual const char *type() { return "eos_had_base"; }

#ifndef DOXYGEN_INTERNAL

  protected:

    /// Compute t1 for gradient_qij().
    double t1_fun(double barn);
    
    /// Compute t2 for gradient_qij().
    double t2_fun(double barn);
    
    /// The EOS solver
    mroot<mm_funct<>,boost::numeric::ublas::vector<double>, 
      jac_funct<> > *eos_mroot;
    
    /// The solver to compute saturation properties
    root<funct> *sat_root;

    /// The derivative object for saturation properties
    deriv_base<funct> *sat_deriv;
    
    /// The second derivative object for saturation properties
    deriv_base<funct> *sat_deriv2;

    /// The neutron object
    fermion *neutron;

    /// The proton object
    fermion *proton;

#endif
    
  };

  /// A hadronic EOS based on a function of the densities [abstract base]
  class eos_had_base_eden : public eos_had_base {
  public:

    /** \brief Equation of state as a function of density
    */
    virtual int calc_e(fermion &n, fermion &p, thermo &th)=0;
    
    /** \brief Equation of state as a function of the chemical potentials
    */
    virtual int calc_p(fermion &n, fermion &p, thermo &th);

  };

  /** \brief A hadronic EOS based on a function of the chemical 
      potentials [abstract base]
  */
  class eos_had_base_pres : public eos_had_base {
  public:

    /** \brief Equation of state as a function of the chemical potentials
    */
    virtual int calc_p(fermion &n, fermion &p, thermo &th)=0;
    
    /** \brief Equation of state as a function of density
    */
    virtual int calc_e(fermion &n, fermion &p, thermo &th);

  };

  /// A finite temperature hadronic EOS
  class eos_had_base_temp : public eos_had_base {

#ifndef DOXYGEN_INTERNAL
    
  protected:

    /// Fermion thermodynamics (default is \ref def_fet)
    fermion_eval_thermo *fet;

    /// The temperature
    double lT;

    /// Solve for nuclear matter at finite temperature given density
    int nuc_matter_temp_e(size_t nv, const ubvector &x, 
			  ubvector &y, double *&pa);
    
    /// Solve for nuclear matter at finite temperature given mu
    int nuc_matter_temp_p(size_t nv, const ubvector &x, 
			  ubvector &y, double *&pa);
    
#endif

  public:

    eos_had_base_temp() {
      fet=&def_fet;
    }

    virtual ~eos_had_base_temp() {}

    /** \brief Set the object for computing finite-temperature fermions
	(default is \ref def_fet)
     */
    virtual void set_fermion_eval_thermo(fermion_eval_thermo &f) {
      fet=&f;
    }

    /// Default fermion thermodynamics object
    fermion_eff def_fet;

    /** \brief Equation of state as a function of density
    */
    virtual int calc_e(fermion &n, fermion &p, thermo &th)=0;
    
    /** \brief Equation of state as a function of densities at 
	finite temperature
    */
    virtual int calc_temp_e(fermion &n, fermion &p, double T, 
			    thermo &th)=0;

    /** \brief Equation of state as a function of the chemical potentials
    */
    virtual int calc_p(fermion &n, fermion &p, thermo &th)=0;

    /** \brief Equation of state as a function of the chemical potentials
	at finite temperature
    */
    virtual int calc_temp_p(fermion &n, fermion &p, double T, 
			    thermo &th)=0;
    
  };

  /** \brief A hadronic EOS at finite temperature
      based on a function of the densities [abstract base]
  */
  class eos_had_base_temp_eden : public eos_had_base_temp {
  public:

    /** \brief Equation of state as a function of density
    */
    virtual int calc_e(fermion &n, fermion &p, thermo &th)=0;
    
    /** \brief Equation of state as a function of densities at 
	finite temperature
    */
    virtual int calc_temp_e(fermion &n, fermion &p, double T, 
			    thermo &th)=0;

    /** \brief Equation of state as a function of the chemical potentials
    */
    virtual int calc_p(fermion &n, fermion &p, thermo &th);

    /** \brief Equation of state as a function of the chemical potentials
	at finite temperature
    */
    virtual int calc_temp_p(fermion &n, fermion &p, double T, 
			    thermo &th);
    
  };

  /** \brief A hadronic EOS at finite temperature based on a function
      of the chemical potentials [abstract base]
  */
  class eos_had_base_temp_pres : public eos_had_base_temp {
  public:

    /** \brief Equation of state as a function of the chemical potentials
    */
    virtual int calc_p(fermion &n, fermion &p, thermo &th)=0;

    /** \brief Equation of state as a function of the chemical potentials
	at finite temperature
    */
    virtual int calc_temp_p(fermion &n, fermion &p, double T, 
			    thermo &th)=0;
    
    /** \brief Equation of state as a function of density
    */
    virtual int calc_e(fermion &n, fermion &p, thermo &th);

    /** \brief Equation of state as a function of densities at 
	finite temperature
    */
    virtual int calc_temp_e(fermion &n, fermion &p, double T, 
			    thermo &th);
  };

#ifndef DOXYGEN_NO_O2NS
}
#endif

#endif
