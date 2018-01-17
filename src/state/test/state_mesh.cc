/*
 State

 Tests for state as a container of meshes
*/

// TPLs
#include "UnitTest++.h"

// State
#include "MeshFactory.hh"
#include "State.hh"

TEST(STATE_CREATION) {
  using namespace Amanzi;

  auto comm = new Epetra_MpiComm(MPI_COMM_WORLD);
  AmanziMesh::MeshFactory fac(comm);
  auto mesh = fac(0.0, 0.0, 0.0, 4.0, 4.0, 4.0, 2, 2, 2);

  State s;
  s.RegisterDomainMesh(mesh);

  s.RegisterMesh("other_domain_mesh", mesh, true);
  s.AliasMesh("domain", "yet_another_domain_mesh");

  CHECK(s.HasMesh("domain"));
  CHECK(s.HasMesh("other_domain_mesh"));
  CHECK(s.HasMesh("yet_another_domain_mesh"));

  CHECK(!s.IsDeformableMesh("domain"));
  CHECK(s.IsDeformableMesh("other_domain_mesh"));
  CHECK(!s.IsDeformableMesh("yet_another_domain_mesh"));

  CHECK(s.GetMesh() == s.GetMesh("yet_another_domain_mesh"));
}
