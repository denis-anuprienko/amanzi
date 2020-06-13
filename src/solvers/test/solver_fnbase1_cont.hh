/*
  Copyright 2010-201x held jointly by participating institutions.
  Amanzi is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors:

*/

//!

#ifndef AMANZI_TEST_SOLVER_FNBASE1_HH_
#define AMANZI_TEST_SOLVER_FNBASE1_HH_

#include <math.h>

#include "SolverFnBase.hh"

// ODE: f(u) = u (u^2 + 1) = 0.
class NonlinearProblem
  : public Amanzi::AmanziSolvers::SolverFnBase<Epetra_Vector> {
 public:
  NonlinearProblem(double atol, double rtol, bool exact_jacobian)
    : rtol_(rtol), atol_(atol), exact_jacobian_(exact_jacobian)
  {}

  void Residual(const Teuchos::RCP<Epetra_Vector>& u,
                const Teuchos::RCP<Epetra_Vector>& f)
  {
    for (int c = 0; c != u->getLocalLength(); ++c) {
      double x = (*u)[c];
      (*f)[c] = std::pow(x, lambda_) - 0.5;
      std::cout << "x=" << x << ", f=" << (*f)[c] << std::endl;
    }
  }

  int ApplyPreconditioner(const Teuchos::RCP<const Epetra_Vector>& u,
                          const Teuchos::RCP<Epetra_Vector>& hu)
  {
    return hu->ReciprocalMultiply(1.0, *h_, *u, 0.0);
  }

  double ErrorNorm(const Teuchos::RCP<const Epetra_Vector>& u,
                   const Teuchos::RCP<const Epetra_Vector>& du)
  {
    double norm_du, norm_u;
    norm_du = du->normInf();
    norm_u = u->normInf();
    return norm_du / (atol_ + rtol_ * norm_u);
  }

  void UpdatePreconditioner(const Teuchos::RCP<const Epetra_Vector>& up)
  {
    h_ = Teuchos::rcp(new Epetra_Vector(*up));

    if (exact_jacobian_) {
      for (int c = 0; c != up->getLocalLength(); ++c) {
        double x = (*up)[c];
        (*h_)[c] = lambda_ * std::pow(x, lambda_ - 1);
      }
    } else {
      for (int c = 0; c != up->getLocalLength(); ++c) {
        double x = (*up)[c];
        (*h_)[c] = 1.0;
      }
    }
  }

  void UpdateContinuationParameter(double lambda)
  {
    lambda_ = 3. - 2. * lambda;
    std::cout << "Updating lambda = " << lambda_ << std::endl;
  }

  void ChangedSolution(){};

 protected:
  double atol_, rtol_;
  bool exact_jacobian_;
  double lambda_;
  Teuchos::RCP<Epetra_Vector> h_; // preconditioner
};

#endif
