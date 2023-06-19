using Flax.Build;

public class OnlinePlatformEOSTarget : GameProjectTarget
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        // Reference the modules for game
        Modules.Add("OnlinePlatformEOS");
    }
}
