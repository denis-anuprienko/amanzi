/*
  This is the solver component of the Amanzi code.

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Ethan Coon (ecoon@lanl.gov)

  Interface to Newton solver.
*/

#ifndef AMANZI_NEWTON_SOLVER_
#define AMANZI_NEWTON_SOLVER_

#include "Teuchos_RCP.hpp"

#include "VerboseObject.hh"

#include "Solver.hh"
#include "SolverFnBase.hh"
#include "SolverDefs.hh"

namespace Amanzi {
namespace AmanziSolvers {

template<class Vector, class VectorSpace>
class SolverNewton : public Solver<Vector,VectorSpace> {
 public:
  SolverNewton(Teuchos::ParameterList& plist) :
      plist_(plist) {};

  SolverNewton(Teuchos::ParameterList& plist,
            const Teuchos::RCP<SolverFnBase<Vector> >& fn,
            const VectorSpace& map) :
      plist_(plist) {
    Init(fn, map);
  }

  void Init(const Teuchos::RCP<SolverFnBase<Vector> >& fn,
       const VectorSpace& map);

  virtual int Solve(const Teuchos::RCP<Vector>& u) {
    returned_code_ = Newton_(u);
    return (returned_code_ >= 0) ? 0 : 1;
  }

  // mutators
  void set_tolerance(double tol) { tol_ = tol; }
  void set_pc_lag(double pc_lag) {};  // Newton does not need it

  // access
  double tolerance() { return tol_; }
  double residual() { return residual_; }
  int num_itrs() { return num_itrs_; }
  int pc_calls() { return pc_calls_; }
  int pc_updates() { return pc_calls_; }
  int returned_code() { return returned_code_; }

 private:
  void Init_();

 protected:
  int Newton_(const Teuchos::RCP<Vector>& u);
  int Newton_ErrorControl_(double error, double previous_error, double l2_error,
                           double previous_du_norm, double du_norm);

 protected:
  Teuchos::ParameterList plist_;
  Teuchos::RCP<SolverFnBase<Vector> > fn_;

  Teuchos::RCP<VerboseObject> vo_;

 private:
  double tol_, overflow_tol_;

  int max_itrs_, num_itrs_, returned_code_;
  int fun_calls_, pc_calls_;
  int max_error_growth_factor_, max_du_growth_factor_;
  int max_divergence_count_, stagnation_itr_check_;

