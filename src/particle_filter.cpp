/**
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <limits>

#include "helper_functions.h"

using std::string;
using std::vector;

using std::normal_distribution;
using std::numeric_limits;
using std::random_device;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * TODO: Set the number of particles. Initialize all particles to 
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1. 
   * TODO: Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method 
   *   (and others in this file).
   */
  num_particles = 100;  // TODO: Set the number of particles
  
  //Random generator
  std::default_random_engine gen;
  
  //Get the standard deviations
  double std_x, std_y, std_angle;
  std_x = std[0];
  std_y = std[1];
  std_angle = std[2]; 
  
  //Creates the normal (Gaussian) distributions
  normal_distribution<double> dist_x(x, std_x);
  normal_distribution<double> dist_y(y, std_y);
  normal_distribution<double> dist_angle(theta, std_angle);
  
  for (int i = 0; i < num_particles; ++i) {
    // Create a new particle
  	Particle newP;
  	newP.weight = 1.0;
    newP.id = i;
    newP.x = dist_x(gen);
    newP.y = dist_y(gen);
    newP.theta = dist_angle(gen);
    
    // Push-back the new particle in the particles vector
    particles.push_back (newP);
  }
  
  // set the is_initialized attribute to true
  is_initialized = true;

}

void ParticleFilter::prediction(double delta_t, double std_pos[], 
                                double velocity, double yaw_rate) {
  /**
   * TODO: Add measurements to each particle and add random Gaussian noise.
   * NOTE: When adding noise you may find std::normal_distribution 
   *   and std::default_random_engine useful.
   *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
   *  http://www.cplusplus.com/reference/random/default_random_engine/
   */
  
  //Random generator
  std::default_random_engine gen;
  
  // For each particle, calculates the predict step and add Gaussian noise
  for (int i = 0; i < particles.size(); ++i) {
    
    double oldTh = particles[i].theta;
    double new_theta = particles[i].theta + yaw_rate*delta_t;
    double v_y;
    // check division by zero
    if (fabs(yaw_rate) < 0.0001) {
      double v_y = 0;
    }
    else{
      double v_y = velocity/yaw_rate;
    }
    
    // Calculate the new predicted states
    double new_x = particles[i].x + v_y*(sin(new_theta) - sin(oldTh));
    double new_y = particles[i].y + v_y*(cos(oldTh) - cos(new_theta));
    //new_theta
    
    //Creates the normal (Gaussian) distributions
    normal_distribution<double> dist_x(new_x, std_pos[0]);
    normal_distribution<double> dist_y(new_y, std_pos[1]);
    normal_distribution<double> dist_angle(new_theta, std_pos[2]);
    
    // Update the particle state
    particles[i].x = dist_x(gen);
    particles[i].y = dist_y(gen);
    particles[i].theta = dist_angle(gen);
     
  }

}

