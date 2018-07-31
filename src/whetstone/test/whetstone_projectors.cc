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
#include <vector>

// TPLs
#include "Teuchos_RCP.hpp"
#include "UnitTest++.h"

// Amanzi
#include "Mesh.hh"
#include "MeshFactory.hh"
#include "Point.hh"

// WhetStone
#include "MFD3D_CrouzeixRaviart.hh"
#include "MFD3D_CrouzeixRaviartSerendipity.hh"
#include "MFD3D_Diffusion.hh"
#include "MFD3D_Lagrange.hh"
#include "MFD3D_LagrangeSerendipity.hh"
#include "NumericalIntegration.hh"
#include "Tensor.hh"
#include "Polynomial.hh"


/* **************************************************************** */
TEST(PROJECTORS_SQUARE_CR) {
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::WhetStone;

  std::cout << "\nTest: Crouziex-Raviart harmonic projectors for square" << std::endl;
  Epetra_MpiComm *comm = new Epetra_MpiComm(MPI_COMM_WORLD);

  MeshFactory meshfactory(comm);
  meshfactory.preference(FrameworkPreference({MSTK}));
  Teuchos::RCP<const Amanzi::AmanziGeometry::GeometricModel> gm;
  Teuchos::RCP<Mesh> mesh = meshfactory(-1.2, 0.0, 1.2, 1.1, 2, 1, gm, true, true); 
 
  int cell(1);
  AmanziGeometry::Point zero(2);
  VectorPolynomial uc;
  std::vector<VectorPolynomial> vf(4);

  // test zero cell deformation
  std::cout << "      subtest: ZERO deformation" << std::endl;
  for (int n = 0; n < 4; ++n) {
    vf[n].resize(2);
    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 1, true);
    }
  }

  MFD3D_CrouzeixRaviart mfd(mesh);
  VectorPolynomial moments(2, 2);  // trivial polynomials p=0

  mfd.set_order(2);
  mfd.H1Cell(cell, vf, moments, uc);

  uc.ChangeOrigin(zero);
  CHECK(uc[0].NormMax() < 1e-12 && uc[1].NormMax() < 1e-12);

  // test linear deformation
  std::cout << "      subtest: LINEAR deformation" << std::endl;
  for (int n = 0; n < 4; ++n) {
    for (int i = 0; i < 2; ++i) {
      vf[n][i](0, 0) = 1.0;
      vf[n][i](1, 0) = 2.0;
      vf[n][i](1, 1) = 3.0;
    }
  }
  
  mfd.set_order(1);
  mfd.H1Cell(cell, vf, moments, uc);

  uc.ChangeOrigin(zero);
  std::cout << uc[0] << std::endl;

  uc[0] -= vf[0][0];
  uc[1] -= vf[0][1];
  CHECK(uc[0].NormMax() < 1e-12 && uc[1].NormMax() < 1e-12);

  for (int k = 2; k < 4; ++k) {
    moments[0].Reshape(2, k - 2, true);

    moments[0](0, 0) = 3.85;
    if (k > 2) {
      moments[0](1, 0) = 0.208893187146837;
      moments[0](1, 1) = 0.263292454632993;
    }
    moments[1] = moments[0];

    mfd.set_order(k);
    mfd.H1Cell(cell, vf, moments, uc);

    uc.ChangeOrigin(zero);
    std::cout << uc[1] << std::endl;

    uc[0] -= vf[0][0];
    uc[1] -= vf[0][1];
    CHECK(uc[0].NormMax() < 1e-11 && uc[1].NormMax() < 1e-11);
  }

  // test re-location of the right-top corner to (2,3)
  std::cout << "      subtest: BILINEAR deformation" << std::endl;
  for (int n = 0; n < 4; ++n) vf[n].PutScalar(0.0);
  vf[1][0](1, 1) = 0.8 / 1.1; 
  vf[1][1](1, 1) = 1.9 / 1.1; 

  vf[2][0](1, 0) = 0.8 / 1.2; 
  vf[2][1](1, 0) = 1.9 / 1.2; 

  moments[0].Reshape(2, 0, true);
  moments[1].Reshape(2, 0, true);

  moments[0](0, 0) = 0.2;
  moments[1](0, 0) = 0.475;

  mfd.set_order(2);
  mfd.H1Cell(cell, vf, moments, uc);

  uc.ChangeOrigin(zero);
  std::cout << uc << std::endl;

  auto p = AmanziGeometry::Point(1.2, 1.1);
  CHECK(fabs(uc[0].Value(p) - 0.8) < 1e-12 &&
        fabs(uc[1].Value(p) - 1.9) < 1e-12);

  delete comm;
}


