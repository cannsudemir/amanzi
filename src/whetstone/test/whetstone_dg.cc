/*
  WhetStone

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include <cstdlib>
#include <cmath>
#include <iostream>

#include "Teuchos_RCP.hpp"
#include "UnitTest++.h"

#include "Mesh.hh"
#include "MeshFactory.hh"

#include "DenseMatrix.hh"
#include "DG_Modal.hh"
#include "Polynomial.hh"


/* ****************************************************************
* Test of Taylor polynomials
**************************************************************** */
TEST(DG_TAYLOR_POLYNOMIALS) {
  using namespace Amanzi;
  using namespace Amanzi::WhetStone;

  // polynomials in two dimentions
  Polynomial p(2, 3);

  int i(0);
  for (auto it = p.begin(); it.end() <= p.end(); ++it) {
    const int* index = it.multi_index();
    CHECK(index[0] >= 0 && index[1] >= 0);

    int pos = p.PolynomialPosition(index);
    CHECK(pos == i++);

    int m = p.MonomialPosition(index);
    p.monomials(index[0] + index[1]).coefs()[m] = pos;
  }
  std::cout << p << std::endl; 
  CHECK(p.size() == 10);

  // re-define polynomials
  p.Reshape(2, 4);
  std::cout << p << std::endl; 
  CHECK(p.size() == 15);

  Polynomial p_tmp(p);
  p.Reshape(2, 2);
  std::cout << "Reshaping last polynomial\n" << p << std::endl; 
  CHECK(p.size() == 6);

  // operations with polynomials
  AmanziGeometry::Point xy(1.0, 0.0);
  double val = p.Value(xy) + p_tmp.Value(xy);

  p += p_tmp;
  CHECK(p.size() == 15);
  CHECK_CLOSE(p.Value(xy), val, 1e-12);

  // polynomials in 3D
  Polynomial q(3, 3);

  i = 0;
  for (auto it = q.begin(); it.end() <= q.end(); ++it) {
    const int* index = it.multi_index();
    CHECK(index[0] >= 0 && index[1] >= 0 && index[2] >= 0);

    int pos = q.PolynomialPosition(index);
    CHECK(pos == i++);

    int m = q.MonomialPosition(index);
    q.monomials(index[0] + index[1] + index[2]).coefs()[m] = pos;
  }
  std::cout << q << std::endl; 
  CHECK(q.size() == 20);
  Polynomial q_orig(q);

  // reshape polynomials
  q.Reshape(3, 2);
  Polynomial q1(q), q2(q), q3(q);
  std::cout << "Reshaping last 3D polynomial\n" << q << std::endl; 
  CHECK(q.size() == 10);

  q.Reshape(3, 3);
  std::cout << "Reshaping last 3D polynomial, q=\n" << q << std::endl; 
  CHECK(q.size() == 20);

  // ring operations with polynomials
  AmanziGeometry::Point xyz(1.0, 2.0, 3.0);
  val = q1.Value(xyz);
  q1 *= q2;
  CHECK_CLOSE(q1.Value(xyz), val * val, 1e-10);

  val = q2.Value(xyz);
  q2 *= q2;
  CHECK_CLOSE(q2.Value(xyz), val * val, 1e-10);

  Polynomial q4 = q2 - q3 * q3;
  CHECK_CLOSE(q4.Value(xyz), 0.0, 1e-10);

  // derivatives
  std::vector<Polynomial> grad;
  q_orig.Gradient(grad);
  std::cout << "Gradient:\n" << grad[0] << grad[1] << grad[2] << std::endl;

  // change coordinate system
  AmanziGeometry::Point origin(0.5, 0.5, 0.5);
  q.Reshape(3, 1);
  q.ChangeOrigin(origin);
  std::cout << "Changed origin of polynomial q\n" << q << std::endl; 
}


