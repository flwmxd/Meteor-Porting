

using Maple;

public class Test : Maple.MapleScript
{
    public override void OnUpdate(float dt)
    {
        if (physicsObject != null)
        {
            if (Input.IsKeyPressed(KeyCode.W))
            {
                physicsObject.SetVelocity(new Vector3(5, 0, 0));
            }
            else
            {
                physicsObject.SetVelocity(new Vector3(0, 10, 0));
            }
        }
    }
    public override void OnStart()
    {

    }
};