/*
  Copyright 2010-201x held jointly by participating institutions.
  Amanzi is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors:
      Ethan Coon (coonet@ornl.gov)
*/

//! <MISSING_ONELINE_DOCSTRING>

#include <vector>

#include "UnitTest++.h"
#include "Teuchos_GlobalMPISession.hpp"
#include "Teuchos_RCP.hpp"

#include "CSR.hh"
#include "DenseVector.hh"
#include "DenseMatrix.hh"

struct TestHarness {};

using namespace Amanzi; 

SUITE(COMMON_CSR)
{
  TEST_FIXTURE(TestHarness, CSR_TEST)
  {
    const int ncells = 100; 

    // Create Matrix CSR 
    const int nmatrices = 100000; 
    const int nrows = 10; 
    const int ncols = 10; 
    CSR<double,2,Kokkos::CudaSpace> csr_mat(nmatrices); 

    // Fill the matrices on the host 
    int total = 0; 
    for(int i = 0 ; i < nmatrices; ++i){
      total += nrows*ncols; 
      int vals[] = {nrows, ncols}; 
      csr_mat.set_shape_host(i,vals,nrows*ncols); 
    }
    csr_mat.update_row_map_device(); 
    csr_mat.update_sizes_device(); 
    csr_mat.prefix_sum_device(total);

    // access and modify the matrices on device 
    Kokkos::parallel_for(
      "data_structures_CSR::test",
      csr_mat.size(),
      KOKKOS_LAMBDA(const int f) {

        WhetStone::DenseMatrix<DeviceOnlyMemorySpace> lm(
          csr_mat.at(f),
          csr_mat.size(f,0),csr_mat.size(f,1)); 
        for(int i = 0 ; i < nrows; ++i){
          for(int j = 0 ; j < ncols; ++j){
            lm(i,j) = i*nrows+j; 
          }
        }
      });
    csr_mat.update_entries_host();

    for(int f = 0 ; f < csr_mat.size(); ++f){
      WhetStone::DenseMatrix<Kokkos::HostSpace> lm(
            csr_mat.at_host(f),
            csr_mat.size_host(f,0),csr_mat.size_host(f,1)); 
      for(int i = 0 ; i < nrows; ++i){
        for(int j = 0 ; j < ncols; ++j){
          if(lm(i,j) != i*nrows+j)
            std::cout<<f<<": "<<i<<";"<<j<<" = "<<lm(i,j)<<" != "<<i*nrows+j<<std::endl;
          assert(lm(i,j) == i*nrows+j); 
        }
      }
    }
    // Create vector CSR based on matrix sizes 
    CSR<double,1,Kokkos::CudaSpace> csr_v = csr_mat.size();
    CSR<double,1,Kokkos::CudaSpace> csr_Av =csr_mat.size(); 

    int total1 = 0; 
    int total2 = 0; 
    // CSR version 
    // 1. Compute size 
    for (int i=0; i!=csr_mat.size(); ++i) {
      total1 += csr_mat.size_host(i,0);
      total2 += csr_mat.size_host(i,1);
    }

    Kokkos::parallel_for(
      "Operator_Cell::ApplyMatrixFreeOp Op_Face_Cell COPY",
      csr_mat.size(),
      KOKKOS_LAMBDA(const int& i){      
        csr_v.sizes_.view_device()(i,0) = csr_mat.size(i,0);
        csr_v.row_map_.view_device()(i) = csr_mat.size(i,0);
        csr_Av.sizes_.view_device()(i,0) = csr_mat.size(i,1);
        csr_Av.row_map_.view_device()(i) = csr_mat.size(i,1);
      });

    csr_v.prefix_sum_device(total1); 
    csr_Av.prefix_sum_device(total2);
    Kokkos::DualView<double*,DeviceOnlyMemorySpace> res; 
    res.realloc(ncells);

    // Test on device 
    Kokkos::parallel_for(
      "data_structures_CSR::test",
      csr_mat.size(),
      KOKKOS_LAMBDA(const int f) {

        WhetStone::DenseVector<DeviceOnlyMemorySpace> vv(
          csr_v.at(f),csr_v.size(f));
        WhetStone::DenseVector<DeviceOnlyMemorySpace> Avv(
          csr_Av.at(f), csr_Av.size(f));

        for (int n = 0; n != ncells; ++n) {
          vv(n) = f+n;
        }

        WhetStone::DenseMatrix<DeviceOnlyMemorySpace> lm(
          csr_mat.at(f),
          csr_mat.size(f,0),csr_mat.size(f,1)); 
        lm.Multiply(vv,Avv,false);

        for (int n = 0; n != ncells; ++n) {
          Kokkos::atomic_add(&res.view_device()(n), Avv(n));
        }
      });
    res.modify_device(); 
    res.sync_host(); 
    for(int i = 0 ; i < ncells; ++i){
      std::cout<<res.view_host()(i)<<std::endl;
    }
  }
}