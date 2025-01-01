#include "SensorSimulation.h"

void simulateSensors(DroneData& drone)
{
  if (!drone.body.fuselageBody) return;

  // Read Bullet transform
  btTransform transform;
  drone.body.fuselageBody->getMotionState()->getWorldTransform(transform);
  btVector3 pos = transform.getOrigin();

  // Altimeter (just y-pos)
  drone.sensors.sensedAltitude = pos.y();

  // Orientation (pitch, roll, yaw)
  // For simplicity, direct from bullet basis
  btScalar yaw, pitch, roll;
  transform.getBasis().getEulerZYX(yaw, pitch, roll);
  drone.sensors.sensedPitch = pitch;
  drone.sensors.sensedRoll  = roll;
  drone.sensors.sensedYaw   = yaw;

  // Add some noise
  static std::default_random_engine rng;
  static std::normal_distribution<float> noiseDist(0.0f, 0.01f);

  drone.sensors.sensedAltitude += noiseDist(rng);
  drone.sensors.sensedPitch    += noiseDist(rng) * 0.05f;
  drone.sensors.sensedRoll     += noiseDist(rng) * 0.05f;
  drone.sensors.sensedYaw      += noiseDist(rng) * 0.05f;
}