/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h>
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of
	//   x, y, theta and their uncertainties from GPS) and all weights to 1.
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).
  num_particles = 10;
  normal_distribution<double> dist_x(x, std[0]);
  normal_distribution<double> dist_y(y, std[1]);
  normal_distribution<double> dist_theta(theta, std[2]);

  default_random_engine gen;

  for (int i=0; i<num_particles; i++) {
    double sample_x, sample_y, sample_theta;
    sample_x = dist_x(gen);
    sample_y = dist_y(gen);
    sample_theta = dist_theta(gen);

    Particle particle = Particle();
    particle.id = i;
    particle.x = sample_x;
    particle.y = sample_y;
    particle.theta = sample_theta;
    particle.weight = 1;

    particles.push_back(particle);
  }

}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/

  normal_distribution<double> dist_x(0, std_pos[0]);
  normal_distribution<double> dist_y(0, std_pos[1]);
  normal_distribution<double> dist_theta(0, std_pos[2]);

  default_random_engine gen;

  for (int i=0; i<num_particles; i++) {
    Particle particle = particles[i];
    particle.x = particle.x + (velocity / yaw_rate) * (sin(particle.theta + yaw_rate*delta_t) - sin(particle.theta)) + dist_x(gen);
    particle.y = particle.y + (velocity / yaw_rate) * (cos(particle.theta) - cos(particle.theta + yaw_rate*delta_t)) + dist_y(gen);
    particle.theta = particle.theta + yaw_rate * delta_t + dist_theta(gen);
  }

}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to
	//   implement this method and use it as a helper during the updateWeights phase.
  for (int i=0; i<observations.size(); i++) {
    LandmarkObs observation = observations[i];
    double max_dist = numeric_limits<double>::infinity();
    for (int j=0; j<predicted.size(); j++) {
      LandmarkObs landmark = predicted[j];
      double distance = dist(observation.x, observation.y, landmark.x, landmark.y);
      if (distance < max_dist) {
        max_dist = distance;
        observation.id = landmark.id;
      }
    }
  }
}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[],
		const std::vector<LandmarkObs> &observations, const Map &map_landmarks) {
	// TODO: Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html

  vector<LandmarkObs> landmarks;
  for (int i=0; i<map_landmarks.landmark_list.size(); i++) {
    LandmarkObs landmark = LandmarkObs();
    landmark.id = map_landmarks.landmark_list[i].id_i;
    landmark.x = map_landmarks.landmark_list[i].x_f;
    landmark.y = map_landmarks.landmark_list[i].y_f;
  }

  for (int i=0; i<particles.size(); i++) {
    Particle particle = particles[i];
    vector<LandmarkObs> transformed_observations;
    for (int j=0; j<observations.size(); j++) {
      LandmarkObs observation = observations[j];
      LandmarkObs transformed_observation = LandmarkObs();
      transformed_observation.x = particle.x + cos(particle.theta)*observation.x - sin(particle.theta)*observation.y;
      transformed_observation.y = particle.y + sin(particle.theta)*observation.x + cos(particle.theta)*observation.y;
      transformed_observations.push_back(transformed_observation);
    }
    dataAssociation(landmarks, transformed_observations);
    double prob = 1.0;
    for (int j=0; j<transformed_observations.size(); j++) {
      LandmarkObs transformed_observation = transformed_observations[j];
      LandmarkObs landmark;
      for (int k=0; k<landmarks.size(); k++) {
        if (landmarks[k].id ==  transformed_observation.id) {
          landmark = landmarks[k];
        }
      }
      double a = (transformed_observation.x - landmark.x)*(transformed_observation.x - landmark.x) / (2 * std_landmark[0] * std_landmark[0]);
      double b = (transformed_observation.y - landmark.y)*(transformed_observation.y - landmark.y) / (2 * std_landmark[1] * std_landmark[1]);
      double mvn = exp(-(a + b)) / (2 * M_PI * std_landmark[0] * std_landmark[1]);
      prob = prob*mvn;
    }
    particle.weight = prob;
  }

  double sum = 0;
  for (int i=0; i<particles.size(); i++) {
    sum += particles[i].weight;
  }
  for (int i=0; i<particles.size(); i++) {
    particles[i].weight = particles[i].weight / sum;
  }

}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight.
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
  default_random_engine gen;
  weights = vector<double>();
  for (int i=0; i<particles.size(); i++) {
    weights.push_back(particles[i].weight);
  }
  discrete_distribution<int> dist(weights.begin(), weights.end());
  vector<Particle> resampled_particles;
  for (int i=0; i<num_particles; i++) {
    resampled_particles.push_back(particles[dist(gen)]);
  }
  particles = resampled_particles;

}

Particle ParticleFilter::SetAssociations(Particle& particle, const std::vector<int>& associations,
                                     const std::vector<double>& sense_x, const std::vector<double>& sense_y)
{
    //particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
    // associations: The landmark id that goes along with each listed association
    // sense_x: the associations x mapping already converted to world coordinates
    // sense_y: the associations y mapping already converted to world coordinates

    particle.associations= associations;
    particle.sense_x = sense_x;
    particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
