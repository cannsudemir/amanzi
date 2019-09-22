

c ::: -----------------------------------------------------------
c ::: This routine is called at problem initialization time
c ::: and when restarting from a checkpoint file.
c ::: The purpose is (1) to specify the initial time value
c ::: (not all problems start at time=0.0) and (2) to read
c ::: problem specific data from a namelist or other input
c ::: files and possibly store them or derived information
c ::: in FORTRAN common blocks for later use.
c ::: 
c ::: INPUTS/OUTPUTS:
c ::: 
c ::: init      => TRUE if called at start of problem run
c :::              FALSE if called from restart
c ::: name      => name of "probin" file
c ::: namlen    => length of name
c ::: strttime <=  start problem with this time variable 
c ::: 
c ::: -----------------------------------------------------------
      subroutine probinit (init,name,namlen,problo,probhi)

      implicit none

      integer init,namlen
      integer name(namlen)
      integer untin, i
      DOUBLE PRECISION  problo(3), probhi(3)
      end

c ::: -----------------------------------------------------------
c ::: This routine will tag high error cells based on the 
c ::: value provided
c ::: 
c ::: INPUTS/OUTPUTS:
c ::: 
c ::: tag      <=  integer tag array
c ::: tag_l1, tag_l2, tag_l3, tag_h1, tag_h2, tag_h3 => index extent of tag array
c ::: set       => integer value to tag cell for refinement
c ::: clear     => integer value to untag cell
c ::: adv       => scalar array
c ::: adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, adv_h3 => index extent of scalar array
c ::: lo,hi     => index extent of grid
c ::: nvar      => number of components in rho array (should be 1)
c ::: domlo,hi  => index extent of problem domain
c ::: dx        => cell spacing
c ::: xlo       => physical location of lower left hand
c :::	           corner of tag array
c ::: problo    => phys loc of lower left corner of prob domain
c ::: time      => problem evolution time
c ::: value     => threshold value
c ::: min_time  => minimum time to apply this condition
c ::: max_time  => minimum time to apply this condition
c ::: max_level => maximum level to be generated by this condition
c ::: -----------------------------------------------------------
      subroutine valgterr (tag,tag_l1, tag_l2, tag_l3, tag_h1, tag_h2, t
     &ag_h3,set,clear,adv,adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, adv_
     &h3,mask,mask_l1, mask_l2, mask_l3, mask_h1, mask_h2, mask_h3,lo,
     &hi,nvar,domlo,domhi,dx,xlo,problo,time,level,value)

      implicit none

      DOUBLE PRECISION  denerr, adverr
      DOUBLE PRECISION  rhoval(1:100)
      DOUBLE PRECISION  muval(1:100)
      DOUBLE PRECISION  gravity
      integer phaseidx(1:100)
      integer ncomps, nphases, nreact, ntracers, naux
      integer probtype, model, gravity_dir

      common /probdt1/ probtype, model, gravity_dir
      common /probdt2/ denerr, adverr
      common /probdt3/ ncomps, nphases, nreact, ntracers, naux
      common /probdt4/ rhoval, muval, phaseidx
      common /probdt5/ gravity

      namelist /fortin/ probtype, denerr, adverr
      save /probdt1/, /probdt2/, /probdt3/, /probdt4/, /probdt5/

      DOUBLE PRECISION sThresh
      integer use_shifted_kr
      common /eoscomr/ sThresh
      common /eoscomi/ use_shifted_kr
      save /eoscomr/, /eoscomi/

      integer   tag_l1, tag_l2, tag_l3, tag_h1, tag_h2, tag_h3
      integer   adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, adv_h3
      integer   mask_l1, mask_l2, mask_l3, mask_h1, mask_h2, mask_h3
      integer   nvar, set, clear, level
      integer   domlo(3), domhi(3)
      integer   lo(3), hi(3)
      integer   tag(tag_l1:tag_h1, tag_l2:tag_h2, tag_l3:tag_h3)
      DOUBLE PRECISION    dx(3), xlo(3), problo(3), time
      DOUBLE PRECISION adv(adv_l1:adv_h1, adv_l2:adv_h2, adv_l3:adv_h3,1
     &)
      DOUBLE PRECISION mask(mask_l1:mask_h1, mask_l2:mask_h2, mask_l3:ma
     &sk_h3)
      DOUBLE PRECISION    value

      integer   i, j, k

      do i = lo(1), hi(1)
         do j = lo(2), hi(2)
            do k = lo(3), hi(3)
               tag(i,j,k) = merge(set,tag(i,j,k), mask(i,j,k).eq.1 .and.
     & adv(i,j,k,1).gt.value)
            end do
         end do
      end do

      end

