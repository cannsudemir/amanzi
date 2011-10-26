/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */
#include "ion_exchange_rxn.hh"

#include <cmath>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "chemistry_exception.hh"
#include "block.hh"

#include "exceptions.hh"

namespace amanzi {
namespace chemistry {

IonExchangeRxn::IonExchangeRxn()
    : ref_cation_sorbed_conc_(1.e-9),
      uniform_z_(false),
      uniform_z_set_(false) {
  ionx_site_.clear();
  ionx_complexes_.clear();
}

IonExchangeRxn::IonExchangeRxn(
    IonExchangeSite* ionx_sites,
    const std::vector<IonExchangeComplex>& ionx_complexes) 
    : ref_cation_sorbed_conc_(1.e-9),
      uniform_z_(false),
      uniform_z_set_(false) {
  // surface site
  ionx_site_.push_back(*ionx_sites);

  // surface complexes
  for (std::vector<IonExchangeComplex>::const_iterator i = ionx_complexes.begin();
       i != ionx_complexes.end(); i++) {
    ionx_complexes_.push_back(*i);
  }
}

IonExchangeRxn::IonExchangeRxn(IonExchangeSite ionx_sites) 
    : ref_cation_sorbed_conc_(1.e-9),
      uniform_z_(false),
      uniform_z_set_(false) {
  // surface site
  ionx_site_.push_back(ionx_sites);
  ionx_complexes_.clear();
}

IonExchangeRxn::~IonExchangeRxn() {
}

void IonExchangeRxn::AddIonExchangeComplex(const IonExchangeComplex& ionx_complex) {
  ionx_complexes_.push_back(ionx_complex);
}

void IonExchangeRxn::AddIonExchangeSite(const IonExchangeSite& site) {
  ionx_site_.push_back(site);
}

void IonExchangeRxn::Update(const std::vector<Species>& primarySpecies) {

  bool one_more;
  double tol = 1.e-12;

  if (!uniform_z_set()) {
    CheckUniformZ(primarySpecies);
  }

  for (std::vector<IonExchangeComplex>::iterator ionx = ionx_complexes_.begin(); 
       ionx != ionx_complexes_.end(); ionx++)
    ionx->set_X(0.);

  double omega = ionx_site_[0].cation_exchange_capacity();
  if (!uniform_z()) { // Z_i /= Z_j for all i,j
    int interation_count = 0;
    int ref_cation = ionx_complexes_[0].primary_id();
    double ref_cation_act = primarySpecies[ref_cation].activity();
    double ref_cation_Z = primarySpecies[ref_cation].charge();
    double ref_cation_K = ionx_complexes_[0].K();
    double ref_cation_X = ref_cation_Z*ref_cation_sorbed_conc_/omega;
    one_more = false;
    while(true) {
      if (ref_cation_X < 0.) ref_cation_X = 0.99;
      ionx_complexes_[0].set_X(ref_cation_X);
      double ref_cation_quotient = ref_cation_X*ref_cation_K/ref_cation_act;
      double total = ref_cation_X;
      for (unsigned int i = 1; i < ionx_complexes_.size(); i++) {
        int icomp = ionx_complexes_[i].primary_id();
        double value = primarySpecies[icomp].activity()/
                       ionx_complexes_[i].K()*
                       pow(ref_cation_quotient, 
                           primarySpecies[icomp].charge()/
                             ref_cation_Z);
        ionx_complexes_[i].set_X(value);
        total += value;
      }
      ++interation_count;
      if (one_more) break;
      double res = 1. - total;
      double dres_dref_cation_X = 1.;
      for (unsigned int i = 1; i < ionx_complexes_.size(); i++) {
        int icomp = ionx_complexes_[i].primary_id();
        dres_dref_cation_X += primarySpecies[icomp].charge()*
                              ionx_complexes_[i].X()/ref_cation_X;
      }
      double dref_cation_X = -res / dres_dref_cation_X;
      ref_cation_X -= dref_cation_X;
      if (abs(dref_cation_X/ref_cation_X) < tol) one_more = true;
    }
    ref_cation_sorbed_conc_ = ref_cation_X*omega/ref_cation_Z;
  }
  else { // Z_i == Z_j for all i,j
    double sumkm = 0.;
    for (std::vector<IonExchangeComplex>::iterator ionx = ionx_complexes_.begin(); 
       ionx != ionx_complexes_.end(); ionx++) {
      int icomp = ionx->primary_id();
      double value = primarySpecies[icomp].activity()*
                     ionx->K();
      ionx->set_X(value);
      sumkm += value;
    }
    for (std::vector<IonExchangeComplex>::iterator ionx = ionx_complexes_.begin(); 
       ionx != ionx_complexes_.end(); ionx++) {
      double temp = ionx->X() / sumkm;
      ionx->set_X(temp);
    }
  }

  for (std::vector<IonExchangeComplex>::iterator ionx =
           ionx_complexes_.begin();
       ionx != ionx_complexes_.end(); ionx++) {
    int icomp = ionx->primary_id();
    ionx->set_concentration(ionx->X()*omega/primarySpecies[icomp].charge());
  }

}  // end Update()

void IonExchangeRxn::AddContributionToTotal(std::vector<double> *total) {
  for (std::vector<IonExchangeComplex>::iterator ionx =
           ionx_complexes_.begin();
       ionx != ionx_complexes_.end(); ionx++) {
    int icomp = ionx->primary_id();
    (*total)[icomp] += ionx->concentration();
  }
}  // end AddContributionToTotal()

void IonExchangeRxn::AddContributionToDTotal(
    const std::vector<Species>& primarySpecies,
    Block* dtotal) {

  // sum up charges
  double sumZX = 0.;
  for (std::vector<IonExchangeComplex>::iterator ionx = ionx_complexes_.begin(); 
       ionx != ionx_complexes_.end(); ionx++) {
    int icomp = ionx->primary_id();
    sumZX += primarySpecies[icomp].charge()*ionx->X();
  }

  // add contribution to derivatives
  for (std::vector<IonExchangeComplex>::iterator ionx =
           ionx_complexes_.begin();
       ionx != ionx_complexes_.end(); ionx++) {
    int icomp = ionx->primary_id();
    double temp = primarySpecies[icomp].charge()*sumZX;
    for (std::vector<IonExchangeComplex>::iterator ionx2 =
             ionx_complexes_.begin();
         ionx2 != ionx_complexes_.end(); ionx2++) {
      int jcomp = ionx2->primary_id();
      double value;
      if (ionx == ionx2) {
        value = ionx->concentration()*(1.-temp*ionx2->X())/primarySpecies[jcomp].molality();
      }
      else {
        value = -ionx->concentration()*temp*ionx2->X()/primarySpecies[jcomp].molality();
      }
      dtotal->addValue(icomp,jcomp,value);
    }
  }
}  // end AddContributionToDTotal()

void IonExchangeRxn::CheckUniformZ(const std::vector<Species>& primarySpecies) {
  bool uniform_z = true;
  for (unsigned int i = 0; i < ionx_complexes_.size(); i++) {
    for (unsigned int j = i+1; j < ionx_complexes_.size(); j++) {
      if (primarySpecies[ionx_complexes_[i].primary_id()].charge() != 
          primarySpecies[ionx_complexes_[j].primary_id()].charge()) {
        uniform_z = false;
        i = ionx_complexes_.size();
        break;
      }
    }
  }
  set_uniform_z(uniform_z);
}

void IonExchangeRxn::DisplaySite(void) const {
  std::vector<IonExchangeSite>::const_iterator site;
  for (site = ionx_site_.begin(); site != ionx_site_.end(); site++) {
    site->Display();
  }
}  // end DisplaySite()

void IonExchangeRxn::DisplayComplexes(void) const {
  std::vector<IonExchangeComplex>::const_iterator complex;
  for (complex = ionx_complexes_.begin();
       complex != ionx_complexes_.end(); complex++) {
    complex->Display();
  }
}  // end DisplayComplexes()

void IonExchangeRxn::Display(void) const {
  DisplaySite();
  DisplayComplexes();
}  // end Display()

void IonExchangeRxn::display(void) const {
  DisplaySite();
  DisplayComplexes();
}  // end display()

void IonExchangeRxn::DisplayResultsHeader(void) const {
  std::cout << std::setw(15) << "---"
            << std::endl;
}  // end DisplayResultsHeader()

void IonExchangeRxn::DisplayResults(void) const {
  ionx_site_[0].DisplayResultsHeader();
  std::vector<IonExchangeSite>::const_iterator site;
  for (site = ionx_site_.begin();
       site != ionx_site_.end(); site++) {
    site->DisplayResults();
  }

  ionx_complexes_[0].DisplayResultsHeader();
  std::vector<IonExchangeComplex>::const_iterator complex;
  for (complex = ionx_complexes_.begin();
       complex != ionx_complexes_.end(); complex++) {
    complex->DisplayResults();
  }
}  // end DisplayResults()

}  // namespace chemistry
}  // namespace amanzi
