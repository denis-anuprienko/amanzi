/*
  Flow PK 

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov

  Self-registering factory for mulscale porosity models.
*/

#ifndef MULTISCALE_FLOW_POROSITY_FACTORY_HH_
#define MULTISCALE_FLOW_POROSITY_FACTORY_HH_

#include "Teuchos_ParameterList.hpp"

#include "MultiscaleFlowPorosity.hh"
#include "factory.hh"

namespace Amanzi {
namespace Flow {

class MultiscaleFlowPorosityFactory : public Utils::Factory<MultiscaleFlowPorosity> {
 public:
  Teuchos::RCP<MultiscaleFlowPorosity> Create(Teuchos::ParameterList& plist);
};

}  // namespace Flow
}  // namespace Amanzi

#endif