c ::: -----------------------------------------------------------
c ::: This routine will tag high error cells based on the 
c ::: value provided
c ::: 
c ::: INPUTS/OUTPUTS:
c ::: 
c ::: tag      <=  integer tag array
c ::: tag_l1, tag_l2, tag_l3, tag_h1, tag_h2, tag_h3 => index extent of tag array
c ::: set       => integer value to tag cell for refinement
c ::: clear     => integer value to untag cell
c ::: adv       => scalar array
c ::: adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, adv_h3 => index extent of scalar array
c ::: lo,hi     => index extent of grid
c ::: nvar      => number of components in rho array (should be 1)
c ::: domlo,hi  => index extent of problem domain
c ::: dx        => cell spacing
c ::: xlo       => physical location of lower left hand
c :::	           corner of tag array
c ::: problo    => phys loc of lower left corner of prob domain
c ::: time      => problem evolution time
c ::: value     => threshold value
c ::: min_time  => minimum time to apply this condition
c ::: max_time  => minimum time to apply this condition
c ::: max_level => maximum level to be generated by this condition
c ::: -----------------------------------------------------------
      subroutine vallterr (tag,tag_l1, tag_l2, tag_l3, tag_h1, tag_h2, t
     &ag_h3,set,clear,adv,adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, adv_
     &h3,mask,mask_l1, mask_l2, mask_l3, mask_h1, mask_h2, mask_h3,lo,
     &hi,nvar,domlo,domhi,dx,xlo,problo,time,level,value)

      implicit none

      DOUBLE PRECISION  denerr, adverr
      DOUBLE PRECISION  rhoval(1:100)
      DOUBLE PRECISION  muval(1:100)
      DOUBLE PRECISION  gravity
      integer phaseidx(1:100)
      integer ncomps, nphases, nreact, ntracers, naux
      integer probtype, model, gravity_dir

      common /probdt1/ probtype, model, gravity_dir
      common /probdt2/ denerr, adverr
      common /probdt3/ ncomps, nphases, nreact, ntracers, naux
      common /probdt4/ rhoval, muval, phaseidx
      common /probdt5/ gravity

      namelist /fortin/ probtype, denerr, adverr
      save /probdt1/, /probdt2/, /probdt3/, /probdt4/, /probdt5/

      DOUBLE PRECISION sThresh
      integer use_shifted_kr
      common /eoscomr/ sThresh
      common /eoscomi/ use_shifted_kr
      save /eoscomr/, /eoscomi/

      integer   tag_l1, tag_l2, tag_l3, tag_h1, tag_h2, tag_h3
      integer   adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, adv_h3
      integer   mask_l1, mask_l2, mask_l3, mask_h1, mask_h2, mask_h3
      integer   nvar, set, clear, level
      integer   domlo(3), domhi(3)
      integer   lo(3), hi(3)
      integer   tag(tag_l1:tag_h1, tag_l2:tag_h2, tag_l3:tag_h3)
      DOUBLE PRECISION    dx(3), xlo(3), problo(3), time
      DOUBLE PRECISION adv(adv_l1:adv_h1, adv_l2:adv_h2, adv_l3:adv_h3,1
     &)
      DOUBLE PRECISION mask(mask_l1:mask_h1, mask_l2:mask_h2, mask_l3:ma
     &sk_h3)
      DOUBLE PRECISION    value

      integer   i, j, k

      do i = lo(1), hi(1)
         do j = lo(2), hi(2)
            do k = lo(3), hi(3)
               tag(i,j,k) = merge(set,tag(i,j,k), mask(i,j,k).eq.1 .and.
     & adv(i,j,k,1).lt.value)
            end do
         end do
      end do

      end

