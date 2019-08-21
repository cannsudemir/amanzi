/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */
/* -------------------------------------------------------------------------
   Amanzi

   License: see $AMANZI_DIR/COPYRIGHT
   Author: Ethan Coon

   Unit tests for the communication patterns used in CompositeVector
------------------------------------------------------------------------- */
#include <vector>

#include "UnitTest++.h"
#include "Teuchos_GlobalMPISession.hpp"
#include "Teuchos_RCP.hpp"

#include "AmanziComm.hh"
#include "AmanziMap.hh"
#include "AmanziVector.hh"
#include "Mesh.hh"
#include "MeshFactory.hh"

using namespace Amanzi;


TEST(COMMUNICATION_PATTERN_DISTINCT_VECTORS) {
  // having some trouble with communication -- lets make sure we understand
  // how to use tpetra
  auto comm = getDefaultComm();

  int size = comm->getSize(); CHECK_EQUAL(2,size);
  int rank = comm->getRank();

  Map_ptr_type owned_map, ghost_map;
  if (rank == 0) {
    std::vector<int> gids_owned{0,1};
    owned_map = Teuchos::rcp(new Map_type(4, gids_owned.data(), 2, 0, comm));

    std::vector<int> gids_used{0,1,2};
    ghost_map = Teuchos::rcp(new Map_type(6, gids_used.data(), 3, 0, comm));
  } else {
    std::vector<int> gids_owned{2,3};
    owned_map = Teuchos::rcp(new Map_type(4, gids_owned.data(), 2, 0, comm));

    std::vector<int> gids_used{2,3,1};
    ghost_map = Teuchos::rcp(new Map_type(6, gids_used.data(), 3, 0, comm));
  }

  Import_type importer(owned_map, ghost_map);

  auto ghost = Teuchos::rcp(new MultiVector_type(ghost_map, 1));
  //  auto owned = ghost->offsetViewNonConst(owned_map, 0);
  auto owned = Teuchos::rcp(new MultiVector_type(owned_map, 1));

  owned->putScalar((double)(rank+1));
  ghost->doImport(*owned, importer, Tpetra::INSERT);

  // manually get a host view and check
  ghost->sync_host();
  {
    // NOTE INDICES HERE -- they are backwards relative to Epetra!
    auto ghost_v = ghost->getLocalViewHost();
    if (rank == 0) {
      CHECK_CLOSE(1.0, ghost_v(0,0), 1.e-6);
      CHECK_CLOSE(2.0, ghost_v(2,0), 1.e-6);
    } else {
      CHECK_CLOSE(2.0, ghost_v(0,0), 1.e-6);
      CHECK_CLOSE(1.0, ghost_v(2,0), 1.e-6);
    }
  }
}


TEST(COMMUNICATION_PATTERN_OFFSET_VIEW) {
  // having some trouble with communication -- lets make sure we understand
  // how to use tpetra
  auto comm = getDefaultComm();

  int size = comm->getSize(); CHECK_EQUAL(2,size);
  int rank = comm->getRank();

  Map_ptr_type owned_map, ghost_map;
  if (rank == 0) {
    std::vector<int> gids_owned{0,1};
    owned_map = Teuchos::rcp(new Map_type(4, gids_owned.data(), 2, 0, comm));

    std::vector<int> gids_used{0,1,2};
    ghost_map = Teuchos::rcp(new Map_type(6, gids_used.data(), 3, 0, comm));
  } else {
    std::vector<int> gids_owned{2,3};
    owned_map = Teuchos::rcp(new Map_type(4, gids_owned.data(), 2, 0, comm));

    std::vector<int> gids_used{2,3,1};
    ghost_map = Teuchos::rcp(new Map_type(6, gids_used.data(), 3, 0, comm));
  }

  Import_type importer(owned_map, ghost_map);

  auto ghost = Teuchos::rcp(new MultiVector_type(ghost_map, 1));
  auto owned = ghost->offsetViewNonConst(owned_map, 0);

  owned->putScalar((double)(rank+1));
  ghost->doImport(*owned, importer, Tpetra::INSERT);

  // manually get a host view and check
  ghost->sync_host();
  {
    // NOTE INDICES HERE -- they are backwards relative to Epetra!
    auto ghost_v = ghost->getLocalViewHost();
    if (rank == 0) {
      CHECK_CLOSE(1.0, ghost_v(0,0), 1.e-6);
      CHECK_CLOSE(2.0, ghost_v(2,0), 1.e-6);
    } else {
      CHECK_CLOSE(2.0, ghost_v(0,0), 1.e-6);
      CHECK_CLOSE(1.0, ghost_v(2,0), 1.e-6);
    }
  }
}


