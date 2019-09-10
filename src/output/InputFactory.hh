//! Factory for Input objects.
/*
  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Ethan Coon
*/

/*

*/

#ifndef AMANZI_INPUT_FACTORY_HH_
#define AMANZI_INPUT_FACTORY_HH_

#include "Teuchos_RCP.hpp"

#include "errors.hh"
#include "Input.hh"

namespace Amanzi {

namespace AmanziMesh {
class Mesh;
}

Teuchos::RCP<Input> CreateInput(Teuchos::ParameterList& plist,
        const Comm_ptr_type& comm);


} // namespace

#endif
