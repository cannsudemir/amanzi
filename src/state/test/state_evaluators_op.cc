/*
  State

  Authors: Ethan Coon
*/

#include "Epetra_MpiComm.h"
#include "Epetra_Vector.h"
#include "Teuchos_ParameterXMLFileReader.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_RCP.hpp"
#include "UnitTest++.h"

#include "Mesh.hh"
#include "MeshFactory.hh"
#include "TensorVector.hh"
#include "Op_Factory.hh"
#include "Op_Cell_Cell.hh"
#include "Op_Face_Cell.hh"
#include "Operator_Factory.hh"
#include "Operator.hh"
#include "BCs_Factory.hh"
#include "PDE_DiffusionFV.hh"
#include "evaluator/EvaluatorPrimary.hh"
#include "evaluator/EvaluatorSecondary.hh"
#include "evaluator/EvaluatorSecondaries.hh"
#include "evaluator/EvaluatorIndependent.hh"
#include "evaluator/Evaluator_OperatorApply.hh"
#include "State.hh"

using namespace Amanzi;
using namespace Amanzi::AmanziMesh;

class BIndependent : public EvaluatorIndependent<CompositeVector,CompositeVectorSpace> {
 public:
  using EvaluatorIndependent<CompositeVector,CompositeVectorSpace>::EvaluatorIndependent;

  virtual Teuchos::RCP<Evaluator> Clone() const override {
    return Teuchos::rcp(new BIndependent(*this)); };

 protected:
  virtual void Update_(State& s) override {
    double cv = s.GetMesh()->cell_volume(0);
    s.GetW<CompositeVector>(my_key_, my_tag_, my_key_).PutScalar(-4.*cv);
  }
};

class XIndependent : public EvaluatorIndependent<CompositeVector,CompositeVectorSpace> {
 public:
  using EvaluatorIndependent<CompositeVector,CompositeVectorSpace>::EvaluatorIndependent;

  virtual Teuchos::RCP<Evaluator> Clone() const override {
    return Teuchos::rcp(new XIndependent(*this)); };

 protected:
  virtual void Update_(State& s) override {
    auto& x = s.GetW<CompositeVector>(my_key_, my_tag_, my_key_);
    auto& x_c = *x.ViewComponent("cell",false);
    for (int c=0; c!=x_c.MyLength(); ++c) {
      AmanziGeometry::Point cc = x.Mesh()->cell_centroid(c);
      x_c[0][c] = cc[0]*cc[0] + cc[1]*cc[1]; // x^2 + y^2      
    }
    
  }
};

class DiagIndependent : public EvaluatorIndependent<CompositeVector,CompositeVectorSpace> {
 public:
  using EvaluatorIndependent<CompositeVector,CompositeVectorSpace>::EvaluatorIndependent;

  virtual Teuchos::RCP<Evaluator> Clone() const override {
    return Teuchos::rcp(new DiagIndependent(*this)); };

 protected:
  virtual void Update_(State& s) override {
    s.GetW<CompositeVector>(my_key_, my_tag_, my_key_).PutScalar(1.);
  }
};

class KIndependent : public EvaluatorIndependent<TensorVector,TensorVector_Factory> {
 public:
  using EvaluatorIndependent<TensorVector,TensorVector_Factory>::EvaluatorIndependent;

  virtual Teuchos::RCP<Evaluator> Clone() const override {
    return Teuchos::rcp(new KIndependent(*this)); };

 protected:
  virtual void Update_(State& s) override {
    auto& K = s.GetW<TensorVector>(my_key_, my_tag_, my_key_);
    for (auto& k : K) {
      k.Init(2, 0);
      k.PutScalar(1.0);
    }
  }
};

class BCsIndependent : public EvaluatorIndependent<Operators::BCs,Operators::BCs_Factory> {
 public:
  using EvaluatorIndependent<Operators::BCs,Operators::BCs_Factory>::EvaluatorIndependent;

  virtual Teuchos::RCP<Evaluator> Clone() const override {
    return Teuchos::rcp(new BCsIndependent(*this)); };