/* **************************************************************** */
TEST(PROJECTORS_POLYGON_CR) {
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::WhetStone;

  std::cout << "\nTest: Crouzeix-Raviart harmonic projector for pentagon" << std::endl;
  Epetra_MpiComm *comm = new Epetra_MpiComm(MPI_COMM_WORLD);

  MeshFactory meshfactory(comm);
  meshfactory.preference(FrameworkPreference({MSTK}));
  Teuchos::RCP<const Amanzi::AmanziGeometry::GeometricModel> gm;
  Teuchos::RCP<Mesh> mesh = meshfactory("test/one_pentagon.exo", gm, true, true);
  // Teuchos::RCP<Mesh> mesh = meshfactory("test/one_quad.exo", gm, true, true);
 
  int cell(0), nfaces(5);
  AmanziGeometry::Point zero(2);
  VectorPolynomial uc;
  std::vector<VectorPolynomial> vf(nfaces);

  // test linear deformation
  std::cout << "    subtest: LINEAR deformation" << std::endl;
  for (int n = 0; n < nfaces; ++n) {
    vf[n].resize(2);
    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 1, true);
      vf[n][i](0, 0) = 1.0;
      vf[n][i](1, 0) = 2.0;
      vf[n][i](1, 1) = 3.0;
    }
  }
  
  MFD3D_CrouzeixRaviart mfd(mesh);
  VectorPolynomial moments(2, 2);  // trivial polynomials p=0

  // -- old scheme
  mfd.set_order(1);
  mfd.H1Cell(cell, vf, moments, uc);

  uc.ChangeOrigin(zero);
  std::cout << uc[0] << std::endl;

  uc[0] -= vf[0][0];
  uc[1] -= vf[0][1];
  CHECK(uc[0].NormMax() < 1e-12 && uc[1].NormMax() < 1e-12);

  // -- new scheme (k=1)
  mfd.set_use_always_ho(true);
  for (int k = 1; k < 4; ++k) {
    if (k > 1) moments[0].Reshape(2, k - 2, true);
    moments[0](0, 0) = 5.366066066066;
    if (k > 2) {
      moments[0](1, 0) = 0.45291015482207;
      moments[0](1, 1) = 0.25739762151369;
    }
    moments[1] = moments[0];

    mfd.set_order(k);
    mfd.H1Cell(cell, vf, moments, uc);

    uc.ChangeOrigin(zero);
    std::cout << uc[0] << std::endl;

    uc[0] -= vf[0][0];
    uc[1] -= vf[0][1];
    CHECK(uc[0].NormMax() < 1e-12 && uc[1].NormMax() < 1e-12);
  }

  // test quadratic deformation
  std::cout << "    subtest: QUADRATIC deformation" << std::endl;
  for (int n = 0; n < nfaces; ++n) {
    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 2, false);
      vf[n][i](2, 0) = 4.0;
      vf[n][i](2, 1) = 5.0;
      vf[n][i](2, 2) = -4.0;
    }
  }

  for (int k = 2; k < 4; ++k) {
    moments[0].Reshape(2, k - 2, true);
    moments[0](0, 0) = 13.99442192192193;
    if (k > 2) {
      moments[0](1, 0) = 3.30733251805033;
      moments[0](1, 1) = 0.32898471449271;
    }
    moments[1] = moments[0];

    mfd.set_order(k);
    mfd.H1Cell(cell, vf, moments, uc);

    uc.ChangeOrigin(zero);
    std::cout << uc[0] << std::endl;

    uc[0] -= vf[0][0];
    uc[1] -= vf[0][1];
    CHECK(uc[0].NormMax() < 1e-12 && uc[1].NormMax() < 1e-12);
  }

  // test cubic deformation
  std::cout << "    subtest: CUBIC deformation" << std::endl;
  for (int n = 0; n < nfaces; ++n) {
    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 3, false);
      vf[n][i](3, 0) = 2.0;
      vf[n][i](3, 1) = -6.0;
      vf[n][i](3, 2) = -6.0;
      vf[n][i](3, 3) = 2.0;
    }
  }

  for (int k = 3; k < 4; ++k) {
    moments[0].Reshape(2, k - 2, true);
    moments[0](0, 0) = 9.72312102102103;
    if (k > 2) {
      moments[0](1, 0) = 2.60365194630611;
      moments[0](1, 1) =-0.95827249608879;
    }
    moments[1] = moments[0];

    mfd.set_order(k);
    mfd.H1Cell(cell, vf, moments, uc);

    uc.ChangeOrigin(zero);
    std::cout << vf[0][0] << std::endl;
    std::cout << uc[0] << std::endl;

    uc[0] -= vf[0][0];
    uc[1] -= vf[0][1];
    std::cout << uc[0].NormMax() << " " << uc[1].NormMax() << std::endl;
    CHECK(uc[0].NormMax() < 1e-12 && uc[1].NormMax() < 1e-12);
  }

  // test trace compatibility between function and its projecton (k < 3 only!)
  std::cout << "    subtest: trace compatibility" << std::endl;
  for (int n = 0; n < nfaces; ++n) {
    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 2, false);
      vf[n][i](2, 0) = 4.0;
      vf[n][i](2, 1) = 5.0;
      vf[n][i](2, 2) = 6.0;
    }
  }

  moments[0].Reshape(2, 0, true);
  moments[0](0, 0) = 19.88406156156157;
  moments[1] = moments[0];

  mfd.set_order(2);
  mfd.H1Cell(cell, vf, moments, uc);

  uc.ChangeOrigin(zero);
  std::cout << uc[0] << std::endl;

  int dir;
  double val1(0.0), valx(0.0);
  NumericalIntegration numi(mesh);

  for (int n = 0; n < nfaces; ++n) {
    const AmanziGeometry::Point& normal = mesh->face_normal(n, false, cell, &dir);
    double factor = normal[0] / mesh->face_area(n) * dir;

    std::vector<const Polynomial*> polys;

    Polynomial tmp = vf[n][0] - uc[0];
    polys.push_back(&tmp);
    val1 += factor * numi.IntegratePolynomialsFace(n, polys);

    Polynomial q(2, 1);
    q(1, 0) = 1.0;
    q(1, 1) = 2.0;
    polys.push_back(&q);
    valx += factor * numi.IntegratePolynomialsFace(n, polys);
  }
  std::cout << "values: " << val1 << " " << valx << std::endl;
  CHECK_CLOSE(val1, 0.0, 1e-12);
  CHECK_CLOSE(valx, 0.0, 1e-12);

  // preservation of moments (reusing previous boundary functions)
  std::cout << "    subtest: verify calculated moments" << std::endl;
  for (int k = 2; k < 4; ++k) {
    moments[0].Reshape(2, k - 2);
    moments[1].Reshape(2, k - 2);

    for (auto it = moments[0].begin(); it < moments[0].end(); ++it) {
      int m = it.MonomialSetOrder();
      int i = it.MonomialSetPosition();
      int n = it.PolynomialPosition();
      moments[0](m, i) = 1.0 + n;
    }

    mfd.set_order(k);
    mfd.H1Cell(cell, vf, moments, uc);

    for (auto it = moments[0].begin(); it < moments[0].end(); ++it) {
      Polynomial mono(2, it.multi_index(), 1.0);
      mono.set_origin(mesh->cell_centroid(cell));
   
      Polynomial poly(uc[0]);
      poly.ChangeOrigin(mesh->cell_centroid(cell));
      poly *= mono;

      double val = numi.IntegratePolynomialCell(cell, poly) / mesh->cell_volume(cell);
      int n = it.PolynomialPosition();
      if (n == 0) CHECK_CLOSE(1.0, val, 1e-12);
      if (n >= 1) CHECK(fabs(val - (1.0 + n)) > 0.05);
    }
  }

  delete comm;
}


