/*
  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Konstantin Lipnikov (amklinv@sandia.gov)
           Ethan Coon (coonet@ornl.gov)
*/
//!  Direct solvers via Trilinos.
/*!

.. warning:: undocumented

*/
#include "Epetra_LinearProblem.h"
#include "Amesos.h"

#include "Key.hh"
#include "InverseDefs.hh"
#include "VerboseObject.hh"

#include "DirectMethodAmesos.hh"

namespace Amanzi {
namespace AmanziSolvers {


/* ******************************************************************
* Initialization from a parameter list.
****************************************************************** */
void DirectMethodAmesos::InitInverse(Teuchos::ParameterList& plist)
{
  plist_ = plist;

  this->set_name(Keys::cleanPListName(plist.name()));
  solver_name_ = plist.get<std::string>("solver name", "Klu");
  std::string vo_name = this->name()+" (Amesos " + solver_name_ + ")";

  vo_ = Teuchos::rcp(new VerboseObject(vo_name, plist));

  Teuchos::OSTab tab = vo_->getOSTab();
  if (vo_->os_OK(Teuchos::VERB_EXTREME))
    *vo_->os() << "InitInverse()" << std::endl;
  
  inited_ = true;
}


/* ******************************************************************
* Update sets symbolic structure
****************************************************************** */
void DirectMethodAmesos::UpdateInverse() {
  Teuchos::OSTab tab = vo_->getOSTab();
  if (vo_->os_OK(Teuchos::VERB_EXTREME))
    *vo_->os() << "UpdateInverse()" << std::endl;

  AMANZI_ASSERT(inited_);
  AMANZI_ASSERT(h_.get());

  returned_code_ = 0;
  problem_ = Teuchos::rcp(new Epetra_LinearProblem());
  problem_->SetOperator(&*h_);

  Amesos factory;
  solver_ = Teuchos::rcp(factory.Create(solver_name_, *problem_));
  if (!solver_.get()) {
    Errors::Message msg;
    msg << "DirectMethodAmesos: solver \"" << solver_name_ << "\" is not available";
    Exceptions::amanzi_throw(msg);
  }
  solver_->SetParameters(plist_);

  int ierr = solver_->SymbolicFactorization();
  if (ierr > 0) {
    returned_code_ = -ierr;

    if (vo_->os_OK(Teuchos::VERB_MEDIUM))
      *vo_->os() << "SymbolicFactorization() failed with error code: " <<
          this->returned_code_string() << std::endl;

    // throw on this error?
    Errors::Message msg("DirectMethodAmesos: SymbolicFactorization failed");
    Exceptions::amanzi_throw(msg);
  }
  updated_ = true;
}


/* ******************************************************************
* Compute sets symbolic structure
****************************************************************** */
void DirectMethodAmesos::ComputeInverse() {
  Teuchos::OSTab tab = vo_->getOSTab();
  if (vo_->os_OK(Teuchos::VERB_EXTREME))
    *vo_->os() << "ComputeInverse()" << std::endl;

  AMANZI_ASSERT(updated_);

  if (returned_code_ == 0) {
    int ierr = solver_->NumericFactorization();
    if (ierr > 0) {
      returned_code_ = -ierr;

      if (vo_->os_OK(Teuchos::VERB_MEDIUM))
        *vo_->os() << "SymbolicFactorization() failed with error code: " <<
            this->returned_code_string() << std::endl;

      // don't throw on this error?
      //Errors::Message msg("DirectMethodAmesos: NumericFactorization failed");
      //Exceptions::amanzi_throw(msg);
    }
  }
  computed_ = true;
}


int DirectMethodAmesos::ApplyInverse(const Epetra_Vector& v, Epetra_Vector& hv) const
{
  AMANZI_ASSERT(computed_);

  // is this a mistake, or can Amesos change RHS?  That could be bad...
  Epetra_Vector* vv = const_cast<Epetra_Vector*>(&v);
  problem_->SetRHS(vv);
  problem_->SetLHS(&hv);
  
  if (returned_code_ == 0) {
    int ierr = solver_->Solve();
    if (ierr) {
      returned_code_ = -ierr;

      if (vo_->os_OK(Teuchos::VERB_MEDIUM))
        *vo_->os() << "ApplyInverse() failed with error code: " <<
            this->returned_code_string() << std::endl;
    } else {
      returned_code_ = 1;
      if (vo_->os_OK(Teuchos::VERB_MEDIUM))
        *vo_->os() << "ApplyInverse() succeeded." << std::endl;
    }
  }    
  return returned_code_;
}


std::string DirectMethodAmesos::returned_code_string() const
{
  switch(returned_code_) {
    case (1) :
      return "success";
    case (0) :
      return "not yet applied";
    case (-1) :
      return "singular matrix";
    case (-2) :
      return "non-symmetric matrix";
    case (-3) :
      return "non-positive-definite matrix";
    case (-4) :
      return "insufficient memory";
  }
  return "unknown error";
}


}  // namespace AmanziSolvers
}  // namespace Amanzi
