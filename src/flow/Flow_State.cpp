/*
This is the flow component of the Amanzi code. 
License: BSD
Authors: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include "Epetra_Vector.h"
#include "Epetra_MultiVector.h"
#include "Epetra_Import.h"

#include "Point.hh"
#include "errors.hh"

#include "State.hpp"
#include "Flow_State.hpp"

namespace Amanzi {
namespace AmanziFlow {

/* **************************************************************** */
Flow_State::Flow_State(Teuchos::RCP<State> S)
{
  porosity = S->get_porosity();
  fluid_density = S->get_density();
  fluid_viscosity = S->get_viscosity();
  gravity = S->get_gravity();
  absolute_permeability = S->get_permeability();
  pressure = S->get_pressure();
  darcy_flux = S->get_darcy_flux();
  mesh_ = S->get_mesh_maps();

  S_ = &*S;
}


/* *******************************************************************
 * mode = CopyPointers (default) a trivial copy of the given state           
 * mode = ViewMemory   creates the flow state from internal one   
 *                     as the MPC expected                     
 * mode = CopyMemory   creates internal flow state based on 
 *                     ovelapped mesh maps                       
 * **************************************************************** */
Flow_State::Flow_State(Flow_State& S, FlowCreateMode mode)
{
  if (mode == CopyPointers) {
    porosity = S.get_porosity();
    fluid_density = S.get_fluid_density();
    fluid_viscosity = S.get_fluid_viscosity();
    gravity = S.get_gravity();
    absolute_permeability = S.get_absolute_permeability();
    pressure = S.get_pressure();
    darcy_flux = S.get_darcy_flux();
    mesh_ = S.get_mesh();
  }
  else if (mode == CopyMemory ) { 
    porosity = S.get_porosity();
    fluid_density = S.get_fluid_density();
    fluid_viscosity = S.get_fluid_viscosity();
    gravity = S.get_gravity();
    absolute_permeability = S.get_absolute_permeability();
    mesh_ = S.get_mesh();

    // allocate memory for internal state
    const Epetra_Map& cmap = mesh_->cell_map(true);
    const Epetra_Map& fmap = mesh_->face_map(true);

    pressure = Teuchos::rcp(new Epetra_Vector(cmap));
    copymemory_vector(S.ref_pressure(), *pressure);

    darcy_flux = Teuchos::rcp(new Epetra_Vector(fmap));
    copymemory_vector(S.ref_darcy_flux(), *darcy_flux);
  }

  else if (mode == ViewMemory) {
    porosity = S.get_porosity(); 
    fluid_density = S.get_fluid_density();
    fluid_viscosity = S.get_fluid_viscosity();
    gravity = S.get_gravity();
    absolute_permeability = S.get_absolute_permeability();
    mesh_ = S.get_mesh();

    double *data_dp, *data_ddf;
    const Epetra_Map& cmap = mesh_->cell_map(false);
    const Epetra_Map& fmap = mesh_->face_map(false);

    Epetra_Vector& dp = S.ref_pressure();
    dp.ExtractView(&data_dp);     
    pressure = Teuchos::rcp(new Epetra_Vector(View, cmap, data_dp));

    Epetra_Vector& ddf = S.ref_darcy_flux();
    ddf.ExtractView(&data_ddf);     
    darcy_flux = Teuchos::rcp(new Epetra_Vector(View, fmap, data_ddf));
  }

  S_ = S.S_;
}


/* *******************************************************************
 * Routine imports a short multivector to a parallel overlaping vector.
 ****************************************************************** */
void Flow_State::copymemory_multivector(Epetra_MultiVector& source, 
                                        Epetra_MultiVector& target)
{
  const Epetra_BlockMap& source_cmap = source.Map();
  const Epetra_BlockMap& target_cmap = target.Map();

  int cmin, cmax, cmax_s, cmax_t;
  cmin = source_cmap.MinLID();
  cmax_s = source_cmap.MaxLID();
  cmax_t = target_cmap.MaxLID();
  cmax = std::min(cmax_s, cmax_t);

  int number_vectors = source.NumVectors();
  for (int c=cmin; c<=cmax; c++) {
    for (int i=0; i<number_vectors; i++) target[i][c] = source[i][c];
  }

#ifdef HAVE_MPI
  if (cmax_s > cmax_t) {
    Errors::Message msg;
    msg << "Source map (in copy_multivector) is larger than target map.\n";
    Exceptions::amanzi_throw(msg);
  }

  Epetra_Import importer(target_cmap, source_cmap);
  target.Import(source, importer, Insert);
#endif
}


