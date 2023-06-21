
#if FLAX_EDITOR

using System;
using FlaxEditor;
using FlaxEditor.Content;
using FlaxEngine;
using FlaxEngine.Online.EOS;

namespace OnlinePlatformEOS
{
    /// <summary>
    /// OnlinePlatformEOS Script.
    /// </summary>
    public class EOSEditorPlugin : EditorPlugin
    {
        private AssetProxy _assetProxy;

        /// <summary>
        /// Initializes a new instance of the <see cref="EOSEditorPlugin"/> class.
        /// </summary>
        public EOSEditorPlugin()
        {
            _description = new PluginDescription
            {
                Name = "EOS",
                Category = "Online",
                Description = "Online platform implementation for EOS.",
                Author = "Flax & Tryibion",
                RepositoryUrl = "",
                Version = new Version(1, 0),
            };
        }

        /// <inheritdoc />
        public override void InitializeEditor()
        {
            base.InitializeEditor();

            //GameCooker.DeployFiles += OnDeployFiles;
            _assetProxy = new CustomSettingsProxy(typeof(EOSSettings), "EOS");
            Editor.ContentDatabase.Proxy.Add(_assetProxy);
        }

        /// <inheritdoc />
        public override void Deinitialize()
        {
            Editor.ContentDatabase.Proxy.Remove(_assetProxy);
            _assetProxy = null;
            //GameCooker.DeployFiles -= OnDeployFiles;

            base.Deinitialize();
        }
        
        /* TODO: may need for EOS Steam integration
        private void OnDeployFiles()
        {
            // Include steam_appid.txt file with a game
            var data = GameCooker.CurrentData;
            var settingsAsset = Engine.GetCustomSettings("EOS");
            var settings = settingsAsset?.CreateInstance<EOSSettings>();
            var appId = settings?.AppId ?? 480;
            switch (data.Platform)
            {
                case BuildPlatform.Windows32:
                case BuildPlatform.Windows64:
                case BuildPlatform.LinuxX64:
                case BuildPlatform.MacOSx64:
                    File.WriteAllText(Path.Combine(data.NativeCodeOutputPath, "steam_appid.txt"), appId.ToString());
                    break;
            }
        }
        */
    }
}
#endif
