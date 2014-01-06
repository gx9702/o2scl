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

#include <o2scl/test_mgr.h>
#include <o2scl/apr_eos.h>
#include <o2scl/mroot_hybrids.h>
#include <o2scl/mroot_cern.h>
#include <o2scl/tov_eos.h>
#include <o2scl/tov_solve.h>
#include <o2scl/hdf_file.h>
#include <o2scl/hdf_io.h>

using namespace std;
using namespace o2scl;
using namespace o2scl_const;

typedef boost::numeric::ublas::vector<double> ubvector;

class simple_apr {

public:
  
  fermion n, p, e;
  thermo ht, hb, l;
  apr_eos ap;
  fermion_zerot fzt;
  double barn;

  simple_apr() {
    n.init(939.0/hc_mev_fm,2.0);
    p.init(939.0/hc_mev_fm,2.0);
    e.init(0.511/hc_mev_fm,2.0);
    ap.select(1);
    ap.pion=0;
  }

  int nstarfun(size_t nv, const ubvector &x, ubvector &y) {
    
    n.n=x[0];
    p.n=x[1];
    e.mu=x[2];
    
    if (x[0]<0.0 || x[1]<0.0) return 1;
    
    ap.calc_e(n,p,hb);
    fzt.calc_mu_zerot(e);
    
    l.ed=e.ed;
    l.pr=e.pr;
    ht=l+hb;
    
    y[0]=n.n+p.n-barn;
    y[1]=p.n-e.n;
    y[2]=n.mu-p.mu-e.mu;
    
    return 0;
  }
  
};