  int modify_correction_;
  double residual_;
  ConvergenceMonitor monitor_;
};


/* ******************************************************************
* Public Init method.
****************************************************************** */
template<class Vector, class VectorSpace>
void
SolverNewton<Vector, VectorSpace>::Init(
    const Teuchos::RCP<SolverFnBase<Vector> >& fn,
    const VectorSpace& map) {
  fn_ = fn;
  Init_();
}


/* ******************************************************************
* Initialization of the Newton solver
****************************************************************** */
template<class Vector, class VectorSpace>
void SolverNewton<Vector, VectorSpace>::Init_()
{
  tol_ = plist_.get<double>("nonlinear tolerance", 1.0e-6);
  overflow_tol_ = plist_.get<double>("diverged tolerance", 1.0e10);
  max_itrs_ = plist_.get<int>("limit iterations", 50);
  max_du_growth_factor_ = plist_.get<double>("max du growth factor", 1.0e5);
  max_error_growth_factor_ = plist_.get<double>("max error growth factor", 1.0e5);
  max_divergence_count_ = plist_.get<int>("max divergent iterations", 3);
  stagnation_itr_check_ = plist_.get<int>("stagnation iteration check", 8);
  modify_correction_ = plist_.get<bool>("modify correction", true);

  std::string monitor_name = plist_.get<std::string>("monitor", "monitor update");

  if (monitor_name == "monitor update") {
    monitor_ = SOLVER_MONITOR_UPDATE;
  } else {
    monitor_ = SOLVER_MONITOR_RESIDUAL;  // default value
  }

  fun_calls_ = 0;
  pc_calls_ = 0;

  residual_ = -1.0;

  // update the verbose options
  vo_ = Teuchos::rcp(new VerboseObject("Solver::Newton", plist_));
}


/* ******************************************************************
* Base Newton solver
****************************************************************** */
template<class Vector, class VectorSpace>
int SolverNewton<Vector, VectorSpace>::Newton_(const Teuchos::RCP<Vector>& u) {
  Teuchos::OSTab tab = vo_->getOSTab();

  // initialize the iteration and pc counters
  num_itrs_ = 0;
  pc_calls_ = 0;

  // create storage
  Teuchos::RCP<Vector> r = Teuchos::rcp(new Vector(*u));
  Teuchos::RCP<Vector> du = Teuchos::rcp(new Vector(*u));

  // variables to monitor the progress of the nonlinear solver
  double error(0.0), previous_error(0.0), l2_error(0.0);
  double l2_error_initial(0.0), u_norm(0.);
  double res_l2(0.0), res_inf(0.0);
  double du_l2(0.0), du_inf(0.0);
  double du_norm(1.0), previous_du_norm(1.0);
  int divergence_count(0);

  do {
    // Check for too many nonlinear iterations.
    if (num_itrs_ > max_itrs_) {
      if (vo_->os_OK(Teuchos::VERB_MEDIUM))
        *vo_->os() << "Solve reached maximum of iterations " << num_itrs_ 
                   << "  error=" << error << std::endl;
      return SOLVER_MAX_ITERATIONS;
    }

    // Update the preconditioner.
    pc_calls_++;
    fn_->UpdatePreconditioner(u);

    // Evaluate the nonlinear function.
    fun_calls_++;
    fn_->Residual(u, r);

    // If monitoring the residual, check for convergence.
    if (monitor_ == SOLVER_MONITOR_RESIDUAL) {
      previous_error = error;
      error = fn_->ErrorNorm(u, r);
      residual_ = error;

      r->Norm2(&l2_error);
      u->Norm2(&u_norm);
      *vo_->os() << "||u||=" << u_norm << " ||r||=" << l2_error << " error=" << error << "\n";

      // attempt to catch non-convergence early
      if (num_itrs_ == 1) {
        l2_error_initial = l2_error;
      } else if (num_itrs_ > stagnation_itr_check_) {
        if (l2_error > l2_error_initial) {
          if (vo_->getVerbLevel() >= Teuchos::VERB_HIGH) 
            *vo_->os() << "Solver stagnating, L2-error=" << l2_error
                       << " > " << l2_error_initial << " (initial L2-error)" << std::endl;
          return SOLVER_STAGNATING;
        }
      }

      int ierr = Newton_ErrorControl_(error, previous_error, l2_error, previous_du_norm, du_norm);
      if (ierr == SOLVER_CONVERGED) return num_itrs_;
      if (ierr != SOLVER_CONTINUE) return ierr;
    }

    // Increment iteration counter.
    num_itrs_++;

    // Apply the preconditioner to the nonlinear residual.
    pc_calls_++;
    r->Norm2(&res_l2);
    r->NormInf(&res_inf);

    int pc_error = fn_->ApplyPreconditioner(r, du);
    if (pc_error < 0) return SOLVER_LINEAR_SOLVER_ERROR;

    du->Norm2(&du_l2);
    du->NormInf(&du_inf);

    // Hack the correction
    if (modify_correction_) {
      bool hacked = fn_->ModifyCorrection(r, u, du);
    }

    // Make sure that we do not diverge and cause numerical overflow.
    previous_du_norm = du_norm;
    du->NormInf(&du_norm);

    if ((num_itrs_ > 1) && (du_norm > max_du_growth_factor_ * previous_du_norm)) {
      if (vo_->getVerbLevel() >= Teuchos::VERB_HIGH) 
        *vo_->os() << "Solver threatens to overflow: " << "  ||du||=" 
                   << du_norm << ", ||du_prev||=" << previous_du_norm << std::endl;

      // If it fails again, give up.
      if ((num_itrs_ > 1) && (du_norm > max_du_growth_factor_ * previous_du_norm)) {
        if (vo_->getVerbLevel() >= Teuchos::VERB_MEDIUM) 
           *vo_->os() << "Solver threatens to overflow: FAIL." << std::endl
                      << "||du||=" << du_norm << ", ||du_prev||=" << previous_du_norm << std::endl;
        return SOLVER_OVERFLOW;
      }
    }

    // Keep track of diverging iterations
    if (num_itrs_ > 1 && du_norm >= previous_du_norm) {
      // The solver is threatening to diverge.
      divergence_count++;

      // If it does not recover quickly, abort.
      if (divergence_count == max_divergence_count_) {
        if (vo_->getVerbLevel() >= Teuchos::VERB_LOW)
          *vo_->os() << "Solver is diverging repeatedly, FAIL." << std::endl;
        return SOLVER_DIVERGING;
      }
    } else {
      divergence_count = 0;
    }

    // Next solution iterate and error estimate: u  = u - du
    u->Update(-1.0, *du, 1.0);
    fn_->ChangedSolution();
    
    // If we monitor the update...
    if (monitor_ == SOLVER_MONITOR_UPDATE) {
      previous_error = error;
      error = fn_->ErrorNorm(u, du);
      residual_ = error;
      du->Norm2(&l2_error);

      int ierr = Newton_ErrorControl_(error, previous_error, l2_error, previous_du_norm, du_norm);
      if (ierr == SOLVER_CONVERGED) return num_itrs_;
      if (ierr != SOLVER_CONTINUE) return ierr;
    }
  } while (true);
}


/* ******************************************************************
* Internal error control
****************************************************************** */
template<class Vector, class VectorSpace>
int SolverNewton<Vector, VectorSpace>::Newton_ErrorControl_(double error, 
                                                            double previous_error, 
							    double l2_error, 
							    double previous_du_norm, 
							    double du_norm)
{
  if (vo_->getVerbLevel() >= Teuchos::VERB_HIGH) 
    *vo_->os() << num_itrs_ << ": error=" << error << "  L2-error=" << l2_error 
               << " contr. factor=" << du_norm/previous_du_norm << std::endl;

  if (error < tol_) {
    if (vo_->getVerbLevel() >= Teuchos::VERB_HIGH) 
      *vo_->os() << "Solver converged: " << num_itrs_ << " itrs, error=" << error << std::endl;
    return SOLVER_CONVERGED;
  } else if (error > overflow_tol_) {
    if (vo_->getVerbLevel() >= Teuchos::VERB_MEDIUM) 
      *vo_->os() << "Solve failed, error " << error << " > "
                 << overflow_tol_ << " (overflow)" << std::endl;
    return SOLVER_OVERFLOW;
  } else if ((num_itrs_ > 1) && (error > max_error_growth_factor_ * previous_error)) {
    if (vo_->getVerbLevel() >= Teuchos::VERB_MEDIUM) 
      *vo_->os() << "Solver threatens to overflow, error " << error << " > "
                 << previous_error << " (previous error)" << std::endl;
    return SOLVER_OVERFLOW;
  }
  return SOLVER_CONTINUE;
}

}  // namespace AmanziSolvers
}  // namespace Amanzi

#endif
