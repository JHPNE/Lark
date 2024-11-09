using DrosimEditor.Components;
using DrosimEditor.DllWrapper;
using DrosimEditor.SimDev;
using DrosimEditor.Utils;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;

namespace DrosimEditor.SimProject
{
    
    [DataContract(Name = "Game")]
    class Project : ViewModelBase
    {
        public static string Extension => ".drosim";
        [DataMember]
        public string Name { get; private set; } = "New Project";

        [DataMember]
        public string Path { get; private set; }

        public string FullPath => $@"{Path}{Name}{Extension}";
        public string Solution => $@"{Path}{Name}.sln";
        public string TempFolder => $@"{Path}.Drosim\Temp\";


        private int _buildConfig;
        [DataMember]
        public int BuildConfig
        {
            get => _buildConfig;
            set
            {
                if (_buildConfig != value)
                {
                    _buildConfig = value;
                    OnPropertyChanged(nameof(BuildConfig));
                }
            }
        }

        public BuildConfiguration DllBuildConfig => BuildConfig ==  0 ? BuildConfiguration.DebugEditor : BuildConfiguration.ReleaseEditor;

        private string[] _availableScripts;
        public string[] AvailableScripts
        {
            get => _availableScripts;
            private set
            {
                if (_availableScripts != value)
                {
                    _availableScripts = value;
                    OnPropertyChanged(nameof(AvailableScripts));
                }
            }
        }


        [DataMember(Name = nameof(Scenes))]
        private readonly ObservableCollection<Scene> _scenes = new ObservableCollection<Scene>();
        public ReadOnlyObservableCollection<Scene> Scenes { get; private set; }

        private Scene _activeScene;
        public Scene ActiveScene
        {
            get => _activeScene;
            set
            {
                if (_activeScene != value)
                {
                    _activeScene = value;
                    OnPropertyChanged(nameof(ActiveScene));
                }
            }
        }
        public static Project Current => Application.Current.MainWindow?.DataContext as Project; 
        public static UndoRedo UndoRedo { get; } = new UndoRedo();
        public ICommand UndoCommand {  get; private set; }
        public ICommand RedoCommand {  get; private set; }
        public ICommand AddSceneCommand {  get; private set; }
        public ICommand RemoveSceneCommand {  get; private set; }
        public ICommand SaveCommand {  get; private set; }
        public ICommand BuildCommand { get; private set; }
        public ICommand DebugRunCommand { get; private set; }
        public ICommand DebugRunWithoutDebuggingCommand { get; private set; }
        public ICommand DebugStopCommand { get; private set; }

        public Project(string name, string path) 
        {
            Name = name;
            Path = path;

            OnDeserialized(new StreamingContext());
        }

        private void SetCommands()
        {
            AddSceneCommand = new RelayCommands<object>(x =>
            {
                AddScene($"New Scene {_scenes.Count}");
                var newScene = _scenes.Last();
                var sceneIndex = _scenes.Count - 1;

                UndoRedo.Add(new UndoRedoAction(
                    () => RemoveScene(newScene),
                    () => _scenes.Insert(sceneIndex, newScene),
                    $"Add {newScene.Name}"));
            });

            RemoveSceneCommand = new RelayCommands<Scene>(x =>
            {
                var sceneIndex = _scenes.IndexOf(x);
                RemoveScene(x);

                UndoRedo.Add(new UndoRedoAction(
                    () => _scenes.Insert(sceneIndex, x), 
                    () => RemoveScene(x),
                    $"Remove {x.Name}"));
            }, x => !x.IsActive);

            UndoCommand = new RelayCommands<object>(x => UndoRedo.Undo(), x => UndoRedo.UndoList.Any());
            RedoCommand = new RelayCommands<object>(x => UndoRedo.Redo(), x => UndoRedo.RedoList.Any());
            SaveCommand = new RelayCommands<object>(x => Save(this));
            DebugRunCommand = new RelayCommands<object>(async x => await RunSim(true), x=> !VisualStudio.IsDebugging() && VisualStudio.BuildDone);
            DebugRunWithoutDebuggingCommand = new RelayCommands<object>(async x => await RunSim(false), x=> !VisualStudio.IsDebugging() && VisualStudio.BuildDone);
            DebugStopCommand = new RelayCommands<object>(async x => await StopSim(), x=> VisualStudio.IsDebugging());
            // Replace this with Simulation
            BuildCommand = new RelayCommands<bool>(async x => await BuildGameCodeDll(x), x => !VisualStudio.IsDebugging() && VisualStudio.BuildDone);

            OnPropertyChanged(nameof(AddSceneCommand));
            OnPropertyChanged(nameof(RemoveSceneCommand));
            OnPropertyChanged(nameof(UndoCommand));
            OnPropertyChanged(nameof(RedoCommand));
            OnPropertyChanged(nameof(SaveCommand));
            OnPropertyChanged(nameof(DebugRunCommand));
            OnPropertyChanged(nameof(DebugRunWithoutDebuggingCommand));
            OnPropertyChanged(nameof(DebugStopCommand));
            OnPropertyChanged(nameof(BuildCommand));
        }