c ::: -----------------------------------------------------------
c ::: This routine will tag high error cells based on the 
c ::: value provided
c ::: 
c ::: INPUTS/OUTPUTS:
c ::: 
c ::: tag      <=  integer tag array
c ::: tag_l1, tag_l2, tag_l3, tag_h1, tag_h2, tag_h3 => index extent of tag array
c ::: set       => integer value to tag cell for refinement
c ::: clear     => integer value to untag cell
c ::: adv       => scalar array
c ::: adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, adv_h3 => index extent of scalar array
c ::: lo,hi     => index extent of grid
c ::: nvar      => number of components in rho array (should be 1)
c ::: domlo,hi  => index extent of problem domain
c ::: dx        => cell spacing
c ::: xlo       => physical location of lower left hand
c :::	           corner of tag array
c ::: problo    => phys loc of lower left corner of prob domain
c ::: time      => problem evolution time
c ::: value     => threshold value
c ::: min_time  => minimum time to apply this condition
c ::: max_time  => minimum time to apply this condition
c ::: max_level => maximum level to be generated by this condition
c ::: -----------------------------------------------------------
      subroutine diffgterr (tag,tag_l1, tag_l2, tag_l3, tag_h1, tag_h2, 
     &tag_h3,set,clear,adv,adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, adv
     &_h3,mask,mask_l1, mask_l2, mask_l3, mask_h1, mask_h2, mask_h3,lo
     &,hi,nvar,domlo,domhi,dx,xlo,problo,time,level,value)

      implicit none

      DOUBLE PRECISION  denerr, adverr
      DOUBLE PRECISION  rhoval(1:100)
      DOUBLE PRECISION  muval(1:100)
      DOUBLE PRECISION  gravity
      integer phaseidx(1:100)
      integer ncomps, nphases, nreact, ntracers, naux
      integer probtype, model, gravity_dir

      common /probdt1/ probtype, model, gravity_dir
      common /probdt2/ denerr, adverr
      common /probdt3/ ncomps, nphases, nreact, ntracers, naux
      common /probdt4/ rhoval, muval, phaseidx
      common /probdt5/ gravity

      namelist /fortin/ probtype, denerr, adverr
      save /probdt1/, /probdt2/, /probdt3/, /probdt4/, /probdt5/

      DOUBLE PRECISION sThresh
      integer use_shifted_kr
      common /eoscomr/ sThresh
      common /eoscomi/ use_shifted_kr
      save /eoscomr/, /eoscomi/

      integer   tag_l1, tag_l2, tag_l3, tag_h1, tag_h2, tag_h3
      integer   adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, adv_h3
      integer   mask_l1, mask_l2, mask_l3, mask_h1, mask_h2, mask_h3
      integer   nvar, set, clear, level
      integer   domlo(3), domhi(3)
      integer   lo(3), hi(3)
      integer   tag(tag_l1:tag_h1, tag_l2:tag_h2, tag_l3:tag_h3)
      DOUBLE PRECISION    dx(3), xlo(3), problo(3), time
      DOUBLE PRECISION adv(adv_l1:adv_h1, adv_l2:adv_h2, adv_l3:adv_h3,1
     &)
      DOUBLE PRECISION mask(mask_l1:mask_h1, mask_l2:mask_h2, mask_l3:ma
     &sk_h3)
      DOUBLE PRECISION    value
      DOUBLE PRECISION    ax, ay, az
      DOUBLE PRECISION    axp, axm, ayp, aym, azp, azm, aerr

      integer   i, j, k

      if (mask_l1.gt.lo(1)-1.or. mask_h1.lt.hi(1)+1.or. mask_l2.gt.lo(2)
     &-1.or. mask_h2.lt.hi(2)+1.or. mask_l3.gt.lo(3)-1.or. mask_h3.lt.
     &hi(3)+1 ) then
         print *,'bad mask bounds',lo,hi,mask_l1, mask_l2, mask_l3, mask
     &_h1, mask_h2, mask_h3
         print *,mask_l1, mask_l2, mask_l3, mask_h1, mask_h2, mask_h3
         print *,adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, adv_h3
         call abort()
      endif

      do i = lo(1), hi(1)
         do j = lo(2), hi(2)
            do k = lo(3), hi(3)
               if (mask(i,j,k).eq.1) then

                  if (mask(i+1,j,k).eq.1) then
                     axp = ABS(adv(i+1,j,k,1) - adv(i,j,k,1))
                  else
                     axp = 0.d0
                  endif
                  if (mask(i-1,j,k).eq.1) then
                     axm = ABS(adv(i-1,j,k,1) - adv(i,j,k,1))
                  else
                     axm = 0.d0
                  endif
                  if (mask(i,j+1,k).eq.1) then
                     ayp = ABS(adv(i,j+1,k,1) - adv(i,j,k,1))
                  else
                     ayp = 0.d0
                  endif
                  if (mask(i,j-1,k).eq.1) then
                     aym = ABS(adv(i,j-1,k,1) - adv(i,j,k,1))
                  else
                     aym = 0.d0
                  endif
                  if (mask(i,j,k+1).eq.1) then
                     azp = ABS(adv(i,j,k+1,1) - adv(i,j,k,1))
                  else
                     azp = 0.d0
                  endif
                  if (mask(i,j,k-1).eq.1) then
                     azm = ABS(adv(i,j,k-1,1) - adv(i,j,k,1))
                  else
                     azm = 0.d0
                  endif

                  aerr = MAX(axp,MAX(axm,MAX(ayp,MAX(aym,MAX(azp,azm))))
     &)

                  if (aerr.gt.value) then
                     tag(i,j,k) = set
                  endif
               endif
            end do
         end do
      end do

      end

