/*
This is the flow component of the Amanzi code. 

Copyright 2010-2013 held jointly by LANS/LANL, LBNL, and PNNL. 
Amanzi is released under the three-clause BSD License. 
The terms of use and "as is" disclaimer for this license are 
provided in the top-level COPYRIGHT file.

Authors: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#ifndef AMANZI_RELATIVE_PERMEABILITY_HH_
#define AMANZI_RELATIVE_PERMEABILITY_HH_

#include <string>
#include <vector>

#include "Epetra_IntVector.h"
#include "Epetra_Vector.h"
#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"

#include "VerboseObject.hh"
#include "Mesh.hh"
#include "tensor.hh"

#include "WaterRetentionModel.hh"
#include "Flow_State.hh"
#include "Flow_typedefs.hh"

namespace Amanzi {
namespace AmanziFlow {

class RelativePermeability {
 public:
  RelativePermeability(const Teuchos::RCP<const AmanziMesh::Mesh>& mesh,
                       Teuchos::ParameterList& list)
      : mesh_(mesh), list_(list) {};
  ~RelativePermeability() {};

  // main methods
  void Init(double p0, const Teuchos::RCP<Flow_State> FS);
  void ProcessParameterList();

  void Compute(const Epetra_Vector& p, 
                const std::vector<int>& bc_model, const std::vector<bc_tuple>& bc_values);
  void ComputeInCells(const Epetra_Vector& p);
  void ComputeOnFaces(const Epetra_Vector& p,
                      const std::vector<int>& bc_model, const std::vector<bc_tuple>& bc_values);
  void ComputeDerivativeOnFaces(
      const Epetra_Vector& p,
      const std::vector<int>& bc_model, const std::vector<bc_tuple>& bc_values);

  void DerivedSdP(const Epetra_Vector& p, Epetra_Vector& ds);
  void DerivedKdP(const Epetra_Vector& p, Epetra_Vector& dk);

  void CalculateKVectorUnit(const std::vector<WhetStone::Tensor>& K, const AmanziGeometry::Point& g);
  void SetFullySaturated();
  void PopulateMapC2MB();

  void VerifyWRMparameters(double m, double alpha, double sr, double pc0);
  void VerifyStringMualemBurdine(const std::string name);
  void ProcessStringRelativePermeability(const std::string name);
  void PlotWRMcurves();

  // access methods
  std::vector<Teuchos::RCP<WaterRetentionModel> >& WRM() { return WRM_; }

  Epetra_Vector& Krel_cells() { return *Krel_cells_; }
  Epetra_Vector& Krel_faces() { return *Krel_faces_; }
  Teuchos::RCP<Epetra_Vector>& Krel_cells_ptr() { return Krel_cells_;}
  Teuchos::RCP<Epetra_Vector>& Krel_faces_ptr() { return Krel_faces_;}

  std::vector<std::vector<double> >& Krel_amanzi() { return Krel_amanzi_; }
  const Epetra_Vector& dKdP_cells() { return *dKdP_cells_; }
  const Epetra_Vector& dKdP_faces() { return *dKdP_faces_; }
  std::vector<AmanziGeometry::Point >& Kgravity_unit() {return Kgravity_unit_;}

  int method() { return method_; }
  Epetra_Vector& map_c2mb() { return *map_c2mb_; }
  void set_experimental_solver(int solver) { experimental_solver_ = solver; }

 private:
  void FaceArithmeticMean_(const Epetra_Vector& p);
  void FaceUpwindGravityInit_();
  void FaceUpwindGravityInit_(const AmanziGeometry::Point& g);
  void FaceUpwindGravity_(
      const Epetra_Vector& p,
      const std::vector<int>& bc_model, const std::vector<bc_tuple>& bc_values);
  void FaceUpwindGravityInSoil_(
      const Epetra_Vector& p,
      const std::vector<int>& bc_model, const std::vector<bc_tuple>& bc_values);
  void FaceUpwindFlux_(
      const Epetra_Vector& p, const Epetra_Vector& flux,
      const std::vector<int>& bc_model, const std::vector<bc_tuple>& bc_values);

  void DerivativeFaceUpwindGravity_(
      const Epetra_Vector& p,
      const std::vector<int>& bc_model, const std::vector<bc_tuple>& bc_values);
  void DerivativeFaceUpwindFlux_(
      const Epetra_Vector& p, const Epetra_Vector& flux,
      const std::vector<int>& bc_model, const std::vector<bc_tuple>& bc_values);

 protected:
  VerboseObject* vo_;

 private:
  Teuchos::RCP<const AmanziMesh::Mesh> mesh_;
  Teuchos::ParameterList list_;

  std::vector<Teuchos::RCP<WaterRetentionModel> > WRM_;
  double atm_pressure;

  int ncells_owned, ncells_wghost;
  int nfaces_owned, nfaces_wghost;

  int method_;  // method for calculating relative permeability
  Teuchos::RCP<Epetra_Vector> Krel_cells_;  // realitive permeability 
  Teuchos::RCP<Epetra_Vector> Krel_faces_;
  Teuchos::RCP<Epetra_Vector> dKdP_cells_;  // derivative of realitive permeability 
  Teuchos::RCP<Epetra_Vector> dKdP_faces_;

  Teuchos::RCP<Epetra_IntVector> upwind_cell;
  Teuchos::RCP<Epetra_IntVector> downwind_cell;
  Teuchos::RCP<Epetra_IntVector> face_flag;
  std::vector<std::vector<double> > Krel_amanzi_;

  std::vector<AmanziGeometry::Point> Kgravity_unit_;  // normalized vector Kg

  // Miscallenous maps
  Teuchos::RCP<Epetra_Vector> map_c2mb_;

  // obsolete, must go away (lipnikov@lanl.gov)
  int experimental_solver_; 
  Teuchos::RCP<Flow_State> FS_;
};

}  // namespace AmanziFlow
}  // namespace Amanzi

#endif

