/*
  The discretization component of Amanzi.
  License: BSD
  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>

#include "UnitTest++.h"

#include "Teuchos_ParameterList.hpp"
#include "Teuchos_RCP.hpp"
#include "Teuchos_XMLParameterListHelpers.hpp"

#include "MeshFactory.hh"
#include "MeshAudit.hh"

#include "Mesh.hh"
#include "Point.hh"

#include "mfd3d_elasticity.hh"
#include "Tensor.hh"

/* ******************************************************************
* Dofs: vectors at nodes, normal component on faces.
****************************************************************** */
TEST(DIFFUSION_STOKES_2D) {
  using namespace Teuchos;
  using namespace Amanzi;
  using namespace Amanzi::AmanziGeometry;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::WhetStone;

  std::cout << "\nTest: Stiffness matrix for Stokes in 2D" << std::endl;
#ifdef HAVE_MPI
  Epetra_MpiComm *comm = new Epetra_MpiComm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm *comm = new Epetra_SerialComm();
#endif

  FrameworkPreference pref;
  pref.clear();
  pref.push_back(MSTK);

  MeshFactory meshfactory(comm);
  meshfactory.preference(pref);
  // RCP<Mesh> mesh = meshfactory(0.0, 0.0, 1.0, 1.0, 1, 1); 
  RCP<Mesh> mesh = meshfactory("test/one_cell2.exo"); 
 
  MFD3D_Elasticity mfd(mesh);

  AmanziMesh::Entity_ID_List nodes, faces;
  std::vector<int> dirs;

  // extract single cell
  int cell(0);
  mesh->cell_get_nodes(cell, &nodes);
  int nnodes = nodes.size();

  mesh->cell_get_faces_and_dirs(cell, &faces, &dirs);
  int nfaces = faces.size();

  // calcualte stiffness matrix
  Tensor T(2, 1);
  T(0, 0) = 1.0;

  int nrows = 2 * nnodes + nfaces;
  DenseMatrix A(nrows, nrows);

  mfd.StiffnessMatrixBernardiRaugel(cell, T, A);

  printf("Stiffness matrix for cell %3d\n", cell);
  for (int i=0; i<nrows; i++) {
    for (int j=0; j<nrows; j++ ) printf("%8.4f ", A(i, j)); 
    printf("\n");
  }

  // verify SPD propery
  for (int i = 0; i < nrows; i++) CHECK(A(i, i) > 0.0);

  // verify exact integration property
  int d = mesh->space_dimension();
  Point p(d);

  DenseVector ax(nrows), ay(nrows);
  ax.PutScalar(0.0);
  ay.PutScalar(0.0);
  for (int i = 0; i < nnodes; i++) {
    int v = nodes[i];
    mesh->node_get_coordinates(v, &p);
    ax(2*i) = p[0];
    ay(2*i + 1) = p[1];
  }
  for (int i = 0; i < nfaces; i++) {
    int f = faces[i];
    const AmanziGeometry::Point& normal = mesh->face_normal(f);
    const AmanziGeometry::Point& xf = mesh->face_centroid(f);
    double area = mesh->face_area(f);

    ax(2*nnodes + i) = xf[0] * normal[0] / area;
    ay(2*nnodes + i) = xf[1] * normal[1] / area;
  }

  double vxx(0.0), vxy(0.0); 
  double volume = mesh->cell_volume(cell);

  for (int i = 0; i < nrows; i++) {
    for (int j = 0; j < nrows; j++) {
      vxx += A(i, j) * ax(i) * ax(j);
      vxy += A(i, j) * ay(i) * ax(j);
    }
  }
  CHECK_CLOSE(vxx, volume, 1e-10);
  CHECK_CLOSE(vxy, 0.0, 1e-10);

  delete comm;
}


