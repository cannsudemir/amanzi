#include "Epetra_IntVector.h"

#include "RichardsProblem.hpp"

RichardsProblem::RichardsProblem(const Teuchos::RCP<Mesh_maps_base> &mesh,
			   Teuchos::ParameterList &list,
			   const Teuchos::RCP<FlowBC> &bc) : mesh_(mesh), bc_(bc)
{
  // Create the combined cell/face DoF map.
  dof_map_ = create_dof_map_(CellMap(), FaceMap());

  face_importer_ = new Epetra_Import(FaceMap(true),FaceMap(false));
  cell_importer_ = new Epetra_Import(CellMap(true),CellMap(false));

  // Create the MimeticHexLocal objects.
  init_mimetic_disc_(*mesh, MD);
  md_ = new MimeticHex(mesh); // evolving replacement for mimetic_hex

  // Create the diffusion matrix (structure only, no values)
  D_ = Teuchos::rcp<DiffusionMatrix>(create_diff_matrix_(mesh, bc));

  // Create the preconditioner (structure only, no values)
  Teuchos::ParameterList diffprecon_plist = list.sublist("Diffusion Preconditioner");
  precon_ = new DiffusionPrecon(D_, diffprecon_plist, Map());

  // DEFINE DEFAULTS FOR PROBLEM PARAMETERS
  rho_ = 1.0;
  mu_  = 1.0;
  k_.resize(CellMap(true).NumMyElements(), 1.0);
  k_rl_.resize(CellMap(true).NumMyElements(), 1.0);
  for (int i = 0; i < 3; ++i) g_[i] = 0.0; // no gravity

  // read values for the van Genuchten model
  vG_m_      = list.get<double>("van Genuchten m");
  vG_n_      = 1.0/(1.0-vG_m_); 
  vG_alpha_  = list.get<double>("van Genuchten alpha");
  vG_sr_     = list.get<double>("van Genuchten residual saturation");
  p_atm_     = list.get<double>("atmospheric pressure");
}


RichardsProblem::~RichardsProblem()
{
  delete dof_map_;
  delete face_importer_;
  delete cell_importer_;
  delete md_;
  delete precon_;
}


DiffusionMatrix* RichardsProblem::create_diff_matrix_(const Teuchos::RCP<Mesh_maps_base> &mesh, const Teuchos::RCP<FlowBC> &bc) const
{
  // Generate the list of all Dirichlet-type faces.
  // The provided lists should include all used BC faces.
  std::vector<int> dir_faces;
  for (int j = 0; j < bc->NumBC(); ++j) {
    FlowBC::bc_spec& BC = (*bc)[j];
    switch (BC.Type) {
      case FlowBC::PRESSURE_CONSTANT:
      case FlowBC::STATIC_HEAD:
        dir_faces.reserve(dir_faces.size() + BC.Faces.size());
        for (int i = 0; i < BC.Faces.size(); ++i)
          dir_faces.push_back(BC.Faces[i]);
        break;
    }
  }
  return new DiffusionMatrix(mesh, dir_faces);
}


void RichardsProblem::init_mimetic_disc_(Mesh_maps_base &mesh, std::vector<MimeticHexLocal> &MD) const
{
  // Local storage for the 8 vertex coordinates of a hexahedral cell.
  double x[8][3];
  double *xBegin = &x[0][0];  // begin iterator
  double *xEnd = xBegin+24;   // end iterator

  MD.resize(mesh.cell_map(true).NumMyElements());
  for (int j = 0; j < MD.size(); ++j) {
    mesh.cell_to_coordinates((unsigned int) j, xBegin, xEnd);
    MD[j].update(x);
  }
}


void RichardsProblem::SetFluidDensity(double rho)
{
  /// should verify rho > 0
  rho_ = rho;
}


void RichardsProblem::SetFluidViscosity(double mu)
{
  /// should verify mu > 0
  mu_ = mu;
}


void RichardsProblem::SetGravity(const double g[3])
{
  for (int i = 0; i < 3; ++i) g_[i] = g[i];
}


void RichardsProblem::SetPermeability(double k)
{
  /// should verify k > 0
  for (int i = 0; i < k_.size(); ++i) k_[i] = k;
}