void ParticleFilter::dataAssociation(vector<LandmarkObs> predicted, 
                                     vector<LandmarkObs>& observations) {
  /**
   * TODO: Find the predicted measurement that is closest to each 
   *   observed measurement and assign the observed measurement to this 
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will 
   *   probably find it useful to implement this method and use it as a helper 
   *   during the updateWeights phase.
   */
  
  // Associate each observation with the closest landmark using nearest neighbor
  for (int i = 0; i < observations.size(); i++) {
    
    double min = numeric_limits<double>::max();
    
    for (int j = 0; j < predicted.size(); j++) {
      
       // if the distance between the observation and the landmark is tne minimum
       // update the association pair
       double dist_op = dist(observations[i].x, observations[i].y, 
                             predicted[j].x, predicted[j].y);
       if(dist_op < min){
         min = dist_op;
         observations[i].id = predicted[j].id;
       }
    }
  }
}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
                                   const vector<LandmarkObs> &observations, 
                                   const Map &map_landmarks) {
  /**
   * TODO: Update the weights of each particle using a mult-variate Gaussian 
   *   distribution. You can read more about this distribution here: 
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system. 
   *   Your particles are located according to the MAP'S coordinate system. 
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */
  
  // for each particle
  for (int i = 0; i < num_particles; i++) {
    
    //Get the particle state (position - heading)
    double p_x = particles[i].x;
    double p_y = particles[i].y;
	double p_theta = particles[i].theta;
    
    // create a LandmarkObs vector for the map landmark locations within sensor range of the particle
    vector<LandmarkObs> predictions;

    // for each map landmark
    for (int j = 0; j < map_landmarks.landmark_list.size(); j++) {
      
      // get id and x,y coordinates
      float lm_x = map_landmarks.landmark_list[j].x_f;
      float lm_y = map_landmarks.landmark_list[j].y_f;
      int lm_id = map_landmarks.landmark_list[j].id_i;
      
      // only consider landmarks within sensor range of the particle
      if (dist(p_x, p_y, lm_x, lm_y) <= sensor_range) {
        // add prediction to vector
        predictions.push_back(LandmarkObs{ lm_id, lm_x, lm_y });
      }
    }

    // create and populate a LandmarkObs vector of the observations in map coordinates
    vector<LandmarkObs> map_obs;
    for (int j = 0; j < observations.size(); j++) {
      double map_x = cos(p_theta)*observations[j].x - sin(p_theta)*observations[j].y + p_x;
      double map_y = sin(p_theta)*observations[j].x + cos(p_theta)*observations[j].y + p_y;
      map_obs.push_back(LandmarkObs{ observations[j].id, map_x, map_y });
    }
    
    // Performs data association between the prediction and transformated observations of the particle
    dataAssociation(predictions, map_obs);   
    
    //init associations vectors
    vector<int> associations;
    vector<double> sense_x;
    vector<double> sense_y;
    
    //reinit weight
    particles[i].weight = 1.0;
    
    //set common components of the multivariate Gaussian distribution
    double std_2_pi = 2.0*M_PI*std_landmark[0]*std_landmark[1];
    double std_x_2  = 2.0*std_landmark[0]*std_landmark[0];
    double std_y_2  = 2.0*std_landmark[1]*std_landmark[1];
    
    // for each transformated observations
    for (int k = 0; k < map_obs.size(); k++) {
      //actual transformated observation (map coordinates)
      LandmarkObs o = map_obs[k];
      
      //recover associated landmark
      Map::single_landmark_s m =  map_landmarks.landmark_list.at(o.id-1);
      
      //compute Multivariate Gaussian probability
      double c1 = pow(o.x - m.x_f, 2);
      double c2 = pow(o.y - m.y_f, 2);
      
      double c3 = (c1/std_x_2 + c2/std_y_2);
      double c_e = exp(-c3);
      
      double w = c_e/std_2_pi;
      
      //product of all the weights
      particles[i].weight *= w;
      
      //add values to associations vectors
      associations.push_back(o.id);
      sense_x.push_back(o.x);
      sense_y.push_back(o.y);      
    }

    //update particle associations
    SetAssociations(particles[i], associations, sense_x, sense_y); 
  }

}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional 
   *   to their weight. 
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */
  
  //new (temporal) particles set
  vector<Particle> new_particles;

  // Get a vector with all the current weights.
  vector<double> weights;
  for (int i = 0; i < num_particles; i++) {
     weights.push_back(particles[i].weight);
  }

  //create discrete distibution from particles weights.
  random_device rd;  
  std::mt19937 gen(rd());
  std::discrete_distribution<> d(weights.begin(), weights.end());
  
  //Resample.
  for(int i=0; i < num_particles; i++){      
    int index = d(gen);  
    Particle new_p = particles[index];
    new_particles.push_back(new_p);
  }
  particles.clear();
  weights.clear();
  
  particles = new_particles;
}

void ParticleFilter::SetAssociations(Particle& particle, 
                                     const vector<int>& associations, 
                                     const vector<double>& sense_x, 
                                     const vector<double>& sense_y) {
  // particle: the particle to which assign each listed association, 
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates
  particle.associations= associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}