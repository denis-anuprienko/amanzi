/*
This is the flow component of the Amanzi code. 
License: BSD
Authors: Neil Carlson (version 1)
         Konstantin Lipnikov (version 2) (lipnikov@lanl.gov)
*/

#ifndef __VAN_GENUCHTEN_MODEL_HPP__
#define __VAN_GENUCHTEN_MODEL_HPP__

#include "WaterRetentionModel.hpp"

namespace Amanzi {
namespace AmanziFlow {

class vanGenuchtenModel : public WaterRetentionModel {
 public:
  vanGenuchtenModel(std::string region_, double m_, double alpha_, double sr_, double atm_pressure_);
  ~vanGenuchtenModel() {};
  
  // requires methods from the base class
  double k_relative(double p);
  double saturation(double p);
  double d_saturation(double p);  

  void update_atm_pressure(double pressure) { atm_pressure = pressure; }
  double pressure(double saturation);

  // access methods
  const double get_atm_pressure() { return atm_pressure; }

 private:
  const double m;  // van Genuchten parameters: m, n, alpha
  double n; 
  const double alpha; 
  double atm_pressure;
  const double sr;  // van Genuchten effective saturation
};

}  // namespace AmanziFlow
}  // namespace Amanzi
 
#endif
