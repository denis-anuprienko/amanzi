/*
  This is the multiphase component of the Amanzi code. 

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Quan Bui (mquanbui@math.umd.edu)
*/

#include <cmath>
#include <string>
#include <iostream>

#include "errors.hh"
#include "MultiphaseDefs.hh"
#include "WRMmp_VanGenuchten.hh"

namespace Amanzi {
namespace Multiphase {

/* ******************************************************************
* Setup fundamental parameters for this model.                                            
****************************************************************** */
WRMmp_VanGenuchten::WRMmp_VanGenuchten(Teuchos::ParameterList& plist)
{
  double S_rw = plist.get<double>("residual saturation wet", FLOW_WRM_EXCEPTION);
  double S_rn = plist.get<double>("residual saturation nonwet", FLOW_WRM_EXCEPTION);
  double n = plist.get<double>("van genuchten n", FLOW_WRM_EXCEPTION);
  double Pr = plist.get<double>("van genuchten entry pressure", FLOW_WRM_EXCEPTION);

  Init_(S_rw, S_rn, n, Pr);
}

void WRMmp_VanGenuchten::Init_(double S_rw, double S_rn, double n, double Pr)
{
  S_rw_ = S_rw;
  S_rn_ = S_rn;
  n_ = n;
  m_ = 1.0 - 1.0/n_;
  Pr_ = Pr;
  eps_ = 1.0e-3;
}


/* ******************************************************************
* Relative permeability formula.                                          
****************************************************************** */
double WRMmp_VanGenuchten::k_relative(double Sw, std::string phase_name)
{
  Errors::Message msg;
  double Swe = (Sw - S_rw_)/(1.0 - S_rw_ - S_rn_);
  //std::cout << "Swe - 1.0: " << Swe - 1.0 << "\n";
  if (phase_name == "wetting") {
    if (Swe < 1.0e-09) {
      return 0.0;
    } else if(Swe - 1.0 > -1.0e-09) {
      return 1.0;
    } else {
      double tmp = pow(Swe, 0.5) * pow(1.0 - pow(1.0 - pow(Swe, 1.0/m_), m_), 2.0);
      //std::cout << "compute krel with pow: " << tmp << "\n";
      return tmp; 
    }
    //return pow(Swe, 2.0);
  }
  else if (phase_name == "non wetting") {
    if (Swe < 1.0e-09) {
      return 1.0;
    } else if(Swe - 1.0 > -1.0e-09) {
      return 0.0;
    } else {
      return pow(1.0 - Swe, 0.5) * pow(1.0 - pow(Swe, 1.0/m_), 2.0 * m_);
    }
    //return pow(1.0 - Swe, 2.0);
  }
  else {
    msg << "Multiphase PK: phase_name \"" << phase_name.c_str() << "\" not recognized \n";
    Exceptions::amanzi_throw(msg);
  }
}


/* ******************************************************************
* Derivative of Relative permeability wrt wetting saturation formula.                                          
****************************************************************** */
double WRMmp_VanGenuchten::dKdS(double Sw, std::string phase_name)
{
  Errors::Message msg;
  double Swe = 0.0;
  double factor = 1.0/(1.0 - S_rw_ - S_rn_);
  Swe = (Sw - S_rw_)/(1.0 - S_rw_ - S_rn_);
  //std::cout << "Swe - 1.0: " << Swe - 1.0 << "\n";
  if (phase_name == "wetting") {
    if (Swe < 1.0e-09) {
      return 0.0;
    } else if(Swe - 1.0 > -1.0e-09) {
      return 0.5 * factor;
    } else {
      return factor*0.5*pow(Swe,-0.5)*pow(1.0 - pow(1.0 - pow(Swe,1.0/m_),m_),2.0) + 
        2.0*(1.0 - pow(1.0 - pow(Swe, 1.0/m_),m_))*pow(1.0 - pow(Swe, 1.0/m_), m_ - 1.0)*pow(Swe, 1.0/m_ - 0.5)*factor;
    }
  }
  else if (phase_name == "non wetting") {
    if (Swe < 1.0e-09) {
      return -0.5 * factor;
    } else if(Swe - 1.0 > -1.0e-09) {
      return 0.0;
    } else {
      return -factor*0.5*pow(1.0 - Swe,-0.5)*pow(1.0 - pow(Swe, 1.0/m_), 2.0*m_) - 
      factor*pow(1.0 - Swe, 0.5)*2.0*pow(1.0 - pow(Swe, 1.0/m_), 2.0*m_ - 1.0)*pow(Swe, 1.0/m_ - 1.0);
    }
  }
  else {
    msg << "Multiphase PK: phase_name \"" << phase_name.c_str() << "\" not recognized \n";
    Exceptions::amanzi_throw(msg);
  }
}


/* ******************************************************************
* Capillary Pressure formula. 
****************************************************************** */
double WRMmp_VanGenuchten::capillaryPressure(double Sw)
{
  double Sn = 1.0 - Sw;
  if (Sn - S_rn_ < 1e-15) {
    return mod_VGM(S_rn_) + deriv_mod_VGM(S_rn_) * (Sn - S_rn_);
  } else if (Sn + S_rw_ - 1.0 > -1e-15) {
    return mod_VGM(1.0 - S_rw_) + deriv_mod_VGM(1.0 - S_rw_) * (Sn - 1.0 + S_rw_);
  } else {
    return mod_VGM(Sn);
  }
}


/* ******************************************************************
* Derivative of capillary pressure formula. 
****************************************************************** */
double WRMmp_VanGenuchten::dPc_dS(double Sw)
{
  double Sn = 1.0 - Sw;
  if (Sn - S_rn_ < 1e-15) {
    return -deriv_mod_VGM(S_rn_);
  } else if (Sn + S_rw_ - 1.0 > -1e-15) {
    return -deriv_mod_VGM(1.0 - S_rw_);
  } else {
    return -deriv_mod_VGM(Sn); // wrt Sw
  }
}



/* ******************************************************************
* Return irreducible residual saturation of the phase.                                          
****************************************************************** */
double WRMmp_VanGenuchten::VGM(double Sn) {
  double Sw = 1.0 - Sn;
  double Swe = (Sw - S_rw_)/(1.0 - S_rw_ - S_rn_);
  return Pr_ * pow(pow(Swe, -1.0/m_) - 1.0, 1.0/n_);
}

double WRMmp_VanGenuchten::mod_VGM(double Sn) {
  double s_mod = S_rn_ + (1.0 - eps_) * (Sn - S_rn_) + eps_/2.0 * (1.0 - S_rw_ - S_rn_);
  return VGM(s_mod) - VGM(S_rn_ + eps_/2 * (1.0 - S_rw_ - S_rn_));
}

double WRMmp_VanGenuchten::deriv_VGM(double Sn) { // wrt Sn
  double Sw = 1.0 - Sn;
  double Swe = (Sw - S_rw_)/(1.0 - S_rw_ - S_rn_);
  return Pr_ / (m_ * n_) * pow(pow(Swe, -1.0/m_) - 1.0, 1.0/n_ - 1.0) * pow(Swe, -1.0/m_ - 1.0) / (1.0 - S_rw_ - S_rn_);
}

double WRMmp_VanGenuchten::deriv_mod_VGM(double Sn) { // wrt Sn
  double s_mod = S_rn_ + (1.0 - eps_) * (Sn - S_rn_) + eps_/2.0 * (1.0 - S_rw_ - S_rn_);
  return (1.0 - eps_) * deriv_VGM(s_mod);
}

}  // namespace Multiphase
}  // namespace Amanzi
