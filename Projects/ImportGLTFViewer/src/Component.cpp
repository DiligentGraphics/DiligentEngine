#include "Component.h"
#include "Actor.h"

namespace Diligent
{

Component::Component(Actor* ownerP, int updateOrderP) :
    owner(*ownerP),
    updateOrder(updateOrderP)
{
    owner.addComponent(this);
}

Component::~Component()
{
    owner.removeComponent(this);
}

void Component::update(double CurrTime, double ElapsedTime)
{
}

} // namespace Diligent