/* **************************************************************** */
TEST(L2_PROJECTORS_SQUARE_CR) {
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::WhetStone;

  std::cout << "\nTest: Crouzeix-Raviart L2 projector for square" << std::endl;
  Epetra_MpiComm *comm = new Epetra_MpiComm(MPI_COMM_WORLD);

  MeshFactory meshfactory(comm);
  meshfactory.preference(FrameworkPreference({MSTK}));
  Teuchos::RCP<const Amanzi::AmanziGeometry::GeometricModel> gm;
  Teuchos::RCP<Mesh> mesh = meshfactory(0.0, 0.0, 2.0, 4.0, 1, 2, gm, true, true); 
 
  int cell(1);
  AmanziGeometry::Point zero(2);
  VectorPolynomial uc;
  std::vector<VectorPolynomial> vf(4);

  // test quartic deformation
  std::cout << "    subtest: QUARTIC deformation" << std::endl;
  for (int n = 0; n < 4; ++n) {
    vf[n].resize(2);
    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 4, true);
      vf[n][i].set_origin(mesh->cell_centroid(cell));
      vf[n][i](4, 1) = 1.0;
      vf[n][i].ChangeOrigin(AmanziGeometry::Point(0.0, 0.0));
    }
  }

  MFD3D_CrouzeixRaviart mfd(mesh);
  VectorPolynomial moments(2, 2, 2);
  moments[0](2, 1) = 1.0 / 60;
  moments[1](2, 1) = 1.0 / 60;

  mfd.set_order(4);
  mfd.H1Cell(cell, vf, moments, uc);

  uc.ChangeOrigin(zero);
  std::cout << uc[0] << std::endl;

  uc[0] -= vf[0][0];
  CHECK(uc[0].NormMax() < 1e-12);

  delete comm;
}


