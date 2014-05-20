/****************************************************************************
 * Copyright 2014 Evan Drumwright
 * This library is distributed under the terms of the GNU Lesser General Public 
 * License (found in COPYING).
 ****************************************************************************/

#ifdef USE_OSG
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/LightModel>
#endif
#include <boost/algorithm/minmax_element.hpp>
#include <Moby/Constants.h>
#include <Moby/CompGeom.h>
#include <Moby/sorted_pair>
#include <Moby/XMLTree.h>
#include <Moby/BoundingSphere.h>
#include <Moby/CollisionGeometry.h>
#include <Moby/SpherePrimitive.h>
#include <Moby/GJK.h>
#include <Moby/PlanePrimitive.h>

using namespace Ravelin;
using namespace Moby;
using boost::shared_ptr; 
using std::map;
using std::vector;
using std::list;
using std::pair;
using boost::dynamic_pointer_cast;
using boost::const_pointer_cast;
using std::make_pair;

/// Initializes the heightmap primitive
PlanePrimitive::PlanePrimitive()
{
}

/// Initializes the heightmap primitive
PlanePrimitive::PlanePrimitive(const Ravelin::Pose3d& T) : Primitive(T)
{
}

/// Gets the mesh of the heightmap
shared_ptr<const IndexedTriArray> PlanePrimitive::get_mesh(shared_ptr<const Pose3d> P)
{
  // TODO: not sure whether implementation is necesary 
  assert(false);
  return shared_ptr<const IndexedTriArray>();
}

/// Transforms the primitive
void PlanePrimitive::set_pose(const Pose3d& p)
{
  // convert p to a shared pointer
  shared_ptr<Pose3d> x(new Pose3d(p));

  // determine the transformation from the old pose to the new one 
  Transform3d T = Pose3d::calc_relative_pose(_F, x);

  // go ahead and set the new transform
  Primitive::set_pose(p);
}

/// Gets the BVH root for the heightmap
BVPtr PlanePrimitive::get_BVH_root(CollisionGeometryPtr geom)
{
  const unsigned X = 0, Y = 1, Z = 2;
  const double SZ = 1e+2;

  // get the pointer to the geometry
  shared_ptr<OBB>& obb = _obbs[geom];

  // if the OBB hasn't been created, create it and initialize it
  if (!obb)
  {
    // create the OBB 
    obb = shared_ptr<OBB>(new OBB);
    obb->geom = geom;

    // get the pose for the geometry
    shared_ptr<const Pose3d> P = get_pose(geom);

    // setup the rotation matrix for the OBB
    obb->R.set_identity();

    // setup the translation for the OBB
    obb->center.set_zero(P);
    obb->center[Y] = -SZ*0.5;

    // setup obb dimensions
    obb->l.pose = P;
    obb->l[X] = SZ;
    obb->l[Y] = SZ;
    obb->l[Z] = SZ;
  }

  return obb;
}

/// Gets the vertices of the heightmap that could intersect with a given bounding volume
void PlanePrimitive::get_vertices(BVPtr bv, shared_ptr<const Pose3d> P, vector<Point3d>& vertices) const
{
  assert(_poses.find(const_pointer_cast<Pose3d>(P)) != _poses.end());
  const unsigned X = 0, Z = 2;

  // clear the vector of vertices
  vertices.clear();

  // get the bounding volume
  OBBPtr obb = dynamic_pointer_cast<OBB>(bv);
  obb->get_vertices(std::back_inserter(vertices));
}

/// Gets the vertices of the heightmap
void PlanePrimitive::get_vertices(shared_ptr<const Pose3d> P, vector<Point3d>& vertices) const
{
  const unsigned X = 0, Y = 1, Z = 2;
  const double SZ = 1e+2;
  assert(_poses.find(const_pointer_cast<Pose3d>(P)) != _poses.end() || P == get_pose());

  // clear the vector of vertices
  vertices.clear();

  // create an OBB
  OBB obb;

  // setup the rotation matrix for the OBB
  obb.R.set_identity();

  // setup the translation for the OBB
  obb.center.set_zero(P);
  obb.center[Y] = -SZ*0.5;

  // setup obb dimensions
  obb.l.pose = P;
  obb.l[X] = SZ;
  obb.l[Y] = SZ;
  obb.l[Z] = SZ;

  // get the vertices
  obb.get_vertices(std::back_inserter(vertices));
}

