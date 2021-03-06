#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <math.h>

#include "problem.hpp"
#include "model.hpp"
#include "evaluator.hpp"
#include "solver/sgd.hpp"
#include "solver/pairsgd.hpp"
#include "solver/pairregsgd.hpp"
#include "solver/pairregssgd.hpp"


using namespace std;

struct configuration {
  std::string algo = "pairregssgd";
  std::string type_str = "numeric", train_comps_file, train_file, test_file;
  int rank = 50, n_threads = 1, max_iter = 10;
  double lambda = 1000, tol = 1e-5;
  double alpha, beta, gamma;
  bool evaluate_every_iter = true;
};

int readConf(struct configuration& conf, std::string conFile) {

  std::ifstream infile(conFile);
  std::string line;
  while(std::getline(infile,line))
  {
    std::istringstream iss(line);
    if ((line[0] == '#') || (line[0] == '[')) {
      continue;
    }
  
    std::string key, equal, val;
    if (iss >> key >> equal >> val) {
      if (equal != "=") {
        continue;
      }
      if (key == "type") {
        conf.type_str = val;
      }
      if (key == "train_file") {
        conf.train_comps_file = val;
      }
      if (key == "train_rating_file") {
        conf.train_file = val;
      }
      if (key == "test_file") {
        conf.test_file = val;
      }
      if (key == "algorithm") {
        conf.algo = val;
      }
      if (key == "lambda") {
        conf.lambda = std::stod(val);
      }
      if (key == "gamma") {
      	conf.gamma = std::stod(val);
      }
      if (key == "rank") {
        conf.rank = std::stoi(val);
      }
      if (key == "max_outer_iter") {
        conf.max_iter = std::stoi(val);
      }
      if (key == "tol") {
        conf.tol = std::stod(val);
      }
      if (key == "evaluate") {
        if (val == "true") conf.evaluate_every_iter = true;
        if (val == "false") conf.evaluate_every_iter = false;
      }
      if (key == "nthreads") {
        conf.n_threads = std::stoi(val);
      }
      if (key == "stepsize_alpha") {
        conf.alpha = std::stod(val);
      }
      if (key == "stepsize_beta") {
        conf.beta = std::stod(val);
      }
    }
  }

  return 1;
}

int main (int argc, char* argv[]) {
  struct configuration conf;
  std::string config_file = "config/default.cfg";

  if (argc > 2) {
    std::cerr << "Usage : " << std::string(argv[0]) << " [config_file]" << std::endl;
    return -1;
  }

  if (argc == 2) {
    config_file = std::string(argv[1]);
  }

  if (!readConf(conf, config_file)) {
    std::cerr << "Usage : " << std::string(argv[0]) << " [config_file]" << std::endl;
    return -1;
  }

  // Problem definition 
  Problem prob;

  if ((conf.type_str != "numeric") && (conf.type_str != "binary")) {
    cerr << "ERROR : provide correct experiment type !\n";
    return 1;
  }

  prob.lambda = conf.lambda;

  std::cout << "Loading training set file : " << conf.train_comps_file << std::endl;
  prob.read_data(conf.train_comps_file);

  int n_users_train = prob.get_nusers();  // get the number of users in training
  int n_items_train = prob.get_nitems();  // get the number of items in the testing

  // Evaluator definition
  Evaluator* eval;
  
  vector<int> k_list;
  
  if (conf.type_str == "numeric") {
    eval = new EvaluatorRating;
    // current only ndcg@10 can be computed 
    k_list.push_back(10);
  }

  std::cout << "Reading test set file : " << conf.test_file << std::endl;
  eval->load_files(conf.train_file, conf.test_file, k_list);
  	 
  int n_users_test = eval->get_nusers();
  int n_items_test = eval->get_nitems();

  int n_users = max(n_users_train, n_users_test);
  int n_items = max(n_items_train, n_items_test);

  Model model(n_users, n_items, conf.rank);
  // Solver definition
  omp_set_dynamic(0);
  omp_set_num_threads(conf.n_threads);

  if(conf.algo == "pairregssgd"){
	for(int i = 0; i < 1; ++i){
		cout << "**************n_threads="<<conf.n_threads<<"**************"<<endl;
		Solver* mySolver = new SolverPairRegSSGD(conf.alpha, conf.beta, conf.gamma, INIT_RANDOM, conf.n_threads, conf.max_iter);
		mySolver->solve(prob, model, eval);		
		delete mySolver;
		cout<<endl;
	}
  }
  else if(conf.algo == "pairregsgd"){
	for(int i = 0; i < 1; ++i){
		cout << "**************n_threads="<<conf.n_threads<<"**************"<<endl;
		Solver* mySolver = new SolverPairRegSGD(conf.alpha, conf.beta, conf.gamma, INIT_RANDOM, conf.n_threads, conf.max_iter);
		mySolver->solve(prob, model, eval);		
		delete mySolver;
		cout<<endl;
	}
  }else if(conf.algo == "pairsgd"){
	printf("Pairwise ranking with all-aggregated comparisons.. \n");
	printf("iteration, training time (sec), pairwise error, ndcg@10\n");		
	double step_alpha[4] = {0.0001, 0.001, 0.01,0.1};
	 	
	for (int idx = 0; idx < 4; idx++){
		Solver* mySolver2;
		cout << "-----------------Step_size:" <<step_alpha[idx]<<"  Regularization:"<<conf.lambda << "---------------------"<<endl;
		mySolver2 = new SolverPairSGD(conf.alpha, conf.beta, INIT_RANDOM, conf.n_threads, conf.max_iter);
		mySolver2->solve(prob, model, eval);
		delete mySolver2;
	}
  }
  else {
    std::cerr << "ERROR : provide correct algorithm !\n";
    return -1;
  } 
  return 0;
}
