#include "em.hpp"

vd calc_Q(vd alphas,
		  vd betas,
		  double rho,
		  ObservationDataset* dataset,
		  int num_random_collectors,
		std::vector<int> rc_idxs) {

	std::vector<double> Q;

	for(int i = 0; i < dataset->num_classes; i++) {

		double v_a = log(rho);
		double v_b = log(1 - rho);

		for(int kk = 0; kk < num_random_collectors; kk++) {

			int k = rc_idxs[kk];

			double a = alphas[kk];
			double b = betas[kk];
			int E = dataset->records[i]->E[k];
			int F = dataset->records[i]->F[k];
			// note: We need to not let collectors with missing data
			// impact our results
			if(!(isnan(a) || isnan(b))) {
				v_a += E*log(a) + F*log(1 - a);
				v_b += E*log(b) + F*log(1 - b);
			}
		}

		double result = 1.0/(1.0 + exp(v_b - v_a));
		Q.push_back(result);
	}

	return Q;
}

vd calc_alpha_numerator(vd Q, ObservationDataset* dataset,
						int num_random_collectors,
						std::vector<int> rc_idxs) {
	std::vector<double> alpha_numerator (num_random_collectors);
	std::fill(alpha_numerator.begin(), alpha_numerator.end(), 0.0);

	for(int kk = 0; kk < num_random_collectors; kk++) {
		int k = rc_idxs[kk];
		double partial = 0.0;
		for(int i = 0; i < dataset->num_classes; i++) {
			partial += (dataset->records[i]->count)*
						(dataset->records[i]->E[k])*Q[i];
		}
		alpha_numerator[kk] = partial;
	}
	return alpha_numerator;
}

vd calc_alpha_denominator(vd Q, ObservationDataset* dataset,
						int num_random_collectors,
						std::vector<int> rc_idxs) {

	std::vector<double> alpha_denominator (num_random_collectors);
	std::fill(alpha_denominator.begin(), alpha_denominator.end(), 0.0);

	for(int kk = 0; kk < num_random_collectors; kk++) {
		int k = rc_idxs[kk];
		double partial = 0.0;
		for(int i = 0; i < dataset->num_classes; i++) {
			partial += (dataset->records[i]->count)*
						(dataset->records[i]->E[k] + dataset->records[i]->F[k])*Q[i];
		}
		alpha_denominator[kk] = partial;
	}

	return alpha_denominator;
}

vd calc_beta_numerator(vd Q, ObservationDataset* dataset,
						int num_random_collectors,
						std::vector<int> rc_idxs) {

	std::vector<double> beta_numerator (num_random_collectors);
	std::fill(beta_numerator.begin(), beta_numerator.end(), 0.0);

	for(int kk = 0; kk < num_random_collectors; kk++) {
		int k = rc_idxs[kk];
		double partial = 0.0;
		for(int i = 0; i < dataset->num_classes; i++) {
			partial += (dataset->records[i]->count)*
						(dataset->records[i]->E[k])*(1.0 - Q[i]);
		}
		beta_numerator[kk] = partial;
	}
	return beta_numerator;
}

vd calc_beta_denominator(vd Q, ObservationDataset* dataset,
						int num_random_collectors,
						std::vector<int> rc_idxs) {

	std::vector<double> beta_denominator (num_random_collectors);
	std::fill(beta_denominator.begin(), beta_denominator.end(), 0.0);

	for(int kk = 0; kk < num_random_collectors; kk++) {
		int k = rc_idxs[kk];
		double partial = 0.0;
		for(int i = 0; i < dataset->num_classes; i++) {
			partial += (dataset->records[i]->count)*
						(dataset->records[i]->E[k] + dataset->records[i]->F[k])*(1.0 - Q[i]);
		}
		beta_denominator[kk] = partial;
	}

	return beta_denominator;
}

double calc_rho_term(vd Q) {
	double value = 0.0;
	for(int i = 0; i < Q.size(); i++) {
		value += Q[i];
	}
	return value;
}

double calc_D(vd Q, ObservationDataset* dataset, double rho) {
	double value = 0.0;
	double eps = 10e-8;
	for(int i = 0; i < Q.size(); i++) {
		double C = (double)dataset->records[i]->count;
		double Q_val = Q[i];
		if(Q_val < eps) {
			value += C*( (1 - Q[i])*log( (1-Q[i]) / (1-rho)) );
		}
		else if((1.0 - Q_val) < eps) {
			value += C*(Q[i]*log(Q[i]/rho));
		}
		else {
			value += C*(Q[i]*log(Q[i]/rho) + (1 - Q[i])*log( (1-Q[i]) / (1-rho)));
		}
	}
	return value;
}

double calc_H(vd Q, ObservationDataset* dataset, double rho) {
	double value = 0.0;
	double eps = 10e-8;
	double denom = (rho*log(rho)) + ((1-rho)*log(1-rho));
	for(int i = 0; i < Q.size(); i++) {
		double C = (double)dataset->records[i]->count;
		double Q_val = Q[i];
		if(Q_val < eps) {
			value += C*(( (1.0 - Q_val)*(log(1 - Q_val)) )/( denom ));
		}
		else if((1.0 - Q_val) < eps) {
			value += C*((Q_val*log(Q_val))/denom);
		}
		else {
			value += C*(((Q_val*log(Q_val)) + ( (1.0 - Q_val)*(log(1 - Q_val)) ))/( denom ));
		}
	}
	return value;
}