/* **************************************************************** */
TEST(L2GRADIENT_PROJECTORS_SQUARE_CR) {
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::WhetStone;

  std::cout << "\nTest: Crouzeix-Raviart L2 projector of gradient for square" << std::endl;
  Epetra_MpiComm *comm = new Epetra_MpiComm(MPI_COMM_WORLD);

  MeshFactory meshfactory(comm);
  meshfactory.preference(FrameworkPreference({MSTK}));
  Teuchos::RCP<const Amanzi::AmanziGeometry::GeometricModel> gm;
  Teuchos::RCP<Mesh> mesh = meshfactory(0.0, 0.0, 4.0, 2.0, 2, 1, gm, true, true); 
 
  int cell(1);
  AmanziGeometry::Point zero(2);
  MatrixPolynomial uc;
  std::vector<VectorPolynomial> vf(4);

  // test quadratic deformation
  std::cout << "    subtest: CUBIC deformation, exact moments" << std::endl;
  for (int n = 0; n < 4; ++n) {
    vf[n].resize(1);
    vf[n][0].Reshape(2, 3, true);
    vf[n][0].set_origin(mesh->cell_centroid(cell));
    vf[n][0](3, 1) = 3.0;
    vf[n][0](3, 3) =-1.0;
    vf[n][0].ChangeOrigin(AmanziGeometry::Point(0.0, 0.0));
  }
  VectorPolynomial grad;
  grad.Gradient(vf[0][0]);

  MFD3D_CrouzeixRaviart mfd(mesh);
  auto moments = std::make_shared<WhetStone::DenseVector>(3);
  moments->PutScalar(0.0);
  (*moments)(2) = 1.0 / 15;

  mfd.set_order(3);
  mfd.L2GradientCell(cell, vf, moments, uc);

  uc[0][0].ChangeOrigin(zero);
  uc[0][1].ChangeOrigin(zero);
  std::cout << uc[0][0] << std::endl;

  uc[0][0] -= grad[0];
  uc[0][1] -= grad[1];
  CHECK(uc[0][0].NormMax() < 1e-12 && uc[0][1].NormMax() < 1e-12);

  std::cout << "    subtest: CUBIC deformation, computed moments" << std::endl;
  mfd.L2GradientCell(cell, vf, moments, uc);
  std::cout << "    moments: " << *moments << std::endl;

  CHECK_CLOSE(1.0 / 15, (*moments)(2), 1e-12);

  delete comm;
}