c ::: -----------------------------------------------------------
c ::: This routine is called during a filpatch operation when
c ::: the patch to be filled falls outside the interior
c ::: of the problem domain.  You are requested to supply the
c ::: data outside the problem interior in such a way that the
c ::: data is consistant with the types of the boundary conditions
c ::: you specified in the C++ code.  
c ::: E
c ::: NOTE:  you can assume all interior cells have been filled
c :::        with valid data and that all non-interior cells have
c ::         have been filled with a large real number.
c ::: 
c ::: INPUTS/OUTPUTS:
c ::: 
c ::: rho      <=  density array
c ::: rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, rho_h3 => index extent of rho array
c ::: domlo,hi  => index extent of problem domain
c ::: dx        => cell spacing
c ::: xlo       => physical location of lower left hand
c :::	           corner of rho array
c ::: time      => problem evolution time
c ::: bc	=> array of boundary flags bc(3,lo:hi)
c ::: -----------------------------------------------------------
      subroutine onenfill (rho,rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, r
     &ho_h3,domlo,domhi,dx,xlo,time,bc)

      DOUBLE PRECISION  denerr, adverr
      DOUBLE PRECISION  rhoval(1:100)
      DOUBLE PRECISION  muval(1:100)
      DOUBLE PRECISION  gravity
      integer phaseidx(1:100)
      integer ncomps, nphases, nreact, ntracers, naux
      integer probtype, model, gravity_dir

      common /probdt1/ probtype, model, gravity_dir
      common /probdt2/ denerr, adverr
      common /probdt3/ ncomps, nphases, nreact, ntracers, naux
      common /probdt4/ rhoval, muval, phaseidx
      common /probdt5/ gravity

      namelist /fortin/ probtype, denerr, adverr
      save /probdt1/, /probdt2/, /probdt3/, /probdt4/, /probdt5/

      DOUBLE PRECISION sThresh
      integer use_shifted_kr
      common /eoscomr/ sThresh
      common /eoscomi/ use_shifted_kr
      save /eoscomr/, /eoscomi/

      integer    rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, rho_h3
      integer    domlo(3), domhi(3)
      DOUBLE PRECISION     dx(3), xlo(3), time
      DOUBLE PRECISION rho(rho_l1:rho_h1, rho_l2:rho_h2, rho_l3:rho_h3)
      integer    bc(3,2)

      call filcc(rho(rho_l1,rho_l2,rho_l3),rho_l1, rho_l2, rho_l3, rho_h
     &1, rho_h2, rho_h3,domlo,domhi,dx,xlo,bc) 

      end

c ::: -----------------------------------------------------------
c ::: This routine is called during a filpatch operation when
c ::: the patch to be filled falls outside the interior
c ::: of the problem domain.  You are requested to supply the
c ::: data outside the problem interior in such a way that the
c ::: data is consistant with the types of the boundary conditions
c ::: you specified in the C++ code.  
c ::: E
c ::: NOTE:  you can assume all interior cells have been filled
c :::        with valid data and that all non-interior cells have
c ::         have been filled with a large real number.
c ::: 
c ::: INPUTS/OUTPUTS:
c ::: 
c ::: rho      <=  density array
c ::: rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, rho_h3 => index extent of rho array
c ::: domlo,hi  => index extent of problem domain
c ::: dx        => cell spacing
c ::: xlo       => physical location of lower left hand
c :::	           corner of rho array
c ::: time      => problem evolution time
c ::: bc	=> array of boundary flags bc(3,lo:hi)
c ::: -----------------------------------------------------------
      subroutine allnfill (rho,rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, r
     &ho_h3,domlo,domhi,dx,xlo,time,bc)

      implicit none

      DOUBLE PRECISION  denerr, adverr
      DOUBLE PRECISION  rhoval(1:100)
      DOUBLE PRECISION  muval(1:100)
      DOUBLE PRECISION  gravity
      integer phaseidx(1:100)
      integer ncomps, nphases, nreact, ntracers, naux
      integer probtype, model, gravity_dir

      common /probdt1/ probtype, model, gravity_dir
      common /probdt2/ denerr, adverr
      common /probdt3/ ncomps, nphases, nreact, ntracers, naux
      common /probdt4/ rhoval, muval, phaseidx
      common /probdt5/ gravity

      namelist /fortin/ probtype, denerr, adverr
      save /probdt1/, /probdt2/, /probdt3/, /probdt4/, /probdt5/

      DOUBLE PRECISION sThresh
      integer use_shifted_kr
      common /eoscomr/ sThresh
      common /eoscomi/ use_shifted_kr
      save /eoscomr/, /eoscomi/

      integer  rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, rho_h3
      integer  domlo(3), domhi(3)
      integer  bc(3,2)
      DOUBLE PRECISION   dx(3), xlo(3), time
      DOUBLE PRECISION rho(rho_l1:rho_h1, rho_l2:rho_h2, rho_l3:rho_h3,n
     &comps)

      integer  i,j,n
      integer  bctmp(3,2)

      !
      ! I'll call filcc() with a slightly different bc.
      ! filcc() doesn't know about SEEPAGE.
      !
      do j=1,2
         do i=1,3
            bctmp(i,j) = bc(i,j)
            if (bc(i,j) .eq. 5 .or. bc(i,j) .eq. -1) then
               bctmp(i,j) = 2
            end if
         end do
      end do

      do n = 1,ncomps
        call filcc(rho(rho_l1,rho_l2,rho_l3,n),rho_l1, rho_l2, rho_l3, r
     &ho_h1, rho_h2, rho_h3,domlo,domhi,dx,xlo,bctmp)
      end do

      end