void RichardsProblem::SetPermeability(const Epetra_Vector &k)
{
  /// should verify k.Map() is CellMap()
  /// should verify k values are all > 0
  Epetra_Vector k_ovl(mesh_->cell_map(true));
  Epetra_Import importer(mesh_->cell_map(true), mesh_->cell_map(false));

  k_ovl.Import(k, importer, Insert);

  for (int i = 0; i < k_.size(); ++i) k_[i] = k_ovl[i];
}


void RichardsProblem::UpdateVanGenuchtenRelativePermeability(const Epetra_Vector &P)
{
  for (int i=0; i<k_rl_.size(); i++)
    {
      // first calculate the capillary pressure (reference pressure is one bar)

      double pc = P[i] - p_atm_;

      // if the capillary pressure is negative, we apply the van Genuchten model
      if (pc < 0.0)
	{
	  // compute the effective saturation
	  double se = pow(1.0 + pow(-vG_alpha_*P[i],vG_n_),-vG_m_);
	  
	  // from that we can compute the relative permeability 
	  k_rl_[i] = sqrt(se)*pow(1.0-pow(1.0-pow(se,1.0/vG_m_),vG_m_),2.0);
	}
      else
	{
	  // if the capillary pressure is positive, we're in the saturated 
	  k_rl_[i] = 1.0;
	}

    }
}


void RichardsProblem::dSofP(const Epetra_Vector &P, Epetra_Vector &dS)
{
  for (int i=0; i<P.MyLength(); i++)
    {
      // compute the capillary pressure
      double pc =  P[i] - p_atm_;
      
      if (pc < 0.0) 
	{
	  // compute the derivative of liquid saturation using the effective and residual saturation
	  dS[i] = -vG_m_ * pow(1.0 + pow(-vG_alpha_*pc,vG_n_),-vG_m_-1.0) 
	    * vG_n_ * pow(-vG_alpha_*pc,vG_n_ - 1.0) * (-vG_alpha_) * (1.0 - vG_sr_);
	}
      else
	{
	  // where the capillary pressure is positive, saturation is one and the
	  // derivative of saturation is zero 
	  dS[i] = 0.0;
	}      




    }
}

void RichardsProblem::DeriveVanGenuchtenSaturation(const Epetra_Vector &P, Epetra_Vector &S)
{
  for (int i=0; i<P.MyLength(); i++)
    {
      // compute the capillary pressure
      double pc =  P[i] - p_atm_;
      
      if (pc < 0.0) 
	{
	  // compute the liquid saturation using the effective and residual saturation
	  S[i] = pow(1.0 + pow(-vG_alpha_*pc,vG_n_),-vG_m_) * (1.0 - vG_sr_) + vG_sr_;

	}
      else
	{
	  // where the capillary pressure is positive, saturation is one
	  S[i] = 1.0;
	}
      
    }
}


// void RichardsProblem::DeriveVanGenuchtenPressure(const Epetra_Vector &S, Epetra_Vector &P)
// {
//   // derive from a one-off constant effective saturation
//   double se = 0.9
  
//   for (int i=0; i<S.MyLength(); i++)
//     {
//       P[i] = 1/vG_alpha_ * pow( pow(se,-1/vG_m_) - 1.0, 1/vG_n_) ;
//     }
// }



void RichardsProblem::ComputePrecon(const Epetra_Vector &X)
{
  // Fill the diffusion matrix with values.
  // THIS IS WHERE WE USE THE CELL PRESSURE PART OF X TO EVALUATE THE RELATIVE PERMEABILITY
  std::vector<double> K(k_);

  Epetra_Vector &Pcell_own = *CreateCellView(X);
  Epetra_Vector Pcell(CellMap(true));
  Pcell.Import(Pcell_own, *cell_importer_, Insert);

  UpdateVanGenuchtenRelativePermeability(Pcell);
  
  for (int j = 0; j < K.size(); ++j) K[j] = rho_ * K[j] * k_rl_[j] / mu_;
  D_->Compute(K);

  // Compute the face Schur complement of the diffusion matrix.
  D_->ComputeFaceSchur();

  // Compute the preconditioner from the newly computed diffusion matrix and Schur complement.
  precon_->Compute();
}



