data {
  int<lower=0> num_collectors;        // The total number of route collectors
  int<lower=0> num_nodes; // The number of nodes in the AS network for all collectors
  int<lower=0> total_positive[num_collectors]; // The total number of positive observations for each route collector
  int<lower=0> total_negative[num_collectors]; // The total number of negative observatiosn for each route collector
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
  for (k in 1:num_collectors)
  {
    // a combinatorial factor, needed for numerical stabillity
    real comb = (num_nodes*(num_nodes - 1.0)/2.0*num_collectors);
    real updateA = log(rho)*comb;
    real updateB = log(1.0 - rho)*comb;
    updateA += log(rates_hi[k])*total_positive[k];
    updateA += log(1.0 - rates_hi[k])*total_negative[k];

    updateB += log(rates_lo[k])*total_positive[k];
    updateB += log(1.0 - rates_lo[k])*total_negative[k];
    print("-------");
    print(updateA);
    print(updateB);
    print(log_sum_exp(updateA, updateB));
    print(log(exp(updateA) + exp(updateB)));
    target += log_sum_exp(updateA, updateB);
    // target += log(exp(updateA) + exp(updateB));
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