int main(void) {

  cout.setf(ios::scientific);

  test_mgr t;
  t.set_output_level(2);
  
  // --------------------------------------------------------------
  // Calculate APR EOS, with naive phase transition and no muons

  simple_apr sa;

  cout << "----------------------------------------------------" << endl;
  cout << "Compute APR EOS for testing:" << endl;

  mroot_hybrids<mm_funct<> > gmh;
  mroot_cern<mm_funct<> > cm;
  mm_funct_mfptr<simple_apr> nf(&sa,&simple_apr::nstarfun);

  ubvector x(3);
  x[0]=0.09663;
  x[1]=0.003365;
  x[2]=0.4636;
  
  table_units<> eos;
  eos.line_of_names("ed pr nb mun");
  eos.set_unit("ed","Msun/km^3");
  eos.set_unit("pr","Msun/km^3");
  eos.set_unit("nb","1/fm^3");
  eos.set_unit("mun","MeV");

  convert_units &cu=o2scl_settings.get_convert_units();

  for(sa.barn=0.1;sa.barn<=2.0001;sa.barn+=0.01) {
    
    gmh.err_nonconv=false;
    int ret=gmh.msolve(3,x,nf);
    gmh.err_nonconv=true;

    // If the GSL solver fails, use CERNLIB
    if (ret!=0) {
      int ret2=cm.msolve(3,x,nf);
      t.test_gen(ret2==0,"Solver success.");
    }
    
    double line[4]={cu.convert("1/fm^4","Msun/km^3",sa.ht.ed),
		    cu.convert("1/fm^4","Msun/km^3",sa.ht.pr),
		    sa.barn,sa.n.mu*hc_mev_fm};
    eos.line_of_data(4,line);
  }
  
  tov_interp_eos te;
  te.verbose=2;
  te.default_low_dens_eos();
  te.read_table(eos,"ed","pr","nb");
  
  t.test_gen(te.baryon_column==true,"baryon column");
  vector<string> auxp, auxu;
  size_t np;
  te.get_names_units(np,auxp,auxu);
  t.test_gen(np==1,"One aux parm");
  cout << endl;

  // --------------------------------------------------------------
  // The tov_solve object

  tov_solve at;
  at.def_solver.tol_rel*=10.0;
  at.def_solver.tol_abs*=10.0;
  at.calc_gpot=true;
  at.verbose=0;
  at.set_eos(te);

  // Get a pointer to the results table for use later
  o2_shared_ptr<table_units<> >::type tab=at.get_results();
  tab->set_interp_type(itp_linear);

  // --------------------------------------------------------------
  // Test 1.4 solar mass star.

  cout << "----------------------------------------------------" << endl;
  cout << "1.4 solar mass neutron star:" << endl;

  // 1.4 solar mass star
  at.verbose=1;
  at.fixed(1.4);
  at.verbose=0;

  // The interpolations can fail if there are rows with duplicate
  // masses or radii, so we remove those here.
  for(size_t i=0;i<tab->get_nlines()-1;i++) {
    if (tab->get("r",i)==tab->get("r",i+1) ||
	tab->get("gm",i)==tab->get("gm",i+1)) {
      tab->delete_row(i);
      i=0;
    }
  }

  tab->summary(&cout);
  cout << endl;
  cout << tab->interp("nb",0.08,"ed") << " " 
       << tab->interp("nb",0.08,"pr") << endl;
  cout << tab->interp("r",at.rad,"gm") << " "
       << tab->interp("r",at.rad,"bm") << endl;
  t.test_rel(tab->interp("r",at.rad,"gm"),1.4,1.0e-4,"grav. mass.");
  t.test_rel(tab->interp("nb",0.08,"ed"),6.79e-5,5.0e-3,"trans. ed");
  t.test_rel(tab->interp("nb",0.08,"pr"),3.64e-7,2.0e-1,"trans. pr");
  t.test_rel(at.mass,1.4,1.0e-6,"APR Maximum mass.");
  t.test_rel(at.rad,11.4,0.03,"APR Radius of 1.4 solar mass star.");
  t.test_rel(at.bmass,1.58,0.02,"APR baryonic mass of 1.4 solar mass star.");
  t.test_gen(tab->get_unit("mun")==((string)"MeV"),"Aux unit.");
  cout << endl;

  // Record radial location of two pressures for testing later
  double pr1=cu.convert("MeV/fm^3","Msun/km^3",10.0);
  double pr2=cu.convert("MeV/fm^3","Msun/km^3",40.0);
  double r_pr1=tab->interp("pr",pr1,"r");
  double r_pr2=tab->interp("pr",pr2,"r");
  double gm_pr1=tab->interp("pr",pr1,"gm");
  double gm_pr2=tab->interp("pr",pr2,"gm");
  cout << r_pr1 << " " << r_pr2 << " "
       << gm_pr1 << " " << gm_pr2 << endl;

  // --------------------------------------------------------------
  // With rotation

  cout << "----------------------------------------------------" << endl;
  cout << "With rotation: " << endl;

  // Try 1.4 solar mass neutron star again, but this time with rotation
  at.ang_vel=true;
  at.fixed(1.4);
  at.ang_vel=false;
  tab=at.get_results();
  cout << "calc_gpot: " << at.calc_gpot << endl;
  tab->summary(&cout);

  // The interpolations can fail if there are rows with duplicate
  // masses or radii, so we remove those here.
  for(size_t i=0;i<tab->get_nlines()-1;i++) {
    if (tab->get("r",i)==tab->get("r",i+1) ||
	tab->get("gm",i)==tab->get("gm",i+1)) {
      tab->delete_row(i);
      i=0;
    }
  }

  double schwarz_km=o2scl_cgs::schwarzchild_radius/1.0e5;
  string sfunc=((string)"iand=8.0*acos(-1)/3.0*r^4*(ed+pr)")+
    "*exp(-gp)*omega_rat/sqrt(1-schwarz*gm/r)";
  tab->functions_columns(sfunc);
  double mom=tab->integ("r",0.0,at.rad,"iand");
  t.test_rel(mom,67.7,1.0e-2,"I method 1");
  double mom2=at.domega_rat*pow(at.rad,4.0)/3.0/schwarz_km;
  t.test_rel(mom2,67.7,1.0e-2,"I method 2");
  // Crustal fraction of the moment of inertia
  double r08=tab->interp("nb",0.08,"r");
  double mom_crust=tab->integ("r",r08,at.rad,"iand");
  t.test_rel(mom_crust/mom,0.029,2.0e-1,"crustal fraction of I");
  cout << endl;

  {
    o2scl_hdf::hdf_file hf;
    hf.open_or_create("tov_solve_rot.o2");
    o2scl_hdf::hdf_output(hf,*tab,"t");
    hf.close();
  }

  // --------------------------------------------------------------
  // Maximum mass star

  cout << "----------------------------------------------------" << endl;
  cout << "Maximum mass star: " << endl;

  at.max();

  // The interpolations can fail if there are rows with duplicate
  // masses or radii, so we remove those here.
  for(size_t i=0;i<tab->get_nlines()-1;i++) {
    if (tab->get("r",i)==tab->get("r",i+1) ||
	tab->get("gm",i)==tab->get("gm",i+1)) {
      tab->delete_row(i);
      i=0;
    }
  }

  cout << "Maximum mass table:" << endl;
  tab->summary(&cout);
  cout << endl;
  double massmax=at.mass;
  double radmax=at.rad;
  t.test_rel(tab->interp("nb",0.08,"ed"),6.79e-5,5.0e-3,"trans. ed");
  t.test_rel(tab->interp("nb",0.08,"pr"),3.64e-7,2.0e-1,"trans. pr");
  t.test_rel(tab->interp("r",at.rad,"gm"),2.20,0.03,"grav. mass.");
  t.test_rel(at.mass,2.20,0.03,"APR Maximum mass.");
  t.test_rel(at.rad,10.0,0.02,"APR Radius of maximum mass star.");
  t.test_rel(at.bmass,2.68,0.01,"APR baryonic mass of 1.4 solar mass star.");
  t.test_gen(tab->get_unit("mun")==((string)"MeV"),"Aux unit.");
  cout << endl;

  {
    o2scl_hdf::hdf_file hf;
    hf.open_or_create("tov_solve_max.o2");
    o2scl_hdf::hdf_output(hf,*tab,"t");
    hf.close();
  }

  // --------------------------------------------------------------
  // Mass vs. radius curve

  // Put in some pressures to find the radial location of
  at.pr_list.push_back(pr1);
  at.pr_list.push_back(pr2);

  cout << "----------------------------------------------------" << endl;
  cout << "Mass vs. radius curve: " << endl;
  at.mvsr();
  tab->summary(&cout);
  cout << endl;

  // Test the radial locations of the specified pressures
  double r1test=tab->interp("gm",1.4,"r0");
  double r2test=tab->interp("gm",1.4,"r1");
  double gm1test=tab->interp("gm",1.4,"gm0");
  double gm2test=tab->interp("gm",1.4,"gm1");
  t.test_rel(r_pr1,r1test,1.0e-2,"r_pr1");
  t.test_rel(r_pr2,r2test,1.0e-2,"r_pr2");
  t.test_rel(gm_pr1,gm1test,1.0e-2,"gm_pr1");
  t.test_rel(gm_pr2,gm2test,1.0e-2,"gm_pr2");
  
  {
    o2scl_hdf::hdf_file hf;
    hf.open_or_create("tov_solve_mvsr.o2");
    o2scl_hdf::hdf_output(hf,*tab,"t");
    hf.close();
  }

  t.test_gen(tab->get_unit("mun")==((string)"MeV"),"Aux unit.");

  // --------------------------------------------------------------
  // Test the Buchdahl EOS 

  cout << "----------------------------------------------------" << endl;
  cout << "Buchdahl EOS: " << endl;

  // Return tolerances to normal values
  at.def_solver.tol_rel/=10.0;
  at.def_solver.tol_abs/=10.0;
  tov_buchdahl_eos buch;
  at.set_eos(buch);

  // 1.4 solar mass star
  int info=at.fixed(1.4,1.0e-4);
  double beta=o2scl_mks::schwarzchild_radius/2.0e3*at.mass/at.rad;
  t.test_rel(36.0*buch.Pstar*beta*beta,tab->get("pr",0),1.0e-8,"Buch Pc");
  t.test_rel(72.0*buch.Pstar*beta*(1.0-2.5*beta),
	     tab->get("ed",0),1.0e-8,"Buch rho_c");
  cout << endl;
  
  t.report();

  return 0;
}