c ::: -----------------------------------------------------------
c ::: This routine is called during a filpatch operation of Tracers 
c ::: when the patch to be filled falls outside the interior
c ::: of the problem domain.  You are requested to supply the
c ::: data outside the problem interior in such a way that the
c ::: data is consistant with the types of the boundary conditions
c ::: you specified in the C++ code.  
c ::: 
c ::: NOTE:  you can assume all interior cells have been filled
c :::        with valid data and that all non-interior cells have
c ::         have been filled with a large real number.
c ::: 
c ::: INPUTS/OUTPUTS:
c ::: 
c ::: rho      <=  density array
c ::: rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, rho_h3 => index extent of rho array
c ::: domlo,hi  => index extent of problem domain
c ::: dx        => cell spacing
c ::: xlo       => physical location of lower left hand
c :::	           corner of rho array
c ::: time      => problem evolution time
c ::: bc	=> array of boundary flags bc(3,lo:hi)
c ::: -----------------------------------------------------------
      subroutine alltfill (rho,rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, r
     &ho_h3,domlo,domhi,dx,xlo,time,bc)

      DOUBLE PRECISION  denerr, adverr
      DOUBLE PRECISION  rhoval(1:100)
      DOUBLE PRECISION  muval(1:100)
      DOUBLE PRECISION  gravity
      integer phaseidx(1:100)
      integer ncomps, nphases, nreact, ntracers, naux
      integer probtype, model, gravity_dir

      common /probdt1/ probtype, model, gravity_dir
      common /probdt2/ denerr, adverr
      common /probdt3/ ncomps, nphases, nreact, ntracers, naux
      common /probdt4/ rhoval, muval, phaseidx
      common /probdt5/ gravity

      namelist /fortin/ probtype, denerr, adverr
      save /probdt1/, /probdt2/, /probdt3/, /probdt4/, /probdt5/

      DOUBLE PRECISION sThresh
      integer use_shifted_kr
      common /eoscomr/ sThresh
      common /eoscomi/ use_shifted_kr
      save /eoscomr/, /eoscomi/

      integer  rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, rho_h3
      integer  domlo(3), domhi(3)
      integer  bc(3,2)
      DOUBLE PRECISION   dx(3), xlo(3), time
      DOUBLE PRECISION rho(rho_l1:rho_h1, rho_l2:rho_h2, rho_l3:rho_h3,n
     &tracers)

      integer  i,j,n
      integer  bctmp(3,2)

      !
      ! I'll call filcc() with a slightly different bc.
      ! filcc() doesn't know about SEEPAGE.
      !
      do j=1,2
         do i=1,3
            bctmp(i,j) = bc(i,j)
            if (bc(i,j) .eq. 5) bctmp(i,j) = 2
         end do
      end do

      do n = 1,ntracers
        call filcc(rho(rho_l1,rho_l2,rho_l3,n),rho_l1, rho_l2, rho_l3, r
     &ho_h1, rho_h2, rho_h3,domlo,domhi,dx,xlo,bctmp)
      end do

      end

c ::: -----------------------------------------------------------
c ::: This routine is called during a filpatch operation of Auxilliary 
c ::: when the patch to be filled falls outside the interior
c ::: of the problem domain.  You are requested to supply the
c ::: data outside the problem interior in such a way that the
c ::: data is consistant with the types of the boundary conditions
c ::: you specified in the C++ code.  
c ::: 
c ::: NOTE:  you can assume all interior cells have been filled
c :::        with valid data and that all non-interior cells have
c ::         have been filled with a large real number.
c ::: 
c ::: INPUTS/OUTPUTS:
c ::: 
c ::: rho      <=  density array
c ::: rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, rho_h3 => index extent of rho array
c ::: domlo,hi  => index extent of problem domain
c ::: dx        => cell spacing
c ::: xlo       => physical location of lower left hand
c :::	           corner of rho array
c ::: time      => problem evolution time
c ::: bc	=> array of boundary flags bc(3,lo:hi)
c ::: -----------------------------------------------------------
      subroutine oneafill (rho,rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, r
     &ho_h3,domlo,domhi,dx,xlo,time,bc)

      integer  rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, rho_h3
      integer  domlo(3), domhi(3)
      integer  bc(3,2)
      DOUBLE PRECISION   dx(3), xlo(3), time
      DOUBLE PRECISION rho(rho_l1:rho_h1, rho_l2:rho_h2, rho_l3:rho_h3)

      integer  i,j
      integer  bctmp(3,2)

      do j=1,2
         do i=1,3
            bctmp(i,j) = 2
         end do
      end do

      call filcc(rho,rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, rho_h3,doml
     &o,domhi,dx,xlo,bctmp)

      end