void RichardsProblem::ComputePrecon(const Epetra_Vector &X, const double h)
{
  // Fill the diffusion matrix with values.
  // THIS IS WHERE WE USE THE CELL PRESSURE PART OF X TO EVALUATE THE RELATIVE PERMEABILITY
  std::vector<double> K(k_);

  Epetra_Vector &Pcell_own = *CreateCellView(X);
  Epetra_Vector Pcell(CellMap(true));
  Pcell.Import(Pcell_own, *cell_importer_, Insert);

  UpdateVanGenuchtenRelativePermeability(Pcell);
  
  for (int j = 0; j < K.size(); ++j) K[j] = rho_ * K[j] * k_rl_[j] / mu_;
  D_->Compute(K);

  // add the time derivative to the diagonal
  Epetra_Vector celldiag(CellMap(false));
  dSofP(Pcell_own, celldiag);
  celldiag.Scale(1.0/h);
  
  D_->add_to_celldiag(celldiag);

  // Compute the face Schur complement of the diffusion matrix.
  D_->ComputeFaceSchur();

  // Compute the preconditioner from the newly computed diffusion matrix and Schur complement.
  precon_->Compute();
}



void RichardsProblem::ComputeF(const Epetra_Vector &X, Epetra_Vector &F)
{
  // The cell and face-based DoF are packed together into the X and F Epetra
  // vectors: cell-based DoF in the first part, followed by the face-based DoF.
  // In addition, only the owned DoF belong to the vectors.


  // Create views into the cell and face segments of X and F
  Epetra_Vector &Pcell_own = *CreateCellView(X);
  Epetra_Vector &Pface_own = *CreateFaceView(X);

  Epetra_Vector &Fcell_own = *CreateCellView(F);
  Epetra_Vector &Fface_own = *CreateFaceView(F);


  // Create input cell and face pressure vectors that include ghosts.
  Epetra_Vector Pcell(CellMap(true));
  Pcell.Import(Pcell_own, *cell_importer_, Insert);

  UpdateVanGenuchtenRelativePermeability(Pcell);

  Epetra_Vector Pface(FaceMap(true));
  Pface.Import(Pface_own, *face_importer_, Insert);

  // Apply initial BC fixups to PFACE.
  apply_BC_initial_(Pface); // modifies used values

  // Create cell and face result vectors that include ghosts.
  Epetra_Vector Fcell(CellMap(true));
  Epetra_Vector Fface(FaceMap(true));

  int cface[6];
  double aux1[6], aux2[6], gflux[6];

  Fface.PutScalar(0.0);
  for (int j = 0; j < Pcell.MyLength(); ++j) {
    // Get the list of process-local face indices for this cell.
    mesh_->cell_to_faces((unsigned int) j, (unsigned int*) cface, (unsigned int*) cface+6);
    // Gather the local face pressures int AUX1.
    for (int k = 0; k < 6; ++k) aux1[k] = Pface[cface[k]];
    // Compute the local value of the diffusion operator.
    double K = (rho_ * k_[j] * k_rl_[j] / mu_);
    MD[j].diff_op(K, Pcell[j], aux1, Fcell[j], aux2);
    // Gravity contribution
    MD[j].GravityFlux(g_, gflux);
    for (int k = 0; k < 6; ++k) aux2[k] -= rho_ * K * gflux[k];
    // Scatter the local face result into FFACE.
    for (int k = 0; k < 6; ++k) Fface[cface[k]] += aux2[k];
  }

  // Apply final BC fixups to FFACE.
  apply_BC_final_(Fface); // modifies used values

  // Copy owned part of result into the output vectors.
  for (int j = 0; j < Fcell_own.MyLength(); ++j) Fcell_own[j] = Fcell[j];
  for (int j = 0; j < Fface_own.MyLength(); ++j) Fface_own[j] = Fface[j];

  delete &Pcell_own, &Pface_own, &Fcell_own, &Fface_own;
}


// BC fixups for F computation: initial pass.
void RichardsProblem::apply_BC_initial_(Epetra_Vector &Pface)
{
  for (int j = 0; j < (*bc_).NumBC(); ++j) {
    FlowBC::bc_spec& BC = (*bc_)[j];
    switch (BC.Type) {
      case FlowBC::PRESSURE_CONSTANT:
        for (int i = 0; i < BC.Faces.size(); ++i) {
          int n = BC.Faces[i];
          BC.Aux[i] = Pface[n] - BC.Value;
          Pface[n] = BC.Value;
        }
        break;
      // WARNING: THIS IS A ONE-OFF HACK.
      // It is assumed that g is aligned with z.
      // The BC value is used to set the height (for the entire
      // set of BC faces) of zero pressure.
      case FlowBC::STATIC_HEAD:
        for (int i = 0; i < BC.Faces.size(); ++i) {
          int n = BC.Faces[i];
          double x[3];
          face_centroid_(n, x);
          double p = rho_ * g_[2] * ( x[2]  - BC.Value);
          BC.Aux[i] = Pface[n] - p - p_atm_;
          Pface[n] = p + p_atm_;
        }
        break;
    }
  }
}

