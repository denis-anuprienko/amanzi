/*
  MultiPhase PK

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Quan Bui (mquanbui@math.umd.edu)
           Konstantin Lipnikov (lipnikov@lanl.gov)

  Reduced multiphase model: 
    - water is only in liquid phase
    - water density is constant
    - porosity is constant
  Solution vectors: pressure (l), mole fraction, saturation (l).
*/

#ifndef AMANZI_MULTIPHASE_REDUCED_PK_HH_
#define AMANZI_MULTIPHASE_REDUCED_PK_HH_

// Amanzi
#include "PK_Factory.hh"

// Multiphase
#include "Multiphase_PK.hh"

namespace Amanzi {
namespace Multiphase {

class MultiphaseReduced_PK: public Multiphase_PK {
 public:
  MultiphaseReduced_PK(Teuchos::ParameterList& pk_tree,
                       const Teuchos::RCP<Teuchos::ParameterList>& global_list,
                       const Teuchos::RCP<State>& S,
                       const Teuchos::RCP<TreeVector>& soln);

  ~MultiphaseReduced_PK() {};

  // modifying interface for PKs
  virtual void Setup(const Teuchos::Ptr<State>& S) override;
  virtual std::string name() override { return "multiphase reduced"; }

  // interface multiphase models
  virtual void InitSolutionVector() override;
  virtual void InitPreconditioner() override;
  virtual void PopulateBCs() override;

 private:
  int missed_bc_faces_;

 private:
  // factory registration
  static RegisteredPKFactory<MultiphaseReduced_PK> reg_;
};

}  // namespace Multiphase
}  // namespace Amanzi
#endif