/// Computes the height at a particular point
double PlanePrimitive::calc_height(const Point3d& p) const
{
  assert(_poses.find(const_pointer_cast<Pose3d>(p.pose)) != _poses.end());
  const unsigned X = 0, Y = 1, Z = 2;

  return p[Y];
}

/// Computes the OSG visualization
osg::Node* PlanePrimitive::create_visualization()
{
  #ifdef USE_OSG
  const unsigned X = 0, Y = 1, Z = 2;

  // get the pose and compute transform from the global frame to it 
  shared_ptr<const Pose3d> P = get_pose();
  Transform3d T = Pose3d::calc_relative_pose(P, GLOBAL);

  // create necessary OSG elements for visualization
  osg::Group* group = new osg::Group;

  // create the necessary osg mechanicals
  osg::Group* subgroup = new osg::Group;
  osg::Geode* geode = new osg::Geode;
  subgroup->addChild(geode);
  group->addChild(subgroup); 

  // create a new material with random color
  const float RED = (float) rand() / RAND_MAX;
  const float GREEN = (float) rand() / RAND_MAX;
  const float BLUE = (float) rand() / RAND_MAX;
  osg::Material* mat = new osg::Material;
  mat->setColorMode(osg::Material::DIFFUSE);
  mat->setDiffuse(osg::Material::FRONT, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
  subgroup->getOrCreateStateSet()->setAttribute(mat);

  // create the plane
  osg::InfinitePlane* plane = new osg::InfinitePlane;
  plane->set(0.0, 1.0, 0.0, 0.0);
  geode->addDrawable(new osg::ShapeDrawable(plane));

  return group;
  #else
  return NULL;
  #endif 
}

/// Computes the signed distance of the given point from this primitive
double PlanePrimitive::calc_signed_dist(const Point3d& p) const
{
  assert(_poses.find(const_pointer_cast<Pose3d>(p.pose)) != _poses.end());

  // compute the height
  return calc_height(p);
}

/// Finds the signed distance between the heightmap and another primitive
double PlanePrimitive::calc_signed_dist(shared_ptr<const Primitive> p, Point3d& pthis, Point3d& pp) const
{
  const unsigned Y = 1;

  // if the primitive is convex, can use GJK
  if (p->is_convex())
  {
    shared_ptr<const Pose3d> Pplane = pthis.pose;
    shared_ptr<const Pose3d> Pgeneric = pp.pose;
    shared_ptr<const Primitive> prim_this = dynamic_pointer_cast<const Primitive>(shared_from_this());
    return GJK::do_gjk(prim_this, p, Pplane, Pgeneric, pthis, pp);
  }

  // get p as non-const
  shared_ptr<Primitive> pnc = const_pointer_cast<Primitive>(p);

  // still here? no specialization; get all vertices from other primitive
  vector<Point3d> verts;
  pnc->get_vertices(pp.pose, verts);
  double mindist = std::numeric_limits<double>::max();
  for (unsigned i=0; i< verts.size(); i++)
  {
     Point3d pt = Pose3d::transform_point(pthis.pose, verts[i]);
     const double HEIGHT = calc_height(pt);
     if (HEIGHT < mindist)
     {
       mindist = HEIGHT;
       pp = verts[i];
       pthis = pt;
       pthis[Y] = -(HEIGHT - pt[Y]);
     }
  }

  return mindist;
}

/// Finds the signed distance betwen the plane and a point
double PlanePrimitive::calc_dist_and_normal(const Point3d& p, Vector3d& normal) const
{
  // compute the distance
  double d = calc_height(p);

  // setup the normal
  normal = Vector3d(0.0, 1.0, 0.0, p.pose);

  // compute the distance
  return d;
}

/// Implements Base::load_from_xml() for serialization
void PlanePrimitive::load_from_xml(shared_ptr<const XMLTree> node, std::map<std::string, BasePtr>& id_map)
{
  // verify that the node type is heightmap
  assert(strcasecmp(node->name.c_str(), "Plane") == 0);

  // load the parent data
  Primitive::load_from_xml(node, id_map);
}

/// Implements Base::save_to_xml() for serialization
void PlanePrimitive::save_to_xml(XMLTreePtr node, std::list<shared_ptr<const Base> >& shared_objects) const
{
  // save the parent data
  Primitive::save_to_xml(node, shared_objects);

  // (re)set the node name
  node->name = "Plane";
}