        public void Unload()
        {
            UnloadGameCodeDll();
            VisualStudio.CloseVisualStudio();
            UndoRedo.Reset();
            Logger.Clear();
            DeleteTempFolder();
        }

        private void DeleteTempFolder()
        {
            if (Directory.Exists(TempFolder))
            {
                Directory.Delete(TempFolder, true);
            }
        }

        public static Project Load(string file)
        {
            Debug.Assert(File.Exists(file));
            return Serializer.FromFile<Project>(file);
        }
        private static void Save(Project project)
        {
            Serializer.ToFile(project, project.FullPath);
            Logger.Log(MessageType.Info, $"Saved project to {project.FullPath}");
        }

        private async Task RunSim(bool debug)
        {
            await Task.Run(() => VisualStudio.BuildSolution(this, DllBuildConfig, debug));
            if (VisualStudio.BuildSucceeded)
            {
                await Task.Run(() => VisualStudio.Run(this, DllBuildConfig, debug));
            }
        }

        private async Task StopSim() => await Task.Run(() => VisualStudio.Stop());

        private async Task BuildGameCodeDll(bool showWindow = true)
        {
            try
            {
                UnloadGameCodeDll();
                await Task.Run(() => VisualStudio.BuildSolution(this, DllBuildConfig, showWindow));

                if (VisualStudio.BuildSucceeded)
                {
                    LoadGameCodeDll();
                }
            }
            catch (Exception ex)
            { 
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to build game code dll");
            }
            
        }

        private void LoadGameCodeDll()
        {
            var configName = VisualStudio.GetConfigurationName(DllBuildConfig);
            var dll = $@"{Path}x64\{configName}\{Name}.dll";
            AvailableScripts = null;

            if (File.Exists(dll) && EngineAPI.LoadGameCodeDll(dll) != 0)
            {
                AvailableScripts = EngineAPI.GetScriptNames();

                ActiveScene.GameEntities.Where(x => x.GetComponent<Script>() != null).ToList().ForEach(x => x.IsActive = true);
                Logger.Log(MessageType.Info, "Game code DLL loaded succesfully");
            }
            else
            {
                Logger.Log(MessageType.Warn, "Failed to load game code DLL. Try to build the project first");
            }
        }

        private void UnloadGameCodeDll()
        {
            ActiveScene.GameEntities.Where(x => x.GetComponent<Script>() != null).ToList().ForEach(x => x.IsActive = false);
            if(EngineAPI.UnloadGameCodeDll() != 0)
            {
                Logger.Log(MessageType.Info, "Game code DLL unloaded successfully");
                AvailableScripts = null;
            }
        }

        [OnDeserialized]
        private async void OnDeserialized(StreamingContext context)
        {
            if (_scenes != null)
            {
                Scenes = new ReadOnlyObservableCollection<Scene>(_scenes);
                OnPropertyChanged(nameof(Scenes));
            }

            ActiveScene = Scenes.FirstOrDefault(x => x.IsActive);
            Debug.Assert(ActiveScene != null);

            await BuildGameCodeDll(false);

            SetCommands();
        }

        private void AddScene(string sceneName)
        {
            Debug.Assert(!string.IsNullOrEmpty(sceneName.Trim()));
            _scenes.Add(new Scene(this, sceneName));
        }

        private void RemoveScene(Scene scene) 
        {
            Debug.Assert(_scenes.Contains(scene));
            _scenes.Remove(scene);
        }
    }
}