/* *******************************************************************
 * Routine imports a short vector to a parallel overlaping vector.                
 ****************************************************************** */
void Flow_State::copymemory_vector(Epetra_Vector& source, Epetra_Vector& target)
{
  const Epetra_BlockMap& source_fmap = source.Map();
  const Epetra_BlockMap& target_fmap = target.Map();

  int fmin, fmax, fmax_s, fmax_t;
  fmin   = source_fmap.MinLID();
  fmax_s = source_fmap.MaxLID();
  fmax_t = target_fmap.MaxLID();
  fmax   = std::min(fmax_s, fmax_t);

  for (int f=fmin; f<=fmax; f++) target[f] = source[f];

#ifdef HAVE_MPI
  if (fmax_s > fmax_t)  {
    Errors::Message msg;
    msg << "Source map (in copy_vector) is larger than target map.\n";
    Exceptions::amanzi_throw(msg);
  }

  Epetra_Import importer(target_fmap, source_fmap);
  target.Import(source, importer, Insert);
#endif
}


/* *******************************************************************
 * Copy data in ghost positions for a vector.              
 ****************************************************************** */
void Flow_State::distribute_cell_vector(Epetra_Vector& v)
{
#ifdef HAVE_MPI
  const Epetra_BlockMap& source_cmap = mesh_->cell_map(false);
  const Epetra_BlockMap& target_cmap = mesh_->cell_map(true);
  Epetra_Import importer(target_cmap, source_cmap);

  double* vdata;
  v.ExtractView(&vdata);
  Epetra_Vector vv(View, source_cmap, vdata);

  v.Import(vv, importer, Insert);
#endif
}


/* *******************************************************************
 * Copy data in ghost positions for a multivector.              
 ****************************************************************** */
void Flow_State::distribute_cell_multivector(Epetra_MultiVector& v)
{
#ifdef HAVE_MPI
  const Epetra_BlockMap& source_cmap = mesh_->cell_map(false);
  const Epetra_BlockMap& target_cmap = mesh_->cell_map(true);
  Epetra_Import importer(target_cmap, source_cmap);

  double** vdata;
  v.ExtractView(&vdata);
  Epetra_MultiVector vv(View, source_cmap, vdata, v.NumVectors());

  v.Import(vv, importer, Insert);
#endif
}


/* *******************************************************************
 * Extract cells from a supervector             
 ****************************************************************** */
Epetra_Vector* Flow_State::create_cell_view(const Epetra_Vector &X) const
{
  // should verify that X.Map() is the same as Map()
  double *data;
  X.ExtractView(&data);
  return new Epetra_Vector(View, mesh_->cell_map(false), data);
}


/* *******************************************************************
 * Extract faces from a supervector             
 ****************************************************************** */
Epetra_Vector* Flow_State::create_face_view(const Epetra_Vector &X) const
{
  // should verify that X.Map() is the same as Map()
  double *data;
  X.ExtractView(&data);
  int ncells = (mesh_->cell_map(false)).NumMyElements();
  return new Epetra_Vector(View, mesh_->face_map(false), data+ncells);
}


/* *******************************************************************
 * DEBUG: create constant fluid density    
 ****************************************************************** */
void Flow_State::set_fluid_density(double rho)
{
  *fluid_density = rho;  // verify that it is positive (lipnikov@lanl.gov)
}


/* *******************************************************************
 * DEBUG: create constant fluid viscosity
 ****************************************************************** */
void Flow_State::set_fluid_viscosity(double mu)
{
  *fluid_viscosity = mu;  // verify that it is positive (lipnikov@lanl.gov)
}


}  // namespace AmanziTransport
}  // namespace Amanzi