 protected:
  virtual void Update_(State& s) override {
    auto& bcs = s.GetW<Operators::BCs>(my_key_, my_tag_, my_key_);

    auto mesh = bcs.mesh();
    auto kind = bcs.kind();

    auto& model = bcs.bc_model();
    auto& value = bcs.bc_value();
    
    for (auto& bc : model) bc = Operators::OPERATOR_BC_NONE;
    for (auto& val : value) val = 0.;

    // set all exterior faces to dirichlet 0
    int nfaces_owned = mesh->num_entities(AmanziMesh::FACE, AmanziMesh::OWNED);
    AmanziMesh::Entity_ID_List cells;
    for (int f=0; f!= nfaces_owned; ++f) {
      mesh->face_get_cells(f, AmanziMesh::USED, &cells);
      if (cells.size() == 1) {
        model[f] = Operators::OPERATOR_BC_DIRICHLET;
        auto fc = mesh->face_centroid(f);
        value[f] = fc[0]*fc[0] + fc[1]*fc[1]; // x^2 + y^2
      }
    }    
  }
};

class Evaluator_PDE_Diagonal : public EvaluatorSecondary<Operators::Op,Operators::Op_Factory<Operators::Op_Cell_Cell>> {
 public:
  using EvaluatorSecondary<Operators::Op,Operators::Op_Factory<Operators::Op_Cell_Cell>>::EvaluatorSecondary;

  virtual Teuchos::RCP<Evaluator> Clone() const override {
    return Teuchos::rcp(new Evaluator_PDE_Diagonal(*this)); };

 protected:
  virtual void Evaluate_(const State& s, Operators::Op& op) override {
    *op.diag = *s.Get<CompositeVector>(dependencies_.begin()->first, dependencies_.begin()->second)
                .ViewComponent("cell", false);
  }

  virtual void EvaluatePartialDerivative_(const State& s, const Key& wrt_key, const Key& wrt_tag,
          Operators::Op& op) override {
    ASSERT(0);
  }
};

class Evaluator_PDE_DiffusionFV : public EvaluatorSecondaries {
 public:
  Evaluator_PDE_DiffusionFV(Teuchos::ParameterList& plist) :
      EvaluatorSecondaries(plist) {
    tag_ = plist.get<std::string>("tag");

    // my keys
    rhs_key_ = plist.get<std::string>("rhs key");
    local_op_key_ = plist.get<std::string>("local operator key");
    my_keys_.emplace_back(std::make_pair(local_op_key_, tag_));
    my_keys_.emplace_back(std::make_pair(rhs_key_, tag_));

    // dependencies
    tensor_coef_key_ = plist.get<std::string>("tensor coefficient key");
    scalar_coef_key_ = plist.get<std::string>("scalar coefficient key");
    bcs_key_ = plist.get<std::string>("boundary conditions key");
    source_key_ = plist.get<std::string>("source key");
    dependencies_.emplace_back(std::make_pair(tensor_coef_key_, tag_));
    dependencies_.emplace_back(std::make_pair(scalar_coef_key_, tag_));
    dependencies_.emplace_back(std::make_pair(bcs_key_, tag_));
    dependencies_.emplace_back(std::make_pair(source_key_, tag_));
  }

  virtual Teuchos::RCP<Evaluator> Clone() const override {
    return Teuchos::rcp(new Evaluator_PDE_DiffusionFV(*this)); };
  
  virtual bool IsDifferentiableWRT(const State& S, const Key& wrt_key, const Key& wrt_tag) const override {
    return false;
  }
    
  virtual void EnsureCompatibility(State& S) override {
    S.Require<Operators::Op,Operators::Op_Factory<Operators::Op_Face_Cell>>(
        local_op_key_, tag_, local_op_key_);
    S.Require<CompositeVector,CompositeVectorSpace>(rhs_key_, tag_, rhs_key_);

    S.Require<Operators::BCs,Operators::BCs_Factory>(bcs_key_, tag_);
    S.Require<CompositeVector,CompositeVectorSpace>(scalar_coef_key_, tag_);
    S.Require<TensorVector,TensorVector_Factory>(tensor_coef_key_, tag_);
    S.Require<CompositeVector,CompositeVectorSpace>(source_key_, tag_, source_key_);
    // could do lots of stuff here -- eg. check meshes, check spaces, etc
  }


