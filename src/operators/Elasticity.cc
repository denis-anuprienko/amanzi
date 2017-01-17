/*
  Operators 

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include <vector>

// TPLs
#include "Epetra_Vector.h"

// Amanzi
#include "errors.hh"
#include "MatrixFE.hh"
#include "mfd3d_elasticity.hh"
#include "PreconditionerFactory.hh"
#include "WhetStoneDefs.hh"

// Amanzi::Operators
#include "Elasticity.hh"
#include "Op.hh"
#include "Op_Cell_Schema.hh"
#include "OperatorDefs.hh"
#include "Operator_Schema.hh"
#include "Schema.hh"

namespace Amanzi {
namespace Operators {

/* ******************************************************************
* Initialization of the operator, scalar coefficient.
****************************************************************** */
void Elasticity::SetTensorCoefficient(
    const Teuchos::RCP<std::vector<WhetStone::Tensor> >& K)
{
  K_ = K;
}


/* ******************************************************************
* Calculate elemental matrices.
****************************************************************** */
void Elasticity::UpdateMatrices()
{
  WhetStone::MFD3D_Elasticity mfd(mesh_);
  AmanziMesh::Entity_ID_List nodes;
  int d = mesh_->space_dimension();

  WhetStone::Tensor Kc(mesh_->space_dimension(), 1);
  Kc(0, 0) = 1.0;
  
  for (int c = 0; c < ncells_owned; c++) {
    if (method_ == BERNARDI_RAUGEL) {
      mesh_->cell_get_nodes(c, &nodes);
      int nnodes = nodes.size();
      int nfaces = mesh_->cell_get_num_faces(c);
      int ndofs = d * nnodes + nfaces;

      WhetStone::DenseMatrix Acell(ndofs, ndofs);
      if (K_.get()) Kc = (*K_)[c];
      mfd.StiffnessMatrixBernardiRaugel(c, Kc, Acell);

      local_op_->matrices[c] = Acell;
    }
  }
}


/* ******************************************************************
* Apply boundary conditions to the local matrices. We always zero-out
* matrix rows for essential test BCs. As to trial BCs, there are
* options: (a) eliminate or not, (b) if eliminate, then put 1 on
* the diagonal or not.
****************************************************************** */
void Elasticity::ApplyBCs(bool primary, bool eliminate)
{
  for (auto bc = bcs_trial_.begin(); bc != bcs_trial_.end(); ++bc) {
    if ((*bc)->type() == OPERATOR_BC_TYPE_FACE) {
      ApplyBCs_Face(bc->ptr(), local_op_, primary, eliminate);
    } 
    else if ((*bc)->type() == OPERATOR_BC_TYPE_NODE) {
      ApplyBCs_Node(bc->ptr(), local_op_, primary, eliminate);
    }
  }
}


/* ******************************************************************
* Put here stuff that has to be done in constructor.
****************************************************************** */
void Elasticity::InitElasticity_(Teuchos::ParameterList& plist)
{
  // Read schema for the mimetic discretization method.
  Schema my_schema;
  Teuchos::ParameterList& schema_list = plist.sublist("schema");
  my_schema.Init(schema_list, mesh_);

  // create or check the existing Operator
  local_schema_row_ = my_schema;
  local_schema_row_ = my_schema;

  if (global_op_ == Teuchos::null) {
    global_schema_col_ = my_schema;
    global_schema_row_ = my_schema;

    // build the CVS from the global schema
    Teuchos::RCP<CompositeVectorSpace> cvs = Teuchos::rcp(new CompositeVectorSpace());
    cvs->SetMesh(mesh_)->SetGhosted(true);

    for (auto it = my_schema.items().begin(); it != my_schema.items().end(); ++it) {
      std::string name(my_schema.KindToString(it->kind));
      cvs->AddComponent(name, it->kind, it->num);
    }

    global_op_ = Teuchos::rcp(new Operator_Schema(cvs, cvs, plist, my_schema, my_schema));
  } else {
    // constructor was given an Operator
    global_schema_col_ = global_op_->schema_col();
    global_schema_row_ = global_op_->schema_row();
    mesh_ = global_op_->DomainMap().Mesh();
  }

  // create the local Op and register it with the global Operator
  local_op_ = Teuchos::rcp(new Op_Cell_Schema(my_schema, my_schema, mesh_));
  global_op_->OpPushBack(local_op_);
  
  K_ = Teuchos::null;

  // discretization method
  std::string name = plist.get<std::string>("discretization", "none");
  if (name == "BernardiRaugel") {
    method_ = BERNARDI_RAUGEL;
  } else if (name == "BernardiRaugel" && local_schema_row_ != local_schema_col_) {
    method_ = BERNARDI_RAUGEL_P0;
  } else {
    Errors::Message msg;
    msg << "Name of the discretization method is missing.";
    Exceptions::amanzi_throw(msg);
  }
}

}  // namespace Operators
}  // namespace Amanzi
