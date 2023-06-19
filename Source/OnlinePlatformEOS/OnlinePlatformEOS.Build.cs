using Flax.Build;
using Flax.Build.NativeCpp;

public class OnlinePlatformEOS : GameModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        BuildNativeCode = true;
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.ScriptingAPI.IgnoreMissingDocumentationWarnings = true;

        options.PublicDependencies.Add("Online");
        options.PrivateDependencies.Add("EOSSDK");
    }
}
