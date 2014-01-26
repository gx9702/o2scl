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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <o2scl/rel_boson.h>

using namespace std;
using namespace o2scl;
using namespace o2scl_const;

//--------------------------------------------
// rel_boson class

rel_boson::rel_boson() {
  density_root=&def_density_root;
  nit=&def_nit;
  dit=&def_dit;
}

rel_boson::~rel_boson() {
}

void rel_boson::set_inte(inte<funct > &l_nit, inte<funct > &l_dit) {
  nit=&l_nit;
  dit=&l_dit;
  return;
}

void rel_boson::calc_mu(boson &b, double temper) {

  T=temper;
  bp=&b;

  if (temper<=0.0) {
    O2SCL_ERR2("Temperature less than or equal to zero in ",
	       "rel_boson::calc_mu().",exc_einval);
  }
  if (b.non_interacting==true) { b.nu=b.mu; b.ms=b.m; }

  funct_mfptr<rel_boson> fd(this,&rel_boson::deg_density_fun);
  funct_mfptr<rel_boson> fe(this,&rel_boson::deg_energy_fun);
  funct_mfptr<rel_boson> fs(this,&rel_boson::deg_entropy_fun);

  b.n=dit->integ(fd,0.0,sqrt(pow(15.0*temper+b.nu,2.0)-b.ms*b.ms));
  b.n*=b.g/2.0/pi2;
  b.ed=dit->integ(fe,0.0,sqrt(pow(15.0*temper+b.nu,2.0)-b.ms*b.ms));
  b.ed*=b.g/2.0/pi2;
  b.en=dit->integ(fs,0.0,sqrt(pow(15.0*temper+b.nu,2.0)-b.ms*b.ms));
  b.en*=b.g/2.0/pi2;

  b.pr=-b.ed+temper*b.en+b.mu*b.n;

  return;
}

void rel_boson::nu_from_n(boson &b, double temper) {
  double nex;

  T=temper;
  bp=&b;

  nex=b.nu/temper;
  funct_mfptr<rel_boson> mf(this,&rel_boson::solve_fun);
  density_root->solve(nex,mf);
  b.nu=nex*temper;
  
  return;
}

void rel_boson::calc_density(boson &b, double temper) {

  T=temper;
  bp=&b;

  if (temper<=0.0) {
    O2SCL_ERR2("Temperature less than or equal to zero in ",
	       "rel_boson::calc_density().",exc_einval);
  }
  if (b.non_interacting==true) { b.nu=b.mu; b.ms=b.m; }

  nu_from_n(b,temper);

  funct_mfptr<rel_boson> fe(this,&rel_boson::deg_energy_fun);
  funct_mfptr<rel_boson> fs(this,&rel_boson::deg_entropy_fun);

  b.ed=dit->integ(fe,0.0,sqrt(pow(20.0*temper+b.nu,2.0)-b.ms*b.ms));
  b.ed*=b.g/2.0/pi2;
  b.en=dit->integ(fs,0.0,sqrt(pow(20.0*temper+b.nu,2.0)-b.ms*b.ms));
  b.en*=b.g/2.0/pi2;

  b.pr=-b.ed+temper*b.en+b.mu*b.n;

  return;
}

double rel_boson::deg_density_fun(double k) {

  double E=sqrt(k*k+bp->ms*bp->ms), ret;

  ret=k*k/(exp(E/T-bp->nu/T)-1.0);

  return ret;
}
  
double rel_boson::deg_energy_fun(double k) {

  double E=sqrt(k*k+bp->ms*bp->ms), ret;

  ret=k*k*E/(exp(E/T-bp->nu/T)-1.0);
  
  return ret;
}
  
double rel_boson::deg_entropy_fun(double k) {

  double E=sqrt(k*k+bp->ms*bp->ms), nx, ret;
  nx=1.0/(exp(E/T-bp->nu/T)-1.0);
  ret=-k*k*(nx*log(nx)-(1.0+nx)*log(1.0+nx));
  
  return ret;
}
  
double rel_boson::density_fun(double u) {
  double ret, y, mx;

  y=bp->nu/T;
  mx=bp->ms/T;
  
  ret=(mx+u)*sqrt(u*u+2.0*mx*u)*exp(u+y)/(exp(y)-exp(mx+u));

  return ret;
}

double rel_boson::energy_fun(double u) {
  double ret, y, mx;

  y=bp->nu/T;
  mx=bp->ms/T;
  
  ret=(mx+u)*(mx+u)*sqrt(u*u+2.0*mx*u)*exp(u+y)/(exp(y)-exp(mx+u));
  
  return ret;
}

double rel_boson::entropy_fun(double u) {
  double ret, y, mx, term1, term2;

  y=bp->mu/T;
  mx=bp->ms/T;

  term1=log(exp(y-mx-u)-1.0)/(exp(y-mx-u)-1.0);
  term2=log(1.0-exp(mx+u-y))/(1.0-exp(mx+u-y));
  ret=(mx+u)*exp(u)*sqrt(u*u+2.0*mx*u)*(term1+term2);
  
  return ret;
}

double rel_boson::solve_fun(double x) {
  double nden, yy;

  funct_mfptr<rel_boson> fd(this,&rel_boson::deg_density_fun);
  
  bp->nu=T*x;
  nden=dit->integ(fd,0.0,sqrt(pow(20.0*T+bp->nu,2.0)-bp->ms*bp->ms));
  nden*=bp->g/2.0/pi2;
  yy=nden/bp->n-1.0;

  return yy;
}

void rel_boson::pair_mu(boson &b, double temper) {
  
  T=temper;
  bp=&b;

  if (b.non_interacting==true) { b.nu=b.mu; b.ms=b.m; }
  calc_mu(b,temper);
  
  boson antip(b.ms,b.g);
  b.anti(antip);
  calc_mu(antip,temper);
  b.n-=antip.n;
  b.pr+=antip.pr;
  b.ed+=antip.ed;
  b.en+=antip.en;

  return;
}

