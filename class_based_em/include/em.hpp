#ifndef EM_H
#define EM_H
#include "record.hpp"
#include <math.h>
#include <vector>
#include <algorithm>
#include <random>
#endif

using vd = std::vector<double>;

vd calc_Q(vd alphas,
		  vd betas,
		  double rho,
		  ObservationDataset* dataset,
		  int num_random_collectors,
			std::vector<int> rc_idxs);

vd calc_alpha_numerator(vd Q, ObservationDataset* dataset,
						int num_random_collectors, std::vector<int> rc_idxs);

vd calc_alpha_denominator(vd Q, ObservationDataset* dataset,
						int num_random_collectors, std::vector<int> rc_idxs);

vd calc_beta_numerator(vd Q, ObservationDataset* dataset,
						int num_random_collectors, std::vector<int> rc_idxs);

vd calc_beta_denominator(vd Q, ObservationDataset* dataset,
						int num_random_collectors, std::vector<int> rc_idxs);

double calc_rho_term(vd Q);
double calc_D(vd Q, ObservationDataset* dataset, double rho);
double calc_H(vd Q, ObservationDataset* dataset, double rho);