#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "timer.h"


#define SOFTENING 1e-9f

typedef struct { float *x, *y, *z, *vx, *vy, *vz; } BodySystem;

void randomizeBodies(float *data, int n) {
  for (int i = 0; i < n; i++) {
    data[i] = 2.0f * (rand() / (float)RAND_MAX) - 1.0f;
  }
}


void bodyForce(BodySystem p, float dt, int n) {
  #pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < n; i++) { 
    float Fx = 0.0f; float Fy = 0.0f; float Fz = 0.0f;

    for (int j = 0; j < n; j++) {
      float dy = p.y[j] - p.y[i];
      float dz = p.z[j] - p.z[i];
      float dx = p.x[j] - p.x[i];
      float distSqr = dx*dx + dy*dy + dz*dz + SOFTENING;
      float invDist = 1.0f / sqrtf(distSqr);
      float invDist3 = invDist * invDist * invDist;

      Fx += dx * invDist3; Fy += dy * invDist3; Fz += dz * invDist3;
    }

    p.vx[i] += dt*Fx; p.vy[i] += dt*Fy; p.vz[i] += dt*Fz;
  }
}

int main(const int argc, const char** argv) {
  
  int nBodies = 32768;
  if (argc > 1) nBodies = atoi(argv[1]);

  const float dt = 0.01f; // time step
  const int nIters = 10;  // simulation iterations

  int bytes = 6*nBodies*sizeof(float);
  float *buf = (float*)malloc(bytes);
  BodySystem p;
  p.x  = buf+0*nBodies; p.y  = buf+1*nBodies; p.z  = buf+2*nBodies;
  p.vx = buf+3*nBodies; p.vy = buf+4*nBodies; p.vz = buf+5*nBodies;

  randomizeBodies(buf, 6*nBodies); // Init pos / vel data

  double totalTime = 0.0;

  for (int iter = 1; iter <= nIters; iter++) {
    StartTimer();

    bodyForce(p, dt, nBodies); // compute interbody forces

    for (int i = 0 ; i < nBodies; i++) { // integrate position
      p.x[i] += p.vx[i]*dt;
      p.y[i] += p.vy[i]*dt;
      p.z[i] += p.vz[i]*dt;
    }

    const double tElapsed = GetTimer() / 1000.0;
    if (iter > 1) { // First iter is warm up
      totalTime += tElapsed; 
    }
#ifndef SHMOO
    printf("Iteration %d: %.3f seconds\n", iter, tElapsed);
#endif
  }
  double avgTime = totalTime / (double)(nIters-1); 

#ifdef SHMOO
  printf("%d, %0.3f\n", nBodies, 1e-9 * nBodies * nBodies / avgTime);
#else
  printf("Average rate for iterations 2 through %d: %.3f steps per second.\n",
         nIters);
  printf("%d Bodies: average %0.3f Billion Interactions / second\n", nBodies, 1e-9 * nBodies * nBodies / avgTime);
#endif
  free(buf);
}