void RichardsProblem::face_centroid_(int face, double xc[])
{
   // Local storage for the 4 vertex coordinates of a face.
  double x[4][3];
  double *xBegin = &x[0][0];  // begin iterator
  double *xEnd = xBegin+12;   // end iterator

  mesh_->face_to_coordinates((unsigned int) face, xBegin, xEnd);

  for (int k = 0; k < 3; ++k) {
    double s = 0.0;
    for (int i = 0; i < 4; ++i)
       s += x[i][k];
    xc[k] = s / 4.0;
  }
}


// BC fixups for F computation: final pass.
void RichardsProblem::apply_BC_final_(Epetra_Vector &Fface)
{
  for (int j = 0; j < (*bc_).NumBC(); ++j) {
    FlowBC::bc_spec& BC = (*bc_)[j];
    switch (BC.Type) {
      case FlowBC::PRESSURE_CONSTANT:
      case FlowBC::STATIC_HEAD:
        for (int i = 0; i < BC.Faces.size(); ++i) {
          int n = BC.Faces[i];
          Fface[n] = BC.Aux[i];
        }
        break;
      case FlowBC::DARCY_CONSTANT:
        for (int i = 0; i < BC.Faces.size(); ++i) {
          int n = BC.Faces[i];
          Fface[n] += rho_ * BC.Value * md_->face_area_[n];
        }
        break;
      case FlowBC::NO_FLOW:
        // The do-nothing boundary condition.
        break;
    }
  }
}


Epetra_Map* RichardsProblem::create_dof_map_(const Epetra_Map &cell_map, const Epetra_Map &face_map) const
{
  // Create the combined cell/face DoF map
  int ncell_tot = cell_map.NumGlobalElements();
  int ndof_tot = ncell_tot + face_map.NumGlobalElements();
  int ncell = cell_map.NumMyElements();
  int ndof = ncell + face_map.NumMyElements();
  int *gids = new int[ndof];
  cell_map.MyGlobalElements(&(gids[0]));
  face_map.MyGlobalElements(&(gids[ncell]));
  for (int i = ncell; i < ndof; ++i) gids[i] += ncell_tot;
  Epetra_Map *map = new Epetra_Map(ndof_tot, ndof, gids, 0, cell_map.Comm());
  delete [] gids;
  return map;
}


Epetra_Vector* RichardsProblem::CreateCellView(const Epetra_Vector &X) const
{
  // should verify that X.Map() is the same as Map()
  double *data;
  X.ExtractView(&data);
  return new Epetra_Vector(View, CellMap(), data);
}


Epetra_Vector* RichardsProblem::CreateFaceView(const Epetra_Vector &X) const
{
  // should verify that X.Map() is the same as Map()
  double *data;
  X.ExtractView(&data);
  int ncell = CellMap().NumMyElements();
  return new Epetra_Vector(View, FaceMap(), data+ncell);
}


void RichardsProblem::DeriveDarcyVelocity(const Epetra_Vector &X, Epetra_MultiVector &Q) const
{
  // should verify X.Map() is Map()
  // should verify Q.Map() is CellMap(false)
  // should verify Q.NumVectors() is 3

  // Create views into the cell and face segments of X and F
  Epetra_Vector &Pcell = *CreateCellView(X);
  Epetra_Vector &Pface_own = *CreateFaceView(X);

  // Create face vectors that include ghosts.
  Epetra_Vector Pface(FaceMap(true));

  // Populate the face pressure vector from the input.
  Pface.Import(Pface_own, *face_importer_, Insert);

  int cface[6];
  double aux1[6], aux2[6], aux3[3], gflux[6], dummy;

  for (int j = 0; j < CellMap(false).NumMyElements(); ++j) {

  }
  for (int j = 0; j < Pcell.MyLength(); ++j) {
    mesh_->cell_to_faces((unsigned int) j, (unsigned int*) cface, (unsigned int*) cface+6);
    for (int k = 0; k < 6; ++k) aux1[k] = Pface[cface[k]];
    double K = (k_[j] / mu_);
    MD[j].diff_op(K, Pcell[j], aux1, dummy, aux2);
    MD[j].GravityFlux(g_, gflux);
    for (int k = 0; k < 6; ++k) aux2[k] = rho_ * K * gflux[k] - aux2[k];
    MD[j].CellFluxVector(aux2, aux3);
    Q[0][j] = aux3[0];
    Q[1][j] = aux3[1];
    Q[2][j] = aux3[2];
  }

  delete &Pcell, &Pface_own;
}