  virtual void Update_(State& S) override {
    auto A_rhs = S.GetPtrW<CompositeVector>(rhs_key_, tag_, rhs_key_);
    *A_rhs = S.Get<CompositeVector>(source_key_, tag_);
    auto A_lop = S.GetPtrW<Operators::Op>(local_op_key_, tag_, local_op_key_);

    // create the global operator
    Operators::Operator_Factory global_op_fac;
    global_op_fac.set_mesh(A_rhs->Mesh());
    global_op_fac.set_cvs(A_rhs->Map(), A_rhs->Map());

    auto global_op_unique = global_op_fac.Create();
    // need to figure out a way to move unique_ptr into rcp
    // In the mean time, i don't think this will break the world.
    auto global_op = Teuchos::rcpFromRef(*global_op_unique);

    global_op->set_rhs(A_rhs);

    Teuchos::ParameterList plist;
    Operators::PDE_DiffusionFV pde(plist,global_op);
    pde.set_local_operator(A_lop);

    // These are explicitly spelled out to show the smell!
    // CAN THESE BCS BE MADE CONST or REF?  SMELL!
    Teuchos::RCP<Operators::BCs> bcs = S.GetPtrW<Operators::BCs>(bcs_key_, tag_, bcs_key_);
    pde.SetBCs(bcs, bcs);

    // CAN K BE MADE CONST OR REF? SMELL!
    auto& K = S.GetW<TensorVector>(tensor_coef_key_, tag_, tensor_coef_key_);
    Teuchos::RCP<std::vector<WhetStone::Tensor>> Kdata = Teuchos::rcpFromRef(K.data);
    pde.SetTensorCoefficient(Kdata);

    // at least this is const!
    Teuchos::RCP<const CompositeVector> kr = S.GetPtr<CompositeVector>(scalar_coef_key_, tag_);
    pde.SetScalarCoefficient(kr, Teuchos::null);

    // compute local ops
    pde.UpdateMatrices(Teuchos::null, Teuchos::null);
    pde.ApplyBCs(true, true);
  }


  virtual void UpdateDerivative_(State& S, const Key& wrt_key, const Key& wrt_tag) override {
    ASSERT(0);
  }
  
  
 protected:
  Key tag_;
  Key rhs_key_, local_op_key_;
  Key tensor_coef_key_, scalar_coef_key_, source_key_;
  Key bcs_key_;

};