/* **************************************************************** */
TEST(PROJECTORS_SQUARE_PK) {
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::WhetStone;

  std::cout << "\nTest: HO Lagrange projectors for square (linear deformation)" << std::endl;
  Epetra_MpiComm *comm = new Epetra_MpiComm(MPI_COMM_WORLD);

  MeshFactory meshfactory(comm);
  meshfactory.preference(FrameworkPreference({MSTK}));
  Teuchos::RCP<const Amanzi::AmanziGeometry::GeometricModel> gm;
  Teuchos::RCP<Mesh> mesh = meshfactory(-1.2, 0.0, 1.2, 1.1, 2, 1, gm, true, true); 
 
  int cell(1);
  AmanziGeometry::Point zero(2);
  VectorPolynomial uc, uc2;
  std::vector<VectorPolynomial> vf(4);

  // test zero cell deformation
  for (int n = 0; n < 4; ++n) {
    vf[n].resize(2);
    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 1, true);
    }
  }

  MFD3D_Lagrange mfd(mesh);
  MFD3D_CrouzeixRaviart mfd_cr(mesh);
  VectorPolynomial moments(2, 2);

  // test linear deformation
  for (int n = 0; n < 4; ++n) {
    for (int i = 0; i < 2; ++i) {
      vf[n][i](0, 0) = 1.0;
      vf[n][i](1, 0) = 2.0;
      vf[n][i](1, 1) = 3.0;
    }
  }
  
  for (int k = 1; k < 4; ++k) {
    if (k > 1) moments[0].Reshape(2, k - 2, true);
    moments[0](0, 0) = 3.85;
    if (k > 2) {
      moments[0](1, 0) = 0.20889318714684;
      moments[0](1, 1) = 0.26329245463299;
    }
    moments[1] = moments[0];

    mfd.set_order(k);
    mfd.H1Cell(cell, vf, moments, uc);  

    uc.ChangeOrigin(zero);
    for (int i = 0; i < 2; ++i) {
      uc[i] -= vf[0][i];
      CHECK(uc[i].NormMax() < 1e-12);
    }
  }

  // test re-location of the right-top corner to (2,3)
  // cross-check with the CR projectors
  std::cout << "Test: HO Lagrange projectors for square (bilinear deformation)" << std::endl;
  for (int n = 0; n < 4; ++n) vf[n].PutScalar(0.0);
  vf[1][0](1, 1) = 0.8 / 1.1; 
  vf[1][1](1, 1) = 1.9 / 1.1; 

  vf[2][0](1, 0) = 0.8 / 1.2; 
  vf[2][1](1, 0) = 1.9 / 1.2; 

  moments[0].Reshape(2, 0, true);
  moments[1].Reshape(2, 0, true);

  moments[0](0, 0) = 0.2;
  moments[1](0, 0) = 0.475;

  for (int k = 1; k < 3; ++k) { 
    mfd.set_order(k);
    mfd.H1Cell(cell, vf, moments, uc);  

    mfd_cr.set_order(k);
    mfd_cr.H1Cell(cell, vf, moments, uc2);

    uc.ChangeOrigin(zero);
    uc2.ChangeOrigin(zero);
    for (int i = 0; i < 2; ++i) {
      uc2[i] -= uc[i];
      CHECK(uc2[i].NormMax() < 1e-12);
    }

    // Compare H1 and L2 projectors
    mfd.L2Cell(cell, vf, moments, uc2);

    uc2.ChangeOrigin(zero);
    uc2[0] -= uc[0];
    CHECK(uc2[0].NormMax() < 1e-12);
  }

  auto p = AmanziGeometry::Point(1.2, 1.1);
  CHECK(fabs(uc[0].Value(p) - 0.8) < 1e-12 &&
        fabs(uc[1].Value(p) - 1.9) < 1e-12);

  delete comm;
}