TEST(COMMUNICATION_PATTERN_VANDELAY) {
  // having some trouble with communication -- lets make sure we understand
  // how to use tpetra
  auto comm = getDefaultComm();

  int size = comm->getSize(); CHECK_EQUAL(2,size);
  int rank = comm->getRank();

  Map_ptr_type owned_map, ghost_map, vandelay_map;
  if (rank == 0) {
    std::vector<int> gids_owned{0,1};
    owned_map = Teuchos::rcp(new Map_type(4, gids_owned.data(), 2, 0, comm));

    std::vector<int> gids_used{0,1,2};
    ghost_map = Teuchos::rcp(new Map_type(6, gids_used.data(), 3, 0, comm));

    std::vector<int> gids_v{0};
    vandelay_map = Teuchos::rcp(new Map_type(2, gids_v.data(), 1, 0, comm));
    
  } else {
    std::vector<int> gids_owned{2,3};
    owned_map = Teuchos::rcp(new Map_type(4, gids_owned.data(), 2, 0, comm));

    std::vector<int> gids_used{2,3,1};
    ghost_map = Teuchos::rcp(new Map_type(6, gids_used.data(), 3, 0, comm));

    std::vector<int> gids_v{3};
    vandelay_map = Teuchos::rcp(new Map_type(2, gids_v.data(), 1, 0, comm));
  }


  Import_type importer(owned_map, ghost_map);
  Import_type vandelay_importer(owned_map, vandelay_map);

  auto ghost = Teuchos::rcp(new MultiVector_type(ghost_map, 1));
  auto owned = ghost->offsetViewNonConst(owned_map, 0);
  auto vand = Teuchos::rcp(new MultiVector_type(vandelay_map, 1));

  owned->putScalar((double)(rank+1));
  ghost->doImport(*owned, importer, Tpetra::INSERT);

  // manually get a host view and check
  ghost->sync_host();
  {
    // NOTE INDICES HERE -- they are backwards relative to Epetra!
    auto ghost_v = ghost->getLocalViewHost();
    if (rank == 0) {
      CHECK_CLOSE(1.0, ghost_v(0,0), 1.e-6);
      CHECK_CLOSE(2.0, ghost_v(2,0), 1.e-6);
    } else {
      CHECK_CLOSE(2.0, ghost_v(0,0), 1.e-6);
      CHECK_CLOSE(1.0, ghost_v(2,0), 1.e-6);
    }
  }

  // check the vandelay map
  vand->doImport(*owned, vandelay_importer, Tpetra::INSERT);
  vand->sync_host();
  {
    // NOTE INDICES HERE -- they are backwards relative to Epetra!
    auto vand_v = vand->getLocalViewHost();
    if (rank == 0) {
      CHECK_CLOSE(1.0, vand_v(0,0), 1.e-6);
    } else {
      CHECK_CLOSE(2.0, vand_v(0,0), 1.e-6);
    }
  }
  
}


TEST(COMMUNICATION_INTERMEDIATE) {
  using namespace Amanzi;
  
  auto comm = getDefaultComm();
  int rank = comm->getRank();
  AmanziMesh::Preference pref;
  pref.clear();
  pref.push_back(AmanziMesh::Framework::MSTK);

  AmanziMesh::MeshFactory meshfactory(comm);
  meshfactory.set_preference(pref);

  auto mesh = meshfactory.create(0.0, 0.0, 0.0, 4.0, 4.0, 4.0, 1, 1, 8);

  auto ghost = Teuchos::rcp(new MultiVector_type(mesh->face_map(true), 1));
  auto owned = ghost->offsetViewNonConst(mesh->face_map(false), 0);
  auto vand = Teuchos::rcp(new MultiVector_type(mesh->exterior_face_map(false), 1));

  Import_type importer(mesh->face_map(false), mesh->face_map(true));
  owned->putScalar((double)(rank+1));
  ghost->doImport(*owned, importer, Tpetra::INSERT);

  // manually get a host view and check
  ghost->sync_host();
  {
    // NOTE INDICES HERE -- they are backwards relative to Epetra!
    auto ghost_v = ghost->getLocalViewHost();
    if (rank == 0) {
      CHECK_CLOSE(1.0, ghost_v(0,0), 1.e-6);
      CHECK_CLOSE(2.0, ghost_v( mesh->num_entities(AmanziMesh::FACE, AmanziMesh::Parallel_type::OWNED), 0), 1.e-6);
    } else {
      CHECK_CLOSE(2.0, ghost_v(0,0), 1.e-6);
      CHECK_CLOSE(1.0, ghost_v(mesh->num_entities(AmanziMesh::FACE, AmanziMesh::Parallel_type::OWNED),0), 1.e-6);
    }
  }

  // check the vandelay map
  Import_type vimporter(mesh->face_map(false), mesh->exterior_face_map(false));
  vand->doImport(*owned, vimporter, Tpetra::INSERT);
  //auto vimporter = mesh->exterior_face_importer();
  //vand->doImport(*owned, *vimporter, Tpetra::INSERT);
  vand->sync_host();
  {
    // NOTE INDICES HERE -- they are backwards relative to Epetra!
    auto vand_v = vand->getLocalViewHost();
    if (rank == 0) {
      CHECK_CLOSE(1.0, vand_v(0,0), 1.e-6);
    } else {
      CHECK_CLOSE(2.0, vand_v(0,0), 1.e-6);
    }
  }
}