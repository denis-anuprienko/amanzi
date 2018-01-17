/* -*-  mode: c++; indent-tabs-mode: nil -*- */
/*
  ATS is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

//! A base class with default implementations of methods for a leaf of the PK
//! tree (a conservation equation, or similar).

/*!

``PK_MixinLeaf`` is a mixin class providing some functionality for PKs which
are defined on a single mesh, and represent a single process model.  Typically
all leaves of the PK tree will inherit from ``PK_MixinLeaf``.

* `"domain`" ``[string]`` **""**, e.g. `"surface`".

  Domains and meshes are 1-to-1, and the empty string refers to the main domain
or mesh.  PKs defined on other domains must specify which domain/mesh they refer
to.

* `"primary variable`" ``[string]``

  The primary variable associated with this PK, i.e. `"pressure`",
`"temperature`", `"surface_pressure`", etc.

*/

#ifndef AMANZI_PK_MIXIN_LEAF_HH_
#define AMANZI_PK_MIXIN_LEAF_HH_

#include "Teuchos_ParameterList.hpp"

#include "Debugger.hh"
#include "EvaluatorPrimary.hh"
#include "Key.hh"
#include "PK.hh"

namespace Amanzi {

template <class Base_t, class Data_t = CompositeVector,
          class DataFactory_t = CompositeVectorSpace>
class PK_MixinLeaf : public Base_t {
public:
  PK_MixinLeaf(const Teuchos::RCP<Teuchos::ParameterList> &pk_tree,
               const Teuchos::RCP<Teuchos::ParameterList> &global_plist,
               const Teuchos::RCP<State> &S);

  // Construct all sub-PKs.  Leaf PKs have no children, hence have nothing to
  // do
  void ConstructChildren(){};

  // require data
  void Setup();

  // Mark, as changed, any primary variable evaluator owned by this PK
  void ChangedSolutionPK(const Key &tag);

  void CommitStep(const Key &tag_old, const Key &tag_new);
  void FailStep(const Key &tag_old, const Key &tag_new);

  // Accessor for debugger, for use by coupling MPCs
  Teuchos::Ptr<Debugger> debugger() { return db_.ptr(); }

protected:
  void SolutionToState(const Key &tag, const Key &suffix);
  void StateToState(const Key &tag_from, const Key &tag_to);

protected:
  // name of domain, associated mesh
  Key domain_;
  Teuchos::RCP<const AmanziMesh::Mesh> mesh_;

  // solution and evaluator
  Key key_;

  // debugger for dumping vectors
  Teuchos::RCP<Debugger> db_;

  using Base_t::S_;
  using Base_t::plist_;
};

template <class Base_t, class Data_t, class DataFactory_t>
PK_MixinLeaf<Base_t, Data_t, DataFactory_t>::PK_MixinLeaf(
    const Teuchos::RCP<Teuchos::ParameterList> &pk_tree,
    const Teuchos::RCP<Teuchos::ParameterList> &global_plist,
    const Teuchos::RCP<State> &S)
    : Base_t(pk_tree, global_plist, S) {
  // get the domain and mesh
  domain_ = plist_->template get<std::string>("domain name");
  mesh_ = S->GetMesh(domain_);

  // get the primary variable key
  key_ = Keys::readKey(*plist_, domain_, "primary variable");

  // create the primary variable evaluator
  S->FEList().sublist(key_).set("evaluator type", "primary variable");

  // create a debugger
  db_ = Teuchos::rcp(new Debugger(mesh_, this->name(), *plist_));
};

template <class Base_t, class Data_t, class DataFactory_t>
void PK_MixinLeaf<Base_t, Data_t, DataFactory_t>::Setup() {
  // note that this will inherit all of its metadata from requirements made
  // elsewhere
  S_->template Require<Data_t, DataFactory_t>(key_, "", key_);
}

// Mark, as changed, any primary variable evaluator owned by this PK
template <class Base_t, class Data_t, class DataFactory_t>
void PK_MixinLeaf<Base_t, Data_t, DataFactory_t>::ChangedSolutionPK(
    const Key &tag) {
  auto eval = S_->GetEvaluator(key_, tag);
  auto eval_primary =
      Teuchos::rcp_dynamic_cast<EvaluatorPrimary<Data_t, DataFactory_t>>(eval);
  ASSERT(eval_primary.get());
  eval_primary->SetChanged();
};

template <class Base_t, class Data_t, class DataFactory_t>
void PK_MixinLeaf<Base_t, Data_t, DataFactory_t>::CommitStep(
    const Key &tag_old, const Key &tag_new) {
  Base_t::CommitStep(tag_old, tag_new);
  this->StateToState(tag_new, tag_old); // copy new into old
}

template <class Base_t, class Data_t, class DataFactory_t>
void PK_MixinLeaf<Base_t, Data_t, DataFactory_t>::FailStep(const Key &tag_old,
                                                           const Key &tag_new) {
  Base_t::FailStep(tag_old, tag_new);
  this->StateToState(tag_old, tag_new); // copy old into new

  // e.g. explicit PKs don't have an eval at the next tag.
  if (S_->HasEvaluator(key_, tag_new)) {
    this->ChangedSolutionPK(tag_new);
  }
}

template <class Base_t, class Data_t, class DataFactory_t>
void PK_MixinLeaf<Base_t, Data_t, DataFactory_t>::SolutionToState(
    const Key &tag, const Key &suffix) {
  Key key = key_ + suffix;
  S_->template Require<Data_t, DataFactory_t>(key, tag, key);
}

template <class Base_t, class Data_t, class DataFactory_t>
void PK_MixinLeaf<Base_t, Data_t, DataFactory_t>::StateToState(
    const Key &tag_from, const Key &tag_to) {
  S_->template GetW<Data_t>(key_, tag_to, key_) =
      S_->template Get<Data_t>(key_, tag_from);
}

template <class Base_t>
class PK_MixinLeafCompositeVector
    : public PK_MixinLeaf<Base_t, CompositeVector, CompositeVectorSpace> {
public:
  using PK_MixinLeaf<Base_t, CompositeVector,
                     CompositeVectorSpace>::PK_MixinLeaf;
  void StateToSolution(TreeVector &soln, const Key &tag, const Key &suffix);
};

template <class Base_t>
void PK_MixinLeafCompositeVector<Base_t>::StateToSolution(TreeVector &soln,
                                                          const Key &tag,
                                                          const Key &suffix) {
  Key key = this->key_ + suffix;
  if (soln.Data() == Teuchos::null)
    soln.SetData(this->S_->template GetPtrW<CompositeVector>(key, tag, key));

  // ASSERTS for now?
  ASSERT(soln.Data() == this->S_->template GetPtr<CompositeVector>(key, tag));
}

} // namespace Amanzi

#endif