/* **************************************************************** */
TEST(PROJECTORS_POLYGON_PK) {
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::WhetStone;

  std::cout << "\nTest: HO Lagrange projectors for pentagon (linear deformation)" << std::endl;
  Epetra_MpiComm *comm = new Epetra_MpiComm(MPI_COMM_WORLD);

  MeshFactory meshfactory(comm);
  meshfactory.preference(FrameworkPreference({MSTK}));
  Teuchos::RCP<const Amanzi::AmanziGeometry::GeometricModel> gm;
  Teuchos::RCP<Mesh> mesh = meshfactory("test/one_pentagon.exo", gm, true, true);
  // Teuchos::RCP<Mesh> mesh = meshfactory("test/one_quad.exo", gm, true, true);
 
  int cell(0), nfaces(5);
  AmanziGeometry::Point zero(2);
  VectorPolynomial uc, uc2;
  std::vector<VectorPolynomial> vf(nfaces);

  MFD3D_Lagrange mfd(mesh);
  MFD3D_CrouzeixRaviart mfd_cr(mesh);
  VectorPolynomial moments(2, 2);

  // test globally linear deformation
  for (int n = 0; n < nfaces; ++n) {
    vf[n].resize(2);
    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 1, true);
      vf[n][i](0, 0) = 1.0;
      vf[n][i](1, 0) = 2.0;
      vf[n][i](1, 1) = 3.0;
    }
  }
  
  for (int k = 1; k < 4; ++k) {
    if (k > 1) moments[0].Reshape(2, k - 2, true);
    moments[0](0, 0) = 5.36606606606607;
    if (k > 2) {
      moments[0](1, 0) = 0.45291015482207;
      moments[0](1, 1) = 0.25739762151369;
    }
    moments[1] = moments[0];

    mfd.set_order(k);
    mfd.H1Cell(cell, vf, moments, uc);

    uc.ChangeOrigin(zero);
    std::cout << uc[0] << std::endl;

    uc[0] -= vf[0][0];
    uc[1] -= vf[0][1];
    CHECK(uc[0].NormMax() < 1e-12 && uc[1].NormMax() < 1e-12);
  }

  // test globally quadratic deformation
  std::cout << "\nTest: HO Lagrange for pentagon (quadratic deformation)" << std::endl;
  for (int n = 0; n < nfaces; ++n) {
    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 2, false);
      vf[n][i](2, 0) = 4.0;
      vf[n][i](2, 1) = 5.0;
      vf[n][i](2, 2) = -4.0;
    }
  }

  for (int k = 2; k < 4; ++k) {
    moments[0].Reshape(2, k - 2, true);
    moments[0](0, 0) = 13.99442192192193;
    if (k > 2) {
      moments[0](1, 0) = 3.30733251805033;
      moments[0](1, 1) = 0.32898471449271;
    }
    moments[1] = moments[0];

    mfd.set_order(k);
    mfd.H1Cell(cell, vf, moments, uc);

    uc.ChangeOrigin(zero);
    std::cout << uc[0] << std::endl;
    uc[0] -= vf[0][0];
    uc[1] -= vf[0][1];
    CHECK(uc[0].NormMax() < 1e-10 && uc[1].NormMax() < 1e-10);
  }

  // test trace compatibility between function and its projecton (k < 3 only!)
  std::cout << "\nTest: HO Lagrange projectors for pentagon (trace compatibility)" << std::endl;
  for (int n = 0; n < nfaces; ++n) {
    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 2, false);
      vf[n][i](2, 0) = 4.0;
      vf[n][i](2, 1) = 5.0;
      vf[n][i](2, 2) = 6.0;
    }
  }
  moments[0].Reshape(2, 0, true);
  moments[0](0, 0) = 19.88406156156157;
  moments[1] = moments[0];

  mfd.set_order(2);
  mfd.H1Cell(cell, vf, moments, uc);

  uc.ChangeOrigin(zero);
  std::cout << uc[0] << std::endl;

  int dir;
  double val1(0.0), valx(0.0);
  NumericalIntegration numi(mesh);

  for (int n = 0; n < nfaces; ++n) {
    const AmanziGeometry::Point& normal = mesh->face_normal(n, false, cell, &dir);
    double factor = normal[0] / mesh->face_area(n) * dir;

    std::vector<const Polynomial*> polys;

    Polynomial tmp = vf[n][0] - uc[0];
    polys.push_back(&tmp);
    val1 += factor * numi.IntegratePolynomialsFace(n, polys);

    Polynomial q(2, 1);
    q(1, 0) = 1.0;
    q(1, 1) = 2.0;
    polys.push_back(&q);
    valx += factor * numi.IntegratePolynomialsFace(n, polys);
  }
  std::cout << "values: " << val1 << " " << valx << std::endl;
  CHECK_CLOSE(val1, 0.0, 1e-12);
  CHECK_CLOSE(valx, 0.0, 1e-12);

  // test piecewise linear deformation
  std::cout << "\nTest: HO Lagrange projectors for pentagon (piece-wice linear deformation)" << std::endl;
  std::vector<AmanziGeometry::Point> vv;
  vv.push_back(AmanziGeometry::Point( 0.0, 0.0));
  vv.push_back(AmanziGeometry::Point( 0.0,-0.1));
  vv.push_back(AmanziGeometry::Point( 0.1, 0.0));
  vv.push_back(AmanziGeometry::Point( 0.0, 0.1));
  vv.push_back(AmanziGeometry::Point(-0.1, 0.0));

  AmanziGeometry::Point x1(2), x2(2), tau(2);
  for (int n = 0; n < 5; ++n) {
    int m = (n + 1) % 5;
    mesh->node_get_coordinates(n, &x1);
    mesh->node_get_coordinates(m, &x2);
    tau = x2 - x1;
    tau /= AmanziGeometry::L22(tau);

    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 1);
      vf[n][i](0, 0) = vv[n][i] * (x2 * tau) - vv[m][i] * (x1 * tau);

      vf[n][i](1, 0) = (vv[m][i] - vv[n][i]) * tau[0];
      vf[n][i](1, 1) = (vv[m][i] - vv[n][i]) * tau[1];
    }
  }
  
  for (int k = 1; k < 4; ++k) {
    if (k > 1) moments[0].Reshape(2, k - 2, true);
    moments[0](0, 0) = 0.1;
    if (k > 2) {
      moments[0](1, 0) = 0.2;
      moments[0](1, 1) = 0.3;
    }
    moments[1] = moments[0];

    mfd.set_order(k);
    mfd.H1Cell(cell, vf, moments, uc);

    mfd_cr.set_order(k);
    mfd_cr.H1Cell(cell, vf, moments, uc2);

    uc.ChangeOrigin(zero);
    uc2.ChangeOrigin(zero);
    uc2 -= uc;
    if (k < 3) CHECK(uc2[0].NormMax() < 1e-12);

    mfd.L2Cell(cell, vf, moments, uc2);

    uc2.ChangeOrigin(zero);
    uc2 -= uc;
    if (k < 3) CHECK(uc2[0].NormMax() < 1e-12);
    if (k > 2) std::cout << " moments: " << moments[0](0, 0) << " " 
                                         << moments[0](1, 0) << " " << moments[0](1, 1) << std::endl;
  }

  // preservation of moments (reusing previous boundary functions)
  std::cout << "\nTest: HO Lagrange projectors for pentagon (verify moments)" << std::endl;
  for (int k = 2; k < 4; ++k) {
    moments[0].Reshape(2, k - 2);
    moments[0].PutScalar(1.0);
    moments[1] = moments[0];

    mfd.set_order(k);
    mfd.H1Cell(cell, vf, moments, uc);
    double tmp = numi.IntegratePolynomialCell(cell, uc[0]) / mesh->cell_volume(cell);
    CHECK_CLOSE(1.0, tmp, 1e-12);

    mfd.L2Cell(cell, vf, moments, uc);
    tmp = numi.IntegratePolynomialCell(cell, uc[0]) / mesh->cell_volume(cell);
    CHECK_CLOSE(1.0, tmp, 1e-12);
  }

  delete comm;
}


