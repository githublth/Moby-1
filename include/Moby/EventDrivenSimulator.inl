/****************************************************************************
 * Copyright 2005 Evan Drumwright
 * This library is distributed under the terms of the Apache V2.0 
 * License (obtainable from http://www.apache.org/licenses/LICENSE-2.0).
 ****************************************************************************/

/// Integrates both position and velocity of rigid _bodies
/**
 * \return the size of step taken
 */
template <class ForwardIterator>
double EventDrivenSimulator::integrate_with_sustained_constraints(double step_size, ForwardIterator begin, ForwardIterator end)
{
  // get the simulator pointer
  boost::shared_ptr<Simulator> shared_this = boost::dynamic_pointer_cast<Simulator>(shared_from_this());

  // get the state size
  unsigned state_sz = 0;
  for (ForwardIterator i = begin; i != end; i++)
  {
    // update the state size
    state_sz += (*i)->num_generalized_coordinates(DynamicBody::eEuler);
    state_sz += (*i)->num_generalized_coordinates(DynamicBody::eSpatial);
  }

  // init x and work vectors
  Ravelin::VectorNd x(state_sz);

  // get the current generalized coordinates and velocity for each body
  unsigned idx = 0;
  for (ForwardIterator i = begin; i != end; i++)
  {
    // get number of generalized coordinates and velocities
    const unsigned NGC = (*i)->num_generalized_coordinates(DynamicBody::eEuler);
    const unsigned NGV = (*i)->num_generalized_coordinates(DynamicBody::eSpatial);

    // get the shared vectors
    Ravelin::SharedVectorNd xgc = x.segment(idx, idx+NGC); idx += NGC;
    Ravelin::SharedVectorNd xgv = x.segment(idx, idx+NGV); idx += NGV;
    (*i)->get_generalized_coordinates(DynamicBody::eEuler, xgc);
    (*i)->get_generalized_velocity(DynamicBody::eSpatial, xgv);
  }

  // call the integrator
  integrator->integrate(x, &ode_sustained_constraints, current_time, step_size, (void*) &shared_this);

  // update the generalized coordinates and velocity
  idx = 0;
  for (ForwardIterator i = begin; i != end; i++)
  {
    // get number of generalized coordinates and velocities
    const unsigned NGC = (*i)->num_generalized_coordinates(DynamicBody::eEuler);
    const unsigned NGV = (*i)->num_generalized_coordinates(DynamicBody::eSpatial);

    // get the shared vectors
    Ravelin::SharedConstVectorNd xgc = x.segment(idx, idx+NGC); idx += NGC;
    Ravelin::SharedConstVectorNd xgv = x.segment(idx, idx+NGV); idx += NGV;

    // set the generalized coordinates and velocity
    (*i)->set_generalized_coordinates(DynamicBody::eEuler, xgc);
    (*i)->set_generalized_velocity(DynamicBody::eSpatial, xgv);
  }

  return step_size;
}