void RichardsProblem::DeriveDarcyFlux(const Epetra_Vector &P, Epetra_Vector &F, double &l2_error) const
{
  /// should verify P.Map() is Map()
  /// should verify F.Map() is FaceMap()

  int fdirs[6];
  unsigned int cface[6];
  double aux1[6], aux2[6], gflux[6], dummy;

  // Create a view into the cell pressure segment of P.
  Epetra_Vector &Pcell_own = *CreateCellView(P);

  // Create a copy of the face pressure segment of P that includes ghosts.
  Epetra_Vector &Pface_own = *CreateFaceView(P);
  Epetra_Vector Pface(FaceMap(true));
  Pface.Import(Pface_own, *face_importer_, Insert);

  // Create face flux and face count vectors that include ghosts.
  Epetra_Vector Fface(FaceMap(true)); // fills with 0
  Epetra_Vector error(FaceMap(true)); // fills with 0
  Epetra_IntVector count(FaceMap(true)); // fills with 0

  // Process-local assembly of the face mass fluxes.
  for (unsigned int j = 0; j < Pcell_own.MyLength(); ++j) {
    // Get the list of process-local face indices for this cell.
    mesh_->cell_to_faces(j, cface, cface+6);
    // Gather the local face pressures int AUX1.
    for (int k = 0; k < 6; ++k) aux1[k] = Pface[cface[k]];
    // Compute the local value of the diffusion operator.
    double K = (rho_ * k_[j] *k_rl_[j] / mu_);
    MD[j].diff_op(K, Pcell_own[j], aux1, dummy, aux2);
    // Gravity contribution
    MD[j].GravityFlux(g_, gflux);
    for (int k = 0; k < 6; ++k) aux2[k] = rho_ * K * gflux[k] - aux2[k];
    mesh_->cell_to_face_dirs(j, fdirs, fdirs+6);
    // Scatter the local face result into FFACE.
    for (int k = 0; k < 6; ++k) {
      Fface[cface[k]] += fdirs[k] * aux2[k];
      error[cface[k]] += aux2[k]; // sums to the flux discrepancy
      count[cface[k]]++;
    }
  }

  // Global assembly of face mass fluxes into the return vector.
  F.Export(Fface, *face_importer_, Add);

  // Create an owned face count vector that overlays the count vector with ghosts.
  int *count_data;
  count.ExtractView(&count_data);
  Epetra_IntVector count_own(View, FaceMap(false), count_data);

  // Final global assembly of the face count vector.
  count_own.Export(count, *face_importer_, Add);

  // Correct the double counting of fluxes on interior faces and convert
  // mass flux to Darcy flux by dividing by the constant fluid density.
  for (int j = 0; j < F.MyLength(); ++j)
    F[j] = F[j] / (rho_ * count[j]);

  // Create an owned face error vector that overlays the error vector with ghosts.
  double *error_data;
  error.ExtractView(&error_data);
  Epetra_Vector error_own(View, FaceMap(false), error_data);

  // Final global assembly of the flux discrepancy vector.
  error_own.Export(error, *face_importer_, Add);

  // Set the flux discrepancy error to 0 on boundary faces where there was only one flux computed.
  for (int j = 0; j < F.MyLength(); ++j)
    if (count[j] == 1) error_own[j] = 0.0;

  // Compute the norm of the flux discrepancy.
  error_own.Norm2(&l2_error);

  delete &Pcell_own, &Pface_own;
}


void RichardsProblem::Compute_udot(const double t, const Epetra_Vector& u, Epetra_Vector &udot)
{
  ComputeF(u,udot);

  // zero out the face part
  Epetra_Vector *udot_face = CreateFaceView(udot);
  udot_face->PutScalar(0.0);

  udot.Print(cout);
}