/* **************************************************************** */
template<class Serendipity>
void SerendipityProjectorPolygon() {
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::WhetStone;

  std::cout << "\nTest: HO Serendipity Lagrange projectors for pentagon (linear deformation)" << std::endl;
  Epetra_MpiComm *comm = new Epetra_MpiComm(MPI_COMM_WORLD);

  MeshFactory meshfactory(comm);
  meshfactory.preference(FrameworkPreference({MSTK}));
  Teuchos::RCP<const Amanzi::AmanziGeometry::GeometricModel> gm;
  Teuchos::RCP<Mesh> mesh = meshfactory("test/one_pentagon.exo", gm, true, true);
 
  int cell(0), nfaces(5);
  AmanziGeometry::Point zero(2);
  VectorPolynomial uc, uc2;
  std::vector<VectorPolynomial> vf(nfaces);

  Serendipity mfd(mesh);
  VectorPolynomial moments(2, 2);

  // test globally linear deformation
  for (int n = 0; n < nfaces; ++n) {
    vf[n].resize(2);
    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 1, true);
      vf[n][i](0, 0) = 1.0;
      vf[n][i](1, 0) = 2.0;
      vf[n][i](1, 1) = 3.0;
    }
  }
  
  for (int k = 1; k < 4; ++k) {
    mfd.set_order(k);
    mfd.L2Cell(cell, vf, moments, uc);
    uc.ChangeOrigin(zero);
    std::cout << uc[0] << std::endl;

    uc -= vf[0];
    CHECK(uc[0].NormMax() < 1e-10 && uc[1].NormMax() < 1e-10);

    mfd.H1Cell(cell, vf, moments, uc);
    uc.ChangeOrigin(zero);
    uc -= vf[0];
    CHECK(uc[0].NormMax() < 2e-10 && uc[1].NormMax() < 2e-10);
  }

  // test globally quadratic deformation
  std::cout << "\nTest: HO Serendipity Lagrange projectors for pentagon (quadratic deformation)" << std::endl;
  for (int n = 0; n < nfaces; ++n) {
    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 2, false);
      vf[n][i](2, 0) = 4.0;
      vf[n][i](2, 1) = 5.0;
      vf[n][i](2, 2) = 6.0 + i;
    }
  }

  for (int k = 2; k < 4; ++k) {
    mfd.set_order(k);
    mfd.L2Cell(cell, vf, moments, uc);
    uc.ChangeOrigin(zero);
    std::cout << uc[0] << std::endl;

    uc[0] -= vf[0][0];
    uc[1] -= vf[0][1];
    CHECK(uc[0].NormMax() < 4e-10 && uc[1].NormMax() < 2e-10);

    mfd.H1Cell(cell, vf, moments, uc);
    uc.ChangeOrigin(zero);
    uc -= vf[0];
    CHECK(uc[0].NormMax() < 4e-10 && uc[1].NormMax() < 5e-10);
  }

  // test piecewise linear deformation (part I)
  std::cout << "\nTest: HO Sependipity Lagrange projectors for pentagon (piece-wice linear)" << std::endl;
  std::vector<AmanziGeometry::Point> vv;
  vv.push_back(AmanziGeometry::Point( 0.0, 0.0));
  vv.push_back(AmanziGeometry::Point( 0.05,-0.1));
  vv.push_back(AmanziGeometry::Point( 0.1, 0.0));
  vv.push_back(AmanziGeometry::Point( 0.0, 0.1));
  vv.push_back(AmanziGeometry::Point(-0.1, 0.0));

  AmanziGeometry::Point x1(2), x2(2), tau(2);
  for (int n = 0; n < 5; ++n) {
    int m = (n + 1) % 5;
    mesh->node_get_coordinates(n, &x1);
    mesh->node_get_coordinates(m, &x2);
    tau = x2 - x1;
    tau /= AmanziGeometry::L22(tau);

    for (int i = 0; i < 2; ++i) {
      vf[n][i].Reshape(2, 1);
      vf[n][i](0, 0) = vv[n][i] * (x2 * tau) - vv[m][i] * (x1 * tau);

      vf[n][i](1, 0) = (vv[m][i] - vv[n][i]) * tau[0];
      vf[n][i](1, 1) = (vv[m][i] - vv[n][i]) * tau[1];
    }
  }
  
  for (int k = 1; k < 4; ++k) {
    mfd.set_order(k);
    mfd.L2Cell(cell, vf, moments, uc);
    uc.ChangeOrigin(zero);
    std::cout << "order=" << k << " " << uc[0] << std::endl;

    mfd.L2Cell_LeastSquare(cell, vf, moments, uc2);
    uc2.ChangeOrigin(zero);
    uc2 -= uc;
    CHECK(uc2[0].NormMax() < 1e-11 && uc2[1].NormMax() < 1e-11);

    mfd.H1Cell(cell, vf, moments, uc2);
    uc2.ChangeOrigin(zero);
    uc2 -= uc;
    CHECK(uc2[0].NormMax() < 2e-2 && uc2[1].NormMax() < 4e-2);
  }

  delete comm;
}

TEST(SERENDIPITY_PROJECTORS_POLYGON_PK) {
  SerendipityProjectorPolygon<Amanzi::WhetStone::MFD3D_LagrangeSerendipity>();
}

TEST(SERENDIPITY_PROJECTORS_POLYGON_CR) {
  SerendipityProjectorPolygon<Amanzi::WhetStone::MFD3D_CrouzeixRaviartSerendipity>();
}