c ::: -----------------------------------------------------------
c ::: This routine is called during a filpatch operation of Auxilliary 
c ::: when the patch to be filled falls outside the interior
c ::: of the problem domain.  You are requested to supply the
c ::: data outside the problem interior in such a way that the
c ::: data is consistant with the types of the boundary conditions
c ::: you specified in the C++ code.  
c ::: 
c ::: NOTE:  you can assume all interior cells have been filled
c :::        with valid data and that all non-interior cells have
c ::         have been filled with a large real number.
c ::: 
c ::: INPUTS/OUTPUTS:
c ::: 
c ::: rho      <=  density array
c ::: rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, rho_h3 => index extent of rho array
c ::: domlo,hi  => index extent of problem domain
c ::: dx        => cell spacing
c ::: xlo       => physical location of lower left hand
c :::	           corner of rho array
c ::: time      => problem evolution time
c ::: bc	=> array of boundary flags bc(3,lo:hi)
c ::: -----------------------------------------------------------
      subroutine allafill (rho,rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, r
     &ho_h3,domlo,domhi,dx,xlo,time,bc)

      DOUBLE PRECISION  denerr, adverr
      DOUBLE PRECISION  rhoval(1:100)
      DOUBLE PRECISION  muval(1:100)
      DOUBLE PRECISION  gravity
      integer phaseidx(1:100)
      integer ncomps, nphases, nreact, ntracers, naux
      integer probtype, model, gravity_dir

      common /probdt1/ probtype, model, gravity_dir
      common /probdt2/ denerr, adverr
      common /probdt3/ ncomps, nphases, nreact, ntracers, naux
      common /probdt4/ rhoval, muval, phaseidx
      common /probdt5/ gravity

      namelist /fortin/ probtype, denerr, adverr
      save /probdt1/, /probdt2/, /probdt3/, /probdt4/, /probdt5/

      DOUBLE PRECISION sThresh
      integer use_shifted_kr
      common /eoscomr/ sThresh
      common /eoscomi/ use_shifted_kr
      save /eoscomr/, /eoscomi/

      integer  rho_l1, rho_l2, rho_l3, rho_h1, rho_h2, rho_h3
      integer  domlo(3), domhi(3)
      integer  bc(3,2)
      DOUBLE PRECISION   dx(3), xlo(3), time
      DOUBLE PRECISION rho(rho_l1:rho_h1, rho_l2:rho_h2, rho_l3:rho_h3,n
     &aux)

      integer  i,j,n
      integer  bctmp(3,2)

      do j=1,2
         do i=1,3
            bctmp(i,j) = 2
         end do
      end do
      do n = 1,naux 
        call filcc(rho(rho_l1,rho_l2,rho_l3,n),rho_l1, rho_l2, rho_l3, r
     &ho_h1, rho_h2, rho_h3,domlo,domhi,dx,xlo,bctmp)
      end do

      end

      subroutine xvelfill (vel,vel_l1, vel_l2, vel_l3, vel_h1, vel_h2, v
     &el_h3,domlo,domhi,dx,xlo,time,bc )

      integer    vel_l1, vel_l2, vel_l3, vel_h1, vel_h2, vel_h3
      integer    domlo(3), domhi(3)
      DOUBLE PRECISION     dx(3), xlo(3), time
      DOUBLE PRECISION vel(vel_l1:vel_h1, vel_l2:vel_h2, vel_l3:vel_h3)
      integer    bc(3,2)

      integer    i, j

      DOUBLE PRECISION  denerr, adverr
      DOUBLE PRECISION  rhoval(1:100)
      DOUBLE PRECISION  muval(1:100)
      DOUBLE PRECISION  gravity
      integer phaseidx(1:100)
      integer ncomps, nphases, nreact, ntracers, naux
      integer probtype, model, gravity_dir

      common /probdt1/ probtype, model, gravity_dir
      common /probdt2/ denerr, adverr
      common /probdt3/ ncomps, nphases, nreact, ntracers, naux
      common /probdt4/ rhoval, muval, phaseidx
      common /probdt5/ gravity

      namelist /fortin/ probtype, denerr, adverr
      save /probdt1/, /probdt2/, /probdt3/, /probdt4/, /probdt5/

      DOUBLE PRECISION sThresh
      integer use_shifted_kr
      common /eoscomr/ sThresh
      common /eoscomi/ use_shifted_kr
      save /eoscomr/, /eoscomi/

      call filcc(vel,vel_l1, vel_l2, vel_l3, vel_h1, vel_h2, vel_h3,doml
     &o,domhi,dx,xlo,bc)
      end

      subroutine yvelfill (vel,vel_l1, vel_l2, vel_l3, vel_h1, vel_h2, v
     &el_h3,domlo,domhi,dx,xlo,time,bc )

      integer    vel_l1, vel_l2, vel_l3, vel_h1, vel_h2, vel_h3
      integer    domlo(3), domhi(3)
      DOUBLE PRECISION     dx(3), xlo(3), time
      DOUBLE PRECISION vel(vel_l1:vel_h1, vel_l2:vel_h2, vel_l3:vel_h3)
      integer    bc(3,2)

      integer    i, j

      DOUBLE PRECISION  denerr, adverr
      DOUBLE PRECISION  rhoval(1:100)
      DOUBLE PRECISION  muval(1:100)
      DOUBLE PRECISION  gravity
      integer phaseidx(1:100)
      integer ncomps, nphases, nreact, ntracers, naux
      integer probtype, model, gravity_dir

      common /probdt1/ probtype, model, gravity_dir
      common /probdt2/ denerr, adverr
      common /probdt3/ ncomps, nphases, nreact, ntracers, naux
      common /probdt4/ rhoval, muval, phaseidx
      common /probdt5/ gravity

      namelist /fortin/ probtype, denerr, adverr
      save /probdt1/, /probdt2/, /probdt3/, /probdt4/, /probdt5/

      DOUBLE PRECISION sThresh
      integer use_shifted_kr
      common /eoscomr/ sThresh
      common /eoscomi/ use_shifted_kr
      save /eoscomr/, /eoscomi/

      call filcc(vel,vel_l1, vel_l2, vel_l3, vel_h1, vel_h2, vel_h3,doml
     &o,domhi,dx,xlo,bc)
      end

      subroutine zvelfill (vel,vel_l1, vel_l2, vel_l3, vel_h1, vel_h2, v
     &el_h3,domlo,domhi,dx,xlo,time,bc )

      integer    vel_l1, vel_l2, vel_l3, vel_h1, vel_h2, vel_h3
      integer    domlo(3), domhi(3)
      DOUBLE PRECISION     dx(3), xlo(3), time
      DOUBLE PRECISION vel(vel_l1:vel_h1, vel_l2:vel_h2, vel_l3:vel_h3)
      integer    bc(3,2)

      integer    i, j

      DOUBLE PRECISION  denerr, adverr
      DOUBLE PRECISION  rhoval(1:100)
      DOUBLE PRECISION  muval(1:100)
      DOUBLE PRECISION  gravity
      integer phaseidx(1:100)
      integer ncomps, nphases, nreact, ntracers, naux
      integer probtype, model, gravity_dir

      common /probdt1/ probtype, model, gravity_dir
      common /probdt2/ denerr, adverr
      common /probdt3/ ncomps, nphases, nreact, ntracers, naux
      common /probdt4/ rhoval, muval, phaseidx
      common /probdt5/ gravity

      namelist /fortin/ probtype, denerr, adverr
      save /probdt1/, /probdt2/, /probdt3/, /probdt4/, /probdt5/

      DOUBLE PRECISION sThresh
      integer use_shifted_kr
      common /eoscomr/ sThresh
      common /eoscomi/ use_shifted_kr
      save /eoscomr/, /eoscomi/

      call filcc(vel,vel_l1, vel_l2, vel_l3, vel_h1, vel_h2, vel_h3,doml
     &o,domhi,dx,xlo,bc)
      end

