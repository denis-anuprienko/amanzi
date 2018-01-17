/* -*-  mode: c++; indent-tabs-mode: nil -*- */
/* -------------------------------------------------------------------------
Arcos

License: BSD
Author: Ethan Coon

An evaluator with no dependencies solved for by a PK.

------------------------------------------------------------------------- */

#ifndef AMANZI_STATE_EVALUATOR_PRIMARY_
#define AMANZI_STATE_EVALUATOR_PRIMARY_

#include <memory>

#include "Evaluator.hh"
#include "Evaluator_Factory.hh"

namespace Amanzi {

//
// Dummy class, does everything but know the type, which is required to
// UpdateDerivative.  This is never used, instead the below templated one
// is.
//
class EvaluatorPrimary_ : public Evaluator {

public:
  // ---------------------------------------------------------------------------
  // Constructors
  // ---------------------------------------------------------------------------
  explicit EvaluatorPrimary_(Teuchos::ParameterList &plist);
  EvaluatorPrimary_(const EvaluatorPrimary_ &other) = default;

  virtual Evaluator &operator=(const Evaluator &other) override;
  EvaluatorPrimary_ &operator=(const EvaluatorPrimary_ &other);

  // ---------------------------------------------------------------------------
  // Lazy evaluation of the evaluator.
  //
  // Updates the data, if needed.  Returns true if the value of the data has
  // changed since the last request for an update.
  // ---------------------------------------------------------------------------
  virtual bool Update(State &S, const Key &request) override final;

  // ---------------------------------------------------------------------------
  // Lazy evaluation of derivatives of evaluator.
  //
  // Updates the derivative, if needed.  Returns true if the value of the
  // derivative with respect to wrt_key has changed since the last request for
  // an update.
  // ---------------------------------------------------------------------------
  virtual bool UpdateDerivative(State &S, const Key &request,
                                const Key &wrt_key,
                                const Key &wrt_tag) override final;

  virtual bool IsDependency(const State &S, const Key &key,
                            const Key &tag) const override final;
  virtual bool ProvidesKey(const Key &key, const Key &tag) const override final;
  virtual bool IsDifferentiableWRT(const State &S, const Key &wrt_key,
                                   const Key &wrt_tag) const override final {
    return ProvidesKey(wrt_key, wrt_tag);
  }

  // ---------------------------------------------------------------------------
  // How a PK informs this leaf of the tree that it has changed.
  //
  // Effectively this simply tosses the request history, so that the next
  // requests will say this has changed.
  // ---------------------------------------------------------------------------
  void SetChanged();

  virtual std::string WriteToString() const override;

protected:
  virtual void UpdateDerivative_(State &S) = 0;

protected:
  Key my_key_;
  Key my_tag_;
  KeySet requests_;
  KeyTripleSet deriv_requests_;

  bool deriv_once_;

  VerboseObject vo_;
};

//
// Class to set types that can do requirements for compatibility and
// derivatives.
//
template <typename Data_t, typename DataFactory_t = NullFactory>
class EvaluatorPrimary : public EvaluatorPrimary_ {
public:
  using EvaluatorPrimary_::EvaluatorPrimary_;

  virtual Teuchos::RCP<Evaluator> Clone() const override final {
    return Teuchos::rcp(new EvaluatorPrimary<Data_t, DataFactory_t>(*this));
  }

  virtual void EnsureCompatibility(State &S) override final {
    S.Require<Data_t, DataFactory_t>(my_key_, my_tag_, my_key_);
  }

protected:
  virtual void UpdateDerivative_(State &S) override final {
    Errors::Message message("EvaluatorPrimary::UpdateDerivative_ not "
                            "implemented for arbitrary types");
    throw(message);
  }

private:
  static Utils::RegisteredFactory<Evaluator,
                                  EvaluatorPrimary<Data_t, DataFactory_t>>
      fac_;
};

template <> inline void EvaluatorPrimary<double>::UpdateDerivative_(State &s) {
  if (!s.HasDerivativeData(my_key_, my_tag_, my_key_, my_tag_)) {
    s.RequireDerivative(my_key_, my_tag_, my_key_, my_tag_, my_key_);
  }
  s.GetDerivativeW<double>(my_key_, my_tag_, my_key_, my_tag_, my_key_) = 1.0;
}

template <>
inline void
EvaluatorPrimary<CompositeVector, CompositeVectorSpace>::UpdateDerivative_(
    State &s) {
  if (!s.HasDerivativeData(my_key_, my_tag_, my_key_, my_tag_)) {
    s.RequireDerivative(my_key_, my_tag_, my_key_, my_tag_, my_key_);
  }
  s.GetDerivativeW<CompositeVector>(my_key_, my_tag_, my_key_, my_tag_, my_key_)
      .PutScalar(1.0);
}

} // namespace Amanzi

#endif
