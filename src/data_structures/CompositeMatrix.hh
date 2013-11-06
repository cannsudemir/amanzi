/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */
/* -------------------------------------------------------------------------

ATS

License: see $ATS_DIR/COPYRIGHT
Author: Ethan Coon (ecoon@lanl.gov)

Interface for a Matrix that acts on CompositeVector.
------------------------------------------------------------------------- */

#ifndef AMANZI_COMPOSITEMATRIX_HH_
#define AMANZI_COMPOSITEMATRIX_HH_

#include "Teuchos_RCP.hpp"

namespace Amanzi {

class CompositeVector;
class CompositeVectorSpace;

class CompositeMatrix {

 public:

  // Vector space of the Matrix's domain.
  virtual const CompositeVectorSpace& DomainMap() const = 0;

  // Vector space of the Matrix's range.
  virtual const CompositeVectorSpace& RangeMap() const = 0;

  // Virtual copy constructor.
  virtual Teuchos::RCP<CompositeMatrix> Clone() const = 0;

  // Apply matrix, b <-- Ax, returns ierr
  virtual int Apply(const CompositeVector& x,
                     CompositeVector& b) const = 0;

  // Apply the inverse, x <-- A^-1 b, returns ierr
  virtual int ApplyInverse(const CompositeVector& b,
                            CompositeVector& x) const = 0;

};

} // namespace

#endif
