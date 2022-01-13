data {
  // Number of observation classes
  int<lower=0> num_classes;

  // Number of Route Collectors
  int<lower=0> num_collectors;

  //class_counts[i] -> the number of AS pairs with the ith observation class
  int<lower=0> class_counts[num_classes];

  //E[i,k] -> The number of positive observations seen by the ith class in the kth collector 
  int<lower=0> E[num_classes, num_collectors];

  //F[i,k] -> The number of negative observations seen by the ith class in the kth collector 
  int<lower=0> F[num_classes, num_collectors];
}

parameters {
  ordered[2] rates[num_collectors];  // pair of rates for each mode, ordered for identifiability
  real<lower=0,upper=1> rho;   // global density, no need for logit space here
}
transformed parameters {
  vector[num_collectors] rates_lo;
  vector[num_collectors] rates_hi;
  for (i in 1:num_collectors)
  {
    rates_lo[i] = inv_logit(rates[i, 1]);
    rates_hi[i] = inv_logit(rates[i, 2]);
  }
}
model {
  for (i in 1:num_classes)
  {
    real L_ij_a = bernoulli_lpmf(1| rho);
    real L_ij_b = bernoulli_lpmf(0| rho);
    for (k in 1:num_collectors)  // loop over modes
    {
      L_ij_a += binomial_lpmf(E[i,k] | E[i,k] + F[i,k], rates_hi[k]);
      L_ij_b += binomial_lpmf(E[i,k] | E[i,k] + F[i,k], rates_lo[k]);
    }
    target += class_counts[i]* log_sum_exp(L_ij_a, L_ij_b);
  }
}
/*generated quantities {
  // Posterior edge probability matrix
  real <lower=0, upper=1> Q[numClasses];
  for (i in 1:numClasses)
  {
    real nu_a = log(rho);
    real nu_b = log(1 - rho);
    for (m in 1:numModes)
    {
      nu_a += counts[i,m] * log(rates_hi[m]) + (trials[i,m] -  counts[i,m]) * log(1 - rates_hi[m]);
      nu_b += counts[i,m] * log(rates_lo[m]) + (trials[i,m] -  counts[i,m]) * log(1 - rates_lo[m]);
    }
    if (nu_a > 0)
      Q[i] = 1 / (1 + exp(nu_b - nu_a));
    else
      Q[i] = exp(nu_a) / (exp(nu_b) + exp(nu_a));
  }
}*/
