/*
  This is the EOS component of the ATS and Amanzi codes.
   
  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Ethan Coon (ecoon@lanl.gov)

  Constant prescribed viscosity.
*/

#ifndef AMANZI_EOS_VISCOSITY_CONSTANT_HH_
#define AMANZI_EOS_VISCOSITY_CONSTANT_HH_

#include "Teuchos_ParameterList.hpp"
#include "factory.hh"
#include "dbc.hh"
#include "viscosity_relation.hh"

namespace Amanzi {
namespace Relations {

// Equation of State model
class ViscosityConstant : public ViscosityRelation {
 public:
  explicit
  ViscosityConstant(Teuchos::ParameterList& visc_plist);

  virtual double Viscosity(double T) { return visc_; }
  virtual double DViscosityDT(double T) { return 0.; }

 protected:
  virtual void InitializeFromPlist_();

  Teuchos::ParameterList visc_plist_;
  double visc_;

 private:
  static Utils::RegisteredFactory<ViscosityRelation,ViscosityConstant> factory_;
};

}  // namespace Relations
}  // namespace Amanzi

#endif