c ::: -----------------------------------------------------------
c ::: This routine is called during a filpatch operation when
c ::: the patch to be filled falls outside the interior
c ::: of the problem domain.  You are requested to supply the
c ::: data outside the problem interior in such a way that the
c ::: data is consistant with the types of the boundary conditions
c ::: you specified in the C++ code.  
c ::: 
c ::: NOTE:  you can assume all interior cells have been filled
c :::        with valid data and that all non-interior cells have
c ::         have been filled with a large real number.
c ::: 
c ::: INPUTS/OUTPUTS:
c ::: 
c ::: adv      <=  advected quantity array
c ::: adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, adv_h3 => index extent of adv array
c ::: domlo,hi  => index extent of problem domain
c ::: dx        => cell spacing
c ::: xlo       => physical location of lower left hand
c :::	           corner of adv array
c ::: time      => problem evolution time
c ::: bc	=> array of boundary flags bc(3,lo:hi)
c ::: -----------------------------------------------------------
      subroutine advfill (adv,adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, ad
     &v_h3,domlo,domhi,dx,xlo,time,bc)

      integer    adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, adv_h3
      integer    domlo(3), domhi(3)
      DOUBLE PRECISION     dx(3), xlo(3), time
      DOUBLE PRECISION adv(adv_l1:adv_h1, adv_l2:adv_h2, adv_l3:adv_h3)
      integer    bc(3,2)

      integer    i, j

      DOUBLE PRECISION  denerr, adverr
      DOUBLE PRECISION  rhoval(1:100)
      DOUBLE PRECISION  muval(1:100)
      DOUBLE PRECISION  gravity
      integer phaseidx(1:100)
      integer ncomps, nphases, nreact, ntracers, naux
      integer probtype, model, gravity_dir

      common /probdt1/ probtype, model, gravity_dir
      common /probdt2/ denerr, adverr
      common /probdt3/ ncomps, nphases, nreact, ntracers, naux
      common /probdt4/ rhoval, muval, phaseidx
      common /probdt5/ gravity

      namelist /fortin/ probtype, denerr, adverr
      save /probdt1/, /probdt2/, /probdt3/, /probdt4/, /probdt5/

      DOUBLE PRECISION sThresh
      integer use_shifted_kr
      common /eoscomr/ sThresh
      common /eoscomi/ use_shifted_kr
      save /eoscomr/, /eoscomi/

      call filcc(adv,adv_l1, adv_l2, adv_l3, adv_h1, adv_h2, adv_h3,doml
     &o,domhi,dx,xlo,bc)
      end

