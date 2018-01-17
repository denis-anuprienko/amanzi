/* -*-  mode: c++; indent-tabs-mode: nil -*- */
/* -------------------------------------------------------------------------
ATS

License: see $ATS_DIR/COPYRIGHT
Author: Ethan Coon

A field evaluator with no dependencies specified by discrete data in an HDF5
file.

TODO: This needs a test and documentation! --etc
------------------------------------------------------------------------- */

#ifndef AMANZI_INDEPENDENT_FIELD_EVALUATOR_FROMFILE_
#define AMANZI_INDEPENDENT_FIELD_EVALUATOR_FROMFILE_

#include "EvaluatorIndependent.hh"
#include "Evaluator_Factory.hh"

namespace Amanzi {

class EvaluatorIndependentFromFile
    : public EvaluatorIndependent<CompositeVector, CompositeVectorSpace> {

public:
  // ---------------------------------------------------------------------------
  // Constructors
  // ---------------------------------------------------------------------------
  explicit EvaluatorIndependentFromFile(Teuchos::ParameterList &plist);

  EvaluatorIndependentFromFile(const EvaluatorIndependentFromFile &other) =
      default;

  virtual Teuchos::RCP<Evaluator> Clone() const override;

  virtual Evaluator &operator=(const Evaluator &other) override;

  EvaluatorIndependentFromFile &
  operator=(const EvaluatorIndependentFromFile &other);

  virtual void EnsureCompatibility(State &S) override;

protected:
  // ---------------------------------------------------------------------------
  // Update the value in the state.
  // ---------------------------------------------------------------------------
  virtual void Update_(State &S) override;

  // implementation
  void LoadFile_(int i);
  void Interpolate_(double time, CompositeVector &v);

protected:
  double t_before_, t_after_;
  Teuchos::RCP<CompositeVector> val_before_, val_after_;
  std::string filename_;
  std::vector<double> times_;
  int current_interval_;

  std::string meshname_;
  std::string compname_;
  std::string varname_;
  std::string locname_;
  int ndofs_;

  Teuchos::RCP<Function> time_func_;

private:
  static Utils::RegisteredFactory<Evaluator, EvaluatorIndependentFromFile> fac_;
};

} // namespace Amanzi

#endif
