/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

/* -------------------------------------------------------------------------


Author: Ethan Coon

Default base with a few methods implemented in standard ways.
------------------------------------------------------------------------- */

#ifndef AMANZI_PK_PHYSICAL_BASE_HH_
#define AMANZI_PK_PHYSICAL_BASE_HH_

#include "Teuchos_ParameterList.hpp"

#include "VerboseObject.hh"
#include "primary_variable_field_evaluator.hh"
#include "PK_Default.hh"
#include "Debugger.hh"

namespace Amanzi {

class PK_Physical : public virtual PK_Default {

 public:

  PK_Physical(){};

  PK_Physical(Teuchos::ParameterList& pk_tree,
              const Teuchos::RCP<Teuchos::ParameterList>& glist,
              const Teuchos::RCP<State>& S,
              const Teuchos::RCP<TreeVector>& soln){};

  PK_Physical(const Teuchos::RCP<Teuchos::ParameterList>& plist,
              Teuchos::ParameterList& FElist,
              const Teuchos::RCP<TreeVector>& solution):
    PK_Default(plist, FElist, solution){};


  // Virtual destructor
  virtual ~PK_Physical() {}

  // Default implementations of PK methods.
  // -- transfer operators -- pointer copies only
  // virtual void state_to_solution(const Teuchos::RCP<State>& S,
  //                                TreeVector& soln);
  // virtual void solution_to_state(TreeVector& soln,
  //                                const Teuchos::RCP<State>& S);

  // new virtual set_states() to also get the primary field evaulator.
  // virtual void set_states(const Teuchos::RCP<const State>& S,
  //         const Teuchos::RCP<State>& S_inter,
  //         const Teuchos::RCP<State>& S_next);


  // -- setup
  //virtual void Setup(const Teuchos::Ptr<State>& S){};

  // -- initialize
  //virtual void Initialize(const Teuchos::Ptr<State>& S){};

  // Accessor for debugger, for use by coupling MPCs
  //  Teuchos::RCP<Debugger> debugger() { return db_; }

 protected: // methods
  //  void DeriveFaceValuesFromCellValues_(const Teuchos::Ptr<CompositeVector>& cv);

 protected: // data
  // name of domain, associated mesh
  Teuchos::RCP<const AmanziMesh::Mesh> mesh_;
  std::string domain_;

  // solution and evaluator
  std::string key_;
  Teuchos::RCP<PrimaryVariableFieldEvaluator> solution_evaluator_;

  // debugger for dumping vectors
  Teuchos::RCP<Debugger> db_;

  //ENORM struct
  typedef struct ENorm_t {
    double value;
    int gid;
  } ENorm_t;

};

} // namespace

#endif