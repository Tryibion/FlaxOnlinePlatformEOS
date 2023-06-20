# EOS for Flax Engine

This repository contains a plugin project for [Flax Engine](https://flaxengine.com/) games with [EOS](https://dev.epicgames.com/en-US/services) online platform implementation that covers: user profile, friends list, online presence, achevements, cloud savegames and more.

Minimum supported Flax version: `1.3`.

## Installation

1. Clone repo into `<game-project>\Plugins\OnlinePlatformEOS`

2. Add reference to *OnlinePlatformEOS* project in your game by modyfying `<game-project>.flaxproj` as follows:

```
...
"References": [
    {
        "Name": "$(EnginePath)/Flax.flaxproj"
    },
    {
        "Name": "$(ProjectPath)/Plugins/OnlinePlatformEOS/OnlinePlatformEOS.flaxproj"
    }
]
```

3. Add reference to the EOS plugin module in you game code module by modyfying `Source/Game/Game.Build.cs` as follows (or any other game modules using Online):

```cs
/// <inheritdoc />
public override void Setup(BuildOptions options)
{
    base.Setup(options);

    ...

    switch (options.Platform.Target)
    {
    case TargetPlatform.Windows:
    case TargetPlatform.Linux:
    case TargetPlatform.Mac:
        options.PublicDependencies.Add("OnlinePlatformEOS");
        break;
    }
}
```

This will add reference to `OnlinePlatformEOS` module on Windows/Linux/Mac platforms that are supported by Steam.

4. Test it out!

Finally you can use EOS as online platform in your game:

```cs
// C#
using FlaxEngine.Online;
using FlaxEngine.Online.EOS;

var platform = platform = new OnlinePlatformEOS();
Online.Initialize(platform);
```

```cpp
// C++
#include "Engine/Online/Online.h"
#include "OnlinePlatformEOS/OnlinePlatformEOS.h"

auto platform = New<OnlinePlatformEOS>();
Online::Initialize(platform);
```

Then use [Online](https://docs.flaxengine.com/manual/networking/online/index.html) system to access online platform (user profile, friends, achievements, cloud saves, etc.). 

5. Setup settings



## License

This plugin ais released under **MIT License**.