c ::: -----------------------------------------------------------
c ::: This routine is called during a filpatch operation when
c ::: the patch to be filled falls outside the interior
c ::: of the problem domain.  You are requested to supply the
c ::: data outside the problem interior in such a way that the
c ::: data is consistant with the types of the boundary conditions
c ::: you specified in the C++ code.
c :::
c ::: NOTE:  you can assume all interior cells have been filled
c :::        with valid data and that all non-interior cells have
c ::         have been filled with a large real number.
c :::
c ::: INPUTS/OUTPUTS:
c :::
c ::: temperature <=  temperature array
c ::: temp_l1, temp_l2, temp_l3, temp_h1, temp_h2, temp_h3   => index extent of adv array
c ::: domlo,hi     => index extent of problem domain
c ::: dx           => cell spacing
c ::: xlo          => physical location of lower left hand
c :::                 corner of temperature array
c ::: time         => problem evolution time
c ::: bc           => array of boundary flags bc(3,lo:hi)
c ::: -----------------------------------------------------------
      subroutine enthfill (enthalpy,temp_l1, temp_l2, temp_l3, temp_h1, 
     &temp_h2, temp_h3,domlo,domhi,dx,xlo,time,bc )

      integer    temp_l1, temp_l2, temp_l3, temp_h1, temp_h2, temp_h3
      integer    domlo(3), domhi(3)
      DOUBLE PRECISION     dx(3), xlo(3), time
      DOUBLE PRECISION enthalpy(temp_l1:temp_h1, temp_l2:temp_h2, temp_l
     &3:temp_h3)
      integer    bc(3,2)

      integer    i, j

      DOUBLE PRECISION  denerr, adverr
      DOUBLE PRECISION  rhoval(1:100)
      DOUBLE PRECISION  muval(1:100)
      DOUBLE PRECISION  gravity
      integer phaseidx(1:100)
      integer ncomps, nphases, nreact, ntracers, naux
      integer probtype, model, gravity_dir

      common /probdt1/ probtype, model, gravity_dir
      common /probdt2/ denerr, adverr
      common /probdt3/ ncomps, nphases, nreact, ntracers, naux
      common /probdt4/ rhoval, muval, phaseidx
      common /probdt5/ gravity

      namelist /fortin/ probtype, denerr, adverr
      save /probdt1/, /probdt2/, /probdt3/, /probdt4/, /probdt5/

      DOUBLE PRECISION sThresh
      integer use_shifted_kr
      common /eoscomr/ sThresh
      common /eoscomi/ use_shifted_kr
      save /eoscomr/, /eoscomi/

      call filcc(enthalpy,temp_l1, temp_l2, temp_l3, temp_h1, temp_h2, t
     &emp_h3,domlo,domhi,dx,xlo,bc)
      end

c ::: -----------------------------------------------------------
c ::: This routine is called during a filpatch operation when
c ::: the patch to be filled falls outside the interior
c ::: of the problem domain.  You are requested to supply the
c ::: data outside the problem interior in such a way that the
c ::: data is consistant with the types of the boundary conditions
c ::: you specified in the C++ code.  
c ::: 
c ::: NOTE:  you can assume all interior cells have been filled
c :::        with valid data.
c ::: 
c ::: INPUTS/OUTPUTS:
c ::: 
c ::: p        <=  pressure array
c ::: p_l1, p_l2, p_l3, p_h1, p_h2, p_h3   => index extent of p array
c ::: domlo,hi  => index extent of problem domain
c ::: dx        => cell spacing
c ::: xlo       => physical location of lower left hand
c :::	           corner of rho array
c ::: time      => problem evolution time
c ::: bc	=> array of boundary flags bc(3,lo:hi) 
c ::: -----------------------------------------------------------
      subroutine presfill (p,p_l1, p_l2, p_l3, p_h1, p_h2, p_h3,domlo,do
     &mhi,dx,xlo,time,bc)

      integer    p_l1, p_l2, p_l3, p_h1, p_h2, p_h3
      integer    domlo(3), domhi(3)
      DOUBLE PRECISION     dx(3), xlo(3), time
      DOUBLE PRECISION     p(p_l1:p_h1, p_l2:p_h2, p_l3:p_h3)
      integer    bc(3,2)

      integer    i, j

      call filcc(p,p_l1, p_l2, p_l3, p_h1, p_h2, p_h3,domlo,domhi,dx,xlo
     &,bc)
      end