/* ****************************************************************
* Test of DG mass matrices: K is tensor
**************************************************************** */
TEST(DG_MASS_MATRIX) {
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::WhetStone;

  std::cout << "Test: DG mass matrices (tensors)" << std::endl;
  Epetra_MpiComm *comm = new Epetra_MpiComm(MPI_COMM_WORLD);

  MeshFactory meshfactory(comm);
  meshfactory.preference(FrameworkPreference({MSTK}));
  Teuchos::RCP<Mesh> mesh = meshfactory(0.0, 0.0, 0.5, 0.5, 1, 1); 

  DenseMatrix M;
  Tensor T(2, 1);
  T(0, 0) = 1.0;

  for (int k = 0; k < 3; k++) {
    DG_Modal dg(k, mesh);

    dg.MassMatrix(0, T, M);
    int nk = M.NumRows();

    printf("Mass matrix for order=%d\n", k);
    for (int i = 0; i < nk; i++) {
      for (int j = 0; j < nk; j++ ) printf("%9.5f ", M(i, j)); 
      printf("\n");
    }

    double area = mesh->cell_volume(0);
    CHECK_CLOSE(M(0, 0), area, 1e-12);
    if (k > 0) {
      CHECK_CLOSE(M(1, 1), area / 48, 1e-12);
    }
    if (k > 1) {
      CHECK_CLOSE(M(3, 3), area / 1280, 1e-12);
      // CHECK_CLOSE(M(4, 4), area / 144, 1e-12);
    }
  }

  delete comm;
}


/* ****************************************************************
* Test of DG mass matrices: K is polynomial
**************************************************************** */
TEST(DG_MASS_MATRIX_POLYNOMIAL) {
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::WhetStone;

  std::cout << "\nTest: DG mass matrices (polynomials)" << std::endl;
  Epetra_MpiComm *comm = new Epetra_MpiComm(MPI_COMM_WORLD);

  MeshFactory meshfactory(comm);
  meshfactory.preference(FrameworkPreference({MSTK}));
  // Teuchos::RCP<Mesh> mesh = meshfactory(0.0, 0.0, 1.0, 1.0, 1, 1); 
  Teuchos::RCP<Mesh> mesh = meshfactory("test/one_cell2.exo");
 
  double tmp, integral[2];
  DenseMatrix A;
  AmanziGeometry::Point zero(0.0, 0.0);

  for (int k = 0; k < 2; k++) {
    DG_Modal dg(1, mesh);

    Polynomial u(2, k);
    u.monomials(0).coefs()[0] = 1.0;
    u.monomials(k).coefs()[0] = 1.0;

    dg.MassMatrix(0, u, A);
    int nk = A.NumRows();

    printf("Mass matrix for polynomial of order=%d\n", k);
    for (int i = 0; i < nk; i++) {
      for (int j = 0; j < nk; j++ ) printf("%8.4f ", A(i, j)); 
      printf("\n");
    }

    // TEST1: accuracy
    DenseVector v(nk), av(nk);
    const AmanziGeometry::Point& xc = mesh->cell_centroid(0);

    v.PutScalar(0.0);
    v(0) = xc[0] + 2 * xc[1];
    v(1) = 1.0;
    v(2) = 2.0;
    
    A.Multiply(v, av, false);
    v.Dot(av, &tmp);
    integral[k] = tmp;
  }
  CHECK_CLOSE(20.2332916667, integral[0], 1e-10);
  CHECK(integral[0] < integral[1]);

  delete comm;
}


/* ****************************************************************
* Test of DG advection matrices on a face
**************************************************************** */
TEST(DG_ADVECTION_MATRIX_FACE) {
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::WhetStone;

  std::cout << "\nTest: DG advection matrices on faces" << std::endl;
  Epetra_MpiComm *comm = new Epetra_MpiComm(MPI_COMM_WORLD);

  MeshFactory meshfactory(comm);
  meshfactory.preference(FrameworkPreference({MSTK}));
  Teuchos::RCP<Mesh> mesh = meshfactory(0.0, 0.0, 1.0, 1.0, 2, 2); 
 
  AmanziGeometry::Point zero(0.0, 0.0);

  for (int k = 0; k < 2; k++) {
    DG_Modal dg(k, mesh);
    DenseMatrix A0, A1;

    Polynomial un(2, 0);
    un.monomials(0).coefs()[0] = 1.0;

    // TEST1: constant u
    dg.AdvectionMatrixFace(1, un, A0);

    printf("Advection matrix (face-based) for order=%d  u=constant\n", k);
    int nk = A0.NumRows();
    for (int i = 0; i < nk; i++) {
      for (int j = 0; j < nk; j++ ) printf("%8.4f ", A0(i, j)); 
      printf("\n");
    }

    // TEST2: linear u with zero gradient
    /*
    u.push_back(zero);
    u.push_back(zero);

    dg.AdvectionMatrixFace(1, u, A1);

    A1 -= A0;
    CHECK_CLOSE(0.0, A1.NormInf(), 1e-12);

    // TEST3: nonzero linear component of u
    u.clear();
    u.push_back(zero);
    u.push_back(zero);
    u.push_back(AmanziGeometry::Point(1.0, 0.0));

    dg.AdvectionMatrixFace(1, u, A1);

    printf("Advection matrix (cell-based) for order=%d u=(y-y0,0)\n", k);
    for (int i = 0; i < nk; i++) {
      for (int j = 0; j < nk; j++ ) printf("%8.4f ", A1(i, j)); 
      printf("\n");
    }
    */
  }

  delete comm;
}


