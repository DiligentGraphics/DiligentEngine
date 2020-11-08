#pragma once


namespace Diligent
{

class Actor;

class Component
{
public:
    Component(Actor* ownerP, int updateOrderP = 100);
    Component() = delete;
    virtual ~Component();
    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;

    int getUpdateOrder() const { return updateOrder; }

    virtual void update(double CurrTime, double ElapsedTime);
    virtual void onUpdateWorldTransform() {}

protected:
    Actor& owner;
    int    updateOrder; // Order of the component in the actor's updateComponent method
};

} // namespace Diligent