SUITE(EVALUATOR_ON_OP) {

  // A test that simply tests the ability to put a Local Op into the DAG
  TEST(PRIMARY) {
    std::cout << "Local Op as a primary variable." << std::endl;
    auto comm = new Epetra_MpiComm(MPI_COMM_WORLD);
    MeshFactory meshfac(comm);
    auto mesh = meshfac(0.0, 0.0, 0.0, 4.0, 4.0, 4.0, 2, 2, 2);

    State S;
    S.RegisterDomainMesh(mesh);

    Teuchos::ParameterList es_list;
    es_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    es_list.setName("my_op");

    // require some op data
    auto& f = S.Require<Operators::Op, Operators::Op_Factory<Operators::Op_Cell_Cell>>("my_op", "", "my_op");
    f.set_mesh(mesh);
    f.set_name("cell");

    // make a primary evaluator for it
    auto op_eval = Teuchos::rcp(new EvaluatorPrimary<Operators::Op, Operators::Op_Factory<Operators::Op_Cell_Cell>>(es_list));
    S.SetEvaluator("my_op", op_eval);

    // Setup fields and marked as initialized.  Note: USER CODE SHOULD NOT DO IT THIS WAY!
    S.Setup();
    S.GetW<Operators::Op>("my_op", "", "my_op").diag->PutScalar(3.14);
    S.GetRecordW("my_op", "my_op").set_initialized();
    S.Initialize();
  }


  // Apply a diagonal global operator through the DAG
  TEST(OP_APPLY_DIAG) {
    auto comm = new Epetra_MpiComm(MPI_COMM_WORLD);
    MeshFactory meshfac(comm);
    auto mesh = meshfac(0.0, 0.0, 0.0, 4.0, 4.0, 4.0, 2, 2, 2);

    State S;
    S.RegisterDomainMesh(mesh);

    Teuchos::ParameterList es_list;
    es_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    es_list.setName("my_op");

    // require vector and primary evaluator for x
    S.Require<CompositeVector,CompositeVectorSpace>("x", "").SetMesh(mesh)
        ->SetGhosted(true)
        ->SetComponent("cell", AmanziMesh::CELL, 1);
    Teuchos::ParameterList xe_list;
    xe_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    xe_list.setName("x");
    auto x_eval = Teuchos::rcp(new EvaluatorPrimary<CompositeVector,CompositeVectorSpace>(xe_list));
    S.SetEvaluator("x", x_eval);

    // require vector and independent evaluator for source term b
    S.Require<CompositeVector,CompositeVectorSpace>("b", "").SetMesh(mesh)
        ->SetGhosted(true)
        ->SetComponent("cell", AmanziMesh::CELL, 1);
    Teuchos::ParameterList be_list;
    be_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    be_list.setName("b");
    auto b_eval = Teuchos::rcp(new BIndependent(be_list));
    S.SetEvaluator("b", b_eval);
    
    // require vector and independent evaluator for Diag(A)
    S.Require<CompositeVector,CompositeVectorSpace>("Diag", "").SetMesh(mesh)
        ->SetGhosted(true)
        ->SetComponent("cell", AmanziMesh::CELL, 1);
    Teuchos::ParameterList de_list;
    de_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    de_list.setName("Diag");
    auto D_eval = Teuchos::rcp(new DiagIndependent(de_list));
    S.SetEvaluator("Diag", D_eval);
    
    // require the local operator and evaluator
    auto& fac = S.Require<Operators::Op,Operators::Op_Factory<Operators::Op_Cell_Cell>>("Alocal", "");
    fac.set_mesh(mesh);
    Teuchos::ParameterList Ae_list;
    Ae_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    Ae_list.setName("Alocal");
    Ae_list.set("dependencies", Teuchos::Array<std::string>(1,"Diag"));
    Ae_list.set("dependency tags are my tag", true);
    auto A_eval = Teuchos::rcp(new Evaluator_PDE_Diagonal(Ae_list));
    S.SetEvaluator("Alocal", A_eval);
    
    // require vector and secondary evaluator for r = Ax - b
    S.Require<CompositeVector,CompositeVectorSpace>("residual", "").SetMesh(mesh)
        ->SetGhosted(true)
        ->SetComponent("cell", AmanziMesh::CELL, 1);
    Teuchos::ParameterList re_list;
    re_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    re_list.set("rhs key", "b");
    re_list.set("x key", "x");
    re_list.set("local operator keys", Teuchos::Array<std::string>(1,"Alocal"));
    re_list.setName("residual");
    auto r_eval = Teuchos::rcp(new Evaluator_OperatorApply(re_list));
    S.SetEvaluator("residual", r_eval);

    // Setup fields and marked as initialized.  Note: USER CODE SHOULD NOT DO IT THIS WAY!
    S.Setup();
    S.GetW<CompositeVector>("x", "", "x").PutScalar(1.);
    S.GetRecordW("x", "", "x").set_initialized();
    S.Initialize();

    // Update residual
    CHECK(S.GetEvaluator("residual")->Update(S, "pk"));

    // b - Ax
    CHECK_CLOSE(-33.0, (*S.Get<CompositeVector>("residual","").ViewComponent("cell", false))[0][0], 1.e-10);
    
  }


  // Apply a non-diagonal operator, including boundary conditions
  TEST(OP_APPLY_DIFFUSION) {
    auto comm = new Epetra_MpiComm(MPI_COMM_WORLD);
    MeshFactory meshfac(comm);
    auto mesh = meshfac(-1.0, -1.0, 1.0, 1.0, 80,80);

    State S;
    S.RegisterDomainMesh(mesh);

    Teuchos::ParameterList es_list;
    es_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    es_list.setName("my_op");

    // require vector and primary evaluator for x
    S.Require<CompositeVector,CompositeVectorSpace>("x", "").SetMesh(mesh)
        ->SetGhosted(true)
        ->SetComponent("cell", AmanziMesh::CELL, 1);
    Teuchos::ParameterList xe_list;
    xe_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    xe_list.setName("x");
    auto x_eval = Teuchos::rcp(new XIndependent(xe_list));
    S.SetEvaluator("x", x_eval);

    // require vector and independent evaluator for source term b
    auto& b_space = *S.Require<CompositeVector,CompositeVectorSpace>("b", "").SetMesh(mesh)
        ->SetGhosted(true)
        ->SetComponent("cell", AmanziMesh::CELL, 1);
    Teuchos::ParameterList be_list;
    be_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    be_list.setName("b");
    auto b_eval = Teuchos::rcp(new BIndependent(be_list));
    S.SetEvaluator("b", b_eval);

    // require vector and independent evaluator for Tensor
    auto& f = S.Require<TensorVector,TensorVector_Factory>("K", "");
    f.set_map(b_space); // cells
    f.set_rank(0);
    Teuchos::ParameterList Ke_list;
    Ke_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    Ke_list.setName("K");
    auto K_eval = Teuchos::rcp(new KIndependent(Ke_list));
    S.SetEvaluator("K", K_eval);
    
    // require vector and independent evaluator for kr (on faces!)
    S.Require<CompositeVector,CompositeVectorSpace>("k_relative", "").SetMesh(mesh)
        ->SetGhosted(true)
        ->SetComponent("face", AmanziMesh::FACE, 1);
    Teuchos::ParameterList kre_list;
    kre_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    kre_list.setName("k_relative");
    auto kr_eval = Teuchos::rcp(new DiagIndependent(kre_list));
    S.SetEvaluator("k_relative", kr_eval);
    
    // require boundary conditions
    auto& bc_fac = S.Require<Operators::BCs,Operators::BCs_Factory>("bcs", "");
    bc_fac.set_mesh(mesh);
    bc_fac.set_kind(AmanziMesh::FACE);
    bc_fac.set_type(Operators::SCHEMA_DOFS_SCALAR);
    Teuchos::ParameterList bce_list;
    bce_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    bce_list.setName("bcs");
    auto bc_eval = Teuchos::rcp(new BCsIndependent(bce_list));
    S.SetEvaluator("bcs", bc_eval);
    

    // require the local operator and rhs
    auto& Afac = S.Require<Operators::Op,Operators::Op_Factory<Operators::Op_Face_Cell>>("A_local", "");
    Afac.set_mesh(mesh);
    Afac.set_name("A_local");

    S.Require<CompositeVector,CompositeVectorSpace>("A_rhs", "").SetMesh(mesh)
        ->SetGhosted(true)
        ->SetComponent("cell", AmanziMesh::CELL, 1);
    
    Teuchos::ParameterList Ae_list;
    Ae_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    Ae_list.setName("A_local");
    Ae_list.set("tag", "");
    Ae_list.set("local operator key", "A_local");
    Ae_list.set("rhs key", "A_rhs");
    Ae_list.set("tensor coefficient key", "K");
    Ae_list.set("scalar coefficient key", "k_relative");
    Ae_list.set("boundary conditions key", "bcs");
    Ae_list.set("source key", "b");

    auto A_eval = Teuchos::rcp(new Evaluator_PDE_DiffusionFV(Ae_list));
    S.SetEvaluator("A_local", A_eval);
    S.SetEvaluator("A_rhs", A_eval);
    
    // require vector and secondary evaluator for r = Ax - b
    S.Require<CompositeVector,CompositeVectorSpace>("residual", "").SetMesh(mesh)
        ->SetGhosted(true)
        ->SetComponent("cell", AmanziMesh::CELL, 1);
    Teuchos::ParameterList re_list;
    re_list.sublist("verbose object").set<std::string>("verbosity level", "extreme");
    re_list.set("rhs key", "A_rhs");
    re_list.set("x key", "x");
    re_list.set("local operator keys", Teuchos::Array<std::string>(1,"A_local"));
    re_list.setName("residual");
    auto r_eval = Teuchos::rcp(new Evaluator_OperatorApply(re_list));
    S.SetEvaluator("residual", r_eval);

    // Setup fields and marked as initialized.  Note: USER CODE SHOULD NOT DO IT THIS WAY!
    S.Setup();
    S.Initialize();

    // Update residual
    CHECK(S.GetEvaluator("residual")->Update(S, "pk"));

    // b - Ax
    double error(0.);
    auto& r = *S.Get<CompositeVector>("residual","").ViewComponent("cell", false);
    r.NormInf(&error);
    CHECK_CLOSE(0.0, error, 1.e-3);
    
  }
  

}