/* ****************************************************************
* Test of polynomial approximation in cells
**************************************************************** */
TEST(DG_MAP_APPROXIMATION_CELL) {
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::WhetStone;

  std::cout << "\nTest: Polynomial approximation of map in cells." << std::endl;
  Epetra_MpiComm *comm = new Epetra_MpiComm(MPI_COMM_WORLD);

  MeshFactory meshfactory(comm);
  meshfactory.preference(FrameworkPreference({MSTK}));
  Teuchos::RCP<Mesh> mesh = meshfactory("test/one_cell2.exo");

  // extract polygon from the mesh
  Entity_ID_List nodes;
  AmanziGeometry::Point xv;
  std::vector<AmanziGeometry::Point> x1;

  mesh->cell_get_nodes(0, &nodes);

  for (int i = 0; i < nodes.size(); ++i) {
    mesh->node_get_coordinates(nodes[i], &xv);
    x1.push_back(xv);
  }

  // test identity map
  DG_Modal dg(1, mesh);
  std::vector<AmanziGeometry::Point> u;
  AmanziGeometry::Point ex(1.0, 0.0), ey(0.0, 1.0);

  dg.LeastSquareFit(x1, x1, u);
  CHECK_CLOSE(norm(u[0]), 0.0, 1e-12);
  CHECK_CLOSE(norm(u[1] - ex), 0.0, 1e-12);
  CHECK_CLOSE(norm(u[2] - ey), 0.0, 1e-12);

  // test linear map
  std::vector<AmanziGeometry::Point> x2(x1);
  AmanziGeometry::Point shift(0.1, 0.2);
  for (int i = 0; i < nodes.size(); ++i) {
    x2[i] += shift;
  }

  dg.LeastSquareFit(x1, x2, u);
  CHECK_CLOSE(norm(u[0] - shift), 0.0, 1e-12);
  CHECK_CLOSE(norm(u[1] - ex), 0.0, 1e-12);
  CHECK_CLOSE(norm(u[2] - ey), 0.0, 1e-12);

  // test rotation map
  double s(std::sin(0.3)), c(std::cos(0.3));
  for (int i = 0; i < nodes.size(); ++i) {
    x2[i][0] = c * x1[i][0] - s * x1[i][1];
    x2[i][1] = s * x1[i][0] + c * x1[i][1];
  }

  dg.LeastSquareFit(x1, x2, u);
  CHECK_CLOSE(norm(u[0]), 0.0, 1e-12);
  CHECK_CLOSE(norm(u[1] - AmanziGeometry::Point(c, s)), 0.0, 1e-12);
  CHECK_CLOSE(norm(u[2] - AmanziGeometry::Point(-s, c)), 0.0, 1e-12);

  // test non-linear deformation map
  x1.clear();
  x1.push_back(AmanziGeometry::Point(-0.5, -0.5));
  x1.push_back(AmanziGeometry::Point( 0.5, -0.5));
  x1.push_back(AmanziGeometry::Point(-0.5,  0.5));
  x1.push_back(AmanziGeometry::Point( 0.5,  0.5));

  x2 = x1;
  x2[3] += AmanziGeometry::Point(0.1, 0.1);

  dg.LeastSquareFit(x1, x2, u);

  for (int i = 0; i < u.size(); ++i) {
    printf("u[%d] = %8.4g %8.4g\n", i, u[i][0], u[i][1]); 
  }

  CHECK_CLOSE(0.025, u[0][0], 1e-12);

  delete comm;
}

