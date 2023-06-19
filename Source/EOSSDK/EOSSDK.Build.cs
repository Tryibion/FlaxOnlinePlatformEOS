// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// EOS SDK
/// </summary>
public class EOSSDK : DepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Custom;
        LicenseFilePath = "license";
        BinaryModuleName = null;
        BuildNativeCode = false;
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var binariesFolder = Path.Combine(FolderPath, "Bin");
        var libraryFolder = Path.Combine(FolderPath, "Lib");

        //TODO: Add android, IOS, and consoles
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            options.OutputFiles.Add(Path.Combine(libraryFolder, "EOSSDK-Win64-Shipping.lib"));
            options.DependencyFiles.Add(Path.Combine(binariesFolder, "EOSSDK-Win64-Shipping.dll"));
            options.DelayLoadLibraries.Add("EOSSDK-Win64-Shipping.dll");
            break;
        case TargetPlatform.Linux:
            options.DependencyFiles.Add(Path.Combine(binariesFolder, "libEOSSDK-Linux-Shipping.so"));
            options.Libraries.Add(Path.Combine(binariesFolder, "llibEOSSDK-Linux-Shipping.so"));
            break;
        case TargetPlatform.Mac:
            options.DependencyFiles.Add(Path.Combine(binariesFolder, "libEOSSDK-Mac-Shipping.dylib"));
            options.Libraries.Add(Path.Combine(binariesFolder, "libEOSSDK-Mac-Shipping.dylib"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }
}
