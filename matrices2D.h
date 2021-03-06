/*

POZOR:

PRI GENEROVANI LAPLACEOVY MATICE BY SE MELO VSUDE PRICITAT - 
                                                       VZDY TOMU TAK ALE NENI!!!

*/

#if DIM == 2

/******************************************************************************/
/*                                                                            */
/*                           2D stiffness matrices                            */
/*                                                                            */
/******************************************************************************/

void mapped_points();
FLOAT integr_rhs_zeta();
FLOAT gsd_tau();
FLOAT gsc_tau_k();
FLOAT integr_f_X_phi_q1();
FLOAT lp_delta();
FLOAT lpm_delta();
INT is_macro_node();
void edges_for_b_macros();
void sggem();
void ggem();

FLOAT heaviside(x)
FLOAT x;
{
   if (x > 0.)
      return(1.);
   else
      return(0.);
}

FLOAT regularized_sign_divided_by_x(x)
FLOAT x;
{
   if (fabs(x) < 1.e-10)
      return(1.);
   else
      return(tanh(x)/x);
}

FLOAT regularized_fabs(x,eps)
FLOAT x, eps;
{
   if (fabs(x) < 1.e-10)
      return(0.);
   else
      return(x*tanh(x/eps));
}

FLOAT dx_of_regularized_fabs(x,eps)
FLOAT x, eps;
{
   FLOAT y;

   if (fabs(x) < 1.e-10)
      return(2.*x/eps);
   else{
      x /= eps;
      y = cosh(x);
      return(tanh(x) + x/(y*y));
   }
}

/* f is a linear function on an interval, f0 and f1 are its values at the
end points; the integral mean value of |f| over the interval is computed */
DOUBLE mean_abs_value_1d(f0,f1)
DOUBLE f0, f1;
{
   DOUBLE max;

   if (f0*f1 >= 0.)
      return(0.5*fabs(f0+f1));
   else{
      max = MAX(fabs(f0),fabs(f1));
      f0 /= max;
      f1 /= max;
      return(0.5*max*(f0*f0 + f1*f1)/fabs(f0-f1));
   }
}

DOUBLE L1_norm_of_p1_fcn_on_triangle_special(x0,x1,x2,f0,f1,f2)
DOUBLE *x0, *x1, *x2, f0, f1, f2;  /*  f0*f1 >= 0  */
{
   DOUBLE x02[DIM], x12[DIM], p, q;

   if (f0 < 0. || f1 < 0.){
      f0 = -f0;
      f1 = -f1;
      f2 = -f2;
   }
   if (f2 < 0. && f2 > -1.e-30)
      f2 = 0.;
   if (f2 >= 0.)
      return((f0+f1+f2)*AREA_OF_TRIANGLE(x0,x1,x2)/3.);
   else{
      f2 = -f2;
      q = f0 + f2;
      p = f2/q; 
      q = f0/q;
      SET20(x02,x0,p,x2,q)
      q = f1 + f2;
      p = f2/q; 
      q = f1/q;
      SET20(x12,x1,p,x2,q)
      return(((f0+f1)*AREA_OF_TRIANGLE(x0,x1,x02)
                 + f1*AREA_OF_TRIANGLE(x1,x02,x12)
                 + f2*AREA_OF_TRIANGLE(x2,x02,x12))/3.);
   }
}

DOUBLE L1_norm_of_p1_fcn_on_triangle(x0,x1,x2,f0,f1,f2)
DOUBLE *x0, *x1, *x2, f0, f1, f2;
{
   if (f0*f1 >= 0.)
      return(L1_norm_of_p1_fcn_on_triangle_special(x0,x1,x2,f0,f1,f2));
   else if (f0*f2 >= 0.)
      return(L1_norm_of_p1_fcn_on_triangle_special(x0,x2,x1,f0,f2,f1));
   else if (f1*f2 >= 0.)
      return(L1_norm_of_p1_fcn_on_triangle_special(x1,x2,x0,f1,f2,f0));
   else{
      eprintf("Error in L1_norm_of_p1_fcn_on_triangle.\n");
      return(0.);
   }
}

#if ELEMENT_TYPE == SIMPLEX

FLOAT vnorm_on_element(pelem,v0,v1)
ELEMENT *pelem;
FLOAT (*v0)(), (*v1)();
{
   FLOAT *x0, *x1, *x2, x01[DIM], x02[DIM], x12[DIM], xc[DIM],
         r0, r1, r2, r01, r02, r12, rc, s0, s1, s2, s01, s02, s12, sc;

   VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
   AVERAGE(x0,x1,x01);
   AVERAGE(x0,x2,x02);
   AVERAGE(x1,x2,x12);
   POINT3(x0,x1,x2,xc);
   POINT_VALUES_7(x0,x1,x2,x01,x02,x12,xc,
                  r0,r1,r2,r01,r02,r12,rc,v0)
   POINT_VALUES_7(x0,x1,x2,x01,x02,x12,xc,
                  s0,s1,s2,s01,s02,s12,sc,v1)
   return(sqrt( (3.*(r0*r0 + r1*r1 + r2*r2 + s0*s0 + s1*s1 + s2*s2)
           + 8.*(r01*r01 + r02*r02 + r12*r12 + s01*s01 + s02*s02 + s12*s12) 
           + 27.*(rc*rc + sc*sc))/60. ));
}

/*
FLOAT vnorm_on_element(pelem,v0,v1)
ELEMENT *pelem;
FLOAT (*v0)(), (*v1)();
{
   FLOAT *x0, *x1, *x2, x01[DIM], x02[DIM], x12[DIM], 
         v0_01, v0_02, v0_12, v1_01, v1_02, v1_12;

   VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
   AVERAGE(x0,x1,x01);
   AVERAGE(x0,x2,x02);
   AVERAGE(x1,x2,x12);
   v0_01 = v0(x01);
   v0_02 = v0(x02);
   v0_12 = v0(x12);
   v1_01 = v1(x01);
   v1_02 = v1(x02);
   v1_12 = v1(x12);
   return(sqrt( (v0_01*v0_01 + v0_02*v0_02 + v0_12*v0_12 +
                 v1_01*v1_01 + v1_02*v1_02 + v1_12*v1_12)/3. ) );
 
}
*/

FLOAT vnorm_at_barycentre(pelem,v0,v1)
ELEMENT *pelem;
FLOAT (*v0)(), (*v1)();
{
   FLOAT *x0, *x1, *x2, xc[DIM], rc, sc;

   VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
   POINT3(x0,x1,x2,xc);
   rc = v0(xc);
   sc = v1(xc);
   return(sqrt(rc*rc + sc*sc));
}

void coord_of_barycentre(pelem,xc)
ELEMENT *pelem;
FLOAT xc[DIM];
{
   FLOAT *x0, *x1, *x2;

   VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
   POINT3(x0,x1,x2,xc);
}

#elif ELEMENT_TYPE == CUBE

FLOAT vnorm_on_element(pelem,v0,v1)
ELEMENT *pelem;
FLOAT (*v0)(), (*v1)();
{
   FLOAT *x0, *x1, *x2, *x3, x01[DIM], x12[DIM], x23[DIM], x30[DIM], xc[DIM],
         r0, r1, r2, r3, r01, r12, r23, r30, rc,
         s0, s1, s2, s3, s01, s12, s23, s30, sc;

   VERTICES_OF_4ELEMENT(x0,x1,x2,x3,pelem);
   AVERAGE(x0,x1,x01);
   AVERAGE(x1,x2,x12);
   AVERAGE(x2,x3,x23);
   AVERAGE(x3,x0,x30);
   POINT4(x01,x12,x23,x30,xc);
   POINT_VALUES_9(x0,x1,x2,x3,x01,x12,x23,x30,xc,
                  r0,r1,r2,r3,r01,r12,r23,r30,rc,v0)
   POINT_VALUES_9(x0,x1,x2,x3,x01,x12,x23,x30,xc,
                  s0,s1,s2,s3,s01,s12,s23,s30,sc,v1)
   return(sqrt( (r0*r0 + r1*r1 + r2*r2 + r3*r3 + s0*s0 + s1*s1 + s2*s2 + s3*s3
           + 4.*(r01*r01 + r12*r12 + r23*r23 + r30*r30 +
                 s01*s01 + s12*s12 + s23*s23 + s30*s30) 
           + 16.*(rc*rc + sc*sc))/36. ));
}

FLOAT vnorm_at_barycentre(pelem,v0,v1)
ELEMENT *pelem;
FLOAT (*v0)(), (*v1)();
{
   FLOAT *x0, *x1, *x2, *x3, x01[DIM], x12[DIM], x23[DIM], x30[DIM], xc[DIM],
         rc, sc;

   VERTICES_OF_4ELEMENT(x0,x1,x2,x3,pelem);
   AVERAGE(x0,x1,x01);
   AVERAGE(x1,x2,x12);
   AVERAGE(x2,x3,x23);
   AVERAGE(x3,x0,x30);
   POINT4(x01,x12,x23,x30,xc);
   rc = v0(xc);
   sc = v1(xc);
   return(sqrt(rc*rc + sc*sc));
}

void coord_of_barycentre(pelem,xc)
ELEMENT *pelem;
FLOAT xc[DIM];
{
   FLOAT *x0, *x1, *x2, *x3, x01[DIM], x12[DIM], x23[DIM], x30[DIM];

   VERTICES_OF_4ELEMENT(x0,x1,x2,x3,pelem);
   AVERAGE(x0,x1,x01);
   AVERAGE(x1,x2,x12);
   AVERAGE(x2,x3,x23);
   AVERAGE(x3,x0,x30);
   POINT4(x01,x12,x23,x30,xc);
}

#else

FLOAT vnorm_on_element(pelem,v0,v1)
ELEMENT *pelem; FLOAT (*v0)(), (*v1)();
{  eprintf("Error: vnorm_on_element not available.\n");  }

FLOAT vnorm_at_barycentre(pelem,v0,v1)
ELEMENT *pelem; FLOAT (*v0)(), (*v1)();
{  eprintf("Error: vnorm_at_barycentre not available.\n");  }

void coord_of_barycentre(pelem,xc)
ELEMENT *pelem; FLOAT xc[DIM];
{  eprintf("Error: coord_of_barycentre not available.\n");  }

#endif

/* integrates uxi.vxj on the reference element */
DOUBLE uxi_vxj_ref(i,j,u0,u1,v0,v1,b,jac,n,x,w) 
INT i, j, n;
FLOAT (*u0)(), (*u1)(), (*v0)(), (*v1)(), b[2][2][DIM2], jac[DIM2], x[][2], *w;
{
   INT k;
   FLOAT s=0;

   for (k=0; k < n; k++)
      s += w[k]*(u0(x[k])*LINV(b[0][i],x[k]) + u1(x[k])*LINV(b[1][i],x[k]))*
                (v0(x[k])*LINV(b[0][j],x[k]) + v1(x[k])*LINV(b[1][j],x[k]))/
                fabs(LINV(jac,x[k]));
   return(s*QR_VOL);
}

/* integrates grad(u).grad(v) on the reference element */
DOUBLE gu_gv_ref(u0,u1,v0,v1,b,jac,qr) 
FLOAT (*u0)(), (*u1)(), (*v0)(), (*v1)(), b[2][2][DIM2], jac[DIM2];
QUADRATURE_RULE qr;
{
   return(uxi_vxj_ref(0,0,u0,u1,v0,v1,b,jac,qr.n,qr.points,qr.weights) + 
          uxi_vxj_ref(1,1,u0,u1,v0,v1,b,jac,qr.n,qr.points,qr.weights) );
}

/* integrates v*(b0,b1)*grad u on the reference element */
DOUBLE b_grad_u_v_ref(u0,u1,v,b0,b1,b,a,c,alpha,jac,n,x,w) 
INT n;
FLOAT (*u0)(), (*u1)(), (*v)(), (*b0)(), (*b1)(), 
      b[2][2][DIM2], a[DIM][DIM], c[DIM], alpha[DIM], jac[DIM2], x[][2], *w;
{
   INT k;
   FLOAT s=0, y[DIM];

   for (k=0; k < n; k++){
      V_BILIN_VALUE(y,a,c,alpha,x[k][0],x[k][1])
      s += w[k]*v(x[k])* 
           (b0(y)*(u0(x[k])*LINV(b[0][0],x[k]) + u1(x[k])*LINV(b[1][0],x[k])) +
            b1(y)*(u0(x[k])*LINV(b[0][1],x[k]) + u1(x[k])*LINV(b[1][1],x[k])));
   }
   return(s*QR_VOL*SGN(jac[2]));
}

/* integrates d*u*v on the reference element */
DOUBLE d_u_v_ref(u,v,d,a,c,alpha,jac,n,x,w) 
INT n;
FLOAT (*u)(), (*v)(), (*d)(), 
      a[DIM][DIM], c[DIM], alpha[DIM], jac[DIM2], x[][2], *w;
{
   INT k;
   FLOAT s=0, y[DIM];

   for (k=0; k < n; k++){
      V_BILIN_VALUE(y,a,c,alpha,x[k][0],x[k][1])
      s += w[k]*u(x[k])*v(x[k])*d(y)*fabs(LINV(jac,x[k]));
   }
   return(s*QR_VOL);
}

DOUBLE sd_ref(u,u0,u1,v0,v1,b0,b1,r,delta,b,a,c,alpha,jac,n,x,w) 
INT n;
FLOAT (*u)(), (*u0)(), (*u1)(), (*v0)(), (*v1)(), (*b0)(), (*b1)(), (*r)(),
      delta, b[2][2][DIM2], a[DIM][DIM], c[DIM], alpha[DIM], jac[DIM2], 
      x[][2], *w;
{
   INT k;
   FLOAT s=0, y[DIM];

   for (k=0; k < n; k++){
      V_BILIN_VALUE(y,a,c,alpha,x[k][0],x[k][1])
      s += w[k]*(
            r(y)*u(x[k]) +
           (b0(y)*(u0(x[k])*LINV(b[0][0],x[k]) + u1(x[k])*LINV(b[1][0],x[k])) +
            b1(y)*(u0(x[k])*LINV(b[0][1],x[k]) + u1(x[k])*LINV(b[1][1],x[k])))/ 
            LINV(jac,x[k]) )
          *(b0(y)*(v0(x[k])*LINV(b[0][0],x[k]) + v1(x[k])*LINV(b[1][0],x[k])) +
            b1(y)*(v0(x[k])*LINV(b[0][1],x[k]) + v1(x[k])*LINV(b[1][1],x[k])));
   }
   return(s*QR_VOL*SGN(jac[2])*delta);
}

DOUBLE macro_conv_stab_ref(u0,u1,v0,v1,bx,delta,b,jac,n,x,w) 
FLOAT (*u0)(), (*u1)(), (*v0)(), (*v1)(), *bx, delta, 
      b[2][2][DIM2], jac[DIM2], x[][2], *w;
INT n;
{
   FLOAT s=0.;
   INT k;

   for (k = 0; k < n; k++)
      s += w[k]*
           (bx[0]*(u0(x[k])*LINV(b[0][0],x[k]) + u1(x[k])*LINV(b[1][0],x[k])) +
            bx[1]*(u0(x[k])*LINV(b[0][1],x[k]) + u1(x[k])*LINV(b[1][1],x[k]))) 
          *(bx[0]*(v0(x[k])*LINV(b[0][0],x[k]) + v1(x[k])*LINV(b[1][0],x[k])) +
            bx[1]*(v0(x[k])*LINV(b[0][1],x[k]) + v1(x[k])*LINV(b[1][1],x[k])))/
            fabs(LINV(jac,x[k]));
   return(s*QR_VOL*delta);
}

/* integrates rhs*(b0,b1)*grad v on the reference element */
DOUBLE sd_rhs_ref(v0,v1,rhs,b0,b1,delta,b,a,c,alpha,jac,n,x,w) 
INT n;
FLOAT (*v0)(), (*v1)(), (*rhs)(), (*b0)(), (*b1)(), delta,
      b[2][2][DIM2], a[DIM][DIM], c[DIM], alpha[DIM], jac[DIM2], x[][2], *w;
{
   INT k;
   FLOAT s=0, y[DIM];

   for (k=0; k < n; k++){
      V_BILIN_VALUE(y,a,c,alpha,x[k][0],x[k][1])
      s += w[k]*rhs(y)* 
           (b0(y)*(v0(x[k])*LINV(b[0][0],x[k]) + v1(x[k])*LINV(b[1][0],x[k])) +
            b1(y)*(v0(x[k])*LINV(b[0][1],x[k]) + v1(x[k])*LINV(b[1][1],x[k])));
   }
   return(s*QR_VOL*SGN(jac[2])*delta);
}

/*  computation of the SDFEM matrix on a TRIANGLE using ref. element  */
DOUBLE sd_ref_triang(pelem,u,u0,u1,u00,u01,u11,v0,v1,bar,bb0,bb1,react,eps,rhs,
                     uu,space,sc_type,par1,par2,sd_tau,b,a,c,n,x,w)
ELEMENT *pelem;
INT n,  uu, space, sc_type;
FLOAT (*u)(), (*u0)(), (*u1)(), (*u00)(), (*u01)(), (*u11)(), (*v0)(), (*v1)(), 
      (*bb0)(), (*bb1)(), (*react)(), eps, (*rhs)(), (*sd_tau)(), par1, par2,
      bar[DIM2][DIM2], b[DIM][DIM], a[DIM][DIM], c[DIM], x[][2], *w;
{
   INT k;
   FLOAT s=0, y[DIM], u0x, u1x, v0x, v1x;

   for (k=0; k < n; k++){
      u0x = u0(x[k]);
      u1x = u1(x[k]);
      v0x = v0(x[k]);
      v1x = v1(x[k]);
      V_LIN_VALUE(y,a,c,x[k][0],x[k][1])
      s += w[k]*sd_tau(y,pelem,bar,eps,
                       bb0,bb1,react,rhs,uu,space,sc_type,par1,par2)*(
           react(y)*u(x[k]) +
           bb0(y)*(u0x*b[0][0]+u1x*b[1][0]) + bb1(y)*(u0x*b[0][1]+u1x*b[1][1]) -
           eps*(   u00(x[k])*(b[0][0]*b[0][0]+b[0][1]*b[0][1])+
                2.*u01(x[k])*(b[0][0]*b[1][0]+b[0][1]*b[1][1])+
                   u11(x[k])*(b[1][0]*b[1][0]+b[1][1]*b[1][1]) ) )
         *(bb0(y)*(v0x*b[0][0]+v1x*b[1][0]) + bb1(y)*(v0x*b[0][1]+v1x*b[1][1]));
   }
   return(s*QR_VOL*fabs(a[0][0]*a[1][1]-a[0][1]*a[1][0]));
}

/*  computation of the integral of tau*(b*grad u)*(b*grad v) on a TRIANGLE 
using ref. element  */
DOUBLE bgu_bgv_ref_triang(pelem,u0,u1,v0,v1,bar,bs0,bs1,bb0,bb1,react,eps,rhs,
                          u,space,sc_type,par1,par2,sd_tau,b,a,c,n,x,w)
ELEMENT *pelem;
INT n, u, space, sc_type;
FLOAT (*u0)(), (*u1)(), (*v0)(), (*v1)(), (*bs0)(), (*bs1)(),
      (*bb0)(), (*bb1)(), (*react)(), eps, (*rhs)(), (*sd_tau)(), par1, par2,
      bar[DIM2][DIM2], b[DIM][DIM], a[DIM][DIM], c[DIM], x[][2], *w;
{
   INT k;
   FLOAT s=0, y[DIM], u0x, u1x, v0x, v1x;

   for (k=0; k < n; k++){
      u0x = u0(x[k]);
      u1x = u1(x[k]);
      v0x = v0(x[k]);
      v1x = v1(x[k]);
      V_LIN_VALUE(y,a,c,x[k][0],x[k][1])
      s += w[k]*sd_tau(y,pelem,bar,eps,
                       bb0,bb1,react,rhs,u,space,sc_type,par1,par2)
         *(bs0(y)*(u0x*b[0][0]+u1x*b[1][0]) + bs1(y)*(u0x*b[0][1]+u1x*b[1][1]))
         *(bs0(y)*(v0x*b[0][0]+v1x*b[1][0]) + bs1(y)*(v0x*b[0][1]+v1x*b[1][1]));
   }
   return(s*QR_VOL*fabs(a[0][0]*a[1][1]-a[0][1]*a[1][0]));
}

/*  computation of the integral of sc_eps*(grad u)*(grad v) on a TRIANGLE 
using ref. element  */
DOUBLE gu_gv_ref_triang(pelem,u0,u1,v0,v1,bar,bb0,bb1,react,eps,rhs,
                        u,space,sc_type,par1,par2,sc_eps,b,a,c,n,x,w)
ELEMENT *pelem;
INT n, u, space, sc_type;
FLOAT (*u0)(), (*u1)(), (*v0)(), (*v1)(), 
      (*bb0)(), (*bb1)(), (*react)(), eps, (*rhs)(), (*sc_eps)(), par1, par2,
      bar[DIM2][DIM2], b[DIM][DIM], a[DIM][DIM], c[DIM], x[][2], *w;
{
   INT k;
   FLOAT s=0, y[DIM], u0x, u1x, v0x, v1x;

   for (k=0; k < n; k++){
      u0x = u0(x[k]);
      u1x = u1(x[k]);
      v0x = v0(x[k]);
      v1x = v1(x[k]);
      V_LIN_VALUE(y,a,c,x[k][0],x[k][1])
      s += w[k]*sc_eps(y,pelem,bar,eps,
                       bb0,bb1,react,rhs,u,space,sc_type,par1,par2)
                       *((u0x*b[0][0]+u1x*b[1][0])*(v0x*b[0][0]+v1x*b[1][0]) + 
                         (u0x*b[0][1]+u1x*b[1][1])*(v0x*b[0][1]+v1x*b[1][1]));
   }
   return(s*QR_VOL*fabs(a[0][0]*a[1][1]-a[0][1]*a[1][0]));
}

DOUBLE rhs_ref_triang(pelem,v,rhs,a,c,n,x,w)
ELEMENT *pelem;
INT n;
FLOAT (*v)(), (*rhs)(), a[DIM][DIM], c[DIM], x[][2], *w;
{
   INT k;
   FLOAT s=0, y[DIM];

   for (k=0; k < n; k++){
      V_LIN_VALUE(y,a,c,x[k][0],x[k][1])
      s += w[k]*rhs(y)*v(x[k]);
   }
   return(s*QR_VOL*fabs(a[0][0]*a[1][1]-a[0][1]*a[1][0]));
}

/*  computation of the SDFEM rhs term on a TRIANGLE using ref. element  */
DOUBLE sd_rhs_ref_triang(pelem,v0,v1,bar,bb0,bb1,react,eps,rhs,
       u,space,sc_type,par1,par2,sd_tau,b,a,c,n,x,w)
ELEMENT *pelem;
INT n, u, space, sc_type;
FLOAT (*v0)(), (*v1)(), 
      (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), eps, (*sd_tau)(), par1, par2,
      bar[DIM2][DIM2], b[DIM][DIM], a[DIM][DIM], c[DIM], x[][2], *w;
{
   INT k;
   FLOAT s=0, y[DIM], v0x, v1x;

   for (k=0; k < n; k++){
      v0x = v0(x[k]);
      v1x = v1(x[k]);
      V_LIN_VALUE(y,a,c,x[k][0],x[k][1])
      s += w[k]*sd_tau(y,pelem,bar,eps,
                       bb0,bb1,react,rhs,u,space,sc_type,par1,par2)*rhs(y)*
          (bb0(y)*(v0x*b[0][0]+v1x*b[1][0]) + bb1(y)*(v0x*b[0][1]+v1x*b[1][1]));
   }
   return(s*QR_VOL*fabs(a[0][0]*a[1][1]-a[0][1]*a[1][0]));
}

#if N_DATA & ONE_NODE_MATR

void p1c_smass_matr(tGrid,Z,tau)
GRID *tGrid;
FLOAT tau;
INT Z;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT s;
  
   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      s = VOLUME(pelem)/(12.*tau);
      putaij(n0->tstart,n1,n2,s,s,Z);
      putaij(n1->tstart,n2,n0,s,s,Z);
      putaij(n2->tstart,n0,n1,s,s,Z);
      s += s;
      COEFFN(n0,Z) += s;
      COEFFN(n1,Z) += s;
      COEFFN(n2,Z) += s;
   }
}

void laijb_2D(n0,n1,n2,Z,b0,b1,b2,detB)
NODE *n0, *n1, *n2;                    /* detB = volume * nu */
INT Z;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], detB;
{
      COEFFN(n0,Z) += DOT(b0,b0)*detB;
      putaij(n0->tstart,n1,n2,DOT(b0,b1)*detB,DOT(b0,b2)*detB,Z);
}

void aij_mini_lin_div_free(n0,n1,n2,Z,b0,b1,b2,c,detB)
NODE *n0, *n1, *n2;                    /* detB = volume * nu */
INT Z;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], c, detB;
{
      COEFFN(n0,Z) += (DOT(b0,b0)+c)*detB;
      putaij(n0->tstart,n1,n2,(DOT(b0,b1)+c)*detB,(DOT(b0,b2)+c)*detB,Z);
}

void q1_aij(n0,n1,n2,n3,Z,nu,qr)
NODE *n0, *n1, *n2, *n3;
INT Z;
FLOAT nu;
QUADRATURE_RULE qr;
{
   FLOAT b[2][2][DIM2], jac[DIM2];
  
   inverse_of_Q1_reference_mapping(n0,n1,n2,n3,b,jac);
   COEFFN(n0,Z) += 
      nu*gu_gv_ref(r_q1_0_0,r_q1_0_1,r_q1_0_0,r_q1_0_1,b,jac,qr);
   putaij3(n0->tstart,n1,n2,n3,
   nu*gu_gv_ref(r_q1_0_0,r_q1_0_1,r_q1_1_0,r_q1_1_1,b,jac,qr),
   nu*gu_gv_ref(r_q1_0_0,r_q1_0_1,r_q1_2_0,r_q1_2_1,b,jac,qr),
   nu*gu_gv_ref(r_q1_0_0,r_q1_0_1,r_q1_3_0,r_q1_3_1,b,jac,qr),Z);
}

#else

void p1c_smass_matr(tGrid,Z,tau)
GRID *tGrid; FLOAT tau; INT Z;
{  eprintf("Error: p1c_smass_matr not available.\n");  }

void laijb_2D(n0,n1,n2,Z,b0,b1,b2,detB)
NODE *n0, *n1, *n2; INT Z; FLOAT b0[DIM2], b1[DIM2], b2[DIM2], detB;
{  eprintf("Error: laijb_2D not available.\n");  }

void aij_mini_lin_div_free(n0,n1,n2,Z,b0,b1,b2,c,detB)
NODE *n0, *n1, *n2; INT Z; FLOAT b0[DIM2], b1[DIM2], b2[DIM2], c, detB;
{  eprintf("Error: aij_mini_lin_div_free not available.\n");  }

void q1_aij(n0,n1,n2,n3,Z,nu,qr)
NODE *n0, *n1, *n2, *n3; INT Z; FLOAT nu; QUADRATURE_RULE qr;
{  eprintf("Error: q1_aij not available.\n");  }

#endif

#if (N_DATA & DxD_NODE_MATR) && (DATA_S & N_LINK_TO_NODES)

void aij_sn_Korn(n0,n1,n2,Z,b0,b1,b2,detB) /* detB=volume * nu */
NODE *n0, *n1, *n2; 
INT Z;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], detB;
{
   LINK *pli;
   FLOAT ann, an1, an2, a00, a01, a10, a11;
  
   ann = DOT(b0,b0);
   an1 = DOT(b0,b1);
   an2 = DOT(b0,b2);
  
   a00 =       (b0[0]*b0[0] + ann)*detB;
   a10 = a01 =  b0[0]*b0[1]       *detB;
   a11 =       (b0[1]*b0[1] + ann)*detB;
   SET_COEFFNN(n0,Z,a00,a01,a10,a11);

   a00 = (b1[0]*b0[0] + an1)*detB;
   a01 =  b1[0]*b0[1]       *detB;
   a10 =  b1[1]*b0[0]       *detB;
   a11 = (b1[1]*b0[1] + an1)*detB;
   for (pli=n0->tstart; pli->nbnode != n1; pli=pli->next);
   SET_COEFFNN(pli,Z,a00,a01,a10,a11);

   a00 = (b2[0]*b0[0] + an2)*detB;
   a01 =  b2[0]*b0[1]       *detB;
   a10 =  b2[1]*b0[0]       *detB;
   a11 = (b2[1]*b0[1] + an2)*detB;
   for (pli=n0->tstart; pli->nbnode != n2; pli=pli->next);
   SET_COEFFNN(pli,Z,a00,a01,a10,a11);
}

#else  /*  if !((N_DATA & DxD_NODE_MATR) && (DATA_S & N_LINK_TO_NODES))  */

void aij_sn_Korn(n0,n1,n2,Z,b0,b1,b2,detB)
NODE *n0, *n1, *n2; INT Z; FLOAT b0[DIM2], b1[DIM2], b2[DIM2], detB;
{  eprintf("Error: aij_sn_Korn not available.\n");  }

#endif

#if (ELEMENT_TYPE == SIMPLEX) && (E_DATA & ExN_MATR) && (E_DATA & NxE_MATR) && (E_DATA & ExF_MATR) && (E_DATA & FxE_MATR) && (E_DATA & ExE_MATR)

void fill_symmetric_e_matrices(pel,Z,a0,a1,a2,b0,b1,b2,c)
ELEMENT *pel;
DOUBLE a0, a1, a2, b0, b1, b2, c;
INT Z;
{
   COEFF_NE(pel,Z,0) += a0;
   COEFF_NE(pel,Z,1) += a1;
   COEFF_NE(pel,Z,2) += a2;
   COEFF_EN(pel,Z,0) += a0;
   COEFF_EN(pel,Z,1) += a1;
   COEFF_EN(pel,Z,2) += a2;
   COEFF_FE(pel,Z,0) += b0;
   COEFF_FE(pel,Z,1) += b1;
   COEFF_FE(pel,Z,2) += b2;
   COEFF_EF(pel,Z,0) += b0;
   COEFF_EF(pel,Z,1) += b1;
   COEFF_EF(pel,Z,2) += b2;
   COEFF_EE(pel,Z) += c;
}

void fill_four_e_matrices(pel,i,Z,a,b,c,d)
ELEMENT *pel;
DOUBLE a, b, c, d;
INT i, Z;
{
   COEFF_NE(pel,Z,i) += a;
   COEFF_EN(pel,Z,i) += b;
   COEFF_FE(pel,Z,i) += c;
   COEFF_EF(pel,Z,i) += d;
}

void fill_diag_e_matrix(pel,Z,a)
ELEMENT *pel;
DOUBLE a;
INT Z;
{
   COEFF_EE(pel,Z) += a;
}

#else

void fill_symmetric_e_matrices(pel,Z,a0,a1,a2,b0,b1,b2,c)
ELEMENT *pel; DOUBLE a0, a1, a2, b0, b1, b2, c; INT Z;
{  eprintf("Error: fill_symmetric_e_matrices not available.\n");  }

void fill_four_e_matrices(pel,i,Z,a,b,c,d)
ELEMENT *pel; DOUBLE a, b, c, d; INT i, Z;
{  eprintf("Error: fill_four_e_matrices not available.\n");  }

void fill_diag_e_matrix(pel,Z,a)
ELEMENT *pel; DOUBLE a; INT Z;
{  eprintf("Error: fill_diag_e_matrix not available.\n");  }

#endif

#if F_DATA & ONE_FACE_MATR

void sfill_sf(fa0,fa1,fa2,Z,ff0,ff1,ff2)
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT ff0, ff1, ff2; 
{
   COEFF_FF(fa0,Z) += ff0;
   putppij(fa0->tfstart,fa1,fa2,ff1,ff2,Z);
}

#else

void sfill_sf(fa0,fa1,fa2,Z,ff0,ff1,ff2)
FACE *fa0, *fa1, *fa2; INT Z; FLOAT ff0, ff1, ff2; 
{  eprintf("Error: sfill_sf not available.\n");  }

#endif

#if F_DATA & DxD_FACE_MATR

void vfill_sf(fa0,fa1,fa2,Z,ff0,ff1,ff2)
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT ff0, ff1, ff2; 
{
   SET_COEFFNN(fa0,Z,ff0,0.,0.,ff0);
   vputppij(fa0->tfstart,fa1,fa2,ff1,ff2,Z);
}

#else

void vfill_sf(fa0,fa1,fa2,Z,ff0,ff1,ff2)
FACE *fa0, *fa1, *fa2; INT Z; FLOAT ff0, ff1, ff2; 
{  eprintf("Error: vfill_sf not available.\n");  }

#endif

#if (N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES) 

void sfill_sn_sf(pelem,n0,n1,n2,fa0,fa1,fa2,i,Z,bubble,
                 nn0,nn1,nn2,nf0,nf1,nf2,fn0,fn1,fn2,ff0,ff1,ff2,ne,en,fe,ef)
ELEMENT *pelem;
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i, Z, bubble;
FLOAT nn0, nn1, nn2, nf0, nf1, nf2, fn0, fn1, fn2, ff0, ff1, ff2, ne, en, fe, ef; 
{
   NFLINK *pnf;
   FNLINK *pfn;
  
   COEFFN(n0,Z) += nn0;
   putaij(n0->tstart,n1,n2,nn1,nn2,Z);
   for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
   COEFFL(pnf,Z) += nf0;
   putasij(n0->tnfstart,fa1,fa2,nf1,nf2,Z);

   COEFF_FF(fa0,Z) += ff0;
   putppij(fa0->tfstart,fa1,fa2,ff1,ff2,Z);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) += fn0;
   for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
   COEFFL(pfn,Z) += fn1;
   for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
   COEFFL(pfn,Z) += fn2;
   if (bubble)
      fill_four_e_matrices(pelem,i,Z,ne,en,fe,ef);
}

#else  /*  if !((N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && 
                (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && 
                (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && 
                (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES))  */

void sfill_sn_sf(pelem,n0,n1,n2,fa0,fa1,fa2,i,Z,bubble,nn0,nn1,nn2,nf0,nf1,nf2,fn0,fn1,fn2,ff0,ff1,ff2,ne,en,fe,ef)
ELEMENT *pelem; NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i, Z, bubble; FLOAT nn0, nn1, nn2, nf0, nf1, nf2, fn0, fn1, fn2, ff0, ff1, ff2, ne, en, fe, ef; 
{  eprintf("Error: sfill_sn_sf not available.\n");  }

#endif

#if (N_DATA & DxD_NODE_MATR) && (N_DATA & DxD_NODE_FACE_MATR) && (F_DATA & DxD_FACE_MATR) && (F_DATA & DxD_FACE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES)

void vfill_sn_sf(pelem,n0,n1,n2,fa0,fa1,fa2,i,Z,bubble,
                 nn0,nn1,nn2,nf0,nf1,nf2,fn0,fn1,fn2,ff0,ff1,ff2,ne,en,fe,ef)
ELEMENT *pelem;
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i, Z, bubble;
FLOAT nn0, nn1, nn2, nf0, nf1, nf2, fn0, fn1, fn2, ff0, ff1, ff2, ne, en, fe, ef; 
{
   NFLINK *pnf;
   FNLINK *pfn;
  
   SET_COEFFNN(n0,Z,nn0,0.,0.,nn0);
   vputaij(n0->tstart,n1,n2,nn1,nn2,Z);
   for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
   SET_COEFFNN(pnf,Z,nf0,0.,0.,nf0);
   vputasij(n0->tnfstart,fa1,fa2,nf1,nf2,Z);

   SET_COEFFNN(fa0,Z,ff0,0.,0.,ff0);
   vputppij(fa0->tfstart,fa1,fa2,ff1,ff2,Z);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   SET_COEFFNN(pfn,Z,fn0,0.,0.,fn0);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
   SET_COEFFNN(pfn,Z,fn1,0.,0.,fn1);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
   SET_COEFFNN(pfn,Z,fn2,0.,0.,fn2);
   if (bubble)
      eprintf("Error: bubble part not implemented in vfill_sn_sf.\n");
/*      fill_four_e_matrices(pelem,i,Z,ne,en,fe,ef);  */
}

#else  /*  if !((N_DATA & DxD_NODE_MATR) && (N_DATA & DxD_NODE_FACE_MATR) && 
                (F_DATA & DxD_FACE_MATR) && (F_DATA & DxD_FACE_NODE_MATR) && 
                (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && 
                (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES))  */

void vfill_sn_sf(pelem,n0,n1,n2,fa0,fa1,fa2,i,Z,bubble,nn0,nn1,nn2,nf0,nf1,nf2,fn0,fn1,fn2,ff0,ff1,ff2,ne,en,fe,ef)
ELEMENT *pelem; NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i, Z, bubble; FLOAT nn0, nn1, nn2, nf0, nf1, nf2, fn0, fn1, fn2, ff0, ff1, ff2, ne, en, fe, ef; 
{  eprintf("Error: vfill_sn_sf not available.\n");  }

#endif

void aij_sn_sf(n0,n1,n2,fa0,fa1,fa2,Z,b0,b1,b2,detB,mstruct)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT Z, mstruct;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], detB;        /* detB = volume * nu  */
{
   ELEMENT *pelem;
   NFLINK *pnf;
   FNLINK *pfn;
   INT i=0, bubble=0;
   FLOAT ann, an1, an2, a11, a22, ne=0. ,en=0. ,fe=0. ,ef=0.;
  
   ann = DOT(b0,b0)*detB;
   an1 = DOT(b0,b1)*detB;
   an2 = DOT(b0,b2)*detB;
   a11 = DOT(b1,b1)*detB;
   a22 = DOT(b2,b2)*detB;
   if (mstruct & Q_FULL)
      vfill_sn_sf(pelem,n0,n1,n2,fa0,fa1,fa2,i,Z,bubble,
                  ann,an1,an2,-ann/3.,-an1/3.,-an2/3.,
                  -ann/3.,-an1/3.,-an2/3.,(ann + a11 + a22)/12.,an1/6.,an2/6.,
                  ne,en,fe,ef);
   else
      sfill_sn_sf(pelem,n0,n1,n2,fa0,fa1,fa2,i,Z,bubble,
                  ann,an1,an2,-ann/3.,-an1/3.,-an2/3.,
                  -ann/3.,-an1/3.,-an2/3.,(ann + a11 + a22)/12.,an1/6.,an2/6.,
                  ne,en,fe,ef);
}

#if (N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES)

void cij_sn_sf(n0,n1,n2,fa0,fa1,fa2,Z,rdetB)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT rdetB;
{
   NFLINK *pnf;
   FNLINK *pfn;
   FLOAT q12, q30, q60, q180;
  
   q12  = rdetB/12.;
   q60  = rdetB/60.;
   q30  = q60 + q60;
   q180 = rdetB/180.;

      COEFFN(n0,Z) += rdetB/6.;
      putaij(n0->tstart,n1,n2,q12,q12,Z);
      putasij(n0->tnfstart,fa1,fa2,q30,q30,Z);
      for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
      COEFFL(pnf,Z) += rdetB/60.;
                                          
      COEFF_FF(fa0,Z) += rdetB/90.;
      putppij(fa0->tfstart,fa1,fa2,q180,q180,Z); 
      for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
      COEFFL(pfn,Z) += q60;
      for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
      COEFFL(pfn,Z) += q30;
      for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
      COEFFL(pfn,Z) += q30;
}

void aij_sn_sf_iso(n0,n1,n2,fa0,fa1,fa2,Z,nu,qr)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT nu;
QUADRATURE_RULE qr;
{
   NFLINK *pnf;
   FNLINK *pfn;
   FLOAT b[2][2][DIM2], jac[DIM2], a0, a1, a2;
  
   inverse_of_P2_reference_mapping(n0,n1,n2,fa0,fa1,fa2,b,jac);
  
   COEFFN(n0,Z) += 
              nu*gu_gv_ref(r_l0_0,r_l0_1,r_l0_0,r_l0_1,b,jac,qr);
   putaij(n0->tstart,n1,n2,
           nu*gu_gv_ref(r_l0_0,r_l0_1,r_l1_0,r_l1_1,b,jac,qr),
           nu*gu_gv_ref(r_l0_0,r_l0_1,r_l2_0,r_l2_1,b,jac,qr),Z);
   a0 = nu*gu_gv_ref(r_l0_0,r_l0_1,r_l1l2_0,r_l1l2_1,b,jac,qr);
   a1 = nu*gu_gv_ref(r_l0_0,r_l0_1,r_l0l2_0,r_l0l2_1,b,jac,qr);
   a2 = nu*gu_gv_ref(r_l0_0,r_l0_1,r_l0l1_0,r_l0l1_1,b,jac,qr);
   for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
   COEFFL(pnf,Z) += a0;
   putasij(n0->tnfstart,fa1,fa2,a1,a2,Z);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) += a0;
   for (pfn=fa1->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) += a1;
   for (pfn=fa2->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) += a2;
                                          
   COEFF_FF(fa0,Z) += 
      nu*gu_gv_ref(r_l1l2_0,r_l1l2_1,r_l1l2_0,r_l1l2_1,b,jac,qr);
   putppij(fa0->tfstart,fa1,fa2,
   nu*gu_gv_ref(r_l1l2_0,r_l1l2_1,r_l0l2_0,r_l0l2_1,b,jac,qr),
   nu*gu_gv_ref(r_l1l2_0,r_l1l2_1,r_l0l1_0,r_l0l1_1,b,jac,qr),Z);
}

#else  /*  if !((N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && 
                (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && 
                (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && 
                (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES))  */

void cij_sn_sf(n0,n1,n2,fa0,fa1,fa2,Z,rdetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z; FLOAT rdetB;
{  eprintf("Error: cij_sn_sf not available.\n");  }

void aij_sn_sf_iso(n0,n1,n2,fa0,fa1,fa2,Z,nu,qr)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z; FLOAT nu; QUADRATURE_RULE qr;
{  eprintf("Error: aij_sn_sf_iso not available.\n");  }

#endif

#if (N_DATA & DxD_NODE_MATR) && (N_DATA & DxD_NODE_FACE_MATR) && (F_DATA & DxD_FACE_MATR) && (F_DATA & DxD_FACE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES)

void aij_sn_sf_Korn(n0,n1,n2,fa0,fa1,fa2,Z,b0,b1,b2,detB) /* detB=volume * nu */
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], detB;
{
   LINK *pli;
   NFLINK *pnf;
   FNLINK *pfn;
   FLINK *pfl;
   FLOAT ann, an1, an2, a00, a01, a10, a11, s, r;
  
   ann = DOT(b0,b0);
   an1 = DOT(b0,b1);
   an2 = DOT(b0,b2);
  
   a00 =       (b0[0]*b0[0] + ann)*detB;
   a10 = a01 =  b0[0]*b0[1]       *detB;
   a11 =       (b0[1]*b0[1] + ann)*detB;
   SET_COEFFNN(n0,Z,a00,a01,a10,a11);
   for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
   SET_COEFFNN(pnf,Z,-a00/3.,-a01/3.,-a10/3.,-a11/3.);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   SET_COEFFNN(pfn,Z,-a00/3.,-a01/3.,-a10/3.,-a11/3.);

   a00 = (b1[0]*b0[0] + an1)*detB;
   a01 =  b1[0]*b0[1]       *detB;
   a10 =  b1[1]*b0[0]       *detB;
   a11 = (b1[1]*b0[1] + an1)*detB;
   for (pli=n0->tstart; pli->nbnode != n1; pli=pli->next);
   SET_COEFFNN(pli,Z,a00,a01,a10,a11);
   for (pnf=n0->tnfstart; pnf->nbface!=fa1; pnf=pnf->next);
   SET_COEFFNN(pnf,Z,-a00/3.,-a01/3.,-a10/3.,-a11/3.);
   for (pfn=fa1->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   SET_COEFFNN(pfn,Z,-a00/3.,-a01/3.,-a10/3.,-a11/3.);

   a00 = (b2[0]*b0[0] + an2)*detB;
   a01 =  b2[0]*b0[1]       *detB;
   a10 =  b2[1]*b0[0]       *detB;
   a11 = (b2[1]*b0[1] + an2)*detB;
   for (pli=n0->tstart; pli->nbnode != n2; pli=pli->next);
   SET_COEFFNN(pli,Z,a00,a01,a10,a11);
   for (pnf=n0->tnfstart; pnf->nbface!=fa2; pnf=pnf->next);
   SET_COEFFNN(pnf,Z,-a00/3.,-a01/3.,-a10/3.,-a11/3.);
   for (pfn=fa2->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   SET_COEFFNN(pfn,Z,-a00/3.,-a01/3.,-a10/3.,-a11/3.);
                                       
      detB /= 12.;
      s = ann + DOT(b1,b1) + DOT(b2,b2);
      r = (b0[0]*b0[1] + b1[0]*b1[1] + b2[0]*b2[1])*detB;
      SET_COEFFNN(fa0,Z,(b0[0]*b0[0] + b1[0]*b1[0] + b2[0]*b2[0] + s)*detB,r,r,
                        (b0[1]*b0[1] + b1[1]*b1[1] + b2[1]*b2[1] + s)*detB);
      for (pfl=fa0->tfstart; pfl->nbface!=fa1; pfl=pfl->next);
      r = (b0[0]*b1[1] + b0[1]*b1[0])*detB;
      SET_COEFFNN(pfl,Z,2.*(b0[0]*b1[0] + an1)*detB,r,r,
                        2.*(b0[1]*b1[1] + an1)*detB);
      for (pfl=fa0->tfstart; pfl->nbface!=fa2; pfl=pfl->next);
      r = (b0[0]*b2[1] + b0[1]*b2[0])*detB;
      SET_COEFFNN(pfl,Z,2.*(b0[0]*b2[0] + an2)*detB,r,r,
                        2.*(b0[1]*b2[1] + an2)*detB);
}

void newton_convij_sn_sf(n0,n1,n2,fa0,fa1,fa2,i0,i1,i2,vf,s,p,q,Z,b0,b1,b2,rdetB)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i0, i1, i2, Z;
FLOAT vf[DIM2][DIM], s[DIM], p[DIM][DIM], q[DIM][DIM], 
      b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{
   LINK *pli;
   NFLINK *pnf;
   FNLINK *pfn;
   FLINK *pfl;
   FLOAT r, z[2][2];
  
   z[0][0] = vf[i1][0]*b2[0]+vf[i2][0]*b1[0];
   z[0][1] = vf[i1][0]*b2[1]+vf[i2][0]*b1[1];
   z[1][0] = vf[i1][1]*b2[0]+vf[i2][1]*b1[0];
   z[1][1] = vf[i1][1]*b2[1]+vf[i2][1]*b1[1];
   r = rdetB/30.;
   SET_COEFFNN(n0,Z,(5.*p[0][0]-q[0][0]+2.*z[0][0])*r,
                    (5.*p[0][1]-q[0][1]+2.*z[0][1])*r,
                    (5.*p[1][0]-q[1][0]+2.*z[1][0])*r,
                    (5.*p[1][1]-q[1][1]+2.*z[1][1])*r);
   r = rdetB/60.;
   for (pli=n0->tstart; pli->nbnode != n1; pli=pli->next);
   SET_COEFFNN(pli,Z,(5.*p[0][0]-q[0][0]+(s[0]-2.*vf[i2][0])*b2[0])*r,
                     (5.*p[0][1]-q[0][1]+(s[0]-2.*vf[i2][0])*b2[1])*r,
                     (5.*p[1][0]-q[1][0]+(s[1]-2.*vf[i2][1])*b2[0])*r,
                     (5.*p[1][1]-q[1][1]+(s[1]-2.*vf[i2][1])*b2[1])*r);
   for (pli=n0->tstart; pli->nbnode != n2; pli=pli->next);
   SET_COEFFNN(pli,Z,(5.*p[0][0]-q[0][0]+(s[0]-2.*vf[i1][0])*b1[0])*r,
                     (5.*p[0][1]-q[0][1]+(s[0]-2.*vf[i1][0])*b1[1])*r,
                     (5.*p[1][0]-q[1][0]+(s[1]-2.*vf[i1][1])*b1[0])*r,
                     (5.*p[1][1]-q[1][1]+(s[1]-2.*vf[i1][1])*b1[1])*r);
   r = rdetB/180.;
   for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
   SET_COEFFNN(pnf,Z,(3.*p[0][0]-q[0][0])*r,
                     (3.*p[0][1]-q[0][1])*r,
                     (3.*p[1][0]-q[1][0])*r,
                     (3.*p[1][1]-q[1][1])*r);
   for (pnf=n0->tnfstart; pnf->nbface!=fa1; pnf=pnf->next);
   SET_COEFFNN(pnf,Z,
           (6.*p[0][0]-q[0][0]+2.*z[0][0]+vf[i1][0]*b0[0]+vf[i0][0]*b1[0])*r,
           (6.*p[0][1]-q[0][1]+2.*z[0][1]+vf[i1][0]*b0[1]+vf[i0][0]*b1[1])*r,
           (6.*p[1][0]-q[1][0]+2.*z[1][0]+vf[i1][1]*b0[0]+vf[i0][1]*b1[0])*r,
           (6.*p[1][1]-q[1][1]+2.*z[1][1]+vf[i1][1]*b0[1]+vf[i0][1]*b1[1])*r);
   for (pnf=n0->tnfstart; pnf->nbface!=fa2; pnf=pnf->next);
   SET_COEFFNN(pnf,Z,
           (6.*p[0][0]-q[0][0]+2.*z[0][0]+vf[i2][0]*b0[0]+vf[i0][0]*b2[0])*r,
           (6.*p[0][1]-q[0][1]+2.*z[0][1]+vf[i2][0]*b0[1]+vf[i0][0]*b2[1])*r,
           (6.*p[1][0]-q[1][0]+2.*z[1][0]+vf[i2][1]*b0[0]+vf[i0][1]*b2[0])*r,
           (6.*p[1][1]-q[1][1]+2.*z[1][1]+vf[i2][1]*b0[1]+vf[i0][1]*b2[1])*r);

   for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   SET_COEFFNN(pfn,Z,(3.*p[0][0]-q[0][0])*r,
                     (3.*p[0][1]-q[0][1])*r,
                     (3.*p[1][0]-q[1][0])*r,
                     (3.*p[1][1]-q[1][1])*r);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
   SET_COEFFNN(pfn,Z,(6.*p[0][0]-q[0][0]+2.*(vf[i2][0]*b0[0]+vf[i0][0]*b2[0])+
                                             vf[i1][0]*b0[0]+vf[i0][0]*b1[0])*r,
                     (6.*p[0][1]-q[0][1]+2.*(vf[i2][0]*b0[1]+vf[i0][0]*b2[1])+
                                             vf[i1][0]*b0[1]+vf[i0][0]*b1[1])*r,
                     (6.*p[1][0]-q[1][0]+2.*(vf[i2][1]*b0[0]+vf[i0][1]*b2[0])+
                                             vf[i1][1]*b0[0]+vf[i0][1]*b1[0])*r,
                     (6.*p[1][1]-q[1][1]+2.*(vf[i2][1]*b0[1]+vf[i0][1]*b2[1])+
                                             vf[i1][1]*b0[1]+vf[i0][1]*b1[1])*r);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
   SET_COEFFNN(pfn,Z,(6.*p[0][0]-q[0][0]+2.*(vf[i1][0]*b0[0]+vf[i0][0]*b1[0])+
                                             vf[i2][0]*b0[0]+vf[i0][0]*b2[0])*r,
                     (6.*p[0][1]-q[0][1]+2.*(vf[i1][0]*b0[1]+vf[i0][0]*b1[1])+
                                             vf[i2][0]*b0[1]+vf[i0][0]*b2[1])*r,
                     (6.*p[1][0]-q[1][0]+2.*(vf[i1][1]*b0[0]+vf[i0][1]*b1[0])+
                                             vf[i2][1]*b0[0]+vf[i0][1]*b2[0])*r,
                     (6.*p[1][1]-q[1][1]+2.*(vf[i1][1]*b0[1]+vf[i0][1]*b1[1])+
                                             vf[i2][1]*b0[1]+vf[i0][1]*b2[1])*r);
   r = rdetB/1260.;
   SET_COEFFNN(fa0,Z,
           (14.*p[0][0]-2.*q[0][0]+4.*(vf[i2][0]+vf[i1][0]-vf[i0][0])*b0[0])*r,
           (14.*p[0][1]-2.*q[0][1]+4.*(vf[i2][0]+vf[i1][0]-vf[i0][0])*b0[1])*r,
           (14.*p[1][0]-2.*q[1][0]+4.*(vf[i2][1]+vf[i1][1]-vf[i0][1])*b0[0])*r,
           (14.*p[1][1]-2.*q[1][1]+4.*(vf[i2][1]+vf[i1][1]-vf[i0][1])*b0[1])*r);
   for (pfl=fa0->tfstart; pfl->nbface!=fa1; pfl=pfl->next);
   SET_COEFFNN(pfl,Z,(7.*p[0][0]-2.*q[0][0]+vf[i1][0]*b0[0]+vf[i0][0]*b1[0])*r,
                     (7.*p[0][1]-2.*q[0][1]+vf[i1][0]*b0[1]+vf[i0][0]*b1[1])*r,
                     (7.*p[1][0]-2.*q[1][0]+vf[i1][1]*b0[0]+vf[i0][1]*b1[0])*r,
                     (7.*p[1][1]-2.*q[1][1]+vf[i1][1]*b0[1]+vf[i0][1]*b1[1])*r);
   for (pfl=fa0->tfstart; pfl->nbface!=fa2; pfl=pfl->next);
   SET_COEFFNN(pfl,Z,(7.*p[0][0]-2.*q[0][0]+vf[i2][0]*b0[0]+vf[i0][0]*b2[0])*r,
                     (7.*p[0][1]-2.*q[0][1]+vf[i2][0]*b0[1]+vf[i0][0]*b2[1])*r,
                     (7.*p[1][0]-2.*q[1][0]+vf[i2][1]*b0[0]+vf[i0][1]*b2[0])*r,
                     (7.*p[1][1]-2.*q[1][1]+vf[i2][1]*b0[1]+vf[i0][1]*b2[1])*r);
}

#else  /*  if !((N_DATA & DxD_NODE_MATR) && (N_DATA & DxD_NODE_FACE_MATR) && 
                (F_DATA & DxD_FACE_MATR) && (F_DATA & DxD_FACE_NODE_MATR) && 
                (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && 
                (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES))  */

void aij_sn_sf_Korn(n0,n1,n2,fa0,fa1,fa2,Z,b0,b1,b2,detB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z; FLOAT b0[DIM2], b1[DIM2], b2[DIM2], detB;
{  eprintf("Error: aij_sn_sf_Korn not available.\n");  }

void newton_convij_sn_sf(n0,n1,n2,fa0,fa1,fa2,i0,i1,i2,vf,s,p,q,Z,b0,b1,b2,rdetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i0, i1, i2, Z; FLOAT vf[DIM2][DIM], s[DIM], p[DIM][DIM], q[DIM][DIM], b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{  eprintf("Error: newton_convij_sn_sf not available.\n");  }

#endif

FLOAT upwind_lambda();

#if (N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES) 

/*  stiffness matrix corresponding to \int_\om u_i(v\cdot\nabla u_j) \dx  */
void convij_sn_sf_iso(pelem,n0,n1,n2,fa0,fa1,fa2,i0,i1,i2,vn,vf,vb,Z,bubble)
ELEMENT *pelem;
NODE *n0, *n1, *n2;
FACE *fa0, *fa1, *fa2;
INT i0, i1, i2, Z, bubble;
FLOAT vn[DIM2][DIM], vf[DIM2][DIM], vb[DIM];
{
   NFLINK *pnf;
   FNLINK *pfn;
   FLOAT *v0, *v1, *v2, *v01, *v02, *v12, b[2][2][DIM2], jac[DIM2],
         a, bb, c, d, e, c0=0., c1=0., c2=0., d0=0., d1=0., d2=0., a1, a2, s=1.;

   v0 = vn[i0];
   v1 = vn[i1];
   v2 = vn[i2];
   v01 = vf[i2];
   v02 = vf[i1];
   v12 = vf[i0];
   inverse_of_P2_reference_mapping(n0,n1,n2,fa0,fa1,fa2,b,jac);
   if (jac[2] < 0.) s = -1;

   if (bubble){
      c0 = -(vb[0]*(2*b[0][0][0]+2*b[0][0][1]+7*b[0][0][2]+2*b[1][0][0]+2*b[1][0][1]+7*b[1][0][2])+vb[1]*(2*b[0][1][0]+2*b[0][1][1]+7*b[0][1][2]+2*b[1][1][0]+2*b[1][1][1]+7*b[1][1][2]))/7.;
      c1 = (vb[0]*(2*b[0][0][1]+2*b[0][0][0]+7*b[0][0][2])+vb[1]*(2*b[0][1][1]+2*b[0][1][0]+7*b[0][1][2]))/7.;
      c2 = (vb[0]*(2*b[1][0][1]+2*b[1][0][0]+7*b[1][0][2])+vb[1]*(2*b[1][1][1]+2*b[1][1][0]+7*b[1][1][2]))/7.;
      d0 = (vb[0]*(2*b[0][0][0]+3*b[0][0][1]+8*b[0][0][2]+3*b[1][0][0]+2*b[1][0][1]+8*b[1][0][2])+vb[1]*(2*b[0][1][0]+3*b[0][1][1]+8*b[0][1][2]+3*b[1][1][0]+2*b[1][1][1]+8*b[1][1][2]))*0.25;
      d1 = (vb[0]*(-2*b[0][0][0]-3*b[0][0][1]-8*b[0][0][2]+b[1][0][0]+4*b[1][0][2])+vb[1]*(-2*b[0][1][0]-3*b[0][1][1]-8*b[0][1][2]+b[1][1][0]+4*b[1][1][2]))*0.25;
      d2 = (vb[0]*(b[0][0][1]+4*b[0][0][2]-3*b[1][0][0]-2*b[1][0][1]-8*b[1][0][2])+vb[1]*(b[0][1][1]+4*b[0][1][2]-3*b[1][1][0]-2*b[1][1][1]-8*b[1][1][2]))*0.25;
   }

   COEFFN(n0,Z) += s*(-2*v02[0]*b[0][0][1]-2*v02[0]*b[1][0][1]-2*v02[1]*b[0][1][1]-6*v02[0]*b[0][0][2]-v02[0]*b[0][0][0]-6*v02[0]*b[1][0][2]-v02[0]*b[1][0][0]-6*v02[1]*b[0][1][2]-v02[1]*b[0][1][0]-v01[0]*b[0][0][1]-6*v0[0]*b[1][0][1]-v01[0]*b[1][0][1]-6*v0[1]*b[0][1][1]-v01[1]*b[0][1][1]-30*v0[0]*b[0][0][2]-6*v0[0]*b[0][0][0]-2*v01[0]*b[0][0][0]-30*v0[0]*b[1][0][2]-6*v0[0]*b[1][0][0]-6*v01[0]*b[1][0][2]-2*v01[0]*b[1][0][0]-30*v0[1]*b[0][1][2]-6*v0[1]*b[0][1][0]-6*v01[1]*b[0][1][2]-2*v01[1]*b[0][1][0]-6*v0[0]*b[0][0][1]-6*v01[0]*b[0][0][2]-6*b[0][0][1]*v2[0]-b[0][0][1]*v12[0]-6*b[1][0][1]*v2[0]-b[1][0][1]*v12[0]-6*b[0][1][1]*v2[1]-b[0][1][1]*v12[1]-3*b[0][0][1]*v1[0]-15*b[0][0][2]*v2[0]-3*b[0][0][0]*v2[0]-3*b[0][0][2]*v12[0]-b[0][0][0]*v12[0]-3*b[1][0][1]*v1[0]-15*b[1][0][2]*v2[0]-3*b[1][0][0]*v2[0]-3*b[1][0][2]*v12[0]-b[1][0][0]*v12[0]-3*b[0][1][1]*v1[1]-15*b[0][1][2]*v2[1]-3*b[0][1][0]*v2[1]-3*b[0][1][2]*v12[1]-b[0][1][0]*v12[1]-15*b[0][0][2]*v1[0]-6*b[0][0][0]*v1[0]-15*b[1][0][2]*v1[0]-6*b[1][0][0]*v1[0]-15*b[0][1][2]*v1[1]-6*b[0][1][0]*v1[1]-2*v02[1]*b[1][1][1]-6*v02[1]*b[1][1][2]-v02[1]*b[1][1][0]-v01[1]*b[1][1][1]-30*v0[1]*b[1][1][2]-6*v0[1]*b[1][1][0]-6*v01[1]*b[1][1][2]-2*v01[1]*b[1][1][0]-6*v0[1]*b[1][1][1]-6*b[1][1][1]*v2[1]-b[1][1][1]*v12[1]-3*b[1][1][1]*v1[1]-15*b[1][1][2]*v2[1]-3*b[1][1][0]*v2[1]-3*b[1][1][2]*v12[1]-b[1][1][0]*v12[1]-15*b[1][1][2]*v1[1]-6*b[1][1][0]*v1[1]+c0)/360.;
   a1 = s*(2*v02[0]*b[0][0][1]+2*v02[1]*b[0][1][1]+6*v02[0]*b[0][0][2]+v02[0]*b[0][0][0]+6*v02[1]*b[0][1][2]+v02[1]*b[0][1][0]+v01[0]*b[0][0][1]+6*v0[1]*b[0][1][1]+v01[1]*b[0][1][1]+30*v0[0]*b[0][0][2]+6*v0[0]*b[0][0][0]+2*v01[0]*b[0][0][0]+30*v0[1]*b[0][1][2]+6*v0[1]*b[0][1][0]+6*v01[1]*b[0][1][2]+2*v01[1]*b[0][1][0]+6*v0[0]*b[0][0][1]+6*v01[0]*b[0][0][2]+6*b[0][0][1]*v2[0]+b[0][0][1]*v12[0]+6*b[0][1][1]*v2[1]+b[0][1][1]*v12[1]+3*b[0][0][1]*v1[0]+15*b[0][0][2]*v2[0]+3*b[0][0][0]*v2[0]+3*b[0][0][2]*v12[0]+b[0][0][0]*v12[0]+3*b[0][1][1]*v1[1]+15*b[0][1][2]*v2[1]+3*b[0][1][0]*v2[1]+3*b[0][1][2]*v12[1]+b[0][1][0]*v12[1]+15*b[0][0][2]*v1[0]+6*b[0][0][0]*v1[0]+15*b[0][1][2]*v1[1]+6*b[0][1][0]*v1[1]+c1)/360.;
   a2 = s*(2*v02[0]*b[1][0][1]+6*v02[0]*b[1][0][2]+v02[0]*b[1][0][0]+6*v0[0]*b[1][0][1]+v01[0]*b[1][0][1]+30*v0[0]*b[1][0][2]+6*v0[0]*b[1][0][0]+6*v01[0]*b[1][0][2]+2*v01[0]*b[1][0][0]+6*b[1][0][1]*v2[0]+b[1][0][1]*v12[0]+3*b[1][0][1]*v1[0]+15*b[1][0][2]*v2[0]+3*b[1][0][0]*v2[0]+3*b[1][0][2]*v12[0]+b[1][0][0]*v12[0]+15*b[1][0][2]*v1[0]+6*b[1][0][0]*v1[0]+2*v02[1]*b[1][1][1]+6*v02[1]*b[1][1][2]+v02[1]*b[1][1][0]+v01[1]*b[1][1][1]+30*v0[1]*b[1][1][2]+6*v0[1]*b[1][1][0]+6*v01[1]*b[1][1][2]+2*v01[1]*b[1][1][0]+6*v0[1]*b[1][1][1]+6*b[1][1][1]*v2[1]+b[1][1][1]*v12[1]+3*b[1][1][1]*v1[1]+15*b[1][1][2]*v2[1]+3*b[1][1][0]*v2[1]+3*b[1][1][2]*v12[1]+b[1][1][0]*v12[1]+15*b[1][1][2]*v1[1]+6*b[1][1][0]*v1[1]+c2)/360.;
   putaij(n0->tstart,n1,n2,a1,a2,Z);
   a1 = s*(-6*v02[0]*b[0][0][1]-6*v02[1]*b[0][1][1]-14*v02[0]*b[0][0][2]-2*v02[0]*b[0][0][0]+7*v02[0]*b[1][0][2]+v02[0]*b[1][0][0]-14*v02[1]*b[0][1][2]-2*v02[1]*b[0][1][0]-2*v01[0]*b[0][0][1]+7*v0[0]*b[1][0][1]+v01[0]*b[1][0][1]-14*v0[1]*b[0][1][1]-2*v01[1]*b[0][1][1]-42*v0[0]*b[0][0][2]-7*v0[0]*b[0][0][0]-2*v01[0]*b[0][0][0]+84*v0[0]*b[1][0][2]+14*v0[0]*b[1][0][0]+14*v01[0]*b[1][0][2]+4*v01[0]*b[1][0][0]-42*v0[1]*b[0][1][2]-7*v0[1]*b[0][1][0]-7*v01[1]*b[0][1][2]-2*v01[1]*b[0][1][0]-14*v0[0]*b[0][0][1]-7*v01[0]*b[0][0][2]-21*b[0][0][1]*v2[0]-3*b[0][0][1]*v12[0]-7*b[1][0][1]*v2[0]-b[1][0][1]*v12[0]-21*b[0][1][1]*v2[1]-3*b[0][1][1]*v12[1]-7*b[0][0][1]*v1[0]-42*b[0][0][2]*v2[0]-7*b[0][0][0]*v2[0]-7*b[0][0][2]*v12[0]-2*b[0][0][0]*v12[0]-7*b[0][1][1]*v1[1]-42*b[0][1][2]*v2[1]-7*b[0][1][0]*v2[1]-7*b[0][1][2]*v12[1]-2*b[0][1][0]*v12[1]-21*b[0][0][2]*v1[0]-7*b[0][0][0]*v1[0]+21*b[1][0][2]*v1[0]+7*b[1][0][0]*v1[0]-21*b[0][1][2]*v1[1]-7*b[0][1][0]*v1[1]+7*v02[1]*b[1][1][2]+v02[1]*b[1][1][0]+v01[1]*b[1][1][1]+84*v0[1]*b[1][1][2]+14*v0[1]*b[1][1][0]+14*v01[1]*b[1][1][2]+4*v01[1]*b[1][1][0]+7*v0[1]*b[1][1][1]-7*b[1][1][1]*v2[1]-b[1][1][1]*v12[1]+21*b[1][1][2]*v1[1]+7*b[1][1][0]*v1[1]+d1)/2520.;
   a2 = s*(4*v02[0]*b[0][0][1]-2*v02[0]*b[1][0][1]+4*v02[1]*b[0][1][1]+14*v02[0]*b[0][0][2]+v02[0]*b[0][0][0]-7*v02[0]*b[1][0][2]-2*v02[0]*b[1][0][0]+14*v02[1]*b[0][1][2]+v02[1]*b[0][1][0]+v01[0]*b[0][0][1]-7*v0[0]*b[1][0][1]-2*v01[0]*b[1][0][1]+14*v0[1]*b[0][1][1]+v01[1]*b[0][1][1]+84*v0[0]*b[0][0][2]+7*v0[0]*b[0][0][0]-42*v0[0]*b[1][0][2]-14*v0[0]*b[1][0][0]-14*v01[0]*b[1][0][2]-6*v01[0]*b[1][0][0]+84*v0[1]*b[0][1][2]+7*v0[1]*b[0][1][0]+7*v01[1]*b[0][1][2]+14*v0[0]*b[0][0][1]+7*v01[0]*b[0][0][2]+7*b[0][0][1]*v2[0]-7*b[1][0][1]*v2[0]-2*b[1][0][1]*v12[0]+7*b[0][1][1]*v2[1]+21*b[0][0][2]*v2[0]-b[0][0][0]*v12[0]-7*b[1][0][1]*v1[0]-21*b[1][0][2]*v2[0]-7*b[1][0][0]*v2[0]-7*b[1][0][2]*v12[0]-3*b[1][0][0]*v12[0]+21*b[0][1][2]*v2[1]-b[0][1][0]*v12[1]-7*b[0][0][0]*v1[0]-42*b[1][0][2]*v1[0]-21*b[1][0][0]*v1[0]-7*b[0][1][0]*v1[1]-2*v02[1]*b[1][1][1]-7*v02[1]*b[1][1][2]-2*v02[1]*b[1][1][0]-2*v01[1]*b[1][1][1]-42*v0[1]*b[1][1][2]-14*v0[1]*b[1][1][0]-14*v01[1]*b[1][1][2]-6*v01[1]*b[1][1][0]-7*v0[1]*b[1][1][1]-7*b[1][1][1]*v2[1]-2*b[1][1][1]*v12[1]-7*b[1][1][1]*v1[1]-21*b[1][1][2]*v2[1]-7*b[1][1][0]*v2[1]-7*b[1][1][2]*v12[1]-3*b[1][1][0]*v12[1]-42*b[1][1][2]*v1[1]-21*b[1][1][0]*v1[1]+d2)/2520.;
   putasij(n0->tnfstart,fa1,fa2,a1,a2,Z);
   for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
   COEFFL(pnf,Z) += s*(21*b[0][1][2]*v1[1]+7*b[0][0][1]*v1[0]+21*b[0][0][1]*v2[0]+42*b[0][1][2]*v2[1]+7*b[0][0][0]*v1[0]+7*b[0][1][0]*v1[1]+42*b[0][0][2]*v2[0]+21*b[0][1][1]*v2[1]+2*b[0][0][0]*v12[0]+2*b[0][1][0]*v12[1]+6*v02[0]*b[0][0][1]+2*v02[0]*b[1][0][1]+6*v02[1]*b[0][1][1]+2*v02[1]*b[1][1][1]+21*b[0][0][2]*v1[0]+14*v02[0]*b[0][0][2]+2*v02[0]*b[0][0][0]+2*v02[0]*b[1][0][0]+14*v02[1]*b[0][1][2]+2*v02[1]*b[0][1][0]+7*v02[1]*b[1][1][2]+2*v02[1]*b[1][1][0]+14*v0[0]*b[0][0][1]+2*v01[0]*b[0][0][1]+7*v0[0]*b[1][0][1]+2*v01[0]*b[1][0][1]+14*v0[1]*b[0][1][1]+2*v01[1]*b[0][1][1]+7*v0[1]*b[1][1][1]+2*v01[1]*b[1][1][1]+42*v0[0]*b[0][0][2]+7*v0[0]*b[0][0][0]+7*v01[0]*b[0][0][2]+2*v01[0]*b[0][0][0]+42*v0[0]*b[1][0][2]+14*v0[0]*b[1][0][0]+14*v01[0]*b[1][0][2]+7*v02[0]*b[1][0][2]+6*v01[0]*b[1][0][0]+42*v0[1]*b[0][1][2]+7*v0[1]*b[0][1][0]+7*v01[1]*b[0][1][2]+2*v01[1]*b[0][1][0]+42*v0[1]*b[1][1][2]+14*v0[1]*b[1][1][0]+14*v01[1]*b[1][1][2]+6*v01[1]*b[1][1][0]+2*b[1][0][1]*v12[0]+3*b[1][0][0]*v12[0]+2*b[1][1][1]*v12[1]+3*b[1][1][0]*v12[1]+7*b[1][0][1]*v2[0]+7*b[1][0][1]*v1[0]+21*b[1][0][2]*v2[0]+7*b[1][0][0]*v2[0]+7*b[1][0][2]*v12[0]+42*b[1][0][2]*v1[0]+21*b[1][0][0]*v1[0]+7*b[1][1][1]*v2[1]+7*b[1][1][1]*v1[1]+21*b[1][1][2]*v2[1]+7*b[1][1][0]*v2[1]+7*b[1][1][2]*v12[1]+42*b[1][1][2]*v1[1]+21*b[1][1][0]*v1[1]+3*b[0][0][1]*v12[0]+3*b[0][1][1]*v12[1]+7*b[0][1][2]*v12[1]+7*b[0][1][0]*v2[1]+7*b[0][1][1]*v1[1]+7*b[0][0][2]*v12[0]+7*b[0][0][0]*v2[0]+d0)/2520.;

   if (bubble){
      c0 = (vb[0]*(3*b[0][0][0]+4*b[0][0][1]+9*b[0][0][2]+4*b[1][0][0]+3*b[1][0][1]+9*b[1][0][2])+vb[1]*(3*b[0][1][0]+4*b[0][1][1]+9*b[0][1][2]+4*b[1][1][0]+3*b[1][1][1]+9*b[1][1][2]))/3.;
      c1 = -(vb[0]*(3*b[0][0][0]+4*b[0][0][1]+9*b[0][0][2]+b[1][0][0]+2*b[1][0][1]+3*b[1][0][2])+vb[1]*(3*b[0][1][0]+4*b[0][1][1]+9*b[0][1][2]+b[1][1][0]+2*b[1][1][1]+3*b[1][1][2]))/3.;
      c2 = -(vb[0]*(2*b[0][0][0]+b[0][0][1]+3*b[0][0][2]+4*b[1][0][0]+3*b[1][0][1]+9*b[1][0][2])+vb[1]*(2*b[0][1][0]+b[0][1][1]+3*b[0][1][2]+4*b[1][1][0]+3*b[1][1][1]+9*b[1][1][2]))/3.;
      d0 = -(vb[0]*(3*b[0][0][0]+3*b[0][0][1]+8*b[0][0][2]+3*b[1][0][0]+3*b[1][0][1]+8*b[1][0][2])+vb[1]*(3*b[0][1][0]+3*b[0][1][1]+8*b[0][1][2]+3*b[1][1][0]+3*b[1][1][1]+8*b[1][1][2]))*0.25;
      d1 = (vb[0]*(3*b[0][0][1]+3*b[0][0][0]+8*b[0][0][2])+vb[1]*(3*b[0][1][1]+3*b[0][1][0]+8*b[0][1][2]))*0.25;
      d2 = (vb[0]*(3*b[1][0][1]+3*b[1][0][0]+8*b[1][0][2])+vb[1]*(3*b[1][1][1]+3*b[1][1][0]+8*b[1][1][2]))*0.25;
   }
   COEFF_FF(fa0,Z) += s*(56*b[0][1][2]*v1[1]+24*b[0][0][1]*v1[0]+48*b[0][0][1]*v2[0]+84*b[0][1][2]*v2[1]+24*b[0][0][0]*v1[0]+24*b[0][1][0]*v1[1]+84*b[0][0][2]*v2[0]+48*b[0][1][1]*v2[1]+9*b[0][0][0]*v12[0]+9*b[0][1][0]*v12[1]+6*v02[0]*b[0][0][1]+3*v02[0]*b[1][0][1]+6*v02[1]*b[0][1][1]+3*v02[1]*b[1][1][1]+56*b[0][0][2]*v1[0]+12*v02[0]*b[0][0][2]+3*v02[0]*b[0][0][0]+3*v02[0]*b[1][0][0]+12*v02[1]*b[0][1][2]+3*v02[1]*b[0][1][0]+8*v02[1]*b[1][1][2]+3*v02[1]*b[1][1][0]+12*v0[0]*b[0][0][1]+3*v01[0]*b[0][0][1]+8*v0[0]*b[1][0][1]+3*v01[0]*b[1][0][1]+12*v0[1]*b[0][1][1]+3*v01[1]*b[0][1][1]+8*v0[1]*b[1][1][1]+3*v01[1]*b[1][1][1]+28*v0[0]*b[0][0][2]+8*v0[0]*b[0][0][0]+8*v01[0]*b[0][0][2]+3*v01[0]*b[0][0][0]+28*v0[0]*b[1][0][2]+12*v0[0]*b[1][0][0]+12*v01[0]*b[1][0][2]+8*v02[0]*b[1][0][2]+6*v01[0]*b[1][0][0]+28*v0[1]*b[0][1][2]+8*v0[1]*b[0][1][0]+8*v01[1]*b[0][1][2]+3*v01[1]*b[0][1][0]+28*v0[1]*b[1][1][2]+12*v0[1]*b[1][1][0]+12*v01[1]*b[1][1][2]+6*v01[1]*b[1][1][0]+9*b[1][0][1]*v12[0]+12*b[1][0][0]*v12[0]+9*b[1][1][1]*v12[1]+12*b[1][1][0]*v12[1]+24*b[1][0][1]*v2[0]+24*b[1][0][1]*v1[0]+56*b[1][0][2]*v2[0]+24*b[1][0][0]*v2[0]+24*b[1][0][2]*v12[0]+84*b[1][0][2]*v1[0]+48*b[1][0][0]*v1[0]+24*b[1][1][1]*v2[1]+24*b[1][1][1]*v1[1]+56*b[1][1][2]*v2[1]+24*b[1][1][0]*v2[1]+24*b[1][1][2]*v12[1]+84*b[1][1][2]*v1[1]+48*b[1][1][0]*v1[1]+12*b[0][0][1]*v12[0]+12*b[0][1][1]*v12[1]+24*b[0][1][2]*v12[1]+24*b[0][1][0]*v2[1]+24*b[0][1][1]*v1[1]+24*b[0][0][2]*v12[0]+24*b[0][0][0]*v2[0]+c0)/10080.;
   a1 = s*(-56*b[0][1][2]*v1[1]-24*b[0][0][1]*v1[0]-48*b[0][0][1]*v2[0]-84*b[0][1][2]*v2[1]-24*b[0][0][0]*v1[0]-24*b[0][1][0]*v1[1]-84*b[0][0][2]*v2[0]-48*b[0][1][1]*v2[1]-9*b[0][0][0]*v12[0]-9*b[0][1][0]*v12[1]-6*v02[0]*b[0][0][1]-3*v02[0]*b[1][0][1]-6*v02[1]*b[0][1][1]-3*v02[1]*b[1][1][1]-56*b[0][0][2]*v1[0]-12*v02[0]*b[0][0][2]-3*v02[0]*b[0][0][0]-v02[0]*b[1][0][0]-12*v02[1]*b[0][1][2]-3*v02[1]*b[0][1][0]-4*v02[1]*b[1][1][2]-v02[1]*b[1][1][0]-12*v0[0]*b[0][0][1]-3*v01[0]*b[0][0][1]-4*v0[0]*b[1][0][1]-v01[0]*b[1][0][1]-12*v0[1]*b[0][1][1]-3*v01[1]*b[0][1][1]-4*v0[1]*b[1][1][1]-v01[1]*b[1][1][1]-28*v0[0]*b[0][0][2]-8*v0[0]*b[0][0][0]-8*v01[0]*b[0][0][2]-3*v01[0]*b[0][0][0]-4*v02[0]*b[1][0][2]-28*v0[1]*b[0][1][2]-8*v0[1]*b[0][1][0]-8*v01[1]*b[0][1][2]-3*v01[1]*b[0][1][0]-9*b[1][0][1]*v12[0]-6*b[1][0][0]*v12[0]-9*b[1][1][1]*v12[1]-6*b[1][1][0]*v12[1]-36*b[1][0][1]*v2[0]-16*b[1][0][1]*v1[0]-56*b[1][0][2]*v2[0]-16*b[1][0][0]*v2[0]-16*b[1][0][2]*v12[0]-28*b[1][0][2]*v1[0]-12*b[1][0][0]*v1[0]-36*b[1][1][1]*v2[1]-16*b[1][1][1]*v1[1]-56*b[1][1][2]*v2[1]-16*b[1][1][0]*v2[1]-16*b[1][1][2]*v12[1]-28*b[1][1][2]*v1[1]-12*b[1][1][0]*v1[1]-12*b[0][0][1]*v12[0]-12*b[0][1][1]*v12[1]-24*b[0][1][2]*v12[1]-24*b[0][1][0]*v2[1]-24*b[0][1][1]*v1[1]-24*b[0][0][2]*v12[0]-24*b[0][0][0]*v2[0]+c1)/10080.;
   a2 = s*(-56*b[0][1][2]*v1[1]-16*b[0][0][1]*v1[0]-12*b[0][0][1]*v2[0]-28*b[0][1][2]*v2[1]-36*b[0][0][0]*v1[0]-36*b[0][1][0]*v1[1]-28*b[0][0][2]*v2[0]-12*b[0][1][1]*v2[1]-9*b[0][0][0]*v12[0]-9*b[0][1][0]*v12[1]-3*v02[0]*b[1][0][1]-3*v02[1]*b[1][1][1]-56*b[0][0][2]*v1[0]-v02[0]*b[0][0][0]-3*v02[0]*b[1][0][0]-v02[1]*b[0][1][0]-8*v02[1]*b[1][1][2]-3*v02[1]*b[1][1][0]-v01[0]*b[0][0][1]-8*v0[0]*b[1][0][1]-3*v01[0]*b[1][0][1]-v01[1]*b[0][1][1]-8*v0[1]*b[1][1][1]-3*v01[1]*b[1][1][1]-4*v0[0]*b[0][0][0]-4*v01[0]*b[0][0][2]-3*v01[0]*b[0][0][0]-28*v0[0]*b[1][0][2]-12*v0[0]*b[1][0][0]-12*v01[0]*b[1][0][2]-8*v02[0]*b[1][0][2]-6*v01[0]*b[1][0][0]-4*v0[1]*b[0][1][0]-4*v01[1]*b[0][1][2]-3*v01[1]*b[0][1][0]-28*v0[1]*b[1][1][2]-12*v0[1]*b[1][1][0]-12*v01[1]*b[1][1][2]-6*v01[1]*b[1][1][0]-9*b[1][0][1]*v12[0]-12*b[1][0][0]*v12[0]-9*b[1][1][1]*v12[1]-12*b[1][1][0]*v12[1]-24*b[1][0][1]*v2[0]-24*b[1][0][1]*v1[0]-56*b[1][0][2]*v2[0]-24*b[1][0][0]*v2[0]-24*b[1][0][2]*v12[0]-84*b[1][0][2]*v1[0]-48*b[1][0][0]*v1[0]-24*b[1][1][1]*v2[1]-24*b[1][1][1]*v1[1]-56*b[1][1][2]*v2[1]-24*b[1][1][0]*v2[1]-24*b[1][1][2]*v12[1]-84*b[1][1][2]*v1[1]-48*b[1][1][0]*v1[1]-6*b[0][0][1]*v12[0]-6*b[0][1][1]*v12[1]-16*b[0][1][2]*v12[1]-16*b[0][1][0]*v2[1]-16*b[0][1][1]*v1[1]-16*b[0][0][2]*v12[0]-16*b[0][0][0]*v2[0]+c2)/10080.;
   putppij(fa0->tfstart,fa1,fa2,a1,a2,Z);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) += s*(-42*b[0][1][2]*v1[1]-14*b[0][0][1]*v1[0]-21*b[0][0][1]*v2[0]-42*b[0][1][2]*v2[1]-21*b[0][0][0]*v1[0]-21*b[0][1][0]*v1[1]-42*b[0][0][2]*v2[0]-21*b[0][1][1]*v2[1]-6*b[0][0][0]*v12[0]-6*b[0][1][0]*v12[1]-3*v02[0]*b[0][0][1]-3*v02[0]*b[1][0][1]-3*v02[1]*b[0][1][1]-3*v02[1]*b[1][1][1]-42*b[0][0][2]*v1[0]-7*v02[0]*b[0][0][2]-2*v02[0]*b[0][0][0]-2*v02[0]*b[1][0][0]-7*v02[1]*b[0][1][2]-2*v02[1]*b[0][1][0]-7*v02[1]*b[1][1][2]-2*v02[1]*b[1][1][0]-7*v0[0]*b[0][0][1]-2*v01[0]*b[0][0][1]-7*v0[0]*b[1][0][1]-2*v01[0]*b[1][0][1]-7*v0[1]*b[0][1][1]-2*v01[1]*b[0][1][1]-7*v0[1]*b[1][1][1]-2*v01[1]*b[1][1][1]-21*v0[0]*b[0][0][2]-7*v0[0]*b[0][0][0]-7*v01[0]*b[0][0][2]-3*v01[0]*b[0][0][0]-21*v0[0]*b[1][0][2]-7*v0[0]*b[1][0][0]-7*v01[0]*b[1][0][2]-7*v02[0]*b[1][0][2]-3*v01[0]*b[1][0][0]-21*v0[1]*b[0][1][2]-7*v0[1]*b[0][1][0]-7*v01[1]*b[0][1][2]-3*v01[1]*b[0][1][0]-21*v0[1]*b[1][1][2]-7*v0[1]*b[1][1][0]-7*v01[1]*b[1][1][2]-3*v01[1]*b[1][1][0]-6*b[1][0][1]*v12[0]-6*b[1][0][0]*v12[0]-6*b[1][1][1]*v12[1]-6*b[1][1][0]*v12[1]-21*b[1][0][1]*v2[0]-14*b[1][0][1]*v1[0]-42*b[1][0][2]*v2[0]-14*b[1][0][0]*v2[0]-14*b[1][0][2]*v12[0]-42*b[1][0][2]*v1[0]-21*b[1][0][0]*v1[0]-21*b[1][1][1]*v2[1]-14*b[1][1][1]*v1[1]-42*b[1][1][2]*v2[1]-14*b[1][1][0]*v2[1]-14*b[1][1][2]*v12[1]-42*b[1][1][2]*v1[1]-21*b[1][1][0]*v1[1]-6*b[0][0][1]*v12[0]-6*b[0][1][1]*v12[1]-14*b[0][1][2]*v12[1]-14*b[0][1][0]*v2[1]-14*b[0][1][1]*v1[1]-14*b[0][0][2]*v12[0]-14*b[0][0][0]*v2[0]+d0)/2520.;
   for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
   COEFFL(pfn,Z) += s*(42*b[0][1][2]*v1[1]+14*b[0][0][1]*v1[0]+21*b[0][0][1]*v2[0]+42*b[0][1][2]*v2[1]+21*b[0][0][0]*v1[0]+21*b[0][1][0]*v1[1]+42*b[0][0][2]*v2[0]+21*b[0][1][1]*v2[1]+6*b[0][0][0]*v12[0]+6*b[0][1][0]*v12[1]+3*v02[0]*b[0][0][1]+3*v02[1]*b[0][1][1]+42*b[0][0][2]*v1[0]+7*v02[0]*b[0][0][2]+2*v02[0]*b[0][0][0]+7*v02[1]*b[0][1][2]+2*v02[1]*b[0][1][0]+7*v0[0]*b[0][0][1]+2*v01[0]*b[0][0][1]+7*v0[1]*b[0][1][1]+2*v01[1]*b[0][1][1]+21*v0[0]*b[0][0][2]+7*v0[0]*b[0][0][0]+7*v01[0]*b[0][0][2]+3*v01[0]*b[0][0][0]+21*v0[1]*b[0][1][2]+7*v0[1]*b[0][1][0]+7*v01[1]*b[0][1][2]+3*v01[1]*b[0][1][0]+6*b[0][0][1]*v12[0]+6*b[0][1][1]*v12[1]+14*b[0][1][2]*v12[1]+14*b[0][1][0]*v2[1]+14*b[0][1][1]*v1[1]+14*b[0][0][2]*v12[0]+14*b[0][0][0]*v2[0]+d1)/2520.;
   for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
   COEFFL(pfn,Z) += s*(3*v02[0]*b[1][0][1]+3*v02[1]*b[1][1][1]+2*v02[0]*b[1][0][0]+7*v02[1]*b[1][1][2]+2*v02[1]*b[1][1][0]+7*v0[0]*b[1][0][1]+2*v01[0]*b[1][0][1]+7*v0[1]*b[1][1][1]+2*v01[1]*b[1][1][1]+21*v0[0]*b[1][0][2]+7*v0[0]*b[1][0][0]+7*v01[0]*b[1][0][2]+7*v02[0]*b[1][0][2]+3*v01[0]*b[1][0][0]+21*v0[1]*b[1][1][2]+7*v0[1]*b[1][1][0]+7*v01[1]*b[1][1][2]+3*v01[1]*b[1][1][0]+6*b[1][0][1]*v12[0]+6*b[1][0][0]*v12[0]+6*b[1][1][1]*v12[1]+6*b[1][1][0]*v12[1]+21*b[1][0][1]*v2[0]+14*b[1][0][1]*v1[0]+42*b[1][0][2]*v2[0]+14*b[1][0][0]*v2[0]+14*b[1][0][2]*v12[0]+42*b[1][0][2]*v1[0]+21*b[1][0][0]*v1[0]+21*b[1][1][1]*v2[1]+14*b[1][1][1]*v1[1]+42*b[1][1][2]*v2[1]+14*b[1][1][0]*v2[1]+14*b[1][1][2]*v12[1]+42*b[1][1][2]*v1[1]+21*b[1][1][0]*v1[1]+d2)/2520.;
   if (bubble){
      a = s*(12*b[0][0][1]*v2[0]+28*b[0][1][2]*v2[1]-4*b[0][0][0]*v1[0]-4*b[0][1][0]*v1[1]+28*b[0][0][2]*v2[0]+12*b[0][1][1]*v2[1]-b[0][0][0]*v12[0]-b[0][1][0]*v12[1]+6*v02[0]*b[0][0][1]+6*v02[1]*b[0][1][1]+16*v02[0]*b[0][0][2]+v02[0]*b[0][0][0]+v02[0]*b[1][0][0]+16*v02[1]*b[0][1][2]+v02[1]*b[0][1][0]+4*v02[1]*b[1][1][2]+v02[1]*b[1][1][0]+16*v0[0]*b[0][0][1]+v01[0]*b[0][0][1]+4*v0[0]*b[1][0][1]+v01[0]*b[1][0][1]+16*v0[1]*b[0][1][1]+v01[1]*b[0][1][1]+4*v0[1]*b[1][1][1]+v01[1]*b[1][1][1]+56*v0[0]*b[0][0][2]+4*v0[0]*b[0][0][0]+4*v01[0]*b[0][0][2]+56*v0[0]*b[1][0][2]+16*v0[0]*b[1][0][0]+16*v01[0]*b[1][0][2]+4*v02[0]*b[1][0][2]+6*v01[0]*b[1][0][0]+56*v0[1]*b[0][1][2]+4*v0[1]*b[0][1][0]+4*v01[1]*b[0][1][2]+56*v0[1]*b[1][1][2]+16*v0[1]*b[1][1][0]+16*v01[1]*b[1][1][2]+6*v01[1]*b[1][1][0]-b[1][0][1]*v12[0]-b[1][1][1]*v12[1]-4*b[1][0][1]*v2[0]+28*b[1][0][2]*v1[0]+12*b[1][0][0]*v1[0]-4*b[1][1][1]*v2[1]+28*b[1][1][2]*v1[1]+12*b[1][1][0]*v1[1] + (vb[0]*(b[0][0][1]+3*b[0][0][2]+b[1][0][0]+3*b[1][0][2])+vb[1]*(b[0][1][1]+3*b[0][1][2]+b[1][1][0]+3*b[1][1][2]))/3.)/10080.;
      bb = s*(-28*b[0][1][2]*v1[1]-8*b[0][0][1]*v1[0]-12*b[0][0][1]*v2[0]-28*b[0][1][2]*v2[1]-12*b[0][0][0]*v1[0]-12*b[0][1][0]*v1[1]-28*b[0][0][2]*v2[0]-12*b[0][1][1]*v2[1]-3*b[0][0][0]*v12[0]-3*b[0][1][0]*v12[1]-3*v02[0]*b[0][0][1]-3*v02[0]*b[1][0][1]-3*v02[1]*b[0][1][1]-3*v02[1]*b[1][1][1]-28*b[0][0][2]*v1[0]-8*v02[0]*b[0][0][2]-2*v02[0]*b[0][0][0]-2*v02[0]*b[1][0][0]-8*v02[1]*b[0][1][2]-2*v02[1]*b[0][1][0]-8*v02[1]*b[1][1][2]-2*v02[1]*b[1][1][0]-8*v0[0]*b[0][0][1]-2*v01[0]*b[0][0][1]-8*v0[0]*b[1][0][1]-2*v01[0]*b[1][0][1]-8*v0[1]*b[0][1][1]-2*v01[1]*b[0][1][1]-8*v0[1]*b[1][1][1]-2*v01[1]*b[1][1][1]-28*v0[0]*b[0][0][2]-8*v0[0]*b[0][0][0]-8*v01[0]*b[0][0][2]-3*v01[0]*b[0][0][0]-28*v0[0]*b[1][0][2]-8*v0[0]*b[1][0][0]-8*v01[0]*b[1][0][2]-8*v02[0]*b[1][0][2]-3*v01[0]*b[1][0][0]-28*v0[1]*b[0][1][2]-8*v0[1]*b[0][1][0]-8*v01[1]*b[0][1][2]-3*v01[1]*b[0][1][0]-28*v0[1]*b[1][1][2]-8*v0[1]*b[1][1][0]-8*v01[1]*b[1][1][2]-3*v01[1]*b[1][1][0]-3*b[1][0][1]*v12[0]-3*b[1][0][0]*v12[0]-3*b[1][1][1]*v12[1]-3*b[1][1][0]*v12[1]-12*b[1][0][1]*v2[0]-8*b[1][0][1]*v1[0]-28*b[1][0][2]*v2[0]-8*b[1][0][0]*v2[0]-8*b[1][0][2]*v12[0]-28*b[1][0][2]*v1[0]-12*b[1][0][0]*v1[0]-12*b[1][1][1]*v2[1]-8*b[1][1][1]*v1[1]-28*b[1][1][2]*v2[1]-8*b[1][1][0]*v2[1]-8*b[1][1][2]*v12[1]-28*b[1][1][2]*v1[1]-12*b[1][1][0]*v1[1]-3*b[0][0][1]*v12[0]-3*b[0][1][1]*v12[1]-8*b[0][1][2]*v12[1]-8*b[0][1][0]*v2[1]-8*b[0][1][1]*v1[1]-8*b[0][0][2]*v12[0]-8*b[0][0][0]*v2[0] - (vb[0]*(10*b[0][0][0]+10*b[0][0][1]+30*b[0][0][2]+10*b[1][0][0]+10*b[1][0][1]+30*b[1][0][2])+vb[1]*(10*b[0][1][0]+10*b[0][1][1]+30*b[0][1][2]+10*b[1][1][0]+10*b[1][1][1]+30*b[1][1][2]))/15.)/10080.;
      c = s*(-48*b[0][1][2]*v1[1]-18*b[0][0][1]*v1[0]-18*b[0][0][1]*v2[0]-36*b[0][1][2]*v2[1]-27*b[0][0][0]*v1[0]-27*b[0][1][0]*v1[1]-36*b[0][0][2]*v2[0]-18*b[0][1][1]*v2[1]-9*b[0][0][0]*v12[0]-9*b[0][1][0]*v12[1]-2*v02[0]*b[1][0][1]-2*v02[1]*b[1][1][1]-48*b[0][0][2]*v1[0]-v02[0]*b[0][0][0]-v02[0]*b[1][0][0]-v02[1]*b[0][1][0]-3*v02[1]*b[1][1][2]-v02[1]*b[1][1][0]-v01[0]*b[0][0][1]-3*v0[0]*b[1][0][1]-v01[0]*b[1][0][1]-v01[1]*b[0][1][1]-3*v0[1]*b[1][1][1]-v01[1]*b[1][1][1]-3*v0[0]*b[0][0][0]-3*v01[0]*b[0][0][2]-2*v01[0]*b[0][0][0]-3*v02[0]*b[1][0][2]-3*v0[1]*b[0][1][0]-3*v01[1]*b[0][1][2]-2*v01[1]*b[0][1][0]-9*b[1][0][1]*v12[0]-8*b[1][0][0]*v12[0]-9*b[1][1][1]*v12[1]-8*b[1][1][0]*v12[1]-27*b[1][0][1]*v2[0]-18*b[1][0][1]*v1[0]-48*b[1][0][2]*v2[0]-18*b[1][0][0]*v2[0]-18*b[1][0][2]*v12[0]-36*b[1][0][2]*v1[0]-18*b[1][0][0]*v1[0]-27*b[1][1][1]*v2[1]-18*b[1][1][1]*v1[1]-48*b[1][1][2]*v2[1]-18*b[1][1][0]*v2[1]-18*b[1][1][2]*v12[1]-36*b[1][1][2]*v1[1]-18*b[1][1][0]*v1[1]-8*b[0][0][1]*v12[0]-8*b[0][1][1]*v12[1]-18*b[0][1][2]*v12[1]-18*b[0][1][0]*v2[1]-18*b[0][1][1]*v1[1]-18*b[0][0][2]*v12[0]-18*b[0][0][0]*v2[0] - (vb[0]*(3*b[0][0][0]+2*b[0][0][1]+5*b[0][0][2]+2*b[1][0][0]+3*b[1][0][1]+5*b[1][0][2])+vb[1]*(3*b[0][1][0]+2*b[0][1][1]+5*b[0][1][2]+2*b[1][1][0]+3*b[1][1][1]+5*b[1][1][2]))*0.2)/30240.;
      d = s*(24*b[0][1][2]*v1[1]+9*b[0][0][1]*v1[0]+18*b[0][0][1]*v2[0]+36*b[0][1][2]*v2[1]+9*b[0][0][0]*v1[0]+9*b[0][1][0]*v1[1]+36*b[0][0][2]*v2[0]+18*b[0][1][1]*v2[1]+3*b[0][0][0]*v12[0]+3*b[0][1][0]*v12[1]+4*v02[0]*b[0][0][1]+2*v02[0]*b[1][0][1]+4*v02[1]*b[0][1][1]+2*v02[1]*b[1][1][1]+24*b[0][0][2]*v1[0]+9*v02[0]*b[0][0][2]+2*v02[0]*b[0][0][0]+2*v02[0]*b[1][0][0]+9*v02[1]*b[0][1][2]+2*v02[1]*b[0][1][0]+6*v02[1]*b[1][1][2]+2*v02[1]*b[1][1][0]+9*v0[0]*b[0][0][1]+2*v01[0]*b[0][0][1]+6*v0[0]*b[1][0][1]+2*v01[0]*b[1][0][1]+9*v0[1]*b[0][1][1]+2*v01[1]*b[0][1][1]+6*v0[1]*b[1][1][1]+2*v01[1]*b[1][1][1]+24*v0[0]*b[0][0][2]+6*v0[0]*b[0][0][0]+6*v01[0]*b[0][0][2]+2*v01[0]*b[0][0][0]+24*v0[0]*b[1][0][2]+9*v0[0]*b[1][0][0]+9*v01[0]*b[1][0][2]+6*v02[0]*b[1][0][2]+4*v01[0]*b[1][0][0]+24*v0[1]*b[0][1][2]+6*v0[1]*b[0][1][0]+6*v01[1]*b[0][1][2]+2*v01[1]*b[0][1][0]+24*v0[1]*b[1][1][2]+9*v0[1]*b[1][1][0]+9*v01[1]*b[1][1][2]+4*v01[1]*b[1][1][0]+3*b[1][0][1]*v12[0]+4*b[1][0][0]*v12[0]+3*b[1][1][1]*v12[1]+4*b[1][1][0]*v12[1]+9*b[1][0][1]*v2[0]+9*b[1][0][1]*v1[0]+24*b[1][0][2]*v2[0]+9*b[1][0][0]*v2[0]+9*b[1][0][2]*v12[0]+36*b[1][0][2]*v1[0]+18*b[1][0][0]*v1[0]+9*b[1][1][1]*v2[1]+9*b[1][1][1]*v1[1]+24*b[1][1][2]*v2[1]+9*b[1][1][0]*v2[1]+9*b[1][1][2]*v12[1]+36*b[1][1][2]*v1[1]+18*b[1][1][0]*v1[1]+4*b[0][0][1]*v12[0]+4*b[0][1][1]*v12[1]+9*b[0][1][2]*v12[1]+9*b[0][1][0]*v2[1]+9*b[0][1][1]*v1[1]+9*b[0][0][2]*v12[0]+9*b[0][0][0]*v2[0] + (vb[0]*(3*b[0][0][0]+4*b[0][0][1]+10*b[0][0][2]+4*b[1][0][0]+3*b[1][0][1]+10*b[1][0][2])+vb[1]*(3*b[0][1][0]+4*b[0][1][1]+10*b[0][1][2]+4*b[1][1][0]+3*b[1][1][1]+10*b[1][1][2]))*0.2)/30240.;
      e = s*(-15*b[0][1][2]*v1[1]-5*b[0][0][1]*v1[0]-10*b[0][0][0]*v1[0]-10*b[0][1][0]*v1[1]-3*b[0][0][0]*v12[0]-3*b[0][1][0]*v12[1]+2*v02[0]*b[0][0][1]-v02[0]*b[1][0][1]+2*v02[1]*b[0][1][1]-v02[1]*b[1][1][1]-15*b[0][0][2]*v1[0]+5*v02[0]*b[0][0][2]+5*v02[1]*b[0][1][2]+5*v0[0]*b[0][0][1]+5*v0[1]*b[0][1][1]+15*v0[0]*b[0][0][2]-v01[0]*b[0][0][0]+15*v0[0]*b[1][0][2]+5*v0[0]*b[1][0][0]+5*v01[0]*b[1][0][2]+2*v01[0]*b[1][0][0]+15*v0[1]*b[0][1][2]-v01[1]*b[0][1][0]+15*v0[1]*b[1][1][2]+5*v0[1]*b[1][1][0]+5*v01[1]*b[1][1][2]+2*v01[1]*b[1][1][0]-3*b[1][0][1]*v12[0]-2*b[1][0][0]*v12[0]-3*b[1][1][1]*v12[1]-2*b[1][1][0]*v12[1]-10*b[1][0][1]*v2[0]-5*b[1][0][1]*v1[0]-15*b[1][0][2]*v2[0]-5*b[1][0][0]*v2[0]-5*b[1][0][2]*v12[0]-10*b[1][1][1]*v2[1]-5*b[1][1][1]*v1[1]-15*b[1][1][2]*v2[1]-5*b[1][1][0]*v2[1]-5*b[1][1][2]*v12[1]-2*b[0][0][1]*v12[0]-2*b[0][1][1]*v12[1]-5*b[0][1][2]*v12[1]-5*b[0][1][0]*v2[1]-5*b[0][1][1]*v1[1]-5*b[0][0][2]*v12[0]-5*b[0][0][0]*v2[0])/151200.;
      fill_four_e_matrices(pelem,i0,Z,a,bb,c,d);
      if (i0 == 0)
         fill_diag_e_matrix(pelem,Z,e);
   }
}

void p2c_up_ij(n0,n1,n2,fa0,fa1,fa2,nu,Z,flux0,flux1,flux2)
NODE *n0, *n1, *n2;
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT nu, flux0, flux1, flux2;
{
   NFLINK *pnf;
   FNLINK *pfn;
   FLOAT a0=0., a1, a2, b0, b1, b2, c0, c1, c2;

   if (IS_BF(fa0))
      a0 = flux0*(1. - 2.*upwind_lambda(flux0,nu))/12.;
   a1 = 0.25*flux1*(1. - upwind_lambda(flux1,nu));
   a2 = 0.25*flux2*(1. - upwind_lambda(flux2,nu));
   b0 = a1 + a2 - a0 - a0;
   b1 = a0 - a1;
   b2 = a0 - a2;
   a0 *= 4./3.;
   c0 = 0.5*(a0*1.25 - a1 - a2);
   c1 = 0.5*(a1 - a0); 
   c2 = 0.5*(a2 - a0); 
   COEFFN(n1,Z) += b1;
   COEFFN(n2,Z) += b2;
   putaij(n1->tstart,n0,n2,b0,b2,Z);
   putaij(n2->tstart,n0,n1,b0,b1,Z);
   putasij(n1->tnfstart,fa0,fa2,c0,c2,Z);
   putasij(n2->tnfstart,fa0,fa1,c0,c1,Z);
   for (pnf=n1->tnfstart; pnf->nbface!=fa1; pnf=pnf->next);
   COEFFL(pnf,Z) += c1;
   for (pnf=n2->tnfstart; pnf->nbface!=fa2; pnf=pnf->next);
   COEFFL(pnf,Z) += c2;
   COEFF_FF(fa0,Z) += c0*0.5;
   putppij(fa0->tfstart,fa1,fa2,c1*0.5,c2*0.5,Z);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) += b0*0.5;
   for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
   COEFFL(pfn,Z) += b1*0.5;
   for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
   COEFFL(pfn,Z) += b2*0.5;
}

void r_p2c_ij(n0,n1,n2,fa0,fa1,fa2,i0,i1,i2,rn,rf,c,s,Z,rdetB)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i0, i1, i2, Z;
FLOAT rn[DIM2], rf[DIM2], c, s, rdetB;
{
   NFLINK *pnf;
   FNLINK *pfn;
  
      COEFFN(n0,Z) += (s/60. + (4.*rn[i0]-rf[i0]-c)/90.)*rdetB;
      putaij(n0->tstart,n1,n2,(s+rf[i2]-rn[i2])*rdetB/180.,
                              (s+rf[i1]-rn[i1])*rdetB/180.,Z);
      putasij(n0->tnfstart,fa1,fa2,
                  (6.*(s+rn[i0])-4.*rf[i0]-3.*(c+rf[i2])+rn[i2])*rdetB/1260.,
                  (6.*(s+rn[i0])-4.*rf[i0]-3.*(c+rf[i1])+rn[i1])*rdetB/1260.,Z);
      for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
      COEFFL(pnf,Z) += (s+s-c)*rdetB/1260.;
                                          
      COEFF_FF(fa0,Z) += (3.*s+6.*rf[i0]-4.*rn[i0])*rdetB/5040.;
      putppij(fa0->tfstart,fa1,fa2,(3.*s-rf[i2]+2.*(rn[i2]-c))*rdetB/5040.,
                                   (3.*s-rf[i1]+2.*(rn[i1]-c))*rdetB/5040.,Z);
      for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
      COEFFL(pfn,Z) += (s+s-c)*rdetB/1260.;
      for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
      COEFFL(pfn,Z) +=
                     (6.*(s+rn[i1])-4.*rf[i1]-3.*(c+rf[i2])+rn[i2])*rdetB/1260.;
      for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
      COEFFL(pfn,Z) +=
                     (6.*(s+rn[i2])-4.*rf[i2]-3.*(c+rf[i1])+rn[i1])*rdetB/1260.;
}

#else  /*  if !((N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && 
                (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && 
                (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && 
                (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES))  */

void p2c_up_ij(n0,n1,n2,fa0,fa1,fa2,nu,Z,flux0,flux1,flux2)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z; FLOAT nu, flux0, flux1, flux2;
{  eprintf("Error: p2c_up_ij not available.\n");  }

void r_p2c_ij(n0,n1,n2,fa0,fa1,fa2,i0,i1,i2,rn,rf,c,s,Z,rdetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i0, i1, i2, Z; FLOAT rn[DIM2], rf[DIM2], c, s, rdetB;
{  eprintf("Error: r_p2c_ij not available.\n");  }

#endif

#if (N_DATA & DxD_NODE_MATR) && (N_DATA & DxD_NODE_FACE_MATR) && (F_DATA & DxD_FACE_MATR) && (F_DATA & DxD_FACE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES)

void vp2c_up_ij(n0,n1,n2,fa0,fa1,fa2,nu,Z,flux0,flux1,flux2)
NODE *n0, *n1, *n2;
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT nu, flux0, flux1, flux2;
{
   NFLINK *pnf;
   FNLINK *pfn;
   FLOAT a0=0., a1, a2, b0, b1, b2, c0, c1, c2;

   if (IS_BF(fa0))
      a0 = flux0*(1. - 2.*upwind_lambda(flux0,nu))/12.;
   a1 = 0.25*flux1*(1. - upwind_lambda(flux1,nu));
   a2 = 0.25*flux2*(1. - upwind_lambda(flux2,nu));
   b0 = a1 + a2 - a0 - a0;
   b1 = a0 - a1;
   b2 = a0 - a2;
   a0 *= 4./3.;
   c0 = 0.5*(a0*1.25 - a1 - a2);
   c1 = 0.5*(a1 - a0); 
   c2 = 0.5*(a2 - a0); 
   SET_COEFFNN(n1,Z,b1,0.,0.,b1);
   SET_COEFFNN(n2,Z,b2,0.,0.,b2);
   vputaij(n1->tstart,n0,n2,b0,b2,Z);
   vputaij(n2->tstart,n0,n1,b0,b1,Z);
   vputasij(n1->tnfstart,fa0,fa2,c0,c2,Z);
   vputasij(n2->tnfstart,fa0,fa1,c0,c1,Z);
   for (pnf=n1->tnfstart; pnf->nbface!=fa1; pnf=pnf->next);
   SET_COEFFNN(pnf,Z,c1,0.,0.,c1);
   for (pnf=n2->tnfstart; pnf->nbface!=fa2; pnf=pnf->next);
   SET_COEFFNN(pnf,Z,c2,0.,0.,c2);
   c0 *= 0.5;
   c1 *= 0.5;
   c2 *= 0.5;
   b0 *= 0.5;
   b1 *= 0.5;
   b2 *= 0.5;
   SET_COEFFNN(fa0,Z,c0,0.,0.,c0);
   vputppij(fa0->tfstart,fa1,fa2,c1,c2,Z);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   SET_COEFFNN(pfn,Z,b0,0.,0.,b0);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
   SET_COEFFNN(pfn,Z,b1,0.,0.,b1);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
   SET_COEFFNN(pfn,Z,b2,0.,0.,b2);
}

#else

void vp2c_up_ij(n0,n1,n2,fa0,fa1,fa2,nu,Z,flux0,flux1,flux2)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z; FLOAT nu, flux0, flux1, flux2;
{  eprintf("Error: vp2c_up_ij not available.\n");  }

#endif

/*  stiffness matrix corresponding to \int_\om u_i(v\cdot\nabla u_j) \dx  */
void convij_sn_sf(pelem,n0,n1,n2,fa0,fa1,fa2,i0,i1,i2,vn,vf,vb,s,c,Z,b0,b1,b2,rdetB,bubble,mstruct)
ELEMENT *pelem;
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i0, i1, i2, Z, bubble, mstruct;
FLOAT vn[DIM2][DIM], vf[DIM2][DIM], vb[DIM], *s, *c, b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{
   FLOAT *v0, *v1, *v2, *v01, *v02, *v12, s0, s1, s2, c0, c1, c2, p, q, r, 
         a0=0., a1=0., a2=0., d0=0., d1=0., d2=0., 
         nn0, nn1, nn2, nf0, nf1, nf2, fn0, fn1, fn2, ff0, ff1, ff2, 
         ne, en, fe, ef, v0_0, v0_1, v0_2, v1_0, v2_0, v12_0, 
         v12_1, v12_2, v01_0, v02_0, v01_2, v02_1, vb_0, vb_1, vb_2;
  
   v0 = vn[i0];
   v1 = vn[i1];
   v2 = vn[i2];
   v01 = vf[i2];
   v02 = vf[i1];
   v12 = vf[i0];
   s0 = DOT(s,b0);
   s1 = DOT(s,b1);
   s2 = DOT(s,b2);
   c0 = DOT(c,b0);
   c1 = DOT(c,b1);
   c2 = DOT(c,b2);
   v0_0 = DOT(v0,b0);
   v0_1 = DOT(v0,b1);
   v0_2 = DOT(v0,b2);
   v1_0 = DOT(v1,b0);
   v2_0 = DOT(v2,b0);
   v12_0 = DOT(v12,b0);
   v12_1 = DOT(v12,b1);
   v12_2 = DOT(v12,b2);
   v01_0 = DOT(v01,b0);
   v02_0 = DOT(v02,b0);
   v01_2 = DOT(v01,b2);
   v02_1 = DOT(v02,b1);
   if (bubble){
      vb_0 = DOT(vb,b0);
      vb_1 = DOT(vb,b1);
      vb_2 = DOT(vb,b2);
      a0 = vb_0/3.;
      a1 = vb_1/3.;
      a2 = vb_2/3.;
      d0 = -vb_0/3.5;
      d1 = (vb_2-2.*vb_1)/7.;
      d2 = (vb_1-2.*vb_2)/7.;
   }
   q = rdetB/180.;
  
   p = rdetB/60.;
   nn0 = (5.*(s0+v0_0)+c0+c0-v12_0+a0)*p;
   nn1 = (5.*(s1+v0_1)+c1+c1-v12_1+a1)*p;
   nn2 = (5.*(s2+v0_2)+c2+c2-v12_2+a2)*p;
   nf0 = (-3.*(s0+s0+DOT(v1,b1)+DOT(v2,b2))-c0+v02_1+v01_2+d0)*q;
   nf1 = (-3.*(s1+s1+v1_0-4.*v0_2)-c1+v02_0+c2+c2-v12_2-v12_2+d1)*q;
   nf2 = (-3.*(s2+s2+v2_0-4.*v0_1)-c2+v01_0+c1+c1-v12_1-v12_1+d2)*q;
                                          
   if (bubble){
      a0 = -vb_0*0.75;
      a1 = (vb_0-2*vb_1)*0.25;
      a2 = (vb_0-2*vb_2)*0.25;
      d0 = vb_0/3.5;
      d1 = vb_1/3.5;
      d2 = vb_2/3.5;
   }
   p = rdetB/1260.;
   r = 4.*v12_0;
   ff0 = (7.*(-s0-s0+v0_0+DOT(v1,b2)+DOT(v2,b1))-c0-c0+v02_1+v01_2-r+a0)*p;
   ff1 = (7.*(-s1+v1_0+v2_0+v2_0)-c1-c1+v02_0+r+a1)*p,
   ff2 = (7.*(-s2+v1_0+v1_0+v2_0)-c2-c2+v01_0+r+a2)*p;
   fn0 = (3.*(s0+s0-v0_0)+c0+v12_0+d0)*q;
   fn1 = (3.*(s1+s1-v0_1)+c1+v12_1+d1)*q;
   fn2 = (3.*(s2+s2-v0_2)+c2+v12_2+d2)*q;
   if (bubble){
      r = DOT(v2,b1)+DOT(v1,b2);
      q = DOT(v02,b2)+DOT(v01,b1);
      ne = (7.*r-14.*v0_0+4.*(v01_2+v02_1)+q-vb_0*0.25)*p;
      en = (7.*s0+2.*c0+vb_0*0.5)*p;
      fe = (16.*(v1_0+v2_0)+4.*r+6.*v12_0-q+vb_0/3.)*p*0.25;
      ef = (4.*r-q-8.*s0-3.*c0-2.*vb_0/3.)*p*0.25;
   }
   if (mstruct & Q_FULL)
      vfill_sn_sf(pelem,n0,n1,n2,fa0,fa1,fa2,i0,Z,bubble,
                  nn0,nn1,nn2,nf0,nf1,nf2,fn0,fn1,fn2,ff0,ff1,ff2,ne,en,fe,ef);
   else
      sfill_sn_sf(pelem,n0,n1,n2,fa0,fa1,fa2,i0,Z,bubble,
                  nn0,nn1,nn2,nf0,nf1,nf2,fn0,fn1,fn2,ff0,ff1,ff2,ne,en,fe,ef);
}

#if (N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES) && (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA)

/*  stiffness matrix corresponding to 
              0.5*\int_\om u_i(v\cdot\nabla u_j) - u_j(v\cdot\nabla u_i) \dx  */
void conv_skew_ij_sn_sf(n0,n1,n2,fa0,fa1,fa2,s,c,v,Z,b0,b1,b2,rdetB)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT v, Z;
FLOAT *s, *c, b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{
   NFLINK *pnf;
   FNLINK *pfn;
   FLOAT *v0, *v1, *v2, *v01, *v02, *v12, s0, s1, s2, c0, c1, c2, p, q, 
         v0_0, v0_1, v0_2, v1_0, v2_0, v2_1, v1_2, v12_0, 
         v12_1, v12_2, v01_0, v02_0, v01_2, v02_1, p01, p02, p00;
  
   v0 = NDD(n0,v);
   v1 = NDD(n1,v);
   v2 = NDD(n2,v);
   v01 = FDVP(fa2,v);
   v02 = FDVP(fa1,v);
   v12 = FDVP(fa0,v);
   s0 = DOT(s,b0);
   s1 = DOT(s,b1);
   s2 = DOT(s,b2);
   c0 = DOT(c,b0);
   c1 = DOT(c,b1);
   c2 = DOT(c,b2);
   v0_0 = DOT(v0,b0);
   v0_1 = DOT(v0,b1);
   v0_2 = DOT(v0,b2);
   v1_0 = DOT(v1,b0);
   v2_0 = DOT(v2,b0);
   v2_1 = DOT(v2,b1);
   v1_2 = DOT(v1,b2);
   v12_0 = DOT(v12,b0);
   v12_1 = DOT(v12,b1);
   v12_2 = DOT(v12,b2);
   v01_0 = DOT(v01,b0);
   v02_0 = DOT(v02,b0);
   v01_2 = DOT(v01,b2);
   v02_1 = DOT(v02,b1);
  
      p = rdetB/120.;
      putaij(n0->tstart,n1,n2,(5.*(s1-s0+v0_1-v1_0)+c1+c1-c0-c0-v12_1+v02_0)*p,
                              (5.*(s2-s0+v0_2-v2_0)+c2+c2-c0-c0-v12_2+v01_0)*p,Z);
   q = rdetB/360.;
   p00 = (-3.*(4.*s0-v0_0+DOT(v1,b1)+DOT(v2,b2))-c0-c0-v12_0+v02_1+v01_2)*q;
   p01 = (6.*(s2+v0_2+v0_2)+3.*c2-v12_2-v12_2)*q;
   p02 = (6.*(s1+v0_1+v0_1)+3.*c1-v12_1-v12_1)*q;
   for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
   COEFFL(pnf,Z) += p00;
   putasij(n0->tnfstart,fa1,fa2,p01,p02,Z);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) -= p00;
   for (pfn=fa1->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) -= p01;
   for (pfn=fa2->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) -= p02;
 
      p = rdetB/2520.;
      putppij(fa0->tfstart,fa1,fa2,(s0-s1+v1_0-v0_1+v2_0+v2_0-v2_1-v2_1)*q+
                                   (c0+c0-c1-c1+v02_0-v12_1+4.*(v12_0-v02_1))*p,
                                   (s0-s2+v2_0-v0_2+v1_0+v1_0-v1_2-v1_2)*q+
                                   (c0+c0-c2-c2+v01_0-v12_2+4.*(v12_0-v01_2))*p,Z);
}

#else  /*  if !((N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && 
                (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && 
                (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && 
                (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES) &&
                (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA))  */

void conv_skew_ij_sn_sf(n0,n1,n2,fa0,fa1,fa2,s,c,v,Z,b0,b1,b2,rdetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT v, Z; FLOAT *s, *c, b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{  eprintf("Error: conv_skew_ij_sn_sf not available.\n");  }

#endif

/*  w = w0*l0 + w1*l1 + w2*l2 + w01*l0*l1 + w02*l0*l2 + w12*l1*l2;
    z = z0*l0 + z1*l1 + z2*l2 + z01*l0*l1 + z02*l0*l2 + z12*l1*l2;
    s = w0 + w1 + w2; 
    c = w01 + w02 + w12;
    sz = z0 + z1 + z2; 
    cz = z01 + z02 + z12;  */

/*  2*int(int(l0*w,y=0..1-x),x=0..1)  */
#define INT_QUADR_L0(w0,w12,s,c)   ((5.*(s+w0)+c+c-w12)/60.)

/*  2*int(int(w*l0*l0,y=0..1-x),x=0..1)  */
#define INT_QUADR_L0_L0(w0,w12,s,c)    ((6.*(s+w0+w0)+3.*c-w12-w12)/180.)

/*  2*int(int(w*l0*l1,y=0..1-x),x=0..1)  */
#define INT_QUADR_L0_L1(w2,w01,s,c)    ((3.*(s+s-w2)+c+w01)/180.)

/*  2*int(int(w*l0*l0*l1,y=0..1-x),x=0..1)  */
#define INT_QUADR_L0_L0_L1(w0,w1,w01,w12,s,c)                                  \
                                       ((7.*(s+w0+w0+w1)+3.*(c+w01)-w12)/1260.)

/*  2*int(int(l1*l2*w*z,y=0..1-x),x=0..1)  */
#define CONV_INT_L1_L2(w0,w1,w2,w12,w02,w01,z0,z1,z2,z12,z02,z01,s,c,sz,cz)    \
  ((w01*z01+w02*z02+w12*z12 + 28*s*sz + 8*(s*cz +sz*c) + 56*(w1*z1+w2*z2)+     \
    16*(w12*sz+z12*s - w12*z0 -z12*w0) + w12*cz + z12*c + 2*c*cz +             \
    28*(w1*z2+w2*z1) + 4*(z01*w1+z02*w2+w01*z1+w02*z2+w12*z12) )/5040.)

/*  2*int(int(l0*l0*w*z,y=0..1-x),x=0..1)  */
#define CONV_INT_L0_L0(w0,w1,w2,w12,w02,w01,z0,z1,z2,z12,z02,z01,s,c,sz,cz)    \
  ((10*(w01*z01+w02*z02+w12*z12) + 28*(w0*z0+w1*z1+w2*z2+s*sz) +               \
    36*(z0*c+w0*cz) + 56*(z0*s + w0*sz) + 168*w0*z0 - 32*(w12*z0 +z12*w0) +    \
    w12*cz + z12*c + 2*c*cz + 12*(z01*w1+z02*w2+w01*z1+w02*z2-w12*z12+         \
    s*cz +sz*c) + 4*(w02*z01+w01*z02-w12*sz-z12*s) )/5040.)

/*  2*int(int(l0*l0*l0*w*z,y=0..1-x),x=0..1)  */
#define CONV_INT_L0_L0_L0(w0,w1,w2,w12,w02,w01,z0,z1,z2,z12,z02,z01,s,c,sz,cz) \
  ((-2*w12*z12+4*c*cz+6*(w02*z01+w01*z02)+9*(-w2*z12-w1*z12-w12*z1-w12*z2)+    \
    16*(w01*z01+w02*z02)+18*(z02*w2+w02*z2+w1*z01+w01*z1+s*cz+sz*c)+           \
    72*(w02*z0+w0*z02+w0*z01+w01*z0-w2*z2-w1*z1)+108*(-w1*z2-w2*z1)+144*s*sz+  \
    576*w0*z0)/15120.)

/*  2*int(int(l0*l0*l1*w*z,y=0..1-x),x=0..1))  */
#define CONV_INT_L0_L0_L1(w0,w1,w2,w12,w02,w01,z0,z1,z2,z12,z02,z01,s,c,sz,cz) \
  ((120*w0*z0+24*s*sz + 9*(s*cz+c*sz) + 48*(w1*z0+w0*z1+w1*z1) +               \
    27*(w0*z01+w01*z0) + 12*(w0*z2+w2*z0+w01*z01) + 9*(w0*z02+w02*z0) +        \
    18*(w1*z01+w01*z1) + 4*(w02*z02+w01*z02+w02*z01) + 3*(w12*z01+w01*z12) -   \
    3*(w12*z2+w2*z12) + 2*(z02*w12+w02*z12+w12*z12))/15120.)

/*  2*int(int(l0*l1*l2*w*z,y=0..1-x),x=0..1))  */
#define CONV_INT_L0_L1_L2(w0,w1,w2,w12,w02,w01,z0,z1,z2,z12,z02,z01,s,c,sz,cz) \
  ((24*s*sz + 9*(s*cz+c*sz) + 2*c*cz + 12*(w0*z0+w1*z1+w2*z2) +                \
    z01*w01 + z02*w02+z12*w12 -                                                \
    3*(w0*z12+w1*z02+w2*z01+z0*w12+z1*w02+z2*w01))/15120.)

/*  2*int(int(l0*l0*l0*l0*w*z,y=0..1-x),x=0..1))  */
#define CONV_INT_L0_L0_L0_L0(w0,w1,w2,w12,w02,w01,z0,z1,z2,z12,z02,z01,s,c,sz,cz)\
  ((-3*w12*z12+5*c*cz+10*(w02*z01+w01*z02)+15*(-w2*z12-w1*z12-w12*z1-w12*z2)+  \
    25*(w01*z1+w1*z01+w02*z02+w01*z01+z02*w2+w02*z2+s*cz+sz*c)+                \
    125*(w0*z02+w02*z0+w01*z0+w0*z01)+135*(-w2*z2-w1*z1)+180*(-w1*z2-w2*z1)+   \
    225*s*sz+1125*w0*z0)/37800.)

/*  2*int(int(l0*l0*l0*l1*w*z,y=0..1-x),x=0..1))  */
#define CONV_INT_L0_L0_L0_L1(w0,w1,w2,w12,w02,w01,z0,z1,z2,z12,z02,z01,s,c,sz,cz)\
  ((4*(-w01*z12-w12*z01)+5*(-w1*z12-w12*z1)+6*(-z02*w12-w02*z12)-7*w12*z12+    \
    10*(-w2*z12-w12*z2+c*cz)+20*(w01*z01+s*cz+c*sz)+30*(w02*z0+w0*z02)+        \
    40*(w01*z1+w1*z01)+45*(w0*z2+w2*z0+s*sz)+80*(w0*z01+w01*z0)+               \
    90*w1*z1+135*(w1*z0+w0*z1)+405*w0*z0)/75600.)

/*  2*int(int(l0*l0*l1*l1*w*z,y=0..1-x),x=0..1))  */
#define CONV_INT_L0_L0_L1_L1(w0,w1,w2,w12,w02,w01,z0,z1,z2,z12,z02,z01,s,c,sz,cz)\
  ((2*(-w12*z12-w02*z02)+3*(-z02*w12-w02*z12)+                                 \
    5*(-w02*z2+w02*z0+w0*z02+w12*z1+w1*z12-z02*w2-w2*z12-w12*z2)+6*c*cz+       \
    15*(-w2*z2+s*cz+sz*c)+18*w01*z01+45*(w01*z1+w01*z0+w0*z01+w1*z01+s*sz)+    \
    90*w1*z0+90*w0*z1+135*(w0*z0+w1*z1))/75600.)

/*  2*int(int(l0*l0*l1*l2*w*z,y=0..1-x),x=0..1))  */
#define CONV_INT_L0_L0_L1_L2(w0,w1,w2,w12,w02,w01,z0,z1,z2,z12,z02,z01,s,c,sz,cz)\
  ((3*(w02*z02+w01*z01+c*cz)+5*(z02*w2+w02*z2+w1*z01+w01*z1)+                  \
    10*(w0*z02+w01*z02+w02*z0+w02*z01+w0*z01+w01*z0+s*cz+c*sz)-                \
    15*(w1*z2+w2*z1)+45*(w0*z0+s*sz))/75600.)

/*  2*int(int(l0*w*z,y=0..1-x),x=0..1)  */
#define CONV_INT_L0(w0,w1,w2,w12,w02,w01,z0,z1,z2,z12,z02,z01,s,c,sz,cz)       \
  ((6*(w01*z01+w02*z02) + 42*s*sz + 21*(s*cz +sz*c) + 2*(w12*cz+z12*c-w12*z12)-\
    14*(w1*z02+w2*z01+z1*w02+z2*w01+w12*sz+z12*s) -                            \
    7*(z01*w1+z02*w2+w01*z1+w02*z2) +                                          \
    84*w0*z0-21*(w1*z2+w2*z1)+3*(w01*z02+w02*z01))/1260.)

/*  2*int(int(w*z,y=0..1-x),x=0..1)  */
#define CONV_INT(w0,w1,w2,w12,w02,w01,z0,z1,z2,z12,z02,z01,s,c,sz,cz)          \
  ((15*(s*sz+w0*z0+w1*z1+w2*z2)+6*(s*cz +sz*c) + c*cz+w01*z01+w02*z02+w12*z12- \
    3*(w0*z12+w1*z02+w2*z01+z0*w12+z1*w02+z2*w01))/180.)

/*  2*int(int(w*f,y=0..1-x),x=0..1))  */
#define CONV_RHS_INT(w0,w1,w2,w12,w02,w01,                                     \
                     f0,f1,f2,f001,f002,f110,f112,f220,f221,f012)              \
  ((14*w0*f0+w01*f0+6*w01*f112+36*w01*f012-3*w01*f220-3*w01*f221+6*w01*f002+   \
    7*f0*w2+63*w0*f001+7*w0*f1+7*w2*f1+6*w02*f001-3*w02*f110+2*w02*f1+7*f0*w1+ \
    63*w1*f110+14*w1*f1-3*w12*f001+6*w12*f110+w12*f1+2*f0*w12+w01*f1+          \
    12*w01*f001+12*w01*f110+63*w1*f112+126*w1*f012+36*w12*f012-3*w02*f112+     \
    14*w2*f2+12*w12*f112+12*w12*f221+w02*f2+12*w02*f220+12*w02*f002+7*w0*f2+   \
    63*w0*f002+7*w1*f2+2*w01*f2+126*w0*f012+63*w2*f220+126*w2*f012+63*w2*f221+ \
    36*w02*f012+6*w02*f221-3*w12*f002+w12*f2+6*w12*f220+w02*f0)/840.)

/*  2*int(int(l0*w*f,y=0..1-x),x=0..1))  */
#define CONV_RHS_INT_L0(w0,w1,w2,w12,w02,w01,                                  \
                     f0,f1,f2,f001,f002,f110,f112,f220,f221,f012)              \
  ((144*w0*f0+6*w01*f0+9*w01*f112+162*w01*f012-27*w01*f220-18*w01*f221+        \
    54*w01*f002+12*f0*w2+540*w0*f001-108*w0*f110+48*w0*f1+72*w2*f001-          \
    36*w2*f110+24*w2*f1+54*w02*f001-27*w02*f110+11*w02*f1+12*f0*w1+            \
    144*w1*f001+144*w1*f110+12*w1*f1+9*w12*f001+9*w12*f110+2*w12*f1+2*f0*w12+  \
    4*w01*f1+108*w01*f001+27*w01*f110+72*w1*f112+432*w1*f012-36*w1*f221+       \
    108*w12*f012-18*w02*f112+12*w2*f2-36*w0*f112-36*w2*f112+9*w12*f112+        \
    9*w12*f221+4*w02*f2+27*w02*f220+108*w02*f002+48*w0*f2-108*w0*f220+         \
    540*w0*f002+24*w1*f2-36*w1*f220+72*w1*f002+11*w01*f2+648*w0*f012-          \
    36*w0*f221+144*w2*f002+144*w2*f220+432*w2*f012+72*w2*f221+162*w02*f012+    \
    9*w02*f221+9*w12*f002+2*w12*f2+9*w12*f220+6*w02*f0)/10080.)

/*  2*int(int(l0*l0*w*f,y=0..1-x),x=0..1))  */
#define CONV_RHS_INT_L0_L0(w0,w1,w2,w12,w02,w01,                               \
                     f0,f1,f2,f001,f002,f110,f112,f220,f221,f012)              \
  ((6*w02*f0+132*w0*f0+6*w01*f0+3*w02*f2+72*w02*f002+33*w0*f2-108*w0*f220+     \
    378*w0*f002+6*w2*f0+4*w2*f2+27*w2*f220+108*w2*f002+6*w01*f2-18*w01*f220+   \
    36*w01*f002+w12*f2+9*w12*f002+36*w02*f001+72*w02*f012+6*w1*f0+11*w1*f2-    \
    27*w1*f220+54*w1*f002+378*w0*f001+324*w0*f012-27*w0*f221+54*w2*f001+       \
    162*w2*f012+9*w2*f221+72*w01*f001+72*w01*f012-9*w01*f221+9*w12*f001+       \
    36*w12*f012-18*w02*f110-9*w02*f112+108*w1*f001+162*w1*f012-18*w1*f221-     \
    108*w0*f110-27*w0*f112-27*w2*f110-18*w2*f112+6*w02*f1+27*w1*f110+          \
    9*w1*f112+33*w0*f1+11*w2*f1+3*w01*f1+w12*f1+4*w1*f1)/10080.)

/*  2*int(int(l0*l1*w*f,y=0..1-x),x=0..1))  */
#define CONV_RHS_INT_L0_L1(w0,w1,w2,w12,w02,w01,                               \
                     f0,f1,f2,f001,f002,f110,f112,f220,f221,f012)              \
  ((6*w0*f0+w02*f2+9*w02*f002+11*w0*f2-27*w0*f220+54*w0*f002+2*w2*f0+2*w2*f2+  \
  9*w2*f220+9*w2*f002+4*w01*f2-9*w01*f220+9*w01*f002+w12*f0+w12*f2+9*w02*f001+ \
  36*w02*f012+4*w1*f0+11*w1*f2-18*w1*f220+9*w1*f002+108*w0*f001+162*w0*f012-   \
  18*w0*f221+9*w2*f001+108*w2*f012+9*w2*f221+27*w01*f001+54*w01*f012-          \
  9*w01*f221+36*w12*f012+27*w1*f001+162*w1*f012-27*w1*f221+27*w0*f110+         \
  9*w0*f112+9*w2*f110+9*w2*f112+27*w01*f110+9*w01*f112+9*w12*f110+             \
  9*w12*f112+w02*f1+108*w1*f110+54*w1*f112+4*w0*f1+2*w2*f1+6*w1*f1)/10080.)

#if (N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES)

/*  stiff. matrix corresponding to \int_K (v\cdot\nabla u)(v\cdot\nabla v)\dx */
void conv_stab_ij_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i, j, k, Z;
FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], ndetB;
{
   LINK *pli;
   FLINK *pfli;
   NFLINK *pnf;
   FNLINK *pfn;
   FLOAT p, p00, p01, p02;
  
   COEFFN(n0,Z) += 
               CONV_INT(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                        vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i], 
                        s[i],c[i],s[i],c[i])*ndetB;
   if (n1->index > n0->index){
      p = CONV_INT(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                   vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j], 
                   s[i],c[i],s[j],c[j])*ndetB;
      for (pli = n0->tstart; pli->nbnode != n1; pli=pli->next);
      COEFFL(pli,Z) += p;
      for (pli = n1->tstart; pli->nbnode != n0; pli=pli->next);
      COEFFL(pli,Z) += p;
   }
   if (n2->index > n0->index){
      p = CONV_INT(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                   vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k], 
                   s[i],c[i],s[k],c[k])*ndetB;
      for (pli = n0->tstart; pli->nbnode != n2; pli=pli->next);
      COEFFL(pli,Z) += p;
      for (pli = n2->tstart; pli->nbnode != n0; pli=pli->next);
      COEFFL(pli,Z) += p;
   }

   p00 = (CONV_INT_L0(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                      vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                      s[i],c[i],s[k],c[k]) +
          CONV_INT_L0(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                      vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                      s[i],c[i],s[j],c[j]))*ndetB;
   p01 = (CONV_INT_L0(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                      vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k], 
                      s[i],c[i],s[k],c[k]) +
          CONV_INT_L0(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                      vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                      s[i],c[i],s[i],c[i]))*ndetB;
   p02 = (CONV_INT_L0(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                      vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j], 
                      s[i],c[i],s[j],c[j]) +
          CONV_INT_L0(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                      vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                      s[i],c[i],s[i],c[i]))*ndetB;
   for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
   COEFFL(pnf,Z) += p00;
   putasij(n0->tnfstart,fa1,fa2,p01,p02,Z);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) += p00;
   for (pfn=fa1->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) += p01;
   for (pfn=fa2->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) += p02;
 
   COEFF_FF(fa0,Z) += 
         (CONV_INT_L0_L0(vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                         vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                         s[k],c[k],s[k],c[k]) +
       2.*CONV_INT_L1_L2(vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j],
                         vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k], 
                         s[j],c[j],s[k],c[k]) +
          CONV_INT_L0_L0(vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                         vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                         s[j],c[j],s[j],c[j]))*ndetB;
   if (fa1->index > fa0->index){
      p = (CONV_INT_L1_L2(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                          vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k],
                          s[i],c[i],s[k],c[k]) +
           CONV_INT_L1_L2(vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j],
                          vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                          s[j],c[j],s[k],c[k]) +
           CONV_INT_L1_L2(vn[k][k],vn[i][k],vn[j][k],vf[k][k],vf[i][k],vf[j][k],
                          vn[k][k],vn[i][k],vn[j][k],vf[k][k],vf[i][k],vf[j][k],
                          s[k],c[k],s[k],c[k]) +
           CONV_INT_L0_L0(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                          vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                          s[i],c[i],s[j],c[j]))*ndetB;
      for (pfli = fa0->tfstart; pfli->nbface != fa1; pfli = pfli->next);
      COEFF_FL(pfli,Z) += p;
      for (pfli = fa1->tfstart; pfli->nbface != fa0; pfli = pfli->next);
      COEFF_FL(pfli,Z) += p;
   }
   if (fa2->index > fa0->index){
      p = (CONV_INT_L1_L2(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                          vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j],
                          s[i],c[i],s[j],c[j]) +
           CONV_INT_L1_L2(vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j],
                          vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j],
                          s[j],c[j],s[j],c[j]) +
           CONV_INT_L1_L2(vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                          vn[k][k],vn[i][k],vn[j][k],vf[k][k],vf[i][k],vf[j][k],
                          s[j],c[j],s[k],c[k]) +
           CONV_INT_L0_L0(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                          vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                          s[i],c[i],s[k],c[k]))*ndetB;
      for (pfli = fa0->tfstart; pfli->nbface != fa2; pfli = pfli->next);
      COEFF_FL(pfli,Z) += p;
      for (pfli = fa2->tfstart; pfli->nbface != fa0; pfli = pfli->next);
      COEFF_FL(pfli,Z) += p;
   }
}

/*  stiff. matrix corresponding to \int_K (v\cdot\nabla u)(v\cdot\nabla v)\dx */
void conv_stab_ij_p1nc_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB,delta_0,delta_1,delta_2)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i, j, k, Z;
FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], ndetB;
{
   LINK *pli;
   FLINK *pfli;
   NFLINK *pnf;
   FNLINK *pfn;
   FLOAT p, p00, p01, p02;
  
   COEFFN(n0,Z) += 
              (CONV_INT_L0(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                        vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i], 
                        s[i],c[i],s[i],c[i])*delta_0 +
               CONV_INT_L0(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                        vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i], 
                        s[i],c[i],s[i],c[i])*delta_1 +
               CONV_INT_L0(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                        vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i], 
                        s[i],c[i],s[i],c[i])*delta_2)*ndetB;
   if (n1->index > n0->index){
      p = (CONV_INT_L0(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                       vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j], 
                       s[i],c[i],s[j],c[j])*delta_0 +
           CONV_INT_L0(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                       vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j], 
                       s[i],c[i],s[j],c[j])*delta_1 +
           CONV_INT_L0(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                       vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j], 
                       s[i],c[i],s[j],c[j])*delta_2)*ndetB;
      for (pli = n0->tstart; pli->nbnode != n1; pli=pli->next);
      COEFFL(pli,Z) += p;
      for (pli = n1->tstart; pli->nbnode != n0; pli=pli->next);
      COEFFL(pli,Z) += p;
   }
   if (n2->index > n0->index){
      p = (CONV_INT_L0(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                       vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k], 
                       s[i],c[i],s[k],c[k])*delta_0 +
           CONV_INT_L0(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                       vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k], 
                       s[i],c[i],s[k],c[k])*delta_1 +
           CONV_INT_L0(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                       vn[k][k],vn[i][k],vn[j][k],vf[k][k],vf[i][k],vf[j][k], 
                       s[i],c[i],s[k],c[k])*delta_2)*ndetB;
      for (pli = n0->tstart; pli->nbnode != n2; pli=pli->next);
      COEFFL(pli,Z) += p;
      for (pli = n2->tstart; pli->nbnode != n0; pli=pli->next);
      COEFFL(pli,Z) += p;
   }

   p00 = (CONV_INT_L1_L2(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                         vn[k][k],vn[i][k],vn[j][k],vf[k][k],vf[i][k],vf[j][k],
                         s[i],c[i],s[k],c[k])*delta_0 + 
          CONV_INT_L0_L0(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                         vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                         s[i],c[i],s[k],c[k])*delta_1 +
          CONV_INT_L1_L2(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                         vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k],
                         s[i],c[i],s[k],c[k])*delta_2 +

          CONV_INT_L1_L2(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                         vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j],
                         s[i],c[i],s[j],c[j])*delta_0 +
          CONV_INT_L1_L2(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                         vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j],
                         s[i],c[i],s[j],c[j])*delta_1 +
          CONV_INT_L0_L0(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                         vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                         s[i],c[i],s[j],c[j])*delta_2
         )*ndetB;

   p01 = (CONV_INT_L0_L0(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                         vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k], 
                         s[i],c[i],s[k],c[k])*delta_0 +
          CONV_INT_L1_L2(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                         vn[k][k],vn[i][k],vn[j][k],vf[k][k],vf[i][k],vf[j][k], 
                         s[i],c[i],s[k],c[k])*delta_1 +
          CONV_INT_L1_L2(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                         vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k], 
                         s[i],c[i],s[k],c[k])*delta_2 +

          CONV_INT_L1_L2(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                         vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                         s[i],c[i],s[i],c[i])*delta_0 +
          CONV_INT_L1_L2(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                         vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                         s[i],c[i],s[i],c[i])*delta_1 +
          CONV_INT_L0_L0(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                         vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                         s[i],c[i],s[i],c[i])*delta_2
         )*ndetB;

   p02 = (CONV_INT_L0_L0(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                         vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j], 
                         s[i],c[i],s[j],c[j])*delta_0 +
          CONV_INT_L1_L2(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                         vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j], 
                         s[i],c[i],s[j],c[j])*delta_1 +
          CONV_INT_L1_L2(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                         vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j], 
                         s[i],c[i],s[j],c[j])*delta_2 +

          CONV_INT_L1_L2(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                         vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                         s[i],c[i],s[i],c[i])*delta_0 +
          CONV_INT_L0_L0(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                         vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                         s[i],c[i],s[i],c[i])*delta_1 +
          CONV_INT_L1_L2(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                         vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                         s[i],c[i],s[i],c[i])*delta_2
         )*ndetB;

   for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
   COEFFL(pnf,Z) += p00;
   putasij(n0->tnfstart,fa1,fa2,p01,p02,Z);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) += p00;
   for (pfn=fa1->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) += p01;
   for (pfn=fa2->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFL(pfn,Z) += p02;
 
   COEFF_FF(fa0,Z) += 
         (CONV_INT_L0_L0_L1(vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                            vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                            s[k],c[k],s[k],c[k])*delta_0 +
          CONV_INT_L0_L0_L0(vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                            vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                            s[k],c[k],s[k],c[k])*delta_1 +
          CONV_INT_L0_L0_L1(vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                            vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                            s[k],c[k],s[k],c[k])*delta_2 +

       2.*CONV_INT_L0_L1_L2(vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j],
                            vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k], 
                            s[j],c[j],s[k],c[k])*delta_0 +
       2.*CONV_INT_L0_L0_L1(vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j],
                            vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k], 
                            s[j],c[j],s[k],c[k])*delta_1 +
       2.*CONV_INT_L0_L0_L1(vn[k][j],vn[j][j],vn[i][j],vf[k][j],vf[j][j],vf[i][j],
                            vn[k][k],vn[j][k],vn[i][k],vf[k][k],vf[j][k],vf[i][k], 
                            s[j],c[j],s[k],c[k])*delta_2 +

          CONV_INT_L0_L0_L1(vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                            vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                            s[j],c[j],s[j],c[j])*delta_0 +
          CONV_INT_L0_L0_L1(vn[k][j],vn[j][j],vn[i][j],vf[k][j],vf[j][j],vf[i][j],
                            vn[k][j],vn[j][j],vn[i][j],vf[k][j],vf[j][j],vf[i][j],
                            s[j],c[j],s[j],c[j])*delta_1 +
          CONV_INT_L0_L0_L0(vn[k][j],vn[j][j],vn[i][j],vf[k][j],vf[j][j],vf[i][j],
                            vn[k][j],vn[j][j],vn[i][j],vf[k][j],vf[j][j],vf[i][j],
                            s[j],c[j],s[j],c[j])*delta_2
         )*ndetB;

   if (fa1->index > fa0->index){
      p = (CONV_INT_L0_L1_L2(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                             vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k],
                             s[i],c[i],s[k],c[k])*delta_0 +
           CONV_INT_L0_L0_L1(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                             vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                             s[i],c[i],s[k],c[k])*delta_1 +
           CONV_INT_L0_L0_L1(vn[k][i],vn[j][i],vn[i][i],vf[k][i],vf[j][i],vf[i][i],
                             vn[k][k],vn[j][k],vn[i][k],vf[k][k],vf[j][k],vf[i][k],
                             s[i],c[i],s[k],c[k])*delta_2 +

           CONV_INT_L0_L0_L1(vn[i][j],vn[k][j],vn[j][j],vf[i][j],vf[k][j],vf[j][j],
                             vn[i][k],vn[k][k],vn[j][k],vf[i][k],vf[k][k],vf[j][k],
                             s[j],c[j],s[k],c[k])*delta_0 +
           CONV_INT_L0_L1_L2(vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j],
                             vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k],
                             s[j],c[j],s[k],c[k])*delta_1 +
           CONV_INT_L0_L0_L1(vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                             vn[k][k],vn[i][k],vn[j][k],vf[k][k],vf[i][k],vf[j][k],
                             s[j],c[j],s[k],c[k])*delta_2 +

           CONV_INT_L0_L0_L1(vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k],
                             vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k],
                             s[k],c[k],s[k],c[k])*delta_0 +
           CONV_INT_L0_L0_L1(vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                             vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                             s[k],c[k],s[k],c[k])*delta_1 +
           CONV_INT_L0_L1_L2(vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                             vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                             s[k],c[k],s[k],c[k])*delta_2 +

           CONV_INT_L0_L0_L1(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                             vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                             s[i],c[i],s[j],c[j])*delta_0 +
           CONV_INT_L0_L0_L1(vn[k][i],vn[j][i],vn[i][i],vf[k][i],vf[j][i],vf[i][i],
                             vn[k][j],vn[j][j],vn[i][j],vf[k][j],vf[j][j],vf[i][j],
                             s[i],c[i],s[j],c[j])*delta_1 +
           CONV_INT_L0_L0_L0(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                             vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                             s[i],c[i],s[j],c[j])*delta_2
          )*ndetB;

      for (pfli = fa0->tfstart; pfli->nbface != fa1; pfli = pfli->next);
      COEFF_FL(pfli,Z) += p;
      for (pfli = fa1->tfstart; pfli->nbface != fa0; pfli = pfli->next);
      COEFF_FL(pfli,Z) += p;
   }
   if (fa2->index > fa0->index){
      p = (CONV_INT_L0_L1_L2(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                             vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j],
                             s[i],c[i],s[j],c[j])*delta_0 +
           CONV_INT_L0_L0_L1(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                             vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j],
                             s[i],c[i],s[j],c[j])*delta_1 +
           CONV_INT_L0_L0_L1(vn[k][i],vn[j][i],vn[i][i],vf[k][i],vf[j][i],vf[i][i],
                             vn[k][j],vn[j][j],vn[i][j],vf[k][j],vf[j][j],vf[i][j],
                             s[i],c[i],s[j],c[j])*delta_2 +

           CONV_INT_L0_L0_L1(vn[i][j],vn[k][j],vn[j][j],vf[i][j],vf[k][j],vf[j][j],
                             vn[i][j],vn[k][j],vn[j][j],vf[i][j],vf[k][j],vf[j][j],
                             s[j],c[j],s[j],c[j])*delta_0 +
           CONV_INT_L0_L1_L2(vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j],
                             vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j],
                             s[j],c[j],s[j],c[j])*delta_1 +
           CONV_INT_L0_L0_L1(vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                             vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                             s[j],c[j],s[j],c[j])*delta_2 +

           CONV_INT_L0_L0_L1(vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j],
                             vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k],
                             s[j],c[j],s[k],c[k])*delta_0 +
           CONV_INT_L0_L0_L1(vn[j][j],vn[i][j],vn[k][j],vf[j][j],vf[i][j],vf[k][j],
                             vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                             s[j],c[j],s[k],c[k])*delta_1 +
           CONV_INT_L0_L1_L2(vn[j][j],vn[i][j],vn[k][j],vf[j][j],vf[i][j],vf[k][j],
                             vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                             s[j],c[j],s[k],c[k])*delta_2 +

           CONV_INT_L0_L0_L1(vn[j][i],vn[i][i],vn[k][i],vf[j][i],vf[i][i],vf[k][i],
                             vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                             s[i],c[i],s[k],c[k])*delta_0 +
           CONV_INT_L0_L0_L0(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                             vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                             s[i],c[i],s[k],c[k])*delta_1 +
           CONV_INT_L0_L0_L1(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                             vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                             s[i],c[i],s[k],c[k])*delta_2
          )*ndetB;

      for (pfli = fa0->tfstart; pfli->nbface != fa2; pfli = pfli->next);
      COEFF_FL(pfli,Z) += p;
      for (pfli = fa2->tfstart; pfli->nbface != fa0; pfli = pfli->next);
      COEFF_FL(pfli,Z) += p;
   }
}

/* stiff.matrix corresponding to \int_K (-nu \Delta u_j)(v\cdot\nabla v_i)\dx */
void lapl_stab_ij_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,b,ndetB)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i, j, k, Z;
FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], b[DIM2][DIM2], ndetB;
{
   NFLINK *pnf;
   FLOAT p, p0, p1, p2;
  
   p0 = 2.*DOT(b[j],b[k]);
   p1 = 2.*DOT(b[i],b[k]);
   p2 = 2.*DOT(b[i],b[j]);
      p = -ndetB*(4.*s[i]+c[i])/12.;
      putasij(n0->tnfstart,fa1,fa2,p1*p,p2*p,Z);
      for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
      COEFFL(pnf,Z) += p0*p;
                                          
      p = -(INT_QUADR_L0(vn[j][k],vf[j][k],s[k],c[k]) +
            INT_QUADR_L0(vn[k][j],vf[k][j],s[j],c[j]))*ndetB;
      COEFF_FF(fa0,Z) += p0*p;
      putppij(fa0->tfstart,fa1,fa2,p1*p,p2*p,Z);
}

/* stiff.matrix corresponding to \int_K (-nu \Delta u_j)(v\cdot\nabla v_i)\dx */
void lapl_stab_ij_p1nc_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,b,ndetB,
                                                       delta_0,delta_1, delta_2)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i, j, k, Z;
FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], b[DIM2][DIM2], ndetB,
                                                      delta_0, delta_1, delta_2;
{
   NFLINK *pnf;
   FLOAT p, p0, p1, p2;
  
   p0 = 2.*DOT(b[j],b[k]);
   p1 = 2.*DOT(b[i],b[k]);
   p2 = 2.*DOT(b[i],b[j]);
   p = -(INT_QUADR_L0(vn[i][i],vf[i][i],s[i],c[i])*delta_0 +
         INT_QUADR_L0(vn[j][i],vf[j][i],s[i],c[i])*delta_1 +
         INT_QUADR_L0(vn[k][i],vf[k][i],s[i],c[i])*delta_2
        )*ndetB;
   putasij(n0->tnfstart,fa1,fa2,p1*p,p2*p,Z);
   for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
   COEFFL(pnf,Z) += p0*p;

   p = -(INT_QUADR_L0_L1(vn[k][k],vf[k][k],s[k],c[k])*delta_0 +
         INT_QUADR_L0_L0(vn[j][k],vf[j][k],s[k],c[k])*delta_1 +
         INT_QUADR_L0_L1(vn[i][k],vf[i][k],s[k],c[k])*delta_2 +

         INT_QUADR_L0_L1(vn[j][j],vf[j][j],s[j],c[j])*delta_0 +
         INT_QUADR_L0_L1(vn[i][j],vf[i][j],s[j],c[j])*delta_1 +
         INT_QUADR_L0_L0(vn[k][j],vf[k][j],s[j],c[j])*delta_2
        )*ndetB;
   COEFF_FF(fa0,Z) += p0*p;
   putppij(fa0->tfstart,fa1,fa2,p1*p,p2*p,Z);
}

/* stiff.matrix corresponding to \int_K u_j(v\cdot\nabla v_i)\dx */
void time_stab_ij_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i, j, k, Z;
FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], ndetB;
{
   NFLINK *pnf;
   FNLINK *pfn;
  
      COEFFN(n0,Z) += INT_QUADR_L0(vn[i][i],vf[i][i],s[i],c[i])*ndetB;
      putaij(n0->tstart,n1,n2,INT_QUADR_L0(vn[j][i],vf[j][i],s[i],c[i])*ndetB,
                              INT_QUADR_L0(vn[k][i],vf[k][i],s[i],c[i])*ndetB,Z);
      putasij(n0->tnfstart,fa1,fa2,
                          INT_QUADR_L0_L1(vn[j][i],vf[j][i],s[i],c[i])*ndetB,
                          INT_QUADR_L0_L1(vn[k][i],vf[k][i],s[i],c[i])*ndetB,Z);
      for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
      COEFFL(pnf,Z) += INT_QUADR_L0_L1(vn[i][i],vf[i][i],s[i],c[i])*ndetB;
                                          
      COEFF_FF(fa0,Z) += 
      (INT_QUADR_L0_L0_L1(vn[j][k],vn[k][k],vf[i][k],vf[j][k],s[k],c[k]) +
       INT_QUADR_L0_L0_L1(vn[k][j],vn[j][j],vf[i][j],vf[k][j],s[j],c[j]))*ndetB;
      putppij(fa0->tfstart,fa1,fa2,
       ((7.*s[k]+c[k]+c[k])/1260. +
       INT_QUADR_L0_L0_L1(vn[k][j],vn[i][j],vf[j][j],vf[k][j],s[j],c[j]))*ndetB,
      (INT_QUADR_L0_L0_L1(vn[j][k],vn[i][k],vf[k][k],vf[j][k],s[k],c[k]) +
        (7.*s[j]+c[j]+c[j])/1260.)*ndetB,Z);
      for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
      COEFFL(pfn,Z) += (INT_QUADR_L0_L1(vn[k][k],vf[k][k],s[k],c[k]) +
                        INT_QUADR_L0_L1(vn[j][j],vf[j][j],s[j],c[j]))*ndetB;
      for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
      COEFFL(pfn,Z) += (INT_QUADR_L0_L0(vn[j][k],vf[j][k],s[k],c[k]) +
                        INT_QUADR_L0_L1(vn[i][j],vf[i][j],s[j],c[j]))*ndetB;
      for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
      COEFFL(pfn,Z) += (INT_QUADR_L0_L1(vn[i][k],vf[i][k],s[k],c[k]) +
                        INT_QUADR_L0_L0(vn[k][j],vf[k][j],s[j],c[j]))*ndetB;
}

/* stiff.matrix corresponding to \int_K r u_j(v\cdot\nabla v_i)\dx */
void react_stab_ij_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,
                                                    s,c,vn,vf,sr,cr,rn,rf,ndetB)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i, j, k, Z;
FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], sr, cr, rn[SIDES], rf[SIDES], 
                                                                          ndetB;
{
   NFLINK *pnf;
   FNLINK *pfn;
  
      COEFFN(n0,Z) += CONV_INT_L0(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                              vn[i][i],vn[j][i],vn[k][i],
                              vf[i][i],vf[j][i],vf[k][i],sr,cr,s[i],c[i])*ndetB;
      putaij(n0->tstart,n1,n2,CONV_INT_L0(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                           vn[j][i],vn[k][i],vn[i][i],
                           vf[j][i],vf[k][i],vf[i][i],sr,cr,s[i],c[i])*ndetB,
                              CONV_INT_L0(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                           vn[k][i],vn[i][i],vn[j][i],
                           vf[k][i],vf[i][i],vf[j][i],sr,cr,s[i],c[i])*ndetB,Z);
      putasij(n0->tnfstart,fa1,fa2,
                           CONV_INT_L1_L2(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                           vn[j][i],vn[k][i],vn[i][i],
                           vf[j][i],vf[k][i],vf[i][i],sr,cr,s[i],c[i])*ndetB,
                           CONV_INT_L1_L2(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                           vn[k][i],vn[i][i],vn[j][i],
                           vf[k][i],vf[i][i],vf[j][i],sr,cr,s[i],c[i])*ndetB,Z);
      for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
      COEFFL(pnf,Z) += CONV_INT_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                              vn[i][i],vn[j][i],vn[k][i],
                              vf[i][i],vf[j][i],vf[k][i],sr,cr,s[i],c[i])*ndetB;
                                          
      COEFF_FF(fa0,Z) += (CONV_INT_L0_L0_L1(rn[k],rn[j],rn[i],rf[k],rf[j],rf[i],
                           vn[k][j],vn[j][j],vn[i][j],
                           vf[k][j],vf[j][j],vf[i][j],sr,cr,s[j],c[j]) +
                          CONV_INT_L0_L0_L1(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                           vn[j][k],vn[k][k],vn[i][k],
                           vf[j][k],vf[k][k],vf[i][k],sr,cr,s[k],c[k]))*ndetB;
      putppij(fa0->tfstart,fa1,fa2,
                    (CONV_INT_L0_L0_L1(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                          vn[k][j],vn[i][j],vn[j][j],
                          vf[k][j],vf[i][j],vf[j][j],sr,cr,s[j],c[j]) +
                     CONV_INT_L0_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                          vn[i][k],vn[j][k],vn[k][k],
                          vf[i][k],vf[j][k],vf[k][k],sr,cr,s[k],c[k]))*ndetB,
                    (CONV_INT_L0_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                             vn[i][j],vn[j][j],vn[k][j],
                             vf[i][j],vf[j][j],vf[k][j],sr,cr,s[j],c[j]) +
                     CONV_INT_L0_L0_L1(rn[j],rn[i],rn[k],rf[j],rf[i],rf[k],
                          vn[j][k],vn[i][k],vn[k][k],
                          vf[j][k],vf[i][k],vf[k][k],sr,cr,s[k],c[k]))*ndetB,Z);
      for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
      COEFFL(pfn,Z) += (CONV_INT_L1_L2(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                           vn[j][j],vn[k][j],vn[i][j],
                           vf[j][j],vf[k][j],vf[i][j],sr,cr,s[j],c[j]) +
                        CONV_INT_L1_L2(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                           vn[k][k],vn[i][k],vn[j][k],
                           vf[k][k],vf[i][k],vf[j][k],sr,cr,s[k],c[k]))*ndetB;
      for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
      COEFFL(pfn,Z) += (CONV_INT_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                              vn[i][j],vn[j][j],vn[k][j],
                              vf[i][j],vf[j][j],vf[k][j],sr,cr,s[j],c[j]) +
                        CONV_INT_L0_L0(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                           vn[j][k],vn[k][k],vn[i][k],
                           vf[j][k],vf[k][k],vf[i][k],sr,cr,s[k],c[k]))*ndetB;
      for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
      COEFFL(pfn,Z) += (CONV_INT_L0_L0(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                           vn[k][j],vn[i][j],vn[j][j],
                           vf[k][j],vf[i][j],vf[j][j],sr,cr,s[j],c[j]) +
                        CONV_INT_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                           vn[i][k],vn[j][k],vn[k][k],
                           vf[i][k],vf[j][k],vf[k][k],sr,cr,s[k],c[k]))*ndetB;
}

/* stiff.matrix corresponding to \int_K r u_j(v\cdot\nabla v_i)\dx */
void react_stab_ij_p1nc_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,
                            s,c,vn,vf,sr,cr,rn,rf,ndetB,delta_0,delta_1,delta_2)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i, j, k, Z;
FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], sr, cr, rn[SIDES], rf[SIDES], 
                                               ndetB, delta_0, delta_1, delta_2;
{
   NFLINK *pnf;
   FNLINK *pfn;
  
      COEFFN(n0,Z) += 
            (CONV_INT_L0_L0(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                            vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                            sr,cr,s[i],c[i])*delta_0 +
             CONV_INT_L1_L2(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                            vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                            sr,cr,s[i],c[i])*delta_1 +
             CONV_INT_L1_L2(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                            vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                            sr,cr,s[i],c[i])*delta_2
            )*ndetB;

      putaij(n0->tstart,n1,n2,
            (CONV_INT_L1_L2(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                            vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                            sr,cr,s[i],c[i])*delta_0 +
             CONV_INT_L0_L0(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                            vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                            sr,cr,s[i],c[i])*delta_1 +
             CONV_INT_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                            vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                            sr,cr,s[i],c[i])*delta_2
            )*ndetB,

            (CONV_INT_L1_L2(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                            vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                            sr,cr,s[i],c[i])*delta_0 +
             CONV_INT_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                            vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                            sr,cr,s[i],c[i])*delta_1 +
             CONV_INT_L0_L0(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                            vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                            sr,cr,s[i],c[i])*delta_2
            )*ndetB,Z);

      putasij(n0->tnfstart,fa1,fa2,
            (CONV_INT_L0_L0_L1(rn[i],rn[k],rn[j],rf[i],rf[k],rf[j],
                               vn[i][i],vn[k][i],vn[j][i],vf[i][i],vf[k][i],vf[j][i],
                               sr,cr,s[i],c[i])*delta_0 +
             CONV_INT_L0_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                               vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                               sr,cr,s[i],c[i])*delta_1 +
             CONV_INT_L0_L0_L1(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                               vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                               sr,cr,s[i],c[i])*delta_2
            )*ndetB,

            (CONV_INT_L0_L0_L1(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                               vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                               sr,cr,s[i],c[i])*delta_0 +
             CONV_INT_L0_L0_L1(rn[j],rn[i],rn[k],rf[j],rf[i],rf[k],
                               vn[j][i],vn[i][i],vn[k][i],vf[j][i],vf[i][i],vf[k][i],
                               sr,cr,s[i],c[i])*delta_1 +
             CONV_INT_L0_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                               vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                               sr,cr,s[i],c[i])*delta_2
            )*ndetB,Z);

      for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);



      COEFFL(pnf,Z) += (CONV_INT_L0_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                               vn[i][i],vn[j][i],vn[k][i], vf[i][i],vf[j][i],vf[k][i],
                               sr,cr,s[i],c[i])*delta_0 +
                        CONV_INT_L0_L0_L1(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                               vn[j][i],vn[k][i],vn[i][i], vf[j][i],vf[k][i],vf[i][i],
                               sr,cr,s[i],c[i])*delta_1 +
                        CONV_INT_L0_L0_L1(rn[k],rn[j],rn[i],rf[k],rf[j],rf[i],
                               vn[k][i],vn[j][i],vn[i][i], vf[k][i],vf[j][i],vf[i][i],
                               sr,cr,s[i],c[i])*delta_2
                       )*ndetB;

      COEFF_FF(fa0,Z) += (CONV_INT_L0_L0_L1_L2(rn[k],rn[j],rn[i],rf[k],rf[j],rf[i],
                               vn[k][j],vn[j][j],vn[i][j],vf[k][j],vf[j][j],vf[i][j],
                               sr,cr,s[j],c[j])*delta_0 +
                          CONV_INT_L0_L0_L1_L1(rn[k],rn[j],rn[i],rf[k],rf[j],rf[i],
                               vn[k][j],vn[j][j],vn[i][j],vf[k][j],vf[j][j],vf[i][j],
                               sr,cr,s[j],c[j])*delta_1 +
                          CONV_INT_L0_L0_L0_L1(rn[k],rn[j],rn[i],rf[k],rf[j],rf[i],
                               vn[k][j],vn[j][j],vn[i][j],vf[k][j],vf[j][j],vf[i][j],
                               sr,cr,s[j],c[j])*delta_2 +

                          CONV_INT_L0_L0_L1_L2(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                               vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                               sr,cr,s[k],c[k])*delta_0 +
                          CONV_INT_L0_L0_L0_L1(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                               vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                               sr,cr,s[k],c[k])*delta_1 +
                          CONV_INT_L0_L0_L1_L1(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                               vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                               sr,cr,s[k],c[k])*delta_2
                         )*ndetB;

      putppij(fa0->tfstart,fa1,fa2,
                    (CONV_INT_L0_L0_L1_L1(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                          vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                          sr,cr,s[j],c[j])*delta_0 +
                     CONV_INT_L0_L0_L1_L2(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                          vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                          sr,cr,s[j],c[j])*delta_1 +
                     CONV_INT_L0_L0_L0_L1(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                          vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                          sr,cr,s[j],c[j])*delta_2 +

                     CONV_INT_L0_L0_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                          vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k],
                          sr,cr,s[k],c[k])*delta_0 +
                     CONV_INT_L0_L0_L1_L2(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                          vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                          sr,cr,s[k],c[k])*delta_1 +
                     CONV_INT_L0_L0_L1_L2(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                          vn[k][k],vn[i][k],vn[j][k],vf[k][k],vf[i][k],vf[j][k],
                          sr,cr,s[k],c[k])*delta_2
                    )*ndetB,
                    
                    (CONV_INT_L0_L0_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                          vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j],
                          sr,cr,s[j],c[j])*delta_0 +
                     CONV_INT_L0_L0_L1_L2(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                          vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j],
                          sr,cr,s[j],c[j])*delta_1 +
                     CONV_INT_L0_L0_L1_L2(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                          vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                          sr,cr,s[j],c[j])*delta_2 +

                     CONV_INT_L0_L0_L1_L1(rn[j],rn[i],rn[k],rf[j],rf[i],rf[k],
                          vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                          sr,cr,s[k],c[k])*delta_0 +
                     CONV_INT_L0_L0_L0_L1(rn[j],rn[i],rn[k],rf[j],rf[i],rf[k],
                          vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                          sr,cr,s[k],c[k])*delta_1 +
                     CONV_INT_L0_L0_L1_L2(rn[j],rn[i],rn[k],rf[j],rf[i],rf[k],
                          vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                          sr,cr,s[k],c[k])*delta_2
                    )*ndetB,Z);

      for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
      COEFFL(pfn,Z) += (CONV_INT_L0_L0_L1(rn[i],rn[k],rn[j],rf[i],rf[k],rf[j],
                          vn[i][j],vn[k][j],vn[j][j],vf[i][j],vf[k][j],vf[j][j],
                          sr,cr,s[j],c[j])*delta_0 +
                        CONV_INT_L0_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                          vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j],
                          sr,cr,s[j],c[j])*delta_1 +
                        CONV_INT_L0_L0_L1(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                          vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                          sr,cr,s[j],c[j])*delta_2 +

                        CONV_INT_L0_L0_L1(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                          vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k],
                          sr,cr,s[k],c[k])*delta_0 +
                        CONV_INT_L0_L0_L1(rn[j],rn[i],rn[k],rf[j],rf[i],rf[k],
                          vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                          sr,cr,s[k],c[k])*delta_1 +
                        CONV_INT_L0_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                          vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k],
                          sr,cr,s[k],c[k])*delta_2
                       )*ndetB;

      for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
      COEFFL(pfn,Z) += (CONV_INT_L0_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                          vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j],
                          sr,cr,s[j],c[j])*delta_0 +
                        CONV_INT_L0_L0_L1(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                          vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j],
                          sr,cr,s[j],c[j])*delta_1 +
                        CONV_INT_L0_L0_L1(rn[k],rn[j],rn[i],rf[k],rf[j],rf[i],
                          vn[k][j],vn[j][j],vn[i][j],vf[k][j],vf[j][j],vf[i][j],
                          sr,cr,s[j],c[j])*delta_2 +

                        CONV_INT_L0_L0_L1(rn[j],rn[i],rn[k],rf[j],rf[i],rf[k],
                          vn[j][k],vn[i][k],vn[k][k],vf[j][k],vf[i][k],vf[k][k],
                          sr,cr,s[k],c[k])*delta_0 +
                        CONV_INT_L0_L0_L0(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                          vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                          sr,cr,s[k],c[k])*delta_1 +
                        CONV_INT_L0_L0_L1(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                          vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                          sr,cr,s[k],c[k])*delta_2
                       )*ndetB;

      for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
      COEFFL(pfn,Z) += (CONV_INT_L0_L0_L1(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                          vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                          sr,cr,s[j],c[j])*delta_0 +
                        CONV_INT_L0_L0_L1(rn[k],rn[j],rn[i],rf[k],rf[j],rf[i],
                          vn[k][j],vn[j][j],vn[i][j],vf[k][j],vf[j][j],vf[i][j],
                          sr,cr,s[j],c[j])*delta_1 +
                        CONV_INT_L0_L0_L0(rn[k],rn[i],rn[j],rf[k],rf[i],rf[j],
                          vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                          sr,cr,s[j],c[j])*delta_2 +

                        CONV_INT_L0_L1_L2(rn[i],rn[j],rn[k],rf[i],rf[j],rf[k],
                          vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k],
                          sr,cr,s[k],c[k])*delta_0 +
                        CONV_INT_L0_L0_L1(rn[j],rn[k],rn[i],rf[j],rf[k],rf[i],
                          vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                          sr,cr,s[k],c[k])*delta_1 +
                        CONV_INT_L0_L0_L1(rn[k],rn[j],rn[i],rf[k],rf[j],rf[i],
                          vn[k][k],vn[j][k],vn[i][k],vf[k][k],vf[j][k],vf[i][k],
                          sr,cr,s[k],c[k])*delta_2
                       )*ndetB;
}

#else  /*  if !((N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && 
                (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && 
                (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && 
                (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES))   */

void conv_stab_ij_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i, j, k, Z; FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], ndetB;
{  eprintf("Error: conv_stab_ij_sn_sf not available.\n");  }

void conv_stab_ij_p1nc_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB,delta_0,delta_1,delta_2)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i, j, k, Z; FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], ndetB, delta_0, delta_1, delta_2;
{  eprintf("Error: conv_stab_ij_p1nc_sn_sf not available.\n");  }

void lapl_stab_ij_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,b,ndetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i, j, k, Z; FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], b[DIM2][DIM2], ndetB;
{  eprintf("Error: lapl_stab_ij_sn_sf not available.\n");  }

void lapl_stab_ij_p1nc_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,b,ndetB,delta_0,delta_1,delta_2)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i, j, k, Z; FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], b[DIM2][DIM2], ndetB, delta_0, delta_1, delta_2;
{  eprintf("Error: lapl_stab_ij_p1nc_sn_sf not available.\n");  }

void time_stab_ij_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i, j, k, Z; FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], ndetB;
{  eprintf("Error: time_stab_ij_sn_sf not available.\n");  }

void react_stab_ij_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,sr,cr,rn,rf,ndetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i, j, k, Z; FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], sr, cr, rn[SIDES], rf[SIDES], ndetB;
{  eprintf("Error: react_stab_ij_sn_sf not available.\n");  }

void react_stab_ij_p1nc_sn_sf(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,sr,cr,rn,rf,ndetB,delta_0,delta_1,delta_2)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i, j, k, Z; FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], sr, cr, rn[SIDES], rf[SIDES], ndetB, delta_0, delta_1, delta_2;
{  eprintf("Error: react_stab_ij_p1nc_sn_sf not available.\n");  }

#endif

/* integrates uxi.vxj on the reference element */
DOUBLE general_uxi_vxj_ref(i,j,u0,u1,v0,v1,ref_map,qr) 
INT i, j;
FLOAT (*u0)(), (*u1)(), (*v0)(), (*v1)();
REF_MAPPING *ref_map;
QUADRATURE_RULE qr;
{
   INT k;
   FLOAT b[DIM][DIM], s=0;

   for (k=0; k < qr.n; k++){
      inv_of_jac_matr_times_jacobian(qr.points[k],b,ref_map);
      s += qr.weights[k]*
           (u0(qr.points[k])*b[0][i] + u1(qr.points[k])*b[1][i])*
           (v0(qr.points[k])*b[0][j] + v1(qr.points[k])*b[1][j])/
           fabs(jacobian(qr.points[k],ref_map));
   }
   return(s*QR_VOL);
}

/* integrates grad(u).grad(v) on the reference element */
FLOAT general_gu_gv_ref(tGrid,pel,q,q0,q1,q00,q01,q11,r,r0,r1,r00,r01,r11,
     nu,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*q00)(), (*q01)(), (*q11)(),
       (*r)(), (*r0)(), (*r1)(), (*r00)(), (*r01)(), (*r11)(),
       nu, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), 
       (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT i0, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   return(nu*(general_uxi_vxj_ref(0,0,q0,q1,r0,r1,&ref_map,qr) + 
              general_uxi_vxj_ref(1,1,q0,q1,r0,r1,&ref_map,qr)) );
}

/* integrates sc_tau*grad(u).grad(v) on the reference element */
FLOAT general_sctau_gu_gv_ref(tGrid,pel,q,q0,q1,q00,q01,q11,r,r0,r1,r00,r01,r11,
      eps,par1,par2,a3,a4,u,space,sc_type,bb0,bb1,react,rhs,f4,f5,f6,f7,f8,f9,
      finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*q00)(), (*q01)(), (*q11)(),
       (*r)(), (*r0)(), (*r1)(), (*r00)(), (*r01)(), (*r11)(),
       eps, par1, par2, a3, a4, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), 
       (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT u, space, sc_type;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT k;
   FLOAT b[DIM][DIM], u0, u1, v0, v1, s=0;

   if(PW_CONST_PAR == YES){
      for (k=0; k < qr.n; k++){
         u0 = q0(qr.points[k]);
         u1 = q1(qr.points[k]);
         v0 = r0(qr.points[k]);
         v1 = r1(qr.points[k]);
         inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
         s += qr.weights[k]*
              ((u0*b[0][0] + u1*b[1][0])*(v0*b[0][0] + v1*b[1][0]) +
               (u0*b[0][1] + u1*b[1][1])*(v0*b[0][1] + v1*b[1][1]))/
              fabs(jacobian(qr.points[k],&ref_map));
      }
      s *= gsc_tau_k(qr.points[0],tGrid,pel,eps,bb0,bb1,react,rhs,u,space,
                     sc_type,par1,par2,finite_el,ref_map);
   }
   else
      for (k=0; k < qr.n; k++){
         u0 = q0(qr.points[k]);
         u1 = q1(qr.points[k]);
         v0 = r0(qr.points[k]);
         v1 = r1(qr.points[k]);
         inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
         s += gsc_tau_k(qr.points[k],tGrid,pel,eps,bb0,bb1,react,rhs,u,space,
                        sc_type,par1,par2,finite_el,ref_map)*
              qr.weights[k]*
              ((u0*b[0][0] + u1*b[1][0])*(v0*b[0][0] + v1*b[1][0]) +
               (u0*b[0][1] + u1*b[1][1])*(v0*b[0][1] + v1*b[1][1]))/
              fabs(jacobian(qr.points[k],&ref_map));
      }
   return(s*QR_VOL);
}

FLOAT general_orth_sc_ref(tGrid,pel,q,q0,q1,q00,q01,q11,r,r0,r1,r00,r01,r11,
      eps,par1,par2,a3,a4,u,space,sc_type,bb0,bb1,react,rhs,f4,f5,f6,f7,f8,f9,
      finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*q00)(), (*q01)(), (*q11)(),
       (*r)(), (*r0)(), (*r1)(), (*r00)(), (*r01)(), (*r11)(),
       eps, par1, par2, a3, a4, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), 
       (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT u, space, sc_type;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT k;
   FLOAT b0x, b1x, b[DIM][DIM], y[DIM], u0, u1, v0, v1, s=0,
         jac0=jacobian(qr.points[0],&ref_map);

   if(PW_CONST_PAR == YES){
      for (k=0; k < qr.n; k++){
         inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
         ref_map_point(qr.points[k],y,&ref_map);
         b0x = -bb1(y);
         b1x =  bb0(y);
         u0 = q0(qr.points[k]);
         u1 = q1(qr.points[k]);
         v0 = r0(qr.points[k]);
         v1 = r1(qr.points[k]);
         s += qr.weights[k]*
              (b0x*(v0*b[0][0] + v1*b[1][0]) + b1x*(v0*b[0][1] + v1*b[1][1]))
             *(b0x*(u0*b[0][0] + u1*b[1][0]) + b1x*(u0*b[0][1] + u1*b[1][1]))
              /jacobian(qr.points[k],&ref_map);
      }
      s *= gsc_tau_k(qr.points[0],tGrid,pel,eps,bb0,bb1,react,rhs,u,space,
                     sc_type,par1,par2,finite_el,ref_map);
   }
   else
      for (k=0; k < qr.n; k++){
         inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
         ref_map_point(qr.points[k],y,&ref_map);
         b0x = -bb1(y);
         b1x =  bb0(y);
         u0 = q0(qr.points[k]);
         u1 = q1(qr.points[k]);
         v0 = r0(qr.points[k]);
         v1 = r1(qr.points[k]);
         s += gsc_tau_k(qr.points[k],tGrid,pel,eps,bb0,bb1,react,rhs,u,space,
                        sc_type,par1,par2,finite_el,ref_map)*
               qr.weights[k]*
              (b0x*(v0*b[0][0] + v1*b[1][0]) + b1x*(v0*b[0][1] + v1*b[1][1]))
             *(b0x*(u0*b[0][0] + u1*b[1][0]) + b1x*(u0*b[0][1] + u1*b[1][1]))
              /jacobian(qr.points[k],&ref_map);
      }
   return(s*QR_VOL*SGN(jac0));
}

/* integrates q*(b0,b1)*grad r on the reference element */
FLOAT general_b_grad_u_v_ref(tGrid,pel,q,q0,q1,q00,q01,q11,r,r0,r1,r00,r01,r11,
     a0,a1,a2,a3,a4,i0,i1,i2,b0,b1,f2,f3,f4,f5,f6,f7,f8,f9,finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*q00)(), (*q01)(), (*q11)(),
       (*r)(), (*r0)(), (*r1)(), (*r00)(), (*r01)(), (*r11)(),
       a0, a1, a2, a3, a4, (*b0)(), (*b1)(), (*f2)(), (*f3)(), 
       (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT i0, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT k;
   FLOAT b0x, b1x, b[DIM][DIM], s=0, jac0=jacobian(qr.points[0],&ref_map);

   for (k=0; k < qr.n; k++){
      inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
      b0x = fcn_ref_map_value(qr.points[k],&ref_map,b0);
      b1x = fcn_ref_map_value(qr.points[k],&ref_map,b1);
      s += qr.weights[k]*q(qr.points[k])* 
           (b0x*(r0(qr.points[k])*b[0][0] + r1(qr.points[k])*b[1][0]) +
            b1x*(r0(qr.points[k])*b[0][1] + r1(qr.points[k])*b[1][1]));
   }
   return(s*QR_VOL*SGN(jac0));
}

/* integrates d*q*r on the reference element */
FLOAT general_d_u_v_ref(tGrid,pel,q,q0,q1,q00,q01,q11,r,r0,r1,r00,r01,r11,
      a0,a1,a2,a3,a4,i0,i1,i2,d,f1,f2,f3,f4,f5,f6,f7,f8,f9,finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*q00)(), (*q01)(), (*q11)(),
       (*r)(), (*r0)(), (*r1)(), (*r00)(), (*r01)(), (*r11)(),
       a0, a1, a2, a3, a4, (*d)(), (*f1)(), (*f2)(), (*f3)(), 
       (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT i0, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT k;
   FLOAT dx, s=0;

   for (k=0; k < qr.n; k++){
      dx = fcn_ref_map_value(qr.points[k],&ref_map,d);
      s += qr.weights[k]*q(qr.points[k])*r(qr.points[k])*dx
           *fabs(jacobian(qr.points[k],&ref_map));
   }
   return(s*QR_VOL);
}

DOUBLE general_rhs_ref(tGrid,pel,v,v0,v1,rhs,a0,a1,a2,a3,a4,i0,i1,i2,
                       f0,f1,f2,f3,f4,finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
FLOAT (*v)(), (*v0)(), (*v1)(), (*rhs)(), a0, a1, a2, a3, a4, 
      (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)();
INT i0, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT k;
   FLOAT s=0;

   for (k=0; k < qr.n; k++)
      s += qr.weights[k]*fcn_ref_map_value(qr.points[k],&ref_map,rhs)
           *v(qr.points[k])*fabs(jacobian(qr.points[k],&ref_map));
   return(s*QR_VOL);
}

FLOAT general_sd_ref(tGrid,pel,q,q0,q1,q00,q01,q11,r,r0,r1,r00,r01,r11,nu,
  a1,a2,a3,a4,space,i1,i2,b0,b1,react,f3,f4,f5,f6,f7,f8,f9,finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*q00)(), (*q01)(), (*q11)(),
       (*r)(), (*r0)(), (*r1)(), (*r00)(), (*r01)(), (*r11)(),
       nu, a1, a2, a3, a4, (*b0)(), (*b1)(), (*react)(), (*f3)(), 
       (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT space, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT k;
   FLOAT b0x, b1x, b[DIM][DIM], y[DIM], s=0, 
         jac0=jacobian(qr.points[0],&ref_map);

   if(PW_CONST_PAR == YES){
      for (k=0; k < qr.n; k++){
         inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
         ref_map_point(qr.points[k],y,&ref_map);
         b0x = b0(y);
         b1x = b1(y);
         s += qr.weights[k]*(react(y)*r(qr.points[k]) +
              (b0x*(r0(qr.points[k])*b[0][0] + r1(qr.points[k])*b[1][0]) +
               b1x*(r0(qr.points[k])*b[0][1] + r1(qr.points[k])*b[1][1]))/ 
               jacobian(qr.points[k],&ref_map) )
             *(b0x*(q0(qr.points[k])*b[0][0] + q1(qr.points[k])*b[1][0]) +
               b1x*(q0(qr.points[k])*b[0][1] + q1(qr.points[k])*b[1][1]));
      }
      s *= gsd_tau(qr.points[0],pel,nu,b0,b1,space);
   }
   else
      for (k=0; k < qr.n; k++){
         inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
         ref_map_point(qr.points[k],y,&ref_map);
         b0x = b0(y);
         b1x = b1(y);
         s += qr.weights[k]*gsd_tau(y,pel,nu,b0,b1,space)*(
               react(y)*r(qr.points[k]) +
              (b0x*(r0(qr.points[k])*b[0][0] + r1(qr.points[k])*b[1][0]) +
               b1x*(r0(qr.points[k])*b[0][1] + r1(qr.points[k])*b[1][1]))/ 
               jacobian(qr.points[k],&ref_map) )
             *(b0x*(q0(qr.points[k])*b[0][0] + q1(qr.points[k])*b[1][0]) +
               b1x*(q0(qr.points[k])*b[0][1] + q1(qr.points[k])*b[1][1]));
      }
   return(s*QR_VOL*SGN(jac0));
}

DOUBLE general_sd_rhs_ref(tGrid,pel,v,v0,v1,rhs,nu,a1,a2,a3,a4,space,i1,i2,
                          b0,b1,f2,f3,f4,finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
FLOAT (*v)(), (*v0)(), (*v1)(), (*rhs)(), nu, a1, a2, a3, a4, 
      (*b0)(), (*b1)(), (*f2)(), (*f3)(), (*f4)();
INT space, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT k;
   FLOAT b[DIM][DIM], y[DIM], s=0, jac0=jacobian(qr.points[0],&ref_map);

   if(PW_CONST_PAR == YES){
      for (k=0; k < qr.n; k++){
         inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
         ref_map_point(qr.points[k],y,&ref_map);
         s += qr.weights[k]*rhs(y)*(
                b0(y)*(v0(qr.points[k])*b[0][0] + v1(qr.points[k])*b[1][0]) 
              + b1(y)*(v0(qr.points[k])*b[0][1] + v1(qr.points[k])*b[1][1]));
      }
      s *= gsd_tau(qr.points[0],pel,nu,b0,b1,space);
   }
   else
      for (k=0; k < qr.n; k++){
         inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
         ref_map_point(qr.points[k],y,&ref_map);
         s += qr.weights[k]*gsd_tau(y,pel,nu,b0,b1,space)*rhs(y)*(
                b0(y)*(v0(qr.points[k])*b[0][0] + v1(qr.points[k])*b[1][0]) 
              + b1(y)*(v0(qr.points[k])*b[0][1] + v1(qr.points[k])*b[1][1]));
      }
   return(s*QR_VOL*SGN(jac0));
}

DOUBLE general_layton_polman_penalty_ref(tGrid,pel,v,v0,v1,rhs,beta,a1,a2,a3,a4,
                                 u,space,i2,f0,f1,f2,f3,f4,finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
FLOAT (*v)(), (*v0)(), (*v1)(), (*rhs)(), beta, a1, a2, a3, a4,
      (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)();
INT u, space, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT k;
   FLOAT d0, d1, s=0, z, min=0., max=1.;

   for (k=0; k < qr.n; k++){
      if (space == GP1C || space == GP1X3C || space == GP2C || 
          space == GP2X3C || space == GP3C || space == GP4C || 
          space == GP1C_ELBUB || space == GP2C_3ELBUB || space == GP2C_6ELBUB ||
          space == GQ1C || 
          space == GQ1X4C || space == GQ1C_ELBUB || space == GQ2C || 
          space == GQ2X4C || space == GQ2C_2ELBUB || space == GQ2C_3ELBUB || 
          space == GQ3C || space == GQ4C)
         z = gfunc_value_ref(tGrid,pel,u,qr.points[k],finite_el);
      else
         z = sfunc_value_ref(pel,qr.points[k],u,space);
      d0 = z - min;
      d1 = z - max;
      z = MIN(d0,0) + MAX(d1,0);
      s += qr.weights[k]*v(qr.points[k])*z
           *fabs(jacobian(qr.points[k],&ref_map));
   }
   return(-s*QR_VOL/(beta*diameter(pel)));
}

/* integrates local projection term (kappa grad r)*(kappa grad q) on the 
   reference element */
FLOAT general_local_projection_ref(tGrid,pel,q,q0,q1,q00,q01,q11,
                                             r,r0,r1,r00,r01,r11,
   eps,a1,a2,a3,a4,n,i1,i2,bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*q00)(), (*q01)(), (*q11)(),
       (*r)(), (*r0)(), (*r1)(), (*r00)(), (*r01)(), (*r11)(),
       eps, a1, a2, a3, a4, (*bb0)(), (*bb1)(), (*f2)(), (*f3)(), 
       (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT n, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT i, j, k;
   DOUBLE_FUNC f[8] = { f2, f3, f4, f5, f6, f7, f8, f9 };
   DOUBLE a[8][N_SGGEM], cq[2][N_SGGEM], cr[2][8], x[2][N_SGGEM], b[DIM][DIM],
          fi, q0i, q1i, r0i, r1i, s;

   if (n > 8)
      eprintf("Error: too large n in general_local_projection_ref.\n");
   for (i=0; i < n; i++){
      cq[0][i] = cq[1][i] = cr[0][i] = cr[1][i] = 0.;
      for (j=0; j < n; j++)
         a[i][j] = 0.;
   }
   for (k=0; k < qr.n; k++){
      inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
      q0i = q0(qr.points[k])*b[0][0] + q1(qr.points[k])*b[1][0];
      q1i = q0(qr.points[k])*b[0][1] + q1(qr.points[k])*b[1][1];
      r0i = r0(qr.points[k])*b[0][0] + r1(qr.points[k])*b[1][0];
      r1i = r0(qr.points[k])*b[0][1] + r1(qr.points[k])*b[1][1];
      s = fabs(jacobian(qr.points[k],&ref_map));
      for (i=0; i < n; i++){
         fi = qr.weights[k]*f[i](qr.points[k]);
         cq[0][i] += fi*q0i;
         cq[1][i] += fi*q1i;
         cr[0][i] += fi*r0i;
         cr[1][i] += fi*r1i;
         for (j=0; j < n; j++)
            a[i][j] += fi*f[j](qr.points[k])*s;
      }
   }
   sggem(a,cq,x,n,2);
   s = 0.;
   for (i=0; i < n; i++)
      s += x[0][i]*cr[0][i] + x[1][i]*cr[1][i];
   s = general_gu_gv_ref(tGrid,pel,q,q0,q1,q00,q01,q11,r,r0,r1,r00,r01,r11,
                         1.,a1,a2,a3,a4,0,i1,i2,bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,
                         finite_el,ref_map,qr) - s*QR_VOL;
   return(lp_delta(pel,eps,bb0,bb1)*s);
}

/* integrates local projection term (kappa b*grad r)*(kappa b*grad q) on the 
   reference element */
FLOAT general_conv_local_proj_ref(tGrid,pel,q,q0,q1,q00,q01,q11,
                                            r,r0,r1,r00,r01,r11,
   eps,a1,a2,a3,a4,n,i1,i2,bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*q00)(), (*q01)(), (*q11)(),
       (*r)(), (*r0)(), (*r1)(), (*r00)(), (*r01)(), (*r11)(),
       eps, a1, a2, a3, a4, (*bb0)(), (*bb1)(), (*f2)(), (*f3)(), 
       (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT n, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT i, j, k;
   DOUBLE_FUNC f[8] = { f2, f3, f4, f5, f6, f7, f8, f9 };
   DOUBLE a[8][N_GGEM], cq[8], cr[8], x[8], b[DIM][DIM], xc[DIM],
          b0x, b1x, fi, qk, rk, s, z=0.;

   if (n > 8)
      eprintf("Error: too large n in general_local_projection_ref.\n");
   for (i=0; i < n; i++){
      cq[i] = cr[i] = 0.;
      for (j=0; j < n; j++)
         a[i][j] = 0.;
   }
   if (USE_BM == YES){
      coord_of_barycentre(pel,xc);
      b0x = bb0(xc);
      b1x = bb1(xc);
   }
   for (k=0; k < qr.n; k++){
      inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
      if (USE_BM == NO){
         b0x = fcn_ref_map_value(qr.points[k],&ref_map,bb0);
         b1x = fcn_ref_map_value(qr.points[k],&ref_map,bb1);
      }
      qk = b0x*(q0(qr.points[k])*b[0][0] + q1(qr.points[k])*b[1][0]) +
           b1x*(q0(qr.points[k])*b[0][1] + q1(qr.points[k])*b[1][1]);
      rk = b0x*(r0(qr.points[k])*b[0][0] + r1(qr.points[k])*b[1][0]) +
           b1x*(r0(qr.points[k])*b[0][1] + r1(qr.points[k])*b[1][1]);
      s = fabs(jacobian(qr.points[k],&ref_map));
      z += qr.weights[k]*qk*rk/s;
      for (i=0; i < n; i++){
         fi = qr.weights[k]*f[i](qr.points[k]);
         cq[i] += fi*qk;
         cr[i] += fi*rk;
         for (j=0; j < n; j++)
            a[i][j] += fi*f[j](qr.points[k])*s;
      }
   }
   ggem(a,cq,x,n);
   for (i=0; i < n; i++)
      z -= x[i]*cr[i];
   return(lp_delta(pel,eps,bb0,bb1)*z*QR_VOL);
}

DOUBLE pw(x)
DOUBLE *x;
{
   return(1.);
}

/* integrates local projection term (kappa b*grad r)*(kappa b*grad q) on the 
   reference element */
FLOAT general_weighted_conv_local_proj_ref(tGrid,pel,q,q0,q1,q00,q01,q11,
                                            r,r0,r1,r00,r01,r11,
   eps,a1,a2,a3,a4,n,i1,i2,bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*q00)(), (*q01)(), (*q11)(),
       (*r)(), (*r0)(), (*r1)(), (*r00)(), (*r01)(), (*r11)(),
       eps, a1, a2, a3, a4, (*bb0)(), (*bb1)(), (*f2)(), (*f3)(), 
       (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT n, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT i, j, k;
   DOUBLE_FUNC f[8] = { f2, f3, f4, f5, f6, f7, f8, f9 };
   DOUBLE a[8][8], wa[8][N_GGEM], cq[8], cr[8], wc[2][N_GGEM], x[2][N_GGEM],
          b[DIM][DIM], xc[DIM], b0x, b1x, pwx, fi, pfi, qk, rk, s, z=0.;

   if (n > 8)
      eprintf("Error: too large n in general_local_projection_ref.\n");
   for (i=0; i < n; i++){
      cq[i] = cr[i] = wc[0][i] = wc[1][i] = 0.;
      for (j=0; j < n; j++)
         a[i][j] = wa[i][j] = 0.;
   }
   if (USE_BM == YES){
      coord_of_barycentre(pel,xc);
      b0x = bb0(xc);
      b1x = bb1(xc);
   }
   for (k=0; k < qr.n; k++){
      inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
      if (USE_BM == NO){
         b0x = fcn_ref_map_value(qr.points[k],&ref_map,bb0);
         b1x = fcn_ref_map_value(qr.points[k],&ref_map,bb1);
      }
      pwx = pw(qr.points[k]);
      qk = b0x*(q0(qr.points[k])*b[0][0] + q1(qr.points[k])*b[1][0]) +
           b1x*(q0(qr.points[k])*b[0][1] + q1(qr.points[k])*b[1][1]);
      rk = b0x*(r0(qr.points[k])*b[0][0] + r1(qr.points[k])*b[1][0]) +
           b1x*(r0(qr.points[k])*b[0][1] + r1(qr.points[k])*b[1][1]);
      s = fabs(jacobian(qr.points[k],&ref_map));
      z += qr.weights[k]*qk*rk/s;
      for (i=0; i < n; i++){
         fi = qr.weights[k]*f[i](qr.points[k]);
         pfi = fi*pwx;
         cq[i] += fi*qk;
         cr[i] += fi*rk;
         wc[0][i] += pfi*qk;
         wc[1][i] += pfi*rk;
         for (j=0; j < n; j++){
            a[ i][j] +=  fi*f[j](qr.points[k])*s;
            wa[i][j] += pfi*f[j](qr.points[k])*s;
         }
      }
   }
   sggem(wa,wc,x,n,2);
   for (i=0; i < n; i++){
      z -= x[0][i]*cr[i] + x[1][i]*cq[i];
      for (j=0; j < n; j++)
         z += a[i][j]*x[0][i]*x[1][j];
   }
   return(lp_delta(pel,eps,bb0,bb1)*z*QR_VOL);
}

/* integrates local projection term -(pi b*grad r)*(pi b*grad q) on the 
   reference element */
FLOAT general_pure_conv_local_proj_ref(tGrid,pel,q,q0,q1,q00,q01,q11,
                                                 r,r0,r1,r00,r01,r11,
   eps,a1,a2,a3,a4,n,i1,i2,bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*q00)(), (*q01)(), (*q11)(),
       (*r)(), (*r0)(), (*r1)(), (*r00)(), (*r01)(), (*r11)(),
       eps, a1, a2, a3, a4, (*bb0)(), (*bb1)(), (*f2)(), (*f3)(), 
       (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT n, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT i, j, k;
   DOUBLE_FUNC f[8] = { f2, f3, f4, f5, f6, f7, f8, f9 };
   DOUBLE a[8][N_GGEM], cq[8], cr[8], x[8], b[DIM][DIM], xc[DIM],
          b0x, b1x, fi, qk, rk, s, z=0.;

   if (n > 8)
      eprintf("Error: too large n in general_local_projection_ref.\n");
   for (i=0; i < n; i++){
      cq[i] = cr[i] = 0.;
      for (j=0; j < n; j++)
         a[i][j] = 0.;
   }
   if (USE_BM == YES){
      coord_of_barycentre(pel,xc);
      b0x = bb0(xc);
      b1x = bb1(xc);
   }
   for (k=0; k < qr.n; k++){
      inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
      if (USE_BM == NO){
         b0x = fcn_ref_map_value(qr.points[k],&ref_map,bb0);
         b1x = fcn_ref_map_value(qr.points[k],&ref_map,bb1);
      }
      qk = b0x*(q0(qr.points[k])*b[0][0] + q1(qr.points[k])*b[1][0]) +
           b1x*(q0(qr.points[k])*b[0][1] + q1(qr.points[k])*b[1][1]);
      rk = b0x*(r0(qr.points[k])*b[0][0] + r1(qr.points[k])*b[1][0]) +
           b1x*(r0(qr.points[k])*b[0][1] + r1(qr.points[k])*b[1][1]);
      s = fabs(jacobian(qr.points[k],&ref_map));
      for (i=0; i < n; i++){
         fi = qr.weights[k]*f[i](qr.points[k]);
         cq[i] += fi*qk;
         cr[i] += fi*rk;
         for (j=0; j < n; j++)
            a[i][j] += fi*f[j](qr.points[k])*s;
      }
   }
   ggem(a,cq,x,n);
   for (i=0; i < n; i++)
      z += x[i]*cr[i];
   return(-lp_delta(pel,eps,bb0,bb1)*z*QR_VOL);
}

/* integrates (b*grad r)*(b*grad q) on the reference element */
FLOAT general_conv_stab_ref(tGrid,pel,q,q0,q1,q00,q01,q11,
                                      r,r0,r1,r00,r01,r11,
         eps,a1,a2,a3,a4,i0,i1,i2,bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,
                                               finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*q00)(), (*q01)(), (*q11)(),
       (*r)(), (*r0)(), (*r1)(), (*r00)(), (*r01)(), (*r11)(),
       eps, a1, a2, a3, a4, (*bb0)(), (*bb1)(), (*f2)(), (*f3)(), 
       (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT i0, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT k;
   FLOAT b[DIM][DIM], xc[DIM], b0x, b1x, s=0;

   coord_of_barycentre(pel,xc);
   b0x = bb0(xc);
   b1x = bb1(xc);
   for (k=0; k < qr.n; k++){
      inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
      s += qr.weights[k]*
           (b0x*(r0(qr.points[k])*b[0][0] + r1(qr.points[k])*b[1][0]) +
            b1x*(r0(qr.points[k])*b[0][1] + r1(qr.points[k])*b[1][1]))
          *(b0x*(q0(qr.points[k])*b[0][0] + q1(qr.points[k])*b[1][0]) +
            b1x*(q0(qr.points[k])*b[0][1] + q1(qr.points[k])*b[1][1]))/
            fabs(jacobian(qr.points[k],&ref_map));
   }
   return(lp_delta(pel,eps,bb0,bb1)*s*QR_VOL);
}

/* integrates (b*grad r)*(b*grad q) on the reference element */
FLOAT general_macro_conv_stab_ref(tGrid,pel,q,q0,q1,q00,q01,q11,
                                            r,r0,r1,r00,r01,r11,
                 eps,b0x,b1x,delta,a4,i0,i1,i2,bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,
                                                          finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*q00)(), (*q01)(), (*q11)(),
       (*r)(), (*r0)(), (*r1)(), (*r00)(), (*r01)(), (*r11)(),
       eps, b0x, b1x, delta, a4, (*bb0)(), (*bb1)(), (*f2)(), (*f3)(), 
       (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT i0, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT k;
   FLOAT b[DIM][DIM], s=0;

   for (k=0; k < qr.n; k++){
      inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
      s += qr.weights[k]*
           (b0x*(r0(qr.points[k])*b[0][0] + r1(qr.points[k])*b[1][0]) +
            b1x*(r0(qr.points[k])*b[0][1] + r1(qr.points[k])*b[1][1]))
          *(b0x*(q0(qr.points[k])*b[0][0] + q1(qr.points[k])*b[1][0]) +
            b1x*(q0(qr.points[k])*b[0][1] + q1(qr.points[k])*b[1][1]))/
            fabs(jacobian(qr.points[k],&ref_map));
   }
   return(delta*s*QR_VOL);
}

/* integrates rhs local projection term (kappa b*grad u)*(kappa b*grad q) 
   on the reference element */
FLOAT g_consist_err_conv_local_proj_ref(tGrid,pel,q,q0,q1,
u0,u1,eps,n,bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*u0)(), (*u1)(), eps, (*bb0)(), (*bb1)(), 
       (*f2)(), (*f3)(), (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT n;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT i, j, k;
   DOUBLE_FUNC f[8] = { f2, f3, f4, f5, f6, f7, f8, f9 };
   DOUBLE a[8][N_GGEM], cq[8], cr[8], x[8], b[DIM][DIM], xc[DIM], qx, qy, 
          b0x, b1x, bgu, fi, qk, s, z=0., y[DIM];

   coord_of_barycentre(pel,xc);
   if (n > 8)
      eprintf("Error: too large n in general_local_projection_ref.\n");
   for (i=0; i < n; i++){
      cq[i] = cr[i] = 0.;
      for (j=0; j < n; j++)
         a[i][j] = 0.;
   }
   if (USE_BM == YES){
      b0x = bb0(xc);
      b1x = bb1(xc);
   }
   for (k=0; k < qr.n; k++){
      inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
      if (USE_BM == NO){
         b0x = fcn_ref_map_value(qr.points[k],&ref_map,bb0);
         b1x = fcn_ref_map_value(qr.points[k],&ref_map,bb1);
      }
      V_BILIN_VALUE(y,ref_map.q1_a,ref_map.q1_c,ref_map.q1_alpha,
                    qr.points[k][0],qr.points[k][1]);
      bgu = b0x*fcn_ref_map_value(qr.points[k],&ref_map,u0) +
            b1x*fcn_ref_map_value(qr.points[k],&ref_map,u1);
//    bgu = b0x*u0(y) + b1x*u1(y);
      qx = y[0] - xc[0];
      qy = y[1] - xc[1];
//    bgu = 2.*y[1]*qx*qx + 4.*(xc[0]-0.5)*qx*qy;
      qk = b0x*(q0(qr.points[k])*b[0][0] + q1(qr.points[k])*b[1][0]) +
           b1x*(q0(qr.points[k])*b[0][1] + q1(qr.points[k])*b[1][1]);
      s = fabs(jacobian(qr.points[k],&ref_map));
      z += qr.weights[k]*qk*bgu;
      for (i=0; i < n; i++){
         fi = qr.weights[k]*f[i](qr.points[k]);
         cq[i] += fi*qk;
         fi *= s;
         cr[i] += fi*bgu;
         for (j=0; j < n; j++)
            a[i][j] += fi*f[j](qr.points[k]);
      }
   }
   ggem(a,cq,x,n);
   for (i=0; i < n; i++)
      z -= x[i]*cr[i];
   s = jacobian(qr.points[0],&ref_map);
   return(lp_delta(pel,eps,bb0,bb1)*z*QR_VOL*SGN(s));
}

/* integrates eps*grad(r).grad(q) + q*(bb0,bb1)*grad r + react*q*r +
   tau*(b*grad r)*(b*grad q) on the reference element */
FLOAT general_conv_diff_react_stab_ref(tGrid,pel,q,q0,q1,q00,q01,q11,
                                                 r,r0,r1,r00,r01,r11,
         eps,a1,a2,a3,a4,i0,i1,i2,bb0,bb1,react,f3,f4,f5,f6,f7,f8,f9,
                                                 finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE (*q)(), (*q0)(), (*q1)(), (*q00)(), (*q01)(), (*q11)(),
       (*r)(), (*r0)(), (*r1)(), (*r00)(), (*r01)(), (*r11)(),
       eps, a1, a2, a3, a4, (*bb0)(), (*bb1)(), (*react)(), (*f3)(), 
       (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
INT i0, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   return(general_gu_gv_ref(tGrid,pel,q,q0,q1,q00,q01,q11,
                                      r,r0,r1,r00,r01,r11,
            eps,1.,2.,3.,4.,0,1,2,NULL,NULL,NULL,f3,f4,f5,f6,f7,f8,f9,
            finite_el,ref_map,qr)
        + general_b_grad_u_v_ref(tGrid,pel,q,q0,q1,q00,q01,q11,
                                           r,r0,r1,r00,r01,r11,
            0.,1.,2.,3.,4.,0,1,2,bb0,bb1,NULL,f3,f4,f5,f6,f7,f8,f9,
            finite_el,ref_map,qr)
        + general_d_u_v_ref(tGrid,pel,q,q0,q1,q00,q01,q11,
                                      r,r0,r1,r00,r01,r11,
            0.,1.,2.,3.,4.,0,1,2,react,NULL,NULL,f3,f4,f5,f6,f7,f8,f9,
            finite_el,ref_map,qr)
        + general_conv_stab_ref(tGrid,pel,q,q0,q1,q00,q01,q11,
                                          r,r0,r1,r00,r01,r11,
            eps,1.,2.,3.,4.,0,1,2,bb0,bb1,NULL,f3,f4,f5,f6,f7,f8,f9,
            finite_el,ref_map,qr));
}

void solve_3x3_symm_system(a00,a01,a02,a11,a12,a22,r0,r1,r2,p0,p1,p2,q)
DOUBLE a00, a01, a02, a11, a12, a22, r0, r1, r2, *p0, *p1, *p2, q;
{
   a11 = a00*a11 - a01*a01;
   a12 = a00*a12 - a01*a02;
   a22 = a00*a22 - a02*a02;
   r1 = a00*r1 - a01*r0;
   r2 = a00*r2 - a02*r0;
   *p0 = a11*a22 - a12*a12;
   *p1 = q*(a22*r1 - a12*r2)/(*p0);
   *p2 = q*(a11*r2 - a12*r1)/(*p0);
   *p0 = (q*r0 - a01*(*p1) - a02*(*p2))/a00;
}

#if DATA_S & N_LINK_TO_ELEMENTS

void subtract_local_proj_p2_p1(tGrid,eps,bb0,bb1,u,f)
GRID *tGrid;
DOUBLE eps, (*bb0)(), (*bb1)();
INT u, f;
{
   NELINK *pnel, *pnel2;
   ELEMENT *pel;
   NODE *pn;
   DOUBLE bx[DIM], nn[DIM], un[3], uf[3], sk, sj, q, z,
          p0, p1, p2, r0, r1, r2, a00, a01, a02, a11, a12, a22, 
          *x0, *x1, *x2, x01[DIM], x02[DIM], x12[DIM];
   INT j, k, l;

   for (pn = FIRSTNODE(tGrid); pn; pn = pn->succ)
   if (is_macro_node(pn))
   {
      bx[0] = bb0(pn->myvertex->x);
      bx[1] = bb1(pn->myvertex->x);
      a00 = a01 = a02 = a11 = a12 = a22 = r0 = r1 = r2 = 0.;
      for (pnel = NESTART(pn); pnel; pnel = pnel->next){
         pel = pnel->nbel;
         VERTICES_OF_ELEMENT(x0,x1,x2,pel);
         AVERAGE(x0,x1,x01);
         AVERAGE(x0,x2,x02);
         AVERAGE(x1,x2,x12);
         z = VOLUME(pel);
         a22 += z;
         z /= 3.;
         a00 += z*(x01[0]*x01[0] + x02[0]*x02[0] + x12[0]*x12[0]);
         a01 += z*(x01[0]*x01[1] + x02[0]*x02[1] + x12[0]*x12[1]);
         a02 += z*(x0[0] + x1[0] + x2[0]);
         a11 += z*(x01[1]*x01[1] + x02[1]*x02[1] + x12[1]*x12[1]);
         a12 += z*(x0[1] + x1[1] + x2[1]);
         un[0] = sn_node_value(pel->n[0],u,U_SPACE);
         un[1] = sn_node_value(pel->n[1],u,U_SPACE);
         un[2] = sn_node_value(pel->n[2],u,U_SPACE);
         uf[0] = sf_face_value(pel->f[0],u,U_SPACE);
         uf[1] = sf_face_value(pel->f[1],u,U_SPACE);
         uf[2] = sf_face_value(pel->f[2],u,U_SPACE);
         z *= un[0]+un[1]+un[2] + (uf[0]+uf[1]+uf[2])*0.25;
         r0 -= z*bx[0];
         r1 -= z*bx[1];
         for (l = 0; pel->n[l] != pn; l++);
         j = (l+1)%3;
         k = (l+2)%3;
         outer_nonunit_normal_vector_to_element_edge(pel->n[j],pel->n[k],pn,nn);
         q = DOT(bx,nn)/6.;
         z = un[j] + 0.5*uf[l] + un[k];
         sj = z + un[j];
         sk = z + un[k];
         r0 += (sj*pel->n[j]->myvertex->x[0] + sk*pel->n[k]->myvertex->x[0])*q;
         r1 += (sj*pel->n[j]->myvertex->x[1] + sk*pel->n[k]->myvertex->x[1])*q;
         r2 += (sj + sk)*q;
      }
      solve_3x3_symm_system(a00,a01,a02,a11,a12,a22,r0,r1,r2,&p0,&p1,&p2,
                            lpm_delta(pn,eps,bb0,bb1));
      for (pnel = NESTART(pn); pnel; pnel = pnel->next){
         pel = pnel->nbel;
         q = (p0*bx[0]+p1*bx[1])*VOLUME(pel)/3.;
         add_to_sn_node_value(pel->n[0],f,q,U_SPACE);
         add_to_sn_node_value(pel->n[1],f,q,U_SPACE);
         add_to_sn_node_value(pel->n[2],f,q,U_SPACE);
         q *= 0.25;
         add_to_sf_face_value(pel->f[0],f,q,U_SPACE);
         add_to_sf_face_value(pel->f[1],f,q,U_SPACE);
         add_to_sf_face_value(pel->f[2],f,q,U_SPACE);
         for (l = 0; pel->n[l] != pn; l++);
         j = (l+1)%3;
         k = (l+2)%3;
         outer_nonunit_normal_vector_to_element_edge(pel->n[j],pel->n[k],pn,nn);
         q = DOT(bx,nn)/6.;
         sj = p0*pel->n[j]->myvertex->x[0] + p1*pel->n[j]->myvertex->x[1] + p2;
         sk = p0*pel->n[k]->myvertex->x[0] + p1*pel->n[k]->myvertex->x[1] + p2;
         z = sj + sk;
         add_to_sn_node_value(pel->n[j],f,-(sj+z)*q,U_SPACE);
         add_to_sn_node_value(pel->n[k],f,-(sk+z)*q,U_SPACE);
         add_to_sf_face_value(pel->f[l],f, -0.5*z*q,U_SPACE);
      }
   }
}

void subtract_local_proj_q1_q0(tGrid,eps,bb0,bb1,u,f)
GRID *tGrid;
DOUBLE eps, (*bb0)(), (*bb1)();
INT u, f;
{
   NELINK *pnel;
   ELEMENT *pel;
   NODE *pn;
   DOUBLE bx[DIM], nn[DIM], ar, s, z;
   INT j, k, l;

   for (pn = FIRSTNODE(tGrid); pn; pn = pn->succ)
   if (is_macro_node(pn))
   {
      bx[0] = bb0(pn->myvertex->x);
      bx[1] = bb1(pn->myvertex->x);
      s = ar = 0.;
      for (pnel = NESTART(pn); pnel; pnel = pnel->next){
         pel = pnel->nbel;
         ar += VOLUME(pel);
         for (l = 0; pel->n[l] != pn; l++);
         for (j = 0; j < NVERT; j++)
            if (j != l && (k=(j+1)%NVERT) != l){
               outer_nonunit_normal_vector_to_element_edge(pel->n[j],
                                                           pel->n[k],pn,nn);
               s += ( sn_node_value(pel->n[j],u,U_SPACE) 
                    + sn_node_value(pel->n[k],u,U_SPACE) )*DOT(bx,nn);
            }
      }
      s *= 0.25/ar*lpm_delta(pn,eps,bb0,bb1);
      for (pnel = NESTART(pn); pnel; pnel = pnel->next){
         pel = pnel->nbel;
         for (l = 0; pel->n[l] != pn; l++);
         for (j = 0; j < NVERT; j++)
            if (j != l && (k=(j+1)%NVERT) != l){
               outer_nonunit_normal_vector_to_element_edge(pel->n[j],
                                                           pel->n[k],pn,nn);
               z = -s*DOT(bx,nn);
               add_to_sn_node_value(pel->n[j],f,z,U_SPACE);
               add_to_sn_node_value(pel->n[k],f,z,U_SPACE);
            }
      }
   }
}

/* ONLY FOR RECTANGLES!!! */
void add_to_p1_macro_mass_matr_for_rectangles(pel,a00,a01,a02,a11,a12,a22,vol)
ELEMENT *pel;
DOUBLE *a00, *a01, *a02, *a11, *a12, *a22, *vol;
{
   DOUBLE z, *x0, *x1, *x2, *x3, x01[DIM], x12[DIM], x23[DIM], x30[DIM], xc[DIM];

   VERTICES_OF_4ELEMENT(x0,x1,x2,x3,pel);
   AVERAGE(x0,x1,x01);
   AVERAGE(x1,x2,x12);
   AVERAGE(x2,x3,x23);
   AVERAGE(x3,x0,x30);
   POINT4(x01,x12,x23,x30,xc);
   z = VOLUME(pel);
   *a00 += z*(x0[0]*x0[0] + x1[0]*x1[0] + x2[0]*x2[0] + x3[0]*x3[0] 
           + 4.*(x01[0]*x01[0] + x12[0]*x12[0] + x23[0]*x23[0] + x30[0]*x30[0])
           + 16.*xc[0]*xc[0])/36.;
   *a01 += z*(x0[0]*x0[1] + x1[0]*x1[1] + x2[0]*x2[1] + x3[0]*x3[1] 
           + 4.*(x01[0]*x01[1] + x12[0]*x12[1] + x23[0]*x23[1] + x30[0]*x30[1])
           + 16.*xc[0]*xc[1])/36.;
   *a02 += z*xc[0];
   *a11 += z*(x0[1]*x0[1] + x1[1]*x1[1] + x2[1]*x2[1] + x3[1]*x3[1] 
           + 4.*(x01[1]*x01[1] + x12[1]*x12[1] + x23[1]*x23[1] + x30[1]*x30[1])
           + 16.*xc[1]*xc[1])/36.;
   *a12 += z*xc[1];
   *a22 += z;
   *vol = z;
}

/* ONLY FOR RECTANGLES!!! */
void subtract_local_proj_q2_p1_for_one_node(pn,eps,bb0,bb1,u,f)
NODE *pn;
DOUBLE eps, (*bb0)(), (*bb1)();
INT u, f;
{
   NELINK *pnel;
   ELEMENT *pel;
   DOUBLE bx[DIM], nn[DIM], un[4], uf[4], sk, sj, q, z, 
          p0, p1, p2, r0, r1, r2, a00, a01, a02, a11, a12, a22;
   INT j, k, l;

   bx[0] = bb0(pn->myvertex->x);
   bx[1] = bb1(pn->myvertex->x);
   a00 = a01 = a02 = a11 = a12 = a22 = r0 = r1 = r2 = 0.;
   for (pnel = NESTART(pn); pnel; pnel = pnel->next){
      pel = pnel->nbel;
      add_to_p1_macro_mass_matr_for_rectangles(pel,&a00,&a01,&a02,
                                                   &a11,&a12,&a22,&z);
      un[0] = sn_node_value(pel->n[0],u,U_SPACE);
      un[1] = sn_node_value(pel->n[1],u,U_SPACE);
      un[2] = sn_node_value(pel->n[2],u,U_SPACE);
      un[3] = sn_node_value(pel->n[3],u,U_SPACE);
      uf[0] = sf_face_value(pel->f[0],u,U_SPACE);
      uf[1] = sf_face_value(pel->f[1],u,U_SPACE);
      uf[2] = sf_face_value(pel->f[2],u,U_SPACE);
      uf[3] = sf_face_value(pel->f[3],u,U_SPACE);
      z *= (un[0]+un[1]+un[2]+un[3])*0.25 + (uf[0]+uf[1]+uf[2]+uf[3])/3.
           + se_element_value(pel,u,U_SPACE)*4/9.;
      r0 -= z*bx[0];
      r1 -= z*bx[1];
      for (l = 0; pel->n[l] != pn; l++);
      for (j = 0; j < NVERT; j++)
         if (j != l && (k=(j+1)%NVERT) != l){
            outer_nonunit_normal_vector_to_element_edge(pel->n[j],
                                                        pel->n[k],pn,nn);
            z = 2.*(un[j] + uf[j] + un[k]);
            sj = z - un[k];
            sk = z - un[j];
            q = DOT(bx,nn)/6.;
            r0 += (sj*pel->n[j]->myvertex->x[0] +
                   sk*pel->n[k]->myvertex->x[0])*q;
            r1 += (sj*pel->n[j]->myvertex->x[1] +
                   sk*pel->n[k]->myvertex->x[1])*q;
            r2 += (sj + sk)*q;
         }
   }
   solve_3x3_symm_system(a00,a01,a02,a11,a12,a22,r0,r1,r2,&p0,&p1,&p2,
                         lpm_delta(pn,eps,bb0,bb1));
   for (pnel = NESTART(pn); pnel; pnel = pnel->next){
      pel = pnel->nbel;
      q = (p0*bx[0]+p1*bx[1])*VOLUME(pel);
      r0 = q*0.25;
      r1 = q/3.;
      add_to_sn_node_value(pel->n[0],f,r0,U_SPACE);
      add_to_sn_node_value(pel->n[1],f,r0,U_SPACE);
      add_to_sn_node_value(pel->n[2],f,r0,U_SPACE);
      add_to_sn_node_value(pel->n[3],f,r0,U_SPACE);
      add_to_sf_face_value(pel->f[0],f,r1,U_SPACE);
      add_to_sf_face_value(pel->f[1],f,r1,U_SPACE);
      add_to_sf_face_value(pel->f[2],f,r1,U_SPACE);
      add_to_sf_face_value(pel->f[3],f,r1,U_SPACE);
      add_to_se_element_value(pel,f,q*4./9.,U_SPACE); 
      for (l = 0; pel->n[l] != pn; l++);
      for (j = 0; j < NVERT; j++)
         if (j != l && (k=(j+1)%NVERT) != l){
            outer_nonunit_normal_vector_to_element_edge(pel->n[j],
                                                        pel->n[k],pn,nn);
            q = DOT(bx,nn)/6.;
            sj = p0*pel->n[j]->myvertex->x[0] 
               + p1*pel->n[j]->myvertex->x[1] + p2;
            sk = p0*pel->n[k]->myvertex->x[0] 
               + p1*pel->n[k]->myvertex->x[1] + p2;
            z = 2.*(sj+sk);
            add_to_sn_node_value(pel->n[j],f,(sk - z)*q,U_SPACE);
            add_to_sn_node_value(pel->n[k],f,(sj - z)*q,U_SPACE);
            add_to_sf_face_value(pel->f[j],f,      -z*q,U_SPACE);
         }
   }
}

/* ONLY FOR RECTANGLES!!! */
void subtract_local_proj_q2_p1(tGrid,eps,bb0,bb1,u,f)
GRID *tGrid;
DOUBLE eps, (*bb0)(), (*bb1)();
INT u, f;
{
   NODE *pn;

   for (pn = FIRSTNODE(tGrid); pn; pn = pn->succ)
      if (is_macro_node(pn))
         subtract_local_proj_q2_p1_for_one_node(pn,eps,bb0,bb1,u,f);
}

#else

void subtract_local_proj_p2_p1(tGrid,eps,bb0,bb1,u,f)
GRID *tGrid; DOUBLE eps, (*bb0)(), (*bb1)(); INT u, f;
{  eprintf("Error: subtract_local_proj_p2_p1 not available.\n");  }

void subtract_local_proj_q1_q0(tGrid,eps,bb0,bb1,u,f)
GRID *tGrid; DOUBLE eps, (*bb0)(), (*bb1)(); INT u, f;
{  eprintf("Error: subtract_local_proj_q1_q0 not available.\n");  }

void subtract_local_proj_q2_p1(tGrid,eps,bb0,bb1,u,f)
GRID *tGrid; DOUBLE eps, (*bb0)(), (*bb1)(); INT u, f;
{  eprintf("Error: subtract_local_proj_q2_p1 not available.\n");  }

#endif

#if (N_DATA & MVECTOR_NODE_DATA) && (F_DATA & MVECTOR_FACE_DATA) && (E_DATA & MVECTOR_ELEMENT_DATA)

/* ONLY FOR RECTANGLES!!! */
void subtract_local_proj_q2b3_p1_for_one_edge(pn,nb,fb,eb,eps,bb0,bb1,u,f)
NODE *pn, *nb;
FACE *fb;
ELEMENT **eb;
DOUBLE eps, (*bb0)(), (*bb1)();
INT u, f;
{
   ELEMENT *pel;
   DOUBLE bx[DIM], nn[DIM], un[4], uf[4], ug[4], sk, sj, q, z, 
          p0, p1, p2, r0, r1, r2, a00, a01, a02, a11, a12, a22;
   INT i, j, k, l;

   bx[0] = bb0(pn->myvertex->x);
   bx[1] = bb1(pn->myvertex->x);
   a00 = a01 = a02 = a11 = a12 = a22 = r0 = r1 = r2 = 0.;
   for (i = 0; i < 2; i++){
      pel = eb[i];
      add_to_p1_macro_mass_matr_for_rectangles(pel,&a00,&a01,&a02,
                                                   &a11,&a12,&a22,&z);
      un[0] = NDMV(pel->n[0],u,0);
      un[1] = NDMV(pel->n[1],u,0);
      un[2] = NDMV(pel->n[2],u,0);
      un[3] = NDMV(pel->n[3],u,0);
      uf[0] = FDMV(pel->f[0],u,0);
      uf[1] = FDMV(pel->f[1],u,0);
      uf[2] = FDMV(pel->f[2],u,0);
      uf[3] = FDMV(pel->f[3],u,0);
      ug[0] = FDMV(pel->f[0],u,1);
      ug[1] = FDMV(pel->f[1],u,1);
      ug[2] = FDMV(pel->f[2],u,1);
      ug[3] = FDMV(pel->f[3],u,1);
      z *= (un[0]+un[1]+un[2]+un[3])*0.25 + (uf[0]+uf[1]+uf[2]+uf[3])/3.
           +  EDMV(pel,u,0)*4/9.          + (ug[0]+ug[1]+ug[2]+ug[3])*3./64.
           + (EDMV(pel,u,1)+EDMV(pel,u,2)+EDMV(pel,u,3))*9./64.;
      r0 -= z*bx[0];
      r1 -= z*bx[1];
      for (j = 0; j < EDGES; j++)
         if (pel->f[j] != fb){
            k = (j+1)%NVERT;
            outer_nonunit_normal_vector_to_element_edge(pel->n[j],pel->n[k],
                                                        pel->n[(j+2)%NVERT],nn);
            z = 2.*(un[j] + uf[j] + un[k]);
            sj = z - un[k];
            sk = z - un[j];
            if (NOT_BF(pel->f[j]) && ((IS_DN(pel->n[j]) && IS_FN(pel->n[k]))
                                   || (IS_DN(pel->n[k]) && IS_FN(pel->n[j])))){
               if (pel->n[j]->index < pel->n[k]->index){
                  sj += 1.80*ug[j];
                  sk += 0.45*ug[j];
               }
               else{
                  sj += 0.45*ug[j];
                  sk += 1.80*ug[j];
               }
            }
            q = DOT(bx,nn)/6.;
            r0 += (sj*pel->n[j]->myvertex->x[0] +
                   sk*pel->n[k]->myvertex->x[0])*q;
            r1 += (sj*pel->n[j]->myvertex->x[1] +
                   sk*pel->n[k]->myvertex->x[1])*q;
            r2 += (sj + sk)*q;
         }
   }
   solve_3x3_symm_system(a00,a01,a02,a11,a12,a22,r0,r1,r2,&p0,&p1,&p2,
                         lpm_delta(pn,eps,bb0,bb1));
   for (i = 0; i < 2; i++){
      pel = eb[i];
      q = (p0*bx[0]+p1*bx[1])*VOLUME(pel);
      r0 = q*0.25;
      r1 = q/3.;
      r2 = q*9./64.;
      NDMV(pel->n[0],f,0) += r0;
      NDMV(pel->n[1],f,0) += r0;
      NDMV(pel->n[2],f,0) += r0;
      NDMV(pel->n[3],f,0) += r0;
      FDMV(pel->f[0],f,0) += r1;
      FDMV(pel->f[1],f,0) += r1;
      FDMV(pel->f[2],f,0) += r1;
      FDMV(pel->f[3],f,0) += r1;
      EDMV(pel,f,0) += q*4./9.; 
      EDMV(pel,f,1) += r2;
      EDMV(pel,f,2) += r2;
      EDMV(pel,f,3) += r2;
      r2 = q*3./64.;
      FDMV(fb,f,1) += r2;
      for (j = 0; j < EDGES; j++)
         if (pel->f[j] != fb){
            k = (j+1)%NVERT;
            outer_nonunit_normal_vector_to_element_edge(pel->n[j],pel->n[k],
                                                        pel->n[(j+2)%NVERT],nn);
            q = DOT(bx,nn)/6.;
            sj = p0*pel->n[j]->myvertex->x[0] 
               + p1*pel->n[j]->myvertex->x[1] + p2;
            sk = p0*pel->n[k]->myvertex->x[0] 
               + p1*pel->n[k]->myvertex->x[1] + p2;
            z = 2.*(sj+sk);
            NDMV(pel->n[j],f,0) += (sk - z)*q;
            NDMV(pel->n[k],f,0) += (sj - z)*q;
            FDMV(pel->f[j],f,0) -= z*q;
            if (NOT_BF(pel->f[j]) && ((IS_DN(pel->n[j]) && IS_FN(pel->n[k]))
                                   || (IS_DN(pel->n[k]) && IS_FN(pel->n[j])))){
               FDMV(pel->f[j],f,1) += r2;
               if (pel->n[j]->index < pel->n[k]->index)
                  FDMV(pel->f[j],f,1) -= (1.8*sj + 0.45*sk)*q;
               else
                  FDMV(pel->f[j],f,1) -= (1.8*sk + 0.45*sj)*q;
            }
         }
   }
}

/* ONLY FOR RECTANGLES!!! */
void subtract_local_proj_q2b3_p1(tGrid,eps,bb0,bb1,u,f)
GRID *tGrid;
DOUBLE eps, (*bb0)(), (*bb1)();
INT u, f;
{
   NODE *pn, *nb[10];
   FACE *fb[10];
   ELEMENT *eb[10][2];
   INT i, j, n;

   for (pn = FIRSTNODE(tGrid); pn; pn = pn->succ)
      if (is_macro_node(pn))
         if (TSTART(pn) != START(pn)){
            edges_for_b_macros(pn,&n,fb,eb,nb);
            for (i = 0; i < n; i++)
               subtract_local_proj_q2b3_p1_for_one_edge(pn,nb[i],fb[i],eb[i],
                                                        eps,bb0,bb1,u,f);
         }
         else
            subtract_local_proj_q2_p1_for_one_node(pn,eps,bb0,bb1,u,f);
}

#else

void subtract_local_proj_q2b3_p1(tGrid,eps,bb0,bb1,u,f)
GRID *tGrid; DOUBLE eps, (*bb0)(), (*bb1)(); INT u, f;
{  eprintf("Error: subtract_local_proj_q2b3_p1 not available.\n");  }

#endif

void general_contrib_to_loc_proj_p1(tGrid,pel,u,bx,a00,a01,a02,a11,a12,a22,
                                    r0,r1,r2,finite_el,rm_type,qr)
GRID *tGrid;
ELEMENT *pel;
DOUBLE *bx, *a00, *a01, *a02, *a11, *a12, *a22, *r0, *r1, *r2;
INT u, rm_type;
QUADRATURE_RULE qr;
FINITE_ELEMENT finite_el;
{
   INT k;
   FLOAT b[DIM][DIM], grad[DIM], y[DIM], jac, q, z0, z1, s=0.;
   REF_MAPPING ref_map;

   reference_mapping_with_inverse(pel,rm_type,&ref_map);
   for (k=0; k < qr.n; k++){
      inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
      ggrad_value_ref(tGrid,pel,u,qr.points[k],finite_el,grad);
      ref_map_point(qr.points[k],y,&ref_map);
      jac = jacobian(qr.points[k],&ref_map);
      q = qr.weights[k]*QR_VOL*SGN(jac);;
      *r0 += q*(bx[0]*(grad[0]*b[0][0] + grad[1]*b[1][0])
              + bx[1]*(grad[0]*b[0][1] + grad[1]*b[1][1]))*y[0];
      *r1 += q*(bx[0]*(grad[0]*b[0][0] + grad[1]*b[1][0])
              + bx[1]*(grad[0]*b[0][1] + grad[1]*b[1][1]))*y[1];
      *r2 += q*(bx[0]*(grad[0]*b[0][0] + grad[1]*b[1][0])
              + bx[1]*(grad[0]*b[0][1] + grad[1]*b[1][1]));
      q *= jac;
      *a00 += q*y[0]*y[0];
      *a01 += q*y[0]*y[1];
      *a02 += q*y[0];
      *a11 += q*y[1]*y[1];
      *a12 += q*y[1];
      *a22 += q;
   }
}

#if (N_DATA & MVECTOR_NODE_DATA) && (F_DATA & MVECTOR_FACE_DATA) && (E_DATA & MVECTOR_ELEMENT_DATA) && (DIM == 2)

void general_local_integrate_rhs0(tGrid,pel,f,rhs,a0,a1,a2,a3,a4,i0,i1,i2,
                                 f0,f1,f2,f3,f4,integral,finite_el,rmtype,qr)
GRID *tGrid;
ELEMENT *pel;
INT f, rmtype, i0, i1, i2;
FLOAT (*rhs)(), a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)();
DOUBLE (*integral)();
FINITE_ELEMENT finite_el;
QUADRATURE_RULE qr;
{
   REF_MAPPING ref_map;
   DOUBLE_FUNC *nq, *nq0, *nq1, *fq, *fq0, *fq1, *eq, *eq0, *eq1;
   INT i, k, m, kk, nn, mm, dir[4];
  
   nq   = finite_el.nbasis;
   nq0  = finite_el.nbasis_0;
   nq1  = finite_el.nbasis_1;
   fq   = finite_el.fbasis;
   fq0  = finite_el.fbasis_0;
   fq1  = finite_el.fbasis_1;
   eq   = finite_el.ebasis;
   eq0  = finite_el.ebasis_0;
   eq1  = finite_el.ebasis_1;
   kk = finite_el.k;
   nn = finite_el.n;
   mm = finite_el.m;
   reference_mapping_with_inverse(pel,rmtype,&ref_map);
   if (kk)
      for (i=0; i < NVERT; i++){
         m = i*kk;
         for (k = m; k < m+kk; k++)
               NDMV(pel->n[i],f,k-m) += integral(tGrid,pel,nq[k],nq0[k],nq1[k],
               rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,finite_el,ref_map,qr);
      }
   if (nn){
      set_directions_of_edges(pel,dir);
      for (i=0; i < NVERT; i++){
         m = i*nn;
         for (k = m; k < m+nn; k++) 
            FDMV(pel->f[i],f,f_i(k-m,nn,dir[i])) += 
                   integral(tGrid,pel,fq[k],fq0[k],fq1[k],rhs,
                   a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,finite_el,ref_map,qr);
      }
   }
   for (i = 0; i < mm; i++)
      EDMV(pel,f,i) += integral(tGrid,pel,eq[i],eq0[i],eq1[i],
               rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,finite_el,ref_map,qr);
}

#else

void general_local_integrate_rhs0(tGrid,pel,f,rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,integral,finite_el,rmtype,qr)
GRID *tGrid; ELEMENT *pel; INT f, rmtype, i0, i1, i2; FLOAT (*rhs)(), a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(); DOUBLE (*integral)(); FINITE_ELEMENT finite_el; QUADRATURE_RULE qr;
{  eprintf("Error: general_local_integrate_rhs0 not available.\n");  }

#endif

DOUBLE general_subtr_loc_proj_p1_ref(tGrid,pel,v,v0,v1,rhs,p0,p1,p2,b0,b1,
                                   i0,i1,i2,f0,f1,f2,f3,f4,finite_el,ref_map,qr)
GRID *tGrid;
ELEMENT *pel;
FLOAT (*v)(), (*v0)(), (*v1)(), (*rhs)(), p0, p1, p2, b0, b1, 
      (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)();
INT i0, i1, i2;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
QUADRATURE_RULE qr;
{
   INT k;
   FLOAT b[DIM][DIM], y[DIM], s=0., jac0=jacobian(qr.points[0],&ref_map);

   for (k=0; k < qr.n; k++){
      inv_of_jac_matr_times_jacobian(qr.points[k],b,&ref_map);
      ref_map_point(qr.points[k],y,&ref_map);
      s -= qr.weights[k]*(p0*y[0] + p1*y[1] + p2)*(
                b0*(v0(qr.points[k])*b[0][0] + v1(qr.points[k])*b[1][0]) 
              + b1*(v0(qr.points[k])*b[0][1] + v1(qr.points[k])*b[1][1]));
   }
   return(s*QR_VOL*SGN(jac0));
}

#if DATA_S & N_LINK_TO_ELEMENTS

void general_subtract_local_proj_p1(tGrid,eps,bb0,bb1,u,f)
GRID *tGrid;
DOUBLE eps, (*bb0)(), (*bb1)();
INT u, f;
{
   NELINK *pnel;
   ELEMENT *pel;
   NODE *pn;
   DOUBLE bx[DIM], nn[DIM], un[4], uf[4], sk, sj, q, z, 
          p0, p1, p2, r0, r1, r2, a00, a01, a02, a11, a12, a22; 
   INT j, k, l;

   for (pn = FIRSTNODE(tGrid); pn; pn = pn->succ)
   if (is_macro_node(pn)){
      bx[0] = bb0(pn->myvertex->x);
      bx[1] = bb1(pn->myvertex->x);
      a00 = a01 = a02 = a11 = a12 = a22 = r0 = r1 = r2 = 0.;
      for (pnel = NESTART(pn); pnel; pnel = pnel->next)
         general_contrib_to_loc_proj_p1(tGrid,pnel->nbel,u,bx,
         &a00,&a01,&a02,&a11,&a12,&a22,&r0,&r1,&r2,ELEM,REF_MAP,LAPL_Q_RULE);
      solve_3x3_symm_system(a00,a01,a02,a11,a12,a22,r0,r1,r2,&p0,&p1,&p2,
                            lpm_delta(pn,eps,bb0,bb1));
      for (pnel = NESTART(pn); pnel; pnel = pnel->next)
         general_local_integrate_rhs0(tGrid,pnel->nbel,f,NULL,
                                   p0,p1,p2,bx[0],bx[1],
                                   0,1,2,NULL,NULL,NULL,NULL,NULL,
                                   general_subtr_loc_proj_p1_ref,
                                   ELEM,REF_MAP,SDRHS_Q_RULE);
   }
}

#else

void general_subtract_local_proj_p1(tGrid,eps,bb0,bb1,u,f)
GRID *tGrid; DOUBLE eps, (*bb0)(), (*bb1)(); INT u, f;
{  eprintf("Error: general_subtract_local_proj_p1 not available.\n");  }

#endif

void subtract_local_proj(tGrid,eps,bb0,bb1,u,f,space)
GRID *tGrid;
DOUBLE eps, (*bb0)(), (*bb1)();
INT u, f, space;
{
   if (space == P2C || space == GP2C || space == GP2X3C)
      subtract_local_proj_p2_p1(tGrid,eps,bb0,bb1,u,f);
   else if (space == Q1C || space == GQ1C || space == GQ1X4C)
      subtract_local_proj_q1_q0(tGrid,eps,bb0,bb1,u,f);
   else if (space == Q2C || space == GQ2C || space == GQ2X4C)
//    general_subtract_local_proj_p1(tGrid,eps,bb0,bb1,u,f);
      subtract_local_proj_q2_p1(tGrid,eps,bb0,bb1,u,f);
   else if (space == GQ2B3C){
      partial_set_vect_to_zero_for_q2b3(tGrid,u);
      subtract_local_proj_q2b3_p1(tGrid,eps,bb0,bb1,u,f);
      partial_set_vect_to_zero_for_q2b3(tGrid,f);
   }
   else
      eprintf("Error: subtract_local_proj not available.\n");
}

#if (N_DATA & NxN_NODE_MATR) && (N_DATA & NxM_NODE_FACE_MATR) && (F_DATA & NxN_FACE_MATR) && (F_DATA & MxN_FACE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES) && (E_DATA & MxM_E_E_MATR) && (E_DATA & MxN_E_N_MATR) && (E_DATA & NxM_N_E_MATR) && (E_DATA & MxN_E_F_MATR) && (E_DATA & NxM_F_E_MATR) && (DIM == 2)

void general_local_stiff_matr(tGrid,pel,Z,a0,a1,a2,a3,a4,i0,i1,i2,
                     f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,integral,finite_el,rmtype,qr,
                     kk,nn,mm,
                     nq,nq0,nq1,nq00,nq01,nq11,
                     fq,fq0,fq1,fq00,fq01,fq11,
                     eq,eq0,eq1,eq00,eq01,eq11)
GRID *tGrid;
ELEMENT *pel;
INT Z, rmtype, i0, i1, i2, kk, nn, mm;
FLOAT a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(), (*f5)(),
      (*f6)(), (*f7)(), (*f8)(), (*f9)(), (*integral)();
FINITE_ELEMENT finite_el;
QUADRATURE_RULE qr;
DOUBLE_FUNC *nq, *nq0, *nq1, *nq00, *nq01, *nq11,
            *fq, *fq0, *fq1, *fq00, *fq01, *fq11,
            *eq, *eq0, *eq1, *eq00, *eq01, *eq11;
{
   NFLINK *pnf;
   FNLINK *pfn;
   FLINK *pfl;
   LINK *pli;
   REF_MAPPING ref_map;
   INT i, j, k, l, m, h, dir[4];
  
   reference_mapping_with_inverse(pel,rmtype,&ref_map);
   set_directions_of_edges(pel,dir);
   if (kk > 1)
      eprintf("Error: kk > 1 not implemented in general_local_stiff_matr.\n");
   for (i=0; i < NVERT; i++){
      for (j=0; j < NVERT; j++){
         if (i == j){
            if (kk)
               COEFF_NN(pel->n[i],Z,0,0) += 
            integral(tGrid,pel,nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                               nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
            m = j*nn;
            for (k = m; k < m+nn; k++) 
               for (l = m; l < m+nn; l++) 
                  COEFF_NN(pel->f[i],Z,f_i(k-m,nn,dir[i]),f_i(l-m,nn,dir[i])) +=
            integral(tGrid,pel,fq[k],fq0[k],fq1[k],fq00[k],fq01[k],fq11[k],
                               fq[l],fq0[l],fq1[l],fq00[l],fq01[l],fq11[l],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
         }
         else{
            for (pli=pel->n[i]->tstart; pli->nbnode!=pel->n[j]; pli=pli->next);
            for (pfl=pel->f[i]->tfstart; pfl->nbface!=pel->f[j]; pfl=pfl->next);
            if (kk)
               COEFF_NN(pli,Z,0,0) += 
            integral(tGrid,pel,nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                               nq[j],nq0[j],nq1[j],nq00[j],nq01[j],nq11[j],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
            m = i*nn;
            h = j*nn;
            for (k = m; k < m+nn; k++) 
               for (l = h; l < h+nn; l++) 
                  COEFF_NN(pfl,Z,f_i(k-m,nn,dir[i]),f_i(l-h,nn,dir[j])) +=
            integral(tGrid,pel,fq[k],fq0[k],fq1[k],fq00[k],fq01[k],fq11[k],
                               fq[l],fq0[l],fq1[l],fq00[l],fq01[l],fq11[l],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
         }
         for (pnf=pel->n[i]->tnfstart; pnf->nbface!=pel->f[j]; pnf=pnf->next);
         m = j*nn;
         for (k = m; k < m+nn; k++) 
            COEFF_NN(pnf,Z,0,f_i(k-m,nn,dir[j])) +=
            integral(tGrid,pel,nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                               fq[k],fq0[k],fq1[k],fq00[k],fq01[k],fq11[k],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
         for (pfn=pel->f[i]->tfnstart; pfn->nbnode!=pel->n[j]; pfn=pfn->next);
         m = i*nn;
         for (k = m; k < m+nn; k++)
            COEFF_NN(pfn,Z,f_i(k-m,nn,dir[i]),0) +=
            integral(tGrid,pel,fq[k],fq0[k],fq1[k],fq00[k],fq01[k],fq11[k],
                               nq[j],nq0[j],nq1[j],nq00[j],nq01[j],nq11[j],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
      }
      for (l = 0; l < mm; l++){
         COEFF_NE_NM(pel,Z,i,0,l) += 
            integral(tGrid,pel,nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                               eq[l],eq0[l],eq1[l],eq00[l],eq01[l],eq11[l],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
         COEFF_EN_MN(pel,Z,i,l,0) += 
            integral(tGrid,pel,eq[l],eq0[l],eq1[l],eq00[l],eq01[l],eq11[l],
                               nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
         m = i*nn;
         for (k = m; k < m+nn; k++){
            COEFF_FE_NM(pel,Z,i,f_i(k-m,nn,dir[i]),l) +=
            integral(tGrid,pel,fq[k],fq0[k],fq1[k],fq00[k],fq01[k],fq11[k],
                               eq[l],eq0[l],eq1[l],eq00[l],eq01[l],eq11[l],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
            COEFF_EF_MN(pel,Z,i,l,f_i(k-m,nn,dir[i])) +=
            integral(tGrid,pel,eq[l],eq0[l],eq1[l],eq00[l],eq01[l],eq11[l],
                               fq[k],fq0[k],fq1[k],fq00[k],fq01[k],fq11[k],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
         }
      }
   }
   for (k = 0; k < mm; k++)
      for (l = 0; l < mm; l++)
         COEFF_EE_MM(pel,Z,k,l) +=
            integral(tGrid,pel,eq[k],eq0[k],eq1[k],eq00[k],eq01[k],eq11[k],
                               eq[l],eq0[l],eq1[l],eq00[l],eq01[l],eq11[l],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
}

void general_stiff_matr_spec(tGrid,Z,a0,a1,a2,a3,a4,i0,i1,i2,
                     f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,integral,finite_el,rmtype,qr)
GRID *tGrid;
INT Z, rmtype, i0, i1, i2;
FLOAT a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(), (*f5)(),
      (*f6)(), (*f7)(), (*f8)(), (*f9)(), (*integral)();
FINITE_ELEMENT finite_el;
QUADRATURE_RULE qr;
{
   ELEMENT *pel;
   NFLINK *pnf;
   FNLINK *pfn;
   FLINK *pfl;
   LINK *pli;
   REF_MAPPING ref_map;
   DOUBLE_FUNC *nq, *nq0, *nq1, *nq00, *nq01, *nq11,
               *fq, *fq0, *fq1, *fq00, *fq01, *fq11,
               *eq, *eq0, *eq1, *eq00, *eq01, *eq11;
   INT i, j, k, l, m, h, kk, nn, mm, dir[4];
  
   nq   = finite_el.nbasis;
   nq0  = finite_el.nbasis_0;
   nq1  = finite_el.nbasis_1;
   nq00 = finite_el.nbasis_00;
   nq01 = finite_el.nbasis_01;
   nq11 = finite_el.nbasis_11;
   fq   = finite_el.fbasis;
   fq0  = finite_el.fbasis_0;
   fq1  = finite_el.fbasis_1;
   fq00 = finite_el.fbasis_00;
   fq01 = finite_el.fbasis_01;
   fq11 = finite_el.fbasis_11;
   eq   = finite_el.ebasis;
   eq0  = finite_el.ebasis_0;
   eq1  = finite_el.ebasis_1;
   eq00 = finite_el.ebasis_00;
   eq01 = finite_el.ebasis_01;
   eq11 = finite_el.ebasis_11;
   kk = finite_el.k;
   nn = finite_el.n;
   mm = finite_el.m;
   for (pel = FIRSTELEMENT(tGrid); pel; pel = pel->succ){
   reference_mapping_with_inverse(pel,rmtype,&ref_map);
   set_directions_of_edges(pel,dir);
   if (kk > 1)
      eprintf("Error: kk > 1 not implemented in general_stiff_matr_spec.\n");
   for (i=0; i < NVERT; i++){
      for (j=0; j < NVERT; j++){
         if (i == j){
            if (kk)
               COEFF_NN(pel->n[i],Z,0,0) += 
            integral(tGrid,pel,nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                               nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
            m = j*nn;
            for (k = m; k < m+nn; k++) 
               for (l = m; l < m+nn; l++) 
                  COEFF_NN(pel->f[i],Z,f_i(k-m,nn,dir[i]),f_i(l-m,nn,dir[i])) +=
            integral(tGrid,pel,fq[k],fq0[k],fq1[k],fq00[k],fq01[k],fq11[k],
                               fq[l],fq0[l],fq1[l],fq00[l],fq01[l],fq11[l],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
         }
         else if (fabs(i-j)!=2){
            for (pfl=pel->f[i]->tfstart; pfl->nbface!=pel->f[j]; pfl=pfl->next);
            m = i*nn;
            h = j*nn;
            for (k = m; k < m+nn; k++) 
               for (l = h; l < h+nn; l++) 
                  COEFF_NN(pfl,Z,f_i(k-m,nn,dir[i]),f_i(l-h,nn,dir[j])) +=
            integral(tGrid,pel,fq[k],fq0[k],fq1[k],fq00[k],fq01[k],fq11[k],
                               fq[l],fq0[l],fq1[l],fq00[l],fq01[l],fq11[l],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
         }
         for (pnf=pel->n[i]->tnfstart; pnf->nbface!=pel->f[j]; pnf=pnf->next);
         m = j*nn;
         if (i==j || i == (j+1)%4)
         for (k = m; k < m+nn; k++) 
            COEFF_NN(pnf,Z,0,f_i(k-m,nn,dir[j])) +=
            integral(tGrid,pel,nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                               fq[k],fq0[k],fq1[k],fq00[k],fq01[k],fq11[k],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
         for (pfn=pel->f[i]->tfnstart; pfn->nbnode!=pel->n[j]; pfn=pfn->next);
         m = i*nn;
         if (i==j || j == (i+1)%4)
         for (k = m; k < m+nn; k++)
            COEFF_NN(pfn,Z,f_i(k-m,nn,dir[i]),0) +=
            integral(tGrid,pel,fq[k],fq0[k],fq1[k],fq00[k],fq01[k],fq11[k],
                               nq[j],nq0[j],nq1[j],nq00[j],nq01[j],nq11[j],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
      }
      for (l = 0; l < mm; l++){
         COEFF_NE_NM(pel,Z,i,0,l) += 
            integral(tGrid,pel,nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                               eq[l],eq0[l],eq1[l],eq00[l],eq01[l],eq11[l],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
         COEFF_EN_MN(pel,Z,i,l,0) += 
            integral(tGrid,pel,eq[l],eq0[l],eq1[l],eq00[l],eq01[l],eq11[l],
                               nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
         m = i*nn;
         for (k = m; k < m+nn; k++){
            COEFF_FE_NM(pel,Z,i,f_i(k-m,nn,dir[i]),l) +=
            integral(tGrid,pel,fq[k],fq0[k],fq1[k],fq00[k],fq01[k],fq11[k],
                               eq[l],eq0[l],eq1[l],eq00[l],eq01[l],eq11[l],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
            COEFF_EF_MN(pel,Z,i,l,f_i(k-m,nn,dir[i])) +=
            integral(tGrid,pel,eq[l],eq0[l],eq1[l],eq00[l],eq01[l],eq11[l],
                               fq[k],fq0[k],fq1[k],fq00[k],fq01[k],fq11[k],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
         }
      }
   }
   for (k = 0; k < mm; k++)
      for (l = 0; l < mm; l++)
         COEFF_EE_MM(pel,Z,k,l) +=
            integral(tGrid,pel,eq[k],eq0[k],eq1[k],eq00[k],eq01[k],eq11[k],
                               eq[l],eq0[l],eq1[l],eq00[l],eq01[l],eq11[l],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
   }
}

void general_local_1_bubble_contrib_to_nodes(tGrid,pel,Z,a0,a1,a2,a3,a4,f,i1,i2,
                     f0,f1,f2,f3,f4,f5,f6,f7,rhs,integral_rhs,integral,
                     finite_el,rmtype,qr,kk,nn,mm,
                     nq,nq0,nq1,nq00,nq01,nq11,
                     fq,fq0,fq1,fq00,fq01,fq11,
                     eq,eq0,eq1,eq00,eq01,eq11)
GRID *tGrid;
ELEMENT *pel;
INT Z, rmtype, f, i1, i2, kk, nn, mm;
FLOAT a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(), (*f5)(),
      (*f6)(), (*f7)(), (*rhs)(), (*integral_rhs)(), (*integral)();
FINITE_ELEMENT finite_el;
QUADRATURE_RULE qr;
DOUBLE_FUNC *nq, *nq0, *nq1, *nq00, *nq01, *nq11,
            *fq, *fq0, *fq1, *fq00, *fq01, *fq11,
            *eq, *eq0, *eq1, *eq00, *eq01, *eq11;
{
   LINK *pli;
   REF_MAPPING ref_map;
   DOUBLE a[NVERT], b[NVERT], c, d;
   INT i, j;
  
   reference_mapping_with_inverse(pel,rmtype,&ref_map);
   c =  -integral(tGrid,pel,eq[0],eq0[0],eq1[0],eq00[0],eq01[0],eq11[0],
                            eq[0],eq0[0],eq1[0],eq00[0],eq01[0],eq11[0],
                            a0,a1,a2,a3,a4,0,i1,i2,f0,f1,f2,f3,f4,f5,
                            f6,f7,NULL,NULL,finite_el,ref_map,qr);
   d = integral_rhs(tGrid,pel,eq[0],eq0[0],eq1[0],
                rhs,a0,a1,a2,a3,a4,0,i1,i2,f0,f1,f2,f3,f4,finite_el,ref_map,qr);
   for (i=0; i < NVERT; i++){
      a[i] = integral(tGrid,pel,eq[0],eq0[0],eq1[0],eq00[0],eq01[0],eq11[0],
                                nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                                a0,a1,a2,a3,a4,0,i1,i2,f0,f1,f2,f3,f4,f5,
                                f6,f7,NULL,NULL,finite_el,ref_map,qr)/c;
      b[i] = integral(tGrid,pel,nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                                eq[0],eq0[0],eq1[0],eq00[0],eq01[0],eq11[0],
                                a0,a1,a2,a3,a4,0,i1,i2,f0,f1,f2,f3,f4,f5,
                                f6,f7,NULL,NULL,finite_el,ref_map,qr);
      NDMV(pel->n[i],f,0) += a[i]*d;
   }
   for (i=0; i < NVERT; i++)
      for (j=0; j < NVERT; j++)
         if (i == j)
            COEFF_NN(pel->n[i],Z,0,0) += a[i]*b[i];
         else{
            for (pli=pel->n[i]->tstart; pli->nbnode!=pel->n[j]; pli=pli->next);
            COEFF_NN(pli,Z,0,0) += a[i]*b[j];
         }
}

#else

void general_local_stiff_matr(tGrid,pel,Z,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,integral,finite_el,rmtype,qr,kk,nn,mm,nq,nq0,nq1,nq00,nq01,nq11,fq,fq0,fq1,fq00,fq01,fq11,eq,eq0,eq1,eq00,eq01,eq11)
GRID *tGrid; ELEMENT *pel; INT Z, rmtype, i0, i1, i2, kk, nn, mm; FLOAT a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)(), (*integral)(); FINITE_ELEMENT finite_el; QUADRATURE_RULE qr; DOUBLE_FUNC *nq, *nq0, *nq1, *nq00, *nq01, *nq11, *fq, *fq0, *fq1, *fq00, *fq01, *fq11, *eq, *eq0, *eq1, *eq00, *eq01, *eq11;
{  eprintf("Error: general_local_stiff_matr not available.\n");  }

void general_local_1_bubble_contrib_to_nodes(tGrid,pel,Z,a0,a1,a2,a3,a4,f,i1,i2,f0,f1,f2,f3,f4,f5,f6,f7,rhs,integral_rhs,integral,finite_el,rmtype,qr,kk,nn,mm,nq,nq0,nq1,nq00,nq01,nq11,fq,fq0,fq1,fq00,fq01,fq11,eq,eq0,eq1,eq00,eq01,eq11)
GRID *tGrid; ELEMENT *pel; INT Z, rmtype, f, i1, i2, kk, nn, mm; FLOAT a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*rhs)(), (*integral_rhs)(), (*integral)(); FINITE_ELEMENT finite_el; QUADRATURE_RULE qr; DOUBLE_FUNC *nq, *nq0, *nq1, *nq00, *nq01, *nq11, *fq, *fq0, *fq1, *fq00, *fq01, *fq11, *eq, *eq0, *eq1, *eq00, *eq01, *eq11;
{  eprintf("Error: general_local_1_bubble_contrib_to_nodes not available.\n");  }

#endif

void general_stiff_matr(tGrid,Z,a0,a1,a2,a3,a4,i0,i1,i2,
                     f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,integral,finite_el,rmtype,qr)
GRID *tGrid;
INT Z, rmtype, i0, i1, i2;
FLOAT a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(), (*f5)(),
      (*f6)(), (*f7)(), (*f8)(), (*f9)(), (*integral)();
FINITE_ELEMENT finite_el;
QUADRATURE_RULE qr;
{
   ELEMENT *pel;
   DOUBLE_FUNC *nq, *nq0, *nq1, *nq00, *nq01, *nq11,
               *fq, *fq0, *fq1, *fq00, *fq01, *fq11,
               *eq, *eq0, *eq1, *eq00, *eq01, *eq11;
   INT kk, nn, mm;
  
   nq   = finite_el.nbasis;
   nq0  = finite_el.nbasis_0;
   nq1  = finite_el.nbasis_1;
   nq00 = finite_el.nbasis_00;
   nq01 = finite_el.nbasis_01;
   nq11 = finite_el.nbasis_11;
   fq   = finite_el.fbasis;
   fq0  = finite_el.fbasis_0;
   fq1  = finite_el.fbasis_1;
   fq00 = finite_el.fbasis_00;
   fq01 = finite_el.fbasis_01;
   fq11 = finite_el.fbasis_11;
   eq   = finite_el.ebasis;
   eq0  = finite_el.ebasis_0;
   eq1  = finite_el.ebasis_1;
   eq00 = finite_el.ebasis_00;
   eq01 = finite_el.ebasis_01;
   eq11 = finite_el.ebasis_11;
   kk = finite_el.k;
   nn = finite_el.n;
   mm = finite_el.m;
   for (pel = FIRSTELEMENT(tGrid); pel; pel = pel->succ)
      general_local_stiff_matr(tGrid,pel,Z,a0,a1,a2,a3,a4,i0,i1,i2,
                     f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,integral,finite_el,rmtype,qr,
                     kk,nn,mm,
                     nq,nq0,nq1,nq00,nq01,nq11,
                     fq,fq0,fq1,fq00,fq01,fq11,
                     eq,eq0,eq1,eq00,eq01,eq11);
}

void general_stiff_matr_b(tGrid,Z,a0,a1,a2,a3,a4,i0,i1,i2,
       f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,integral,i_finite_el,b_finite_el,rmtype,qr)
GRID *tGrid;
INT Z, rmtype, i0, i1, i2;
FLOAT a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(), (*f5)(),
      (*f6)(), (*f7)(), (*f8)(), (*f9)(), (*integral)();
FINITE_ELEMENT i_finite_el, b_finite_el;
QUADRATURE_RULE qr;
{
   ELEMENT *pel;
   DOUBLE_FUNC *i_nq, *i_nq0, *i_nq1, *i_nq00, *i_nq01, *i_nq11,
               *i_fq, *i_fq0, *i_fq1, *i_fq00, *i_fq01, *i_fq11,
               *i_eq, *i_eq0, *i_eq1, *i_eq00, *i_eq01, *i_eq11,
               *b_nq, *b_nq0, *b_nq1, *b_nq00, *b_nq01, *b_nq11,
               *b_fq, *b_fq0, *b_fq1, *b_fq00, *b_fq01, *b_fq11,
               *b_eq, *b_eq0, *b_eq1, *b_eq00, *b_eq01, *b_eq11;
   INT i_kk, i_nn, i_mm, b_kk, b_nn, b_mm;
  
   i_nq   = i_finite_el.nbasis;
   i_nq0  = i_finite_el.nbasis_0;
   i_nq1  = i_finite_el.nbasis_1;
   i_nq00 = i_finite_el.nbasis_00;
   i_nq01 = i_finite_el.nbasis_01;
   i_nq11 = i_finite_el.nbasis_11;
   i_fq   = i_finite_el.fbasis;
   i_fq0  = i_finite_el.fbasis_0;
   i_fq1  = i_finite_el.fbasis_1;
   i_fq00 = i_finite_el.fbasis_00;
   i_fq01 = i_finite_el.fbasis_01;
   i_fq11 = i_finite_el.fbasis_11;
   i_eq   = i_finite_el.ebasis;
   i_eq0  = i_finite_el.ebasis_0;
   i_eq1  = i_finite_el.ebasis_1;
   i_eq00 = i_finite_el.ebasis_00;
   i_eq01 = i_finite_el.ebasis_01;
   i_eq11 = i_finite_el.ebasis_11;
   i_kk = i_finite_el.k;
   i_nn = i_finite_el.n;
   i_mm = i_finite_el.m;
   b_nq   = b_finite_el.nbasis;
   b_nq0  = b_finite_el.nbasis_0;
   b_nq1  = b_finite_el.nbasis_1;
   b_nq00 = b_finite_el.nbasis_00;
   b_nq01 = b_finite_el.nbasis_01;
   b_nq11 = b_finite_el.nbasis_11;
   b_fq   = b_finite_el.fbasis;
   b_fq0  = b_finite_el.fbasis_0;
   b_fq1  = b_finite_el.fbasis_1;
   b_fq00 = b_finite_el.fbasis_00;
   b_fq01 = b_finite_el.fbasis_01;
   b_fq11 = b_finite_el.fbasis_11;
   b_eq   = b_finite_el.ebasis;
   b_eq0  = b_finite_el.ebasis_0;
   b_eq1  = b_finite_el.ebasis_1;
   b_eq00 = b_finite_el.ebasis_00;
   b_eq01 = b_finite_el.ebasis_01;
   b_eq11 = b_finite_el.ebasis_11;
   b_kk = b_finite_el.k;
   b_nn = b_finite_el.n;
   b_mm = b_finite_el.m;
   for (pel = FIRSTELEMENT(tGrid); pel; pel = pel->succ)
      if (IS_B_EL(pel))
         general_local_stiff_matr(tGrid,pel,Z,a0,a1,a2,a3,a4,i0,i1,i2,
                  f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,integral,b_finite_el,rmtype,qr,
                  b_kk,b_nn,b_mm,
                  b_nq,b_nq0,b_nq1,b_nq00,b_nq01,b_nq11,
                  b_fq,b_fq0,b_fq1,b_fq00,b_fq01,b_fq11,
                  b_eq,b_eq0,b_eq1,b_eq00,b_eq01,b_eq11);
      else
         general_local_stiff_matr(tGrid,pel,Z,a0,a1,a2,a3,a4,i0,i1,i2,
                  f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,integral,i_finite_el,rmtype,qr,
                  i_kk,i_nn,i_mm,
                  i_nq,i_nq0,i_nq1,i_nq00,i_nq01,i_nq11,
                  i_fq,i_fq0,i_fq1,i_fq00,i_fq01,i_fq11,
                  i_eq,i_eq0,i_eq1,i_eq00,i_eq01,i_eq11);


}

#if DATA_S & N_LINK_TO_ELEMENTS

void general_macro_stiff_matr(tGrid,Z,eps,a1,a2,a3,a4,i0,i1,i2,
                   bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,integral,finite_el,rmtype,qr)
GRID *tGrid;
INT Z, rmtype, i0, i1, i2;
FLOAT eps, a1, a2, a3, a4, (*bb0)(), (*bb1)(), (*f2)(), (*f3)(), (*f4)(), 
      (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)(), (*integral)();
FINITE_ELEMENT finite_el;
QUADRATURE_RULE qr;
{
   NELINK *pnel;
   NODE *pn;
   DOUBLE bx[DIM], delta;
   DOUBLE_FUNC *nq, *nq0, *nq1, *nq00, *nq01, *nq11,
               *fq, *fq0, *fq1, *fq00, *fq01, *fq11,
               *eq, *eq0, *eq1, *eq00, *eq01, *eq11;
   INT kk, nn, mm;
  
   nq   = finite_el.nbasis;
   nq0  = finite_el.nbasis_0;
   nq1  = finite_el.nbasis_1;
   nq00 = finite_el.nbasis_00;
   nq01 = finite_el.nbasis_01;
   nq11 = finite_el.nbasis_11;
   fq   = finite_el.fbasis;
   fq0  = finite_el.fbasis_0;
   fq1  = finite_el.fbasis_1;
   fq00 = finite_el.fbasis_00;
   fq01 = finite_el.fbasis_01;
   fq11 = finite_el.fbasis_11;
   eq   = finite_el.ebasis;
   eq0  = finite_el.ebasis_0;
   eq1  = finite_el.ebasis_1;
   eq00 = finite_el.ebasis_00;
   eq01 = finite_el.ebasis_01;
   eq11 = finite_el.ebasis_11;
   kk = finite_el.k;
   nn = finite_el.n;
   mm = finite_el.m;
   for (pn = FIRSTNODE(tGrid); pn; pn = pn->succ)
   if (is_macro_node(pn)){
      bx[0] = bb0(pn->myvertex->x);
      bx[1] = bb1(pn->myvertex->x);
      delta = lpm_delta(pn,eps,bb0,bb1);
      for (pnel = NESTART(pn); pnel; pnel = pnel->next)
         general_local_stiff_matr(tGrid,pnel->nbel,Z,
                  eps,bx[0],bx[1],delta,a4,i0,i1,i2,
                  bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,integral,finite_el,rmtype,qr,
                  kk,nn,mm,
                  nq,nq0,nq1,nq00,nq01,nq11,
                  fq,fq0,fq1,fq00,fq01,fq11,
                  eq,eq0,eq1,eq00,eq01,eq11);
   }
}

void general_macro_stiff_matr_b(tGrid,Z,eps,a1,a2,a3,a4,i0,i1,i2,
                                bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,
                                integral,i_finite_el,b_finite_el,rmtype,qr)
GRID *tGrid;
INT Z, rmtype, i0, i1, i2;
FLOAT eps, a1, a2, a3, a4, (*bb0)(), (*bb1)(), (*f2)(), (*f3)(), (*f4)(), 
      (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)(), (*integral)();
FINITE_ELEMENT i_finite_el, b_finite_el;
QUADRATURE_RULE qr;
{
   NELINK *pnel;
   NODE *pn, *nb[10];
   FACE *fb[10];
   ELEMENT *eb[10][2];
   DOUBLE bx[DIM], delta;
   DOUBLE_FUNC *i_nq, *i_nq0, *i_nq1, *i_nq00, *i_nq01, *i_nq11,
               *i_fq, *i_fq0, *i_fq1, *i_fq00, *i_fq01, *i_fq11,
               *i_eq, *i_eq0, *i_eq1, *i_eq00, *i_eq01, *i_eq11,
               *b_nq, *b_nq0, *b_nq1, *b_nq00, *b_nq01, *b_nq11,
               *b_fq, *b_fq0, *b_fq1, *b_fq00, *b_fq01, *b_fq11,
               *b_eq, *b_eq0, *b_eq1, *b_eq00, *b_eq01, *b_eq11;
   INT i_kk, i_nn, i_mm, b_kk, b_nn, b_mm, i, j, n;
  
   i_nq   = i_finite_el.nbasis;
   i_nq0  = i_finite_el.nbasis_0;
   i_nq1  = i_finite_el.nbasis_1;
   i_nq00 = i_finite_el.nbasis_00;
   i_nq01 = i_finite_el.nbasis_01;
   i_nq11 = i_finite_el.nbasis_11;
   i_fq   = i_finite_el.fbasis;
   i_fq0  = i_finite_el.fbasis_0;
   i_fq1  = i_finite_el.fbasis_1;
   i_fq00 = i_finite_el.fbasis_00;
   i_fq01 = i_finite_el.fbasis_01;
   i_fq11 = i_finite_el.fbasis_11;
   i_eq   = i_finite_el.ebasis;
   i_eq0  = i_finite_el.ebasis_0;
   i_eq1  = i_finite_el.ebasis_1;
   i_eq00 = i_finite_el.ebasis_00;
   i_eq01 = i_finite_el.ebasis_01;
   i_eq11 = i_finite_el.ebasis_11;
   i_kk = i_finite_el.k;
   i_nn = i_finite_el.n;
   i_mm = i_finite_el.m;
   b_nq   = b_finite_el.nbasis;
   b_nq0  = b_finite_el.nbasis_0;
   b_nq1  = b_finite_el.nbasis_1;
   b_nq00 = b_finite_el.nbasis_00;
   b_nq01 = b_finite_el.nbasis_01;
   b_nq11 = b_finite_el.nbasis_11;
   b_fq   = b_finite_el.fbasis;
   b_fq0  = b_finite_el.fbasis_0;
   b_fq1  = b_finite_el.fbasis_1;
   b_fq00 = b_finite_el.fbasis_00;
   b_fq01 = b_finite_el.fbasis_01;
   b_fq11 = b_finite_el.fbasis_11;
   b_eq   = b_finite_el.ebasis;
   b_eq0  = b_finite_el.ebasis_0;
   b_eq1  = b_finite_el.ebasis_1;
   b_eq00 = b_finite_el.ebasis_00;
   b_eq01 = b_finite_el.ebasis_01;
   b_eq11 = b_finite_el.ebasis_11;
   b_kk = b_finite_el.k;
   b_nn = b_finite_el.n;
   b_mm = b_finite_el.m;
   for (pn = FIRSTNODE(tGrid); pn; pn = pn->succ)
   if (is_macro_node(pn))
      if (TSTART(pn) != START(pn)){
         edges_for_b_macros(pn,&n,fb,eb,nb);
         for (i = 0; i < n; i++){
            bx[0] = bb0(pn->myvertex->x);
            bx[1] = bb1(pn->myvertex->x);
            delta = lpm_delta(pn,eps,bb0,bb1);
            for (j = 0; j < 2; j++)
               general_local_stiff_matr(tGrid,eb[i][j],Z,
                 eps,bx[0],bx[1],delta,a4,i0,i1,i2,
                 bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,integral,b_finite_el,rmtype,qr,
                 b_kk,b_nn,b_mm,
                 b_nq,b_nq0,b_nq1,b_nq00,b_nq01,b_nq11,
                 b_fq,b_fq0,b_fq1,b_fq00,b_fq01,b_fq11,
                 b_eq,b_eq0,b_eq1,b_eq00,b_eq01,b_eq11);
         }
      }
      else{
         bx[0] = bb0(pn->myvertex->x);
         bx[1] = bb1(pn->myvertex->x);
         delta = lpm_delta(pn,eps,bb0,bb1);
         for (pnel = NESTART(pn); pnel; pnel = pnel->next)
            general_local_stiff_matr(tGrid,pnel->nbel,Z,
                 eps,bx[0],bx[1],delta,a4,i0,i1,i2,
                 bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,integral,i_finite_el,rmtype,qr,
                 i_kk,i_nn,i_mm,
                 i_nq,i_nq0,i_nq1,i_nq00,i_nq01,i_nq11,
                 i_fq,i_fq0,i_fq1,i_fq00,i_fq01,i_fq11,
                 i_eq,i_eq0,i_eq1,i_eq00,i_eq01,i_eq11);
      }
}

#else

void general_macro_stiff_matr(tGrid,Z,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,integral,finite_el,rmtype,qr)
GRID *tGrid; INT Z, rmtype, i0, i1, i2; FLOAT a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)(), (*integral)(); FINITE_ELEMENT finite_el; QUADRATURE_RULE qr;
{  eprintf("Error: general_macro_stiff_matr not available.\n");  }

void general_macro_stiff_matr_b(tGrid,Z,eps,a1,a2,a3,a4,i0,i1,i2,bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,integral,i_finite_el,b_finite_el,rmtype,qr)
GRID *tGrid; INT Z, rmtype, i0, i1, i2; FLOAT eps, a1, a2, a3, a4, (*bb0)(), (*bb1)(), (*f2)(), (*f3)(), (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)(), (*integral)(); FINITE_ELEMENT i_finite_el, b_finite_el; QUADRATURE_RULE qr;
{  eprintf("Error: general_macro_stiff_matr_b not available.\n");  }

#endif

void general_bubble_contribution(tGrid,Z,a0,a1,a2,a3,a4,f,i1,i2,
        f0,f1,f2,f3,f4,f5,f6,f7,rhs,integral_rhs,integral,finite_el,rmtype,qr)
GRID *tGrid;
INT Z, rmtype, f, i1, i2;
FLOAT a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(), (*f5)(),
      (*f6)(), (*f7)(), (*rhs)(), (*integral_rhs)(), (*integral)();
FINITE_ELEMENT finite_el;
QUADRATURE_RULE qr;
{
   ELEMENT *pel;
   DOUBLE_FUNC *nq, *nq0, *nq1, *nq00, *nq01, *nq11,
               *fq, *fq0, *fq1, *fq00, *fq01, *fq11,
               *eq, *eq0, *eq1, *eq00, *eq01, *eq11;
   INT kk, nn, mm;
  
   nq   = finite_el.nbasis;
   nq0  = finite_el.nbasis_0;
   nq1  = finite_el.nbasis_1;
   nq00 = finite_el.nbasis_00;
   nq01 = finite_el.nbasis_01;
   nq11 = finite_el.nbasis_11;
   fq   = finite_el.fbasis;
   fq0  = finite_el.fbasis_0;
   fq1  = finite_el.fbasis_1;
   fq00 = finite_el.fbasis_00;
   fq01 = finite_el.fbasis_01;
   fq11 = finite_el.fbasis_11;
   eq   = finite_el.ebasis;
   eq0  = finite_el.ebasis_0;
   eq1  = finite_el.ebasis_1;
   eq00 = finite_el.ebasis_00;
   eq01 = finite_el.ebasis_01;
   eq11 = finite_el.ebasis_11;
   kk = finite_el.k;
   nn = finite_el.n;
   mm = finite_el.m;
   for (pel = FIRSTELEMENT(tGrid); pel; pel = pel->succ)
     general_local_1_bubble_contrib_to_nodes(tGrid,pel,Z,a0,a1,a2,a3,a4,f,i1,i2,
                     f0,f1,f2,f3,f4,f5,f6,f7,rhs,integral_rhs,integral,
                     finite_el,rmtype,qr,kk,nn,mm,
                     nq,nq0,nq1,nq00,nq01,nq11,
                     fq,fq0,fq1,fq00,fq01,fq11,
                     eq,eq0,eq1,eq00,eq01,eq11);
}

#if (N_DATA & ONE_NODE_MATR) && (N_DATA & Dx1_NODE_FACE_MATR) && (F_DATA & ONE_FACE_MATR) && (F_DATA & IxD_FACE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES) && (N_DATA & VECTOR_NODE_DATA) && (F_DATA & SCALAR_FACE_DATA)

void aijb_fo(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,Z,b0,b1,b2,detB,fmult2)
NODE *n0, *n1, *n2;               /* detB = volume * nu;  rdetB = volume      */
FACE *fa0, *fa1, *fa2;            /* NF matrix, FN matrix multiplied by FMULT */
INT Z;                            /* FF matrix multiplied by fmult2           */
FLOAT nn0[DIM], nn1[DIM], nn2[DIM], b0[DIM2], b1[DIM2], b2[DIM2], 
      detB, fmult2;
{
  NFLINK *pnf;
  FNLINK *pfn;
  FLOAT ann, an1, an2, a11, a22,
        pnn, pn1, pn2, p1n, p2n, 
        ppn, pp1, pp2;
  
  ann = DOT(b0,b0)*detB;
  an1 = DOT(b0,b1)*detB;
  an2 = DOT(b0,b2)*detB;
  
   COEFFN(n0,Z) += ann;
   putaij(n0->tstart,n1,n2,an1,an2,Z);
  
   pnn = -ann/3.0*FMULT;
   pn1 = -an1/3.0*FMULT;
   pn2 = -an2/3.0*FMULT;
   putapij(n0->tnfstart,fa1,fa2,pn1,pn2,Z,nn1,nn2);
   for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
   SET4(COEFF_NFP(pnf,Z),nn0,pnn)
                                          
  a11 = DOT(b1,b1)*detB;
  a22 = DOT(b2,b2)*detB;
  ppn = (ann + a11 + a22)/12.*fmult2;
  pp1 = an1*DOT(nn0,nn1)/6.*fmult2;
  pp2 = an2*DOT(nn0,nn2)/6.*fmult2; 
  
  COEFF_FF(fa0,Z) += ppn;
  putppij(fa0->tfstart,fa1,fa2,pp1,pp2,Z); 

  pnn = -ann/3.*FMULT;
  p1n = -an1/3.*FMULT;
  p2n = -an2/3.*FMULT;
  
  for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
  SET4(COEFF_FNP(pfn,Z),nn0,pnn)

  for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
  SET4(COEFF_FNP(pfn,Z),nn0,p1n)

  for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
  SET4(COEFF_FNP(pfn,Z),nn0,p2n)
}

#else

void aijb_fo(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,Z,b0,b1,b2,detB,fmult2)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z; FLOAT nn0[DIM], nn1[DIM], nn2[DIM], b0[DIM2], b1[DIM2], b2[DIM2], detB, fmult2;
{  eprintf("Error: aijb_fo not available.\n");  }

#endif

#if (N_DATA & ONE_NODE_MATR) && (F_DATA & ONE_FACE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & F_LINK_TO_FACES) && (N_DATA & VECTOR_NODE_DATA) && (F_DATA & SCALAR_FACE_DATA)

/* boundary condition has to be defined be means of pw. linear functions only */

void aijb_ro(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,Z,b0,b1,b2,detB,fmult2)
NODE *n0, *n1, *n2;               /* detB = volume * nu;  rdetB = volume      */
FACE *fa0, *fa1, *fa2;            /* FF matrix multiplied by fmult2           */
INT Z;
FLOAT nn0[DIM], nn1[DIM], nn2[DIM], b0[DIM2], b1[DIM2], b2[DIM2], 
      detB, fmult2;
{
  FLOAT ann, an1, an2, a11, a22, ppn, pp1, pp2;
  
  ann = DOT(b0,b0)*detB;
  an1 = DOT(b0,b1)*detB;
  an2 = DOT(b0,b2)*detB;
  
     COEFFN(n0,Z) += ann;
     putaij(n0->tstart,n1,n2,an1,an2,Z);
                                          
    a11 = DOT(b1,b1)*detB;
    a22 = DOT(b2,b2)*detB;
  
    ppn = (ann + a11 + a22)/12.*fmult2;
    pp1 = an1*DOT(nn0,nn1)/6.*fmult2;
    pp2 = an2*DOT(nn0,nn2)/6.*fmult2; 
  
    COEFF_FF(fa0,Z) += ppn;
    putppij(fa0->tfstart,fa1,fa2,pp1,pp2,Z); 
}

#else

void aijb_ro(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,Z,b0,b1,b2,detB,fmult2)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z; FLOAT nn0[DIM], nn1[DIM], nn2[DIM], b0[DIM2], b1[DIM2], b2[DIM2], detB, fmult2;
{  eprintf("Error: aijb_ro not available.\n");  }

#endif

#if (N_DATA & ONE_NODE_MATR) && (F_DATA & ONE_FACE_MATR) && (DATA_S & N_LINK_TO_NODES) && (N_DATA & VECTOR_NODE_DATA) && (F_DATA & SCALAR_FACE_DATA)

/* boundary condition has to be defined be means of pw. linear functions only */

void aijb_rn(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,Z,b0,b1,b2,detB,fmult2)
NODE *n0, *n1, *n2;               /* detB = volume * nu;  rdetB = volume      */
FACE *fa0, *fa1, *fa2;            /* FF matrix multiplied by fmult2           */
INT Z;
FLOAT nn0[DIM], nn1[DIM], nn2[DIM], b0[DIM2], b1[DIM2], b2[DIM2], 
      detB, fmult2;
{
  FLOAT ann, an1, an2, qnn;
  
  qnn = DOT(b0,b0);
  ann = qnn*detB;
  
     an1 = DOT(b0,b1)*detB;
     an2 = DOT(b0,b2)*detB;
     COEFFN(n0,Z) += ann;
     putaij(n0->tstart,n1,n2,an1,an2,Z);
                                          
     COEFF_FF(fa0,Z) += (DOT(b1,b1) + DOT(b2,b2) + 13.*qnn)/36.*detB*fmult2;
}

#else

void aijb_rn(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,Z,b0,b1,b2,detB,fmult2)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z; FLOAT nn0[DIM], nn1[DIM], nn2[DIM], b0[DIM2], b1[DIM2], b2[DIM2], detB, fmult2;
{  eprintf("Error: aijb_rn not available.\n");  }

#endif

/* if (DATA_STR & REDUCED), then the boundary condition has to be defined be
   means of pw. linear functions only */
void aijb_2D(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,Z,b0,b1,b2,detB,fmult2)
NODE *n0, *n1, *n2;               /* detB = volume * nu                       */
FACE *fa0, *fa1, *fa2;            /* NF matrix, FN matrix multiplied by FMULT */
INT Z;                            /* FF matrix multiplied by fmult2           */
FLOAT nn0[DIM], nn1[DIM], nn2[DIM], b0[DIM2], b1[DIM2], b2[DIM2], 
      detB, fmult2;
{
   if (!(DATA_STR & REDUCED) && !(U_SPACE == P1C_NEW_FBUB))
      aijb_fo(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,Z,b0,b1,b2,detB,fmult2);
   else if ((DATA_STR & REDUCED) && !(U_SPACE == P1C_NEW_FBUB))
      aijb_ro(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,Z,b0,b1,b2,detB,fmult2);
   else if ((DATA_STR & REDUCED) && (U_SPACE == P1C_NEW_FBUB))
      aijb_rn(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,Z,b0,b1,b2,detB,fmult2);
   else
      eprintf("Error: aijb not available.\n");
}

void p1c_fbub_Laplace_matr(tGrid,nu,Z)
GRID *tGrid;
FLOAT nu;
INT Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT nn0[DIM], nn1[DIM], nn2[DIM], rdetB, ndetB, 
         b[DIM2][DIM2], fmult2, mult=1.;

   fmult2 = FMULT2*mult;
    
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      ndetB = nu*rdetB;
      normal_vector(n1->myvertex->x,n2->myvertex->x,fa0,nn0);
      normal_vector(n0->myvertex->x,n2->myvertex->x,fa1,nn1);
      normal_vector(n0->myvertex->x,n1->myvertex->x,fa2,nn2);
      aijb_2D(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,Z,b[0],b[1],b[2],ndetB,fmult2);
      aijb_2D(n1,n2,n0,fa1,fa2,fa0,nn1,nn2,nn0,Z,b[1],b[2],b[0],ndetB,fmult2);
      aijb_2D(n2,n0,n1,fa2,fa0,fa1,nn2,nn0,nn1,Z,b[2],b[0],b[1],ndetB,fmult2);
   } 
}

void p1c_Laplace_stiff_matr(tGrid,nu,Z)
GRID *tGrid;
FLOAT nu;
INT Z;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT ndetB, b[DIM2][DIM2];
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      ndetB = nu*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
      laijb_2D(n0,n1,n2,Z,b[0],b[1],b[2],ndetB);
      laijb_2D(n1,n2,n0,Z,b[1],b[2],b[0],ndetB);
      laijb_2D(n2,n0,n1,Z,b[2],b[0],b[1],ndetB);
   }
}

void p1c_Laplace_stiff_matr_Korn(tGrid,nu,Z)
GRID *tGrid;
FLOAT nu;
INT Z;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT ndetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      ndetB = nu*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
      aij_sn_Korn(n0,n1,n2,Z,b[0],b[1],b[2],ndetB);
      aij_sn_Korn(n1,n2,n0,Z,b[1],b[2],b[0],ndetB);
      aij_sn_Korn(n2,n0,n1,Z,b[2],b[0],b[1],ndetB);
   }
}

void q1c_Laplace_stiff_matr(tGrid,nu,Z,qr)
GRID *tGrid;
FLOAT nu;
INT Z;
QUADRATURE_RULE qr;
{
   NODE *n0, *n1, *n2, *n3;
   ELEMENT *pelem;
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_4ELEMENT(n0,n1,n2,n3,pelem);
      q1_aij(n0,n1,n2,n3,Z,nu,qr);
      q1_aij(n3,n0,n1,n2,Z,nu,qr);
      q1_aij(n2,n3,n0,n1,Z,nu,qr);
      q1_aij(n1,n2,n3,n0,Z,nu,qr);
   }
}

void q1c_Laplace_stiff_matr_Korn(tGrid,nu,Z,qr)
GRID *tGrid; FLOAT nu; INT Z; QUADRATURE_RULE qr;
{  eprintf("Error: q1c_Laplace_stiff_matr_Korn not available.\n");  }

#if N_DATA & VECTOR_NODE_DATA

void p1c_up_ij();

void p1c_upwind_matr(tGrid,nu,v,Z)
GRID *tGrid;
FLOAT nu;
INT v, Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT c[DIM], a0, a1, a2, b0=0., b1=0., b2=0., b[DIM2][DIM2], rdetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      if (IS_BF(fa0))
         b0 = -( (ND(n1,v,0)+ND(n2,v,0))*b[0][0] 
               + (ND(n1,v,1)+ND(n2,v,1))*b[0][1] )*rdetB;
      if (IS_BF(fa1))
         b1 = -( (ND(n0,v,0)+ND(n2,v,0))*b[1][0] 
               + (ND(n0,v,1)+ND(n2,v,1))*b[1][1] )*rdetB;
      if (IS_BF(fa2))
         b2 = -( (ND(n0,v,0)+ND(n1,v,0))*b[2][0] 
               + (ND(n0,v,1)+ND(n1,v,1))*b[2][1] )*rdetB;
      c[0] = (ND(n0,v,0) +  ND(n1,v,0) +  ND(n2,v,0))/3.;
      c[1] = (ND(n0,v,1) +  ND(n1,v,1) +  ND(n2,v,1))/3.;
      rdetB /= 3.;
      a0 = ( (ND(n0,v,0)+c[0])*(b[1][0]-b[2][0])
           + (ND(n0,v,1)+c[1])*(b[1][1]-b[2][1]) )*rdetB;
      a1 = ( (ND(n1,v,0)+c[0])*(b[2][0]-b[0][0])
           + (ND(n1,v,1)+c[1])*(b[2][1]-b[0][1]) )*rdetB;
      a2 = ( (ND(n2,v,0)+c[0])*(b[0][0]-b[1][0])
           + (ND(n2,v,1)+c[1])*(b[0][1]-b[1][1]) )*rdetB;
      p1c_up_ij(n0,n1,n2,fa0,nu,Z,b0,a2,-a1);
      p1c_up_ij(n1,n2,n0,fa1,nu,Z,b1,a0,-a2);
      p1c_up_ij(n2,n0,n1,fa2,nu,Z,b2,a1,-a0);
   } 
}

#else

void p1c_upwind_matr(tGrid,nu,v,Z)
GRID *tGrid; FLOAT nu; INT v, Z;
{  eprintf("Error: p1c_upwind_matr not available.\n");  }

#endif

#if (E_DATA & SCALAR_ELEMENT_DATA) && (ELEMENT_TYPE == SIMPLEX)

void mini_Laplace_stiff_matr(tGrid,nu,Z)
GRID *tGrid;
FLOAT nu;
INT Z;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT ndetB, b[DIM2][DIM2], mult=1.;
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      ndetB = nu*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
      laijb_2D(n0,n1,n2,Z,b[0],b[1],b[2],ndetB);
      laijb_2D(n1,n2,n0,Z,b[1],b[2],b[0],ndetB);
      laijb_2D(n2,n0,n1,Z,b[2],b[0],b[1],ndetB);
      ED(pelem,Z) = (DOT(b[0],b[0])+DOT(b[1],b[1])+
                                      DOT(b[2],b[2]))*ndetB*mult*FMULT2/180.;
   }
}

#else

void mini_Laplace_stiff_matr(tGrid,nu,Z)
GRID *tGrid; FLOAT nu; INT Z;
{  eprintf("Error: mini_Laplace_stiff_matr not available.\n");  }

#endif

#if (ELEMENT_TYPE == SIMPLEX) && (E_DATA & ExE_MATR)

void add_P1CB_Laplace_matr(tGrid,nu,Z)
GRID *tGrid;
FLOAT nu;
INT Z;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT ndetB, b[DIM2][DIM2];
  
   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      ndetB = nu*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
      laijb_2D(n0,n1,n2,Z,b[0],b[1],b[2],ndetB);
      laijb_2D(n1,n2,n0,Z,b[1],b[2],b[0],ndetB);
      laijb_2D(n2,n0,n1,Z,b[2],b[0],b[1],ndetB);
      COEFF_EE(pelem,Z) += (DOT(b[0],b[0])+DOT(b[1],b[1])
                                          +DOT(b[2],b[2]))*ndetB/180.;
   }
}

void add_P1CB_lp_term_matr(tGrid,Z,eps,bb0,bb1) /*  local projection term  */
GRID *tGrid;
INT Z;
FLOAT eps, (*bb0)(), (*bb1)();
{
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, b[DIM2][DIM2], rdetB;
  
   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      rdetB = barycentric_coordinates(x0,x1,x2,b); 
      COEFF_EE(pelem,Z) += lp_delta(pelem,eps,bb0,bb1)*(DOT(b[0],b[0])+
                                      DOT(b[1],b[1])+DOT(b[2],b[2]))*rdetB/180.;
   }
}

#else

void add_P1CB_Laplace_matr(tGrid,nu,Z)
GRID *tGrid; FLOAT nu; INT Z;
{  eprintf("Error: add_P1CB_Laplace_matr not available.\n");  }

void add_P1CB_lp_term_matr(tGrid,Z,eps,bb0,bb1)
GRID *tGrid; INT Z; FLOAT eps, (*bb0)(), (*bb1)();
{  eprintf("Error: add_P1CB_lp_term_matr not available.\n");  }

#endif

void mini_lin_div_free_Laplace_stiff_matr(tGrid,nu,Z)
GRID *tGrid;
FLOAT nu;
INT Z;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT ndetB, b[DIM2][DIM2], c;
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      ndetB = nu*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
      c = (DOT(b[0],b[0])+DOT(b[1],b[1])+DOT(b[2],b[2]))*20./9.;
      aij_mini_lin_div_free(n0,n1,n2,Z,b[0],b[1],b[2],c,ndetB);
      aij_mini_lin_div_free(n1,n2,n0,Z,b[1],b[2],b[0],c,ndetB);
      aij_mini_lin_div_free(n2,n0,n1,Z,b[2],b[0],b[1],c,ndetB);
   }
}

/******************************************************************************/
/*                                                                            */
/*     conv. term for P1C scalar conv.-diff. eq. with P1C_FBUB velocity       */
/*                                                                            */
/******************************************************************************/

#if (F_DATA & SCALAR_FACE_DATA) && (N_DATA & VECTOR_NODE_DATA) && (N_DATA & SCALAR_NODE_DATA) && (N_DATA & ONE_NODE_MATR) && (ELEMENT_TYPE == SIMPLEX)

/*  stiffness matrix on one grid corresponding to 
                                       nu*\int_\om \nabla u\cdot\nabla v \dx  */
void snijb(n0,n1,n2,vn,sumv,mn,summ,Z,f,u,b0,b1,b2,detB,t)
NODE *n0, *n1, *n2;
INT Z, f, u, t;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], detB, 
      vn[DIM], sumv[DIM], mn[DIM], summ[DIM];
{
   LINK *plin, *pli;
   FLOAT ann, an1, an2, sum[DIM];
  
   if (!IS_DIR(n0,t)){
      sum[0] = (vn[0]+sumv[0] + (summ[0]+summ[0]-mn[0])*FMULT/5.)*detB/12.;
      sum[1] = (vn[1]+sumv[1] + (summ[1]+summ[1]-mn[1])*FMULT/5.)*detB/12.;
      ann = DOT(sum,b0);
      an1 = DOT(sum,b1);
      an2 = DOT(sum,b2);

      COEFFS(n0,Z) += ann;
      plin = TSTART(n0);
      if (!IS_DIR(n1,t) && !IS_DIR(n2,t))
         sputaij(plin,n1,n2,an1,an2,Z);
      else if (!IS_DIR(n1,t)){
         for (pli=plin; NBNODE(pli) != n1 ; pli=pli->next);
         COEFFLS(pli,Z) += an1;
         NDS(n0,f) -= an2*NDS(n2,u);
      }                                         
      else if (!IS_DIR(n2,t)){
         for (pli=plin; NBNODE(pli) != n2 ; pli=pli->next);
         COEFFLS(pli,Z) += an2; 
         NDS(n0,f) -= an1*NDS(n1,u);
      }                                       
      else
         NDS(n0,f) -= an1*NDS(n1,u) + an2*NDS(n2,u);

   }
}
  
/*  stiffness matrix on one grid corresponding to 
                                          \int_\om u_i(v\cdot\nabla u_j) \dx  */
void sn_matr(tGrid,Z,f,u,v,t)
GRID *tGrid;
INT Z,f,u,t;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT b[DIM2][DIM2], nn0[DIM], nn1[DIM], nn2[DIM],
         v0[DIM], v1[DIM], v2[DIM], m0[DIM], m1[DIM], m2[DIM], 
         sumv[DIM], summ[DIM], detB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      detB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                     n2->myvertex->x,b);
      normal_vector(n1->myvertex->x,n2->myvertex->x,fa0,nn0);
      normal_vector(n0->myvertex->x,n2->myvertex->x,fa1,nn1);
      normal_vector(n0->myvertex->x,n1->myvertex->x,fa2,nn2);
      SET1(v0,NDD(n0,v))
      SET1(v1,NDD(n1,v))
      SET1(v2,NDD(n2,v))
      SET2(m0,nn0,FD(fa0,v))
      SET2(m1,nn1,FD(fa1,v))
      SET2(m2,nn2,FD(fa2,v))
      SET14(sumv,v0,v1,v2);
      SET14(summ,m0,m1,m2);
      snijb(n0,n1,n2,v0,sumv,m0,summ,Z,f,u,b[0],b[1],b[2],detB,t);
      snijb(n1,n2,n0,v1,sumv,m1,summ,Z,f,u,b[1],b[2],b[0],detB,t);
      snijb(n2,n0,n1,v2,sumv,m2,summ,Z,f,u,b[2],b[0],b[1],detB,t);
   }
}

#else

void sn_matr(tGrid,Z,f,u,v,t)
GRID *tGrid; INT Z,f,u,t;
{  eprintf("Error: sn_matr not available.\n");  }

#endif

/******************************************************************************/

#if ELEMENT_TYPE == SIMPLEX

void smass_matr_P2(tGrid,Z,tau)
GRID *tGrid;
INT Z;
FLOAT tau;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB;

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = VOLUME(pelem)/tau;
      cij_sn_sf(n0,n1,n2,fa0,fa1,fa2,Z,rdetB);
      cij_sn_sf(n1,n2,n0,fa1,fa2,fa0,Z,rdetB);
      cij_sn_sf(n2,n0,n1,fa2,fa0,fa1,Z,rdetB);
   }
}

void p2c_Laplace_stiff_matr_Korn(tGrid,nu,Z)
GRID *tGrid;
FLOAT nu;
INT Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT ndetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      ndetB = nu*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
      aij_sn_sf_Korn(n0,n1,n2,fa0,fa1,fa2,Z,b[0],b[1],b[2],ndetB);
      aij_sn_sf_Korn(n1,n2,n0,fa1,fa2,fa0,Z,b[1],b[2],b[0],ndetB);
      aij_sn_sf_Korn(n2,n0,n1,fa2,fa0,fa1,Z,b[2],b[0],b[1],ndetB);
   }
}

#else

void smass_matr_P2(tGrid,Z,tau)
GRID *tGrid; INT Z; FLOAT tau;
{  eprintf("Error: smass_matr_P2 not available.\n");  }

void p2c_Laplace_stiff_matr_Korn(tGrid,nu,Z)
GRID *tGrid; FLOAT nu; INT Z;
{  eprintf("Error: p2c_Laplace_stiff_matr_Korn not available.\n");  }

#endif

#if (ELEMENT_TYPE == SIMPLEX) && (E_DATA & ExN_MATR) && (E_DATA & NxE_MATR) && (E_DATA & ExF_MATR) && (E_DATA & FxE_MATR) && (E_DATA & ExE_MATR)

void smass_matr_p2c_elbub(tGrid,Z,tau)
GRID *tGrid;
INT Z;
FLOAT tau;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, r;

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = VOLUME(pelem)/tau;
      cij_sn_sf(n0,n1,n2,fa0,fa1,fa2,Z,rdetB);
      cij_sn_sf(n1,n2,n0,fa1,fa2,fa0,Z,rdetB);
      cij_sn_sf(n2,n0,n1,fa2,fa0,fa1,Z,rdetB);
      r = rdetB/180.;
      SET26(COEFF_NEP(pelem,Z),r)
      SET26(COEFF_ENP(pelem,Z),r)
      r = rdetB/630.;
      SET26(COEFF_FEP(pelem,Z),r)
      SET26(COEFF_EFP(pelem,Z),r)
      COEFF_EE(pelem,Z) += rdetB/2520.;
   }
}

#else

void smass_matr_p2c_elbub(tGrid,Z,tau)
GRID *tGrid; INT Z; FLOAT tau;
{  eprintf("Error: smass_matr_p2c_elbub not available.\n");  }

#endif

#if (F_DATA & CURVED_FACE_MIDDLE) && (ELEMENT_TYPE == SIMPLEX)

void p2c_Laplace_stiff_matr_iso(tGrid,nu,Z,bubble,mstruct,qr)
GRID *tGrid;
FLOAT nu;
INT Z, bubble, mstruct;
QUADRATURE_RULE qr;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT ndetB, b[DIM2][DIM2], m[2][2][DIM2], jac[DIM2], a0, a1, a2, a;

   if (mstruct & Q_FULL)
      eprintf("Error: p2c_Laplace_stiff_matr_iso not implemented for (mstruct & Q_FULL).\n");
   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      if (fa0->c_midpoint || fa1->c_midpoint || fa2->c_midpoint){
         aij_sn_sf_iso(n0,n1,n2,fa0,fa1,fa2,Z,nu,qr);
         aij_sn_sf_iso(n1,n2,n0,fa1,fa2,fa0,Z,nu,qr);
         aij_sn_sf_iso(n2,n0,n1,fa2,fa0,fa1,Z,nu,qr);
         if (bubble){
            inverse_of_P2_reference_mapping(n0,n1,n2,fa0,fa1,fa2,m,jac);
            fill_symmetric_e_matrices(pelem,Z,
               nu*gu_gv_ref(r_l0_0,r_l0_1,r_l0l1l2_0,r_l0l1l2_1,m,jac,qr),
               nu*gu_gv_ref(r_l1_0,r_l1_1,r_l0l1l2_0,r_l0l1l2_1,m,jac,qr),
               nu*gu_gv_ref(r_l2_0,r_l2_1,r_l0l1l2_0,r_l0l1l2_1,m,jac,qr),
               nu*gu_gv_ref(r_l1l2_0,r_l1l2_1,r_l0l1l2_0,r_l0l1l2_1,m,jac,qr),
               nu*gu_gv_ref(r_l0l2_0,r_l0l2_1,r_l0l1l2_0,r_l0l1l2_1,m,jac,qr),
               nu*gu_gv_ref(r_l0l1_0,r_l0l1_1,r_l0l1l2_0,r_l0l1l2_1,m,jac,qr),
               nu*gu_gv_ref(r_l0l1l2_0,r_l0l1l2_1,r_l0l1l2_0,r_l0l1l2_1,
                                                                    m,jac,qr));
         }
      }
      else{
         ndetB = nu*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                            n2->myvertex->x,b);
         aij_sn_sf(n0,n1,n2,fa0,fa1,fa2,Z,b[0],b[1],b[2],ndetB,mstruct);
         aij_sn_sf(n1,n2,n0,fa1,fa2,fa0,Z,b[1],b[2],b[0],ndetB,mstruct);
         aij_sn_sf(n2,n0,n1,fa2,fa0,fa1,Z,b[2],b[0],b[1],ndetB,mstruct);
         if (bubble){
            a0 = DOT(b[0],b[0]);
            a1 = DOT(b[1],b[1]);
            a2 = DOT(b[2],b[2]);
            a = a0 + a1 + a2;
            ndetB /= 60.;
            fill_symmetric_e_matrices(pelem,Z,0.,0.,0.,
                    (a-a0-a0)*ndetB,(a-a1-a1)*ndetB,(a-a2-a2)*ndetB,a*ndetB/3.);
         }
      }
   }
}

void p2c_Laplace_stiff_matr_Korn_iso(tGrid,nu,Z)
GRID *tGrid; FLOAT nu; INT Z;
{  eprintf("Error: p2c_Laplace_stiff_matr_Korn_iso not available.\n");  }

#else

void p2c_Laplace_stiff_matr_iso(tGrid,nu,Z,bubble,mstruct,qr)
GRID *tGrid; FLOAT nu; INT Z, bubble, mstruct; QUADRATURE_RULE qr;
{  eprintf("Error: p2c_Laplace_stiff_matr_iso not available.\n");  }

void p2c_Laplace_stiff_matr_Korn_iso(tGrid,nu,Z)
GRID *tGrid; FLOAT nu; INT Z;
{  eprintf("Error: p2c_Laplace_stiff_matr_Korn_iso not available.\n");  }

#endif

void p2c_Laplace_stiff_matr(tGrid,nu,Z,bubble,mstruct)
GRID *tGrid;
FLOAT nu;
INT Z, bubble, mstruct;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT ndetB, b[DIM2][DIM2], a0, a1, a2, a;

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      ndetB = nu*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
      aij_sn_sf(n0,n1,n2,fa0,fa1,fa2,Z,b[0],b[1],b[2],ndetB,mstruct);
      aij_sn_sf(n1,n2,n0,fa1,fa2,fa0,Z,b[1],b[2],b[0],ndetB,mstruct);
      aij_sn_sf(n2,n0,n1,fa2,fa0,fa1,Z,b[2],b[0],b[1],ndetB,mstruct);
      if (bubble && !(mstruct & Q_FULL)){
         a0 = DOT(b[0],b[0]);
         a1 = DOT(b[1],b[1]);
         a2 = DOT(b[2],b[2]);
         a = a0 + a1 + a2;
         ndetB /= 60.;
         fill_symmetric_e_matrices(pelem,Z,0.,0.,0.,
                    (a-a0-a0)*ndetB,(a-a1-a1)*ndetB,(a-a2-a2)*ndetB,a*ndetB/3.);
      }
      else if (bubble && (mstruct & Q_FULL))
         eprintf("Error: bubble part not implemented in p2c_Laplace_stiff_matr for (mstruct & Q_FULL).\n");
   }
}

/*  stiffness matrix corresponding to \int_\om u_i(bb\cdot\nabla u_j) \dx  */
void add_P2C_conv_term_matr(tGrid,Z,bb0,bb1,mstruct)
GRID *tGrid;
INT Z, mstruct;
FLOAT (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, x01[DIM], x02[DIM], x12[DIM], vn[DIM2][DIM], 
         vf[DIM2][DIM], vb[DIM], s[DIM], c[DIM], rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      MIDPOINTS(x0,x1,x2,x01,x02,x12)
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      V_POINT_VALUES_6(  x0,   x1,   x2,   x01,  x02,  x12,
                       vn[0],vn[1],vn[2],vf[2],vf[1],vf[0],bb0,bb1)
      SET27(vf[0],vf[0],4.,vn[1],-2.,vn[2],-2.)
      SET27(vf[1],vf[1],4.,vn[2],-2.,vn[0],-2.)
      SET27(vf[2],vf[2],4.,vn[0],-2.,vn[1],-2.)
      SET14(s,vn[0],vn[1],vn[2]);
      SET14(c,vf[0],vf[1],vf[2]);
      convij_sn_sf(pelem,n0,n1,n2,fa0,fa1,fa2,0,1,2,vn,vf,vb,s,c,Z,b[0],b[1],b[2],rdetB,0,mstruct);
      convij_sn_sf(pelem,n1,n2,n0,fa1,fa2,fa0,1,2,0,vn,vf,vb,s,c,Z,b[1],b[2],b[0],rdetB,0,mstruct);
      convij_sn_sf(pelem,n2,n0,n1,fa2,fa0,fa1,2,0,1,vn,vf,vb,s,c,Z,b[2],b[0],b[1],rdetB,0,mstruct);
   }
}

void add_P2C_react_term_matr(tGrid,Z,r)
GRID *tGrid;
INT Z;
FLOAT (*r)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, x01[DIM], x02[DIM], x12[DIM], rn[DIM2], rf[DIM2],
         c, s, rdetB;

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      MIDPOINTS(x0,x1,x2,x01,x02,x12)
      POINT_VALUES_6(  x0,   x1,   x2,   x01,  x02,  x12,
                     rn[0],rn[1],rn[2],rf[2],rf[1],rf[0],r)
      rf[0] *= 4.;
      rf[1] *= 4.;
      rf[2] *= 4.;
      c = rn[0] + rn[1] + rn[2];
      s = rf[0] + rf[1] + rf[2];
      rdetB = VOLUME(pelem);
      r_p2c_ij(n0,n1,n2,fa0,fa1,fa2,0,1,2,rn,rf,c,s,Z,rdetB);
      r_p2c_ij(n1,n2,n0,fa1,fa2,fa0,1,2,0,rn,rf,c,s,Z,rdetB);
      r_p2c_ij(n2,n0,n1,fa2,fa0,fa1,2,0,1,rn,rf,c,s,Z,rdetB);
   }
}

void compute_vn_vf_s_c(v0,v1,v2,v01,v02,v12,b,vn,vf,s,c)
FLOAT v0[DIM], v1[DIM], v2[DIM], v01[DIM], v02[DIM], v12[DIM], b[DIM2][DIM2],
      vn[SIDES][SIDES], vf[SIDES][SIDES], s[SIDES], c[SIDES];
{
   vn[0][0] = DOT(v0,b[0]);
   vn[0][1] = DOT(v0,b[1]);
   vn[0][2] = DOT(v0,b[2]);
   vn[1][0] = DOT(v1,b[0]);
   vn[1][1] = DOT(v1,b[1]);
   vn[1][2] = DOT(v1,b[2]);
   vn[2][0] = DOT(v2,b[0]);
   vn[2][1] = DOT(v2,b[1]);
   vn[2][2] = DOT(v2,b[2]);
   vf[0][0] = DOT(v12,b[0]);
   vf[0][1] = DOT(v12,b[1]);
   vf[0][2] = DOT(v12,b[2]);
   vf[1][0] = DOT(v02,b[0]);
   vf[1][1] = DOT(v02,b[1]);
   vf[1][2] = DOT(v02,b[2]);
   vf[2][0] = DOT(v01,b[0]);
   vf[2][1] = DOT(v01,b[1]);
   vf[2][2] = DOT(v01,b[2]);
   s[0] = vn[0][0] + vn[1][0] + vn[2][0];
   s[1] = vn[0][1] + vn[1][1] + vn[2][1];
   s[2] = vn[0][2] + vn[1][2] + vn[2][2];
   c[0] = vf[0][0] + vf[1][0] + vf[2][0];
   c[1] = vf[0][1] + vf[1][1] + vf[2][1];
   c[2] = vf[0][2] + vf[1][2] + vf[2][2];
}

/*
FLOAT sd_tau_classic(y,pelem,bar,eps,bb0,bb1,react,rhs,u,space,sc_type,
                     par1,par2)
ELEMENT *pelem;
FLOAT *y, bar[DIM2][DIM2], eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(),
      par1, par2;
INT u, space, sc_type;
{
   FLOAT bb0y=bb0(y), bb1y=bb1(y), bb, hK, Pe;
   INT order=approx_order(space);

   bb = sqrt(bb0y*bb0y + bb1y*bb1y);
   if (DIR_DIAM == YES)
      hK = sdiameter(pelem,bar,bb0y,bb1y);
   else
      hK = diameter(pelem);
   Pe = bb*hK/(2.*eps*order);
   if (Pe < 1.e-10)
      return(hK*hK/(12.*eps*order*order));
   else
      return(hK/(2.*bb*order)*(1./tanh(Pe) - 1./Pe));
}
*/

FLOAT sd_tau_classic(pelem,bar,eps,bb0y,bb1y,space)
ELEMENT *pelem;
FLOAT bar[DIM2][DIM2], eps, bb0y, bb1y;
INT space;
{
   FLOAT bb, hK, Pe, h=0.1;
   INT order=approx_order(space);

   bb = sqrt(bb0y*bb0y + bb1y*bb1y);
   if (DIR_DIAM == YES)
      hK = sdiameter(pelem,bar,bb0y,bb1y);
   else
      hK = diameter(pelem);
   Pe = bb*hK/(2.*eps*order);
/*
   if (VERTEX_ON_LINE(pelem,0,1.))
      return(h);
   else if (VERTEX_ON_LINE(pelem,1,0.))
      return(h/sqrt(3.));
   else
*/
   if (Pe < 1.e-10)
      return(hK*hK/(12.*eps*order*order));
   else
      return(hK/(2.*bb*order)*(1./tanh(Pe) - 1./Pe));
   if (0){
      if (EDGE_ON_LINE(pelem,0,1.)) 
         return(hK);
      else if (VERTEX_ON_LINE(pelem,0,1.))
         return(hK/3.);
      else
         return(hK/(2.*bb*order)*(1./tanh(Pe) - 1./Pe));
   }
}

FLOAT sd_tau_shih_elman1(pelem,bar,eps,bb0y,bb1y)
ELEMENT *pelem;
FLOAT bar[DIM2][DIM2], eps, bb0y, bb1y;
{
   FLOAT bb, h, Pe0, Pe1, Pe, s, tau;

   h = sdiameter(pelem,bar,1.,0.);
   if (fabs(bb0y) < 1.e-10 || fabs(bb1y) < 1.e-10){
      bb = fabs(bb0y) + fabs(bb1y);
      Pe = bb*h/(2.*eps);
      tau = h/(2.*bb)*(1./tanh(Pe) - 1./Pe);
   }
   else{
      Pe0 = bb0y*h/(2.*eps);
      s = bb0y*bb0y - bb1y*bb1y;
      if (fabs(s) < 1.e-10){
         Pe0 = fabs(Pe0);
         s   = 1./tanh(Pe0);
         tau = ( 0.5*(3.*s + Pe0*(1.-s*s)) - 1./Pe0 )*h/(4.*fabs(bb0y));
      }
      else{
         Pe1 = bb1y*h/(2.*eps);
         bb = sqrt(bb0y*bb0y + bb1y*bb1y);
         Pe = bb*h/(2.*eps);
         s  = (bb0y*bb0y*bb0y/tanh(Pe0) - bb1y*bb1y*bb1y/tanh(Pe1))/(bb*s);
         tau = h/(2.*bb)*(s - 1./Pe);
      }
   }
   return(tau);
}

#if E_DATA & E_TAU
#define RETURN_TAU_ON(pelem)     return(pelem->tau);
#else
#define RETURN_TAU_ON(pelem)   { eprintf("Error: elem-tau not available.\n");  \
                                 return(0.); }
#endif

FLOAT sd_delta(pelem,eps,bb0,bb1)
ELEMENT *pelem;
FLOAT eps, (*bb0)(), (*bb1)();
{
   FLOAT xc[DIM], b[DIM2][DIM2], b_norm, hT;

   if (DELTA_TYPE == OUTFLOW_D){
      #if (E_DATA & SCALAR_ELEMENT_DATA)
      return(ED(pelem,TAU_VARIABLE));
      #else
      eprintf("Error: sd_delta not available.\n");
      return(0.);
      #endif
   }
   else if (DELTA_TYPE == CLASSIC_D){
      coord_of_barycentre(pelem,xc);
      barycentric_coordinates(pelem->n[0]->myvertex->x,
                              pelem->n[1]->myvertex->x,
                              pelem->n[2]->myvertex->x,b); 
      return(sd_tau_classic(pelem,b,eps,bb0(xc),bb1(xc),U_SPACE));
   }
   else if (DELTA_TYPE == BASIC_D1 || DELTA_TYPE == BASIC_D2){
      b_norm = vnorm_on_element(pelem,bb0,bb1);
      hT = diameter(pelem);
      if (hT*b_norm < eps)
         return(0.);
      else if (DELTA_TYPE == BASIC_D1)
         return(hT*DELTA_FACTOR);
      else if (DELTA_TYPE == BASIC_D2)
         return(hT*DELTA_FACTOR/b_norm);
   }
   else{
      eprintf("Error: sd_delta not available.\n");
      return(0.);
   }
}

FLOAT sd_p1c_delta(pelem,eps,bb0,bb1,tau_index,node)
ELEMENT *pelem; 
INT tau_index;
NODE *node;
FLOAT eps, (*bb0)(), (*bb1)();
{
   FLOAT xc[DIM], b[DIM2][DIM2], b_norm, hT;

   if (DELTA_TYPE == OUTFLOW_D){
      #if (N_DATA & SCALAR_NODE_DATA) && (TAU_SPACE == P1C)
      return(NDS(node,tau_index));
      #else
      eprintf("Error: sd_p1c_delta not available.\n");
      return(0.);
      #endif
   }
   else if (DELTA_TYPE == CLASSIC_D){
      coord_of_barycentre(pelem,xc);
      barycentric_coordinates(pelem->n[0]->myvertex->x,
                              pelem->n[1]->myvertex->x,
                              pelem->n[2]->myvertex->x,b); 
      return(sd_tau_classic(pelem,b,eps,bb0(xc),bb1(xc),U_SPACE));
   }
   else if (DELTA_TYPE == BASIC_D1 || DELTA_TYPE == BASIC_D2){
      b_norm = vnorm_on_element(pelem,bb0,bb1);
      hT = diameter(pelem);
      if (hT*b_norm < eps)
         return(0.);
      else if (DELTA_TYPE == BASIC_D1)
         return(hT*DELTA_FACTOR);
      else if (DELTA_TYPE == BASIC_D2)
         return(hT*DELTA_FACTOR/b_norm);
   }
   else{
      eprintf("Error: sd_p1c_delta not available.\n");
      return(0.);
   }
}

FLOAT sd_p1_nc_delta(pelem,eps,bb0,bb1,tau_index,node)
ELEMENT *pelem; 
INT tau_index, node;
FLOAT eps, (*bb0)(), (*bb1)();
{
   FLOAT xc[DIM], b[DIM2][DIM2], b_norm, hT;

   if (DELTA_TYPE == OUTFLOW_D){
      #if (E_DATA & SCALAR_DATA_IN_ELEMENT_NODES) && (TAU_SPACE == P1_NC)
      return(EDSN(pelem,tau_index,node));
      #else
      eprintf("Error: sd_p1_nc_delta not available.\n");
      return(0.);
      #endif
   }
   else if (DELTA_TYPE == CLASSIC_D){
      coord_of_barycentre(pelem,xc);
      barycentric_coordinates(pelem->n[0]->myvertex->x,
                              pelem->n[1]->myvertex->x,
                              pelem->n[2]->myvertex->x,b); 
      return(sd_tau_classic(pelem,b,eps,bb0(xc),bb1(xc),U_SPACE));
   }
   else if (DELTA_TYPE == BASIC_D1 || DELTA_TYPE == BASIC_D2){
      b_norm = vnorm_on_element(pelem,bb0,bb1);
      hT = diameter(pelem);
      if (hT*b_norm < eps)
         return(0.);
      else if (DELTA_TYPE == BASIC_D1)
         return(hT*DELTA_FACTOR);
      else if (DELTA_TYPE == BASIC_D2)
         return(hT*DELTA_FACTOR/b_norm);
   }
   else{
      eprintf("Error: sd_p1_nc_delta not available.\n");
      return(0.);
   }
}


#if (E_DATA & SCALAR_ELEMENT_DATA) && (PAR_TYPE == Q_SE)

void set_tau(tGrid,eps,bb0,bb1,v)
GRID *tGrid;
FLOAT eps, (*bb0)(), (*bb1)();
INT v;
{
   ELEMENT *pelem;
   FLOAT xc[DIM], b[DIM2][DIM2], b_norm, hT;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      coord_of_barycentre(pelem,xc);
      barycentric_coordinates(pelem->n[0]->myvertex->x,
                              pelem->n[1]->myvertex->x,
                              pelem->n[2]->myvertex->x,b);
      ED(pelem,v) = sd_tau_classic(pelem,b,eps,bb0(xc),bb1(xc),U_SPACE);
   }
}

#elif (E_DATA & VECTOR_ELEMENT_DATA) && (PAR_TYPE == Q_VE)

void set_tau(tGrid,eps,bb0,bb1,v)
GRID *tGrid;
FLOAT eps, (*bb0)(), (*bb1)();
INT v;
{
   ELEMENT *pelem;
   FLOAT xc[DIM], b[DIM2][DIM2], b_norm, hT;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      coord_of_barycentre(pelem,xc);
      barycentric_coordinates(pelem->n[0]->myvertex->x,
                              pelem->n[1]->myvertex->x,
                              pelem->n[2]->myvertex->x,b); 
      EDV(pelem,v,0) = sd_tau_classic(pelem,b,eps,bb0(xc),bb1(xc),U_SPACE);
      EDV(pelem,v,1) = 0.;
//printf("tau = %e\n",EDV(pelem,v,0));
   }
}

#elif (E_DATA & SCALAR_ELEMENT_DATA) && (TAU_SPACE == P0)

void set_tau(tGrid,eps,bb0,bb1,v)
GRID *tGrid;
FLOAT eps, (*bb0)(), (*bb1)();
INT v;
{
   ELEMENT *pelem;
   FLOAT xc[DIM], b[DIM2][DIM2], b_norm, hT;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      coord_of_barycentre(pelem,xc);
      barycentric_coordinates(pelem->n[0]->myvertex->x,
                              pelem->n[1]->myvertex->x,
                              pelem->n[2]->myvertex->x,b); 
      ED(pelem,v) = sd_tau_classic(pelem,b,eps,bb0(xc),bb1(xc),U_SPACE);
   }
}

#elif (E_DATA & SCALAR_DATA_IN_ELEMENT_NODES) && (TAU_SPACE == P1_NC)

void set_tau(tGrid,eps,bb0,bb1,v)
GRID *tGrid;
FLOAT eps, (*bb0)(), (*bb1)();
INT v;
{
   ELEMENT *pelem;
   FLOAT xc[DIM], b[DIM2][DIM2], b_norm, hT;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      coord_of_barycentre(pelem,xc);
      barycentric_coordinates(pelem->n[0]->myvertex->x,
                              pelem->n[1]->myvertex->x,
                              pelem->n[2]->myvertex->x,b); 
      EDSN(pelem,v,0) = sd_tau_classic(pelem,b,eps,bb0(xc),bb1(xc),U_SPACE);
      EDSN(pelem,v,1) = sd_tau_classic(pelem,b,eps,bb0(xc),bb1(xc),U_SPACE);
      EDSN(pelem,v,2) = sd_tau_classic(pelem,b,eps,bb0(xc),bb1(xc),U_SPACE);
   }
}

#elif (N_DATA & SCALAR_NODE_DATA) && (TAU_SPACE == P1C)

void set_tau(tGrid,eps,bb0,bb1,v)
GRID *tGrid;
FLOAT eps, (*bb0)(), (*bb1)();
INT v;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT xc[DIM], b[DIM2][DIM2], b_norm, hT;

   for (n0 = FIRSTNODE(tGrid); n0; n0 = n0->succ){
      NDS(n0,v) = 0.;
   }
   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      coord_of_barycentre(pelem,xc);
      barycentric_coordinates(pelem->n[0]->myvertex->x,
                              pelem->n[1]->myvertex->x,
                              pelem->n[2]->myvertex->x,b); 
      NDS(n0,v) = NDS(n0,v) + sd_tau_classic(pelem,b,eps,bb0(xc),bb1(xc),U_SPACE)/3.;
      NDS(n1,v) = NDS(n1,v) + sd_tau_classic(pelem,b,eps,bb0(xc),bb1(xc),U_SPACE)/3.;
      NDS(n2,v) = NDS(n2,v) + sd_tau_classic(pelem,b,eps,bb0(xc),bb1(xc),U_SPACE)/3.;
   }
}

#else

void set_tau(tGrid,eps,bb0,bb1,v)
GRID *tGrid; FLOAT eps, (*bb0)(), (*bb1)(); INT v;
{  eprintf("Error: set_tau not available.\n");  }

#endif







// Na vymazani (vse az do pristi velke mezery),
// zadne E_TAU ani E_TAU_SOLD by se nemeli pouzivat, misto toho
// proste promenna v nejakem prostoru. Co kdybychom pak chteli vice ruznych stavu
// si ulozit atp. ...
#if (E_DATA & E_TAU) && (E_DATA & SCALAR_ELEMENT_DATA) && (PAR_TYPE == Q_SE)

void copy_ed_to_tau(tGrid,v)
GRID *tGrid;
INT v;
{
   ELEMENT *pelem;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ)
      pelem->tau = ED(pelem,v);
}

DOUBLE sc_lin_par(pelem)
ELEMENT *pelem;
{  eprintf("Error: sc_lin_par not available.\n"); return(0.);  }

#elif (E_DATA & E_TAU) && (E_DATA & E_TAU_SOLD) && (E_DATA & VECTOR_ELEMENT_DATA) && (PAR_TYPE == Q_VE)

void copy_ed_to_tau(tGrid,v)
GRID *tGrid;
INT v;
{
   ELEMENT *pelem;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      pelem->tau = EDV(pelem,v,0);
      pelem->tau_sold = EDV(pelem,v,1);
   }
}

DOUBLE sc_lin_par(pelem)
ELEMENT *pelem;
{
   return(EDV(pelem,W,1)*pelem->tau_sold);
}

#else

void copy_ed_to_tau(tGrid,v)
GRID *tGrid; INT v;
{  eprintf("Error: copy_ed_to_tau not available.\n");  }

DOUBLE sc_lin_par(pelem)
ELEMENT *pelem;
{  eprintf("Error: sc_lin_par not available.\n"); return(0.);  }

#endif











void max_b_and_diam(pelem,bb0,bb1,b,hK)
ELEMENT *pelem; 
FLOAT (*bb0)(), (*bb1)(), *b, *hK;
{
   FLOAT q, r, bb[DIM], xc[DIM];
   INT i;

   coord_of_barycentre(pelem,xc);
   bb[0] = bb0(xc);
   bb[1] = bb1(xc);
   q = DOT(bb,bb);
   for (i=0; i < NVERT; i++){
      bb[0] = bb0(pelem->n[i]->myvertex->x);
      bb[1] = bb1(pelem->n[i]->myvertex->x);
      if ((r=DOT(bb,bb)) > q)
         q = r;
   }
   *b = sqrt(q);
   *hK = diameter(pelem);
}

FLOAT lp_delta(pelem,eps,bb0,bb1)
ELEMENT *pelem; 
FLOAT eps, (*bb0)(), (*bb1)();
{
   FLOAT *x0, *x1, *x2, bc[DIM2][DIM2], a0, a1, a2, q, r, bb[DIM], delta, b, hK;
   INT i;

/*
   VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
   barycentric_coordinates(x0,x1,x2,bc); 
   q = DOT(bc[0],bc[0])+DOT(bc[1],bc[1])+DOT(bc[2],bc[2]);
   if (LOC_PROJECT == YES)
      delta = 1./(20.*q*sd_delta(pelem,eps,bb0,bb1)) - eps;
   else if (CONV_LOC_PROJECT == YES){
      bb[0] = bb0(x0);
      bb[1] = bb1(x0);
      a0 = DOT(bb,bc[0]);
      a1 = DOT(bb,bc[1]);
      a2 = DOT(bb,bc[2]);
      r = a0*a0 + a1*a1 + a2*a2;
      delta = (1./(20.*sd_delta(pelem,eps,bb0,bb1)) - eps*q)/r;
   }
*/
   max_b_and_diam(pelem,bb0,bb1,&b,&hK);
   if (eps < hK*b)
      delta = hK/b;
   else
      delta = hK*hK/eps;
   if (LOC_PROJECT == YES)
      delta *= b*b;
   delta = TAU0*diameter(pelem);
// return(1./320.);
   return(delta);
}

#if DATA_S & N_LINK_TO_ELEMENTS

void macro_max_b_and_diam(pn,bb0,bb1,b,hM)
NODE *pn;
FLOAT (*bb0)(), (*bb1)(), *b, *hM;
{
   LINK *pl, *pli;
   NELINK *pnel;
   DOUBLE *x0, *x1, x01[DIM], bb[DIM], xc[DIM], q;

   bb[0] = bb0(pn->myvertex->x);
   bb[1] = bb1(pn->myvertex->x);
   *b = DOT(bb,bb);
   *hM = 0.;
   for (pli = pn->tstart; pli; pli = pli->next){
      x0 = NBNODE(pli)->myvertex->x;
      bb[0] = bb0(x0);
      bb[1] = bb1(x0);
      if ((q=DOT(bb,bb)) > *b)
         *b = q;
      for (pl = pli->next; pl; pl = pl->next){
         x1 = NBNODE(pl)->myvertex->x;
         SUBTR(x0,x1,x01);
         if ((q=DOT(x01,x01)) > *hM)
            *hM = q;
      }
   }
   for (pnel = NESTART(pn); pnel; pnel = pnel->next){
      coord_of_barycentre(pnel->nbel,xc);
      bb[0] = bb0(xc);
      bb[1] = bb1(xc);
      if ((q=DOT(bb,bb)) > *b)
         *b = q;
   }
   *b  = sqrt(*b);
   *hM = sqrt(*hM);
}

#else

void macro_max_b_and_diam(pn,bb0,bb1,b,hM)
NODE *pn; FLOAT (*bb0)(), (*bb1)(), *b, *hM;
{  eprintf("Error: macro_max_b_and_diam not available.\n");  }

#endif

FLOAT lpm_delta(pn,eps,bb0,bb1)
NODE *pn;
FLOAT eps, (*bb0)(), (*bb1)();
{
   DOUBLE delta, b, hM;

   macro_max_b_and_diam(pn,bb0,bb1,&b,&hM);
   if (eps < hM*b)
      delta = hM/b;
   else
      delta = hM*hM/eps;
   if (LOC_PROJECT == YES)
      delta *= b*b;
   delta *= TAU0;
   delta = TAU0*hM;
// return(1./320.);
   return(delta);
}

INT is_macro_node(pn)
NODE *pn;
{
   LINK *pli;
   DOUBLE h=1./32.;
   INT i, j, k=0, n=NV_POINTS-1;
/*
   n = 32;
   i = round0(pn->myvertex->x[0]*n);
   j = round0(pn->myvertex->x[1]*n);
   if (2*(i/2) == i || 2*(j/2) == j)
      return(0);
   else
      return(1);
*/
/*
   for (pli = pn->tstart; pli; pli=pli->next)
      if (NBNODE(pli)->index > k)
         k = NBNODE(pli)->index;
   if (pn->index > k)
*/
// if (pn->myvertex->x[0] < 0.96874)
   if (NOT_BN(pn))
      return(1);
   else
      return(0);
}

#if DATA_S & N_LINK_TO_ELEMENTS

void edges_for_b_macros(pn,n,fb,eb,nb)
NODE *pn, **nb;
FACE **fb;
ELEMENT *eb[][2];
INT *n;
{
   NELINK *pnel;
   NODE *n0[EDGES], *n1[EDGES];
   FACE *ef[EDGES];
   INT i, j;
  
   *n = 0;
   for (pnel = NESTART(pn); pnel; pnel = pnel->next){
      FACES_AND_NODES_OF_ELEMENT(pnel->nbel,ef,n0,n1)
      for (i = 0; i < EDGES; i++)
         if ((n0[i] == pn || n1[i] == pn) && (IS_DN(n0[i]) || IS_DN(n1[i]))){
            for (j = 0; j < *n; j++)
               if (fb[j] == ef[i]){
                  eb[j][1] = pnel->nbel;
                  j = *n;
               }
            if (j == *n){
               eb[*n][0] = pnel->nbel;
               if (n0[i] == pn)
                  nb[*n] = n1[i];
               else
                  nb[*n] = n0[i];
               fb[(*n)++] = ef[i];
            }
         }
   }
}

#else

void edges_for_b_macros(pn,n,fb,eb,nb)
NODE *pn, **nb; FACE **fb; ELEMENT *eb[][2]; INT *n;
{  eprintf("Error: edges_for_b_macros not available.\n");  }

#endif

FLOAT sd_tau(y,pelem,bar,eps,bb0,bb1,react,rhs,u,space,sc_type,par1,par2)
ELEMENT *pelem;
FLOAT *y, bar[DIM2][DIM2], eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(),
      par1, par2;
INT u, space, sc_type;
{
   FLOAT xc[DIM], b_norm, hT;

   if (DELTA_TYPE == OUTFLOW_D){
      #if (E_DATA & SCALAR_ELEMENT_DATA) && (TAU_SPACE == P0)
      return(ED(pelem,TAU_VARIABLE));
      #else
      eprintf("Error: sd_delta not available.\n");
      return(0.);
      #endif
   }
   else if (DELTA_TYPE == CLASSIC_D){
      if (PW_CONST_PAR == YES){
         coord_of_barycentre(pelem,xc);
         return(sd_tau_classic(pelem,bar,eps,bb0(xc),bb1(xc),space));
      }
      else
         return(sd_tau_classic(pelem,bar,eps,bb0(y),bb1(y),space));
/*
      return(sd_tau_classic(y,pelem,bar,eps,bb0,bb1,react,rhs,u,space,sc_type,
                            par1,par2));
*/
   }
   else if (DELTA_TYPE == BASIC_D1 || DELTA_TYPE == BASIC_D2){
      b_norm = vnorm_on_element(pelem,bb0,bb1);
      hT = diameter(pelem);
      if (hT*b_norm < eps)
         return(0.);
      else if (DELTA_TYPE == BASIC_D1)
         return(hT*DELTA_FACTOR);
      else if (DELTA_TYPE == BASIC_D2)
         return(hT*DELTA_FACTOR/b_norm);
   }
   else if (DELTA_TYPE == SE1_D){
      if(PW_CONST_PAR == YES){
         coord_of_barycentre(pelem,xc);
         return(sd_tau_shih_elman1(pelem,bar,eps,bb0(xc),bb1(xc)));
      }
      else
         return(sd_tau_shih_elman1(pelem,bar,eps,bb0(y),bb1(y)));
   }
   else{
      eprintf("Error: sd_tau not available.\n");
      return(0.);
   }
}

FLOAT gsd_tau_classic(pelem,eps,bb0y,bb1y,space)
ELEMENT *pelem;
FLOAT eps, bb0y, bb1y;
INT space;
{
   FLOAT bb, hK, Pe;
   INT order=approx_order(space);

   bb = sqrt(bb0y*bb0y + bb1y*bb1y);
   if (DIR_DIAM == YES)
      hK = gsdiameter(pelem,bb0y,bb1y);
   else
      hK = diameter(pelem);
   Pe = bb*hK/(2.*eps*order);
   if (Pe < 1.e-10)
      return(hK*hK/(12.*eps*order*order));
   else
      return(hK/(2.*bb*order)*(1./tanh(Pe) - 1./Pe));
}

FLOAT gsd_tau(y,pelem,eps,bb0,bb1,space)
ELEMENT *pelem;
FLOAT *y, eps, (*bb0)(), (*bb1)();
INT space;
{
   FLOAT xc[DIM];

// return(TAU0*diameter(pelem));
   if (DELTA_TYPE == CLASSIC_D){
      if(PW_CONST_PAR == YES){
         coord_of_barycentre(pelem,xc);
         return(gsd_tau_classic(pelem,eps,bb0(xc),bb1(xc),space));
      }
      else
         return(gsd_tau_classic(pelem,eps,bb0(y),bb1(y),space));
   }
   else{
      eprintf("Error: gsd_tau not available.\n");
      return(0.);
   }
}

void set_crosswind_norm_vec(v,cross)
FLOAT v[DIM], cross[DIM];
{
   FLOAT norm;

   norm = sqrt(DOT(v,v));
   if (norm < 1.e-14)
      cross[0] = cross[1] = 0.;
   else{
      ORT_VECT(cross,v);
      cross[0] /= norm;
      cross[1] /= norm;
   }
}

void project_conv_into_grad_u(pel,x,b,bb,u,space,v)
ELEMENT *pel;
FLOAT *x, b[DIM2][DIM2], bb[DIM], v[DIM];
INT u, space;
{
   FLOAT grad_u[DIM], d;

   sgrad_value(pel,x,b,u,space,grad_u);
   d = DOT(grad_u,grad_u);
   if (d < 1.e-10)
      SET7(v,0.)
   else{
      d = DOT(bb,grad_u)/d;
      SET2(v,grad_u,d)
   }
}

INT non_isotropic_diffusion(sc_type)
INT sc_type;
{
   INT type=1;

   switch(sc_type) {
      case SCM_JSW:
      case SCM_LIN:
      case SCM_C:
      case SCM_CS:
      case SCM_K6:
      case SCM_K6red:
      case SCM_KLR1:
      case SCM_K3:
      case SCM_BE1:
      case SCM_K4:
      break;
      case SCM_HMM:
      case SCM_TP1:
      case SCM_TP2:
      case SCM_CS1:
      case SCM_GC1:
      case SCM_GC2:
      case SCM_AS:
      case SCM_CA:
      case SCM_KLR2:
      case SCM_J:
      case SCM_K6red_iso:
         type = 0;
      break;
      default:
         eprintf("Error: type of diffusion not defined for sc_type used.\n");
   }
   return(type);
}

void get_sc_vectors(pelem,n0,n1,n2,x0,x1,x2,b,bb0,bb1,an0,an1,an2,af0,af1,af2,
                                                                u,space,sc_type)
ELEMENT *pelem;
NODE *n0, *n1, *n2;
FLOAT *x0, *x1, *x2, b[DIM2][DIM2], 
      *an0, *an1, *an2, *af0, *af1, *af2, (*bb0)(), (*bb1)();
INT u, space, sc_type;
{
   FLOAT x01[DIM], x02[DIM], x12[DIM], xc[DIM], bn0[DIM], bn1[DIM], bn2[DIM], 
         bf0[DIM], bf1[DIM], bf2[DIM];
   INT	doface = 0, donode = 1;

   if (space & P2C)
      doface = 1;
   if (doface) {
      MIDPOINTS(x0, x1, x2, x01, x02, x12);
      POINT_VALUES_6(x0, x1, x2, x01, x02, x12, bn0[0], bn1[0],
                     bn2[0], bf2[0], bf1[0], bf0[0], bb0);
      POINT_VALUES_6(x0, x1, x2, x01, x02, x12, bn0[1], bn1[1],
			    bn2[1], bf2[1], bf1[1], bf0[1], bb1);
   }
   else{
      bn0[0] = bb0(x0);
      bn0[1] = bb1(x0);
      bn1[0] = bb0(x1);
      bn1[1] = bb1(x1);
      bn2[0] = bb0(x2);
      bn2[1] = bb1(x2);
   }
   switch(sc_type) {
      case SCM_JSW:
      case SCM_LIN:
      case SCM_C:
      case SCM_CS:
      case SCM_K6:
      case SCM_K6red:
      case SCM_KLR1:
      case SCM_K3:
      case SCM_BE1:
      case SCM_K4:
         if (donode)
            THREE_ORT_VECT(an0,an1,an2,bn0,bn1,bn2);
         if (doface)
            THREE_ORT_VECT(af0,af1,af2,bf0,bf1,bf2);
      break;
      case SCM_HMM:
      case SCM_TP1:
      case SCM_TP2:
      case SCM_CS1:
      case SCM_GC1:
      case SCM_GC2:
      case SCM_CA:
      case SCM_AS:
      case SCM_KLR2:
      case SCM_J:
      case SCM_K6red_iso:
      break;
      default:
         eprintf("Error: get_sc_vectors not available for sc_type used.\n");
   }
}

INT is_linear_sc(sc_type)
INT sc_type;
{
   if (sc_type == SCM_JSW || sc_type == SCM_LIN)
      return(1);
   else
      return(0);
}

void add_sd_term_matr_quadr(tGrid,Z,eps,bb0,bb1,react,rhs,
                            u,space,sc_type,par1,par2,sd_tau,qr,sd_q_ij)
GRID *tGrid;                  /*  streamline-diffusion term using quadrature  */
INT Z, u, space, sc_type;
FLOAT eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), par1, par2, (*sd_tau)();
QUADRATURE_RULE qr;
void (*sd_q_ij)();
{
   ELEMENT *pelem;
   NODE *n0, *n1, *n2;
   FLOAT b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                              n2->myvertex->x,b); 
      sd_q_ij(pelem,0,1,2,Z,b,bb0,bb1,react,rhs,eps,
              u,space,sc_type,par1,par2,sd_tau,qr.n,qr.points,qr.weights);
      sd_q_ij(pelem,1,2,0,Z,b,bb0,bb1,react,rhs,eps,
              u,space,sc_type,par1,par2,sd_tau,qr.n,qr.points,qr.weights); 
      sd_q_ij(pelem,2,0,1,Z,b,bb0,bb1,react,rhs,eps,
              u,space,sc_type,par1,par2,sd_tau,qr.n,qr.points,qr.weights);
   }
}

void add_p2c_sd_term_matr(tGrid,Z,nu,bb0,bb1,react)
GRID *tGrid;        /*  streamline-diffusion term; exact for pw. quadratic b, */
INT Z;              /*  and pw. quadratic reaction term                       */
FLOAT nu, (*bb0)(), (*bb1)(), (*react)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT s[SIDES], c[SIDES], vn[SIDES][SIDES], vf[SIDES][SIDES], 
         v0[DIM], v1[DIM], v2[DIM], v01[DIM], v02[DIM], v12[DIM], 
         sr, cr, rn[SIDES], rf[SIDES], ndetB, b[DIM2][DIM2], delta;
   INT n=0;

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      delta = sd_delta(pelem,nu,bb0,bb1);
      if (delta > 0.){
         n++;
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
         p2cv_dofs(pelem,v0,v1,v2,v01,v02,v12,bb0,bb1);
         p2cs_dofs(pelem,&rn[0],&rn[1],&rn[2],&rf[2],&rf[1],&rf[0],react);
         sr = rn[0] + rn[1] + rn[2];
         cr = rf[0] + rf[1] + rf[2];
         ndetB = delta*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                               n2->myvertex->x,b);
         compute_vn_vf_s_c(v0,v1,v2,v01,v02,v12,b,vn,vf,s,c);
         conv_stab_ij_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB);
         conv_stab_ij_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,ndetB);
         conv_stab_ij_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,ndetB);
         react_stab_ij_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,
                                                             sr,cr,rn,rf,ndetB);
         react_stab_ij_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,
                                                             sr,cr,rn,rf,ndetB);
         react_stab_ij_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,
                                                             sr,cr,rn,rf,ndetB);
/* if react = const, then also the following can be used:
         time_stab_ij_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,rn[0]*ndetB);
         time_stab_ij_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,rn[0]*ndetB);
         time_stab_ij_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,rn[0]*ndetB);
*/
         ndetB *= nu;
         lapl_stab_ij_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,b,ndetB);
         lapl_stab_ij_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,b,ndetB);
         lapl_stab_ij_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,b,ndetB);
      }
   }
   printf("SDFEM (p2c) on %i elements.\n",n);
}

void add_p2c_sd_p1_nc_term_matr(tGrid,Z,nu,bb0,bb1,react)
GRID *tGrid;        /*  streamline-diffusion term; exact for pw. quadratic b, */
INT Z;              /*  and pw. quadratic reaction term                       */
FLOAT nu, (*bb0)(), (*bb1)(), (*react)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT s[SIDES], c[SIDES], vn[SIDES][SIDES], vf[SIDES][SIDES], 
         v0[DIM], v1[DIM], v2[DIM], v01[DIM], v02[DIM], v12[DIM], 
         sr, cr, rn[SIDES], rf[SIDES], ndetB, b[DIM2][DIM2], delta[DIM2], ndetB2;
   INT n=0;

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      delta[0] = sd_p1_nc_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,0);
      delta[1] = sd_p1_nc_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,1);
      delta[2] = sd_p1_nc_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,2);
      if ((delta[0] > 0.) || (delta[1] > 0.) || (delta[2] > 0.)){
         n++;
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
         p2cv_dofs(pelem,v0,v1,v2,v01,v02,v12,bb0,bb1);
         p2cs_dofs(pelem,&rn[0],&rn[1],&rn[2],&rf[2],&rf[1],&rf[0],react);
         sr = rn[0] + rn[1] + rn[2];
         cr = rf[0] + rf[1] + rf[2];
         ndetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                               n2->myvertex->x,b);
         compute_vn_vf_s_c(v0,v1,v2,v01,v02,v12,b,vn,vf,s,c);
         conv_stab_ij_p1nc_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB,delta[0],delta[1],delta[2]);
         conv_stab_ij_p1nc_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,ndetB,delta[1],delta[2],delta[0]);
         conv_stab_ij_p1nc_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,ndetB,delta[2],delta[0],delta[1]);
         
         
         //ndetB2 = delta[1]*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
         //                              n2->myvertex->x,b);
         //conv_stab_ij_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB);
         //conv_stab_ij_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,ndetB);
         //conv_stab_ij_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,ndetB);
         
         
         react_stab_ij_p1nc_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,
                                                             sr,cr,rn,rf,ndetB,delta[0],delta[1],delta[2]);
         react_stab_ij_p1nc_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,
                                                             sr,cr,rn,rf,ndetB,delta[1],delta[2],delta[0]);
         react_stab_ij_p1nc_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,
                                                             sr,cr,rn,rf,ndetB,delta[2],delta[0],delta[1]);
         ndetB *= nu;
         lapl_stab_ij_p1nc_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,b,ndetB,delta[0],delta[1],delta[2]);
         lapl_stab_ij_p1nc_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,b,ndetB,delta[1],delta[2],delta[0]);
         lapl_stab_ij_p1nc_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,b,ndetB,delta[2],delta[0],delta[1]);
      }
   }
   printf("SDFEM on %i elements.\n",n);
}

void add_p2c_sd_p1c_term_matr(tGrid,Z,nu,bb0,bb1,react)
GRID *tGrid;        /*  streamline-diffusion term; exact for pw. quadratic b, */
INT Z;              /*  and pw. quadratic reaction term                       */
FLOAT nu, (*bb0)(), (*bb1)(), (*react)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT s[SIDES], c[SIDES], vn[SIDES][SIDES], vf[SIDES][SIDES], 
         v0[DIM], v1[DIM], v2[DIM], v01[DIM], v02[DIM], v12[DIM], 
         sr, cr, rn[SIDES], rf[SIDES], ndetB, b[DIM2][DIM2], delta[DIM2];
   INT n=0;

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      delta[0] = sd_p1c_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,n0);
      delta[1] = sd_p1c_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,n1);
      delta[2] = sd_p1c_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,n2);
      if ((delta[0] > 0.) || (delta[1] > 0.) || (delta[2] > 0.)){
         n++;
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
         p2cv_dofs(pelem,v0,v1,v2,v01,v02,v12,bb0,bb1);
         p2cs_dofs(pelem,&rn[0],&rn[1],&rn[2],&rf[2],&rf[1],&rf[0],react);
         sr = rn[0] + rn[1] + rn[2];
         cr = rf[0] + rf[1] + rf[2];
         ndetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                               n2->myvertex->x,b);
         compute_vn_vf_s_c(v0,v1,v2,v01,v02,v12,b,vn,vf,s,c);
         conv_stab_ij_p1nc_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB,delta[0],delta[1],delta[2]);
         conv_stab_ij_p1nc_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,ndetB,delta[1],delta[2],delta[0]);
         conv_stab_ij_p1nc_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,ndetB,delta[2],delta[0],delta[1]);
         react_stab_ij_p1nc_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,
                                                             sr,cr,rn,rf,ndetB,delta[0],delta[1],delta[2]);
         react_stab_ij_p1nc_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,
                                                             sr,cr,rn,rf,ndetB,delta[1],delta[2],delta[0]);
         react_stab_ij_p1nc_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,
                                                             sr,cr,rn,rf,ndetB,delta[2],delta[0],delta[1]);
         ndetB *= nu;
         lapl_stab_ij_p1nc_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,b,ndetB,delta[0],delta[1],delta[2]);
         lapl_stab_ij_p1nc_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,b,ndetB,delta[1],delta[2],delta[0]);
         lapl_stab_ij_p1nc_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,b,ndetB,delta[2],delta[0],delta[1]);
      }
   }
   printf("SDFEM on %i elements.\n",n);
}

void add_sd_rhs_n_f(vn,vf,f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rdetB,
                    rhs_n0,rhs_n1,rhs_n2,rhs_f0,rhs_f1,rhs_f2)
FLOAT vn[SIDES][SIDES], vf[SIDES][SIDES],
      f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, rdetB,
      *rhs_n0, *rhs_n1, *rhs_n2, *rhs_f0, *rhs_f1, *rhs_f2;
{
   *rhs_n0 += CONV_RHS_INT(vn[0][0],vn[1][0],vn[2][0],
                           vf[0][0],vf[1][0],vf[2][0],f0,f1,f2,
                           f001,f002,f110,f112,f220,f221,f012)*rdetB; 
   *rhs_n1 += CONV_RHS_INT(vn[0][1],vn[1][1],vn[2][1],
                           vf[0][1],vf[1][1],vf[2][1],f0,f1,f2,
                           f001,f002,f110,f112,f220,f221,f012)*rdetB; 
   *rhs_n2 += CONV_RHS_INT(vn[0][2],vn[1][2],vn[2][2],
                           vf[0][2],vf[1][2],vf[2][2],f0,f1,f2,
                           f001,f002,f110,f112,f220,f221,f012)*rdetB; 
   *rhs_f0 += (CONV_RHS_INT_L0(vn[1][2],vn[2][2],vn[0][2],
                               vf[1][2],vf[2][2],vf[0][2],f1,f2,f0,
                               f112,f110,f221,f220,f001,f002,f012) +
               CONV_RHS_INT_L0(vn[2][1],vn[0][1],vn[1][1],
                               vf[2][1],vf[0][1],vf[1][1],f2,f0,f1,
                               f220,f221,f002,f001,f112,f110,f012))*rdetB; 
   *rhs_f1 += (CONV_RHS_INT_L0(vn[0][2],vn[1][2],vn[2][2],
                               vf[0][2],vf[1][2],vf[2][2],f0,f1,f2,
                               f001,f002,f110,f112,f220,f221,f012) +
               CONV_RHS_INT_L0(vn[2][0],vn[0][0],vn[1][0],
                               vf[2][0],vf[0][0],vf[1][0],f2,f0,f1,
                               f220,f221,f002,f001,f112,f110,f012))*rdetB; 
   *rhs_f2 += (CONV_RHS_INT_L0(vn[0][1],vn[1][1],vn[2][1],
                               vf[0][1],vf[1][1],vf[2][1],f0,f1,f2,
                               f001,f002,f110,f112,f220,f221,f012) +
               CONV_RHS_INT_L0(vn[1][0],vn[2][0],vn[0][0],
                               vf[1][0],vf[2][0],vf[0][0],f1,f2,f0,
                               f112,f110,f221,f220,f001,f002,f012))*rdetB; 
}

void add_sd_p1_nc_rhs_n_f(vn,vf,f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,
                          rdetB,delta_0,delta_1,delta_2,rhs_n0,rhs_n1,rhs_n2,
                          rhs_f0,rhs_f1,rhs_f2)
FLOAT vn[SIDES][SIDES], vf[SIDES][SIDES],
      f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, rdetB,
      delta_0,delta_1,delta_2,
      *rhs_n0, *rhs_n1, *rhs_n2, *rhs_f0, *rhs_f1, *rhs_f2;
{
   *rhs_n0 += (CONV_RHS_INT_L0(vn[0][0],vn[1][0],vn[2][0],
                               vf[0][0],vf[1][0],vf[2][0],f0,f1,f2,
                               f001,f002,f110,f112,f220,f221,f012)*delta_0 +
               CONV_RHS_INT_L0(vn[1][0],vn[2][0],vn[0][0],
                               vf[1][0],vf[2][0],vf[0][0],f1,f2,f0,
                               f112,f110,f221,f220,f001,f002,f012)*delta_1 +
               CONV_RHS_INT_L0(vn[2][0],vn[0][0],vn[1][0],
                               vf[2][0],vf[0][0],vf[1][0],f2,f0,f1,
                               f220,f221,f002,f001,f112,f110,f012)*delta_2
              )*rdetB;
   *rhs_n1 += (CONV_RHS_INT_L0(vn[0][1],vn[1][1],vn[2][1],
                               vf[0][1],vf[1][1],vf[2][1],f0,f1,f2,
                               f001,f002,f110,f112,f220,f221,f012)*delta_0 +
               CONV_RHS_INT_L0(vn[1][1],vn[2][1],vn[0][1],
                               vf[1][1],vf[2][1],vf[0][1],f1,f2,f0,
                               f112,f110,f221,f220,f001,f002,f012)*delta_1 +
               CONV_RHS_INT_L0(vn[2][1],vn[0][1],vn[1][1],
                               vf[2][1],vf[0][1],vf[1][1],f2,f0,f1,
                               f220,f221,f002,f001,f112,f110,f012)*delta_2
              )*rdetB;
   *rhs_n2 += (CONV_RHS_INT_L0(vn[0][2],vn[1][2],vn[2][2],
                               vf[0][2],vf[1][2],vf[2][2],f0,f1,f2,
                               f001,f002,f110,f112,f220,f221,f012)*delta_0 +
               CONV_RHS_INT_L0(vn[1][2],vn[2][2],vn[0][2],
                               vf[1][2],vf[2][2],vf[0][2],f1,f2,f0,
                               f112,f110,f221,f220,f001,f002,f012)*delta_1 +
               CONV_RHS_INT_L0(vn[2][2],vn[0][2],vn[1][2],
                               vf[2][2],vf[0][2],vf[1][2],f2,f0,f1,
                               f220,f221,f002,f001,f112,f110,f012)*delta_2
              )*rdetB;

   *rhs_f0 += (CONV_RHS_INT_L0_L1(vn[0][2],vn[1][2],vn[2][2],
                                  vf[0][2],vf[1][2],vf[2][2],f0,f1,f2,
                                  f001,f002,f110,f112,f220,f221,f012)*delta_0 +
               CONV_RHS_INT_L0_L0(vn[1][2],vn[2][2],vn[0][2],
                                  vf[1][2],vf[2][2],vf[0][2],f1,f2,f0,
                                  f112,f110,f221,f220,f001,f002,f012)*delta_1 +
               CONV_RHS_INT_L0_L1(vn[1][2],vn[2][2],vn[0][2],
                                  vf[1][2],vf[2][2],vf[0][2],f1,f2,f0,
                                  f112,f110,f221,f220,f001,f002,f012)*delta_2 +

               CONV_RHS_INT_L0_L1(vn[2][1],vn[0][1],vn[1][1],
                                  vf[2][1],vf[0][1],vf[1][1],f2,f0,f1,
                                  f220,f221,f002,f001,f112,f110,f012)*delta_0 +
               CONV_RHS_INT_L0_L1(vn[1][1],vn[2][1],vn[0][1],
                                  vf[1][1],vf[2][1],vf[0][1],f1,f2,f0,
                                  f112,f110,f221,f220,f001,f002,f012)*delta_1 +
               CONV_RHS_INT_L0_L0(vn[2][1],vn[0][1],vn[1][1],
                                  vf[2][1],vf[0][1],vf[1][1],f2,f0,f1,
                                  f220,f221,f002,f001,f112,f110,f012)*delta_2
              )*rdetB;

   *rhs_f1 += (CONV_RHS_INT_L0_L0(vn[0][2],vn[1][2],vn[2][2],
                                  vf[0][2],vf[1][2],vf[2][2],f0,f1,f2,
                                  f001,f002,f110,f112,f220,f221,f012)*delta_0 +
               CONV_RHS_INT_L0_L1(vn[0][2],vn[1][2],vn[2][2],
                                  vf[0][2],vf[1][2],vf[2][2],f0,f1,f2,
                                  f001,f002,f110,f112,f220,f221,f012)*delta_1 +
               CONV_RHS_INT_L0_L1(vn[2][2],vn[0][2],vn[1][2],
                                  vf[2][2],vf[0][2],vf[1][2],f2,f0,f1,
                                  f220,f221,f002,f001,f112,f110,f012)*delta_2 +

               CONV_RHS_INT_L0_L1(vn[2][0],vn[0][0],vn[1][0],
                                  vf[2][0],vf[0][0],vf[1][0],f2,f0,f1,
                                  f220,f221,f002,f001,f112,f110,f012)*delta_0 +
               CONV_RHS_INT_L0_L1(vn[1][0],vn[2][0],vn[0][0],
                                  vf[1][0],vf[2][0],vf[0][0],f1,f2,f0,
                                  f112,f110,f221,f220,f001,f002,f012)*delta_1 +
               CONV_RHS_INT_L0_L0(vn[2][0],vn[0][0],vn[1][0],
                                  vf[2][0],vf[0][0],vf[1][0],f2,f0,f1,
                                  f220,f221,f002,f001,f112,f110,f012)*delta_2
              )*rdetB;

   *rhs_f2 += (CONV_RHS_INT_L0_L0(vn[0][1],vn[1][1],vn[2][1],
                                  vf[0][1],vf[1][1],vf[2][1],f0,f1,f2,
                                  f001,f002,f110,f112,f220,f221,f012)*delta_0 +
               CONV_RHS_INT_L0_L1(vn[0][1],vn[1][1],vn[2][1],
                                  vf[0][1],vf[1][1],vf[2][1],f0,f1,f2,
                                  f001,f002,f110,f112,f220,f221,f012)*delta_1 +
               CONV_RHS_INT_L0_L1(vn[2][1],vn[0][1],vn[1][1],
                                  vf[2][1],vf[0][1],vf[1][1],f2,f0,f1,
                                  f220,f221,f002,f001,f112,f110,f012)*delta_2 +

               CONV_RHS_INT_L0_L1(vn[0][0],vn[1][0],vn[2][0],
                                  vf[0][0],vf[1][0],vf[2][0],f0,f1,f2,
                                  f001,f002,f110,f112,f220,f221,f012)*delta_0 +
               CONV_RHS_INT_L0_L0(vn[1][0],vn[2][0],vn[0][0],
                                  vf[1][0],vf[2][0],vf[0][0],f1,f2,f0,
                                  f112,f110,f221,f220,f001,f002,f012)*delta_1 +
               CONV_RHS_INT_L0_L1(vn[1][0],vn[2][0],vn[0][0],
                                  vf[1][0],vf[2][0],vf[0][0],f1,f2,f0,
                                  f112,f110,f221,f220,f001,f002,f012)*delta_2
              )*rdetB;
}

#if (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA) && (F_DATA & CURVED_FACE_MIDDLE) && (ELEMENT_TYPE == SIMPLEX)

/*  stiffness matrix corresponding to \int_\om u_i(v\cdot\nabla u_j) \dx  */
void sconv_matr2_iso(tGrid,v,Z,bubble,mstruct)
GRID *tGrid;
INT v, Z, bubble, mstruct;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT vn[DIM2][DIM], vf[DIM2][DIM], vb[DIM], s[DIM], c[DIM], rdetB, 
         b[DIM2][DIM2];

   if (mstruct & Q_FULL)
      eprintf("Error: sconv_matr2_iso not implemented for (mstruct & Q_FULL).\n");
   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      SET1(vn[0],NDD(n0,v))
      SET1(vn[1],NDD(n1,v))
      SET1(vn[2],NDD(n2,v))
      SET1(vf[0],FDVP(fa0,v))
      SET1(vf[1],FDVP(fa1,v))
      SET1(vf[2],FDVP(fa2,v))
      if (bubble)
         get_edv_value(pelem,v,vb);
      if (fa0->c_midpoint || fa1->c_midpoint || fa2->c_midpoint){
         convij_sn_sf_iso(pelem,n0,n1,n2,fa0,fa1,fa2,0,1,2,vn,vf,vb,Z,bubble);
         convij_sn_sf_iso(pelem,n1,n2,n0,fa1,fa2,fa0,1,2,0,vn,vf,vb,Z,bubble);
         convij_sn_sf_iso(pelem,n2,n0,n1,fa2,fa0,fa1,2,0,1,vn,vf,vb,Z,bubble);
      }
      else{
         rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
         SET14(s,vn[0],vn[1],vn[2]);
         SET14(c,vf[0],vf[1],vf[2]);
         convij_sn_sf(pelem,n0,n1,n2,fa0,fa1,fa2,0,1,2,vn,vf,vb,s,c,Z,
                                           b[0],b[1],b[2],rdetB,bubble,mstruct);
         convij_sn_sf(pelem,n1,n2,n0,fa1,fa2,fa0,1,2,0,vn,vf,vb,s,c,Z,
                                           b[1],b[2],b[0],rdetB,bubble,mstruct);
         convij_sn_sf(pelem,n2,n0,n1,fa2,fa0,fa1,2,0,1,vn,vf,vb,s,c,Z,
                                           b[2],b[0],b[1],rdetB,bubble,mstruct);
         if (bubble)
            fill_diag_e_matrix(pelem,Z,
               (DOT(vf[0],b[0])+DOT(vf[1],b[1])+DOT(vf[2],b[2])
           -3.*(DOT(vn[0],b[0])+DOT(vn[1],b[1])+DOT(vn[2],b[2])))*rdetB/15120.);
      }
   }
}

#else

void sconv_matr2_iso(tGrid,v,Z,bubble,mstruct)
GRID *tGrid; INT v, Z, bubble, mstruct;
{  eprintf("Error: sconv_matr2_iso not available.\n");  }

#endif

#if (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX)

/*  stiffness matrix corresponding to \int_\om u_i(v\cdot\nabla u_j) \dx  */
void sconv_matr2(tGrid,v,Z,bubble,mstruct)
GRID *tGrid;
INT v, Z, bubble, mstruct;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT vn[DIM2][DIM], vf[DIM2][DIM], vb[DIM], s[DIM], c[DIM], rdetB, 
         b[DIM2][DIM2];

   if ((mstruct & Q_FULL) && bubble)
      eprintf("Error: sconv_matr2 not implemented for bubble and (mstruct & Q_FULL).\n");
   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      SET1(vn[0],NDD(n0,v))
      SET1(vn[1],NDD(n1,v))
      SET1(vn[2],NDD(n2,v))
      SET1(vf[0],FDVP(fa0,v))
      SET1(vf[1],FDVP(fa1,v))
      SET1(vf[2],FDVP(fa2,v))
      SET14(s,vn[0],vn[1],vn[2]);
      SET14(c,vf[0],vf[1],vf[2]);
      if (bubble){
         get_edv_value(pelem,v,vb);
         fill_diag_e_matrix(pelem,Z,
               (DOT(vf[0],b[0])+DOT(vf[1],b[1])+DOT(vf[2],b[2])
           -3.*(DOT(vn[0],b[0])+DOT(vn[1],b[1])+DOT(vn[2],b[2])))*rdetB/15120.);
      }
      convij_sn_sf(pelem,n0,n1,n2,fa0,fa1,fa2,0,1,2,vn,vf,vb,s,c,Z,
                                           b[0],b[1],b[2],rdetB,bubble,mstruct);
      convij_sn_sf(pelem,n1,n2,n0,fa1,fa2,fa0,1,2,0,vn,vf,vb,s,c,Z,
                                           b[1],b[2],b[0],rdetB,bubble,mstruct);
      convij_sn_sf(pelem,n2,n0,n1,fa2,fa0,fa1,2,0,1,vn,vf,vb,s,c,Z,
                                           b[2],b[0],b[1],rdetB,bubble,mstruct);
   }
}

/*  stiffness matrix corresponding to 
              0.5*\int_\om u_i(v\cdot\nabla u_j) - u_j(v\cdot\nabla u_i) \dx  */
void sconv_skew_matr2(tGrid,v,Z)
GRID *tGrid;
INT v, Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT s[DIM], c[DIM], rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      SET14(s,NDD(n0,v),NDD(n1,v),NDD(n2,v));
      SET14(c,FDVP(fa0,v),FDVP(fa1,v),FDVP(fa2,v));
      conv_skew_ij_sn_sf(n0,n1,n2,fa0,fa1,fa2,s,c,v,Z,b[0],b[1],b[2],rdetB);
      conv_skew_ij_sn_sf(n1,n2,n0,fa1,fa2,fa0,s,c,v,Z,b[1],b[2],b[0],rdetB);
      conv_skew_ij_sn_sf(n2,n0,n1,fa2,fa0,fa1,s,c,v,Z,b[2],b[0],b[1],rdetB);
   }
}

/*  stiffness matrix corresponding to \int_\om u_i(u_j\cdot\nabla v) \dx  */
void newton_conv_matr2(tGrid,v,Z,bubble)
GRID *tGrid;
INT v, Z, bubble;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT vn[DIM2][DIM], vf[DIM2][DIM], vb[DIM], s[DIM], 
         p[DIM][DIM], q[DIM][DIM], b[DIM2][DIM2], rdetB;

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      SET1(vn[0],NDD(n0,v))
      SET1(vn[1],NDD(n1,v))
      SET1(vn[2],NDD(n2,v))
      SET1(vf[0],FDVP(fa0,v))
      SET1(vf[1],FDVP(fa1,v))
      SET1(vf[2],FDVP(fa2,v))
      SET14(s,vf[0],vf[1],vf[2]);
      SET27(p[0],b[0],vn[0][0],b[1],vn[1][0],b[2],vn[2][0])
      SET27(p[1],b[0],vn[0][1],b[1],vn[1][1],b[2],vn[2][1])
      SET27(q[0],b[0],vf[0][0],b[1],vf[1][0],b[2],vf[2][0])
      SET27(q[1],b[0],vf[0][1],b[1],vf[1][1],b[2],vf[2][1])
      if (bubble){
         eprintf("Error: bubble part not implemented for the Newton method.\n");
         get_edv_value(pelem,v,vb);
//         fill_diag_e_matrix(pelem,Z,0.);
      }
      newton_convij_sn_sf(n0,n1,n2,fa0,fa1,fa2,0,1,2,vf,s,p,q,Z,b[0],b[1],b[2],
                                                                         rdetB);
      newton_convij_sn_sf(n1,n2,n0,fa1,fa2,fa0,1,2,0,vf,s,p,q,Z,b[1],b[2],b[0],
                                                                         rdetB);
      newton_convij_sn_sf(n2,n0,n1,fa2,fa0,fa1,2,0,1,vf,s,p,q,Z,b[2],b[0],b[1],
                                                                         rdetB);
   }
}

void p2c_upwind_matr(tGrid,nu,v,Z,mstruct)
GRID *tGrid;
FLOAT nu;
INT v, Z, mstruct;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT c[DIM], a0, a1, a2, b0=0., b1=0., b2=0., b[DIM2][DIM2], rdetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      if (IS_BF(fa0))
         b0 = -( (ND(n1,v,0)+ND(n2,v,0)+FDV(fa0,v,0)/3.)*b[0][0] 
               + (ND(n1,v,1)+ND(n2,v,1)+FDV(fa0,v,1)/3.)*b[0][1] )*rdetB;
      if (IS_BF(fa1))
         b1 = -( (ND(n0,v,0)+ND(n2,v,0)+FDV(fa1,v,0)/3.)*b[1][0] 
               + (ND(n0,v,1)+ND(n2,v,1)+FDV(fa1,v,1)/3.)*b[1][1] )*rdetB;
      if (IS_BF(fa2))
         b2 = -( (ND(n0,v,0)+ND(n1,v,0)+FDV(fa2,v,0)/3.)*b[2][0] 
               + (ND(n0,v,1)+ND(n1,v,1)+FDV(fa2,v,1)/3.)*b[2][1] )*rdetB;
      c[0] = 3.*( ND( n0,v,0) +  ND( n1,v,0) +  ND( n2,v,0)) +
             5.*(FDV(fa0,v,0) + FDV(fa1,v,0) + FDV(fa2,v,0))/3.;
      c[1] = 3.*( ND( n0,v,1) +  ND( n1,v,1) +  ND( n2,v,1)) +
             5.*(FDV(fa0,v,1) + FDV(fa1,v,1) + FDV(fa2,v,1))/3.;
      rdetB /= 27.;
      a0 = ( (9.*ND(n0,v,0)-FDV(fa0,v,0)+c[0])*(b[1][0]-b[2][0])
           + (9.*ND(n0,v,1)-FDV(fa0,v,1)+c[1])*(b[1][1]-b[2][1]) )*rdetB;
      a1 = ( (9.*ND(n1,v,0)-FDV(fa1,v,0)+c[0])*(b[2][0]-b[0][0])
           + (9.*ND(n1,v,1)-FDV(fa1,v,1)+c[1])*(b[2][1]-b[0][1]) )*rdetB;
      a2 = ( (9.*ND(n2,v,0)-FDV(fa2,v,0)+c[0])*(b[0][0]-b[1][0])
           + (9.*ND(n2,v,1)-FDV(fa2,v,1)+c[1])*(b[0][1]-b[1][1]) )*rdetB;
      if (mstruct & Q_FULL){
         vp2c_up_ij(n0,n1,n2,fa0,fa1,fa2,nu,Z,b0,a2,-a1);
         vp2c_up_ij(n1,n2,n0,fa1,fa2,fa0,nu,Z,b1,a0,-a2);
         vp2c_up_ij(n2,n0,n1,fa2,fa0,fa1,nu,Z,b2,a1,-a0);
      }
      else{
         p2c_up_ij(n0,n1,n2,fa0,fa1,fa2,nu,Z,b0,a2,-a1);
         p2c_up_ij(n1,n2,n0,fa1,fa2,fa0,nu,Z,b1,a0,-a2);
         p2c_up_ij(n2,n0,n1,fa2,fa0,fa1,nu,Z,b2,a1,-a0);
      }
   }
}

#if N_DATA & SCALAR_NODE_DATA

void pressure_gradient(n0,n1,n2,p,b,grad_p)
NODE *n0, *n1, *n2;
FLOAT b[DIM2][DIM2], grad_p[DIM];
INT p;
{
   grad_p[0] = NDS(n0,p)*b[0][0] + NDS(n1,p)*b[1][0] + NDS(n2,p)*b[2][0];
   grad_p[1] = NDS(n0,p)*b[0][1] + NDS(n1,p)*b[1][1] + NDS(n2,p)*b[2][1];
}

#elif P_SPACE == P0

void pressure_gradient(n0,n1,n2,p,b,grad_p)
NODE *n0, *n1, *n2; FLOAT b[DIM2][DIM2], grad_p[DIM]; INT p;
{
   grad_p[0] = 0.;
   grad_p[1] = 0.;
}

#else

void pressure_gradient(n0,n1,n2,p,b,grad_p)
NODE *n0, *n1, *n2; FLOAT b[DIM2][DIM2], grad_p[DIM]; INT p;
{  eprintf("Error: pressure_gradient not available.\n");  }

#endif

/*  stiffness matrix corresponding to 
                    \sum_K \delta_K\int_K (v\cdot\nabla u)(v\cdot\nabla v)\dx */
void sconv_stab_matr2(tGrid,nu,v,Z,f,p,u_old,rhs0,rhs1,tau)
GRID *tGrid;
FLOAT nu, (*rhs0)(), (*rhs1)(), tau;
INT v, Z, f, p, u_old;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT s[SIDES], c[SIDES], vn[SIDES][SIDES], vf[SIDES][SIDES], 
         rdetB, b[DIM2][DIM2];
   FLOAT *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], sv[DIM], cv[DIM],
         v0[DIM], v1[DIM], v2[DIM], v01[DIM], v02[DIM], v12[DIM], v_norm,
         u0, u1, u2, u001, u002, u110, u112, u220, u221, u012, 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, 
         hT, delta_T=1., grad_p[DIM], q;
   INT n=0;

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      SET1(v0,NDD(n0,v))
      SET1(v1,NDD(n1,v))
      SET1(v2,NDD(n2,v))
      SET1(v12,FDVP(fa0,v))
      SET1(v02,FDVP(fa1,v))
      SET1(v01,FDVP(fa2,v))
      SET14(sv,v0,v1,v2)
      SET14(cv,v01,v02,v12)
      v_norm = (fabs(v0[0])+fabs(v1[0])+fabs(v2[0])+
                fabs(v0[1])+fabs(v1[1])+fabs(v2[1])+
               (fabs(v01[0]+2.*(v0[0]+v1[0]))+
                fabs(v02[0]+2.*(v0[0]+v2[0]))+
                fabs(v12[0]+2.*(v1[0]+v2[0]))+
                fabs(v01[1]+2.*(v0[1]+v1[1]))+
                fabs(v02[1]+2.*(v0[1]+v2[1]))+
                fabs(v12[1]+2.*(v1[1]+v2[1])))*0.25)/12.;
/*
      v_norm = sqrt((6.*DOT(sv,cv) + DOT(cv,cv)  + 15.*(DOT(v0,v0)+DOT(v1,v1)+
                     DOT(v2,v2)+DOT(v0,v1)+DOT(v1,v2)+DOT(v2,v0)) - 
                     3.*(DOT(v0,v12)+DOT(v1,v02)+DOT(v2,v01)) -
                     (DOT(v01,v02)+DOT(v02,v12)+DOT(v12,v01)))/90.*rdetB);
*/
      hT = diameter(pelem);
      if (v_norm > 1.e-15)
         delta_T = 0.5*hT/v_norm; 
      if (v_norm > 1.e-15 && 0.25*PECLET_FACTOR*hT*hT/nu/delta_T > 1.){
         n++;
         rdetB *= delta_T*DELTA_FACTOR;
         compute_vn_vf_s_c(v0,v1,v2,v01,v02,v12,b,vn,vf,s,c);
         conv_stab_ij_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,rdetB);
         conv_stab_ij_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,rdetB);
         conv_stab_ij_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,rdetB);

         pressure_gradient(n0,n1,n2,p,b,grad_p);
         VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
         points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012);
         q = grad_p[0];
         POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                         f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs0)
         SUBTR_CONST_10( f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,q)
         if (fabs(tau) > 1.e-16){
            q = 1./tau;
            QUADR_VALUES_10(ND(n0,u_old,0),ND(n1,u_old,0),ND(n2,u_old,0),
                            FDV(fa2,u_old,0),FDV(fa1,u_old,0),FDV(fa0,u_old,0),
                            u0,u1,u2,u001,u002,u110,u112,u220,u221,u012)
            MULT_AND_ADD_10_TO_10(f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,
                                  u0,u1,u2,u001,u002,u110,u112,u220,u221,u012,q)
         }
         add_sd_rhs_n_f(vn,vf,f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rdetB,
                        &ND(n0,f,0),&ND(n1,f,0),&ND(n2,f,0),
                        &FDV(fa0,f,0),&FDV(fa1,f,0),&FDV(fa2,f,0));
         q = grad_p[1];
         POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                         f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs1)
         SUBTR_CONST_10( f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,q)
         if (fabs(tau) > 1.e-16){
            q = 1./tau;
            QUADR_VALUES_10(ND(n0,u_old,1),ND(n1,u_old,1),ND(n2,u_old,1),
                            FDV(fa2,u_old,1),FDV(fa1,u_old,1),FDV(fa0,u_old,1),
                            u0,u1,u2,u001,u002,u110,u112,u220,u221,u012)
            MULT_AND_ADD_10_TO_10(f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,
                                  u0,u1,u2,u001,u002,u110,u112,u220,u221,u012,q)
         }
         add_sd_rhs_n_f(vn,vf,f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rdetB,
                        &ND(n0,f,1),&ND(n1,f,1),&ND(n2,f,1),
                        &FDV(fa0,f,1),&FDV(fa1,f,1),&FDV(fa2,f,1));
         if (fabs(tau) > 1.e-16){
            q = rdetB/tau;
            time_stab_ij_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,q);
            time_stab_ij_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,q);
            time_stab_ij_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,q);
         }
         rdetB *= nu;
         lapl_stab_ij_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,b,rdetB);
         lapl_stab_ij_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,b,rdetB);
         lapl_stab_ij_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,b,rdetB);
      }
   }
   printf("SDFEM on %i elements.\n",n);
}

#else

void sconv_matr2(tGrid,v,Z,bubble,mstruct)
GRID *tGrid; INT v, Z, bubble, mstruct;
{  eprintf("Error: sconv_matr2 not available.\n");  }

void sconv_skew_matr2(tGrid,v,Z)
GRID *tGrid; INT v, Z;
{  eprintf("Error: sconv_skew_matr2 not available.\n");  }

void newton_conv_matr2(tGrid,v,Z,bubble)
GRID *tGrid; INT v, Z, bubble;
{  eprintf("Error: newton_conv_matr2 not available.\n");  }

void p2c_upwind_matr(tGrid,nu,v,Z,mstruct)
GRID *tGrid; FLOAT nu; INT v, Z, mstruct;
{  eprintf("Error: p2c_upwind_matr not available.\n");  }

void sconv_stab_matr2(tGrid,nu,v,Z,f,p,u_old,rhs0,rhs1,tau)
GRID *tGrid; FLOAT nu, (*rhs0)(), (*rhs1)(), tau; INT v, Z, f, p, u_old;
{  eprintf("Error: sconv_stab_matr2 not available.\n");  }

#endif

/******************************************************************************/
/*                                                                            */
/*                                     NS                                     */
/*                                                                            */
/******************************************************************************/

#if (N_DATA & DxD_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (N_DATA & VECTOR_NODE_DATA) && (ELEMENT_TYPE == SIMPLEX)

void put_diag2(n0,p,Z)
NODE *n0;
FLOAT p;
INT Z;
{
   COEFFNN(n0,Z,0,0) += p;
   COEFFNN(n0,Z,1,1) += p;
}

void put_nn1_2(n0,n1,p,u,f,Z)
NODE *n0, *n1;
FLOAT p;
INT u, f, Z;
{
   LINK *pli;   

      for (pli=n0->tstart; pli->nbnode != n1; pli=pli->next); 
      COEFFLL(pli,Z,0,0) += p;
      COEFFLL(pli,Z,1,1) += p;
   if (IS_DN(n1))
      SET9(NDD(n0,f),NDD(n1,u),p)
}

#else

void put_diag2(n0,p,Z)
NODE *n0; FLOAT p; INT Z;
{  eprintf("Error: put_diag2 not available.\n");  }

void put_nn1_2(n0,n1,p,u,f,Z)
NODE *n0, *n1; FLOAT p; INT u, f, Z;
{  eprintf("Error: put_nn1_2 not available.\n");  }

#endif

#if !(U_SPACE == P1C_NEW_FBUB) && (DATA_S & F_LINK_TO_FACES) && (F_DATA & ONE_FACE_MATR) && (F_DATA & SCALAR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX)

void put_ff1(fan,fa1,nn,nn1,sumv,summ,m0,m1,m2,v1,v2,b0,b1,b2,u,f,Z,q)
FACE *fan, *fa1;
INT u, f, Z;
FLOAT nn[DIM], nn1[DIM], sumv[DIM], summ[DIM], 
      m0[DIM], m1[DIM], m2[DIM], v1[DIM], v2[DIM], 
      b0[DIM2], b1[DIM2], b2[DIM2], q;
{
   FLINK *pfli;
   FLOAT p, y[DIM], z[DIM];
   
   SET14(z,v1,v2,v2)
   SET21(y,sumv,180.,summ,630.)
   p = DOT(nn,nn1)*(-DOT(b1,y) + DOT(b0,z)/180. + DOT2(b0,m1,m0)/1260.)*q*FMULT2;
      for (pfli=fan->tfstart; pfli->nbface!=fa1; pfli=pfli->next);
      COEFF_FL(pfli,Z) += p;
   if (NOT_FF(fa1))
      FD(fan,f) -= FD(fa1,u)*p; 
}

#else

void put_ff1(fan,fa1,nn,nn1,sumv,summ,m0,m1,m2,v1,v2,b0,b1,b2,u,f,Z,q)
FACE *fan, *fa1; INT u, f, Z; FLOAT nn[DIM], nn1[DIM], sumv[DIM], summ[DIM], 
m0[DIM], m1[DIM], m2[DIM], v1[DIM], v2[DIM], b0[DIM2], b1[DIM2], b2[DIM2], q;
{  eprintf("Error: put_ff1 not available.\n");  }

#endif

#if !(DATA_STR & REDUCED) && (DATA_S & N_LINK_TO_FACES) && (N_DATA & Dx1_NODE_FACE_MATR) && (F_DATA & ONE_FACE_MATR) && (N_DATA & VECTOR_NODE_DATA) && (F_DATA & SCALAR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX)

/*  x = 3*v0 + v1 + v2, summ = m0 + m1 + m2 */
void put_nf1(pnode,fa1,nn1,x,summ,m1,m2,v0,v1,v2,b0,b1,b2,Z,q)
NODE *pnode;
FACE *fa1;
INT Z;
FLOAT nn1[DIM], x[DIM], summ[DIM], m1[DIM], m2[DIM], 
      v0[DIM], v1[DIM], v2[DIM], 
      b0[DIM2], b1[DIM2], b2[DIM2], q;
{
   NFLINK *pnf;
   FLOAT y[DIM], p;

   for (pnf=pnode->tnfstart; pnf->nbface!=fa1; pnf=pnf->next);
   SET17(y,m1,summ)
   SET21(y,x,30.,y,180.)
   p = (-DOT(b1,y) - DOT2(b0,v1,v0)/60. + 
        (2.*DOT(b2,m2) - DOT(b0,m1))/180.)*q*FMULT;
   SET4(COEFF_NFP(pnf,Z),nn1,p)
}

void nijb(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,v0,v1,v2,m0,m1,m2,
                                                  sumv,summ,Z,f,u,b0,b1,b2,detB)
NODE *n0, *n1, *n2;
FACE *fa0, *fa1, *fa2;
INT Z, f, u;
FLOAT nn0[DIM], nn1[DIM], nn2[DIM], v0[DIM], v1[DIM], v2[DIM], 
      m0[DIM], m1[DIM], m2[DIM], sumv[DIM], summ[DIM], 
      b0[DIM2], b1[DIM2], b2[DIM2], detB;
{
   NFLINK *pnf;
   FLOAT x[DIM], y[DIM], z[DIM], p;
  
      SET10(x,v0,sumv)
      SET18(y,summ,m0)
      SET20(y,x,detB/12.,y,detB/60.)
      put_diag2(n0,DOT(b0,y),Z);
      put_nn1_2(n0,n1,DOT(b1,y),u,f,Z);
      put_nn1_2(n0,n2,DOT(b2,y),u,f,Z);
      SET17(x,v0,sumv)
      put_nf1(n0,fa1,nn1,x,summ,m1,m2,v0,v1,v2,b0,b1,b2,Z,detB);
      put_nf1(n0,fa2,nn2,x,summ,m2,m1,v0,v2,v1,b0,b2,b1,Z,detB);
      SET21(x,sumv,30.,summ,180.)
      p = (-DOT(b0,x) - (DOT(v1,b1)+DOT(v2,b2))/60. + 
                       (DOT(m1,b1)+DOT(m2,b2))/180.)*detB*FMULT;
      if (NOT_FF(fa0)){
         p *= FD(fa0,u);
         SET9(NDD(n0,f),nn0,p)
      }
      else{
         for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
         SET4(COEFF_NFP(pnf,Z),nn0,p)
      }
   
      SET17(y,m0,summ)
      SET19(x,sumv,v0)
      SET21(z,x,180.,y,630.)
      COEFF_FF(fa0,Z) += (-DOT(b0,z)-(DOT(v1,b1)+DOT(v2,b2))/180. + 
                                     (DOT(m1,b1)+DOT(m2,b2))/1260.)*detB*FMULT2;
      put_ff1(fa0,fa1,nn0,nn1,sumv,summ,m0,m1,m2,v1,v2,b0,b1,b2,u,f,Z,detB);
      put_ff1(fa0,fa2,nn0,nn2,sumv,summ,m0,m2,m1,v2,v1,b0,b2,b1,u,f,Z,detB);
      SET18(x,sumv,v0)
      SET10(y,summ,m0)
      SET20(z,x,detB/60.,y,detB/180.)
      put_fn1(fa0,n0,nn0,b0,z,u,f,Z);
      put_fn1(fa0,n1,nn0,b1,z,u,f,Z);
      put_fn1(fa0,n2,nn0,b2,z,u,f,Z);
}

#else

void nijb(n0,n1,n2,v0,sumv,Z,f,u,b0,b1,b2,detB)
NODE *n0, *n1, *n2;
INT Z, f, u;
FLOAT v0[DIM], sumv[DIM], b0[DIM2], b1[DIM2], b2[DIM2], detB;
{
   FLOAT x[DIM], p;
  
      SET10(x,v0,sumv)
      p = detB/12.;
      SET2(x,x,p)
      put_diag2(n0,DOT(b0,x),Z);
      put_nn1_2(n0,n1,DOT(b1,x),u,f,Z);
      put_nn1_2(n0,n2,DOT(b2,x),u,f,Z);
}

#endif

#if !(DATA_STR & LG_DATA) && !(DATA_STR & REDUCED) && (DATA_S & N_LINK_TO_FACES) && (N_DATA & VECTOR_NODE_DATA) && (F_DATA & SCALAR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX)

/* matrix and rhs corresp. to \int_\om w_i\dot(\grad w_j)v\dx; u contains    */
void n_matr(tGrid,Z,f,u,v)                             /*  the Dirichlet b.c. */
GRID *tGrid;
INT Z, f, u, v;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT nn0[DIM], nn1[DIM], nn2[DIM], 
         v0[DIM], v1[DIM], v2[DIM], m0[DIM], m1[DIM], m2[DIM],
         sumv[DIM], summ[DIM], detB, b[DIM2][DIM2], p;
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      detB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                     n2->myvertex->x,b);
      normal_vector(n1->myvertex->x,n2->myvertex->x,fa0,nn0);
      normal_vector(n0->myvertex->x,n2->myvertex->x,fa1,nn1);
      normal_vector(n0->myvertex->x,n1->myvertex->x,fa2,nn2);
      SET1(v0,NDD(n0,v))
      SET1(v1,NDD(n1,v))
      SET1(v2,NDD(n2,v))
      SET3(m0,nn0,FD(fa0,v),FMULT,p)
      SET3(m1,nn1,FD(fa1,v),FMULT,p)
      SET3(m2,nn2,FD(fa2,v),FMULT,p)
      SET14(sumv,v0,v1,v2)
      SET14(summ,m0,m1,m2)
      nijb(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,v0,v1,v2,m0,m1,m2,
                                           sumv,summ,Z,f,u,b[0],b[1],b[2],detB);
      nijb(n1,n2,n0,fa1,fa2,fa0,nn1,nn2,nn0,v1,v2,v0,m1,m2,m0,
                                           sumv,summ,Z,f,u,b[1],b[2],b[0],detB);
      nijb(n2,n0,n1,fa2,fa0,fa1,nn2,nn0,nn1,v2,v0,v1,m2,m0,m1,
                                           sumv,summ,Z,f,u,b[2],b[0],b[1],detB);
   } 
}

#endif

#if !(DATA_STR & LG_DATA) && (DATA_STR & REDUCED) && (ELEMENT_TYPE == SIMPLEX)

/* matrix and rhs corresp. to \int_\om w_i\dot(\grad w_j)v\dx; u contains    */
void n_matr(tGrid,Z,f,u,v)                             /*  the Dirichlet b.c. */
GRID *tGrid;
INT Z, f, u, v;
{
   ELEMENT *pelem;
   NODE *n0, *n1, *n2;
   FLOAT v0[DIM], v1[DIM], v2[DIM], sumv[DIM], detB, b[DIM2][DIM2];
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      detB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                     n2->myvertex->x,b);
      SET1(v0,NDD(n0,v))
      SET1(v1,NDD(n1,v))
      SET1(v2,NDD(n2,v))
      SET14(sumv,v0,v1,v2)
      nijb(n0,n1,n2,v0,sumv,Z,f,u,b[0],b[1],b[2],detB);
      nijb(n1,n2,n0,v1,sumv,Z,f,u,b[1],b[2],b[0],detB);
      nijb(n2,n0,n1,v2,sumv,Z,f,u,b[2],b[0],b[1],detB);
   } 
}

#endif

#if DATA_STR & LG_DATA

void n_matr(tGrid,Z,f,u,v)
GRID *tGrid;
INT Z, f, u, v;
{
   eprintf("n_matr not available.\n");
}

#endif

void n_matr_up(tGrid,Z,f,u,v,q1,q2,q3)
GRID *tGrid;
INT Z, f, u, v, q1, q2, q3;
{
   eprintf("n_matr_up not available.\n");
}

void n_matr_up2(tGrid,Z,f,u,v,q1,q2,q3)
GRID *tGrid;
INT Z, f, u, v, q1, q2, q3;
{
   eprintf("n_matr_up2 not available.\n");
}

/******************************************************************************/
/*                                                                            */
/*           nonconforming stiffness matrices for Stokes equations            */
/*                                                                            */
/******************************************************************************/

void vnc_aij(fa0,fa1,fa2,Z,b0,b1,b2,detB,mstruct)
FACE *fa0, *fa1, *fa2;
INT Z, mstruct;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], detB;  /*  detB = volume * nu   */
{
   detB *= 4.;
   if (mstruct & Q_FULL)
      vfill_sf(fa0,fa1,fa2,Z,DOT(b0,b0)*detB,DOT(b0,b1)*detB,DOT(b0,b2)*detB);
   else
      sfill_sf(fa0,fa1,fa2,Z,DOT(b0,b0)*detB,DOT(b0,b1)*detB,DOT(b0,b2)*detB);
}

FLOAT upwind_lambda(flux,nu)  /*  flux = \int_{\Gamma_ik} b\cdot n_ik\ds  */
FLOAT flux, nu;
{
   FLOAT t;

   t = fabs(0.5*flux/nu);
   return(0.5 + SGN(flux)*0.5*t/(t+1));
} 

#if (F_DATA & VECTOR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX) && (DIM == 2)

void nc_up_aij(fa0,fa1,fa2,nu,Z,v,v1,v2,b0,b1,b2,detB,mstruct)
FACE *fa0, *fa1, *fa2;
INT v, Z, mstruct;
FLOAT nu, *v1, *v2, *b0, *b1, *b2, detB;  /*  detB = volume * (2./3.)  */
{
   FLOAT a0, a1, a2, s, c[DIM];

   SUBTR(b0,b1,c);
   a1 = detB*DOT(c,v2);
   a1 *= 1. - upwind_lambda(a1,nu);
   SUBTR(b0,b2,c);
   a2 = detB*DOT(c,v1);
   a2 *= 1. - upwind_lambda(a2,nu);
   a0 = -a1 - a2;
   if (IS_BF(fa0)){
      s = detB*DOT(b0,FDVP(fa0,v));
      s *= 1. - 2.*upwind_lambda(-3.*s,nu);
      a0 -= s + s;
      a1 += s;
      a2 += s;
   }
   if (mstruct & Q_FULL)
      vfill_sf(fa0,fa1,fa2,Z,a0,a1,a2);
   else
      sfill_sf(fa0,fa1,fa2,Z,a0,a1,a2);
}

#else

void nc_up_aij(fa0,fa1,fa2,nu,Z,v,v1,v2,b0,b1,b2,detB,mstruct)
FACE *fa0, *fa1, *fa2; INT v, Z, mstruct; FLOAT nu, *v1, *v2, *b0, *b1, *b2, detB;
{ eprintf("Error: nc_up_aij not available.\n"); }

#endif

#if (DATA_S & F_LINK_TO_FACES) && (F_DATA & DxD_FACE_MATR) && (ELEMENT_TYPE == SIMPLEX)

void vadd_to_matrix_f(fa0,fa1,fa2,Z,
                                ann,an1,an2,bnn,bn1,bn2,cnn,cn1,cn2,dnn,dn1,dn2)
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT ann, an1, an2, bnn, bn1, bn2, cnn, cn1, cn2, dnn, dn1, dn2;
{
   FLINK *pfli;
  
   SET_COEFFNN(fa0,Z,ann,bnn,cnn,dnn);
  
   for (pfli=fa0->tfstart; pfli->nbface!=fa1 && pfli->nbface!=fa2; 
                                                               pfli=pfli->next);
   if (pfli->nbface==fa1){
      SET_COEFFNN(pfli,Z,an1,bn1,cn1,dn1);
      for (pfli=pfli->next; pfli->nbface!=fa2; pfli=pfli->next);
      SET_COEFFNN(pfli,Z,an2,bn2,cn2,dn2);
   }
   else{
      SET_COEFFNN(pfli,Z,an2,bn2,cn2,dn2);
      for (pfli=pfli->next; pfli->nbface!=fa1; pfli=pfli->next);
      SET_COEFFNN(pfli,Z,an1,bn1,cn1,dn1);
   }
}

#else

void vadd_to_matrix_f(fa0,fa1,fa2,Z,ann,an1,an2,bnn,bn1,bn2,cnn,cn1,cn2,dnn,dn1,dn2)
FACE *fa0, *fa1, *fa2; INT Z; FLOAT ann, an1, an2, bnn, bn1, bn2, cnn, cn1, cn2, dnn, dn1, dn2;
{  eprintf("Error: vadd_to_matrix_f not available.\n");  }

#endif

#if (F_DATA & VECTOR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX) && (DIM == 2)

void newton_nc_up_aij(fa0,fa1,fa2,nu,Z,v,v1,v2,b0,b1,b2,detB)
FACE *fa0, *fa1, *fa2;
INT v, Z;
FLOAT nu, *v1, *v2, *b0, *b1, *b2, detB;  /*  detB = volume * (4./9.)  */
{
   FLOAT a0[DIM], c0[DIM], c1[DIM], c2[DIM], d0[DIM], d1[DIM], d2[DIM], c[DIM],
         ann, an1, an2, bnn, bn1, bn2, cnn, cn1, cn2, dnn, dn1, dn2, q1, q2, r;

   SET28(c1,b0,b1,detB)
   SET28(c2,b0,b2,detB)
   q1 = 1. - upwind_lambda(1.5*DOT(c1,v2),nu);
   q2 = 1. - upwind_lambda(1.5*DOT(c2,v1),nu);
   SET11(d1,FDVP(fa1,v),FDVP(fa0,v));
   SET11(d2,FDVP(fa2,v),FDVP(fa0,v));
   ann = q1*c1[0]*d1[0] + q2*c2[0]*d2[0];
   bnn = q1*c1[1]*d1[0] + q2*c2[1]*d2[0];
   cnn = q1*c1[0]*d1[1] + q2*c2[0]*d2[1];
   dnn = q1*c1[1]*d1[1] + q2*c2[1]*d2[1];
   if (IS_BF(fa0)){
      SET3(c0,b0,-4.5,detB,r)
      r = (1. - 2.*upwind_lambda(DOT(c0,FDVP(fa0,v)),nu))/3.;
      ann -= r*c0[0]*(d1[0]+d2[0]);
      bnn -= r*c0[1]*(d1[0]+d2[0]);
      cnn -= r*c0[0]*(d1[1]+d2[1]);
      dnn -= r*c0[1]*(d1[1]+d2[1]);
   }
   SET2(a0,c2,-0.5) 
   an1 = q1*c1[0]*d1[0] + q2*a0[0]*d2[0];
   bn1 = q1*c1[1]*d1[0] + q2*a0[1]*d2[0];
   cn1 = q1*c1[0]*d1[1] + q2*a0[0]*d2[1];
   dn1 = q1*c1[1]*d1[1] + q2*a0[1]*d2[1];
   SET2(a0,c1,-0.5) 
   an2 = q1*a0[0]*d1[0] + q2*c2[0]*d2[0];
   bn2 = q1*a0[1]*d1[0] + q2*c2[1]*d2[0];
   cn2 = q1*a0[0]*d1[1] + q2*c2[0]*d2[1];
   dn2 = q1*a0[1]*d1[1] + q2*c2[1]*d2[1];
   vadd_to_matrix_f(fa0,fa1,fa2,Z,
                               ann,an1,an2,bnn,bn1,bn2,cnn,cn1,cn2,dnn,dn1,dn2);
}

#else

void newton_nc_up_aij(fa0,fa1,fa2,nu,Z,v,v1,v2,b0,b1,b2,detB)
FACE *fa0, *fa1, *fa2; INT v, Z; FLOAT nu, *v1, *v2, *b0, *b1, *b2, detB;
{  eprintf("Error: newton_nc_up_aij not available.\n");  }

#endif

#define TRANSF1(a11,a12,a21,a22,d11,d12,d22)                                   \
                               ((a11)*(d11) + ((a12)+(a21))*(d12) + (a22)*(d22))

void nc_aijb_mbub(n0,n1,n2,fa0,fa1,fa2,Z,b0,b1,b2,detB)
NODE *n0, *n1, *n2;                                   /*  detB = volume * nu  */
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], detB;
{
   FLOAT ann, an1, an2, bnn, bn1, bn2, cnn, cn1, cn2, dnn, dn1, dn2, 
         d11, d12, d22;
  
      d11 = DOT(b1,b1);
      d12 = DOT(b1,b2);
      d22 = DOT(b2,b2);

      /* grad fi_i * grad fi_j */
      ann = TRANSF1(136.,116.,116.,136.,d11,d12,d22)/9.*detB;
      an1 = TRANSF1(-116.,-40.,-76.,-20.,d11,d12,d22)/9.*detB;
      an2 = TRANSF1(-20.,-76.,-40.,-116.,d11,d12,d22)/9.*detB;
 
      /* grad fi_i * grad chi_j */
      bnn = TRANSF1(-2.,0.,0.,2.,d11,d12,d22)/9.*detB*NINDI(n2,n1);
      bn1 = TRANSF1(4.,4.,4.,6.,d11,d12,d22)/9.*detB*NINDI(n2,n0);
      bn2 = TRANSF1(6.,4.,4.,4.,d11,d12,d22)/9.*detB*NINDI(n1,n0);
  
      /* grad chi_i * grad fi_j */
      cnn = bnn;
      cn1 = TRANSF1(4.,0.,0.,2.,d11,d12,d22)/9.*detB*NINDI(n2,n1);
      cn2 = TRANSF1(-2.,0.,0.,-4.,d11,d12,d22)/9.*detB*NINDI(n2,n1);

      /* grad chi_i * grad chi_j */
      dnn = TRANSF1(4.,1.,1.,4.,d11,d12,d22)/90.*detB;
      dn1 = TRANSF1(-2.,-1.,-1.,0.,d11,d12,d22)/90.*detB*NINDI(n2,n0)*NINDI(n2,n1);
      dn2 = TRANSF1(0.,1.,1.,2.,d11,d12,d22)/90.*detB*NINDI(n1,n0)*NINDI(n2,n1);

      vadd_to_matrix_f(fa0,fa1,fa2,Z,
                               ann,an1,an2,bnn,bn1,bn2,cnn,cn1,cn2,dnn,dn1,dn2);
}

void dvnc_aijb_mbub(n0,n1,n2,fa0,fa1,fa2,Z,b0,b1,b2,detB)
NODE *n0, *n1, *n2;                                 /*   detB = volume * nu   */
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], detB;
{
   FLOAT ann, an1, an2, bnn, bn1, bn2, cnn, cn1, cn2, dnn, dn1, dn2, c[DIM],
         q29 = 2./9.;
  
      /* grad fi_i * grad fi_j */
      ann = ( 580.*DOT(b0,b0) + 100.*DOT(b1,b1) + 100.*DOT(b2,b2) )*detB/45.;
      SUBTR(b0,b1,c);
      an1 = ( 4.*DOT(b0,b1) - 20.*DOT(c,c)/9. )*detB;
      SUBTR(b0,b2,c);
      an2 = ( 4.*DOT(b0,b2) - 20.*DOT(c,c)/9. )*detB;
  
      /* grad fi_i * grad chi_j */
      bnn = q29*( DOT(b0,b1) - DOT(b0,b2) )*detB*NINDI(n2,n1);
      bn1 = q29*( 2.*DOT(b0,b0) + DOT(b2,b2) )*detB*NINDI(n2,n0);
      bn2 = q29*( 2.*DOT(b0,b0) + DOT(b1,b1) )*detB*NINDI(n1,n0);
  
      /* grad chi_i * grad fi_j */
      cnn = bnn;
      cn1 = q29*( 2.*DOT(b1,b1) + DOT(b2,b2) )*detB*NINDI(n2,n1);
      cn2 = q29*( 2.*DOT(b2,b2) + DOT(b1,b1) )*detB*NINDI(n1,n2);
  
      /* grad chi_i * grad chi_j */
      dnn = ( 2.*DOT(b1,b1) + 2.*DOT(b2,b2) + DOT(b1,b2) )*detB/45.;
      dn1 = DOT(b0,b1)*detB/45.*NINDI(n0,n2)*NINDI(n1,n2);
      dn2 = DOT(b0,b2)*detB/45.*NINDI(n0,n1)*NINDI(n2,n1);
  
      vadd_to_matrix_f(fa0,fa1,fa2,Z,
                               ann,an1,an2,bnn,bn1,bn2,cnn,cn1,cn2,dnn,dn1,dn2);
}

void p1mod_Laplace_matr(tGrid,nu,Z)
GRID *tGrid;
FLOAT nu;
INT Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT ndetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      ndetB = nu*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
      dvnc_aijb_mbub(n0,n1,n2,fa0,fa1,fa2,Z,b[0],b[1],b[2],ndetB);
      dvnc_aijb_mbub(n1,n2,n0,fa1,fa2,fa0,Z,b[1],b[2],b[0],ndetB);
      dvnc_aijb_mbub(n2,n0,n1,fa2,fa0,fa1,Z,b[2],b[0],b[1],ndetB);
/*  equivalently,  nc_aijb_mbub  can be used  */
   } 
}

void nc_Laplace_matr(tGrid,nu,Z,mstruct)
GRID *tGrid;
FLOAT nu;
INT Z, mstruct;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT ndetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      ndetB = nu*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
      vnc_aij(fa0,fa1,fa2,Z,b[0],b[1],b[2],ndetB,mstruct);
      vnc_aij(fa1,fa2,fa0,Z,b[1],b[2],b[0],ndetB,mstruct);
      vnc_aij(fa2,fa0,fa1,Z,b[2],b[0],b[1],ndetB,mstruct);
   } 
}

void vnc_aij_Korn(fa0,fa1,fa2,Z,b0,b1,b2,detB)      /*   detB = volume * nu   */
FACE *fa0, *fa1, *fa2;  
INT Z;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], detB;
{
   FLOAT ann, an1, an2, bnn, bn1, bn2, cnn, cn1, cn2, dnn, dn1, dn2, c;
  
      detB *= 4.;

      c = DOT(b0,b0);
      ann = (c + b0[0]*b0[0])*detB;
      bnn =      b0[0]*b0[1] *detB;
      cnn = bnn;
      dnn = (c + b0[1]*b0[1])*detB;

      c = DOT(b0,b1);
      an1 = (c + b1[0]*b0[0])*detB;
      bn1 =      b1[0]*b0[1] *detB;
      cn1 =      b1[1]*b0[0] *detB;
      dn1 = (c + b1[1]*b0[1])*detB;

      c = DOT(b0,b2);
      an2 = (c + b2[0]*b0[0])*detB;
      bn2 =      b2[0]*b0[1] *detB;
      cn2 =      b2[1]*b0[0] *detB;
      dn2 = (c + b2[1]*b0[1])*detB;

      vadd_to_matrix_f(fa0,fa1,fa2,Z,
                               ann,an1,an2,bnn,bn1,bn2,cnn,cn1,cn2,dnn,dn1,dn2);
}

void nc_Laplace_matr_Korn(tGrid,nu,Z)
GRID *tGrid;
FLOAT nu;
INT Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT ndetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      ndetB = nu*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
      vnc_aij_Korn(fa0,fa1,fa2,Z,b[0],b[1],b[2],ndetB);
      vnc_aij_Korn(fa1,fa2,fa0,Z,b[1],b[2],b[0],ndetB);
      vnc_aij_Korn(fa2,fa0,fa1,Z,b[2],b[0],b[1],ndetB);
   } 
}

#if (F_DATA & ONE_FACE_MATR) && (ELEMENT_TYPE == SIMPLEX) && (DIM == 2)

void nc_smass_matr(tGrid,Z,tau)
GRID *tGrid;
INT Z;
FLOAT tau;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT ndetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      ndetB = VOLUME(pelem)/tau/3.;
      COEFF_FF(fa0,Z) += ndetB;
      COEFF_FF(fa1,Z) += ndetB;
      COEFF_FF(fa2,Z) += ndetB;
   } 
}

#else

void nc_smass_matr(tGrid,Z,tau)
GRID *tGrid; INT Z; FLOAT tau;
{ eprintf("Error: nc_smass_matr not available.\n"); }

#endif

#if (F_DATA & VECTOR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX)

void nc_upwind_matr(tGrid,nu,v,Z,mstruct)
GRID *tGrid;
FLOAT nu;
INT v, Z, mstruct;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT c[DIM], v0[DIM], v1[DIM], v2[DIM], b[DIM2][DIM2], ndetB, q=2./3.;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      ndetB = q*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                        n2->myvertex->x,b);
      c[0] = q*(FDV(fa0,v,0) + FDV(fa1,v,0) + FDV(fa2,v,0));
      c[1] = q*(FDV(fa0,v,1) + FDV(fa1,v,1) + FDV(fa2,v,1));
      v0[0] = c[0] - FDV(fa0,v,0);
      v1[0] = c[0] - FDV(fa1,v,0);
      v2[0] = c[0] - FDV(fa2,v,0);
      v0[1] = c[1] - FDV(fa0,v,1);
      v1[1] = c[1] - FDV(fa1,v,1);
      v2[1] = c[1] - FDV(fa2,v,1);
      nc_up_aij(fa0,fa1,fa2,nu,Z,v,v1,v2,b[0],b[1],b[2],ndetB,mstruct);
      nc_up_aij(fa1,fa2,fa0,nu,Z,v,v2,v0,b[1],b[2],b[0],ndetB,mstruct);
      nc_up_aij(fa2,fa0,fa1,nu,Z,v,v0,v1,b[2],b[0],b[1],ndetB,mstruct);
   } 
}

void newton_nc_upwind_matr(tGrid,nu,v,Z)
GRID *tGrid;
FLOAT nu;
INT v, Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT c[DIM], v0[DIM], v1[DIM], v2[DIM], b[DIM2][DIM2], ndetB, q=4./9.;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      ndetB = q*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                        n2->myvertex->x,b);
      c[0] = 2./3.*(FDV(fa0,v,0) + FDV(fa1,v,0) + FDV(fa2,v,0));
      c[1] = 2./3.*(FDV(fa0,v,1) + FDV(fa1,v,1) + FDV(fa2,v,1));
      SET11(v0,c,FDVP(fa0,v))
      SET11(v1,c,FDVP(fa1,v))
      SET11(v2,c,FDVP(fa2,v))
      newton_nc_up_aij(fa0,fa1,fa2,nu,Z,v,v1,v2,b[0],b[1],b[2],ndetB);
      newton_nc_up_aij(fa1,fa2,fa0,nu,Z,v,v2,v0,b[1],b[2],b[0],ndetB);
      newton_nc_up_aij(fa2,fa0,fa1,nu,Z,v,v0,v1,b[2],b[0],b[1],ndetB);
   } 
}

#else

void nc_upwind_matr(tGrid,nu,v,Z,mstruct)
GRID *tGrid; FLOAT nu; INT v, Z, mstruct;
{  eprintf("Error: nc_upwind_matr not available.\n");  }

void newton_nc_upwind_matr(tGrid,nu,v,Z)
GRID *tGrid; FLOAT nu; INT v, Z;
{  eprintf("Error: newton_nc_upwind_matr not available.\n");  }

#endif

#if (DATA_S & F_LINK_TO_FACES) && (F_DATA & ONE_FACE_MATR) && (ELEMENT_TYPE == CUBE) && (DIM == 2)

/*  only for square elements  */
void q1rot_aij(fa0,fa1,fa2,fa3,Z,e,nu)
FACE *fa0, *fa1, *fa2, *fa3;
INT Z;
FLOAT e, nu;
{
   FLINK *pfli;
   FLOAT d=2.*e*nu;
  
   COEFF_FF(fa0,Z) += d+nu;
   for (pfli=fa0->tfstart; pfli->nbface!=fa1; pfli=pfli->next);
   COEFF_FL(pfli,Z) = -d;
   for (pfli=fa0->tfstart; pfli->nbface!=fa2; pfli=pfli->next);
   COEFF_FL(pfli,Z) = d-nu;
   for (pfli=fa0->tfstart; pfli->nbface!=fa3; pfli=pfli->next);
   COEFF_FL(pfli,Z) = -d;
}

void q1rot_Laplace_matr(tGrid,nu,Z)
GRID *tGrid;
FLOAT nu;
INT Z;
{
   FACE *fa0, *fa1, *fa2, *fa3;
   ELEMENT *pelem;
   FLOAT e = Q1ROT_E;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      FACES_OF_4ELEMENT(fa0,fa1,fa2,fa3,pelem);
      q1rot_aij(fa0,fa1,fa2,fa3,Z,e,nu);
      q1rot_aij(fa1,fa2,fa3,fa0,Z,e,nu);
      q1rot_aij(fa2,fa3,fa0,fa1,Z,e,nu);
      q1rot_aij(fa3,fa0,fa1,fa2,Z,e,nu);
   } 
}

#else

void q1rot_Laplace_matr(tGrid,nu,Z)
GRID *tGrid; FLOAT nu; INT Z;
{ eprintf("Error: q1rot_Laplace_matr not available.\n"); }

#endif

#if (DATA_S & F_LINK_TO_FACES) && (F_DATA & DxD_FACE_MATR) && (ELEMENT_TYPE == CUBE) && (DIM == 2)

/*  only for square elements with sides parallel to the axes  */
void vq1rot_aij_Korn(n0,n1,fa0,fa1,fa2,fa3,Z,e,nu)
NODE *n0, *n1;
FACE *fa0, *fa1, *fa2, *fa3;
INT Z;
FLOAT e, nu;
{
   FLINK *pfli;
   FLOAT c=-nu, d=3.*e*nu;
   INT i0=0, i1=1;
  
   if (fabs(n0->myvertex->x[1]-n1->myvertex->x[1]) < 1.e-10){
      i0 = 1;
      i1 = 0;
      c  = nu;
   }
   else if (fabs(n0->myvertex->x[0]-n1->myvertex->x[0]) > 1.e-10)
      eprintf("Error: element not parallel to the axes.\n");
   SET_COEFFNN2(fa0,Z,i0,i1,d+2.*nu,0.,0.,d+nu);
   for (pfli=fa0->tfstart; pfli->nbface!=fa1; pfli=pfli->next);
   SET_COEFFNN2(pfli,Z,i0,i1,-d,0.,-c,-d);
   for (pfli=fa0->tfstart; pfli->nbface!=fa2; pfli=pfli->next);
   SET_COEFFNN2(pfli,Z,i0,i1,d-2.*nu,0.,0.,d-nu);
   for (pfli=fa0->tfstart; pfli->nbface!=fa3; pfli=pfli->next);
   SET_COEFFNN2(pfli,Z,i0,i1,-d,0.,c,-d);
}

void q1rot_Laplace_matr_Korn(tGrid,nu,Z)
GRID *tGrid;
FLOAT nu;
INT Z;
{
   NODE *n0, *n1, *n2, *n3;
   FACE *fa0, *fa1, *fa2, *fa3;
   ELEMENT *pelem;
   FLOAT e = Q1ROT_E;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_4ELEMENT(n0,n1,n2,n3,pelem);
      FACES_OF_4ELEMENT(fa0,fa1,fa2,fa3,pelem);
      vq1rot_aij_Korn(n0,n1,fa0,fa1,fa2,fa3,Z,e,nu);
      vq1rot_aij_Korn(n1,n2,fa1,fa2,fa3,fa0,Z,e,nu);
      vq1rot_aij_Korn(n2,n3,fa2,fa3,fa0,fa1,Z,e,nu);
      vq1rot_aij_Korn(n3,n0,fa3,fa0,fa1,fa2,Z,e,nu);
   } 
}

#else

void q1rot_Laplace_matr_Korn(tGrid,nu,Z)
GRID *tGrid; FLOAT nu; INT Z;
{ eprintf("Error: q1rot_Laplace_matr_Korn not available.\n"); }

#endif

/******************************************************************************/
/*                                                                            */
/*                       SUPG tau at outflow boundaries                       */
/*                                                                            */
/******************************************************************************/

#if (E_DATA & E_TAU) && (ELEMENT_TYPE == SIMPLEX)

DOUBLE mean_value1_on_element(pel,f)
ELEMENT *pel;
DOUBLE (*f)();
{
   return((f(pel->n[0]->myvertex->x) + 
           f(pel->n[1]->myvertex->x) +
           f(pel->n[2]->myvertex->x))/3.);
}

DOUBLE mean_value_on_element(pel,f)
ELEMENT *pel;
DOUBLE (*f)();
{
   FLOAT *x0, *x1, *x2, x01[DIM], x02[DIM], x12[DIM], x012[DIM];

   VERTICES_OF_ELEMENT(x0,x1,x2,pel);
   AVERAGE(x0,x1,x01);
   AVERAGE(x0,x2,x02);
   AVERAGE(x1,x2,x12);
   POINT3(x0,x1,x2,x012);
   return( (3.*((*f)(x0)  + (*f)(x1)  + (*f)(x2)) +
            8.*((*f)(x01) + (*f)(x02) + (*f)(x12)) + 27.*(*f)(x012))/60 );
}

void set_outflow_type(pel,i,j,bb0,bb1,out_mask)
ELEMENT *pel;
DOUBLE (*bb0)(), (*bb1)();
INT i, j, out_mask;
{
   INT k=3-i-j;
   DOUBLE a[DIM], n[DIM], *xi, *xj, *xk;

   if (NOT_FF(pel->f[k])){
      xi = pel->n[i]->myvertex->x;
      xj = pel->n[j]->myvertex->x;
      xk = pel->n[k]->myvertex->x;
      SUBTR(xk,xi,a);
      n[0] = xi[1] - xj[1];
      n[1] = xj[0] - xi[0];
      if (DOT(a,n) > 0.)
         SET8(n,n)
      a[0] = bb0(xi);
      a[1] = bb1(xi);
      if (DOT(a,n) > 0.05*fabs(MAX(a[0],a[1])*MAX(n[0],n[1])))
         NTYPE(pel->n[i]) |= out_mask;      
      a[0] = bb0(xj);
      a[1] = bb1(xj);
      if (DOT(a,n) > 0.05*fabs(MAX(a[0],a[1])*MAX(n[0],n[1])))
         NTYPE(pel->n[j]) |= out_mask;      
   }
}

void get_remaining_node(pel,n0,n1,n2)
ELEMENT *pel;
NODE *n0, *n1, **n2;
{
   if ((n0 == pel->n[0] || n0 == pel->n[1]) && 
       (n1 == pel->n[0] || n1 == pel->n[1]))
      *n2 = pel->n[2];
   else if ((n0 == pel->n[1] || n0 == pel->n[2]) && 
            (n1 == pel->n[1] || n1 == pel->n[2]))
      *n2 = pel->n[0];
   else
      *n2 = pel->n[1];
}

void get_remaining_two_nodes(pel,n0,n1,n2)
ELEMENT *pel;
NODE *n0, **n1, **n2;
{
   if (n0 == pel->n[0]){
       *n1 = pel->n[1];
       *n2 = pel->n[2];
   }
   else if (n0 == pel->n[1]){
            *n1 = pel->n[2];
            *n2 = pel->n[0];
   }
   else{ /* pel->n[2] */
      *n1 = pel->n[0];
      *n2 = pel->n[1];
   }
}

void set_tau_on_single_g1_element(pel,n0,bb,amin)
ELEMENT *pel;
NODE *n0;
DOUBLE bb[DIM], amin;
{
   NODE *n1, *n2, *na;
   DOUBLE b[DIM2][DIM2], a, r=1., s1, s2=0., hK;

   get_remaining_two_nodes(pel,n0,&n1,&n2);
   if (IS_DN(n1) && IS_DN(n2))
      pel->tau = 0.;
   else{
      if (IS_DN(n1))
         EXCHANGE(n1,n2,na)
      barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                              n2->myvertex->x,b);
      hK = sdiameter(pel,b,bb[0],bb[1]);
      a = sqrt(DOT(bb,bb));
      s1 = DOT(bb,b[1]);
      if (!IS_DN(n2))
         s2 = DOT(bb,b[2]);
      if (s1 > -1.e-20 && (IS_DN(n2) || s2 > -1.e-20))
         a *= 2./hK;
      else
         CHECK_TAU(r,-3.*MIN(s1,s2),a,amin,hK) 
      SET_TAU(pel,a,r)
   }
}

/* Let i be the common interior node of pel and peln. Then s=b*grad phi_i|_pel,
   sn=b*grad phi_i|_peln, s2n=b*grad phi_j|_peln, b2n==1 if j is a Dirichlet
   node */
void set_tau_on_neighbour_element(pel,peln,vol,voln,s,sn,s2n,b2n,bbn,hKn,amin)
ELEMENT *pel, *peln;
DOUBLE vol, voln, s, sn, s2n, bbn[DIM], hKn, amin;
INT b2n;
{
   DOUBLE a=sqrt(DOT(bbn,bbn)), r;

   if (sn > -1.e-20 && (b2n || s2n > -1.e-20))
      a *= 2./hKn;
   else{
      r = 1. + vol/voln*(1. + 3.*s*pel->tau);
      CHECK_TAU(r,-3.*sn/r,a,amin,hKn) 
   }
   SET_TAU(peln,a,r)
}

void set_tau_on_element_of_g2(pel,el,n_el,bb0,bb1,amin,out_mask)
ELEMENT *pel, **el;
DOUBLE (*bb0)(), (*bb1)(), amin;
INT n_el, out_mask;
{
   ELEMENT *pel1, *pel2;
   NODE *n0, *n1, *n2, *n11, *n22;
   DOUBLE b[DIM2][DIM2], bb[DIM], bb_1[DIM], bb_2[DIM], hK, vol, vol1, vol2,
          s, a, r=1.;
   INT j, k;

   if (!IS_DN(pel->n[0])) n0 = pel->n[0];
   else if (!IS_DN(pel->n[1])) n0 = pel->n[1];
   else n0 = pel->n[2];
   get_remaining_two_nodes(pel,n0,&n1,&n2);
   pel1 = pel2 = NULL;
   for (k = 0, j = 1; k < n_el && j; k++)
      if ((el[k]->n[0] == n0 || el[k]->n[1] == n0 || el[k]->n[2] == n0) &&
          (el[k]->n[0] == n1 || el[k]->n[1] == n1 || el[k]->n[2] == n1) &&
           el[k] != pel)
         j = 0;
   if (!j && !IS_FROM_G2(el[k-1],out_mask))
      pel1 = el[k-1];
   for (k = 0, j = 1; k < n_el && j; k++)
      if ((el[k]->n[0] == n0 || el[k]->n[1] == n0 || el[k]->n[2] == n0) &&
          (el[k]->n[0] == n2 || el[k]->n[1] == n2 || el[k]->n[2] == n2) &&
           el[k] != pel)
         j = 0;
   if (!j && !IS_FROM_G2(el[k-1],out_mask))
      pel2 = el[k-1];
   bb[0] = mean_value_on_element(pel,bb0);
   bb[1] = mean_value_on_element(pel,bb1);
   vol = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                 n2->myvertex->x,b);
   hK = sdiameter(pel,b,bb[0],bb[1]);
   s = DOT(bb,b[0]);
   a = sqrt(DOT(bb,bb));
   if (s > -1.e-20)
      a *= 2./hK;
   else{
      if (pel1){
         bb_1[0] = mean_value_on_element(pel1,bb0);
         bb_1[1] = mean_value_on_element(pel1,bb1);
         get_remaining_node(pel1,n0,n1,&n11);
         vol1 = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                       n11->myvertex->x,b);
         r += vol1/vol*(1. + 3.*DOT(bb_1,b[0])*pel1->tau);
      }
      if (pel2){
         bb_2[0] = mean_value_on_element(pel2,bb0);
         bb_2[1] = mean_value_on_element(pel2,bb1);
         get_remaining_node(pel2,n0,n2,&n22);
         vol2 = barycentric_coordinates(n0->myvertex->x,n2->myvertex->x,
                                                       n22->myvertex->x,b);
         r += vol2/vol*(1. + 3.*DOT(bb_2,b[0])*pel2->tau);
      }
      CHECK_TAU(r,-3.*s/r,a,amin,hK)
   }
   SET_TAU(pel,a,r)
}

void test_j(s1,s2,j,k,l)
DOUBLE *s1, *s2;
INT j, k, l;
{
   INT i, m=0;

   for (i = k; i < j-1; i++)
      if (s1[i] >= 0. || s2[i] < 0.)
         m = 1;
   for (i = j+1; i <= l; i++)
      if (s1[i] < 0. || s2[i] >= 0.)
         m = 1;
   if (j > k && (s2[k-1] < 0. || s1[j-1] >= 0.))
      m = 1;
   if (j <= l && (s2[j] >= 0. || s1[l+1] < 0.))
      m = 1;
   if (m)
      eprintf("Error: conditions from Lemma 3 not satisfied.\n");
} 
   
void set_tau_on_g1z_from_lemma3(z_el,pn,z_n,z_bb,k,l,amin)
ELEMENT **z_el;
NODE *pn, **z_n;
DOUBLE z_bb[][DIM], amin;
INT k, l;
{
   DOUBLE b[DIM2][DIM2], s1[20], s2[20], vol[20], hK[20];
   INT i, j, m=l+2;

   for (i = k; i < m; i++){
      vol[i] = barycentric_coordinates(pn->myvertex->x,z_n[i]->myvertex->x,
                                                       z_n[i-1]->myvertex->x,b);
      hK[i] = sdiameter(z_el[i],b,z_bb[i][0],z_bb[i][1]);
      s1[i]   = DOT(b[1],z_bb[i]);
      s2[i-1] = DOT(b[2],z_bb[i]);
   }
   if (s1[k] < 0.){
      j = k + 1;
      while (j < m && s1[j] < 0.) j++;
      j--;
   }
   else
      j = k;
   test_j(s1,s2,j,k,l);
   set_tau_on_single_g1_element(z_el[j],pn,z_bb[j],amin);
   for (i = j-1; i >= k; i--)
      set_tau_on_neighbour_element(z_el[i+1],z_el[i],vol[i+1],vol[i],
                    s2[i],s1[i],s2[i-1],IS_DN(z_n[i-1]),z_bb[i],hK[i],amin);
   for (i = j; i <= l; i++)
      set_tau_on_neighbour_element(z_el[i],z_el[i+1],vol[i],vol[i+1],
                    s1[i],s2[i],s1[i+1],IS_DN(z_n[i+1]),z_bb[i+1],hK[i+1],amin);
}

void construct_elements_around_z(pn,n_el,el,m,z_el,z_n)
NODE *pn, **z_n;
ELEMENT **el, **z_el;
INT n_el, *m;
{
   ELEMENT *z_el_a[20], *pel;
   INT ind0[20], ind[40], fl[20], i, j, k, k1, k2, l;
   NODE *z_na[40], *n0, *n1, *n2, *n3, *n4;

   k = 0;
   for (i = 0; i < n_el; i++)
      if (el[i]->n[0] == pn || el[i]->n[1] == pn || el[i]->n[2] == pn)
         ind0[k++] = i;
   for (i = 0; i < k; i++)
      fl[i] = 1;
   k1 = k2 = 20; 
   ind[k1] = ind0[0];
   fl[0] = 0;
   get_remaining_two_nodes(el[ind0[0]],pn,&n1,&n2);
   z_na[k1-1] = n1;
   z_na[k2] = n2;
   for (l = 1; l < k; l++){
      i = 0;
      j = 1;
      while (j)
         if (fl[++i]){
            pel = el[ind0[i]];
            if (pel->n[0] == n1 || pel->n[1] == n1 || pel->n[2] == n1){
               ind[--k1] = ind0[i];
               fl[i] = 0;
               j = 0;
               get_remaining_two_nodes(pel,pn,&n3,&n4);
               if (n3 == n1)
                  n1 = n4;
               else
                  n1 = n3;
               z_na[k1-1] = n1;
            }
            else if (pel->n[0] == n2 || pel->n[1] == n2 || pel->n[2] == n2){
               ind[++k2] = ind0[i];
               fl[i] = 0;
               j = 0;
               get_remaining_two_nodes(pel,pn,&n3,&n4);
               if (n3 == n2)
                  n2 = n4;
               else
                  n2 = n3;
               z_na[k2] = n2;
            }
         }
   }
   l = k1 - 1;
   for (i = k1; i <= k2; i++){
      z_el[i-l] = el[ind[i]];
      z_n[i-l] = z_na[i];
   }
   z_n[0] = z_na[l];
   *m = k;
   for (i = 1; i <= k; i++){
      NODES_OF_ELEMENT(n0,n1,n2,z_el[i]);
      if ((n0 != pn && n1 != pn && n2 != pn) ||
          (n0 != z_n[i-1] && n1 != z_n[i-1] && n2 != z_n[i-1]) ||
          (n0 != z_n[i] && n1 != z_n[i] && n2 != z_n[i]))
         eprintf("Error: elements arround z not correct.\n");
   }
}

void check_values_of_tau(el,n_el,bb0,bb1,out_mask)
ELEMENT **el;
DOUBLE (*bb0)(), (*bb1)();
INT n_el, out_mask;
{
   NODE *n0, *n1, *n2;
   DOUBLE b[DIM2][DIM2], bb[DIM], sum, vol;
   INT i, j, k, l=0;

/*
   for (i = 0; i < n_el; i++)
   if (!IS_FROM_G2(el[i],out_mask))
      printf("%4i  (%4.2f,%4.2f)  (%4.2f,%4.2f)  (%4.2f,%4.2f)\n",
         i,el[i]->n[0]->myvertex->x[0],el[i]->n[0]->myvertex->x[1],
           el[i]->n[1]->myvertex->x[0],el[i]->n[1]->myvertex->x[1],
           el[i]->n[2]->myvertex->x[0],el[i]->n[2]->myvertex->x[1]);
*/
   for (i = 0; i < n_el; i++)
      if (!IS_FROM_G2(el[i],out_mask))
         printf("G1: %2i  %12.10f\n",i,el[i]->tau);
   for (i = 0; i < n_el; i++)
      if (IS_FROM_G2(el[i],out_mask))
         printf("G2: %2i  %12.10f\n",i,el[i]->tau);

   for (i = 0; i < n_el; i++)
      for (j = 0; j < 3; j++)
         if (!IS_DN((n0=el[i]->n[j]))){
            sum = 0.;
            for (k = 0; k < n_el; k++)
               if (el[k]->n[0] == n0 || el[k]->n[1] == n0 || el[k]->n[2] == n0){
                  get_remaining_two_nodes(el[k],n0,&n1,&n2);
                  vol = barycentric_coordinates(n0->myvertex->x,
                                n1->myvertex->x,n2->myvertex->x,b);
                  bb[0] = mean_value_on_element(el[k],bb0);
                  bb[1] = mean_value_on_element(el[k],bb1);
                  sum += vol*(1./3. + el[k]->tau*DOT(bb,b[0])); 
               }
            if (fabs(sum) > 1.e-12){
               l = 1;
               printf("sum = %e\n",sum);
            }
         }
   if (l)
      eprintf("Error: values of tau are not correct.\n");
   else
      printf("All values of tau are correct.\n");
}

void set_supg_tau(tGrid,eps,bb0,bb1,out_mask)
GRID *tGrid;
DOUBLE eps, (*bb0)(), (*bb1)();
INT out_mask;
{
   INT max_el=3000, n_el=0, i, k, l, m;
   DOUBLE b[DIM2][DIM2], bb[DIM], z_bb[20][DIM], hK, Pe, a, amin=0.1;
   NODE *pn, *z_n[20];
   ELEMENT *el[max_el+2], *z_el[20], *pel;

   for (pn = FIRSTN(tGrid); pn != FIRSTNODE(tGrid); pn = pn->succ)
      NTYPE(pn) &= ~out_mask;
   for (pel = FIRSTELEMENT(tGrid); pel; pel = pel->succ){
      set_outflow_type(pel,0,1,bb0,bb1,out_mask);
      set_outflow_type(pel,1,2,bb0,bb1,out_mask);
      set_outflow_type(pel,2,0,bb0,bb1,out_mask);
   }
   for (pel = FIRSTELEMENT(tGrid); pel; pel = pel->succ)
      if (IS_IN_G(pel,out_mask)){
         el[n_el++] = pel;
         if (n_el > max_el)
            eprintf("Error: max_el exceeded.\n");
      }
   for (pn = FIRSTN(tGrid); pn != FIRSTNODE(tGrid); pn = pn->succ)
      if (NTYPE(pn) & out_mask){
         construct_elements_around_z(pn,n_el,el,&m,z_el,z_n);
         for (i = 1; i <= m; i++){
            z_bb[i][0] = mean_value_on_element(z_el[i],bb0);
            z_bb[i][1] = mean_value_on_element(z_el[i],bb1);
         }
         k = 1;
         while (k <= m){
            while (k <= m && (IS_FROM_G2(z_el[k],out_mask) || HAS_DN(z_el[k])))
               k++;
            if (k <= m){
               l = k + 1;
               while (l <= m && 
                      !IS_FROM_G2(z_el[l],out_mask) && !IS_DN(z_n[l-1])) l++;
               l -= 2;
               if (l < k)
                  set_tau_on_single_g1_element(z_el[k],pn,z_bb[k],amin);
               else
                  set_tau_on_g1z_from_lemma3(z_el,pn,z_n,z_bb,k,l,amin);
               k = l + 2;
            }
         }
      }
   for (i = 0; i < n_el; i++)
      if (IS_FROM_G2(el[i],out_mask) && !HAS_DN(el[i]))
         set_tau_on_element_of_g2(el[i],el,n_el,bb0,bb1,amin,out_mask);
   check_values_of_tau(el,n_el,bb0,bb1,out_mask);
   for (pel = FIRSTELEMENT(tGrid); pel; pel = pel->succ){
      barycentric_coordinates(pel->n[0]->myvertex->x,pel->n[1]->myvertex->x,
                                                     pel->n[2]->myvertex->x,b);
      bb[0] = mean_value_on_element(pel,bb0);
      bb[1] = mean_value_on_element(pel,bb1);
      a  = sqrt(DOT(bb,bb));
      hK = sdiameter(pel,b,bb[0],bb[1]);
      Pe = a*hK/(2.*eps);
      if (IS_IN_G(pel,out_mask)){
         if (Pe < 1.e-10)
            pel->tau = 0.;
         else
            pel->tau *= (1./tanh(Pe) - 1./Pe);
      }
      else if (Pe < 1.e-10)
         pel->tau = hK*hK/(12.*eps);
      else
         pel->tau = hK/(2.*a)*(1./tanh(Pe) - 1./Pe);
   }
}

#else

void set_supg_tau(tGrid,eps,bb0,bb1,out_mask)
GRID *tGrid; DOUBLE eps, (*bb0)(), (*bb1)(); INT out_mask;
{  eprintf("Error: set_supg_tau not available.\n");  }

#endif

/******************************************************************************/
/*                                                                            */
/*         P1c and Q1c stiffness matrices for convection-diffusion eq.        */
/*                                                                            */
/******************************************************************************/

#if N_DATA & ONE_NODE_MATR

void c_p1c_ij(n0,n1,n2,Z,bb_0,s,b0,b1,b2,detB)
NODE *n0, *n1, *n2;                    /* detB = volume/12 */
INT Z;
FLOAT bb_0[DIM], s[DIM], b0[DIM2], b1[DIM2], b2[DIM2], detB;
{
   FLOAT a[DIM];

      SET10(a,s,bb_0)
      COEFFN(n0,Z) += DOT(a,b0)*detB;
      putaij(n0->tstart,n1,n2,DOT(a,b1)*detB,DOT(a,b2)*detB,Z);
}

void p1c_up_ij(n0,n1,n2,fa0,nu,Z,flux0,flux1,flux2)
NODE *n0, *n1, *n2;
FACE *fa0;
INT Z;
FLOAT nu, flux0, flux1, flux2;
{
   FLOAT a0=0., a1, a2, b0, b1, b2;

   if (IS_BF(fa0))
      a0 = flux0*(1. - 2.*upwind_lambda(flux0,nu))/12.;
   a1 = 0.25*flux1*(1. - upwind_lambda(flux1,nu));
   a2 = 0.25*flux2*(1. - upwind_lambda(flux2,nu));
   b0 = a1 + a2 - a0 - a0;
   b1 = a0 - a1;
   b2 = a0 - a2;
   COEFFN(n1,Z) += b1;
   COEFFN(n2,Z) += b2;
   putaij(n1->tstart,n0,n2,b0,b2,Z);
   putaij(n2->tstart,n0,n1,b0,b1,Z);
}

void r_p1c_ij(n0,n1,n2,Z,a0,a1,a2,detB)
NODE *n0, *n1, *n2;
INT Z;
FLOAT a0, a1, a2, detB;
{
      COEFFN(n0,Z) += a0*detB;
      putaij(n0->tstart,n1,n2,a1*detB,a2*detB,Z);
}

void sd_p1c_ij(n0,n1,n2,Z,bb_0,bb_1,bb_2,r0,r1,r2,r,b0,b1,b2,detB)
NODE *n0, *n1, *n2;                                 /* detB = volume*delta/60 */
INT Z;
FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], b0[DIM2], b1[DIM2], b2[DIM2], 
      r0, r1, r2, r, detB;
{
   FLOAT t0[DIM2], t1[DIM2], t2[DIM2], ann, an1, an2, s, z;
  
      t0[0] = DOT(b0,bb_0);
      t0[1] = DOT(b0,bb_1);
      t0[2] = DOT(b0,bb_2);
      t1[0] = DOT(b1,bb_0);
      t1[1] = DOT(b1,bb_1);
      t1[2] = DOT(b1,bb_2);
      t2[0] = DOT(b2,bb_0);
      t2[1] = DOT(b2,bb_1);
      t2[2] = DOT(b2,bb_2);
      s = t0[0] + t0[1] + t0[2];
      z = 2.*s*r;
      ann = (5.*(t0[0]*(s+t0[0]) + t0[1]*(s+t0[1]) + t0[2]*(s+t0[2])) 
             + z + 4.*r0*t0[0] - r1*t0[2] - r2*t0[1])*detB;
      an1 = (5.*(t1[0]*(s+t0[0]) + t1[1]*(s+t0[1]) + t1[2]*(s+t0[2]))
             + z + 4.*r1*t0[1] - r0*t0[2] - r2*t0[0])*detB;
      an2 = (5.*(t2[0]*(s+t0[0]) + t2[1]*(s+t0[1]) + t2[2]*(s+t0[2]))
             + z + 4.*r2*t0[2] - r0*t0[1] - r1*t0[0])*detB;
      COEFFN(n0,Z) += ann;
      putaij(n0->tstart,n1,n2,an1,an2,Z);
}

void sdp1nc_p1c_ij(n0,n1,n2,Z,bb_0,bb_1,bb_2,r0,r1,r2,b0,b1,b2,detB,delta_0,delta_1,delta_2)
NODE *n0, *n1, *n2;                                 /* detB = volume */
INT Z;
FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], b0[DIM2], b1[DIM2], b2[DIM2], 
      r0, r1, r2, detB, delta_0, delta_1, delta_2;
{
   FLOAT t0[DIM2], t1[DIM2], t2[DIM2], ann, an1, an2;
  
      t0[0] = DOT(b0,bb_0);
      t0[1] = DOT(b0,bb_1);
      t0[2] = DOT(b0,bb_2);
      t1[0] = DOT(b1,bb_0);
      t1[1] = DOT(b1,bb_1);
      t1[2] = DOT(b1,bb_2);
      t2[0] = DOT(b2,bb_0);
      t2[1] = DOT(b2,bb_1);
      t2[2] = DOT(b2,bb_2);
      ann = (6.*(3.*delta_0+delta_1+delta_2)   *t0[0]*t0[0] +
             6.*(2.*delta_0+2.*delta_1+delta_2)*t0[0]*t0[1] +
             6.*(2.*delta_0+delta_1+2.*delta_2)*t0[0]*t0[2] +
             6.*(delta_0+3.*delta_1+delta_2)   *t0[1]*t0[1] +
             6.*(delta_0+2.*delta_1+2.*delta_2)*t0[1]*t0[2] +
             6.*(delta_0+delta_1+3.*delta_2)   *t0[2]*t0[2] +
             3.*(4.*delta_0+delta_1+delta_2)*r0*t0[0] +
             (3.*delta_0+2.*delta_1+delta_2)*r0*t0[1] +
             (3.*delta_0+delta_1+2.*delta_2)*r0*t0[2] +
             (3.*delta_0+2.*delta_1+delta_2)*r1*t0[0] +
             (2.*delta_0+3.*delta_1+delta_2)*r1*t0[1] +
             (   delta_0+   delta_1+delta_2)*r1*t0[2] +
             (3.*delta_0+delta_1+2.*delta_2)*r2*t0[0] +
             (   delta_0+   delta_1+delta_2)*r2*t0[1] +
             (2.*delta_0+delta_1+3.*delta_2)*r2*t0[2])
             *detB/180.;
      an1 = (6.*(3.*delta_0+delta_1+delta_2)   *t1[0]*t0[0] +
             3.*(2.*delta_0+2.*delta_1+delta_2)*t1[0]*t0[1] +
             3.*(2.*delta_0+delta_1+2.*delta_2)*t1[0]*t0[2] +
             3.*(2.*delta_0+2.*delta_1+delta_2)*t1[1]*t0[0] +
             6.*(delta_0+3.*delta_1+delta_2)   *t1[1]*t0[1] +
             3.*(delta_0+2.*delta_1+2.*delta_2)*t1[1]*t0[2] +
             3.*(2.*delta_0+delta_1+2.*delta_2)*t1[2]*t0[0] +
             3.*(delta_0+2.*delta_1+2.*delta_2)*t1[2]*t0[1] +
             6.*(delta_0+delta_1+3.*delta_2)   *t1[2]*t0[2] +
             (3.*delta_0+2.*delta_1+delta_2)*r0*t0[0] +
             (2.*delta_0+3.*delta_1+delta_2)*r0*t0[1] +
             (   delta_0+   delta_1+delta_2)*r0*t0[2] +
             (2.*delta_0+3.*delta_1+delta_2)*r1*t0[0] +
             3.*(delta_0+4.*delta_1+delta_2)*r1*t0[1] +
             (delta_0+3.*delta_1+2.*delta_2)*r1*t0[2] +
             (   delta_0+   delta_1+delta_2)*r2*t0[0] +
             (delta_0+3.*delta_1+2.*delta_2)*r2*t0[1] +
             (delta_0+2.*delta_1+3.*delta_2)*r2*t0[2])
             *detB/180.;
      an2 = (6.*(3.*delta_0+delta_1+delta_2)   *t2[0]*t0[0] +
             3.*(2.*delta_0+2.*delta_1+delta_2)*t2[0]*t0[1] +
             3.*(2.*delta_0+delta_1+2.*delta_2)*t2[0]*t0[2] +
             3.*(2.*delta_0+2.*delta_1+delta_2)*t2[1]*t0[0] +
             6.*(delta_0+3.*delta_1+delta_2)   *t2[1]*t0[1] +
             3.*(delta_0+2.*delta_1+2.*delta_2)*t2[1]*t0[2] +
             3.*(2.*delta_0+delta_1+2.*delta_2)*t2[2]*t0[0] +
             3.*(delta_0+2.*delta_1+2.*delta_2)*t2[2]*t0[1] +
             6.*(delta_0+delta_1+3.*delta_2)   *t2[2]*t0[2] +
             (3.*delta_0+delta_1+2.*delta_2)*r0*t0[0] +
             (   delta_0+   delta_1+delta_2)*r0*t0[1] +
             (2.*delta_0+delta_1+3.*delta_2)*r0*t0[2] +
             (   delta_0+   delta_1+delta_2)*r1*t0[0] +
             (delta_0+3.*delta_1+2.*delta_2)*r1*t0[1] +
             (delta_0+2.*delta_1+3.*delta_2)*r1*t0[2] +
             (2.*delta_0+delta_1+3.*delta_2)*r2*t0[0] +
             (delta_0+2.*delta_1+3.*delta_2)*r2*t0[1] +
             3.*(delta_0+delta_1+4.*delta_2)*r2*t0[2])
             *detB/180.;
      COEFFN(n0,Z) += ann;
      putaij(n0->tstart,n1,n2,an1,an2,Z);
}

void sd_p1c_quadr_ij(pelem,i0,i1,i2,Z,bar,bb0,bb1,react,rhs,eps,
                     u,space,sc_type,par1,par2,sd_tau,n,x,w)
ELEMENT *pelem;
INT i0, i1, i2, n, Z, u, space, sc_type;
FLOAT bar[DIM2][DIM2], (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), eps, 
      par1, par2, (*sd_tau)(), x[][2], *w;
{
   NODE *n0, *n1, *n2;
   FLOAT b[DIM][DIM], a[DIM][DIM], c[DIM];
  
   INODES_OF_ELEMENT(n0,n1,n2,i0,i1,i2,pelem);
   P1_reference_mapping0_with_inverse(n0,n1,n2,b,a,c);
   COEFFN(n0,Z) += sd_ref_triang(pelem,r_p1_0,r_p1_0_0,r_p1_0_1,
                                       r_p1_0_00,r_p1_0_01,r_p1_0_11,    
                                       r_p1_0_0,r_p1_0_1,
                                       bar,bb0,bb1,react,eps,rhs,
                                 u,space,sc_type,par1,par2,sd_tau,b,a,c,n,x,w);
   putaij(n0->tstart,n1,n2,sd_ref_triang(pelem,r_p1_1,r_p1_1_0,r_p1_1_1,
                                       r_p1_1_00,r_p1_1_01,r_p1_1_11,
                                       r_p1_0_0,r_p1_0_1,
                                       bar,bb0,bb1,react,eps,rhs,
                                 u,space,sc_type,par1,par2,sd_tau,b,a,c,n,x,w),
                           sd_ref_triang(pelem,r_p1_2,r_p1_2_0,r_p1_2_1,
                                       r_p1_2_00,r_p1_2_01,r_p1_2_11,
                                       r_p1_0_0,r_p1_0_1,
                                       bar,bb0,bb1,react,eps,rhs,
                               u,space,sc_type,par1,par2,sd_tau,b,a,c,n,x,w),Z);
}

void bgu_bgv_p1c_quadr_ij(pelem,i0,i1,i2,Z,bar,bs0,bs1,bb0,bb1,react,rhs,eps,
                          u,space,sc_type,par1,par2,sd_tau,qr)
ELEMENT *pelem;
INT i0, i1, i2, Z, u, space, sc_type;
FLOAT bar[DIM2][DIM2], (*bs0)(), (*bs1)(), (*bb0)(), (*bb1)(), (*react)(), 
      (*rhs)(), eps, par1, par2, (*sd_tau)();
QUADRATURE_RULE qr;
{
   NODE *n0, *n1, *n2;
   FLOAT b[DIM][DIM], a[DIM][DIM], c[DIM];
  
   INODES_OF_ELEMENT(n0,n1,n2,i0,i1,i2,pelem);
   P1_reference_mapping0_with_inverse(n0,n1,n2,b,a,c);
   COEFFN(n0,Z) += bgu_bgv_ref_triang(pelem,r_p1_0_0,r_p1_0_1,
                                      r_p1_0_0,r_p1_0_1,
                                      bar,bs0,bs1,bb0,bb1,react,eps,rhs,
                               u,space,sc_type,par1,par2,sd_tau,b,a,c,qr.n,qr.points,qr.weights);
   putaij(n0->tstart,n1,n2,bgu_bgv_ref_triang(pelem,r_p1_1_0,r_p1_1_1,
                                              r_p1_0_0,r_p1_0_1,
                                              bar,bs0,bs1,bb0,bb1,react,eps,rhs,
                               u,space,sc_type,par1,par2,sd_tau,b,a,c,qr.n,qr.points,qr.weights),
                           bgu_bgv_ref_triang(pelem,r_p1_2_0,r_p1_2_1,
                                              r_p1_0_0,r_p1_0_1,
                                              bar,bs0,bs1,bb0,bb1,react,eps,rhs,
                               u,space,sc_type,par1,par2,sd_tau,b,a,c,qr.n,qr.points,qr.weights),Z);
}

void gu_gv_p1c_quadr_ij(pelem,i0,i1,i2,Z,bar,bb0,bb1,react,rhs,eps,
                        u,space,sc_type,par1,par2,sc_eps,qr)
ELEMENT *pelem;
INT i0, i1, i2, Z, u, space, sc_type;
FLOAT bar[DIM2][DIM2], (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), eps, 
      par1, par2, (*sc_eps)();
QUADRATURE_RULE qr;
{
   NODE *n0, *n1, *n2;
   FLOAT b[DIM][DIM], a[DIM][DIM], c[DIM];
  
   INODES_OF_ELEMENT(n0,n1,n2,i0,i1,i2,pelem);
   P1_reference_mapping0_with_inverse(n0,n1,n2,b,a,c);
   COEFFN(n0,Z) += gu_gv_ref_triang(pelem,r_p1_0_0,r_p1_0_1,
                                    r_p1_0_0,r_p1_0_1,
                                    bar,bb0,bb1,react,eps,rhs,
                               u,space,sc_type,par1,par2,sc_eps,b,a,c,qr.n,qr.points,qr.weights);
   putaij(n0->tstart,n1,n2,gu_gv_ref_triang(pelem,r_p1_1_0,r_p1_1_1,
                                            r_p1_0_0,r_p1_0_1,
                                            bar,bb0,bb1,react,eps,rhs,
                               u,space,sc_type,par1,par2,sc_eps,b,a,c,qr.n,qr.points,qr.weights),
                           gu_gv_ref_triang(pelem,r_p1_2_0,r_p1_2_1,
                                            r_p1_0_0,r_p1_0_1,
                                            bar,bb0,bb1,react,eps,rhs,
                               u,space,sc_type,par1,par2,sc_eps,b,a,c,qr.n,qr.points,qr.weights),Z);
}

void c_q1c_ij(n0,n1,n2,n3,Z,bb0,bb1,qr)
NODE *n0, *n1, *n2, *n3;
INT Z;
FLOAT (*bb0)(), (*bb1)();
QUADRATURE_RULE qr;
{
   FLOAT b[2][2][DIM2], a[DIM][DIM], c[DIM], alpha[DIM], jac[DIM2];
  
   Q1_reference_mapping_with_inverse(n0,n1,n2,n3,b,a,c,alpha,jac);
   COEFFN(n0,Z) += 
   b_grad_u_v_ref(r_q1_0_0,r_q1_0_1,r_q1_0,bb0,bb1,b,a,c,alpha,jac,
                  qr.n,qr.points,qr.weights);
   putaij3(n0->tstart,n1,n2,n3,
   b_grad_u_v_ref(r_q1_1_0,r_q1_1_1,r_q1_0,bb0,bb1,b,a,c,alpha,jac,
                  qr.n,qr.points,qr.weights),
   b_grad_u_v_ref(r_q1_2_0,r_q1_2_1,r_q1_0,bb0,bb1,b,a,c,alpha,jac,
                  qr.n,qr.points,qr.weights),
   b_grad_u_v_ref(r_q1_3_0,r_q1_3_1,r_q1_0,bb0,bb1,b,a,c,alpha,jac,
                  qr.n,qr.points,qr.weights),Z);
}

void r_q1c_ij(n0,n1,n2,n3,Z,r,qr)
NODE *n0, *n1, *n2, *n3;
INT Z;
FLOAT (*r)();
QUADRATURE_RULE qr;
{
   FLOAT a[DIM][DIM], c[DIM], alpha[DIM], jac[DIM2];
  
   Q1_reference_mapping(n0,n1,n2,n3,a,alpha,c,jac);
   COEFFN(n0,Z) += d_u_v_ref(r_q1_0,r_q1_0,r,a,c,alpha,jac,
                             qr.n,qr.points,qr.weights);
   putaij3(n0->tstart,n1,n2,n3,
          d_u_v_ref(r_q1_1,r_q1_0,r,a,c,alpha,jac,qr.n,qr.points,qr.weights),
          d_u_v_ref(r_q1_2,r_q1_0,r,a,c,alpha,jac,qr.n,qr.points,qr.weights),
          d_u_v_ref(r_q1_3,r_q1_0,r,a,c,alpha,jac,qr.n,qr.points,qr.weights),Z);
}

void sd_q1c_ij(n0,n1,n2,n3,Z,bb0,bb1,react,delta,qr)
NODE *n0, *n1, *n2, *n3;
INT Z;
FLOAT (*bb0)(), (*bb1)(), (*react)(), delta;
QUADRATURE_RULE qr;
{
   FLOAT b[2][2][DIM2], a[DIM][DIM], c[DIM], alpha[DIM], jac[DIM2];
  
   Q1_reference_mapping_with_inverse(n0,n1,n2,n3,b,a,c,alpha,jac);
   COEFFN(n0,Z) += sd_ref(r_q1_0,r_q1_0_0,r_q1_0_1,r_q1_0_0,r_q1_0_1,
                 bb0,bb1,react,delta,b,a,c,alpha,jac,qr.n,qr.points,qr.weights);
   putaij3(n0->tstart,n1,n2,n3,
           sd_ref(r_q1_1,r_q1_1_0,r_q1_1_1,r_q1_0_0,r_q1_0_1,
              bb0,bb1,react,delta,b,a,c,alpha,jac,qr.n,qr.points,qr.weights),
           sd_ref(r_q1_2,r_q1_2_0,r_q1_2_1,r_q1_0_0,r_q1_0_1,
              bb0,bb1,react,delta,b,a,c,alpha,jac,qr.n,qr.points,qr.weights),
           sd_ref(r_q1_3,r_q1_3_0,r_q1_3_1,r_q1_0_0,r_q1_0_1,
              bb0,bb1,react,delta,b,a,c,alpha,jac,qr.n,qr.points,qr.weights),Z);
}

void macro_conv_stab_q1c_ij(n0,n1,n2,n3,Z,bx,delta,qr)
NODE *n0, *n1, *n2, *n3;
INT Z;
DOUBLE *bx, delta;
QUADRATURE_RULE qr;
{
   FLOAT b[2][2][DIM2], jac[DIM2];
  
   inverse_of_Q1_reference_mapping(n0,n1,n2,n3,b,jac);
   COEFFN(n0,Z) += macro_conv_stab_ref(r_q1_0_0,r_q1_0_1,r_q1_0_0,r_q1_0_1,
                                 bx,delta,b,jac,qr.n,qr.points,qr.weights);
   putaij3(n0->tstart,n1,n2,n3,
              macro_conv_stab_ref(r_q1_1_0,r_q1_1_1,r_q1_0_0,r_q1_0_1,
                                  bx,delta,b,jac,qr.n,qr.points,qr.weights),
              macro_conv_stab_ref(r_q1_2_0,r_q1_2_1,r_q1_0_0,r_q1_0_1,
                                  bx,delta,b,jac,qr.n,qr.points,qr.weights),
              macro_conv_stab_ref(r_q1_3_0,r_q1_3_1,r_q1_0_0,r_q1_0_1,
                                  bx,delta,b,jac,qr.n,qr.points,qr.weights),Z);
}

void P1_tangential_derivatives_on_triangle(pelem,u,du,le)
ELEMENT *pelem;
DOUBLE *du, *le;
INT u;
{
   NODE *n0, *n1, *n2;
   FLOAT *x0, *x1, *x2, t[DIM];

   NODES_OF_ELEMENT(n0,n1,n2,pelem);
   VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
   SUBTR(x1,x2,t);
   le[0] = sqrt(DOT(t,t));
   du[0] = (NDS(n2,u)-NDS(n1,u))/le[0];
   SUBTR(x2,x0,t);
   le[1] = sqrt(DOT(t,t));
   du[1] = (NDS(n0,u)-NDS(n2,u))/le[1];
   SUBTR(x0,x1,t);
   le[2] = sqrt(DOT(t,t));
   du[2] = (NDS(n1,u)-NDS(n0,u))/le[2];
}

void sc_phi_on_edges_old(pelem,eps,u,bar,rdetB,bb0,bb1,react,rhs,space,sc_type,
                        beta1,beta2,phi0,phi1,phi2)
ELEMENT *pelem;
FLOAT eps, bar[DIM2][DIM2], rdetB, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), 
      beta1, beta2, *phi0, *phi1, *phi2;
INT u, space, sc_type;
{
   NODE *n0, *n1, *n2;
   FLOAT *x0, *x1, *x2, x01[DIM], x02[DIM], x12[DIM], 
         bb_0[DIM], bb_1[DIM], bb_2[DIM], bb, a, max, r0, r1, r2;

   NODES_OF_ELEMENT(n0,n1,n2,pelem);
   VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
   MIDPOINTS(x0,x1,x2,x01,x02,x12)
   if (sc_type == SCM_BH){
      V_MID_VALUES(x01,x02,x12,bb0,bb1,bb_0,bb_1,bb_2)
      a = (bb_0[0] + bb_1[0] + bb_2[0])/3.;
      bb = a*a;
      a = (bb_0[1] + bb_1[1] + bb_2[1])/3.;
      bb = sqrt(bb + a*a);
      max = fabs(p1c_jump_of_normal_derivative(n0,n1,n2,pelem->f[2],u));
      a = fabs(p1c_jump_of_normal_derivative(n0,n2,n1,pelem->f[1],u));
      max = MAX(a,max);
      a = fabs(p1c_jump_of_normal_derivative(n1,n2,n0,pelem->f[0],u));
      max = MAX(a,max);
      a = diameter(pelem);
      (*phi0) = (*phi1) = (*phi2) = a*(beta2*eps + beta1*bb*a)*max;
   }
   else if (sc_type == SCM_BE2){
      V_MID_VALUES(x01,x02,x12,bb0,bb1,bb_0,bb_1,bb_2)
      a = diameter(pelem);
      a *= beta1*a;
      *phi0 = a*sqrt(DOT(bb_0,bb_0))*
                    fabs(p1c_jump_of_normal_derivative(n1,n2,n0,pelem->f[0],u));
      *phi1 = a*sqrt(DOT(bb_1,bb_1))*
                    fabs(p1c_jump_of_normal_derivative(n0,n2,n1,pelem->f[1],u));
      *phi2 = a*sqrt(DOT(bb_2,bb_2))*
                    fabs(p1c_jump_of_normal_derivative(n0,n1,n2,pelem->f[2],u));
   }
   else if (sc_type == SCM_BE3){
      r0 = scalar_conv_diff_res(pelem,x0,bar,u,space,eps,bb0,bb1,react,rhs);
      r1 = scalar_conv_diff_res(pelem,x1,bar,u,space,eps,bb0,bb1,react,rhs);
      r2 = scalar_conv_diff_res(pelem,x2,bar,u,space,eps,bb0,bb1,react,rhs);
      beta1 *= rdetB;
      *phi0 = beta1*mean_abs_value_1d(r1,r2);
      *phi1 = beta1*mean_abs_value_1d(r2,r0);
      *phi2 = beta1*mean_abs_value_1d(r0,r1);
/*
      *phi0 = beta1*rdetB*fabs(scalar_conv_diff_res(pelem,x12,
                                            bar,u,space,eps,bb0,bb1,react,rhs));
      *phi1 = beta1*rdetB*fabs(scalar_conv_diff_res(pelem,x02,
                                            bar,u,space,eps,bb0,bb1,react,rhs));
      *phi2 = beta1*rdetB*fabs(scalar_conv_diff_res(pelem,x01,
                                            bar,u,space,eps,bb0,bb1,react,rhs));
*/
   }
   else
      eprintf("Error: wrong sc_type in sc_phi_on_edges_old.\n");
}

void edge_stabilization_old(pelem,eps,Z,u,bb0,bb1,react,rhs,space,sc_type,
                            beta1,beta2)
ELEMENT *pelem;
FLOAT eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), beta1, beta2;
INT Z, u, space, sc_type;
{
   NODE *n0, *n1, *n2;
   FLOAT *x0, *x1, *x2, grad_u[DIM], t[DIM], b[DIM2][DIM2], a0, a1, a2, p,
         rdetB;

   if (space != P1C)
      eprintf("Error: edge_stabilization available for P1C only.\n");
   else{
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      rdetB = barycentric_coordinates(x0,x1,x2,b); 
      sc_phi_on_edges_old(pelem,eps,u,b,rdetB,bb0,bb1,react,rhs,space,sc_type,
                          beta1,beta2,&a0,&a1,&a2);
      sgrad_value(pelem,x0,b,u,space,grad_u);
      SUBTR(x1,x2,t);
      UNIT_VECTOR(t,t,p)
      a0 *= regularized_sign_divided_by_x(DOT(t,grad_u))/p;
      SUBTR(x0,x2,t);
      UNIT_VECTOR(t,t,p)
      a1 *= regularized_sign_divided_by_x(DOT(t,grad_u))/p;
      SUBTR(x0,x1,t);
      UNIT_VECTOR(t,t,p)
      a2 *= regularized_sign_divided_by_x(DOT(t,grad_u))/p;
      COEFFN(n0,Z) += a1 + a2;
      COEFFN(n1,Z) += a0 + a2;
      COEFFN(n2,Z) += a0 + a1;
      putaij(n0->tstart,n1,n2,-a2,-a1,Z);
      putaij(n1->tstart,n2,n0,-a0,-a2,Z);
      putaij(n2->tstart,n0,n1,-a1,-a0,Z);
   }
}

void coeff_for_GC2_Newton(y,pelem,bar,eps,bb0,bb1,react,rhs,u,space,par1,
                          coeff0,coeff1,coeff2)
ELEMENT *pelem;
FLOAT *y, bar[DIM2][DIM2], eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(),
      par1, *coeff0, *coeff1, *coeff2;
INT u, space;
{
   DOUBLE bb0y, bb1y, bb, bb2, g2, gu, r, res, rres, s, t1, w, z, grad_u[DIM];

   sgrad_value(pelem,y,bar,u,space,grad_u);
   if ( (g2=DOT(grad_u,grad_u)) < 1.e-15 )
      *coeff0 = coeff1[0] = coeff1[1] = coeff2[0] = coeff2[1] = 0.;
   else{
      res = scalar_conv_diff_res(pelem,y,bar,u,space,eps,bb0,bb1,react,rhs);
      if (REG_FABS == YES)
         rres = regularized_fabs(res,REG_EPS);
      else
         rres = fabs(res);
      bb0y = bb0(y);
      bb1y = bb1(y);
      bb2 = bb0y*bb0y + bb1y*bb1y;
      if (rres*rres < bb2*g2){
         gu = sqrt(g2);
         bb = sqrt(bb2);
         r = rres/gu;
         t1 = sd_tau_classic(pelem,bar,eps,bb0y,bb1y,space);
         *coeff0 = t1*r*(bb - r);
         coeff1[0] = grad_u[0]/gu;
         coeff1[1] = grad_u[1]/gu;
         z = t1*(bb - 2.*r);
         s = rres/g2;
         if (REG_FABS == YES)
            w = dx_of_regularized_fabs(res,REG_EPS);
         else
            w = SGN(res);
         coeff2[0] = (w*bb0y - s*grad_u[0])*z;
         coeff2[1] = (w*bb1y - s*grad_u[1])*z;
      }
      else
         *coeff0 = coeff1[0] = coeff1[1] = coeff2[0] = coeff2[1] = 0.;
   }
}

void coeff_for_K4_Newton(y,pelem,bar,eps,bb0,bb1,react,rhs,u,space,par1,
                         coeff0,coeff1,coeff2)
ELEMENT *pelem;
FLOAT *y, bar[DIM2][DIM2], eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(),
      par1, *coeff0, *coeff1, *coeff2;
INT u, space;
{
   DOUBLE bb0y, bb1y, bb, bb2, g2, gu, r, res, rres, s, t1, z, grad_u[DIM];

   sgrad_value(pelem,y,bar,u,space,grad_u);
   res = scalar_conv_diff_res(pelem,y,bar,u,space,eps,bb0,bb1,react,rhs);
   if (REG_FABS == YES)
      rres = regularized_fabs(res,REG_EPS);
   else
      rres = fabs(res);
   bb0y = bb0(y);
   bb1y = bb1(y);
   bb2 = bb0y*bb0y + bb1y*bb1y;
   bb = sqrt(bb2);
   g2 = DOT(grad_u,grad_u);
   gu = sqrt(g2);
   r  = bb*gu + rres;
   if (r < 1.e-10 || bb2 < 1.e-15)
      *coeff0 = coeff1[0] = coeff1[1] = coeff2[0] = coeff2[1] = 0.;
   else{
      t1 = sd_tau_classic(pelem,bar,eps,bb0y,bb1y,space);
      z  = t1*bb2/r;
      *coeff0 = z*rres;
      if ( g2 < 1.e-15 )
         coeff1[0] = coeff1[1] = coeff2[0] = coeff2[1] = 0.;
      else{
         s = (bb0y*grad_u[0]+bb1y*grad_u[1])/bb2;
         coeff1[0] = (grad_u[0] - s*bb0y)/gu;
         coeff1[1] = (grad_u[1] - s*bb1y)/gu;
         if (REG_FABS == YES)
            s = dx_of_regularized_fabs(res,REG_EPS)*g2;
         else
            s = SGN(res)*g2;
         z *= bb/r;
         coeff2[0] = (s*bb0y - rres*grad_u[0])*z;
         coeff2[1] = (s*bb1y - rres*grad_u[1])*z;
      }
   }
}

void coeff_for_K6_Newton(y,pelem,bar,eps,bb0,bb1,react,rhs,u,space,par1,
                         coeff0,coeff1,coeff2)
ELEMENT *pelem;
FLOAT *y, bar[DIM2][DIM2], eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(),
      par1, *coeff0, *coeff1, *coeff2;
INT u, space;
{
   DOUBLE bb0y, bb1y, g2, gu, res, rres, s, w, z, grad_u[DIM];

   sgrad_value(pelem,y,bar,u,space,grad_u);
   if ( (g2=DOT(grad_u,grad_u)) < 1.e-15 )
      *coeff0 = coeff1[0] = coeff1[1] = coeff2[0] = coeff2[1] = 0.;
   else{
      z = 0.5*par1*diameter(pelem);
      res = scalar_conv_diff_res(pelem,y,bar,u,space,eps,bb0,bb1,react,rhs);
      if (REG_FABS == YES)
         rres = regularized_fabs(res,REG_EPS);
      else
         rres = fabs(res);
      gu = sqrt(g2);
      *coeff0 = z*rres/gu;
      bb0y = bb0(y);
      bb1y = bb1(y);
      if ((s=bb0y*bb0y + bb1y*bb1y) < 1.e-15 )
         coeff1[0] = coeff1[1] = coeff2[0] = coeff2[1] = 0.;
      else{
         s = (bb0y*grad_u[0]+bb1y*grad_u[1])/s;
         coeff1[0] = (grad_u[0] - s*bb0y)/gu;
         coeff1[1] = (grad_u[1] - s*bb1y)/gu;
         if (REG_FABS == YES)
            w = dx_of_regularized_fabs(res,REG_EPS);
         else
            w = SGN(res);
         s = rres/g2;
         coeff2[0] = (w*bb0y - s*grad_u[0])*z;
         coeff2[1] = (w*bb1y - s*grad_u[1])*z;
      }
   }
}

void p1c_Newton_matr_for_iso_ij(pelem,i0,i1,i2,Z,bar,detB,
                                eps,bb0,bb1,react,rhs,u,space,sc_type,par1)
ELEMENT *pelem;
FLOAT bar[DIM2][DIM2], detB, eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), 
      par1;
INT i0, i1, i2, Z, u, space, sc_type;
{
   NODE *n0=pelem->n[i0], *n1=pelem->n[i1], *n2=pelem->n[i2];
   FLOAT y[DIM], coeff0, coeff1[DIM], coeff2[DIM], s, an1, an2;

   coord_of_barycentre(pelem,y);
   if (sc_type == SCM_GC2)
      coeff_for_GC2_Newton(y,pelem,bar,eps,bb0,bb1,react,rhs,u,space,par1,
                           &coeff0,coeff1,coeff2);
   else if (sc_type == SCM_K6red_iso)
      coeff_for_K6_Newton(y,pelem,bar,eps,bb0,bb1,react,rhs,u,space,par1,
                           &coeff0,coeff1,coeff2);
   else
      eprintf("Error: coeff_for_ _Newton not available.\n");
   coeff0 *= detB;
   s = DOT(coeff1,bar[i0])*detB;
   COEFFN(n0,Z) += coeff0*DOT(bar[i0],bar[i0]) + s*DOT(coeff2,bar[i0]);
   an1 = coeff0*DOT(bar[i0],bar[i1]) + s*DOT(coeff2,bar[i1]);
   an2 = coeff0*DOT(bar[i0],bar[i2]) + s*DOT(coeff2,bar[i2]);
   putaij(n0->tstart,n1,n2,an1,an2,Z);
}

void p1c_Newton_matr_for_orth_ij(pelem,i0,i1,i2,Z,bar,detB,
                               eps,bb0,bb1,react,rhs,u,space,sc_type,par1)
ELEMENT *pelem;
FLOAT bar[DIM2][DIM2], detB, eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), 
      par1;
INT i0, i1, i2, Z, u, space, sc_type;
{
   NODE *n0=pelem->n[i0], *n1=pelem->n[i1], *n2=pelem->n[i2];
   FLOAT y[DIM], coeff0, coeff1[DIM], coeff2[DIM], bboy[DIM], bb, s, an1, an2;

   coord_of_barycentre(pelem,y);
   bboy[0] = -bb1(y);
   bboy[1] =  bb0(y);
   bb = DOT(bboy,bboy);
   if (bb > 1.e-15){
      if (sc_type == SCM_K4)
         coeff_for_K4_Newton(y,pelem,bar,eps,bb0,bb1,react,rhs,u,space,par1,
                             &coeff0,coeff1,coeff2);
      else if (sc_type == SCM_K6 || sc_type == SCM_K6red)
         coeff_for_K6_Newton(y,pelem,bar,eps,bb0,bb1,react,rhs,u,space,par1,
                             &coeff0,coeff1,coeff2);
      else
         eprintf("Error: coeff_for_ _Newton not available.\n");
      coeff0 *= DOT(bboy,bar[i0])*detB/bb;
      s = DOT(coeff1,bar[i0])*detB;
      COEFFN(n0,Z) += coeff0*DOT(bboy,bar[i0]) + s*DOT(coeff2,bar[i0]);
      an1 = coeff0*DOT(bboy,bar[i1]) + s*DOT(coeff2,bar[i1]);
      an2 = coeff0*DOT(bboy,bar[i2]) + s*DOT(coeff2,bar[i2]);
      putaij(n0->tstart,n1,n2,an1,an2,Z);
   }
}

#else

void c_p1c_ij(n0,n1,n2,Z,bb_0,s,b0,b1,b2,detB)
NODE *n0, *n1, *n2; INT Z; FLOAT bb_0[DIM], s[DIM], b0[DIM2], b1[DIM2], b2[DIM2], detB;
{  eprintf("Error: c_p1c_ij not available.\n");  }

void p1c_up_ij(n0,n1,n2,fa0,nu,Z,flux0,flux1,flux2)
NODE *n0, *n1, *n2; FACE *fa0; INT Z; FLOAT nu, flux0, flux1, flux2;
{  eprintf("Error: p1c_up_ij not available.\n");  }

void r_p1c_ij(n0,n1,n2,Z,a0,a1,a2,detB)
NODE *n0, *n1, *n2; INT Z; FLOAT a0, a1, a2, detB;
{  eprintf("Error: r_p1c_ij not available.\n");  }

void sd_p1c_ij(n0,n1,n2,Z,bb_0,bb_1,bb_2,r0,r1,r2,r,b0,b1,b2,detB)
NODE *n0, *n1, *n2; INT Z; FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], b0[DIM2], b1[DIM2], b2[DIM2], r0, r1, r2, r, detB;
{  eprintf("Error: sd_p1c_ij not available.\n");  }

void sd_p1c_quadr_ij(pelem,i0,i1,i2,Z,bar,bb0,bb1,react,rhs,eps,u,space,sc_type,par1,par2,sd_tau,n,x,w)
ELEMENT *pelem; INT i0, i1, i2, n, Z, u, space, sc_type; FLOAT bar[DIM2][DIM2], (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), eps, par1, par2, (*sd_tau)(), x[][2], *w;
{  eprintf("Error: sd_p1c_quadr_ij not available.\n");  }

void bgu_bgv_p1c_quadr_ij(pelem,i0,i1,i2,Z,bar,bs0,bs1,bb0,bb1,react,rhs,eps,u,space,sc_type,par1,par2,sd_tau,qr)
ELEMENT *pelem; INT i0, i1, i2, Z, u, space, sc_type; FLOAT bar[DIM2][DIM2], (*bs0)(), (*bs1)(), (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), eps, par1, par2, (*sd_tau)(); QUADRATURE_RULE qr;
{  eprintf("Error: bgu_bgv_p1c_quadr_ij not available.\n");  }

void gu_gv_p1c_quadr_ij(pelem,i0,i1,i2,Z,bar,bb0,bb1,react,rhs,eps,u,space,sc_type,par1,par2,sc_eps,qr)
ELEMENT *pelem; INT i0, i1, i2, Z, u, space, sc_type; FLOAT bar[DIM2][DIM2], (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), eps, par1, par2, (*sc_eps)(); QUADRATURE_RULE qr;
{  eprintf("Error: gu_gv_p1c_quadr_ij not available.\n");  }

void c_q1c_ij(n0,n1,n2,n3,Z,bb0,bb1,qr)
NODE *n0, *n1, *n2, *n3; INT Z; FLOAT (*bb0)(), (*bb1)(); QUADRATURE_RULE qr;
{  eprintf("Error: c_q1c_ij not available.\n");  }

void r_q1c_ij(n0,n1,n2,n3,Z,r,qr)
NODE *n0, *n1, *n2, *n3; INT Z; FLOAT (*r)(); QUADRATURE_RULE qr;
{  eprintf("Error: r_q1c_ij not available.\n");  }

void sd_q1c_ij(n0,n1,n2,n3,Z,bb0,bb1,react,delta,qr)
NODE *n0, *n1, *n2, *n3; INT Z; FLOAT (*bb0)(), (*bb1)(), (*react)(), delta; QUADRATURE_RULE qr;
{  eprintf("Error: sd_q1c_ij not available.\n");  }

void macro_conv_stab_q1c_ij(n0,n1,n2,n3,Z,bx,delta,qr)
NODE *n0, *n1, *n2, *n3; INT Z; DOUBLE *bx, delta; QUADRATURE_RULE qr;
{  eprintf("Error: macro_conv_stab_q1c_ij not available.\n");  }

void P1_tangential_derivatives_on_triangle(pelem,u,du,le)
ELEMENT *pelem; DOUBLE *du, *le; INT u;
{  eprintf("Error: P1_tangential_derivatives_on_triangle not available.\n");  }

void p1c_Newton_matr_for_iso_ij(pelem,i0,i1,i2,Z,bar,detB,eps,bb0,bb1,react,rhs,u,space,sc_type,par1)
ELEMENT *pelem; FLOAT bar[DIM2][DIM2], detB, eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), par1; INT i0, i1, i2, Z, u, space, sc_type;
{  eprintf("Error: p1c_Newton_matr_for_iso_ij not available.\n");  }

void p1c_Newton_matr_for_orth_ij(pelem,i0,i1,i2,Z,bar,detB,eps,bb0,bb1,react,rhs,u,space,sc_type,par1)
ELEMENT *pelem; FLOAT bar[DIM2][DIM2], detB, eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), par1; INT i0, i1, i2, Z, u, space, sc_type;
{  eprintf("Error: p1c_Newton_matr_for_orth_ij not available.\n");  }

#endif

#if (N_DATA & MVECTOR_NODE_DATA) && (ELEMENT_TYPE == CUBE)

void GQ1_tangential_derivatives_on_square(pelem,u,du,le)
ELEMENT *pelem;
DOUBLE *du, *le;
INT u;
{
   NODE *n0, *n1, *n2, *n3;
   FLOAT *x0, *x1, *x2, *x3, t[DIM];

   NODES_OF_4ELEMENT(n0,n1,n2,n3,pelem);
   VERTICES_OF_4ELEMENT(x0,x1,x2,x3,pelem);
   SUBTR(x1,x0,t);
   le[0] = sqrt(DOT(t,t));
   du[0] = (NDMV(n1,u,0)-NDMV(n0,u,0))/le[0];
   SUBTR(x2,x1,t);
   le[1] = sqrt(DOT(t,t));
   du[1] = (NDMV(n2,u,0)-NDMV(n1,u,0))/le[1];
   SUBTR(x3,x2,t);
   le[2] = sqrt(DOT(t,t));
   du[2] = (NDMV(n3,u,0)-NDMV(n2,u,0))/le[2];
   SUBTR(x0,x3,t);
   le[3] = sqrt(DOT(t,t));
   du[3] = (NDMV(n0,u,0)-NDMV(n3,u,0))/le[3];
}

#else

void GQ1_tangential_derivatives_on_square(pelem,u,du,le)
ELEMENT *pelem; DOUBLE *du, *le; INT u;
{  eprintf("Error: GQ1_tangential_derivatives_on_square not available.\n");  }

#endif

void sc_phi_on_edges(tGrid,pelem,eps,u,bar,rdetB,bb0,bb1,react,rhs,
                     space,sc_type,beta1,beta2,phi)
GRID *tGrid;
ELEMENT *pelem;
FLOAT eps, bar[DIM2][DIM2], rdetB, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), 
      beta1, beta2, *phi;
INT u, space, sc_type;
{
   NODE *n0, *n1, *n2;
   FLOAT *x0, *x1, *x2, x01[DIM], x02[DIM], x12[DIM], x[DIM],
         bb_0[DIM], bb_1[DIM], bb_2[DIM], bb, a, max, r0, r1, r2, r3;
   REF_MAPPING ref_map;

   if (space == P1C){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      MIDPOINTS(x0,x1,x2,x01,x02,x12)
      if (sc_type == SCM_BH){
         V_MID_VALUES(x01,x02,x12,bb0,bb1,bb_0,bb_1,bb_2)
         a = (bb_0[0] + bb_1[0] + bb_2[0])/3.;
         bb = a*a;
         a = (bb_0[1] + bb_1[1] + bb_2[1])/3.;
         bb = sqrt(bb + a*a);
         max = fabs(p1c_jump_of_normal_derivative(n0,n1,n2,pelem->f[2],u));
         a = fabs(p1c_jump_of_normal_derivative(n0,n2,n1,pelem->f[1],u));
         max = MAX(a,max);
         a = fabs(p1c_jump_of_normal_derivative(n1,n2,n0,pelem->f[0],u));
         max = MAX(a,max);
         a = diameter(pelem);
         phi[0] = phi[1] = phi[2] = a*(beta2*eps + beta1*bb*a)*max;
      }
      else if (sc_type == SCM_BE2){
         V_MID_VALUES(x01,x02,x12,bb0,bb1,bb_0,bb_1,bb_2)
         a = diameter(pelem);
         a *= beta1*a;
         phi[0] = a*sqrt(DOT(bb_0,bb_0))*
                    fabs(p1c_jump_of_normal_derivative(n1,n2,n0,pelem->f[0],u));
         phi[1] = a*sqrt(DOT(bb_1,bb_1))*
                    fabs(p1c_jump_of_normal_derivative(n0,n2,n1,pelem->f[1],u));
         phi[2] = a*sqrt(DOT(bb_2,bb_2))*
                    fabs(p1c_jump_of_normal_derivative(n0,n1,n2,pelem->f[2],u));
      }
      else if (sc_type == SCM_BE3){
         r0 = scalar_conv_diff_res(pelem,x0,bar,u,space,eps,bb0,bb1,react,rhs);
         r1 = scalar_conv_diff_res(pelem,x1,bar,u,space,eps,bb0,bb1,react,rhs);
         r2 = scalar_conv_diff_res(pelem,x2,bar,u,space,eps,bb0,bb1,react,rhs);
         beta1 *= rdetB;
         phi[0] = beta1*mean_abs_value_1d(r1,r2);
         phi[1] = beta1*mean_abs_value_1d(r2,r0);
         phi[2] = beta1*mean_abs_value_1d(r0,r1);
/*
         phi[0] = beta1*rdetB*fabs(scalar_conv_diff_res(pelem,x12,
                                            bar,u,space,eps,bb0,bb1,react,rhs));
         phi[1] = beta1*rdetB*fabs(scalar_conv_diff_res(pelem,x02,
                                            bar,u,space,eps,bb0,bb1,react,rhs));
         phi[2] = beta1*rdetB*fabs(scalar_conv_diff_res(pelem,x01,
                                            bar,u,space,eps,bb0,bb1,react,rhs));
*/
      }
      else
         eprintf("Error: wrong sc_type in sc_phi_on_edges for P1C.\n");
   }
   else if (space == GQ1C){
      if (sc_type == SCM_BE3){
         reference_mapping_with_inverse(pelem,REF_MAP,&ref_map);
         x[0] = x[1] = -1.;
         r0 = gscalar_conv_diff_res(tGrid,pelem,x,u,eps,bb0,bb1,react,rhs,
                                                                  ELEM,ref_map);
         x[0] = 1.;
         r1 = gscalar_conv_diff_res(tGrid,pelem,x,u,eps,bb0,bb1,react,rhs,
                                                                  ELEM,ref_map);
         x[1] = 1.;
         r2 = gscalar_conv_diff_res(tGrid,pelem,x,u,eps,bb0,bb1,react,rhs,
                                                                  ELEM,ref_map);
         x[0] = -1.;
         r3 = gscalar_conv_diff_res(tGrid,pelem,x,u,eps,bb0,bb1,react,rhs,
                                                                  ELEM,ref_map);
         beta1 *= rdetB;
         phi[0] = beta1*mean_abs_value_1d(r0,r1);
         phi[1] = beta1*mean_abs_value_1d(r1,r2);
         phi[2] = beta1*mean_abs_value_1d(r2,r3);
         phi[3] = beta1*mean_abs_value_1d(r3,r0);
      }
      else
         eprintf("Error: wrong sc_type in sc_phi_on_edges for Q1C.\n");
   }
   else
      eprintf("Error: wrong space in sc_phi_on_edges.\n");
}

void edge_stabilization(tGrid,pelem,eps,Z,u,bb0,bb1,react,rhs,space,sc_type,
                        beta1,beta2)
GRID *tGrid;
ELEMENT *pelem;
FLOAT eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), beta1, beta2;
INT Z, u, space, sc_type;
{
   NODE *n0, *n1, *n2, *n3;
   FLOAT *x0, *x1, *x2, b[DIM2][DIM2], p, rdetB, du[SIDES], le[SIDES], 
         phi[SIDES];

   if (space == P1C){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      rdetB = barycentric_coordinates(x0,x1,x2,b);
      sc_phi_on_edges(tGrid,pelem,eps,u,b,rdetB,bb0,bb1,react,rhs,space,sc_type,
                      beta1,beta2,phi);
      P1_tangential_derivatives_on_triangle(pelem,u,du,le);
      phi[0] *= regularized_sign_divided_by_x(du[0])/le[0];
      phi[1] *= regularized_sign_divided_by_x(du[1])/le[1];
      phi[2] *= regularized_sign_divided_by_x(du[2])/le[2];
      putai3(n0,n1,n2, phi[1]+phi[2], phi[0]+phi[2], phi[0]+phi[1],Z);
      putaij(n0->tstart,n1,n2,-phi[2],-phi[1],Z);
      putaij(n1->tstart,n2,n0,-phi[0],-phi[2],Z);
      putaij(n2->tstart,n0,n1,-phi[1],-phi[0],Z);
   }
   else if (space == GQ1C){
      NODES_OF_4ELEMENT(n0,n1,n2,n3,pelem);
      rdetB = VOLUME(pelem);
      sc_phi_on_edges(tGrid,pelem,eps,u,b,rdetB,bb0,bb1,react,rhs,space,sc_type,
                      beta1,beta2,phi);
      GQ1_tangential_derivatives_on_square(pelem,u,du,le);
      phi[0] *= regularized_sign_divided_by_x(du[0])/le[0];
      phi[1] *= regularized_sign_divided_by_x(du[1])/le[1];
      phi[2] *= regularized_sign_divided_by_x(du[2])/le[2];
      phi[3] *= regularized_sign_divided_by_x(du[3])/le[3];
      putai4g(n0,n1,n2,n3, phi[0]+phi[3], phi[1]+phi[0],
                           phi[2]+phi[1], phi[3]+phi[2],Z);
      putaijg(n0->tstart,n1,n3,-phi[0],-phi[3],Z);
      putaijg(n1->tstart,n2,n0,-phi[1],-phi[0],Z);
      putaijg(n2->tstart,n3,n1,-phi[2],-phi[1],Z);
      putaijg(n3->tstart,n0,n2,-phi[3],-phi[2],Z);
   }
   else
      eprintf("Error: edge_stabilization not available.\n");
}

#if N_DATA & SCALAR_NODE_DATA

void edge_stabilization_to_rhs(tGrid,pelem,eps,f,u,bb0,bb1,react,rhs,
                               space,sc_type,beta1,beta2)
GRID *tGrid;
ELEMENT *pelem;
FLOAT eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), beta1, beta2;
INT f, u, space, sc_type;
{
   NODE *n0, *n1, *n2;
   FLOAT *x0, *x1, *x2, grad_u[DIM], t[DIM], b[DIM2][DIM2], phi[SIDES], p,
         rdetB;

   if (space != P1C)
      eprintf("Error: edge_stabilization available for P1C only.\n");
   else{
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      rdetB = barycentric_coordinates(x0,x1,x2,b); 
      sc_phi_on_edges(tGrid,pelem,eps,u,b,rdetB,bb0,bb1,react,rhs,space,sc_type,
                      beta1,beta2,phi);
      sgrad_value(pelem,x0,b,u,space,grad_u);
      SUBTR(x1,x2,t);
      UNIT_VECTOR(t,t,p)
      phi[0] *= tanh(DOT(t,grad_u));
      SUBTR(x0,x2,t);
      UNIT_VECTOR(t,t,p)
      phi[1] *= tanh(DOT(t,grad_u));
      SUBTR(x0,x1,t);
      UNIT_VECTOR(t,t,p)
      phi[2] *= tanh(DOT(t,grad_u));
      NDS(n0,f) -=  phi[1] + phi[2];
      NDS(n1,f) -=  phi[0] - phi[2];
      NDS(n2,f) -= -phi[0] - phi[1];
   }
}

#else

void edge_stabilization_to_rhs(tGrid,pelem,eps,f,u,bb0,bb1,react,rhs,space,sc_type,beta1,beta2)
GRID *tGrid; ELEMENT *pelem; FLOAT eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), beta1, beta2; INT f, u, space, sc_type;
{  eprintf("Error: edge_stabilization_to_rhs not available.\n");  }

#endif

void add_P1C_conv_term_matr(tGrid,Z,bb0,bb1)
GRID *tGrid;
INT Z;
FLOAT (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], s[DIM], ndetB, b[DIM2][DIM2];
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      ndetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b)/12.;
      V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
      SET14(s,bb_0,bb_1,bb_2)
      c_p1c_ij(n0,n1,n2,Z,bb_0,s,b[0],b[1],b[2],ndetB);
      c_p1c_ij(n1,n2,n0,Z,bb_1,s,b[1],b[2],b[0],ndetB);
      c_p1c_ij(n2,n0,n1,Z,bb_2,s,b[2],b[0],b[1],ndetB);
   }
}

void add_P1C_react_term_matr(tGrid,Z,r)
GRID *tGrid;
INT Z;
FLOAT (*r)();
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT r0, r1, r2, s, ndetB;
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      ndetB = VOLUME(pelem)/60.;
      S_NODE_VALUES(n0,n1,n2,r,r0,r1,r2)
      s = 2.*(r0 + r1 + r2);
      r_p1c_ij(n0,n1,n2,Z,s+4.*r0,s-r2,s-r1,ndetB);
      r_p1c_ij(n1,n2,n0,Z,s+4.*r1,s-r0,s-r2,ndetB);
      r_p1c_ij(n2,n0,n1,Z,s+4.*r2,s-r1,s-r0,ndetB);
   }
}

void add_p1c_sd_term_matr(tGrid,Z,nu,bb0,bb1,react)
GRID *tGrid;             /*  streamline-diffusion term; exact for pw. lin. b, */
INT Z;                   /*  and pw. linear reaction term                     */
FLOAT nu, (*bb0)(), (*bb1)(), (*react)();
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], r0, r1, r2, r, delta, ndetB, 
                                                                  b[DIM2][DIM2];
   INT n=0;
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      delta = sd_delta(pelem,nu,bb0,bb1);
      if (delta > 0.){
         n++;
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
         S_NODE_VALUES(n0,n1,n2,react,r0,r1,r2)
         ndetB = delta*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                               n2->myvertex->x,b)/60.;
         r = r0 + r1 + r2;
         sd_p1c_ij(n0,n1,n2,Z,bb_0,bb_1,bb_2,r0,r1,r2,r,b[0],b[1],b[2],ndetB);
         sd_p1c_ij(n1,n2,n0,Z,bb_1,bb_2,bb_0,r1,r2,r0,r,b[1],b[2],b[0],ndetB);
         sd_p1c_ij(n2,n0,n1,Z,bb_2,bb_0,bb_1,r2,r0,r1,r,b[2],b[0],b[1],ndetB);
      }
   }
// printf("SDFEM on %i elements.\n",n);
}

void add_p1c_sd_p1_nc_term_matr(tGrid,Z,nu,bb0,bb1,react)
GRID *tGrid;             /*  streamline-diffusion term; exact for pw. lin. b, */
INT Z;                   /*  and pw. linear reaction term                     */
FLOAT nu, (*bb0)(), (*bb1)(), (*react)();
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], r0, r1, r2, r, delta[DIM2], ndetB, 
                                                                  b[DIM2][DIM2];
   INT n=0;
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      delta[0] = sd_p1_nc_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,0);
      delta[1] = sd_p1_nc_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,1);
      delta[2] = sd_p1_nc_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,2);
      if ((delta[0] > 0.)||(delta[1] > 0.)||(delta[2] > 0.)){
         n++;
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
         S_NODE_VALUES(n0,n1,n2,react,r0,r1,r2)
         ndetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                               n2->myvertex->x,b);
         sdp1nc_p1c_ij(n0,n1,n2,Z,bb_0,bb_1,bb_2,r0,r1,r2,b[0],b[1],b[2],ndetB,delta[0],delta[1],delta[2]);
         sdp1nc_p1c_ij(n1,n2,n0,Z,bb_1,bb_2,bb_0,r1,r2,r0,b[1],b[2],b[0],ndetB,delta[1],delta[2],delta[0]);
         sdp1nc_p1c_ij(n2,n0,n1,Z,bb_2,bb_0,bb_1,r2,r0,r1,b[2],b[0],b[1],ndetB,delta[2],delta[0],delta[1]);
      }
   }
// printf("SDFEM on %i elements.\n",n);
}

void add_p1c_sd_p1c_term_matr(tGrid,Z,nu,bb0,bb1,react)
GRID *tGrid;             /*  streamline-diffusion term; exact for pw. lin. b, */
INT Z;                   /*  and pw. linear reaction term                     */
FLOAT nu, (*bb0)(), (*bb1)(), (*react)();
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], r0, r1, r2, r, delta[DIM2], ndetB, 
                                                                  b[DIM2][DIM2];
   INT n=0;
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      delta[0] = sd_p1c_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,n0);
      delta[1] = sd_p1c_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,n1);
      delta[2] = sd_p1c_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,n2);
      if ((delta[0] > 0.)||(delta[1] > 0.)||(delta[2] > 0.)){
         n++;
         
         V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
         S_NODE_VALUES(n0,n1,n2,react,r0,r1,r2)
         ndetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                               n2->myvertex->x,b);
         sdp1nc_p1c_ij(n0,n1,n2,Z,bb_0,bb_1,bb_2,r0,r1,r2,b[0],b[1],b[2],ndetB,delta[0],delta[1],delta[2]);
         sdp1nc_p1c_ij(n1,n2,n0,Z,bb_1,bb_2,bb_0,r1,r2,r0,b[1],b[2],b[0],ndetB,delta[1],delta[2],delta[0]);
         sdp1nc_p1c_ij(n2,n0,n1,Z,bb_2,bb_0,bb_1,r2,r0,r1,b[2],b[0],b[1],ndetB,delta[2],delta[0],delta[1]);
      }
   }
// printf("SDFEM on %i elements.\n",n);
}






void p1c_Newton_matr_for_sc(tGrid,eps,Z,u,bb0,bb1,react,rhs,space,sc_type,par1)
GRID *tGrid;
FLOAT eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), par1;
INT Z, u, space, sc_type;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT detB, bar[DIM2][DIM2];
  
   if (sc_type == SCM_K4 || sc_type == SCM_K6 || sc_type == SCM_K6red) 
      for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         detB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                        n2->myvertex->x,bar);
         p1c_Newton_matr_for_orth_ij(pelem,0,1,2,Z,bar,detB,
                                    eps,bb0,bb1,react,rhs,u,space,sc_type,par1);
         p1c_Newton_matr_for_orth_ij(pelem,1,2,0,Z,bar,detB,
                                    eps,bb0,bb1,react,rhs,u,space,sc_type,par1);
         p1c_Newton_matr_for_orth_ij(pelem,2,0,1,Z,bar,detB,
                                    eps,bb0,bb1,react,rhs,u,space,sc_type,par1);
      }
   else if (sc_type == SCM_GC2 || sc_type == SCM_K6red_iso)
      for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         detB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                        n2->myvertex->x,bar);
         p1c_Newton_matr_for_iso_ij(pelem,0,1,2,Z,bar,detB,
                                    eps,bb0,bb1,react,rhs,u,space,sc_type,par1);
         p1c_Newton_matr_for_iso_ij(pelem,1,2,0,Z,bar,detB,
                                    eps,bb0,bb1,react,rhs,u,space,sc_type,par1);
         p1c_Newton_matr_for_iso_ij(pelem,2,0,1,Z,bar,detB,
                                    eps,bb0,bb1,react,rhs,u,space,sc_type,par1);
      }
   else
      eprintf("Error: p1c_Newton_matr not available.\n");
}

void add_Q1C_conv_term_matr(tGrid,Z,bb0,bb1,qr)
GRID *tGrid;
INT Z;
FLOAT (*bb0)(), (*bb1)();
QUADRATURE_RULE qr;
{
   NODE *n0, *n1, *n2, *n3;
   ELEMENT *pelem;
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_4ELEMENT(n0,n1,n2,n3,pelem);
      c_q1c_ij(n0,n1,n2,n3,Z,bb0,bb1,qr);
      c_q1c_ij(n3,n0,n1,n2,Z,bb0,bb1,qr);
      c_q1c_ij(n2,n3,n0,n1,Z,bb0,bb1,qr);
      c_q1c_ij(n1,n2,n3,n0,Z,bb0,bb1,qr);
   }
}

void add_Q1C_react_term_matr(tGrid,Z,r,qr)
GRID *tGrid;
INT Z;
FLOAT (*r)();
QUADRATURE_RULE qr;
{
   NODE *n0, *n1, *n2, *n3;
   ELEMENT *pelem;
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_4ELEMENT(n0,n1,n2,n3,pelem);
      r_q1c_ij(n0,n1,n2,n3,Z,r,qr);
      r_q1c_ij(n3,n0,n1,n2,Z,r,qr);
      r_q1c_ij(n2,n3,n0,n1,Z,r,qr);
      r_q1c_ij(n1,n2,n3,n0,Z,r,qr);
   }
}

void add_Q1C_sd_term_matr(tGrid,Z,nu,bb0,bb1,react,qr)
GRID *tGrid;
INT Z;
FLOAT nu, (*bb0)(), (*bb1)(), (*react)();
QUADRATURE_RULE qr;
{
   NODE *n0, *n1, *n2, *n3;
   ELEMENT *pelem;
   FLOAT delta;
   INT n=0;
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      delta = sd_delta(pelem,nu,bb0,bb1);
      if (delta > 0.){
         n++;
         NODES_OF_4ELEMENT(n0,n1,n2,n3,pelem);
         sd_q1c_ij(n0,n1,n2,n3,Z,bb0,bb1,react,delta,qr);
         sd_q1c_ij(n3,n0,n1,n2,Z,bb0,bb1,react,delta,qr);
         sd_q1c_ij(n2,n3,n0,n1,Z,bb0,bb1,react,delta,qr);
         sd_q1c_ij(n1,n2,n3,n0,Z,bb0,bb1,react,delta,qr);
      }
   }
   printf("SDFEM on %i elements.\n",n);
}

#if DATA_S & N_LINK_TO_ELEMENTS

void add_Q1C_macro_conv_stab_term_matr(tGrid,Z,eps,bb0,bb1,qr)
GRID *tGrid;
INT Z;
FLOAT eps, (*bb0)(), (*bb1)();
QUADRATURE_RULE qr;
{
   NELINK *pnel;
   NODE *n0, *n1, *n2, *n3, *pn;
   DOUBLE bx[DIM], delta;
   INT k;
  
   for (pn = FIRSTNODE(tGrid); pn; pn = pn->succ)
   if (is_macro_node(pn))
   {
      bx[0] = bb0(pn->myvertex->x);
      bx[1] = bb1(pn->myvertex->x);
      for (pnel = NESTART(pn); pnel; pnel = pnel->next)
         if ((delta=lpm_delta(pn,eps,bb0,bb1)) > 0.){
            NODES_OF_4ELEMENT(n0,n1,n2,n3,pnel->nbel);
            macro_conv_stab_q1c_ij(n0,n1,n2,n3,Z,bx,delta,qr);
            macro_conv_stab_q1c_ij(n3,n0,n1,n2,Z,bx,delta,qr);
            macro_conv_stab_q1c_ij(n2,n3,n0,n1,Z,bx,delta,qr);
            macro_conv_stab_q1c_ij(n1,n2,n3,n0,Z,bx,delta,qr);
         }
   }
}

#else

void add_Q1C_macro_conv_stab_term_matr(tGrid,Z,eps,bb0,bb1,qr)
GRID *tGrid; INT Z; FLOAT eps, (*bb0)(), (*bb1)(); QUADRATURE_RULE qr;
{  eprintf("Error: add_Q1C_macro_conv_stab_term_matr not available.\n");  }

#endif

void mh_constants_q1(z1,z2,v,bb_xc,w,b1,b34,c1,c2)
FLOAT *z1, *z2, *v, *bb_xc, *w, b1, b34, *c1, *c2;
{
   DOUBLE s[DIM], z1o[DIM], c=1., d, r=1.;

   UNIT_VECTOR(s,bb_xc,d)
   ORT_VECT(z1o,z1)
   if (DOT(bb_xc,z1) > 0. && (d=fabs(DOT(s,z1o)/DOT(v,z1o))) < 1.)
      r = d;
   if ((r*=DOT(v,z1)) > (d=2.*fabs(DOT(w,z1o))))
      c = d/r;
   *c1 = -b1 + 0.5*c*(1. + ((DOT(s,z1)-DOT(s,z2))/(1.-DOT(z1,z2))));
   *c2 = b34 - *c1;
}

void treat_q1_zone(bb_xc,grad_u,z1,z2,s1,s2,a1,a2,b1,b2,b3,b4,c1,c2,c3,c4)
FLOAT *bb_xc, *grad_u, *z1, *z2, *s1, *s2, a1, a2, b1, b2, b3, b4, 
      *c1, *c2, *c3, *c4;
{
   FLOAT v[DIM], w[DIM], q;

   *c3 = -b3;
   *c4 = -b4;
   if (fabs(DOT(bb_xc,grad_u)) < EPSC*MAX(fabs(bb_xc[0]),fabs(bb_xc[1]))){
      *c1 = 0.5 - b1;
      *c2 = 0.5 - b2;
   }
   else{
      ORT_VECT(w,grad_u)
      q = DOT(w,s1)*DOT(w,s2);
      if (q >= 0. && fabs(DOT(w,s1)*a2) < fabs(DOT(w,s2)*a1)){
         *c1 = 1. - b1;
         *c2 = -b2;
      }
      else if (q >= 0.){
         *c1 = -b1;
         *c2 = 1. - b2;
      }
      else{
         SET10(v,z1,z2)
         UNIT_VECTOR(v,v,q)
         UNIT_VECTOR(w,w,q)
         if (DOT(v,w) < 0.)
            SET8(w,w)
         if (DOT(w,s1) > 0.)
            mh_constants_q1(z1,z2,v,bb_xc,w,b1,b3+b4,c1,c2);
         else
            mh_constants_q1(z2,z1,v,bb_xc,w,b2,b3+b4,c2,c1);
      }
   }
}

void compute_q1_upwind_const(bb_xc,grad_u,z1,z2,z3,z4,s1,s2,s3,s4,b1,b2,b3,b4,
                                                                  c1,c2,c3,c4)
FLOAT *bb_xc, *grad_u, *z1, *z2, *z3, *z4, *s1, *s2, *s3, *s4, b1, b2, b3, b4, 
      *c1, *c2, *c3, *c4;
{
   FLOAT a1, a2, a3, a4;

   if (fabs(bb_xc[0]) < EPSC && fabs(bb_xc[1]) < EPSC)
      *c1 = *c2 = *c3 = *c4 = 0.;
   else{
      a1 = DOT(s1,bb_xc);
      a2 = DOT(s2,bb_xc);
      a3 = DOT(s3,bb_xc);
      a4 = DOT(s4,bb_xc);
      if (a1 > 0. && a2 >= 0.)
         treat_q1_zone(bb_xc,grad_u,z1,z2,s1,s2,a1,a2,b1,b2,b3,b4,c1,c2,c3,c4);
      else if (a2 > 0. && a3 >= 0.)
         treat_q1_zone(bb_xc,grad_u,z2,z3,s2,s3,a2,a3,b2,b3,b4,b1,c2,c3,c4,c1);
      else if (a3 > 0. && a4 >= 0.)
         treat_q1_zone(bb_xc,grad_u,z3,z4,s3,s4,a3,a4,b3,b4,b1,b2,c3,c4,c1,c2);
      else if (a4 > 0. && a1 >= 0.)
         treat_q1_zone(bb_xc,grad_u,z4,z1,s4,s1,a4,a1,b4,b1,b2,b3,c4,c1,c2,c3);
      else
         eprintf("Error in compute_q1_upwind_const.\n");
   }
}

#if (N_DATA & ONE_NODE_MATR) && (N_DATA & SCALAR_NODE_DATA) && (ELEMENT_TYPE == CUBE)

void add_Q1C_MH_conv_term_matr(tGrid,Z,u,f,bb0,bb1,react,rhs)
GRID *tGrid;
INT Z, u, f;
FLOAT (*bb0)(), (*bb1)(), (*react)(), (*rhs)();
{
   ELEMENT *pelem;
   NODE *n1, *n2, *n3, *n4;
   FLOAT bb_xc[DIM], xc[DIM], s1[DIM], s2[DIM], s3[DIM], s4[DIM], 
         z1[DIM], z2[DIM], z3[DIM], z4[DIM], grad_u[DIM],
         a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, q, *x1, *x2, *x3, *x4;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      b1 = b2 = b3 = b4 = 0.25;
      NODES_OF_4ELEMENT(n1,n2,n3,n4,pelem);
      VERTICES_OF_4ELEMENT(x1,x2,x3,x4,pelem);
      coord_of_barycentre(pelem,xc);
      bb_xc[0] = bb0(xc);
      bb_xc[1] = bb1(xc);
      SUBTR(x1,x3,z1); 
      UNIT_VECTOR(z1,z1,q)
      q *= 0.5;
      s2[0] = -q*z1[1];
      s2[1] =  q*z1[0];
      SUBTR(x2,x4,z2); 
      UNIT_VECTOR(z2,z2,q)
      q *= 0.5;
      s1[0] = -q*z2[1];
      s1[1] =  q*z2[0];
      if (DOT(s1,z1) < 0.)
         SET8(s1,s1)
      if (DOT(s2,z2) < 0.)
         SET8(s2,s2)
      SET8(z3,z1)
      SET8(z4,z2)
      SET8(s3,s1)
      SET8(s4,s2)
      q = VOLUME(pelem);
      grad_u[0] = (NDS(n1,u)*s1[0] + NDS(n2,u)*s2[0] + 
                   NDS(n3,u)*s3[0] + NDS(n4,u)*s4[0])/q;
      grad_u[1] = (NDS(n1,u)*s1[1] + NDS(n2,u)*s2[1] + 
                   NDS(n3,u)*s3[1] + NDS(n4,u)*s4[1])/q;
      if ((NOT_FN(n1) || NOT_FN(n2) || NOT_FN(n3) || NOT_FN(n4)) && 
          (xc[0] > 0.9 || xc[1] < 0.1))
         c1 = c2 = c3 = c4 = -0.25;
      else
         compute_q1_upwind_const(bb_xc,grad_u,z1,z2,z3,z4,s1,s2,s3,s4,
                                                   b1,b2,b3,b4,&c1,&c2,&c3,&c4);
      b1 += c1;
      b2 += c2;
      b3 += c3;
      b4 += c4;
      a1 = DOT(bb_xc,s1);
      a2 = DOT(bb_xc,s2);
      a3 = DOT(bb_xc,s3);
      a4 = DOT(bb_xc,s4);
      COEFFN(n1,Z) += b1*a1;
      COEFFN(n2,Z) += b2*a2;
      COEFFN(n3,Z) += b3*a3;
      COEFFN(n4,Z) += b4*a4;
      putaij3(n1->tstart,n2,n3,n4,b1*a2,b1*a3,b1*a4,Z);
      putaij3(n2->tstart,n3,n4,n1,b2*a3,b2*a4,b2*a1,Z);
      putaij3(n3->tstart,n4,n1,n2,b3*a4,b3*a1,b3*a2,Z);
      putaij3(n4->tstart,n1,n2,n3,b4*a1,b4*a2,b4*a3,Z);
   }
}

#else

void add_Q1C_MH_conv_term_matr(tGrid,Z,u,f,bb0,bb1,react,rhs)
GRID *tGrid; INT Z, u, f; FLOAT (*bb0)(), (*bb1)(), (*react)(), (*rhs)();
{  eprintf("Error: add_Q1C_MH_conv_term_matr not available.\n");  }

#endif

INT exists_alpha();
FLOAT constant_c3();

void treat_edge_zone_for_half_quadrilateral(grad_u,bb_xc,n1,n2,n3,s1,s2,s3,
                                            p1,p2,p3,c1i,c2i,c3i,c4i,c1,c2,c3)
NODE *n1, *n2, *n3;                                  /* edge zone opposite n1 */
FLOAT *grad_u, *bb_xc, s1, s2, s3, p1, p2, p3, c1i, c2i, c3i, c4i, *c1, *c2, *c3;
{
   FLOAT v2[DIM], v3[DIM], vm[DIM], vmo[DIM], w[DIM], d, min2, min3, max2, max3,
         q=c1i+c4i;
   INT i2=0, i3=0;

   *c1 = -c1i;
   if (fabs(DOT(bb_xc,grad_u)) < EPSC*MAX(fabs(bb_xc[0]),fabs(bb_xc[1])))
      *c2 = *c3 = 0.5*q;
   else{
      i2 = exists_alpha(p2,p1,p3,s2,s1,s3,&min2,&max2);
      i3 = exists_alpha(p3,p1,p2,s3,s1,s2,&min3,&max3);
      if (i2 && !i3){
         *c2 = q + c3i;
         *c3 = -c3i;
      }
      else if (!i2 && i3){
         *c2 = -c2i;
         *c3 = q + c2i;
      }
      else{
         SET11(v2,n2->myvertex->x,n1->myvertex->x)
         SET11(v3,n3->myvertex->x,n1->myvertex->x)
         UNIT_VECTOR(v2,v2,d)
         UNIT_VECTOR(v3,v3,d)
         SET10(vm,v2,v3)
         UNIT_VECTOR(vm,vm,d)
         ORT_VECT_DIR(vmo,vm,v3)
         ORT_VECT_DIR(w,grad_u,vm)
         UNIT_VECTOR(w,w,d)
         if (DOT(w,vmo) < 0.){
            *c2 = -c2i + 6.*constant_c3(v3,v2,vm,bb_xc,w);
            *c3 = q - *c2;
         }
         else{
            *c3 = -c3i + 6.*constant_c3(v2,v3,vm,bb_xc,w);
            *c2 = q - *c3; 
         }
      }
   }
}

void compute_upwind_const_for_half_quadrilateral(grad_u,bb_xc,b,
                                     n0,n1,n2,s0,s1,s2,c0i,c1i,c2i,c3i,c0,c1,c2)
NODE *n0, *n1, *n2;
FLOAT *grad_u, *bb_xc, b[DIM2][DIM2], s0, s1, s2, c0i, c1i, c2i, c3i, 
      *c0, *c1, *c2;
{
   FLOAT w[DIM], p0, p1, p2, d, sum=12.;

   if (s0 > EPSC && s1 < EPSC && s2 < EPSC){
      *c0 = sum - c0i;
      *c1 = -c1i;
      *c2 = -c2i;
   }
   else if (s1 > EPSC && s2 < EPSC && s0 < EPSC){
      *c0 = -c0i;
      *c1 = sum - c1i;
      *c2 = -c2i;
   }
   else if (s2 > EPSC && s0 < EPSC && s1 < EPSC){
      *c0 = -c0i;
      *c1 = -c1i;
      *c2 = sum - c2i;
   }
   else if (NOT_FN(n0) || NOT_FN(n1) || NOT_FN(n2)){
      *c0 = -c0i;
      *c1 = -c1i;
      *c2 = -c2i;
   }
   else{
      ORT_VECT(w,grad_u)
      if (fabs(w[0]) > EPSC || fabs(w[1]) > EPSC)
         UNIT_VECTOR(w,w,d)
      p0 = DOT(b[0],w);
      p1 = DOT(b[1],w);
      p2 = DOT(b[2],w);
      if (s0 < -EPSC)
         treat_edge_zone_for_half_quadrilateral(grad_u,bb_xc,
                           n0,n1,n2,s0,s1,s2,p0,p1,p2,c0i,c1i,c2i,c3i,c0,c1,c2);
      else if (s1 < -EPSC)
         treat_edge_zone_for_half_quadrilateral(grad_u,bb_xc,
                           n1,n2,n0,s1,s2,s0,p1,p2,p0,c1i,c2i,c0i,c3i,c1,c2,c0);
      else if (s2 < -EPSC)
         treat_edge_zone_for_half_quadrilateral(grad_u,bb_xc,
                           n2,n0,n1,s2,s0,s1,p2,p0,p1,c2i,c0i,c1i,c3i,c2,c0,c1);
      else
         eprintf("Error in compute_upwind_const_for_half_quadrilateral.\n");
   }
}

#if (N_DATA & SCALAR_NODE_DATA) && (N_DATA & ONE_NODE_MATR)

void add_Q1C_MH_for_half_quadrilateral(n0,n1,n2,n3,Z,u,f,bb_xc,rhs)
NODE *n0, *n1, *n2, *n3;  /* integration over (n0,n1,n2) */
INT Z, u, f;              /* (n0,n2) is the common edge  */
FLOAT *bb_xc, (*rhs)();
{
   FLOAT b[DIM2][DIM2], grad_u[DIM], x012[DIM], c0i=3., c1i=5., c2i=3., c3i=1.,
         c0, c1, c2, s0, s1, s2, s, ndetB;

   ndetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                   n2->myvertex->x,b)/12.;
   s0 = DOT(b[0],bb_xc);
   s1 = DOT(b[1],bb_xc);
   s2 = DOT(b[2],bb_xc);
   if (fabs(bb_xc[0]) < EPSC && fabs(bb_xc[1]) < EPSC){
      c0 = c1 = c2 = 0.;
      s = c3i*ndetB;
      putaij3(n3->tstart,n0,n1,n2,s*s0,s*s1,s*s2,Z);
   }
   else{
      SET27(grad_u,b[0],NDS(n0,u),b[1],NDS(n1,u),b[2],NDS(n2,u))
      compute_upwind_const_for_half_quadrilateral(grad_u,bb_xc,b,
                                 n0,n1,n2,s0,s1,s2,c0i,c1i,c2i,c3i,&c0,&c1,&c2);
      POINT3(n0->myvertex->x,n1->myvertex->x,n2->myvertex->x,x012);
      s = (*rhs)(x012)*ndetB;
      NDS(n0,f) += c0*s;
      NDS(n1,f) += c1*s;
      NDS(n2,f) += c2*s;
      NDS(n3,f) += -c3i*s;
   }
   s0 *= ndetB;
   s1 *= ndetB;
   s2 *= ndetB;
   COEFFN(n0,Z) += (c0i+c0)*s0;
   COEFFN(n1,Z) += (c1i+c1)*s1;
   COEFFN(n2,Z) += (c2i+c2)*s2;
   putaij(n0->tstart,n1,n2,(c0i+c0)*s1,(c0i+c0)*s2,Z);
   putaij(n1->tstart,n2,n0,(c1i+c1)*s2,(c1i+c1)*s0,Z);
   putaij(n2->tstart,n0,n1,(c2i+c2)*s0,(c2i+c2)*s1,Z);
}

#else

void add_Q1C_MH_for_half_quadrilateral(n0,n1,n2,n3,Z,u,f,bb_xc,rhs)
NODE *n0, *n1, *n2, *n3; INT Z, u, f; FLOAT *bb_xc, (*rhs)();
{  eprintf("Error: add_Q1C_MH_for_half_quadrilateral not available.\n");  }

#endif

void add_projected_Q1C_MH_conv_term_matr(tGrid,Z,u,f,bb0,bb1,react,rhs)
GRID *tGrid;
INT Z, u, f;
FLOAT (*bb0)(), (*bb1)(), (*react)(), (*rhs)();
{
   ELEMENT *pelem;
   NODE *n0, *n1, *n2, *n3;
   FLOAT bb_xc[DIM], xc[DIM], s02[DIM], s13[DIM], s;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_4ELEMENT(n0,n1,n2,n3,pelem);
      coord_of_barycentre(pelem,xc);
      bb_xc[0] = bb0(xc);
      bb_xc[1] = bb1(xc);
      SUBTR(n0->myvertex->x,n2->myvertex->x,s02);
      SUBTR(n1->myvertex->x,n3->myvertex->x,s13);
      UNIT_VECTOR(s02,s02,s)
      UNIT_VECTOR(s13,s13,s)
      if (fabs(DOT(bb_xc,s02)) > fabs(DOT(bb_xc,s13))){
         add_Q1C_MH_for_half_quadrilateral(n0,n1,n2,n3,Z,u,f,bb_xc,rhs);
         add_Q1C_MH_for_half_quadrilateral(n0,n3,n2,n1,Z,u,f,bb_xc,rhs);
      }
      else{
         add_Q1C_MH_for_half_quadrilateral(n1,n0,n3,n2,Z,u,f,bb_xc,rhs);
         add_Q1C_MH_for_half_quadrilateral(n1,n2,n3,n0,Z,u,f,bb_xc,rhs);
      }
   }
}

/* minimum of max(0,a+b*x) on [xmin,xmax] */
FLOAT pos_min_lin(a,b,xmin,xmax)
FLOAT a, b, xmin, xmax;
{
   FLOAT eps=1.e-15;

   if (fabs(b) < eps)
      return(MAX(0,a));
   else if (b > 0.){
      if (xmin > -a/b)
         return(a+b*xmin);
      else
         return(0.);
   }
   else{
      if (xmax < -a/b) 
         return(a+b*xmax);
      else
         return(0.);
   }
}

/* minimum of max(0,a1+b1*x,a2+b2*x) on [xmin,xmax] */
FLOAT pos_min_max_2lin(a1,b1,a2,b2,xmin,xmax)
FLOAT a1, b1, a2, b2, xmin, xmax;
{
   FLOAT x0, v0, z, eps=1.e-15;

   if (fabs(b1-b2) < eps)
      return(pos_min_lin(MAX(a1,a2),b1,xmin,xmax));
   else if (fabs(b1) < eps){
      if (a1 > 0.)
         return(a1 + pos_min_lin(a2-a1,b2,xmin,xmax));
      else
         return(pos_min_lin(a2,b2,xmin,xmax));
   }
   else if (fabs(b2) < eps){
      if (a2 > 0.)
         return(a2 + pos_min_lin(a1-a2,b1,xmin,xmax));
      else
         return(pos_min_lin(a1,b1,xmin,xmax));
   }
   else if (b1 < b2){
      EXCHANGE(a1,a2,z)
      EXCHANGE(b1,b2,z)
   }
   x0 = -(a1-a2)/(b1-b2);
   v0 = (a2*b1-a1*b2)/(b1-b2);
   if (b1 > 0. && b2 > 0.){
      if (v0 > 0.){
         if (xmin > x0)
            return(a1+b1*xmin);
         else if (xmin > -a2/b2)
            return(a2+b2*xmin);
         else
            return(0.);
      }
      else
         return(pos_min_lin(a1,b1,xmin,xmax));
   }
   else if (b1 < 0. && b2 < 0.){
      if (v0 > 0.){
         if (xmax < x0)
            return(a2+b2*xmax);
         else if (xmax < -a1/b1)
            return(a1+b1*xmax);
         else
            return(0.);
      }
      else
         return(pos_min_lin(a2,b2,xmin,xmax));
   }
   else if (v0 < 0.){
      if (xmax < -a2/b2)
         return(a2+b2*xmax);
      else if (xmin > -a1/b1)
         return(a1+b1*xmin);
      else
         return(0.);
   }
   else if (xmax < x0)
      return(a2+b2*xmax);
   else if (xmin > x0)
      return(a1+b1*xmin);
   else
      return(v0);
}

FLOAT reaction_induced_change_of_const(c1,min1,max1,s2,s3,p2,p3,r_xc)
FLOAT c1, min1, max1, s2, s3, p2, p3, r_xc;
{
   FLOAT c;

   if (r_xc < EPSC)
      return(c1);
   else{
      c = pos_min_max_2lin(12.*s2 + 4.*r_xc, -12.*p2,
                           12.*s3 + 4.*r_xc, -12.*p3, min1,max1);
      if (c < EPSC)
         return(c1);
      else{
         c = -1. + r_xc/c;
         return(MIN(c1,c));
      }
   }
}

INT exists_alpha(p0,p1,p2,s0,s1,s2,amin,amax) /*  = 1, if a exists such that  */
FLOAT p0, p1, p2, s0, s1, s2, *amin, *amax;   /*       s0 > a*p0, s1 <= a*p1, */
{                                             /*                  s2 <= a*p2  */
   FLOAT min=-1.e200, max=1.e200, s;
   INT exists=1;

   if (fabs(p1) < EPSC){
      if (s1 > 0.)
         exists = 0;
   }
   else{
      s = s1/p1;
      if (p1 > 0.)
         min = s;
      else
         max = s;
   }
   if (fabs(p2) < EPSC){
      if (s2 > 0.)
         exists = 0;
   }
   else{
      s = s2/p2;
      if (p2 > 0.)
         min = MAX(min,s);
      else
         max = MIN(max,s);
   }
   if (min > max)
      exists = 0;
   if (exists){
      if (fabs(p0) < EPSC){
         if (s0 < EPSC)
            exists = 0;
      }
      else{
         s = s0/p0;
         if (p0 > 0.){ 
            if (min >= s)
               exists = 0;
            else
               max = MIN(max,s-EPSC);
         }
         else{
            if (max <= s)
               exists = 0;
            else
               min = MAX(min,s+EPSC);
         }
      }
   }
   if (min > max)
      exists = 0;
   *amin = min;
   *amax = max;
   return(exists);
}

FLOAT coeff_for_correction_of_c(x,y,xm,ym,t)
FLOAT x, y, xm, ym, t;
{
   FLOAT z=x*t/xm;

   if ((x > 0.0001) && (y < z)){
      z = 1. - y/z;
      return(z*z);
   }
   else
      return(0.);
}

#define MAX_SIN   0.01
#define MAX_SIN2  0.02
#define LARGE_GRAD 1.e-8
#define SMALL_GRAD 1.e-10

void treat_edge_zone9(grad_u,bb_xc,r_xc,teps,n0,n1,n2,s0,s1,s2,b0,b1,b2,c0,c1,c2)
NODE *n0, *n1, *n2;                                  /* edge zone opposite n0 */
FLOAT *grad_u, *bb_xc, r_xc, teps, s0, s1, s2, *b0, *b1, *b2, *c0, *c1, *c2;
{
   FLOAT v1[DIM], v2[DIM], vm[DIM], v1o[DIM], v2o[DIM], vmo[DIM], w[DIM],
         bn[DIM], bno[DIM], d, sb, sw, sbm, swm, gu, min, max, p0, p1, p2;
   INT i1=0, i2=0, flag=0;

   ORT_VECT(w,grad_u)
   if (fabs(w[0]) > EPSC || fabs(w[1]) > EPSC)
      UNIT_VECTOR(w,w,d)
   p0 = DOT(b0,w);
   p1 = DOT(b1,w);
   p2 = DOT(b2,w);
   *c0 = *c1 = *c2 = -1.;
   if (IS_DN(n1) && IS_DN(n2))
      *c1 = *c2 = 0.5;
   else if (IS_DN(n1))
      *c1 = 2.;
   else if (IS_DN(n2))
      *c2 = 2.;
   else if ((gu=sqrt(DOT(grad_u,grad_u))) < SMALL_GRAD)
      *c1 = *c2 = 0.5;
   else{
      SET11(v1,n1->myvertex->x,n0->myvertex->x)
      SET11(v2,n2->myvertex->x,n0->myvertex->x)
      UNIT_VECTOR(v1,v1,d)
      UNIT_VECTOR(v2,v2,d)
      SET10(vm,v1,v2)
      UNIT_VECTOR(bn,bb_xc,d)
      ORT_VECT_DIR(bno,bn,v2)
      ORT_VECT_DIR(w,grad_u,vm)
      SET22(w,w,gu)
      if (gu < 1.e-8){
         flag = 1;
      }
      else if (fabs(d=DOT(bno,w)) < MAX_SIN){
/*
         if (fabs(DOT(v1,bno)) > MAX_SIN2 && fabs(DOT(v2,bno)) > MAX_SIN2){
            *c1 = 0.5 + 1.5*d/MAX_SIN;
            flag = 3;
         }
         else
*/
            flag = 2;
      }
      if (!flag){
         i1 = exists_alpha(p1,p0,p2,s1,s0,s2,&min,&max);
         i2 = exists_alpha(p2,p0,p1,s2,s0,s1,&min,&max);
      }
      if ((i1 && i2) || flag == 1 || flag == 2){
         d = (DOT(v1,bn)-DOT(v2,bn))/(1.-DOT(v1,v2));
         *c1 = 0.5 + 1.5*d;
         *c2 = 1. - *c1;
         UNIT_VECTOR(vm,vm,d)
         ORT_VECT(v1o,v1)
         ORT_VECT(v2o,v2)
         ORT_VECT_DIR(vmo,vm,v2)
         swm = DOT(v1,vm);
         sbm = fabs(DOT(v2,vmo));
         if (DOT(w,vmo) > 0.){      /*  w lies in VZ2  */
            sw = fabs(DOT(w,v2o)); 
            if (DOT(bn,vmo) > 0.){  /*  b points to n2 */
               sb = fabs(DOT(bn,v2o));
               d = coeff_for_correction_of_c(sb,sw,sbm,swm,0.3);
               *c1 = (1.-d)*(*c1) + 2.*d; 
            }
            else{                   /*  b points to n1 */
               if (sw < 0.2*swm){
                  d = 1. - sw/(0.2*swm);
                  d *= d;
                  *c1 = 1. - (1.-d)*(*c2) + d;
               }
            }
         }
         else{                      /*  w lies in VZ1  */
            sw = fabs(DOT(w,v1o)); 
            if (DOT(bn,vmo) > 0.){  /*  b points to n2 */
               if (sw < 0.2*swm){
                  d = 1. - sw/(0.2*swm);
                  d *= d;
                  *c1 = (1.-d)*(*c1) - d;
               }
            }
            else{                   /*  b points to n1 */
               sb = fabs(DOT(bn,v1o));
               d = coeff_for_correction_of_c(sb,sw,sbm,swm,0.3);
               *c1 = (1.-d)*(*c1) - d; 
            }

         }
      }
      else if (i1){
            *c1 = 2.;
      }
      else if (i2){
            *c1 = -1.;
      }
      else
         eprintf("Error in treat_edge_zone.\n");
      if (gu < LARGE_GRAD){
         d = (gu-SMALL_GRAD)/(LARGE_GRAD-SMALL_GRAD);
         *c1 = d*(*c1) + (1.-d)*0.5;
      }
      *c2 = 1. - *c1;
   }
}

void treat_vertex_zone(c0,c1,c2,s0,s1,s2,b0,b1,b2,r_xc,teps)
FLOAT *c0, *c1, *c2, s0, s1, s2, *b0, *b1, *b2, r_xc, teps;
{                                                        /* vertex zone at n0 */
   FLOAT c, q1, q2;

   if (r_xc < EPSC){
      c = 3.*teps/s0;
      *c1 = -1. - c*DOT(b0,b1);
      *c2 = -1. - c*DOT(b0,b2);
      *c1 = MIN(0.,*c1);
      *c2 = MIN(0.,*c2);
      *c0 = -(*c1) - (*c2);
   }
   else{
      *c1 = *c2 = -1.;
      q1 = 12.*s1 + 4.*r_xc;
      q2 = 12.*s2 + 4.*r_xc;
      c = MAX(q1,q2);
      if (c < EPSC)
         *c0 = 2.;
      else{
         c = -1. + r_xc/c;
         *c0 = MIN(2.,c);
      }
   }
}

FLOAT gg(x)
FLOAT x;
{
   return(x);
}

FLOAT constant_c3(v2,v3,vm,bb_xc,w)
FLOAT *v2, *v3, *vm, *bb_xc, *w;
{
   DOUBLE bn[DIM], v3o[DIM], phi=1., d, r=1.;

   UNIT_VECTOR(bn,bb_xc,d)
   ORT_VECT(v3o,v3)
   if (DOT(bb_xc,v3) > 0. && (d=fabs(DOT(bn,v3o)/DOT(vm,v3o))) < 1.)
      r = d;
   if ((r*=DOT(vm,v3)) > (d=2.*fabs(DOT(w,v3o))))
      phi = d/r;
   return(phi*(1. + gg((DOT(bn,v3)-DOT(bn,v2))/(1.-DOT(v2,v3)))));
}

void modified_treat_edge_zone(grad_u,bb_xc,r_xc,teps,n1,n2,n3,s1,s2,s3,b1,b2,b3,
                                                                       c1,c2,c3)
NODE *n1, *n2, *n3;                                  /* edge zone opposite n1 */
FLOAT *grad_u, *bb_xc, r_xc, teps, s1, s2, s3, *b1, *b2, *b3, *c1, *c2, *c3;
{
   FLOAT p1, p2, p3, w[DIM], v2[DIM], v3[DIM], vm[DIM], vmo[DIM], d,
         min2, min3, max2, max3, c21=0., c22=0., c23=0., c31=0., c32=0., c33=0.;
   INT i2=0, i3=0;

   if (  (NOT_FN(n1) || NOT_FN(n2) || NOT_FN(n3))
      || (COARSE_GRID != cube && COARSE_GRID != cube_for_donut && 
            n1->tstart != n1->start && n2->tstart != n2->start && 
                                       n3->tstart != n3->start)
      || ((USE_COARSE_F == YES) && (n1->tstart != n1->start ||
         n2->tstart != n2->start || n3->tstart != n3->start))  ){
         if ((p2=DOT(b1,b2)) < (p3=DOT(b2,b3)))
            p2 = p3;
         p2 /= s2;
         if (p3 < (p1=DOT(b3,b1))) 
            p3 = p1;
         p3 /= s3;
         d = -1. - 3.*teps*MAX(p2,p3);
      *c1 = *c2 = *c3 = MIN(0.,d);
   }
   else if (fabs(DOT(bb_xc,grad_u)) < EPSC*MAX(fabs(bb_xc[0]),fabs(bb_xc[1]))){
      *c1 = -1.;
      if (r_xc < EPSC)
         *c2 = *c3 = 0.5;
      else
         *c2 = *c3 = -0.75;
   }
   else{
      ORT_VECT(w,grad_u)
      UNIT_VECTOR(w,w,d)
      p1 = DOT(b1,w);
      p2 = DOT(b2,w);
      p3 = DOT(b3,w);
      i2 = exists_alpha(p2,p1,p3,s2,s1,s3,&min2,&max2);
      i3 = exists_alpha(p3,p1,p2,s3,s1,s2,&min3,&max3);
      if (i2 && !i3){
         *c2 = reaction_induced_change_of_const(2.,min2,max2,s1,s3,p1,p3,r_xc);
         *c3 = -1.;
      }
      else if (!i2 && i3){
         *c2 = -1.;
         *c3 = reaction_induced_change_of_const(2.,min3,max3,s1,s2,p1,p2,r_xc);
      }
      else{
         SET11(v2,n2->myvertex->x,n1->myvertex->x)
         SET11(v3,n3->myvertex->x,n1->myvertex->x)
         UNIT_VECTOR(v2,v2,d)
         UNIT_VECTOR(v3,v3,d)
         SET10(vm,v2,v3)
         UNIT_VECTOR(vm,vm,d)
         ORT_VECT_DIR(vmo,vm,v3)
         if (DOT(w,vm) < 0.)
            SET8(w,w)
         if (DOT(w,vmo) < 0.)
            *c3 = 2. - 1.5*constant_c3(v3,v2,vm,bb_xc,w);
         else 
            *c3 = -1. + 1.5*constant_c3(v2,v3,vm,bb_xc,w);
         *c2 = 1. - *c3; 
         *c2 = reaction_induced_change_of_const(*c2,min2,max2,s1,s3,p1,p3,r_xc);
         *c3 = reaction_induced_change_of_const(*c3,min3,max3,s1,s2,p1,p2,r_xc);
      }
      if (i2){
         if (p2 > 0.)
            d = 3.*teps/(s2-max2*p2);
         else
            d = 3.*teps/(s2-min2*p2);
         c21 = -1. - d*DOT(b2,b1);
         c23 = -1. - d*DOT(b2,b3);
         c21 = MIN(0.,c21);
         c23 = MIN(0.,c23);
         c22 = -c21 - c23;
      }
      if (i3){
         if (p3 > 0.)
            d = 3.*teps/(s3-max3*p3);
         else
            d = 3.*teps/(s3-min3*p3);
         c31 = -1. - d*DOT(b3,b1);
         c32 = -1. - d*DOT(b3,b2);
         c31 = MIN(0.,c31);
         c32 = MIN(0.,c32);
         c33 = -c31 - c32;
      }
      if (r_xc < EPSC){
           d = ((1.+(*c2))*c22 + (1.+(*c3))*c32)/3.;
         *c3 = ((1.+(*c2))*c23 + (1.+(*c3))*c33)/3.;
         *c2 = d;
         *c1 = -(*c2) - (*c3);
      }
   }
}

void mh_treat_edge_zone(grad_u,bb_xc,r_xc,teps,
                        n1,n2,n3,s1,s2,s3,b1,b2,b3,c1,c2,c3)
NODE *n1, *n2, *n3;                                  /* edge zone opposite n1 */
FLOAT *grad_u, *bb_xc, r_xc, teps, s1, s2, s3, *b1, *b2, *b3, *c1, *c2, *c3;
{
   FLOAT p1, p2, p3, d, min, max, w[DIM];
   INT i2=0, i3=0;

   if (fabs(DOT(bb_xc,grad_u)) < EPSC*MAX(fabs(bb_xc[0]),fabs(bb_xc[1])))
      *c3 = s3/fabs(s1);
   else{
      ORT_VECT(w,grad_u)
      UNIT_VECTOR(w,w,d)
      p1 = DOT(b1,w);
      p2 = DOT(b2,w);
      p3 = DOT(b3,w);
      i2 = exists_alpha(p2,p1,p3,s2,s1,s3,&min,&max);
      i3 = exists_alpha(p3,p1,p2,s3,s1,s2,&min,&max);
      if (i2 && !i3)
         *c3 = -1.;
      else if (!i2 && i3)
         *c3 = 2.;
      else
         *c3 = s3/fabs(s1);
   }
   *c2 = 1. - *c3; 
   *c1 = -1.;
}

void compute_upwind_const(pelem,xc,bb_xc,r_xc,b,u,s0,s1,s2,c0,c1,c2,space,
                          treat_edge_zone)
ELEMENT *pelem;
FLOAT *xc, *bb_xc, r_xc, b[DIM2][DIM2], s0, s1, s2, *c0, *c1, *c2;
INT u, space;
void (*treat_edge_zone)();
{
   NODE *n0, *n1, *n2;
   FLOAT grad_u[DIM], w[DIM], d, teps=0.*0.95*VALUE_OF_EPS;

   if (fabs(bb_xc[0]) < EPSC && fabs(bb_xc[1]) < EPSC){
      if (r_xc < EPSC)
         *c0 = *c1 = *c2 = 0.;
      else
         *c0 = *c1 = *c2 = -0.75;
   }
   else if (s0 > EPSC && s1 < EPSC && s2 < EPSC)
      treat_vertex_zone(c0,c1,c2,s0,s1,s2,b[0],b[1],b[2],r_xc,teps);
   else if (s1 > EPSC && s2 < EPSC && s0 < EPSC)
      treat_vertex_zone(c1,c2,c0,s1,s2,s0,b[1],b[2],b[0],r_xc,teps);
   else if (s2 > EPSC && s0 < EPSC && s1 < EPSC)
      treat_vertex_zone(c2,c0,c1,s2,s0,s1,b[2],b[0],b[1],r_xc,teps);
   else{
      sgrad_value(pelem,xc,b,u,space,grad_u);
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      if (s0 < -EPSC)
         treat_edge_zone(grad_u,bb_xc,r_xc,teps,n0,n1,n2,s0,s1,s2,
                                     b[0],b[1],b[2],c0,c1,c2);
      else if (s1 < -EPSC)
         treat_edge_zone(grad_u,bb_xc,r_xc,teps,n1,n2,n0,s1,s2,s0,
                                     b[1],b[2],b[0],c1,c2,c0);
      else if (s2 < -EPSC)
         treat_edge_zone(grad_u,bb_xc,r_xc,teps,n2,n0,n1,s2,s0,s1,
                                     b[2],b[0],b[1],c2,c0,c1);
      else
         eprintf("Error in compute_upwind_const.\n");
   }
}

#if (N_DATA & SCALAR_NODE_DATA) && (N_DATA & ONE_NODE_MATR)

void add_P1C_MH_conv_term_matr(tGrid,Z,u,f,bb0,bb1,react,rhs)
GRID *tGrid;                  /*  Mizukami, Hughes, CMAME 50 (1985), 181-193  */
INT Z, u, f;
FLOAT (*bb0)(), (*bb1)(), (*react)(), (*rhs)();
{
   ELEMENT *pelem;
   NODE *n0, *n1, *n2;
   FLOAT bb_xc[DIM], r_xc, xc[DIM], b[DIM2][DIM2], x01[DIM], x02[DIM], x12[DIM],
         x012[DIM], *x0, *x1, *x2, c0, c1, c2, s0, s1, s2, s, z, ndetB;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      ndetB = barycentric_coordinates(x0,x1,x2,b)/3.;
      POINT3(x0,x1,x2,xc);
      bb_xc[0] = bb0(xc);
      bb_xc[1] = bb1(xc);
      r_xc     = react(xc);
      s0 = DOT(b[0],bb_xc);
      s1 = DOT(b[1],bb_xc);
      s2 = DOT(b[2],bb_xc);
      if (MH_VER == 0)
         compute_upwind_const(pelem,xc,bb_xc,r_xc,b,u,s0,s1,s2,&c0,&c1,&c2,P1C,
                              mh_treat_edge_zone);
      else if (MH_VER == 1)
         compute_upwind_const(pelem,xc,bb_xc,r_xc,b,u,s0,s1,s2,&c0,&c1,&c2,P1C,
                              modified_treat_edge_zone);
      else if (MH_VER == 9)
         compute_upwind_const(pelem,xc,bb_xc,r_xc,b,u,s0,s1,s2,&c0,&c1,&c2,P1C,
                              treat_edge_zone9);
      else
         eprintf("Error in add_P1C_MH_conv_term_matr.\n");
// c1 = MIN(-c0 - c2,-0.75+3600./r_xc*VALUE_OF_EPS);
      s0 *= ndetB;
      s1 *= ndetB;
      s2 *= ndetB;
      z = r_xc*ndetB/3.;
      COEFFN(n0,Z) += (1.+c0)*s0 + (1.5+c0)*z;
      COEFFN(n1,Z) += (1.+c1)*s1 + (1.5+c1)*z;
      COEFFN(n2,Z) += (1.+c2)*s2 + (1.5+c2)*z;
      putaij(n0->tstart,n1,n2,(1.+c0)*s1 + (0.75+c0)*z,
                              (1.+c0)*s2 + (0.75+c0)*z,Z);
      putaij(n1->tstart,n2,n0,(1.+c1)*s2 + (0.75+c1)*z,
                              (1.+c1)*s0 + (0.75+c1)*z,Z);
      putaij(n2->tstart,n0,n1,(1.+c2)*s0 + (0.75+c2)*z,
                              (1.+c2)*s1 + (0.75+c2)*z,Z);
      if (RHS_INTEGR == ZERO)
         s = (*rhs)(xc)*ndetB;
      else{
         AVERAGE(x0,x1,x01);
         AVERAGE(x0,x2,x02);
         AVERAGE(x1,x2,x12);
         if (RHS_INTEGR == QUADRATIC)
            s = ((*rhs)(x01) + (*rhs)(x02) + (*rhs)(x12))*ndetB/3.;
         else{
            POINT3(x0,x1,x2,x012);
            s = (3.*((*rhs)(x0)  + (*rhs)(x1)  + (*rhs)(x2)) +
                 8.*((*rhs)(x01) + (*rhs)(x02) + (*rhs)(x12)) +
                 27.*(*rhs)(x012))*ndetB/60;
         }
      }
      NDS(n0,f) += c0*s;
      NDS(n1,f) += c1*s;
      NDS(n2,f) += c2*s;
   }
}

void P1C_upwind_conv_term(tGrid,nu,Z,bb0,bb1)
GRID *tGrid;
FLOAT nu, (*bb0)(), (*bb1)();
INT Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], c[DIM], 
         a0, a1, a2, b0=0., b1=0., b2=0., b[DIM2][DIM2], rdetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
      if (IS_BF(fa0))
         b0 = -( (bb_1[0]+bb_2[0])*b[0][0] 
               + (bb_1[1]+bb_2[1])*b[0][1] )*rdetB;
      if (IS_BF(fa1))
         b1 = -( (bb_0[0]+bb_2[0])*b[1][0] 
               + (bb_0[1]+bb_2[1])*b[1][1] )*rdetB;
      if (IS_BF(fa2))
         b2 = -( (bb_0[0]+bb_1[0])*b[2][0] 
               + (bb_0[1]+bb_1[1])*b[2][1] )*rdetB;
      c[0] = (bb_0[0] +  bb_1[0] +  bb_2[0])/3.;
      c[1] = (bb_0[1] +  bb_1[1] +  bb_2[1])/3.;
      rdetB /= 3.;
      a0 = ( (bb_0[0]+c[0])*(b[1][0]-b[2][0])
           + (bb_0[1]+c[1])*(b[1][1]-b[2][1]) )*rdetB;
      a1 = ( (bb_1[0]+c[0])*(b[2][0]-b[0][0])
           + (bb_1[1]+c[1])*(b[2][1]-b[0][1]) )*rdetB;
      a2 = ( (bb_2[0]+c[0])*(b[0][0]-b[1][0])
           + (bb_2[1]+c[1])*(b[0][1]-b[1][1]) )*rdetB;
      p1c_up_ij(n0,n1,n2,fa0,nu,Z,b0,a2,-a1);
      p1c_up_ij(n1,n2,n0,fa1,nu,Z,b1,a0,-a2);
      p1c_up_ij(n2,n0,n1,fa2,nu,Z,b2,a1,-a0);
   } 
}

void p1c_up2_ij(n0,n1,n2,fa0,eps,Z,b01,b02,lambda)
NODE *n0, *n1, *n2;
FACE *fa0;
INT Z;
FLOAT eps, b01, b02, (*lambda)();
{
   FLOAT a1, a2;

   a1 = b01*(1.-lambda(b01));
   a2 = b02*(1.-lambda(b02));
   COEFFN(n0,Z) -= a1 + a2;
   putaij(n0->tstart,n1,n2,a1,a2,Z);
}

void p1c_up2t_ij(n0,n1,n2,fa0,eps,Z,b01,b02,tb01,tb02,lambda)
NODE *n0, *n1, *n2;
FACE *fa0;
INT Z;
FLOAT eps, b01, b02, tb01, tb02, (*lambda)();
{
   FLOAT a1, a2;

   a1 = b01*(1.-lambda(tb01));
   a2 = b02*(1.-lambda(tb02));
   COEFFN(n0,Z) -= a1 + a2;
   putaij(n0->tstart,n1,n2,a1,a2,Z);
}

void compute_barycentric_volumes(tGrid,v)
GRID *tGrid;
INT v;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT s;
  
   set_value(tGrid,0.,v,0,Q_SN);
   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      s = VOLUME(pelem)/3.;
      NDS(n0,v) += s;
      NDS(n1,v) += s;
      NDS(n2,v) += s;
   }
}

void p1c_tabata_up_ij(n0,n1,n2,Z,v,w,bb0,bb1)
NODE *n0, *n1, *n2;
INT Z, v, w;
FLOAT (*bb0)(), (*bb1)();
{
   FLOAT *x0, *x1, *x2, t01[DIM], t12[DIM], t20[DIM], t[DIM], b[DIM], bo[DIM], 
         a, d;
   LINK *pli;

   x0 = n0->myvertex->x;
   x1 = n1->myvertex->x;
   x2 = n2->myvertex->x;
   SUBTR(x1,x0,t01);
   SUBTR(x2,x1,t12);
   b[0] = bb0(x0);
   b[1] = bb1(x0);
   ORT_VECT(bo,b)
   if (fabs(a=DOT(bo,t12)) > 1.e-15*(fabs(t12[0])+fabs(t12[1]))){
      a = -DOT(bo,t01)/a;
      SET23(t,t01,t12,a)
      if (NDS(n0,w) < 0.5 && DOT(b,t) < 0. && a > -1.e-5 && a-1. < 1.e-5){
         NDS(n0,w) = 1.;
         d = sqrt(DOT(b,b)/DOT(t,t))*NDS(n0,v);
         COEFFN(n0,Z) += d;
         if (a > 0. && a < 1.)
            putaij(n0->tstart,n1,n2,(a-1.)*d,-a*d,Z);
         else if (a < 0.5){ 
            for (pli=n0->tstart; pli->nbnode != n1; pli=pli->next);
            COEFFL(pli,Z) -= d;
         }
         else{ 
            for (pli=n0->tstart; pli->nbnode != n2; pli=pli->next);
            COEFFL(pli,Z) -= d;
         }
      }
   }
}

FLOAT check_P1_method_on_cube_t2(n0,h,b,u)
NODE *n0;
FLOAT h, *b;
INT u;
{
   LINK *pli;
   FLOAT *x0, *xn, v, eps=h*0.01, sum=b[0]*NDS(n0,u);

   x0 = n0->myvertex->x;
   for (pli=n0->tstart; pli; pli=pli->next){
      xn = pli->nbnode->myvertex->x;
      v  = NDS(pli->nbnode,u);
      if (fabs(x0[0]+h-xn[0]) < eps && fabs(x0[1]-xn[1]) < eps)
         sum += b[1]*v;
      else if (fabs(x0[0]-xn[0]) < eps && fabs(x0[1]+h-xn[1]) < eps)
         sum += b[2]*v;
      else if (fabs(x0[0]-h-xn[0]) < eps && fabs(x0[1]+h-xn[1]) < eps)
         sum += b[3]*v;
      else if (fabs(x0[0]-h-xn[0]) < eps && fabs(x0[1]-xn[1]) < eps)
         sum += b[4]*v;
      else if (fabs(x0[0]-xn[0]) < eps && fabs(x0[1]-h-xn[1]) < eps)
         sum += b[5]*v;
      else if (fabs(x0[0]+h-xn[0]) < eps && fabs(x0[1]-h-xn[1]) < eps)
         sum += b[6]*v;
      else 
         eprintf("Error in check_P1_method_on_cube_t2.\n");
   }
   printf("sum = %e\n",fabs(sum));
   return(fabs(sum));
}

#else

void add_P1C_MH_conv_term_matr(tGrid,Z,u,f,bb0,bb1,react,rhs)
GRID *tGrid; INT Z, u, f; FLOAT (*bb0)(), (*bb1)(), (*react)(), (*rhs)();
{  eprintf("Error: add_P1C_MH_conv_term_matr not available.\n");  }

void P1C_upwind_conv_term(tGrid,nu,Z,bb0,bb1)
GRID *tGrid; FLOAT nu, (*bb0)(), (*bb1)(); INT Z;
{  eprintf("Error: P1C_upwind_conv_term not available.\n");  }

void p1c_up2_ij(n0,n1,n2,fa0,eps,Z,b01,b02,lambda)
NODE *n0, *n1, *n2; FACE *fa0; INT Z; FLOAT eps, b01, b02, (*lambda)();
{  eprintf("Error: p1c_up2_ij not available.\n");  }

void p1c_up2t_ij(n0,n1,n2,fa0,eps,Z,b01,b02,tb01,tb02,lambda)
NODE *n0, *n1, *n2; FACE *fa0; INT Z; FLOAT eps, b01, b02, tb01, tb02, (*lambda)();
{  eprintf("Error: p1c_up2t_ij not available.\n");  }

void compute_barycentric_volumes(tGrid,v)
GRID *tGrid; INT v;
{  eprintf("Error: compute_barycentric_volumes not available.\n");  }

void p1c_tabata_up_ij(n0,n1,n2,Z,v,w,bb0,bb1)
NODE *n0, *n1, *n2; INT Z, v, w; FLOAT (*bb0)(), (*bb1)();
{  eprintf("Error: p1c_tabata_up_ij not available.\n");  }

FLOAT check_P1_method_on_cube_t2(n0,h,b,u)
NODE *n0; FLOAT h, *b; INT u;
{  eprintf("Error: check_P1_method_on_cube_t2 not available.\n"); return(0.);  }

#endif

/* bij is the integral over the line between the barycentre and the midpoint
of the edge (ni,nj) of the vector b multiplied by the unit normal vector
pointing to nj */
void barycentric_fluxes(n0,n1,n2,b01,b12,b20,bb0,bb1)
NODE *n0, *n1, *n2;
FLOAT *b01, *b12, *b20, (*bb0)(), (*bb1)();
{
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], c[DIM], b[DIM2][DIM2], ndetB;

   ndetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                   n2->myvertex->x,b)/12.;
   V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
   c[0] = (bb_0[0] +  bb_1[0] +  bb_2[0])*5./3.;
   c[1] = (bb_0[1] +  bb_1[1] +  bb_2[1])*5./3.;
   *b01 = ndetB*DOT_DIFF(c,bb_2,b[1],b[0]);
   *b12 = ndetB*DOT_DIFF(c,bb_0,b[2],b[1]);
   *b20 = ndetB*DOT_DIFF(c,bb_1,b[0],b[2]);
}

/* Let M be the point, where axes of all three edges cross. Then bij is the 
integral over the line between M and the midpoint of the edge (ni,nj) of the 
vector b multiplied by the unit normal vector pointing to nj */
void circumcentric_fluxes(n0,n1,n2,b01,b12,b20,bb0,bb1)
NODE *n0, *n1, *n2;
FLOAT *b01, *b12, *b20, (*bb0)(), (*bb1)();
{
   FLOAT *x0, *x1, *x2, x01[DIM], x02[DIM], x12[DIM], x012[DIM],
         t01[DIM], t12[DIM], t20[DIM], n01[DIM], n12[DIM], n20[DIM], 
         d01[DIM], d12[DIM], d20[DIM], bb_0[DIM], bb_1[DIM], bb_2[DIM], 
         m[DIM], c[DIM], s, s0, s1, s2;

   x0 = n0->myvertex->x;
   x1 = n1->myvertex->x;
   x2 = n2->myvertex->x;
   MIDPOINTS(x0,x1,x2,x01,x02,x12)
   SUBTR(x1,x0,t01);
   SUBTR(x2,x1,t12);
   SUBTR(x0,x2,t20);
   ORT_VECT(n01,t01)
   ORT_VECT(n12,t12)
   ORT_VECT(n20,t20)
   POINT3(x0,x1,x2,x012);
   s = 2.*(DOT(t01,n12) + DOT(t12,n20) +DOT(t20,n01));
   s0 = DOT(t12,t20)/s;
   s1 = DOT(t20,t01)/s;
   s2 = DOT(t01,t12)/s;
   m[0] = x012[0] + s0*n01[0] + s1*n12[0] + s2*n20[0];
   m[1] = x012[1] + s0*n01[1] + s1*n12[1] + s2*n20[1];
   SUBTR(x01,m,n01);
   SUBTR(x12,m,n12);
   SUBTR(x02,m,n20);
   V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
   c[0] = bb0(m);
   c[1] = bb1(m);
   SET29(d01,bb_0,bb_1,c,2.)
   SET29(d12,bb_1,bb_2,c,2.)
   SET29(d20,bb_2,bb_0,c,2.)
   *b01 = 0.25*sqrt(DOT(n01,n01)/DOT(t01,t01))*DOT(d01,t01);
   *b12 = 0.25*sqrt(DOT(n12,n12)/DOT(t12,t12))*DOT(d12,t12);
   *b20 = 0.25*sqrt(DOT(n20,n20)/DOT(t20,t20))*DOT(d20,t20);
}

#if F_DATA & VECTOR_FACE_DATA

void add_flux(fa0,t12,b12,t,f)
FACE *fa0;
FLOAT *t12, b12;
INT t, f;
{
   if (fabs(FDV(fa0,t,0))+fabs(FDV(fa0,t,1)) < 1.e-20){
      SET1(FDVP(fa0,t),t12)
      FDV(fa0,f,0) = b12;
   }
   else if (DOT(FDVP(fa0,t),t12) > 0.)
      FDV(fa0,f,0) += b12;
   else
      FDV(fa0,f,0) -= b12;
}

void total_fluxes(tGrid,t,f,fluxes,bb0,bb1)
GRID *tGrid;
INT t, f;
void (*fluxes)();
FLOAT (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, b01, b12, b20, t01[DIM], t12[DIM], t20[DIM];

   set_value(tGrid,0.,t,0,Q_VF);
   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      fluxes(n0,n1,n2,&b01,&b12,&b20,bb0,bb1);
      x0 = n0->myvertex->x;
      x1 = n1->myvertex->x;
      x2 = n2->myvertex->x;
      SUBTR(x1,x0,t01);
      SUBTR(x2,x1,t12);
      SUBTR(x0,x2,t20);
      add_flux(fa0,t12,b12,t,f);
      add_flux(fa1,t20,b20,t,f);
      add_flux(fa2,t01,b01,t,f);
   }
}

FLOAT total_flux(fa0,t12,t,f)
FACE *fa0;
FLOAT *t12;
INT t, f;
{
   if (DOT(FDVP(fa0,t),t12) > 0.)
      return( FDV(fa0,f,0));
   else
      return(-FDV(fa0,f,0));
}

void get_total_fluxes(n0,n1,n2,fa0,fa1,fa2,tb01,tb12,tb20,t,f)
NODE *n0, *n1, *n2;
FACE *fa0, *fa1, *fa2;
FLOAT *tb01, *tb12, *tb20;
INT t, f;
{
   FLOAT *x0, *x1, *x2, t01[DIM], t12[DIM], t20[DIM];

   x0 = n0->myvertex->x;
   x1 = n1->myvertex->x;
   x2 = n2->myvertex->x;
   SUBTR(x1,x0,t01);
   SUBTR(x2,x1,t12);
   SUBTR(x0,x2,t20);
   *tb12 = total_flux(fa0,t12,t,f);
   *tb20 = total_flux(fa1,t20,t,f);
   *tb01 = total_flux(fa2,t01,t,f);
}

#else

void total_fluxes(tGrid,t,f,fluxes,bb0,bb1)
GRID *tGrid; INT t, f; void (*fluxes)(); FLOAT (*bb0)(), (*bb1)();
{  eprintf("Error: total_fluxes not available.\n");  }

void get_total_fluxes(n0,n1,n2,fa0,fa1,fa2,tb01,tb12,tb20,t,f)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; FLOAT *tb01, *tb12, *tb20; INT t, f;
{  eprintf("Error: get_total_fluxes not available.\n");  }

#endif

void P1C_upwind_convective_term(tGrid,eps,Z,bb0,bb1,fluxes,lambda)
GRID *tGrid;                                     /*  for Dirichlet b.c. only  */
FLOAT eps, (*bb0)(), (*bb1)(), (*lambda)();
INT Z;
void (*fluxes)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT b01, b12, b20;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      fluxes(n0,n1,n2,&b01,&b12,&b20,bb0,bb1);
      p1c_up2_ij(n0,n1,n2,fa0,eps,Z,b01,-b20,lambda);
      p1c_up2_ij(n1,n2,n0,fa1,eps,Z,b12,-b01,lambda);
      p1c_up2_ij(n2,n0,n1,fa2,eps,Z,b20,-b12,lambda);
   } 
}

void P1C_upwind_t_convective_term(tGrid,eps,Z,t,f,bb0,bb1,fluxes,lambda)
GRID *tGrid;                                     /*  for Dirichlet b.c. only  */
FLOAT eps, (*bb0)(), (*bb1)(), (*lambda)();
INT Z, t, f;
void (*fluxes)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT b01, b12, b20, tb01, tb12, tb20;

   total_fluxes(tGrid,t,f,fluxes,bb0,bb1);
   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      fluxes(n0,n1,n2,&b01,&b12,&b20,bb0,bb1);
      get_total_fluxes(n0,n1,n2,fa0,fa1,fa2,&tb01,&tb12,&tb20,t,f);
      p1c_up2t_ij(n0,n1,n2,fa0,eps,Z,b01,-b20,tb01,-tb20,lambda);
      p1c_up2t_ij(n1,n2,n0,fa1,eps,Z,b12,-b01,tb12,-tb01,lambda);
      p1c_up2t_ij(n2,n0,n1,fa2,eps,Z,b20,-b12,tb20,-tb12,lambda);
   } 
}

void P1C_tabata_upwind_convective_term(tGrid,Z,v,w,bb0,bb1)
GRID *tGrid;
FLOAT (*bb0)(), (*bb1)();
INT Z, v, w;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;

   compute_barycentric_volumes(tGrid,v);
   set_value(tGrid,0.,w,0,Q_SN);
   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      p1c_tabata_up_ij(n0,n1,n2,Z,v,w,bb0,bb1);
      p1c_tabata_up_ij(n1,n2,n0,Z,v,w,bb0,bb1);
      p1c_tabata_up_ij(n2,n0,n1,Z,v,w,bb0,bb1);
   } 
}

void check_tabata(tGrid,u,b0,b1)
GRID *tGrid;
FLOAT b0, b1;
INT u;
{
   NODE *pnode;
   FLOAT b[7], h=1./(NV_POINTS-1.), sum=0.; 

   printf("***** check_tabata *****\n");
   b[0] = -b1;
   b[1] = 0.;
   b[2] = b0+b1;
   b[3] = -b0;
   b[4] = 0.;
   b[5] = 0.;
   b[6] = 0.;
   for (pnode = FIRSTNODE(tGrid); pnode; pnode = pnode->succ)
      sum += check_P1_method_on_cube_t2(pnode,h,b,u);
   printf("total sum = %e\n",sum);
   printf("***** end of check_tabata *****\n");
}

void check_baba_tabata(tGrid,u,b0,b1)
GRID *tGrid;
FLOAT b0, b1;
INT u;
{
   NODE *pnode;
   FLOAT b[7], h=1./(NV_POINTS-1.), sum=0.; 

   printf("***** check_baba_tabata *****\n");
   b[0] = 2.*(b0-b1);
   b[1] = 0.;
   b[2] = b0+2.*b1;
   b[3] = -b0+b1;
   b[4] = -2.*b0-b1;
   b[5] = 0.;
   b[6] = 0.;
   for (pnode = FIRSTNODE(tGrid); pnode; pnode = pnode->succ)
      sum += check_P1_method_on_cube_t2(pnode,h,b,u);
   printf("total sum = %e\n",sum);
   printf("***** end of check_baba_tabata *****\n");
}

void check_kanayama(tGrid,u,b0,b1)
GRID *tGrid;
FLOAT b0, b1;
INT u;
{
   NODE *pnode;
   FLOAT b[7], h=1./(NV_POINTS-1.), sum=0.; 

   printf("***** check_kanayama *****\n");
   b[0] = b0-b1;
   b[1] = 0.;
   b[2] = b1;
   b[3] = 0.;
   b[4] = -b0;
   b[5] = 0.;
   b[6] = 0.;
   for (pnode = FIRSTNODE(tGrid); pnode; pnode = pnode->succ)
      sum += check_P1_method_on_cube_t2(pnode,h,b,u);
   printf("total sum = %e\n",sum);
   printf("***** end of check_kanayama *****\n");
}

#if (ELEMENT_TYPE == SIMPLEX) && (E_DATA & ExN_MATR) && (E_DATA & NxE_MATR) && (E_DATA & ExE_MATR)

void add_P1CB_conv_term_matr(tGrid,Z,bb0,bb1)
GRID *tGrid;
INT Z;
FLOAT (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], s[DIM], b[DIM2][DIM2], rdetB, ndetB, 
         a0, a1, a2;;
  
   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
      SET14(s,bb_0,bb_1,bb_2)
      ndetB = rdetB/12.;
      c_p1c_ij(n0,n1,n2,Z,bb_0,s,b[0],b[1],b[2],ndetB);
      c_p1c_ij(n1,n2,n0,Z,bb_1,s,b[1],b[2],b[0],ndetB);
      c_p1c_ij(n2,n0,n1,Z,bb_2,s,b[2],b[0],b[1],ndetB);
      a0 = DOT(bb_0,b[0]);
      a1 = DOT(bb_1,b[1]);
      a2 = DOT(bb_2,b[2]);
      ndetB = rdetB/180.;
      COEFF_NE(pelem,Z,0) += (-a0-a0 + DOT(bb_1,b[2]) + DOT(bb_2,b[1]))*ndetB;
      COEFF_NE(pelem,Z,1) += (-a1-a1 + DOT(bb_0,b[2]) + DOT(bb_2,b[0]))*ndetB;
      COEFF_NE(pelem,Z,2) += (-a2-a2 + DOT(bb_0,b[1]) + DOT(bb_1,b[0]))*ndetB;
      COEFF_EN(pelem,Z,0) += DOT(s,b[0])*ndetB;
      COEFF_EN(pelem,Z,1) += DOT(s,b[1])*ndetB;
      COEFF_EN(pelem,Z,2) += DOT(s,b[2])*ndetB;
      COEFF_EE(pelem,Z) += -(a0 + a1 + a2)*rdetB/5040.;
   }
}

#else

void add_P1CB_conv_term_matr(tGrid,Z,bb0,bb1)
GRID *tGrid; INT Z; FLOAT (*bb0)(), (*bb1)();
{  eprintf("Error: add_P1CB_conv_term_matr not available.\n");  }

#endif

#if (ELEMENT_TYPE == SIMPLEX) && (N_DATA & ONE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (E_DATA & ExN_MATR) && (E_DATA & NxE_MATR) && (E_DATA & ExE_MATR)

void p1cb_stiff_matr(tGrid,Z,a0,a1,a2,a3,a4,i0,i1,i2,
                     f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,integral,finite_el,rmtype,qr)
GRID *tGrid;
INT Z, rmtype, i0, i1, i2;
FLOAT a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(), (*f5)(),
      (*f6)(), (*f7)(), (*f8)(), (*f9)(), (*integral)();
FINITE_ELEMENT finite_el;
QUADRATURE_RULE qr;
{
   ELEMENT *pel;
   LINK *pli;
   REF_MAPPING ref_map;
   DOUBLE_FUNC *nq, *nq0, *nq1, *nq00, *nq01, *nq11,
               *eq, *eq0, *eq1, *eq00, *eq01, *eq11;
   INT i, j;
  
   nq   = finite_el.nbasis;
   nq0  = finite_el.nbasis_0;
   nq1  = finite_el.nbasis_1;
   nq00 = finite_el.nbasis_00;
   nq01 = finite_el.nbasis_01;
   nq11 = finite_el.nbasis_11;
   eq   = finite_el.ebasis;
   eq0  = finite_el.ebasis_0;
   eq1  = finite_el.ebasis_1;
   eq00 = finite_el.ebasis_00;
   eq01 = finite_el.ebasis_01;
   eq11 = finite_el.ebasis_11;
   for (pel = FIRSTELEMENT(tGrid); pel; pel = pel->succ){
   reference_mapping_with_inverse(pel,rmtype,&ref_map);
   for (i=0; i < NVERT; i++){
      for (j=0; j < NVERT; j++)
         if (i == j)
            COEFFN(pel->n[i],Z) += 
               integral(tGrid,pel,nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                                  nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                                  a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                                  f6,f7,f8,f9,finite_el,ref_map,qr);
         else{
            for (pli=pel->n[i]->tstart; pli->nbnode!=pel->n[j]; pli=pli->next);
            COEFFL(pli,Z) += 
               integral(tGrid,pel,nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                                  nq[j],nq0[j],nq1[j],nq00[j],nq01[j],nq11[j],
                                  a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                                  f6,f7,f8,f9,finite_el,ref_map,qr);
         }
      COEFF_NE(pel,Z,i) += integral(tGrid,pel,nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                               eq[0],eq0[0],eq1[0],eq00[0],eq01[0],eq11[0],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
      COEFF_EN(pel,Z,i) += integral(tGrid,pel,eq[0],eq0[0],eq1[0],eq00[0],eq01[0],eq11[0],
                               nq[i],nq0[i],nq1[i],nq00[i],nq01[i],nq11[i],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
   }
   COEFF_EE(pel,Z) += integral(tGrid,pel,eq[0],eq0[0],eq1[0],eq00[0],eq01[0],eq11[0],
                               eq[0],eq0[0],eq1[0],eq00[0],eq01[0],eq11[0],
                               a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,
                               f6,f7,f8,f9,finite_el,ref_map,qr);
   }
}

#else

void p1cb_stiff_matr(tGrid,Z,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,integral,finite_el,rmtype,qr)
GRID *tGrid; INT Z, rmtype, i0, i1, i2; FLOAT a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)(), (*integral)(); FINITE_ELEMENT finite_el; QUADRATURE_RULE qr;
{  eprintf("Error: p1cb_stiff_matr not available.\n");  }

#endif

/******************************************************************************/
/*                                                                            */
/*            P1nc stiffness matrices for convection-diffusion eq.            */
/*                                                                            */
/******************************************************************************/

#if (DATA_S & F_LINK_TO_FACES) && (F_DATA & ONE_FACE_MATR)

void nc_rijb(n0,n1,n2,fa0,fa1,fa2,Z,r,rdetB)
NODE *n0, *n1, *n2;                  /*  reactive term; exact for pw. lin. r  */
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT (*r)(), rdetB;
{
   FLOAT r0, r1, r2, ann, an1, an2;
  
      S_NODE_VALUES(n0,n1,n2,r,r0,r1,r2)
      ann = (r0 + 2.*r1 + 2.*r2)/15.*rdetB;
      an1 = (2.*r2 - r0 - r1)/30.*rdetB;
      an2 = (2.*r1 - r0 - r2)/30.*rdetB;
      COEFF_FF(fa0,Z) += ann;
      putppij(fa0->tfstart,fa1,fa2,an1,an2,Z);
}

void nc_cijb(n0,n1,n2,fa0,fa1,fa2,Z,bb0,bb1,div_bb,b0,b1,b2,rdetB,type)
NODE *n0, *n1, *n2;                /*  convective term; exact for pw. lin. b  */
FACE *fa0, *fa1, *fa2;
INT Z, type;
FLOAT (*bb0)(), (*bb1)(), (*div_bb)(), b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], divb0, divb1, divb2,
         b01[DIM], b02[DIM], b12[DIM], ann, an1, an2;
  
      V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
      SET10(b01,bb_0,bb_1)
      SET10(b02,bb_0,bb_2)
      SET10(b12,bb_1,bb_2)

      if (type == CONV){
         ann = ( -DOT(b0,b12)  )*rdetB/3.;
         an1 = ( -DOT(b1,b12)  )*rdetB/3.;
         an2 = ( -DOT(b2,b12)  )*rdetB/3.;
      }
      else if (type == SKEW){
         S_NODE_VALUES(n0,n1,n2,div_bb,divb0,divb1,divb2)
         ann = -(divb0 + 2.*divb1 + 2.*divb2)/30.*rdetB;
         an1 = ( -DOT(b1,b12) + DOT(b0,b02) - 
                     (2.*divb2 - divb0 - divb1)/10. )*rdetB/6.;
         an2 = ( -DOT(b2,b12) + DOT(b0,b01) - 
                     (2.*divb1 - divb0 - divb2)/10. )*rdetB/6.;
      }
      else
         eprintf("Error: wrong type in nc_cijb.\n");

      COEFF_FF(fa0,Z) += ann;
      putppij(fa0->tfstart,fa1,fa2,an1,an2,Z);
}

#else

void nc_rijb(n0,n1,n2,fa0,fa1,fa2,Z,r,rdetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z; FLOAT (*r)(), rdetB;
{  eprintf("Error: nc_rijb not available.\n");  }

void nc_cijb(n0,n1,n2,fa0,fa1,fa2,Z,bb0,bb1,div_bb,b0,b1,b2,rdetB,type)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z, type; FLOAT (*bb0)(), (*bb1)(), (*div_bb)(), b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{  eprintf("Error: nc_cijb not available.\n");  }

#endif

#define MULT_LIN(r,s)    ( ( 2.*(r[0]*s[0] + r[1]*s[1] + r[2]*s[2]) +          \
    r[0]*s[1] + r[0]*s[2] + r[1]*s[0] + r[1]*s[2] +r[2]*s[0] + r[2]*s[1] )/12. )

#if (DATA_S & F_LINK_TO_FACES) && (F_DATA & ONE_FACE_MATR)

void nc_sd_ijb(n0,n1,n2,fa0,fa1,fa2,Z,bb0,bb1,react,b0,b1,b2,detB)
NODE *n0, *n1, *n2;      /*  streamline-diffusion term; exact for pw. lin. b, */
FACE *fa0, *fa1, *fa2;   /*  and pw. const. react                             */
INT Z;                   /*  detB = volume*delta                              */
FLOAT (*bb0)(), (*bb1)(), (*react)(), b0[DIM2], b1[DIM2], b2[DIM2], 
                                                                           detB;
{
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], s[DIM2][DIM2], t[DIM2], r0, r1, r2, c;
  
      V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
      S_NODE_VALUES(n0,n1,n2,react,r0,r1,r2)
      t[0] = DOT(b0,bb_0);
      t[1] = DOT(b0,bb_1);
      t[2] = DOT(b0,bb_2);
      s[0][0] = 2.*t[0] + r0;
      s[0][1] = 2.*t[1] - r1;
      s[0][2] = 2.*t[2] - r2;
      s[1][0] = 2.*DOT(b1,bb_0) - r0;
      s[1][1] = 2.*DOT(b1,bb_1) + r1;
      s[1][2] = 2.*DOT(b1,bb_2) - r2;
      s[2][0] = 2.*DOT(b2,bb_0) - r0;
      s[2][1] = 2.*DOT(b2,bb_1) - r1;
      s[2][2] = 2.*DOT(b2,bb_2) + r2;
      c = 2.*detB;
      COEFF_FF(fa0,Z) += c*MULT_LIN(s[0],t);
      putppij(fa0->tfstart,fa1,fa2, c*MULT_LIN(s[1],t), c*MULT_LIN(s[2],t), Z);
}

#else

void nc_sd_ijb(n0,n1,n2,fa0,fa1,fa2,Z,bb0,bb1,react,b0,b1,b2,detB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z; FLOAT (*bb0)(), (*bb1)(), (*react)(), b0[DIM2], b1[DIM2], b2[DIM2], detB;
{  eprintf("Error: nc_sd_ijb not available.\n");  }

#endif

void nc_react_term_to_stiff_matr(tGrid,Z,r)
GRID *tGrid;
INT Z;
FLOAT (*r)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
     NODES_OF_ELEMENT(n0,n1,n2,pelem);
     FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
     rdetB = VOLUME(pelem);
     nc_rijb(n0,n1,n2,fa0,fa1,fa2,Z,r,rdetB);
     nc_rijb(n1,n2,n0,fa1,fa2,fa0,Z,r,rdetB);
     nc_rijb(n2,n0,n1,fa2,fa0,fa1,Z,r,rdetB);
   } 
}

void nc_conv_term_to_stiff_matr(tGrid,Z,bb0,bb1,div_bb,type)
GRID *tGrid;
INT Z, type;
FLOAT (*bb0)(), (*bb1)(), (*div_bb)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      nc_cijb(n0,n1,n2,fa0,fa1,fa2,Z,bb0,bb1,div_bb,b[0],b[1],b[2],rdetB,type);
      nc_cijb(n1,n2,n0,fa1,fa2,fa0,Z,bb0,bb1,div_bb,b[1],b[2],b[0],rdetB,type);
      nc_cijb(n2,n0,n1,fa2,fa0,fa1,Z,bb0,bb1,div_bb,b[2],b[0],b[1],rdetB,type);
   } 
}

void nc_sd_term_to_stiff_matr(tGrid,nu,Z,bb0,bb1,react)
GRID *tGrid;
INT Z;
FLOAT nu, (*bb0)(), (*bb1)(), (*react)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2], delta;
   INT n=0;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      delta = sd_delta(pelem,nu,bb0,bb1);
      if (delta > 0.){
         n++;
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
         rdetB = delta*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                             n2->myvertex->x,b);
         nc_sd_ijb(n0,n1,n2,fa0,fa1,fa2,Z,bb0,bb1,react,b[0],b[1],b[2],rdetB);
         nc_sd_ijb(n1,n2,n0,fa1,fa2,fa0,Z,bb0,bb1,react,b[1],b[2],b[0],rdetB);
         nc_sd_ijb(n2,n0,n1,fa2,fa0,fa1,Z,bb0,bb1,react,b[2],b[0],b[1],rdetB);
      }
   } 
   printf("SDFEM on %i elements.\n",n);
}

#if (F_DATA & SCALAR_FACE_DATA) && (F_DATA & ONE_FACE_MATR)

void add_P1_NC_MH_conv_term_matr(tGrid,Z,u,f,bb0,bb1,react,rhs)
GRID *tGrid;                  /*  Mizukami, Hughes, CMAME 50 (1985), 181-193  */
INT Z, u, f;
FLOAT (*bb0)(), (*bb1)(), (*react)(), (*rhs)();
{
   ELEMENT *pelem;
   FACE *fa0, *fa1, *fa2;
   FLOAT bb_xc[DIM], r_xc, xc[DIM], b[DIM2][DIM2], x01[DIM], x02[DIM], x12[DIM],
         *x0, *x1, *x2, c0, c1, c2, s0, s1, s2, r0, r1, r2, h0, h1, h2, s, z, 
         ndetB;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      ndetB = barycentric_coordinates(x0,x1,x2,b)/3.;
      POINT3(x0,x1,x2,xc);
      bb_xc[0] = bb0(xc);
      bb_xc[1] = bb1(xc);
      r_xc     = react(xc);
      s0 = -2.*DOT(b[0],bb_xc);
      s1 = -2.*DOT(b[1],bb_xc);
      s2 = -2.*DOT(b[2],bb_xc);
      h0 = sqrt(edge_length_square(pelem->n[1],pelem->n[2]));
      h1 = sqrt(edge_length_square(pelem->n[2],pelem->n[0]));
      h2 = sqrt(edge_length_square(pelem->n[0],pelem->n[1]));
      if (fabs(bb_xc[0]) < EPSC && fabs(bb_xc[1]) < EPSC)
         c0 = c1 = c2 = 0.;
      else
         compute_upwind_const(pelem,xc,bb_xc,r_xc,b,u,s0,s1,s2,&c0,&c1,&c2,
                              P1_NC,mh_treat_edge_zone);
      s0 *= ndetB;
      s1 *= ndetB;
      s2 *= ndetB;
      r0 = (*react)(x0);
      r1 = (*react)(x1);
      r2 = (*react)(x2);
      s = ndetB/6.;
      r0 = (r1 + r2)*s;
      r1 = (r0 + r2)*s;
      r2 = (r0 + r1)*s;
      COEFF_FF(fa0,Z) += (1.+c0)*s0 + c0*r0;
      COEFF_FF(fa1,Z) += (1.+c1)*s1 + c1*r1;
      COEFF_FF(fa2,Z) += (1.+c2)*s2 + c2*r2;
      putppij(fa0->tfstart,fa1,fa2,(1.+c0)*s1 + c0*r1,(1.+c0)*s2 + c0*r2,Z);
      putppij(fa1->tfstart,fa2,fa0,(1.+c1)*s2 + c1*r2,(1.+c1)*s0 + c1*r0,Z);
      putppij(fa2->tfstart,fa0,fa1,(1.+c2)*s0 + c2*r0,(1.+c2)*s1 + c2*r1,Z);
      AVERAGE(x0,x1,x01);
      AVERAGE(x0,x2,x02);
      AVERAGE(x1,x2,x12);
      if (RHS_INTEGR == ZERO)
         s = (*rhs)(xc)*ndetB;
      else
         s = ((*rhs)(x01) + (*rhs)(x02) + (*rhs)(x12))*ndetB/3.;
      FD(fa0,f) += c0*s;
      FD(fa1,f) += c1*s;
      FD(fa2,f) += c2*s;
   }
}

#else

void add_P1_NC_MH_conv_term_matr(tGrid,Z,u,f,bb0,bb1,react,rhs)
GRID *tGrid; INT Z, u, f; FLOAT (*bb0)(), (*bb1)(), (*react)(), (*rhs)();
{  eprintf("Error: add_P1_NC_MH_conv_term_matr not available.\n");  }

#endif

/******************************************************************************/
/*                                                                            */
/*           P1mod stiffness matrices for convection-diffusion eq.            */
/*                                                                            */
/******************************************************************************/

#define TRANSF2(a1,a2,a3,a4,a5,a6,d1,d2,d3,d4,d5,d6)                           \
         ((a1)*(d1) + (a2)*(d2) + (a3)*(d3) + (a4)*(d4) + (a5)*(d5) + (a6)*(d6))

#define TRANSF3(a1,a2,a3,d1,d2,d3)           ((a1)*(d1) + (a2)*(d2) + (a3)*(d3))

#define TRANSF4(e0000,e0001,e0010,e0011, e0100,e0101,e0110,e0111,              \
                e0200,e0201,e0210,e0211, e1100,e1101,e1110,e1111,              \
                e1200,e1201,e1210,e1211, e2200,e2201,e2210,e2211,              \
                d01,d02,d11,d12,d21,d22)                                       \
((e0000)*(d01)*(d01) + ((e0001) + (e0010))*(d01)*(d02) + (e0011)*(d02)*(d02) + \
 2.*(e0100)*(d01)*(d11) + ((e0101) + (e0110))*((d01)*(d12) + (d11)*(d02)) +    \
 2.*(e0111)*(d02)*(d12) + 2.*(e0200)*(d01)*(d21) +                             \
 ((e0201) + (e0210))*((d01)*(d22) + (d21)*(d02)) + 2.*(e0211)*(d02)*(d22) +    \
 (e1100)*(d11)*(d11) + ((e1101) + (e1110))*(d11)*(d12) + (e1111)*(d12)*(d12) + \
 2.*(e1200)*(d11)*(d21) + ((e1201) + (e1210))*((d11)*(d22) + (d21)*(d12)) +    \
 2.*(e1211)*(d12)*(d22) + (e2200)*(d21)*(d21) +                                \
 ((e2201) + (e2210))*(d21)*(d22) + (e2211)*(d22)*(d22))

#define TRANSF5(e000,e001,e010,e011,e020,e021,e110,e111,e120,e121,e220,e221,   \
                c0,c1,c2,d01,d02,d11,d12,d21,d22)                              \
((e000)*(c0)*(d01) + (e001)*(c0)*(d02) + (e010)*((c0)*(d11) + (c1)*(d01)) +    \
 (e011)*((c0)*(d12) + (c1)*(d02)) + (e020)*((c0)*(d21) + (c2)*(d01)) +         \
 (e021)*((c0)*(d22) + (c2)*(d02)) + (e110)*(c1)*(d11) + (e111)*(c1)*(d12) +    \
 (e120)*((c1)*(d21) + (c2)*(d11)) + (e121)*((c1)*(d22) + (c2)*(d12)) +         \
 (e220)*(c2)*(d21) + (e221)*(c2)*(d22))

#define TRANSF6(e0000,e0001,e0010,e0011,e0110,e0111,                           \
                e1000,e1001,e1010,e1011,e1110,e1111,                           \
                e2000,e2001,e2010,e2011,e2110,e2111,                           \
                a11,a12,a22,d01,d02,d11,d12,d21,d22)                           \
((e0000)*(a11)*(d01) + (e0001)*(a11)*(d02) + 2.*(e0010)*(a12)*(d01) +          \
 2.*(e0011)*(a12)*(d02) + (e0110)*(a22)*(d01) + (e0111)*(a22)*(d02) +          \
 (e1000)*(a11)*(d11) + (e1001)*(a11)*(d12) + 2.*(e1010)*(a12)*(d11) +          \
 2.*(e1011)*(a12)*(d12) + (e1110)*(a22)*(d11) + (e1111)*(a22)*(d12) +          \
 (e2000)*(a11)*(d21) + (e2001)*(a11)*(d22) + 2.*(e2010)*(a12)*(d21) +          \
 2.*(e2011)*(a12)*(d22) + (e2110)*(a22)*(d21) + (e2111)*(a22)*(d22))

FLOAT wl2_n_fi_fi(r1,r2,r3)  /*  \int_T r fi_12 fi_23 dx/|T|                  */
FLOAT r1, r2, r3;            /*  values of r in the vertices                  */
{
   return( (7.*r1 + 6.*r2 + 7.*r3)/315. );
}

FLOAT wl2_d_fi_fi(r1,r2,r3)  /*  \int_T r fi_12 fi_12 dx/|T|                  */
FLOAT r1, r2, r3;            /*  values of r in the vertices                  */
{
   return( (22.*r1 + 22.*r2 + 21.*r3)/315. );
}

FLOAT wl2_n_fi_chi(r1,r2,r3,n1,n2,n3)  /*  \int_T r fi_12 chi_23 dx/|T|       */
FLOAT r1, r2, r3;                      /*  values of r in the vertices        */
NODE *n1, *n2, *n3;
{
   return( -(r1 + r2 + 10.*r3)/2520.*NINDI(n3,n2) );
}

FLOAT wl2_d_fi_chi(r1,r2,r3,n1,n2,n3)  /*  \int_T r fi_12 chi_12 dx/|T|       */
FLOAT r1, r2, r3;                      /*  values of r in the vertices        */
NODE *n1, *n2, *n3;
{
   return( (r1 - r2)/504.*NINDI(n2,n1) );
}

FLOAT wl2_n_chi_chi(r1,r2,r3,n1,n2,n3)  /*  \int_T r chi_12 chi_23 dx/|T|     */
FLOAT r1, r2, r3;                       /*  values of r in the vertices       */
NODE *n1, *n2, *n3;
{
   return( -(r1 + 4.*r2 + r3)/15120.*NINDI(n2,n1)*NINDI(n3,n2) );
}

FLOAT wl2_d_chi_chi(r1,r2,r3)  /*  \int_T r chi_12 chi_12 dx/|T|              */
FLOAT r1, r2, r3;              /*  values of r in the vertices                */
{
   return( (4.*r1 + 4.*r2 + r3)/7560. );
}

void nc_rijb_mbub(n0,n1,n2,fa0,fa1,fa2,Z,r,rdetB)
NODE *n0, *n1, *n2;                  /*  reactive term; exact for pw. lin. r  */
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT (*r)(), rdetB;
{
   FLOAT d0, d1, d2, r0, r1, r2, ann, an1, an2, bnn, bn1, bn2, cnn, cn1, cn2, dnn, dn1, dn2;
  
   S_NODE_VALUES(n0,n1,n2,r,r0,r1,r2)

   d0 = r0*rdetB;
   d1 = r1*rdetB;
   d2 = r2*rdetB;

   /* r fi_j fi_i */
   ann = TRANSF3(21,22,22,d0,d1,d2)/315.;
   an1 = TRANSF3(7,7,6,d0,d1,d2)/315.;
   an2 = TRANSF3(7,6,7,d0,d1,d2)/315.;

   /* r chi_j fi_i */
   bnn = TRANSF3(0,5,-5,d0,d1,d2)/2520.*NINDI(n2,n1);
   bn1 = TRANSF3(10,1,1,d0,d1,d2)/2520.*NINDI(n2,n0);
   bn2 = TRANSF3(10,1,1,d0,d1,d2)/2520.*NINDI(n1,n0);

   /* r fi_j chi_i */
   cnn = bnn;
   cn1 = TRANSF3(1,10,1,d0,d1,d2)/2520.*NINDI(n2,n1);
   cn2 = TRANSF3(-1,-1,-10,d0,d1,d2)/2520.*NINDI(n2,n1);
  
   /* r chi_j chi_i */
   dnn = TRANSF3(1,4,4,d0,d1,d2)/7560.;
   dn1 = TRANSF3(1,1,4,d0,d1,d2)/15120.*NINDI(n2,n1)*NINDI(n2,n0);
   dn2 = TRANSF3(-1,-4,-1,d0,d1,d2)/15120.*NINDI(n2,n1)*NINDI(n1,n0);

/* ann = wl2_d_fi_fi(r1,r2,r0)*rdetB;
   an1 = wl2_n_fi_fi(r1,r2,r0)*rdetB;
   an2 = wl2_n_fi_fi(r2,r1,r0)*rdetB;
   bnn = wl2_d_fi_chi(r1,r2,r0,n1,n2,n0)*rdetB;
   bn1 = wl2_n_fi_chi(r1,r2,r0,n1,n2,n0)*rdetB;
   bn2 = wl2_n_fi_chi(r2,r1,r0,n2,n1,n0)*rdetB;
   cnn = bnn;
   cn1 = wl2_n_fi_chi(r0,r2,r1,n0,n2,n1)*rdetB;
   cn2 = wl2_n_fi_chi(r0,r1,r2,n0,n1,n2)*rdetB;
   dnn = wl2_d_chi_chi(r1,r2,r0)*rdetB;
   dn1 = wl2_n_chi_chi(r1,r2,r0,n1,n2,n0)*rdetB;
   dn2 = wl2_n_chi_chi(r2,r1,r0,n2,n1,n0)*rdetB;
*/
    vadd_to_matrix_f(fa0,fa1,fa2,Z,
                               ann,an1,an2,bnn,bn1,bn2,cnn,cn1,cn2,dnn,dn1,dn2);
}

FLOAT conv_n_fi_fi(bb_1,bb_2,bb_3,b1,b2,b3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM];  /* \int_T bb*grad(fi_12) fi_23 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];     /*  barycentric coordinates           */
{
   return( -( DOT(b3,bb_2) + DOT(b3,bb_3) )/3. + 
     ( DOT(b3,bb_2) + DOT(b3,bb_3) - 2.*DOT(b3,bb_1) )/9. +
     ( DOT(b1,bb_1) + DOT(b1,bb_2) + 4.*DOT(b1,bb_3) + 
       2.*DOT(b2,bb_3) - 2.*DOT(b2,bb_2) + 
       3.*DOT(b3,bb_1) - DOT(b3,bb_2) + 4.*DOT(b3,bb_3) )/9. -
     ( 7.*DOT(b1,bb_1) + 2.*DOT(b1,bb_2) + 7.*DOT(b1,bb_3) +
       DOT(b2,bb_1) - 2.*DOT(b2,bb_2) + 5.*DOT(b2,bb_3) +
       11.*DOT(b3,bb_1) - 2.*DOT(b3,bb_2) + 7.*DOT(b3,bb_3) )*10./252. );
}

FLOAT conv_d_fi_fi(bb_1,bb_2,bb_3,b1,b2,b3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM];  /* \int_T bb*grad(fi_12) fi_12 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];     /*  barycentric coordinates           */
{
   return( -( DOT(b3,bb_1) + DOT(b3,bb_2) )/3. - 
     ( 2.*DOT(b1,bb_1) + DOT(b1,bb_2) + 3.*DOT(b1,bb_3) + 
       DOT(b2,bb_1) + 2.*DOT(b2,bb_2) + 3.*DOT(b2,bb_3) +
       4.*(DOT(b3,bb_1) + DOT(b3,bb_2) + DOT(b3,bb_3)) )/9. +
     ( 5.*DOT(b1,bb_1) + 3.*DOT(b1,bb_2) + 12.*DOT(b1,bb_3) +
       3.*DOT(b2,bb_1) + 5.*DOT(b2,bb_2) + 12.*DOT(b2,bb_3) +
       9.*DOT(b3,bb_1) + 9.*DOT(b3,bb_2) + 14.*DOT(b3,bb_3) )*10./252. );
}

FLOAT conv_n_fi_chi(bb_1,bb_2,bb_3,b1,b2,b3,n1,n2,n3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM]; /* \int_T bb*grad(fi_12) chi_23 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];    /*  barycentric coordinates            */
NODE *n1, *n2, *n3;
{
   return( ( ( DOT(b3,bb_3) - DOT(b3,bb_2) )/90. - 
     ( DOT(b1,bb_1) + 3.*DOT(b1,bb_2) + 12.*DOT(b1,bb_3) + 
       3.*DOT(b2,bb_1) + 9.*DOT(b2,bb_2) + 12.*DOT(b2,bb_3) +
       5.*DOT(b3,bb_1) + 13.*DOT(b3,bb_2) + 14.*DOT(b3,bb_3) )/504. )*
       NINDI(n3,n2) );
}

FLOAT conv_d_fi_chi(bb_1,bb_2,bb_3,b1,b2,b3,n1,n2,n3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM]; /* \int_T bb*grad(fi_12) chi_12 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];    /*  barycentric coordinates            */
NODE *n1, *n2, *n3;
{
   return( ( ( DOT(b3,bb_2) - DOT(b3,bb_1) )/90. - 
     ( 5.*DOT(b1,bb_1) + DOT(b1,bb_2) + 2.*DOT(b1,bb_3) - 
       DOT(b2,bb_1) - 5.*DOT(b2,bb_2) - 2.*DOT(b2,bb_3) +
       9.*DOT(b3,bb_1) - 9.*DOT(b3,bb_2) )/504. )*NINDI(n2,n1) );
}

FLOAT conv_n_chi_fi(bb_1,bb_2,bb_3,b1,b2,b3,n1,n2,n3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM]; /* \int_T bb*grad(chi_12) fi_23 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];    /*  barycentric coordinates            */
NODE *n1, *n2, *n3;
{
   return( ( -( DOT(b1,bb_1) + 4.*DOT(b1,bb_2) + DOT(b1,bb_3) + 
       3.*DOT(b2,bb_1) + 2.*DOT(b2,bb_2) + DOT(b2,bb_3) )/90. +
     ( 7.*DOT(b1,bb_1) + 7.*DOT(b1,bb_2) + 2.*DOT(b1,bb_3) + 
       12.*DOT(b2,bb_1) + 5.*DOT(b2,bb_2) + 3.*DOT(b2,bb_3) )/252. )*
       NINDI(n2,n1) );
}

FLOAT conv_d_chi_fi(bb_1,bb_2,bb_3,b1,b2,b3,n1,n2,n3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM]; /* \int_T bb*grad(chi_12) fi_12 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];    /*  barycentric coordinates            */
NODE *n1, *n2, *n3;
{
   return( ( ( DOT(b3,bb_2) - DOT(b3,bb_1) )/45. + 
     ( -2.*DOT(b1,bb_1) + 5.*DOT(b1,bb_2) + DOT(b1,bb_3) - 
       5.*DOT(b2,bb_1) + 2.*DOT(b2,bb_2) - DOT(b2,bb_3) )/252. )*NINDI(n2,n1) );
}

FLOAT conv_n_chi_chi(bb_1,bb_2,bb_3,b1,b2,b3,n1,n2,n3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM];/* \int_T bb*grad(chi_12) chi_23 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];   /*  barycentric coordinates             */
NODE *n1, *n2, *n3;
{
   return( -( DOT(b1,bb_1) + 12.*DOT(b1,bb_2) + 3.*DOT(b1,bb_3) +
           2.*DOT(b2,bb_1) +  5.*DOT(b2,bb_2) +    DOT(b2,bb_3) )/5040.*
             NINDI(n2,n1)*NINDI(n3,n2) );
}

FLOAT conv_d_chi_chi(bb_1,bb_2,bb_3,b1,b2,b3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM];/* \int_T bb*grad(chi_12) chi_12 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];   /*  barycentric coordinates             */
{
   return( ( 9.*DOT(b1,bb_1) + 12.*DOT(b1,bb_2) + 3.*DOT(b1,bb_3) +
             12.*DOT(b2,bb_1) + 9.*DOT(b2,bb_2) + 3.*DOT(b2,bb_3) )/5040. );
}

FLOAT conv_n_z_chi(bb_1,bb_2,bb_3,b1,b2,b3,n1,n2,n3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM];/* \int_T bb*grad(z_12) chi_23 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];   /*  barycentric coordinates */
NODE *n1, *n2, *n3;
{
   return( (DOT(b3,bb_3) - DOT(b3,bb_2))/90.*NINDI(n3,n2) );
}

FLOAT conv_d_z_chi(bb_1,bb_2,bb_3,b1,b2,b3,n1,n2,n3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM];/* \int_T bb*grad(z_12) chi_12 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];   /*  barycentric coordinates */
NODE *n1, *n2, *n3;
{
   return( (DOT(b3,bb_2) - DOT(b3,bb_1))/90.*NINDI(n2,n1) );
}

FLOAT conv_n_chi_z(bb_1,bb_2,bb_3,b1,b2,b3,n1,n2,n3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM];/* \int_T bb*grad(chi_12) z_23 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];   /*  barycentric coordinates */
NODE *n1, *n2, *n3;
{
   return( -( DOT(b1,bb_1) + 4.*DOT(b1,bb_2) + DOT(b1,bb_3) +
           3.*DOT(b2,bb_1) + 2.*DOT(b2,bb_2) + DOT(b2,bb_3) )/90.*NINDI(n2,n1) );
}

FLOAT conv_d_chi_z(bb_1,bb_2,bb_3,b1,b2,b3,n1,n2,n3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM];/* \int_T bb*grad(chi_12) z_12 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];   /*  barycentric coordinates */
NODE *n1, *n2, *n3;
{
   return( (DOT(b3,bb_2) - DOT(b3,bb_1))/45.*NINDI(n2,n1) );
}

FLOAT conv_n_z_z(bb_1,bb_2,bb_3,b1,b2,b3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM];/* \int_T bb*grad(z_12) z_23 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];   /*  barycentric coordinates */
{
   return( -(DOT(b3,bb_2) + DOT(b3,bb_3))/3. );
}

FLOAT conv_d_z_z(bb_1,bb_2,bb_3,b1,b2,b3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM];/* \int_T bb*grad(z_12) z_12 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];   /*  barycentric coordinates */
{
   return( -(DOT(b3,bb_1) + DOT(b3,bb_2))/3. );
}

FLOAT cconv_d_fi_fi(bb_1,bb_2,bb_3,b1,b2,b3,n1,n2,n3)
FLOAT bb_1[DIM], bb_2[DIM], bb_3[DIM];  /* \int_T bb*grad(fi_12) fi_12 dx/|T| */
FLOAT b1[DIM2], b2[DIM2], b3[DIM2];     /*  barycentric coordinates           */
NODE *n1, *n2, *n3;
{
   return( conv_d_z_z(bb_1,bb_2,bb_3,b1,b2,b3) +
         10.*( conv_n_z_chi(bb_2,bb_1,bb_3,b2,b1,b3,n2,n1,n3) +
               conv_n_chi_z(bb_3,bb_1,bb_2,b3,b1,b2,n3,n1,n2) )*NINDI(n1,n3) +
         10.*( conv_n_z_chi(bb_1,bb_2,bb_3,b1,b2,b3,n1,n2,n3) +
               conv_n_chi_z(bb_3,bb_2,bb_1,b3,b2,b1,n3,n2,n1) )*NINDI(n2,n3) +
        100.*( conv_d_chi_chi(bb_1,bb_3,bb_2,b1,b3,b2) +
               conv_d_chi_chi(bb_2,bb_3,bb_1,b2,b3,b1) ) +
        100.*( conv_n_chi_chi(bb_1,bb_3,bb_2,b1,b3,b2,n1,n3,n2) +
               conv_n_chi_chi(bb_2,bb_3,bb_1,b2,b3,b1,n2,n3,n1) )*
               NINDI(n1,n3)*NINDI(n2,n3) );
}

void nc_convective_form_mbub(bb_0,bb_1,bb_2,b0,b1,b2,n0,n1,n2,rdetB,
                             ann,an1,an2,bnn,bn1,bn2,cnn,cn1,cn2,dnn,dn1,dn2)
FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], rdetB;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2];     /*  barycentric coordinates           */
NODE *n0, *n1, *n2;
FLOAT *ann,*an1,*an2,*bnn,*bn1,*bn2,*cnn,*cn1,*cn2,*dnn,*dn1,*dn2;
{
   FLOAT d01, d02, d11, d12, d21, d22;

   d01 = DOT(bb_0,b1)*rdetB;
   d02 = DOT(bb_0,b2)*rdetB;
   d11 = DOT(bb_1,b1)*rdetB;
   d12 = DOT(bb_1,b2)*rdetB;
   d21 = DOT(bb_2,b1)*rdetB;
   d22 = DOT(bb_2,b2)*rdetB;

   /* bb * (grad fi_j) fi_i */
   *ann = TRANSF2(2,2,25,27,27,25,d01,d02,d11,d12,d21,d22)/63.; 
   *an1 = TRANSF2(-10,8,-14,-9,-18,-11,d01,d02,d11,d12,d21,d22)/63.; 
   *an2 = TRANSF2(8,-10,-11,-18,-9,-14,d01,d02,d11,d12,d21,d22)/63.;
  
   /* bb * (grad chi_j) fi_i */
   *bnn = TRANSF2(5,-5,18,3,-3,-18,d01,d02,d11,d12,d21,d22)/1260.*NINDI(n2,n1);
   *bn1 = TRANSF2(-21,-3,4,5,21,18,d01,d02,d11,d12,d21,d22)/1260.*NINDI(n2,n0);
   *bn2 = TRANSF2(-3,-21,18,21,5,4,d01,d02,d11,d12,d21,d22)/1260.*NINDI(n1,n0);
    
   /* bb * (grad fi_j) chi_i */
   *cnn = TRANSF2(-5,5,24,39,-39,-24,d01,d02,d11,d12,d21,d22)/1260.*NINDI(n2,n1);
   *cn1 = TRANSF2(10,5,-9,0,39,15,d01,d02,d11,d12,d21,d22)/1260.*NINDI(n2,n1);
   *cn2 = TRANSF2(-5,-10,-15,-39,0,9,d01,d02,d11,d12,d21,d22)/1260.*NINDI(n2,n1);
  
   /* bb * (grad chi_j) chi_i */
   *dnn = TRANSF2(1,1,3,4,4,3,d01,d02,d11,d12,d21,d22)/1680.;
   *dn1 = TRANSF2(-1,1,-3,-2,-12,-7,d01,d02,d11,d12,d21,d22)/5040.*NINDI(n2,n1)*
          NINDI(n2,n0);
   *dn2 = TRANSF2(-1,1,7,12,2,3,d01,d02,d11,d12,d21,d22)/5040.*NINDI(n2,n1)*
          NINDI(n1,n0);

/* *ann = cconv_d_fi_fi(bb_1,bb_2,bb_0,b1,b2,b0,n1,n2,n0)*rdetB; 
   *ann = conv_d_fi_fi(bb_1,bb_2,bb_0,b1,b2,b0)*rdetB;
   *an1 = conv_n_fi_fi(bb_0,bb_2,bb_1,b0,b2,b1)*rdetB;
   *an2 = conv_n_fi_fi(bb_0,bb_1,bb_2,b0,b1,b2)*rdetB;
   *bnn = conv_d_chi_fi(bb_1,bb_2,bb_0,b1,b2,b0,n1,n2,n0)*rdetB;
   *bn1 = conv_n_chi_fi(bb_0,bb_2,bb_1,b0,b2,b1,n0,n2,n1)*rdetB;
   *bn2 = conv_n_chi_fi(bb_0,bb_1,bb_2,b0,b1,b2,n0,n1,n2)*rdetB;
   *cnn = conv_d_fi_chi(bb_1,bb_2,bb_0,b1,b2,b0,n1,n2,n0)*rdetB;
   *cn1 = conv_n_fi_chi(bb_0,bb_2,bb_1,b0,b2,b1,n0,n2,n1)*rdetB;
   *cn2 = conv_n_fi_chi(bb_0,bb_1,bb_2,b0,b1,b2,n0,n1,n2)*rdetB;
   *dnn = conv_d_chi_chi(bb_1,bb_2,bb_0,b1,b2,b0)*rdetB;
   *dn1 = conv_n_chi_chi(bb_0,bb_2,bb_1,b0,b2,b1,n0,n2,n1)*rdetB;
   *dn2 = conv_n_chi_chi(bb_0,bb_1,bb_2,b0,b1,b2,n0,n1,n2)*rdetB;
*/
} 

void nc_skew_symm_form_mbub(bb_0,bb_1,bb_2,divb0,divb1,divb2,b0,b1,b2,
                 n0,n1,n2,rdetB,ann,an1,an2,bnn,bn1,bn2,cnn,cn1,cn2,dnn,dn1,dn2)
FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], divb0, divb1, divb2, rdetB;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2];     /*  barycentric coordinates           */
NODE *n0, *n1, *n2;
FLOAT *ann,*an1,*an2,*bnn,*bn1,*bn2,*cnn,*cn1,*cn2,*dnn,*dn1,*dn2;
{
   FLOAT d0, d1, d2, d01, d02, d11, d12, d21, d22, q1, q2;

   d0 = divb0*rdetB;
   d1 = divb1*rdetB;
   d2 = divb2*rdetB;
   d01 = DOT(bb_0,b1)*rdetB;
   d02 = DOT(bb_0,b2)*rdetB;
   d11 = DOT(bb_1,b1)*rdetB;
   d12 = DOT(bb_1,b2)*rdetB;
   d21 = DOT(bb_2,b1)*rdetB;
   d22 = DOT(bb_2,b2)*rdetB;

   /* bb * (grad fi_j) fi_i */
   *ann = -TRANSF3(21,22,22,d0,d1,d2)/630.;
   *an1 =  TRANSF2(-8,1,-8,-9,-12,-6,d01,d02,d11,d12,d21,d22)/42.
          -TRANSF3(7,7,6,d0,d1,d2)/630.;
   *an2 =  TRANSF2(1,-8,-6,-12,-9,-8,d01,d02,d11,d12,d21,d22)/42.
          -TRANSF3(7,6,7,d0,d1,d2)/630.;

   /* bb * (grad chi_j) fi_i */
   q1   = TRANSF2(5,-5,-3,-18,18,3,d01,d02,d11,d12,d21,d22)/1260.*NINDI(n2,n1);
   q2   = TRANSF3(0,5,-5,d0,d1,d2)/5040.*NINDI(n2,n1);
   *bnn = q1 - q2;
   *bn1 = (TRANSF2(-15,-6,7,5,30,21,d01,d02,d11,d12,d21,d22)/1260. -
           TRANSF3(10,1,1,d0,d1,d2)/5040.)*NINDI(n2,n0);
   *bn2 = (TRANSF2(-6,-15,21,30,5,7,d01,d02,d11,d12,d21,d22)/1260. -
           TRANSF3(10,1,1,d0,d1,d2)/5040.)*NINDI(n1,n0);

   /* bb * (grad fi_j) chi_i */
   *cnn = -q1 - q2;
   *cn1 = (TRANSF2(7,2,-15,-9,30,9,d01,d02,d11,d12,d21,d22)/1260.-
           TRANSF3(1,10,1,d0,d1,d2)/5040.)*NINDI(n2,n1);
   *cn2 = (TRANSF2(-2,-7,-9,-30,9,15,d01,d02,d11,d12,d21,d22)/1260.-
           TRANSF3(-1,-1,-10,d0,d1,d2)/5040.)*NINDI(n2,n1);
  
   /* bb * (grad chi_j) chi_i */
   *dnn = -TRANSF3(1,4,4,d0,d1,d2)/15120.;
   *dn1 = (TRANSF2(-1,0,-1,-1,-6,-3,d01,d02,d11,d12,d21,d22)/2520.-
           TRANSF3(1,1,4,d0,d1,d2)/30240.)*NINDI(n2,n1)*NINDI(n2,n0);
   *dn2 = (TRANSF2(0,1,3,6,1,1,d01,d02,d11,d12,d21,d22)/2520.-
           TRANSF3(-1,-4,-1,d0,d1,d2)/30240.)*NINDI(n2,n1)*NINDI(n1,n0);

/*
   *ann = -0.5*wl2_d_fi_fi(divb1,divb2,divb0)*rdetB;
   *an1 = 0.5*(conv_n_fi_fi(bb_0,bb_2,bb_1,b0,b2,b1) -
               conv_n_fi_fi(bb_1,bb_2,bb_0,b1,b2,b0) -
               wl2_n_fi_fi(divb1,divb2,divb0) )*rdetB;
   *an2 = 0.5*(conv_n_fi_fi(bb_0,bb_1,bb_2,b0,b1,b2) -
               conv_n_fi_fi(bb_2,bb_1,bb_0,b2,b1,b0) -
               wl2_n_fi_fi(divb2,divb1,divb0) )*rdetB;
   q1   = 0.5*(conv_d_chi_fi(bb_1,bb_2,bb_0,b1,b2,b0,n1,n2,n0) -
               conv_d_fi_chi(bb_1,bb_2,bb_0,b1,b2,b0,n1,n2,n0))*rdetB;
   q2   = 0.5*wl2_d_fi_chi(divb1,divb2,divb0,n1,n2,n0)*rdetB;
   *bnn = q1 - q2;
   *bn1 = 0.5*(conv_n_chi_fi(bb_0,bb_2,bb_1,b0,b2,b1,n0,n2,n1) -
               conv_n_fi_chi(bb_1,bb_2,bb_0,b1,b2,b0,n1,n2,n0) -
               wl2_n_fi_chi(divb1,divb2,divb0,n1,n2,n0) )*rdetB;
   *bn2 = 0.5*(conv_n_chi_fi(bb_0,bb_1,bb_2,b0,b1,b2,n0,n1,n2) -
               conv_n_fi_chi(bb_2,bb_1,bb_0,b2,b1,b0,n2,n1,n0) -
               wl2_n_fi_chi(divb2,divb1,divb0,n2,n1,n0) )*rdetB;
   *cnn = -q1 - q2;
   *cn1 = 0.5*(conv_n_fi_chi(bb_0,bb_2,bb_1,b0,b2,b1,n0,n2,n1) -
               conv_n_chi_fi(bb_1,bb_2,bb_0,b1,b2,b0,n1,n2,n0) -
               wl2_n_fi_chi(divb0,divb2,divb1,n0,n2,n1) )*rdetB;
   *cn2 = 0.5*(conv_n_fi_chi(bb_0,bb_1,bb_2,b0,b1,b2,n0,n1,n2) -
               conv_n_chi_fi(bb_2,bb_1,bb_0,b2,b1,b0,n2,n1,n0) -
               wl2_n_fi_chi(divb0,divb1,divb2,n0,n1,n2) )*rdetB;
   *dnn = -0.5*wl2_d_chi_chi(divb1,divb2,divb0)*rdetB;
   *dn1 = 0.5*(conv_n_chi_chi(bb_0,bb_2,bb_1,b0,b2,b1,n0,n2,n1) -
               conv_n_chi_chi(bb_1,bb_2,bb_0,b1,b2,b0,n1,n2,n0) -
               wl2_n_chi_chi(divb1,divb2,divb0,n1,n2,n0) )*rdetB;
   *dn2 = 0.5*(conv_n_chi_chi(bb_0,bb_1,bb_2,b0,b1,b2,n0,n1,n2) -
               conv_n_chi_chi(bb_2,bb_1,bb_0,b2,b1,b0,n2,n1,n0) -
               wl2_n_chi_chi(divb2,divb1,divb0,n2,n1,n0) )*rdetB;
*/
} 

void nc_cijb_mbub(n0,n1,n2,fa0,fa1,fa2,Z,bb0,bb1,div_bb,b0,b1,b2,rdetB,type)
NODE *n0, *n1, *n2;                /*  convective term; exact for pw. lin. b  */
FACE *fa0, *fa1, *fa2;
INT Z, type;
FLOAT (*bb0)(), (*bb1)(), (*div_bb)(), b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], divb0, divb1, divb2,
         ann, an1, an2, bnn, bn1, bn2, cnn, cn1, cn2, dnn, dn1, dn2;
  
      V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)

      if (type == CONV)
         nc_convective_form_mbub(bb_0,bb_1,bb_2,b0,b1,b2,n0,n1,n2,rdetB,
                   &ann,&an1,&an2,&bnn,&bn1,&bn2,&cnn,&cn1,&cn2,&dnn,&dn1,&dn2);
      else if (type == SKEW){
         S_NODE_VALUES(n0,n1,n2,div_bb,divb0,divb1,divb2)
         nc_skew_symm_form_mbub(bb_0,bb_1,bb_2,divb0,divb1,divb2,b0,b1,b2,
                   n0,n1,n2,rdetB,
                   &ann,&an1,&an2,&bnn,&bn1,&bn2,&cnn,&cn1,&cn2,&dnn,&dn1,&dn2);
      }
      else
         eprintf("Error: wrong type in nc_cijb_mbub.\n");

      vadd_to_matrix_f(fa0,fa1,fa2,Z,
                               ann,an1,an2,bnn,bn1,bn2,cnn,cn1,cn2,dnn,dn1,dn2);
}

/* (\int_K p q \dx)/|K|; exact for p,q\in\P_3 */
FLOAT pq3(p1,p2,p3,p112,p113,p221,p223,p331,p332,p123,
          q1,q2,q3,q112,q113,q221,q223,q331,q332,q123)
FLOAT p1,p2,p3,p112,p113,p221,p223,p331,p332,p123, 
      q1,q2,q3,q112,q113,q221,q223,q331,q332,q123;
{
   return((76.*( p1*q1+p2*q2+p3*q3 ) +
           11.*( p1*(q2+q3)+p2*(q1+q3)+p3*(q1+q2) ) +
          540.*( p112*q112+p113*q113+p221*q221+p223*q223+p331*q331+p332*q332 ) +
          270.*( p112*q113+p113*q112+p221*q223+p223*q221+p331*q332+p332*q331 ) -
          189.*( p112*q221+p113*q331+p221*q112+p223*q332+p331*q113+p332*q223 ) -
          135.*( p112*q331+p113*q221+p221*q332+p223*q112+p331*q223+p332*q113 ) -
          135.*( p112*q223+p113*q332+p221*q113+p223*q331+p331*q112+p332*q221 ) -
           54.*( p112*q332+p113*q223+p221*q331+p223*q113+p331*q221+p332*q112 ) +
         1944.*p123*q123 +
           18.*( p1*q112 + q1*p112 + p1*q113 + q1*p113 +
                 p2*q221 + q2*p221 + p2*q223 + q2*p223 + 
                 p3*q331 + q3*p331 + p3*q332 + q3*p332) +
           27.*( p1*q223 + q1*p223 + p1*q332 + q1*p332 +
                 p2*q113 + q2*p113 + p2*q331 + q2*p331 +
                 p3*q112 + q3*p112 + p3*q221 + q3*p221) +
          162.*( p123*(q112+q113+q221+q223+q331+q332) +
                 q123*(p112+p113+p221+p223+p331+p332) )+
           36.*( p123*(q1+q2+q3) + q123*(p1+p2+p3) ) )/6720.);
}

FLOAT get_pq3(p,q,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu,detB,
             x1,x2,x3,x112,x113,x221,x223,x331,x332,x123)
FLOAT (*p)(), (*q)(), b0[DIM2], b1[DIM2], b2[DIM2], (*bb0)(), (*bb1)(), (*c)(), 
      nu, detB, *x1, *x2, *x3, *x112, *x113, *x221, *x223, *x331, *x332, *x123;
NODE *n0, *n1, *n2;
{
   FLOAT p1, p2, p3, p112, p113, p221, p223, p331, p332, p123,
         q1, q2, q3, q112, q113, q221, q223, q331, q332, q123;

   p1 = p(x1,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu);
   p2 = p(x2,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu);
   p3 = p(x3,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu);
   p112 = p(x112,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu);
   p113 = p(x113,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu);
   p221 = p(x221,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu);
   p223 = p(x223,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu);
   p331 = p(x331,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu);
   p332 = p(x332,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu);
   p123 = p(x123,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu);

   q1 = q(x1,b0,b1,b2,n1,n2,bb0,bb1);
   q2 = q(x2,b0,b1,b2,n1,n2,bb0,bb1);
   q3 = q(x3,b0,b1,b2,n1,n2,bb0,bb1);
   q112 = q(x112,b0,b1,b2,n1,n2,bb0,bb1);
   q113 = q(x113,b0,b1,b2,n1,n2,bb0,bb1);
   q221 = q(x221,b0,b1,b2,n1,n2,bb0,bb1);
   q223 = q(x223,b0,b1,b2,n1,n2,bb0,bb1);
   q331 = q(x331,b0,b1,b2,n1,n2,bb0,bb1);
   q332 = q(x332,b0,b1,b2,n1,n2,bb0,bb1);
   q123 = q(x123,b0,b1,b2,n1,n2,bb0,bb1);

   return( pq3(p1,p2,p3,p112,p113,p221,p223,p331,p332,p123,
               q1,q2,q3,q112,q113,q221,q223,q331,q332,q123)*detB );
}

FLOAT get_pq3_rhs(p,q,b0,b1,b2,n1,n2,bb0,bb1,detB,
                 x1,x2,x3,x112,x113,x221,x223,x331,x332,x123)
FLOAT (*p)(), (*q)(), b0[DIM2], b1[DIM2], b2[DIM2], (*bb0)(), (*bb1)(), detB,  
      *x1, *x2, *x3, *x112, *x113, *x221, *x223, *x331, *x332, *x123;
NODE *n1, *n2;
{
   FLOAT p1, p2, p3, p112, p113, p221, p223, p331, p332, p123,
         q1, q2, q3, q112, q113, q221, q223, q331, q332, q123;

   p1 = p(x1);
   p2 = p(x2);
   p3 = p(x3);
   p112 = p(x112);
   p113 = p(x113);
   p221 = p(x221);
   p223 = p(x223);
   p331 = p(x331);
   p332 = p(x332);
   p123 = p(x123);

   q1 = q(x1,b0,b1,b2,n1,n2,bb0,bb1);
   q2 = q(x2,b0,b1,b2,n1,n2,bb0,bb1);
   q3 = q(x3,b0,b1,b2,n1,n2,bb0,bb1);
   q112 = q(x112,b0,b1,b2,n1,n2,bb0,bb1);
   q113 = q(x113,b0,b1,b2,n1,n2,bb0,bb1);
   q221 = q(x221,b0,b1,b2,n1,n2,bb0,bb1);
   q223 = q(x223,b0,b1,b2,n1,n2,bb0,bb1);
   q331 = q(x331,b0,b1,b2,n1,n2,bb0,bb1);
   q332 = q(x332,b0,b1,b2,n1,n2,bb0,bb1);
   q123 = q(x123,b0,b1,b2,n1,n2,bb0,bb1);

   return( pq3(p1,p2,p3,p112,p113,p221,p223,p331,p332,p123,
               q1,q2,q3,q112,q113,q221,q223,q331,q332,q123)*detB );
}

FLOAT integr_rhs_zeta(nc_zeta,b0,b1,b2,n0,n1,n2,rhs,bb0,bb1,detB)
FLOAT (*nc_zeta)(), b0[DIM2], b1[DIM2], b2[DIM2], (*rhs)(), (*bb0)(), (*bb1)(), detB;
NODE *n0, *n1, *n2;
{
   FLOAT *x1, *x2, *x3, x112[DIM], x113[DIM], x221[DIM], x223[DIM],
                                              x331[DIM], x332[DIM], x123[DIM];

   x1 = n0->myvertex->x;
   x2 = n1->myvertex->x;
   x3 = n2->myvertex->x;
   points(x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   return( get_pq3_rhs(rhs,nc_zeta,b0,b1,b2,n1,n2,bb0,bb1,detB,
                              x1,x2,x3,x112,x113,x221,x223,x331,x332,x123) );
} 

void integr_rhs_fi_chi(b0,b1,b2,n0,n1,n2,rhs,detB,fnn,gnn)
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], (*rhs)(), detB, *fnn, *gnn;
NODE *n0, *n1, *n2;
{
   FLOAT *x1, *x2, *x3, x112[DIM], x113[DIM], x221[DIM], x223[DIM],
                                              x331[DIM], x332[DIM], x123[DIM];

   x1 = n0->myvertex->x;
   x2 = n1->myvertex->x;
   x3 = n2->myvertex->x;
   points(x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   *fnn = get_pq3_rhs(rhs,nc_fi12,b0,b1,b2,n1,n2,rhs,rhs,detB,
                              x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   *gnn = get_pq3_rhs(rhs,nc_chi12,b0,b1,b2,n1,n2,rhs,rhs,detB,
                              x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
} 

void nc_sd_terms_mbub(pel,b0,b1,b2,n0,n1,n2,bb0,bb1,c0,c1,c2,c,nu,detB,
                                ann,an1,an2,bnn,bn1,bn2,cnn,cn1,cn2,dnn,dn1,dn2)
ELEMENT *pel;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], (*bb0)(), (*bb1)(), (*c)(), 
                                                           c0, c1, c2, nu, detB;
NODE *n0, *n1, *n2;
FLOAT *ann,*an1,*an2,*bnn,*bn1,*bn2,*cnn,*cn1,*cn2,*dnn,*dn1,*dn2;
{
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], a11, a12, a22, 
         d01, d02, d11, d12, d21, d22, p;
   FLOAT *x1, *x2, *x3, x112[DIM], x113[DIM], x221[DIM], x223[DIM],
                                              x331[DIM], x332[DIM], x123[DIM];

   VERTICES_OF_ELEMENT(x1,x2,x3,pel);
   points(x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
   d01 = DOT(bb_0,b1);
   d02 = DOT(bb_0,b2);
   d11 = DOT(bb_1,b1);
   d12 = DOT(bb_1,b2);
   d21 = DOT(bb_2,b1);
   d22 = DOT(bb_2,b2);
   a11 = DOT(b1,b1);
   a12 = DOT(b1,b2);
   a22 = DOT(b2,b2);

   /* -nu*lapl fi_j + bb*(grad fi_j), bb*(grad fi_i) */
   *ann = (TRANSF4(124,94,94,124,34,23,23,42,42,23,23,34,220,228,228,276,
                  90,85,85,90,276,228,228,220,d01,d02,d11,d12,d21,d22)/63. +
           TRANSF5(8,8,1,11,11,1,166,176,83,83,176,166,
                                  c0,c1,c2,d01,d02,d11,d12,d21,d22)/630. -
           TRANSF6(-5,-3,-2,-2,-3,-5,14,17,12,14,10,9,9,10,14,12,17,14,
                   a11,a12,a22,d01,d02,d11,d12,d21,d22)/0.45*nu)*detB; 
   *an1 = (TRANSF4(-152,-2,-92,28,-29,-16,-13,9,-51,-10,-7,-5,-152,-60,-150,-30,
            -51,-44,-41,-39,-246,-78,-168,-68,d01,d02,d11,d12,d21,d22)/63. +
           TRANSF5(96,36,6,21,38,-7,68,128,26,31,116,46,
                                  c0,c1,c2,d01,d02,d11,d12,d21,d22)/630. -
           TRANSF6(7,3,1,1,0,-2,-10,-13,-6,-7,-8,-12,3,-2,-7,-6,-4,-4,
                   a11,a12,a22,d01,d02,d11,d12,d21,d22)/0.45*nu)*detB; 
   *an2 = (TRANSF4(28,-92,-2,-152,-5,-7,-10,-51,9,-13,-16,-29,-68,-168,-78,-246,
            -39,-41,-44,-51,-30,-150,-60,-152,d01,d02,d11,d12,d21,d22)/63. +
           TRANSF5(36,96,-7,38,21,6,46,116,31,26,128,68,
                                  c0,c1,c2,d01,d02,d11,d12,d21,d22)/630. -
           TRANSF6(-2,0,1,1,3,7,-4,-4,-6,-7,-2,3,-12,-8,-7,-6,-13,-10,
                   a11,a12,a22,d01,d02,d11,d12,d21,d22)/0.45*nu)*detB; 

   /* -nu*lapl chi_j + bb*(grad chi_j), bb*(grad fi_i) */
   p = TRANSF4(-20,-10,10,20,-1,4,14,39,-39,-14,-4,1,96,204,126,294,9,-24,24,-9,
            -294,-126,-204,-96,d01,d02,d11,d12,d21,d22)/1260.;
   *bnn = (p + TRANSF5(-5,5,1,11,-11,-1,76,101,-5,5,-101,-76,
                     c0,c1,c2,d01,d02,d11,d12,d21,d22)/3780. -
           TRANSF6(1,0,-1,1,0,-1,2,2,2,4,-4,-6,6,4,-4,-2,-2,-2,
              a11,a12,a22,d01,d02,d11,d12,d21,d22)/4.5*nu)*detB*NINDI(n2,n1);
   *bn1 = (TRANSF4(-36,138,-6,228,11,10,6,35,21,12,36,57,40,20,30,30,69,
                   50,64,55,354,258,324,288,d01,d02,d11,d12,d21,d22)/1260. +
           TRANSF5(41,31,-4,1,-10,-5,-10,-5,-16,-11,-91,-56,
                     c0,c1,c2,d01,d02,d11,d12,d21,d22)/3780. -
           TRANSF6(1,0,0,-2,-3,-6,2,2,4,3,6,3,6,4,11,8,15,12,
                 a11,a12,a22,d01,d02,d11,d12,d21,d22)/4.5*nu)*detB*NINDI(n2,n0);
   *bn2 = (TRANSF4(228,-6,138,-36,57,36,12,21,35,6,10,11,288,324,258,354,
                   55,64,50,69,30,30,20,40,d01,d02,d11,d12,d21,d22)/1260. +
           TRANSF5(31,41,-5,-10,1,-4,-56,-91,-11,-16,-5,-10,
                     c0,c1,c2,d01,d02,d11,d12,d21,d22)/3780. -
           TRANSF6(-6,-3,-2,0,0,1,12,15,8,11,4,6,3,6,3,4,2,2,
                 a11,a12,a22,d01,d02,d11,d12,d21,d22)/4.5*nu)*detB*NINDI(n1,n0);

   /* -nu*lapl fi_j + bb*(grad fi_j), bb*(grad chi_i) */
   *cnn = (p + TRANSF5(10,-10,13,-7,7,-13,70,50,25,-25,-50,-70,
                     c0,c1,c2,d01,d02,d11,d12,d21,d22)/7560. -
           TRANSF6(1,1,0,0,-1,-1,5,7,4,4,2,0,0,-2,-4,-4,-7,-5,
                 a11,a12,a22,d01,d02,d11,d12,d21,d22)/4.5*nu)*detB*NINDI(n2,n1);
   *cn1 = (TRANSF4(40,10,20,20,11,5,1,30,69,5,19,10,-36,-30,-174,60,21,-15,9,30,
                   354,30,96,60,d01,d02,d11,d12,d21,d22)/1260. + 
           TRANSF5(-14,-4,19,20,-29,-10,82,80,25,8,-122,-16,
                     c0,c1,c2,d01,d02,d11,d12,d21,d22)/7560. -
           TRANSF6(-2,-2,0,0,-1,-1,-4,-8,-2,-2,-4,-6,-6,-2,2,2,-1,1,
                 a11,a12,a22,d01,d02,d11,d12,d21,d22)/4.5*nu)*detB*NINDI(n2,n1);
   *cn2 = (TRANSF4(-20,-20,-10,-40,-10,-19,-5,-69,-30,-1,-5,-11,-60,
      -96,-30,-354,-30,-9,15,-21,-60,174,30,36,d01,d02,d11,d12,d21,d22)/1260. +
           TRANSF5(4,14,10,29,-20,-19,16,122,-8,-25,-80,-82,
                     c0,c1,c2,d01,d02,d11,d12,d21,d22)/7560. -
           TRANSF6(1,1,0,0,2,2,-1,1,-2,-2,2,6,6,4,2,2,8,4,
              a11,a12,a22,d01,d02,d11,d12,d21,d22)/4.5*nu)*detB*NINDI(n2,n1);

   /* -nu*lapl chi_j + bb*(grad chi_j), bb*(grad chi_i) */
   *dnn = (TRANSF4(8,2,2,8,6,3,3,18,18,3,3,6,24,18,18,108,18,3,3,18,108,
                   18,18,24,d01,d02,d11,d12,d21,d22)/5040. +
           TRANSF5(2,2,3,4,4,3,16,24,8,8,24,16,
                                 c0,c1,c2,d01,d02,d11,d12,d21,d22)/15120. -
           TRANSF6(-1,-1,2,2,-1,-1,1,-1,3,7,-4,-6,-6,-4,7,3,-1,1,
                   a11,a12,a22,d01,d02,d11,d12,d21,d22)/90.*nu)*detB;
   *dn1 = (TRANSF4(0,6,2,8,-2,-1,-1,4,-6,3,-1,4,0,-2,-6,0,-6,-5,-9,-4,
                   -84,-54,-30,-16,d01,d02,d11,d12,d21,d22)/5040. +
           TRANSF5(2,0,1,1,6,2,0,2,2,3,28,10,
                   c0,c1,c2,d01,d02,d11,d12,d21,d22)/15120. -
           TRANSF6(-1,-1,-2,-2,-3,-3,1,-1,1,-3,0,-6,-6,-4,-11,-7,-15,-9,
    a11,a12,a22,d01,d02,d11,d12,d21,d22)/90.*nu)*detB*NINDI(n2,n1)*NINDI(n2,n0);
   *dn2 = (TRANSF4(-8,-2,-6,0,-4,1,-3,6,-4,1,1,2,16,30,54,84,4,9,5,6,0,6,2,0,
                   d01,d02,d11,d12,d21,d22)/5040. +
           TRANSF5(0,-2,-2,-6,-1,-1,-10,-28,-3,-2,-2,0,
                   c0,c1,c2,d01,d02,d11,d12,d21,d22)/15120. -
           TRANSF6(3,3,2,2,1,1,9,15,7,11,4,6,6,0,3,-1,1,-1,
    a11,a12,a22,d01,d02,d11,d12,d21,d22)/90.*nu)*detB*NINDI(n2,n1)*NINDI(n1,n0);

/* *ann = get_pq3(nc_lbgfi12,nc_bgfi12,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu,detB,
                             x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   *an1 = get_pq3(nc_lbgfi02,nc_bgfi12,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu,detB,
                             x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   *an2 = get_pq3(nc_lbgfi01,nc_bgfi12,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu,detB,
                             x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   *bnn = get_pq3(nc_lbgchi12,nc_bgfi12,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu,detB,
                              x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   *bn1 = get_pq3(nc_lbgchi02,nc_bgfi12,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu,detB,
                              x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   *bn2 = get_pq3(nc_lbgchi01,nc_bgfi12,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu,detB,
                              x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   *cnn = get_pq3(nc_lbgfi12,nc_bgchi12,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu,detB,
                             x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   *cn1 = get_pq3(nc_lbgfi02,nc_bgchi12,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu,detB,
                             x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   *cn2 = get_pq3(nc_lbgfi01,nc_bgchi12,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu,detB,
                             x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   *dnn = get_pq3(nc_lbgchi12,nc_bgchi12,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu,detB,
                              x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   *dn1 = get_pq3(nc_lbgchi02,nc_bgchi12,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu,detB,
                              x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   *dn2 = get_pq3(nc_lbgchi01,nc_bgchi12,b0,b1,b2,n0,n1,n2,bb0,bb1,c,nu,detB,
                              x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
*/
}

void nc_sd_ijb_mbub(pel,n0,n1,n2,fa0,fa1,fa2,Z,bb0,bb1,react,b0,b1,b2,detB,nu)
ELEMENT *pel;            /*  streamline-diffusion term; exact for pw. lin. b, */
NODE *n0, *n1, *n2;      /*  and pw. const. react                             */
FACE *fa0, *fa1, *fa2;   /*  detB = volume*delta                              */
INT Z;
FLOAT (*bb0)(), (*bb1)(), (*react)(), b0[DIM2], b1[DIM2], b2[DIM2], 
                                                                       detB, nu;
{
   FLOAT ann, an1, an2, bnn, bn1, bn2, cnn, cn1, cn2, dnn, dn1, dn2, c0, c1, c2;
  
      S_NODE_VALUES(n0,n1,n2,react,c0,c1,c2)

      nc_sd_terms_mbub(pel,b0,b1,b2,n0,n1,n2,bb0,bb1,c0,c1,c2,react,nu,detB,
         &ann,&an1,&an2,&bnn,&bn1,&bn2,&cnn,&cn1,&cn2,&dnn,&dn1,&dn2);

      vadd_to_matrix_f(fa0,fa1,fa2,Z,
                               ann,an1,an2,bnn,bn1,bn2,cnn,cn1,cn2,dnn,dn1,dn2);
}

void nc_react_term_to_stiff_matr_mbub(tGrid,Z,r)
GRID *tGrid;
INT Z;
FLOAT (*r)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
     NODES_OF_ELEMENT(n0,n1,n2,pelem);
     FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
     rdetB = VOLUME(pelem);
     nc_rijb_mbub(n0,n1,n2,fa0,fa1,fa2,Z,r,rdetB);
     nc_rijb_mbub(n1,n2,n0,fa1,fa2,fa0,Z,r,rdetB);
     nc_rijb_mbub(n2,n0,n1,fa2,fa0,fa1,Z,r,rdetB);
   } 
}

void nc_conv_term_to_stiff_matr_mbub(tGrid,Z,bb0,bb1,div_bb,type)
GRID *tGrid;
INT Z, type;
FLOAT (*bb0)(), (*bb1)(), (*div_bb)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
     NODES_OF_ELEMENT(n0,n1,n2,pelem);
     FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
     rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                     n2->myvertex->x,b);
     nc_cijb_mbub(n0,n1,n2,fa0,fa1,fa2,Z,bb0,bb1,div_bb,b[0],b[1],b[2],
                                                                    rdetB,type);
     nc_cijb_mbub(n1,n2,n0,fa1,fa2,fa0,Z,bb0,bb1,div_bb,b[1],b[2],b[0],
                                                                    rdetB,type);
     nc_cijb_mbub(n2,n0,n1,fa2,fa0,fa1,Z,bb0,bb1,div_bb,b[2],b[0],b[1],
                                                                    rdetB,type);
   } 
}

void nc_sd_term_to_stiff_matr_mbub(tGrid,nu,Z,bb0,bb1,react)
GRID *tGrid;
INT Z;
FLOAT nu, (*bb0)(), (*bb1)(), (*react)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2], delta;
   INT n=0;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      delta = sd_delta(pelem,nu,bb0,bb1);
      if (delta > 0.){
         n++;
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
         rdetB = delta*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                             n2->myvertex->x,b);
         nc_sd_ijb_mbub(pelem,n0,n1,n2,fa0,fa1,fa2,Z,bb0,bb1,react,
                                                       b[0],b[1],b[2],rdetB,nu);
         nc_sd_ijb_mbub(pelem,n1,n2,n0,fa1,fa2,fa0,Z,bb0,bb1,react,
                                                       b[1],b[2],b[0],rdetB,nu);
         nc_sd_ijb_mbub(pelem,n2,n0,n1,fa2,fa0,fa1,Z,bb0,bb1,react,
                                                       b[2],b[0],b[1],rdetB,nu);
      }
   } 
   printf("SDFEM on %i elements.\n",n);
}

/******************************************************************************/
/*                                                                            */
/*                 2D stiffness matrices by Martin Sodomka                    */
/*                                                                            */
/******************************************************************************/

pro_plot(NODE *n0, NODE *n1, NODE *n2, DOUBLE *alpha)
{
  DOUBLE *x0, *x1, *x2;
  DOUBLE x01[2], x02[2], x12[2];
  FILE *fa;
  char file_nam[50];

  sprintf(file_nam, "%s%s", DIR_FOR_RESULTS, "bad_elem.dat");
  
  fa = fopen(file_nam, "a");

   x0 = n0->myvertex->x;
   x1 = n1->myvertex->x;
   x2 = n2->myvertex->x;   
   MIDPOINTS(x0,x1,x2,x01,x02,x12)

   fprintf(fa, "\n%lf %lf\n", x0[0], x0[1]);
   fprintf(fa, "%lf %lf\n", x1[0], x1[1]);
   fprintf(fa, "%lf %lf\n", x2[0], x2[1]);
   fprintf(fa, "%lf %lf\n\n", x0[0], x0[1]);
   
   if (IS_BN(n0) && IS_BN(n1)) {
     fprintf(fa, "%lf %lf\n", x01[0], x01[1]);
     fprintf(fa, "%lf %lf\n", x01[0] + alpha[0], x01[1] + alpha[1]);
   }
   else
   if (IS_BN(n0) && IS_BN(n2)) {
     fprintf(fa, "%lf %lf\n", x02[0], x02[1]);
     fprintf(fa, "%lf %lf\n", x02[0] + alpha[0], x02[1] + alpha[1]);
   }
   else
   if (IS_BN(n1) && IS_BN(n2)) {
     fprintf(fa, "%lf %lf\n", x12[0], x12[1]);
     fprintf(fa, "%lf %lf\n", x12[0] + alpha[0], x12[1] + alpha[1]);
   }
  
   fclose(fa);
}

/* jac = jac[0]*x + jac[1]*y + jac[2] is the Jacobian;
   (b[i][j][0]*x + b[i][j][1]*y + b[i][j][2])/jac is the inverse of the 
   Jacobi matrix of the reference mapping */
/* The curved edge n1n2 is mapped on the longest edge of the ref. element */
void inverse_of_Q1_reference_mapping0(n0,n1,n2,alpha,b,jac)
NODE *n0, *n1, *n2;
FLOAT alpha[DIM], b[2][2][DIM2], jac[DIM2];
{
   FLOAT *x0, *x1, *x2, a[2][2], s0, s1, s2;   
  
   x0 = n0->myvertex->x;
   x1 = n1->myvertex->x;
   x2 = n2->myvertex->x;

   a[0][0] = x1[0] - x0[0];
   a[0][1] = x2[0] - x0[0];
   a[1][0] = x1[1] - x0[1];
   a[1][1] = x2[1] - x0[1];

   jac[0] = a[0][0]*alpha[1] - a[1][0]*alpha[0];
   jac[1] = a[1][1]*alpha[0] - a[0][1]*alpha[1];
   jac[2] = a[0][0]*a[1][1]-a[0][1]*a[1][0];
  
   s0 = jac[2];
   s1 = jac[0]+jac[2];
   s2 = jac[1]+jac[2];
   if ( fabs(s0) < 1.e-7 || fabs(s1) < 1.e-7 || fabs(s2) < 1.e-7 ||
        s0*s1 < 1.e-14 || s0*s2 < 1.e-14 || s1*s2 < 1.e-14)    
   {
      eprintf("ZERO DETERMINANT\n");
      pro_plot(n0, n1, n2, alpha);
   }
  
   b[0][0][0] =  alpha[1];
   b[0][0][1] =  0.;
   b[0][0][2] =  a[1][1];
   b[0][1][0] =  -alpha[0];
   b[0][1][1] =  0.;
   b[0][1][2] =  -a[0][1];
   b[1][0][0] =  0.;
   b[1][0][1] =  -alpha[1];
   b[1][0][2] =  -a[1][0];
   b[1][1][0] =  0.;
   b[1][1][1] =  alpha[0];
   b[1][1][2] =  a[0][0];
}
 
/* The curved edge n0n2 is mapped on the edge between (0,0) and (0,1). */
void inverse_of_Q1_reference_mapping1(n0,n1,n2,alpha,b,jac)
NODE *n0, *n1, *n2;
FLOAT alpha[DIM], b[2][2][DIM2], jac[DIM2];
{
   FLOAT *x0, *x1, *x2, a[2][2], s0, s1, s2; 
   
   x0 = n0->myvertex->x;
   x1 = n1->myvertex->x;
   x2 = n2->myvertex->x;
   a[0][0] = x1[0] - x0[0];
   a[0][1] = x2[0] - x0[0];
   a[1][0] = x1[1] - x0[1];
   a[1][1] = x2[1] - x0[1];

   jac[0] = -a[0][0]*alpha[1]+a[1][0]*alpha[0];
   jac[1] = 2.*(a[1][0]*alpha[0] - a[0][0]*alpha[1]) -
            alpha[0]*a[1][1]+alpha[1]*a[0][1];
   jac[2] = a[0][0]*a[1][1]+a[0][0]*alpha[1]-a[1][0]*alpha[0]-a[1][0]*a[0][1];
 
   s0 = jac[2];
   s1 = jac[0]+jac[2];
   s2 = jac[1]+jac[2];
   if ( fabs(s0) < 1.e-7 || fabs(s1) < 1.e-7 || fabs(s2) < 1.e-7 ||
        s0*s1 < 1.e-14 || s0*s2 < 1.e-14 || s1*s2 < 1.e-14)
   {
      eprintf("ZERO DETERMINANT\n");
      pro_plot(n0, n1, n2, alpha);
   }
  
   b[0][0][0] =  -alpha[1];
   b[0][0][1] =  -2.*alpha[1];
   b[0][0][2] =  a[1][1]+alpha[1];
   b[0][1][0] =  alpha[0];
   b[0][1][1] =  2.*alpha[0];
   b[0][1][2] =  -a[0][1]-alpha[0];
   b[1][0][0] =  0.;
   b[1][0][1] =  alpha[1];
   b[1][0][2] =  -a[1][0];
   b[1][1][0] =  0.;
   b[1][1][1] =  -alpha[0];
   b[1][1][2] =  a[0][0];
}
 
/* The curved edge n0n1 is mapped on the edge between (0,0) and (1,0). */
void inverse_of_Q1_reference_mapping2(n0,n1,n2,alpha,b,jac)
NODE *n0, *n1, *n2;
FLOAT alpha[DIM], b[2][2][DIM2], jac[DIM2];
{
   FLOAT *x0, *x1, *x2, a[2][2], s0, s1, s2; 
  
   x0 = n0->myvertex->x;
   x1 = n1->myvertex->x;
   x2 = n2->myvertex->x;
   a[0][0] = x1[0] - x0[0];
   a[0][1] = x2[0] - x0[0];
   a[1][0] = x1[1] - x0[1];
   a[1][1] = x2[1] - x0[1];

   jac[0] = 2.*(a[0][1]*alpha[1]-alpha[0]*a[1][1])+
            alpha[0]*a[1][0]-a[0][0]*alpha[1];
   jac[1] = a[0][1]*alpha[1]-alpha[0]*a[1][1];
   jac[2] = a[0][0]*a[1][1]-a[0][1]*a[1][0]+alpha[0]*a[1][1]-a[0][1]*alpha[1];
  
   s0 = jac[2];
   s1 = jac[0]+jac[2];
   s2 = jac[1]+jac[2];
   if ( fabs(s0) < 1.e-7 || fabs(s1) < 1.e-7 || fabs(s2) < 1.e-7 ||
        s0*s1 < 1.e-14 || s0*s2 < 1.e-14 || s1*s2 < 1.e-14)
   {
      eprintf("ZERO DETERMINANT\n");
      pro_plot(n0, n1, n2, alpha);
   }
  
   b[0][0][0] =  -alpha[1];
   b[0][0][1] =  0.;
   b[0][0][2] =  a[1][1];
   b[0][1][0] =  alpha[0];
   b[0][1][1] =  0.;
   b[0][1][2] =  -a[0][1];
   b[1][0][0] =  2.*alpha[1];
   b[1][0][1] =  alpha[1];
   b[1][0][2] =  -a[1][0]-alpha[1];
   b[1][1][0] =  -2.*alpha[0];
   b[1][1][1] =  -alpha[0];
   b[1][1][2] =  a[0][0]+alpha[0];
}

#if (N_DATA & IxD_NODE_MATR) && (N_DATA & Dx1_NODE_FACE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (MOVING_BOUNDARY == YES)

void mso_bij_sn_sf_iso(n0,n1,n2,fa0,fa1,fa2,Z)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT Z;
{
  FLOAT *x0, *x1, *x2, x01[DIM], x02[DIM], x12[DIM], alpha[DIM];
//   NODE *n;
//   FACE *f;
   LINK *pli;
   NFLINK *pnf;
   FLOAT b[2][2][DIM2], jac[DIM2], s=1;

   FLOAT uzly[6], hrany[6];

   x0=n0->myvertex->x;
   x1=n1->myvertex->x;
   x2=n2->myvertex->x;
   MIDPOINTS(x0,x1,x2,x01,x02,x12)
		   

   if (IS_CURVED_EDGE(fa0)){
     SUBTR(fa0->myvertex->x,x12,alpha);
     SET2(alpha,alpha,4.);
//     pro_plot(n0, n1, n2, alpha);
     inverse_of_Q1_reference_mapping0(n0,n1,n2,alpha,b,jac);
   }
   else if (IS_CURVED_EDGE(fa1)){
      SUBTR(fa1->myvertex->x,x02,alpha);
      SET2(alpha,alpha,4.);
//      pro_plot(n1, n0, n2, alpha);
      inverse_of_Q1_reference_mapping1(n0,n1,n2,alpha,b,jac);
   }
   else if (IS_CURVED_EDGE(fa2)){
      SUBTR(fa2->myvertex->x,x01,alpha);
      SET2(alpha,alpha,4.);
//      pro_plot(n2, n0, n1, alpha);
      inverse_of_Q1_reference_mapping2(n0,n1,n2,alpha,b,jac);
   }
   else{
     alpha[0]=0.;
     alpha[1]=0.;
     inverse_of_Q1_reference_mapping2(n0,n1,n2,alpha,b,jac);
   }

   /* the reference mapping always maps (0,0)->n0, (1,0)->n1, (0,1)->n2  */

   if (jac[2] < 0.) s = -1;
   s /= 24.;
   COEFFB(n0,Z,0) +=uzly[0]= s*(b[1][0][1] + b[0][0][1] + b[0][0][0] +    b[1][0][0] +
                                              4.*b[1][0][2] + 4.*b[0][0][2]);
   COEFFB(n0,Z,1) +=uzly[1]= s*(b[1][1][1] + b[0][1][1] + b[0][1][0] +    b[1][1][0] +
                                              4.*b[1][1][2] + 4.*b[0][1][2]);
   for (pli = n0->tstart; pli->nbnode != n1; pli = pli->next);
   COEFFBL(pli,Z,0) +=uzly[2]= -s*(b[0][0][1] + b[0][0][0] + 4.*b[0][0][2]);
   COEFFBL(pli,Z,1) +=uzly[3]= -s*(b[0][1][1] + b[0][1][0] + 4.*b[0][1][2]);
   for (pli = n0->tstart; pli->nbnode != n2; pli = pli->next);
   COEFFBL(pli,Z,0) +=uzly[4]= -s*(b[1][0][1] + b[1][0][0] + 4.*b[1][0][2]);
   COEFFBL(pli,Z,1) +=uzly[5]= -s*(b[1][1][1] + b[1][1][0] + 4.*b[1][1][2]);
   s *= 0.2;
   for (pnf = n0->tnfstart; pnf->nbface != fa0; pnf = pnf->next);
   COEFF_NF(pnf,Z,0) +=hrany[0]= -s*(2.*b[0][0][1] +    b[0][0][0] +    b[1][0][1] +
                           2.*b[1][0][0] + 5.*b[1][0][2] + 5.*b[0][0][2]);
   COEFF_NF(pnf,Z,1) +=hrany[1]= -s*(2.*b[0][1][1] +    b[0][1][0] +    b[1][1][1] +
                           2.*b[1][1][0] + 5.*b[1][1][2] + 5.*b[0][1][2]);
   for (pnf = n0->tnfstart; pnf->nbface != fa1; pnf = pnf->next);
   COEFF_NF(pnf,Z,0) +=hrany[2]= s*(2.*b[0][0][1] + b[0][0][0] -    b[1][0][0] +
                                       5.*b[0][0][2] - 5.*b[1][0][2]);
   COEFF_NF(pnf,Z,1) +=hrany[3]= s*(2.*b[0][1][1] + b[0][1][0] -    b[1][1][0] +
                                       5.*b[0][1][2] - 5.*b[1][1][2]);
   for (pnf = n0->tnfstart; pnf->nbface != fa2; pnf = pnf->next);
   COEFF_NF(pnf,Z,0) +=hrany[4]= s*(-b[0][0][1] + b[1][0][1] + 2.*b[1][0][0] -
                                     5.*b[0][0][2] + 5.*b[1][0][2]);
   COEFF_NF(pnf,Z,1) +=hrany[5]= s*(-b[0][1][1] + b[1][1][1] + 2.*b[1][1][0] -
                                     5.*b[0][1][2] + 5.*b[1][1][2]);
}

#else

void mso_bij_sn_sf_iso(n0,n1,n2,fa0,fa1,fa2,Z)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z;
{  eprintf("Error: mso_bij_sn_sf_iso not available.\n");  }
 
#endif

void bij_sn_sf();
void add_D_matrix();

#if (MOVING_BOUNDARY == YES)

void mso_iso_B_matr_P2C_P1C(tGrid,Z)
GRID *tGrid;
INT Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
	  if (IS_CURVED_EDGE(fa0) || IS_CURVED_EDGE(fa1) || IS_CURVED_EDGE(fa2)) {
         mso_bij_sn_sf_iso(n0,n1,n2,fa0,fa1,fa2,Z);
         mso_bij_sn_sf_iso(n1,n2,n0,fa1,fa2,fa0,Z);
         mso_bij_sn_sf_iso(n2,n0,n1,fa2,fa0,fa1,Z);
	  } else {
         rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
         bij_sn_sf(n0,n1,n2,fa0,fa1,fa2,Z,b[0],b[1],b[2],rdetB);
         bij_sn_sf(n1,n2,n0,fa1,fa2,fa0,Z,b[1],b[2],b[0],rdetB);
         bij_sn_sf(n2,n0,n1,fa2,fa0,fa1,Z,b[2],b[0],b[1],rdetB);
         
         add_D_matrix(n0,n1,n2,fa0,fa1,fa2,Z);
      }
   }
}

#else

void mso_iso_B_matr_P2C_P1C(tGrid,Z)
GRID *tGrid; INT Z;
{  eprintf("Error: mso_iso_B_matr_P2C_P1C not available.\n");  }

#endif

#if ((DATA_STR & KORN_MATRIX)) && (N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES) && (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA) && (MOVING_BOUNDARY == YES)

void putaij_korn(plin,n1,n2,an1,an2,Z)
 LINK *plin;
 NODE *n1, *n2;
 FLOAT an1, an2;
 INT Z;
 {
   LINK *pli;
   
   for (pli=plin; pli->nbnode != n1 && pli->nbnode != n2; pli=pli->next);
   if (pli->nbnode == n1){
      COEFFLL(pli,Z,0,0) += an1;
	  COEFFLL(pli,Z,1,1) += an1;
      for (pli=pli->next; pli->nbnode != n2; pli=pli->next); 
	  COEFFLL(pli,Z,0,0) += an2;
	  COEFFLL(pli,Z,1,1) += an2;
   }
   else {
	  COEFFLL(pli,Z,0,0) += an2;
	  COEFFLL(pli,Z,1,1) += an2;
      for (pli=pli->next; pli->nbnode != n1; pli=pli->next); 
	  COEFFLL(pli,Z,0,0) += an1;
	  COEFFLL(pli,Z,1,1) += an1;
   }
 }

void putasij_korn(pnfl,fa1,fa2,pn1,pn2,Z)
NFLINK *pnfl; 
FACE *fa1, *fa2;
INT Z;
FLOAT pn1,pn2;
{
  NFLINK *pnf;
  
  for (pnf = pnfl; pnf->nbface != fa1 && pnf->nbface != fa2; pnf = pnf->next);
  if (pnf->nbface==fa1){
     COEFFLL(pnf,Z,0,0) += pn1;
	 COEFFLL(pnf,Z,1,1) += pn1;
     for (pnf = pnf->next; pnf->nbface != fa2; pnf = pnf->next); 
     COEFFLL(pnf,Z,0,0) += pn2;
	 COEFFLL(pnf,Z,1,1) += pn2;
  }
  else {
     COEFFLL(pnf,Z,0,0) += pn2;
	 COEFFLL(pnf,Z,1,1) += pn2;
     for (pnf = pnf->next; pnf->nbface != fa1; pnf = pnf->next); 
     COEFFLL(pnf,Z,0,0) += pn1;
	 COEFFLL(pnf,Z,1,1) += pn1;
  }
}

void putppij_korn(pflink,fa1,fa2,pp1,pp2,Z)
 FLINK *pflink;
 FACE *fa1, *fa2;
 FLOAT pp1, pp2;
 INT Z;
 {
   FLINK *pfli;
   
   for (pfli=pflink; pfli->nbface!=fa1 && pfli->nbface!=fa2; pfli=pfli->next);
   if (pfli->nbface==fa1){
      COEFFLL(pfli,Z,0,0) += pp1;
	  COEFFLL(pfli,Z,1,1) += pp1;
      for (pfli=pfli->next; pfli->nbface!=fa2; pfli=pfli->next); 
	  COEFFLL(pfli,Z,0,0) += pp2;
	  COEFFLL(pfli,Z,1,1) += pp2;
   }
   else {
	  COEFFLL(pfli,Z,0,0) += pp2;
	  COEFFLL(pfli,Z,1,1) += pp2;
      for (pfli=pfli->next; pfli->nbface!=fa1; pfli=pfli->next); 
	  COEFFLL(pfli,Z,0,0) += pp1;
	  COEFFLL(pfli,Z,1,1) += pp1;
   }
 }

/*  stiffness matrix corresponding to \int_\om u_i(v\cdot\nabla u_j) \dx  */
void convij_sn_sf_korn(n0,n1,n2,fa0,fa1,fa2,s,c,v,Z,b0,b1,b2,rdetB)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT v, Z;
FLOAT *s, *c, b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{
   NFLINK *pnf;
   FNLINK *pfn;
   FLOAT *v0, *v1, *v2, *v01, *v02, *v12, s0, s1, s2, c0, c1, c2, p, q, r, 
         v0_0, v0_1, v0_2, v1_0, v2_0, v12_0, 
         v12_1, v12_2, v01_0, v02_0, v01_2, v02_1;
   FLOAT hf;
  
   v0 = NDD(n0,v);
   v1 = NDD(n1,v);
   v2 = NDD(n2,v);
   v01 = FDVP(fa2,v);
   v02 = FDVP(fa1,v);
   v12 = FDVP(fa0,v);
   s0 = DOT(s,b0);
   s1 = DOT(s,b1);
   s2 = DOT(s,b2);
   c0 = DOT(c,b0);
   c1 = DOT(c,b1);
   c2 = DOT(c,b2);
   v0_0 = DOT(v0,b0);
   v0_1 = DOT(v0,b1);
   v0_2 = DOT(v0,b2);
   v1_0 = DOT(v1,b0);
   v2_0 = DOT(v2,b0);
   v12_0 = DOT(v12,b0);
   v12_1 = DOT(v12,b1);
   v12_2 = DOT(v12,b2);
   v01_0 = DOT(v01,b0);
   v02_0 = DOT(v02,b0);
   v01_2 = DOT(v01,b2);
   v02_1 = DOT(v02,b1);
   q = rdetB/180.;
  
   if (IS_FN(n0)){
      p = rdetB/60.;
	  hf = (5.*(s0+v0_0)+c0+c0-v12_0)*p;
      COEFFNN(n0,Z,0,0) += hf;
	  COEFFNN(n0,Z,1,1) += hf;
      putaij_korn(n0->tstart,n1,n2,(5.*(s1+v0_1)+c1+c1-v12_1)*p,
                              (5.*(s2+v0_2)+c2+c2-v12_2)*p,Z);
      putasij_korn(n0->nfstart,fa1,fa2,
                     (-3.*(s1+s1+v1_0-4.*v0_2)-c1+v02_0+c2+c2-v12_2-v12_2)*q,
                     (-3.*(s2+s2+v2_0-4.*v0_1)-c2+v01_0+c1+c1-v12_1-v12_1)*q,Z);
      for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
	  hf = (-3.*(s0+s0+DOT(v1,b1)+DOT(v2,b2))-c0+v02_1+v01_2)*q;
      COEFFLL(pnf,Z,0,0) += hf;
	  COEFFLL(pnf,Z,1,1) += hf;
   }
                                          
   if (IS_FF(fa0)){ 
      p = rdetB/1260.;
      r = 4.*v12_0;
	  hf = (7.*(-s0-s0+v0_0+DOT(v1,b2)+DOT(v2,b1))
                          -c0-c0+v02_1+v01_2-DOT(v01,b1)-DOT(v02,b2)-r)*p;
      COEFFNN(fa0,Z,0,0) += hf;
	  COEFFNN(fa0,Z,1,1) += hf;
      putppij_korn(fa0->tfstart,fa1,fa2,(7.*(-s1+v1_0+v2_0+v2_0)-c1-c1+v02_0+r)*p,
                                   (7.*(-s2+v1_0+v1_0+v2_0)-c2-c2+v01_0+r)*p,Z);
      for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
	  hf = (3.*(s0+s0-v0_0)+c0+v12_0)*q;
      COEFFLL(pfn,Z,0,0) += hf;
	  COEFFLL(pfn,Z,1,1) += hf;
      for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
	  hf = (3.*(s1+s1-v0_1)+c1+v12_1)*q;
      COEFFLL(pfn,Z,0,0) += hf;
	  COEFFLL(pfn,Z,1,1) += hf;
      for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
	  hf = (3.*(s2+s2-v0_2)+c2+v12_2)*q;
      COEFFLL(pfn,Z,0,0) += hf;
	  COEFFLL(pfn,Z,1,1) += hf;
   }
}

#else  /*  if !((N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && 
                (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && 
                (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && 
                (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES) &&
                (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA))  */

void convij_sn_sf_korn(n0,n1,n2,fa0,fa1,fa2,s,c,v,Z,b0,b1,b2,rdetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT v, Z; FLOAT *s, *c, b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{  eprintf("Error: convij_sn_sf_korn not available.\n");  }

#endif

#if (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA) && (MOVING_BOUNDARY == YES)

/*  stiffness matrix corresponding to \int_\om u_i(v\cdot\nabla u_j) \dx  */
void sconv_matr2_korn(tGrid,v,Z)
GRID *tGrid;
INT v, Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT s[DIM], c[DIM], rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      SET14(s,NDD(n0,v),NDD(n1,v),NDD(n2,v));
      SET14(c,FDVP(fa0,v),FDVP(fa1,v),FDVP(fa2,v));
      convij_sn_sf_korn(n0,n1,n2,fa0,fa1,fa2,s,c,v,Z,b[0],b[1],b[2],rdetB);
      convij_sn_sf_korn(n1,n2,n0,fa1,fa2,fa0,s,c,v,Z,b[1],b[2],b[0],rdetB);
      convij_sn_sf_korn(n2,n0,n1,fa2,fa0,fa1,s,c,v,Z,b[2],b[0],b[1],rdetB);
   }
}

#else

void sconv_matr2_korn(tGrid,v,Z)
GRID *tGrid; INT v, Z;
{  eprintf("Error: sconv_matr2_korn not available.\n");  }

#endif

#if MOVING_BOUNDARY == YES

void sstiff_matr2(tGrid,nu,Z)
GRID *tGrid;
FLOAT nu;
INT Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, ndetB, b[DIM2][DIM2];
   FLOAT *x0, *x1, *x2;//, x01[DIM], x02[DIM], x12[DIM], alpha[DIM], *x_cur;
//   FILE *soub;
//   int i=0;
   
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      ndetB = nu*rdetB;
/*      soub=fopen("hranicni_el.txt","a");
      if (fa0->myvertex->type || fa1->myvertex->type || fa2->myvertex->type){
        VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
        fprintf(soub,"je hranicny element %lf,%lf %lf,%lf %lf,%lf ?\n",x0[0],x0[1],x1[0],x1[1],x2[0],x2[1]);
      }
      fclose(soub);*/

      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
#if (IZOPARAMETRIC==YES)
      if (IS_CURVED_EDGE(fa0) || IS_CURVED_EDGE(fa1) || IS_CURVED_EDGE(fa2) ){
        aij_izo(n0,n1,n2,fa0,fa1,fa2,Z,nu);
        aij_izo(n1,n2,n0,fa1,fa2,fa0,Z,nu);
        aij_izo(n2,n0,n1,fa2,fa0,fa1,Z,nu);
      }
      else
#endif
      {
       aij_new(n0,n1,n2,fa0,fa1,fa2,Z,b[0],b[1],b[2],ndetB);
       aij_new(n1,n2,n0,fa1,fa2,fa0,Z,b[1],b[2],b[0],ndetB);
       aij_new(n2,n0,n1,fa2,fa0,fa1,Z,b[2],b[0],b[1],ndetB);
      }
   }
}

#else

void sstiff_matr2(tGrid,nu,Z)
GRID *tGrid; FLOAT nu; INT Z;
{  eprintf("Error: sstiff_matr2 not available.\n");  }

#endif

#if (DATA_STR & KORN_MATRIX) && (N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES) && (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA) && (MOVING_BOUNDARY == YES)

/*  stiff. matrix corresponding to \int_K (v\cdot\nabla u)(v\cdot\nabla v)\dx */
void conv_stab_ij_sn_sf_korn(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i, j, k, Z;
FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], ndetB;
{
   LINK *pli;
   FLINK *pfli;
   NFLINK *pnf;
   FNLINK *pfn;
   FLOAT p, p00, p01, p02;
  
   p = CONV_INT(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i], 
                s[i],c[i],s[i],c[i])*ndetB;
   COEFFNN(n0,Z,0,0) += p;
   COEFFNN(n0,Z,1,1) += p;
   if (n1->index > n0->index){
      p = CONV_INT(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                   vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j], 
                   s[i],c[i],s[j],c[j])*ndetB;
      for (pli = n0->tstart; pli->nbnode != n1; pli=pli->next);
      COEFFLL(pli,Z,0,0) += p;
	  COEFFLL(pli,Z,1,1) += p;
      for (pli = n1->tstart; pli->nbnode != n0; pli=pli->next);
      COEFFLL(pli,Z,0,0) += p;
	  COEFFLL(pli,Z,1,1) += p;
   }
   if (n2->index > n0->index){
      p = CONV_INT(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                   vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k], 
                   s[i],c[i],s[k],c[k])*ndetB;
      for (pli = n0->tstart; pli->nbnode != n2; pli=pli->next);
      COEFFLL(pli,Z,0,0) += p;
	  COEFFLL(pli,Z,1,1) += p;
      for (pli = n2->tstart; pli->nbnode != n0; pli=pli->next);
      COEFFLL(pli,Z,0,0) += p;
	  COEFFLL(pli,Z,1,1) += p;
   }

   p00 = (CONV_INT_L0(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                      vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                      s[i],c[i],s[k],c[k]) +
          CONV_INT_L0(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                      vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                      s[i],c[i],s[j],c[j]))*ndetB;
   p01 = (CONV_INT_L0(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                      vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k], 
                      s[i],c[i],s[k],c[k]) +
          CONV_INT_L0(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                      vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                      s[i],c[i],s[i],c[i]))*ndetB;
   p02 = (CONV_INT_L0(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                      vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j], 
                      s[i],c[i],s[j],c[j]) +
          CONV_INT_L0(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                      vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                      s[i],c[i],s[i],c[i]))*ndetB;
   for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
   COEFFLL(pnf,Z,0,0) += p00;
   COEFFLL(pnf,Z,1,1) += p00;
   putasij_korn(n0->tnfstart,fa1,fa2,p01,p02,Z);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFLL(pfn,Z,0,0) += p00;
   COEFFLL(pfn,Z,1,1) += p00;
   for (pfn=fa1->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFLL(pfn,Z,0,0) += p01;
   COEFFLL(pfn,Z,1,1) += p01;
   for (pfn=fa2->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFLL(pfn,Z,0,0) += p02;
   COEFFLL(pfn,Z,1,1) += p02;
 
   p =   (CONV_INT_L0_L0(vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                         vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                         s[k],c[k],s[k],c[k]) +
       2.*CONV_INT_L1_L2(vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j],
                         vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k], 
                         s[j],c[j],s[k],c[k]) +
          CONV_INT_L0_L0(vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                         vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                         s[j],c[j],s[j],c[j]))*ndetB;
   COEFFNN(fa0,Z,0,0) += p;
   COEFFNN(fa0,Z,1,1) += p;
   if (fa1->index > fa0->index){
      p = (CONV_INT_L1_L2(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                          vn[i][k],vn[j][k],vn[k][k],vf[i][k],vf[j][k],vf[k][k],
                          s[i],c[i],s[k],c[k]) +
           CONV_INT_L1_L2(vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j],
                          vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                          s[j],c[j],s[k],c[k]) +
           CONV_INT_L1_L2(vn[k][k],vn[i][k],vn[j][k],vf[k][k],vf[i][k],vf[j][k],
                          vn[k][k],vn[i][k],vn[j][k],vf[k][k],vf[i][k],vf[j][k],
                          s[k],c[k],s[k],c[k]) +
           CONV_INT_L0_L0(vn[k][i],vn[i][i],vn[j][i],vf[k][i],vf[i][i],vf[j][i],
                          vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                          s[i],c[i],s[j],c[j]))*ndetB;
      for (pfli = fa0->tfstart; pfli->nbface != fa1; pfli = pfli->next);
      COEFFLL(pfli,Z,0,0) += p;
	  COEFFLL(pfli,Z,1,1) += p;
      for (pfli = fa1->tfstart; pfli->nbface != fa0; pfli = pfli->next);
      COEFFLL(pfli,Z,0,0) += p;
	  COEFFLL(pfli,Z,1,1) += p;
   }
   if (fa2->index > fa0->index){
      p = (CONV_INT_L1_L2(vn[i][i],vn[j][i],vn[k][i],vf[i][i],vf[j][i],vf[k][i],
                          vn[i][j],vn[j][j],vn[k][j],vf[i][j],vf[j][j],vf[k][j],
                          s[i],c[i],s[j],c[j]) +
           CONV_INT_L1_L2(vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j],
                          vn[j][j],vn[k][j],vn[i][j],vf[j][j],vf[k][j],vf[i][j],
                          s[j],c[j],s[j],c[j]) +
           CONV_INT_L1_L2(vn[k][j],vn[i][j],vn[j][j],vf[k][j],vf[i][j],vf[j][j],
                          vn[k][k],vn[i][k],vn[j][k],vf[k][k],vf[i][k],vf[j][k],
                          s[j],c[j],s[k],c[k]) +
           CONV_INT_L0_L0(vn[j][i],vn[k][i],vn[i][i],vf[j][i],vf[k][i],vf[i][i],
                          vn[j][k],vn[k][k],vn[i][k],vf[j][k],vf[k][k],vf[i][k],
                          s[i],c[i],s[k],c[k]))*ndetB;
      for (pfli = fa0->tfstart; pfli->nbface != fa2; pfli = pfli->next);
      COEFFLL(pfli,Z,0,0) += p;
	  COEFFLL(pfli,Z,1,1) += p;
      for (pfli = fa2->tfstart; pfli->nbface != fa0; pfli = pfli->next);
      COEFFLL(pfli,Z,0,0) += p;
	  COEFFLL(pfli,Z,1,1) += p;
   }
}

/* stiff.matrix corresponding to \int_K (-nu \Delta u_j)(v\cdot\nabla v_i)\dx */
void lapl_stab_ij_sn_sf_korn(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,b,ndetB)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i, j, k, Z;
FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], b[DIM2][DIM2], ndetB;
{
   NFLINK *pnf;
   FLOAT p, p0, p1, p2;
  
   p0 = 2.*DOT(b[j],b[k]);
   p1 = 2.*DOT(b[i],b[k]);
   p2 = 2.*DOT(b[i],b[j]);
   if (IS_FN(n0)){
      p = -ndetB*(4.*s[i]+c[i])/12.;
      putasij_korn(n0->nfstart,fa1,fa2,p1*p,p2*p,Z);
      for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
      COEFFLL(pnf,Z,0,0) += p0*p;
	  COEFFLL(pnf,Z,1,1) += p0*p;
   }
                                          
   if (IS_FF(fa0)){ 
      p = -(INT_QUADR_L0(vn[j][k],vf[j][k],s[k],c[k]) +
            INT_QUADR_L0(vn[k][j],vf[k][j],s[j],c[j]))*ndetB;
      COEFFNN(fa0,Z,0,0) += p0*p;
	  COEFFNN(fa0,Z,1,1) += p0*p;
      putppij_korn(fa0->tfstart,fa1,fa2,p1*p,p2*p,Z);
   }
}

/* stiff.matrix corresponding to \int_K u_j(v\cdot\nabla v_i)\dx */
void time_stab_ij_sn_sf_korn(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT i, j, k, Z;
FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], ndetB;
{
   NFLINK *pnf;
   FNLINK *pfn;
   FLOAT p;
  
   if (IS_FN(n0)){  
      p = INT_QUADR_L0(vn[i][i],vf[i][i],s[i],c[i])*ndetB;
	  COEFFNN(n0,Z,0,0) += p;
	  COEFFNN(n0,Z,1,1) += p;
      putaij_korn(n0->tstart,n1,n2,INT_QUADR_L0(vn[j][i],vf[j][i],s[i],c[i])*ndetB,
                              INT_QUADR_L0(vn[k][i],vf[k][i],s[i],c[i])*ndetB,Z);
      putasij_korn(n0->nfstart,fa1,fa2,
                          INT_QUADR_L0_L1(vn[j][i],vf[j][i],s[i],c[i])*ndetB,
                          INT_QUADR_L0_L1(vn[k][i],vf[k][i],s[i],c[i])*ndetB,Z);
      for (pnf=n0->tnfstart; pnf->nbface!=fa0; pnf=pnf->next);
      p = INT_QUADR_L0_L1(vn[i][i],vf[i][i],s[i],c[i])*ndetB;
	  COEFFLL(pnf,Z,0,0) += p;
	  COEFFLL(pnf,Z,1,1) += p;
   }
                                          
   if (IS_FF(fa0)){ 
      p = 
      (INT_QUADR_L0_L0_L1(vn[j][k],vn[k][k],vf[i][k],vf[j][k],s[k],c[k]) +
       INT_QUADR_L0_L0_L1(vn[k][j],vn[j][j],vf[i][j],vf[k][j],s[j],c[j]))*ndetB;
	  COEFFNN(fa0,Z,0,0) += p;
	  COEFFNN(fa0,Z,1,1) += p;
      putppij_korn(fa0->tfstart,fa1,fa2,
       ((7.*s[k]+c[k]+c[k])/1260. +
       INT_QUADR_L0_L0_L1(vn[k][j],vn[i][j],vf[j][j],vf[k][j],s[j],c[j]))*ndetB,
      (INT_QUADR_L0_L0_L1(vn[j][k],vn[i][k],vf[k][k],vf[j][k],s[k],c[k]) +
        (7.*s[j]+c[j]+c[j])/1260.)*ndetB,Z);
      for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
      p = (INT_QUADR_L0_L1(vn[k][k],vf[k][k],s[k],c[k]) +
           INT_QUADR_L0_L1(vn[j][j],vf[j][j],s[j],c[j]))*ndetB;
	  COEFFLL(pfn,Z,0,0) += p;
	  COEFFLL(pfn,Z,1,1) += p;
      for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
      p = (INT_QUADR_L0_L0(vn[j][k],vf[j][k],s[k],c[k]) +
           INT_QUADR_L0_L1(vn[i][j],vf[i][j],s[j],c[j]))*ndetB;
	  COEFFLL(pfn,Z,0,0) += p;
	  COEFFLL(pfn,Z,1,1) += p;
      for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
      p = (INT_QUADR_L0_L1(vn[i][k],vf[i][k],s[k],c[k]) +
           INT_QUADR_L0_L0(vn[k][j],vf[k][j],s[j],c[j]))*ndetB;
	  COEFFLL(pfn,Z,0,0) += p;
	  COEFFLL(pfn,Z,1,1) += p;
   }
}

#else  /*  if !((N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && 
                (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && 
                (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && 
                (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES) &&
                (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA))  */

void conv_stab_ij_sn_sf_korn(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i, j, k, Z; FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], ndetB;
{  eprintf("Error: conv_stab_ij_sn_sf not available.\n");  }

void lapl_stab_ij_sn_sf_korn(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,b,ndetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i, j, k, Z; FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], b[DIM2][DIM2], ndetB;
{  eprintf("Error: lapl_stab_ij_sn_sf not available.\n");  }

void time_stab_ij_sn_sf_korn(i,j,k,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,ndetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT i, j, k, Z; FLOAT *s, *c, vn[SIDES][SIDES], vf[SIDES][SIDES], ndetB;
{  eprintf("Error: time_stab_ij_sn_sf not available.\n");  }

#endif

#if (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX) && (MOVING_BOUNDARY == YES)

/*  stiffness matrix corresponding to 
                    \sum_K \delta_K\int_K (v\cdot\nabla u)(v\cdot\nabla v)\dx */
void sconv_stab_matr2_korn(tGrid,nu,v,Z,f,p,u_old,rhs0,rhs1,tau)
GRID *tGrid;
FLOAT nu, (*rhs0)(), (*rhs1)(), tau;
INT v, Z, f, p, u_old;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT s[SIDES], c[SIDES], vn[SIDES][SIDES], vf[SIDES][SIDES], 
         rdetB, b[DIM2][DIM2];
   FLOAT *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], sv[DIM], cv[DIM],
         v0[DIM], v1[DIM], v2[DIM], v01[DIM], v02[DIM], v12[DIM], v_norm,
         u0, u1, u2, u001, u002, u110, u112, u220, u221, u012, 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, 
         hT, delta_T=1., grad_p[DIM], q;
   INT n=0;

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      SET1(v0,NDD(n0,v))
      SET1(v1,NDD(n1,v))
      SET1(v2,NDD(n2,v))
      SET1(v12,FDVP(fa0,v))
      SET1(v02,FDVP(fa1,v))
      SET1(v01,FDVP(fa2,v))
      SET14(sv,v0,v1,v2)
      SET14(cv,v01,v02,v12)
      v_norm = (fabs(v0[0])+fabs(v1[0])+fabs(v2[0])+
                fabs(v0[1])+fabs(v1[1])+fabs(v2[1])+
               (fabs(v01[0]+2.*(v0[0]+v1[0]))+
                fabs(v02[0]+2.*(v0[0]+v2[0]))+
                fabs(v12[0]+2.*(v1[0]+v2[0]))+
                fabs(v01[1]+2.*(v0[1]+v1[1]))+
                fabs(v02[1]+2.*(v0[1]+v2[1]))+
                fabs(v12[1]+2.*(v1[1]+v2[1])))*0.25)/12.;
/*
      v_norm = sqrt((6.*DOT(sv,cv) + DOT(cv,cv)  + 15.*(DOT(v0,v0)+DOT(v1,v1)+
                     DOT(v2,v2)+DOT(v0,v1)+DOT(v1,v2)+DOT(v2,v0)) - 
                     3.*(DOT(v0,v12)+DOT(v1,v02)+DOT(v2,v01)) -
                     (DOT(v01,v02)+DOT(v02,v12)+DOT(v12,v01)))/90.*rdetB);
*/
      hT = max_edge_length(pelem);
      if (v_norm > 1.e-15)
         delta_T = 0.5*hT/v_norm; 
      if (v_norm > 1.e-15 && 0.25*PECLET_FACTOR*hT*hT/nu/delta_T > 1.){
         n++;
         rdetB *= delta_T*DELTA_FACTOR;
         vn[0][0] = DOT(v0,b[0]);
         vn[0][1] = DOT(v0,b[1]);
         vn[0][2] = DOT(v0,b[2]);
         vn[1][0] = DOT(v1,b[0]);
         vn[1][1] = DOT(v1,b[1]);
         vn[1][2] = DOT(v1,b[2]);
         vn[2][0] = DOT(v2,b[0]);
         vn[2][1] = DOT(v2,b[1]);
         vn[2][2] = DOT(v2,b[2]);
         vf[0][0] = DOT(v12,b[0]);
         vf[0][1] = DOT(v12,b[1]);
         vf[0][2] = DOT(v12,b[2]);
         vf[1][0] = DOT(v02,b[0]);
         vf[1][1] = DOT(v02,b[1]);
         vf[1][2] = DOT(v02,b[2]);
         vf[2][0] = DOT(v01,b[0]);
         vf[2][1] = DOT(v01,b[1]);
         vf[2][2] = DOT(v01,b[2]);
         s[0] = vn[0][0] + vn[1][0] + vn[2][0];
         s[1] = vn[0][1] + vn[1][1] + vn[2][1];
         s[2] = vn[0][2] + vn[1][2] + vn[2][2];
         c[0] = vf[0][0] + vf[1][0] + vf[2][0];
         c[1] = vf[0][1] + vf[1][1] + vf[2][1];
         c[2] = vf[0][2] + vf[1][2] + vf[2][2];
         conv_stab_ij_sn_sf_korn(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,rdetB);
         conv_stab_ij_sn_sf_korn(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,rdetB);
         conv_stab_ij_sn_sf_korn(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,rdetB);

         pressure_gradient(n0,n1,n2,p,b,grad_p);
         VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
         points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012);
         q = grad_p[0];
         POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                         f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs0)
         SUBTR_CONST_10( f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,q)
         if (fabs(tau) > 1.e-16){
            q = 1./tau;
            QUADR_VALUES_10(ND(n0,u_old,0),ND(n1,u_old,0),ND(n2,u_old,0),
                            FDV(fa2,u_old,0),FDV(fa1,u_old,0),FDV(fa0,u_old,0),
                            u0,u1,u2,u001,u002,u110,u112,u220,u221,u012)
            MULT_AND_ADD_10_TO_10(f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,
                                  u0,u1,u2,u001,u002,u110,u112,u220,u221,u012,q)
         }
         ND(n0,f,0) += CONV_RHS_INT(vn[0][0],vn[1][0],vn[2][0],
                                    vf[0][0],vf[1][0],vf[2][0],f0,f1,f2,
                                    f001,f002,f110,f112,f220,f221,f012)*rdetB; 
         ND(n1,f,0) += CONV_RHS_INT(vn[0][1],vn[1][1],vn[2][1],
                                    vf[0][1],vf[1][1],vf[2][1],f0,f1,f2,
                                    f001,f002,f110,f112,f220,f221,f012)*rdetB; 
         ND(n2,f,0) += CONV_RHS_INT(vn[0][2],vn[1][2],vn[2][2],
                                    vf[0][2],vf[1][2],vf[2][2],f0,f1,f2,
                                    f001,f002,f110,f112,f220,f221,f012)*rdetB; 
         FDV(fa0,f,0) += (CONV_RHS_INT_L0(vn[1][2],vn[2][2],vn[0][2],
                                          vf[1][2],vf[2][2],vf[0][2],f1,f2,f0,
                                          f112,f110,f221,f220,f001,f002,f012) +
                          CONV_RHS_INT_L0(vn[2][1],vn[0][1],vn[1][1],
                                          vf[2][1],vf[0][1],vf[1][1],f2,f0,f1,
                                          f220,f221,f002,f001,f112,f110,f012))*rdetB; 
         FDV(fa1,f,0) += (CONV_RHS_INT_L0(vn[0][2],vn[1][2],vn[2][2],
                                          vf[0][2],vf[1][2],vf[2][2],f0,f1,f2,
                                          f001,f002,f110,f112,f220,f221,f012) +
                          CONV_RHS_INT_L0(vn[2][0],vn[0][0],vn[1][0],
                                          vf[2][0],vf[0][0],vf[1][0],f2,f0,f1,
                                          f220,f221,f002,f001,f112,f110,f012))*rdetB; 
         FDV(fa2,f,0) += (CONV_RHS_INT_L0(vn[0][1],vn[1][1],vn[2][1],
                                          vf[0][1],vf[1][1],vf[2][1],f0,f1,f2,
                                          f001,f002,f110,f112,f220,f221,f012) +
                          CONV_RHS_INT_L0(vn[1][0],vn[2][0],vn[0][0],
                                          vf[1][0],vf[2][0],vf[0][0],f1,f2,f0,
                                          f112,f110,f221,f220,f001,f002,f012))*rdetB; 
         q = grad_p[1];
         POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                         f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs1)
         SUBTR_CONST_10( f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,q)
         if (fabs(tau) > 1.e-16){
            q = 1./tau;
            QUADR_VALUES_10(ND(n0,u_old,1),ND(n1,u_old,1),ND(n2,u_old,1),
                            FDV(fa2,u_old,1),FDV(fa1,u_old,1),FDV(fa0,u_old,1),
                            u0,u1,u2,u001,u002,u110,u112,u220,u221,u012)
            MULT_AND_ADD_10_TO_10(f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,
                                  u0,u1,u2,u001,u002,u110,u112,u220,u221,u012,q)
         }
         ND(n0,f,1) += CONV_RHS_INT(vn[0][0],vn[1][0],vn[2][0],
                                    vf[0][0],vf[1][0],vf[2][0],f0,f1,f2,
                                    f001,f002,f110,f112,f220,f221,f012)*rdetB; 
         ND(n1,f,1) += CONV_RHS_INT(vn[0][1],vn[1][1],vn[2][1],
                                    vf[0][1],vf[1][1],vf[2][1],f0,f1,f2,
                                    f001,f002,f110,f112,f220,f221,f012)*rdetB; 
         ND(n2,f,1) += CONV_RHS_INT(vn[0][2],vn[1][2],vn[2][2],
                                    vf[0][2],vf[1][2],vf[2][2],f0,f1,f2,
                                    f001,f002,f110,f112,f220,f221,f012)*rdetB; 
         FDV(fa0,f,1) += (CONV_RHS_INT_L0(vn[1][2],vn[2][2],vn[0][2],
                                          vf[1][2],vf[2][2],vf[0][2],f1,f2,f0,
                                          f112,f110,f221,f220,f001,f002,f012) +
                          CONV_RHS_INT_L0(vn[2][1],vn[0][1],vn[1][1],
                                          vf[2][1],vf[0][1],vf[1][1],f2,f0,f1,
                                          f220,f221,f002,f001,f112,f110,f012))*rdetB; 
         FDV(fa1,f,1) += (CONV_RHS_INT_L0(vn[0][2],vn[1][2],vn[2][2],
                                          vf[0][2],vf[1][2],vf[2][2],f0,f1,f2,
                                          f001,f002,f110,f112,f220,f221,f012) +
                          CONV_RHS_INT_L0(vn[2][0],vn[0][0],vn[1][0],
                                          vf[2][0],vf[0][0],vf[1][0],f2,f0,f1,
                                          f220,f221,f002,f001,f112,f110,f012))*rdetB; 
         FDV(fa2,f,1) += (CONV_RHS_INT_L0(vn[0][1],vn[1][1],vn[2][1],
                                          vf[0][1],vf[1][1],vf[2][1],f0,f1,f2,
                                          f001,f002,f110,f112,f220,f221,f012) +
                          CONV_RHS_INT_L0(vn[1][0],vn[2][0],vn[0][0],
                                          vf[1][0],vf[2][0],vf[0][0],f1,f2,f0,
                                          f112,f110,f221,f220,f001,f002,f012))*rdetB; 
         
         if (fabs(tau) > 1.e-16){
            q = rdetB/tau;
            time_stab_ij_sn_sf_korn(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,q);
            time_stab_ij_sn_sf_korn(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,q);
            time_stab_ij_sn_sf_korn(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,q);
         }
         rdetB *= nu;
         lapl_stab_ij_sn_sf_korn(0,1,2,n0,n1,n2,fa0,fa1,fa2,Z,s,c,vn,vf,b,rdetB);
         lapl_stab_ij_sn_sf_korn(1,2,0,n1,n2,n0,fa1,fa2,fa0,Z,s,c,vn,vf,b,rdetB);
         lapl_stab_ij_sn_sf_korn(2,0,1,n2,n0,n1,fa2,fa0,fa1,Z,s,c,vn,vf,b,rdetB);
      }
   }
   printf("SDFEM on %i elements.\n",n);
}

#else

void sconv_stab_matr2_korn(tGrid,nu,v,Z,f,p,u_old,rhs0,rhs1,tau)
GRID *tGrid;
FLOAT nu, (*rhs0)(), (*rhs1)(), tau;
INT v, Z, f, p, u_old;
{ eprintf("Error: sconv_stab_matr2_korn not available.\n"); }

#endif

#if (DATA_STR & KORN_MATRIX) && (N_DATA & ONE_NODE_MATR) && (N_DATA & ONE_NODE_FACE_MATR) && (F_DATA & ONE_FACE_MATR) && (F_DATA & ONE_FACE_NODE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES) && (DATA_S & F_LINK_TO_FACES) && (DATA_S & F_LINK_TO_NODES) && (MOVING_BOUNDARY == YES)

void p2c_up_aij_korn(n0,n1,n2,fa0,fa1,fa2,nu,Z,flux0,flux1,flux2)
NODE *n0, *n1, *n2;
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT nu, flux0, flux1, flux2;
{
   NFLINK *pnf;
   FNLINK *pfn;
   FLOAT a0=0., a1, a2, b0, b1, b2, c0, c1, c2;

   if (IS_BF(fa0))
      a0 = flux0*(1. - 2.*upwind_lambda(flux0,nu))/12.;
   a1 = 0.25*flux1*(1. - upwind_lambda(flux1,nu));
   a2 = 0.25*flux2*(1. - upwind_lambda(flux2,nu));
   b0 = a1 + a2 - a0 - a0;
   b1 = a0 - a1;
   b2 = a0 - a2;
   a0 *= 4./3.;
   c0 = 0.5*(a0*1.25 - a1 - a2);
   c1 = 0.5*(a1 - a0); 
   c2 = 0.5*(a2 - a0); 
   COEFFNN(n1,Z,0,0) += b1;
   COEFFNN(n1,Z,1,1) += b1;
   COEFFNN(n2,Z,0,0) += b2;
   COEFFNN(n2,Z,1,1) += b2;
   putaij_korn(n1->tstart,n0,n2,b0,b2,Z);
   putaij_korn(n2->tstart,n0,n1,b0,b1,Z);
   putasij_korn(n1->tnfstart,fa0,fa2,c0,c2,Z);
   putasij_korn(n2->tnfstart,fa0,fa1,c0,c1,Z);
   for (pnf=n1->tnfstart; pnf->nbface!=fa1; pnf=pnf->next);
   COEFFLL(pnf,Z,0,0) += c1;
   COEFFLL(pnf,Z,1,1) += c1;
   for (pnf=n2->tnfstart; pnf->nbface!=fa2; pnf=pnf->next);
   COEFFLL(pnf,Z,0,0) += c2;
   COEFFLL(pnf,Z,1,1) += c2;
   COEFFNN(fa0,Z,0,0) += c0*0.5;
   COEFFNN(fa0,Z,1,1) += c0*0.5;
   putppij_korn(fa0->tfstart,fa1,fa2,c1*0.5,c2*0.5,Z);
   for (pfn=fa0->tfnstart; pfn->nbnode!=n0; pfn=pfn->next);
   COEFFLL(pfn,Z,0,0) += b0*0.5;
   COEFFLL(pfn,Z,1,1) += b0*0.5;
   for (pfn=fa0->tfnstart; pfn->nbnode!=n1; pfn=pfn->next);
   COEFFLL(pfn,Z,0,0) += b1*0.5;
   COEFFLL(pfn,Z,1,1) += b1*0.5;
   for (pfn=fa0->tfnstart; pfn->nbnode!=n2; pfn=pfn->next);
   COEFFLL(pfn,Z,0,0) += b2*0.5;
   COEFFLL(pfn,Z,1,1) += b2*0.5;
}

void p2c_upwind_matr_korn(tGrid,nu,v,Z)
GRID *tGrid;
FLOAT nu;
INT v, Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT c[DIM], a0, a1, a2, b0=0., b1=0., b2=0., b[DIM2][DIM2], rdetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      if (IS_BF(fa0))
         b0 = -( (ND(n1,v,0)+ND(n2,v,0)+FDV(fa0,v,0)/3.)*b[0][0] 
               + (ND(n1,v,1)+ND(n2,v,1)+FDV(fa0,v,1)/3.)*b[0][1] )*rdetB;
      if (IS_BF(fa1))
         b1 = -( (ND(n0,v,0)+ND(n2,v,0)+FDV(fa1,v,0)/3.)*b[1][0] 
               + (ND(n0,v,1)+ND(n2,v,1)+FDV(fa1,v,1)/3.)*b[1][1] )*rdetB;
      if (IS_BF(fa2))
         b2 = -( (ND(n0,v,0)+ND(n1,v,0)+FDV(fa2,v,0)/3.)*b[2][0] 
               + (ND(n0,v,1)+ND(n1,v,1)+FDV(fa2,v,1)/3.)*b[2][1] )*rdetB;
      c[0] = 3.*( ND( n0,v,0) +  ND( n1,v,0) +  ND( n2,v,0)) +
             5.*(FDV(fa0,v,0) + FDV(fa1,v,0) + FDV(fa2,v,0))/3.;
      c[1] = 3.*( ND( n0,v,1) +  ND( n1,v,1) +  ND( n2,v,1)) +
             5.*(FDV(fa0,v,1) + FDV(fa1,v,1) + FDV(fa2,v,1))/3.;
      rdetB /= 27.;
      a0 = ( (9.*ND(n0,v,0)-FDV(fa0,v,0)+c[0])*(b[1][0]-b[2][0])
           + (9.*ND(n0,v,1)-FDV(fa0,v,1)+c[1])*(b[1][1]-b[2][1]) )*rdetB;
      a1 = ( (9.*ND(n1,v,0)-FDV(fa1,v,0)+c[0])*(b[2][0]-b[0][0])
           + (9.*ND(n1,v,1)-FDV(fa1,v,1)+c[1])*(b[2][1]-b[0][1]) )*rdetB;
      a2 = ( (9.*ND(n2,v,0)-FDV(fa2,v,0)+c[0])*(b[0][0]-b[1][0])
           + (9.*ND(n2,v,1)-FDV(fa2,v,1)+c[1])*(b[0][1]-b[1][1]) )*rdetB;
      p2c_up_aij_korn(n0,n1,n2,fa0,fa1,fa2,nu,Z,b0,a2,-a1);
      p2c_up_aij_korn(n1,n2,n0,fa1,fa2,fa0,nu,Z,b1,a0,-a2);
      p2c_up_aij_korn(n2,n0,n1,fa2,fa0,fa1,nu,Z,b2,a1,-a0);
   } 
}

#else

void p2c_upwind_matr_korn(tGrid,nu,v,Z)
GRID *tGrid; FLOAT nu; INT v, Z;
{  eprintf("Error: p2c_upwind_matr_korn not available.\n");  }

#endif

/******************************************************************************/
/*                                                                            */
/*               2D stiffness matrices for free boundary problem              */
/*                              by Martin Sodomka                             */
/******************************************************************************/

#define QSIM	0.11785113019775792073347406035081	// sqrt(2)/12

#define INT_DIV(integral,u,v,f,i) \
{ integral = 0.;\
  for(i = 0; i < MAX_KVADR_POINTS; i++) \
    integral += P_KVADR_WEIGHT[i] * v(P_KVADR_ALPHA[i],P_KVADR_BETA[i]) * \
                u(P_KVADR_ALPHA[i],P_KVADR_BETA[i]) * f[i]; \
  integral /= 2.;\
}

#if (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA) && (MOVING_BOUNDARY == YES)

void ci_fb(f0, n0, n1, n2, Z)
FACE *f0;
NODE *n0, *n1, *n2;
int Z;
{
	FLOAT a[2], b[2], c[2], fl[2], fm[2], fr[2], c1[2];
	FLOAT dl, dm, dr;

	SET11(a, n1->myvertex->x, n0->myvertex->x);	// a = n1 - n0
	SET11(c, n2->myvertex->x, n0->myvertex->x);	// c = n2 - n0
	SET11(fm, n1->myvertex->x, n2->myvertex->x);	// fm = a - c
	b[0] = 4.*f0->myvertex->x[0] - 2.*(n1->myvertex->x[0] + n2->myvertex->x[0]);
	b[1] = 4.*f0->myvertex->x[1] - 2.*(n1->myvertex->x[1] + n2->myvertex->x[1]);

	dl = c[1]*(a[0]+b[0]) - c[0]*(a[1]+b[1]);
	dr = a[0]*(c[1]+b[1]) - a[1]*(c[0]+b[0]);
	dm = (dl+dr)/2.;
	dl = fabs(dl); dr = fabs(dr); dm = fabs(dm);

	SET10(fl, fm, b);	// fl = fm + b
	SET11(fr, fm, b);
	SET2(fl, fl, dl);	// fl = fl*dl
	SET2(fr, fr, dr);

	c1[0] = QSIM*(fl[0] + 4*fm[0]*dm + fr[0]);
	c1[1] = QSIM*(fl[1] + 4*fm[1]*dm + fr[1]);

	ND(n1,Z,0) += c1[0];
	ND(n1,Z,1) += c1[1];
	ND(n2,Z,0) -= c1[0];
	ND(n2,Z,1) -= c1[1];
	FDV(f0,Z,0) += QSIM*(fl[0] - fr[0]);
	FDV(f0,Z,1) += QSIM*(fl[1] - fr[1]);
}
#else

void ci_fb(f0, n0, n1, n2, Z)
FACE *f0;
NODE *n0, *n1, *n2;
int Z;
{
	eprintf("ci_fb not available.\n");
}
#endif

#if MOVING_BOUNDARY == YES

void bc_free_boundary_IP2C(tGrid, Z)
GRID *tGrid;
INT Z;
{
	ELEMENT *pel;
	NODE *n0, *n1, *n2;
	FACE *f0, *f1, *f2;

	for (pel = FIRSTELEMENT(tGrid); pel != NULL; pel = pel->succ) {
		if (IS_TOP_ELEMENT(pel)) {
			NODES_OF_ELEMENT(n0, n1, n2, pel);
			FACES_OF_ELEMENT(f0, f1, f2, pel);
			if (IS_CURVED_EDGE(f0)) {
				ci_fb(f0, n0, n1, n2, Z);
			} else
				if (IS_CURVED_EDGE(f1)) {
					ci_fb(f1, n1, n0, n2, Z);
				} else
					if (IS_CURVED_EDGE(f2)) {
						ci_fb(f2, n2, n0, n1, Z);
					}
		}
	}
}

#else

void bc_free_boundary_IP2C(tGrid, Z)
GRID *tGrid; INT Z;
{  eprintf("Error: bc_free_boundary_IP2C not available.\n");  }

#endif

/*	Adds boundary integral term
*/
void bc_free_boundary(tGrid, Z, u_space, structure)
GRID *tGrid;
INT Z, u_space;
{
	switch (u_space) {
	case IP2C:
		if (structure == VECTOR)
			bc_free_boundary_IP2C(tGrid, Z);
		else
			eprintf("Error: bc_free_boundary not available!\n");
		break;
	default:
		eprintf("Error: bc_free_boundary not available!\n");
		break;
	}
}

/* making BD matrix: */
#ifdef B_STRUCT
#if (B_STRUCT & Q_BDAUGMENT) && (ELEMENT_TYPE == SIMPLEX) && (MOVING_BOUNDARY == YES)

void put_dij(n1,n2,f0,Z)
NODE *n1, *n2;
FACE *f0;
INT Z;
{
	LINK *pli;
    NFLINK *pnf;
    FNLINK *pfn;
    FLOAT n[2], len, d;

	len = normal_vector(n1->myvertex->x,n2->myvertex->x,f0,n);
	if (IS_FN(n1)) {
		d = len/3.;
		COEFFD(n1,Z,0) += n[0]*d;
		COEFFD(n1,Z,1) += n[1]*d;
		d /= 2.;
		if (IS_FN(n2)) {
			for (pli = TSTART(n1); pli->nbnode != n2; pli = pli->next);
			COEFFD(pli,Z,0) += n[0]*d;
			COEFFD(pli,Z,1) += n[1]*d;
		}
		d /= 2.;
		for (pnf = TNFSTART(n1); pnf->nbface != f0; pnf = pnf->next);
		COEFFD(pnf,Z,0) += n[0]*d;
		COEFFD(pnf,Z,1) += n[1]*d;
		for (pfn = TFNSTART(f0); pfn->nbnode != n1; pfn = pfn->next);
		COEFFD(pfn,Z,0) += n[0]*d;
		COEFFD(pfn,Z,1) += n[1]*d;
	}
    if (IS_FN(n2)) {
		d = len/3.;
		COEFFD(n2,Z,0) += n[0]*d;
		COEFFD(n2,Z,1) += n[1]*d;
		d /= 2.;
		if (IS_FN(n1)) {
			for (pli = TSTART(n2); pli->nbnode != n1; pli = pli->next);
			COEFFD(pli,Z,0) = n[0]*d;
			COEFFD(pli,Z,1) = n[1]*d;
		}
		d /= 2.;
		for (pnf = TNFSTART(n2); pnf->nbface != f0; pnf = pnf->next);
		COEFFD(pnf,Z,0) += n[0]*d;
		COEFFD(pnf,Z,1) += n[1]*d;
		for (pfn = TFNSTART(f0); pfn->nbnode != n2; pfn = pfn->next);
		COEFFD(pfn,Z,0) += n[0]*d;
		COEFFD(pfn,Z,1) += n[1]*d;
	}
	d = len/30.;
	COEFFD(f0,Z,0) += n[0]*d;
	COEFFD(f0,Z,1) += n[1]*d;
}

void add_D_matrix(n0,n1,n2,f0,f1,f2,Z)
NODE *n0, *n1, *n2;
FACE *f0, *f1, *f2;
INT Z;
{
    if (IS_ZNF(f0)) put_dij(n1,n2,f0,Z);
    else 
		if (IS_ZNF(f1)) put_dij(n0,n2,f1,Z);
		else
			if (IS_ZNF(f2)) put_dij(n0,n1,f2,Z);
}
#else

void add_D_matrix(n0,n1,n2,f0,f1,f2,Z)
NODE *n0, *n1, *n2;
FACE *f0, *f1, *f2;
INT Z;
{}

#endif
#endif

#if MOVING_BOUNDARY == YES

void domain_velocity_IP2Cv(tGrid,Z,tau)
GRID *tGrid;
INT Z;
FLOAT tau;
{
	ELEMENT *pel;
	NODE *n0, *n1, *n2;
	FACE *f0, *f1, *f2;
	FLOAT c = 4./tau, xij[2], xij_new[2];

	for (pel = FIRSTELEMENT(tGrid); pel; pel=pel->succ) {
		NODES_OF_ELEMENT(n0,n1,n2,pel);
		FACES_OF_ELEMENT(f0,f1,f2,pel);
		ND(n0,Z,0) = (n0->newvertex->x[0] - n0->myvertex->x[0])/tau;
		ND(n0,Z,1) = (n0->newvertex->x[1] - n0->myvertex->x[1])/tau;
		ND(n1,Z,0) = (n1->newvertex->x[0] - n1->myvertex->x[0])/tau;
		ND(n1,Z,1) = (n1->newvertex->x[1] - n1->myvertex->x[1])/tau;
		ND(n2,Z,0) = (n2->newvertex->x[0] - n2->myvertex->x[0])/tau;
		ND(n2,Z,1) = (n2->newvertex->x[1] - n2->myvertex->x[1])/tau;
		if (IS_CURVED_EDGE(f0)) {
			SET1(xij,f0->myvertex->x);
			SET1(xij_new,f0->newvertex->x);
		} else {
			AVERAGE(n1->myvertex->x,n2->myvertex->x,xij);
			AVERAGE(n1->newvertex->x,n2->newvertex->x,xij_new);
		}
		FDV(f0,Z,0) = c*(xij_new[0] - xij[0]) - 2.*(ND(n1,Z,0)+ND(n2,Z,0));
		FDV(f0,Z,1) = c*(xij_new[1] - xij[1]) - 2.*(ND(n1,Z,1)+ND(n2,Z,1));
		if (IS_CURVED_EDGE(f1)) {
			SET1(xij,f1->myvertex->x);
			SET1(xij_new,f1->newvertex->x);
		} else {
			AVERAGE(n0->myvertex->x,n2->myvertex->x,xij);
			AVERAGE(n0->newvertex->x,n2->newvertex->x,xij_new);
		}
		FDV(f1,Z,0) = c*(xij_new[0] - xij[0]) - 2.*(ND(n0,Z,0)+ND(n2,Z,0));
		FDV(f1,Z,1) = c*(xij_new[1] - xij[1]) - 2.*(ND(n0,Z,1)+ND(n2,Z,1));
		if (IS_CURVED_EDGE(f2)) {
			SET1(xij,f2->myvertex->x);
			SET1(xij_new,f2->newvertex->x);
		} else {
			AVERAGE(n1->myvertex->x,n0->myvertex->x,xij);
			AVERAGE(n1->newvertex->x,n0->newvertex->x,xij_new);
		}
		FDV(f2,Z,0) = c*(xij_new[0] - xij[0]) - 2.*(ND(n1,Z,0)+ND(n0,Z,0));
		FDV(f2,Z,1) = c*(xij_new[1] - xij[1]) - 2.*(ND(n1,Z,1)+ND(n0,Z,1));
	}
}

void make_domain_velocity(tGrid,Z,tau,space,structure)
GRID *tGrid;
INT Z, space, structure;
FLOAT tau;
{
	switch (space) {
	case IP2C:
		if (structure == VECTOR)
			domain_velocity_IP2Cv(tGrid,Z,tau);
		else
			eprintf("Error: make_domain_velocity not available!\n");
		break;
	default:
		eprintf("Error: make_domain_velocity not available!\n");
	}
}

void divij_korn(n0,n1,n2,f0,f1,f2,Z,w)
NODE *n0, *n1, *n2;
FACE *f0, *f1, *f2;
INT Z, w;
{
	LINK *pli;
	FLINK *pfl;
	NFLINK *pnf;
	FNLINK *pfn;
	INT i;
	FLOAT *x0, *x1, *x2, *alpha, d00, d01, d10, d11, sign, integral;
	FLOAT f[MAX_KVADR_POINTS], l0;

	x0 = n0->myvertex->x;
	x1 = n1->myvertex->x;
	x2 = n2->myvertex->x;

	if (IS_CURVED_EDGE(f0)) {
		alpha = f0->myvertex->x;
		for(i = 0; i < MAX_KVADR_POINTS; i++){
			j0_izo(P_KVADR_ALPHA[i],P_KVADR_BETA[i],alpha,x0,x1,x2,&d00,&d01,&d10,&d11,&sign);
			l0 = 1. - P_KVADR_ALPHA[i] - P_KVADR_BETA[i];
			f[i] = d00*(ND(n1,w,0) + FDV(f0,w,0)*P_KVADR_BETA[i] + FDV(f2,w,0)*l0)
			     + d10*(ND(n2,w,0) + FDV(f0,w,0)*P_KVADR_ALPHA[i] + FDV(f1,w,0)*l0)
			     + d01*(ND(n1,w,1) + FDV(f0,w,1)*P_KVADR_BETA[i] + FDV(f2,w,1)*l0)
			     + d11*(ND(n2,w,1) + FDV(f0,w,1)*P_KVADR_ALPHA[i] + FDV(f1,w,1)*l0);
			f[i] *= sign;
	    }
	} else if (IS_CURVED_EDGE(f1)) {
		alpha = f1->myvertex->x;
		for(i = 0; i < MAX_KVADR_POINTS; i++){
			j1_izo(P_KVADR_ALPHA[i],P_KVADR_BETA[i],alpha,x0,x1,x2,&d00,&d01,&d10,&d11,&sign);
			l0 = 1. - P_KVADR_ALPHA[i] - P_KVADR_BETA[i];
			f[i] = d00*(ND(n1,w,0) + FDV(f0,w,0)*P_KVADR_BETA[i] + FDV(f2,w,0)*l0)
			     + d10*(ND(n2,w,0) + FDV(f0,w,0)*P_KVADR_ALPHA[i] + FDV(f1,w,0)*l0)
			     + d01*(ND(n1,w,1) + FDV(f0,w,1)*P_KVADR_BETA[i] + FDV(f2,w,1)*l0)
			     + d11*(ND(n2,w,1) + FDV(f0,w,1)*P_KVADR_ALPHA[i] + FDV(f1,w,1)*l0);
			f[i] *= sign;
	    }
	} else if (IS_CURVED_EDGE(f2)) {
		alpha = f2->myvertex->x;
		for(i = 0; i < MAX_KVADR_POINTS; i++){
			j2_izo(P_KVADR_ALPHA[i],P_KVADR_BETA[i],alpha,x0,x1,x2,&d00,&d01,&d10,&d11,&sign);
			l0 = 1. - P_KVADR_ALPHA[i] - P_KVADR_BETA[i];
			f[i] = d00*(ND(n1,w,0) + FDV(f0,w,0)*P_KVADR_BETA[i] + FDV(f2,w,0)*l0)
			     + d10*(ND(n2,w,0) + FDV(f0,w,0)*P_KVADR_ALPHA[i] + FDV(f1,w,0)*l0)
			     + d01*(ND(n1,w,1) + FDV(f0,w,1)*P_KVADR_BETA[i] + FDV(f2,w,1)*l0)
			     + d11*(ND(n2,w,1) + FDV(f0,w,1)*P_KVADR_ALPHA[i] + FDV(f1,w,1)*l0);
			f[i] *= sign;
	    }
	} else {
		alpha = f0->myvertex->x;
		for(i = 0; i < MAX_KVADR_POINTS; i++){
			j_afi(P_KVADR_ALPHA[i],P_KVADR_BETA[i],alpha,x0,x1,x2,&d00,&d01,&d10,&d11,&sign);
			l0 = 1. - P_KVADR_ALPHA[i] - P_KVADR_BETA[i];
			f[i] = d00*(ND(n1,w,0) + FDV(f0,w,0)*P_KVADR_BETA[i] + FDV(f2,w,0)*l0)
			     + d10*(ND(n2,w,0) + FDV(f0,w,0)*P_KVADR_ALPHA[i] + FDV(f1,w,0)*l0)
			     + d01*(ND(n1,w,1) + FDV(f0,w,1)*P_KVADR_BETA[i] + FDV(f2,w,1)*l0)
			     + d11*(ND(n2,w,1) + FDV(f0,w,1)*P_KVADR_ALPHA[i] + FDV(f1,w,1)*l0);
			f[i] *= sign;
	    }
	}

	if(IS_FN(n0)) {
	    INT_DIV(integral,l0_ref,l0_ref,f,i)
		COEFFLL(n0,Z,0,0) += integral;
	    COEFFLL(n0,Z,1,1) += integral;

		for(pli=n0->tstart;pli->nbnode!=n1;pli=pli->next);
	    INT_DIV(integral,l1_ref,l0_ref,f,i)
	    COEFFLL(pli,Z,0,0) += integral;
		COEFFLL(pli,Z,1,1) += integral;
      
	    for(pli=n0->tstart;pli->nbnode!=n2;pli=pli->next);
	    INT_DIV(integral,l2_ref,l0_ref,f,i)
		COEFFLL(pli,Z,0,0) += integral;
		COEFFLL(pli,Z,1,1) += integral;
		   
		for(pnf=n0->tnfstart;pnf->nbface!=f0;pnf=pnf->next);
		INT_DIV(integral,l1l2_ref,l0_ref,f,i)
		COEFFLL(pnf,Z,0,0) += integral;
		COEFFLL(pnf,Z,1,1) += integral;
			   
		for(pnf=n0->tnfstart;pnf->nbface!=f1;pnf=pnf->next);
		INT_DIV(integral,l0l2_ref,l0_ref,f,i)
		COEFFLL(pnf,Z,0,0) += integral;
		COEFFLL(pnf,Z,1,1) += integral;
			   
		for(pnf=n0->tnfstart;pnf->nbface!=f2;pnf=pnf->next);
		INT_DIV(integral,l0l1_ref,l0_ref,f,i)
		COEFFLL(pnf,Z,0,0) += integral;
		COEFFLL(pnf,Z,1,1) += integral;
	}
	if(IS_FF(f0)){
		INT_DIV(integral,l1l2_ref,l1l2_ref,f,i)
		COEFFLL(f0,Z,0,0) += integral;
		COEFFLL(f0,Z,1,1) += integral;
			   
		for(pfl=f0->tfstart;pfl->nbface!=f1;pfl=pfl->next);
		INT_DIV(integral,l0l2_ref,l1l2_ref,f,i)
		COEFFLL(pfl,Z,0,0) += integral;
		COEFFLL(pfl,Z,1,1) += integral;
			   
		for(pfl=f0->tfstart;pfl->nbface!=f2;pfl=pfl->next);
		INT_DIV(integral,l0l1_ref,l1l2_ref,f,i)
		COEFFLL(pfl,Z,0,0) += integral;
		COEFFLL(pfl,Z,1,1) += integral;
			   
		for(pfn=f0->tfnstart;pfn->nbnode!=n0;pfn=pfn->next);
		INT_DIV(integral,l0_ref,l1l2_ref,f,i)
		COEFFLL(pfn,Z,0,0) += integral;
		COEFFLL(pfn,Z,1,1) += integral;
			   
		for(pfn=f0->tfnstart;pfn->nbnode!=n1;pfn=pfn->next);
		INT_DIV(integral,l1_ref,l1l2_ref,f,i)
		COEFFLL(pfn,Z,0,0) += integral;
		COEFFLL(pfn,Z,1,1) += integral;
			   
		for(pfn=f0->tfnstart;pfn->nbnode!=n2;pfn=pfn->next);
		INT_DIV(integral,l2_ref,l1l2_ref,f,i)
		COEFFLL(pfn,Z,0,0) += integral;
		COEFFLL(pfn,Z,1,1) += integral;
	}
}

void add_divw_IP2Cv_korn(tGrid,Z,w)
GRID *tGrid;
INT Z, w;
{
	ELEMENT *pel;
	NODE *n0, *n1, *n2;
	FACE *f0, *f1, *f2;

	for (pel = FIRSTELEMENT(tGrid); pel; pel=pel->succ) {
		NODES_OF_ELEMENT(n0,n1,n2,pel);
		FACES_OF_ELEMENT(f0,f1,f2,pel);
		divij_korn(n0,n1,n2,f0,f1,f2,Z,w);
		divij_korn(n2,n0,n1,f2,f0,f1,Z,w);
		divij_korn(n1,n2,n0,f1,f2,f0,Z,w);
	}
}

// \int_\omega uv div(w)
void add_divw_matrix(tGrid,Z,w,space,A_struct,vstruct)
GRID *tGrid;
INT Z, w, space, A_struct, vstruct;
{
	switch (space) {
	case IP2C:
		if ((A_struct & Q_FULL) && vstruct == VECTOR)
			add_divw_IP2Cv_korn(tGrid,Z,w);
		else
			eprintf("Error: add_divw_matrix not available!\n");
		break;
	default:
		eprintf("Error: add_divw_matrix not available!\n");
	}
}

/*********************/
/*  convective term  -je to spatne!!*/
/*********************/
void add_conv(n1,n2,n3,f1,f2,f3,tau,Z)
NODE *n1,*n2,*n3;
FACE *f1,*f2,*f3;
FLOAT tau;
INT Z;
{
	LINK *pl;
	FLINK *pfl;
	NFLINK *pnf;
	FNLINK *pfn;
	FLOAT *x1, *x2, *x3, *x1_new, *x2_new, *x3_new;
	FLOAT w[3][2], divw, divw_2, d, d10, d21;
	FLOAT int_iw[3][2], int_ijw[3][3][2], int_ijkw[7][2];
	FLOAT vol, vol_6, vol_30, vol_90, b[DIM2][DIM2];
	INT i;

	x1 = n1->myvertex->x;
	x2 = n2->myvertex->x;
	x3 = n3->myvertex->x;
	x1_new = n1->newvertex->x;
	x2_new = n2->newvertex->x;
	x3_new = n3->newvertex->x;

	vol = barycentric_coordinates(x1, x2, x3, b);
	vol_6  = vol/6.;
	vol_30 = vol/30.;
	vol_90 = vol/90.;

	SET12(w[0],x1_new,x1,tau);	//w[0] = (x1_new - x1)/tau
	SET12(w[1],x2_new,x2,tau);
	SET12(w[2],x3_new,x3,tau);

	d = x1[0]*(x2[1] - x3[1]) + x2[0]*(x3[1] - x1[1]) + x3[0]*(x1[1] - x2[1]);
	d10 = x1_new[0]*(x2[1] - x3[1]) + x2_new[0]*(x3[1] - x1[1]) + x3_new[0]*(x1[1] - x2[1]);
	d21 = x1[0]*(x2_new[1] - x3_new[1]) + x2[0]*(x3_new[1] - x1_new[1]) + 
		  x3[0]*(x1_new[1] - x2_new[1]);
	divw = ((d10 + d21)/d - 2.)/tau;
	divw_2 = divw/2.;

	for (i = 0; i < 2; i++) {
		/* |E|/6 */
		int_iw[0][i] = w[0][i] + (w[1][i] + w[2][i])/2.;
		int_iw[1][i] = w[1][i] + (w[0][i] + w[2][i])/2.;
		int_iw[2][i] = w[2][i] + (w[0][i] + w[1][i])/2.;
		/* |E|/30 */
		int_ijw[0][0][i] = 3.*w[0][i] + w[1][i] + w[2][i];
		int_ijw[0][1][i] = int_ijw[1][0][i] = w[0][i] + w[1][i] + w[2][i]/2.;
		int_ijw[0][2][i] = int_ijw[2][0][i] = w[0][i] + w[1][i]/2. + w[2][i];
		int_ijw[1][1][i] = w[0][i] + 3.*w[1][i] + w[2][i];
		int_ijw[1][2][i] = int_ijw[2][1][i] = w[0][i]/2. + w[1][i] + w[2][i];
		int_ijw[2][2][i] = w[0][i] + w[1][i] + 3.*w[2][i];
		/* |E|/90 */
		int_ijkw[0][i] = (w[0][i] + w[1][i] + w[2][i])/2.;
		int_ijkw[1][i] = w[1][i] + (3*w[0][i] + w[2][i])/2.;
		int_ijkw[2][i] = w[2][i] + (3*w[0][i] + w[1][i])/2.;
		int_ijkw[3][i] = w[0][i] + (3*w[1][i] + w[2][i])/2.;
		int_ijkw[4][i] = w[0][i] + (3*w[2][i] + w[1][i])/2.;
		int_ijkw[5][i] = w[1][i] + (3*w[2][i] + w[0][i])/2.;
		int_ijkw[6][i] = w[2][i] + (3*w[1][i] + w[0][i])/2.;
	}

	if (IS_FN(n1)) {
		CSET_NN(COEFFNNP(n1,Z),vol_6,divw,b[0],int_iw[0])
		if (IS_FN(n2)) {
			for (pl = START(n2); n1 != NBNODE(pl); pl = NEXT(pl));
			CSET_NN(COEFFLLP(pl,Z),vol_6,divw_2,b[0],int_iw[1])
		}
		if (IS_FN(n3)) {
			for (pl = START(n3); n1 != NBNODE(pl); pl = NEXT(pl));
			CSET_NN(COEFFLLP(pl,Z),vol_6,divw_2,b[0],int_iw[2])
		}
		if (IS_FF(f3)) {
			for (pfn = FNSTART(f3); n1 != NBNODE(pfn); pfn = NEXT(pfn));
			CSET_FN(COEFFLLP(pfn,Z),vol_30,divw,b[0],int_ijw[0][1])
		}
		if (IS_FF(f2)) {
			for (pfn = FNSTART(f2); n1 != NBNODE(pfn); pfn = NEXT(pfn));
			CSET_FN(COEFFLLP(pfn,Z),vol_30,divw,b[0],int_ijw[0][2])
		}
		if (IS_FF(f1)) {
			for (pfn = FNSTART(f1); n1 != NBNODE(pfn); pfn = NEXT(pfn));
			CSET_FN(COEFFLLP(pfn,Z),vol_30,divw_2,b[0],int_ijw[1][2])
		}
	}
	if (IS_FN(n2)) {
		CSET_NN(COEFFNNP(n2,Z),vol_6,divw,b[1],int_iw[1])
		if (IS_FN(n1)) {
			for (pl = START(n1); n2 != NBNODE(pl); pl = NEXT(pl));
			CSET_NN(COEFFLLP(pl,Z),vol_6,divw_2,b[1],int_iw[0])
		}
		if (IS_FN(n3)) {
			for (pl = START(n3); n2 != NBNODE(pl); pl = NEXT(pl));
			CSET_NN(COEFFLLP(pl,Z),vol_6,divw_2,b[1],int_iw[2])
		}
		if (IS_FF(f3)) {
			for (pfn = FNSTART(f3); n2 != NBNODE(pfn); pfn = NEXT(pfn));
			CSET_FN(COEFFLLP(pfn,Z),vol_30,divw,b[1],int_ijw[0][1])
		}
		if (IS_FF(f2)) {
			for (pfn = FNSTART(f2); n2 != NBNODE(pfn); pfn = NEXT(pfn));
			CSET_FN(COEFFLLP(pfn,Z),vol_30,divw_2,b[1],int_ijw[0][2])
		}
		if (IS_FF(f1)) {
			for (pfn = FNSTART(f1); n2 != NBNODE(pfn); pfn = NEXT(pfn));
			CSET_FN(COEFFLLP(pfn,Z),vol_30,divw,b[1],int_ijw[1][2])
		}
	}
	if (IS_FN(n3)) {
		CSET_NN(COEFFNNP(n3,Z),vol_6,divw,b[2],int_iw[2])
		if (IS_FN(n1)) {
			for (pl = START(n1); n3 != NBNODE(pl); pl = NEXT(pl));
			CSET_NN(COEFFLLP(pl,Z),vol_6,divw_2,b[2],int_iw[0])
		}
		if (IS_FN(n2)) {
			for (pl = START(n2); n3 != NBNODE(pl); pl = NEXT(pl));
			CSET_NN(COEFFLLP(pl,Z),vol_6,divw_2,b[2],int_iw[1])
		}
		if (IS_FF(f3)) {
			for (pfn = FNSTART(f3); n3 != NBNODE(pfn); pfn = NEXT(pfn));
			CSET_FN(COEFFLLP(pfn,Z),vol_30,divw_2,b[2],int_ijw[0][1])
		}
		if (IS_FF(f2)) {
			for (pfn = FNSTART(f2); n3 != NBNODE(pfn); pfn = NEXT(pfn));
			CSET_FN(COEFFLLP(pfn,Z),vol_30,divw,b[2],int_ijw[0][2])
		}
		if (IS_FF(f1)) {
			for (pfn = FNSTART(f1); n3 != NBNODE(pfn); pfn = NEXT(pfn));
			CSET_FN(COEFFLLP(pfn,Z),vol_30,divw,b[2],int_ijw[1][2])
		}
	}
	if (IS_FF(f3)) {
		CSET_FF(COEFFNNP(f3,Z),vol_90,divw,b[0],int_ijkw[3],b[1],int_ijkw[1])
		if (IS_FF(f2)) {
			for (pfl = FSTART(f2); f3 != NBFACE(pfl); pfl = NEXT(pfl));
			CSET_FF(COEFFLLP(pfl,Z),vol_90,divw_2,b[0],int_ijkw[0],b[1],int_ijkw[2])
		}
		if (IS_FF(f1)) {
			for (pfl = FSTART(f1); f3 != NBFACE(pfl); pfl = NEXT(pfl));
			CSET_FF(COEFFLLP(pfl,Z),vol_90,divw_2,b[0],int_ijkw[6],b[1],int_ijkw[0])
		}
		if (IS_FN(n1)) {
			for (pnf = NFSTART(n1); f3 != NBFACE(pnf); pnf = NEXT(pnf));
			CSET_NF(COEFFLLP(pnf,Z),vol_30,divw,b[0],int_ijw[0][1],b[1],int_ijw[0][0])
		}
		if (IS_FN(n2)) {
			for (pnf = NFSTART(n2); f3 != NBFACE(pnf); pnf = NEXT(pnf));
			CSET_NF(COEFFLLP(pnf,Z),vol_30,divw,b[0],int_ijw[1][1],b[1],int_ijw[0][1])
		}
		if (IS_FN(n3)) {
			for (pnf = NFSTART(n3); f3 != NBFACE(pnf); pnf = NEXT(pnf));
			CSET_NF(COEFFLLP(pnf,Z),vol_30,divw_2,b[0],int_ijw[1][2],b[1],int_ijw[0][2])
		}
	}
	if (IS_FF(f2)) {
		CSET_FF(COEFFNNP(f2,Z),vol_90,divw,b[0],int_ijkw[4],b[2],int_ijkw[2])
		if (IS_FF(f3)) {
			for (pfl = FSTART(f3); f2 != NBFACE(pfl); pfl = NEXT(pfl));
			CSET_FF(COEFFLLP(pfl,Z),vol_90,divw_2,b[0],int_ijkw[0],b[2],int_ijkw[1])
		}
		if (IS_FF(f1)) {
			for (pfl = FSTART(f1); f2 != NBFACE(pfl); pfl = NEXT(pfl));
			CSET_FF(COEFFLLP(pfl,Z),vol_90,divw_2,b[0],int_ijkw[5],b[2],int_ijkw[0])
		}
		if (IS_FN(n1)) {
			for (pnf = NFSTART(n1); f2 != NBFACE(pnf); pnf = NEXT(pnf));
			CSET_NF(COEFFLLP(pnf,Z),vol_30,divw,b[0],int_ijw[0][2],b[2],int_ijw[0][0])
		}
		if (IS_FN(n2)) {
			for (pnf = NFSTART(n2); f2 != NBFACE(pnf); pnf = NEXT(pnf));
			CSET_NF(COEFFLLP(pnf,Z),vol_30,divw_2,b[0],int_ijw[1][2],b[2],int_ijw[0][1])
		}
		if (IS_FN(n3)) {
			for (pnf = NFSTART(n3); f2 != NBFACE(pnf); pnf = NEXT(pnf));
			CSET_NF(COEFFLLP(pnf,Z),vol_30,divw,b[0],int_ijw[2][2],b[2],int_ijw[0][2])
		}
	}
	if (IS_FF(f1)) {
		CSET_FF(COEFFNNP(f1,Z),vol_90,divw,b[1],int_ijkw[5],b[2],int_ijkw[6])
		if (IS_FF(f3)) {
			for (pfl = FSTART(f3); f1 != NBFACE(pfl); pfl = NEXT(pfl));
			CSET_FF(COEFFLLP(pfl,Z),vol_90,divw_2,b[1],int_ijkw[0],b[2],int_ijkw[3])
		}
		if (IS_FF(f2)) {
			for (pfl = FSTART(f2); f1 != NBFACE(pfl); pfl = NEXT(pfl));
			CSET_FF(COEFFLLP(pfl,Z),vol_90,divw_2,b[1],int_ijkw[4],b[2],int_ijkw[0])
		}
		if (IS_FN(n1)) {
			for (pnf = NFSTART(n1); f1 != NBFACE(pnf); pnf = NEXT(pnf));
			CSET_NF(COEFFLLP(pnf,Z),vol_30,divw_2,b[1],int_ijw[0][2],b[2],int_ijw[0][1])
		}
		if (IS_FN(n2)) {
			for (pnf = NFSTART(n2); f1 != NBFACE(pnf); pnf = NEXT(pnf));
			CSET_NF(COEFFLLP(pnf,Z),vol_30,divw,b[1],int_ijw[1][2],b[2],int_ijw[1][1])
		}
		if (IS_FN(n3)) {
			for (pnf = NFSTART(n3); f1 != NBFACE(pnf); pnf = NEXT(pnf));
			CSET_NF(COEFFLLP(pnf,Z),vol_30,divw,b[1],int_ijw[2][2],b[2],int_ijw[1][2])
		}
	}
}

void convective_term_IP2C_korn(tGrid, tau, Z)
GRID *tGrid;
FLOAT tau;
INT Z;
{
	ELEMENT *pel;
	NODE *n1, *n2, *n3;
	FACE *f1, *f2, *f3;

	for (pel = FIRSTELEMENT(tGrid); pel != NULL; pel = SUCC(pel)) {
		NODES_OF_ELEMENT(n1,n2,n3,pel);
		FACES_OF_ELEMENT(f1,f2,f3,pel);
		
		add_conv(n1,n2,n3,f1,f2,f3,tau,Z);
	}
}

void convective_term(tGrid, tau, Z, space, mstruct, vstruct)
GRID *tGrid;
FLOAT tau;
INT Z, space, mstruct, vstruct;
{
	switch (space) {
	case IP2C:
		if (vstruct == VECTOR) {
			if (mstruct & Q_FULL)
				convective_term_IP2C_korn(tGrid, tau, Z);
			else
			    eprintf("Error: convective_term not available!\n");
		} else
			eprintf("Error: convective_term not available!\n");
		break;
	default:
		eprintf("Error: convective_term not available!\n");
	}
}

#endif

/******************************************************************************/
/*             end of 2D stiffness matrices by Martin Sodomka                 */
/******************************************************************************/

void add_Laplace_matr(tGrid,nu,Z,space,mstruct,vstruct,korn_laplace)
GRID *tGrid;
FLOAT nu;
INT Z, space, mstruct, vstruct, korn_laplace;
{
   switch(space){
   case P1_NC:   if (vstruct == SCALAR || (vstruct == VECTOR &&
                       (korn_laplace == NO) && (mstruct & (Q_FEBDIAG|Q_FULL))) )
                    nc_Laplace_matr(tGrid,nu,Z,mstruct);
                 else if ( vstruct == VECTOR && (mstruct & Q_FULL) && 
                                                         (korn_laplace == YES) )
                    nc_Laplace_matr_Korn(tGrid,nu,Z);
                 else
                    eprintf("Error: add_Laplace_matr not available.\n");
        break;
   case P1_MOD:  if (vstruct == SCALAR || 
                                (vstruct == VECTOR && (mstruct & Q_DVFEBDIAG)) )
                    p1mod_Laplace_matr(tGrid,nu,Z);
                 else
                    eprintf("Error: add_Laplace_matr not available.\n");
        break;
   case Q1ROT:   if (vstruct == SCALAR ||
                                  (vstruct == VECTOR && (mstruct & Q_FEBDIAG)) )
                    q1rot_Laplace_matr(tGrid,nu,Z);
                 else if ( vstruct == VECTOR && (mstruct & Q_FULL) && 
                                                         (korn_laplace == YES) )
                    q1rot_Laplace_matr_Korn(tGrid,nu,Z);
                 else
                    eprintf("Error: add_Laplace_matr not available.\n");
        break;
   case P1C:     if (vstruct == SCALAR || 
                                 (vstruct == VECTOR  && (mstruct & Q_NEBDIAG)) )
                    p1c_Laplace_stiff_matr(tGrid,nu,Z);
                 else if ( vstruct == VECTOR && (mstruct & Q_FULL) && 
                                                         (korn_laplace == YES) )
                    p1c_Laplace_stiff_matr_Korn(tGrid,nu,Z);
                 else
                    eprintf("Error: add_Laplace_matr not available.\n");
        break;
   case P1C_FBUB: if (mstruct & (Q_NEBDIAG | Q_NEBDIAG_NRED | Q_NEBDIAG_NFRED))
                    p1c_fbub_Laplace_matr(tGrid,nu,Z);
                 else
                    eprintf("Error: add_Laplace_matr not available.\n");
        break;
   case P1C_NEW_FBUB: 
                 if (mstruct & (Q_NEBDIAG | Q_NEBDIAG_NRED | Q_NEBDIAG_NFRED))
                    p1c_fbub_Laplace_matr(tGrid,nu,Z);
                 else
                    eprintf("Error: add_Laplace_matr not available.\n");
        break;
   case P1C_ELBUB: if (vstruct == SCALAR)
                    add_P1CB_Laplace_matr(tGrid,nu,Z);
                 else if (vstruct == VECTOR && (mstruct & Q_NEBDIAG_EDIAG))
                    mini_Laplace_stiff_matr(tGrid,nu,Z);
                 else
                    eprintf("Error: add_Laplace_matr not available.\n");
        break;
   case MINI_L_DIV_FR: if (vstruct == SCALAR || 
                                 (vstruct == VECTOR  && (mstruct & Q_NEBDIAG)) )
                    mini_lin_div_free_Laplace_stiff_matr(tGrid,nu,Z);
                 else
                    eprintf("Error: add_Laplace_matr not available.\n");
        break;
   case Q1C:     if (vstruct == SCALAR || 
                                 (vstruct == VECTOR  && (mstruct & Q_NEBDIAG)) )
                    q1c_Laplace_stiff_matr(tGrid,nu,Z,LAPL_Q_RULE);
                 else if ( vstruct == VECTOR && (mstruct & Q_FULL) && 
                                                         (korn_laplace == YES) )
                    q1c_Laplace_stiff_matr_Korn(tGrid,nu,Z,LAPL_Q_RULE);
                 else
                    eprintf("Error: add_Laplace_matr not available.\n");
        break;
   case P2C:     if (vstruct == SCALAR || (vstruct == VECTOR && 
                        (korn_laplace == NO) && (mstruct & (Q_EBDIAG|Q_FULL))) )
                    p2c_Laplace_stiff_matr(tGrid,nu,Z,0,mstruct);
                 else if ( vstruct == VECTOR && (mstruct & Q_FULL) && 
                                                         (korn_laplace == YES) )
                    p2c_Laplace_stiff_matr_Korn(tGrid,nu,Z);
                 else
                    eprintf("Error: add_Laplace_matr not available.\n");
        break;
   case IP2C:    if (vstruct == SCALAR || 
                                   (vstruct == VECTOR && (mstruct & Q_EBDIAG)) )
                    p2c_Laplace_stiff_matr_iso(tGrid,nu,Z,0,mstruct,LAPL_Q_RULE);
                 else if ( vstruct == VECTOR && (mstruct & Q_FULL) && 
                                                        (korn_laplace == YES) ){
                    if (MOVING_BOUNDARY == YES)
                       sstiff_matr2(tGrid, nu, Z);
                    else
                       p2c_Laplace_stiff_matr_Korn_iso(tGrid,nu,Z);
                 }
                 else
                    eprintf("Error: add_Laplace_matr not available.\n");
        break;
   case P2C_ELBUB: if (vstruct == SCALAR ||
                                   (vstruct == VECTOR && (mstruct & Q_EBDIAG)) )
                    p2c_Laplace_stiff_matr(tGrid,nu,Z,1,mstruct);
                 else
                    eprintf("Error: add_Laplace_matr not available.\n");
        break;
   case IP2C_ELBUB: if (vstruct == SCALAR ||
                                   (vstruct == VECTOR && (mstruct & Q_EBDIAG)) )
                    p2c_Laplace_stiff_matr_iso(tGrid,nu,Z,1,mstruct,LAPL_Q_RULE);
                 else
                    eprintf("Error: add_Laplace_matr not available.\n");
        break;
   case GP1C:
   case GP1X3C:
   case GP1C_ELBUB:
   case GQ1C:
   case GQ1X4C:
   case GQ1C_ELBUB:
   case GP2C:
   case GP2X3C:
   case GP2C_3ELBUB:
   case GP2C_6ELBUB:
   case GQ2C:
   case GQ2X4C:
   case GQ2C_2ELBUB:
   case GQ2C_3ELBUB:
                 general_stiff_matr(tGrid,Z,nu,1.,2.,3.,4.,0,1,2,
                            NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                            general_gu_gv_ref,ELEM,REF_MAP,LAPL_Q_RULE);
        break;
   case GQ2B3C:  general_stiff_matr_b(tGrid,Z,nu,1.,2.,3.,4.,0,1,2,
                            NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                            general_gu_gv_ref,
                            q2c_element,ELEM,REF_MAP,LAPL_Q_RULE);
        break;
   default:
        eprintf("Error: add_Laplace_matr not available.\n");
        break;
   }
}

void add_mass_matr(tGrid,tau,Z,space,mstruct,vstruct)
GRID *tGrid;
FLOAT tau;
INT Z, space, mstruct, vstruct;
{
   switch(space){
   case P1_NC:   if (vstruct == SCALAR ||
                                  (vstruct == VECTOR && (mstruct & Q_FEBDIAG)) )
                    nc_smass_matr(tGrid,Z,tau);
                 else
                    eprintf("Error: add_mass_matr not available.\n");
        break;
   case P1C:     if (vstruct == SCALAR || 
                                 (vstruct == VECTOR  && (mstruct & Q_NEBDIAG)) )
                    p1c_smass_matr(tGrid,Z,tau);
                 else
                    eprintf("Error: add_mass_matr not available.\n");
        break;
   case P2C:     if (vstruct == SCALAR || 
                                   (vstruct == VECTOR && (mstruct & Q_EBDIAG)) )
                    smass_matr_P2(tGrid,Z,tau);
                 else
                    eprintf("Error: add_mass_matr not available.\n");
        break;
   case P2C_ELBUB: if (vstruct == SCALAR || 
                                   (vstruct == VECTOR && (mstruct & Q_EBDIAG)) )
                    smass_matr_p2c_elbub(tGrid,Z,tau);
                 else
                    eprintf("Error: add_mass_matr not available.\n");
        break;
   default:
        eprintf("Error: add_mass_matr not available.\n");
        break;
   }
}

void add_conv_term_matr_and_rhs(tGrid,nu,v,Z,f,p,u_old,rhs0,rhs1,tau,
                                            stabilization,space,mstruct,vstruct)
GRID *tGrid;
FLOAT nu, (*rhs0)(), (*rhs1)(), tau;
INT v, Z, f, p, u_old, stabilization, space, mstruct, vstruct;
{
   switch(space){
   case P1_NC:   if (vstruct == VECTOR && (mstruct & (Q_FEBDIAG|Q_FULL)) &&
                     stabilization == UPWIND_STAB)
                    nc_upwind_matr(tGrid,nu,v,Z,mstruct);
                 else
                    eprintf("Error: add_conv_term_matr_and_rhs not available.\n");
        break;
   case P1C_ELBUB: if (vstruct == VECTOR && (mstruct & Q_NEBDIAG_EDIAG) &&
                       stabilization == UPWIND_STAB)
                    p1c_upwind_matr(tGrid,nu,v,Z);
                 else
                    eprintf("Error: add_conv_term_matr_and_rhs not available.\n");
        break;
   case P2C:     if (vstruct == VECTOR && (mstruct & (Q_EBDIAG|Q_FULL))){
                    if (stabilization == UPWIND_STAB)
                       p2c_upwind_matr(tGrid,nu,v,Z,mstruct);
                    else{
                       sconv_matr2(tGrid,v,Z,0,mstruct);
                   /*  sconv_skew_matr2(tGrid,v,Z); */
                       if (stabilization == SDFEM_STAB){
                          if (mstruct & Q_EBDIAG)
                             sconv_stab_matr2(tGrid,nu,v,Z,f,p,u_old,rhs0,rhs1,tau);
                          else
                             eprintf("Error: SDFEM_STAB in add_conv_term_matr_and_rhs not available.\n");
                       }
                    }
                 }
                 else
                    eprintf("Error: add_conv_term_matr_and_rhs not available.\n");
        break;
   case IP2C:    if (vstruct == VECTOR && (mstruct & Q_EBDIAG)){
                    if (stabilization == NONE)
                       sconv_matr2_iso(tGrid,v,Z,0,mstruct);
                    else if (stabilization == UPWIND_STAB)
                       p2c_upwind_matr(tGrid,nu,v,Z,mstruct);
                    else if (stabilization == SDFEM_STAB && 
                                                        MOVING_BOUNDARY == YES){
                       sconv_matr2_iso(tGrid,v,Z,0,mstruct);
                       sconv_stab_matr2(tGrid,nu,v,Z,f,p,u_old,rhs0,rhs1,tau);
                    }
                    else
                       eprintf("Error: add_conv_term_matr_and_rhs not available.\n");
                 }
                 else if (vstruct == VECTOR && (mstruct & Q_FULL) &&
                                                        MOVING_BOUNDARY == YES){
                    if (stabilization == UPWIND_STAB)
                       p2c_upwind_matr_korn(tGrid,nu,v,Z);
                    else{
                       sconv_matr2_korn(tGrid,v,Z);
                       if (stabilization == SDFEM_STAB)
                          sconv_stab_matr2_korn(tGrid,nu,v,Z,f,p,u_old,rhs0,rhs1,tau);
                    }
                 }
                 else
                    eprintf("Error: add_conv_term_matr_and_rhs not available.\n");
        break;
   case P2C_ELBUB: if (vstruct == VECTOR && (mstruct & Q_EBDIAG)){
                    if (stabilization == NONE)
                       sconv_matr2(tGrid,v,Z,1,mstruct);
                    else if (stabilization == UPWIND_STAB)
                       p2c_upwind_matr(tGrid,nu,v,Z,mstruct);
                    else
                       eprintf("Error: add_conv_term_matr_and_rhs not available.\n");
                 }
                 else
                    eprintf("Error: add_conv_term_matr_and_rhs not available.\n");
        break;
   case IP2C_ELBUB: if (vstruct == VECTOR && (mstruct & Q_EBDIAG)){
                    if (stabilization == NONE)
                       sconv_matr2_iso(tGrid,v,Z,1,mstruct);
                    else if (stabilization == UPWIND_STAB)
                       p2c_upwind_matr(tGrid,nu,v,Z,mstruct);
                    else
                       eprintf("Error: add_conv_term_matr_and_rhs not available.\n");
                 }
                 else
                    eprintf("Error: add_conv_term_matr_and_rhs not available.\n");
        break;
   default:
        eprintf("Error: add_conv_term_matr_and_rhs not available.\n");
        break;
   }
}

void add_newton_conv_term_matr(tGrid,nu,v,Z,stabilization,space,mstruct,vstruct)
GRID *tGrid;
FLOAT nu;
INT v, Z, stabilization, space, mstruct, vstruct;
{
   switch(space){
   case P1_NC:   if (vstruct == VECTOR && (mstruct & Q_FULL) &&
                                                  stabilization == UPWIND_STAB)
                    newton_nc_upwind_matr(tGrid,nu,v,Z);
                 else
                    eprintf("Error: add_newton_conv_term_matr not available.\n");
        break;
   case P2C:     if (vstruct == VECTOR && (mstruct & Q_FULL)){
                    if (stabilization == UPWIND_STAB)
                       eprintf("Error: UPWIND_STAB in add_newton_conv_term_matr not available.\n");
                    else{
                       newton_conv_matr2(tGrid,v,Z,0);
                       if (stabilization == SDFEM_STAB)
                          eprintf("Error: SDFEM_STAB in add_newton_conv_term_matr not available.\n");
                    }
                 }
                 else
                    eprintf("Error: add_newton_conv_term_matr not available.\n");
        break;
   default:
        eprintf("Error: add_newton_conv_term_matr not available.\n");
        break;
   }
}

void add_react_term_matr(tGrid,Z,r,space,mstruct,vstruct)
GRID *tGrid;
FLOAT (*r)();
INT Z, space, mstruct, vstruct;
{
   switch(space){
   case P1_NC:   if (vstruct == SCALAR ||
                                  (vstruct == VECTOR && (mstruct & Q_FEBDIAG)) )
                    nc_react_term_to_stiff_matr(tGrid,Z,r);
                 else
                    eprintf("Error: add_react_term_matr not available.\n");
        break;
   case P1_MOD:  if (vstruct == SCALAR || 
                                (vstruct == VECTOR && (mstruct & Q_DVFEBDIAG)) )
                    nc_react_term_to_stiff_matr_mbub(tGrid,Z,r);
                 else
                    eprintf("Error: add_react_term_matr not available.\n");
        break;
   case P1C:     if (vstruct == SCALAR || 
                                 (vstruct == VECTOR  && (mstruct & Q_NEBDIAG)) )
                    add_P1C_react_term_matr(tGrid,Z,r);
                 else
                    eprintf("Error: add_react_term_matr not available.\n");
        break;
   case Q1C:     if (vstruct == SCALAR || 
                                 (vstruct == VECTOR  && (mstruct & Q_NEBDIAG)) )
                    add_Q1C_react_term_matr(tGrid,Z,r,REACT_Q_RULE);
                 else
                    eprintf("Error: add_react_term_matr not available.\n");
        break;
   case P2C:     if (vstruct == SCALAR || 
                                   (vstruct == VECTOR && (mstruct & Q_EBDIAG)) )
                    add_P2C_react_term_matr(tGrid,Z,r);
                 else
                    eprintf("Error: add_react_term_matr not available.\n");
        break;
   case GP1C:
   case GP1X3C:
   case GP1C_ELBUB:
   case GQ1C:
   case GQ1X4C:
   case GQ1C_ELBUB:
   case GP2C:
   case GP2X3C:
   case GP2C_3ELBUB:
   case GP2C_6ELBUB:
   case GQ2C:
   case GQ2X4C:
   case GQ2C_2ELBUB:
   case GQ2C_3ELBUB:
                 if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,0.,1.,2.,3.,4.,0,1,2,
                            r,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                            general_d_u_v_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_react_term_matr not available.\n");
        break;
   case GQ2B3C:  if (vstruct == SCALAR)
                    general_stiff_matr_b(tGrid,Z,0.,1.,2.,3.,4.,0,1,2,
                            r,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                            general_d_u_v_ref,
                            q2c_element,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_react_term_matr not available.\n");
        break;
   default:
        eprintf("Error: add_react_term_matr not available.\n");
        break;
   }
}

void add_convective_term_matr(tGrid,Z,bb0,bb1,div_bb,discr,space,mstruct,vstruct)
GRID *tGrid;
FLOAT (*bb0)(), (*bb1)(), (*div_bb)();
INT Z, discr, space, mstruct, vstruct;
{
   switch(space){
   case P1_NC:   if (vstruct == SCALAR ||
                                  (vstruct == VECTOR && (mstruct & Q_FEBDIAG)) )
                    nc_conv_term_to_stiff_matr(tGrid,Z,bb0,bb1,div_bb,discr);
                 else
                    eprintf("Error: add_convective_term_matr not available.\n");
        break;
   case P1_MOD:  if (vstruct == SCALAR || 
                                (vstruct == VECTOR && (mstruct & Q_DVFEBDIAG)) )
                    nc_conv_term_to_stiff_matr_mbub(tGrid,Z,bb0,bb1,div_bb,discr);
                 else
                    eprintf("Error: add_convective_term_matr not available.\n");
        break;
   case P1C:     if (vstruct == SCALAR || 
                                 (vstruct == VECTOR  && (mstruct & Q_NEBDIAG)) )
                    add_P1C_conv_term_matr(tGrid,Z,bb0,bb1);
                 else
                    eprintf("Error: add_convective_term_matr not available.\n");
        break;
   case P1C_ELBUB: if (vstruct == SCALAR){
                    if (USE_QUADRATURE == YES)
                       p1cb_stiff_matr(tGrid,Z,0.,1.,2.,3.,4.,0,1,2,
                       bb0,bb1,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                       general_b_grad_u_v_ref,p1cb_element,REF_MAP,CONV_Q_RULE);
                    else
                       add_P1CB_conv_term_matr(tGrid,Z,bb0,bb1);
                 }
                 else
                    eprintf("Error: add_convective_term_matr not available.\n");
        break;
   case Q1C:     if (vstruct == SCALAR || 
                                 (vstruct == VECTOR  && (mstruct & Q_NEBDIAG)) )
                    add_Q1C_conv_term_matr(tGrid,Z,bb0,bb1,CONV_Q_RULE);
                 else
                    eprintf("Error: add_convective_term_matr not available.\n");
        break;
   case P2C:     if (vstruct == SCALAR || 
                         (vstruct == VECTOR  && (mstruct & (Q_EBDIAG|Q_FULL))) )
                    add_P2C_conv_term_matr(tGrid,Z,bb0,bb1,mstruct);
                 else
                    eprintf("Error: add_convective_term_matr not available.\n");
        break;
   case GP1C:
   case GP1X3C:
   case GP1C_ELBUB:
   case GQ1C:
   case GQ1X4C:
   case GQ1C_ELBUB:
   case GP2C:
   case GP2X3C:
   case GP2C_3ELBUB:
   case GP2C_6ELBUB:
   case GQ2C:
   case GQ2X4C:
   case GQ2C_2ELBUB:
   case GQ2C_3ELBUB:
                 if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,0.,1.,2.,3.,4.,0,1,2,
                            bb0,bb1,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                            general_b_grad_u_v_ref,ELEM,REF_MAP,CONV_Q_RULE);
                 else
                    eprintf("Error: add_convective_term_matr not available.\n");
        break;
   case GQ2B3C:  if (vstruct == SCALAR)
                    general_stiff_matr_b(tGrid,Z,0.,1.,2.,3.,4.,0,1,2,
                            bb0,bb1,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                            general_b_grad_u_v_ref,
                            q2c_element,ELEM,REF_MAP,CONV_Q_RULE);
                 else
                    eprintf("Error: add_convective_term_matr not available.\n");
        break;
   default:
        eprintf("Error: add_convective_term_matr not available.\n");
        break;
   }
}

void add_sd_term_matr(tGrid,nu,Z,bb0,bb1,react,space,mstruct,vstruct)
GRID *tGrid;
FLOAT nu, (*bb0)(), (*bb1)(), (*react)();
INT Z, space, mstruct, vstruct;
{
   switch(space){
   case P1_NC:   if (vstruct == SCALAR ||
                                  (vstruct == VECTOR && (mstruct & Q_FEBDIAG)) )
                    nc_sd_term_to_stiff_matr(tGrid,nu,Z,bb0,bb1,react);
                 else
                    eprintf("Error: add_sd_term_matr not available.\n");
        break;
   case P1_MOD:  if (vstruct == SCALAR || 
                                (vstruct == VECTOR && (mstruct & Q_DVFEBDIAG)) )
                    nc_sd_term_to_stiff_matr_mbub(tGrid,nu,Z,bb0,bb1,react);
                 else
                    eprintf("Error: add_sd_term_matr not available.\n");
        break;
   case P1C:     if (vstruct == SCALAR || 
                                (vstruct == VECTOR  && (mstruct & Q_NEBDIAG)) ){
                    if (USE_QUADRATURE == YES){
                       printf("sd matrix computed using quadrature\n");
                       add_sd_term_matr_quadr(tGrid,Z,nu,bb0,bb1,react,
                                             zero_func,0,space,0,0.,0.,sd_tau,
                                             SD_Q_RULE,sd_p1c_quadr_ij);
                    }
                    else
                       if (TAU_SPACE == P0){
                          add_p1c_sd_term_matr(tGrid,Z,nu,bb0,bb1,react);
                       }
                       else if (TAU_SPACE == P1_NC){
                          add_p1c_sd_p1_nc_term_matr(tGrid,Z,nu,bb0,bb1,react);
                       }
                       else if (TAU_SPACE == P1C){
                          add_p1c_sd_p1c_term_matr(tGrid,Z,nu,bb0,bb1,react);
                       }
                       else
                          eprintf("Error: add_sd_term_matr not available.\n");
                 }
                 else
                    eprintf("Error: add_sd_term_matr not available.\n");
        break;
   case Q1C:     if (vstruct == SCALAR || 
                                 (vstruct == VECTOR  && (mstruct & Q_NEBDIAG)) )
                    add_Q1C_sd_term_matr(tGrid,Z,nu,bb0,bb1,react,SD_Q_RULE);
                 else
                    eprintf("Error: add_sd_term_matr not available.\n");
        break;
   case P2C:     if (vstruct == SCALAR || 
                                 (vstruct == VECTOR  && (mstruct & Q_NEBDIAG)) ){
                    if (TAU_SPACE == P0){
                       add_p2c_sd_term_matr(tGrid,Z,nu,bb0,bb1,react);
                    }
                    else if (TAU_SPACE == P1_NC){
                       add_p2c_sd_p1_nc_term_matr(tGrid,Z,nu,bb0,bb1,react);
                    }
                    else if (TAU_SPACE == P1C){
                       add_p2c_sd_p1c_term_matr(tGrid,Z,nu,bb0,bb1,react);
                    }
                    else
                       eprintf("Error: add_sd_term_matr not available.\n");
                 }
                 else
                    eprintf("Error: add_sd_term_matr not available.\n");
        break;
   case GP1C:
   case GP1X3C:
   case GP1C_ELBUB:
   case GQ1C:
   case GQ1X4C:
   case GQ1C_ELBUB:
   case GP2C:
   case GP2X3C:
   case GP2C_3ELBUB:
   case GP2C_6ELBUB:
   case GQ2C:
   case GQ2X4C:
   case GQ2C_2ELBUB:
   case GQ2C_3ELBUB:
                 if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,nu,1.,2.,3.,4.,space,1,2,
                            bb0,bb1,react,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                            general_sd_ref,ELEM,REF_MAP,SD_Q_RULE);
                 else
                    eprintf("Error: add_sd_term_matr not available.\n");
        break;
   default:
        eprintf("Error: add_sd_term_matr not available.\n");
        break;
   }
}

void add_conv_stab_term_matr(tGrid,eps,Z,bb0,bb1,space,mstruct,vstruct)
GRID *tGrid;
FLOAT eps, (*bb0)(), (*bb1)();
INT Z, space, mstruct, vstruct;
{
   switch(space){
   case GP1C:
   case GP1X3C:
   case GP1C_ELBUB:
   case GQ1C:
   case GQ1X4C:
   case GQ1C_ELBUB:
   case GP2C:
   case GP2X3C:
   case GP2C_3ELBUB:
   case GP2C_6ELBUB:
   case GQ2C:
   case GQ2X4C:
   case GQ2C_2ELBUB:
   case GQ2C_3ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4.,0,1,2,
                        bb0,bb1,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                        general_conv_stab_ref,ELEM,REF_MAP,SD_Q_RULE);
                 else
                    eprintf("Error: add_conv_stab_term_matr not available.\n");
        break;
   default:
        eprintf("Error: add_conv_stab_term_matr not available.\n");
        break;
   }
}

void add_macro_conv_stab_term_matr(tGrid,eps,Z,bb0,bb1,space,mstruct,vstruct)
GRID *tGrid;
FLOAT eps, (*bb0)(), (*bb1)();
INT Z, space, mstruct, vstruct;
{
   switch(space){
   case Q1C:     if (vstruct == SCALAR || 
                                 (vstruct == VECTOR  && (mstruct & Q_NEBDIAG)) )
                    add_Q1C_macro_conv_stab_term_matr(tGrid,Z,eps,
                                                      bb0,bb1,SD_Q_RULE);
                 else
                    eprintf("Error: add_sd_term_matr not available.\n");
        break;
   case GP1C:
   case GP1X3C:
   case GP1C_ELBUB:
   case GQ1C:
   case GQ1X4C:
   case GQ1C_ELBUB:
   case GP2C:
   case GP2X3C:
   case GP2C_3ELBUB:
   case GP2C_6ELBUB:
   case GQ2C:
   case GQ2X4C:
   case GQ2C_2ELBUB:
   case GQ2C_3ELBUB: if (vstruct == SCALAR)
                    general_macro_stiff_matr(tGrid,Z,eps,1.,2.,3.,4.,0,1,2,
                        bb0,bb1,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                        general_macro_conv_stab_ref,ELEM,REF_MAP,SD_Q_RULE);
                 else
                    eprintf("Error: add_macro_conv_stab_term_matr not available.\n");
        break;
   case GQ2B3C:  if (vstruct == SCALAR)
                    general_macro_stiff_matr_b(tGrid,Z,eps,1.,2.,3.,4.,0,1,2,
                        bb0,bb1,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                        general_macro_conv_stab_ref,
                        q2c_element,ELEM,REF_MAP,SD_Q_RULE);
                 else
                    eprintf("Error: add_macro_conv_stab_term_matr not available.\n");
        break;
   default:
        eprintf("Error: add_macro_conv_stab_term_matr not available.\n");
        break;
   }
}

void add_lp_term_matr(tGrid,eps,Z,bb0,bb1,space,mstruct,vstruct)
GRID *tGrid;
FLOAT eps, (*bb0)(), (*bb1)();
INT Z, space, mstruct, vstruct;
{
   switch(space){
   case P1C_ELBUB: if (vstruct == SCALAR)
                    add_P1CB_lp_term_matr(tGrid,Z,eps,bb0,bb1);
                 else
                    eprintf("Error: add_lp_term_matr not available.\n");
        break;
   case GQ1X4C:  
   case GQ1C_ELBUB:
   case GP1X3C:  
   case GP1C_ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 1 ,1,2,
                        bb0,bb1,one_fcn,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                        general_local_projection_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_lp_term_matr not available.\n");
        break;
   case GP2X3C:  
   case GP2C_3ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 3 ,1,2,
                        bb0,bb1,r_l0,r_l1,r_l2,NULL,NULL,NULL,NULL,NULL,
                        general_local_projection_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_lp_term_matr not available.\n");
        break;
   case GP2C_6ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 3 ,1,2,
                        bb0,bb1,r_l0,r_l1,r_l2,NULL,NULL,NULL,NULL,NULL,
                        general_local_projection_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_lp_term_matr not available.\n");
        break;
   case GQ2X4C:  if (vstruct == SCALAR)
/*
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 3 ,1,2,
                        bb0,bb1,one_fcn,r_l1,r_l2,NULL,NULL,NULL,NULL,NULL,
                        general_local_projection_ref,ELEM,REF_MAP,REACT_Q_RULE);
*/
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 4 ,1,2,
                        bb0,bb1,one_fcn,r_l1,r_l2,r_l1l2,NULL,NULL,NULL,NULL,
                        general_local_projection_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_lp_term_matr not available.\n");
        break;
   case GQ2C_2ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 3 ,1,2,
                        bb0,bb1,one_fcn,r_l1,r_l2,NULL,NULL,NULL,NULL,NULL,
                        general_local_projection_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_lp_term_matr not available.\n");
        break;
   case GQ2C_3ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 4 ,1,2,
                        bb0,bb1,one_fcn,r_l1,r_l2,r_l1l2,NULL,NULL,NULL,NULL,
                        general_local_projection_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_lp_term_matr not available.\n");
        break;
   default:
        eprintf("Error: add_lp_term_matr not available.\n");
        break;
   }
}

void add_conv_lp_term_matr(tGrid,eps,Z,bb0,bb1,space,mstruct,vstruct)
GRID *tGrid;
FLOAT eps, (*bb0)(), (*bb1)();
INT Z, space, mstruct, vstruct;
{
   switch(space){
   case GQ1X4C:
   case GQ1C_ELBUB:
   case GP1X3C:
   case GP1C_ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 1 ,1,2,
                        bb0,bb1,one_fcn,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                        general_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_conv_lp_term_matr not available.\n");
        break;
   case GP2X3C:
   case GP2C_3ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 3 ,1,2,
                        bb0,bb1,r_l0,r_l1,r_l2,NULL,NULL,NULL,NULL,NULL,
                        general_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_conv_lp_term_matr not available.\n");
        break;
   case GP2C_6ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 3 ,1,2,
                        bb0,bb1,r_l0,r_l1,r_l2,NULL,NULL,NULL,NULL,NULL,
                        general_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_conv_lp_term_matr not available.\n");
        break;
   case GQ2X4C:  if (vstruct == SCALAR)
/*
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 3 ,1,2,
                        bb0,bb1,one_fcn,r_l1,r_l2,NULL,NULL,NULL,NULL,NULL,
                        general_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
*/
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 4 ,1,2,
                        bb0,bb1,one_fcn,r_l1,r_l2,r_l1l2,NULL,NULL,NULL,NULL,
                        general_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_conv_lp_term_matr not available.\n");
        break;
   case GQ2C_2ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 3 ,1,2,
                        bb0,bb1,one_fcn,r_l1,r_l2,NULL,NULL,NULL,NULL,NULL,
                        general_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_conv_lp_term_matr not available.\n");
        break;
   case GQ2C_3ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 4 ,1,2,
                        bb0,bb1,one_fcn,r_l1,r_l2,r_l1l2,NULL,NULL,NULL,NULL,
                        general_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_conv_lp_term_matr not available.\n");
        break;
   default:
        eprintf("Error: add_conv_lp_term_matr not available.\n");
        break;
   }
}

void add_pure_conv_lp_term_matr(tGrid,eps,Z,bb0,bb1,space,mstruct,vstruct)
GRID *tGrid;
FLOAT eps, (*bb0)(), (*bb1)();
INT Z, space, mstruct, vstruct;
{
   switch(space){
   case GQ1X4C:
   case GQ1C_ELBUB:
   case GP1X3C:
   case GP1C_ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 1 ,1,2,
                        bb0,bb1,one_fcn,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                        general_pure_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_pure_conv_lp_term_matr not available.\n");
        break;
   case GP2X3C:
   case GP2C_3ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 3 ,1,2,
                        bb0,bb1,r_l0,r_l1,r_l2,NULL,NULL,NULL,NULL,NULL,
                        general_pure_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_pure_conv_lp_term_matr not available.\n");
        break;
   case GP2C_6ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 3 ,1,2,
                        bb0,bb1,r_l0,r_l1,r_l2,NULL,NULL,NULL,NULL,NULL,
                        general_pure_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_pure_conv_lp_term_matr not available.\n");
        break;
   case GQ2X4C:  if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 3 ,1,2,
                        bb0,bb1,one_fcn,r_l1,r_l2,NULL,NULL,NULL,NULL,NULL,
                        general_pure_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
/*
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 4 ,1,2,
                        bb0,bb1,one_fcn,r_l1,r_l2,r_l1l2,NULL,NULL,NULL,NULL,
                        general_pure_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
*/
                 else
                    eprintf("Error: add_pure_conv_lp_term_matr not available.\n");
        break;
   case GQ2C_2ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 3 ,1,2,
                        bb0,bb1,one_fcn,r_l1,r_l2,NULL,NULL,NULL,NULL,NULL,
                        general_pure_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_pure_conv_lp_term_matr not available.\n");
        break;
   case GQ2C_3ELBUB: if (vstruct == SCALAR)
                    general_stiff_matr(tGrid,Z,eps,1.,2.,3.,4., 4 ,1,2,
                        bb0,bb1,one_fcn,r_l1,r_l2,r_l1l2,NULL,NULL,NULL,NULL,
                        general_pure_conv_local_proj_ref,ELEM,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_pure_conv_lp_term_matr not available.\n");
        break;
   default:
        eprintf("Error: add_pure_conv_lp_term_matr not available.\n");
        break;
   }
}

void add_contributions_from_bubble_elimination(tGrid,Z,f,eps,bb0,bb1,react,rhs,space,mstruct,vstruct)
GRID *tGrid;
FLOAT eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)();
INT Z, f, space, mstruct, vstruct;
{
   switch(space){
   case GQ1C_ELBUB:
   case GQ1C:    if (vstruct == SCALAR)
                    general_bubble_contribution(tGrid,Z,eps,1.,2.,3.,4.,f,1,2,
                        bb0,bb1,react,NULL,NULL,NULL,NULL,NULL,rhs,
                        general_rhs_ref,general_conv_diff_react_stab_ref,
                        q1cb_element,REF_MAP,REACT_Q_RULE);
                 else
                    eprintf("Error: add_contributions_from_bubble_elimination not available.\n");
        break;
   default:
        eprintf("Error: add_contributions_from_bubble_elimination not available.\n");
        break;
   }
}

FLOAT sc_tau_k(y,pelem,bar,eps,bb0,bb1,react,rhs,u,space,sc_type,par1,par2)
ELEMENT *pelem;
FLOAT *y, bar[DIM2][DIM2], eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(),
      par1, par2;
INT u, space, sc_type;
{
   FLOAT *x0, *x1, *x2, d=1., grad, grad_u[DIM], g2, hK, hKp, res, rres, 
         t1, t2, tau, h, h1, h2,
         bbp0, bbp1, bbp, bb0y, bb1y, bb, bb2, zz0, zz1, zz, zz2, r, s, z, xi;
   FLOAT ak, bk, gk, lk, kk, ok, ro, Pe0, Pe1;

   if (PW_CONST_PAR == YES && TAU_K > -0.01)
      return(TAU_K);
   else{
      switch(sc_type){
      case SCM_HMM:
      case SCM_TP1:
      case SCM_TP2:
      case SCM_C:
      case SCM_CS:
      case SCM_CS1:
      case SCM_K6:
      case SCM_K6red:
      case SCM_K6red_iso:
/*
                     if (sc_type == SCM_K6 || sc_type == SCM_K6red){
                        VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
                        h1 = 1./(NVX-1.);
                        h2 = 1./(NVY-1.);
                        if ((x0[0] > 0.99999 && x0[1] < 1.e-10) ||
                            (x1[0] > 0.99999 && x1[1] < 1.e-10) ||
                            (x2[0] > 0.99999 && x2[1] < 1.e-10)){
                           if ((x0[0] < 0.99999 && x0[1] < 1.e-10) ||
                               (x1[0] < 0.99999 && x1[1] < 1.e-10) ||
                               (x2[0] < 0.99999 && x2[1] < 1.e-10))
                              par1 = 0.;
                           else
                              par1 = 0.33683;
                        }
                        else if (x0[1] < 1.e-10 || x1[1] < 1.e-10 || 
                                                   x2[1] < 1.e-10)
                           par1 = 0.;
                        else if (x0[0] > 0.99999 || x1[0] > 0.99999 || 
                                                    x2[0] > 0.99999){
                           s = sin(THETA);
                           z = cos(THETA);
                           h = sqrt(h1*h1+h2*h2);
                           par1 = (h1*s - h2*z)/(h*s*s*s);
                        }
                        else if (x0[0] > 0.2 || x0[1] < 0.5)
                           par1 = 0.3;
                        else
                           par1 = 0.664;
                     }
                     if ((sc_type == SCM_K6 || sc_type == SCM_K6red) && 
                          SC_EXAMPLE == 55){
                        s = (NVY-1.)/(NVX-1.);
                        s = 3.*sqrt(1+s*s);
                        par1 = 2./s;
                     }
*/
                     sgrad_value(pelem,y,bar,u,space,grad_u);
                     if ( (g2=DOT(grad_u,grad_u)) < 1.e-15 )
                        tau = 0.;
                     else{
                        res = scalar_conv_diff_res(pelem,y,bar,u,space,
                                                   eps,bb0,bb1,react,rhs);
                        bb0y = bb0(y);
                        bb1y = bb1(y);
                        r = (bb0y*grad_u[0]+bb1y*grad_u[1])/g2;
                        bbp0 = r*grad_u[0];
                        bbp1 = r*grad_u[1];
                        if (sc_type == SCM_HMM){
                           t1 = sd_tau_classic(pelem,bar,eps,bb0y,bb1y,space);
                           t2 = sd_tau_classic(pelem,bar,eps,bbp0,bbp1,space);
                           tau = res*r*MAX(0.,t2-t1);
                        }
                        else if (sc_type == SCM_TP1 || sc_type == SCM_TP2){
                           if ( (bb =sqrt(bb0y*bb0y+bb1y*bb1y)) < 1.e-10 ||
                                (bbp=sqrt(bbp0*bbp0+bbp1*bbp1)) < 1.e-10 )
                              tau = 0.;
                           else{
                              s = bbp/bb;
                              hKp = sdiameter(pelem,bar,bbp0,bbp1);
                              tau = res*r*hKp*s*(1-s)/(bbp*approx_order(space));
                              if (sc_type == SCM_TP2)
                                 tau *= hKp*sqrt(g2);
                           }
                        }
                        else if (sc_type == SCM_C   || sc_type == SCM_CS || 
                                 sc_type == SCM_CS1 || sc_type == SCM_K6){
                           tau = 0.;
                           if (REG_FABS == YES)
                              rres = regularized_fabs(res,REG_EPS);
                           else
                              rres = fabs(res);
                           s = 0.5*par1*diameter(pelem)*rres/sqrt(g2);
                           if (sc_type == SCM_C){
                              z = fabs(bb0y*grad_u[0]+bb1y*grad_u[1]);
                              r = eps*rres;
                              if (s*z > r)
                                 tau = s - r/z;
                           }
                           else if (s > eps)
                              tau = s - eps;
                           if (tau > 0. && (sc_type == SCM_CS || 
                                            sc_type == SCM_CS1)){
                              t1 = sd_tau_classic(pelem,bar,eps,bb0y,bb1y,space)
                                   *(bb0y*bb0y+bb1y*bb1y);
                              if (sc_type == SCM_CS1)
                                 tau -= t1;
                              else if (sc_type == SCM_CS && t1 < tau)
                                 tau = t1;
                           }
                        }
                        else if (sc_type == SCM_K6red || sc_type == SCM_K6red_iso)
                           tau = 0.5*par1*diameter(pelem)*fabs(res)/sqrt(g2);
                     }
           break;
      case SCM_BE1:
      case SCM_K4:
                     sgrad_value(pelem,y,bar,u,space,grad_u);
                     res = scalar_conv_diff_res(pelem,y,bar,u,space,
                                                        eps,bb0,bb1,react,rhs);
                     bb0y = bb0(y);
                     bb1y = bb1(y);
                     bb2 = bb0y*bb0y + bb1y*bb1y;
                     r = sqrt(bb2*DOT(grad_u,grad_u));
                     if (sc_type == SCM_BE1){
                        res = regularized_fabs(res,2.);
                        xi = fabs(bb0y*grad_u[1]-bb1y*grad_u[0])/sqrt(3.); 
                        if (res < 1.e-10 && xi < 1.e-10)
                           tau = 0.;
                        else{
                           t1 = sd_tau_classic(pelem,bar,eps,bb0y,bb1y,space);
                           s = res + xi;
                           z = (r + res)*s;
                           tau = t1*bb2*res*(r+s)/z;
//                           tau = t1*bb2*res/sqrt(res*res + xi*xi);
                        }
                     }
                     else if (sc_type == SCM_K4){
                        if (REG_FABS == YES)
                           res = regularized_fabs(res,REG_EPS);
                        else
                           res = fabs(res);
                        if (r+res < 1.e-10)
                           tau = 0.;
                        else{
                           t1 = sd_tau_classic(pelem,bar,eps,bb0y,bb1y,space);
                           tau = t1*bb2*res/(r+res);
                        }
                     }
           break;
      case SCM_GC1:
      case SCM_GC2:
      case SCM_AS:
      case SCM_CA:
      case SCM_KLR2:
                     sgrad_value(pelem,y,bar,u,space,grad_u);
                     if ( (g2=DOT(grad_u,grad_u)) < 1.e-15 )
                        tau = 0.;
                     else{
                        res = scalar_conv_diff_res(pelem,y,bar,u,space,
                                                   eps,bb0,bb1,react,rhs);
                        r = res/g2;
                        zz0 = r*grad_u[0];
                        zz1 = r*grad_u[1];
                        bb0y = bb0(y);
                        bb1y = bb1(y);
                        t1 = sd_tau_classic(pelem,bar,eps,bb0y,bb1y,space);
                        if (sc_type == SCM_GC1 || sc_type == SCM_KLR2){
                           t2 = sd_tau_classic(pelem,bar,eps,zz0,zz1,space);
                           if (sc_type == SCM_GC1)
                              tau = res*r*MAX(0.,t2-t1); 
                           else if (sc_type == SCM_KLR2){
                              L2norms_of_cd_res_and_grad_u(pelem,bar,u,space,
                                             eps,bb0,bb1,react,rhs,&res,&grad);
                              z = L2norm_of_u(pelem,bar,u,space);
                              z = res/(par2 + sqrt(z*z + grad*grad));
                              tau = z*z*MAX(0.,t2-t1); 
                           }
                        }
                        else if (sc_type == SCM_CA){
                           t2 = sd_tau_classic(pelem,bar,eps,zz0,zz1,space);
                           tau = res*r*MAX(0.,t2-t1); 
                           if ((zz2=zz0*zz0+zz1*zz1) >= 
                               (bb2=bb0y*bb0y+bb1y*bb1y))
                              ro = 1.;
                           else if (bb2 < 1.e-15)
                              ro = 1.;
                           else{
                              z = zz2/bb2;
                              ak = sqrt(z);
                              s = sdiameter(pelem,bar,bb0y,bb1y);
                              bk = pow(MIN(1.,s),1.-z);
                              gk = 0.5*(ak+bk);
                              gk = MIN(bk,gk);
                              lk = pow(MAX(ak,fabs(res)),3.+0.5*ak+z)/
                                   pow(gk,MAX(0.5,0.25+ak));
                              if (lk >= 1.)
                                 ro = 1.;
                              else{
                                 kk = pow(2.-lk,(1.-lk)/(1.+lk)) - 1.;
                                 ok = z*pow(gk,2.-z)/t1;
                                 ro = pow(ok*MAX(0.,t2-t1),kk);
                              }
                           }
                           tau *= ro;
                        }
                        else if (sc_type == SCM_GC2){
                           zz2 = zz0*zz0 + zz1*zz1;
                           if ( zz2 < 1.e-15 || fabs(res) < 1.e-10 ||
                                        ((bb2=bb0y*bb0y+bb1y*bb1y) <= zz2) )
                              tau = 0.;
                           else{
                              if (REG_FABS == YES)
                                 rres = regularized_fabs(res,REG_EPS);
                              else
                                 rres = fabs(res);
                              tau = rres*t1*(sqrt(bb2/g2) - rres/g2);
                           }
                        }
                        else if (sc_type == SCM_AS){
                           zz2 = zz0*zz0 + zz1*zz1;
                           bb2 = bb0y*bb0y + bb1y*bb1y;
                           if ( zz2 < 1.e-15 || fabs(res) < 1.e-10 )
                              tau = 0.;
                           else{
                              s = (bb0y*grad_u[0]+bb1y*grad_u[1])/res;
                              s = sqrt(bb2/zz2) - MAX(1.,s);
                              tau = res*r*t1*MAX(0.,s);
                           }
                        }
                     }
           break;
      case SCM_J:    s = diameter(pelem);
                     tau = par1*s*s*fabs(scalar_conv_diff_res(pelem,y,bar,u,
                                            space,eps,bb0,bb1,react,rhs)) - eps;
           break;
      case SCM_JSW:  bb0y = bb0(y);
                     bb1y = bb1(y);
                     bb2 = bb0y*bb0y + bb1y*bb1y;
                     hK = sdiameter(pelem,bar,bb0y,bb1y);
                     tau = par1*hK*sqrt(hK*bb2) - eps;
           break;
      case SCM_LIN:  tau = sc_lin_par(pelem);
           break;
      case SCM_KLR1:
      case SCM_K3:   L2norms_of_cd_res_and_grad_u(pelem,bar,u,space,
                                             eps,bb0,bb1,react,rhs,&res,&grad);
                     if (sc_type == SCM_KLR1){
                        z = L2norm_of_u(pelem,bar,u,space);
                        z = par2 + sqrt(z*z + grad*grad);
                     }
                     else if (sc_type == SCM_K3)
                        z = grad;
                     else
                        eprintf("Error in sc_tau_k.\n");
                     if ( z < 1.e-10 )
                        tau = 0.;
                     else{
                        r = res/z;
                        s = r*diameter(pelem)/(2.*eps);
                        if (par1*s > 1.)
                           tau = 0.5*(par1 - 1./s)*diameter(pelem)*r;
                        else
                           tau = 0.;
                     }
           break;
      default:
                eprintf("Error: sc_tau_k not available for sc_type used.\n");
           break;
      }
      if (non_isotropic_diffusion(sc_type)){
         bb0y = bb0(y);
         bb1y = bb1(y);
         bb2 = bb0y*bb0y + bb1y*bb1y;
         tau /= bb2;
      }
      if (sc_type == SCM_HMM                      || sc_type == SCM_TP1 ||
          sc_type == SCM_TP2 || sc_type == SCM_C  || sc_type == SCM_CS  ||
          sc_type == SCM_K6  || sc_type == SCM_K6red || sc_type == SCM_K6red_iso)
         return(tau);
      else
         return(MAX(0.,tau));
   }
}

/* value of sc_tau at the point F(y_ref) */
FLOAT gsc_tau_k(y_ref,tGrid,pel,eps,bb0,bb1,react,rhs,u,space,sc_type,par1,par2,
                                                              finite_el,ref_map)
GRID *tGrid;
ELEMENT *pel;
FLOAT *y_ref, eps, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), par1, par2;
INT u, space, sc_type;
FINITE_ELEMENT finite_el;
REF_MAPPING ref_map;
{
   FLOAT d=1., grad, grad_u[DIM], y[DIM], g2, hK, hKp, res, rres, t1, t2, tau, 
         h, bbp0, bbp1, bbp, bb0y, bb1y, bb, bb2, zz0, zz1, zz, zz2, r, s, z,
         xi, ak, bk, gk, lk, kk, ok, ro, Pe0, Pe1;

   ref_map_point(y_ref,y,&ref_map);
   switch(sc_type){
   case SCM_HMM:
   case SCM_TP1:
   case SCM_TP2:
   case SCM_C:
   case SCM_CS:
   case SCM_CS1:
   case SCM_K6:
   case SCM_K6red:
   case SCM_K6red_iso:
                  ggrad_value(tGrid,pel,u,y_ref,finite_el,ref_map,grad_u);
                  if ( (g2=DOT(grad_u,grad_u)) < 1.e-15 )
                     tau = 0.;
                  else{
                     res = gscalar_conv_diff_res(tGrid,pel,y_ref,u,
                                       eps,bb0,bb1,react,rhs,finite_el,ref_map);
                     bb0y = bb0(y);
                     bb1y = bb1(y);
                     r = (bb0y*grad_u[0]+bb1y*grad_u[1])/g2;
                     bbp0 = r*grad_u[0];
                     bbp1 = r*grad_u[1];
                     if (sc_type == SCM_HMM){
                        t1 = gsd_tau_classic(pel,eps,bb0y,bb1y,space);
                        t2 = gsd_tau_classic(pel,eps,bbp0,bbp1,space);
                        tau = res*r*MAX(0.,t2-t1);
                     }
                     else if (sc_type == SCM_TP1 || sc_type == SCM_TP2){
                        if ( (bb =sqrt(bb0y*bb0y+bb1y*bb1y)) < 1.e-10 ||
                             (bbp=sqrt(bbp0*bbp0+bbp1*bbp1)) < 1.e-10 )
                           tau = 0.;
                        else{
                           s = bbp/bb;
                           hKp = gsdiameter(pel,bbp0,bbp1);
                           tau = res*r*hKp*s*(1-s)/(bbp*approx_order(space));
                           if (sc_type == SCM_TP2)
                              tau *= hKp*sqrt(g2);
                        }
                     }
                     else if (sc_type == SCM_C   || sc_type == SCM_CS || 
                              sc_type == SCM_CS1 || sc_type == SCM_K6){
                        tau = 0.;
                        if (REG_FABS == YES)
                           rres = regularized_fabs(res,REG_EPS);
                        else
                           rres = fabs(res);
                        s = 0.5*par1*diameter(pel)*rres/sqrt(g2);
                        if (sc_type == SCM_C){
                           z = fabs(bb0y*grad_u[0]+bb1y*grad_u[1]);
                           r = eps*rres;
                           if (s*z > r)
                              tau = s - r/z;
                        }
                        else if (s > eps)
                           tau = s - eps;
                        if (tau > 0. && (sc_type == SCM_CS || 
                                         sc_type == SCM_CS1)){
                           t1 = gsd_tau_classic(pel,eps,bb0y,bb1y,space)
                                *(bb0y*bb0y+bb1y*bb1y);
                           if (sc_type == SCM_CS1)
                              tau -= t1;
                           else if (sc_type == SCM_CS && t1 < tau)
                              tau = t1;
                        }
                     }
                     else if (sc_type == SCM_K6red || sc_type == SCM_K6red_iso)
                        tau = 0.5*par1*diameter(pel)*fabs(res)/sqrt(g2);
                  }
        break;
   case SCM_BE1:
   case SCM_K4:
                  ggrad_value(tGrid,pel,u,y_ref,finite_el,ref_map,grad_u);
                  res = gscalar_conv_diff_res(tGrid,pel,y_ref,u,
                                    eps,bb0,bb1,react,rhs,finite_el,ref_map);
                  bb0y = bb0(y);
                  bb1y = bb1(y);
                  bb2 = bb0y*bb0y + bb1y*bb1y;
                  r = sqrt(bb2*DOT(grad_u,grad_u));
                  if (sc_type == SCM_BE1){
                     res = regularized_fabs(res,2.);
                     xi = fabs(bb0y*grad_u[1]-bb1y*grad_u[0])/sqrt(3.); 
                     if (res < 1.e-10 && xi < 1.e-10)
                        tau = 0.;
                     else{
                        t1 = gsd_tau_classic(pel,eps,bb0y,bb1y,space);
                        s = res + xi;
                        z = (r + res)*s;
                        tau = t1*bb2*res*(r+s)/z;
//                        tau = t1*bb2*res/sqrt(res*res + xi*xi);
                     }
                  }
                  else if (sc_type == SCM_K4){
                     if (REG_FABS == YES)
                        res = regularized_fabs(res,REG_EPS);
                     else
                        res = fabs(res);
                     if (r+res < 1.e-10)
                        tau = 0.;
                     else{
                        t1 = gsd_tau_classic(pel,eps,bb0y,bb1y,space);
                        tau = t1*bb2*res/(r+res);
                     }
                  }
        break;
   case SCM_GC1:
   case SCM_GC2:
   case SCM_AS:
   case SCM_CA:
   case SCM_KLR2:
                  ggrad_value(tGrid,pel,u,y_ref,finite_el,ref_map,grad_u);
                  if ( (g2=DOT(grad_u,grad_u)) < 1.e-15 )
                     tau = 0.;
                  else{
                     res = gscalar_conv_diff_res(tGrid,pel,y_ref,u,
                                    eps,bb0,bb1,react,rhs,finite_el,ref_map);
                     r = res/g2;
                     zz0 = r*grad_u[0];
                     zz1 = r*grad_u[1];
                     bb0y = bb0(y);
                     bb1y = bb1(y);
                     t1 = gsd_tau_classic(pel,eps,bb0y,bb1y,space);
                     if (sc_type == SCM_GC1 || sc_type == SCM_KLR2){
                        t2 = gsd_tau_classic(pel,eps,zz0,zz1,space);
                        if (sc_type == SCM_GC1)
                           tau = res*r*MAX(0.,t2-t1); 
                        else if (sc_type == SCM_KLR2){
                           gL2norms_of_cd_res_and_grad_u(pel,u,space,
                                          eps,bb0,bb1,react,rhs,&res,&grad);
                           z = gL2norm_of_u(pel,u,space);
                           z = res/(par2 + sqrt(z*z + grad*grad));
                           tau = z*z*MAX(0.,t2-t1); 
                        }
                     }
                     else if (sc_type == SCM_CA){
                        t2 = gsd_tau_classic(pel,eps,zz0,zz1,space);
                        tau = res*r*MAX(0.,t2-t1); 
                        if ((zz2=zz0*zz0+zz1*zz1) >= 
                            (bb2=bb0y*bb0y+bb1y*bb1y))
                           ro = 1.;
                        else if (bb2 < 1.e-15)
                           ro = 1.;
                        else{
                           z = zz2/bb2;
                           ak = sqrt(z);
                           s = gsdiameter(pel,bb0y,bb1y);
                           bk = pow(MIN(1.,s),1.-z);
                           gk = 0.5*(ak+bk);
                           gk = MIN(bk,gk);
                           lk = pow(MAX(ak,fabs(res)),3.+0.5*ak+z)/
                                pow(gk,MAX(0.5,0.25+ak));
                           if (lk >= 1.)
                              ro = 1.;
                           else{
                              kk = pow(2.-lk,(1.-lk)/(1.+lk)) - 1.;
                              ok = z*pow(gk,2.-z)/t1;
                              ro = pow(ok*MAX(0.,t2-t1),kk);
                           }
                        }
                        tau *= ro;
                     }
                     else if (sc_type == SCM_GC2){
                        zz2 = zz0*zz0 + zz1*zz1;
                        if ( zz2 < 1.e-15 || fabs(res) < 1.e-10 ||
                                     ((bb2=bb0y*bb0y+bb1y*bb1y) <= zz2) )
                           tau = 0.;
                        else{
                           if (REG_FABS == YES)
                              rres = regularized_fabs(res,REG_EPS);
                           else
                              rres = fabs(res);
                           tau = rres*t1*(sqrt(bb2/g2) - rres/g2);
                        }
                     }
                     else if (sc_type == SCM_AS){
                        zz2 = zz0*zz0 + zz1*zz1;
                        bb2 = bb0y*bb0y + bb1y*bb1y;
                        if ( zz2 < 1.e-15 || fabs(res) < 1.e-10 )
                           tau = 0.;
                        else{
                           s = (bb0y*grad_u[0]+bb1y*grad_u[1])/res;
                           s = sqrt(bb2/zz2) - MAX(1.,s);
                           tau = res*r*t1*MAX(0.,s);
                        }
                     }
                  }
        break;
   case SCM_J:    s = diameter(pel);
                  tau = par1*s*s*fabs(gscalar_conv_diff_res(tGrid,pel,y_ref,
                           u,eps,bb0,bb1,react,rhs,finite_el,ref_map)) - eps;
        break;
   case SCM_JSW:  bb0y = bb0(y);
                  bb1y = bb1(y);
                  bb2 = bb0y*bb0y + bb1y*bb1y;
                  hK = gsdiameter(pel,bb0y,bb1y);
                  tau = par1*hK*sqrt(hK*bb2) - eps;
        break;
   case SCM_LIN:  tau = sc_lin_par(pel);
        break;
   case SCM_KLR1:
   case SCM_K3:   gL2norms_of_cd_res_and_grad_u(pel,u,space,
                                          eps,bb0,bb1,react,rhs,&res,&grad);
                  if (sc_type == SCM_KLR1){
                     z = gL2norm_of_u(pel,u,space);
                     z = par2 + sqrt(z*z + grad*grad);
                  }
                  else if (sc_type == SCM_K3)
                     z = grad;
                  else
                     eprintf("Error in sc_tau_k.\n");
                  if ( z < 1.e-10 )
                     tau = 0.;
                  else{
                     r = res/z;
                     s = r*diameter(pel)/(2.*eps);
                     if (par1*s > 1.)
                        tau = 0.5*(par1 - 1./s)*diameter(pel)*r;
                     else
                        tau = 0.;
                  }
        break;
   default:
             eprintf("Error: gsc_tau_k not available for sc_type used.\n");
        break;
   }
   if (non_isotropic_diffusion(sc_type)){
      bb0y = bb0(y);
      bb1y = bb1(y);
      bb2 = bb0y*bb0y + bb1y*bb1y;
      tau /= bb2;
   }
   if (sc_type == SCM_HMM                      || sc_type == SCM_TP1 ||
       sc_type == SCM_TP2 || sc_type == SCM_C  || sc_type == SCM_CS  ||
       sc_type == SCM_K6  || sc_type == SCM_K6red || sc_type == SCM_K6red_iso)
      return(tau);
   else
      return(MAX(0.,tau));
}

#if SOLVED_PDE == CONV_DIFF

DOUBLE bb_orth0(x)
DOUBLE *x;
{
   return(-bb1(x));
}

DOUBLE bb_orth1(x)
DOUBLE *x;
{
   return(bb0(x));
}

#else

DOUBLE bb_orth0(x)
DOUBLE *x;
{  eprintf("Error: bb_orth0 not available.\n"); return(0.);  }

DOUBLE bb_orth1(x)
DOUBLE *x;
{  eprintf("Error: bb_orth1 not available.\n"); return(0.);  }

#endif

DOUBLE mid_value_on_triangle(pelem,fcn,n,x,w)
ELEMENT *pelem;
DOUBLE (*fcn)(), x[][2], *w;
INT n;
{
   INT k;
   NODE *n0, *n1, *n2;
   FLOAT b[DIM][DIM], a[DIM][DIM], c[DIM], y[DIM], s=0;

   NODES_OF_ELEMENT(n0,n1,n2,pelem);
   P1_reference_mapping0_with_inverse(n0,n1,n2,b,a,c);
   for (k=0; k < n; k++){
      V_LIN_VALUE(y,a,c,x[k][0],x[k][1])
      s += w[k]*fcn(y);
   }
   return(s);
}

DOUBLE mid_sc_tau_on_triangle(pelem,bar,nu,bb0,bb1,react,rhs,u,space,sc_type,
                              beta,beta2,n,x,w)
ELEMENT *pelem;
FLOAT bar[DIM2][DIM2], nu, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(),
      beta, beta2;
INT u, space, sc_type;
DOUBLE x[][2], *w;
INT n;
{
   INT k;
   NODE *n0, *n1, *n2;
   FLOAT bb[DIM][DIM], a[DIM][DIM], c[DIM], y[DIM], s=0;

   NODES_OF_ELEMENT(n0,n1,n2,pelem);
   P1_reference_mapping0_with_inverse(n0,n1,n2,bb,a,c);
   for (k=0; k < n; k++){
      V_LIN_VALUE(y,a,c,x[k][0],x[k][1])
      s += w[k]*sc_tau_k(y,pelem,bar,nu,bb0,bb1,react,rhs,u,space,sc_type,
                         beta,beta2);
   }
   return(s);
}

DOUBLE rhs_in_20(x)
DOUBLE x[DIM];
{
   return(-32.*(x[0]-0.5));
}

DOUBLE rhs_out_20(x)
DOUBLE x[DIM];
{
   return(0.);
}

void add_stab_matr(tGrid,nu,Z,u,bb0,bb1,react,rhs,space,sc_type,beta,beta2)
GRID *tGrid;
FLOAT nu, (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), beta, beta2;
INT Z, u, space, sc_type;
{
   ELEMENT *pelem;
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   FLOAT *x0, *x1, *x2, an0[DIM], an1[DIM], an2[DIM], af0[DIM], af1[DIM], 
         af2[DIM], vn[SIDES][SIDES], vf[SIDES][SIDES], s[SIDES], c[SIDES], 
         b[DIM2][DIM2], rdetB, tau_k=0., xc[DIM];
   INT elcnt=0, type;

   if (sc_type == SCM_BH || sc_type == SCM_BE2 || sc_type == SCM_BE3)
      for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
/*
         coord_of_barycentre(pelem,xc);
//         if (xc[0] < 1.-1./(NVX-1.))
         if (xc[0] > 1.-1./(NVX-1.))
            beta=0.1056624;
         else if (xc[1] < 1./(NVY-1.))
            beta=0.;
         else
            beta=SC_BETA;
*/
         if (SC_EXAMPLE == 20){
            coord_of_barycentre(pelem,xc);
            if (fabs(xc[0]-0.5) < 0.249999 && fabs(xc[1]-0.5) < 0.249999)
               edge_stabilization(tGrid,pelem,nu,Z,u,bb0,bb1,react,rhs_in_20,
                                  space,sc_type,beta,beta2);
            else
               edge_stabilization(tGrid,pelem,nu,Z,u,bb0,bb1,react,rhs_out_20,
                                  space,sc_type,beta,beta2);
         }
         else
            edge_stabilization(tGrid,pelem,nu,Z,u,bb0,bb1,react,rhs,
                               space,sc_type,beta,beta2);
      }
   else if (space == GP1C || space == GQ1C)
      if (non_isotropic_diffusion(sc_type))
         general_stiff_matr(tGrid,Z,nu,beta,beta2,3.,4.,u,space,sc_type,
                            bb0,bb1,react,rhs,NULL,NULL,NULL,NULL,NULL,NULL,
                            general_orth_sc_ref,ELEM,REF_MAP,SC_Q_RULE);
      else
         general_stiff_matr(tGrid,Z,nu,beta,beta2,3.,4.,u,space,sc_type,
                            bb0,bb1,react,rhs,NULL,NULL,NULL,NULL,NULL,NULL,
                            general_sctau_gu_gv_ref,ELEM,REF_MAP,SC_Q_RULE);
   else if (USE_QUADRATURE == YES){
   printf("sc matrix computed using quadrature\n");
   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      barycentric_coordinates(x0,x1,x2,b);
      TAU_K = -1.;
      if (PW_CONST_PAR == YES){
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         get_sc_vectors(pelem,n0,n1,n2,x0,x1,x2,b,
                               bb0,bb1,an0,an1,an2,af0,af1,af2,u,space,sc_type);
         coord_of_barycentre(pelem,xc);
         tau_k = sc_tau_k(xc,pelem,b,nu,bb0,bb1,react,rhs,u,space,sc_type,
                                                                    beta,beta2);
         TAU_K = tau_k;
         lin_coeff0_0 = an0[0]*b[0][0] + an1[0]*b[1][0] + an2[0]*b[2][0];
         lin_coeff0_1 = an0[0]*b[0][1] + an1[0]*b[1][1] + an2[0]*b[2][1];
         lin_coeff0_2 = an0[0]*b[0][2] + an1[0]*b[1][2] + an2[0]*b[2][2];
         lin_coeff1_0 = an0[1]*b[0][0] + an1[1]*b[1][0] + an2[1]*b[2][0];
         lin_coeff1_1 = an0[1]*b[0][1] + an1[1]*b[1][1] + an2[1]*b[2][1];
         lin_coeff1_2 = an0[1]*b[0][2] + an1[1]*b[1][2] + an2[1]*b[2][2];
      }
      if (PW_CONST_PAR == NO || fabs(tau_k) > 1.e-15){
         switch(space){
         case P1C:    
                     if (non_isotropic_diffusion(sc_type)){
   if (PW_CONST_PAR == YES){
      bgu_bgv_p1c_quadr_ij(pelem,0,1,2,Z,b,lin_func0,lin_func1,
                           bb0,bb1,react,rhs,nu,u,space,sc_type,
                           beta,beta2,sc_tau_k,SC_Q_RULE);
      bgu_bgv_p1c_quadr_ij(pelem,1,2,0,Z,b,lin_func0,lin_func1,
                           bb0,bb1,react,rhs,nu,u,space,sc_type,
                           beta,beta2,sc_tau_k,SC_Q_RULE);
      bgu_bgv_p1c_quadr_ij(pelem,2,0,1,Z,b,lin_func0,lin_func1,
                           bb0,bb1,react,rhs,nu,u,space,sc_type,
                           beta,beta2,sc_tau_k,SC_Q_RULE);
   }
   else{
      bgu_bgv_p1c_quadr_ij(pelem,0,1,2,Z,b,bb_orth0,bb_orth1,
                           bb0,bb1,react,rhs,nu,u,space,sc_type,
                           beta,beta2,sc_tau_k,SC_Q_RULE);
      bgu_bgv_p1c_quadr_ij(pelem,1,2,0,Z,b,bb_orth0,bb_orth1,
                           bb0,bb1,react,rhs,nu,u,space,sc_type,
                           beta,beta2,sc_tau_k,SC_Q_RULE);
      bgu_bgv_p1c_quadr_ij(pelem,2,0,1,Z,b,bb_orth0,bb_orth1,
                           bb0,bb1,react,rhs,nu,u,space,sc_type,
                           beta,beta2,sc_tau_k,SC_Q_RULE);
   }
                      }
                      else{
      gu_gv_p1c_quadr_ij(pelem,0,1,2,Z,b,bb0,bb1,react,rhs,nu,
                         u,space,sc_type,beta,beta2,sc_tau_k,SC_Q_RULE);
      gu_gv_p1c_quadr_ij(pelem,1,2,0,Z,b,bb0,bb1,react,rhs,nu,
                         u,space,sc_type,beta,beta2,sc_tau_k,SC_Q_RULE);
      gu_gv_p1c_quadr_ij(pelem,2,0,1,Z,b,bb0,bb1,react,rhs,nu,
                         u,space,sc_type,beta,beta2,sc_tau_k,SC_Q_RULE);
                      }
              break;
         default:
              eprintf("Error: add_stab_matr not available (USE_QUADRATURE == YES).\n");
              break;
         }
      }
   }
   }
   else{
   TAU_K = -1.;
   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(x0,x1,x2,b);
      get_sc_vectors(pelem,n0,n1,n2,x0,x1,x2,b,
                               bb0,bb1,an0,an1,an2,af0,af1,af2,u,space,sc_type);
      lin_coeff0_0 = an0[0]*b[0][0] + an1[0]*b[1][0] + an2[0]*b[2][0];
      lin_coeff0_1 = an0[0]*b[0][1] + an1[0]*b[1][1] + an2[0]*b[2][1];
      lin_coeff0_2 = an0[0]*b[0][2] + an1[0]*b[1][2] + an2[0]*b[2][2];
      lin_coeff1_0 = an0[1]*b[0][0] + an1[1]*b[1][0] + an2[1]*b[2][0];
      lin_coeff1_1 = an0[1]*b[0][1] + an1[1]*b[1][1] + an2[1]*b[2][1];
      lin_coeff1_2 = an0[1]*b[0][2] + an1[1]*b[1][2] + an2[1]*b[2][2];
      coord_of_barycentre(pelem,xc);
      tau_k = sc_tau_k(xc,pelem,b,nu,bb0,bb1,react,rhs,u,space,sc_type,
                       beta,beta2);
      rdetB *= tau_k;
      if (fabs(tau_k) > 1.e-15){
         elcnt++;
         type = non_isotropic_diffusion(sc_type);
         switch(space){
         case P1C:    if (type){ 
                         rdetB /= 60.;
                         sd_p1c_ij(n0,n1,n2,Z,an0,an1,an2,0.,0.,0.,0.,
                                                          b[0],b[1],b[2],rdetB);
                         sd_p1c_ij(n1,n2,n0,Z,an1,an2,an0,0.,0.,0.,0.,
                                                          b[1],b[2],b[0],rdetB);
                         sd_p1c_ij(n2,n0,n1,Z,an2,an0,an1,0.,0.,0.,0.,
                                                          b[2],b[0],b[1],rdetB);
                      }
                      else{
                         laijb_2D(n0,n1,n2,Z,b[0],b[1],b[2],rdetB);
                         laijb_2D(n1,n2,n0,Z,b[1],b[2],b[0],rdetB);
                         laijb_2D(n2,n0,n1,Z,b[2],b[0],b[1],rdetB);
                      }
              break;
         case P2C:    if (type){
                         SET27(af0,af0,4.,an1,-2.,an2,-2.)
                         SET27(af1,af1,4.,an0,-2.,an2,-2.)
                         SET27(af2,af2,4.,an0,-2.,an1,-2.)
                         compute_vn_vf_s_c(an0,an1,an2,af2,af1,af0,b,vn,vf,s,c);
                         conv_stab_ij_sn_sf(0,1,2,n0,n1,n2,fa0,fa1,fa2,
                                                             Z,s,c,vn,vf,rdetB);
                         conv_stab_ij_sn_sf(1,2,0,n1,n2,n0,fa1,fa2,fa0,
                                                             Z,s,c,vn,vf,rdetB);
                         conv_stab_ij_sn_sf(2,0,1,n2,n0,n1,fa2,fa0,fa1,
                                                             Z,s,c,vn,vf,rdetB);
                      }
                      else{
                         aij_sn_sf(n0,n1,n2,fa0,fa1,fa2,Z,b[0],b[1],b[2],rdetB,0);
                         aij_sn_sf(n1,n2,n0,fa1,fa2,fa0,Z,b[1],b[2],b[0],rdetB,0);
                         aij_sn_sf(n2,n0,n1,fa2,fa0,fa1,Z,b[2],b[0],b[1],rdetB,0);
                      }
              break;
         case P1_NC:  if (type){
                         nc_sd_ijb(n0,n1,n2,fa0,fa1,fa2,Z,lin_func0,lin_func1,
                                                zero_func,b[0],b[1],b[2],rdetB);
                         nc_sd_ijb(n1,n2,n0,fa1,fa2,fa0,Z,lin_func0,lin_func1,
                                                zero_func,b[1],b[2],b[0],rdetB);
                         nc_sd_ijb(n2,n0,n1,fa2,fa0,fa1,Z,lin_func0,lin_func1,
                                                zero_func,b[2],b[0],b[1],rdetB);
                      }
                      else{
                         vnc_aij(fa0,fa1,fa2,Z,b[0],b[1],b[2],rdetB,0);
                         vnc_aij(fa1,fa2,fa0,Z,b[1],b[2],b[0],rdetB,0);
                         vnc_aij(fa2,fa0,fa1,Z,b[2],b[0],b[1],rdetB,0);
                      }
              break;
         case P1_MOD: if (type){
                         nc_sd_ijb_mbub(pelem,n0,n1,n2,fa0,fa1,fa2,Z,lin_func0,
                                   lin_func1,zero_func,b[0],b[1],b[2],rdetB,0.);
                         nc_sd_ijb_mbub(pelem,n1,n2,n0,fa1,fa2,fa0,Z,lin_func0,
                                   lin_func1,zero_func,b[1],b[2],b[0],rdetB,0.);
                         nc_sd_ijb_mbub(pelem,n2,n0,n1,fa2,fa0,fa1,Z,lin_func0,
                                   lin_func1,zero_func,b[2],b[0],b[1],rdetB,0.);
                      }
                      else{
                         dvnc_aijb_mbub(n0,n1,n2,fa0,fa1,fa2,Z,b[0],b[1],b[2],
                                                                         rdetB);
                         dvnc_aijb_mbub(n1,n2,n0,fa1,fa2,fa0,Z,b[1],b[2],b[0],
                                                                         rdetB);
                         dvnc_aijb_mbub(n2,n0,n1,fa2,fa0,fa1,Z,b[2],b[0],b[1],
                                                                         rdetB);
                      }
              break;
         default:
              eprintf("Error: add_stab_matr not available.\n");
              break;
         }
      }
   }
// printf("Shock-capturing stabilization added on %i elements.\n",elcnt);
   }
}

void add_MH_conv_term_matr(tGrid,Z,u,f,bb0,bb1,react,rhs,space)
GRID *tGrid;                  /*  Mizukami, Hughes, CMAME 50 (1985), 181-193  */
INT Z, u, f, space;
FLOAT (*bb0)(), (*bb1)(), (*react)(), (*rhs)();
{
   switch(space){
   case P1_NC:   add_P1_NC_MH_conv_term_matr(tGrid,Z,u,f,bb0,bb1,react,rhs);
        break;
   case P1C:     add_P1C_MH_conv_term_matr(tGrid,Z,u,f,bb0,bb1,react,rhs);
        break;
   case Q1C:     add_projected_Q1C_MH_conv_term_matr(tGrid,Z,u,f,
                                                             bb0,bb1,react,rhs);
//               add_Q1C_MH_conv_term_matr(tGrid,Z,u,f,bb0,bb1,react,rhs);
        break;
   default:
        eprintf("Error: add_MH_conv_term_matr not available.\n");
        break;
   }
}

/******************************************************************************/
/*                                                                            */
/*                                  matrix B                                  */
/*                                                                            */
/******************************************************************************/

#if E_DATA & ExDF_MATR

void nc_B_matr(tGrid,Z)
GRID *tGrid;
INT Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT q, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      q = 2.*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                     n2->myvertex->x,b);
      SET2(COEFF_BDFP(pelem,Z,0),b[0],q)
      SET2(COEFF_BDFP(pelem,Z,1),b[1],q)
      SET2(COEFF_BDFP(pelem,Z,2),b[2],q)
   } 
}

#else

void nc_B_matr(tGrid,Z)
GRID *tGrid; INT Z;
{  eprintf("Error: nc_B_matr not available.\n");  }

#endif

#if (E_DATA & ExDF_MATR) && (ELEMENT_TYPE == CUBE) && (DIM == 2)

void q1rot_B_matr(tGrid,Z)
GRID *tGrid;
INT Z;
{
   FACE *fa0, *fa1, *fa2, *fa3;
   ELEMENT *pelem;
   FLOAT h;

   h = FIRSTELEMENT(tGrid)->n[1]->myvertex->x[0] - 
       FIRSTELEMENT(tGrid)->n[0]->myvertex->x[0];
   printf("h = %e\n",h);
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      FACES_OF_4ELEMENT(fa0,fa1,fa2,fa3,pelem);
      COEFF_BDF(pelem,Z,0,0) = 0.;
      COEFF_BDF(pelem,Z,0,1) =  h;
      COEFF_BDF(pelem,Z,1,0) = -h;
      COEFF_BDF(pelem,Z,1,1) = 0.;
      COEFF_BDF(pelem,Z,2,0) = 0.;
      COEFF_BDF(pelem,Z,2,1) = -h;
      COEFF_BDF(pelem,Z,3,0) =  h;
      COEFF_BDF(pelem,Z,3,1) = 0.;
   } 
}

#else

void q1rot_B_matr(tGrid,Z)
GRID *tGrid; INT Z;
{  eprintf("Error: q1rot_B_matr not available.\n");  }

#endif

#if (E_DATA & E_E_FNEIGHBOURS) && (E_DATA & ExE_MATR) && (E_DATA & ExF_MATR)

void p1c_p0_div_stab_matr(tGrid,Z,nu)
GRID *tGrid;
INT Z;
DOUBLE nu;
{
   ELEMENT *pelem, *pel;
   NODE *n0, *n1, *n2;
   FLOAT d[DIM2][DIM], s, v, z, l[SIDES];
   INT i, j;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      SUBTR(n1->myvertex->x,n2->myvertex->x,d[0]);
      SUBTR(n2->myvertex->x,n0->myvertex->x,d[1]);
      SUBTR(n0->myvertex->x,n1->myvertex->x,d[2]);
      l[0] = DOT(d[0],d[0]);
      l[1] = DOT(d[1],d[1]);
      l[2] = DOT(d[2],d[2]);
      s = l[0] + l[1] + l[2];
      v = fabs(d[0][0]*d[1][1] - d[0][1]*d[1][0]);
      for (i = 0; i < SIDES; i++)
         if (IS_BF(pelem->f[i]))
            COEFF_BF(pelem,Z,i) = 0.;
         else{
            pel = NB_EL(pelem,i);
            if (pel->f[0] == pelem->f[i]) j = 0;
            else if (pel->f[1] == pelem->f[i]) j = 1;
            else j = 2;
            NODES_OF_ELEMENT(n0,n1,n2,pel);
            SUBTR(n1->myvertex->x,n2->myvertex->x,d[0]);
            SUBTR(n2->myvertex->x,n0->myvertex->x,d[1]);
            SUBTR(n0->myvertex->x,n1->myvertex->x,d[2]);
            z = (DOT(d[0],d[0]) + DOT(d[1],d[1]) + DOT(d[2],d[2]) + 
                 12.*DOT(d[j],d[j]))/(fabs(d[0][0]*d[1][1] - d[0][1]*d[1][0]));
            COEFF_BF(pelem,Z,i) = -2.*l[i]/(nu*(z + (s + 12.*l[i])/v));
         }
      COEFF_EE(pelem,Z) = -COEFF_BF(pelem,Z,0) - COEFF_BF(pelem,Z,1)
                                               - COEFF_BF(pelem,Z,2);
   }
}

#else

void p1c_p0_div_stab_matr(tGrid,Z,nu)
GRID *tGrid; INT Z; DOUBLE nu;
{  eprintf("Error: p1c_p0_div_stab_matr not available.\n");  }

#endif

#if E_DATA & ExDN_MATR

void p1c_p0_B_matr(tGrid,Z)
GRID *tGrid;
INT Z;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      SET2(COEFF_BNP(pelem,Z,0),b[0],(-rdetB))
      SET2(COEFF_BNP(pelem,Z,1),b[1],(-rdetB))
      SET2(COEFF_BNP(pelem,Z,2),b[2],(-rdetB))
   } 
}

#else

void p1c_p0_B_matr(tGrid,Z)
GRID *tGrid; INT Z;
{  eprintf("Error: p1c_p0_B_matr not available.\n");  }

#endif

#if (E_DATA & ExDN_MATR) && (E_DATA & ExF_MATR)

void p1c_fbub_B_matr(tGrid,Z)
GRID *tGrid;
INT Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT nn0[DIM], nn1[DIM], nn2[DIM], ar0, ar1, ar2, rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      ar0 = normal_vector(n1->myvertex->x,n2->myvertex->x,fa0,nn0);
      ar1 = normal_vector(n0->myvertex->x,n2->myvertex->x,fa1,nn1);
      ar2 = normal_vector(n0->myvertex->x,n1->myvertex->x,fa2,nn2);
      test_normal_vector_direction(nn0,n0->myvertex->x,n1->myvertex->x,&ar0);
      test_normal_vector_direction(nn1,n1->myvertex->x,n2->myvertex->x,&ar1);
      test_normal_vector_direction(nn2,n2->myvertex->x,n0->myvertex->x,&ar2);
      SET2(COEFF_BNP(pelem,Z,0),b[0],(-rdetB))
      SET2(COEFF_BNP(pelem,Z,1),b[1],(-rdetB))
      SET2(COEFF_BNP(pelem,Z,2),b[2],(-rdetB))
      COEFF_BF(pelem,Z,0) = -ar0/6.*FMULT;
      COEFF_BF(pelem,Z,1) = -ar1/6.*FMULT;
      COEFF_BF(pelem,Z,2) = -ar2/6.*FMULT;
   } 
}

#else

void p1c_fbub_B_matr(tGrid,Z)
GRID *tGrid; INT Z;
{  eprintf("Error: p1c_fbub_B_matr not available.\n");  }

#endif

#if (N_DATA & IxD_NODE_MATR) && (E_DATA & ExDN_MATR)

void bijb(pelem,i,n0,n1,n2,Z,b0,b1,b2,tdetB) /* n0 = pelem->n[i] */
ELEMENT *pelem;                                /* tdetB = |K|/3    */
NODE *n0, *n1, *n2;
INT i, Z;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], tdetB;
{
   LINK *pli;

   SET9(COEFFBP(n0,Z),b0,tdetB)
   for (pli=n0->tstart; pli->nbnode != n1; pli=pli->next);
   SET9(COEFFBLP(pli,Z),b1,tdetB)
   for (pli=n0->tstart; pli->nbnode != n2; pli=pli->next);
   SET9(COEFFBLP(pli,Z),b2,tdetB)
   SET2(COEFF_BNP(pelem,Z,i),b0,FMULT*tdetB/20.)
}

#else

void bijb(pelem,i,n0,n1,n2,Z,b0,b1,b2,tdetB) 
ELEMENT *pelem; NODE *n0, *n1, *n2; INT i, Z; FLOAT b0[DIM2], b1[DIM2], b2[DIM2], tdetB;
{  eprintf("Error: bijb not available.\n");  }

#endif

void mini_B_stiff_matr(tGrid,Zbn)
GRID *tGrid;
INT Zbn;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT rdetB, ndetB, b[DIM2][DIM2];
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      ndetB = rdetB/3.;
      bijb(pelem,0,n0,n1,n2,Zbn,b[0],b[1],b[2],ndetB);
      bijb(pelem,1,n1,n2,n0,Zbn,b[1],b[2],b[0],ndetB);
      bijb(pelem,2,n2,n0,n1,Zbn,b[2],b[0],b[1],ndetB);
   }
}

#if (E_DATA & ExDN_MATR) && (E_DATA & ExDF_MATR)

void B_matr_P2C_P0(tGrid,Z)
GRID *tGrid;
INT Z;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT detB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      detB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                     n2->myvertex->x,b);
      detB = -detB;
      SET2(COEFF_BNP(pelem,Z,0),b[0],detB)
      SET2(COEFF_BNP(pelem,Z,1),b[1],detB)
      SET2(COEFF_BNP(pelem,Z,2),b[2],detB)
      detB = -detB/3.;
      SET2(COEFF_BDFP(pelem,Z,0),b[0],detB)
      SET2(COEFF_BDFP(pelem,Z,1),b[1],detB)
      SET2(COEFF_BDFP(pelem,Z,2),b[2],detB)
   }
}

#else

void B_matr_P2C_P0(tGrid,Z)
GRID *tGrid; INT Z;
{  eprintf("Error: B_matr_P2C_P0 not available.\n");  }

#endif

#if (N_DATA & IxD_NODE_MATR) && (N_DATA & Dx1_NODE_FACE_MATR) && (DATA_S & N_LINK_TO_NODES) && (DATA_S & N_LINK_TO_FACES)

void bij_sn_sf(n0,n1,n2,fa0,fa1,fa2,Z,b0,b1,b2,rdetB)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT Z;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{
   LINK *pli;
   NFLINK *pnf;
   FLOAT c[DIM], q;
  
   q = -rdetB/3.;
   SET4(COEFFBP(n0,Z),b0,q)
   for (pli = n0->tstart; pli->nbnode != n1; pli = pli->next);
   SET4(COEFFBLP(pli,Z),b1,q)
   for (pli = n0->tstart; pli->nbnode != n2; pli = pli->next);
   SET4(COEFFBLP(pli,Z),b2,q)
   q = rdetB/12.;
   SUBTR(b1,b2,c);
   for (pnf = n0->tnfstart; pnf->nbface != fa0; pnf = pnf->next);
   SET4(COEFF_NFP(pnf,Z),b0,q)
   for (pnf = n0->tnfstart; pnf->nbface != fa1; pnf = pnf->next);
   SET4(COEFF_NFP(pnf,Z),c,q)
   for (pnf = n0->tnfstart; pnf->nbface != fa2; pnf = pnf->next);
   SET4(COEFF_NFP(pnf,Z),c,-q)
}

void bij_sn_sf_iso(n0,n1,n2,fa0,fa1,fa2,Z)
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT Z;
{
   LINK *pli;
   NFLINK *pnf;
   FLOAT b[2][2][DIM2], jac[DIM2], s=1;
  
   inverse_of_P2_reference_mapping(n0,n1,n2,fa0,fa1,fa2,b,jac);
   if (jac[2] < 0.) s = -1;
   s /= 24.;
   COEFFB(n0,Z,0) += s*(b[1][0][1] + b[0][0][1] + b[0][0][0] +    b[1][0][0] + 
                                               4.*b[1][0][2] + 4.*b[0][0][2]);
   COEFFB(n0,Z,1) += s*(b[1][1][1] + b[0][1][1] + b[0][1][0] +    b[1][1][0] + 
                                               4.*b[1][1][2] + 4.*b[0][1][2]);
   for (pli = n0->tstart; pli->nbnode != n1; pli = pli->next);
   COEFFBL(pli,Z,0) += -s*(b[0][0][1] + b[0][0][0] + 4.*b[0][0][2]);
   COEFFBL(pli,Z,1) += -s*(b[0][1][1] + b[0][1][0] + 4.*b[0][1][2]);
   for (pli = n0->tstart; pli->nbnode != n2; pli = pli->next);
   COEFFBL(pli,Z,0) += -s*(b[1][0][1] + b[1][0][0] + 4.*b[1][0][2]);
   COEFFBL(pli,Z,1) += -s*(b[1][1][1] + b[1][1][0] + 4.*b[1][1][2]);
   s *= 0.2;
   for (pnf = n0->tnfstart; pnf->nbface != fa0; pnf = pnf->next);
   COEFF_NF(pnf,Z,0) += -s*(2.*b[0][0][1] +    b[0][0][0] +    b[1][0][1] + 
                            2.*b[1][0][0] + 5.*b[1][0][2] + 5.*b[0][0][2]);
   COEFF_NF(pnf,Z,1) += -s*(2.*b[0][1][1] +    b[0][1][0] +    b[1][1][1] + 
                            2.*b[1][1][0] + 5.*b[1][1][2] + 5.*b[0][1][2]);
   for (pnf = n0->tnfstart; pnf->nbface != fa1; pnf = pnf->next);
   COEFF_NF(pnf,Z,0) += s*(2.*b[0][0][1] + b[0][0][0] -    b[1][0][0] + 
                                        5.*b[0][0][2] - 5.*b[1][0][2]);
   COEFF_NF(pnf,Z,1) += s*(2.*b[0][1][1] + b[0][1][0] -    b[1][1][0] + 
                                        5.*b[0][1][2] - 5.*b[1][1][2]);
   for (pnf = n0->tnfstart; pnf->nbface != fa2; pnf = pnf->next);
   COEFF_NF(pnf,Z,0) += s*(-b[0][0][1] + b[1][0][1] + 2.*b[1][0][0] - 
                                      5.*b[0][0][2] + 5.*b[1][0][2]);
   COEFF_NF(pnf,Z,1) += s*(-b[0][1][1] + b[1][1][1] + 2.*b[1][1][0] - 
                                      5.*b[0][1][2] + 5.*b[1][1][2]);
}

#else

void bij_sn_sf(n0,n1,n2,fa0,fa1,fa2,Z,b0,b1,b2,rdetB)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z; FLOAT b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{  eprintf("Error: bij_sn_sf not available.\n");  }

void bij_sn_sf_iso(n0,n1,n2,fa0,fa1,fa2,Z)
NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z;
{  eprintf("Error: bij_sn_sf_iso not available.\n");  }

#endif

#if E_DATA & ExDN_MATR

void add_bubble_part_to_B_matr_P2C_P1C(pelem,Z,b0,b1,b2,ndetB)
ELEMENT *pelem;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], ndetB;
INT Z;
{
   SET2(COEFF_BNP(pelem,Z,0),b0,ndetB)
   SET2(COEFF_BNP(pelem,Z,1),b1,ndetB)
   SET2(COEFF_BNP(pelem,Z,2),b2,ndetB)
}

void add_bubble_part_to_B_matr_P2C_P1C_iso(pel,n0,n1,n2,fa0,fa1,fa2,Z)
ELEMENT *pel;
NODE *n0, *n1, *n2; 
FACE *fa0, *fa1, *fa2;
INT Z;
{
   FLOAT b[2][2][DIM2], jac[DIM2], s=1/360.;
  
   inverse_of_P2_reference_mapping(n0,n1,n2,fa0,fa1,fa2,b,jac);
   if (jac[2] < 0.) s *= -1;
   COEFF_BN(pel,Z,0,0) = -s*(b[0][0][1]+3.*b[0][0][2]+b[1][0][0]+3.*b[1][0][2]);
   COEFF_BN(pel,Z,0,1) = -s*(b[0][1][1]+3.*b[0][1][2]+b[1][1][0]+3.*b[1][1][2]);
   COEFF_BN(pel,Z,1,0) =  s*(2.*b[0][0][0]+b[0][0][1]+3.*b[0][0][2]+b[1][0][1]);
   COEFF_BN(pel,Z,1,1) =  s*(2.*b[0][1][0]+b[0][1][1]+3.*b[0][1][2]+b[1][1][1]);
   COEFF_BN(pel,Z,2,0) =  s*(b[0][0][0]+b[1][0][0]+2.*b[1][0][1]+3.*b[1][0][2]);
   COEFF_BN(pel,Z,2,1) =  s*(b[0][1][0]+b[1][1][0]+2.*b[1][1][1]+3.*b[1][1][2]);
}

#else

void add_bubble_part_to_B_matr_P2C_P1C(pelem,Z,b0,b1,b2,ndetB)
ELEMENT *pelem; FLOAT b0[DIM2], b1[DIM2], b2[DIM2], ndetB; INT Z;
{  eprintf("Error: add_bubble_part_to_B_matr_P2C_P1C not available.\n");  }

void add_bubble_part_to_B_matr_P2C_P1C_iso(pel,n0,n1,n2,fa0,fa1,fa2,Z)
ELEMENT *pel; NODE *n0, *n1, *n2; FACE *fa0, *fa1, *fa2; INT Z;
{  eprintf("Error: add_bubble_part_to_B_matr_P2C_P1C_iso not available.\n");  }

#endif

#if MOVING_BOUNDARY == YES
#define ADD_D_MATRIX        add_D_matrix(n0,n1,n2,fa0,fa1,fa2,Z);
#else
#define ADD_D_MATRIX
#endif

void B_matr_P2C_P1C(tGrid,Z,bubble)
GRID *tGrid;
INT Z, bubble;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      bij_sn_sf(n0,n1,n2,fa0,fa1,fa2,Z,b[0],b[1],b[2],rdetB);
      bij_sn_sf(n1,n2,n0,fa1,fa2,fa0,Z,b[1],b[2],b[0],rdetB);
      bij_sn_sf(n2,n0,n1,fa2,fa0,fa1,Z,b[2],b[0],b[1],rdetB);
      if (bubble)
         add_bubble_part_to_B_matr_P2C_P1C(pelem,Z,b[0],b[1],b[2],rdetB/60.);
      ADD_D_MATRIX
   }
}

#if F_DATA & CURVED_FACE_MIDDLE

void B_matr_P2C_P1C_iso(tGrid,Z,bubble)
GRID *tGrid;
INT Z, bubble;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      if (fa0->c_midpoint || fa1->c_midpoint || fa2->c_midpoint){
         bij_sn_sf_iso(n0,n1,n2,fa0,fa1,fa2,Z);
         bij_sn_sf_iso(n1,n2,n0,fa1,fa2,fa0,Z);
         bij_sn_sf_iso(n2,n0,n1,fa2,fa0,fa1,Z);
         if (bubble)
            add_bubble_part_to_B_matr_P2C_P1C_iso(pelem,n0,n1,n2,fa0,fa1,fa2,Z);
      }
      else{
         rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
         bij_sn_sf(n0,n1,n2,fa0,fa1,fa2,Z,b[0],b[1],b[2],rdetB);
         bij_sn_sf(n1,n2,n0,fa1,fa2,fa0,Z,b[1],b[2],b[0],rdetB);
         bij_sn_sf(n2,n0,n1,fa2,fa0,fa1,Z,b[2],b[0],b[1],rdetB);
         if (bubble)
            add_bubble_part_to_B_matr_P2C_P1C(pelem,Z,b[0],b[1],b[2],rdetB/60.);
      }
   }
}

#else

void B_matr_P2C_P1C_iso(tGrid,Z,bubble)
GRID *tGrid; INT Z, bubble;
{  eprintf("Error: B_matr_P2C_P1C_iso not available.\n");  }

#endif

#if E_DATA & ExDF_MATR

void element_B_matr_B_P1disc(pel,Z,b)
ELEMENT *pel;
INT Z;
FLOAT b[DIM2][DIM2];
{
   SET2(COEFF_BDFP(pel,Z,0),b[0],0.1)
   SET2(COEFF_BDFP(pel,Z,1),b[1],0.1)
   SET2(COEFF_BDFP(pel,Z,2),b[2],0.1)
} 

#else

void element_B_matr_B_P1disc(pel,Z,b)
ELEMENT *pel; INT Z; FLOAT b[DIM2][DIM2];
{  eprintf("Error: element_B_matr_B_P1disc not available.\n");  }

#endif

#if (E_DATA & ExFxDN_MATR) && (E_DATA & ExFxDF_MATR)

void element_B_matr_P2CB_P1disc(pel,n0,n1,n2,Z,bubble)
ELEMENT *pel;
NODE *n0, *n1, *n2;
INT Z, bubble;
{
   FLOAT ndetB, b[DIM2][DIM2];

   ndetB = -barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                    n2->myvertex->x,b)/3.;
   SET2(b[0],b[0],ndetB)
   SET2(b[1],b[1],ndetB)
   SET2(b[2],b[2],ndetB)
   SET1(COEFF_BFDNP(pel,Z,0,0),b[0])
   SET1(COEFF_BFDNP(pel,Z,1,0),b[0])
   SET1(COEFF_BFDNP(pel,Z,2,0),b[0])
   SET1(COEFF_BFDNP(pel,Z,0,1),b[1])
   SET1(COEFF_BFDNP(pel,Z,1,1),b[1])
   SET1(COEFF_BFDNP(pel,Z,2,1),b[1])
   SET1(COEFF_BFDNP(pel,Z,0,2),b[2])
   SET1(COEFF_BFDNP(pel,Z,1,2),b[2])
   SET1(COEFF_BFDNP(pel,Z,2,2),b[2])
   SET2(COEFF_BFDFP(pel,Z,0,0),b[0],-0.5)
   SET2(COEFF_BFDFP(pel,Z,1,1),b[1],-0.5)
   SET2(COEFF_BFDFP(pel,Z,2,2),b[2],-0.5)
   SET2(COEFF_BFDFP(pel,Z,0,1),b[0],0.5)
   SET2(COEFF_BFDFP(pel,Z,0,2),b[0],0.5)
   SET2(COEFF_BFDFP(pel,Z,1,0),b[1],0.5)
   SET2(COEFF_BFDFP(pel,Z,1,2),b[1],0.5)
   SET2(COEFF_BFDFP(pel,Z,2,0),b[2],0.5)
   SET2(COEFF_BFDFP(pel,Z,2,1),b[2],0.5)
   if (bubble)
      element_B_matr_B_P1disc(pel,Z,b);
} 

#else  /*  !((E_DATA & ExFxDN_MATR) && (E_DATA & ExFxDF_MATR))  */

void element_B_matr_P2CB_P1disc(pel,n0,n1,n2,Z,bubble)
ELEMENT *pel; NODE *n0, *n1, *n2; INT Z, bubble;
{  eprintf("Error: element_B_matr_P2CB_P1disc not available.\n");  }

#endif

void B_matr_P2CB_P1disc(tGrid,Z,bubble)
GRID *tGrid;
INT Z, bubble;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pel;

   for (pel = FIRSTELEMENT(tGrid);pel != NULL;pel = pel->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pel);
      element_B_matr_P2CB_P1disc(pel,n0,n1,n2,Z,bubble);
   } 
}

#if (F_DATA & CURVED_FACE_MIDDLE) && (E_DATA & ExFxDN_MATR) && (E_DATA & ExFxDF_MATR) && (E_DATA & ExDF_MATR)

void B_matr_P2CB_P1disc_iso(tGrid,Z)
GRID *tGrid;
INT Z;
{
   ELEMENT *pel;
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   FLOAT b[2][2][DIM2], jac[DIM2], r, s, z;
  
   for (pel = FIRSTELEMENT(tGrid);pel != NULL;pel = pel->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pel);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pel);
      if (fa0->c_midpoint || fa1->c_midpoint || fa2->c_midpoint){
         inverse_of_P2_reference_mapping(n0,n1,n2,fa0,fa1,fa2,b,jac);
         if (jac[2] < 0.) s = -1/12.;
         else s = 1/12.;
         z = s*0.1;
         r = z/3.;
         COEFF_BFDN(pel,Z,0,0,0) = (b[1][0][1]+b[0][0][1]+b[0][0][0]
                                   +b[1][0][0]+2*b[1][0][2]+2*b[0][0][2])*s;
         COEFF_BFDN(pel,Z,1,0,0) = (b[1][0][1]+b[0][0][1]+2*b[1][0][2]
                                                         +2*b[0][0][2])*s;
         COEFF_BFDN(pel,Z,2,0,0) = (b[0][0][0]+b[1][0][0]+2*b[1][0][2]
                                                         +2*b[0][0][2])*s;
         COEFF_BFDN(pel,Z,0,1,0) = -(b[0][0][1]+b[0][0][0]+2*b[0][0][2])*s;
         COEFF_BFDN(pel,Z,1,1,0) = -(b[0][0][1]+2*b[0][0][2])*s;
         COEFF_BFDN(pel,Z,2,1,0) = -(b[0][0][0]+2*b[0][0][2])*s;
         COEFF_BFDN(pel,Z,0,2,0) = -(b[1][0][1]+b[1][0][0]+2*b[1][0][2])*s;
         COEFF_BFDN(pel,Z,1,2,0) = -(b[1][0][1]+2*b[1][0][2])*s;
         COEFF_BFDN(pel,Z,2,2,0) = -(b[1][0][0]+2*b[1][0][2])*s;
         COEFF_BFDF(pel,Z,0,0,0) = -(6*b[0][0][1]+3*b[0][0][0]+3*b[1][0][1]
                                  +6*b[1][0][0]+10*b[1][0][2]+10*b[0][0][2])*z;
         COEFF_BFDF(pel,Z,1,0,0) = -(b[0][0][0]+b[1][0][1]+6*b[0][0][1]
                                            -2*b[1][0][0]+10*b[0][0][2])*z;
         COEFF_BFDF(pel,Z,2,0,0) = -(b[0][0][0]+b[1][0][1]+6*b[1][0][0]
                                            -2*b[0][0][1]+10*b[1][0][2])*z;
         COEFF_BFDF(pel,Z,0,1,0) = (6*b[0][0][1]+3*b[0][0][0]+2*b[1][0][0]
                                   +5*b[1][0][1]+10*b[0][0][2]+10*b[1][0][2])*z;
         COEFF_BFDF(pel,Z,1,1,0) = (6*b[0][0][1]+3*b[1][0][1]+b[0][0][0]
                                                          +10*b[0][0][2])*z;
         COEFF_BFDF(pel,Z,2,1,0) = -(-b[0][0][0]+2*b[0][0][1]+2*b[1][0][0]
                                                +3*b[1][0][1]+10*b[1][0][2])*z;
         COEFF_BFDF(pel,Z,0,2,0) = (2*b[0][0][1]+5*b[0][0][0]+3*b[1][0][1]
                                   +6*b[1][0][0]+10*b[0][0][2]+10*b[1][0][2])*z;
         COEFF_BFDF(pel,Z,1,2,0) = -(2*b[0][0][1]+3*b[0][0][0]-b[1][0][1]
                                                 +2*b[1][0][0]+10*b[0][0][2])*z;
         COEFF_BFDF(pel,Z,2,2,0) = (3*b[0][0][0]+b[1][0][1]+6*b[1][0][0]
                                                          +10*b[1][0][2])*z;
         COEFF_BDF(pel,Z,0,0) = (2*b[0][0][1]+3*b[0][0][0]+2*b[1][0][0]
                                +6*b[0][0][2]+3*b[1][0][1]+6*b[1][0][2])*r;
         COEFF_BDF(pel,Z,1,0) = -(2*b[0][0][1]+b[0][0][0]-b[1][0][1]
                                                       +6*b[0][0][2])*r;
         COEFF_BDF(pel,Z,2,0) = -(-b[0][0][0]+2*b[1][0][0]+b[1][0][1]
                                                        +6*b[1][0][2])*r;

         COEFF_BFDN(pel,Z,0,0,1) = (b[1][1][1]+b[0][1][1]+b[0][1][0]
                                   +b[1][1][0]+2*b[1][1][2]+2*b[0][1][2])*s;
         COEFF_BFDN(pel,Z,1,0,1) = (b[1][1][1]+b[0][1][1]+2*b[1][1][2]
                                                         +2*b[0][1][2])*s;
         COEFF_BFDN(pel,Z,2,0,1) = (b[0][1][0]+b[1][1][0]+2*b[1][1][2]
                                                         +2*b[0][1][2])*s;
         COEFF_BFDN(pel,Z,0,1,1) = -(b[0][1][1]+b[0][1][0]+2*b[0][1][2])*s;
         COEFF_BFDN(pel,Z,1,1,1) = -(b[0][1][1]+2*b[0][1][2])*s;
         COEFF_BFDN(pel,Z,2,1,1) = -(b[0][1][0]+2*b[0][1][2])*s;
         COEFF_BFDN(pel,Z,0,2,1) = -(b[1][1][1]+b[1][1][0]+2*b[1][1][2])*s;
         COEFF_BFDN(pel,Z,1,2,1) = -(b[1][1][1]+2*b[1][1][2])*s;
         COEFF_BFDN(pel,Z,2,2,1) = -(b[1][1][0]+2*b[1][1][2])*s;
         COEFF_BFDF(pel,Z,0,0,1) = -(6*b[0][1][1]+3*b[0][1][0]+3*b[1][1][1]
                                  +6*b[1][1][0]+10*b[1][1][2]+10*b[0][1][2])*z;
         COEFF_BFDF(pel,Z,1,0,1) = -(b[0][1][0]+b[1][1][1]+6*b[0][1][1]
                                            -2*b[1][1][0]+10*b[0][1][2])*z;
         COEFF_BFDF(pel,Z,2,0,1) = -(b[0][1][0]+b[1][1][1]+6*b[1][1][0]
                                            -2*b[0][1][1]+10*b[1][1][2])*z;
         COEFF_BFDF(pel,Z,0,1,1) = (6*b[0][1][1]+3*b[0][1][0]+2*b[1][1][0]
                                   +5*b[1][1][1]+10*b[0][1][2]+10*b[1][1][2])*z;
         COEFF_BFDF(pel,Z,1,1,1) = (6*b[0][1][1]+3*b[1][1][1]+b[0][1][0]
                                                          +10*b[0][1][2])*z;
         COEFF_BFDF(pel,Z,2,1,1) = -(-b[0][1][0]+2*b[0][1][1]+2*b[1][1][0]
                                                +3*b[1][1][1]+10*b[1][1][2])*z;
         COEFF_BFDF(pel,Z,0,2,1) = (2*b[0][1][1]+5*b[0][1][0]+3*b[1][1][1]
                                   +6*b[1][1][0]+10*b[0][1][2]+10*b[1][1][2])*z;
         COEFF_BFDF(pel,Z,1,2,1) = -(2*b[0][1][1]+3*b[0][1][0]-b[1][1][1]
                                                 +2*b[1][1][0]+10*b[0][1][2])*z;
         COEFF_BFDF(pel,Z,2,2,1) = (3*b[0][1][0]+b[1][1][1]+6*b[1][1][0]
                                                          +10*b[1][1][2])*z;
         COEFF_BDF(pel,Z,0,1) = (2*b[0][1][1]+3*b[0][1][0]+2*b[1][1][0]
                                +6*b[0][1][2]+3*b[1][1][1]+6*b[1][1][2])*r;
         COEFF_BDF(pel,Z,1,1) = -(2*b[0][1][1]+b[0][1][0]-b[1][1][1]
                                                       +6*b[0][1][2])*r;
         COEFF_BDF(pel,Z,2,1) = -(-b[0][1][0]+2*b[1][1][0]+b[1][1][1]
                                                        +6*b[1][1][2])*r;
      }
      else
         element_B_matr_P2CB_P1disc(pel,n0,n1,n2,Z,1);
   }
}

#else

void B_matr_P2CB_P1disc_iso(tGrid,Z)
GRID *tGrid; INT Z;
{  eprintf("Error: B_matr_P2CB_P1disc_iso not available.\n");  }

#endif

#if (F_DATA & DVECTOR_FACE_DATA) && (E_DATA & ExDF_MATR) && (E_DATA & SCALAR_ELEMENT_DATA)

void P0_P1mod_B_matr_mbub(tGrid,Z)
GRID *tGrid;
INT Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT nn0[DIM], nn1[DIM], nn2[DIM], ar0, ar1, ar2;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      ar0 = normal_vector(n1->myvertex->x,n2->myvertex->x,fa0,nn0);
      ar1 = normal_vector(n2->myvertex->x,n0->myvertex->x,fa1,nn1);
      ar2 = normal_vector(n0->myvertex->x,n1->myvertex->x,fa2,nn2);
      test_normal_vector_direction(nn0,n0->myvertex->x,n1->myvertex->x,&ar0);
      test_normal_vector_direction(nn1,n1->myvertex->x,n2->myvertex->x,&ar1);
      test_normal_vector_direction(nn2,n2->myvertex->x,n0->myvertex->x,&ar2);
      SET2(COEFF_BDFP(pelem,Z,0),nn0,(-ar0))
      SET2(COEFF_BDFP(pelem,Z,1),nn1,(-ar1))
      SET2(COEFF_BDFP(pelem,Z,2),nn2,(-ar2))
   } 
}

#else  /*  !((F_DATA & DVECTOR_FACE_DATA) && (E_DATA & ExDF_MATR) &&
             (E_DATA & SCALAR_ELEMENT_DATA))  */

void P0_P1mod_B_matr_mbub(tGrid,Z)
GRID *tGrid; INT Z;
{  eprintf("Error: P0_P1mod_B_matr_mbub not available.\n");  }

#endif

#if (F_DATA & DVECTOR_FACE_DATA) && (E_DATA & ExFx2DF_MATR) && (E_DATA & SCALAR_DATA_IN_ELEMENT_NODES)

void dvnc_bijb1(pel,Z,ni,nj,nk,fk,i,j,k,bi,bj,bk,rdetB)
ELEMENT *pel;
NODE *ni, *nj, *nk;
FACE *fk;
INT Z, i, j, k;
FLOAT bi[DIM2], bj[DIM2], bk[DIM2], rdetB;
{
   FLOAT nnk[DIM], ark;

   ark = normal_vector(ni->myvertex->x,nj->myvertex->x,fk,nnk);
   test_normal_vector_direction(nnk,nk->myvertex->x,ni->myvertex->x,&ark);
   rdetB *= -2./3.;
   COEFF_BDFS(pel,Z,i,k,0,0) = rdetB*bi[0];
   COEFF_BDFS(pel,Z,i,k,0,1) = rdetB*bi[1];
   COEFF_BDFS(pel,Z,j,k,0,0) = rdetB*bj[0];
   COEFF_BDFS(pel,Z,j,k,0,1) = rdetB*bj[1];
   COEFF_BDFS(pel,Z,k,k,0,0) = rdetB*bk[0] - ark*nnk[0];
   COEFF_BDFS(pel,Z,k,k,0,1) = rdetB*bk[1] - ark*nnk[1];
   ark *= NINDI(nj,ni)/30.;
   COEFF_BDFS(pel,Z,i,k,1,0) =  ark*nnk[0];
   COEFF_BDFS(pel,Z,i,k,1,1) =  ark*nnk[1];
   COEFF_BDFS(pel,Z,j,k,1,0) = -COEFF_BDFS(pel,Z,i,k,1,0); 
   COEFF_BDFS(pel,Z,j,k,1,1) = -COEFF_BDFS(pel,Z,i,k,1,1);
   COEFF_BDFS(pel,Z,k,k,1,0) =  0.;
   COEFF_BDFS(pel,Z,k,k,1,1) =  0.;
}

void P1disc_P1mod_B_matr_mbub(tGrid,Z)
GRID *tGrid;
INT Z;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      dvnc_bijb1(pelem,Z,n0,n1,n2,fa2,0,1,2,b[0],b[1],b[2],rdetB);
      dvnc_bijb1(pelem,Z,n1,n2,n0,fa0,1,2,0,b[1],b[2],b[0],rdetB);
      dvnc_bijb1(pelem,Z,n2,n0,n1,fa1,2,0,1,b[2],b[0],b[1],rdetB);
   } 
}

#else  /*  !((F_DATA & DVECTOR_FACE_DATA) && (E_DATA & ExFx2DF_MATR) && 
             (E_DATA & SCALAR_DATA_IN_ELEMENT_NODES))  */

void P1disc_P1mod_B_matr_mbub(tGrid,Z)
GRID *tGrid; INT Z;
{  eprintf("Error: P1disc_P1mod_B_matr_mbub not available.\n");  }

#endif

#if DATA_S & SPECIAL_NODES_AND_FACES

void p2_lagrange_mult_matr(tGrid,Z)
GRID *tGrid;
INT Z;
{
   SNODE *psn;
   SFACE *psf;
   FLOAT n[DIM], n1[DIM], n2[DIM], d, d1=1., d2=1.;

   for (psn = FIRSTSN(tGrid); psn; psn = psn->succ)
      if (IS_LN(psn->n0)){
         if (IS_LF(psn->f1)){
            if (psn->n0->index < psn->n1->index)
               d1 = normal_vector(psn->n0->myvertex->x,psn->n1->myvertex->x,
                                                                    psn->f1,n1);
            else
               d1 = normal_vector(psn->n1->myvertex->x,psn->n0->myvertex->x,
                                                                    psn->f1,n1);
         }
         else
            n1[0] = n1[1] = 0.;
         if (psn->pel2 && IS_LF(psn->f2)){
            if (psn->n0->index < psn->n2->index)
               d2 = normal_vector(psn->n0->myvertex->x,psn->n2->myvertex->x,
                                                                    psn->f2,n2);
            else
               d2 = normal_vector(psn->n2->myvertex->x,psn->n0->myvertex->x,
                                                                    psn->f2,n2);
         }
         else
            n2[0] = n2[1] = 0.;
         d1 /= 3.;
         d2 /= 3.;
         SET20(psn->ann0[Z],n1,d1,n2,d2)
         d1 *= 0.5;
         d2 *= 0.5;
         SET2(psn->ann1[Z],n1,d1)
         SET2(psn->ann2[Z],n2,d2)
         d1 *= 0.5;
         d2 *= 0.5;
         SET2(psn->anf1[Z],n1,d1)
         SET2(psn->anf2[Z],n2,d2)
      }
      else
         psn->ann0[Z][0] = psn->ann1[Z][0] = psn->ann2[Z][0] = 
         psn->ann0[Z][1] = psn->ann1[Z][1] = psn->ann2[Z][1] = 
         psn->anf1[Z][0] = psn->anf2[Z][0] =
         psn->anf1[Z][1] = psn->anf2[Z][1] = 0.;
   for (psf = FIRSTSF(tGrid); psf; psf = psf->succ)
      if (IS_LF(psf->f)){
         if (psf->n1->index < psf->n2->index)
          d = normal_vector(psf->n1->myvertex->x,psf->n2->myvertex->x,psf->f,n);
         else
          d = normal_vector(psf->n2->myvertex->x,psf->n1->myvertex->x,psf->f,n);
         d /= 30.;
         SET2(psf->aff[Z],n,d)
         d *= 2.5;
         SET2(psf->afn1[Z],n,d)
         SET2(psf->afn2[Z],n,d)
      }
      else
         psf->aff[Z][0] = psf->afn1[Z][0] = psf->afn2[Z][0] = 
         psf->aff[Z][1] = psf->afn1[Z][1] = psf->afn2[Z][1] = 0.;
}

#else

void p2_lagrange_mult_matr(tGrid,Z)
GRID *tGrid; INT Z;
{  eprintf("Error: p2_lagrange_mult_matr not available.\n");  }

#endif

void make_B_matr(tGrid,Z,u_space,p_space)
GRID *tGrid;
INT Z, u_space, p_space;
{
   if (u_space == P1_NC && p_space == P0)
      nc_B_matr(tGrid,Z);
   else if (u_space == P1_MOD && p_space == P0)
      P0_P1mod_B_matr_mbub(tGrid,Z);
   else if (u_space == P1_MOD && p_space == P1_DISC)
      P1disc_P1mod_B_matr_mbub(tGrid,Z);
   else if (u_space == Q1ROT && p_space == P0)
      q1rot_B_matr(tGrid,Z);
   else if (u_space == P1C && p_space == P0)
      p1c_p0_B_matr(tGrid,Z);
   else if ((u_space == P1C_FBUB || u_space == P1C_NEW_FBUB) && p_space == P0)
      p1c_fbub_B_matr(tGrid,Z);
   else if (u_space == P1C_ELBUB && p_space == P1C)
      mini_B_stiff_matr(tGrid,Z);
   else if ((u_space == P2C || u_space == P2C_ELBUB) && p_space == P0)
      B_matr_P2C_P0(tGrid,Z);
   else if (u_space == P2C && p_space == P1C)
      B_matr_P2C_P1C(tGrid,Z,0);
   else if (u_space == IP2C && p_space == IP1C){
      if (MOVING_BOUNDARY == YES)
         mso_iso_B_matr_P2C_P1C(tGrid, Z);
      else
         B_matr_P2C_P1C_iso(tGrid,Z,0);
   }
   else if (u_space == P2C && p_space == P1_DISC)
      B_matr_P2CB_P1disc(tGrid,Z,0);
   else if (u_space == P2C_ELBUB && p_space == P1C)
      B_matr_P2C_P1C(tGrid,Z,1);
   else if (u_space == IP2C_ELBUB && p_space == IP1C)
      B_matr_P2C_P1C_iso(tGrid,Z,1);
   else if (u_space == P2C_ELBUB && p_space == P1_DISC)
      B_matr_P2CB_P1disc(tGrid,Z,1);
   else if (u_space == IP2C_ELBUB && p_space == IP1_DISC)
      B_matr_P2CB_P1disc_iso(tGrid,Z);
   else if (u_space == P2C && p_space == P1C_P2L){
      B_matr_P2C_P1C(tGrid,Z,0);
      p2_lagrange_mult_matr(tGrid,Z);
   }
   else if (u_space == P2C_ELBUB && p_space == P1C_P2L){
      B_matr_P2C_P1C(tGrid,Z,1);
      p2_lagrange_mult_matr(tGrid,Z);
   }
   else
      eprintf("Error: make_B_matr not available.\n");
}

void make_div_stab_matr(tGrid,Z,nu,u_space,p_space)
GRID *tGrid;
INT Z, u_space, p_space;
DOUBLE nu;
{
   if (u_space == P1C && p_space == P0)
      p1c_p0_div_stab_matr(tGrid,Z,nu);
   else
      eprintf("Error: make_div_stab_matr not available.\n");
}

/******************************************************************************/
/*                                                                            */
/*                     integration of the right-hand side                     */
/*                                                                            */
/******************************************************************************/

DOUBLE integrate_1d(a,b,f,qr)  /*  integrates f over the interval (a,b)  */
DOUBLE a, b, (*f)();
QUADR_RULE_1D qr;
{
   INT k;
   FLOAT r=(b-a)*0.5, s=(b+a)*0.5, sum=0.;

   for (k=0; k < qr.n; k++)
      sum += qr.weights[k]*f(r*qr.points[k]+s);
   return(r*sum);
}

/* Integrates  \int_K f (phi\circ F^{-1}) dx using the reference element.
   The reference mapping F is determined by a and c. */
FLOAT integr_f_X_phi_p1(f,phi,a,c,vol,qr)
FLOAT (*f)(), (*phi)(), a[DIM][DIM], c[DIM], vol;
QUADRATURE_RULE qr;
{
   INT k;
   FLOAT s=0;

   for (k=0; k < qr.n; k++)
      s += qr.weights[k]*fcn_lin_value(qr.points[k],a,c,f)*phi(qr.points[k]);
   return(s*vol);
}

FLOAT integr_f_X_phi_q1(f,phi,a,alpha,c,jac,n,x,w)
INT n;
FLOAT (*f)(), (*phi)(), a[DIM][DIM], alpha[DIM], c[DIM], jac[DIM2], x[][2], *w;
{
   INT k;
   FLOAT s=0;

   for (k=0; k < n; k++)
      s += w[k]*fcn_bilin_value(x[k],a,c,alpha,f)*phi(x[k])*fabs(LINV(jac,x[k]));
   return(s*QR_VOL);
}

/* computes approximately \int_K f0 l_1\dx (exactly for linear f0) */
FLOAT integr1(x1,x2,x3,f0,detB)            /*  detB = volume of K  */
FLOAT (*f0)();
FLOAT x1[DIM],x2[DIM],x3[DIM],detB;
{
   return( (2.0* (*f0)(x1) + (*f0)(x2) + (*f0)(x3))*detB/12.0 );
}
 
 /* computes approximately \int_K f0\cdot p^0\dx (exactly for linear f0), where
    p^0=l_1*l_2*nn */
 FLOAT integr3(n0,n1,n2,nn,detB,rhs0,rhs1)
 NODE *n0,*n1,*n2;
 FLOAT nn[DIM], detB, (*rhs0)(), (*rhs1)();
 {
   FLOAT *x0, *x1, *x2;
   
   x0 = n0->myvertex->x;
   x1 = n1->myvertex->x;
   x2 = n2->myvertex->x;
   
   return(( ( rhs0(x0)/2.0 + rhs0(x1) + rhs0(x2) )*nn[0] +
            ( rhs1(x0)/2.0 + rhs1(x1) + rhs1(x2) )*nn[1] )*detB/30.0);
 }

 /* analogous computation as integr3 on the tetrahedron (n1,n2,n3,xc) */
 FLOAT integr3c(n0,n1,n2,nn,detB,rhs0,rhs1)/* bar. coord. w.r.t. (n1,n2,n3,xc)*/
 NODE *n0,*n1,*n2; 
 FLOAT nn[DIM], detB, (*rhs0)(), (*rhs1)(); /* detB = volume of (n1,n2,n3,n0) */
 {
   FLOAT *x0, *x1, *x2, xc[DIM];
   
   x0 = n0->myvertex->x;
   x1 = n1->myvertex->x;
   x2 = n2->myvertex->x;
   POINT3(x0,x1,x2,xc);
   
   return(( ( rhs0(xc)/2.0 + rhs0(x1) + rhs0(x2) )*nn[0] +
            ( rhs1(xc)/2.0 + rhs1(x1) + rhs1(x2) )*nn[1] )*detB/90.0);
 }

/*
 FLOAT integr3p(n0,n1,n2,nn,detB)
 NODE *n0,*n1,*n2;
 FLOAT nn[DIM], detB;
 {
   FLOAT *x0, *x1, *x2, xc[DIM];
   
   x0 = n0->myvertex->x;
   x1 = n1->myvertex->x;
   x2 = n2->myvertex->x;
   POINT3(x0,x1,x2,xc);
   
   return(( ( fp1(xc)/2.0 + fp1(x1) + fp1(x2) )*nn[0] +
            ( fp2(xc)/2.0 + fp2(x1) + fp2(x2) )*nn[1] )*detB/90.0);
 }
*/

/* computes approximately \int_K f0 l_1 l_2 l_3\dx (exactly for linear f0)*/
void integr3B(n0,n1,n2,a,detB,rhs0,rhs1)
NODE *n0,*n1,*n2;
FLOAT a[DIM], detB, (*rhs0)(), (*rhs1)();
{
   FLOAT *x0, *x1, *x2;

   x0 = n0->myvertex->x;
   x1 = n1->myvertex->x;
   x2 = n2->myvertex->x;
   a[0] = ( rhs0(x0) + rhs0(x1) + rhs0(x2) )*detB/180.;
   a[1] = ( rhs1(x0) + rhs1(x1) + rhs1(x2) )*detB/180.;
}

/* computes approximately \int_K f0 l_12\dx (exactly for linear f0), where    */
FLOAT integr11(n1,n2,n3,f0,detB)         /*  l_12 is the linear nonconforming */
NODE *n1, *n2, *n3;                      /*  basis function                   */
FLOAT (*f0)(), detB;                     /*  detB = volume of K               */
{
   return( ( (*f0)(n1->myvertex->x) + (*f0)(n2->myvertex->x) )*detB/6. );
}

/* computes approximately \int_K f0 fi_12\dx (exactly for linear f0), where   */
FLOAT integr12(n1,n2,n3,f0,detB)         /*  fi_12 is the modified linear     */
NODE *n1, *n2, *n3;                      /*  nonconforming basis function     */
FLOAT (*f0)(), detB;                     /*  detB = volume of K               */
{
   return( ( (*f0)(n1->myvertex->x) + 
             (*f0)(n2->myvertex->x) +
             (*f0)(n3->myvertex->x) )*detB/9. );
}

/* computes approximately \int_K f0 chi_12\dx (exactly for linear f0), where  */
FLOAT integr13(n1,n2,n3,f0,detB)         /*  chi_12 is a pw. cubic            */
NODE *n1, *n2, *n3;                      /*  conforming bubble function       */
FLOAT (*f0)(), detB;                     /*  detB = volume of K               */
{
   return( 
     ((*f0)(n1->myvertex->x) - (*f0)(n2->myvertex->x))*detB/180.*NINDI(n2,n1) );
}

#if (E_DATA & VECTOR_ELEMENT_DATA) && (ELEMENT_TYPE == SIMPLEX)

void set_edv(pelem,f,k,v)
ELEMENT *pelem;
FLOAT v;
INT f, k;
{
   EDV(pelem,f,k) = v;
}

#else

void set_edv(pelem,f,k,v)
ELEMENT *pelem; FLOAT v; INT f, k;
{  eprintf("Error: set_edv not available.\n");  }

#endif

#if (N_DATA & VECTOR_NODE_DATA) && (ELEMENT_TYPE == SIMPLEX)

void integrate_rhs_mini_lin_div_free_1(tGrid,f,rhs0,rhs1)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, f0, f1, f2, ndetB, s;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      ndetB = VOLUME(pelem)/36.;
      POINT_VALUES_3(x0,x1,x2,f0,f1,f2,rhs0)
      s = f0 + f1 + f2;
      ND(n0,f,0) += (4.*f0 - s)*ndetB;
      ND(n1,f,0) += (4.*f1 - s)*ndetB;
      ND(n2,f,0) += (4.*f2 - s)*ndetB;
      POINT_VALUES_3(x0,x1,x2,f0,f1,f2,rhs1)
      s = f0 + f1 + f2;
      ND(n0,f,1) += (4.*f0 - s)*ndetB;
      ND(n1,f,1) += (4.*f1 - s)*ndetB;
      ND(n2,f,1) += (4.*f2 - s)*ndetB;
   }
}

void integrate_rhs_vn_1(tGrid,f,rhs0,rhs1)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT rdetB; 

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      rdetB = VOLUME(pelem);
      ND(n0,f,0) += integr1(n0->myvertex->x,n1->myvertex->x,n2->myvertex->x,rhs0,rdetB);
      ND(n1,f,0) += integr1(n1->myvertex->x,n2->myvertex->x,n0->myvertex->x,rhs0,rdetB);
      ND(n2,f,0) += integr1(n2->myvertex->x,n0->myvertex->x,n1->myvertex->x,rhs0,rdetB);
      ND(n0,f,1) += integr1(n0->myvertex->x,n1->myvertex->x,n2->myvertex->x,rhs1,rdetB);
      ND(n1,f,1) += integr1(n1->myvertex->x,n2->myvertex->x,n0->myvertex->x,rhs1,rdetB);
      ND(n2,f,1) += integr1(n2->myvertex->x,n0->myvertex->x,n1->myvertex->x,rhs1,rdetB);
   } 
}

void integrate_rhs_vn_3(tGrid,f,rhs0,rhs1,bubble)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f, bubble;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, rdetB, c;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      rdetB = VOLUME(pelem);
      points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012);
      POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                      f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs0)
      c = f0 + f1 + f2 + 18.*f012;
      ND(n0,f,0) += INTEGR4_2(f0,f001,f002,c)*rdetB;
      ND(n1,f,0) += INTEGR4_2(f1,f110,f112,c)*rdetB;
      ND(n2,f,0) += INTEGR4_2(f2,f220,f221,c)*rdetB;
      if (bubble){
         c = f001 + f002 + f110 + f112 + f220 + f221;
         set_edv(pelem,f,0,(2.*(f0+f1+f2) + 9.*c + 108.*f012)*rdetB/10080.);
      }
      POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                      f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs1)
      c = f0 + f1 + f2 + 18.*f012;
      ND(n0,f,1) += INTEGR4_2(f0,f001,f002,c)*rdetB;
      ND(n1,f,1) += INTEGR4_2(f1,f110,f112,c)*rdetB;
      ND(n2,f,1) += INTEGR4_2(f2,f220,f221,c)*rdetB;
      if (bubble){
         c = f001 + f002 + f110 + f112 + f220 + f221;
         set_edv(pelem,f,1,(2.*(f0+f1+f2) + 9.*c + 108.*f012)*rdetB/10080.);
      }
   }
}

#else

void integrate_rhs_mini_lin_div_free_1(tGrid,f,rhs0,rhs1)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f;
{  eprintf("Error: integrate_rhs_mini_lin_div_free_1 not available.\n");  } 

void integrate_rhs_vn_1(tGrid,f,rhs0,rhs1)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f;
{  eprintf("Error: integrate_rhs_vn_1 not available.\n");  }

void integrate_rhs_vn_3(tGrid,f,rhs0,rhs1,bubble)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f, bubble;
{  eprintf("Error: integrate_rhs_vn_3 not available.\n");  }

#endif

#if (N_DATA & VECTOR_NODE_DATA) && (F_DATA & SCALAR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX)

void integr_rhs_p1c_fbub(tGrid,f,rhs0,rhs1)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT nn0[DIM], nn1[DIM], nn2[DIM], rdetB; 

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = VOLUME(pelem);
      ND(n0,f,0) += integr1(n0->myvertex->x,n1->myvertex->x,n2->myvertex->x,rhs0,rdetB);
      ND(n1,f,0) += integr1(n1->myvertex->x,n2->myvertex->x,n0->myvertex->x,rhs0,rdetB);
      ND(n2,f,0) += integr1(n2->myvertex->x,n0->myvertex->x,n1->myvertex->x,rhs0,rdetB);
      ND(n0,f,1) += integr1(n0->myvertex->x,n1->myvertex->x,n2->myvertex->x,rhs1,rdetB);
      ND(n1,f,1) += integr1(n1->myvertex->x,n2->myvertex->x,n0->myvertex->x,rhs1,rdetB);
      ND(n2,f,1) += integr1(n2->myvertex->x,n0->myvertex->x,n1->myvertex->x,rhs1,rdetB);
      normal_vector(n1->myvertex->x,n2->myvertex->x,fa0,nn0);
      normal_vector(n0->myvertex->x,n2->myvertex->x,fa1,nn1);
      normal_vector(n0->myvertex->x,n1->myvertex->x,fa2,nn2);
      if ((PROBLEM & WITH_F) && !(U_SPACE == P1C_NEW_FBUB)){
         FD(fa0,f) +=  integr3(n0,n1,n2,nn0,rdetB,rhs0,rhs1)*FMULT;
         FD(fa1,f) +=  integr3(n1,n2,n0,nn1,rdetB,rhs0,rhs1)*FMULT;
         FD(fa2,f) +=  integr3(n2,n0,n1,nn2,rdetB,rhs0,rhs1)*FMULT;
      }
      else if ((PROBLEM & WITH_F) && (U_SPACE == P1C_NEW_FBUB)){
         FD(fa0,f) +=  integr3c(n0,n1,n2,nn0,rdetB,rhs0,rhs1)*FMULT;
         FD(fa1,f) +=  integr3c(n1,n2,n0,nn1,rdetB,rhs0,rhs1)*FMULT;
         FD(fa2,f) +=  integr3c(n2,n0,n1,nn2,rdetB,rhs0,rhs1)*FMULT;
      }
   } 
}

#else

void integr_rhs_p1c_fbub(tGrid,f,rhs0,rhs1)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f;
{  eprintf("Error: integr_rhs_p1c_fbub not available.\n");  }

#endif

#if (E_DATA & SCALAR_ELEMENT_DATA) && (ELEMENT_TYPE == SIMPLEX)

void integr_rhs_se_1(tGrid,f,rhs) /* exact for linear rhs */
GRID *tGrid;
FLOAT (*rhs)();
INT f;
{
   FLOAT *x0, *x1, *x2;
   ELEMENT *pelem;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      ED(pelem,f) = ( rhs(x0) + rhs(x1) + rhs(x2) )*VOLUME(pelem)/180.;
   }
}

#else

void integr_rhs_se_1(tGrid,f,rhs)
GRID *tGrid; FLOAT (*rhs)(); INT f;
{  eprintf("Error: integr_rhs_se_1 not available.\n");  }

#endif

#if (N_DATA & VECTOR_NODE_DATA) && (E_DATA & VECTOR_ELEMENT_DATA) && (ELEMENT_TYPE == SIMPLEX)

void integr_rhs_vn_ve(tGrid,f,rhs0,rhs1)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2];
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      ND(n0,f,0) += integr1(n0->myvertex->x,n1->myvertex->x,n2->myvertex->x,rhs0,rdetB);
      ND(n1,f,0) += integr1(n1->myvertex->x,n2->myvertex->x,n0->myvertex->x,rhs0,rdetB);
      ND(n2,f,0) += integr1(n2->myvertex->x,n0->myvertex->x,n1->myvertex->x,rhs0,rdetB);
      ND(n0,f,1) += integr1(n0->myvertex->x,n1->myvertex->x,n2->myvertex->x,rhs1,rdetB);
      ND(n1,f,1) += integr1(n1->myvertex->x,n2->myvertex->x,n0->myvertex->x,rhs1,rdetB);
      ND(n2,f,1) += integr1(n2->myvertex->x,n0->myvertex->x,n1->myvertex->x,rhs1,rdetB);
      integr3B(n0,n1,n2,EDVP(pelem,f),rdetB*FMULT,rhs0,rhs1);
   }
}

#else

void integr_rhs_vn_ve(tGrid,f,rhs0,rhs1)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f;
{  eprintf("Error: integr_rhs_vn_ve not available.\n");  }

#endif

#if (N_DATA & SCALAR_NODE_DATA) && (E_DATA & SCALAR_ELEMENT_DATA) && (ELEMENT_TYPE == SIMPLEX)

void p1cb_integrate_rhs(tGrid,f,rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,
                           integral,finite_el,rmtype,qr)
GRID *tGrid;
INT f, rmtype, i0, i1, i2;
FLOAT (*rhs)(), a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)();
DOUBLE (*integral)();
FINITE_ELEMENT finite_el;
QUADRATURE_RULE qr;
{
   ELEMENT *pel;
   REF_MAPPING ref_map;
   DOUBLE_FUNC *nq, *eq, *nq0, *nq1, *eq0, *eq1;
   INT i;
  
   nq   = finite_el.nbasis;
   eq   = finite_el.ebasis;
   nq0  = finite_el.nbasis_0;
   nq1  = finite_el.nbasis_1;
   eq0  = finite_el.ebasis_0;
   eq1  = finite_el.ebasis_1;
   for (pel = FIRSTELEMENT(tGrid); pel; pel = pel->succ){
      reference_mapping_with_inverse(pel,rmtype,&ref_map);
      for (i=0; i < NVERT; i++){
         NDS(pel->n[i],f) += integral(tGrid,pel,nq[i],nq0[i],nq1[i],
               rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,finite_el,ref_map,qr);
      }
      ED(pel,f) += integral(tGrid,pel,eq[0],eq0[0],eq1[0],
               rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,finite_el,ref_map,qr);
   }
}

#else

void p1cb_integrate_rhs(tGrid,f,rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,integral,finite_el,rmtype,qr)
GRID *tGrid; INT f, rmtype, i0, i1, i2; FLOAT (*rhs)(), a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(); DOUBLE (*integral)(); FINITE_ELEMENT finite_el; QUADRATURE_RULE qr;
{  eprintf("Error: p1cb_integrate_rhs not available.\n");  }

#endif

#if N_DATA & SCALAR_NODE_DATA

void integrate_rhs_sn_0(tGrid,f,rhs)
GRID *tGrid;
FLOAT (*rhs)();
INT f;
{
   ELEMENT *pelem;
   NODE *n0, *n1, *n2;
   FLOAT *x0, *x1, *x2, xc[DIM], s;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      POINT3(x0,x1,x2,xc);
      s = (*rhs)(xc)*VOLUME(pelem)/3.;
      NDS(n0,f) += s;
      NDS(n1,f) += s;
      NDS(n2,f) += s;
   }
}

void integrate_rhs_sn_1(tGrid,f,rhs)
GRID *tGrid;
FLOAT (*rhs)();
INT f;
{
   ELEMENT *pelem;
   NODE *n0, *n1, *n2;
   FLOAT *x0, *x1, *x2, f0, f1, f2, ndetB, s;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      f0 = (*rhs)(x0);
      f1 = (*rhs)(x1);
      f2 = (*rhs)(x2);
      s = f0 + f1 + f2;
      ndetB = VOLUME(pelem)/12.;
      NDS(n0,f) += (f0 + s)*ndetB;
      NDS(n1,f) += (f1 + s)*ndetB;
      NDS(n2,f) += (f2 + s)*ndetB;
   }
}

void integrate_rhs_sn_2(tGrid,f,rhs)
GRID *tGrid;
FLOAT (*rhs)();
INT f;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, x01[DIM], x02[DIM], x12[DIM], 
          f0, f1, f2, f01, f02, f12, rdetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      rdetB = VOLUME(pelem);
      MIDPOINTS(x0,x1,x2,x01,x02,x12)
      POINT_VALUES_6(x0,x1,x2,x01,x02,x12,f0,f1,f2,f01,f02,f12,rhs)
      NDS(n0,f) += INTEGR1_2(f0,f1,f2,f01,f02,f12)*rdetB;
      NDS(n1,f) += INTEGR1_2(f1,f2,f0,f12,f01,f02)*rdetB;
      NDS(n2,f) += INTEGR1_2(f2,f0,f1,f02,f12,f01)*rdetB;
   }
}

void integrate_rhs_sn_3(tGrid,f,rhs)
GRID *tGrid;
FLOAT (*rhs)();
INT f;
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, rdetB, c;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      rdetB = VOLUME(pelem);
      points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012);
      POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                      f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs)
      c = f0 + f1 + f2 + 18.*f012;
      NDS(n0,f) += INTEGR4_2(f0,f001,f002,c)*rdetB;
      NDS(n1,f) += INTEGR4_2(f1,f110,f112,c)*rdetB;
      NDS(n2,f) += INTEGR4_2(f2,f220,f221,c)*rdetB;
   }
}

void integrate_p1c_rhs_quadr(tGrid,f,rhs,qr)
GRID *tGrid;
INT f;
FLOAT (*rhs)();
QUADRATURE_RULE qr;
{
   ELEMENT *pelem;
   NODE *n0, *n1, *n2;
   FLOAT a[DIM][DIM], c[DIM];

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      P1_reference_mapping0(n0,n1,n2,a,c);
      NDS(n0,f) += rhs_ref_triang(pelem,r_p1_0,rhs,a,c,qr.n,qr.points,qr.weights);
      NDS(n1,f) += rhs_ref_triang(pelem,r_p1_1,rhs,a,c,qr.n,qr.points,qr.weights);
      NDS(n2,f) += rhs_ref_triang(pelem,r_p1_2,rhs,a,c,qr.n,qr.points,qr.weights);
   }
}

void integrate_rhs_q1(tGrid,f,rhs,qr)
GRID *tGrid;
FLOAT (*rhs)();
INT f;
QUADRATURE_RULE qr;
{
   ELEMENT *pelem;
   NODE *n0, *n1, *n2, *n3;
   FLOAT a[DIM][DIM], alpha[DIM], c[DIM], jac[DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_4ELEMENT(n0,n1,n2,n3,pelem);
      Q1_reference_mapping(n0,n1,n2,n3,a,alpha,c,jac);
      NDS(n0,f) += integr_f_X_phi_q1(rhs,r_q1_0,a,alpha,c,jac,
                                     qr.n,qr.points,qr.weights);
      NDS(n1,f) += integr_f_X_phi_q1(rhs,r_q1_1,a,alpha,c,jac,
                                     qr.n,qr.points,qr.weights);
      NDS(n2,f) += integr_f_X_phi_q1(rhs,r_q1_2,a,alpha,c,jac,
                                     qr.n,qr.points,qr.weights);
      NDS(n3,f) += integr_f_X_phi_q1(rhs,r_q1_3,a,alpha,c,jac,
                                     qr.n,qr.points,qr.weights);
   }
}

/*  2*int(int(f*(b0*l0+b1*l1+b2*l2),y=0..1-x),x=0..1),  s=f0+f1+f2, z=2*f012  */
#define CONV_RHS_INT_L0_P1(b0,b1,b2,                                           \
                           f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,s,z)    \
  ((b0*(s + f0 + 9.*(f001 + f002 + z)) + b1*(s + f1 + 9.*(f110 + f112 + z)) +  \
    b2*(s + f2 + 9.*(f220 + f221 + z)))/120.)

void scalar_p1c_sd_to_rhs_cubic(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid;
INT f;
FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], t0[DIM2], t1[DIM2], t2[DIM2], 
         *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, 
         s, z, delta, ndetB, b[DIM2][DIM2];
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      delta = sd_delta(pelem,nu,bb0,bb1);
      if (delta > 0.){
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
         V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
         points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012);
         POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                         f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs)
         ndetB = delta*barycentric_coordinates(x0,x1,x2,b);
         t0[0] = DOT(b[0],bb_0);
         t0[1] = DOT(b[0],bb_1);
         t0[2] = DOT(b[0],bb_2);
         t1[0] = DOT(b[1],bb_0);
         t1[1] = DOT(b[1],bb_1);
         t1[2] = DOT(b[1],bb_2);
         t2[0] = DOT(b[2],bb_0);
         t2[1] = DOT(b[2],bb_1);
         t2[2] = DOT(b[2],bb_2);
         s = f0 + f1 + f2; 
         z = f012 + f012;
         NDS(n0,f) += CONV_RHS_INT_L0_P1(t0[0],t0[1],t0[2],f0,f1,f2,
                                  f001,f002,f110,f112,f220,f221,f012,s,z)*ndetB;
         NDS(n1,f) += CONV_RHS_INT_L0_P1(t1[0],t1[1],t1[2],f0,f1,f2,
                                  f001,f002,f110,f112,f220,f221,f012,s,z)*ndetB;
         NDS(n2,f) += CONV_RHS_INT_L0_P1(t2[0],t2[1],t2[2],f0,f1,f2,
                                  f001,f002,f110,f112,f220,f221,f012,s,z)*ndetB;
      }
   }
}

void scalar_p1c_sdp1c_to_rhs_cubic(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid;
INT f;
FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], t0[DIM2], t1[DIM2], t2[DIM2], 
         *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, 
         s, z, delta[DIM2], ndetB, b[DIM2][DIM2];
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      delta[0] = sd_p1c_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,n0);
      delta[1] = sd_p1c_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,n1);
      delta[2] = sd_p1c_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,n2);
      if ((delta[0] > 0.)||(delta[1] > 0.)||(delta[2] > 0.)){
         VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
         V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
         points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012);
         POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                         f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs)
         ndetB = barycentric_coordinates(x0,x1,x2,b);
         t0[0] = DOT(b[0],bb_0);
         t0[1] = DOT(b[0],bb_1);
         t0[2] = DOT(b[0],bb_2);
         t1[0] = DOT(b[1],bb_0);
         t1[1] = DOT(b[1],bb_1);
         t1[2] = DOT(b[1],bb_2);
         t2[0] = DOT(b[2],bb_0);
         t2[1] = DOT(b[2],bb_1);
         t2[2] = DOT(b[2],bb_2);
         s = delta[0]*f0 + delta[1]*f1 + delta[2]*f2; 
         z = (delta[0]+delta[1]+delta[2])/3.*(f012+f012);
         NDS(n0,f) += CONV_RHS_INT_L0_P1(t0[0],t0[1],t0[2],
                                  f0*delta[0],f1*delta[1],f2*delta[2],
                                  (2.*delta[0]+delta[1])/3.*f001,
                                  (2.*delta[0]+delta[2])/3.*f002,
                                  (2.*delta[1]+delta[0])/3.*f110,
                                  (2.*delta[1]+delta[2])/3.*f112,
                                  (2.*delta[2]+delta[0])/3.*f220,
                                  (2.*delta[2]+delta[1])/3.*f221,
                                  z/2.,s,z)*ndetB;
         NDS(n1,f) += CONV_RHS_INT_L0_P1(t1[0],t1[1],t1[2],
                                  f0*delta[0],f1*delta[1],f2*delta[2],
                                  (2.*delta[0]+delta[1])/3.*f001,
                                  (2.*delta[0]+delta[2])/3.*f002,
                                  (2.*delta[1]+delta[0])/3.*f110,
                                  (2.*delta[1]+delta[2])/3.*f112,
                                  (2.*delta[2]+delta[0])/3.*f220,
                                  (2.*delta[2]+delta[1])/3.*f221,
                                  z/2.,s,z)*ndetB;
          NDS(n2,f) += CONV_RHS_INT_L0_P1(t2[0],t2[1],t2[2],
                                  f0*delta[0],f1*delta[1],f2*delta[2],
                                  (2.*delta[0]+delta[1])/3.*f001,
                                  (2.*delta[0]+delta[2])/3.*f002,
                                  (2.*delta[1]+delta[0])/3.*f110,
                                  (2.*delta[1]+delta[2])/3.*f112,
                                  (2.*delta[2]+delta[0])/3.*f220,
                                  (2.*delta[2]+delta[1])/3.*f221,
                                  z/2.,s,z)*ndetB;
      }
   }
}

void scalar_p1c_sdp1nc_to_rhs_cubic(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid;
INT f;
FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   ELEMENT *pelem;
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], t0[DIM2], t1[DIM2], t2[DIM2], 
         *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, 
         s, z, delta[DIM2], ndetB, b[DIM2][DIM2];
  
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      delta[0] = sd_p1_nc_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,0);
      delta[1] = sd_p1_nc_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,1);
      delta[2] = sd_p1_nc_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,2);
      if ((delta[0] > 0.)||(delta[1] > 0.)||(delta[2] > 0.)){
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
         V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
         points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012);
         POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                         f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs)
         ndetB = barycentric_coordinates(x0,x1,x2,b);
         t0[0] = DOT(b[0],bb_0);
         t0[1] = DOT(b[0],bb_1);
         t0[2] = DOT(b[0],bb_2);
         t1[0] = DOT(b[1],bb_0);
         t1[1] = DOT(b[1],bb_1);
         t1[2] = DOT(b[1],bb_2);
         t2[0] = DOT(b[2],bb_0);
         t2[1] = DOT(b[2],bb_1);
         t2[2] = DOT(b[2],bb_2);
         s = delta[0]*f0 + delta[1]*f1 + delta[2]*f2; 
         z = (delta[0]+delta[1]+delta[2])/3.*(f012+f012);
         NDS(n0,f) += CONV_RHS_INT_L0_P1(t0[0],t0[1],t0[2],
                                  f0*delta[0],f1*delta[1],f2*delta[2],
                                  (2.*delta[0]+delta[1])/3.*f001,
                                  (2.*delta[0]+delta[2])/3.*f002,
                                  (2.*delta[1]+delta[0])/3.*f110,
                                  (2.*delta[1]+delta[2])/3.*f112,
                                  (2.*delta[2]+delta[0])/3.*f220,
                                  (2.*delta[2]+delta[1])/3.*f221,
                                  z/2.,s,z)*ndetB;
         NDS(n1,f) += CONV_RHS_INT_L0_P1(t1[0],t1[1],t1[2],
                                  f0*delta[0],f1*delta[1],f2*delta[2],
                                  (2.*delta[0]+delta[1])/3.*f001,
                                  (2.*delta[0]+delta[2])/3.*f002,
                                  (2.*delta[1]+delta[0])/3.*f110,
                                  (2.*delta[1]+delta[2])/3.*f112,
                                  (2.*delta[2]+delta[0])/3.*f220,
                                  (2.*delta[2]+delta[1])/3.*f221,
                                  z/2.,s,z)*ndetB;
          NDS(n2,f) += CONV_RHS_INT_L0_P1(t2[0],t2[1],t2[2],
                                  f0*delta[0],f1*delta[1],f2*delta[2],
                                  (2.*delta[0]+delta[1])/3.*f001,
                                  (2.*delta[0]+delta[2])/3.*f002,
                                  (2.*delta[1]+delta[0])/3.*f110,
                                  (2.*delta[1]+delta[2])/3.*f112,
                                  (2.*delta[2]+delta[0])/3.*f220,
                                  (2.*delta[2]+delta[1])/3.*f221,
                                  z/2.,s,z)*ndetB;
      }
   }
}

void add_p1c_sd_rhs_term_quadr(tGrid,f,bb0,bb1,react,rhs,eps,
                               u,space,sc_type,par1,par2,sd_tau,qr)
GRID *tGrid;
INT f, u, space, sc_type;
FLOAT (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), eps, par1, par2, (*sd_tau)();
QUADRATURE_RULE qr;
{
   ELEMENT *pelem;
   NODE *n0, *n1, *n2;
   FLOAT bar[DIM2][DIM2], b[DIM][DIM], a[DIM][DIM], c[DIM];

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                              n2->myvertex->x,bar);
      P1_reference_mapping0_with_inverse(n0,n1,n2,b,a,c);
      NDS(n0,f) += sd_rhs_ref_triang(pelem,r_p1_0_0,r_p1_0_1,
        bar,bb0,bb1,react,eps,rhs,u,space,sc_type,par1,par2,sd_tau,b,a,c,qr.n,qr.points,qr.weights);
      NDS(n1,f) += sd_rhs_ref_triang(pelem,r_p1_1_0,r_p1_1_1,
        bar,bb0,bb1,react,eps,rhs,u,space,sc_type,par1,par2,sd_tau,b,a,c,qr.n,qr.points,qr.weights);
      NDS(n2,f) += sd_rhs_ref_triang(pelem,r_p1_2_0,r_p1_2_1,
        bar,bb0,bb1,react,eps,rhs,u,space,sc_type,par1,par2,sd_tau,b,a,c,qr.n,qr.points,qr.weights);
   }
}

void scalar_q1c_sd_to_rhs(tGrid,f,nu,rhs,b0,b1,qr)
GRID *tGrid;
INT f;
FLOAT nu, (*rhs)(), (*b0)(), (*b1)();
QUADRATURE_RULE qr;
{
   ELEMENT *pelem;
   NODE *n0, *n1, *n2, *n3;
   FLOAT b[2][2][DIM2], a[DIM][DIM], c[DIM], alpha[DIM], jac[DIM2], d;
  
   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      d = sd_delta(pelem,nu,b0,b1);
      if (d > 0.){
         NODES_OF_4ELEMENT(n0,n1,n2,n3,pelem);
         Q1_reference_mapping_with_inverse(n0,n1,n2,n3,b,a,c,alpha,jac);
         NDS(n0,f)+= sd_rhs_ref(r_q1_0_0,r_q1_0_1,rhs,b0,b1,d,b,a,c,alpha,jac,
                                qr.n,qr.points,qr.weights);
         NDS(n1,f)+= sd_rhs_ref(r_q1_1_0,r_q1_1_1,rhs,b0,b1,d,b,a,c,alpha,jac,
                                qr.n,qr.points,qr.weights);
         NDS(n2,f)+= sd_rhs_ref(r_q1_2_0,r_q1_2_1,rhs,b0,b1,d,b,a,c,alpha,jac,
                                qr.n,qr.points,qr.weights);
         NDS(n3,f)+= sd_rhs_ref(r_q1_3_0,r_q1_3_1,rhs,b0,b1,d,b,a,c,alpha,jac,
                                qr.n,qr.points,qr.weights);
      }
   }
}

#else

void integrate_rhs_sn_0(tGrid,f,rhs)
GRID *tGrid; FLOAT (*rhs)(); INT f;
{  eprintf("Error: integrate_rhs_sn_0 not available.\n");  }

void integrate_rhs_sn_1(tGrid,f,rhs)
GRID *tGrid; FLOAT (*rhs)(); INT f;
{  eprintf("Error: integrate_rhs_sn_1 not available.\n");  }

void integrate_rhs_sn_2(tGrid,f,rhs)
GRID *tGrid; FLOAT (*rhs)(); INT f;
{  eprintf("Error: integrate_rhs_sn_2 not available.\n");  }

void integrate_rhs_sn_3(tGrid,f,rhs)
GRID *tGrid; FLOAT (*rhs)(); INT f;
{  eprintf("Error: integrate_rhs_sn_3 not available.\n");  }

void integrate_p1c_rhs_quadr(tGrid,f,rhs,qr)
GRID *tGrid; INT f; FLOAT (*rhs)(); QUADRATURE_RULE qr;
{  eprintf("Error: integrate_p1c_rhs_quadr not available.\n");  }

void integrate_rhs_q1(tGrid,f,rhs,qr)
GRID *tGrid; FLOAT (*rhs)(); INT f; QUADRATURE_RULE qr;
{  eprintf("Error: integrate_rhs_q1 not available.\n");  }

void scalar_p1c_sd_to_rhs_cubic(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid; INT f; FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{  eprintf("Error: scalar_p1c_sd_to_rhs_cubic not available.\n");  }

void add_p1c_sd_rhs_term_quadr(tGrid,f,bb0,bb1,react,rhs,eps,u,space,sc_type,par1,par2,sd_tau,qr)
GRID *tGrid; INT f, u, space, sc_type; FLOAT (*bb0)(), (*bb1)(), (*react)(), (*rhs)(), eps, par1, par2, (*sd_tau)(); QUADRATURE_RULE qr;
{  eprintf("Error: add_p1c_sd_rhs_term_quadr not available.\n");  }

void scalar_q1c_sd_to_rhs(tGrid,f,nu,rhs,b0,b1,qr)
GRID *tGrid; INT f; FLOAT nu, (*rhs)(), (*b0)(), (*b1)(); QUADRATURE_RULE qr;
{  eprintf("Error: scalar_q1c_sd_to_rhs not available.\n");  }

#endif

#if (N_DATA & MVECTOR_NODE_DATA) && (F_DATA & MVECTOR_FACE_DATA) && (E_DATA & MVECTOR_ELEMENT_DATA) && (DIM == 2)

void general_local_integrate_rhs(tGrid,pel,f,rhs,a0,a1,a2,a3,a4,i0,i1,i2,
                                 f0,f1,f2,f3,f4,integral,finite_el,rmtype,qr,
                                 kk,nn,mm,nq,nq0,nq1,fq,fq0,fq1,eq,eq0,eq1)
GRID *tGrid;
ELEMENT *pel;
INT f, rmtype, i0, i1, i2, kk, nn, mm;
FLOAT (*rhs)(), a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)();
DOUBLE (*integral)();
FINITE_ELEMENT finite_el;
QUADRATURE_RULE qr;
DOUBLE_FUNC *nq, *nq0, *nq1, *fq, *fq0, *fq1, *eq, *eq0, *eq1;
{
   REF_MAPPING ref_map;
   INT i, k, m, dir[4];
  
   reference_mapping_with_inverse(pel,rmtype,&ref_map);
   if (kk)
      for (i=0; i < NVERT; i++){
         m = i*kk;
         for (k = m; k < m+kk; k++)
               NDMV(pel->n[i],f,k-m) += integral(tGrid,pel,nq[k],nq0[k],nq1[k],
               rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,finite_el,ref_map,qr);
      }
   if (nn){
      set_directions_of_edges(pel,dir);
      for (i=0; i < NVERT; i++){
         m = i*nn;
         for (k = m; k < m+nn; k++) 
            FDMV(pel->f[i],f,f_i(k-m,nn,dir[i])) += 
                   integral(tGrid,pel,fq[k],fq0[k],fq1[k],rhs,
                   a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,finite_el,ref_map,qr);
      }
   }
   for (i = 0; i < mm; i++)
      EDMV(pel,f,i) += integral(tGrid,pel,eq[i],eq0[i],eq1[i],
               rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,finite_el,ref_map,qr);
}

void general_consistency_err_for_conv_lp(tGrid,f,u0,u1,eps,n,bb0,bb1,
                           f2,f3,f4,f5,f6,f7,f8,f9,integral,finite_el,rmtype,qr)
GRID *tGrid;
INT f, rmtype, n;
FLOAT (*u0)(), (*u1)(), eps, (*bb0)(), (*bb1)(), (*f2)(), (*f3)(),
(*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)();
DOUBLE (*integral)();
FINITE_ELEMENT finite_el;
QUADRATURE_RULE qr;
{
   ELEMENT *pel;
   REF_MAPPING ref_map;
   DOUBLE_FUNC *nq, *fq, *eq, *nq0, *nq1, *fq0, *fq1, *eq0, *eq1;
   INT i, k, m, kk, nn, mm, dir[4];
  
   nq   = finite_el.nbasis;
   fq   = finite_el.fbasis;
   eq   = finite_el.ebasis;
   nq0  = finite_el.nbasis_0;
   nq1  = finite_el.nbasis_1;
   fq0  = finite_el.fbasis_0;
   fq1  = finite_el.fbasis_1;
   eq0  = finite_el.ebasis_0;
   eq1  = finite_el.ebasis_1;
   kk = finite_el.k;
   nn = finite_el.n;
   mm = finite_el.m;
   for (pel = FIRSTELEMENT(tGrid); pel; pel = pel->succ){
      reference_mapping_with_inverse(pel,rmtype,&ref_map);
      if (kk)
         for (i=0; i < NVERT; i++){
            m = i*kk;
            for (k = m; k < m+kk; k++)
               NDMV(pel->n[i],f,k-m) += integral(tGrid,pel,nq[k],nq0[k],nq1[k],
               u0,u1,eps,n,bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,finite_el,ref_map,qr);
         }
      if (nn){
         set_directions_of_edges(pel,dir);
         for (i=0; i < NVERT; i++){
            m = i*nn;
            for (k = m; k < m+nn; k++) 
               FDMV(pel->f[i],f,f_i(k-m,nn,dir[i])) += 
                          integral(tGrid,pel,fq[k],fq0[k],fq1[k],u0,u1,eps,n,
                          bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,finite_el,ref_map,qr);
         }
      }
      for (i = 0; i < mm; i++)
         EDMV(pel,f,i) += integral(tGrid,pel,eq[i],eq0[i],eq1[i],
               u0,u1,eps,n,bb0,bb1,f2,f3,f4,f5,f6,f7,f8,f9,finite_el,ref_map,qr);
   }
}

#else

void general_local_integrate_rhs(tGrid,pel,f,rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,integral,finite_el,rmtype,qr,kk,nn,mm,nq,nq0,nq1,fq,fq0,fq1,eq,eq0,eq1)
GRID *tGrid; ELEMENT *pel; INT f, rmtype, i0, i1, i2, kk, nn, mm; FLOAT (*rhs)(), a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(); DOUBLE (*integral)(); FINITE_ELEMENT finite_el; QUADRATURE_RULE qr; DOUBLE_FUNC *nq, *nq0, *nq1, *fq, *fq0, *fq1, *eq, *eq0, *eq1;
{  eprintf("Error: general_local_integrate_rhs not available.\n");  }

void general_consistency_err_for_conv_lp(tGrid,f,u0,u1,eps,n,bb0,bb1,
                           f2,f3,f4,f5,f6,f7,f8,f9,integral,finite_el,rmtype,qr)
GRID *tGrid; INT f, rmtype, n; FLOAT (*u0)(), (*u1)(), eps, (*bb0)(), (*bb1)(), (*f2)(), (*f3)(), (*f4)(), (*f5)(), (*f6)(), (*f7)(), (*f8)(), (*f9)(); DOUBLE (*integral)(); FINITE_ELEMENT finite_el; QUADRATURE_RULE qr;
{  eprintf("Error: general_consistency_err_for_conv_lp not available.\n");  }

#endif

void general_integrate_rhs(tGrid,f,rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,
                           integral,finite_el,rmtype,qr)
GRID *tGrid;
INT f, rmtype, i0, i1, i2;
FLOAT (*rhs)(), a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)();
DOUBLE (*integral)();
FINITE_ELEMENT finite_el;
QUADRATURE_RULE qr;
{
   ELEMENT *pel;
   DOUBLE_FUNC *nq, *nq0, *nq1, *fq, *fq0, *fq1, *eq, *eq0, *eq1;
   INT kk, nn, mm;
  
   nq   = finite_el.nbasis;
   nq0  = finite_el.nbasis_0;
   nq1  = finite_el.nbasis_1;
   fq   = finite_el.fbasis;
   fq0  = finite_el.fbasis_0;
   fq1  = finite_el.fbasis_1;
   eq   = finite_el.ebasis;
   eq0  = finite_el.ebasis_0;
   eq1  = finite_el.ebasis_1;
   kk = finite_el.k;
   nn = finite_el.n;
   mm = finite_el.m;
   for (pel = FIRSTELEMENT(tGrid); pel; pel = pel->succ)
      general_local_integrate_rhs(tGrid,pel,f,rhs,a0,a1,a2,a3,a4,i0,i1,i2,
                                   f0,f1,f2,f3,f4,integral,finite_el,rmtype,qr,
                                   kk,nn,mm,nq,nq0,nq1,fq,fq0,fq1,eq,eq0,eq1);
}

void general_integrate_rhs_b(tGrid,f,rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,
                             integral,i_finite_el,b_finite_el,rmtype,qr)
GRID *tGrid;
INT f, rmtype, i0, i1, i2;
FLOAT (*rhs)(), a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)();
DOUBLE (*integral)();
FINITE_ELEMENT i_finite_el, b_finite_el;
QUADRATURE_RULE qr;
{
   ELEMENT *pel;
   DOUBLE_FUNC *i_nq, *i_nq0, *i_nq1, *i_fq, *i_fq0, *i_fq1, 
               *i_eq, *i_eq0, *i_eq1, *b_nq, *b_nq0, *b_nq1, 
               *b_fq, *b_fq0, *b_fq1, *b_eq, *b_eq0, *b_eq1;
   INT i_kk, i_nn, i_mm, b_kk, b_nn, b_mm;
  
   i_nq   = i_finite_el.nbasis;
   i_nq0  = i_finite_el.nbasis_0;
   i_nq1  = i_finite_el.nbasis_1;
   i_fq   = i_finite_el.fbasis;
   i_fq0  = i_finite_el.fbasis_0;
   i_fq1  = i_finite_el.fbasis_1;
   i_eq   = i_finite_el.ebasis;
   i_eq0  = i_finite_el.ebasis_0;
   i_eq1  = i_finite_el.ebasis_1;
   i_kk = i_finite_el.k;
   i_nn = i_finite_el.n;
   i_mm = i_finite_el.m;
   b_nq   = b_finite_el.nbasis;
   b_nq0  = b_finite_el.nbasis_0;
   b_nq1  = b_finite_el.nbasis_1;
   b_fq   = b_finite_el.fbasis;
   b_fq0  = b_finite_el.fbasis_0;
   b_fq1  = b_finite_el.fbasis_1;
   b_eq   = b_finite_el.ebasis;
   b_eq0  = b_finite_el.ebasis_0;
   b_eq1  = b_finite_el.ebasis_1;
   b_kk = b_finite_el.k;
   b_nn = b_finite_el.n;
   b_mm = b_finite_el.m;
   for (pel = FIRSTELEMENT(tGrid); pel; pel = pel->succ)
      if (IS_B_EL(pel))
         general_local_integrate_rhs(tGrid,pel,f,rhs,a0,a1,a2,a3,a4,i0,i1,i2,
                            f0,f1,f2,f3,f4,integral,b_finite_el,rmtype,qr,
                            b_kk,b_nn,b_mm,
                            b_nq,b_nq0,b_nq1,b_fq,b_fq0,b_fq1,b_eq,b_eq0,b_eq1);
      else
         general_local_integrate_rhs(tGrid,pel,f,rhs,a0,a1,a2,a3,a4,i0,i1,i2,
                            f0,f1,f2,f3,f4,integral,i_finite_el,rmtype,qr,
                            i_kk,i_nn,i_mm,
                            i_nq,i_nq0,i_nq1,i_fq,i_fq0,i_fq1,i_eq,i_eq0,i_eq1);
}

#if N_DATA & SCALAR_NODE_DATA

void general_p1_q1_integrate_rhs(tGrid,f,rhs,a0,a1,a2,a3,a4,i0,i1,i2,
                                 f0,f1,f2,f3,f4,integral,finite_el,rmtype,qr)
GRID *tGrid;
INT f, rmtype, i0, i1, i2;
FLOAT (*rhs)(), a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)();
DOUBLE (*integral)();
FINITE_ELEMENT finite_el;
QUADRATURE_RULE qr;
{
   ELEMENT *pel;
   REF_MAPPING ref_map;
   DOUBLE_FUNC *nq;
   INT i;
  
   nq   = finite_el.nbasis;
   for (pel = FIRSTELEMENT(tGrid); pel; pel = pel->succ){
      reference_mapping_with_inverse(pel,rmtype,&ref_map);
      for (i=0; i < NVERT; i++)
         NDS(pel->n[i],f) += integral(tGrid,pel,nq[i],NULL,NULL,
               rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,finite_el,ref_map,qr);
   }
}

/* the following code has been written by Milos Urbanek 
   and it is without any warranty */
FLOAT sdterm_p1c_rhs(FLOAT a1[DIM2], FLOAT a2[DIM2], FLOAT a3[DIM2],
    FLOAT bn1[DIM2-1], FLOAT bn2[DIM2-1], FLOAT bn3[DIM2-1], FLOAT f0,
    FLOAT f1, FLOAT f2)
{

	return (
	    1./12*(
		a1[0]*(f0*(2.*bn1[0] + bn2[0] + bn3[0]) +
	    	f1*(bn1[0] + 2.*bn2[0] + bn3[0]) +
		f2*(bn1[0] + bn2[0] + 2.*bn3[0])) +
		a1[1]*(f0*(2.*bn1[1] + bn2[1] + bn3[1]) +
		f1*(bn1[1] + 2.*bn2[1] + bn3[1]) +
		f2*(bn1[1] + bn2[1] + 2.*bn3[1]))
		)
		);
}

#define VALUES_IN_VERTICES(n0,n1,n2,f,g0,g1,g2)    g0 = f(n0->myvertex->x);    \
                                                   g1 = f(n1->myvertex->x);    \
                                                   g2 = f(n2->myvertex->x)

/* the following code has been written by Milos Urbanek 
   and it is without any warranty */
void p1c_sd_rhs(n0, n1, n2, f, b0, b1, b2,rdetB,delta,rhs,bb0,bb1)
NODE *n0, *n1, *n2; 
INT f; 
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], rdetB, delta, (*rhs)(), (*bb0)(), (*bb1)();
{
	FLOAT		bn0[DIM], bn1[DIM], bn2[DIM];
	FLOAT		*x0, *x1, *x2;
	FLOAT		f0, f1, f2;

   x0 = n0->myvertex->x;
   x1 = n1->myvertex->x;
   x2 = n2->myvertex->x;
	        bn0[0] = bb0(n0->myvertex->x);
	        bn0[1] = bb1(n0->myvertex->x);
	        bn1[0] = bb0(n1->myvertex->x);
	        bn1[1] = bb1(n1->myvertex->x);
	        bn2[0] = bb0(n2->myvertex->x);
	        bn2[1] = bb1(n2->myvertex->x);
   VALUES_IN_VERTICES(n0, n1, n2, rhs, f0, f1, f2);
   NDS(n0, f) += delta * rdetB * sdterm_p1c_rhs(b0,b1,b2, bn0,bn1,bn2,f0,f1,f2);
}

#else

void general_p1_q1_integrate_rhs(tGrid,f,rhs,a0,a1,a2,a3,a4,i0,i1,i2,f0,f1,f2,f3,f4,integral,finite_el,rmtype,qr)
GRID *tGrid; INT f, rmtype, i0, i1, i2; FLOAT (*rhs)(), a0, a1, a2, a3, a4, (*f0)(), (*f1)(), (*f2)(), (*f3)(), (*f4)(); DOUBLE (*integral)(); FINITE_ELEMENT finite_el; QUADRATURE_RULE qr;
{  eprintf("Error: general_p1_q1_integrate_rhs not available.\n");  }

void p1c_sd_rhs(n0, n1, n2, f, b0, b1, b2,rdetB,delta,rhs,bb0,bb1)
NODE *n0, *n1, *n2; INT f; FLOAT b0[DIM2], b1[DIM2], b2[DIM2], rdetB, delta, (*rhs)(), (*bb0)(), (*bb1)();
{ eprintf("p1c_sd_rhs NOT AVAILABALE\n"); }

#endif

/* from URBANEK - without any warranty */
void add_sd_rhs_p1c_urbanek(tGrid, f, nu, rhs, bb0, bb1)
GRID           *tGrid;
INT             f;
FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{
	NODE		*n0, *n1, *n2;
	ELEMENT		*pelem;
	FLOAT           rdetB, b[DIM2][DIM2], delta;

   eprintf("add_sd_rhs_p1c_urbanek is without any warranty.\n");
   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ) {
	NODES_OF_ELEMENT(n0, n1, n2, pelem);
	rdetB = barycentric_coordinates(n0->myvertex->x,
			n1->myvertex->x, n2->myvertex->x, b);

        delta = sd_delta(pelem, nu, bb0, bb1);
	p1c_sd_rhs(n0, n1, n2, f, b[0], b[1], b[2], rdetB, delta, rhs, bb0,bb1);
	p1c_sd_rhs(n1, n2, n0, f, b[1], b[2], b[0], rdetB, delta, rhs, bb0,bb1);
	p1c_sd_rhs(n2, n0, n1, f, b[2], b[0], b[1], rdetB, delta, rhs, bb0,bb1);
   }
}

#if (N_DATA & SCALAR_NODE_DATA) && (F_DATA & SCALAR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX)

void integrate_rhs_sn_sf_2(tGrid,f,rhs)
GRID *tGrid;
FLOAT (*rhs)();
INT f;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, x01[DIM], x02[DIM], x12[DIM], 
          f0, f1, f2, f01, f02, f12, rdetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      rdetB = VOLUME(pelem);
      MIDPOINTS(x0,x1,x2,x01,x02,x12)
      POINT_VALUES_6(x0,x1,x2,x01,x02,x12,f0,f1,f2,f01,f02,f12,rhs)
      NDS(n0,f) += INTEGR1_2(f0,f1,f2,f01,f02,f12)*rdetB;
      NDS(n1,f) += INTEGR1_2(f1,f2,f0,f12,f01,f02)*rdetB;
      NDS(n2,f) += INTEGR1_2(f2,f0,f1,f02,f12,f01)*rdetB;
      FD(fa0,f) += INTEGR3_2(f0,f01,f02,f12)*rdetB;
      FD(fa1,f) += INTEGR3_2(f1,f12,f01,f02)*rdetB;
      FD(fa2,f) += INTEGR3_2(f2,f02,f12,f01)*rdetB;
   }
}

void integrate_rhs_sn_sf_3(tGrid,f,rhs)
GRID *tGrid;
FLOAT (*rhs)();
INT f;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, rdetB, s, c;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      rdetB = VOLUME(pelem);
      points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012);
      POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                      f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs)
      s = f0 + f1 + f2;
      c = s + 18.*f012;
      NDS(n0,f) += INTEGR4_2(f0,f001,f002,c)*rdetB;
      NDS(n1,f) += INTEGR4_2(f1,f110,f112,c)*rdetB;
      NDS(n2,f) += INTEGR4_2(f2,f220,f221,c)*rdetB;
      c = s + 6.*(f001 + f002 + f110 + f112 + f220 + f221) + 36.*f012;
      FD(fa0,f) += INTEGR5_2(f0,f112,f221,f001,f002,c)*rdetB;
      FD(fa1,f) += INTEGR5_2(f1,f002,f220,f110,f112,c)*rdetB;
      FD(fa2,f) += INTEGR5_2(f2,f001,f110,f220,f221,c)*rdetB;
   }
}

void scalar_p2c_sd_to_rhs_cubic(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid;
INT f;
FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT s[SIDES], c[SIDES], vn[SIDES][SIDES], vf[SIDES][SIDES], 
         *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], ndetB, b[DIM2][DIM2], delta,
         v0[DIM], v1[DIM], v2[DIM], v01[DIM], v02[DIM], v12[DIM], 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012;

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      delta = sd_delta(pelem,nu,bb0,bb1);
      if (delta > 0.){
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
         VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
         p2cv_dofs(pelem,v0,v1,v2,v01,v02,v12,bb0,bb1);
         points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012);
         POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                         f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs)
         ndetB = delta*barycentric_coordinates(x0,x1,x2,b);
         compute_vn_vf_s_c(v0,v1,v2,v01,v02,v12,b,vn,vf,s,c);
         add_sd_rhs_n_f(vn,vf,f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,ndetB,
                        &NDS(n0,f),&NDS(n1,f),&NDS(n2,f),
                        &FD(fa0,f),&FD(fa1,f),&FD(fa2,f));
      }
   }
}

void scalar_p2c_sdp1c_to_rhs_cubic(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid;
INT f;
FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT s[SIDES], c[SIDES], vn[SIDES][SIDES], vf[SIDES][SIDES], 
         *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], ndetB, b[DIM2][DIM2],
         v0[DIM], v1[DIM], v2[DIM], v01[DIM], v02[DIM], v12[DIM], 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, delta[DIM2];

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      delta[0] = sd_p1c_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,n0);
      delta[1] = sd_p1c_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,n1);
      delta[2] = sd_p1c_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,n2);
      if ((delta[0] > 0.) || (delta[1] > 0.) || (delta[2] > 0.)){
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
         VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
         p2cv_dofs(pelem,v0,v1,v2,v01,v02,v12,bb0,bb1);
         points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012);
         POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                         f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs)
         ndetB = barycentric_coordinates(x0,x1,x2,b);
         compute_vn_vf_s_c(v0,v1,v2,v01,v02,v12,b,vn,vf,s,c);
         add_sd_p1_nc_rhs_n_f(vn,vf,f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,
                              ndetB,delta[0],delta[1],delta[2],
                              &NDS(n0,f),&NDS(n1,f),&NDS(n2,f),
                              &FD(fa0,f),&FD(fa1,f),&FD(fa2,f));
      }
   }
}

void scalar_p2c_sdp1nc_to_rhs_cubic(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid;
INT f;
FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT s[SIDES], c[SIDES], vn[SIDES][SIDES], vf[SIDES][SIDES], 
         *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], ndetB, b[DIM2][DIM2],
         v0[DIM], v1[DIM], v2[DIM], v01[DIM], v02[DIM], v12[DIM], 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, delta[DIM2];

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      delta[0] = sd_p1_nc_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,0);
      delta[1] = sd_p1_nc_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,1);
      delta[2] = sd_p1_nc_delta(pelem,nu,bb0,bb1,TAU_VARIABLE,2);
      if ((delta[0] > 0.) || (delta[1] > 0.) || (delta[2] > 0.)){
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
         VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
         p2cv_dofs(pelem,v0,v1,v2,v01,v02,v12,bb0,bb1);
         points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012);
         POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                         f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs)
         ndetB = barycentric_coordinates(x0,x1,x2,b);
         compute_vn_vf_s_c(v0,v1,v2,v01,v02,v12,b,vn,vf,s,c);
         add_sd_p1_nc_rhs_n_f(vn,vf,f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,
                              ndetB,delta[0],delta[1],delta[2],
                              &NDS(n0,f),&NDS(n1,f),&NDS(n2,f),
                              &FD(fa0,f),&FD(fa1,f),&FD(fa2,f));
      }
   }
}

#else

void integrate_rhs_sn_sf_2(tGrid,f,rhs)
GRID *tGrid; FLOAT (*rhs)(); INT f;
{  eprintf("Error: integrate_rhs_sn_sf_2 not available.\n");  }

void integrate_rhs_sn_sf_3(tGrid,f,rhs)
GRID *tGrid; FLOAT (*rhs)(); INT f;
{  eprintf("Error: integrate_rhs_sn_sf_3 not available.\n");  }

void scalar_p2c_sd_to_rhs_cubic(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid; INT f; FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{  eprintf("Error: scalar_p2c_sd_to_rhs_cubic not available.\n");  }

#endif

#if (N_DATA & SCALAR_NODE_DATA) && (F_DATA & SCALAR_FACE_DATA) && (F_DATA & CURVED_FACE_MIDDLE) && (ELEMENT_TYPE == SIMPLEX)

void integrate_rhs_sn_sf_3_iso(tGrid,f,rhs)
GRID *tGrid;
FLOAT (*rhs)();
INT f;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, 
         a[DIM][DIM], c[DIM], alpha[DIM], j[DIM2], s;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      C_DATA_OF_ELEMENT(n0,n1,n2,fa0,fa1,fa2,x0,x1,x2,pelem)
      P2_reference_mapping0(n0,n1,n2,fa0,a,c,alpha,j);
      mapped_points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,a,c,alpha);
      POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                      f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs)
      s = SGN(j[2]);
      NDS(n0,f) += (f0*j[0] + 14*f0*j[2] + f0*j[1] + j[1]*f2 + 12*j[1]*f220 
         + 12*j[1]*f002
         + 63*j[2]*f001 + 7*j[2]*f1 + 12*j[0]*f001 + j[0]*f1 + 12*j[0]*f110
         + 6*j[1]*f001 - 3*j[1]*f110 - 3*j[1]*f112 + 36*j[1]*f012 + 6*j[1]*f221
         + 2*j[1]*f1 + 63*j[2]*f002 + 7*j[2]*f2 + 126*j[2]*f012 + 6*j[0]*f112
         + 36*j[0]*f012 - 3*j[0]*f221 + 6*j[0]*f002 + 2*j[0]*f2 
         - 3*j[0]*f220)/1680.*s;
      NDS(n1,f) += (4*f0*j[0] + 7*f0*j[2] + 2*f0*j[1] + j[1]*f2 + 6*j[1]*f220 
         - 3*j[1]*f002
         + 14*j[2]*f1 + 63*j[2]*f110 - 9*j[0]*f001 + 12*j[0]*f1 + 45*j[0]*f110
         - 3*j[1]*f001 + 6*j[1]*f110 + 12*j[1]*f112 + 36*j[1]*f012
         + 12*j[1]*f221 + j[1]*f1 + 7*j[2]*f2 + 63*j[2]*f112 + 126*j[2]*f012
         + 45*j[0]*f112 + 54*j[0]*f012 - 9*j[0]*f221 - 3*j[0]*f002 + 4*j[0]*f2
         - 3*j[0]*f220)/1680.*s;
      NDS(n2,f) += (2*f0*j[0] + 7*f0*j[2] + 4*f0*j[1] + 12*j[1]*f2 
         + 45*j[1]*f220
         - 9*j[1]*f002 + 7*j[2]*f1 - 3*j[0]*f001 + j[0]*f1 + 6*j[0]*f110
         - 3*j[1]*f001 - 3*j[1]*f110 - 9*j[1]*f112 + 54*j[1]*f012
         + 45*j[1]*f221 + 4*j[1]*f1 + 14*j[2]*f2 + 63*j[2]*f220 + 126*j[2]*f012
         + 63*j[2]*f221 + 12*j[0]*f112 + 36*j[0]*f012 + 12*j[0]*f221
         - 3*j[0]*f002 + j[0]*f2 + 6*j[0]*f220)/1680.*s;
      FD(fa0,f) += (11*f0*j[0] + 24*f0*j[2] + 11*f0*j[1] + 6*j[1]*f2 
         + 54*j[1]*f220
         - 27*j[1]*f002 - 36*j[2]*f001 + 12*j[2]*f1 + 72*j[2]*f110
         - 27*j[0]*f001 + 6*j[0]*f1 + 54*j[0]*f110 - 18*j[1]*f001 + 9*j[1]*f110
         + 27*j[1]*f112 + 162*j[1]*f012 + 108*j[1]*f221 + 4*j[1]*f1
         - 36*j[2]*f002 + 12*j[2]*f2 + 72*j[2]*f220 + 144*j[2]*f112
         + 432*j[2]*f012 + 144*j[2]*f221 + 108*j[0]*f112 + 162*j[0]*f012
         + 27*j[0]*f221 - 18*j[0]*f002 + 4*j[0]*f2 + 9*j[0]*f220)/20160.*s;
      FD(fa1,f) += (2*f0*j[0] + 12*f0*j[2] + 4*f0*j[1] + 6*j[1]*f2 
         + 108*j[1]*f220
         + 27*j[1]*f002 + 72*j[2]*f001 + 24*j[2]*f1 - 36*j[2]*f110
         + 9*j[0]*f001 + 2*j[0]*f1 + 9*j[0]*f110 + 9*j[1]*f001 - 18*j[1]*f110
         - 27*j[1]*f112 + 162*j[1]*f012 + 54*j[1]*f221 + 11*j[1]*f1
         + 144*j[2]*f002 + 12*j[2]*f2 + 144*j[2]*f220 - 36*j[2]*f112
         + 432*j[2]*f012 + 72*j[2]*f221 + 9*j[0]*f112 + 108*j[0]*f012
         + 9*j[0]*f221 + 9*j[0]*f002 + 2*j[0]*f2 + 9*j[0]*f220)/20160.*s;
      FD(fa2,f) += (4*f0*j[0] + 12*f0*j[2] + 2*f0*j[1] + 2*j[1]*f2 + 9*j[1]*f220
         + 9*j[1]*f002 + 144*j[2]*f001 + 12*j[2]*f1 + 144*j[2]*f110
         + 27*j[0]*f001 + 6*j[0]*f1 + 108*j[0]*f110 + 9*j[1]*f001 + 9*j[1]*f110
         + 9*j[1]*f112 + 108*j[1]*f012 + 9*j[1]*f221 + 2*j[1]*f1 + 72*j[2]*f002
         + 24*j[2]*f2 - 36*j[2]*f220 + 72*j[2]*f112 + 432*j[2]*f012
         - 36*j[2]*f221 + 54*j[0]*f112 + 162*j[0]*f012 - 27*j[0]*f221
         + 9*j[0]*f002 + 11*j[0]*f2 - 18*j[0]*f220)/20160.*s;
   }
}

#else

void integrate_rhs_sn_sf_3_iso(tGrid,f,rhs)
GRID *tGrid; FLOAT (*rhs)(); INT f;
{  eprintf("Error: integrate_rhs_sn_sf_3_iso not available.\n");  }

#endif

#if (F_DATA & SCALAR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX)

void integrate_rhs_sf_0(tGrid,f,rhs)   /*  int_K f*nc_zeta  */
GRID *tGrid;
FLOAT (*rhs)();
INT f;
{
   ELEMENT *pelem;
   FACE *fa0, *fa1, *fa2;
   FLOAT *x0, *x1, *x2, xc[DIM], s;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      POINT3(x0,x1,x2,xc);
      s = (*rhs)(xc)*VOLUME(pelem)/3.;
      FD(fa0,f) += s;
      FD(fa1,f) += s;
      FD(fa2,f) += s;
   }
}

void integrate_rhs_sf_1(tGrid,f,rhs)   /*  int_K f*nc_zeta  */
GRID *tGrid;
FLOAT (*rhs)();
INT f;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = VOLUME(pelem); 
      FD(fa0,f) += integr11(n1,n2,n0,rhs,rdetB);
      FD(fa1,f) += integr11(n2,n0,n1,rhs,rdetB);
      FD(fa2,f) += integr11(n0,n1,n2,rhs,rdetB);
   } 
}

void integrate_rhs_sf_3(tGrid,f,rhs)
GRID *tGrid;
FLOAT (*rhs)();
INT f;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
      FD(fa0,f) += integr_rhs_zeta(nc_ze12,b[0],b[1],b[2],n0,n1,n2,rhs,rhs,rhs,rdetB);
      FD(fa1,f) += integr_rhs_zeta(nc_ze12,b[1],b[2],b[0],n1,n2,n0,rhs,rhs,rhs,rdetB);
      FD(fa2,f) += integr_rhs_zeta(nc_ze12,b[2],b[0],b[1],n2,n0,n1,rhs,rhs,rhs,rdetB);
   } 
}

void nc_sd_ijb_rhs1(n0,n1,n2,fa0,f,rhs,bb0,bb1,b0,b1,b2,detB)
NODE *n0, *n1, *n2;      /*  streamline-diffusion term; exact for pw. lin. b, */
FACE *fa0;               /*  pw. const. react and pw. lin./cubic rhs          */
INT f;                   /*  detB = volume*delta                              */
FLOAT (*rhs)(), (*bb0)(), (*bb1)(), b0[DIM2], b1[DIM2], b2[DIM2], detB;
{
   FLOAT bb_0[DIM], bb_1[DIM], bb_2[DIM], t[DIM2], g[DIM2];
  
      V_NODE_VALUES(n0,n1,n2,bb0,bb1,bb_0,bb_1,bb_2)
      t[0] = DOT(b0,bb_0);
      t[1] = DOT(b0,bb_1);
      t[2] = DOT(b0,bb_2);
      S_NODE_VALUES(n0,n1,n2,rhs,g[0],g[1],g[2])
      FD(fa0,f) -= 2.*detB*MULT_LIN(g,t);
}

void scalar_p1nc_sd_to_rhs_linear(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid;
INT f;
FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2], delta;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      delta = sd_delta(pelem,nu,bb0,bb1);
      if (delta > 0.){
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
         rdetB = delta*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                             n2->myvertex->x,b);
         nc_sd_ijb_rhs1(n0,n1,n2,fa0,f,rhs,bb0,bb1,b[0],b[1],b[2],rdetB);
         nc_sd_ijb_rhs1(n1,n2,n0,fa1,f,rhs,bb0,bb1,b[1],b[2],b[0],rdetB);
         nc_sd_ijb_rhs1(n2,n0,n1,fa2,f,rhs,bb0,bb1,b[2],b[0],b[1],rdetB);
      }
   } 
}

void scalar_p1nc_sd_to_rhs_cubic(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid;
INT f;
FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT detB, b[DIM2][DIM2], delta;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      delta = sd_delta(pelem,nu,bb0,bb1);
      if (delta > 0.){
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
         detB = delta*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                             n2->myvertex->x,b);
         if (IS_FF(fa0)) FD(fa0,f) += 
            integr_rhs_zeta(nc_bgze12,b[0],b[1],b[2],n0,n1,n2,rhs,bb0,bb1,detB);
         if (IS_FF(fa1)) FD(fa1,f) += 
            integr_rhs_zeta(nc_bgze12,b[1],b[2],b[0],n1,n2,n0,rhs,bb0,bb1,detB);
         if (IS_FF(fa2)) FD(fa2,f) += 
            integr_rhs_zeta(nc_bgze12,b[2],b[0],b[1],n2,n0,n1,rhs,bb0,bb1,detB);
      }
   } 
}

#else

void integrate_rhs_sf_0(tGrid,f,rhs)
GRID *tGrid; FLOAT (*rhs)(); INT f;
{  eprintf("Error: integrate_rhs_sf_0 not available.\n");  }

void integrate_rhs_sf_1(tGrid,f,rhs)
GRID *tGrid; FLOAT (*rhs)(); INT f;
{  eprintf("Error: integrate_rhs_sf_1 not available.\n");  }

void integrate_rhs_sf_3(tGrid,f,rhs)
GRID *tGrid; FLOAT (*rhs)(); INT f;
{  eprintf("Error: integrate_rhs_sf_3 not available.\n");  }

void scalar_p1nc_sd_to_rhs_linear(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid; INT f; FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{  eprintf("Error: scalar_p1nc_sd_to_rhs_linear not available.\n");  }

void scalar_p1nc_sd_to_rhs_cubic(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid; INT f; FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{  eprintf("Error: scalar_p1nc_sd_to_rhs_cubic not available.\n");  }

#endif

#if (F_DATA & VECTOR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX)

void integrate_rhs_vf_1(tGrid,f,rhs0,rhs1)   /*  int_K f*nc_zeta  */
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = VOLUME(pelem); 
      FDV(fa0,f,0) += integr11(n1,n2,n0,rhs0,rdetB);
      FDV(fa1,f,0) += integr11(n2,n0,n1,rhs0,rdetB);
      FDV(fa2,f,0) += integr11(n0,n1,n2,rhs0,rdetB);
      FDV(fa1,f,1) += integr11(n2,n0,n1,rhs1,rdetB);
      FDV(fa0,f,1) += integr11(n1,n2,n0,rhs1,rdetB);
      FDV(fa2,f,1) += integr11(n0,n1,n2,rhs1,rdetB);
   } 
}

void integrate_rhs_vf_3(tGrid,f,rhs0,rhs1)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                         n2->myvertex->x,b);
      FDV(fa0,f,0) += integr_rhs_zeta(nc_ze12,b[0],b[1],b[2],n0,n1,n2,rhs0,rhs0,rhs0,rdetB);
      FDV(fa1,f,0) += integr_rhs_zeta(nc_ze12,b[1],b[2],b[0],n1,n2,n0,rhs0,rhs0,rhs0,rdetB);
      FDV(fa2,f,0) += integr_rhs_zeta(nc_ze12,b[2],b[0],b[1],n2,n0,n1,rhs0,rhs0,rhs0,rdetB);
      FDV(fa0,f,1) += integr_rhs_zeta(nc_ze12,b[0],b[1],b[2],n0,n1,n2,rhs1,rhs1,rhs1,rdetB);
      FDV(fa1,f,1) += integr_rhs_zeta(nc_ze12,b[1],b[2],b[0],n1,n2,n0,rhs1,rhs1,rhs1,rdetB);
      FDV(fa2,f,1) += integr_rhs_zeta(nc_ze12,b[2],b[0],b[1],n2,n0,n1,rhs1,rhs1,rhs1,rdetB);
   } 
}

void vect_p1nc_sd_to_rhs_cubic(tGrid,f,nu,rhs0,rhs1,bb0,bb1)
GRID *tGrid;
INT f;
FLOAT nu, (*rhs0)(), (*rhs1)(), (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT detB, b[DIM2][DIM2], delta;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      delta = sd_delta(pelem,nu,bb0,bb1);
      if (delta > 0.){
         NODES_OF_ELEMENT(n0,n1,n2,pelem);
         FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
         detB = delta*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                             n2->myvertex->x,b);
         if (IS_FF(fa0)){
           FDV(fa0,f,0) += 
           integr_rhs_zeta(nc_bgze12,b[0],b[1],b[2],n0,n1,n2,rhs0,bb0,bb1,detB);
           FDV(fa0,f,1) += 
           integr_rhs_zeta(nc_bgze12,b[0],b[1],b[2],n0,n1,n2,rhs1,bb0,bb1,detB);
         }
         if (IS_FF(fa1)){
           FDV(fa1,f,0) += 
           integr_rhs_zeta(nc_bgze12,b[1],b[2],b[0],n1,n2,n0,rhs0,bb0,bb1,detB);
           FDV(fa1,f,1) += 
           integr_rhs_zeta(nc_bgze12,b[1],b[2],b[0],n1,n2,n0,rhs1,bb0,bb1,detB);
         }
         if (IS_FF(fa2)){
           FDV(fa2,f,0) += 
           integr_rhs_zeta(nc_bgze12,b[2],b[0],b[1],n2,n0,n1,rhs0,bb0,bb1,detB);
           FDV(fa2,f,1) += 
           integr_rhs_zeta(nc_bgze12,b[2],b[0],b[1],n2,n0,n1,rhs1,bb0,bb1,detB);
         }
      }
   } 
}

#else

void integrate_rhs_vf_1(tGrid,f,rhs0,rhs1)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f;
{  eprintf("Error: integrate_rhs_vf_1 not available.\n");  }

void integrate_rhs_vf_3(tGrid,f,rhs0,rhs1)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f;
{  eprintf("Error: integrate_rhs_vf_3 not available.\n");  }

void vect_p1nc_sd_to_rhs_cubic(tGrid,f,nu,rhs0,rhs1,bb0,bb1)
GRID *tGrid; INT f; FLOAT nu, (*rhs0)(), (*rhs1)(), (*bb0)(), (*bb1)();
{  eprintf("Error: vect_p1nc_sd_to_rhs_cubic not available.\n");  }

#endif

#if (DIM == 2) && (F_DATA & VECTOR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX)

void scalar_p1mod_rhs_int_1(n0,n1,n2,fa0,f,rhs,rdetB) /*  rdetB = volume  */
NODE *n0, *n1, *n2;
FACE *fa0;
INT f;
FLOAT (*rhs)(), rdetB;
{
   if (IS_FF(fa0)){ 
      FDV(fa0,f,0) += integr12(n1,n2,n0,rhs,rdetB);
      FDV(fa0,f,1) += integr13(n1,n2,n0,rhs,rdetB);
   }
}

void scalar_p1mod_rhs_int_3(n0,n1,n2,fa0,f,rhs,b0,b1,b2,rdetB)
NODE *n0, *n1, *n2;                                       /*  rdetB = volume  */
FACE *fa0;
INT f;
FLOAT (*rhs)(), b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{
   FLOAT ann, bnn;
  
   if (IS_FF(fa0)){ 
      integr_rhs_fi_chi(b0,b1,b2,n0,n1,n2,rhs,rdetB,&ann,&bnn);
      FDV(fa0,f,0) += ann;
      FDV(fa0,f,1) += bnn;
   }
}

void scalar_p1mod_rhs_int_linear(tGrid,f,rhs)
GRID *tGrid;
INT f;
FLOAT (*rhs)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = VOLUME(pelem);
      scalar_p1mod_rhs_int_1(n0,n1,n2,fa0,f,rhs,rdetB);
      scalar_p1mod_rhs_int_1(n1,n2,n0,fa1,f,rhs,rdetB);
      scalar_p1mod_rhs_int_1(n2,n0,n1,fa2,f,rhs,rdetB);
   } 
}

void scalar_p1mod_rhs_int_cubic(tGrid,f,rhs)
GRID *tGrid;
INT f;
FLOAT (*rhs)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      scalar_p1mod_rhs_int_3(n0,n1,n2,fa0,f,rhs,b[0],b[1],b[2],rdetB);
      scalar_p1mod_rhs_int_3(n1,n2,n0,fa1,f,rhs,b[1],b[2],b[0],rdetB);
      scalar_p1mod_rhs_int_3(n2,n0,n1,fa2,f,rhs,b[2],b[0],b[1],rdetB);
   } 
}

void scalar_p1mod_sd_rhs_3(pel,b0,b1,b2,fa0,n1,n2,rhs,bb0,bb1,detB,f)
ELEMENT *pel;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], (*rhs)(), (*bb0)(), (*bb1)(), detB;
FACE *fa0;
NODE *n1, *n2;
INT f;
{
   FLOAT *x1, *x2, *x3, x112[DIM], x113[DIM], x221[DIM], x223[DIM],
                                              x331[DIM], x332[DIM], x123[DIM];

   if (IS_FF(fa0)){
      VERTICES_OF_ELEMENT(x1,x2,x3,pel);
      points(x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
      /* rhs, bb*(grad fi_i) */
      FDV(fa0,f,0) += get_pq3_rhs(rhs,nc_bgfi12,b0,b1,b2,n1,n2,bb0,bb1,detB,
                                  x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
      /* rhs, bb*(grad chi_i) */
      FDV(fa0,f,1) += get_pq3_rhs(rhs,nc_bgchi12,b0,b1,b2,n1,n2,bb0,bb1,detB,
                                  x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   }
}

void scalar_p1mod_sd_to_rhs_cubic(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid;
INT f;
FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pel;
   FLOAT detB, b[DIM2][DIM2], delta;

   for (pel = FIRSTELEMENT(tGrid);pel != NULL;pel = pel->succ){
      delta = sd_delta(pel,nu,bb0,bb1);
      if (delta > 0.){
         NODES_OF_ELEMENT(n0,n1,n2,pel);
         FACES_OF_ELEMENT(fa0,fa1,fa2,pel);
         detB = delta*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                             n2->myvertex->x,b);
         scalar_p1mod_sd_rhs_3(pel,b[0],b[1],b[2],fa0,n1,n2,rhs,bb0,bb1,detB,f);
         scalar_p1mod_sd_rhs_3(pel,b[1],b[2],b[0],fa1,n2,n0,rhs,bb0,bb1,detB,f);
         scalar_p1mod_sd_rhs_3(pel,b[2],b[0],b[1],fa2,n0,n1,rhs,bb0,bb1,detB,f);
      }
   } 
}

#else

void scalar_p1mod_rhs_int_linear(tGrid,f,rhs)
GRID *tGrid; INT f; FLOAT (*rhs)();
{  eprintf("Error: scalar_p1mod_rhs_int_linear not available.\n");  }

void scalar_p1mod_rhs_int_cubic(tGrid,f,rhs)
GRID *tGrid; INT f; FLOAT (*rhs)();
{  eprintf("Error: scalar_p1mod_rhs_int_cubic not available.\n");  }

void scalar_p1mod_sd_to_rhs_cubic(tGrid,f,nu,rhs,bb0,bb1)
GRID *tGrid; INT f; FLOAT nu, (*rhs)(), (*bb0)(), (*bb1)();
{  eprintf("Error: scalar_p1mod_sd_to_rhs_cubic not available.\n");  }

#endif

#if (DIM == 2) && (F_DATA & DVECTOR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX)

void p1mod_rhs_int_1(n0,n1,n2,fa0,f,rhs0,rhs1,rdetB)
NODE *n0, *n1, *n2;
FACE *fa0;
INT f;
FLOAT (*rhs0)(), (*rhs1)(), rdetB;
{
   if (IS_FF(fa0)){ 
      FDDV(fa0,f,0,0) += integr12(n1,n2,n0,rhs0,rdetB);
      FDDV(fa0,f,0,1) += integr12(n1,n2,n0,rhs1,rdetB);
      FDDV(fa0,f,1,0) += integr13(n1,n2,n0,rhs0,rdetB);
      FDDV(fa0,f,1,1) += integr13(n1,n2,n0,rhs1,rdetB);
   }
}

void p1mod_rhs_int_3(n0,n1,n2,fa0,f,rhs0,rhs1,b0,b1,b2,rdetB)
NODE *n0, *n1, *n2;
FACE *fa0;
INT f;
FLOAT (*rhs0)(), (*rhs1)(), b0[DIM2], b1[DIM2], b2[DIM2], rdetB;
{
   FLOAT ann, bnn;
  
   if (IS_FF(fa0)){ 
      integr_rhs_fi_chi(b0,b1,b2,n0,n1,n2,rhs0,rdetB,&ann,&bnn);
      FDDV(fa0,f,0,0) += ann;
      FDDV(fa0,f,1,0) += bnn;
      integr_rhs_fi_chi(b0,b1,b2,n0,n1,n2,rhs1,rdetB,&ann,&bnn);
      FDDV(fa0,f,0,1) += ann;
      FDDV(fa0,f,1,1) += bnn;
   }
}

void p1mod_rhs_int_linear(tGrid,f,rhs0,rhs1)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = VOLUME(pelem);
      p1mod_rhs_int_1(n0,n1,n2,fa0,f,rhs0,rhs1,rdetB);
      p1mod_rhs_int_1(n1,n2,n0,fa1,f,rhs0,rhs1,rdetB);
      p1mod_rhs_int_1(n2,n0,n1,fa2,f,rhs0,rhs1,rdetB);
   } 
}

void p1mod_rhs_int_cubic(tGrid,f,rhs0,rhs1)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT rdetB, b[DIM2][DIM2];

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      p1mod_rhs_int_3(n0,n1,n2,fa0,f,rhs0,rhs1,b[0],b[1],b[2],rdetB);
      p1mod_rhs_int_3(n1,n2,n0,fa1,f,rhs0,rhs1,b[1],b[2],b[0],rdetB);
      p1mod_rhs_int_3(n2,n0,n1,fa2,f,rhs0,rhs1,b[2],b[0],b[1],rdetB);
   } 
}

void vect_p1mod_sd_rhs_3(pel,b0,b1,b2,fa0,n1,n2,rhs0,rhs1,bb0,bb1,detB,f)
ELEMENT *pel;
FLOAT b0[DIM2], b1[DIM2], b2[DIM2], (*rhs0)(), (*rhs1)(), (*bb0)(), (*bb1)(), detB;
FACE *fa0;
NODE *n1, *n2;
INT f;
{
   FLOAT *x1, *x2, *x3, x112[DIM], x113[DIM], x221[DIM], x223[DIM],
                                              x331[DIM], x332[DIM], x123[DIM];

   if (IS_FF(fa0)){
      VERTICES_OF_ELEMENT(x1,x2,x3,pel);
      points(x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
      /* rhs, bb*(grad fi_i) */
      FDDV(fa0,f,0,0) += get_pq3_rhs(rhs0,nc_bgfi12,b0,b1,b2,n1,n2,bb0,bb1,detB,
                                   x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
      FDDV(fa0,f,0,1) += get_pq3_rhs(rhs1,nc_bgfi12,b0,b1,b2,n1,n2,bb0,bb1,detB,
                                   x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
      /* rhs, bb*(grad chi_i) */
      FDDV(fa0,f,1,0) += get_pq3_rhs(rhs0,nc_bgchi12,b0,b1,b2,n1,n2,bb0,bb1,detB,
                                   x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
      FDDV(fa0,f,1,1) += get_pq3_rhs(rhs1,nc_bgchi12,b0,b1,b2,n1,n2,bb0,bb1,detB,
                                   x1,x2,x3,x112,x113,x221,x223,x331,x332,x123);
   }
}

void vect_p1mod_sd_to_rhs_cubic(tGrid,f,nu,rhs0,rhs1,bb0,bb1)
GRID *tGrid;
INT f;
FLOAT nu, (*rhs0)(), (*rhs1)(), (*bb0)(), (*bb1)();
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pel;
   FLOAT detB, b[DIM2][DIM2], delta;

   for (pel = FIRSTELEMENT(tGrid);pel != NULL;pel = pel->succ){
      delta = sd_delta(pel,nu,bb0,bb1);
      if (delta > 0.){
         NODES_OF_ELEMENT(n0,n1,n2,pel);
         FACES_OF_ELEMENT(fa0,fa1,fa2,pel);
         detB = delta*barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                             n2->myvertex->x,b);
         vect_p1mod_sd_rhs_3(pel,b[0],b[1],b[2],fa0,n1,n2,rhs0,rhs1,bb0,bb1,detB,f);
         vect_p1mod_sd_rhs_3(pel,b[1],b[2],b[0],fa1,n2,n0,rhs0,rhs1,bb0,bb1,detB,f);
         vect_p1mod_sd_rhs_3(pel,b[2],b[0],b[1],fa2,n0,n1,rhs0,rhs1,bb0,bb1,detB,f);
      }
   } 
}

#else

void p1mod_rhs_int_linear(tGrid,f,rhs0,rhs1)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f;
{  eprintf("Error: p1mod_rhs_int_linear not available.\n");  }

void p1mod_rhs_int_cubic(tGrid,f,rhs0,rhs1)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f;
{  eprintf("Error: p1mod_rhs_int_cubic not available.\n");  }

void vect_p1mod_sd_to_rhs_cubic(tGrid,f,nu,rhs0,rhs1,bb0,bb1)
GRID *tGrid; INT f; FLOAT nu, (*rhs0)(), (*rhs1)(), (*bb0)(), (*bb1)();
{  eprintf("Error: vect_p1mod_sd_to_rhs_cubic not available.\n");  }

#endif

#if (ELEMENT_TYPE == CUBE) && (DIM == 2)

void q5_points(h,xc,x11,x12,x13,x21,x22,x23,x31,x32,x33)
FLOAT h, *xc, *x11, *x12, *x13, *x21, *x22, *x23, *x31, *x32, *x33;
{
   DOUBLE d=0.1*sqrt(15.)*h;

   x11[0] = x21[0] = x31[0] = xc[0] - d;
   x12[0] = x22[0] = x32[0] = xc[0];
   x13[0] = x23[0] = x33[0] = xc[0] + d;
   x11[1] = x12[1] = x13[1] = xc[1] - d;
   x21[1] = x22[1] = x23[1] = xc[1];
   x31[1] = x32[1] = x33[1] = xc[1] + d;
}

#endif

#if (F_DATA & VECTOR_FACE_DATA) && (ELEMENT_TYPE == CUBE) && (DIM == 2)

/*  integration formula exact for Q5 functions  */
void add_q1rot_rhs(xc,h,phi,fa,f,f01,f02)
FACE *fa;
DOUBLE *xc, h, (*phi)(), (*f01)(), (*f02)();
INT f;
{
   DOUBLE x11[DIM], x12[DIM], x13[DIM], x21[DIM], x22[DIM], x23[DIM], 
          x31[DIM], x32[DIM], x33[DIM], vol=h*h;

   q5_points(h,xc,x11,x12,x13,x21,x22,x23,x31,x32,x33);
   FDV(fa,f,0) += ( 25.*(f01(x11)*phi(h,xc,x11)+f01(x13)*phi(h,xc,x13)+
                         f01(x31)*phi(h,xc,x31)+f01(x33)*phi(h,xc,x33))+
                    40.*(f01(x12)*phi(h,xc,x12)+f01(x21)*phi(h,xc,x21)+
                         f01(x23)*phi(h,xc,x23)+f01(x32)*phi(h,xc,x32))+
                     64.*f01(x22)*phi(h,xc,x22) )/324.*vol;
   FDV(fa,f,1) += ( 25.*(f02(x11)*phi(h,xc,x11)+f02(x13)*phi(h,xc,x13)+
                         f02(x31)*phi(h,xc,x31)+f02(x33)*phi(h,xc,x33))+
                    40.*(f02(x12)*phi(h,xc,x12)+f02(x21)*phi(h,xc,x21)+
                         f02(x23)*phi(h,xc,x23)+f02(x32)*phi(h,xc,x32))+
                     64.*f02(x22)*phi(h,xc,x22) )/324.*vol;
}

void q1rot_rhs(tGrid,f,f01,f02)
GRID *tGrid;
DOUBLE (*f01)(), (*f02)();
INT f;
{
   ELEMENT *pelem;
   NODE *n0, *n1, *n2, *n3;
   FACE *fa0, *fa1, *fa2, *fa3;
   FLOAT xc[DIM], h;

   for (pelem = FIRSTELEMENT(tGrid); pelem != NULL; pelem = pelem->succ){
      NODES_OF_4ELEMENT(n0,n1,n2,n3,pelem);
      FACES_OF_4ELEMENT(fa0,fa1,fa2,fa3,pelem);
      xc[0] = 0.5*(n0->myvertex->x[0]+n1->myvertex->x[0]);
      xc[1] = 0.5*(n1->myvertex->x[1]+n2->myvertex->x[1]);
      h = n1->myvertex->x[0]-n0->myvertex->x[0];
      add_q1rot_rhs(xc,h,phi0,fa0,f,f01,f02);
      add_q1rot_rhs(xc,h,phi1,fa1,f,f01,f02);
      add_q1rot_rhs(xc,h,phi2,fa2,f,f01,f02);
      add_q1rot_rhs(xc,h,phi3,fa3,f,f01,f02);
   }
}

#else

void q1rot_rhs(tGrid,f,f01,f02)
GRID *tGrid; DOUBLE (*f01)(), (*f02)(); INT f;
{  eprintf("Error: q1rot_rhs not available.\n");  }

#endif

#if (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX)

void integrate_rhs_vn_vf_2(tGrid,f,rhs0,rhs1,bubble)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f, bubble;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, x01[DIM], x02[DIM], x12[DIM], 
          f0, f1, f2, f01, f02, f12, rdetB;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      rdetB = VOLUME(pelem);
      MIDPOINTS(x0,x1,x2,x01,x02,x12)
      POINT_VALUES_6(x0,x1,x2,x01,x02,x12,f0,f1,f2,f01,f02,f12,rhs0)
      ND(n0,f,0) += INTEGR1_2(f0,f1,f2,f01,f02,f12)*rdetB;
      ND(n1,f,0) += INTEGR1_2(f1,f2,f0,f12,f01,f02)*rdetB;
      ND(n2,f,0) += INTEGR1_2(f2,f0,f1,f02,f12,f01)*rdetB;
      FDV(fa0,f,0) += INTEGR3_2(f0,f01,f02,f12)*rdetB;
      FDV(fa1,f,0) += INTEGR3_2(f1,f12,f01,f02)*rdetB;
      FDV(fa2,f,0) += INTEGR3_2(f2,f02,f12,f01)*rdetB;
      if (bubble)
         set_edv(pelem,f,0,(8.*(f01 + f02 + f12) - (f0 + f1 + f2))*rdetB/1260.);
      POINT_VALUES_6(x0,x1,x2,x01,x02,x12,f0,f1,f2,f01,f02,f12,rhs1)
      ND(n0,f,1) += INTEGR1_2(f0,f1,f2,f01,f02,f12)*rdetB;
      ND(n1,f,1) += INTEGR1_2(f1,f2,f0,f12,f01,f02)*rdetB;
      ND(n2,f,1) += INTEGR1_2(f2,f0,f1,f02,f12,f01)*rdetB;
      FDV(fa0,f,1) += INTEGR3_2(f0,f01,f02,f12)*rdetB;
      FDV(fa1,f,1) += INTEGR3_2(f1,f12,f01,f02)*rdetB;
      FDV(fa2,f,1) += INTEGR3_2(f2,f02,f12,f01)*rdetB;
      if (bubble)
         set_edv(pelem,f,1,(8.*(f01 + f02 + f12) - (f0 + f1 + f2))*rdetB/1260.);
   }
}

void integrate_rhs_vn_vf_3(tGrid,f,rhs0,rhs1,bubble)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f, bubble;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, rdetB, s, c;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      rdetB = VOLUME(pelem);
      points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012);
      POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                      f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs0)
      s = f0 + f1 + f2;
      c = s + 18.*f012;
      ND(n0,f,0) += INTEGR4_2(f0,f001,f002,c)*rdetB;
      ND(n1,f,0) += INTEGR4_2(f1,f110,f112,c)*rdetB;
      ND(n2,f,0) += INTEGR4_2(f2,f220,f221,c)*rdetB;
      c = f001 + f002 + f110 + f112 + f220 + f221;
      if (bubble)
         set_edv(pelem,f,0,(2.*s + 9.*c + 108.*f012)*rdetB/10080.);
      c = s + 6.*c + 36.*f012;
      FDV(fa0,f,0) += INTEGR5_2(f0,f112,f221,f001,f002,c)*rdetB;
      FDV(fa1,f,0) += INTEGR5_2(f1,f002,f220,f110,f112,c)*rdetB;
      FDV(fa2,f,0) += INTEGR5_2(f2,f001,f110,f220,f221,c)*rdetB;
      POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                      f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs1)
      s = f0 + f1 + f2;
      c = s + 18.*f012;
      ND(n0,f,1) += INTEGR4_2(f0,f001,f002,c)*rdetB;
      ND(n1,f,1) += INTEGR4_2(f1,f110,f112,c)*rdetB;
      ND(n2,f,1) += INTEGR4_2(f2,f220,f221,c)*rdetB;
      c = f001 + f002 + f110 + f112 + f220 + f221;
      if (bubble)
         set_edv(pelem,f,1,(2.*s + 9.*c + 108.*f012)*rdetB/10080.);
      c = s + 6.*c + 36.*f012;
      FDV(fa0,f,1) += INTEGR5_2(f0,f112,f221,f001,f002,c)*rdetB;
      FDV(fa1,f,1) += INTEGR5_2(f1,f002,f220,f110,f112,c)*rdetB;
      FDV(fa2,f,1) += INTEGR5_2(f2,f001,f110,f220,f221,c)*rdetB;
   }
}

void q_integrate_rhs_vn_vf(tGrid,f,rhs0,rhs1,bubble,qr)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f, bubble;
QUADRATURE_RULE qr;
{
   ELEMENT *pelem;
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   FLOAT a[DIM][DIM], c[DIM], vol;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      vol = P1_reference_mapping0(n0,n1,n2,a,c);
      ND(n0,f,0) += integr_f_X_phi_p1(rhs0,r_l0,a,c,vol,qr);
      ND(n1,f,0) += integr_f_X_phi_p1(rhs0,r_l1,a,c,vol,qr);
      ND(n2,f,0) += integr_f_X_phi_p1(rhs0,r_l2,a,c,vol,qr);
      FDV(fa0,f,0) += integr_f_X_phi_p1(rhs0,r_l1l2,a,c,vol,qr);
      FDV(fa1,f,0) += integr_f_X_phi_p1(rhs0,r_l0l2,a,c,vol,qr);
      FDV(fa2,f,0) += integr_f_X_phi_p1(rhs0,r_l0l1,a,c,vol,qr);
      if (bubble)
         set_edv(pelem,f,0,integr_f_X_phi_p1(rhs0,r_l0l1l2,a,c,vol,qr));
      ND(n0,f,1) += integr_f_X_phi_p1(rhs1,r_l0,a,c,vol,qr);
      ND(n1,f,1) += integr_f_X_phi_p1(rhs1,r_l1,a,c,vol,qr);
      ND(n2,f,1) += integr_f_X_phi_p1(rhs1,r_l2,a,c,vol,qr);
      FDV(fa0,f,1) += integr_f_X_phi_p1(rhs1,r_l1l2,a,c,vol,qr);
      FDV(fa1,f,1) += integr_f_X_phi_p1(rhs1,r_l0l2,a,c,vol,qr);
      FDV(fa2,f,1) += integr_f_X_phi_p1(rhs1,r_l0l1,a,c,vol,qr);
      if (bubble)
         set_edv(pelem,f,1,integr_f_X_phi_p1(rhs1,r_l0l1l2,a,c,vol,qr));
   }
}

#else

void integrate_rhs_vn_vf_2(tGrid,f,rhs0,rhs1,bubble)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f, bubble;
{  eprintf("Error: integrate_rhs_vn_vf_2 not available.\n");  }

void integrate_rhs_vn_vf_3(tGrid,f,rhs0,rhs1,bubble)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f, bubble;
{  eprintf("Error: integrate_rhs_vn_vf_3 not available.\n");  }

void q_integrate_rhs_vn_vf(tGrid,f,rhs0,rhs1,bubble,qr)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f, bubble; QUADRATURE_RULE qr;
{  eprintf("Error: q_integrate_rhs_vn_vf not available.\n");  }

#endif

#if (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA) && (F_DATA & CURVED_FACE_MIDDLE)

void integrate_rhs_vn_vf_3_iso(tGrid,f,rhs0,rhs1,bubble)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f, bubble;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, x001[DIM], x002[DIM], x110[DIM], x112[DIM], x220[DIM], 
         x221[DIM], x012[DIM], 
         f0, f1, f2, f001, f002, f110, f112, f220, f221, f012, 
         a[DIM][DIM], c[DIM], alpha[DIM], j[DIM2], s;

   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      C_DATA_OF_ELEMENT(n0,n1,n2,fa0,fa1,fa2,x0,x1,x2,pelem)
      P2_reference_mapping0(n0,n1,n2,fa0,a,c,alpha,j);
      mapped_points(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,a,c,alpha);
      s = SGN(j[2]);
      POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                      f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs0)
      ND(n0,f,0) += (f0*j[0] + 14*f0*j[2] + f0*j[1] + j[1]*f2 + 12*j[1]*f220 
         + 12*j[1]*f002
         + 63*j[2]*f001 + 7*j[2]*f1 + 12*j[0]*f001 + j[0]*f1 + 12*j[0]*f110
         + 6*j[1]*f001 - 3*j[1]*f110 - 3*j[1]*f112 + 36*j[1]*f012 + 6*j[1]*f221
         + 2*j[1]*f1 + 63*j[2]*f002 + 7*j[2]*f2 + 126*j[2]*f012 + 6*j[0]*f112
         + 36*j[0]*f012 - 3*j[0]*f221 + 6*j[0]*f002 + 2*j[0]*f2 
         - 3*j[0]*f220)/1680.*s;
      ND(n1,f,0) += (4*f0*j[0] + 7*f0*j[2] + 2*f0*j[1] + j[1]*f2 + 6*j[1]*f220 
         - 3*j[1]*f002
         + 14*j[2]*f1 + 63*j[2]*f110 - 9*j[0]*f001 + 12*j[0]*f1 + 45*j[0]*f110
         - 3*j[1]*f001 + 6*j[1]*f110 + 12*j[1]*f112 + 36*j[1]*f012
         + 12*j[1]*f221 + j[1]*f1 + 7*j[2]*f2 + 63*j[2]*f112 + 126*j[2]*f012
         + 45*j[0]*f112 + 54*j[0]*f012 - 9*j[0]*f221 - 3*j[0]*f002 + 4*j[0]*f2
         - 3*j[0]*f220)/1680.*s;
      ND(n2,f,0) += (2*f0*j[0] + 7*f0*j[2] + 4*f0*j[1] + 12*j[1]*f2 
         + 45*j[1]*f220
         - 9*j[1]*f002 + 7*j[2]*f1 - 3*j[0]*f001 + j[0]*f1 + 6*j[0]*f110
         - 3*j[1]*f001 - 3*j[1]*f110 - 9*j[1]*f112 + 54*j[1]*f012
         + 45*j[1]*f221 + 4*j[1]*f1 + 14*j[2]*f2 + 63*j[2]*f220 + 126*j[2]*f012
         + 63*j[2]*f221 + 12*j[0]*f112 + 36*j[0]*f012 + 12*j[0]*f221
         - 3*j[0]*f002 + j[0]*f2 + 6*j[0]*f220)/1680.*s;
      FDV(fa0,f,0) += (11*f0*j[0] + 24*f0*j[2] + 11*f0*j[1] + 6*j[1]*f2 
         + 54*j[1]*f220
         - 27*j[1]*f002 - 36*j[2]*f001 + 12*j[2]*f1 + 72*j[2]*f110
         - 27*j[0]*f001 + 6*j[0]*f1 + 54*j[0]*f110 - 18*j[1]*f001 + 9*j[1]*f110
         + 27*j[1]*f112 + 162*j[1]*f012 + 108*j[1]*f221 + 4*j[1]*f1
         - 36*j[2]*f002 + 12*j[2]*f2 + 72*j[2]*f220 + 144*j[2]*f112
         + 432*j[2]*f012 + 144*j[2]*f221 + 108*j[0]*f112 + 162*j[0]*f012
         + 27*j[0]*f221 - 18*j[0]*f002 + 4*j[0]*f2 + 9*j[0]*f220)/20160.*s;
      FDV(fa1,f,0) += (2*f0*j[0] + 12*f0*j[2] + 4*f0*j[1] + 6*j[1]*f2 
         + 108*j[1]*f220
         + 27*j[1]*f002 + 72*j[2]*f001 + 24*j[2]*f1 - 36*j[2]*f110
         + 9*j[0]*f001 + 2*j[0]*f1 + 9*j[0]*f110 + 9*j[1]*f001 - 18*j[1]*f110
         - 27*j[1]*f112 + 162*j[1]*f012 + 54*j[1]*f221 + 11*j[1]*f1
         + 144*j[2]*f002 + 12*j[2]*f2 + 144*j[2]*f220 - 36*j[2]*f112
         + 432*j[2]*f012 + 72*j[2]*f221 + 9*j[0]*f112 + 108*j[0]*f012
         + 9*j[0]*f221 + 9*j[0]*f002 + 2*j[0]*f2 + 9*j[0]*f220)/20160.*s;
      FDV(fa2,f,0) += (4*f0*j[0] + 12*f0*j[2] + 2*f0*j[1] + 2*j[1]*f2 
         + 9*j[1]*f220
         + 9*j[1]*f002 + 144*j[2]*f001 + 12*j[2]*f1 + 144*j[2]*f110
         + 27*j[0]*f001 + 6*j[0]*f1 + 108*j[0]*f110 + 9*j[1]*f001 + 9*j[1]*f110
         + 9*j[1]*f112 + 108*j[1]*f012 + 9*j[1]*f221 + 2*j[1]*f1 + 72*j[2]*f002
         + 24*j[2]*f2 - 36*j[2]*f220 + 72*j[2]*f112 + 432*j[2]*f012
         - 36*j[2]*f221 + 54*j[0]*f112 + 162*j[0]*f012 - 27*j[0]*f221
         + 9*j[0]*f002 + 11*j[0]*f2 - 18*j[0]*f220)/20160.*s;
      if (bubble)
         set_edv(pelem,f,0,(f0*j[1] + 2*f0*j[2] + f0*j[0] + 9*j[1]*f220 
         + 36*j[1]*f012 + 9*j[1]*f221 + 2*j[2]*f2 + 9*j[2]*f220 + 9*j[2]*f002 
         + j[0]*f2 + 9*j[2]*f001 + 108*j[2]*f012 + 9*j[2]*f221 + 36*j[0]*f012 
         + j[1]*f1 + 9*j[2]*f110 + 9*j[2]*f112 + 9*j[0]*f110 + 9*j[0]*f112 
         + 2*j[2]*f1)/20160.*s);
      POINT_VALUES_10(x0,x1,x2,x001,x002,x110,x112,x220,x221,x012,
                      f0,f1,f2,f001,f002,f110,f112,f220,f221,f012,rhs1)
      ND(n0,f,1) += (f0*j[0] + 14*f0*j[2] + f0*j[1] + j[1]*f2 + 12*j[1]*f220 
         + 12*j[1]*f002
         + 63*j[2]*f001 + 7*j[2]*f1 + 12*j[0]*f001 + j[0]*f1 + 12*j[0]*f110
         + 6*j[1]*f001 - 3*j[1]*f110 - 3*j[1]*f112 + 36*j[1]*f012 + 6*j[1]*f221
         + 2*j[1]*f1 + 63*j[2]*f002 + 7*j[2]*f2 + 126*j[2]*f012 + 6*j[0]*f112
         + 36*j[0]*f012 - 3*j[0]*f221 + 6*j[0]*f002 + 2*j[0]*f2 
         - 3*j[0]*f220)/1680.*s;
      ND(n1,f,1) += (4*f0*j[0] + 7*f0*j[2] + 2*f0*j[1] + j[1]*f2 + 6*j[1]*f220 
         - 3*j[1]*f002
         + 14*j[2]*f1 + 63*j[2]*f110 - 9*j[0]*f001 + 12*j[0]*f1 + 45*j[0]*f110
         - 3*j[1]*f001 + 6*j[1]*f110 + 12*j[1]*f112 + 36*j[1]*f012
         + 12*j[1]*f221 + j[1]*f1 + 7*j[2]*f2 + 63*j[2]*f112 + 126*j[2]*f012
         + 45*j[0]*f112 + 54*j[0]*f012 - 9*j[0]*f221 - 3*j[0]*f002 + 4*j[0]*f2
         - 3*j[0]*f220)/1680.*s;
      ND(n2,f,1) += (2*f0*j[0] + 7*f0*j[2] + 4*f0*j[1] + 12*j[1]*f2 
         + 45*j[1]*f220
         - 9*j[1]*f002 + 7*j[2]*f1 - 3*j[0]*f001 + j[0]*f1 + 6*j[0]*f110
         - 3*j[1]*f001 - 3*j[1]*f110 - 9*j[1]*f112 + 54*j[1]*f012
         + 45*j[1]*f221 + 4*j[1]*f1 + 14*j[2]*f2 + 63*j[2]*f220 + 126*j[2]*f012
         + 63*j[2]*f221 + 12*j[0]*f112 + 36*j[0]*f012 + 12*j[0]*f221
         - 3*j[0]*f002 + j[0]*f2 + 6*j[0]*f220)/1680.*s;
      FDV(fa0,f,1) += (11*f0*j[0] + 24*f0*j[2] + 11*f0*j[1] + 6*j[1]*f2 
         + 54*j[1]*f220
         - 27*j[1]*f002 - 36*j[2]*f001 + 12*j[2]*f1 + 72*j[2]*f110
         - 27*j[0]*f001 + 6*j[0]*f1 + 54*j[0]*f110 - 18*j[1]*f001 + 9*j[1]*f110
         + 27*j[1]*f112 + 162*j[1]*f012 + 108*j[1]*f221 + 4*j[1]*f1
         - 36*j[2]*f002 + 12*j[2]*f2 + 72*j[2]*f220 + 144*j[2]*f112
         + 432*j[2]*f012 + 144*j[2]*f221 + 108*j[0]*f112 + 162*j[0]*f012
         + 27*j[0]*f221 - 18*j[0]*f002 + 4*j[0]*f2 + 9*j[0]*f220)/20160.*s;
      FDV(fa1,f,1) += (2*f0*j[0] + 12*f0*j[2] + 4*f0*j[1] + 6*j[1]*f2 
         + 108*j[1]*f220
         + 27*j[1]*f002 + 72*j[2]*f001 + 24*j[2]*f1 - 36*j[2]*f110
         + 9*j[0]*f001 + 2*j[0]*f1 + 9*j[0]*f110 + 9*j[1]*f001 - 18*j[1]*f110
         - 27*j[1]*f112 + 162*j[1]*f012 + 54*j[1]*f221 + 11*j[1]*f1
         + 144*j[2]*f002 + 12*j[2]*f2 + 144*j[2]*f220 - 36*j[2]*f112
         + 432*j[2]*f012 + 72*j[2]*f221 + 9*j[0]*f112 + 108*j[0]*f012
         + 9*j[0]*f221 + 9*j[0]*f002 + 2*j[0]*f2 + 9*j[0]*f220)/20160.*s;
      FDV(fa2,f,1) += (4*f0*j[0] + 12*f0*j[2] + 2*f0*j[1] + 2*j[1]*f2 
         + 9*j[1]*f220
         + 9*j[1]*f002 + 144*j[2]*f001 + 12*j[2]*f1 + 144*j[2]*f110
         + 27*j[0]*f001 + 6*j[0]*f1 + 108*j[0]*f110 + 9*j[1]*f001 + 9*j[1]*f110
         + 9*j[1]*f112 + 108*j[1]*f012 + 9*j[1]*f221 + 2*j[1]*f1 + 72*j[2]*f002
         + 24*j[2]*f2 - 36*j[2]*f220 + 72*j[2]*f112 + 432*j[2]*f012
         - 36*j[2]*f221 + 54*j[0]*f112 + 162*j[0]*f012 - 27*j[0]*f221
         + 9*j[0]*f002 + 11*j[0]*f2 - 18*j[0]*f220)/20160.*s;
      if (bubble)
         set_edv(pelem,f,1,(f0*j[1] + 2*f0*j[2] + f0*j[0] + 9*j[1]*f220 
         + 36*j[1]*f012 + 9*j[1]*f221 + 2*j[2]*f2 + 9*j[2]*f220 + 9*j[2]*f002 
         + j[0]*f2 + 9*j[2]*f001 + 108*j[2]*f012 + 9*j[2]*f221 + 36*j[0]*f012 
         + j[1]*f1 + 9*j[2]*f110 + 9*j[2]*f112 + 9*j[0]*f110 + 9*j[0]*f112 
         + 2*j[2]*f1)/20160.*s);
   }
}

#else

void integrate_rhs_vn_vf_3_iso(tGrid,f,rhs0,rhs1,bubble)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f, bubble;
{  eprintf("Error: integrate_rhs_vn_vf_3_iso not available.\n");  }

#endif

#if (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA) && (MOVING_BOUNDARY == YES)

void integrate_rhs_vn_vf(tGrid,f,rhs0,rhs1)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)();
INT f;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      
#if (IZOPARAMETRIC==YES)

      if (IS_CURVED_EDGE(fa2)) {
        rhs_izo(n2,n0,n1,fa2,fa0,fa1,f,rhs0,0);
        rhs_izo(n2,n0,n1,fa2,fa0,fa1,f,rhs1,1);
     }
      else
      if (IS_CURVED_EDGE(fa1)){
        rhs_izo(n1,n2,n0,fa1,fa2,fa0,f,rhs0,0);
        rhs_izo(n1,n2,n0,fa1,fa2,fa0,f,rhs1,1);
      }
      else 
      if (IS_CURVED_EDGE(fa0)){
        rhs_izo(n0,n1,n2,fa0,fa1,fa2,f,rhs0,0);
        rhs_izo(n0,n1,n2,fa0,fa1,fa2,f,rhs1,1);
      }   
      else
#endif      
      {
        rhs_izo(n0,n1,n2,fa0,fa1,fa2,f,rhs0,0);
        rhs_izo(n0,n1,n2,fa0,fa1,fa2,f,rhs1,1);
      }
   }
}

#else

void integrate_rhs_vn_vf(tGrid,f,rhs0,rhs1)
GRID *tGrid; FLOAT (*rhs0)(), (*rhs1)(); INT f;
{  eprintf("Error: integrate_rhs_vn_vf not available.\n");  }

#endif

/*  Below:
     w = w0*l0 + w1*l1 + w2*l2 + f2*l0*l1 + f1*l0*l2 + f0*l1*l2
     z = z0*l0 + z1*l1 + z2*l2
     ww = w0+w1+w2, ff = f0+f1+f2, zz = z0+z1+z2, wz = w0*z0+w1*z1+w2*z2  */

/*  (1/|K|)*int_K w*z*lambda_0 dx  */
#define INTEGR_W_Z_L0(w0,w1,w2,f0,f1,f2,z0,z1,z2,ww,ff,zz)                     \
   ((ff*zz+2.*(ff-f0)*z0+f2*z1+f1*z2+3.*(2.*ww*zz+4.*w0*z0-w1*z2-w2*z1))/180.)

/*  (1/|K|)*int_K w*z*lambda_1*lambda_2 dx  */
#define INTEGR_W_Z_L1_L2(w0,w1,w2,f0,f1,f2,z0,z1,z2,ww,ff,zz,wz)               \
    ((2.*ff*zz+f1*z2+f2*z1+4.*f0*(zz-z0)+7.*(ww*zz+w1*z2+w2*z1)+               \
                                                     14.*(wz-w0*z0))/1260.)

#if (N_DATA & VECTOR_NODE_DATA) && (F_DATA & VECTOR_FACE_DATA) && (N_DATA & SCALAR_NODE_DATA) && (F_DATA & SCALAR_FACE_DATA) && (ELEMENT_TYPE == SIMPLEX)

/* rhs = x*cont. pw. P2 function saved in u  */
void integrate_rhs_vn_vf_xp2(tGrid,f,u)
GRID *tGrid;
INT f, u;
{
   NODE *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   ELEMENT *pelem;
   FLOAT *x0, *x1, *x2, w0, w1, w2, f0, f1, f2, ww, ff, zz, wz, rdetB;

   for (pelem = FIRSTELEMENT(tGrid); pelem; pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      VERTICES_OF_ELEMENT(x0,x1,x2,pelem);
      rdetB = VOLUME(pelem);
      w0 = NDS(n0,u);
      w1 = NDS(n1,u);
      w2 = NDS(n2,u);
      f0 = FD(fa0,u);
      f1 = FD(fa1,u);
      f2 = FD(fa2,u);
      ww = w0 + w1 + w2;
      ff = f0 + f1 + f2;
      zz = x0[0] + x1[0] + x2[0];
      wz = w0*x0[0] + w1*x1[0] + w2*x2[0];
      ND(n0,f,0) += 
              INTEGR_W_Z_L0(w0,w1,w2,f0,f1,f2,x0[0],x1[0],x2[0],ww,ff,zz)*rdetB;
      ND(n1,f,0) += 
              INTEGR_W_Z_L0(w1,w2,w0,f1,f2,f0,x1[0],x2[0],x0[0],ww,ff,zz)*rdetB;
      ND(n2,f,0) += 
              INTEGR_W_Z_L0(w2,w0,w1,f2,f0,f1,x2[0],x0[0],x1[0],ww,ff,zz)*rdetB;
      FDV(fa0,f,0) += 
        INTEGR_W_Z_L1_L2(w0,w1,w2,f0,f1,f2,x0[0],x1[0],x2[0],ww,ff,zz,wz)*rdetB;
      FDV(fa1,f,0) += 
        INTEGR_W_Z_L1_L2(w1,w2,w0,f1,f2,f0,x1[0],x2[0],x0[0],ww,ff,zz,wz)*rdetB;
      FDV(fa2,f,0) += 
        INTEGR_W_Z_L1_L2(w2,w0,w1,f2,f0,f1,x2[0],x0[0],x1[0],ww,ff,zz,wz)*rdetB;
      zz = x0[1] + x1[1] + x2[1];
      wz = w0*x0[1] + w1*x1[1] + w2*x2[1];
      ND(n0,f,1) += 
              INTEGR_W_Z_L0(w0,w1,w2,f0,f1,f2,x0[1],x1[1],x2[1],ww,ff,zz)*rdetB;
      ND(n1,f,1) += 
              INTEGR_W_Z_L0(w1,w2,w0,f1,f2,f0,x1[1],x2[1],x0[1],ww,ff,zz)*rdetB;
      ND(n2,f,1) += 
              INTEGR_W_Z_L0(w2,w0,w1,f2,f0,f1,x2[1],x0[1],x1[1],ww,ff,zz)*rdetB;
      FDV(fa0,f,1) += 
        INTEGR_W_Z_L1_L2(w0,w1,w2,f0,f1,f2,x0[1],x1[1],x2[1],ww,ff,zz,wz)*rdetB;
      FDV(fa1,f,1) += 
        INTEGR_W_Z_L1_L2(w1,w2,w0,f1,f2,f0,x1[1],x2[1],x0[1],ww,ff,zz,wz)*rdetB;
      FDV(fa2,f,1) += 
        INTEGR_W_Z_L1_L2(w2,w0,w1,f2,f0,f1,x2[1],x0[1],x1[1],ww,ff,zz,wz)*rdetB;
   }
}

#else

void integrate_rhs_vn_vf_xp2(tGrid,f,u)
GRID *tGrid; INT f, u;
{  eprintf("Error: integrate_rhs_vn_vf_xp2 not available.\n");  }

#endif

void integrate_rhs(tGrid,f,rhs0,rhs1,rhs2,t,type,space,structure,degree)
GRID *tGrid;
FLOAT (*rhs0)(), (*rhs1)(), (*rhs2)();
INT f, t, type, space, structure, degree;
{
   set_value(tGrid,0.,f,0,type);
   switch(space){
   case P1_NC:  if (structure == SCALAR && degree == 0 && DIM == 2)
                   integrate_rhs_sf_0(tGrid,f,rhs0);
                else if (structure == SCALAR && degree == 1 && DIM == 2)
                   integrate_rhs_sf_1(tGrid,f,rhs0);
                else if (structure == SCALAR && degree == 3 && DIM == 2)
                   integrate_rhs_sf_3(tGrid,f,rhs0);
                else if (structure == VECTOR && degree == 1 && DIM == 2)
                   integrate_rhs_vf_1(tGrid,f,rhs0,rhs1);
                else if (structure == VECTOR && degree == 3 && DIM == 2)
                   integrate_rhs_vf_3(tGrid,f,rhs0,rhs1);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case P1_MOD: if (structure == SCALAR && degree == 1 && DIM == 2)
                   scalar_p1mod_rhs_int_linear(tGrid,f,rhs0);
                else if (structure == SCALAR && degree == 3 && DIM == 2)
                   scalar_p1mod_rhs_int_cubic(tGrid,f,rhs0);
                else if (structure == VECTOR && degree == 1 && DIM == 2)
                   p1mod_rhs_int_linear(tGrid,f,rhs0,rhs1);
                else if (structure == VECTOR && degree == 3 && DIM == 2)
                   p1mod_rhs_int_cubic(tGrid,f,rhs0,rhs1);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case Q1ROT:  if (structure == SCALAR && degree == 3 && DIM == 2)
                   eprintf("Error: integrate_rhs not available.\n");      
                else if (structure == VECTOR && degree == 3 && DIM == 2)
                   q1rot_rhs(tGrid,f,rhs0,rhs1);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case P1C:    if (structure == SCALAR && USE_QUADRATURE == YES){
                   printf("rhs computed using quadrature\n");
                   integrate_p1c_rhs_quadr(tGrid,f,rhs0,RHS_Q_RULE);
                }
                else if (structure == SCALAR && degree == 0)
                   integrate_rhs_sn_0(tGrid,f,rhs0);
                else if (structure == SCALAR && degree == 1)
                   integrate_rhs_sn_1(tGrid,f,rhs0);
                else if (structure == SCALAR && degree == 2)
                   integrate_rhs_sn_2(tGrid,f,rhs0);
                else if (structure == SCALAR && degree == 3)
                   integrate_rhs_sn_3(tGrid,f,rhs0);
                else if (structure == VECTOR && degree == 1)
                   integrate_rhs_vn_1(tGrid,f,rhs0,rhs1);
                else if (structure == VECTOR && degree == 3)
                   integrate_rhs_vn_3(tGrid,f,rhs0,rhs1,0);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case P1C_FBUB: if (structure == VECTOR && degree == 1)
                   integr_rhs_p1c_fbub(tGrid,f,rhs0,rhs1);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case P1C_NEW_FBUB: if (structure == VECTOR && degree == 1)
                   integr_rhs_p1c_fbub(tGrid,f,rhs0,rhs1);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case P1C_ELBUB: if (USE_QUADRATURE == YES){
                   if (structure == SCALAR)
                      p1cb_integrate_rhs(tGrid,f,rhs0,0.,1.,2.,3.,4.,0,1,2,
                                         NULL,NULL,NULL,NULL,NULL,
                                         general_rhs_ref,p1cb_element,
                                         REF_MAP,RHS_Q_RULE);
                   else
                      eprintf("Error: integrate_rhs not available.\n");      
                }
                else if (structure == SCALAR && degree == 1){
                   integrate_rhs_sn_1(tGrid,f,rhs0);
                   integr_rhs_se_1(tGrid,f,rhs0);
                }
                else if (structure == VECTOR && degree == 1)
                   integr_rhs_vn_ve(tGrid,f,rhs0,rhs1);
                else if (structure == VECTOR && degree == 3)
                   integrate_rhs_vn_3(tGrid,f,rhs0,rhs1,1);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case MINI_L_DIV_FR: if (structure == VECTOR && degree == 1)
                   integrate_rhs_mini_lin_div_free_1(tGrid,f,rhs0,rhs1);
                else if (structure == VECTOR && degree == 3)
                   eprintf("Error: integrate_rhs not available.\n");      
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case Q1C:    if (structure == SCALAR && degree == 3)
                   integrate_rhs_q1(tGrid,f,rhs0,qs5);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case P2C:    if (structure == SCALAR && degree == 2)
                   integrate_rhs_sn_sf_2(tGrid,f,rhs0);
                else if (structure == SCALAR && degree == 3)
                   integrate_rhs_sn_sf_3(tGrid,f,rhs0);
                else if (structure == VECTOR && degree == 2 && DIM == 2)
                   integrate_rhs_vn_vf_2(tGrid,f,rhs0,rhs1,0);
                else if (structure == VECTOR && degree == 3 && DIM == 2)
                   integrate_rhs_vn_vf_3(tGrid,f,rhs0,rhs1,0);
                else if (structure == VECTOR && degree == Q_FORMULA && DIM == 2)
                   q_integrate_rhs_vn_vf(tGrid,f,rhs0,rhs1,0,RHS_Q_RULE);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case IP2C:   if (structure == SCALAR && degree == 3 && DIM == 2 && 
                                                          MOVING_BOUNDARY == NO)
                   integrate_rhs_sn_sf_3_iso(tGrid,f,rhs0);
                else if (structure == VECTOR && degree == 3 && DIM == 2 &&
                                                          MOVING_BOUNDARY == NO)
                   integrate_rhs_vn_vf_3_iso(tGrid,f,rhs0,rhs1,0);
                else if (structure == SCALAR && DIM == 2 && 
                                                         MOVING_BOUNDARY == YES)
                   integrate_rhs_sn_sf_2(tGrid,f,rhs0);
                else if (structure == VECTOR && DIM == 2 &&
                                                         MOVING_BOUNDARY == YES)
                   integrate_rhs_vn_vf(tGrid,f,rhs0,rhs1);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case P2C_ELBUB: if (structure == VECTOR && degree == 2 && DIM == 2)
                   integrate_rhs_vn_vf_2(tGrid,f,rhs0,rhs1,1);
                else if (structure == VECTOR && degree == 3 && DIM == 2)
                   integrate_rhs_vn_vf_3(tGrid,f,rhs0,rhs1,1);
                else if (structure == VECTOR && degree == Q_FORMULA && DIM == 2)
                   q_integrate_rhs_vn_vf(tGrid,f,rhs0,rhs1,1,RHS_Q_RULE);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case IP2C_ELBUB: if (structure == VECTOR && degree == 2 && DIM == 2)
                   eprintf("Error: integrate_rhs not available.\n");      
                else if (structure == VECTOR && degree == 3 && DIM == 2)
                   integrate_rhs_vn_vf_3_iso(tGrid,f,rhs0,rhs1,1);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case GP1C:
   case GP1X3C:
   case GP1C_ELBUB:
   case GQ1C:
   case GQ1X4C:
   case GQ1C_ELBUB:
   case GP2C:
   case GP2X3C:
   case GP2C_3ELBUB:
   case GP2C_6ELBUB:
   case GQ2C:
   case GQ2X4C:
   case GQ2C_2ELBUB:
   case GQ2C_3ELBUB:
                if (structure == SCALAR)
                   general_integrate_rhs(tGrid,f,rhs0,0.,1.,2.,3.,4.,0,1,2,
                                       NULL,NULL,NULL,NULL,NULL,
                                       general_rhs_ref,ELEM,REF_MAP,RHS_Q_RULE);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   case GQ2B3C:
                if (structure == SCALAR)
                   general_integrate_rhs_b(tGrid,f,rhs0,0.,1.,2.,3.,4.,0,1,2,
                                       NULL,NULL,NULL,NULL,NULL,
                                       general_rhs_ref,
                                       q2c_element,ELEM,REF_MAP,RHS_Q_RULE);
                else
                   eprintf("Error: integrate_rhs not available.\n");      
        break;
   default:
        eprintf("Error: integrate_rhs not available.\n");      
        break;
   }
}

void add_sd_rhs(tGrid,f,nu,rhs0,rhs1,rhs2,bb0,bb1,bb2,t,type,space,structure,degree)
GRID *tGrid;
FLOAT nu, (*rhs0)(), (*rhs1)(), (*rhs2)(), (*bb0)(), (*bb1)(), (*bb2)();
INT f, t, type, space, structure, degree;
{
   switch(space){
   case P1_NC:  if (structure == SCALAR && degree == 1 && DIM == 2)
                   scalar_p1nc_sd_to_rhs_linear(tGrid,f,nu,rhs0,bb0,bb1);
                else if (structure == SCALAR && degree == 3 && DIM == 2)
                   scalar_p1nc_sd_to_rhs_cubic(tGrid,f,nu,rhs0,bb0,bb1);
                else if (structure == VECTOR && degree == 3 && DIM == 2)
                   vect_p1nc_sd_to_rhs_cubic(tGrid,f,nu,rhs0,rhs1,bb0,bb1);
                else
                   eprintf("Error: add_sd_rhs not available.\n");      
        break;
   case P1_MOD: if (structure == SCALAR && degree == 3 && DIM == 2)
                   scalar_p1mod_sd_to_rhs_cubic(tGrid,f,nu,rhs0,bb0,bb1);
                else if (structure == VECTOR && degree == 3 && DIM == 2)
                   vect_p1mod_sd_to_rhs_cubic(tGrid,f,nu,rhs0,rhs1,bb0,bb1);
                else
                   eprintf("Error: add_sd_rhs not available.\n");      
        break;
   case P1C:    if (structure == SCALAR && USE_QUADRATURE == YES){
                   printf("sd rhs computed using quadrature\n");
                   add_p1c_sd_rhs_term_quadr(tGrid,f,bb0,bb1,zero_func,rhs0,nu,
                                      0,space,0,0.,0.,sd_tau,SDRHS_Q_RULE);
                }
                else if (structure == SCALAR && degree == 1)
                   add_sd_rhs_p1c_urbanek(tGrid, f, nu, rhs0, bb0, bb1);
                else if (structure == SCALAR && degree == 2)
                   add_sd_rhs_p1c_urbanek(tGrid, f, nu, rhs0, bb0, bb1);
                else if (structure == SCALAR && degree == 3){
                   if (TAU_SPACE == P0){
                      scalar_p1c_sd_to_rhs_cubic(tGrid,f,nu,rhs0,bb0,bb1);
                   }
                   else if (TAU_SPACE == P1C){
                      scalar_p1c_sdp1c_to_rhs_cubic(tGrid,f,nu,rhs0,bb0,bb1);
                   }
                   else if (TAU_SPACE == P1_NC){
                      scalar_p1c_sdp1nc_to_rhs_cubic(tGrid,f,nu,rhs0,bb0,bb1);
                   }
                   else
                      eprintf("Error: add_sd_rhs not available.\n");
                }   
                else
                   eprintf("Error: add_sd_rhs not available.\n");      
        break;
   case Q1C:    if (structure == SCALAR && degree == 3)
                   scalar_q1c_sd_to_rhs(tGrid,f,nu,rhs0,bb0,bb1,qs5);
                else
                   eprintf("Error: add_sd_rhs not available.\n");      
        break;
   case P2C:    if (structure == SCALAR && degree == 3){
                   if (TAU_SPACE == P0){
                      scalar_p2c_sd_to_rhs_cubic(tGrid,f,nu,rhs0,bb0,bb1);
                   }
                   else if (TAU_SPACE == P1C){
                      scalar_p2c_sdp1c_to_rhs_cubic(tGrid,f,nu,rhs0,bb0,bb1);
                   }
                   else if (TAU_SPACE == P1_NC){
                      scalar_p2c_sdp1nc_to_rhs_cubic(tGrid,f,nu,rhs0,bb0,bb1);
                   }
                   else
                      eprintf("Error: add_sd_rhs not available.\n");
                }
                else
                   eprintf("Error: add_sd_rhs not available.\n");      
        break;
   case GP1C:
   case GP1X3C:
   case GP1C_ELBUB:
   case GQ1C:
   case GQ1X4C:
   case GQ1C_ELBUB:
   case GP2C:
   case GP2X3C:
   case GP2C_3ELBUB:
   case GP2C_6ELBUB:
   case GQ2C:
   case GQ2X4C:
   case GQ2C_2ELBUB:
   case GQ2C_3ELBUB:
                if (structure == SCALAR)
                   general_integrate_rhs(tGrid,f,rhs0,nu,1.,2.,3.,4.,space,1,2,
                                  bb0,bb1,NULL,NULL,NULL,
                                  general_sd_rhs_ref,ELEM,REF_MAP,SDRHS_Q_RULE);
                else
                   eprintf("Error: add_sd_rhs not available.\n");      
        break;
   default:
        eprintf("Error: add_sd_rhs not available.\n");      
        break;
   }
}

void add_penalty_term_to_rhs(tGrid,f,u,space,par0)
GRID *tGrid;
FLOAT par0;
INT f, u, space;
{
   switch(space){
   case P1C:
   case Q1C:
                general_p1_q1_integrate_rhs(tGrid,f,NULL,par0,1.,2.,3.,4.,
                                       u,space,2,NULL,NULL,NULL,NULL,NULL,
                                       general_layton_polman_penalty_ref,
                                       ELEM,REF_MAP,RHS_Q_RULE);
        break;
   case GP1C:
   case GP1X3C:
   case GP1C_ELBUB:
   case GQ1C:
   case GQ1X4C:
   case GQ1C_ELBUB:
   case GP2C:
   case GP2X3C:
   case GP2C_3ELBUB:
   case GP2C_6ELBUB:
   case GQ2C:
   case GQ2X4C:
   case GQ2C_2ELBUB:
   case GQ2C_3ELBUB:
                general_integrate_rhs(tGrid,f,NULL,par0,1.,2.,3.,4.,
                                       u,space,2,NULL,NULL,NULL,NULL,NULL,
                                       general_layton_polman_penalty_ref,
                                       ELEM,REF_MAP,RHS_Q_RULE);
        break;
   default:
        eprintf("Error: add_penalty_term_to_rhs not available.\n");
        break;
   }
}

/******************************************************************************/

#if (E_DATA & ExDN_MATR) && (E_DATA & ExF_MATR) && (E_DATA & SCALAR_ELEMENT_DATA) && (N_DATA & VECTOR_NODE_DATA) && (F_DATA & SCALAR_FACE_DATA)

void set_Bi(pelement,pnode,n1,fan,nn,arn,i,u,br,b0,detB)
ELEMENT *pelement;
NODE *pnode, *n1;
FACE *fan; 
INT br, u, i;
FLOAT nn[DIM], arn, b0[DIM2], detB;
{
   test_normal_vector_direction(nn,pnode->myvertex->x,n1->myvertex->x,&arn);
   SET2(COEFF_BNP(pelement,0,i),b0,(-detB))
   COEFF_BF(pelement,0,i) = -arn/6.*FMULT;
   if (IS_DN(pnode))
      ED(pelement,br) += detB*DOT(b0,NDD(pnode,u));
   if (NOT_FF(fan))
      ED(pelement,br) += FD(fan,u)*arn/6.*FMULT;
}

#else

void set_Bi(pelement,pnode,n1,fan,nn,arn,i,u,br,b0,detB)
ELEMENT *pelement; NODE *pnode, *n1; FACE *fan; INT br, u, i; FLOAT nn[DIM], arn, b0[DIM2], detB;
{  eprintf("Error: set_Bi not available.\n");  }

#endif

#if (E_DATA & ExDN_MATR) && (E_DATA & ExF_MATR) && (E_DATA & SCALAR_ELEMENT_DATA) && (N_DATA & VECTOR_NODE_DATA)

void stiff_matr(mg,tGrid,nu,mult,Z,f,u,br)/* in case of nonuniform            */
MULTIGRID *mg;                            /* refinement, tGrid = TOP_GRID(mg) */
GRID *tGrid;
FLOAT nu, mult;
INT Z, f, u, br;
{
   GRID *theGrid;
   NODE *pnode, *n0, *n1, *n2;
   FACE *fa0, *fa1, *fa2;
   LINK *plink;
   ELEMENT *pelem;
   FLOAT nn0[DIM], nn1[DIM], nn2[DIM], ar0, ar1, ar2, rdetB, ndetB, 
         b[DIM2][DIM2], fmult2;

   fmult2 = FMULT2*mult;
    
   set_mat_value(tGrid,Z,0.,T_FOR_U,T_FOR_U,U_TYPE,U_TYPE,A_STRUCT);
   set_value(tGrid,0.,f,T_FOR_U,U_TYPE);
    
   for (pelem = FIRSTELEMENT(tGrid);pelem != NULL;pelem = pelem->succ){
      NODES_OF_ELEMENT(n0,n1,n2,pelem);
      FACES_OF_ELEMENT(fa0,fa1,fa2,pelem);
      rdetB = barycentric_coordinates(n0->myvertex->x,n1->myvertex->x,
                                                      n2->myvertex->x,b);
      ndetB = nu*rdetB;
      ar0 = normal_vector(n1->myvertex->x,n2->myvertex->x,fa0,nn0);
      ar1 = normal_vector(n0->myvertex->x,n2->myvertex->x,fa1,nn1);
      ar2 = normal_vector(n0->myvertex->x,n1->myvertex->x,fa2,nn2);
      aijb_2D(n0,n1,n2,fa0,fa1,fa2,nn0,nn1,nn2,Z,b[0],b[1],b[2],ndetB,fmult2);
      aijb_2D(n1,n2,n0,fa1,fa2,fa0,nn1,nn2,nn0,Z,b[1],b[2],b[0],ndetB,fmult2);
      aijb_2D(n2,n0,n1,fa2,fa0,fa1,nn2,nn0,nn1,Z,b[2],b[0],b[1],ndetB,fmult2);
      if (br > -1){
         ED(pelem,br) = 0.0;
         set_Bi(pelem,n0,n1,fa0,nn0,ar0,0,u,br,b[0],rdetB);
         set_Bi(pelem,n1,n2,fa1,nn1,ar1,1,u,br,b[1],rdetB);
         set_Bi(pelem,n2,n0,fa2,nn2,ar2,2,u,br,b[2],rdetB);
      }
   } 
   subtract_Acontribution_from_bc(tGrid,Z,u,f,Q,D,R,T_FOR_U,U_TYPE,A_STRUCT);
}

#else

void stiff_matr(mg,tGrid,nu,mult,Z,f,u,br)
MULTIGRID *mg; GRID *tGrid; FLOAT nu, mult; INT Z, f, u, br;
{  eprintf("Error: stiff_matr not available.\n");  }

#endif

#else   /* DIM == 3 */

FLOAT sd_delta(pelem,nu,bb0,bb1)
ELEMENT *pelem; FLOAT nu, (*bb0)(), (*bb1)();
{  eprintf("Error: sd_delta not available.\n"); return(0.);  }

#endif

