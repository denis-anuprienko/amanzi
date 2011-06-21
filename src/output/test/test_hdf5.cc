#include "UnitTest++.h"
#include "../hdf5_mesh.hh"
#include "Mesh_simple.hh"


TEST(HDF5) {

#ifdef HAVE_MPI
  Epetra_MpiComm *comm = new Epetra_MpiComm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm *comm = new Epetra_SerialComm();
#endif

  std::string hdf5_meshfile  = "new_mesh";
  std::string hdf5_datafile1 = "new_data";
  std::string hdf5_fullfile  = "new_full";

  Amanzi::AmanziMesh::Mesh_simple Mesh(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 4, 1, 1, comm);

  unsigned int num_nodes = Mesh.count_entities(Amanzi::AmanziMesh::NODE, Amanzi::AmanziMesh::OWNED);
  unsigned int num_cells = Mesh.count_entities(Amanzi::AmanziMesh::CELL, Amanzi::AmanziMesh::OWNED);

  Teuchos::RCP<Epetra_Vector> node_quantity;
  Teuchos::RCP<Epetra_Vector> cell_quantity;
  Teuchos::RCP<Epetra_Vector> fake_pressure;

  // Setup node quantity
  int node_index_list[] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  double node_values[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120};
  node_quantity = Teuchos::rcp(new Epetra_Vector(Mesh.node_map(false)));
  node_quantity->ReplaceGlobalValues(12, node_values, node_index_list);

  // Setup cell quantity
  int cell_index_list[] = {0, 1, 2, 3};
  double cell_values[] = {10, 20, 30, 40};
  cell_quantity = Teuchos::rcp(new Epetra_Vector(Mesh.cell_map(false)));
  cell_quantity->ReplaceGlobalValues(4, cell_values, cell_index_list);

  // Setup second cell quantity -- called fake pressure
  double fake_values[] = {9, 8, 7, 6};
  fake_pressure = Teuchos::rcp(new Epetra_Vector(Mesh.cell_map(false)));
  fake_pressure->ReplaceGlobalValues(4, fake_values, cell_index_list);

  // Write a file which contains both mesh and data.
  Amanzi::HDF5 viz_output;
  viz_output.setTrackXdmf(true);
  viz_output.createMeshFile(Mesh, hdf5_meshfile);
  viz_output.createDataFile(hdf5_meshfile, hdf5_datafile1);

  double time = 0.0;
  for (int i = 0; i < 15; i++) {
      // write time step data
      viz_output.createTimestep(time, i);
      viz_output.writeCellData(*cell_quantity, "cell_quantity");
      viz_output.writeCellData(*fake_pressure, "pressure");
      viz_output.writeNodeData(*node_quantity, "node_quantity");

      // advance time and values
      time += 2.0;
      for (int j = 0; j < 4; j++) {
          cell_values[j] += 10.0;
          fake_values[j] += 1.0;
      }
      for (int j = 0; j < 12; j++) {
          node_values[j] += 10.0;
      }
      cell_quantity->ReplaceGlobalValues(4, cell_values, cell_index_list);
      fake_pressure->ReplaceGlobalValues(4, fake_values, cell_index_list);
      node_quantity->ReplaceGlobalValues(12, node_values, node_index_list);

      // close file
      viz_output.endTimestep();
  }

}

