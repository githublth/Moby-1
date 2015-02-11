/*****************************************************************************
 * "Controller" for constrained pendulum example 
 ****************************************************************************/
#include <Moby/EventDrivenSimulator.h>
#include <Moby/RCArticulatedBody.h>
#include <Moby/GravityForce.h>
#include <Ravelin/Pose3d.h>
#include <Ravelin/Vector3d.h>
#include <Ravelin/VectorNd.h>
#include <fstream>
#include <stdlib.h>

using boost::shared_ptr;
using namespace Ravelin;
using namespace Moby;

Moby::RigidBodyPtr l1;
boost::shared_ptr<EventDrivenSimulator> sim;
boost::shared_ptr<GravityForce> grav;

// setup simulator callback
void post_step_callback(Simulator* sim)
{
  const unsigned Z = 2;

  // output the energy of the link
  std::ofstream out("energy.dat", std::ostream::app);
  Transform3d gTw = Pose3d::calc_relative_pose(l1->get_inertial_pose(), GLOBAL);
  double KE = l1->calc_kinetic_energy();
  double PE = l1->get_inertia().m*gTw.x[Z]*-grav->gravity[Z];
  out << KE << " " << PE << " " << (KE+PE) << std::endl;
  out.close();

  // output the position of l1
  std::cout << sim->current_time << " " << gTw.x[0] << " " << gTw.x[1] << " " << gTw.x[2] << std::endl;

  // 
}

/// plugin must be "extern C"
extern "C" {

void init(void* separator, const std::map<std::string, Moby::BasePtr>& read_map, double time)
{
  const unsigned Z = 2;

  // get a reference to the EventDrivenSimulator instance
  for (std::map<std::string, Moby::BasePtr>::const_iterator i = read_map.begin();
       i !=read_map.end(); i++)
  {
    // Find the simulator reference
    if (!sim)
      sim = boost::dynamic_pointer_cast<EventDrivenSimulator>(i->second);
    if (i->first == "l1")
      l1 = boost::dynamic_pointer_cast<RigidBody>(i->second);
    if (!grav)
      grav = boost::dynamic_pointer_cast<GravityForce>(i->second);
  }

  sim->post_step_callback_fn = &post_step_callback;
}
} // end extern C
